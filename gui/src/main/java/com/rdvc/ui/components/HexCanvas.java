package com.rdvc.ui.components;

import com.rdvc.forensic.BookmarkManager;
import javafx.beans.property.IntegerProperty;
import javafx.beans.property.SimpleIntegerProperty;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.input.MouseButton;
import javafx.scene.input.MouseEvent;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;

import java.util.HashSet;
import java.util.Set;
import java.util.function.BiConsumer;

/**
 * HexCanvas — Custom Canvas-based hex viewer with virtual scroll.
 *
 * Renders 3 columns: OFFSET | HEX bytes (16/row) | ASCII
 * Only visible rows are rendered (no cached JTable overhead).
 *
 * Colors (dark FTK theme):
 *   - 0x00 bytes      → dark gray
 *   - printable ASCII → white
 *   - high bytes      → yellow
 *   - selected byte   → cyan highlight
 *   - bookmarked row  → green tint on offset column
 */
public class HexCanvas extends Canvas {

    // ── Layout constants ──────────────────────────────────────
    private static final int    BYTES_PER_ROW = 16;
    public  static final double ROW_HEIGHT    = 18.0;
    private static final double FONT_SIZE   = 13.0;

    private static final double COL_OFFSET_X = 8.0;
    private static final double COL_HEX_X    = 130.0;
    private static final double COL_ASCII_X  = 610.0;
    private static final double HEX_CELL_W   = 30.0;

    // ── Colors ────────────────────────────────────────────────
    private static final Color BG           = Color.web("#1A1A1A");
    private static final Color BG_ALT       = Color.web("#1F1F1F");
    private static final Color BG_HEADER    = Color.web("#252525");
    private static final Color BG_SELECTED  = Color.web("#1A3A5C");
    private static final Color BG_BOOKMARKED= Color.web("#1A3A1A");

    private static final Color TEXT_OFFSET  = Color.web("#5E9FD8");
    private static final Color TEXT_NULL    = Color.web("#3A3A3A");
    private static final Color TEXT_PRINT   = Color.web("#D0D0D0");
    private static final Color TEXT_HIGH    = Color.web("#E8C060");
    private static final Color TEXT_ASCII   = Color.web("#9ACD7A");
    private static final Color TEXT_SELECTED= Color.CYAN;
    private static final Color TEXT_HEADER  = Color.web("#808080");
    private static final Color SEPARATOR    = Color.web("#333333");

    // ── State ─────────────────────────────────────────────────
    private byte[]  data         = new byte[0];
    private long    baseOffset   = 0;   // byte offset of data[0] on disk
    private int     selectedByte = -1;
    private double  scrollOffset = 0;   // pixel scroll within rendered area
    private int     sectorSize   = 512;
    private Set<Long> bookmarkedLbas = new HashSet<>();

    private Font monoFont;

    // JavaFX observable properties
    private final IntegerProperty totalRowsProp     = new SimpleIntegerProperty(0);
    private final IntegerProperty selectedByteProp  = new SimpleIntegerProperty(-1);

    // Callback: (lba, byteOffset) -> add bookmark
    private BiConsumer<Long, Long> bookmarkRequestHandler;

    /** Default constructor — size managed by parent layout. */
    public HexCanvas() {
        this(800, 600);
    }

    public HexCanvas(double width, double height) {
        super(width, height);
        monoFont = Font.font("Courier New", FONT_SIZE);
        if (monoFont == null) monoFont = Font.font("Monospace", FONT_SIZE);

        // Mouse click for byte selection (left = select, right = bookmark)
        addEventHandler(MouseEvent.MOUSE_CLICKED, this::onMouseClick);

        widthProperty().addListener(e -> render());
        heightProperty().addListener(e -> render());
    }

    // ── Public API ────────────────────────────────────────────

    public void setData(byte[] data, long baseOffset) {
        this.data       = data != null ? data : new byte[0];
        this.baseOffset = baseOffset;
        this.selectedByte = -1;
        render();
    }

    public void setSectorSize(int s)            { this.sectorSize = s; }
    public void setScrollOffset(double offset)  { this.scrollOffset = offset; render(); }
    public void setBookmarkedLbas(Set<Long> s)  { this.bookmarkedLbas = s; render(); }

    public int    getSelectedByteIndex()  { return selectedByte; }
    public long   getSelectedByteOffset() { return selectedByte >= 0 ? baseOffset + selectedByte : -1; }
    public byte[] getData()               { return data; }

    public IntegerProperty totalRowsProperty()    { return totalRowsProp; }
    public IntegerProperty selectedByteIndexProperty() { return selectedByteProp; }

    /** Register a handler called on right-click to add a bookmark. */
    public void setOnBookmarkRequest(BiConsumer<Long, Long> handler) {
        this.bookmarkRequestHandler = handler;
    }

