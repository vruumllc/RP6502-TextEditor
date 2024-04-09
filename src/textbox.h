// ---------------------------------------------------------------------------
// textbox.h
// ---------------------------------------------------------------------------

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <stdbool.h>
#include <stdint.h>
#include "doc.h"

#define INSERT_CURSOR 178 // 179 // '|'
#define CLIPBOARD_SIZE 1024

typedef struct textbox {
    uint8_t r;
    uint8_t c;
    uint8_t h;
    uint8_t w;
    uint8_t bg;
    uint8_t fg;
    bool in_focus;
    bool row_dirty[DOC_ROWS_DISPLAYED]; // if display row needs redrawing
} textbox_t;

extern textbox_t TheTextbox;
extern char TheClipboard[CLIPBOARD_SIZE];

void InitTextbox(void);
void UpdateCursor();
void UpdateTextboxFocus(bool has_focus);
void UpdateTextbox(); // Called by main loop periodically to redraw document in textbox
void SetAllTextboxRowsDirty(void);

void StartMarkingText(void);
bool MarkingText(int16_t cur_row, int16_t cur_col);
void MarkText(void);
void StopMarkingText(void);
void ClearMarkedText(void);
bool CopyMarkedTextToClipboard(void);
bool CutMarkedText(void);
bool PasteTextFromClipboard(void);

void * get_popup(void); // NULL, unless popup is overlapping display
void set_popup(void * popup);

popup_type_t get_popup_type(void);
void set_popup_type(popup_type_t popup_type);

#endif // TEXTBOX_H