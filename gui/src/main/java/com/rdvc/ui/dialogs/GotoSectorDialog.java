package com.rdvc.ui.dialogs;

import javafx.geometry.Insets;
import javafx.scene.control.*;
import javafx.scene.layout.*;

import java.util.Optional;
import java.util.OptionalLong;

/**
 * GotoSectorDialog — prompts the user for a target LBA and validates the range.
 */
public class GotoSectorDialog extends Dialog<Long> {

    public GotoSectorDialog(long currentLba, long totalSectors) {
        setTitle("Go To Sector");
        setHeaderText(null);
        getDialogPane().getStyleClass().add("dark-dialog");

        // ── Form ──────────────────────────────────────────────
        Label lbl = new Label("Enter LBA (hex with 0x prefix or decimal):");
        lbl.setStyle("-fx-text-fill: #C0C0C0;");

        TextField field = new TextField(String.valueOf(currentLba));
        field.setStyle("-fx-background-color: #1A1A1A; -fx-text-fill: #D0D0D0;"
                + " -fx-border-color: #555; -fx-font-family: 'Courier New'; -fx-font-size: 13px;");
        field.setPrefWidth(250);
        field.selectAll();

        Label rangeHint = new Label("Valid range: 0 – " + (totalSectors - 1));
        rangeHint.setStyle("-fx-text-fill: #606060; -fx-font-size: 11px;");

        Label errLabel = new Label();
        errLabel.setStyle("-fx-text-fill: #FF6060; -fx-font-size: 11px;");

        VBox content = new VBox(8, lbl, field, rangeHint, errLabel);
        content.setPadding(new Insets(14, 20, 10, 20));
        content.setStyle("-fx-background-color: #1C1C1C;");
        getDialogPane().setContent(content);

        // ── Buttons ───────────────────────────────────────────
        ButtonType goBtn = new ButtonType("Go To", ButtonBar.ButtonData.OK_DONE);
        getDialogPane().getButtonTypes().addAll(goBtn, ButtonType.CANCEL);
        getDialogPane().setStyle("-fx-background-color: #1C1C1C;");

        // Validation on OK press
        Button okButton = (Button) getDialogPane().lookupButton(goBtn);
        okButton.addEventFilter(javafx.event.ActionEvent.ACTION, e -> {
            String text = field.getText().trim();
            try {
                long lba = text.startsWith("0x") || text.startsWith("0X")
                        ? Long.parseLong(text.substring(2), 16)
                        : Long.parseLong(text);
                if (lba < 0 || (totalSectors > 0 && lba >= totalSectors)) {
                    errLabel.setText("LBA out of range.");
                    e.consume();
                }
            } catch (NumberFormatException ex) {
                errLabel.setText("Invalid number format.");
                e.consume();
            }
        });

        // ── Result converter ──────────────────────────────────
        setResultConverter(bt -> {
            if (bt != goBtn) return null;
            String text = field.getText().trim();
            try {
                return text.startsWith("0x") || text.startsWith("0X")
                        ? Long.parseLong(text.substring(2), 16)
                        : Long.parseLong(text);
            } catch (NumberFormatException ex) {
                return null;
            }
        });
    }

    /** Convenience factory — returns empty if cancelled or invalid. */
    public static OptionalLong show(long currentLba, long totalSectors) {
        GotoSectorDialog dlg = new GotoSectorDialog(currentLba, totalSectors);
        Optional<Long> result = dlg.showAndWait();
        return result.map(OptionalLong::of).orElseGet(OptionalLong::empty);
    }
}
