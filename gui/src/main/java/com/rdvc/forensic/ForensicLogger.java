package com.rdvc.forensic;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.time.Instant;
import java.time.ZoneOffset;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * ForensicLogger — Audit-quality forensic event log.
 *
 * Every action (open, read, scan, export) is timestamped in ISO 8601 UTC
 * and appended to an immutable text log file. Suitable for chain-of-custody.
 *
 * Log format per line:
 *   [2026-04-04T15:32:10.441Z] CATEGORY action=details sha256=<hash_of_line>
 */
public class ForensicLogger {

    private static final ForensicLogger GLOBAL = new ForensicLogger();
    private static final DateTimeFormatter FMT =
        DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'")
                         .withZone(ZoneOffset.UTC);

    private PrintWriter writer;
    private final Object lock = new Object();
    private boolean initialized = false;
    /** In-memory ring — tailed by ForensicLogPanel. */
    private final List<String> memoryLines = new ArrayList<>();

    public static ForensicLogger global() { return GLOBAL; }
    /** Alias for global() — convenience accessor. */
    public static ForensicLogger get()    { return GLOBAL; }

    /** Initialize logging to a file. Call once at startup. */
    public void init(File logFile) throws IOException {
        synchronized (lock) {
            if (initialized) return;
            FileOutputStream fos = new FileOutputStream(logFile, true);
            writer = new PrintWriter(
                new BufferedWriter(new OutputStreamWriter(fos, StandardCharsets.UTF_8)), true);
            initialized = true;
            info("LOG_OPEN", "Forensic log opened: " + logFile.getAbsolutePath());
        }
    }

    /** Convenience shorthand: log at INFO level. */
    public void log(String category, String message) {
        info(category, message);
    }

    public void info(String category, String message) {
        log("INFO", category, message);
    }

    public void warn(String category, String message) {
        log("WARN", category, message);
    }

    public void error(String category, String message) {
        log("ERROR", category, message);
    }

    public void logDiskOpen(String path, long handle) {
        info("DISK_OPEN", "path=" + path + " handle=0x" + Long.toHexString(handle));
    }

    public void logDiskClose(long handle) {
        info("DISK_CLOSE", "handle=0x" + Long.toHexString(handle));
    }

    public void logSectorRead(long handle, long lba, String sha256) {
        info("SECTOR_READ", "handle=0x" + Long.toHexString(handle)
             + " lba=" + lba + " sha256=" + sha256);
    }

    public void logScanStart(long handle, long startLba, long endLba) {
        info("SCAN_START", "handle=0x" + Long.toHexString(handle)
             + " start=" + startLba + " end=" + endLba);
    }

    public void logScanEnd(long handle, int resultsFound) {
        info("SCAN_END", "handle=0x" + Long.toHexString(handle)
             + " results=" + resultsFound);
    }

    public void logExport(long handle, long lbaStart, int count, String path) {
        info("EXPORT", "handle=0x" + Long.toHexString(handle)
             + " lba_start=" + lbaStart + " count=" + count + " dest=" + path);
    }

    public void logBookmark(long lba, String note) {
        info("BOOKMARK", "lba=" + lba + " note=" + note);
    }

    /** Returns an unmodifiable snapshot of in-memory log lines. */
    public List<String> getLines() {
        synchronized (lock) {
            return Collections.unmodifiableList(new ArrayList<>(memoryLines));
        }
    }

    /** Clears the in-memory lines (does not affect the on-disk log file). */
    public void clearMemory() {
        synchronized (lock) { memoryLines.clear(); }
    }

    public void flush() {
        synchronized (lock) {
            if (writer != null) writer.flush();
        }
    }

    // ── Private ──────────────────────────────────────────────────

    private void log(String level, String category, String message) {
        String timestamp = FMT.format(Instant.now());
        String line = "[" + timestamp + "] " + level + "  " + category + "  " + message;

        // Append the SHA-256 of the log line for tamper detection
        String lineHash = sha256short(line);
        String fullLine = line + "  integrity=" + lineHash;

        synchronized (lock) {
            // Always print to stderr as backup
            System.err.println(fullLine);
            memoryLines.add(fullLine);
            if (writer != null) {
                writer.println(fullLine);
            }
        }
    }

    private String sha256short(String input) {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] hash = md.digest(input.getBytes(StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder(16);
            for (int i = 0; i < 8; i++) sb.append(String.format("%02x", hash[i]));
            return sb.toString();
        } catch (Exception e) {
            return "N/A";
        }
    }
}
