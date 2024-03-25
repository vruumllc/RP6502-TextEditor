// ---------------------------------------------------------------------------
// doc.h
// ---------------------------------------------------------------------------

#ifndef DOC_H
#define DOC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// doc uses extended mem 0x11300 to 0x1FAFF
#define DOC_MEM_START 0x1300
#define DOC_MEM_SIZE 0xE800 // 58k
#define DOC_COLS 0x50 // 80
#define DOC_ROWS 0x2E6 // 742

#define MAX_FILENAME 31

typedef struct doc_row {
    void * ptxt; // address of (extended) memory for row data
    uint8_t len; // number of valid chars in row, including '\n'
    bool dirty; // if row needs redrawing
} doc_row_t;

typedef struct doc {
    uint8_t cur_filename_r; // cursor row in filename
    uint8_t cur_filename_c; // cursor col in filename
    uint16_t cursor_r; // cursor row in document
    uint16_t cursor_c; // cursor col in document
    uint16_t offset_r; // offset to row displayed
    uint16_t last_row; // last row used for doc
    uint16_t last_col; // last col used for doc
    bool dirty; // true if doc needs to be saved
    char filename[MAX_FILENAME+1];
    doc_row_t * rows; // DOC_MEM_START
} doc_t;

doc_t * GetDoc(void);
void ClearDoc(bool save_filename);
bool ReadStr(void * addr, char * str, uint8_t len);
bool WriteStr(void * addr, char * str, uint8_t len);
bool AddChar(char chr, bool insert_mode);
bool DeleteChar(bool backspace);
bool AddNewLine(void);
bool AppendString(char * str, uint16_t row_index);
bool AddRow(uint16_t row_index);
bool DeleteRow(uint16_t row_index);

#endif // DOC_H