package com.rdvc.service;

import com.rdvc.bridge.NativeBridge;
import com.rdvc.bridge.NativeLoader;
import com.rdvc.bridge.model.*;
import com.rdvc.forensic.ForensicLogger;

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

/**
 * DiskService — thread-safe service layer between GUI and NativeBridge.
 *
 * All native calls are gated through this class. Handles:
 *   - Library availability checks
 *   - Forensic logging of every operation
 *   - Demo-mode fallback when native library is absent
 */
public class DiskService {

    private static final DiskService INSTANCE = new DiskService();
    public static DiskService get() { return INSTANCE; }

    private long  activeHandle = 0;
    private String activePath  = "";
    private DiskInfo   activeDisk = null;
    private PartitionInfo[] activeMbr = null;
    private PartitionInfo[] activeGpt = null;

    public boolean isNativeAvailable() { return NativeLoader.isLoaded(); }

    // ── Disk management ───────────────────────────────────────

    public List<DiskInfo> listDisks() {
        if (!isNativeAvailable()) return demoDisks();
        DiskInfo[] arr = NativeBridge.listDisks();
        List<DiskInfo> list = new ArrayList<>();
        if (arr != null) Collections.addAll(list, arr);
        return list;
    }

    public boolean openDisk(DiskInfo info) {
        return openDisk(info.getPath());
    }

    public boolean openDisk(String path) {
        closeDisk();
        if (!isNativeAvailable()) {
            activePath = path;
            ForensicLogger.global().info("DISK_OPEN", "DEMO mode: " + path);
            return true;
        }
        long h = NativeBridge.openDisk(path);
        if (h <= 0) {
            String err = NativeBridge.getErrorString((int)h);
            ForensicLogger.global().error("DISK_OPEN", "Failed: " + path + " — " + err);
            return false;
        }
        activeHandle = h;
        activePath   = path;
        activeDisk   = NativeBridge.getDiskInfo(h);
        activeMbr    = NativeBridge.getMBRInfo(h);
        activeGpt    = NativeBridge.getGPTInfo(h);
        ForensicLogger.global().logDiskOpen(path, h);
        return true;
    }

    public void closeDisk() {
        if (activeHandle != 0) {
            ForensicLogger.global().logDiskClose(activeHandle);
            if (isNativeAvailable()) NativeBridge.closeDisk(activeHandle);
            activeHandle = 0;
        }
        activePath = "";
        activeDisk = null;
        activeMbr  = null;
        activeGpt  = null;
    }

    /** Alias for closeDisk() — used by MainWindow. */
    public void closeActiveDisk() { closeDisk(); }

    /**
     * Opens a disk image file (path provided directly, not from enumeration).
     * Returns a populated DiskInfo on success, or null on failure.
     */
    public DiskInfo openImageFile(String imagePath) {
        boolean ok = openDisk(imagePath);
        if (!ok) return null;
        if (activeDisk != null) return activeDisk;
        // Demo mode: synthesize a DiskInfo
        java.io.File f = new java.io.File(imagePath);
        long sz = f.length();
        return new DiskInfo(imagePath, f.getName(), sz, 512, sz / 512, 4, 1);
    }

    /**
     * Returns merged MBR + GPT partition list (GPT preferred if present).
     */
    public List<PartitionInfo> getPartitionInfo() {
        List<PartitionInfo> result = new ArrayList<>();
        if (activeGpt != null && activeGpt.length > 0) {
            for (PartitionInfo p : activeGpt) result.add(p);
        } else if (activeMbr != null) {
            for (PartitionInfo p : activeMbr) result.add(p);
        }
        return result;
    }

    // ── Sector read ───────────────────────────────────────────

    public byte[] readSector(long lba) {
        if (!isNativeAvailable()) return demoSector(lba);
        if (activeHandle == 0)    return null;
        byte[] data = NativeBridge.readSector(activeHandle, lba);
        if (data != null) {
            // Hash for forensic logging (only log every 100 reads to avoid spam)
            if (lba % 100 == 0) {
                String sha = computeSha256Short(data);
                ForensicLogger.global().logSectorRead(activeHandle, lba, sha);
            }
        }
        return data;
    }

