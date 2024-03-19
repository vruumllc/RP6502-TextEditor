// ---------------------------------------------------------------------------
// FileDialog.h
// ---------------------------------------------------------------------------

#ifndef FILE_DLG_H
#define FILE_DLG_H

#include <stdint.h>
#include <stdbool.h>
#include "textbox.h"
#include "panel.h"

#define MAX_FILE_DLG_MSG_LEN 80
#define TXTBOX_WIDTH 31

typedef enum {INVALID_FILE_DLG_TYPE, OPEN, SAVE} file_dlg_type_t;

typedef struct file_dlg {
    file_dlg_type_t file_dlg_type;
    char dlg_msg[MAX_FILE_DLG_MSG_LEN+1];
    textbox_t * ptxtbox;
    panel_t panel;
} file_dlg_t;

file_dlg_t * NewFileDlg(file_dlg_type_t type, char * msg);
void DeleteFileDlg(file_dlg_t * pfile_dlg);

bool ShowFileDlg(file_dlg_t * pfile_dlg, uint8_t row, uint8_t col);

void FileDlgButtonPressed(file_dlg_t * pfile_dlg, uint8_t index);
bool IsFileDlgButtonPressed(file_dlg_t * pfile_dlg, int16_t row, int16_t col);

doc_t * GetDoc(file_dlg_t * pfile_dlg);

#endif // FILE_DLG_H