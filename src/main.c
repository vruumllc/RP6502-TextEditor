// ---------------------------------------------------------------------------
// main.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ezpsg.h"
#include "doc.h"
#include "display.h"
#include "textbox.h"
#include "statusbar.h"
#include "menu.h"
#include "keyboard.h"
#include "mouse.h"

// canvas_data = 0x0000 to 0x12BF (display.c)
// DOC buffers = 0x1300 to 0xEF4F (doc.h)
#define MUSIC_CONFIG 0xFE00 // to 0xFE39 (requires 0x40 bytes mem)
// mouse_data = 0xFE60 to 0xFEC3 (mouse.c)
// canvas_struct = 0xFF00 to 0xFF0F (display.c)
// keybd_status = 0xFF20 to 0xFF3F (keyboard.c)
// mouse_state = 0xFF40 to 0xFF44 (mouse.c)
// mouse_struct = 0xFF50 to 0xFF5F (mouse.c)


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
int main(void)
{
    uint8_t v; // vsync counter, incements every 1/60 second, rolls over every 256

    ezpsg_init(MUSIC_CONFIG);
    InitDisplay();
    InitStatusBar();
    InitTextbox();
    InitKeyboard();
    InitMouse();
    if (InitMainMenu()) {
        // vsync loop
        v = RIA.vsync;
        while(true) {
            if (v == RIA.vsync) {
                continue; // wait until vsync is incremented
            } else {
                v = RIA.vsync; // new value for v
            }

            ezpsg_tick(1); // play sound, if started

            // clear the statusbar msg after 10 seconds
            incr_status_timer();
            if (get_status_timer() > 600) {
                UpdateStatusBarMsg("", STATUS_INFO); // resets timer, too
            }

            UpdateTextbox();

            // break out of loop if something returns false
            if (!HandleKeys() || !HandleMouse()) {
                break;
            }
        }
    }
}