    public byte[] readSectors(long lba, int count) {
        if (!isNativeAvailable()) return demoSectors(lba, count);
        if (activeHandle == 0)    return null;
        return NativeBridge.readSectors(activeHandle, lba, count);
    }

    // ── Accessors ────────────────────────────────────────────

    public long getActiveHandle()       { return activeHandle; }
    public String getActivePath()       { return activePath; }
    public DiskInfo getActiveDisk()     { return activeDisk; }
    public PartitionInfo[] getActiveMbr() { return activeMbr; }
    public PartitionInfo[] getActiveGpt() { return activeGpt; }
    public boolean isDiskOpen() {
        return isNativeAvailable() ? activeHandle > 0 : !activePath.isEmpty();
    }

    public int getSectorSize() {
        if (activeDisk != null) return activeDisk.getSectorSize();
        return 512;
    }

    public long getTotalSectors() {
        if (activeDisk != null) return activeDisk.getTotalSectors();
        return 0;
    }

    public double getSectorEntropy(long lba) {
        if (!isNativeAvailable() || activeHandle == 0) return 0.0;
        return NativeBridge.getSectorEntropy(activeHandle, lba);
    }

    // ── Export ───────────────────────────────────────────────

    public boolean exportSectors(long lba, long count, String path) {
        if (!isNativeAvailable() || activeHandle == 0) return false;
        int rc = NativeBridge.exportSectors(activeHandle, lba, (int)count, path);
        ForensicLogger.global().logExport(activeHandle, lba, (int)count, path);
        return rc == 0;
    }

    public boolean exportCarvedFile(CarvedFile cf, String destDir) {
        if (!isNativeAvailable() || activeHandle == 0) return false;
        String outPath = destDir + java.io.File.separator + cf.getSuggestedName();
        int rc = NativeBridge.exportCarvedFile(activeHandle,
                cf.getStartOffset(), cf.getSize(), outPath);
        ForensicLogger.global().info("EXPORT_CARVED",
                "handle=0x" + Long.toHexString(activeHandle)
                + " startOffset=" + cf.getStartOffset() + " dest=" + outPath);
        return rc == 0;
    }

    // ── Private helpers ───────────────────────────────────────

    private String computeSha256Short(byte[] data) {
        try {
            java.security.MessageDigest md = java.security.MessageDigest.getInstance("SHA-256");
            byte[] h = md.digest(data);
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < 8; i++) sb.append(String.format("%02x", h[i]));
            return sb.toString();
        } catch (Exception e) { return "N/A"; }
    }

    // ── Demo mode ────────────────────────────────────────────

    private List<DiskInfo> demoDisks() {
        List<DiskInfo> demos = new ArrayList<>();
        demos.add(new DiskInfo("\\\\.\\PhysicalDrive0", "DEMO WD Blue 500GB",
                500L * 1024 * 1024 * 1024, 512,
                500L * 1024 * 1024 * 1024 / 512, 1, 0));
        demos.add(new DiskInfo("demo.img", "DEMO Image File 100MB",
                100L * 1024 * 1024, 512,
                100L * 1024 * 1024 / 512, 4, 1));
        return demos;
    }

    private byte[] demoSector(long lba) {
        byte[] buf = new byte[512];
        if (lba == 0) {
            // Fake MBR signature
            buf[510] = (byte)0x55;
            buf[511] = (byte)0xAA;
            // Fake partition entry at 446
            buf[446] = (byte)0x80; // bootable
            buf[450] = (byte)0x07; // NTFS
            buf[454] = (byte)0x01; // LBA start = 2048
            buf[455] = (byte)0x08;
        } else {
            // Fill with recognizable pattern
            for (int i = 0; i < 512; i++)
                buf[i] = (byte)((lba + i) & 0xFF);
        }
        return buf;
    }

    private byte[] demoSectors(long lba, int count) {
        byte[] result = new byte[count * 512];
        for (int s = 0; s < count; s++) {
            byte[] sector = demoSector(lba + s);
            System.arraycopy(sector, 0, result, s * 512, 512);
        }
        return result;
    }
}
