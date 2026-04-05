package com.rdvc.ui.dialogs;

import javafx.application.Platform;
import javafx.concurrent.Task;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.layout.*;

/**
 * ScanProgressDialog — modal dialog that monitors a JavaFX background Task.
 *
 * Shows a ProgressBar, status label and Cancel button.
 * Closes automatically when the Task finishes (succeeded, failed, cancelled).
 */
public class ScanProgressDialog<T> extends Dialog<Void> {

    private final Task<T> task;
    private final ProgressBar progressBar;
    private final Label       statusLabel;
    private final Button      cancelButton;

    public ScanProgressDialog(String title, Task<T> task) {
        this.task = task;
        setTitle(title);
        setHeaderText(null);
        setResizable(false);
        getDialogPane().setStyle("-fx-background-color: #1C1C1C;");

        // ── Progress bar ──────────────────────────────────────
        progressBar = new ProgressBar();
        progressBar.setPrefWidth(420);
        progressBar.setStyle("-fx-accent: #5E9FD8;");
        progressBar.progressProperty().bind(task.progressProperty());

        // ── Status label ──────────────────────────────────────
        statusLabel = new Label("Initialising…");
        statusLabel.setStyle("-fx-text-fill: #C0C0C0; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");
        statusLabel.setMaxWidth(420);
        statusLabel.setWrapText(true);
        task.messageProperty().addListener((obs, ov, nv) ->
                Platform.runLater(() -> statusLabel.setText(nv)));

        // ── Cancel ────────────────────────────────────────────
        cancelButton = new Button("Cancel");
        cancelButton.setStyle("-fx-background-color: #3A2020; -fx-text-fill: #FF8080; -fx-border-color: #664444;");
        cancelButton.setOnAction(e -> task.cancel());

        VBox content = new VBox(12, progressBar, statusLabel, cancelButton);
        content.setPadding(new Insets(20, 24, 16, 24));
        content.setAlignment(Pos.CENTER_LEFT);
        content.setStyle("-fx-background-color: #1C1C1C;");
        getDialogPane().setContent(content);
        getDialogPane().getButtonTypes().add(ButtonType.CLOSE);
        getDialogPane().lookupButton(ButtonType.CLOSE).setVisible(false);

        // ── Auto-close ────────────────────────────────────────
        task.setOnSucceeded(e  -> Platform.runLater(this::closeDialog));
        task.setOnFailed(e     -> Platform.runLater(() -> {
            statusLabel.setText("Error: " + task.getException().getMessage());
            cancelButton.setDisable(true);
            getDialogPane().lookupButton(ButtonType.CLOSE).setVisible(true);
        }));
        task.setOnCancelled(e  -> Platform.runLater(this::closeDialog));
    }

    /** Show the dialog and start the task on a daemon thread. */
    public void showAndRun() {
        Thread thread = new Thread(task, "rdvc-scan");
        thread.setDaemon(true);
        thread.start();
        showAndWait();
    }

    private void closeDialog() {
        setResult(null);
        close();
    }
}
