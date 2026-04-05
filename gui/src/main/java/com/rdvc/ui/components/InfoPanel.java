package com.rdvc.ui.components;

import com.rdvc.bridge.model.PartitionInfo;
import com.rdvc.bridge.model.SectorData;
import com.rdvc.service.DiskService;
import javafx.geometry.Insets;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;

import java.util.List;

/**
 * InfoPanel — right panel.
 *
 * Shows:
 *   • Disk metadata (path, size, sector size, total sectors)
 *   • Partition table (MBR or GPT — TreeView)
 *   • Sector analysis (entropy + class) for the currently viewed sector
 *   • Selected byte info (dec / hex / bin / ASCII)
 */
public class InfoPanel extends VBox {

    private final TreeView<String> partitionTree;
    private final TreeItem<String> rootItem;
    private final Label lblEntropy, lblClass, lblByteInfo;

    public InfoPanel() {
        super(0);
        setPrefWidth(300);
        setStyle("-fx-background-color: #1C1C1C; -fx-border-color: #333; -fx-border-width: 0 0 0 1;");

        // ── Partitions section ────────────────────────────────
        TitledPane partPane = titled("Partition Table");

        rootItem = new TreeItem<>("Partitions");
        rootItem.setExpanded(true);
        partitionTree = new TreeView<>(rootItem);
        partitionTree.setStyle("-fx-background-color: #1C1C1C; -fx-border-color: transparent;"
                + " -fx-control-inner-background: #1C1C1C; -fx-text-fill: #C0C0C0;");
        partitionTree.setPrefHeight(220);
        partitionTree.setShowRoot(false);
        VBox partContent = new VBox(partitionTree);
        partContent.setPadding(new Insets(4));
        partContent.setStyle("-fx-background-color: #1C1C1C;");
        partPane.setContent(partContent);

        // ── Sector analysis section ───────────────────────────
        TitledPane analysisPane = titled("Sector Analysis");
        lblEntropy = infoLabel("—");
        lblClass   = infoLabel("—");
        GridPane grid = twoColGrid(
                "Entropy", lblEntropy,
                "Class",   lblClass
        );
        VBox analysisContent = new VBox(grid);
        analysisContent.setPadding(new Insets(6, 8, 6, 8));
        analysisContent.setStyle("-fx-background-color: #1C1C1C;");
        analysisPane.setContent(analysisContent);

        // ── Byte info section ─────────────────────────────────
        TitledPane bytePane = titled("Selected Byte");
        lblByteInfo = new Label("No selection");
        lblByteInfo.setStyle("-fx-text-fill: #909090; -fx-font-family: 'Courier New';"
                + " -fx-font-size: 12px; -fx-padding: 6 8;");
        lblByteInfo.setWrapText(true);
        VBox byteContent = new VBox(lblByteInfo);
        byteContent.setStyle("-fx-background-color: #1C1C1C;");
        bytePane.setContent(byteContent);

        getChildren().addAll(partPane, analysisPane, bytePane);
        VBox.setVgrow(partPane, Priority.ALWAYS);
    }

    /** Populate partition tree from partition list (called after disk is opened). */
    public void showPartitions(List<PartitionInfo> partitions) {
        rootItem.getChildren().clear();

        if (partitions == null || partitions.isEmpty()) {
            rootItem.getChildren().add(new TreeItem<>("(no partitions found)"));
            return;
        }

        // Group by scheme
        boolean isGpt = partitions.stream().anyMatch(p -> p.getScheme() == PartitionInfo.SchemeType.GPT);
        TreeItem<String> schemeNode = new TreeItem<>(isGpt ? "GPT" : "MBR");
        schemeNode.setExpanded(true);

        for (PartitionInfo p : partitions) {
            String label = String.format("Part %d  [%s]  %s  LBA %s–%s",
                    p.getIndex(),
                    p.getScheme().name(),
                    p.getTypeName(),
                    p.getStartLba(),
                    p.getEndLba());
            TreeItem<String> node = new TreeItem<>(label);
            if (p.getName() != null && !p.getName().isEmpty() && !p.getName().equals(p.getTypeName()))
                node.getChildren().add(new TreeItem<>("Name: " + p.getName()));
            node.getChildren().add(new TreeItem<>("Size: " + formatSize(p.getSizeBytes())));
            node.getChildren().add(new TreeItem<>("Bootable: " + (p.isBootable() ? "Yes" : "No")));
            if (p.getGuid() != null && !p.getGuid().isEmpty())
                node.getChildren().add(new TreeItem<>("GUID: " + p.getGuid()));
            schemeNode.getChildren().add(node);
        }

        rootItem.getChildren().add(schemeNode);
    }

    /** Update entropy display for the current sector. */
    public void showEntropy(double entropy, String classification) {
        lblEntropy.setText(String.format("%.4f", entropy));
        lblClass.setText(classification);
        // Color the class label based on threat level
        String color = switch (classification) {
            case "ENCRYPTED"  -> "#FF6060";
            case "COMPRESSED" -> "#E8C060";
            case "TEXT"       -> "#80D880";
            case "EMPTY"      -> "#606060";
            default           -> "#A0A0FF";
        };
        lblClass.setStyle("-fx-text-fill: " + color + "; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");
    }

    /** Update selected byte info display. */
    public void showSelectedByte(int byteValue) {
        if (byteValue < 0) {
            lblByteInfo.setText("No selection");
            return;
        }
        byteValue &= 0xFF;
        String ascii = (byteValue >= 0x20 && byteValue <= 0x7E)
                ? "'" + (char)byteValue + "'" : "(non-print)";
        lblByteInfo.setText(String.format(
                "Dec: %d%nHex: 0x%02X%nBin: %s%nASCII: %s",
                byteValue, byteValue,
                String.format("%8s", Integer.toBinaryString(byteValue)).replace(' ', '0'),
                ascii));
    }

    // ── Helpers ───────────────────────────────────────────────

    private TitledPane titled(String title) {
        TitledPane tp = new TitledPane();
        tp.setText(title);
        tp.setCollapsible(true);
        tp.setExpanded(true);
        tp.setStyle("-fx-text-fill: #909090; -fx-background-color: #222; -fx-border-color: #333;");
        return tp;
    }

    private Label infoLabel(String text) {
        Label l = new Label(text);
        l.setStyle("-fx-text-fill: #C0C0C0; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");
        return l;
    }

    private GridPane twoColGrid(String k1, Label v1, String k2, Label v2) {
        GridPane g = new GridPane();
        g.setHgap(12); g.setVgap(4);
        g.setPadding(new Insets(4, 0, 4, 0));
        Label lk1 = key(k1), lk2 = key(k2);
        g.addRow(0, lk1, v1);
        g.addRow(1, lk2, v2);
        return g;
    }

    private Label key(String text) {
        Label l = new Label(text + ":");
        l.setStyle("-fx-text-fill: #606060; -fx-font-size: 11px;");
        return l;
    }

    private String formatSize(long bytes) {
        if (bytes < 1024) return bytes + " B";
        if (bytes < 1024 * 1024) return String.format("%.1f KB", bytes / 1024.0);
        if (bytes < 1024L * 1024 * 1024) return String.format("%.1f MB", bytes / (1024.0 * 1024));
        return String.format("%.2f GB", bytes / (1024.0 * 1024 * 1024));
    }
}
