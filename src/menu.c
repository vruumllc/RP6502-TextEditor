// ---------------------------------------------------------------------------
// menu.c
// ---------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "colors.h"
#include "display.h"
#include "textbox.h"
#include "button.h"
#include "panel.h"
#include "actions.h"
#include "menu.h"

static panel_t * FileSubmenu = NULL;
static panel_t * EditSubmenu = NULL;
static panel_t * HelpSubmenu = NULL;

panel_t TheMainMenu;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool InitMainMenu(void)
{
    uint8_t i, c, start;
    // Init Main Menu panel parameters
    TheMainMenu.panel_type = NO_POPUP_TYPE;
    TheMainMenu.r = 0;
    TheMainMenu.c = 0;
    TheMainMenu.w = 80;
    TheMainMenu.h = 1;
    TheMainMenu.bg = DARK_BLUE;
    TheMainMenu.fg = LIGHT_GRAY;
    TheMainMenu.btn_layout = HORZ;

    // Create the buttons. We will use local routines, not actions, for ButtonPressed()
    if (AddButtonToPanel(&TheMainMenu, "File", 0, DARK_BLUE, LIGHT_GRAY, BLUE, WHITE, CYAN, NULL) &&
        AddButtonToPanel(&TheMainMenu, "Edit", 0, DARK_BLUE, LIGHT_GRAY, BLUE, WHITE, CYAN, NULL) &&
        AddButtonToPanel(&TheMainMenu, "Help", 0, DARK_BLUE, LIGHT_GRAY, BLUE, WHITE, CYAN, NULL)    ) {
    } else {
        return false;
    }

    // draw Main Menu background
    for (c = 0; c < canvas_cols(); c++) {
        DrawChar(0, c, ' ', TheMainMenu.bg, TheMainMenu.fg);
    }

    // draw the buttons
    start = 0;
    for (i = 0; i < TheMainMenu.num_btns; i++) {
        ShowButton(TheMainMenu.btn_addr[i], TheMainMenu.r, TheMainMenu.c + start);
        start += TheMainMenu.btn_addr[i]->w;
    }

    return true;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static bool InitFileSubmenu(void)
{
    FileSubmenu = NewPanel(SUBMENU, VERT, 25, 5, BLUE, WHITE );
    if (FileSubmenu != NULL) {
        if (AddButtonToPanel(FileSubmenu, "Open...          Ctrl+O", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, FileOpen) &&
            AddButtonToPanel(FileSubmenu, "Save             Ctrl+S", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, FileSave) &&
            AddButtonToPanel(FileSubmenu, "Save As... Ctrl+Shift+S", 5,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, FileSaveAs) &&
            AddButtonToPanel(FileSubmenu, "Close                  ", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, FileClose) &&
            AddButtonToPanel(FileSubmenu, "Exit             Ctrl+Q", 1,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, FileExit)) {
           return true;
        } else {
            DeletePanel(FileSubmenu);
            FileSubmenu = NULL;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static bool InitEditSubmenu(void)
{
    EditSubmenu = NewPanel(SUBMENU, VERT, 16, 3/*5*/, BLUE, WHITE );
    if (EditSubmenu != NULL) {
        if (AddButtonToPanel(EditSubmenu, "Cut     Ctrl+X", 2,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, EditCut) &&
            AddButtonToPanel(EditSubmenu, "Copy    Ctrl+C", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, EditCopy) &&
            AddButtonToPanel(EditSubmenu, "Paste   Ctrl+V", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, EditPaste) /*&&
            AddButtonToPanel(EditSubmenu, "Find    Ctrl+F", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, EditFind) &&
            AddButtonToPanel(EditSubmenu, "Replace Ctrl+H", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, EditReplace) */) {
            return true;
        } else {
            DeletePanel(EditSubmenu);
            EditSubmenu = NULL;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static bool InitHelpSubmenu(void)
{
    HelpSubmenu = NewPanel(SUBMENU, VERT, 7, 1, BLUE, WHITE );
    if (HelpSubmenu != NULL) {
        // add its buttons
        if (AddButtonToPanel(HelpSubmenu, "About", 0,
                             BLUE, WHITE, DARK_CYAN, WHITE, CYAN, HelpAbout)) {
            return true;
        } else {
            DeletePanel(HelpSubmenu);
            HelpSubmenu = NULL;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Show File Submenu
// ---------------------------------------------------------------------------
void ShowFileSubmenu(void)
{
    if (InitFileSubmenu()) {
        set_popup(FileSubmenu);
        set_popup_type(SUBMENU);
        UpdateTextboxFocus(false);
        UpdateButtonFocus(FileSubmenu->btn_addr[0], true);
        ShowPanel(FileSubmenu, 1, 1);
    }
}

// ---------------------------------------------------------------------------
// Show Edit Submenu
// ---------------------------------------------------------------------------
void ShowEditSubmenu(void)
{
    if (InitEditSubmenu()) {
        set_popup(EditSubmenu);
        set_popup_type(SUBMENU);
        UpdateTextboxFocus(false);
        UpdateButtonFocus(EditSubmenu->btn_addr[0], true);
        ShowPanel(EditSubmenu, 1, 8);
    }
}

// ---------------------------------------------------------------------------
// Show Help Submenu
// ---------------------------------------------------------------------------
void ShowHelpSubmenu(void)
{
    if (InitHelpSubmenu()) {
        set_popup(HelpSubmenu);
        set_popup_type(SUBMENU);
        UpdateTextboxFocus(false);
        UpdateButtonFocus(HelpSubmenu->btn_addr[0], true);
        ShowPanel(HelpSubmenu, 1, 8/*13*/);
    }
}


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void MainMenuButtonPressed(uint8_t index)
{
    switch (index) {
        case 0: ShowFileSubmenu(); break;
        case 1: ShowEditSubmenu(); break;
        case 2: ShowHelpSubmenu(); break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
bool IsMainMenuButtonPressed(int16_t row, int16_t col)
{
    uint8_t i;
    for (i = 0; i < TheMainMenu.num_btns; i++) {
        button_t * btn = TheMainMenu.btn_addr[i];
        if (btn != NULL) {
            if (row == btn->r &&
                col >= btn->c && col < (btn->c + btn->w)) {
                MainMenuButtonPressed(i);
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t SubmenuShowing(void)
{
    uint8_t i;
    for (i = 0; i < TheMainMenu.num_btns; i++) {
        if (TheMainMenu.btn_addr[i]->in_focus) {
            return i+1;
        }
    }
    return 0;
}