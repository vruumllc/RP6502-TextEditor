// ---------------------------------------------------------------------------
// display.h
// ---------------------------------------------------------------------------

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "colors.h"

typedef enum {NO_POPUP_TYPE, MSGDIALOG, FILEDIALOG, SUBMENU, CONTEXTMENU} popup_type_t;

void InitDisplay(void);
void ClearDisplay(uint8_t display_bg, uint8_t display_fg);
void DrawChar(uint8_t row, uint8_t col, char ch, uint8_t bg, uint8_t fg);
void GetChar(uint8_t row, uint8_t col, char * pch, uint8_t *pbg, uint8_t * pfg);
bool BackupChars(uint8_t row, uint8_t col, uint8_t width, uint8_t height, uint8_t * pstash);
bool RestoreChars(uint8_t row, uint8_t col, uint8_t width, uint8_t height, uint8_t * pstash);

uint16_t canvas_struct_address(void);
uint16_t canvas_data_address(void);
uint8_t  canvas_opt (void);
uint16_t canvas_width(void);
uint16_t canvas_height(void);
uint8_t canvas_rows(void);
uint8_t canvas_cols(void);
uint8_t font_width(void);
uint8_t font_height(void);

#endif // DISPLAY_H