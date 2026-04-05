package com.rdvc.ui;

import com.rdvc.bridge.model.*;
import com.rdvc.forensic.BookmarkManager;
import com.rdvc.forensic.ForensicLogger;
import com.rdvc.service.DiskService;
import com.rdvc.service.ScanService;
import com.rdvc.ui.components.*;
import com.rdvc.ui.dialogs.*;
import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.stage.Stage;

import java.util.List;
import java.util.OptionalLong;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * MainWindow — root scene graph for RAW DISK VIEWER CPLX.
 *
 * Layout:
 *   ┌────────────────────────────────────────────────────────────────┐
 *   │  MenuBar + ToolBar                                             │
 *   ├────────────┬───────────────────────────────────┬──────────────┤
 *   │ Disk       │  SectorNavigator (top of center)  │ Info         │
 *   │ Selector   │  HexViewerPanel                   │ Panel        │
 *   │ Panel      │                                   │ (right)      │
 *   │ (left)     │                                   │              │
 *   ├────────────┴───────────────────────────────────┴──────────────┤
 *   │  ForensicLogPanel (bottom)                                     │
 *   ├────────────────────────────────────────────────────────────────┤
 *   │  StatusBar                                                      │
 *   └────────────────────────────────────────────────────────────────┘
 */
public class MainWindow extends BorderPane {

    // ── Components ────────────────────────────────────────────
    private final DiskSelectorPanel diskSelector;
    private final SectorNavigator   sectorNav;
    private final HexViewerPanel    hexViewer;
    private final InfoPanel         infoPanel;
    private final ForensicLogPanel  logPanel;
    private final BookmarkPanel     bookmarkPanel;

    // ── Status bar labels ─────────────────────────────────────
    private final Label statusLeft  = statusLabel("Ready");
    private final Label statusMid   = statusLabel("");
    private final Label statusRight = statusLabel("READ-ONLY MODE");

    private final Stage owner;
    private DiskInfo activeDisk = null;

    public MainWindow(Stage stage) {
        this.owner = stage;

        // ── Instantiate components ────────────────────────────
        diskSelector   = new DiskSelectorPanel();
        sectorNav      = new SectorNavigator();
        hexViewer      = new HexViewerPanel(sectorNav);
        infoPanel      = new InfoPanel();
        bookmarkPanel  = new BookmarkPanel();
        logPanel       = new ForensicLogPanel();

        // ── Wire: disk selection ──────────────────────────────
        diskSelector.setOnDiskSelected(this::onDiskSelected);

        // ── Wire: bookmark jumps ──────────────────────────────
        bookmarkPanel.setOnJumpTo(lba -> sectorNav.navigateTo(lba));

        // ── Wire: HexCanvas → InfoPanel byte info ─────────────
        hexViewer.getHexCanvas().selectedByteIndexProperty().addListener((obs, ov, nv) -> {
            if (!nv.equals(-1)) {
                byte[] data = hexViewer.getHexCanvas().getData();
                int idx = nv.intValue();
                if (data != null && idx >= 0 && idx < data.length)
                    infoPanel.showSelectedByte(data[idx] & 0xFF);
            }
        });

        // ── Wire: bookmark add from hex canvas ────────────────
        hexViewer.getHexCanvas().setOnBookmarkRequest((lba, byteOffset) -> {
            if (activeDisk != null)
                BookmarkDialog.showAndSave(lba, byteOffset, activeDisk.getPath());
            bookmarkPanel.refresh();
            hexViewer.refreshBookmarks();
        });

        // ── Center: Navigator on top of HexViewer ────────────
        VBox centerBox = new VBox(sectorNav, hexViewer);
        VBox.setVgrow(hexViewer, Priority.ALWAYS);

        // ── Left tabs: Disks + Bookmarks ──────────────────────
        TabPane leftTabs = new TabPane();
        leftTabs.setTabClosingPolicy(TabPane.TabClosingPolicy.UNAVAILABLE);
        leftTabs.setStyle("-fx-background-color: #1E1E1E;");
        Tab tabDisks = new Tab("Disks",      diskSelector);
        Tab tabBkmk  = new Tab("Bookmarks",  bookmarkPanel);
        tabDisks.setStyle("-fx-background-color: #1E1E1E;");
        tabBkmk.setStyle("-fx-background-color: #1E1E1E;");
        leftTabs.getTabs().addAll(tabDisks, tabBkmk);
        leftTabs.setPrefWidth(290);

        // ── Assemble main area ────────────────────────────────
        SplitPane hSplit = new SplitPane(leftTabs, centerBox, infoPanel);
        hSplit.setDividerPositions(0.22, 0.78);
        hSplit.setStyle("-fx-background-color: #1A1A1A;");
        VBox.setVgrow(hSplit, Priority.ALWAYS);

        // ── Bottom area ────────────────────────────────────────
        logPanel.setPrefHeight(150);
        VBox mainArea = new VBox(hSplit, logPanel);
        VBox.setVgrow(hSplit, Priority.ALWAYS);

        // ── Status bar ─────────────────────────────────────────
        statusRight.setStyle("-fx-text-fill: #FF6060; -fx-font-size: 11px; -fx-font-weight: bold;");
        Region spacer1 = new Region(); HBox.setHgrow(spacer1, Priority.ALWAYS);
        Region spacer2 = new Region(); HBox.setHgrow(spacer2, Priority.ALWAYS);
        HBox statusBar = new HBox(8, statusLeft, spacer1, statusMid, spacer2, statusRight);
        statusBar.setPadding(new Insets(3, 10, 3, 10));
        statusBar.setStyle("-fx-background-color: #141414; -fx-border-color: #333; -fx-border-width: 1 0 0 0;");

        // ── Root layout ────────────────────────────────────────
        setTop(buildMenuBar());
        setCenter(mainArea);
        setBottom(statusBar);
        setStyle("-fx-background-color: #1A1A1A;");
    }

