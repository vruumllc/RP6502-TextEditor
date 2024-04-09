// ---------------------------------------------------------------------------
// menu.h
// ---------------------------------------------------------------------------

#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "panel.h"

extern panel_t TheMainMenu;

bool InitMainMenu(void);

void MainMenuButtonPressed(uint8_t index);
uint8_t SubmenuShowing(void);

void ShowFileSubmenu(void);
void ShowEditSubmenu(void);
void ShowHelpSubmenu(void);

bool IsMainMenuButtonPressed(int16_t row, int16_t col);

#endif // MENU_H