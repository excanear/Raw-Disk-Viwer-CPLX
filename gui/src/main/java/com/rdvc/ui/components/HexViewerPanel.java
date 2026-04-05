package com.rdvc.ui.components;

import com.rdvc.bridge.model.SectorData;
import com.rdvc.forensic.BookmarkManager;
import com.rdvc.service.DiskService;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.scene.control.ScrollBar;
import javafx.scene.layout.*;

import java.util.Collections;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

/**
 * HexViewerPanel — center panel that wraps HexCanvas + vertical ScrollBar.
 *
 * Loads sectors on demand when the LBA changes via SectorNavigator.
 * Renders SECTOR_BATCH sectors at once (8 × 512 = 4 KB view).
 */
public class HexViewerPanel extends BorderPane {

    private static final int SECTOR_BATCH = 8;

    private final HexCanvas   hexCanvas;
    private final ScrollBar   vScrollBar;
    private final SectorNavigator navigator;

    private final ExecutorService ioExecutor = Executors.newSingleThreadExecutor(r -> {
        Thread t = new Thread(r, "hex-io");
        t.setDaemon(true);
        return t;
    });

    private long currentLba = 0;
    private boolean loading = false;

    public HexViewerPanel(SectorNavigator sectorNavigator) {
        this.navigator  = sectorNavigator;
        this.hexCanvas  = new HexCanvas();
        this.vScrollBar = new ScrollBar();

        vScrollBar.setOrientation(javafx.geometry.Orientation.VERTICAL);
        vScrollBar.setStyle("-fx-background-color: #1A1A1A;");
        vScrollBar.setMin(0);
        vScrollBar.setMax(100);
        vScrollBar.setValue(0);

        hexCanvas.totalRowsProperty().addListener((obs, ov, nv) -> {
            double totalH = nv.doubleValue() * HexCanvas.ROW_HEIGHT;
            double viewH  = hexCanvas.getHeight();
            vScrollBar.setMax(Math.max(0, totalH - viewH));
            vScrollBar.setVisibleAmount(viewH);
        });

        vScrollBar.valueProperty().addListener((obs, ov, nv) ->
                hexCanvas.setScrollOffset(nv.doubleValue()));

        // Scroll wheel on canvas moves scrollbar
        hexCanvas.setOnScroll(e -> {
            double delta = -e.getDeltaY() * 2.5;
            double newVal = Math.max(vScrollBar.getMin(),
                    Math.min(vScrollBar.getMax(), vScrollBar.getValue() + delta));
            vScrollBar.setValue(newVal);
        });

        // Navigator connects
        navigator.setOnNavigate(lba -> loadSectors(lba));

        setCenter(hexCanvas);
        setRight(vScrollBar);
        setStyle("-fx-background-color: #1A1A1A;");
    }

    /** Called externally when a disk is opened to initialise the view. */
    public void openDisk(long totalSectors) {
        navigator.setTotalSectors(totalSectors);
        loadSectors(0);
    }

    /** Refresh bookmark highlights from BookmarkManager. */
    public void refreshBookmarks() {
        Set<Long> lbas = BookmarkManager.get().getAll().stream()
            .map(b -> b.getLba())
            .collect(Collectors.toSet());
        hexCanvas.setBookmarkedLbas(lbas);
    }

    /** Reload current LBA (e.g. after changing disk). */
    public void reload() { loadSectors(currentLba); }

    public HexCanvas  getHexCanvas()  { return hexCanvas; }
    public long       getCurrentLba() { return currentLba; }

    // ── Private ──────────────────────────────────────────────

    private void loadSectors(long lba) {
        if (loading) return;
        loading     = true;
        currentLba  = lba;

        ioExecutor.submit(() -> {
            DiskService ds = DiskService.get();
            byte[] data = ds.readSectors(lba, SECTOR_BATCH);
            long baseOffset = lba * 512L;

            Platform.runLater(() -> {
                if (data != null && data.length > 0) {
                    hexCanvas.setData(data, baseOffset);
                    vScrollBar.setValue(0);
                    refreshBookmarks();
                }
                loading = false;
            });
        });
    }
}
