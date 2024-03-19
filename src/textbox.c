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

static const uint8_t cur_threshold = 30; // cursor blink delay

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
textbox_t * NewTextbox(uint8_t width, uint8_t height, uint8_t bg_color, uint8_t fg_color)
{
    textbox_t * txtbox = (textbox_t *)malloc(sizeof(textbox_t));
    if (txtbox != NULL) {
        txtbox->r = 0; // filled in by ShowPanel()
        txtbox->c = 0; // filled in by ShowPanel()
        txtbox->w = width;
        txtbox->h = height;
        txtbox->bg = bg_color;
        txtbox->fg = fg_color;
        txtbox->in_focus = false;
        InitDoc(&txtbox->doc, height==1);
    }
    return txtbox;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DeleteTextbox(textbox_t * txtbox)
{
    if (txtbox != NULL) {
        if (get_active_textbox() == txtbox) {
            set_active_textbox(NULL);
        }
        txtbox->doc.rows = NULL;
        free(txtbox);
    } // bad parameter
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void ShowTextbox(textbox_t * txtbox, uint16_t row, uint16_t col)
{
    if (txtbox != NULL &&
        row + txtbox->h <= canvas_rows() &&
        col + txtbox->w <= canvas_cols()   ) {

        txtbox->r = row;
        txtbox->c = col;

        // draw textbox background
        for (uint8_t i = txtbox->r; i < txtbox->r + txtbox->h; i++) {
            for (uint8_t j = txtbox->c; j < txtbox->c + txtbox->w; j++) {
                DrawChar(i, j, ' ', txtbox->bg, txtbox->fg);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateCursor()
{
    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL) {
        char ch;
        uint8_t fg, bg;
        cursor_state_t old_state = cur_state;
        uint8_t new_row = txtbox->r + txtbox->doc.cursor_r - txtbox->doc.offset_r;
        uint8_t new_col = txtbox->c + txtbox->doc.cursor_c;

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
        if ((get_popup() != NULL && get_popup_type() != FILEDIALOG)||
            new_row == 0 || new_row == canvas_rows()-1) {
            cur_state = BLINK_OFF;
            return;
        }

        if (cur_state == MOVING) {
            if (!blinking_disabled && get_active_textbox() != NULL) {
                // get new data
                GetChar(new_row, new_col, &ch, &bg, &fg);
                // reverse colors at new location
                DrawChar(new_row, new_col, ch, fg, bg);
                cur_state = BLINK_ON;
            }
        } else { // blinking
            if (old_state == BLINK_OFF) {
                if (!blinking_disabled && get_active_textbox() != NULL) {
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
}

// ---------------------------------------------------------------------------
// Called by main loop periodically to redraw document in textbox
// ---------------------------------------------------------------------------
void UpdateActiveTextbox(void)
{
    static uint16_t cursor_timer = 0;

    // redraw any dirty rows
    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL && (get_popup() == NULL || get_popup_type() == FILEDIALOG)) {
        char row[DOC_COLS];
        uint16_t row_max = ((txtbox->h < txtbox->doc.last_row+1-txtbox->doc.offset_r) ?
                             txtbox->h : txtbox->doc.last_row+1-txtbox->doc.offset_r);
        for (uint16_t r = 0; r < row_max; r++) {
            uint16_t R = r + txtbox->doc.offset_r;
            if (txtbox->doc.rows[R].dirty) {
                memset(row, 0, DOC_COLS);
                ReadStr(txtbox->doc.rows[R].ptxt, row, txtbox->doc.rows[R].len+1);
                DisableBlinking();
                for (uint8_t c = 0; c < txtbox->w; c++) {
                    char ch = row[c];
                    if (c < txtbox->doc.rows[R].len) {
                        if (ch >= ' ' && ch <= '~' ) {
                            // limit drawing to current screen area
                            DrawChar(txtbox->r+r, txtbox->c+c, row[c], txtbox->bg, txtbox->fg);
                        }
                    } else { // fill remainder of line with spaces
                        DrawChar(txtbox->r+r, txtbox->c+c, ' ', txtbox->bg, txtbox->fg);
                    }
                }
                txtbox->doc.rows[R].dirty = false;
                EnableBlinking();
            }
        }
        // clear rows beyond end of doc
        for (uint16_t r = row_max; r < txtbox->h; r++) {
            for (uint8_t c = 0; c < txtbox->w; c++) {
                DrawChar(txtbox->r+r, txtbox->c+c, ' ', txtbox->bg, txtbox->fg);
            }
        }
    }

    // update timer counts
    cursor_timer++;

    // blink the cursor
    if (cursor_timer > cur_threshold) {
        cursor_timer = 0;
        if (cur_state != MOVING) {
            UpdateCursor();
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DisableBlinking(void)
{
    blinking_disabled = true;
    UpdateCursor();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EnableBlinking(void)
{
    blinking_disabled = false;
    UpdateCursor();
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

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateTextboxFocus(textbox_t * txtbox, bool has_focus)
{
    if (txtbox != NULL) {
        set_active_textbox(txtbox);
        txtbox->in_focus = has_focus;
        blinking_disabled = has_focus ? false : true;
        UpdateCursor();
    }
}
