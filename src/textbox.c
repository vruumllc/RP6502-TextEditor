// ---------------------------------------------------------------------------
// textbox.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "doc.h"
#include "colors.h"
#include "display.h"
#include "textbox.h"

typedef enum {BLINK_ON, BLINK_OFF} cursor_state_t;
typedef enum {UNMARKED, MARKING, MARKED} mark_state_t;
typedef struct mark_pt {
    int16_t row;
    int16_t col;
} mark_pt_t;

textbox_t TheTextbox = {
    1,                  // r
    0,                  // c
    DOC_ROWS_DISPLAYED, // h
    80,                 // w
    BLACK,              // bg
    LIGHT_GRAY,         // fg
    true,               // in_focus
    {false, false, false, false, false, false, false,
     false, false, false, false, false, false, false,
     false, false, false, false, false, false, false,
     false, false, false, false, false, false, false} // row_dirty[28]
};

char TheClipboard[CLIPBOARD_SIZE] = {0};

// only ever one cursor, so save state here
static cursor_state_t cur_state = BLINK_OFF;
static uint8_t cur_c; // as drawn on display
static uint8_t cur_r; // as drawn on display

static const uint8_t cur_threshold = 30; // cursor blink delay = 1/2 second
static const uint8_t update_threshold = 3; // redraw freq = 20x/second

static void * p_popup = NULL; //unless popup is overlapping display
static uint8_t popuptype = 0; // INVALID


