/**
 * platform.c
 * RAW DISK VIEWER CPLX — Platform I/O Abstraction Implementation
 *
 * Implements OS-specific disk open/close/seek/read primitives.
 * Only READ-ONLY access is ever used. Any write path is absent.
 */

#include "../include/rdvc_platform.h"
#include "../include/rdvc_errors.h"
#include "../include/rdvc_disk.h"

#include <string.h>

#if defined(RDVC_OS_LINUX) || defined(RDVC_OS_MACOS)
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <errno.h>
    #if defined(RDVC_OS_LINUX)
        #include <linux/fs.h>       /* BLKGETSIZE64, BLKSSZGET */
        #include <linux/hdreg.h>
    #endif
    #if defined(RDVC_OS_MACOS)
        #include <sys/disk.h>       /* DKIOCGETBLOCKSIZE, DKIOCGETBLOCKCOUNT */
    #endif
#endif

/* ─────────────────────────────────────────────────────────────
 *  Error translation
 * ───────────────────────────────────────────────────────────── */
const char *rdvc_strerror(rdvc_err_t err)
{
    switch (err) {
        case RDVC_OK:                      return "Success";
        case RDVC_ERR_GENERIC:             return "Generic error";
        case RDVC_ERR_NULL_POINTER:        return "Null pointer";
        case RDVC_ERR_INVALID_PARAM:       return "Invalid parameter";
        case RDVC_ERR_OUT_OF_MEMORY:       return "Out of memory";
        case RDVC_ERR_BUFFER_TOO_SMALL:    return "Buffer too small";
        case RDVC_ERR_NOT_SUPPORTED:       return "Not supported";
        case RDVC_ERR_TIMEOUT:             return "Timeout";
        case RDVC_ERR_DISK_OPEN:           return "Cannot open disk";
        case RDVC_ERR_DISK_READ:           return "Disk read error";
        case RDVC_ERR_DISK_SEEK:           return "Disk seek error";
        case RDVC_ERR_DISK_NOT_OPEN:       return "Disk not open";
        case RDVC_ERR_DISK_TOO_MANY:       return "Too many disks";
        case RDVC_ERR_DISK_NOT_FOUND:      return "Disk not found";
        case RDVC_ERR_DISK_ACCESS_DENIED:  return "Access denied (run as admin/root)";
        case RDVC_ERR_SECTOR_OUT_OF_RANGE: return "Sector LBA out of range";
        case RDVC_ERR_READONLY:            return "Write attempt in read-only mode";
        case RDVC_ERR_PRIVILEGE:           return "Insufficient privileges";
        case RDVC_ERR_MBR_INVALID:         return "Invalid MBR";
        case RDVC_ERR_MBR_NO_SIGNATURE:    return "MBR signature not found (0xAA55)";
        case RDVC_ERR_GPT_INVALID:         return "Invalid GPT";
        case RDVC_ERR_GPT_CRC_FAIL:        return "GPT CRC32 verification failed";
        case RDVC_ERR_GPT_NO_SIGNATURE:    return "GPT signature not found (EFI PART)";
        case RDVC_ERR_ANALYSIS_FAIL:       return "Analysis failed";
        case RDVC_ERR_CARVER_NO_START:     return "No start signature found";
        case RDVC_ERR_CARVER_NO_END:       return "No end signature found";
        case RDVC_ERR_EXPORT_OPEN:         return "Cannot open export file";
        case RDVC_ERR_EXPORT_WRITE:        return "Export write failed";
        case RDVC_ERR_EXPORT_PATH:         return "Invalid export path";
        default:                            return "Unknown error";
    }
}

/* ─────────────────────────────────────────────────────────────
 *  WINDOWS — Physical disk open
 * ───────────────────────────────────────────────────────────── */
#if defined(RDVC_OS_WINDOWS)

