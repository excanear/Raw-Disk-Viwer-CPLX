package com.rdvc.ui.dialogs;

import com.rdvc.forensic.BookmarkManager;
import com.rdvc.forensic.BookmarkManager.Bookmark;
import javafx.geometry.Insets;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;

import java.time.Instant;
import java.util.Optional;

/**
 * BookmarkDialog — add or edit a bookmark at a given LBA / byte offset.
 */
public class BookmarkDialog extends Dialog<Bookmark> {

    private static final String[] FLAG_COLORS = {
        "#FF4444", "#FF9900", "#FFFF44", "#44FF44", "#44AAFF", "#CC44FF", "#FFFFFF"
    };
    private static final String[] FLAG_NAMES  = {
        "Red", "Orange", "Yellow", "Green", "Blue", "Purple", "White"
    };

    public BookmarkDialog(long lba, long byteOffset, String diskPath) {
        this(lba, byteOffset, diskPath, null);
    }

    /** Edit constructor — pre-fills from an existing bookmark. */
    public BookmarkDialog(long lba, long byteOffset, String diskPath, Bookmark existing) {
        setTitle(existing == null ? "Add Bookmark" : "Edit Bookmark");
        setHeaderText(null);
        getDialogPane().setStyle("-fx-background-color: #1C1C1C;");

        // ── Location row ─────────────────────────────────────
        Label lblLoc = new Label(String.format("LBA: %,d    Byte Offset: 0x%X", lba, byteOffset));
        lblLoc.setStyle("-fx-text-fill: #5E9FD8; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");

        // ── Note field ────────────────────────────────────────
        Label lblNote = key("Note:");
        TextField noteField = new TextField(existing != null ? existing.getNote() : "");
        noteField.setStyle("-fx-background-color: #1A1A1A; -fx-text-fill: #D0D0D0; -fx-border-color: #555;");
        noteField.setPrefWidth(300);

        // ── Flag color picker ─────────────────────────────────
        Label lblColor = key("Flag color:");
        ToggleGroup colorGroup = new ToggleGroup();
        HBox colorBox = new HBox(6);
        String initColor = existing != null ? existing.getFlagColor() : FLAG_COLORS[4];
        for (int i = 0; i < FLAG_COLORS.length; i++) {
            RadioButton rb = new RadioButton();
            rb.setToggleGroup(colorGroup);
            rb.setUserData(FLAG_COLORS[i]);
            rb.setSelected(FLAG_COLORS[i].equals(initColor));
            rb.setStyle("-fx-mark-color: " + FLAG_COLORS[i] + ";");
            Tooltip.install(rb, new Tooltip(FLAG_NAMES[i]));
            colorBox.getChildren().add(rb);
        }

        GridPane form = new GridPane();
        form.setHgap(12); form.setVgap(10);
        form.setPadding(new Insets(14, 20, 10, 20));
        form.setStyle("-fx-background-color: #1C1C1C;");
        form.add(lblLoc,   0, 0, 2, 1);
        form.add(lblNote,  0, 1); form.add(noteField, 1, 1);
        form.add(lblColor, 0, 2); form.add(colorBox,  1, 2);
        getDialogPane().setContent(form);

        ButtonType saveBtn = new ButtonType("Save", ButtonBar.ButtonData.OK_DONE);
        getDialogPane().getButtonTypes().addAll(saveBtn, ButtonType.CANCEL);

        setResultConverter(bt -> {
            if (bt != saveBtn) return null;
            String color = FLAG_COLORS[4];
            if (colorGroup.getSelectedToggle() != null)
                color = (String) colorGroup.getSelectedToggle().getUserData();

            if (existing != null) {
                existing.setNote(noteField.getText().trim());
                existing.setFlagColor(color);
                return existing;
            } else {
                Bookmark b = new Bookmark(lba, byteOffset, noteField.getText().trim(), diskPath);
                b.setFlagColor(color);
                return b;
            }
        });
    }

    /** Convenience — shows dialog and saves result into BookmarkManager if confirmed. */
    public static void showAndSave(long lba, long byteOffset, String diskPath) {
        Optional<Bookmark> result = new BookmarkDialog(lba, byteOffset, diskPath).showAndWait();
        result.ifPresent(b -> BookmarkManager.get().add(b.getLba(), b.getByteOffset(), b.getNote(), b.getDiskPath()));
    }

    private Label key(String text) {
        Label l = new Label(text);
        l.setStyle("-fx-text-fill: #808080; -fx-font-size: 12px;");
        return l;
    }
}
