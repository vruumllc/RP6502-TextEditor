// ---------------------------------------------------------------------------
// actions.h
// ---------------------------------------------------------------------------

#ifndef ACTIONS_H
#define ACTIONS_H

#include <stdint.h>
#include <stdbool.h>

void FileOpen(void);
void OpenFile(void);
void FileSave(void);
void FileSaveAs(void);
void SaveFileAs(void);
void FileClose(void);

void FileExit(void);
void FileExitYes(void);
/*
void EditCut(void);
void EditCopy(void);
void EditPaste(void);
void EditFind(void);
void EditReplace(void);
*/
void HelpAbout(void);

void NOP(void);

#endif // ACTIONS_H
