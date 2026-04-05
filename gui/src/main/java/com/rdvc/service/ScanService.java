package com.rdvc.service;

import com.rdvc.bridge.NativeBridge;
import com.rdvc.bridge.model.ScanResult;
import com.rdvc.bridge.model.CarvedFile;
import com.rdvc.forensic.ForensicLogger;
import javafx.concurrent.Task;

import java.util.Arrays;
import java.util.List;
import java.util.ArrayList;
import java.util.function.Consumer;

/**
 * ScanService — async JavaFX Task wrappers for disk scan and file carving.
 *
 * Returns JavaFX Tasks that can be monitored for progress and cancelled.
 */
public class ScanService {

    private static final ScanService INSTANCE = new ScanService();
    public static ScanService get() { return INSTANCE; }

    /**
     * Create a Task that runs signature scanning over a sector range.
     *
     * @param handle      Open disk handle from DiskService
     * @param startLba    First sector
     * @param endLba      Last sector (0 = to end of disk)
     * @param onComplete  Callback with results when done
     */
    public Task<List<ScanResult>> createScanTask(long handle, long startLba, long endLba,
                                                  Consumer<List<ScanResult>> onComplete) {
        return new Task<>() {
            @Override
            protected List<ScanResult> call() throws Exception {
                updateTitle("Scanning disk...");
                updateMessage("Initializing scan from LBA " + startLba);
                updateProgress(0, 1);

                ForensicLogger.global().logScanStart(handle, startLba, endLba);

                if (!DiskService.get().isNativeAvailable()) {
                    // Demo mode: return fake results
                    Thread.sleep(1200);
                    List<ScanResult> demo = new ArrayList<>();
                    demo.add(new ScanResult(0, 0, 0, "jpg", "image/jpeg",
                            "JPEG Image (demo)", 20 * 1024 * 1024));
                    demo.add(new ScanResult(1024, 2, 0, "pdf", "application/pdf",
                            "PDF Document (demo)", 500 * 1024 * 1024));
                    updateProgress(1, 1);
                    if (onComplete != null) {
                        javafx.application.Platform.runLater(() -> onComplete.accept(demo));
                    }
                    ForensicLogger.global().logScanEnd(handle, demo.size());
                    return demo;
                }

                ScanResult[] raw = NativeBridge.runScan(handle, startLba, endLba);
                List<ScanResult> results = (raw != null) ? Arrays.asList(raw) : new ArrayList<>();

                updateProgress(1, 1);
                updateMessage("Scan complete: " + results.size() + " signatures found");

                ForensicLogger.global().logScanEnd(handle, results.size());

                if (onComplete != null) {
                    final List<ScanResult> finalResults = results;
                    javafx.application.Platform.runLater(() -> onComplete.accept(finalResults));
                }
                return results;
            }
        };
    }

    /**
     * Create a Task that runs file carving over a sector range.
     */
    public Task<List<CarvedFile>> createCarvingTask(long handle, long startLba, long endLba,
                                                     Consumer<List<CarvedFile>> onComplete) {
        return new Task<>() {
            @Override
            protected List<CarvedFile> call() throws Exception {
                updateTitle("Running file carver...");
                updateMessage("Carving from LBA " + startLba);
                updateProgress(0, 1);

                ForensicLogger.global().info("CARVE_START",
                        "handle=0x" + Long.toHexString(handle)
                        + " start=" + startLba + " end=" + endLba);

                if (!DiskService.get().isNativeAvailable()) {
                    Thread.sleep(2000);
                    List<CarvedFile> demo = new ArrayList<>();
                    demo.add(new CarvedFile(0, 204799, 204800, 0,
                            "jpg", "image/jpeg", "JPEG Image (demo)",
                            "carved_000000.jpg"));
                    updateProgress(1, 1);
                    if (onComplete != null) {
                        javafx.application.Platform.runLater(() -> onComplete.accept(demo));
                    }
                    return demo;
                }

                CarvedFile[] raw = NativeBridge.runCarver(handle, startLba, endLba);
                List<CarvedFile> results = (raw != null) ? Arrays.asList(raw) : new ArrayList<>();

                updateProgress(1, 1);
                updateMessage("Carving complete: " + results.size() + " files found");

                ForensicLogger.global().info("CARVE_END",
                        "handle=0x" + Long.toHexString(handle)
                        + " files=" + results.size());

                if (onComplete != null) {
                    final List<CarvedFile> finalResults = results;
                    javafx.application.Platform.runLater(() -> onComplete.accept(finalResults));
                }
                return results;
            }
        };
    }

    // ── Static convenience factories (used by MainWindow) ─────

    /**
     * Create a scan Task with default range (LBA 0 to totalSectors-1).
     */
    public static Task<List<ScanResult>> createScanTask(long handle, long totalSectors) {
        return INSTANCE.createScanTask(handle, 0L, totalSectors > 0 ? totalSectors - 1 : 0L, null);
    }

    /**
     * Create a carver Task — alias for createCarvingTask with default range.
     */
    public static Task<List<CarvedFile>> createCarverTask(long handle, long totalSectors) {
        return INSTANCE.createCarvingTask(handle, 0L, totalSectors > 0 ? totalSectors - 1 : 0L, null);
    }
}

