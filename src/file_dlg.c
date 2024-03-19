// ---------------------------------------------------------------------------
// FileDialog.cpp
// ---------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colors.h"
#include "doc.h"
#include "display.h"
#include "textbox.h"
#include "statusbar.h"
#include "button.h"
#include "panel.h"
#include "actions.h"
#include "file_dlg.h"

static textbox_t * p_main_text_box = NULL;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
file_dlg_t * NewFileDlg(file_dlg_type_t type, char * msg)
{
    bool retval = false;
    file_dlg_t * pfile_dlg = (file_dlg_t *)malloc(sizeof(file_dlg_t));
    if (pfile_dlg != NULL) {
        pfile_dlg->file_dlg_type = type;

        if (msg != NULL) {
            strncpy(pfile_dlg->dlg_msg, msg, MAX_FILE_DLG_MSG_LEN);
        } else {
            memset(pfile_dlg->dlg_msg, 0, MAX_FILE_DLG_MSG_LEN+1);
        }
        uint8_t len_msg = strlen(msg) + 2;

        // figure out size required for dialog
        pfile_dlg->panel.w = (len_msg > TXTBOX_WIDTH) ? len_msg : TXTBOX_WIDTH + 2;
        pfile_dlg->panel.h = 1 + 1 + 1 + 4; // msg + editbox + button heights + 4 for margins and spaces between

        pfile_dlg->panel.panel_type = FILEDIALOG;
        pfile_dlg->panel.btn_layout = HORZ;
        pfile_dlg->panel.r = 0; // filled in by ShowMsgDlg()
        pfile_dlg->panel.c = 0; // filled in by ShowMsgDlg()
        pfile_dlg->panel.bg = LIGHT_GRAY;
        pfile_dlg->panel.fg = BLACK;
        pfile_dlg->panel.num_btns = 0;
        for (uint8_t i = 0; i < MAX_PANEL_BTNS; i++) {
            pfile_dlg->panel.btn_addr[i] = NULL;
            pfile_dlg->panel.action[i] = NULL;
        }
        pfile_dlg->panel.pstash = NULL;

        // allocate the textbox
        pfile_dlg->ptxtbox = NewTextbox(TXTBOX_WIDTH, 1, BLACK, WHITE);
        if (pfile_dlg->ptxtbox != NULL) {
            p_main_text_box = get_active_textbox(); // save pointer to the main_textbox

            uint8_t len_btns = 0;
            if (pfile_dlg->file_dlg_type == OPEN) {
                // 3 for margin + space between + 2 for each button margin
                len_btns = strlen("Open") + strlen("Cancel") + 3 + 2 + 2;
                if (AddButtonToPanel(&pfile_dlg->panel, "Open", -1,
                                    DARK_RED, YELLOW, RED, YELLOW, YELLOW, OpenFile) &&
                    AddButtonToPanel(&pfile_dlg->panel, "Cancel", -1,
                                    DARK_RED, YELLOW, RED, YELLOW, YELLOW, NOP)) {
                    retval = true;
                }
            } else if (pfile_dlg->file_dlg_type == SAVE) {
                // 3 for margin + space between + 2 for each button margin
                len_btns = strlen("Save") + strlen("CANCEL") + 3 + 2 + 2;
                if (AddButtonToPanel(&pfile_dlg->panel, "Save", -1,
                                    DARK_RED, YELLOW, RED, YELLOW, YELLOW, SaveFileAs) &&
                    AddButtonToPanel(&pfile_dlg->panel, "Cancel", -1,
                                    DARK_RED, YELLOW, RED, YELLOW, YELLOW, NOP)) {
                    retval = true;
                }
            }
        }
        if (retval == false) {
            DeleteFileDlg(pfile_dlg);
            pfile_dlg = NULL;
        }
    }
    return pfile_dlg;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DeleteFileDlg(file_dlg_t * pfile_dlg)
{
    if (pfile_dlg != NULL) {
        DisableBlinking();
        set_active_textbox(NULL);

        if (pfile_dlg->panel.pstash != NULL) {
            RestoreChars(pfile_dlg->panel.r, pfile_dlg->panel.c, pfile_dlg->panel.w, pfile_dlg->panel.h, pfile_dlg->panel.pstash);

            free(pfile_dlg->panel.pstash);
            pfile_dlg->panel.pstash = NULL;
        }

        if (p_main_text_box != NULL) {
            UpdateTextboxFocus(p_main_text_box, true);
            p_main_text_box = NULL;
        }
        DeleteTextbox(pfile_dlg->ptxtbox);

        for (uint8_t i = 0; i < pfile_dlg->panel.num_btns; i++) {
            button_t * btn = pfile_dlg->panel.btn_addr[i];
            if (btn != NULL){
                free(btn);
                pfile_dlg->panel.btn_addr[i] = NULL;
            }
        }

        set_popup(NULL);
        set_popup_type(NO_POPUP_TYPE);

        free(pfile_dlg);

        return;
    } // bad parameter
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool ShowFileDlg(file_dlg_t * pfile_dlg, uint8_t row, uint8_t col)
{
    if (pfile_dlg != NULL) {
        if (pfile_dlg->panel.pstash == NULL &&
            row + pfile_dlg->panel.h < canvas_rows() &&
            col + pfile_dlg->panel.w < canvas_cols()   ) {

            pfile_dlg->panel.r = row;
            pfile_dlg->panel.c = col;

            // backup display covered by panel (4bpp takes 2 bytes each char)
            uint16_t bytes = pfile_dlg->panel.w*pfile_dlg->panel.h*2;
            pfile_dlg->panel.pstash = (uint8_t *)malloc(bytes);
            if (pfile_dlg->panel.pstash != NULL) {
                memset(pfile_dlg->panel.pstash, 0, pfile_dlg->panel.w*pfile_dlg->panel.h*2);
                BackupChars(pfile_dlg->panel.r, pfile_dlg->panel.c, pfile_dlg->panel.w, pfile_dlg->panel.h, pfile_dlg->panel.pstash);

                // draw panel background
                for (uint8_t i = pfile_dlg->panel.r; i < pfile_dlg->panel.r + pfile_dlg->panel.h; i++) {
                    for (uint8_t j = pfile_dlg->panel.c; j < pfile_dlg->panel.c + pfile_dlg->panel.w; j++) {
                        DrawChar(i, j, ' ', pfile_dlg->panel.bg, pfile_dlg->panel.fg);
                    }
                }

                // draw the dialog message
                uint8_t len  = strlen(pfile_dlg->dlg_msg);
                uint16_t start = pfile_dlg->panel.c + (pfile_dlg->panel.w - len)/2;
                uint8_t i = 0;
                for (uint8_t j = start; j < (start + len); j++) {
                    DrawChar(pfile_dlg->panel.r + 1, j, pfile_dlg->dlg_msg[i++], pfile_dlg->panel.bg, pfile_dlg->panel.fg);
                }

                // show the textbox
                ShowTextbox(pfile_dlg->ptxtbox, pfile_dlg->panel.r + 3, pfile_dlg->panel.c+1);

                // show the buttons
                if (pfile_dlg->panel.btn_addr[0] != NULL &&
                    pfile_dlg->panel.btn_addr[1] != NULL) {
                    ShowButton(pfile_dlg->panel.btn_addr[0], pfile_dlg->panel.r + 5, pfile_dlg->panel.c + (pfile_dlg->panel.w - 15)/2);
                    ShowButton(pfile_dlg->panel.btn_addr[1], pfile_dlg->panel.r + 5, pfile_dlg->panel.c + (pfile_dlg->panel.w - 15)/2 + 7);
                    UpdateTextboxFocus(pfile_dlg->ptxtbox, true);
                    return true;
                }
            } // stash allocation failed
            return false;
        }
    } // bad parameters
    return false;
}

// ---------------------------------------------------------------------------
// Copy the Filename from the file_dlg textbox doc to the main doc.
// ---------------------------------------------------------------------------
static bool GetFilename(file_dlg_t * pfile_dlg)
{
    if (pfile_dlg != NULL){
        uint8_t filename_len = 0;
        if (ReadStr(pfile_dlg->ptxtbox->doc.rows->ptxt,
                    pfile_dlg->ptxtbox->doc.filename,
                    pfile_dlg->ptxtbox->doc.rows->len) &&
                    ((filename_len = strlen(pfile_dlg->ptxtbox->doc.filename)) != 0)) {
            if (p_main_text_box != NULL) {
                memset(p_main_text_box->doc.filename, 0, MAX_FILENAME+1);
                strncpy(p_main_text_box->doc.filename,
                        pfile_dlg->ptxtbox->doc.filename,
                        filename_len);
                pfile_dlg->ptxtbox->doc.filename[filename_len+1] = 0;
                //printf("Filename = %s, len = %u\n", p_main_text_box->doc.filename, filename_len);
                return true;
            }
        }
        UpdateStatusBarMsg("Filename was blank.", STATUS_WARNING);
    }
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileDlgButtonPressed(file_dlg_t * pfile_dlg, uint8_t index)
{
    if (pfile_dlg != NULL) {
        if (pfile_dlg->file_dlg_type == OPEN) {
            switch (index) {
                case 0:
                    if (GetFilename(pfile_dlg)) {
                        pfile_dlg->panel.action[0]();
                    }
                    break; // Open
                case 1:
                    pfile_dlg->panel.action[1]();
                    break; // Cancel
            }
        } else if (pfile_dlg->file_dlg_type == SAVE) {
            switch (index) {
                case 0:
                    if (GetFilename(pfile_dlg)) {
                        pfile_dlg->panel.action[0]();
                    }
                    break; // Save
                case 1:
                    pfile_dlg->panel.action[1]();
                    break; // Cancel
            }
        }
        DeleteFileDlg(pfile_dlg);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool IsFileDlgButtonPressed(file_dlg_t * pfile_dlg, int16_t row, int16_t col)
{
    if (pfile_dlg != NULL) {
        for (uint8_t i = 0; i < pfile_dlg->panel.num_btns; i++) {
            button_t * btn = pfile_dlg->panel.btn_addr[i];
            if (btn != NULL) {
                if (row == btn->r &&
                    col >= btn->c && col < (btn->c + btn->w)) {
                    FileDlgButtonPressed(pfile_dlg, i);
                    return true;
                }
            }
        }
        return true;
    } // bad parameter
    return false;
}

// ---------------------------------------------------------------------------
// When this dialog is open, we can't get access to the Main Textbox
// using get_active_textbox(), so this is the alternative way.
// Use it for FileOpen, FileSaveAs button actions.
// ---------------------------------------------------------------------------
doc_t * GetDoc(file_dlg_t * pfile_dlg)
{
    if (pfile_dlg != NULL && p_main_text_box != NULL) {
        return &p_main_text_box->doc;
    } else {
        return NULL;
    }
}