// mark_pt_t row, col in doc cursor coords
static mark_pt_t mark_start = {-1, -1};
static mark_pt_t mark_end = {-1, -1};
static mark_state_t mark_state = UNMARKED;
static uint16_t mark_min_r = 0;
static uint16_t mark_min_c = 0;
static uint16_t mark_max_r = 0;
static uint16_t mark_max_c = 0;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void InitTextbox(void)
{
    ClearDoc(false);

    // force redraw of textbox background
    mark_start.row = mark_end.row = TheDoc.cursor_r;
    mark_start.col = mark_end.col = TheDoc.cursor_c;
    mark_state = UNMARKED;
    SetAllTextboxRowsDirty();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateCursor()
{
    char ch;
    uint8_t fg, bg, new_row, new_col;

    if (p_popup != NULL && popuptype == FILEDIALOG) {
        new_row = TheDoc.cur_filename_r;
        new_col = TheDoc.cur_filename_c;
    } else {
        // if cursor is beyond doc's last row, move it to last row
        if (TheDoc.cursor_r > TheDoc.last_row) {
            TheDoc.cursor_r = TheDoc.last_row;
        }
        new_row = TheTextbox.r + TheDoc.cursor_r - TheDoc.offset_r;
        // if cursor is beyond new rows's length, move it left appropriately
        if (TheDoc.cursor_c > TheDoc.rows[new_row + TheDoc.offset_r - TheTextbox.r].len) {
            TheDoc.cursor_c = TheDoc.rows[new_row + TheDoc.offset_r - TheTextbox.r].len;
        }
        new_col = TheTextbox.c + TheDoc.cursor_c;
    }

    if (cur_state == BLINK_ON) {
        // get current data
        GetChar(cur_r, cur_c, &ch, &bg, &fg);
        // restore original colors
        DrawChar(cur_r, cur_c, ch, fg, bg);
        cur_state = BLINK_OFF;
    } else {
        // don't update cursor
        // if popup other than FILEDIALOG is present,
        // or in Main menu or status bar
        if ((p_popup != NULL && popuptype != FILEDIALOG) ||
            (new_row == 0 || new_row == DOC_ROWS_DISPLAYED+1)) {
            return;
        } else if (TheTextbox.in_focus) {
            bool moving = (new_row != cur_r || new_col != cur_c);
            uint8_t r = moving ? new_row : cur_r;
            uint8_t c = moving ? new_col : cur_c;
            // get current data
            GetChar(r, c, &ch, &bg, &fg);
            // reverse colors to show cursor
            DrawChar(r, c, ch, fg, bg);
            cur_state = BLINK_ON;
        }
    }
    cur_r = new_row;
    cur_c = new_col;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateTextboxFocus(bool has_focus)
{
    TheTextbox.in_focus = has_focus;
    if (TheTextbox.in_focus == false) {
        if (cur_state == BLINK_ON) {
            char ch;
            uint8_t fg, bg;
            // get current data
            GetChar(cur_r, cur_c, &ch, &bg, &fg);
            // restore original colors
            DrawChar(cur_r, cur_c, ch, fg, bg);
            cur_state = BLINK_OFF;
        }
    }
    UpdateCursor();
}

// ----------------------------------------------------------------------------
// NOTE: Marking limit is determined by left edge of block cursor
// ----------------------------------------------------------------------------
static void ComputeMarkLimits(void)
{
    if (mark_start.row < mark_end.row) { // marked top to bottom
        mark_min_r = mark_start.row;
        mark_max_r = mark_end.row;
        mark_min_c = mark_start.col;
        if (mark_end.col > 0) {
            mark_max_c = mark_end.col-1;
        } else {
            mark_max_r--;
            mark_max_c = TheDoc.rows[mark_max_r].len+1;
        }
    } else if (mark_start.row > mark_end.row) { // marked bottom to top
        mark_min_r = mark_end.row;
        mark_max_r = mark_start.row;
        mark_min_c = mark_end.col;
        if (mark_start.col > 0) {
            mark_max_c = mark_start.col-1;
        } else {
            mark_max_r--;
            mark_max_c = TheDoc.rows[mark_max_r].len+1;
        }
    } else { // single row
        mark_min_r = mark_max_r = mark_start.row;
        if (mark_start.col < mark_end.col) { // marked left to right
            mark_min_c = mark_start.col;
            mark_max_c = mark_end.col-1;
        } else { // marked right to left
            mark_min_c = mark_end.col;
            mark_max_c = mark_start.col-1;
        }
    }
}

// ---------------------------------------------------------------------------
// Called by main loop periodically to redraw document in textbox
// ---------------------------------------------------------------------------
void UpdateTextbox(void)
{
    static uint16_t cursor_timer = 0;
    static uint16_t update_timer = 0;

    // update timer counts
    cursor_timer++;
    update_timer++;

    // blink the cursor
    if (cursor_timer > cur_threshold) {
        cursor_timer = 0;
        UpdateCursor();
    }

    // redraw any dirty or marked rows
    if (update_timer > update_threshold) {
        update_timer = 0;
        if (p_popup == NULL) {
            uint8_t r, c;
            char row[DOC_COLS];
            ComputeMarkLimits();
            for (r = 0; r < TheTextbox.h; r++) {
                uint16_t R = r + TheDoc.offset_r;
                if (R <= TheDoc.last_row) {
                    bool marked_row = (mark_state != UNMARKED &&
                                    R >= mark_min_r && R <= mark_max_r);
                    if (TheTextbox.row_dirty[r]) {
                        UpdateTextboxFocus(false);
                        memset(row, 0, DOC_COLS);
                        ReadStr(TheDoc.rows[R].ptxt, row, TheDoc.rows[R].len+1);
                        for (c = 0; c < TheTextbox.w; c++) {
                            bool marked_ch = (marked_row &&
                                            !(R == mark_min_r && c < mark_min_c) &&
                                            !(R == mark_max_r && c > mark_max_c));
                            if (c < TheDoc.rows[R].len+1) {
                                char ch = row[c];
                                if (ch == '\n') {
                                    ch = ' ';
                                }
                                DrawChar(TheTextbox.r+r,
                                        TheTextbox.c+c,
                                        ch,
                                        marked_ch ? DARK_GREEN
                                                 : TheTextbox.bg, TheTextbox.fg);
                            } else { // fill remainder of line with spaces
                                DrawChar(TheTextbox.r+r,
                                        TheTextbox.c+c,
                                        ' ',
                                        TheTextbox.bg,
                                        TheTextbox.fg);
                            }
                        }
                        TheTextbox.row_dirty[r] = false;
                        UpdateTextboxFocus(true);
                    }
                } else { // beyond last line
                    if (TheTextbox.row_dirty[r]) {
                        uint8_t c;
                        UpdateTextboxFocus(false);
                        for (c = 0; c < TheTextbox.w; c++) {
                            DrawChar(TheTextbox.r+r,
                                        TheTextbox.c+c,
                                        ' ',
                                        TheTextbox.bg,
                                        TheTextbox.fg);
                        }
                        UpdateTextboxFocus(true);
                        TheTextbox.row_dirty[r] = false;
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SetAllTextboxRowsDirty(void)
{
    uint8_t r;
    for (r = 0; r < TheTextbox.h; r++) {
        TheTextbox.row_dirty[r] = true;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void StartMarkingText(void)
{
    ClearMarkedText();
    mark_start.row = mark_end.row = TheDoc.cursor_r;
    mark_start.col = mark_end.col = TheDoc.cursor_c;
    mark_state = MARKING;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool MarkingText(int16_t cur_row, int16_t cur_col)
{
    if (mark_state >= MARKING &&
        (mark_start.row != cur_row || mark_start.col != cur_col) ) {
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void MarkText(void)
{
    if (MarkingText(TheDoc.cursor_r, TheDoc.cursor_c)) { // marking
        mark_end.row = TheDoc.cursor_r;
        mark_end.col = TheDoc.cursor_c;
        mark_state = MARKED;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void ClearMarkedText(void)
{
    if (mark_state != UNMARKED) {
        mark_state = UNMARKED;
        SetAllTextboxRowsDirty();
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void StopMarkingText(void)
{
    if (MarkingText(TheDoc.cursor_r, TheDoc.cursor_c)) { // marking
        mark_end.row = TheDoc.cursor_r;
        mark_end.col = TheDoc.cursor_c;
    } else {
        ClearMarkedText();
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool CopyMarkedTextToClipboard(void)
{
    memset(TheClipboard, 0, CLIPBOARD_SIZE);
    if (mark_state == MARKED) {
        uint16_t n;
        if (mark_min_r == mark_max_r) { // single row
            n = mark_max_c - mark_min_c + 1;
            ReadStr((uint8_t*)TheDoc.rows[mark_min_r].ptxt + mark_min_c, TheClipboard, n);
        } else { // multi-row
            uint16_t R;
            char row[DOC_COLS];
            char * p = TheClipboard;
            n = 0;
            for (R = mark_min_r; R <= mark_max_r; R++) {
                if (R == mark_min_r) {
                    n = TheDoc.rows[R].len + 1 - mark_min_c;
                } else if (R == mark_max_r) {
                    n += mark_max_c + 1;
                } else {
                    n += (TheDoc.rows[R].len + 1);
                }
            }
            if (n >= CLIPBOARD_SIZE) {
                return false;
            }
            n = 0;
            for (R = mark_min_r; R <= mark_max_r; R++) {
                memset(row, 0, DOC_COLS);
                ReadStr(TheDoc.rows[R].ptxt, row, TheDoc.rows[R].len+1);
                if (R == mark_min_r) {
                    n = TheDoc.rows[R].len + 1 - mark_min_c;
                    memcpy(p, row + mark_min_c, n);
                    p += n;
                } else if (R == mark_max_r) {
                    n = mark_max_c + 1;
                    memcpy(p, row, n);
                    p += n;
                } else {
                    n = TheDoc.rows[R].len + 1;
                    memcpy(p, row, n);
                    p += n;
                }
            }
        }
        //printf("%s\n", TheClipboard);
    }
    return true; // nothing marked, so nothing to copy
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool CutMarkedText(void)
{
    if (mark_state == MARKED && strlen(TheClipboard) > 0) {
        uint16_t n, i;
        i = 0;
        n = strlen(TheClipboard);
        TheDoc.cursor_r = mark_min_r;
        TheDoc.cursor_c = mark_min_c;
        while (i++ < n) {
            DeleteChar(false);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool PasteTextFromClipboard(void)
{
    uint16_t n, i;
    ClearMarkedText();
    i = 0;
    n = strlen(TheClipboard);
    while (i < n) {
        char ch = TheClipboard[i];
        if (ch != '\n') {
            if (!AddChar(ch)) {
                return false;
            }
            TheTextbox.row_dirty[TheDoc.cursor_r - TheDoc.offset_r] = true;
        } else {
            if (!AddNewLine()) {
                return false;
            }
            SetAllTextboxRowsDirty(); // since everything shifted down
        }
        i++;
    }
    return true;
}

// ---------------------------------------------------------------------------
// p_popup is NULL, unless a popup is overlapping display
// ---------------------------------------------------------------------------
void * get_popup(void)
{
    return p_popup;
}
void set_popup(void * popup)
{
    p_popup = popup;
}

// ---------------------------------------------------------------------------
// popuptype stores enumerated type defined in display.h
// It is NONE (0), unless a popup is overlapping the display,
// in which case it tells which kind of popup panel is displayed.
// ---------------------------------------------------------------------------
popup_type_t get_popup_type(void)
{
    return popuptype;
};
void set_popup_type(popup_type_t popup_type)
{
    popuptype = popup_type;
};
