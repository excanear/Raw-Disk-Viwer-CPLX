package com.rdvc.bridge;

import com.rdvc.bridge.model.*;

/**
 * NativeBridge — Java declarations for all JNI native methods.
 *
 * The actual implementations live in jni/src/bridge_*.c files.
 * Loading the native library is handled by NativeLoader.
 */
public class NativeBridge {

    /* ── Disk operations ─────────────────────────────────────── */

    /**
     * List all physical disks available on the system.
     * @return Array of DiskInfo, or empty array if none found.
     */
    public static native DiskInfo[] listDisks();

    /**
     * Open a disk or image file for reading.
     * @param path Path to disk (e.g. "\\.\PhysicalDrive0") or image file.
     * @return Native handle (pointer cast to long). Negative on error.
     */
    public static native long openDisk(String path);

    /**
     * Close a previously opened disk handle.
     * @param handle Handle returned by openDisk().
     * @return 0 on success, negative error code on failure.
     */
    public static native int closeDisk(long handle);

    /**
     * Read a single sector from the disk.
     * @param handle    Open disk handle.
     * @param lba       Logical block address (0-based).
     * @return Sector bytes, or null on error.
     */
    public static native byte[] readSector(long handle, long lba);

    /**
     * Read multiple consecutive sectors.
     * @param handle Open disk handle.
     * @param lba    Starting LBA.
     * @param count  Number of sectors to read.
     * @return All sector bytes concatenated, or null on error.
     */
    public static native byte[] readSectors(long handle, long lba, int count);

    /**
     * Get disk metadata for an open handle.
     * @param handle Open disk handle.
     * @return DiskInfo or null on error.
     */
    public static native DiskInfo getDiskInfo(long handle);

    /* ── Partition parsing ───────────────────────────────────── */

    /**
     * Parse MBR from an open disk.
     * @param handle Open disk handle.
     * @return Array of MBR PartitionInfo entries, or null if invalid.
     */
    public static native PartitionInfo[] getMBRInfo(long handle);

    /**
     * Parse GPT from an open disk.
     * @param handle Open disk handle.
     * @return Array of GPT PartitionInfo entries, or null if not GPT.
     */
    public static native PartitionInfo[] getGPTInfo(long handle);

    /**
     * Get the raw 512-byte MBR sector as a hex string for display.
     * @param handle Open disk handle.
     * @return Hex dump of sector 0.
     */
    public static native byte[] getRawMBR(long handle);

    /* ── Scanning ────────────────────────────────────────────── */

    /**
     * Scan a range of sectors for known file signatures.
     * @param handle    Open disk handle.
     * @param startLba  First sector to scan.
     * @param endLba    Last sector to scan (inclusive). 0 = scan to end.
     * @return Array of ScanResult entries found.
     */
    public static native ScanResult[] runScan(long handle, long startLba, long endLba);

    /**
     * Run the file carver over a sector range.
     * @param handle    Open disk handle.
     * @param startLba  First sector.
     * @param endLba    Last sector (0 = to end).
     * @return Array of CarvedFile entries found.
     */
    public static native CarvedFile[] runCarver(long handle, long startLba, long endLba);

    /* ── Export ──────────────────────────────────────────────── */

    /**
     * Export a range of sectors to a file.
     * @param handle     Open disk handle.
     * @param lbaStart   First sector.
     * @param count      Number of sectors.
     * @param outputPath Destination file path.
     * @return 0 on success, negative error code on failure.
     */
    public static native int exportSectors(long handle, long lbaStart,
                                           int count, String outputPath);

    /**
     * Export a carved file to disk.
     * @param handle       Open disk handle.
     * @param startOffset  Byte start of the carved file.
     * @param size         Byte size of the carved file.
     * @param outputPath   Destination file path.
     * @return 0 on success, negative error code on failure.
     */
    public static native int exportCarvedFile(long handle, long startOffset,
                                              long size, String outputPath);

    /* ── Analysis ────────────────────────────────────────────── */

    /**
     * Compute Shannon entropy for a sector.
     * @param handle Open disk handle.
     * @param lba    Sector LBA.
     * @return Entropy value [0.0, 8.0], or -1.0 on error.
     */
    public static native double getSectorEntropy(long handle, long lba);

    /**
     * Get human-readable error string for a native error code.
     * @param errorCode Negative error code returned by other calls.
     * @return Error message string.
     */
    public static native String getErrorString(int errorCode);
}
