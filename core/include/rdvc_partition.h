/**
 * rdvc_partition.h
 * RAW DISK VIEWER CPLX — MBR & GPT Partition Structures and API
 */

#ifndef RDVC_PARTITION_H
#define RDVC_PARTITION_H

#include "rdvc_platform.h"
#include "rdvc_errors.h"
#include "rdvc_disk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────
 *  MBR structures (packed, little-endian)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_MBR_SIGNATURE     0xAA55
#define RDVC_MBR_MAX_PARTS     4
#define RDVC_MBR_BOOTSTRAP_SZ  446
#define RDVC_PART_TYPE_EMPTY   0x00
#define RDVC_PART_TYPE_EXTENDED_CHS  0x05
#define RDVC_PART_TYPE_EXTENDED_LBA  0x0F
#define RDVC_PART_TYPE_GPT_PMBR 0xEE

RDVC_PACKED_BEGIN
typedef struct {
    rdvc_u8  status;           /* 0x80 = bootable */
    rdvc_u8  chs_first[3];
    rdvc_u8  type;             /* Partition type code */
    rdvc_u8  chs_last[3];
    rdvc_u32 lba_start;
    rdvc_u32 lba_size;
} rdvc_mbr_entry_t RDVC_PACKED_ATTR;
RDVC_PACKED_END

RDVC_PACKED_BEGIN
typedef struct {
    rdvc_u8         bootstrap[RDVC_MBR_BOOTSTRAP_SZ];
    rdvc_mbr_entry_t entries[RDVC_MBR_MAX_PARTS];
    rdvc_u16        signature;  /* Must be 0xAA55 */
} rdvc_mbr_t RDVC_PACKED_ATTR;
RDVC_PACKED_END

/* Parsed, human-readable MBR partition info */
typedef struct {
    rdvc_u8  type;
    rdvc_u32 lba_start;
    rdvc_u32 lba_size;
    rdvc_u64 byte_offset;
    rdvc_u64 byte_size;
    rdvc_bool bootable;
    char     type_name[64];
} rdvc_mbr_part_info_t;

typedef struct {
    rdvc_bool           valid;
    rdvc_u32            part_count;
    rdvc_mbr_part_info_t parts[RDVC_MBR_MAX_PARTS];
} rdvc_mbr_info_t;

/* ─────────────────────────────────────────────────────────────
 *  GPT structures (packed, little-endian)
 * ───────────────────────────────────────────────────────────── */
#define RDVC_GPT_SIGNATURE     "EFI PART"
#define RDVC_GPT_REVISION_1_0  0x00010000
#define RDVC_GPT_MAX_PARTS     128

typedef struct {
    rdvc_u32 data1;
    rdvc_u16 data2;
    rdvc_u16 data3;
    rdvc_u8  data4[8];
} rdvc_guid_t;

RDVC_PACKED_BEGIN
typedef struct {
    rdvc_u8  signature[8];       /* "EFI PART" */
    rdvc_u32 revision;
    rdvc_u32 header_size;
    rdvc_u32 header_crc32;
    rdvc_u32 reserved;
    rdvc_u64 my_lba;
    rdvc_u64 alternate_lba;
    rdvc_u64 first_usable_lba;
    rdvc_u64 last_usable_lba;
    rdvc_guid_t disk_guid;
    rdvc_u64 partition_entry_lba;
    rdvc_u32 num_partition_entries;
    rdvc_u32 sizeof_partition_entry;
    rdvc_u32 partition_entry_array_crc32;
} rdvc_gpt_header_t RDVC_PACKED_ATTR;
RDVC_PACKED_END

RDVC_PACKED_BEGIN
typedef struct {
    rdvc_guid_t type_guid;
    rdvc_guid_t unique_guid;
    rdvc_u64    start_lba;
    rdvc_u64    end_lba;
    rdvc_u64    attributes;
    rdvc_u8     name[72];   /* UTF-16LE partition name */
} rdvc_gpt_entry_t RDVC_PACKED_ATTR;
RDVC_PACKED_END

typedef struct {
    rdvc_guid_t type_guid;
    rdvc_guid_t unique_guid;
    rdvc_u64    start_lba;
    rdvc_u64    end_lba;
    rdvc_u64    byte_offset;
    rdvc_u64    byte_size;
    rdvc_u64    attributes;
    char        name[73];        /* UTF-8 converted name */
    char        type_name[64];   /* Known type or GUID string */
} rdvc_gpt_part_info_t;

typedef struct {
    rdvc_bool          valid;
    rdvc_guid_t        disk_guid;
    char               disk_guid_str[40];
    rdvc_u32           part_count;
    rdvc_gpt_part_info_t parts[RDVC_GPT_MAX_PARTS];
} rdvc_gpt_info_t;

/* ─────────────────────────────────────────────────────────────
 *  API
 * ───────────────────────────────────────────────────────────── */

/**
 * Parse the MBR from sector 0 of the open disk.
 * @param ctx   Open disk context
 * @param out   Output MBR info struct
 */
RDVC_API rdvc_err_t rdvc_parse_mbr(rdvc_disk_ctx_t *ctx, rdvc_mbr_info_t *out);

/**
 * Parse the GPT header and entries (LBA 1+) of the open disk.
 * @param ctx   Open disk context
 * @param out   Output GPT info struct
 */
RDVC_API rdvc_err_t rdvc_parse_gpt(rdvc_disk_ctx_t *ctx, rdvc_gpt_info_t *out);

/**
 * Return a human-readable name for a well-known MBR partition type code.
 */
RDVC_API const char *rdvc_mbr_type_name(rdvc_u8 type);

/**
 * Format a GUID as "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" into buf (len >= 40).
 */
RDVC_API void rdvc_guid_to_str(const rdvc_guid_t *guid, char *buf, rdvc_u32 len);

/**
 * Return a human-readable name for a well-known GPT partition type GUID.
 */
RDVC_API const char *rdvc_gpt_type_name(const rdvc_guid_t *type_guid);

#ifdef __cplusplus
}
#endif

#endif /* RDVC_PARTITION_H */
