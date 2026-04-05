/**
 * signatures.c
 * RAW DISK VIEWER CPLX — File Signature Database & Scanner
 *
 * 55+ file type signatures covering forensic-relevant formats.
 */

#include "../include/rdvc_signatures.h"
#include "../include/rdvc_platform.h"

#include <string.h>

/* ─────────────────────────────────────────────────────────────
 *  Static signature database
 *  Fields: magic[], magic_len, magic_offset, eof[], eof_len, max_size, ext, mime, desc
 * ───────────────────────────────────────────────────────────── */
static const rdvc_signature_t g_signatures[] = {

    /* ── Images ─────────────────────────────────────────────── */
    {{0xFF,0xD8,0xFF}, 3, 0, {0xFF,0xD9}, 2, 20*1024*1024,
        "jpg", "image/jpeg", "JPEG Image"},

    {{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A}, 8, 0,
        {0x49,0x45,0x4E,0x44,0xAE,0x42,0x60,0x82}, 8, 10*1024*1024,
        "png", "image/png", "PNG Image"},

    {{0x47,0x49,0x46,0x38,0x37,0x61}, 6, 0, {0x00,0x3B}, 2, 5*1024*1024,
        "gif", "image/gif", "GIF87a Image"},

    {{0x47,0x49,0x46,0x38,0x39,0x61}, 6, 0, {0x00,0x3B}, 2, 5*1024*1024,
        "gif", "image/gif", "GIF89a Image"},

    {{0x42,0x4D}, 2, 0, {0}, 0, 50*1024*1024,
        "bmp", "image/bmp", "BMP Bitmap"},

    {{0x49,0x49,0x2A,0x00}, 4, 0, {0}, 0, 100*1024*1024,
        "tif", "image/tiff", "TIFF (little-endian)"},

    {{0x4D,0x4D,0x00,0x2A}, 4, 0, {0}, 0, 100*1024*1024,
        "tif", "image/tiff", "TIFF (big-endian)"},

    {{0x00,0x00,0x01,0x00}, 4, 0, {0}, 0, 1024*1024,
        "ico", "image/x-icon", "Windows Icon"},

    {{0x38,0x42,0x50,0x53}, 4, 0, {0}, 0, 500*1024*1024,
        "psd", "image/vnd.adobe.photoshop", "Adobe Photoshop"},

    {{0x57,0x45,0x42,0x50}, 4, 8, {0}, 0, 50*1024*1024,
        "webp", "image/webp", "WebP Image"},

    /* ── Documents ───────────────────────────────────────────── */
    {{0x25,0x50,0x44,0x46}, 4, 0, {0x25,0x25,0x45,0x4F,0x46}, 5, 500*1024*1024,
        "pdf", "application/pdf", "PDF Document"},

    {{0xD0,0xCF,0x11,0xE0,0xA1,0xB1,0x1A,0xE1}, 8, 0, {0}, 0, 100*1024*1024,
        "doc", "application/msword", "MS Office OLE2 (doc/xls/ppt)"},

    {{0x50,0x4B,0x03,0x04}, 4, 0, {0x50,0x4B,0x05,0x06}, 4, 500*1024*1024,
        "zip", "application/zip", "ZIP / Office Open XML / JAR"},

    {{0x7B,0x5C,0x72,0x74,0x66}, 5, 0, {0}, 0, 50*1024*1024,
        "rtf", "application/rtf", "Rich Text Format"},

    /* ── Archives ────────────────────────────────────────────── */
    {{0x52,0x61,0x72,0x21,0x1A,0x07,0x00}, 7, 0, {0}, 0, 4ULL*1024*1024*1024,
        "rar", "application/x-rar", "RAR Archive v1.5-4.x"},

    {{0x52,0x61,0x72,0x21,0x1A,0x07,0x01,0x00}, 8, 0, {0}, 0, 4ULL*1024*1024*1024,
        "rar", "application/x-rar", "RAR Archive v5+"},

    {{0x1F,0x8B}, 2, 0, {0}, 0, 4ULL*1024*1024*1024,
        "gz", "application/gzip", "GZIP Compressed"},

    {{0x42,0x5A,0x68}, 3, 0, {0x59,0x41,0x59,0x26,0x53,0x59}, 6, 4ULL*1024*1024*1024,
        "bz2", "application/x-bzip2", "BZIP2 Compressed"},

    {{0xFD,0x37,0x7A,0x58,0x5A,0x00}, 6, 0, {0}, 0, 4ULL*1024*1024*1024,
        "xz", "application/x-xz", "XZ Compressed"},

    {{0x37,0x7A,0xBC,0xAF,0x27,0x1C}, 6, 0, {0}, 0, 4ULL*1024*1024*1024,
        "7z", "application/x-7z-compressed", "7-Zip Archive"},

    {{0x04,0x22,0x4D,0x18}, 4, 0, {0}, 0, 4ULL*1024*1024*1024,
        "lz4", "application/x-lz4", "LZ4 Compressed"},

    {{0x1F,0xA0}, 2, 0, {0}, 0, 4ULL*1024*1024*1024,
        "z", "application/x-compress", "LZW Compressed (.Z)"},

    /* ── Executables ─────────────────────────────────────────── */
    {{0x4D,0x5A}, 2, 0, {0}, 0, 500*1024*1024,
        "exe", "application/x-msdownload", "Windows PE Executable (MZ)"},

    {{0x7F,0x45,0x4C,0x46}, 4, 0, {0}, 0, 500*1024*1024,
        "elf", "application/x-elf", "Linux/Unix ELF Executable"},

    {{0xCE,0xFA,0xED,0xFE}, 4, 0, {0}, 0, 500*1024*1024,
        "macho", "application/x-mach-binary", "Mach-O 32-bit (LE)"},

    {{0xCF,0xFA,0xED,0xFE}, 4, 0, {0}, 0, 500*1024*1024,
        "macho", "application/x-mach-binary", "Mach-O 64-bit (LE)"},

    {{0xFE,0xED,0xFA,0xCE}, 4, 0, {0}, 0, 500*1024*1024,
        "macho", "application/x-mach-binary", "Mach-O 32-bit (BE)"},

    {{0xFE,0xED,0xFA,0xCF}, 4, 0, {0}, 0, 500*1024*1024,
        "macho", "application/x-mach-binary", "Mach-O 64-bit (BE)"},

    {{0xCA,0xFE,0xBA,0xBE}, 4, 0, {0}, 0, 500*1024*1024,
        "class", "application/java-vm", "Java Class File"},

    {{0xDE,0xC0,0x17,0x0B}, 4, 0, {0}, 0, 500*1024*1024,
        "bc", "application/x-llvm-bc", "LLVM Bitcode"},

    /* ── Media ───────────────────────────────────────────────── */
    {{0x00,0x00,0x00,0x18,0x66,0x74,0x79,0x70}, 8, 0, {0}, 0, 4ULL*1024*1024*1024,
        "mp4", "video/mp4", "MPEG-4 Video"},

    {{0x00,0x00,0x00,0x20,0x66,0x74,0x79,0x70}, 8, 0, {0}, 0, 4ULL*1024*1024*1024,
        "mp4", "video/mp4", "MPEG-4 Video (ftyp)"},

    {{0x49,0x44,0x33}, 3, 0, {0}, 0, 50*1024*1024,
        "mp3", "audio/mpeg", "MP3 Audio (ID3)"},

    {{0xFF,0xFB}, 2, 0, {0}, 0, 50*1024*1024,
        "mp3", "audio/mpeg", "MP3 Audio"},

    {{0x52,0x49,0x46,0x46}, 4, 0, {0}, 0, 500*1024*1024,
        "wav", "audio/wav", "WAV Audio (RIFF)"},

    {{0x4F,0x67,0x67,0x53}, 4, 0, {0}, 0, 500*1024*1024,
        "ogg", "audio/ogg", "OGG Container"},

    {{0x66,0x4C,0x61,0x43}, 4, 0, {0}, 0, 500*1024*1024,
        "flac", "audio/flac", "FLAC Audio"},

    {{0x1A,0x45,0xDF,0xA3}, 4, 0, {0}, 0, 4ULL*1024*1024*1024,
        "mkv", "video/x-matroska", "Matroska / WebM Video"},

    {{0x00,0x00,0x01,0xB3}, 4, 0, {0}, 0, 4ULL*1024*1024*1024,
        "mpg", "video/mpeg", "MPEG Video"},

    {{0x52,0x49,0x46,0x46}, 4, 0, {0}, 0, 4ULL*1024*1024*1024,
        "avi", "video/x-msvideo", "AVI Video (RIFF)"},

    /* ── Disk Images / Forensic ──────────────────────────────── */
    {{0x45,0x56,0x46,0x32}, 4, 0, {0}, 0, 0,
        "E01", "application/octet-stream", "EnCase Evidence File v2"},

    {{0x41,0x46,0x46}, 3, 0, {0}, 0, 0,
        "AFF", "application/octet-stream", "Advanced Forensic Format"},

    {{0x4B,0x44,0x4D}, 3, 0, {0}, 0, 0,
        "vmdk", "application/octet-stream", "VMware VMDK"},

    {{0x63,0x6F,0x6E,0x65,0x63,0x74,0x69,0x78}, 8, 0, {0}, 0, 0,
        "vmdk", "application/octet-stream", "VMware VMDK (sparse)"},

    {{0x3C,0x3C,0x3C,0x20,0x4F,0x72,0x61,0x63}, 8, 0, {0}, 0, 0,
        "vdi", "application/octet-stream", "VirtualBox VDI"},

    /* ── Databases ───────────────────────────────────────────── */
    {{0x53,0x51,0x4C,0x69,0x74,0x65,0x20,0x66,0x6F,0x72,0x6D,0x61,0x74,0x20,0x33,0x00}, 16, 0,
        {0}, 0, 4ULL*1024*1024*1024,
        "db", "application/x-sqlite3", "SQLite3 Database"},

    /* ── Certificates & Crypto ───────────────────────────────── */
    {{0x30,0x82}, 2, 0, {0}, 0, 10*1024*1024,
        "der", "application/x-x509-ca-cert", "DER Encoded Certificate"},

    {{0x2D,0x2D,0x2D,0x2D,0x2D,0x42,0x45,0x47,0x49,0x4E}, 10, 0,
        {0x2D,0x2D,0x2D,0x2D,0x2D,0x45,0x4E,0x44}, 8, 10*1024*1024,
        "pem", "application/x-pem-file", "PEM Certificate/Key"},

    /* ── Scripts / Text ──────────────────────────────────────── */
    {{0x23,0x21,0x2F,0x62,0x69,0x6E,0x2F}, 7, 0, {0}, 0, 50*1024*1024,
        "sh", "application/x-sh", "Shell Script (#!/bin)"},

    {{0x23,0x21,0x2F,0x75,0x73,0x72}, 6, 0, {0}, 0, 50*1024*1024,
        "sh", "application/x-sh", "Script (#!/usr)"},

    {{0x3C,0x3F,0x78,0x6D,0x6C}, 5, 0, {0}, 0, 50*1024*1024,
        "xml", "application/xml", "XML Document"},

    {{0x3C,0x21,0x44,0x4F,0x43,0x54,0x59,0x50,0x45}, 9, 0, {0}, 0, 50*1024*1024,
        "html", "text/html", "HTML Document"},

    /* ── Registry & Windows artifacts ───────────────────────── */
    {{0x72,0x65,0x67,0x66}, 4, 0, {0}, 0, 500*1024*1024,
        "hive", "application/octet-stream", "Windows Registry Hive"},

    {{0x50,0x41,0x47,0x45}, 4, 4, {0}, 0, 100*1024*1024,
        "lnk", "application/x-ms-shortcut", "Windows Page File"},

    /* ── Email ───────────────────────────────────────────────── */
    {{0x46,0x72,0x6F,0x6D,0x20,0x20}, 6, 0, {0}, 0, 100*1024*1024,
        "mbox", "application/mbox", "MBOX Email Archive"},

    /* ── Crypto wallets ──────────────────────────────────────── */
    {{0x62,0x31,0x05,0x00}, 4, 0, {0}, 0, 10*1024*1024,
        "wallet", "application/octet-stream", "Bitcoin Core Wallet"},
};

