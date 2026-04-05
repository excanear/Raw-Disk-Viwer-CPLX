package com.rdvc.bridge.model;

/**
 * ScanResult — one signature match found during a disk scan.
 */
public class ScanResult {

    private final long   byteOffset;
    private final long   lba;
    private final int    offsetInSector;
    private final String extension;
    private final String mimeType;
    private final String description;
    private final long   estimatedMaxSize;

    public ScanResult(long byteOffset, long lba, int offsetInSector,
                      String extension, String mimeType,
                      String description, long estimatedMaxSize) {
        this.byteOffset       = byteOffset;
        this.lba              = lba;
        this.offsetInSector   = offsetInSector;
        this.extension        = extension;
        this.mimeType         = mimeType;
        this.description      = description;
        this.estimatedMaxSize = estimatedMaxSize;
    }

    public long   getByteOffset()       { return byteOffset; }
    public long   getLba()              { return lba; }
    public int    getOffsetInSector()   { return offsetInSector; }
    public String getExtension()        { return extension; }
    public String getMimeType()         { return mimeType; }
    public String getDescription()      { return description; }
    public long   getEstimatedMaxSize() { return estimatedMaxSize; }

    public String getFormattedOffset() {
        return String.format("0x%012X", byteOffset);
    }

    @Override
    public String toString() {
        return String.format("[%s] %s at LBA %d (offset %s)",
                extension.toUpperCase(), description, lba, getFormattedOffset());
    }
}