    // ── Public API ────────────────────────────────────────────

    public void shutdown() { logPanel.shutdown(); }

    // ── Disk selection ────────────────────────────────────────

    private void onDiskSelected(DiskInfo disk) {
        // Close previous
        if (activeDisk != null) DiskService.get().closeActiveDisk();

        boolean ok = DiskService.get().openDisk(disk);
        if (!ok) {
            new Alert(Alert.AlertType.ERROR,
                    "Cannot open disk:\n" + disk.getPath()
                    + "\n\nRun as Administrator to access physical drives.").showAndWait();
            return;
        }

        activeDisk = disk;
        owner.setTitle("RAW DISK VIEWER CPLX  —  " + disk.getPath() + "  [READ-ONLY]");

        statusLeft.setText(disk.getModel().isEmpty() ? disk.getPath() : disk.getModel());
        statusMid.setText(disk.getFormattedSize() + "  ·  " + disk.getTotalSectors() + " sectors");

        // Load partition info
        List<PartitionInfo> parts = DiskService.get().getPartitionInfo();
        infoPanel.showPartitions(parts);

        // Initialise hex viewer
        hexViewer.openDisk(disk.getTotalSectors());

        ForensicLogger.get().log("DISK_OPEN", "Opened disk: " + disk.getPath()
                + " (" + disk.getFormattedSize() + ")");
    }

    // ── Menu bar ───────────────────────────────────────────────

