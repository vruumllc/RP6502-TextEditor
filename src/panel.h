// ---------------------------------------------------------------------------
// panel.h
// ---------------------------------------------------------------------------

#ifndef PANEL_H
#define PANEL_H

#include <stdbool.h>
#include <stdint.h>
#include "display.h"
#include "button.h"

typedef enum {HORZ, VERT} panel_btn_layout_t;
typedef void (*panel_btn_action)(void);

#define MAX_PANEL_BTNS 5

typedef struct panel {
    popup_type_t panel_type;
    uint16_t r;
    uint16_t c;
    uint8_t w;
    uint8_t h;
    uint16_t bg;
    uint16_t fg;
    panel_btn_layout_t btn_layout;
    uint8_t num_btns;
    button_t * btn_addr[MAX_PANEL_BTNS];
    panel_btn_action action[MAX_PANEL_BTNS];
    uint8_t * pstash; // mem allocated and used to save & restore chars covered by popup panel
} panel_t;

panel_t * NewPanel(popup_type_t type, panel_btn_layout_t button_layout,
                   uint8_t width, uint8_t height,
                   uint8_t bg_color, uint8_t fg_color);

bool AddButtonToPanel(panel_t * panel, const char *label, int8_t alt_index,
                      uint8_t bg_color, uint8_t fg_color,
                      uint8_t focus_bg_color, uint8_t focus_fg_color,
                      uint8_t alt_fg_color, panel_btn_action action);

bool ShowPanel(panel_t * panel, uint8_t row, uint8_t col);
void DeletePanel(panel_t * panel);

void PanelButtonPressed(panel_t * panel, uint8_t index);
bool IsPanelButtonPressed(panel_t * panel, int16_t row, int16_t col);
void SetFocusToPanelButton(panel_t * panel, int16_t row, int16_t col);
void RemoveFocusFromAllPanelButtons(panel_t * panel);

void CloseAnyPopupMenu(void);

#endif // PANEL_H