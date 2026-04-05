/**
 * disk_io.c
 * RAW DISK VIEWER CPLX — Disk I/O Engine
 *
 * Implements rdvc_disk.h: list_disks, open, close, read_sector(s).
 * All operations are strictly READ-ONLY.
 */

#include "../include/rdvc_disk.h"
#include "../include/rdvc_errors.h"
#include "../include/rdvc_platform.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Forward declarations from platform.c */
rdvc_err_t rdvc_platform_open(const char *path, rdvc_handle_t *out_handle);
rdvc_err_t rdvc_platform_close(rdvc_handle_t handle);
rdvc_err_t rdvc_platform_read(rdvc_handle_t handle, rdvc_u64 byte_offset,
                               rdvc_u8 *buf, rdvc_u32 bytes_to_read,
                               rdvc_u32 *bytes_read);
rdvc_err_t rdvc_platform_get_size(rdvc_handle_t handle, rdvc_u64 *out_bytes,
                                   rdvc_u32 *out_sector_size);
rdvc_err_t rdvc_platform_get_model(rdvc_handle_t handle, char *buf, rdvc_u32 len);

/* ─────────────────────────────────────────────────────────────
 *  rdvc_list_disks
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_list_disks(rdvc_disk_info_t *out_list,
                            rdvc_u32          max_count,
                            rdvc_u32         *out_count)
{
    if (!out_list || !out_count) return RDVC_ERR_NULL_POINTER;
    if (max_count == 0)          return RDVC_ERR_INVALID_PARAM;

    *out_count = 0;
    rdvc_u32 found = 0;

#if defined(RDVC_OS_WINDOWS)
    /* Try \\.\PhysicalDrive0 through PhysicalDriveN */
    for (rdvc_u32 i = 0; i < 32 && found < max_count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%u", i);

        rdvc_handle_t h;
        rdvc_err_t e = rdvc_platform_open(path, &h);
        if (RDVC_IS_ERR(e)) continue;

        rdvc_disk_info_t *info = &out_list[found];
        memset(info, 0, sizeof(*info));
        strncpy(info->path, path, RDVC_MAX_PATH - 1);

        rdvc_platform_get_size(h, &info->total_bytes, &info->sector_size);
        rdvc_platform_get_model(h, info->model, sizeof(info->model));
        rdvc_platform_close(h);

        if (info->sector_size == 0) info->sector_size = RDVC_SECTOR_SIZE_512;
        if (info->total_bytes > 0)
            info->total_sectors = info->total_bytes / info->sector_size;

        info->type  = RDVC_DISK_TYPE_HDD;
        info->index = i;
        found++;
    }

#elif defined(RDVC_OS_LINUX)
    /* Try /dev/sdX and /dev/nvmeXn1 */
    const char *patterns[] = {
        "/dev/sda","/dev/sdb","/dev/sdc","/dev/sdd","/dev/sde","/dev/sdf",
        "/dev/nvme0n1","/dev/nvme1n1","/dev/nvme2n1",
        "/dev/hda","/dev/hdb",
        "/dev/mmcblk0","/dev/mmcblk1",
        NULL
    };

    for (int pi = 0; patterns[pi] != NULL && found < max_count; pi++) {
        rdvc_handle_t h;
        rdvc_err_t e = rdvc_platform_open(patterns[pi], &h);
        if (RDVC_IS_ERR(e)) continue;

        rdvc_disk_info_t *info = &out_list[found];
        memset(info, 0, sizeof(*info));
        strncpy(info->path, patterns[pi], RDVC_MAX_PATH - 1);

        rdvc_platform_get_size(h, &info->total_bytes, &info->sector_size);
        rdvc_platform_get_model(h, info->model, sizeof(info->model));
        rdvc_platform_close(h);

        if (info->sector_size == 0) info->sector_size = RDVC_SECTOR_SIZE_512;
        if (info->total_bytes > 0)
            info->total_sectors = info->total_bytes / info->sector_size;

        info->type  = RDVC_DISK_TYPE_HDD;
        info->index = (rdvc_u32)found;
        found++;
    }

#elif defined(RDVC_OS_MACOS)
    for (rdvc_u32 i = 0; i < 16 && found < max_count; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/disk%u", i);

        rdvc_handle_t h;
        rdvc_err_t e = rdvc_platform_open(path, &h);
        if (RDVC_IS_ERR(e)) continue;

        rdvc_disk_info_t *info = &out_list[found];
        memset(info, 0, sizeof(*info));
        strncpy(info->path, path, RDVC_MAX_PATH - 1);

        rdvc_platform_get_size(h, &info->total_bytes, &info->sector_size);
        rdvc_platform_get_model(h, info->model, sizeof(info->model));
        rdvc_platform_close(h);

        if (info->sector_size == 0) info->sector_size = RDVC_SECTOR_SIZE_512;
        if (info->total_bytes > 0)
            info->total_sectors = info->total_bytes / info->sector_size;

        info->type  = RDVC_DISK_TYPE_HDD;
        info->index = i;
        found++;
    }
