/**
 * partition_mbr.c
 * RAW DISK VIEWER CPLX — MBR Parser Implementation
 */

#include "../include/rdvc_partition.h"
#include "../include/rdvc_disk.h"
#include "../include/rdvc_errors.h"
#include "../include/rdvc_platform.h"

#include <string.h>
#include <stdio.h>

/* ─────────────────────────────────────────────────────────────
 *  MBR partition type name table
 * ───────────────────────────────────────────────────────────── */
typedef struct { rdvc_u8 type; const char *name; } mbr_type_entry_t;

static const mbr_type_entry_t mbr_type_table[] = {
    {0x00, "Empty"},
    {0x01, "FAT12"},
    {0x04, "FAT16 < 32MB"},
    {0x05, "Extended (CHS)"},
    {0x06, "FAT16 >= 32MB"},
    {0x07, "NTFS / exFAT"},
    {0x0B, "FAT32 (CHS)"},
    {0x0C, "FAT32 (LBA)"},
    {0x0E, "FAT16 (LBA)"},
    {0x0F, "Extended (LBA)"},
    {0x11, "Hidden FAT12"},
    {0x14, "Hidden FAT16 < 32MB"},
    {0x16, "Hidden FAT16"},
    {0x17, "Hidden NTFS"},
    {0x1B, "Hidden FAT32 (CHS)"},
    {0x1C, "Hidden FAT32 (LBA)"},
    {0x1E, "Hidden FAT16 (LBA)"},
    {0x27, "Windows RE / Recovery"},
    {0x39, "Plan 9"},
    {0x3C, "PartitionMagic recovery"},
    {0x42, "Windows LDM Dynamic"},
    {0x4D, "QNX 4.x primary"},
    {0x4E, "QNX 4.x secondary"},
    {0x4F, "QNX 4.x tertiary"},
    {0x52, "CP/M"},
    {0x55, "EZ-Drive"},
    {0x56, "GoldenBow"},
    {0x5C, "Priam EDisk"},
    {0x63, "UNIX System V"},
    {0x64, "Novell Netware 286"},
    {0x65, "Novell Netware 386"},
    {0x75, "PC/IX"},
    {0x80, "Old Minix"},
    {0x81, "Minix / Old Linux"},
    {0x82, "Linux swap / Solaris"},
    {0x83, "Linux filesystem"},
    {0x84, "OS/2 hidden C:"},
    {0x85, "Linux extended"},
    {0x86, "NTFS volume set"},
    {0x87, "NTFS volume set"},
    {0x88, "Linux plaintext"},
    {0x8E, "Linux LVM"},
    {0x93, "Amoeba"},
    {0x94, "Amoeba BBT"},
    {0x9F, "BSD/OS"},
    {0xA0, "IBM Thinkpad hibernation"},
    {0xA5, "FreeBSD"},
    {0xA6, "OpenBSD"},
    {0xA7, "NeXTSTEP"},
    {0xA8, "macOS X UFS"},
    {0xA9, "NetBSD"},
    {0xAB, "macOS X boot"},
    {0xAF, "macOS X HFS+"},
    {0xB6, "Windows NT corrupted FAT16"},
    {0xBE, "Solaris boot"},
    {0xBF, "Solaris"},
    {0xC0, "DRDOS / secured FAT"},
    {0xC1, "DRDOS / secured FAT12"},
    {0xC4, "DRDOS / secured FAT16 <32MB"},
    {0xC6, "DRDOS / secured FAT16"},
    {0xC7, "Syrinx"},
    {0xDA, "Non-FS data"},
    {0xDB, "CP/M / CTOS"},
    {0xDE, "Dell PowerEdge server"},
    {0xDF, "BootIt"},
    {0xE1, "DOS access"},
    {0xE3, "DOS read-only"},
    {0xE4, "SpeedStor"},
    {0xEB, "BeOS BFS"},
    {0xEE, "GPT protective MBR"},
    {0xEF, "EFI system partition"},
    {0xF0, "Linux/PA-RISC boot"},
    {0xF1, "SpeedStor"},
    {0xF4, "SpeedStor large"},
    {0xFB, "VMware VMFS"},
    {0xFC, "VMware VMKCORE"},
    {0xFD, "Linux RAID autodetect"},
    {0xFE, "LANstep"},
    {0xFF, "BBT (Bad Block Table)"},
    {0x00, NULL}    /* sentinel */
};

const char *rdvc_mbr_type_name(rdvc_u8 type)
{
    for (int i = 0; mbr_type_table[i].name != NULL; i++) {
        if (mbr_type_table[i].type == type)
            return mbr_type_table[i].name;
    }
    return "Unknown";
}

/* ─────────────────────────────────────────────────────────────
 *  Little-endian read helpers (safe on any architecture)
 * ───────────────────────────────────────────────────────────── */
static inline rdvc_u16 le16(const rdvc_u8 *p)
{
    return (rdvc_u16)(p[0] | ((rdvc_u16)p[1] << 8));
}

static inline rdvc_u32 le32(const rdvc_u8 *p)
{
    return (rdvc_u32)(p[0] | ((rdvc_u32)p[1] << 8)
                            | ((rdvc_u32)p[2] << 16)
                            | ((rdvc_u32)p[3] << 24));
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_parse_mbr
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_parse_mbr(rdvc_disk_ctx_t *ctx, rdvc_mbr_info_t *out)
{
    if (!ctx || !out)  return RDVC_ERR_NULL_POINTER;
    if (!ctx->is_open) return RDVC_ERR_DISK_NOT_OPEN;

    memset(out, 0, sizeof(*out));

    /* Read sector 0 */
    rdvc_u8 sector[RDVC_SECTOR_SIZE_4096];
    rdvc_err_t e = rdvc_read_sector(ctx, 0, sector);
    if (RDVC_IS_ERR(e)) return e;

    /* Check signature at offset 510 */
    rdvc_u16 sig = le16(sector + 510);
    if (sig != RDVC_MBR_SIGNATURE) {
        out->valid = RDVC_FALSE;
        return RDVC_ERR_MBR_NO_SIGNATURE;
    }

    out->valid      = RDVC_TRUE;
    out->part_count = 0;

    /* Parse 4 partition entries starting at byte 446 */
    for (int i = 0; i < RDVC_MBR_MAX_PARTS; i++) {
        const rdvc_u8 *entry = sector + 446 + (i * 16);

        rdvc_u8  status    = entry[0];
        rdvc_u8  type      = entry[4];
        rdvc_u32 lba_start = le32(entry + 8);
        rdvc_u32 lba_size  = le32(entry + 12);

        if (type == RDVC_PART_TYPE_EMPTY && lba_size == 0)
            continue;

        rdvc_mbr_part_info_t *part = &out->parts[out->part_count];
        part->type        = type;
        part->lba_start   = lba_start;
        part->lba_size    = lba_size;
        part->byte_offset = (rdvc_u64)lba_start * ctx->info.sector_size;
        part->byte_size   = (rdvc_u64)lba_size  * ctx->info.sector_size;
        part->bootable    = (status == 0x80) ? RDVC_TRUE : RDVC_FALSE;
        strncpy(part->type_name, rdvc_mbr_type_name(type),
                sizeof(part->type_name) - 1);

        out->part_count++;
    }

    return RDVC_OK;
}
