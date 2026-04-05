package com.rdvc.bridge.model;

/**
 * DiskInfo — mirrors rdvc_disk_info_t from the C core.
 * Populated by NativeBridge.listDisks().
 */
public class DiskInfo {

    public static final int TYPE_UNKNOWN   = 0;
    public static final int TYPE_HDD       = 1;
    public static final int TYPE_SSD       = 2;
    public static final int TYPE_REMOVABLE = 3;
    public static final int TYPE_IMAGE     = 4;
    public static final int TYPE_VIRTUAL   = 5;

    private final String path;
    private final String model;
    private final long   totalBytes;
    private final int    sectorSize;
    private final long   totalSectors;
    private final int    type;
    private final int    index;

    public DiskInfo(String path, String model, long totalBytes,
                    int sectorSize, long totalSectors, int type, int index) {
        this.path         = path;
        this.model        = model;
        this.totalBytes   = totalBytes;
        this.sectorSize   = sectorSize;
        this.totalSectors = totalSectors;
        this.type         = type;
        this.index        = index;
    }

    public String getPath()         { return path; }
    public String getModel()        { return model; }
    public long   getTotalBytes()   { return totalBytes; }
    public int    getSectorSize()   { return sectorSize; }
    public long   getTotalSectors() { return totalSectors; }
    public int    getType()         { return type; }
    public int    getIndex()        { return index; }
    public boolean isImage()        { return type == TYPE_IMAGE; }

    public String getTypeName() {
        return switch (type) {
            case TYPE_HDD       -> "HDD";
            case TYPE_SSD       -> "SSD";
            case TYPE_REMOVABLE -> "Removable";
            case TYPE_IMAGE     -> "Image File";
            case TYPE_VIRTUAL   -> "Virtual";
            default             -> "Unknown";
        };
    }

    public String getFormattedSize() {
        if (totalBytes <= 0) return "Unknown";
        double gb = totalBytes / (1024.0 * 1024.0 * 1024.0);
        if (gb >= 1.0) return String.format("%.2f GB", gb);
        double mb = totalBytes / (1024.0 * 1024.0);
        if (mb >= 1.0) return String.format("%.2f MB", mb);
        return String.format("%d bytes", totalBytes);
    }

    @Override
    public String toString() {
        return String.format("[%d] %s — %s (%s)", index, model, getFormattedSize(), path);
    }
}
