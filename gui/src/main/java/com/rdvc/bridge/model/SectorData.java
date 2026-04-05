package com.rdvc.bridge.model;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * SectorData — holds the raw bytes of one or more sectors,
 * along with their LBA address and computed digest.
 */
public class SectorData {

    private final long   lba;
    private final long   byteOffset;
    private final int    sectorSize;
    private final byte[] data;
    private String sha256Cache;

    public SectorData(long lba, long byteOffset, int sectorSize, byte[] data) {
        this.lba        = lba;
        this.byteOffset = byteOffset;
        this.sectorSize = sectorSize;
        this.data       = data;
    }

    public long   getLba()        { return lba; }
    public long   getByteOffset() { return byteOffset; }
    public int    getSectorSize() { return sectorSize; }
    public byte[] getData()       { return data; }
    public int    getLength()     { return data != null ? data.length : 0; }

    /** Returns the byte at an absolute offset within this sector's data. */
    public byte getByte(int offset) {
        if (data == null || offset < 0 || offset >= data.length) return 0;
        return data[offset];
    }

    /** Returns unsigned byte value at offset. */
    public int getUnsignedByte(int offset) {
        return getByte(offset) & 0xFF;
    }

    /** Compute SHA-256 of sector data (cached). */
    public String getSha256() {
        if (sha256Cache != null) return sha256Cache;
        if (data == null) return "N/A";
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] hash = md.digest(data);
            StringBuilder sb = new StringBuilder(64);
            for (byte b : hash) sb.append(String.format("%02x", b));
            sha256Cache = sb.toString();
        } catch (NoSuchAlgorithmException e) {
            sha256Cache = "N/A";
        }
        return sha256Cache;
    }

    /** True if all bytes are zero. */
    public boolean isEmpty() {
        if (data == null) return true;
        for (byte b : data) if (b != 0) return false;
        return true;
    }
}
