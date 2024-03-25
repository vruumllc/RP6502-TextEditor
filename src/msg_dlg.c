 // ---------------------------------------------------------------------------
// msg_dlg.c
// ---------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colors.h"
#include "display.h"
#include "textbox.h"
#include "button.h"
#include "panel.h"
#include "msg_dlg.h"

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
msg_dlg_t * NewMsgDlg(char * msg, msg_dlg_type_t type,
                      panel_btn_action Yes,
                      panel_btn_action No,
                      panel_btn_action Cancel)
{
    bool retval = false;
    uint8_t i, len_msg, len_btns;
    msg_dlg_t * pmsg_dlg = (msg_dlg_t *)malloc(sizeof(msg_dlg_t));
    if (pmsg_dlg != NULL) {
        pmsg_dlg->msg_dlg_type = type;

        if (msg != NULL) {
            strncpy(pmsg_dlg->dlg_msg, msg, MAX_MSG_LEN);
        } else {
            memset(pmsg_dlg->dlg_msg, 0, MAX_MSG_LEN+1);
        }
        len_msg = strlen(msg) + 2;

        pmsg_dlg->panel.panel_type = MSGDIALOG;
        pmsg_dlg->panel.btn_layout = HORZ;
        pmsg_dlg->panel.r = 0; // filled in by ShowMsgDlg()
        pmsg_dlg->panel.c = 0; // filled in by ShowMsgDlg()
        pmsg_dlg->panel.bg = LIGHT_GRAY;
        pmsg_dlg->panel.fg = BLACK;
        pmsg_dlg->panel.num_btns = 0;
        for (i = 0; i < MAX_PANEL_BTNS; i++) {
            pmsg_dlg->panel.btn_addr[i] = NULL;
            pmsg_dlg->panel.action[i] = NULL;
        }
        pmsg_dlg->panel.pstash = NULL;

        len_btns = 0;
        if (pmsg_dlg->msg_dlg_type == OK) {
            // 2 for dialog margin, 2 for button margins
            len_btns = strlen("OK") + 2 + 2;
            if (AddButtonToPanel(&pmsg_dlg->panel, "Ok", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, Yes)) {
                retval = true;
            }
        } else if (pmsg_dlg->msg_dlg_type == YES_NO) {
            // 3 for margin + space between + 2 for each button margin
            len_btns = strlen("YES") + strlen("NO") + 3 + 2 + 2;
            if (AddButtonToPanel(&pmsg_dlg->panel, "Yes", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, Yes) &&
                AddButtonToPanel(&pmsg_dlg->panel, "No", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, No)) {
                retval = true;
            }
        } else if (pmsg_dlg->msg_dlg_type == OK_CANCEL) {
            // 3 for margin + space between + 2 for each button margin
            len_btns = strlen("OK") + strlen("CANCEL") + 3 + 2 + 2;
            if (AddButtonToPanel(&pmsg_dlg->panel, "Ok", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, Yes) &&
                AddButtonToPanel(&pmsg_dlg->panel, "Cancel", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, Cancel)) {
                retval = true;
            }
        } else if (pmsg_dlg->msg_dlg_type == YES_NO_CANCEL) {
            // 4 for dialog margin and spaces between, plus 2 for each button margin
            len_btns = strlen("YES") + strlen("NO") + strlen("CANCEL") + 4 + 2 + 2 + 2;
            if (AddButtonToPanel(&pmsg_dlg->panel, "Yes", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, Yes) &&
                AddButtonToPanel(&pmsg_dlg->panel, "No", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, No) &&
                AddButtonToPanel(&pmsg_dlg->panel, "Cancel", -1,
                                 DARK_RED, YELLOW, RED, YELLOW, YELLOW, Cancel)) {
                retval = true;
            }
        }
        pmsg_dlg->panel.w = (len_msg > len_btns) ? len_msg : len_btns;
        pmsg_dlg->panel.h = 1 + 1 + 3; // msg height + button height + 3 for margins and space between
    }
    return pmsg_dlg;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool ShowMsgDlg(msg_dlg_t * pmsg_dlg, uint8_t row, uint8_t col)
{
    if (pmsg_dlg != NULL) {
        if (pmsg_dlg->panel.pstash == NULL &&
            row + pmsg_dlg->panel.h < canvas_rows() &&
            col + pmsg_dlg->panel.w < canvas_cols()   ) {
            uint16_t bytes;
            pmsg_dlg->panel.r = row;
            pmsg_dlg->panel.c = col;
            // backup display covered by panel (4bpp takes 2 bytes each char)
            bytes = pmsg_dlg->panel.w*pmsg_dlg->panel.h*2;
            pmsg_dlg->panel.pstash = (uint8_t *)malloc(bytes);
            if (pmsg_dlg->panel.pstash != NULL) {
                int8_t i, r, c, start, len;
                bool succeeded = false;
                memset(pmsg_dlg->panel.pstash, 0, pmsg_dlg->panel.w*pmsg_dlg->panel.h*2);
                BackupChars(pmsg_dlg->panel.r, pmsg_dlg->panel.c, pmsg_dlg->panel.w, pmsg_dlg->panel.h, pmsg_dlg->panel.pstash);
                // draw panel background
                for (r = pmsg_dlg->panel.r; r < pmsg_dlg->panel.r + pmsg_dlg->panel.h; r++) {
                    for (c = pmsg_dlg->panel.c; c < pmsg_dlg->panel.c + pmsg_dlg->panel.w; c++) {
                        DrawChar(r, c, ' ', pmsg_dlg->panel.bg, pmsg_dlg->panel.fg);
                    }
                }
                // draw the dialog message
                len  = strlen(pmsg_dlg->dlg_msg);
                start = pmsg_dlg->panel.c + (pmsg_dlg->panel.w - len)/2;
                i = 0;
                for (c = start; c < (start + len); c++) {
                    DrawChar(pmsg_dlg->panel.r + 1, c, pmsg_dlg->dlg_msg[i++], pmsg_dlg->panel.bg, pmsg_dlg->panel.fg);
                }
                // show the buttons
                if (pmsg_dlg->msg_dlg_type == OK) {
                    if (pmsg_dlg->panel.btn_addr[0] != NULL) {
                        UpdateButtonFocus(pmsg_dlg->panel.btn_addr[0], true);
                        ShowButton(pmsg_dlg->panel.btn_addr[0], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w - 4)/2);
                        succeeded = true;
                    }
                } else if (pmsg_dlg->msg_dlg_type == YES_NO) {
                    if (pmsg_dlg->panel.btn_addr[0] != NULL &&
                        pmsg_dlg->panel.btn_addr[1] != NULL) {
                        UpdateButtonFocus(pmsg_dlg->panel.btn_addr[0], true);
                        ShowButton(pmsg_dlg->panel.btn_addr[0], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w - 10)/2);
                        ShowButton(pmsg_dlg->panel.btn_addr[1], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w - 10)/2 + 6);
                        succeeded = true;
                    }
                } else if (pmsg_dlg->msg_dlg_type == OK_CANCEL) {
                    if (pmsg_dlg->panel.btn_addr[0] != NULL &&
                        pmsg_dlg->panel.btn_addr[1] != NULL) {
                        UpdateButtonFocus(pmsg_dlg->panel.btn_addr[0], true);
                        ShowButton(pmsg_dlg->panel.btn_addr[0], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w - 13)/2);
                        ShowButton(pmsg_dlg->panel.btn_addr[1], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w - 13)/2 + 5);
                        succeeded = true;
                    }
                } else if (pmsg_dlg->msg_dlg_type == YES_NO_CANCEL) {
                    if (pmsg_dlg->panel.btn_addr[0] != NULL &&
                        pmsg_dlg->panel.btn_addr[1] != NULL &&
                        pmsg_dlg->panel.btn_addr[2] != NULL) {
                        UpdateButtonFocus(pmsg_dlg->panel.btn_addr[0], true);
                        ShowButton(pmsg_dlg->panel.btn_addr[0], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w-19)/2);
                        ShowButton(pmsg_dlg->panel.btn_addr[1], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w-19)/2 + 6);
                        ShowButton(pmsg_dlg->panel.btn_addr[2], pmsg_dlg->panel.r + 3, pmsg_dlg->panel.c + (pmsg_dlg->panel.w-19)/2 + 11);
                        succeeded = true;
                    }
                }
                if (succeeded){
                    return true;
                }
            } // stash allocation failed
            return false;
        }
    } // bad parameters
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DeleteMsgDlg(msg_dlg_t * pmsg_dlg)
{
    if (pmsg_dlg != NULL) {
        uint8_t i;
        textbox_t * txtbox = NULL;
        if (pmsg_dlg->panel.pstash != NULL) {
            RestoreChars(pmsg_dlg->panel.r, pmsg_dlg->panel.c, pmsg_dlg->panel.w, pmsg_dlg->panel.h, pmsg_dlg->panel.pstash);

            free(pmsg_dlg->panel.pstash);
            pmsg_dlg->panel.pstash = NULL;
        }
        for (i = 0; i < pmsg_dlg->panel.num_btns; i++) {
            button_t * btn = pmsg_dlg->panel.btn_addr[i];
            if (btn != NULL){
                free(btn);
                pmsg_dlg->panel.btn_addr[i] = NULL;
            }
        }
        set_popup(NULL);
        set_popup_type(NO_POPUP_TYPE);

        free(pmsg_dlg);

        UpdateTextboxFocus(true);

        return;
    } // bad parameter
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void MsgDlgButtonPressed(msg_dlg_t * pmsg_dlg, uint8_t index)
{
    if (pmsg_dlg != NULL) {
        if (get_popup() != NULL) {
            if (pmsg_dlg->msg_dlg_type == OK) {
                pmsg_dlg->panel.action[0](); // Ok (Yes)
            } else if (pmsg_dlg->msg_dlg_type == YES_NO) {
                switch (index) {
                    case 0: pmsg_dlg->panel.action[0](); break; // Yes
                    case 1: pmsg_dlg->panel.action[1](); break; // No
                }
            } else if (pmsg_dlg->msg_dlg_type == OK_CANCEL) {
                switch (index) {
                    case 0: pmsg_dlg->panel.action[0](); break; // Ok (Yes)
                    case 1: pmsg_dlg->panel.action[2](); break; // Cancel
                }
            } else if (pmsg_dlg->msg_dlg_type == YES_NO_CANCEL) {
                switch (index) {
                    case 0: pmsg_dlg->panel.action[0](); break; // Ok (Yes)
                    case 1: pmsg_dlg->panel.action[1](); break; // No
                    case 2: pmsg_dlg->panel.action[2](); break; // Cancel
                }
            }
            DeleteMsgDlg(pmsg_dlg);
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool IsMsgDlgButtonPressed(msg_dlg_t * pmsg_dlg, int16_t row, int16_t col)
{
    if (pmsg_dlg != NULL) {
        uint8_t i;
        for (i = 0; i < pmsg_dlg->panel.num_btns; i++) {
            button_t * btn = pmsg_dlg->panel.btn_addr[i];
            if (btn != NULL) {
                if (row == btn->r &&
                    col >= btn->c && col < (btn->c + btn->w)) {
                    MsgDlgButtonPressed(pmsg_dlg, i);
                    return true;
                }
            }
        }
        return true;
    } // bad parameter
    return false;
}