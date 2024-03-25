// ---------------------------------------------------------------------------
// mouse.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "display.h"
#include "textbox.h"
#include "statusbar.h"
#include "msg_dlg.h"
#include "file_dlg.h"
#include "actions.h"
#include "menu.h"
#include "mouse.h"

#define MOUSE_DIV 1 // Mouse speed divider

static uint16_t mouse_struct = 0xFF50;
static uint16_t mouse_data = 0xFE60; // to 0xFEC3, since 100 bytes are needed for mouse pointer data
static uint16_t mouse_state = 0xFF40;
static uint16_t mouse_X = 0;
static uint16_t mouse_Y = 0;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void DrawMousePointer(void)
{
    int i;
    static const uint8_t data[100] = {
        16, 16, 16, 16, 16, 16, 16,  0,  0,  0,
        16,255,255,255,255,255, 16,  0,  0,  0,
        16,255,255,255,255, 16,  0,  0,  0,  0,
        16,255,255,255,255, 16,  0,  0,  0,  0,
        16,255,255,255,255,255, 16,  0,  0,  0,
        16,255, 16, 16,255,255,255, 16,  0,  0,
        16, 16,  0,  0, 16,255,255,255, 16,  0,
         0,  0,  0,  0,  0, 16,255,255,255, 16,
         0,  0,  0,  0,  0,  0, 16,255, 16,  0,
         0,  0,  0,  0,  0,  0,  0, 16,  0,  0,
    };
    RIA.addr0 = mouse_data;
    RIA.step0 = 1;
    for (i = 0; i < 100; i++) {
        RIA.rw0 = data[i];
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void InitMouse(void)
{
    // initialize bitmap mode structure (to draw mouse pointer)
    xram0_struct_set(mouse_struct, vga_mode3_config_t, x_wrap, false);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, y_wrap, false);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, width_px, 10);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, height_px, 10);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, xram_data_ptr, mouse_data);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);
    xregn( 1, 0, 1, 4, 3, 3, mouse_struct, 2); // mode3 (bitmap), 8-bit color, plane2
    xregn( 0, 0, 1, 1, mouse_state);
    DrawMousePointer();
}

