// ---------------------------------------------------------------------------
// keyboard.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "display.h"
#include "textbox.h"
#include "statusbar.h"
#include "button.h"
#include "panel.h"
#include "msg_dlg.h"
#include "file_dlg.h"
#include "actions.h"
#include "menu.h"
#include "usb_hid_keys.h"
#include "keyboard.h"

static const uint16_t keybd_status = 0xFF20; // to 0xFF3F, for KEYBOARD_BYTES (32) of bitmask data
static const uint8_t key_repeat_delay = 2; // * 1/30 second, approx
static const uint8_t initial_delay = 45; // number of key_repeat_delays to wait before repeating

// 256 HID keys max, stored in 32 x uint8_t
#define KEYBOARD_BYTES 32
static uint8_t keystates[KEYBOARD_BYTES] = {0};

#define KEYBUF_SIZE 8
static uint16_t keybuf[KEYBUF_SIZE];
static uint8_t keybuf_head = 0;
static uint8_t keybuf_tail = 0;

#define TABSIZE 4

// Mode key indicators
#define SHIFT_MASK  0x01
#define CTRL_MASK   0x02
#define ALT_MASK    0x04
#define META_MASK   0x08
#define NUMLK_MASK  0x10
#define CAPSLK_MASK 0x20
#define OVRWRT_MASK 0x40
static uint8_t mode_keys = 0;

static const char shifted[][2] = {{'a','A'}, // 4 = 0x04 (KEY_A)
                                  {'b','B'},
                                  {'c','C'},
                                  {'d','D'},
                                  {'e','E'},
                                  {'f','F'},
                                  {'g','G'},
                                  {'h','H'},
                                  {'i','I'},
                                  {'j','J'},
                                  {'k','K'},
                                  {'l','L'},
                                  {'m','M'},
                                  {'n','N'},
                                  {'o','O'},
                                  {'p','P'},
                                  {'q','Q'},
                                  {'r','R'},
                                  {'s','S'},
                                  {'t','T'},
                                  {'u','U'},
                                  {'v','V'},
                                  {'w','W'},
                                  {'x','X'},
                                  {'y','Y'},
                                  {'z','Z'}, // 29 = 0x1d (KEY_Z)
                                  {'1','!'}, // 30 = 0x1e (KEY_1)
                                  {'2','@'},
                                  {'3','#'},
                                  {'4','$'},
                                  {'5','%'},
                                  {'6','^'},
                                  {'7','&'},
                                  {'8','*'},
                                  {'9','('},
                                  {'0',')'}, // 39 = 0x27 (KEY_0)
                                  {'\n','\n'}, // 40 = 0x28 (KEY_ENTER)
                                  {0,0}, //{'\e','\e'}, // 41 = 0x29 (KEY_ESC)
                                  {'\b','\b'}, // 42 = 0x2a (KEY_BACKSPACE)
                                  {'\t','\t'}, // 43 = 0x2b (KEY_TAB)
                                  {' ',' '}, // 44 = 0x2c (KEY_SPACE)
                                  {'-','_'}, // 45 = 0x2d (KEY_MINUS)
                                  {'=','+'}, // 46 = 0x2e (KEY_EQUAL)
                                  {'[','{'}, // 47 = 0x2f (KEY_LEFTBRACE)
                                  {']','}'}, // 48 = 0x30 (KEY_RIGHTBRACE)
                                  {'\\','|'}, // 49 = 0x31 (KEY_BACKSLASH)
                                  {'#','~'}, // 50 = 0x32 (KEY_HASHTILDE)
                                  {';',':'}, // 51 = 0x33 (KEY_SEMICOLON)
                                  {'\'','"'}, // 52 = 0x34 (KEY_APOSTROPHE)
                                  {'`','~'}, // 53 = 0x35 (KEY_GRAVE)
                                  {',','<'}, // 54 = 0x36 (KEY_COMMA)
                                  {'.','>'}, // 55 = 0x37 (KEY_DOT)
                                  {'/','?'}, // 56 = 0x38 (KEY_SLASH)
                                 };
