// ---------------------------------------------------------------------------
// doc.h
// ---------------------------------------------------------------------------

#ifndef DOC_H
#define DOC_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// dlg uses extended mem 0x11300 to 0x1134F
// doc uses extended mem 0x11350 to 0x1EF4F
#define DOC_MEM_START 0x1350
#define DOC_MEM_SIZE 0xDC00 // 55k
#define DOC_COLS 0x50 // 80
#define DOC_ROWS 0x2C0 // 704

#define MAX_FILENAME 30

typedef struct doc_row {
    void * ptxt; // address of (extended) memory for row data
    uint8_t len; // number of valid chars in row, including '\n'
    bool dirty; // if row needs redrawing
} doc_row_t;

typedef struct doc {
    uint16_t cursor_r; // cursor row in document
    uint16_t cursor_c; // cursor col in document
    uint16_t offset_r; // offset to row displayed
    uint16_t last_row; // last row used for doc
    uint16_t last_col; // last col used for doc
    bool dirty; // true if doc needs to be saved
    char filename[MAX_FILENAME+1];
    doc_row_t * rows;
} doc_t;

bool InitDoc(doc_t * doc, bool for_dlg);
bool ReadStr(void * addr, char * str, uint8_t len);
bool WriteStr(void * addr, char * str, uint8_t len);
bool AddChar(doc_t * doc, char chr, bool insert_mode, uint8_t max_cols);
bool DeleteChar(doc_t * doc, bool backspace);
bool AddNewLine(doc_t * doc);
bool AppendString(doc_t * doc, char * str, uint16_t row_index);
bool AddRow(doc_t * doc, uint16_t row_index);
bool DeleteRow(doc_t * doc, uint16_t row_index);

#endif // DOC_H