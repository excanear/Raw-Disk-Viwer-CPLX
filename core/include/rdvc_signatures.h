/**
 * rdvc_signatures.h
 * RAW DISK VIEWER CPLX — File Signature Database API
 *
 * Provides a static database of known file magic bytes
 * for use in scanning and file carving.
 */

#ifndef RDVC_SIGNATURES_H
#define RDVC_SIGNATURES_H

#include "rdvc_platform.h"
#include "rdvc_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RDVC_SIG_MAX_MAGIC_LEN  16
#define RDVC_SIG_MAX_EXT_LEN    16
#define RDVC_SIG_MAX_MIME_LEN   48
#define RDVC_SIG_MAX_DESC_LEN   64

/* ─────────────────────────────────────────────────────────────
 *  A single file signature entry
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    rdvc_u8  magic[RDVC_SIG_MAX_MAGIC_LEN]; /* Header magic bytes       */
    rdvc_u8  magic_len;                      /* How many bytes to match  */
    rdvc_u32 magic_offset;                   /* Offset within file       */
    rdvc_u8  eof[RDVC_SIG_MAX_MAGIC_LEN];   /* Footer / end signature   */
    rdvc_u8  eof_len;                        /* 0 = no footer            */
    rdvc_u64 max_size;                       /* Max expected file size   */
    char     ext[RDVC_SIG_MAX_EXT_LEN];     /* e.g. "jpg"               */
    char     mime[RDVC_SIG_MAX_MIME_LEN];   /* e.g. "image/jpeg"        */
    char     desc[RDVC_SIG_MAX_DESC_LEN];   /* Human description        */
} rdvc_signature_t;

/* ─────────────────────────────────────────────────────────────
 *  Result of a signature match in a scan
 * ───────────────────────────────────────────────────────────── */
typedef struct {
    rdvc_u64          byte_offset;    /* Absolute byte offset on disk  */
    rdvc_u64          lba;            /* Sector containing this match  */
    rdvc_u32          offset_in_sector; /* Byte offset within sector   */
    const rdvc_signature_t *sig;      /* Pointer to matched signature  */
} rdvc_sig_match_t;

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/**
 * Returns a pointer to the built-in signature table.
 * @param out_count  Number of entries in the table
 * @return Pointer to const array (do not free).
 */
RDVC_API const rdvc_signature_t *rdvc_get_signatures(rdvc_u32 *out_count);

/**
 * Try to match any known signature against a data buffer.
 * Scans the buffer byte-by-byte up to buf_len bytes.
 *
 * @param buf        Data buffer (typically one or more sectors)
 * @param buf_len    Length of buffer in bytes
 * @param base_offset Absolute byte offset of buf[0] on disk
 * @param sector_size Sector size for LBA computation
 * @param out_matches Caller-allocated array for results
 * @param max_matches Capacity of out_matches array
 * @param out_count   Number of matches written
 * @return RDVC_OK
 */
RDVC_API rdvc_err_t rdvc_scan_buffer(const rdvc_u8     *buf,
                                      rdvc_u64           buf_len,
                                      rdvc_u64           base_offset,
                                      rdvc_u32           sector_size,
                                      rdvc_sig_match_t  *out_matches,
                                      rdvc_u32           max_matches,
                                      rdvc_u32          *out_count);

#ifdef __cplusplus
}
#endif

#endif /* RDVC_SIGNATURES_H */