    private Node buildMenuBar() {
        MenuBar menuBar = new MenuBar();
        menuBar.setStyle("-fx-background-color: #252525; -fx-border-color: #333; -fx-border-width: 0 0 1 0;");

        // File menu
        Menu mFile = new Menu("File");
        MenuItem miOpenImg = new MenuItem("Open Image File…");
        MenuItem miExit    = new MenuItem("Exit");
        miOpenImg.setOnAction(e -> diskSelector.refreshDisks());
        miExit.setOnAction(e -> Platform.exit());
        mFile.getItems().addAll(miOpenImg, new SeparatorMenuItem(), miExit);

        // Navigate menu
        Menu mNav = new Menu("Navigate");
        MenuItem miGoto     = new MenuItem("Go To Sector…");
        MenuItem miGotoMbr  = new MenuItem("Go To MBR (LBA 0)");
        MenuItem miGotoGpt  = new MenuItem("Go To GPT Header (LBA 1)");
        miGoto.setOnAction(e -> {
            if (activeDisk == null) return;
            OptionalLong res = GotoSectorDialog.show(sectorNav.getCurrentLba(),
                    activeDisk.getTotalSectors());
            res.ifPresent(sectorNav::navigateTo);
        });
        miGotoMbr.setOnAction(e -> sectorNav.navigateTo(0));
        miGotoGpt.setOnAction(e -> sectorNav.navigateTo(1));
        mNav.getItems().addAll(miGoto, new SeparatorMenuItem(), miGotoMbr, miGotoGpt);

        // Scan menu
        Menu mScan = new Menu("Scan");
        MenuItem miScanSig = new MenuItem("Scan File Signatures…");
        MenuItem miCarve   = new MenuItem("Carve Files…");
        miScanSig.setOnAction(e -> runScanSignatures());
        miCarve.setOnAction(e -> runCarver());
        mScan.getItems().addAll(miScanSig, miCarve);

        // Tools menu
        Menu mTools = new Menu("Tools");
        MenuItem miAddBkmk   = new MenuItem("Add Bookmark at Current LBA");
        MenuItem miExport    = new MenuItem("Export Sectors…");
        miAddBkmk.setOnAction(e -> addBookmarkAtCurrent());
        miExport.setOnAction(e -> openExportDialog(null));
        mTools.getItems().addAll(miAddBkmk, new SeparatorMenuItem(), miExport);

        // Help
        Menu mHelp = new Menu("Help");
        MenuItem miAbout = new MenuItem("About");
        miAbout.setOnAction(e -> showAbout());
        mHelp.getItems().add(miAbout);

        menuBar.getMenus().addAll(mFile, mNav, mScan, mTools, mHelp);

        // ── Toolbar ───────────────────────────────────────────
        Button tbScan   = toolbarBtn("⚡ Scan Signatures");
        Button tbCarve  = toolbarBtn("🗂 Carve Files");
        Button tbGoto   = toolbarBtn("→ Go To Sector");
        Button tbBkmk   = toolbarBtn("🔖 Add Bookmark");
        Button tbExport = toolbarBtn("💾 Export");

        tbScan.setOnAction(e -> runScanSignatures());
        tbCarve.setOnAction(e -> runCarver());
        tbGoto.setOnAction(e -> miGoto.fire());
        tbBkmk.setOnAction(e -> addBookmarkAtCurrent());
        tbExport.setOnAction(e -> openExportDialog(null));

        Label roLabel = new Label("  🔒 READ-ONLY  ");
        roLabel.setStyle("-fx-text-fill: #FF8080; -fx-font-weight: bold; -fx-background-color: #2A1A1A;"
                + " -fx-border-color: #664444; -fx-padding: 3 8;");

        Region tbSpacer = new Region();
        HBox.setHgrow(tbSpacer, Priority.ALWAYS);

        HBox toolbar = new HBox(6, tbScan, tbCarve, new Separator(javafx.geometry.Orientation.VERTICAL),
                tbGoto, tbBkmk, tbExport, tbSpacer, roLabel);
        toolbar.setPadding(new Insets(5, 10, 5, 10));
        toolbar.setAlignment(Pos.CENTER_LEFT);
        toolbar.setStyle("-fx-background-color: #222; -fx-border-color: #333; -fx-border-width: 0 0 1 0;");

        return new VBox(menuBar, toolbar);
    }

    // ── Actions ────────────────────────────────────────────────

    private void runScanSignatures() {
        if (activeDisk == null) { noDisk(); return; }
        var task = ScanService.createScanTask(DiskService.get().getActiveHandle(),
                activeDisk.getTotalSectors());
        new ScanProgressDialog<>("Scanning Signatures", task).showAndRun();
    }

