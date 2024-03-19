// ---------------------------------------------------------------------------
// button.h
// ---------------------------------------------------------------------------

#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_BTN_LBL_LEN 31

typedef struct button {
    uint8_t bg;
    uint8_t fg;
    uint8_t focus_bg;
    uint8_t focus_fg;
    uint8_t alt_fg;
    int8_t alt_index;
    bool in_focus;
    bool is_showing;
    uint8_t r;
    uint8_t c;
    uint8_t w;
    //uint8_t h; // always 1
    char btn_lbl[MAX_BTN_LBL_LEN+1];
} button_t;

button_t * NewButton(const char * label,
                     uint8_t bg_color, uint8_t fg_color,
                     uint8_t focus_bg_color, uint8_t focus_fg_color,
                     uint8_t alt_fg_color, int8_t alt_index);

bool ShowButton(button_t * btn,  uint16_t row, uint16_t col);
void UpdateButtonFocus(button_t * btn, bool has_focus);

#endif // BUTTON_H