/**
 * bridge_scanner.c
 * RAW DISK VIEWER CPLX — JNI Bridge: Scanner, Carver, Analysis, Export
 *
 * Implements:
 *   - runScan()
 *   - runCarver()
 *   - getSectorEntropy()
 *   - exportSectors()
 *   - exportCarvedFile()
 */

#include <jni.h>
#include <string.h>
#include <stdlib.h>

#include "../../core/include/rdvc_disk.h"
#include "../../core/include/rdvc_signatures.h"
#include "../../core/include/rdvc_analysis.h"
#include "../../core/include/rdvc_carver.h"
#include "../../core/include/rdvc_errors.h"
#include "../../core/include/rdvc_platform.h"

#define CLS_SCAN_RESULT  "com/rdvc/bridge/model/ScanResult"
#define CLS_CARVED_FILE  "com/rdvc/bridge/model/CarvedFile"

#define MAX_SCAN_RESULTS  65536
#define MAX_CARVED_FILES  4096
#define SCAN_BATCH_SIZE   128

/* ─────────────────────────────────────────────────────────────
 *  runScan()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jobjectArray JNICALL
Java_com_rdvc_bridge_NativeBridge_runScan(JNIEnv *env, jclass clazz,
                                           jlong handle,
                                           jlong startLba, jlong endLba)
{
    (void)clazz;
    if (handle <= 0) return NULL;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;

    rdvc_u64 end_lba = (rdvc_u64)endLba;
    if (end_lba == 0 || end_lba >= ctx->info.total_sectors)
        end_lba = ctx->info.total_sectors > 0 ? ctx->info.total_sectors - 1 : 0;

    rdvc_sig_match_t *matches = (rdvc_sig_match_t *)calloc(
        MAX_SCAN_RESULTS, sizeof(rdvc_sig_match_t));
    if (!matches) return NULL;
    rdvc_u32 total_matches = 0;

    rdvc_u32 ssz = ctx->info.sector_size;
    rdvc_u8 *buf = (rdvc_u8 *)malloc((rdvc_u64)SCAN_BATCH_SIZE * ssz);
    if (!buf) { free(matches); return NULL; }

    rdvc_u64 lba = (rdvc_u64)startLba;
    while (lba <= end_lba && total_matches < MAX_SCAN_RESULTS) {
        rdvc_u64 remaining = end_lba - lba + 1;
        rdvc_u32 batch = (rdvc_u32)(remaining < SCAN_BATCH_SIZE ? remaining : SCAN_BATCH_SIZE);

        rdvc_u32 read_count = 0;
        rdvc_err_t e = rdvc_read_sectors(ctx, lba, batch, buf, &read_count);
        if (RDVC_IS_ERR(e) || read_count == 0) break;

        rdvc_u32 batch_matches = 0;
        rdvc_u32 slots = MAX_SCAN_RESULTS - total_matches;
        rdvc_scan_buffer(buf, (rdvc_u64)read_count * ssz, lba * ssz, ssz,
                         matches + total_matches, slots, &batch_matches);
        total_matches += batch_matches;
        lba += read_count;
    }
    free(buf);

    /* Build Java ScanResult[] */
    jclass cls = (*env)->FindClass(env, CLS_SCAN_RESULT);
    if (!cls) { free(matches); return NULL; }

    /* Constructor: (long byteOffset, long lba, int offsetInSector,
                     String extension, String mimeType,
                     String description, long estimatedMaxSize) */
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(JJILjava/lang/String;Ljava/lang/String;Ljava/lang/String;J)V");
    if (!ctor) { free(matches); return NULL; }

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)total_matches, cls, NULL);
    if (!arr) { free(matches); return NULL; }

    for (rdvc_u32 i = 0; i < total_matches; i++) {
        const rdvc_sig_match_t *m = &matches[i];
        const rdvc_signature_t *s = m->sig;

        jstring ext  = (*env)->NewStringUTF(env, s->ext);
        jstring mime = (*env)->NewStringUTF(env, s->mime);
        jstring desc = (*env)->NewStringUTF(env, s->desc);

        jobject obj = (*env)->NewObject(env, cls, ctor,
            (jlong)m->byte_offset,
            (jlong)m->lba,
            (jint) m->offset_in_sector,
            ext, mime, desc,
            (jlong)s->max_size
        );

        (*env)->DeleteLocalRef(env, ext);
        (*env)->DeleteLocalRef(env, mime);
        (*env)->DeleteLocalRef(env, desc);

        if (obj) {
            (*env)->SetObjectArrayElement(env, arr, (jsize)i, obj);
            (*env)->DeleteLocalRef(env, obj);
        }
    }

    free(matches);
    return arr;
}

