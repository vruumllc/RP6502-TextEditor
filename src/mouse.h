// ---------------------------------------------------------------------------
// mouse.h
// ---------------------------------------------------------------------------

#ifndef MOUSE_H
#define MOUSE_H

#include <stdbool.h>
#include <stdint.h>

void InitMouse(void);
bool HandleMouse(void);

uint16_t mouse_x(void);
uint16_t mouse_y(void);
uint8_t mouse_row(void);
uint8_t mouse_col(void);

#endif // MOUSE_H