rdvc_err_t rdvc_platform_open(const char *path, rdvc_handle_t *out_handle)
{
    if (!path || !out_handle) return RDVC_ERR_NULL_POINTER;

    HANDLE h = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
        NULL
    );

    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED)   return RDVC_ERR_DISK_ACCESS_DENIED;
        if (e == ERROR_FILE_NOT_FOUND)  return RDVC_ERR_DISK_NOT_FOUND;
        return RDVC_ERR_DISK_OPEN;
    }

    *out_handle = h;
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_close(rdvc_handle_t handle)
{
    if (handle == RDVC_INVALID_HANDLE) return RDVC_ERR_DISK_NOT_OPEN;
    CloseHandle(handle);
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_read(rdvc_handle_t handle,
                               rdvc_u64      byte_offset,
                               rdvc_u8      *buf,
                               rdvc_u32      bytes_to_read,
                               rdvc_u32     *bytes_read)
{
    if (handle == RDVC_INVALID_HANDLE) return RDVC_ERR_DISK_NOT_OPEN;
    if (!buf || !bytes_read)           return RDVC_ERR_NULL_POINTER;

    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)byte_offset;
    if (!SetFilePointerEx(handle, li, NULL, FILE_BEGIN))
        return RDVC_ERR_DISK_SEEK;

    DWORD read = 0;
    if (!ReadFile(handle, buf, (DWORD)bytes_to_read, &read, NULL))
        return RDVC_ERR_DISK_READ;

    *bytes_read = (rdvc_u32)read;
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_get_size(rdvc_handle_t handle,
                                   rdvc_u64     *out_bytes,
                                   rdvc_u32     *out_sector_size)
{
    if (handle == RDVC_INVALID_HANDLE) return RDVC_ERR_DISK_NOT_OPEN;
    if (!out_bytes || !out_sector_size) return RDVC_ERR_NULL_POINTER;

    /* Get disk geometry */
    DISK_GEOMETRY_EX dg;
    DWORD returned = 0;
    if (DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                        NULL, 0, &dg, sizeof(dg), &returned, NULL)) {
        *out_bytes       = (rdvc_u64)dg.DiskSize.QuadPart;
        *out_sector_size = (rdvc_u32)dg.Geometry.BytesPerSector;
        if (*out_sector_size == 0) *out_sector_size = RDVC_SECTOR_SIZE_512;
    } else {
        /* Fallback: use GetFileSizeEx for image files */
        LARGE_INTEGER fs;
        if (!GetFileSizeEx(handle, &fs)) return RDVC_ERR_DISK_READ;
        *out_bytes       = (rdvc_u64)fs.QuadPart;
        *out_sector_size = RDVC_SECTOR_SIZE_512;
    }
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_get_model(rdvc_handle_t handle, char *buf, rdvc_u32 len)
{
    if (!buf || len == 0) return RDVC_ERR_NULL_POINTER;
    /* STORAGE_PROPERTY_QUERY for model string */
    STORAGE_PROPERTY_QUERY query;
    memset(&query, 0, sizeof(query));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType  = PropertyStandardQuery;

    rdvc_u8 desc_buf[512];
    memset(desc_buf, 0, sizeof(desc_buf));
    DWORD returned = 0;
    if (DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
                        &query, sizeof(query),
                        desc_buf, sizeof(desc_buf),
                        &returned, NULL)) {
        STORAGE_DEVICE_DESCRIPTOR *desc = (STORAGE_DEVICE_DESCRIPTOR *)desc_buf;
        if (desc->ProductIdOffset && desc->ProductIdOffset < sizeof(desc_buf)) {
            const char *model = (const char *)desc_buf + desc->ProductIdOffset;
            strncpy(buf, model, (size_t)(len - 1));
            buf[len - 1] = '\0';
            return RDVC_OK;
        }
    }
    strncpy(buf, "Unknown", (size_t)(len - 1));
    buf[len - 1] = '\0';
    return RDVC_OK;
}

/* ─────────────────────────────────────────────────────────────
 *  LINUX / MACOS — Physical disk open
 * ───────────────────────────────────────────────────────────── */
#else /* RDVC_OS_LINUX || RDVC_OS_MACOS */

rdvc_err_t rdvc_platform_open(const char *path, rdvc_handle_t *out_handle)
{
    if (!path || !out_handle) return RDVC_ERR_NULL_POINTER;

    int fd = open(path, O_RDONLY | O_LARGEFILE);
    if (fd < 0) {
        if (errno == EACCES || errno == EPERM) return RDVC_ERR_DISK_ACCESS_DENIED;
        if (errno == ENOENT)                    return RDVC_ERR_DISK_NOT_FOUND;
        return RDVC_ERR_DISK_OPEN;
    }

    *out_handle = fd;
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_close(rdvc_handle_t handle)
{
    if (handle < 0) return RDVC_ERR_DISK_NOT_OPEN;
    close(handle);
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_read(rdvc_handle_t handle,
                               rdvc_u64      byte_offset,
                               rdvc_u8      *buf,
                               rdvc_u32      bytes_to_read,
                               rdvc_u32     *bytes_read)
{
    if (handle < 0)                return RDVC_ERR_DISK_NOT_OPEN;
    if (!buf || !bytes_read)       return RDVC_ERR_NULL_POINTER;

    if (lseek64(handle, (off64_t)byte_offset, SEEK_SET) < 0)
        return RDVC_ERR_DISK_SEEK;

    ssize_t r = read(handle, buf, (size_t)bytes_to_read);
    if (r < 0) return RDVC_ERR_DISK_READ;

    *bytes_read = (rdvc_u32)r;
    return RDVC_OK;
}

rdvc_err_t rdvc_platform_get_size(rdvc_handle_t handle,
                                   rdvc_u64     *out_bytes,
                                   rdvc_u32     *out_sector_size)
{
    if (handle < 0)                  return RDVC_ERR_DISK_NOT_OPEN;
    if (!out_bytes || !out_sector_size) return RDVC_ERR_NULL_POINTER;

#if defined(RDVC_OS_LINUX)
    rdvc_u64 sz = 0;
    rdvc_u32 ssz = RDVC_SECTOR_SIZE_512;
    ioctl(handle, BLKGETSIZE64, &sz);
    ioctl(handle, BLKSSZGET, &ssz);
    if (sz == 0) {
        /* Fallback: seek to end */
        off64_t end = lseek64(handle, 0, SEEK_END);
        sz = (rdvc_u64)end;
        lseek64(handle, 0, SEEK_SET);
    }
    *out_bytes       = sz;
    *out_sector_size = ssz ? ssz : RDVC_SECTOR_SIZE_512;
#elif defined(RDVC_OS_MACOS)
    rdvc_u32 bsize = RDVC_SECTOR_SIZE_512;
    rdvc_u64 bcount = 0;
    ioctl(handle, DKIOCGETBLOCKSIZE, &bsize);
    ioctl(handle, DKIOCGETBLOCKCOUNT, &bcount);
    *out_bytes       = bsize * bcount;
    *out_sector_size = bsize;
#endif

    return RDVC_OK;
}

rdvc_err_t rdvc_platform_get_model(rdvc_handle_t handle, char *buf, rdvc_u32 len)
{
    (void)handle;
    if (!buf || len == 0) return RDVC_ERR_NULL_POINTER;
    strncpy(buf, "Physical Disk", (size_t)(len - 1));
    buf[len - 1] = '\0';
    return RDVC_OK;
}

#endif /* OS selection */
