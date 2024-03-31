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

// only ever one cursor, so save state here
static cursor_state_t cur_state = BLINK_OFF;
static uint8_t cur_c; // as drawn on display
static uint8_t cur_r; // as drawn on display
static bool overwrite_enabled = false;

static const uint8_t cur_threshold = 30; // cursor blink delay = 1/2 second
static const uint8_t update_threshold = 3; // redraw freq = 20x/second

static textbox_t TheTextbox = {
    1,          // r
    0,          // c
    28,         // h
    80,         // w
    BLACK,      // bg
    LIGHT_GRAY, // fg
    true,       // in_focus
    {false, false, false, false, false, false, false,
     false, false, false, false, false, false, false,
     false, false, false, false, false, false, false,
     false, false, false, false, false, false, false} // row_dirty[28]
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void InitTextbox(void)
{
    uint8_t r, c;

    ClearDoc(false);

    // force redraw of textbox background
    for (r = 0; r < TheTextbox.h; r++) {
        TheTextbox.row_dirty[r] = true;
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
textbox_t * GetTextbox(void)
{
    return &TheTextbox;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateCursor()
{
    doc_t * doc = GetDoc();
    char ch;
    uint8_t fg, bg, new_row, new_col;
    void * popup = get_popup();
    popup_type_t popup_type = get_popup_type();

    if (popup != NULL && popup_type == FILEDIALOG) {
        new_row = doc->cur_filename_r;
        new_col = doc->cur_filename_c;
    } else {
        // if cursor is beyond doc's last row, move it to last row
        if (doc->cursor_r > doc->last_row) {
            doc->cursor_r = doc->last_row;
        }
        new_row = TheTextbox.r + doc->cursor_r - doc->offset_r;
        // if cursor is beyond new rows's length, move it left appropriately
        if (doc->cursor_c > doc->rows[new_row + doc->offset_r - TheTextbox.r].len) {
            doc->cursor_c = doc->rows[new_row + doc->offset_r - TheTextbox.r].len;
        }
        new_col = TheTextbox.c + doc->cursor_c;
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
        if ((popup != NULL && popup_type != FILEDIALOG) ||
            (new_row == 0 || new_row == canvas_rows()-1)) {
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

    // redraw any dirty rows
    if (update_timer > update_threshold) {
        update_timer = 0;
        if (get_popup() == NULL) {
            uint8_t r, c;
            doc_t * doc = GetDoc();
            char row[DOC_COLS];
            for (r = 0; r < TheTextbox.h; r++) {
                uint16_t R = r + doc->offset_r;
                if (TheTextbox.row_dirty[r]) {
                    UpdateTextboxFocus(false);
                    memset(row, 0, DOC_COLS);
                    ReadStr(doc->rows[R].ptxt, row, doc->rows[R].len+1);
                    for (c = 0; c < TheTextbox.w; c++) {
                        char ch = row[c];
                        if (c < doc->rows[R].len) {
                            if (ch >= ' ' && ch <= '~' ) {
                                // limit drawing to current screen area
                                DrawChar(TheTextbox.r+r, TheTextbox.c+c, row[c], TheTextbox.bg, TheTextbox.fg);
                            }
                        } else { // fill remainder of line with spaces
                            DrawChar(TheTextbox.r+r, TheTextbox.c+c, ' ', TheTextbox.bg, TheTextbox.fg);
                        }
                    }
                    TheTextbox.row_dirty[r] = false;
                    UpdateTextboxFocus(true);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DisableOverwriteMode(void)
{
    overwrite_enabled = false;
    UpdateCursor();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EnableOverwriteMode(void)
{
    overwrite_enabled = true;
    UpdateCursor();
}


