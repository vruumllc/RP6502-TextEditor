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
#include "file_ops.h"
#include "file_dlg.h"
#include "actions.h"

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileOpen(void)
{
    CloseAnyPopupMenu();
    if (TheDoc.dirty) {
        UpdateStatusBarMsg("File has changed! Save or close before opening new one.", STATUS_WARNING);
    } else {
        file_dlg_t * file_dialog = NewFileDlg(OPEN, "Specify name of file to open:");
        if (file_dialog != NULL) {
            uint8_t show_r, show_c;
            set_popup(file_dialog);
            set_popup_type(FILEDIALOG);
            UpdateTextboxFocus(false);
            show_r = (canvas_rows()-file_dialog->panel.h)/2;
            show_c = (canvas_cols()-file_dialog->panel.w)/2;
            if (!ShowFileDlg(file_dialog, show_r, show_c)) {
                DeleteFileDlg(file_dialog);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileSave(void)
{
    CloseAnyPopupMenu();
    if (strlen(TheDoc.filename) != 0) {
        //printf("FileSave: Filename = %s\n", TheDoc.filename);
        ResaveFile();
    } else { // need to prompt for name first
        UpdateStatusBarMsg("Filename needs to be specified!", STATUS_INFO);
        FileSaveAs();
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void FileSaveAs(void)
{
    file_dlg_t * file_dialog = NULL;
    CloseAnyPopupMenu();
    file_dialog = NewFileDlg(SAVE, "Specify name of file to save:");
    if (file_dialog != NULL) {
        uint8_t show_r, show_c;
        set_popup(file_dialog);
        set_popup_type(FILEDIALOG);
        UpdateTextboxFocus(false);
        show_r = (canvas_rows()-file_dialog->panel.h)/2;
        show_c = (canvas_cols()-file_dialog->panel.w)/2;
        if (!ShowFileDlg(file_dialog, show_r, show_c)) {
            DeleteFileDlg(file_dialog);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SaveBeforeClose(void)
{
    FileSave();
    InitTextbox();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void CloseFileWithoutSaving(void)
{
    InitTextbox();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static void PromptForSaveFromFileClose(void)
{
    msg_dlg_t * msg_dialog = NewMsgDlg("Save modified file before closing?",
                                       YES_NO_CANCEL,
                                       SaveBeforeClose, CloseFileWithoutSaving, NOP);
    if (msg_dialog != NULL) {
        uint8_t show_r, show_c;
        set_popup(msg_dialog);
        set_popup_type(MSGDIALOG);
        UpdateTextboxFocus(false);
        show_r = (canvas_rows()-msg_dialog->panel.h)/2;
        show_c = (canvas_cols()-msg_dialog->panel.w)/2;
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
    if (TheDoc.dirty) {
        UpdateStatusBarMsg("File has changed! Save before close.", STATUS_INFO);
        PromptForSaveFromFileClose();
    } else {
        InitTextbox();
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
        uint8_t show_r, show_c;
        set_popup(msg_dialog);
        set_popup_type(MSGDIALOG);
        UpdateTextboxFocus(false);
        show_r = (canvas_rows()-msg_dialog->panel.h)/2;
        show_c = (canvas_cols()-msg_dialog->panel.w)/2;
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
    if (TheDoc.dirty) {
        UpdateStatusBarMsg("File has changed! Save before exit.", STATUS_INFO);
        PromptForSaveFromFileExit();
    } else {
        msg_dlg_t * msg_dialog = NewMsgDlg("Are you sure you want to exit?",
                                            YES_NO, ExitWithoutSaving, NOP, NOP);
        if (msg_dialog != NULL) {
            uint8_t show_r, show_c;
            set_popup(msg_dialog);
            set_popup_type(MSGDIALOG);
            UpdateTextboxFocus(false);
            show_r = (canvas_rows()-msg_dialog->panel.h)/2;
            show_c = (canvas_cols()-msg_dialog->panel.w)/2;
            if (!ShowMsgDlg(msg_dialog, show_r, show_c)) {
                DeleteMsgDlg(msg_dialog);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditCut(void)
{
    CloseAnyPopupMenu();
    if (!CopyMarkedTextToClipboard()) {
        UpdateStatusBarMsg("Out of memory for Clipboard!", STATUS_ERROR);
    } else {
        CutMarkedText();
    }
    ClearMarkedText();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditCopy(void)
{
    CloseAnyPopupMenu();
    if (!CopyMarkedTextToClipboard()) {
        UpdateStatusBarMsg("Out of memory for Clipboard!", STATUS_ERROR);
    }
    ClearMarkedText();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void EditPaste(void)
{
    CloseAnyPopupMenu();
    if (!PasteTextFromClipboard()) {
        UpdateStatusBarMsg("Paste would exceed row or column limits!", STATUS_WARNING);
    }
}
/*
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
    msg_dlg_t * msg_dialog = NULL;
    CloseAnyPopupMenu();
    msg_dialog = NewMsgDlg("TE, written by tonyvr", OK, NOP, NOP, NOP);
    if (msg_dialog != NULL) {
        uint8_t show_r, show_c;
        set_popup(msg_dialog);
        set_popup_type(MSGDIALOG);
        UpdateTextboxFocus(false);
        show_r = (canvas_rows()-msg_dialog->panel.h)/2;
        show_c = (canvas_cols()-msg_dialog->panel.w)/2;
        if (!ShowMsgDlg(msg_dialog, show_r, show_c)) {
            DeleteMsgDlg(msg_dialog);
        }
    }
}
