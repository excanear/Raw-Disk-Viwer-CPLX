/**
 * rdvc_platform.h
 * RAW DISK VIEWER CPLX — Platform Abstraction Layer
 *
 * Detects OS and defines portable types, compile-time checks,
 * and platform-specific file/disk handle types.
 */

#ifndef RDVC_PLATFORM_H
#define RDVC_PLATFORM_H

/* ─────────────────────────────────────────────────────────────
 *  Compiler & OS detection
 * ───────────────────────────────────────────────────────────── */
#if defined(_WIN32) || defined(_WIN64)
    #define RDVC_OS_WINDOWS 1
#elif defined(__linux__)
    #define RDVC_OS_LINUX   1
#elif defined(__APPLE__)
    #define RDVC_OS_MACOS   1
#else
    #error "Unsupported operating system."
#endif

/* ─────────────────────────────────────────────────────────────
 *  Portable integer types (avoid stdint.h dependency issues on MSVC)
 * ───────────────────────────────────────────────────────────── */
#if defined(RDVC_OS_WINDOWS)
    #include <windows.h>
    typedef unsigned __int8   rdvc_u8;
    typedef unsigned __int16  rdvc_u16;
    typedef unsigned __int32  rdvc_u32;
    typedef unsigned __int64  rdvc_u64;
    typedef signed   __int8   rdvc_i8;
    typedef signed   __int16  rdvc_i16;
    typedef signed   __int32  rdvc_i32;
    typedef signed   __int64  rdvc_i64;
    /* Windows disk handle */
    typedef HANDLE            rdvc_handle_t;
    #define RDVC_INVALID_HANDLE INVALID_HANDLE_VALUE
#else
    #include <stdint.h>
    #include <sys/types.h>
    typedef uint8_t   rdvc_u8;
    typedef uint16_t  rdvc_u16;
    typedef uint32_t  rdvc_u32;
    typedef uint64_t  rdvc_u64;
    typedef int8_t    rdvc_i8;
    typedef int16_t   rdvc_i16;
    typedef int32_t   rdvc_i32;
    typedef int64_t   rdvc_i64;
    /* POSIX file descriptor */
    typedef int       rdvc_handle_t;
    #define RDVC_INVALID_HANDLE (-1)
#endif

/* ─────────────────────────────────────────────────────────────
 *  Struct packing macros
 * ───────────────────────────────────────────────────────────── */
#if defined(RDVC_OS_WINDOWS)
    /* On Windows, MSVC supports __pragma(pack), but GCC/Clang (MinGW)
     * should use the __attribute__((packed)) form. Detect MSVC specifically.
     */
    #if defined(_MSC_VER)
        #define RDVC_PACKED_BEGIN  __pragma(pack(push, 1))
        #define RDVC_PACKED_END    __pragma(pack(pop))
        #define RDVC_PACKED_ATTR
    #else
        #define RDVC_PACKED_BEGIN
        #define RDVC_PACKED_END
        #define RDVC_PACKED_ATTR   __attribute__((packed))
    #endif
#else
    #define RDVC_PACKED_BEGIN
    #define RDVC_PACKED_END
    #define RDVC_PACKED_ATTR   __attribute__((packed))
#endif

/* ─────────────────────────────────────────────────────────────
 *  Export macros (for shared library / DLL)
 * ───────────────────────────────────────────────────────────── */
#if defined(RDVC_OS_WINDOWS)
    #ifdef RDVC_BUILDING_DLL
        #define RDVC_API __declspec(dllexport)
    #else
        #define RDVC_API __declspec(dllimport)
    #endif
#else
    #define RDVC_API __attribute__((visibility("default")))
#endif

/* ─────────────────────────────────────────────────────────────
 *  Boolean type
 * ───────────────────────────────────────────────────────────── */
#ifndef __cplusplus
    #if defined(RDVC_OS_WINDOWS)
        typedef int rdvc_bool;
    #else
        #include <stdbool.h>
        typedef bool rdvc_bool;
    #endif
    #define RDVC_TRUE  1
    #define RDVC_FALSE 0
#else
    typedef bool rdvc_bool;
    #define RDVC_TRUE  true
    #define RDVC_FALSE false
#endif

/* ─────────────────────────────────────────────────────────────
 *  Common constants
 * ───────────────────────────────────────────────────────────── */
#define RDVC_SECTOR_SIZE_512   512U
#define RDVC_SECTOR_SIZE_4096  4096U
#define RDVC_MAX_PATH          4096U
#define RDVC_MAX_DISKS         64U

#endif /* RDVC_PLATFORM_H */
