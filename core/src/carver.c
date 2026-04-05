/**
 * carver.c
 * RAW DISK VIEWER CPLX — File Carving Engine
 *
 * Sliding-window carver: reads disk in batches, looks for
 * known start/end signatures, records found files, and can
 * export them to the filesystem.
 */

#include "../include/rdvc_carver.h"
#include "../include/rdvc_disk.h"
#include "../include/rdvc_signatures.h"
#include "../include/rdvc_analysis.h"
#include "../include/rdvc_errors.h"
#include "../include/rdvc_platform.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CARVER_BATCH_SECTORS  64U   /* Sectors per read pass */

/* ─────────────────────────────────────────────────────────────
 *  rdvc_carve_disk
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_carve_disk(rdvc_disk_ctx_t            *ctx,
                            const rdvc_carver_config_t *config,
                            rdvc_carved_file_t         *out_files,
                            rdvc_u32                    max_files,
                            rdvc_u32                   *out_count)
{
    if (!ctx || !config || !out_files || !out_count)
        return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open)
        return RDVC_ERR_DISK_NOT_OPEN;

    *out_count = 0;

    rdvc_u32 batch = config->read_batch > 0 ? config->read_batch : CARVER_BATCH_SECTORS;
    rdvc_u64 end_lba = config->end_lba;
    if (end_lba == 0 || end_lba >= ctx->info.total_sectors)
        end_lba = ctx->info.total_sectors > 0 ? ctx->info.total_sectors - 1 : 0;

    rdvc_u32 sig_count = 0;
    const rdvc_signature_t *sigs = rdvc_get_signatures(&sig_count);

    /* Allocate read buffer: batch * sector_size */
    rdvc_u32 buf_size = batch * ctx->info.sector_size;
    rdvc_u8 *buf = (rdvc_u8 *)malloc(buf_size);
    if (!buf) return RDVC_ERR_OUT_OF_MEMORY;

    /* Match scratch array for scan_buffer */
    #define MAX_MATCHES_PER_BATCH 512
    rdvc_sig_match_t matches[MAX_MATCHES_PER_BATCH];

    /* Track open carves (files whose start found but not end yet) */
    #define MAX_OPEN_CARVES 128
    typedef struct {
        rdvc_u64 start_offset;
        rdvc_u64 start_lba;
        const rdvc_signature_t *sig;
        rdvc_u32 file_index;  /* index in out_files */
    } open_carve_t;
    open_carve_t open_carves[MAX_OPEN_CARVES];
    rdvc_u32 open_count = 0;

    rdvc_u64 total_lbas = end_lba - config->start_lba + 1;
    rdvc_u64 current_lba = config->start_lba;

    while (current_lba <= end_lba) {
        /* Calculate how many sectors to read this pass */
        rdvc_u64 remaining = end_lba - current_lba + 1;
        rdvc_u32 to_read = (rdvc_u32)(remaining < (rdvc_u64)batch ? remaining : batch);

        rdvc_u32 read_count = 0;
        rdvc_err_t e = rdvc_read_sectors(ctx, current_lba, to_read, buf, &read_count);
        if (RDVC_IS_ERR(e) || read_count == 0) break;

        rdvc_u32 actual_bytes = read_count * ctx->info.sector_size;
        rdvc_u64 base_offset  = current_lba * ctx->info.sector_size;

        /* Skip if all-empty and skip_empty is enabled */
        if (config->skip_empty) {
            rdvc_bool all_zero = RDVC_TRUE;
            for (rdvc_u32 b = 0; b < actual_bytes; b++) {
                if (buf[b] != 0) { all_zero = RDVC_FALSE; break; }
            }
            if (all_zero) {
                current_lba += read_count;
                continue;
            }
        }

        /* Scan for signatures */
        rdvc_u32 match_count = 0;
        rdvc_scan_buffer(buf, (rdvc_u64)actual_bytes, base_offset,
                         ctx->info.sector_size, matches,
                         MAX_MATCHES_PER_BATCH, &match_count);

        for (rdvc_u32 m = 0; m < match_count; m++) {
            const rdvc_sig_match_t *match = &matches[m];
            const rdvc_signature_t *sig   = match->sig;

            /* Check if this offset closes any open carve */
            if (sig->eof_len > 0) {
                /* Look if buf at match position matches any open carve's eof */
                rdvc_u64 pos_in_buf = match->byte_offset - base_offset;

                for (rdvc_u32 oc = 0; oc < open_count; oc++) {
                    if (open_carves[oc].sig != sig) continue;
                    if (pos_in_buf + sig->eof_len > actual_bytes) continue;

                    /* Check EOF magic at pos_in_buf */
                    if (memcmp(buf + pos_in_buf, sig->eof, sig->eof_len) == 0) {
                        rdvc_u64 end_offset = match->byte_offset + sig->eof_len - 1;
                        rdvc_u64 file_size  = end_offset - open_carves[oc].start_offset + 1;

                        if (*out_count < max_files) {
                            rdvc_carved_file_t *cf = &out_files[*out_count];
                            cf->start_offset = open_carves[oc].start_offset;
                            cf->end_offset   = end_offset;
                            cf->size         = file_size;
                            cf->start_lba    = open_carves[oc].start_lba;
                            cf->sig          = sig;
                            snprintf(cf->suggested_name, sizeof(cf->suggested_name),
                                     "carved_%06u.%s",
                                     *out_count, sig->ext);
                            (*out_count)++;
                        }

                        /* Remove from open carves */
                        open_carves[oc] = open_carves[--open_count];
                        break;
                    }
                }
            }

            /* Start signature match: open a new carve if not already open for this type */
            rdvc_bool already_open = RDVC_FALSE;
            for (rdvc_u32 oc = 0; oc < open_count; oc++) {
                if (open_carves[oc].sig == sig) { already_open = RDVC_TRUE; break; }
            }

            if (!already_open && open_count < MAX_OPEN_CARVES) {
                open_carves[open_count].start_offset = match->byte_offset;
                open_carves[open_count].start_lba    = match->lba;
                open_carves[open_count].sig          = sig;
                open_carves[open_count].file_index   = *out_count;
                open_count++;
            }
        }

        /* For open carves without EOF: close if max_size exceeded */
        for (rdvc_u32 oc = 0; oc < open_count; ) {
            rdvc_u64 current_offset = (current_lba + read_count) * ctx->info.sector_size;
            rdvc_u64 size_so_far = current_offset - open_carves[oc].start_offset;

            if (open_carves[oc].sig->max_size > 0 &&
                size_so_far > open_carves[oc].sig->max_size) {
                /* Close it as best-effort if no eof */
                if (open_carves[oc].sig->eof_len == 0 && *out_count < max_files) {
                    rdvc_carved_file_t *cf = &out_files[*out_count];
                    cf->start_offset = open_carves[oc].start_offset;
                    cf->end_offset   = current_offset - 1;
                    cf->size         = current_offset - open_carves[oc].start_offset;
                    cf->start_lba    = open_carves[oc].start_lba;
                    cf->sig          = open_carves[oc].sig;
                    snprintf(cf->suggested_name, sizeof(cf->suggested_name),
                             "carved_%06u.%s", *out_count, open_carves[oc].sig->ext);
                    (*out_count)++;
                }
                open_carves[oc] = open_carves[--open_count];
            } else {
                oc++;
            }
        }

        /* Progress callback */
        if (config->progress_cb) {
            rdvc_u64 done = current_lba - config->start_lba + read_count;
            if (!config->progress_cb(current_lba, total_lbas, *out_count,
                                     config->progress_user)) {
                /* Cancelled */
                break;
            }
        }

        current_lba += read_count;
    }

    free(buf);
    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_export_carved
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_export_carved(rdvc_disk_ctx_t          *ctx,
                               const rdvc_carved_file_t *carved,
                               const char               *output_path)
{
    if (!ctx || !carved || !output_path) return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open)                   return RDVC_ERR_DISK_NOT_OPEN;
    if (!output_path[0])                 return RDVC_ERR_EXPORT_PATH;

    FILE *f = fopen(output_path, "wb");
    if (!f) return RDVC_ERR_EXPORT_OPEN;

    rdvc_u32 sector_size = ctx->info.sector_size;
    rdvc_u8 *buf = (rdvc_u8 *)malloc(sector_size * 64);
    if (!buf) { fclose(f); return RDVC_ERR_OUT_OF_MEMORY; }

    rdvc_u64 remaining = carved->size;
    rdvc_u64 offset    = carved->start_offset;

    while (remaining > 0) {
        rdvc_u64 to_read_bytes = remaining < (rdvc_u64)(sector_size * 64)
                                 ? remaining : (rdvc_u64)(sector_size * 64);
        rdvc_u64 lba = offset / sector_size;
        rdvc_u32 sectors = (rdvc_u32)((to_read_bytes + sector_size - 1) / sector_size);

        rdvc_u32 read_count = 0;
        rdvc_err_t e = rdvc_read_sectors(ctx, lba, sectors, buf, &read_count);
        if (RDVC_IS_ERR(e) || read_count == 0) break;

        rdvc_u64 got_bytes = (rdvc_u64)read_count * sector_size;
        rdvc_u64 write_bytes = got_bytes < remaining ? got_bytes : remaining;

        if (fwrite(buf, 1, (size_t)write_bytes, f) != (size_t)write_bytes) {
            free(buf); fclose(f);
            return RDVC_ERR_EXPORT_WRITE;
        }

        remaining -= write_bytes;
        offset    += write_bytes;
    }

    free(buf);
    fclose(f);
    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_export_sectors
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_export_sectors(rdvc_disk_ctx_t *ctx,
                                rdvc_u64         lba_start,
                                rdvc_u32         count,
                                const char      *output_path)
{
    if (!ctx || !output_path) return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open)        return RDVC_ERR_DISK_NOT_OPEN;
    if (count == 0)           return RDVC_ERR_INVALID_PARAM;
    if (!output_path[0])      return RDVC_ERR_EXPORT_PATH;

    FILE *f = fopen(output_path, "wb");
    if (!f) return RDVC_ERR_EXPORT_OPEN;

    rdvc_u32 sector_size = ctx->info.sector_size;
    rdvc_u32 batch = 64;
    rdvc_u8 *buf   = (rdvc_u8 *)malloc((rdvc_u64)batch * sector_size);
    if (!buf) { fclose(f); return RDVC_ERR_OUT_OF_MEMORY; }

    rdvc_u64 lba     = lba_start;
    rdvc_u32 left    = count;

    while (left > 0) {
        rdvc_u32 to_read = left < batch ? left : batch;
        rdvc_u32 read_count = 0;
        rdvc_err_t e = rdvc_read_sectors(ctx, lba, to_read, buf, &read_count);
        if (RDVC_IS_ERR(e) || read_count == 0) break;

        rdvc_u64 write_bytes = (rdvc_u64)read_count * sector_size;
        if (fwrite(buf, 1, (size_t)write_bytes, f) != (size_t)write_bytes) {
            free(buf); fclose(f);
            return RDVC_ERR_EXPORT_WRITE;
        }

        lba  += read_count;
        left -= read_count;
    }

    free(buf);
    fclose(f);
    return RDVC_OK;
}
