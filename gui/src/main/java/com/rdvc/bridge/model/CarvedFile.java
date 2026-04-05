package com.rdvc.bridge.model;

/**
 * CarvedFile — a file recovered by the file carving engine.
 */
public class CarvedFile {

    private final long   startOffset;
    private final long   endOffset;
    private final long   size;
    private final long   startLba;
    private final String extension;
    private final String mimeType;
    private final String description;
    private final String suggestedName;

    public CarvedFile(long startOffset, long endOffset, long size, long startLba,
                      String extension, String mimeType,
                      String description, String suggestedName) {
        this.startOffset   = startOffset;
        this.endOffset     = endOffset;
        this.size          = size;
        this.startLba      = startLba;
        this.extension     = extension;
        this.mimeType      = mimeType;
        this.description   = description;
        this.suggestedName = suggestedName;
    }

    public long   getStartOffset()   { return startOffset; }
    public long   getEndOffset()     { return endOffset; }
    public long   getSize()          { return size; }
    public long   getStartLba()      { return startLba; }
    public String getExtension()     { return extension; }
    public String getMimeType()      { return mimeType; }
    public String getDescription()   { return description; }
    public String getSuggestedName() { return suggestedName; }

    public String getFormattedSize() {
        if (size < 1024) return size + " B";
        if (size < 1024 * 1024) return String.format("%.1f KB", size / 1024.0);
        if (size < 1024L * 1024 * 1024) return String.format("%.2f MB", size / (1024.0 * 1024.0));
        return String.format("%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
    }

    @Override
    public String toString() {
        return String.format("%s [%s] %s at LBA %d",
                suggestedName, extension.toUpperCase(), getFormattedSize(), startLba);
    }
}
