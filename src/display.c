// ---------------------------------------------------------------------------
// display.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
//#include <stdio.h>
#include "colors.h"
#include "display.h"

static uint16_t canvas_struct = 0xFF00;
static uint16_t canvas_data = 0x0000;
static uint8_t plane = 0;
static uint8_t canvas_type = 3; // 640x480
static uint16_t canvas_w = 640;
static uint16_t canvas_h = 480;
static uint8_t canvas_c = 80;
static uint8_t canvas_r = 30;
static uint8_t font_w = 8;
static uint8_t font_h = 16;
static int8_t bpp = 4;
static uint8_t bg_clr = BLACK;
static uint8_t fg_clr = LIGHT_GRAY;

// ---------------------------------------------------------------------------
// canvas_type = 3 (640x480), font_opt = 1 (8x16), bpp_opt = 2 (4bpp),
// plane = 0, canvas_struct = 0xFF00, canvas_data = 0x000
// ---------------------------------------------------------------------------
void InitDisplay(void)
{
    uint8_t x_offset = 0;
    uint8_t y_offset = 0;
    uint8_t font_bpp_opt = 10; // 8x16 font at 4bpp

    // initialize the canvas
    xregn(1, 0, 0, 1, canvas_type);

    xram0_struct_set(canvas_struct, vga_mode1_config_t, x_wrap, false);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, y_wrap, false);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, x_pos_px, x_offset);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, y_pos_px, y_offset);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, width_chars, canvas_c);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, height_chars, canvas_r);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, xram_data_ptr, canvas_data);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, xram_palette_ptr, 0xFFFF);
    xram0_struct_set(canvas_struct, vga_mode1_config_t, xram_font_ptr, 0xFFFF);

    xregn(1, 0, 1, 4, 1, font_bpp_opt, canvas_struct, plane);

    ClearDisplay(bg_clr, fg_clr);
}

// ----------------------------------------------------------------------------
// Changes TxDisplay bg_clr and fg_clr, then overwrites display using them.
// Doesn't clear top or bottom rows, as these are for menu and status bar.
// ----------------------------------------------------------------------------
void ClearDisplay(uint8_t bg, uint8_t fg)
{
    uint8_t r, c;

    bg_clr = bg;
    fg_clr = fg;

    RIA.addr0 = canvas_data;
    RIA.step0 = 1;

    for (r = 0; r < canvas_r; r++) {
        for (c = 0; c < canvas_c; c++) {
            DrawChar(r, c, ' ', bg, fg);
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void DrawChar(uint8_t row, uint8_t col, char ch, uint8_t bg, uint8_t fg)
{
    // for 4-bit color, index 2 bytes per ch
    RIA.addr0 = canvas_data + 2*(row*canvas_c + col);
    RIA.step0 = 1;
    RIA.rw0 = ch;
    RIA.rw0 = (bg<<4) | fg;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void GetChar(uint8_t row, uint8_t col, char * pch, uint8_t * pbg, uint8_t *pfg)
{
     uint8_t bgfg;

    // for 4-bit color, index 2 bytes per ch
    RIA.addr0 = canvas_data + 2*(row*canvas_c + col);
    RIA.step0 = 1;
    *pch = RIA.rw0;
    bgfg = RIA.rw0;
    *pbg = bgfg >> 4;
    *pfg = bgfg & 0x0F;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool BackupChars(uint8_t row, uint8_t col, uint8_t width, uint8_t height, uint8_t * pstash)
{
    if (pstash != NULL) {
        uint8_t * pbyte = pstash;
        uint16_t r, c;
        for (r = row; r < row + height; r++) {
            for (c = col; c < col + width; c++) {
                // for 4-bit color, index 2 bytes per ch
                RIA.addr0 = canvas_data + 2*(r*canvas_c + c);
                RIA.step0 = 1;
                *(pbyte++) = RIA.rw0; // ch
                *(pbyte++) = RIA.rw0; // bgfg
            }
        }
        return true; // backup succeeded
    }
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool RestoreChars(uint8_t row, uint8_t col, uint8_t width, uint8_t height, uint8_t * pstash)
{
    if (pstash != NULL) {
        uint8_t * pbyte = pstash;
        uint16_t r, c;
        for (r = row; r < row + height; r++) {
            for (c = col; c < col + width; c++) {
                // for 4-bit color, index 2 bytes per ch
                RIA.addr0 = canvas_data + 2*(r*canvas_c + c);
                RIA.step0 = 1;
                RIA.rw0 = *(pbyte++); // ch
                RIA.rw0 = *(pbyte++); // bgfg
            }
        }
        return true; // restore suceeded
    }
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t canvas_struct_address(void)
{
    return canvas_struct;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t canvas_data_address(void)
{
    return canvas_data;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t  canvas_opt (void)
{
    return canvas_type;}
;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t canvas_width(void)
{
    return canvas_w;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t canvas_height(void)
{
    return canvas_h;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t canvas_rows(void)
{
    return canvas_r;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t canvas_cols(void)
{
    return canvas_c;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t font_width(void)
{
    return font_w;
};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t font_height(void) {
    return font_h;
};
