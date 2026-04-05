package com.rdvc.ui.components;

import com.rdvc.forensic.BookmarkManager;
import com.rdvc.forensic.BookmarkManager.Bookmark;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.*;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.shape.Rectangle;

import java.util.List;
import java.util.function.LongConsumer;

/**
 * BookmarkPanel — shows saved bookmarks and provides Add/Delete/Navigate actions.
 */
public class BookmarkPanel extends VBox {

    private final ListView<Bookmark> listView;
    private LongConsumer onJumpTo;

    public BookmarkPanel() {
        super(0);
        setPrefWidth(280);
        setStyle("-fx-background-color: #1C1C1C; -fx-border-color: #333; -fx-border-width: 0 1 0 0;");

        Label title = new Label("BOOKMARKS");
        title.setStyle("-fx-text-fill: #808080; -fx-font-size: 11px; -fx-font-weight: bold;");
        HBox header = new HBox(title);
        header.setPadding(new Insets(10, 10, 8, 10));
        header.setStyle("-fx-border-color: #2A2A2A; -fx-border-width: 0 0 1 0;");

        listView = new ListView<>();
        listView.setStyle("-fx-background-color: #1C1C1C; -fx-border-color: transparent;");
        listView.setCellFactory(lv -> new BookmarkCell());
        listView.setOnMouseClicked(e -> {
            if (e.getClickCount() == 2) jumpToSelected();
        });
        VBox.setVgrow(listView, Priority.ALWAYS);

        Button btnJump   = toolBtn("▶  Jump To");
        Button btnDelete = toolBtn("✕  Delete");
        btnJump.setOnAction(e -> jumpToSelected());
        btnDelete.setOnAction(e -> deleteSelected());

        HBox actions = new HBox(6, btnJump, btnDelete);
        actions.setPadding(new Insets(8));
        actions.setStyle("-fx-border-color: #333; -fx-border-width: 1 0 0 0;");

        getChildren().addAll(header, listView, actions);
        refresh();
    }

    public void setOnJumpTo(LongConsumer cb) { this.onJumpTo = cb; }

    public void refresh() {
        listView.getItems().setAll(BookmarkManager.get().getAll());
    }

    // ── Private ───────────────────────────────────────────────

    private void jumpToSelected() {
        Bookmark sel = listView.getSelectionModel().getSelectedItem();
        if (sel != null && onJumpTo != null) onJumpTo.accept(sel.getLba());
    }

    private void deleteSelected() {
        Bookmark sel = listView.getSelectionModel().getSelectedItem();
        if (sel != null) {
            BookmarkManager.get().remove(sel);
            refresh();
        }
    }

    private Button toolBtn(String text) {
        Button btn = new Button(text);
        btn.setStyle("-fx-background-color: #2A2A2A; -fx-text-fill: #B0B0B0;"
                + " -fx-border-color: #444; -fx-font-size: 12px;");
        return btn;
    }

    // ── Cell ─────────────────────────────────────────────────

    private static class BookmarkCell extends ListCell<Bookmark> {
        private final Rectangle flag = new Rectangle(8, 36);
        private final Label lba  = new Label();
        private final Label note = new Label();
        private final HBox  row;

        BookmarkCell() {
            lba.setStyle("-fx-text-fill: #5E9FD8; -fx-font-family: 'Courier New'; -fx-font-size: 12px;");
            note.setStyle("-fx-text-fill: #909090; -fx-font-size: 11px;");
            VBox info = new VBox(2, lba, note);
            info.setAlignment(Pos.CENTER_LEFT);
            row = new HBox(8, flag, info);
            row.setAlignment(Pos.CENTER_LEFT);
            row.setPadding(new Insets(4, 8, 4, 8));
        }

        @Override
        protected void updateItem(Bookmark item, boolean empty) {
            super.updateItem(item, empty);
            if (empty || item == null) { setGraphic(null); return; }
            lba.setText("LBA " + item.getLba() + "  [Byte 0x" + Long.toHexString(item.getByteOffset()).toUpperCase() + "]");
            note.setText(item.getNote() != null && !item.getNote().isEmpty() ? item.getNote() : "(no note)");
            try {
                flag.setFill(Color.web(item.getFlagColor() != null ? item.getFlagColor() : "#FFFFFF"));
            } catch (Exception e) {
                flag.setFill(Color.WHITE);
            }
            setGraphic(row);
            setStyle("-fx-background-color: transparent;");
        }
    }
}
