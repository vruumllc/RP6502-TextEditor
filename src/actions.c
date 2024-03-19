// ---------------------------------------------------------------------------
// actions.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "doc.h"
#include "display.h"
#include "textbox.h"
#include "statusbar.h"
#include "panel.h"
#include "msg_dlg.h"
#include "file_dlg.h"
#include "actions.h"

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileOpen(void)
{
    CloseAnyPopupMenu();

    file_dlg_t * file_dialog = NewFileDlg(OPEN, "Specify name of file to open:");
    if (file_dialog != NULL) {
        set_popup(file_dialog);
        set_popup_type(FILEDIALOG);

        DisableBlinking();

        uint8_t show_r = (canvas_rows()-file_dialog->panel.h)/2;
        uint8_t show_c = (canvas_cols()-file_dialog->panel.w)/2;
        if (!ShowFileDlg(file_dialog, show_r, show_c)) {
            DeleteFileDlg(file_dialog);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void OpenFile(void)
{
    file_dlg_t * file_dlg = get_popup();
    popup_type_t popup_type = get_popup_type();
    if (file_dlg != NULL && popup_type == FILEDIALOG) {
        // the file dialog is up, so get pointer to main doc
        doc_t * doc = GetDoc(file_dlg);
        if (doc != NULL) {
            //printf("OpenFile: Filename = %s\n", doc->filename);
            char tmp[MAX_FILENAME+10] = {0};
            strncpy(tmp, doc->filename, MAX_FILENAME+1);
            InitDoc(doc, false);
            strncpy(doc->filename, tmp, MAX_FILENAME+1);
            if (strlen(doc->filename) > 0) {
                int16_t retval = EINVAL;
                uint16_t offset = 0;
                uint16_t wrapped_file_lines = 0;

                int16_t fd = open(doc->filename, O_RDONLY);
                if (fd >= 0) {
                    char buf[DOC_COLS] = {0};
                    char row[DOC_COLS] = {0};
                    for (uint16_t r = 0; r < DOC_ROWS; r++) {
                        memset(buf, 0, DOC_COLS);
                        memset(row, 0, DOC_COLS);
                        doc->rows[r].ptxt = (void*)(DOC_MEM_START+ r*DOC_COLS);
                        doc->rows[r].len = 0;
                        if ((retval = read(fd, buf, DOC_COLS)) > 0) {
                            bool IsWindows = false;
                            char * pch = strchr(buf, '\r');
                            if (pch != NULL) { // Windows
                                IsWindows = true;
                                *pch = '\n';
                                *(++pch) = 0;
                            } else { // linux
                                pch = strchr(buf, '\n');
                                if (pch != NULL) {
                                    *(++pch) = 0;
                                } else { // line to too long!
                                    char msg[MAX_STATUS_MSG+1] = {0};
                                    snprintf(msg, MAX_STATUS_MSG,
                                             "File line %u too long, so wrapped it!",
                                             r + 1 - wrapped_file_lines);
                                    wrapped_file_lines++;
                                    buf[DOC_COLS-2] = '\n';
                                    buf[DOC_COLS-1] = 0;
                                    doc->dirty = true;
                                    UpdateStatusBarMsg(msg, STATUS_WARNING);
                                }
                            }
                            strncpy(row, buf, DOC_COLS);
                            doc->rows[r].len = strlen(row)-1; // don't count '\n'
                            //printf("r=%u, %s", r, row);
                            WriteStr(doc->rows[r].ptxt, row, doc->rows[r].len+1); // copy '\n' too
                            doc->rows[r].dirty = true;
                            if (doc->last_col < doc->rows[r].len) {
                                doc->last_col = doc->rows[r].len;
                            }
                            doc->last_row = r;

                            // set file pointer to start of next row text
                            offset += doc->rows[r].len+1+(IsWindows?1:0);
                            if (lseek(fd, offset, SEEK_SET) < 0) {
                                ReportFileError();
                            }
                        } else {
                            if (retval < 0) {
                                ReportFileError();
                            }
                        }
                    }
                    if (close(fd) < 0) {
                        ReportFileError();
                    }
                } else {
                    ReportFileError();
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileSave(void)
{
    CloseAnyPopupMenu();

    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL) {
        doc_t * doc = &txtbox->doc;
        if (strlen(doc->filename) != 0) {
            //printf("FileSave: Filename = %s\n", doc->filename);
            int16_t fd = open(doc->filename, O_WRONLY |
                                             O_TRUNC  ); // rewrite
            if (fd >= 0) {
                char row[DOC_COLS];
                for (uint16_t r = 0; r <= doc->last_row; r++) {
                    memset(row, 0, DOC_COLS);
                    ReadStr(doc->rows[r].ptxt, row, doc->rows[r].len+1);
                    //printf("r=%u %s", r, row);
                    if (write(fd, row, doc->rows[r].len+1) < 0) {
                        ReportFileError();
                    }
                }
                if (close(fd) < 0) {
                    ReportFileError();
                }
                doc->dirty = false;
            } else {
                ReportFileError();
            }
        } else { // need to prompt for name first
            UpdateStatusBarMsg("Filename needs to be specified!", STATUS_INFO);
            FileSaveAs();
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileSaveAs(void)
{
    CloseAnyPopupMenu();

    file_dlg_t * file_dialog = NewFileDlg(SAVE, "Specify name of file to save:");
    if (file_dialog != NULL) {
        set_popup(file_dialog);
        set_popup_type(FILEDIALOG);

        DisableBlinking();

        uint8_t show_r = (canvas_rows()-file_dialog->panel.h)/2;
        uint8_t show_c = (canvas_cols()-file_dialog->panel.w)/2;
        if (!ShowFileDlg(file_dialog, show_r, show_c)) {
            DeleteFileDlg(file_dialog);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SaveFileAs(void)
{
    file_dlg_t * file_dlg = get_popup();
    popup_type_t popup_type = get_popup_type();
    if (file_dlg != NULL && popup_type == FILEDIALOG) {
        // the file dialog is up, so get pointer to main doc
        doc_t * doc = GetDoc(file_dlg);
        if (doc != NULL) {
            //printf("SaveFileAs: Filename = %s\n", doc->filename);
            int16_t fd = open(doc->filename, O_WRONLY|
                                             O_CREAT |
                                             O_EXCL  | // fail if exists
                                             O_TRUNC );
            if (fd >= 0) {
                char row[DOC_COLS];
                for (uint16_t r = 0; r <= doc->last_row; r++) {
                    memset(row, 0, DOC_COLS);
                    ReadStr(doc->rows[r].ptxt, row, doc->rows[r].len+1);
                    //printf("r=%u %s", r, row);
                    if (write(fd, row, doc->rows[r].len+1) < 0) {
                        ReportFileError();
                    }
                }
                close(fd);
                doc->dirty = false;
            } else {
                if (errno == 0) { // bug: open() always sets errno to 0
                    errno = FR_EXIST; // probably
                    // wipeout bad filename in doc!
                    memset(doc->filename, 0, MAX_FILENAME+1);
                }
                ReportFileError();
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SaveBeforeClose(void)
{
    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL) {
        FileSave();
        InitDoc(&txtbox->doc, false);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void CloseFileWithoutSaving(void)
{
    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL) {
        InitDoc(&txtbox->doc, false);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static void PromptForSaveFromFileClose(void)
{
    msg_dlg_t * msg_dialog = NewMsgDlg("Save modified file before closing?",
                                       YES_NO_CANCEL,
                                       SaveBeforeClose, CloseFileWithoutSaving, NOP);
    if (msg_dialog != NULL) {
        set_popup(msg_dialog);
        set_popup_type(MSGDIALOG);

        DisableBlinking();

        uint8_t show_r = (canvas_rows()-msg_dialog->panel.h)/2;
        uint8_t show_c = (canvas_cols()-msg_dialog->panel.w)/2;
        if (!ShowMsgDlg(msg_dialog, show_r, show_c)) {
            DeleteMsgDlg(msg_dialog);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileClose(void)
{
    CloseAnyPopupMenu();

    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL) {
        if (txtbox->doc.dirty) {
            UpdateStatusBarMsg("File has changed! Save before close.", STATUS_INFO);
            PromptForSaveFromFileClose();
        } else {
            InitDoc(&txtbox->doc, false);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SaveBeforeExit(void)
{
    FileSave();
    exit(0);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void ExitWithoutSaving(void)
{
    exit(0);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static void PromptForSaveFromFileExit(void)
{
    msg_dlg_t * msg_dialog = NewMsgDlg("Save modified file before exiting?",
                                       YES_NO_CANCEL,
                                       SaveBeforeExit, ExitWithoutSaving, NOP);
    if (msg_dialog != NULL) {
        set_popup(msg_dialog);
        set_popup_type(MSGDIALOG);

        DisableBlinking();

        uint8_t show_r = (canvas_rows()-msg_dialog->panel.h)/2;
        uint8_t show_c = (canvas_cols()-msg_dialog->panel.w)/2;
        if (!ShowMsgDlg(msg_dialog, show_r, show_c)) {
            DeleteMsgDlg(msg_dialog);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileExit(void)
{
    CloseAnyPopupMenu();

    textbox_t * txtbox = get_active_textbox();
    if (txtbox != NULL) {
        if (txtbox->doc.dirty) {
            UpdateStatusBarMsg("File has changed! Save before exit.", STATUS_INFO);
            PromptForSaveFromFileExit();
        } else {
            msg_dlg_t * msg_dialog = NewMsgDlg("Are you sure you want to exit?",
                                            YES_NO, FileExitYes, NOP, NOP);
            if (msg_dialog != NULL) {
                set_popup(msg_dialog);
                set_popup_type(MSGDIALOG);

                DisableBlinking();

                uint8_t show_r = (canvas_rows()-msg_dialog->panel.h)/2;
                uint8_t show_c = (canvas_cols()-msg_dialog->panel.w)/2;
                if (!ShowMsgDlg(msg_dialog, show_r, show_c)) {
                    DeleteMsgDlg(msg_dialog);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileExitYes(void)
{
    CloseAnyPopupMenu();
    exit(0);
}
/*
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditCut(void)
{
    CloseAnyPopupMenu();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditCopy(void)
{
    CloseAnyPopupMenu();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditPaste(void)
{
    CloseAnyPopupMenu();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditFind(void)
{
    CloseAnyPopupMenu();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditReplace(void)
{
    CloseAnyPopupMenu();
}
*/
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void HelpAbout(void)
{
    CloseAnyPopupMenu();

    msg_dlg_t * msg_dialog = NewMsgDlg("Te, written by tonyvr", OK, NOP, NOP, NOP);
    if (msg_dialog != NULL) {
        set_popup(msg_dialog);
        set_popup_type(MSGDIALOG);

        DisableBlinking();

        uint8_t show_r = (canvas_rows()-msg_dialog->panel.h)/2;
        uint8_t show_c = (canvas_cols()-msg_dialog->panel.w)/2;
        if (!ShowMsgDlg(msg_dialog, show_r, show_c)) {
            DeleteMsgDlg(msg_dialog);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void NOP(void)
{
    CloseAnyPopupMenu();
}