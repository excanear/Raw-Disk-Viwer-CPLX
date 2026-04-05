package com.rdvc.ui.components;

import com.rdvc.service.DiskService;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;

import java.util.function.LongConsumer;

/**
 * SectorNavigator — toolbar for navigating sectors by LBA.
 *
 * Provides: Previous | Current LBA field | Next | Go To | Jump input
 */
public class SectorNavigator extends HBox {

    private long currentLba   = 0;
    private long totalSectors = 0;
    private LongConsumer onNavigate;

    private final Label lbaLabel;
    private final TextField lbaField;
    private final Label totalLabel;
    private final Button btnPrev;
    private final Button btnNext;
    private final Button btnGoto;
    private final Slider sectorSlider;

    public SectorNavigator() {
        super(8);
        setPadding(new Insets(6, 10, 6, 10));
        setAlignment(Pos.CENTER_LEFT);
        setStyle("-fx-background-color: #252525;");

        lbaLabel = new Label("LBA:");
        lbaLabel.setStyle("-fx-text-fill: #808080; -fx-font-size: 12px;");

        lbaField = new TextField("0");
        lbaField.setPrefWidth(110);
        lbaField.setStyle("-fx-background-color: #1A1A1A; -fx-text-fill: #D0D0D0;"
                + " -fx-border-color: #444; -fx-font-family: 'Courier New'; -fx-font-size: 13px;");
        lbaField.setOnKeyPressed(e -> {
            if (e.getCode() == KeyCode.ENTER) gotoCurrentField();
        });

        totalLabel = new Label("/ 0");
        totalLabel.setStyle("-fx-text-fill: #555555; -fx-font-size: 12px;");

        btnPrev = navButton("◀ Prev", () -> navigateTo(currentLba - 1));
        btnNext = navButton("Next ▶", () -> navigateTo(currentLba + 1));
        btnGoto = navButton("Go To", this::gotoCurrentField);

        sectorSlider = new Slider(0, 1, 0);
        sectorSlider.setPrefWidth(200);
        sectorSlider.setStyle("-fx-control-inner-background: #333;");
        sectorSlider.valueProperty().addListener((obs, ov, nv) -> {
            if (sectorSlider.isValueChanging()) {
                long lba = (long)(nv.doubleValue() * (totalSectors - 1));
                navigateTo(lba);
            }
        });

        Separator sep = new Separator(javafx.geometry.Orientation.VERTICAL);
        sep.setStyle("-fx-background-color: #444;");

        getChildren().addAll(btnPrev, lbaLabel, lbaField, totalLabel, btnGoto, btnNext,
                             sep, sectorSlider);
    }

    public void setOnNavigate(LongConsumer cb)  { this.onNavigate = cb; }
    public void setTotalSectors(long total)     { this.totalSectors = total; updateTotal(); }
    public long getCurrentLba()                 { return currentLba; }

    public void navigateTo(long lba) {
        if (lba < 0) lba = 0;
        if (totalSectors > 0 && lba >= totalSectors) lba = totalSectors - 1;
        currentLba = lba;
        lbaField.setText(String.valueOf(lba));
        updateSlider();
        updateButtonState();
        if (onNavigate != null) onNavigate.accept(lba);
    }

    // ── Private ───────────────────────────────────────────────

    private void gotoCurrentField() {
        try {
            String text = lbaField.getText().trim();
            long lba;
            if (text.startsWith("0x") || text.startsWith("0X")) {
                lba = Long.parseLong(text.substring(2), 16);
            } else {
                lba = Long.parseLong(text);
            }
            navigateTo(lba);
        } catch (NumberFormatException ignored) {
            lbaField.setText(String.valueOf(currentLba));
        }
    }

    private void updateTotal() {
        totalLabel.setText("/ " + (totalSectors > 0 ? (totalSectors - 1) : 0));
        sectorSlider.setMax(Math.max(1, totalSectors - 1));
        updateButtonState();
    }

    private void updateButtonState() {
        btnPrev.setDisable(currentLba <= 0);
        btnNext.setDisable(totalSectors > 0 && currentLba >= totalSectors - 1);
    }

    private void updateSlider() {
        if (totalSectors > 1)
            sectorSlider.setValue((double)currentLba / (totalSectors - 1));
    }

    private Button navButton(String text, Runnable action) {
        Button btn = new Button(text);
        btn.setStyle("-fx-background-color: #333; -fx-text-fill: #C0C0C0;"
                + " -fx-border-color: #555; -fx-font-size: 12px;");
        btn.setOnAction(e -> action.run());
        btn.setOnMouseEntered(e ->
                btn.setStyle("-fx-background-color: #444; -fx-text-fill: #E0E0E0;"
                        + " -fx-border-color: #666; -fx-font-size: 12px;"));
        btn.setOnMouseExited(e ->
                btn.setStyle("-fx-background-color: #333; -fx-text-fill: #C0C0C0;"
                        + " -fx-border-color: #555; -fx-font-size: 12px;"));
        return btn;
    }
}
