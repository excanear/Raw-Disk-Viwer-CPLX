/**
 * bridge_disk.c
 * RAW DISK VIEWER CPLX — JNI Bridge: Disk operations
 *
 * Implements Java native methods declared in NativeBridge.java:
 *   - listDisks()
 *   - openDisk()
 *   - closeDisk()
 *   - readSector()
 *   - readSectors()
 *   - getDiskInfo()
 *   - getErrorString()
 */

#include <jni.h>
#include <string.h>
#include <stdlib.h>

#include "../../core/include/rdvc_disk.h"
#include "../../core/include/rdvc_errors.h"
#include "../../core/include/rdvc_platform.h"

/* JNI class paths */
#define CLS_DISK_INFO   "com/rdvc/bridge/model/DiskInfo"
#define CLS_NATIVE_BRIDGE "com/rdvc/bridge/NativeBridge"

/* ─────────────────────────────────────────────────────────────
 *  Helper: build a DiskInfo Java object from rdvc_disk_info_t
 * ───────────────────────────────────────────────────────────── */
static jobject build_disk_info(JNIEnv *env, const rdvc_disk_info_t *info)
{
    jclass cls = (*env)->FindClass(env, CLS_DISK_INFO);
    if (!cls) return NULL;

    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(Ljava/lang/String;Ljava/lang/String;JIJII)V");
    if (!ctor) return NULL;

    jstring path  = (*env)->NewStringUTF(env, info->path);
    jstring model = (*env)->NewStringUTF(env, info->model);

    jobject obj = (*env)->NewObject(env, cls, ctor,
        path,
        model,
        (jlong)info->total_bytes,
        (jint) info->sector_size,
        (jlong)info->total_sectors,
        (jint) info->type,
        (jint) info->index
    );

    (*env)->DeleteLocalRef(env, path);
    (*env)->DeleteLocalRef(env, model);
    return obj;
}

