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
    uint8_t h;
    uint8_t w;
    uint8_t bg;
    uint8_t fg;
    bool in_focus;
    bool row_dirty[28]; // if display row needs redrawing
} textbox_t;

void InitTextbox(void);
textbox_t * GetTextbox(void);
void UpdateCursor();
void UpdateTextboxFocus(bool has_focus);
void UpdateTextbox(); // Called by main loop periodically to redraw document in textbox
void DisableOverwriteMode(void);
void EnableOverwriteMode(void);

#endif // TEXTBOX_H