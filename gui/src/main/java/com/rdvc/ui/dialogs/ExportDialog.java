package com.rdvc.ui.dialogs;

import com.rdvc.bridge.model.CarvedFile;
import com.rdvc.service.DiskService;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.stage.DirectoryChooser;

import java.io.File;
import java.util.Optional;

/**
 * ExportDialog — choose between exporting raw sectors or a carved file.
 *
 * Returns an {@link ExportRequest} describing what to export and where.
 */
public class ExportDialog extends Dialog<ExportDialog.ExportRequest> {

    public enum ExportMode { SECTORS, CARVED_FILE }

    public record ExportRequest(ExportMode mode, long startLba, long count,
                                CarvedFile carvedFile, String destDir) {}

    public ExportDialog(long currentLba, long totalSectors) {
        this(currentLba, totalSectors, null);
    }

    public ExportDialog(long currentLba, long totalSectors, CarvedFile carvedFile) {
        setTitle("Export Data");
        setHeaderText(null);
        getDialogPane().setStyle("-fx-background-color: #1C1C1C;");

        // ── Mode toggle ───────────────────────────────────────
        ToggleGroup modeGroup = new ToggleGroup();
        RadioButton rbSectors = new RadioButton("Export Raw Sectors");
        RadioButton rbCarved  = new RadioButton("Export Carved File");
        rbSectors.setToggleGroup(modeGroup);
        rbCarved.setToggleGroup(modeGroup);
        rbSectors.setStyle("-fx-text-fill: #C0C0C0;");
        rbCarved.setStyle("-fx-text-fill: #C0C0C0;");
        rbSectors.setSelected(carvedFile == null);
        rbCarved.setSelected(carvedFile != null);
        rbCarved.setDisable(carvedFile == null);

        // ── Sector range ──────────────────────────────────────
        Label lblStart = key("Start LBA:");
        Label lblCount = key("Sector Count:");
        TextField tfStart = styledField(String.valueOf(currentLba));
        TextField tfCount = styledField("1");
        GridPane sectorForm = new GridPane();
        sectorForm.setHgap(10); sectorForm.setVgap(6);
        sectorForm.addRow(0, lblStart, tfStart);
        sectorForm.addRow(1, lblCount, tfCount);

        // ── Carved file info ─────────────────────────────────
        Label carvedInfo = new Label(carvedFile != null
                ? carvedFile.getExtension() + "  " + carvedFile.getFormattedSize()
                  + "  @LBA " + carvedFile.getStartLba()
                : "No carved file selected");
        carvedInfo.setStyle("-fx-text-fill: #5E9FD8; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");

        // ── Destination ───────────────────────────────────────
        Label lblDest = key("Destination folder:");
        TextField tfDest = styledField(System.getProperty("user.home"));
        tfDest.setPrefWidth(280);
        Button btnBrowse = new Button("Browse…");
        btnBrowse.setStyle("-fx-background-color: #333; -fx-text-fill: #C0C0C0; -fx-border-color: #555;");
        btnBrowse.setOnAction(e -> {
            DirectoryChooser dc = new DirectoryChooser();
            dc.setTitle("Select Destination Folder");
            File chosen = dc.showDialog(getDialogPane().getScene().getWindow());
            if (chosen != null) tfDest.setText(chosen.getAbsolutePath());
        });
        HBox destRow = new HBox(6, tfDest, btnBrowse);

        // Show/hide panels based on mode
        sectorForm.setVisible(carvedFile == null);
        carvedInfo.setVisible(carvedFile != null);
        modeGroup.selectedToggleProperty().addListener((obs, ov, nv) -> {
            boolean isSector = nv == rbSectors;
            sectorForm.setVisible(isSector);
            sectorForm.setManaged(isSector);
            carvedInfo.setVisible(!isSector);
            carvedInfo.setManaged(!isSector);
        });

        Label errLabel = new Label();
        errLabel.setStyle("-fx-text-fill: #FF6060; -fx-font-size: 11px;");

        VBox content = new VBox(10,
                new HBox(12, rbSectors, rbCarved), new Separator(),
                sectorForm, carvedInfo, new Separator(),
                new HBox(10, lblDest, destRow), errLabel);
        content.setPadding(new Insets(14, 20, 10, 20));
        content.setStyle("-fx-background-color: #1C1C1C;");
        getDialogPane().setContent(content);

        ButtonType exportBtn = new ButtonType("Export", ButtonBar.ButtonData.OK_DONE);
        getDialogPane().getButtonTypes().addAll(exportBtn, ButtonType.CANCEL);
        getDialogPane().lookupButton(exportBtn).setStyle(
                "-fx-background-color: #1A3A5C; -fx-text-fill: #80C8FF; -fx-border-color: #2A5A8C;");

        // ── Validation ────────────────────────────────────────
        ((Button)getDialogPane().lookupButton(exportBtn)).addEventFilter(
                javafx.event.ActionEvent.ACTION, e -> {
            if (tfDest.getText().trim().isEmpty()) {
                errLabel.setText("Destination folder is required."); e.consume(); return;
            }
            if (modeGroup.getSelectedToggle() == rbSectors) {
                try { Long.parseLong(tfStart.getText().trim()); }
                catch (NumberFormatException ex) {
                    errLabel.setText("Invalid start LBA."); e.consume(); return;
                }
                try {
                    long c = Long.parseLong(tfCount.getText().trim());
                    if (c < 1) throw new NumberFormatException();
                } catch (NumberFormatException ex) {
                    errLabel.setText("Sector count must be ≥ 1."); e.consume();
                }
            }
        });

        // ── Result ────────────────────────────────────────────
        setResultConverter(bt -> {
            if (bt != exportBtn) return null;
            if (modeGroup.getSelectedToggle() == rbSectors) {
                return new ExportRequest(ExportMode.SECTORS,
                        Long.parseLong(tfStart.getText().trim()),
                        Long.parseLong(tfCount.getText().trim()),
                        null, tfDest.getText().trim());
            } else {
                return new ExportRequest(ExportMode.CARVED_FILE, 0, 0, carvedFile,
                        tfDest.getText().trim());
            }
        });
    }

    private Label key(String text) {
        Label l = new Label(text);
        l.setStyle("-fx-text-fill: #808080;");
        return l;
    }

    private TextField styledField(String init) {
        TextField tf = new TextField(init);
        tf.setStyle("-fx-background-color: #1A1A1A; -fx-text-fill: #D0D0D0;"
                + " -fx-border-color: #555; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");
        return tf;
    }
}