#define RDVC_SIGNATURE_COUNT (sizeof(g_signatures) / sizeof(g_signatures[0]))

/* ─────────────────────────────────────────────────────────────
 *  rdvc_get_signatures
 * ───────────────────────────────────────────────────────────── */
const rdvc_signature_t *rdvc_get_signatures(rdvc_u32 *out_count)
{
    if (out_count) *out_count = (rdvc_u32)RDVC_SIGNATURE_COUNT;
    return g_signatures;
}

/* ─────────────────────────────────────────────────────────────
 *  rdvc_scan_buffer
 *
 *  Scans 'buf' for any known signature magic.
 *  Uses a sliding window of max_magic_len bytes.
 * ───────────────────────────────────────────────────────────── */
rdvc_err_t rdvc_scan_buffer(const rdvc_u8     *buf,
                             rdvc_u64           buf_len,
                             rdvc_u64           base_offset,
                             rdvc_u32           sector_size,
                             rdvc_sig_match_t  *out_matches,
                             rdvc_u32           max_matches,
                             rdvc_u32          *out_count)
{
    if (!buf || !out_matches || !out_count) return RDVC_ERR_NULL_POINTER;

    *out_count = 0;
    if (sector_size == 0) sector_size = RDVC_SECTOR_SIZE_512;

    for (rdvc_u64 i = 0; i < buf_len && *out_count < max_matches; i++) {
        for (rdvc_u32 s = 0; s < RDVC_SIGNATURE_COUNT && *out_count < max_matches; s++) {
            const rdvc_signature_t *sig = &g_signatures[s];
            rdvc_u32 moff = sig->magic_offset;

            /* The magic_offset is relative to the file start;
               here we apply it as the position within the sector */
            if (i < moff) continue;
            rdvc_u64 check_pos = i - moff;

            if (check_pos + sig->magic_len > buf_len) continue;
            if (memcmp(buf + check_pos, sig->magic, sig->magic_len) != 0) continue;

            rdvc_u64 abs_offset = base_offset + check_pos;

            /* Avoid duplicate matches: check if same sig+offset already found */
            rdvc_bool dup = RDVC_FALSE;
            for (rdvc_u32 k = 0; k < *out_count; k++) {
                if (out_matches[k].byte_offset == abs_offset &&
                    out_matches[k].sig == sig) {
                    dup = RDVC_TRUE;
                    break;
                }
            }
            if (dup) continue;

            rdvc_sig_match_t *m = &out_matches[*out_count];
            m->byte_offset       = abs_offset;
            m->lba               = abs_offset / sector_size;
            m->offset_in_sector  = (rdvc_u32)(abs_offset % sector_size);
            m->sig               = sig;
            (*out_count)++;
        }
    }

    return RDVC_OK;
}
