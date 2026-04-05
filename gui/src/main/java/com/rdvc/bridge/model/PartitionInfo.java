package com.rdvc.bridge.model;

/**
 * PartitionInfo — represents one partition entry from MBR or GPT.
 */
public class PartitionInfo {

    public enum SchemeType { MBR, GPT }

    private final SchemeType scheme;
    private final int    index;
    private final String typeName;
    private final String name;         // GPT only
    private final String typeGuid;     // GPT only
    private final String uniqueGuid;   // GPT only
    private final long   startLba;
    private final long   sizeLba;
    private final long   byteOffset;
    private final long   byteSize;
    private final boolean bootable;    // MBR only
    private final int    mbrTypeCode;  // MBR only

    // MBR constructor
    public PartitionInfo(int index, int mbrTypeCode, String typeName,
                         long startLba, long sizeLba,
                         long byteOffset, long byteSize, boolean bootable) {
        this.scheme      = SchemeType.MBR;
        this.index       = index;
        this.mbrTypeCode = mbrTypeCode;
        this.typeName    = typeName;
        this.name        = "";
        this.typeGuid    = "";
        this.uniqueGuid  = "";
        this.startLba    = startLba;
        this.sizeLba     = sizeLba;
        this.byteOffset  = byteOffset;
        this.byteSize    = byteSize;
        this.bootable    = bootable;
    }

    // GPT constructor
    public PartitionInfo(int index, String typeGuid, String uniqueGuid,
                         String typeName, String name,
                         long startLba, long endLba,
                         long byteOffset, long byteSize) {
        this.scheme      = SchemeType.GPT;
        this.index       = index;
        this.mbrTypeCode = 0;
        this.typeGuid    = typeGuid;
        this.uniqueGuid  = uniqueGuid;
        this.typeName    = typeName;
        this.name        = name;
        this.startLba    = startLba;
        this.sizeLba     = endLba - startLba + 1;
        this.byteOffset  = byteOffset;
        this.byteSize    = byteSize;
        this.bootable    = false;
    }

    public SchemeType getScheme()    { return scheme; }
    public int    getIndex()         { return index; }
    public String getTypeName()      { return typeName; }
    public String getName()          { return name.isEmpty() ? typeName : name; }
    public String getTypeGuid()      { return typeGuid; }
    public String getUniqueGuid()    { return uniqueGuid; }
    public long   getStartLba()      { return startLba; }
    public long   getEndLba()        { return startLba + Math.max(0, sizeLba - 1); }
    public long   getSizeLba()       { return sizeLba; }
    public long   getByteOffset()    { return byteOffset; }
    public long   getByteSize()      { return byteSize; }
    public long   getSizeBytes()     { return byteSize; }
    public boolean isBootable()      { return bootable; }
    public int    getMbrTypeCode()   { return mbrTypeCode; }
    /** Returns the most descriptive GUID available for display. */
    public String getGuid() {
        if (uniqueGuid != null && !uniqueGuid.isEmpty()) return uniqueGuid;
        return typeGuid;
    }

    public String getFormattedSize() {
        if (byteSize <= 0) return "Unknown";
        double gb = byteSize / (1024.0 * 1024.0 * 1024.0);
        if (gb >= 1.0) return String.format("%.2f GB", gb);
        double mb = byteSize / (1024.0 * 1024.0);
        if (mb >= 1.0) return String.format("%.2f MB", mb);
        return String.format("%d bytes", byteSize);
    }

    @Override
    public String toString() {
        if (scheme == SchemeType.GPT) {
            return String.format("Part %d: %s [%s] LBA %d, %s",
                    index, getName(), typeName, startLba, getFormattedSize());
        }
        return String.format("Part %d: %s (0x%02X) LBA %d, %s%s",
                index, typeName, mbrTypeCode, startLba,
                getFormattedSize(), bootable ? " [Boot]" : "");
    }
}
