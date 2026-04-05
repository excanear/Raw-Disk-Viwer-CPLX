/**
 * rdvc_analysis.h
 * RAW DISK VIEWER CPLX — Sector Analysis API
 *
 * Entropy calculation, null/data ratio, pattern detection.
 */

#ifndef RDVC_ANALYSIS_H
#define RDVC_ANALYSIS_H

#include "rdvc_platform.h"
#include "rdvc_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Sector classifications */
typedef enum {
    RDVC_SECTOR_CLASS_UNKNOWN    = 0,
    RDVC_SECTOR_CLASS_EMPTY      = 1,  /* All zeros                    */
    RDVC_SECTOR_CLASS_DATA       = 2,  /* Normal data                  */
    RDVC_SECTOR_CLASS_ENCRYPTED  = 3,  /* High entropy (> 7.8 bits)    */
    RDVC_SECTOR_CLASS_COMPRESSED = 4,  /* High entropy (7.0 - 7.8)     */
    RDVC_SECTOR_CLASS_TEXT       = 5,  /* Mostly printable ASCII       */
    RDVC_SECTOR_CLASS_PATTERN    = 6   /* Repeating pattern found      */
} rdvc_sector_class_t;

typedef struct {
    double              entropy;         /* Shannon entropy 0.0 - 8.0   */
    double              null_ratio;      /* Fraction of zero bytes       */
    double              printable_ratio; /* Fraction of printable ASCII  */
    rdvc_sector_class_t classification;
    rdvc_bool           has_pattern;
    rdvc_u32            pattern_period;  /* Period length if has_pattern */
} rdvc_sector_analysis_t;

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/**
 * Compute Shannon entropy H(X) = -sum(p * log2(p)) for a data block.
 * Returns value in [0.0, 8.0]. 8.0 = perfectly random / encrypted.
 *
 * @param data   Input buffer
 * @param len    Buffer length in bytes
 * @return Entropy value (negative on error)
 */
RDVC_API double rdvc_compute_entropy(const rdvc_u8 *data, rdvc_u64 len);

/**
 * Full sector analysis: entropy + ratios + classification.
 *
 * @param data   Sector data buffer
 * @param len    Length (should be sector_size)
 * @param out    Analysis result struct
 * @return RDVC_OK
 */
RDVC_API rdvc_err_t rdvc_analyze_sector(const rdvc_u8          *data,
                                         rdvc_u64                len,
                                         rdvc_sector_analysis_t *out);

/**
 * Detect if a buffer contains a repeating byte pattern.
 *
 * @param data     Input buffer
 * @param len      Buffer length
 * @param out_period Set to period length if found, 0 otherwise
 * @return RDVC_TRUE if a short repeating pattern is found
 */
RDVC_API rdvc_bool rdvc_detect_pattern(const rdvc_u8 *data,
                                        rdvc_u64       len,
                                        rdvc_u32      *out_period);

#ifdef __cplusplus
}
#endif

#endif /* RDVC_ANALYSIS_H */