#endif

    *out_count = found;
    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_open_disk
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_open_disk(const char *path, rdvc_disk_ctx_t *ctx)
{
    if (!path || !ctx) return RDVC_ERR_NULL_POINTER;

    memset(ctx, 0, sizeof(*ctx));
    ctx->handle   = RDVC_INVALID_HANDLE;
    ctx->is_open  = RDVC_FALSE;

    rdvc_err_t e = rdvc_platform_open(path, &ctx->handle);
    if (RDVC_IS_ERR(e)) return e;

    /* Populate info */
    strncpy(ctx->info.path, path, RDVC_MAX_PATH - 1);

    e = rdvc_platform_get_size(ctx->handle,
                                &ctx->info.total_bytes,
                                &ctx->info.sector_size);
    if (RDVC_IS_ERR(e)) {
        rdvc_platform_close(ctx->handle);
        ctx->handle = RDVC_INVALID_HANDLE;
        return e;
    }

    rdvc_platform_get_model(ctx->handle, ctx->info.model,
                             sizeof(ctx->info.model));

    if (ctx->info.sector_size == 0)
        ctx->info.sector_size = RDVC_SECTOR_SIZE_512;

    if (ctx->info.total_bytes > 0)
        ctx->info.total_sectors =
            ctx->info.total_bytes / ctx->info.sector_size;

    /* Detect if it is a plain file (image) vs block device */
    ctx->is_image  = RDVC_FALSE;
    ctx->info.type = RDVC_DISK_TYPE_HDD;

    /* Simple heuristic: if path ends with .img / .dd / .iso treat as image */
    const char *dot = strrchr(path, '.');
    if (dot) {
        if (strcmp(dot, ".img") == 0 ||
            strcmp(dot, ".dd")  == 0 ||
            strcmp(dot, ".iso") == 0 ||
            strcmp(dot, ".raw") == 0) {
            ctx->is_image  = RDVC_TRUE;
            ctx->info.type = RDVC_DISK_TYPE_IMAGE;
        }
    }

    ctx->is_open = RDVC_TRUE;
    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_close_disk
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_close_disk(rdvc_disk_ctx_t *ctx)
{
    if (!ctx)           return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open)  return RDVC_ERR_DISK_NOT_OPEN;

    rdvc_err_t e = rdvc_platform_close(ctx->handle);
    ctx->handle  = RDVC_INVALID_HANDLE;
    ctx->is_open = RDVC_FALSE;
    return e;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_read_sector
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_read_sector(rdvc_disk_ctx_t *ctx,
                             rdvc_u64         lba,
                             rdvc_u8         *buf)
{
    if (!ctx || !buf)  return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open) return RDVC_ERR_DISK_NOT_OPEN;

    if (ctx->info.total_sectors > 0 && lba >= ctx->info.total_sectors)
        return RDVC_ERR_SECTOR_OUT_OF_RANGE;

    rdvc_u64 byte_offset = lba * (rdvc_u64)ctx->info.sector_size;
    rdvc_u32 bytes_read  = 0;

    rdvc_err_t e = rdvc_platform_read(ctx->handle, byte_offset,
                                       buf, ctx->info.sector_size, &bytes_read);
    if (RDVC_IS_ERR(e)) return e;
    if (bytes_read < ctx->info.sector_size) {
        /* Zero-fill remainder at end-of-disk */
        memset(buf + bytes_read, 0,
               (size_t)(ctx->info.sector_size - bytes_read));
    }
    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_read_sectors
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_read_sectors(rdvc_disk_ctx_t *ctx,
                              rdvc_u64         lba,
                              rdvc_u32         count,
                              rdvc_u8         *buf,
                              rdvc_u32        *out_read)
{
    if (!ctx || !buf || !out_read) return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open)             return RDVC_ERR_DISK_NOT_OPEN;
    if (count == 0)                return RDVC_ERR_INVALID_PARAM;

    *out_read = 0;

    rdvc_u64 total_bytes = (rdvc_u64)count * ctx->info.sector_size;
    rdvc_u64 byte_offset = lba * (rdvc_u64)ctx->info.sector_size;
    rdvc_u32 bytes_read  = 0;

    rdvc_err_t e = rdvc_platform_read(ctx->handle, byte_offset,
                                       buf, (rdvc_u32)total_bytes, &bytes_read);
    if (RDVC_IS_ERR(e)) return e;

    *out_read = bytes_read / ctx->info.sector_size;

    /* Zero-fill partial last sector */
    rdvc_u32 rem = bytes_read % ctx->info.sector_size;
    if (rem > 0) {
        memset(buf + bytes_read, 0, (size_t)(ctx->info.sector_size - rem));
    }

    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_get_disk_info
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_get_disk_info(rdvc_disk_ctx_t  *ctx,
                               rdvc_disk_info_t *out)
{
    if (!ctx || !out)  return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open) return RDVC_ERR_DISK_NOT_OPEN;
    memcpy(out, &ctx->info, sizeof(rdvc_disk_info_t));
    return RDVC_OK;
}
