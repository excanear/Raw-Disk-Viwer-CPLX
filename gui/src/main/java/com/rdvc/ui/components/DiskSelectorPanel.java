package com.rdvc.ui.components;

import com.rdvc.bridge.model.DiskInfo;
import com.rdvc.service.DiskService;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.stage.FileChooser;

import java.io.File;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.function.Consumer;

/**
 * DiskSelectorPanel — left panel. Lists physical disks and image files.
 *
 * Emits disk selection events via {@link #setOnDiskSelected(Consumer)}.
 */
public class DiskSelectorPanel extends VBox {

    private final ListView<DiskInfo> diskList;
    private Consumer<DiskInfo> onDiskSelected;

    private final ExecutorService ioExec = Executors.newSingleThreadExecutor(r -> {
        Thread t = new Thread(r, "disk-lister");
        t.setDaemon(true);
        return t;
    });

    public DiskSelectorPanel() {
        super(0);
        setPrefWidth(280);
        setStyle("-fx-background-color: #1E1E1E;");

        // ── Header ───────────────────────────────────────────
        Label title = new Label("EVIDENCE SOURCES");
        title.setStyle("-fx-text-fill: #808080; -fx-font-size: 11px; -fx-font-weight: bold;");
        HBox header = new HBox(title);
        header.setPadding(new Insets(10, 10, 8, 10));
        header.setStyle("-fx-border-color: #333; -fx-border-width: 0 0 1 0;");

        // ── ListView ──────────────────────────────────────────
        diskList = new ListView<>();
        diskList.setStyle("-fx-background-color: #1E1E1E; -fx-border-color: transparent;");
        diskList.setCellFactory(lv -> new DiskCell());
        diskList.setOnMouseClicked(e -> {
            if (e.getClickCount() == 2) selectCurrent();
        });
        VBox.setVgrow(diskList, Priority.ALWAYS);

        // ── Buttons ───────────────────────────────────────────
        Button btnRefresh = toolButton("⟳  Refresh Disks");
        Button btnOpenImg = toolButton("📂  Open Image File…");
        Button btnSelect  = toolButton("▶  Open Selected");

        btnRefresh.setOnAction(e -> refreshDisks());
        btnOpenImg.setOnAction(e -> openImageFile());
        btnSelect.setOnAction(e -> selectCurrent());

        VBox btnBox = new VBox(4, btnRefresh, btnOpenImg, new Separator(), btnSelect);
        btnBox.setPadding(new Insets(8, 8, 10, 8));
        btnBox.setStyle("-fx-border-color: #333; -fx-border-width: 1 0 0 0;");

        getChildren().addAll(header, diskList, btnBox);

        // Initial load
        refreshDisks();
    }

    public void setOnDiskSelected(Consumer<DiskInfo> cb) { this.onDiskSelected = cb; }

    public void refreshDisks() {
        diskList.setPlaceholder(new Label("Enumerating…"));
        ioExec.submit(() -> {
            List<DiskInfo> disks = DiskService.get().listDisks();
            Platform.runLater(() -> {
                diskList.getItems().setAll(disks);
                if (disks.isEmpty())
                    diskList.setPlaceholder(new Label("No physical disks found"));
            });
        });
    }

    // ── Private ───────────────────────────────────────────────

    private void selectCurrent() {
        DiskInfo sel = diskList.getSelectionModel().getSelectedItem();
        if (sel != null && onDiskSelected != null) onDiskSelected.accept(sel);
    }

    private void openImageFile() {
        FileChooser fc = new FileChooser();
        fc.setTitle("Open Disk Image");
        fc.getExtensionFilters().addAll(
                new FileChooser.ExtensionFilter("Disk Images", "*.img", "*.dd", "*.raw", "*.iso", "*.e01", "*.aff"),
                new FileChooser.ExtensionFilter("All Files", "*.*")
        );
        File f = fc.showOpenDialog(getScene().getWindow());
        if (f == null) return;

        ioExec.submit(() -> {
            DiskInfo di = DiskService.get().openImageFile(f.getAbsolutePath());
            if (di != null) {
                Platform.runLater(() -> {
                    diskList.getItems().add(0, di);
                    diskList.getSelectionModel().select(0);
                    if (onDiskSelected != null) onDiskSelected.accept(di);
                });
            } else {
                Platform.runLater(() -> {
                    Alert a = new Alert(Alert.AlertType.ERROR, "Failed to open image file:\n" + f.getName());
                    a.showAndWait();
                });
            }
        });
    }

    private Button toolButton(String text) {
        Button btn = new Button(text);
        btn.setMaxWidth(Double.MAX_VALUE);
        btn.setAlignment(Pos.CENTER_LEFT);
        btn.setStyle("-fx-background-color: #2A2A2A; -fx-text-fill: #B0B0B0;"
                + " -fx-border-color: #444; -fx-font-size: 12px; -fx-padding: 5 10;");
        btn.setOnMouseEntered(e ->
                btn.setStyle("-fx-background-color: #333; -fx-text-fill: #E0E0E0;"
                        + " -fx-border-color: #555; -fx-font-size: 12px; -fx-padding: 5 10;"));
        btn.setOnMouseExited(e ->
                btn.setStyle("-fx-background-color: #2A2A2A; -fx-text-fill: #B0B0B0;"
                        + " -fx-border-color: #444; -fx-font-size: 12px; -fx-padding: 5 10;"));
        return btn;
    }

    // ── Cell renderer ─────────────────────────────────────────

    private static class DiskCell extends ListCell<DiskInfo> {
        private final Label icon, name, sub;
        private final VBox content;
        private final HBox row;

        DiskCell() {
            icon = new Label("💾");
            icon.setStyle("-fx-font-size: 22px;");
            name = new Label();
            name.setStyle("-fx-text-fill: #D0D0D0; -fx-font-weight: bold; -fx-font-size: 13px;");
            sub  = new Label();
            sub.setStyle("-fx-text-fill: #707070; -fx-font-size: 11px;");
            content = new VBox(2, name, sub);
            content.setAlignment(Pos.CENTER_LEFT);
            row = new HBox(10, icon, content);
            row.setAlignment(Pos.CENTER_LEFT);
            row.setPadding(new Insets(6, 8, 6, 8));
        }

        @Override
        protected void updateItem(DiskInfo item, boolean empty) {
            super.updateItem(item, empty);
            if (empty || item == null) {
                setGraphic(null); return;
            }
            icon.setText(item.isImage() ? "🗂" : "💾");
            name.setText(item.getModel().isEmpty() ? item.getPath() : item.getModel());
            sub.setText(item.getFormattedSize() + "  ·  " + item.getTypeName()
                    + "  ·  " + item.getPath());
            setGraphic(row);
            setStyle("-fx-background-color: transparent;");
        }
    }
}