/* ─────────────────────────────────────────────────────────────
 *  listDisks()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jobjectArray JNICALL
Java_com_rdvc_bridge_NativeBridge_listDisks(JNIEnv *env, jclass clazz)
{
    (void)clazz;
    rdvc_disk_info_t disk_list[RDVC_MAX_DISKS];
    rdvc_u32 found = 0;

    rdvc_list_disks(disk_list, RDVC_MAX_DISKS, &found);

    jclass cls = (*env)->FindClass(env, CLS_DISK_INFO);
    if (!cls) return NULL;

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)found, cls, NULL);
    if (!arr) return NULL;

    for (rdvc_u32 i = 0; i < found; i++) {
        jobject info_obj = build_disk_info(env, &disk_list[i]);
        if (info_obj) {
            (*env)->SetObjectArrayElement(env, arr, (jsize)i, info_obj);
            (*env)->DeleteLocalRef(env, info_obj);
        }
    }

    return arr;
}

/* ─────────────────────────────────────────────────────────────
 *  openDisk() — returns handle as jlong (pointer to heap-alloc ctx)
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jlong JNICALL
Java_com_rdvc_bridge_NativeBridge_openDisk(JNIEnv *env, jclass clazz, jstring path)
{
    (void)clazz;
    if (!path) return (jlong)-RDVC_ERR_NULL_POINTER;

    const char *c_path = (*env)->GetStringUTFChars(env, path, NULL);
    if (!c_path) return (jlong)-RDVC_ERR_NULL_POINTER;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)calloc(1, sizeof(rdvc_disk_ctx_t));
    if (!ctx) {
        (*env)->ReleaseStringUTFChars(env, path, c_path);
        return (jlong)-RDVC_ERR_OUT_OF_MEMORY;
    }

    rdvc_err_t e = rdvc_open_disk(c_path, ctx);
    (*env)->ReleaseStringUTFChars(env, path, c_path);

    if (RDVC_IS_ERR(e)) {
        free(ctx);
        return (jlong)(rdvc_i64)e;
    }

    return (jlong)(uintptr_t)ctx;
}

/* ─────────────────────────────────────────────────────────────
 *  closeDisk()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jint JNICALL
Java_com_rdvc_bridge_NativeBridge_closeDisk(JNIEnv *env, jclass clazz, jlong handle)
{
    (void)env; (void)clazz;
    if (handle <= 0) return (jint)RDVC_ERR_DISK_NOT_OPEN;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    rdvc_err_t e = rdvc_close_disk(ctx);
    free(ctx);
    return (jint)e;
}

/* ─────────────────────────────────────────────────────────────
 *  readSector()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jbyteArray JNICALL
Java_com_rdvc_bridge_NativeBridge_readSector(JNIEnv *env, jclass clazz,
                                              jlong handle, jlong lba)
{
    (void)clazz;
    if (handle <= 0) return NULL;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    rdvc_u32 ssz = ctx->info.sector_size;
    if (ssz == 0) ssz = RDVC_SECTOR_SIZE_512;

    rdvc_u8 *buf = (rdvc_u8 *)malloc(ssz);
    if (!buf) return NULL;

    rdvc_err_t e = rdvc_read_sector(ctx, (rdvc_u64)lba, buf);
    if (RDVC_IS_ERR(e)) { free(buf); return NULL; }

    jbyteArray arr = (*env)->NewByteArray(env, (jsize)ssz);
    if (arr) (*env)->SetByteArrayRegion(env, arr, 0, (jsize)ssz, (const jbyte *)buf);

    free(buf);
    return arr;
}

/* ─────────────────────────────────────────────────────────────
 *  readSectors()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jbyteArray JNICALL
Java_com_rdvc_bridge_NativeBridge_readSectors(JNIEnv *env, jclass clazz,
                                               jlong handle, jlong lba, jint count)
{
    (void)clazz;
    if (handle <= 0 || count <= 0) return NULL;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    rdvc_u32 ssz = ctx->info.sector_size;
    if (ssz == 0) ssz = RDVC_SECTOR_SIZE_512;

    rdvc_u64 buf_size = (rdvc_u64)count * ssz;
    rdvc_u8 *buf = (rdvc_u8 *)malloc(buf_size);
    if (!buf) return NULL;

    rdvc_u32 read_count = 0;
    rdvc_err_t e = rdvc_read_sectors(ctx, (rdvc_u64)lba, (rdvc_u32)count,
                                      buf, &read_count);
    if (RDVC_IS_ERR(e)) { free(buf); return NULL; }

    rdvc_u64 actual = (rdvc_u64)read_count * ssz;
    jbyteArray arr = (*env)->NewByteArray(env, (jsize)actual);
    if (arr) (*env)->SetByteArrayRegion(env, arr, 0, (jsize)actual, (const jbyte *)buf);

    free(buf);
    return arr;
}

/* ─────────────────────────────────────────────────────────────
 *  getDiskInfo()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jobject JNICALL
Java_com_rdvc_bridge_NativeBridge_getDiskInfo(JNIEnv *env, jclass clazz, jlong handle)
{
    (void)clazz;
    if (handle <= 0) return NULL;
    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    return build_disk_info(env, &ctx->info);
}

/* ─────────────────────────────────────────────────────────────
 *  getErrorString()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jstring JNICALL
Java_com_rdvc_bridge_NativeBridge_getErrorString(JNIEnv *env, jclass clazz, jint code)
{
    (void)clazz;
    const char *msg = rdvc_strerror((rdvc_err_t)code);
    return (*env)->NewStringUTF(env, msg);
}

/* ─────────────────────────────────────────────────────────────
 *  getRawMBR()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jbyteArray JNICALL
Java_com_rdvc_bridge_NativeBridge_getRawMBR(JNIEnv *env, jclass clazz, jlong handle)
{
    (void)clazz;
    if (handle <= 0) return NULL;
    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;

    rdvc_u8 sector[RDVC_SECTOR_SIZE_4096];
    rdvc_err_t e = rdvc_read_sector(ctx, 0, sector);
    if (RDVC_IS_ERR(e)) return NULL;

    jbyteArray arr = (*env)->NewByteArray(env, 512);
    if (arr) (*env)->SetByteArrayRegion(env, arr, 0, 512, (const jbyte *)sector);
    return arr;
}
