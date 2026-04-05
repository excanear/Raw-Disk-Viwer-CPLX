package com.rdvc.ui.components;

import com.rdvc.forensic.ForensicLogger;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.stage.FileChooser;

import java.io.*;
import java.time.Instant;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * ForensicLogPanel — bottom panel displaying the live audit log.
 *
 * Tails the in-memory log produced by ForensicLogger and appends new lines.
 * A background daemon thread polls for new entries every 500 ms.
 */
public class ForensicLogPanel extends BorderPane {

    private final TextArea logArea;
    private int  lastLineCount = 0;
    private final AtomicBoolean running = new AtomicBoolean(true);

    public ForensicLogPanel() {
        setPrefHeight(160);
        setStyle("-fx-background-color: #141414; -fx-border-color: #333; -fx-border-width: 1 0 0 0;");

        // ── Header bar ────────────────────────────────────────
        Label title = new Label("FORENSIC AUDIT LOG");
        title.setStyle("-fx-text-fill: #808080; -fx-font-size: 11px; -fx-font-weight: bold;");

        Button btnClear  = smallBtn("Clear");
        Button btnExport = smallBtn("Export…");
        btnClear.setOnAction(e -> clearLog());
        btnExport.setOnAction(e -> exportLog());

        HBox header = new HBox(8, title);
        header.getChildren().addAll(new Region(), btnClear, btnExport);
        HBox.setHgrow(header.getChildren().get(1), Priority.ALWAYS);
        header.setPadding(new Insets(5, 8, 5, 10));
        header.setStyle("-fx-border-color: #2A2A2A; -fx-border-width: 0 0 1 0;");

        // ── Text area ─────────────────────────────────────────
        logArea = new TextArea();
        logArea.setEditable(false);
        logArea.setWrapText(false);
        logArea.setStyle(
                "-fx-control-inner-background: #0E0E0E;"
                + " -fx-text-fill: #70A070;"
                + " -fx-font-family: 'Courier New';"
                + " -fx-font-size: 11px;"
                + " -fx-background-color: #0E0E0E;"
        );

        setTop(header);
        setCenter(logArea);

        startTailThread();
    }

    public void appendLine(String line) {
        Platform.runLater(() -> {
            logArea.appendText(line + "\n");
        });
    }

    public void shutdown() { running.set(false); }

    // ── Private ───────────────────────────────────────────────

    private void startTailThread() {
        Thread t = new Thread(() -> {
            while (running.get()) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
                java.util.List<String> lines = ForensicLogger.get().getLines();
                if (lines.size() > lastLineCount) {
                    java.util.List<String> newLines = lines.subList(lastLineCount, lines.size());
                    lastLineCount = lines.size();
                    String chunk = String.join("\n", newLines) + "\n";
                    Platform.runLater(() -> logArea.appendText(chunk));
                }
            }
        }, "log-tail");
        t.setDaemon(true);
        t.start();
    }

    private void clearLog() {
        ForensicLogger.get().clearMemory();
        lastLineCount = 0;
        logArea.clear();
    }

    private void exportLog() {
        FileChooser fc = new FileChooser();
        fc.setTitle("Export Audit Log");
        fc.getExtensionFilters().add(new FileChooser.ExtensionFilter("Text files", "*.txt", "*.log"));
        fc.setInitialFileName("rdvc-audit-" + Instant.now().getEpochSecond() + ".log");
        File f = fc.showSaveDialog(getScene().getWindow());
        if (f == null) return;
        try (PrintWriter pw = new PrintWriter(new FileWriter(f))) {
            for (String line : ForensicLogger.get().getLines()) pw.println(line);
        } catch (IOException ex) {
            new Alert(Alert.AlertType.ERROR, "Failed to export log: " + ex.getMessage()).showAndWait();
        }
    }

    private Button smallBtn(String text) {
        Button btn = new Button(text);
        btn.setStyle("-fx-background-color: #2A2A2A; -fx-text-fill: #909090;"
                + " -fx-border-color: #444; -fx-font-size: 11px; -fx-padding: 2 8;");
        return btn;
    }
}
