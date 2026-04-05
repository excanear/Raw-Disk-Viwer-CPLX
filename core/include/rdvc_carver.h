/**
 * rdvc_carver.h
 * RAW DISK VIEWER CPLX — File Carving Engine API
 *
 * Sliding-window carver that uses the signature database to
 * locate and extract embedded files from raw disk sectors.
 */

#ifndef RDVC_CARVER_H
#define RDVC_CARVER_H

#include "rdvc_platform.h"
#include "rdvc_errors.h"
#include "rdvc_disk.h"
#include "rdvc_signatures.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RDVC_CARVER_MAX_FILES   4096
#define RDVC_CARVER_MAX_PATH    RDVC_MAX_PATH

/* ─────────────────────────────────────────────────────────────
 *  A carved file record
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    rdvc_u64          start_offset;    /* Absolute byte offset on disk */
    rdvc_u64          end_offset;      /* Last byte (inclusive)        */
    rdvc_u64          size;            /* end - start + 1              */
    rdvc_u64          start_lba;
    const rdvc_signature_t *sig;       /* Matched signature            */
    char              suggested_name[128]; /* e.g. "carved_000042.jpg" */
} rdvc_carved_file_t;

/* ─────────────────────────────────────────────────────────────
 *  Carver progress callback (return RDVC_FALSE to cancel)
 * ───────────────────────────────────────────────────────────── */
typedef rdvc_bool (*rdvc_carver_progress_cb)(rdvc_u64 current_lba,
                                              rdvc_u64 total_lbas,
                                              rdvc_u32 files_found,
                                              void    *user_data);

/* ─────────────────────────────────────────────────────────────
 *  Carver configuration
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    rdvc_u64  start_lba;        /* First sector to scan             */
    rdvc_u64  end_lba;          /* Last sector to scan (inclusive)  */
    rdvc_u32  read_batch;       /* Sectors to read per iteration    */
    rdvc_bool skip_empty;       /* Skip all-zero sectors            */
    rdvc_carver_progress_cb progress_cb;
    void     *progress_user;
} rdvc_carver_config_t;

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/**
 * Run the file carver on an open disk.
 *
 * @param ctx        Open disk context
 * @param config     Carver configuration
 * @param out_files  Caller-allocated array for results
 * @param max_files  Array capacity
 * @param out_count  Number of carved files found
 * @return RDVC_OK on success (or partial success)
 */
RDVC_API rdvc_err_t rdvc_carve_disk(rdvc_disk_ctx_t          *ctx,
                                     const rdvc_carver_config_t *config,
                                     rdvc_carved_file_t        *out_files,
                                     rdvc_u32                   max_files,
                                     rdvc_u32                  *out_count);

/**
 * Export a carved file to disk.
 *
 * @param ctx         Open disk context (source)
 * @param carved      The carved file record
 * @param output_path Destination file path
 * @return RDVC_OK on success
 */
RDVC_API rdvc_err_t rdvc_export_carved(rdvc_disk_ctx_t          *ctx,
                                        const rdvc_carved_file_t *carved,
                                        const char               *output_path);

/**
 * Export raw sectors [lba_start, lba_start+count) to a file.
 */
RDVC_API rdvc_err_t rdvc_export_sectors(rdvc_disk_ctx_t *ctx,
                                         rdvc_u64         lba_start,
                                         rdvc_u32         count,
                                         const char      *output_path);

#ifdef __cplusplus
}
#endif

#endif /* RDVC_CARVER_H */