    private void runCarver() {
        if (activeDisk == null) { noDisk(); return; }
        var task = ScanService.createCarverTask(DiskService.get().getActiveHandle(),
                activeDisk.getTotalSectors());
        new ScanProgressDialog<>("Carving Files", task).showAndRun();
    }

    private void addBookmarkAtCurrent() {
        if (activeDisk == null) { noDisk(); return; }
        long lba    = sectorNav.getCurrentLba();
        int  sel    = hexViewer.getHexCanvas().getSelectedByteIndex();
        long offset = lba * 512L + Math.max(0, sel);
        BookmarkDialog.showAndSave(lba, offset, activeDisk.getPath());
        bookmarkPanel.refresh();
        hexViewer.refreshBookmarks();
    }

    private void openExportDialog(CarvedFile cf) {
        if (activeDisk == null) { noDisk(); return; }
        ExportDialog dlg = new ExportDialog(sectorNav.getCurrentLba(),
                activeDisk.getTotalSectors(), cf);
        dlg.showAndWait().ifPresent(req -> {
            ExecutorService ex = Executors.newSingleThreadExecutor(r -> {
                Thread t = new Thread(r, "rdvc-export"); t.setDaemon(true); return t;
            });
            ex.submit(() -> {
                boolean ok;
                if (req.mode() == ExportDialog.ExportMode.SECTORS) {
                    ok = DiskService.get().exportSectors(req.startLba(), req.count(), req.destDir());
                } else {
                    ok = DiskService.get().exportCarvedFile(req.carvedFile(), req.destDir());
                }
                boolean finalOk = ok;
                Platform.runLater(() -> {
                    if (finalOk) {
                        new Alert(Alert.AlertType.INFORMATION, "Export completed successfully.").showAndWait();
                    } else {
                        new Alert(Alert.AlertType.ERROR, "Export failed. Check audit log for details.").showAndWait();
                    }
                });
                ex.shutdown();
            });
        });
    }

    private void noDisk() {
        new Alert(Alert.AlertType.WARNING, "No disk is currently open.\nSelect a disk from the evidence sources panel.").showAndWait();
    }

    private void showAbout() {
        Alert a = new Alert(Alert.AlertType.INFORMATION);
        a.setTitle("About RAW DISK VIEWER CPLX");
        a.setHeaderText("RAW DISK VIEWER CPLX  v1.0.0");
        a.setContentText(
                "Professional forensic disk viewer inspired by FTK Imager.\n\n"
                + "Architecture:\n"
                + "  • C Core (rdvc_core.dll) — low-level disk I/O\n"
                + "  • JNI Bridge (rdvc_jni.dll) — native bridge\n"
                + "  • JavaFX 21 — GUI\n\n"
                + "READ-ONLY by design. All operations are logged.\n\n"
                + "For forensic use. Handle evidence with care.");
        a.showAndWait();
    }

    // ── Helpers ────────────────────────────────────────────────

    private Label statusLabel(String text) {
        Label l = new Label(text);
        l.setStyle("-fx-text-fill: #808080; -fx-font-size: 11px;");
        return l;
    }

    private Button toolbarBtn(String text) {
        Button btn = new Button(text);
        btn.setStyle("-fx-background-color: #2A2A2A; -fx-text-fill: #C0C0C0;"
                + " -fx-border-color: #444; -fx-font-size: 12px; -fx-padding: 4 10;");
        btn.setOnMouseEntered(e ->
                btn.setStyle("-fx-background-color: #383838; -fx-text-fill: #E8E8E8;"
                        + " -fx-border-color: #666; -fx-font-size: 12px; -fx-padding: 4 10;"));
        btn.setOnMouseExited(e ->
                btn.setStyle("-fx-background-color: #2A2A2A; -fx-text-fill: #C0C0C0;"
                        + " -fx-border-color: #444; -fx-font-size: 12px; -fx-padding: 4 10;"));
        return btn;
    }
}