// ----------------------------------------------------------------------------
// returns true if handled click, else false if more work to do by App
// ----------------------------------------------------------------------------
static bool LeftBtnPressed(int16_t x, int16_t y)
{
    bool retval = false;
    uint16_t r = y/font_height();
    uint16_t c = x/font_width();
    textbox_t * txtbox = GetTextbox();
    panel_t * main_menu = get_main_menu();
    panel_t * popup = get_popup();
    popup_type_t panel_type = get_popup_type();
    if (popup != NULL) { // did we press a popup button?
        if (panel_type == MSGDIALOG) {
            void * msg_dlg = popup;
            retval = IsMsgDlgButtonPressed((msg_dlg_t*)msg_dlg, r, c);
        } else if (panel_type == FILEDIALOG) {
            void * file_dlg = popup;
            retval = IsFileDlgButtonPressed((file_dlg_t*)file_dlg, r, c);
        } else{
            retval = IsPanelButtonPressed(popup, r, c);
            if (panel_type == SUBMENU) {
                RemoveFocusFromAllPanelButtons(main_menu);
            }
        }
    } else if (r == 0 && main_menu != NULL) { // did we press a Main menu button?
        retval = IsMainMenuButtonPressed(r, c);
    } else if (txtbox != NULL && r < canvas_rows()-1) { // not in status bar
        doc_t * doc = GetDoc();
        // move the cursor to the current mouse position
        uint16_t max_r = doc->last_row;
        uint8_t max_c = doc->rows[r-txtbox->r].len;
        doc->cursor_r = doc->offset_r + r - txtbox->r;
        if (doc->cursor_r > max_r) {
            doc->cursor_r = max_r;
        }
        doc->cursor_c = c - txtbox->c;
        if (doc->cursor_c > max_c) {
            doc->cursor_c = max_c;
        }
        UpdateCursor();
        UpdateStatusBarPos();
    }
    return retval;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool LeftBtnReleased(int16_t x, int16_t y)
{
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool RightBtnPressed(int16_t x, int16_t y)
{
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool RightBtnReleased(int16_t x, int16_t y)
{
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void MouseMoved(int16_t x, int16_t y)
{
    panel_t * main_menu = get_main_menu();
    panel_t * popup = get_popup();
    popup_type_t panel_type = get_popup_type();
    uint16_t r = y/font_height();
    uint16_t c = x/font_width();
    if (popup != NULL) {
        if (panel_type == MSGDIALOG) {
            SetFocusToPanelButton(&((msg_dlg_t*)popup)->panel, r, c);
        } else if (panel_type == FILEDIALOG) {
            textbox_t * txtbox = GetTextbox();
            file_dlg_t * dlg = (file_dlg_t*)popup;
            if (r == (txtbox->r) &&
                c >= (txtbox->c) && c <= (txtbox->c + txtbox->w)) {
                RemoveFocusFromAllPanelButtons(&dlg->panel);
                UpdateTextboxFocus(true);
            } else {
                uint8_t i;
                for (i = 0; i < dlg->panel.num_btns; i++) {
                    button_t * btn = dlg->panel.btn_addr[i];
                    if (btn != NULL) {
                        if (r == btn->r &&
                            c >= btn->c && c < (btn->c + btn->w)    ) {
                            if (!btn->in_focus) {
                                UpdateTextboxFocus(false);
                                RemoveFocusFromAllPanelButtons(&dlg->panel);
                                UpdateButtonFocus(btn, true);
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            SetFocusToPanelButton(popup, r, c);
        }
    } else if (main_menu != NULL) {
        if (r == 0) {
            SetFocusToPanelButton(main_menu, r, c);
        } else { // need to find the button with focus and de-focus it
            RemoveFocusFromAllPanelButtons(main_menu);
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool HandleMouse(void)
{
    static int sx, sy;
    static uint8_t mb, mx, my;
    int16_t x, y;
    uint8_t rw, changed, pressed, released;
    bool xchg = false;
    bool ychg = false;
    static bool first_time = true;

    // read current X
    RIA.addr0 = mouse_state + 1;
    rw = RIA.rw0;
    if (mx != rw)
    {
        xchg = true;
        sx += (int8_t)(rw - mx);
        mx = rw;
        if (sx < 0)
            sx = 0;
        if (sx > (canvas_width() - 2) * MOUSE_DIV)
            sx = (canvas_width() - 2) * MOUSE_DIV;
    }

    // read current y
    RIA.addr0 = mouse_state + 2;
    rw = RIA.rw0;
    if (my != rw)
    {
        ychg = true;
        sy += (int8_t)(rw - my);
        my = rw;
        if (sy < 0)
            sy = 0;
        if (sy > (canvas_height() - 2) * MOUSE_DIV)
            sy = (canvas_height() - 2) * MOUSE_DIV;
    }

    // update mouse pointer on screen
    x = sx / MOUSE_DIV;
    y = sy / MOUSE_DIV;
    xram0_struct_set(mouse_struct, vga_mode3_config_t, x_pos_px, x);
    xram0_struct_set(mouse_struct, vga_mode3_config_t, y_pos_px, y);
    x++, y++;

    mouse_X = x;
    mouse_Y = y;

    // read button states, and detect changes
    RIA.addr0 = mouse_state + 0;
    rw = RIA.rw0;
    changed = mb ^ rw;
    pressed = rw & changed;
    released = mb & changed;
    mb = rw;

    if (!first_time) {
        // handle button changes
        if (pressed & 1) {
            LeftBtnPressed(x, y);
        }
        if (released & 1) {
            LeftBtnReleased(x, y);
        }
        if (pressed & 2) {
            RightBtnPressed(x, y);
        }
        if (released & 2) {
            RightBtnReleased(x, y);
        }

        // if mouse moved, do something special, maybe
        if (xchg || ychg) {
            MouseMoved(x, y);
        }
    } else {
        first_time = false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint16_t mouse_x(void)
{
    return mouse_X;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint16_t mouse_y(void)
{
    return mouse_Y;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint8_t mouse_row(void)
{
    return (uint8_t)mouse_X/font_width();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint8_t mouse_col(void)
{
    return (uint8_t)mouse_Y/font_height();
}