/**
 * analysis.c
 * RAW DISK VIEWER CPLX — Sector Analysis Engine
 *
 * Shannon entropy, byte frequency, null/printable ratios,
 * pattern detection, and sector classification.
 */

#include "../include/rdvc_analysis.h"
#include "../include/rdvc_platform.h"

#include <math.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────
 *  rdvc_compute_entropy
 *  Shannon H(X) = -sum(p(x) * log2(p(x))) over 256 symbols
 * ───────────────────────────────────────────────────────────── */
double rdvc_compute_entropy(const rdvc_u8 *data, rdvc_u64 len)
{
    if (!data || len == 0) return 0.0;

    rdvc_u64 freq[256];
    memset(freq, 0, sizeof(freq));

    for (rdvc_u64 i = 0; i < len; i++)
        freq[data[i]]++;

    double entropy = 0.0;
    double n = (double)len;

    for (int i = 0; i < 256; i++) {
        if (freq[i] == 0) continue;
        double p = (double)freq[i] / n;
        entropy -= p * log2(p);
    }

    return entropy;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_detect_pattern
 *  Checks for short repeating byte patterns (period 1-16)
 * ───────────────────────────────────────────────────────────── */
rdvc_bool rdvc_detect_pattern(const rdvc_u8 *data,
                               rdvc_u64       len,
                               rdvc_u32      *out_period)
{
    if (!data || len < 4) return RDVC_FALSE;

    /* Test periods from 1 to 16 */
    for (rdvc_u32 period = 1; period <= 16; period++) {
        rdvc_u64 mismatch = 0;
        rdvc_u64 check_len = len < 256 ? len : 256;

        for (rdvc_u64 i = period; i < check_len; i++) {
            if (data[i] != data[i % period])
                mismatch++;
        }

        /* If < 5% mismatch, it's a pattern */
        if (mismatch < check_len / 20) {
            if (out_period) *out_period = period;
            return RDVC_TRUE;
        }
    }

    if (out_period) *out_period = 0;
    return RDVC_FALSE;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_analyze_sector
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_analyze_sector(const rdvc_u8          *data,
                                rdvc_u64                len,
                                rdvc_sector_analysis_t *out)
{
    if (!data || !out) return RDVC_ERR_NULL_POINTER;
    if (len == 0)      return RDVC_ERR_INVALID_PARAM;

    memset(out, 0, sizeof(*out));

    /* Count byte categories */
    rdvc_u64 null_count      = 0;
    rdvc_u64 printable_count = 0;

    for (rdvc_u64 i = 0; i < len; i++) {
        rdvc_u8 b = data[i];
        if (b == 0x00) null_count++;
        if (b >= 0x20 && b <= 0x7E) printable_count++;
    }

    out->null_ratio      = (double)null_count      / (double)len;
    out->printable_ratio = (double)printable_count / (double)len;
    out->entropy         = rdvc_compute_entropy(data, len);

    /* Pattern detection */
    rdvc_u32 period = 0;
    out->has_pattern    = rdvc_detect_pattern(data, len, &period);
    out->pattern_period = period;

    /* Classification logic */
    if (out->null_ratio > 0.99) {
        out->classification = RDVC_SECTOR_CLASS_EMPTY;
    } else if (out->has_pattern && period <= 4) {
        out->classification = RDVC_SECTOR_CLASS_PATTERN;
    } else if (out->entropy >= 7.8) {
        out->classification = RDVC_SECTOR_CLASS_ENCRYPTED;
    } else if (out->entropy >= 7.0) {
        out->classification = RDVC_SECTOR_CLASS_COMPRESSED;
    } else if (out->printable_ratio >= 0.85) {
        out->classification = RDVC_SECTOR_CLASS_TEXT;
    } else {
        out->classification = RDVC_SECTOR_CLASS_DATA;
    }

    return RDVC_OK;
}
