package com.rdvc.forensic;

import com.google.gson.*;
import com.google.gson.reflect.TypeToken;

import java.io.*;
import java.lang.reflect.Type;
import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.*;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * BookmarkManager — persistent sector bookmarks with notes.
 *
 * Bookmarks are stored in a JSON file in the user's home directory.
 * Thread-safe for concurrent GUI access.
 */
public class BookmarkManager {

    private static final BookmarkManager INSTANCE = new BookmarkManager();
    public static BookmarkManager get() { return INSTANCE; }

    private final List<Bookmark> bookmarks = new CopyOnWriteArrayList<>();
    private File saveFile;
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    // ── Bookmark data class ───────────────────────────────────

    public static class Bookmark {
        private final long   lba;
        private final long   byteOffset;
        private String note;
        private final String createdAt;
        private String diskPath;
        private String flagColor; // "RED", "YELLOW", "GREEN"

        public Bookmark(long lba, long byteOffset, String note, String diskPath) {
            this.lba        = lba;
            this.byteOffset = byteOffset;
            this.note       = note;
            this.diskPath   = diskPath;
            this.createdAt  = Instant.now().toString();
            this.flagColor  = "YELLOW";
        }

        public long   getLba()        { return lba; }
        public long   getByteOffset() { return byteOffset; }
        public String getNote()       { return note; }
        public String getCreatedAt()  { return createdAt; }
        public String getDiskPath()   { return diskPath; }
        public String getFlagColor()  { return flagColor; }

        public void setNote(String note)           { this.note = note; }
        public void setFlagColor(String flagColor) { this.flagColor = flagColor; }

        @Override
        public String toString() {
            return String.format("LBA %d — %s [%s]", lba, note, flagColor);
        }
    }

    // ── API ───────────────────────────────────────────────────

    public void init(File file) {
        this.saveFile = file;
        load();
    }

    public Bookmark add(long lba, long byteOffset, String note, String diskPath) {
        Bookmark bm = new Bookmark(lba, byteOffset, note, diskPath);
        bookmarks.add(bm);
        save();
        ForensicLogger.global().logBookmark(lba, note);
        return bm;
    }

    public void remove(Bookmark bm) {
        bookmarks.remove(bm);
        save();
    }

    public List<Bookmark> getAll() {
        return Collections.unmodifiableList(bookmarks);
    }

    public List<Bookmark> getForDisk(String diskPath) {
        List<Bookmark> result = new ArrayList<>();
        for (Bookmark bm : bookmarks) {
            if (diskPath.equals(bm.getDiskPath())) result.add(bm);
        }
        return result;
    }

    public Optional<Bookmark> findByLba(long lba, String diskPath) {
        return bookmarks.stream()
            .filter(b -> b.getLba() == lba && diskPath.equals(b.getDiskPath()))
            .findFirst();
    }

    // ── Persistence ───────────────────────────────────────────

    private void save() {
        if (saveFile == null) return;
        try (Writer w = new OutputStreamWriter(
                new FileOutputStream(saveFile), StandardCharsets.UTF_8)) {
            gson.toJson(new ArrayList<>(bookmarks), w);
        } catch (IOException e) {
            ForensicLogger.global().error("BOOKMARK", "Save failed: " + e.getMessage());
        }
    }

    private void load() {
        if (saveFile == null || !saveFile.exists()) return;
        try (Reader r = new InputStreamReader(
                new FileInputStream(saveFile), StandardCharsets.UTF_8)) {
            Type listType = new TypeToken<List<Bookmark>>() {}.getType();
            List<Bookmark> loaded = gson.fromJson(r, listType);
            if (loaded != null) bookmarks.addAll(loaded);
        } catch (Exception e) {
            ForensicLogger.global().warn("BOOKMARK", "Load failed: " + e.getMessage());
        }
    }
}
