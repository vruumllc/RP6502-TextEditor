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
#include "file_ops.h"
#include "file_dlg.h"

static doc_t * doc = NULL;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
file_dlg_t * NewFileDlg(file_dlg_type_t type, char * msg)
{
    bool retval = false;
    file_dlg_t * pfile_dlg = (file_dlg_t *)malloc(sizeof(file_dlg_t));
    if (pfile_dlg != NULL) {
        uint8_t i, len_msg, len_btns;

        doc = GetDoc();
        pfile_dlg->file_dlg_type = type;

        if (msg != NULL) {
            strncpy(pfile_dlg->dlg_msg, msg, MAX_FILE_DLG_MSG_LEN);
        } else {
            memset(pfile_dlg->dlg_msg, 0, MAX_FILE_DLG_MSG_LEN+1);
        }
        len_msg = strlen(msg) + 2;

        // figure out size required for dialog
        pfile_dlg->panel.w = ((len_msg > MAX_FILENAME) ? len_msg: MAX_FILENAME) + 2;
        pfile_dlg->panel.h = 1 + 1 + 1 + 4; // msg + editbox + button heights + 4 for margins and spaces between

        pfile_dlg->panel.panel_type = FILEDIALOG;
        pfile_dlg->panel.btn_layout = HORZ;
        pfile_dlg->panel.r = 0; // filled in by ShowMsgDlg()
        pfile_dlg->panel.c = 0; // filled in by ShowMsgDlg()
        pfile_dlg->panel.bg = LIGHT_GRAY;
        pfile_dlg->panel.fg = BLACK;
        pfile_dlg->panel.num_btns = 0;
        for (i = 0; i < MAX_PANEL_BTNS; i++) {
            pfile_dlg->panel.btn_addr[i] = NULL;
            pfile_dlg->panel.action[i] = NULL;
        }
        pfile_dlg->panel.pstash = NULL;

        // add buttons
        len_btns = 0;
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
                                DARK_RED, YELLOW, RED, YELLOW, YELLOW, SaveNewFile) &&
                AddButtonToPanel(&pfile_dlg->panel, "Cancel", -1,
                                DARK_RED, YELLOW, RED, YELLOW, YELLOW, NOP)) {
                retval = true;
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
        uint8_t i;

        if (pfile_dlg->panel.pstash != NULL) {
            RestoreChars(pfile_dlg->panel.r, pfile_dlg->panel.c, pfile_dlg->panel.w, pfile_dlg->panel.h, pfile_dlg->panel.pstash);

            free(pfile_dlg->panel.pstash);
            pfile_dlg->panel.pstash = NULL;
        }

        for (i = 0; i < pfile_dlg->panel.num_btns; i++) {
            button_t * btn = pfile_dlg->panel.btn_addr[i];
            if (btn != NULL){
                free(btn);
                pfile_dlg->panel.btn_addr[i] = NULL;
            }
        }

        set_popup(NULL);
        set_popup_type(NO_POPUP_TYPE);

        free(pfile_dlg);

        UpdateTextboxFocus(true);

        return;
    } // bad parameter
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool ShowFileDlg(file_dlg_t * pfile_dlg, uint8_t row, uint8_t col)
{
    if (pfile_dlg != NULL) {
        uint16_t bytes;
        uint8_t i, r, c, start, len;

        if (pfile_dlg->panel.pstash == NULL &&
            row + pfile_dlg->panel.h < canvas_rows() &&
            col + pfile_dlg->panel.w < canvas_cols()   ) {

            pfile_dlg->panel.r = row;
            pfile_dlg->panel.c = col;

            // backup display covered by panel (4bpp takes 2 bytes each char)
            bytes = pfile_dlg->panel.w*pfile_dlg->panel.h*2;
            pfile_dlg->panel.pstash = (uint8_t *)malloc(bytes);
            if (pfile_dlg->panel.pstash != NULL) {
                memset(pfile_dlg->panel.pstash, 0, pfile_dlg->panel.w*pfile_dlg->panel.h*2);
                BackupChars(pfile_dlg->panel.r, pfile_dlg->panel.c, pfile_dlg->panel.w, pfile_dlg->panel.h, pfile_dlg->panel.pstash);

                // draw panel background
                for (r = pfile_dlg->panel.r; r < pfile_dlg->panel.r + pfile_dlg->panel.h; r++) {
                    for (c = pfile_dlg->panel.c; c < pfile_dlg->panel.c + pfile_dlg->panel.w; c++) {
                        DrawChar(r, c, ' ', pfile_dlg->panel.bg, pfile_dlg->panel.fg);
                    }
                }

                // draw the dialog message
                len  = strlen(pfile_dlg->dlg_msg);
                start = pfile_dlg->panel.c + (pfile_dlg->panel.w - len)/2;
                i = 0;
                for (c = start; c < (start + len); c++) {
                    DrawChar(pfile_dlg->panel.r + 1, c, pfile_dlg->dlg_msg[i++], pfile_dlg->panel.bg, pfile_dlg->panel.fg);
                }

                // draw the filename
                len  = strlen(doc->filename);
                i = 0;
                for (c = pfile_dlg->panel.c + 1; c < pfile_dlg->panel.c + pfile_dlg->panel.w - 1; c++) {
                    DrawChar(pfile_dlg->panel.r + 3, c,  doc->filename[i++], BLACK, WHITE);
                }

                // initialize doc filename cursor locations
                doc->cur_filename_r = pfile_dlg->panel.r + 3;
                doc->cur_filename_c = pfile_dlg->panel.c + 1 + len;
                UpdateTextboxFocus(true);

                // show the buttons
                if (pfile_dlg->panel.btn_addr[0] != NULL &&
                    pfile_dlg->panel.btn_addr[1] != NULL) {
                    ShowButton(pfile_dlg->panel.btn_addr[0], pfile_dlg->panel.r + 5, pfile_dlg->panel.c + (pfile_dlg->panel.w - 15)/2);
                    ShowButton(pfile_dlg->panel.btn_addr[1], pfile_dlg->panel.r + 5, pfile_dlg->panel.c + (pfile_dlg->panel.w - 15)/2 + 7);
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
void FileDlgButtonPressed(file_dlg_t * pfile_dlg, uint8_t index)
{
    if (pfile_dlg != NULL) {
        if (pfile_dlg->file_dlg_type == OPEN) {
            switch (index) {
                case 0:
                    pfile_dlg->panel.action[0]();
                    break; // Open
                case 1:
                    pfile_dlg->panel.action[1]();
                    break; // Cancel
            }
        } else if (pfile_dlg->file_dlg_type == SAVE) {
            switch (index) {
                case 0:
                    pfile_dlg->panel.action[0]();
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
        uint8_t i;
        for (i = 0; i < pfile_dlg->panel.num_btns; i++) {
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
// ---------------------------------------------------------------------------
void AddCharToFilename(char chr)
{
    if ((chr >= 'A' && chr <= 'Z') ||
        (chr >= 'a' && chr <= 'z') ||
        (chr >= '0' && chr <= '9') ||
         chr == '-' || chr == '_' ||
         chr == ':' || chr == '/' || chr == '.') {
        int8_t len = strlen(doc->filename);
        if (len < MAX_FILENAME-1) {
            UpdateTextboxFocus(false);
            DrawChar(doc->cur_filename_r, doc->cur_filename_c++, chr, BLACK, WHITE);
            doc->filename[len++] = chr;
            UpdateTextboxFocus(true);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void DeleteCharFromFilename(void)
{
    int8_t len = strlen(doc->filename);
    if (len > 0) {
        UpdateTextboxFocus(false);
        DrawChar(doc->cur_filename_r, --doc->cur_filename_c, ' ', BLACK, WHITE);
        doc->filename[--len] = 0;
        UpdateTextboxFocus(true);
    }
}
