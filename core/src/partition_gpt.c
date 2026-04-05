/**
 * partition_gpt.c
 * RAW DISK VIEWER CPLX — GPT Parser Implementation
 *
 * Parses GPT header at LBA 1 and partition entries at LBA 2+.
 * Validates CRC32 of both header and entry array.
 */

#include "../include/rdvc_partition.h"
#include "../include/rdvc_disk.h"
#include "../include/rdvc_errors.h"
#include "../include/rdvc_platform.h"

#include <string.h>
#include <stdio.h>

/* ─────────────────────────────────────────────────────────────
 *  CRC32 table (IEEE 802.3 polynomial, used by GPT)
 * ───────────────────────────────────────────────────────────── */
static rdvc_u32 g_crc32_table[256];
static rdvc_bool g_crc32_table_init = RDVC_FALSE;

static void crc32_init_table(void)
{
    if (g_crc32_table_init) return;
    for (rdvc_u32 i = 0; i < 256; i++) {
        rdvc_u32 c = i;
        for (int j = 0; j < 8; j++)
            c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
        g_crc32_table[i] = c;
    }
    g_crc32_table_init = RDVC_TRUE;
}

static rdvc_u32 crc32_compute(const rdvc_u8 *buf, rdvc_u32 len)
{
    crc32_init_table();
    rdvc_u32 crc = 0xFFFFFFFFU;
    for (rdvc_u32 i = 0; i < len; i++)
        crc = g_crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFU;
}

/* ─────────────────────────────────────────────────────────────
 *  Little-endian helpers
 * ───────────────────────────────────────────────────────────── */
static inline rdvc_u32 le32(const rdvc_u8 *p)
{
    return (rdvc_u32)(p[0] | ((rdvc_u32)p[1]<<8)
                            | ((rdvc_u32)p[2]<<16)
                            | ((rdvc_u32)p[3]<<24));
}

static inline rdvc_u64 le64(const rdvc_u8 *p)
{
    return (rdvc_u64)le32(p) | ((rdvc_u64)le32(p+4) << 32);
}

static inline rdvc_u16 le16(const rdvc_u8 *p)
{
    return (rdvc_u16)(p[0] | ((rdvc_u16)p[1] << 8));
}

/* ─────────────────────────────────────────────────────────────
 *  GUID helpers
 * ───────────────────────────────────────────────────────────── */
static void parse_guid(const rdvc_u8 *raw, rdvc_guid_t *out)
{
    out->data1 = le32(raw);
    out->data2 = le16(raw + 4);
    out->data3 = le16(raw + 6);
    memcpy(out->data4, raw + 8, 8);
}

void rdvc_guid_to_str(const rdvc_guid_t *g, char *buf, rdvc_u32 len)
{
    if (!g || !buf || len < 37) return;
    snprintf(buf, (size_t)len,
        "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        g->data1, g->data2, g->data3,
        g->data4[0], g->data4[1],
        g->data4[2], g->data4[3], g->data4[4],
        g->data4[5], g->data4[6], g->data4[7]);
}

/* ─────────────────────────────────────────────────────────────
 *  GPT type GUID name table
 * ───────────────────────────────────────────────────────────── */
typedef struct { const char *guid_str; const char *name; } gpt_type_entry_t;

