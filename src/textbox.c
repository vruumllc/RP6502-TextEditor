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

typedef enum {MOVING, BLINK_ON, BLINK_OFF} cursor_state_t;

// only ever one cursor, so save state here
static cursor_state_t cur_state = BLINK_OFF;
static uint8_t cur_c; // as drawn on display
static uint8_t cur_r; // as drawn on display
static bool blinking_disabled = false;
static bool overwrite_enabled = false;

static const uint8_t cur_threshold = 30; // cursor blink delay = 1/2 second
static const uint8_t update_threshold = 3; // redraw freq = 20x/second

static textbox_t TheTextbox = {
    1,          // r
    0,          // c
    28,         // h
    80,         // w
    BLACK,      // bg
    LIGHT_GRAY, //fg
    true        // in_focus
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void InitTextbox(void)
{
    uint8_t r, c;

    // clear textbox background
    for (r = TheTextbox.r; r < TheTextbox.r + TheTextbox.h; r++) {
        for (c = TheTextbox.c; c < TheTextbox.c + TheTextbox.w; c++) {
            DrawChar(r, c, ' ', TheTextbox.bg, TheTextbox.fg);
        }
    }
    ClearDoc(false);
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
    cursor_state_t old_state = cur_state;

    if (popup != NULL && popup_type == FILEDIALOG) {
        new_row = doc->cur_filename_r;
        new_col = doc->cur_filename_c;
    } else {
        new_row = TheTextbox.r + doc->cursor_r - doc->offset_r;
        new_col = TheTextbox.c + doc->cursor_c;
    }

    // check if move is desired
    if (new_row != cur_r || new_col != cur_c) {
        cur_state = MOVING;
    }

    if (old_state == BLINK_ON) {
        // get current data
        GetChar(cur_r, cur_c, &ch, &bg, &fg);
        // restore original colors
        DrawChar(cur_r, cur_c, ch, fg, bg);
    }

    // don't update cursor if popup other than FILEDIALOG is present,
    // or in Main menu or status bar
    if ((popup != NULL && popup_type != FILEDIALOG)||
        new_row == 0 || new_row == canvas_rows()-1) {
        cur_state = BLINK_OFF;
        return;
    }

    if (cur_state == MOVING) {
        if (!blinking_disabled && (popup == NULL || popup_type == FILEDIALOG)) {
            // get new data
            GetChar(new_row, new_col, &ch, &bg, &fg);
            // reverse colors at new location
            DrawChar(new_row, new_col, ch, fg, bg);
            cur_state = BLINK_ON;
        }
    } else { // blinking
        if (old_state == BLINK_OFF) {
            if (!blinking_disabled && (popup == NULL || popup_type == FILEDIALOG)) {
                // get current data
                GetChar(cur_r, cur_c, &ch, &bg, &fg);
                // reverse colors to show cursor
                DrawChar(cur_r, cur_c, ch, fg, bg);
                cur_state = BLINK_ON;
            }
        } else {
            // restored original colors previously
            cur_state = BLINK_OFF;
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
    blinking_disabled = has_focus ? false : true;
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
        if (cur_state != MOVING) {
            UpdateCursor();
        }
    }

    // redraw any dirty rows
    if (update_timer > update_threshold) {
        update_timer = 0;
        if (get_popup() == NULL) {
            uint16_t r, c;
            doc_t * doc = GetDoc();
            char row[DOC_COLS];
            uint16_t row_max = ((TheTextbox.h < doc->last_row+1-doc->offset_r) ?
                                 TheTextbox.h : doc->last_row+1-doc->offset_r);
            for (r = 0; r < row_max; r++) {
                uint16_t R = r + doc->offset_r;
                if (doc->rows[R].dirty) {
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
                    doc->rows[R].dirty = false;
                    UpdateTextboxFocus(true);
                }
            }
            for (r = row_max; r < TheTextbox.h; r++) {
                for (c = 0; c < TheTextbox.w; c++) {
                    DrawChar(TheTextbox.r+r, TheTextbox.c+c, ' ', TheTextbox.bg, TheTextbox.fg);
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


