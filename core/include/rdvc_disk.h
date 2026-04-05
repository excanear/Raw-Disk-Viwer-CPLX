/**
 * rdvc_disk.h
 * RAW DISK VIEWER CPLX — Disk Abstraction API
 *
 * All operations are READ-ONLY by design.
 * Attempting any write returns RDVC_ERR_READONLY.
 */

#ifndef RDVC_DISK_H
#define RDVC_DISK_H

#include "rdvc_platform.h"
#include "rdvc_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────
 *  Disk types
 * ───────────────────────────────────────────────────────────── */
typedef enum {
    RDVC_DISK_TYPE_UNKNOWN  = 0,
    RDVC_DISK_TYPE_HDD      = 1,
    RDVC_DISK_TYPE_SSD      = 2,
    RDVC_DISK_TYPE_REMOVABLE= 3,
    RDVC_DISK_TYPE_IMAGE    = 4,   /* .img / .dd file */
    RDVC_DISK_TYPE_VIRTUAL  = 5    /* VHD / VMDK etc. */
} rdvc_disk_type_t;

/* ─────────────────────────────────────────────────────────────
 *  Disk info structure (returned by list)
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    char             path[RDVC_MAX_PATH];   /* e.g. \\.\PhysicalDrive0 */
    char             model[256];            /* Disk model string        */
    rdvc_u64         total_bytes;           /* Total disk size in bytes */
    rdvc_u32         sector_size;           /* 512 or 4096              */
    rdvc_u64         total_sectors;         /* total_bytes / sector_size*/
    rdvc_disk_type_t type;
    rdvc_u32         index;                 /* Disk index (0, 1, ...)   */
} rdvc_disk_info_t;

/* ─────────────────────────────────────────────────────────────
 *  Open disk context
 * ───────────────────────────────────────────────────────────── */
typedef struct rdvc_disk_ctx rdvc_disk_ctx_t;

struct rdvc_disk_ctx {
    rdvc_handle_t    handle;
    rdvc_disk_info_t info;
    rdvc_bool        is_open;
    rdvc_bool        is_image;    /* true if backed by .img file */
    rdvc_u32         _reserved;
};

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/**
 * List available physical disks on the system.
 * @param out_list  Caller-allocated array of rdvc_disk_info_t
 * @param max_count Size of the array
 * @param out_count Number of disks found (written by function)
 * @return RDVC_OK on success
 */
RDVC_API rdvc_err_t rdvc_list_disks(rdvc_disk_info_t *out_list,
                                     rdvc_u32          max_count,
                                     rdvc_u32         *out_count);

/**
 * Open a disk or image file for reading.
 * @param path  Path to physical disk or .img file
 * @param ctx   Caller-allocated context to fill
 * @return RDVC_OK on success
 */
RDVC_API rdvc_err_t rdvc_open_disk(const char *path, rdvc_disk_ctx_t *ctx);

/**
 * Close a disk context and release the OS handle.
 */
RDVC_API rdvc_err_t rdvc_close_disk(rdvc_disk_ctx_t *ctx);

/**
 * Read exactly one sector (ctx->info.sector_size bytes) at the given LBA.
 * @param ctx    Open disk context
 * @param lba    Logical block address (0-based)
 * @param buf    Output buffer — must be >= sector_size bytes
 * @return RDVC_OK on success
 */
RDVC_API rdvc_err_t rdvc_read_sector(rdvc_disk_ctx_t *ctx,
                                      rdvc_u64         lba,
                                      rdvc_u8         *buf);

/**
 * Read count consecutive sectors starting at lba.
 * @param ctx       Open disk context
 * @param lba       Starting LBA
 * @param count     Number of sectors to read
 * @param buf       Output buffer — must be >= count * sector_size bytes
 * @param out_read  Actual number of sectors read (may be < count at disk end)
 * @return RDVC_OK on success
 */
RDVC_API rdvc_err_t rdvc_read_sectors(rdvc_disk_ctx_t *ctx,
                                       rdvc_u64         lba,
                                       rdvc_u32         count,
                                       rdvc_u8         *buf,
                                       rdvc_u32        *out_read);

/**
 * Get disk information for an already-open context.
 * @param ctx     Open disk context
 * @param out     Pointer to info struct to fill
 */
RDVC_API rdvc_err_t rdvc_get_disk_info(rdvc_disk_ctx_t  *ctx,
                                        rdvc_disk_info_t *out);

#ifdef __cplusplus
}
#endif

#endif /* RDVC_DISK_H */