#define hid_2_ascii(k,shftd) (shifted[(k)-KEY_A][((shftd)?1:0)])

static const char numpad[] = {'1','2','3','4','5','6','7','8','9','0'};
#define numpad_hid_2_ascii(k) (numpad[(k)-KEY_KP1])

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void InitKeyboard(void)
{
    xregn( 0, 0, 0, 1, keybd_status);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool DetectModeKeys(uint8_t k)
{
    return (k == KEY_INSERT) || (k == KEY_KP0) ||
           (k == KEY_NUMLOCK) || (k == KEY_CAPSLOCK) ||
           (k >= KEY_LEFTCTRL && k <= KEY_RIGHTMETA );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool ProcessModeKeys(uint8_t k, bool key_pressed)
{
    static bool overwrite_mode = false;
    static bool caps_lock = false;
    static bool num_lock = false;
    static bool shift_left = false;
    static bool shift_right = false;
    static bool shift_pressed = false;
    static bool ctrl_left = false;
    static bool ctrl_right = false;
    static bool ctrl_pressed = false;
    static bool alt_left = false;
    static bool alt_right = false;
    static bool alt_pressed = false;
    static bool meta_left = false;
    static bool meta_right = false;
    static bool meta_pressed = false;
    bool mode_changed = false;

    if (DetectModeKeys(k)) {
        mode_changed = true; // until proven otherwise;

        if (((k == KEY_INSERT) ||
             (k == KEY_KP0 && !num_lock)) && key_pressed) {
            overwrite_mode = !overwrite_mode;
        } else if (k == KEY_NUMLOCK && key_pressed) {
            num_lock = !num_lock;
        } else if (k == KEY_CAPSLOCK && key_pressed) {
            caps_lock = !caps_lock;
        } else if (k == KEY_LEFTSHIFT) {
            shift_left = key_pressed;
        } else if (k == KEY_RIGHTSHIFT) {
            shift_right = key_pressed;
        } else if (k == KEY_LEFTCTRL) {
            ctrl_left = key_pressed;
        } else if (k == KEY_RIGHTCTRL) {
            ctrl_right = key_pressed;
        } else if (k == KEY_LEFTALT) {
            alt_left = key_pressed;
        } else if (k == KEY_RIGHTALT) {
            alt_right = key_pressed;
        } else if (k == KEY_LEFTMETA) {
            meta_left = key_pressed;
        } else if (k == KEY_RIGHTMETA) {
            meta_right = key_pressed;
        } else { // mode hasn't changed
            mode_changed = false;
        }
        if (mode_changed) {
            shift_pressed = shift_left || shift_right;
            ctrl_pressed = ctrl_left || ctrl_right;
            alt_pressed = alt_left || alt_right;
            meta_pressed = meta_left || meta_right;
            mode_keys = ((overwrite_mode?1:0)  << 6) |
                        ((caps_lock?1:0)       << 5) |
                        ((num_lock?1:0)        << 4) |
                        ((meta_pressed?1:0)    << 3) |
                        ((alt_pressed?1:0)     << 2) |
                        ((ctrl_pressed?1:0)    << 1) |
                        (shift_pressed?1:0);
        }
    }
    return mode_changed;
}

// ---------------------------------------------------------------------------
// Try to convert USB HID key to printable ASCII char
// ---------------------------------------------------------------------------
static char HID2ASCII(uint8_t key_modes, uint8_t key)
{
    char ch = 0;
    if (((key >= KEY_A && key <= KEY_0) || (key >= KEY_SPACE && key <= KEY_SLASH))) {
        bool shiftit = ((key_modes & SHIFT_MASK)>0) ||
                        (((key_modes & CAPSLK_MASK)>0) && (key >= KEY_A && key <= KEY_Z));
        ch = hid_2_ascii(key, shiftit);
    } else if ((key >= KEY_KPSLASH && key <= KEY_KPPLUS) ||
                ((key_modes & NUMLK_MASK)>0) && (key == KEY_KPDOT)) {
        switch (key) {
            case KEY_KPSLASH:    ch = '/'; break;
            case KEY_KPASTERISK: ch = '*'; break;
            case KEY_KPMINUS:    ch = '-'; break;
            case KEY_KPPLUS:     ch = '+'; break;
            case KEY_KPDOT:      ch = '.'; break;
        }
    } else if (((key_modes & NUMLK_MASK)>0) && (key >= KEY_KP1 && key <= KEY_KP0)) {
        ch = numpad_hid_2_ascii(key);
    }
    return ch;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool ProcessKeysInPopup(panel_t * popup, popup_type_t popup_type,
                               uint8_t key_modes, uint8_t key)
{
    uint8_t i;
    bool retval = true;
    panel_t * main_menu = get_main_menu();
    textbox_t * txtbox = GetTextbox();

    // If submenu is open, can select submenu item using high-lighted letter
    if (SubmenuShowing() == 1) { // File
        if (key == KEY_O) {        //  'O'pen
            RemoveFocusFromAllPanelButtons(main_menu);
            FileOpen();
        } else if (key == KEY_S) { //  'S'ave
            RemoveFocusFromAllPanelButtons(main_menu);
            FileSave();
        } else if (key == KEY_A) { //   Save 'A's
            RemoveFocusFromAllPanelButtons(main_menu);
            FileSaveAs();
        } else if (key == KEY_C) { //  'C'lose
            RemoveFocusFromAllPanelButtons(main_menu);
            FileClose();
        } else if (key == KEY_X) { //   E'x'it
            RemoveFocusFromAllPanelButtons(main_menu);
            FileExit();
        }
/*
    } else if (SubmenuShowing() == 2) { // Edit0
        if (key == KEY_T) {        //   Cu't'
            RemoveFocusFromAllPanelButtons(main_menu);
            EditCut();
        } else if (key == KEY_C) { //  'C'opy
            RemoveFocusFromAllPanelButtons(main_menu);
            EditCopy();
        } else if (key == KEY_P) { //  'P'aste
            RemoveFocusFromAllPanelButtons(main_menu);
            EditPaste();
        } else if (key == KEY_F) { //  'F'ind
            RemoveFocusFromAllPanelButtons(main_menu);
            EditFind();
        } else if (key == KEY_R) { //  'R'eplace
            RemoveFocusFromAllPanelButtons(main_menu);
            EditReplace();
        }
*/
    } else if (SubmenuShowing() == 3) { // Help
        if (key == KEY_A) {        //  'A'bout
            RemoveFocusFromAllPanelButtons(main_menu);
            HelpAbout();
        }
    }

    if (key == KEY_UP || (key == KEY_KP8 && !(key_modes & NUMLK_MASK))) {
        if (popup->btn_layout == VERT) {
            for (i = 1; i < popup->num_btns; i++) {
                button_t * btn = popup->btn_addr[i];
                if (btn != NULL && btn->in_focus) {
                    UpdateButtonFocus(btn, false);
                    UpdateButtonFocus(popup->btn_addr[i-1], true);
                    break;
                }
            }
        }
    } else if (key == KEY_DOWN || (key == KEY_KP2 && !(key_modes & NUMLK_MASK))) {
        if  (popup->btn_layout == VERT) {
            for (i = 0; i < popup->num_btns-1; i++) {
                button_t * btn = popup->btn_addr[i];
                if (btn != NULL && btn->in_focus) {
                    UpdateButtonFocus(btn, false);
                    UpdateButtonFocus(popup->btn_addr[i+1], true);
                    break;
                }
            }
        }
    } else if (key == KEY_LEFT || (key == KEY_KP4 && !(key_modes & NUMLK_MASK))) {
        if (popup->btn_layout == HORZ) {
            for (i = 1; i < popup->num_btns; i++) {
                button_t * btn = popup->btn_addr[i];
                if (btn != NULL && btn->in_focus) {
                    UpdateButtonFocus(btn, false);
                    UpdateButtonFocus(popup->btn_addr[i-1], true);
                    break;
                }
            }
        }
    } else if (key == KEY_RIGHT || (key == KEY_KP6 && !(key_modes & NUMLK_MASK))) {
        if (popup->btn_layout == HORZ) {
            for (i = 0; i < popup->num_btns-1; i++) {
                button_t * btn = popup->btn_addr[i];
                if (btn != NULL &&btn->in_focus) {
                    UpdateButtonFocus(btn, false);
                    UpdateButtonFocus(popup->btn_addr[i+1], true);
                    break;
                }
            }
        }
    } else if (key == KEY_TAB) {
        if (popup_type == FILEDIALOG) {
            if (txtbox->in_focus) {
                UpdateTextboxFocus(false);
                UpdateButtonFocus(popup->btn_addr[0], true);
            } else {
                for (i = 0; i < MAX_PANEL_BTNS-1; i++) {
                    button_t * btn = popup->btn_addr[i];
                    button_t * next_btn = popup->btn_addr[i+1];
                    if (btn != NULL) {
                        if (btn->in_focus) {
                            UpdateButtonFocus(btn, false);
                            if (next_btn == NULL) {
                                UpdateTextboxFocus(true);
                            } else {
                                UpdateButtonFocus(next_btn, true);
                            }
                            break;
                        }
                    }
                }
            }
        } else {
            for (i = 0; i < MAX_PANEL_BTNS-1; i++) {
                button_t * btn = popup->btn_addr[i];
                button_t * next_btn = popup->btn_addr[i+1];
                if (btn != NULL) {
                    if (btn->in_focus) {
                        UpdateButtonFocus(btn, false);
                        if (next_btn == NULL) {
                            next_btn = popup->btn_addr[0];
                        }
                        UpdateButtonFocus(next_btn, true);
                        break;
                    }
                }
            }
        }
    } else if (key == KEY_ENTER || key == KEY_KPENTER) {
        for (i = 0; i < popup->num_btns; i++) {
            button_t * btn = popup->btn_addr[i];
            if (btn != NULL && btn->in_focus) {
                if (popup_type == SUBMENU) {
                    RemoveFocusFromAllPanelButtons(main_menu);
                    PanelButtonPressed(popup, i);
                } else if (popup_type == MSGDIALOG) {
                    MsgDlgButtonPressed(get_popup(), i);
                } else if (popup_type == FILEDIALOG) {
                    FileDlgButtonPressed(get_popup(), i);
                }
                break;
            }
        }
    } else if (key == KEY_ESC) {
        if (popup_type == SUBMENU) {
                RemoveFocusFromAllPanelButtons(main_menu);
                DeletePanel(popup);
        }
    } else if (popup_type == FILEDIALOG && txtbox->in_focus) {
        if (key == KEY_BACKSPACE || key == KEY_DELETE
                                    || (key == KEY_KPDOT && !(key_modes & NUMLK_MASK))) {
            DeleteCharFromFilename();
        } else {
            AddCharToFilename(HID2ASCII(key_modes, key));
        }
    }
    return retval;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void MarkAllTextboxRowsDirty(void)
{
    uint8_t r;
    textbox_t * txtbox = GetTextbox();
    for (r = 0; r < txtbox->h; r++) {
        txtbox->row_dirty[r] = true;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool ProcessKeysInMainTextbox(uint8_t key_modes, uint8_t key)
{
    uint16_t r;
    uint8_t i, n;
    bool retval = true;
    doc_t * doc = GetDoc();
    textbox_t * txtbox = GetTextbox();
    panel_t * main_menu = get_main_menu();
    if ((key_modes & CTRL_MASK)>0) { // Can use Ctrl+... accelerator keys
        if (key == KEY_O) { // File 'O'pen
            FileOpen();
        } else if (key == KEY_S) { // File 'S'ave ('A's)
            ((key_modes & SHIFT_MASK)>0) ? FileSaveAs() : FileSave();
        } else if (key == KEY_Q) { // File Exit ('Q'uit)
            FileExit();
/*
        } else if (key == KEY_X) { // Edit Cut
            EditCut();
        } else if (key == KEY_C) { // Edit Copy
            EditCopy();
        } else if (key == KEY_V) { // Edit Paste
            EditPaste();
        } else if (key == KEY_F) { // Edit Find
            EditFind();
        } else if (key == KEY_H) { // Edit Replace
            EditReplace();
*/
        }
    } else if (((key_modes & ALT_MASK)>0)) { // open main menu submenus
        if (key == KEY_F) { // 'F'ile
            CloseAnyPopupMenu();
            RemoveFocusFromAllPanelButtons(main_menu);
            UpdateButtonFocus(main_menu->btn_addr[0], true);
            ShowFileSubmenu();
/*
        } else if (key == KEY_E) { // 'E'dit
            CloseAnyPopupMenu();
            RemoveFocusFromAllPanelButtons(main_menu);
            UpdateButtonFocus(main_menu->btn_addr[1], true);
            ShowEditSubmenu();
*/
        } else if (key == KEY_H) { // 'H'elp
            CloseAnyPopupMenu();
            RemoveFocusFromAllPanelButtons(main_menu);
            UpdateButtonFocus(main_menu->btn_addr[2], true);
            ShowHelpSubmenu();
        }
    } else if (key == KEY_UP || (key == KEY_KP8 && !(key_modes & NUMLK_MASK))) {
        if (doc->cursor_r > 0) { // room to move up
            // if the cursor is at top of display, scroll by adjusting doc offset
            if (doc->cursor_r-1 < doc->offset_r) {
                MarkAllTextboxRowsDirty();
                doc->offset_r--;
            }
            doc->cursor_r -= 1;
            UpdateCursor();
        }
    } else if (key == KEY_DOWN || (key == KEY_KP2 && !(key_modes & NUMLK_MASK))) {
        if (doc->cursor_r < doc->last_row) { // room to move down
            // if the cursor is at bottom of display, scroll by adjusting doc offset
            if (doc->cursor_r+1 > txtbox->h-1+doc->offset_r) {
                MarkAllTextboxRowsDirty();
                doc->offset_r++;
            }
            doc->cursor_r += 1;
            UpdateCursor();
        }
    } else if (key == KEY_LEFT || (key == KEY_KP4 && !(key_modes & NUMLK_MASK))) {
        if (doc->cursor_c > 0) { // room to move left
            doc->cursor_c -= 1;
            UpdateCursor();
        }
    } else if (key == KEY_RIGHT || (key == KEY_KP6 && !(key_modes & NUMLK_MASK))) {
        if (doc->cursor_c < doc->rows[doc->cursor_r].len) { // room to move right
            doc->cursor_c += 1;
            UpdateCursor();
        }
    } else if (key == KEY_HOME || (key == KEY_KP7 && !(key_modes & NUMLK_MASK))) {
        doc->cursor_c = 0;
        UpdateCursor();
    } else if (key == KEY_END || (key == KEY_KP1 && !(key_modes & NUMLK_MASK))) {
        doc->cursor_c = doc->rows[doc->cursor_r].len;
        UpdateCursor();
    } else if (key == KEY_PAGEUP || (key == KEY_KP9 && !(key_modes & NUMLK_MASK))) {
        uint16_t old_offset = doc->offset_r;
        int16_t new_cursor_r = (int16_t)doc->cursor_r - (int16_t)(txtbox->h-1);
        new_cursor_r = (new_cursor_r >= 0) ? new_cursor_r : 0;
        if (doc->offset_r == 0 ||
            new_cursor_r == 0 ||
            doc->offset_r == new_cursor_r ) {
            doc->cursor_r = new_cursor_r;
            doc->offset_r = new_cursor_r;
        } else { // shift offset a full screen height
            doc->cursor_r = new_cursor_r;
            doc->offset_r -= (txtbox->h-1);
        }
        MarkAllTextboxRowsDirty();
        UpdateCursor();
    } else if (key == KEY_PAGEDOWN || (key == KEY_KP3 && !(key_modes & NUMLK_MASK))) {
        uint16_t old_offset = doc->offset_r;
        uint16_t old_offset_to_bottom = doc->offset_r + (txtbox->h-1);
        uint16_t new_cursor_r = doc->cursor_r + (txtbox->h-1);
        new_cursor_r = (new_cursor_r < doc->last_row) ? new_cursor_r : doc->last_row;
        new_cursor_r = (new_cursor_r < DOC_ROWS-1) ? new_cursor_r : DOC_ROWS-1;
        if (old_offset_to_bottom >= new_cursor_r)  { // no scroll
            doc->cursor_r = new_cursor_r;
        } else { // shift offset a full screen height
            doc->cursor_r = new_cursor_r;
            doc->offset_r += (txtbox->h-1);
        }
        MarkAllTextboxRowsDirty();
        UpdateCursor();
    } else if (key == KEY_TAB) {
        if ((key_modes & OVRWRT_MASK)==0) { // insert mode only
            if ((key_modes & SHIFT_MASK) == 0) { // shift right
                n = TABSIZE - doc->cursor_c % TABSIZE;
                if (doc->rows[doc->cursor_r].len+n < DOC_COLS) { // room to move right?
                    for (i = 0; i < n; i++) {
                        AddChar(HID2ASCII(key_modes, KEY_SPACE), true);
                    }
                    txtbox->row_dirty[doc->cursor_r - doc->offset_r] = true;
                } else {
                    UpdateStatusBarMsg("Maximum line length exceeded!", STATUS_WARNING);
                }
            } else { // shift left

            }
        }
    } else if (key == KEY_ENTER || key == KEY_KPENTER) {
        if(AddNewLine()) {
            MarkAllTextboxRowsDirty();
        }
    } else if (key == KEY_ESC) {
        // no action?
    } else if (key == KEY_BACKSPACE || key == KEY_DELETE
                                    || (key == KEY_KPDOT && !(key_modes & NUMLK_MASK))) {
        bool row_deleted = ((key == KEY_BACKSPACE && doc->cursor_c == 0) ||
                            (key != KEY_BACKSPACE && doc->cursor_c == doc->rows[doc->cursor_r].len));
        DeleteChar(key == KEY_BACKSPACE);
        // did operation delete a row?
        if (row_deleted) {
            for (r = doc->cursor_r - doc->offset_r; r < txtbox->h; r++) {
                txtbox->row_dirty[r] = true;
            }
        } else { // only current row is affected
            txtbox->row_dirty[doc->cursor_r - doc->offset_r] = true;
        }
    } else {
        if (((key_modes & OVRWRT_MASK) && doc->cursor_c < DOC_COLS-1) ||
            doc->rows[doc->cursor_r].len+1 < DOC_COLS) { // room to move right?
            AddChar(HID2ASCII(key_modes, key), !(key_modes & OVRWRT_MASK));
            txtbox->row_dirty[doc->cursor_r - doc->offset_r] = true;
        } else {
            UpdateStatusBarMsg("Maximum line length exceeded!", STATUS_WARNING);
        }
    }
    UpdateStatusBarPos();
    return retval;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static bool ProcessKeyStates(void)
{
    bool retval = true;
    panel_t * popup = get_popup();
    popup_type_t popup_type = get_popup_type();
    if (popup_type == MSGDIALOG) {
        msg_dlg_t * msg_dlg = get_popup();
        popup = &msg_dlg->panel;
    } else if (popup_type == FILEDIALOG) {
        file_dlg_t * file_dlg = get_popup();
        popup = &file_dlg->panel;
    }

    while (keybuf_head != keybuf_tail) {
        uint8_t key = (uint8_t)(keybuf[keybuf_head] & 0x00FF);
        uint8_t key_modes = (uint8_t)(keybuf[keybuf_head] >> 8);

        keybuf_head = ((keybuf_head+1) < KEYBUF_SIZE) ? keybuf_head+1 : 0;

        if (popup != NULL) {
            ProcessKeysInPopup(popup, popup_type, key_modes, key);
        } else {
            ProcessKeysInMainTextbox(key_modes, key);
        }
    }
    return retval; // continue looping if true, else exit
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool HandleKeys()
{
    static uint16_t key_timer = 0;
    static uint8_t initial_delay_counter = 0;
    uint8_t i, j, new_keys;
    bool retval = true; // continue looping if true, else exit

    // update timer counts
    if ((key_timer++) > key_repeat_delay) {
        initial_delay_counter++;
    }

    // fill the keystates bitmask array
    RIA.addr0 = keybd_status;
    RIA.step0 = 0;
    for (i = 0; i < KEYBOARD_BYTES; i++) { // for each byte of 8 keys
        RIA.addr0 = keybd_status + i;
        new_keys = RIA.rw0;
        for (j = 0; j < 8; j++) { // for each key in byte
            uint8_t key_pressed = (new_keys & (1<<j)); // will be 1 if pressed, 0 if released
            uint8_t k = ((i<<3)+j); // compute key HID code
            if (k > 3) { // if regular HID keycode...
                if (key_pressed != (keystates[i] & (1<<j))) { // ...and key state has changed since last time
                    // check if the key is a mode key, and update key modes
                    bool modes_changed = ProcessModeKeys(k, key_pressed);
                    if (!modes_changed && key_pressed) { // non-mode key was just pressed
                        key_timer = 0;
                        initial_delay_counter = 0;
                        // add the new key to the keybuf, along with current mode_keys
                        keybuf[keybuf_tail] = ((uint16_t)mode_keys << 8) | (uint16_t)k;
                        keybuf_tail = ((keybuf_tail+1) < KEYBUF_SIZE) ? keybuf_tail+1 : 0;
                    }
/*
                    printf("modes: %c%c%c%c%c%c%c key: 0x%02x %s\n",
                            ((mode_keys&NUMLK_MASK)>0)?'#':'_',
                            ((mode_keys&CAPSLK_MASK)>0)?'^':'_',
                            ((mode_keys&OVRWRT_MASK)>0)?'O':'_',
                            ((mode_keys&META_MASK)>0)?'M':'_',
                            ((mode_keys&ALT_MASK)>0)?'A':'_',
                            ((mode_keys&CTRL_MASK)>0)?'C':'_',
                            ((mode_keys&SHIFT_MASK)>0)?'S':'_',
                            k, (key_pressed?"pressed":"released"));
*/
                } else if (key_pressed){ // key was not new, but is it still pressed
                    if (key_timer > key_repeat_delay &&
                        initial_delay_counter > initial_delay) {

                        key_timer = 0; // OK, we handled repeat

                        if (!DetectModeKeys(k)) {
                            // add the repeated key to the keybuf, along with current mode_keys
                            keybuf[keybuf_tail] = ((uint16_t)mode_keys << 8) | (uint16_t)k;
                            keybuf_tail = ((keybuf_tail+1) < KEYBUF_SIZE) ? keybuf_tail+1 : 0;
                        }
/*
                        printf("key: 0x%02x still pressed\n", k);
*/
                    }
                }
            }
        }
        keystates[i] = new_keys; // update keystates byte with most recent states
    }

    // check for a key down
    if (!(keystates[0] & 1)) {
        retval = ProcessKeyStates();
    } else { // no keys down
        key_timer = 0;
        initial_delay_counter = 0;
    }
    return retval;
}