static const gpt_type_entry_t gpt_type_table[] = {
    {"00000000-0000-0000-0000-000000000000", "Empty"},
    {"C12A7328-F81F-11D2-BA4B-00A0C93EC93B", "EFI System Partition"},
    {"21686148-6449-6E6F-744E-656564454649", "BIOS Boot Partition"},
    {"024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR Partition Scheme"},
    {"D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", "Intel Fast Flash (iFFS)"},
    {"F4019732-066E-4E12-8273-346C5641494F", "Sony Boot Partition"},
    {"BFBFAFE7-A34F-448A-9A5B-6213EB736C22", "Lenovo Boot Partition"},

    /* Windows */
    {"E3C9E316-0B5C-4DB8-817D-F92DF00215AE", "Microsoft Reserved"},
    {"EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft Basic Data"},
    {"5808C8AA-7E8F-42E0-85D2-E1E90434CFB3", "LDM Metadata"},
    {"AF9B60A0-1431-4F62-BC68-3311714A69AD", "LDM Data"},
    {"DE94BBA4-06D1-4D40-A16A-BFD50179D6AC", "Windows Recovery"},
    {"37AFFC90-EF7D-4e96-91C3-2D7AE055B174", "IBM GPFS"},

    /* Linux */
    {"0FC63DAF-8483-4772-8E79-3D69D8477DE4", "Linux Filesystem Data"},
    {"A19D880F-05FC-4D3B-A006-743F0F84911E", "Linux RAID"},
    {"44479540-F297-41B2-9AF7-D131D5F0458A", "Linux Root x86"},
    {"4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709", "Linux Root x86-64"},
    {"69DAD710-2CE4-4E3C-B16C-21A1D49ABED3", "Linux Root ARM32"},
    {"B921B045-1DF0-41C3-AF44-4C6F280D3FAE", "Linux Root ARM64"},
    {"0657FD6D-A4AB-43C4-84E5-0933C84B4F4F", "Linux Swap"},
    {"E6D6D379-F507-44C2-A23C-238F2A3DF928", "Linux LVM"},
    {"933AC7E1-2EB4-4F13-B844-0E14E2AEF915", "Linux /home"},
    {"3B8F8425-20E0-4F3B-907F-1A25A76F98E8", "Linux /srv"},
    {"7FFEC5C9-2D00-49B7-8941-3EA10A5586B7", "Linux plaintext dm-crypt"},
    {"CA7D7CCB-63ED-4C53-861C-1742536059CC", "Linux LUKS"},

    /* macOS */
    {"48465300-0000-11AA-AA11-00306543ECAC", "Apple HFS+"},
    {"55465300-0000-11AA-AA11-00306543ECAC", "Apple UFS"},
    {"6A898CC3-1DD2-11B2-99A6-080020736631", "Apple ZFS"},
    {"52414944-0000-11AA-AA11-00306543ECAC", "Apple RAID"},
    {"426F6F74-0000-11AA-AA11-00306543ECAC", "Apple Boot"},
    {"4C616265-6C00-11AA-AA11-00306543ECAC", "Apple Label"},
    {"5265636F-7665-11AA-AA11-00306543ECAC", "Apple TV Recovery"},
    {"53746F72-6167-11AA-AA11-00306543ECAC", "Apple Core Storage"},
    {"7C3457EF-0000-11AA-AA11-00306543ECAC", "Apple APFS"},

    /* FreeBSD */
    {"83BD6B9D-7F41-11DC-BE0B-001560B84F0F", "FreeBSD Boot"},
    {"516E7CB4-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Data"},
    {"516E7CB5-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Swap"},
    {"516E7CB6-6ECF-11D6-8FF8-00022D09712B", "FreeBSD UFS"},
    {"516E7CB8-6ECF-11D6-8FF8-00022D09712B", "FreeBSD Vinum/RAID"},
    {"516E7CBA-6ECF-11D6-8FF8-00022D09712B", "FreeBSD ZFS"},

    /* Solaris */
    {"6A82CB45-1DD2-11B2-99A6-080020736631", "Solaris Boot"},
    {"6A85CF4D-1DD2-11B2-99A6-080020736631", "Solaris Root"},
    {"6A898CC3-1DD2-11B2-99A6-080020736631", "Solaris Data"},
    {"6A87C46F-1DD2-11B2-99A6-080020736631", "Solaris Swap"},
    {"6A8B642B-1DD2-11B2-99A6-080020736631", "Solaris Backup"},

    {NULL, NULL}
};

const char *rdvc_gpt_type_name(const rdvc_guid_t *type_guid)
{
    char guid_str[40];
    rdvc_guid_to_str(type_guid, guid_str, sizeof(guid_str));
    for (int i = 0; gpt_type_table[i].guid_str != NULL; i++) {
        if (strcasecmp(gpt_type_table[i].guid_str, guid_str) == 0)
            return gpt_type_table[i].name;
    }
    return "Unknown";
}

/* ─────────────────────────────────────────────────────────────
 *  UTF-16LE → UTF-8 (basic, BMP only)
 * ───────────────────────────────────────────────────────────── */
