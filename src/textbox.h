// ---------------------------------------------------------------------------
// textbox.h
// ---------------------------------------------------------------------------

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <stdbool.h>
#include <stdint.h>
#include "doc.h"

#define INSERT_CURSOR 178 // 179 // '|'

typedef struct textbox {
    uint8_t r;
    uint8_t c;
    uint8_t w;
    uint8_t h;
    uint8_t bg;
    uint8_t fg;
    bool in_focus;
    doc_t doc;
} textbox_t;

textbox_t * NewTextbox(uint8_t width, uint8_t height, uint8_t bg_color, uint8_t fg_color);
void DeleteTextbox(textbox_t * txtbox);
void ShowTextbox(textbox_t * txtbox,  uint16_t row, uint16_t col);

void UpdateCursor();

void UpdateActiveTextbox(); // Called by main loop periodically to redraw document in textbox

void DisableBlinking(void);
void EnableBlinking(void);

void DisableOverwriteMode(void);
void EnableOverwriteMode(void);

void UpdateTextboxFocus(textbox_t * txtbox, bool has_focus/*,
                        uint16_t new_cursor_row, uint16_t new_cursor_col*/);

//uint8_t cursor_row(void);
//uint8_t cursor_col(void);

#endif // TEXTBOX_H