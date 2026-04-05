/**
 * rdvc_errors.h
 * RAW DISK VIEWER CPLX — Unified Error Code System
 *
 * All functions return rdvc_err_t. Negative = error. Zero = success.
 */

#ifndef RDVC_ERRORS_H
#define RDVC_ERRORS_H

#include "rdvc_platform.h"

typedef rdvc_i32 rdvc_err_t;

/* ─────────────────────────────────────────────────────────────
 *  Success
 * ───────────────────────────────────────────────────────────── */
#define RDVC_OK                      0

/* ─────────────────────────────────────────────────────────────
 *  Generic errors (-1 to -99)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_ERR_GENERIC            -1
#define RDVC_ERR_NULL_POINTER       -2
#define RDVC_ERR_INVALID_PARAM      -3
#define RDVC_ERR_OUT_OF_MEMORY      -4
#define RDVC_ERR_BUFFER_TOO_SMALL   -5
#define RDVC_ERR_NOT_SUPPORTED      -6
#define RDVC_ERR_TIMEOUT            -7

/* ─────────────────────────────────────────────────────────────
 *  Disk / I/O errors (-100 to -199)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_ERR_DISK_OPEN          -100
#define RDVC_ERR_DISK_READ          -101
#define RDVC_ERR_DISK_SEEK          -102
#define RDVC_ERR_DISK_NOT_OPEN      -103
#define RDVC_ERR_DISK_TOO_MANY      -104
#define RDVC_ERR_DISK_NOT_FOUND     -105
#define RDVC_ERR_DISK_ACCESS_DENIED -106
#define RDVC_ERR_SECTOR_OUT_OF_RANGE -107

/* ─────────────────────────────────────────────────────────────
 *  Security errors (-200 to -299)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_ERR_READONLY           -200   /* Write attempted in read-only mode */
#define RDVC_ERR_PRIVILEGE          -201   /* Insufficient OS privileges        */

/* ─────────────────────────────────────────────────────────────
 *  Parser errors (-300 to -399)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_ERR_MBR_INVALID        -300
#define RDVC_ERR_MBR_NO_SIGNATURE   -301
#define RDVC_ERR_GPT_INVALID        -302
#define RDVC_ERR_GPT_CRC_FAIL       -303
#define RDVC_ERR_GPT_NO_SIGNATURE   -304

/* ─────────────────────────────────────────────────────────────
 *  Analysis errors (-400 to -499)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_ERR_ANALYSIS_FAIL      -400
#define RDVC_ERR_CARVER_NO_START    -401
#define RDVC_ERR_CARVER_NO_END      -402

/* ─────────────────────────────────────────────────────────────
 *  Export errors (-500 to -599)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_ERR_EXPORT_OPEN        -500
#define RDVC_ERR_EXPORT_WRITE       -501
#define RDVC_ERR_EXPORT_PATH        -502

/* ─────────────────────────────────────────────────────────────
 *  Helpers
 * ───────────────────────────────────────────────────────────── */
#define RDVC_IS_OK(e)    ((e) == RDVC_OK)
#define RDVC_IS_ERR(e)   ((e) < 0)

/**
 * Returns a human-readable string for an error code.
 * The returned pointer is to a static string — do not free.
 */
RDVC_API const char *rdvc_strerror(rdvc_err_t err);

#endif /* RDVC_ERRORS_H */
