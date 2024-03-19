// ---------------------------------------------------------------------------
// panel.c
// ---------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "colors.h"
#include "display.h"
#include "textbox.h"
#include "button.h"
#include "panel.h"

// ---------------------------------------------------------------------------
// mallocs, initializes, and draws a new panel_t, returning it when complete
// ---------------------------------------------------------------------------
panel_t * NewPanel(popup_type_t type, panel_btn_layout_t button_layout,
                   uint8_t width, uint8_t height,
                   uint8_t bg_color, uint8_t fg_color)
{
    panel_t * panel = (panel_t *)malloc(sizeof(panel_t));
    if (panel != NULL) {
        panel->panel_type = type;
        panel->btn_layout = button_layout;
        panel->r = 0; // filled in by ShowPanel()
        panel->c = 0; // filled in by ShowPanel()
        panel->w = width;
        panel->h = height;
        panel->bg = bg_color;
        panel->fg = fg_color;
        panel->num_btns = 0;
        for (uint8_t i = 0; i < MAX_PANEL_BTNS; i++) {
            panel->btn_addr[i] = NULL;
            panel->action[i] = NULL;
        }
        panel->pstash = NULL;
    }
    return panel;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool AddButtonToPanel(panel_t * panel, const char *label, int8_t alt_index,
                      uint8_t bg_color, uint8_t fg_color,
                      uint8_t focus_bg_color, uint8_t focus_fg_color,
                      uint8_t alt_fg_color, panel_btn_action action)
{
    if (panel != NULL) {
        button_t * btn = NewButton(label,
                                   bg_color, fg_color,
                                   focus_bg_color, focus_fg_color,
                                   alt_fg_color, alt_index);
        if (btn != NULL) {
            panel->btn_addr[panel->num_btns] = btn;
            panel->action[panel->num_btns] = action;
            panel->num_btns++;
            return true;
        }
    } // bad parameter
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool ShowPanel(panel_t * panel, uint8_t row, uint8_t col)
{
    if (panel != NULL) {
        if (panel->pstash == NULL &&
            row + panel->h <= canvas_rows() &&
            col + panel->w <= canvas_cols()   ) {

            panel->r = row;
            panel->c = col;

            // backup display covered by panel (4bpp takes 2 bytes each char)
            panel->pstash = (uint8_t *)malloc(panel->w*panel->h*2);
            if (panel->pstash != NULL) {
                memset(panel->pstash, 0, panel->w*panel->h*2);
                BackupChars(panel->r, panel->c, panel->w, panel->h, panel->pstash);

                // draw panel background
                for (uint8_t i = panel->r; i < panel->r + panel->h; i++) {
                    for (uint8_t j = panel->c; j < panel->c + panel->w; j++) {
                        DrawChar(i, j, ' ', panel->bg, panel->fg);
                    }
                }

                // draw the panel buttons, assuming panel is horizontal or vertical menu
                uint8_t start = 0;
                bool retval;
                for (uint8_t i = 0; i < panel->num_btns; i++) {
                    if (panel->btn_layout == VERT) {
                        retval = ShowButton(panel->btn_addr[i], panel->r+i, panel->c);
                    } else {
                        retval = ShowButton(panel->btn_addr[i], panel->r, panel->c + start);
                        start += panel->btn_addr[i]->w;
                    }
                }
                return true;
            } // stash allocation failed
            return false;
        }
    } // bad parameters
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DeletePanel(panel_t * panel)
{
    if (panel != NULL) {
        if (panel->pstash != NULL) {
            RestoreChars(panel->r, panel->c, panel->w, panel->h, panel->pstash);

            for (uint8_t i = 0; i < panel->num_btns; i++) {
                button_t * btn = panel->btn_addr[i];
                if (btn != NULL){
                    free(btn);
                    panel->btn_addr[i] = NULL;
                }
            }
            free(panel->pstash);
            panel->pstash = NULL;

            set_popup(NULL);
            set_popup_type(NO_POPUP_TYPE);

            free(panel);

            textbox_t * txtbox = get_active_textbox();
            if (txtbox != NULL) {
                UpdateTextboxFocus(txtbox, true);
            }

            return;
        }
    } // bad parameter
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void PanelButtonPressed(panel_t * panel, uint8_t index)
{
    if (panel != NULL) {
        panel_t * pmain_menu = get_main_menu();
        panel_t * popup = get_popup();
        if (popup != NULL) {
            panel->action[index]();
            // make sure we don't hide a dialog box created by menu action
            popup_type_t type = get_popup_type();
            if (type == SUBMENU || type == CONTEXTMENU ) {
                RemoveFocusFromAllPanelButtons(pmain_menu);
                DeletePanel(panel);
            }
        }
        return;
    } // bad parameter
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool IsPanelButtonPressed(panel_t * panel, int16_t row, int16_t col)
{
    if (panel != NULL) {
        for (uint8_t i = 0; i < panel->num_btns; i++) {
            button_t * btn = panel->btn_addr[i];
            if (btn != NULL) {
                if (row == btn->r &&
                    col >= btn->c && col < (btn->c + btn->w)) {
                    PanelButtonPressed(panel, i);
                    return true;
                }
            }
        }
        // if we clicked with mouse, but not on a button, close popup menus
        popup_type_t type = get_popup_type();
        if (type == SUBMENU || type == CONTEXTMENU ) {
            DeletePanel(panel);
            return true;
        }
        return true;
    } // bad parameter
    return false;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void SetFocusToPanelButton(panel_t * panel, int16_t row, int16_t col)
{
    if (panel != NULL) {
        for (uint8_t i = 0; i < panel->num_btns; i++) {
            button_t * btn = panel->btn_addr[i];
            if (btn != NULL) {
                if (row == btn->r &&
                    col >= btn->c && col < (btn->c + btn->w)    ) {
                    if (!btn->in_focus) {
                        // need to find the button with focus and de-focus it
                        RemoveFocusFromAllPanelButtons(panel);
                        UpdateButtonFocus(btn, true);
                        break;
                    }
                }
            }
        }
        return;
    } // bad parameter
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void RemoveFocusFromAllPanelButtons(panel_t * panel)
{
    if (panel != NULL) {
        for (uint8_t j = 0; j < panel->num_btns; j++) {
            button_t * btn = panel->btn_addr[j];
            if (btn != NULL) {
                if (btn->in_focus) {
                    UpdateButtonFocus(btn, false);
                }
            }
        }
        return;
    } // bad parameter
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CloseAnyPopupMenu(void)
{
    panel_t * popup = get_popup();
    popup_type_t type = get_popup_type();

    if (popup != NULL && (type == SUBMENU || type == CONTEXTMENU)) {
        DeletePanel(popup);
    }
}

