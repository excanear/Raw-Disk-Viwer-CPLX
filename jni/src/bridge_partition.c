/**
 * bridge_partition.c
 * RAW DISK VIEWER CPLX — JNI Bridge: Partition parsing
 *
 * Implements:
 *   - getMBRInfo()
 *   - getGPTInfo()
 */

#include <jni.h>
#include <string.h>
#include <stdlib.h>

#include "../../core/include/rdvc_disk.h"
#include "../../core/include/rdvc_partition.h"
#include "../../core/include/rdvc_errors.h"
#include "../../core/include/rdvc_platform.h"

#define CLS_PARTITION_INFO "com/rdvc/bridge/model/PartitionInfo"

/* ─────────────────────────────────────────────────────────────
 *  getMBRInfo()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jobjectArray JNICALL
Java_com_rdvc_bridge_NativeBridge_getMBRInfo(JNIEnv *env, jclass clazz, jlong handle)
{
    (void)clazz;
    if (handle <= 0) return NULL;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    rdvc_mbr_info_t mbr;
    rdvc_err_t e = rdvc_parse_mbr(ctx, &mbr);
    if (RDVC_IS_ERR(e) || !mbr.valid) return NULL;

    jclass cls = (*env)->FindClass(env, CLS_PARTITION_INFO);
    if (!cls) return NULL;

    /* MBR constructor: (int index, int mbrTypeCode, String typeName,
                         long startLba, long sizeLba,
                         long byteOffset, long byteSize, boolean bootable) */
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(IILjava/lang/String;JJJJZ)V");
    if (!ctor) return NULL;

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)mbr.part_count, cls, NULL);
    if (!arr) return NULL;

    for (rdvc_u32 i = 0; i < mbr.part_count; i++) {
        const rdvc_mbr_part_info_t *p = &mbr.parts[i];

        jstring type_name = (*env)->NewStringUTF(env, p->type_name);

        jobject obj = (*env)->NewObject(env, cls, ctor,
            (jint)i,
            (jint)p->type,
            type_name,
            (jlong)p->lba_start,
            (jlong)p->lba_size,
            (jlong)p->byte_offset,
            (jlong)p->byte_size,
            (jboolean)p->bootable
        );

        (*env)->DeleteLocalRef(env, type_name);
        if (obj) {
            (*env)->SetObjectArrayElement(env, arr, (jsize)i, obj);
            (*env)->DeleteLocalRef(env, obj);
        }
    }

    return arr;
}

/* ─────────────────────────────────────────────────────────────
 *  getGPTInfo()
 * ───────────────────────────────────────────────────────────── */
JNIEXPORT jobjectArray JNICALL
Java_com_rdvc_bridge_NativeBridge_getGPTInfo(JNIEnv *env, jclass clazz, jlong handle)
{
    (void)clazz;
    if (handle <= 0) return NULL;

    rdvc_disk_ctx_t *ctx = (rdvc_disk_ctx_t *)(uintptr_t)handle;
    rdvc_gpt_info_t gpt;
    rdvc_err_t e = rdvc_parse_gpt(ctx, &gpt);
    if (RDVC_IS_ERR(e) || !gpt.valid) return NULL;

    jclass cls = (*env)->FindClass(env, CLS_PARTITION_INFO);
    if (!cls) return NULL;

    /* GPT constructor: (int index, String typeGuid, String uniqueGuid,
                         String typeName, String name,
                         long startLba, long endLba,
                         long byteOffset, long byteSize) */
    jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>",
        "(ILjava/lang/String;Ljava/lang/String;"
        "Ljava/lang/String;Ljava/lang/String;JJJJ)V");
    if (!ctor) return NULL;

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)gpt.part_count, cls, NULL);
    if (!arr) return NULL;

    for (rdvc_u32 i = 0; i < gpt.part_count; i++) {
        const rdvc_gpt_part_info_t *p = &gpt.parts[i];

        char type_guid_str[40], unique_guid_str[40];
        rdvc_guid_to_str(&p->type_guid,   type_guid_str,   sizeof(type_guid_str));
        rdvc_guid_to_str(&p->unique_guid, unique_guid_str, sizeof(unique_guid_str));

        jstring type_guid  = (*env)->NewStringUTF(env, type_guid_str);
        jstring unique_guid= (*env)->NewStringUTF(env, unique_guid_str);
        jstring type_name  = (*env)->NewStringUTF(env, p->type_name);
        jstring name       = (*env)->NewStringUTF(env, p->name);

        jobject obj = (*env)->NewObject(env, cls, ctor,
            (jint)i,
            type_guid,
            unique_guid,
            type_name,
            name,
            (jlong)p->start_lba,
            (jlong)p->end_lba,
            (jlong)p->byte_offset,
            (jlong)p->byte_size
        );

        (*env)->DeleteLocalRef(env, type_guid);
        (*env)->DeleteLocalRef(env, unique_guid);
        (*env)->DeleteLocalRef(env, type_name);
        (*env)->DeleteLocalRef(env, name);

        if (obj) {
            (*env)->SetObjectArrayElement(env, arr, (jsize)i, obj);
            (*env)->DeleteLocalRef(env, obj);
        }
    }

    return arr;
}
