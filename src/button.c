// ---------------------------------------------------------------------------
// button.c
// ---------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "colors.h"
#include "display.h"
#include "button.h"

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static void DrawButton(button_t * btn)
{
    if (btn != NULL) {
        uint8_t c, n, start, len;
        // draw button background
        for (c = btn->c; c < btn->c + btn->w; c++) {
            DrawChar(btn->r, c, ' ',
                        btn->in_focus ? btn->focus_bg : btn->bg,
                        btn->in_focus ? btn->focus_fg : btn->fg);
        }
        // draw the button label
        len  = strlen(btn->btn_lbl);
        start = btn->c + 1;
        n = 0;
        for (c = start; c < (start + len); c++) {
            DrawChar(btn->r, c, btn->btn_lbl[n++],
                     btn->in_focus ? btn->focus_bg : btn->bg,
                     btn->in_focus ? btn->focus_fg : btn->fg);
        }
        if (btn->alt_index >= 0) {
            DrawChar(btn->r, start + btn->alt_index, btn->btn_lbl[btn->alt_index],
                     btn->in_focus ? btn->focus_bg : btn->bg, btn->alt_fg);
        }
    }
}

// ---------------------------------------------------------------------------
// mallocs, initializes, and draws a new button_t, returning it when complete
// ---------------------------------------------------------------------------
button_t * NewButton(const char * label,
                     uint8_t bg_color, uint8_t fg_color,
                     uint8_t focus_bg_color, uint8_t focus_fg_color,
                     uint8_t alt_fg_color, int8_t alt_index)
{
    button_t * btn = (button_t *)malloc(sizeof(button_t));
    if (btn != NULL) {
        btn->bg = bg_color;
        btn->fg = fg_color;
        btn->focus_bg = focus_bg_color;
        btn->focus_fg = focus_fg_color;
        btn->alt_fg = alt_fg_color;
        btn->alt_index = alt_index;
        btn->in_focus = false;
        btn->is_showing = false;
        btn->r = 0; // filled in by Show()
        btn->c = 0; // fileed in by Show()
        btn->w = 2; // 1 char margin on each side of label
        if (label != NULL) {
            strncpy(btn->btn_lbl, label, MAX_BTN_LBL_LEN);
        } else { // unlabeled (differentiate by color?)
            strncpy(btn->btn_lbl, "    ", MAX_BTN_LBL_LEN);
        }
        btn->w += strlen(label);
    }
    return btn;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool ShowButton(button_t * btn,  uint16_t row, uint16_t col)
{
    bool retval = false;

    if (btn != NULL) {
        btn->r = row;
        btn->c = col;
        btn->is_showing = true;
        DrawButton(btn);

        retval = true;
    }
    return retval;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateButtonFocus(button_t * btn, bool has_focus)
{
    if (btn != NULL) {
        if (btn->in_focus != has_focus) {
            btn->in_focus = has_focus;
            if (btn->is_showing) {
                DrawButton(btn);
            }
        }
    }
}