    /** Returns number of data rows (full + partial) */
    public int getTotalRows() {
        return (data.length + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
    }

    /** Returns how many rows fit in the visible canvas height */
    public int getVisibleRows() {
        return (int)((getHeight() - ROW_HEIGHT) / ROW_HEIGHT);
    }

    // ── Rendering ─────────────────────────────────────────────

    public void render() {
        GraphicsContext gc = getGraphicsContext2D();
        double w = getWidth();
        double h = getHeight();

        // Clear
        gc.setFill(BG);
        gc.fillRect(0, 0, w, h);

        gc.setFont(monoFont);

        // Header row
        drawHeader(gc, w);

        // Data rows
        int startRow = (int)(scrollOffset / ROW_HEIGHT);
        int visRows  = getVisibleRows() + 1;
        double yStart = ROW_HEIGHT - (scrollOffset % ROW_HEIGHT);

        for (int row = startRow; row < startRow + visRows; row++) {
            int dataStart = row * BYTES_PER_ROW;
            if (dataStart >= data.length) break;

            double y = yStart + (row - startRow) * ROW_HEIGHT;
            drawRow(gc, row, dataStart, y, w);
        }

        // Separator lines
        gc.setStroke(SEPARATOR);
        gc.setLineWidth(1.0);
        gc.strokeLine(COL_HEX_X - 8, 0, COL_HEX_X - 8, h);
        gc.strokeLine(COL_ASCII_X - 8, 0, COL_ASCII_X - 8, h);

        // Update observable total rows
        totalRowsProp.set(getTotalRows());
    }

    private void drawHeader(GraphicsContext gc, double w) {
        gc.setFill(BG_HEADER);
        gc.fillRect(0, 0, w, ROW_HEIGHT);

        gc.setFill(TEXT_HEADER);
        gc.fillText("OFFSET (hex)", COL_OFFSET_X, ROW_HEIGHT - 4);

        for (int i = 0; i < BYTES_PER_ROW; i++) {
            gc.fillText(String.format("%02X", i),
                    COL_HEX_X + i * HEX_CELL_W, ROW_HEIGHT - 4);
        }
        gc.fillText("ASCII (0–127)", COL_ASCII_X, ROW_HEIGHT - 4);
    }

    private void drawRow(GraphicsContext gc, int row, int dataStart, double y, double w) {
        long rowOffset = baseOffset + dataStart;
        long rowLba    = rowOffset / sectorSize;

        boolean isAlt        = (row % 2 == 0);
        boolean isBookmarked = bookmarkedLbas.contains(rowLba);

        // Row background
        Color rowBg = isBookmarked ? BG_BOOKMARKED : (isAlt ? BG_ALT : BG);
        gc.setFill(rowBg);
        gc.fillRect(0, y, w, ROW_HEIGHT);

        // Offset column
        gc.setFill(TEXT_OFFSET);
        gc.fillText(String.format("%012X", rowOffset), COL_OFFSET_X, y + ROW_HEIGHT - 4);

        // HEX + ASCII columns
        int count = Math.min(BYTES_PER_ROW, data.length - dataStart);
        StringBuilder ascii = new StringBuilder();

        for (int i = 0; i < count; i++) {
            int byteIndex = dataStart + i;
            int bval = data[byteIndex] & 0xFF;

            boolean isSelected = (selectedByte == byteIndex);
            double hx = COL_HEX_X + i * HEX_CELL_W;

            // Highlight selected byte
            if (isSelected) {
                gc.setFill(BG_SELECTED);
                gc.fillRect(hx - 1, y + 1, HEX_CELL_W - 2, ROW_HEIGHT - 2);
                gc.setFill(TEXT_SELECTED);
            } else {
                if (bval == 0x00)                   gc.setFill(TEXT_NULL);
                else if (bval >= 0x20 && bval < 0x7F) gc.setFill(TEXT_PRINT);
                else                                 gc.setFill(TEXT_HIGH);
            }

            gc.fillText(String.format("%02X", bval), hx, y + ROW_HEIGHT - 4);

            // ASCII char
            char ch = (bval >= 0x20 && bval < 0x7F) ? (char)bval : '.';
            ascii.append(ch);
        }

        // ASCII column
        gc.setFill(TEXT_ASCII);
        gc.fillText(ascii.toString(), COL_ASCII_X, y + ROW_HEIGHT - 4);
    }

    // ── Mouse interaction ─────────────────────────────────────

    private void onMouseClick(MouseEvent e) {
        double y = e.getY();
        double x = e.getX();
        if (y < ROW_HEIGHT) return;  // header

        int startRow = (int)(scrollOffset / ROW_HEIGHT);
        int clickRow = startRow + (int)((y - ROW_HEIGHT + (scrollOffset % ROW_HEIGHT)) / ROW_HEIGHT);
        int dataStart = clickRow * BYTES_PER_ROW;

        if (dataStart >= data.length) return;

        // Determine which column
        if (x >= COL_HEX_X && x < COL_ASCII_X) {
            int col = (int)((x - COL_HEX_X) / HEX_CELL_W);
            if (col >= 0 && col < BYTES_PER_ROW) {
                selectedByte = dataStart + col;
                if (selectedByte >= data.length) selectedByte = -1;
                selectedByteProp.set(selectedByte);
                render();
            }
        } else if (x >= COL_ASCII_X) {
            int col = (int)((x - COL_ASCII_X) / (FONT_SIZE * 0.75));
            if (col >= 0 && col < BYTES_PER_ROW) {
                selectedByte = dataStart + col;
                if (selectedByte >= data.length) selectedByte = -1;
                selectedByteProp.set(selectedByte);
                render();
            }
        }

        // Right-click → bookmark request
        if (e.getButton() == MouseButton.SECONDARY && selectedByte >= 0
                && bookmarkRequestHandler != null) {
            long lba        = (baseOffset + selectedByte) / sectorSize;
            long byteOffset = baseOffset + selectedByte;
            bookmarkRequestHandler.accept(lba, byteOffset);
        }
    }
}
