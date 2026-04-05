package com.rdvc.bridge;

import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import com.rdvc.forensic.ForensicLogger;

/**
 * NativeLoader — detects OS, extracts or locates the native library
 * (rdvc_jni.dll on Windows, librdvc_jni.so on Linux) and loads it.
 *
 * Search order:
 *  1. Java property "rdvc.native.path" (explicit override)
 *  2. Directory next to the running JAR
 *  3. Extracted from JAR resources (if bundled)
 *  4. System library path (java.library.path)
 */
public class NativeLoader {

    private static volatile boolean loaded = false;
    private static volatile Throwable loadError = null;

    public static final String LIB_NAME_WINDOWS = "rdvc_jni";
    public static final String LIB_NAME_LINUX   = "librdvc_jni.so";
    public static final String LIB_NAME_MACOS   = "librdvc_jni.dylib";

    /**
     * Load the native JNI library. Safe to call multiple times.
     * @throws UnsatisfiedLinkError if library cannot be found/loaded.
     */
    public static synchronized void load() {
        if (loaded) return;
        if (loadError != null) throw new UnsatisfiedLinkError(loadError.getMessage());

        try {
            doLoad();
            loaded = true;
        } catch (Throwable t) {
            loadError = t;
            try {
                ForensicLogger.global().error("NATIVE_LOAD",
                    "Failed to load native RDVC library: " + t.toString());
            } catch (Throwable ignore) {
                // best-effort logging, ignore failures
            }
            t.printStackTrace(System.err);
            UnsatisfiedLinkError ule = new UnsatisfiedLinkError(
                "Failed to load native RDVC library: " + t.toString());
            try { ule.initCause(t); } catch (Throwable ignore) {}
            throw ule;
        }
    }

    public static boolean isLoaded() { return loaded; }

    // ── Private ──────────────────────────────────────────────────

    private static void doLoad() throws Exception {
        String os = System.getProperty("os.name", "").toLowerCase();

        // 1. Explicit override via property
        String explicitPath = System.getProperty("rdvc.native.path");
        if (explicitPath != null && !explicitPath.isEmpty()) {
            System.load(new File(explicitPath).getAbsolutePath());
            return;
        }

        // 2. Beside JAR / working directory
        String libFileName = getNativeLibFileName(os);
        File beside = new File(System.getProperty("user.dir"), libFileName);
        if (beside.exists()) {
            System.load(beside.getAbsolutePath());
            return;
        }

        // 3. Extract from JAR resources
        String resourcePath = "/native/" + libFileName;
        InputStream is = NativeLoader.class.getResourceAsStream(resourcePath);
        if (is != null) {
            Path tmp = Files.createTempFile("rdvc_jni_", extractSuffix(os));
            try (FileOutputStream fos = new FileOutputStream(tmp.toFile())) {
                byte[] buf = new byte[8192];
                int n;
                while ((n = is.read(buf)) != -1) fos.write(buf, 0, n);
            }
            tmp.toFile().deleteOnExit();
            System.load(tmp.toAbsolutePath().toString());
            return;
        }

        // 4. System library path
        System.loadLibrary(LIB_NAME_WINDOWS); // will resolve to libXxx.so on Linux
    }

    private static String getNativeLibFileName(String os) {
        if (os.contains("win"))   return LIB_NAME_WINDOWS + ".dll";
        if (os.contains("mac"))   return LIB_NAME_MACOS;
        return LIB_NAME_LINUX;
    }

    private static String extractSuffix(String os) {
        if (os.contains("win")) return ".dll";
        if (os.contains("mac")) return ".dylib";
        return ".so";
    }
}