/* ─────────────────────────────────────────────────────────────
 *  runCarver()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jobjectArray JNICALL
Java_com_rdvc_bridge_NativeBridge_runCarver(JNIEnv *env, jclass clazz,
                                             jlong handle,
                                             jlong startLba, jlong endLba)
{
    (void)clazz;
    if (handle <= 0) return NULL;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;

    rdvc_carved_file_t *files = (rdvc_carved_file_t *)calloc(
        MAX_CARVED_FILES, sizeof(rdvc_carved_file_t));
    if (!files) return NULL;

    rdvc_carver_config_t config;
    memset(&config, 0, sizeof(config));
    config.start_lba   = (rdvc_u64)startLba;
    config.end_lba     = (rdvc_u64)endLba;
    config.read_batch  = 64;
    config.skip_empty  = RDVC_TRUE;
    config.progress_cb = NULL;

    rdvc_u32 found = 0;
    rdvc_carve_disk(ctx, &config, files, MAX_CARVED_FILES, &found);

    /* Build Java CarvedFile[] */
    jclass cls = (*env)->FindClass(env, CLS_CARVED_FILE);
    if (!cls) { free(files); return NULL; }

    /* Constructor: (long startOffset, long endOffset, long size, long startLba,
                     String extension, String mimeType,
                     String description, String suggestedName) */
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(JJJJLjava/lang/String;Ljava/lang/String;"
        "Ljava/lang/String;Ljava/lang/String;)V");
    if (!ctor) { free(files); return NULL; }

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)found, cls, NULL);
    if (!arr) { free(files); return NULL; }

    for (rdvc_u32 i = 0; i < found; i++) {
        const rdvc_carved_file_t *f = &files[i];
        const rdvc_signature_t   *s = f->sig;

        jstring ext  = (*env)->NewStringUTF(env, s ? s->ext  : "bin");
        jstring mime = (*env)->NewStringUTF(env, s ? s->mime : "application/octet-stream");
        jstring desc = (*env)->NewStringUTF(env, s ? s->desc : "Unknown");
        jstring name = (*env)->NewStringUTF(env, f->suggested_name);

        jobject obj = (*env)->NewObject(env, cls, ctor,
            (jlong)f->start_offset,
            (jlong)f->end_offset,
            (jlong)f->size,
            (jlong)f->start_lba,
            ext, mime, desc, name
        );

        (*env)->DeleteLocalRef(env, ext);
        (*env)->DeleteLocalRef(env, mime);
        (*env)->DeleteLocalRef(env, desc);
        (*env)->DeleteLocalRef(env, name);

        if (obj) {
            (*env)->SetObjectArrayElement(env, arr, (jsize)i, obj);
            (*env)->DeleteLocalRef(env, obj);
        }
    }

    free(files);
    return arr;
}

/* ─────────────────────────────────────────────────────────────
 *  getSectorEntropy()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jdouble JNICALL
Java_com_rdvc_bridge_NativeBridge_getSectorEntropy(JNIEnv *env, jclass clazz,
                                                    jlong handle, jlong lba)
{
    (void)env; (void)clazz;
    if (handle <= 0) return -1.0;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    rdvc_u32 ssz = ctx->info.sector_size;
    if (ssz == 0) ssz = RDVC_SECTOR_SIZE_512;

    rdvc_u8 *buf = (rdvc_u8 *)malloc(ssz);
    if (!buf) return -1.0;

    rdvc_err_t e = rdvc_read_sector(ctx, (rdvc_u64)lba, buf);
    if (RDVC_IS_ERR(e)) { free(buf); return -1.0; }

    double entropy = rdvc_compute_entropy(buf, ssz);
    free(buf);
    return (jdouble)entropy;
}

/* ─────────────────────────────────────────────────────────────
 *  exportSectors()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jint JNICALL
Java_com_rdvc_bridge_NativeBridge_exportSectors(JNIEnv *env, jclass clazz,
                                                 jlong handle, jlong lbaStart,
                                                 jint count, jstring outputPath)
{
    (void)clazz;
    if (handle <= 0 || !outputPath) return (jint)RDVC_ERR_NULL_POINTER;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    const char *path = (*env)->GetStringUTFChars(env, outputPath, NULL);
    if (!path) return (jint)RDVC_ERR_NULL_POINTER;

    rdvc_err_t e = rdvc_export_sectors(ctx, (rdvc_u64)lbaStart,
                                        (rdvc_u32)count, path);
    (*env)->ReleaseStringUTFChars(env, outputPath, path);
    return (jint)e;
}

/* ─────────────────────────────────────────────────────────────
 *  exportCarvedFile()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jint JNICALL
Java_com_rdvc_bridge_NativeBridge_exportCarvedFile(JNIEnv *env, jclass clazz,
                                                    jlong handle,
                                                    jlong startOffset, jlong size,
                                                    jstring outputPath)
{
    (void)clazz;
    if (handle <= 0 || !outputPath) return (jint)RDVC_ERR_NULL_POINTER;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    const char *path = (*env)->GetStringUTFChars(env, outputPath, NULL);
    if (!path) return (jint)RDVC_ERR_NULL_POINTER;

    /* Build a temporary carved_file_t */
    rdvc_carved_file_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.start_offset = (rdvc_u64)startOffset;
    tmp.end_offset   = (rdvc_u64)(startOffset + size - 1);
    tmp.size         = (rdvc_u64)size;
    tmp.start_lba    = (rdvc_u64)startOffset / ctx->info.sector_size;
    tmp.sig          = NULL;

    rdvc_err_t e = rdvc_export_carved(ctx, &tmp, path);
    (*env)->ReleaseStringUTFChars(env, outputPath, path);
    return (jint)e;
}