static void utf16le_to_utf8(const rdvc_u8 *src, rdvc_u32 src_bytes, char *dst, rdvc_u32 dst_len)
{
    rdvc_u32 di = 0;
    for (rdvc_u32 si = 0; si + 1 < src_bytes && di + 1 < dst_len; si += 2) {
        rdvc_u16 cp = (rdvc_u16)(src[si] | ((rdvc_u16)src[si+1] << 8));
        if (cp == 0) break;
        if (cp < 0x80) {
            dst[di++] = (char)cp;
        } else if (cp < 0x800) {
            if (di + 2 >= dst_len) break;
            dst[di++] = (char)(0xC0 | (cp >> 6));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        } else {
            if (di + 3 >= dst_len) break;
            dst[di++] = (char)(0xE0 | (cp >> 12));
            dst[di++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            dst[di++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    dst[di] = '\0';
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_parse_gpt
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_parse_gpt(rdvc_disk_ctx_t *ctx, rdvc_gpt_info_t *out)
{
    if (!ctx || !out)  return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open) return RDVC_ERR_DISK_NOT_OPEN;

    memset(out, 0, sizeof(*out));
    out->valid = RDVC_FALSE;

    /* Read LBA 1 — GPT header */
    rdvc_u8 sector[RDVC_SECTOR_SIZE_4096];
    rdvc_err_t e = rdvc_read_sector(ctx, 1, sector);
    if (RDVC_IS_ERR(e)) return e;

    /* Verify signature "EFI PART" */
    if (memcmp(sector, "EFI PART", 8) != 0)
        return RDVC_ERR_GPT_NO_SIGNATURE;

    /* Parse header fields */
    rdvc_u32 header_size    = le32(sector + 12);
    rdvc_u32 stored_crc     = le32(sector + 16);
    rdvc_u64 part_entry_lba = le64(sector + 72);
    rdvc_u32 num_entries    = le32(sector + 80);
    rdvc_u32 entry_size     = le32(sector + 84);
    rdvc_u32 entries_crc    = le32(sector + 88);

    /* Validate header CRC: zero out CRC field, compute, compare */
    rdvc_u8 hdr_copy[512];
    memcpy(hdr_copy, sector, header_size < 512 ? header_size : 512);
    memset(hdr_copy + 16, 0, 4);
    rdvc_u32 computed_crc = crc32_compute(hdr_copy, header_size);
    if (computed_crc != stored_crc)
        return RDVC_ERR_GPT_CRC_FAIL;

    /* Parse disk GUID */
    parse_guid(sector + 56, &out->disk_guid);
    rdvc_guid_to_str(&out->disk_guid, out->disk_guid_str, sizeof(out->disk_guid_str));

    /* Read partition entries */
    if (num_entries > RDVC_GPT_MAX_PARTS)
        num_entries = RDVC_GPT_MAX_PARTS;
    if (entry_size == 0 || entry_size > 512)
        entry_size = 128;

    /* Compute how many sectors the entries span */
    rdvc_u32 entries_total_bytes = num_entries * entry_size;
    rdvc_u32 sectors_needed = (entries_total_bytes + ctx->info.sector_size - 1)
                              / ctx->info.sector_size;
    if (sectors_needed == 0) sectors_needed = 1;

    /* Allocate on stack if small, otherwise cap */
    rdvc_u8 entries_buf[128 * 128]; /* 128 entries × 128 bytes = 16KB max */
    rdvc_u32 buf_capacity = sizeof(entries_buf);
    rdvc_u32 to_read_bytes = entries_total_bytes;
    if (to_read_bytes > buf_capacity) to_read_bytes = buf_capacity;

    rdvc_u32 sectors_to_read = (to_read_bytes + ctx->info.sector_size - 1)
                               / ctx->info.sector_size;

    rdvc_u32 read_count = 0;
    rdvc_err_t ee = rdvc_read_sectors(ctx, part_entry_lba, sectors_to_read,
                                       entries_buf, &read_count);
    if (RDVC_IS_ERR(ee)) return ee;

    /* Validate entries CRC */
    rdvc_u32 actual_bytes = (read_count * ctx->info.sector_size < to_read_bytes)
                            ? read_count * ctx->info.sector_size : to_read_bytes;
    rdvc_u32 comp_entries_crc = crc32_compute(entries_buf, entries_total_bytes < actual_bytes
                                               ? entries_total_bytes : actual_bytes);
    if (comp_entries_crc != entries_crc)
        return RDVC_ERR_GPT_CRC_FAIL;

    /* Parse each entry */
    out->part_count = 0;
    for (rdvc_u32 i = 0; i < num_entries && out->part_count < RDVC_GPT_MAX_PARTS; i++) {
        const rdvc_u8 *entry = entries_buf + ((rdvc_u64)i * entry_size);

        /* Check if entry is used: type GUID must not be all zeros */
        rdvc_bool all_zero = RDVC_TRUE;
        for (int b = 0; b < 16; b++) {
            if (entry[b] != 0) { all_zero = RDVC_FALSE; break; }
        }
        if (all_zero) continue;

        rdvc_gpt_part_info_t *part = &out->parts[out->part_count];
        parse_guid(entry,      &part->type_guid);
        parse_guid(entry + 16, &part->unique_guid);
        part->start_lba   = le64(entry + 32);
        part->end_lba     = le64(entry + 40);
        part->attributes  = le64(entry + 48);

        /* Compute byte range */
        part->byte_offset = part->start_lba * ctx->info.sector_size;
        part->byte_size   = (part->end_lba - part->start_lba + 1)
                           * ctx->info.sector_size;

        /* Partition name: UTF-16LE at offset 56, 72 bytes (max 36 chars) */
        utf16le_to_utf8(entry + 56, 72, part->name, sizeof(part->name));

        /* Type name */
        strncpy(part->type_name, rdvc_gpt_type_name(&part->type_guid),
                sizeof(part->type_name) - 1);

        out->part_count++;
    }

    out->valid = RDVC_TRUE;
    return RDVC_OK;
}
