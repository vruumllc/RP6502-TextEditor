// ---------------------------------------------------------------------------
// doc.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "display.h"
#include "doc.h"

// for the main window's document textbox, starting at 0x1130
static doc_row_t doc_rows[DOC_ROWS]; // array of pointers to extended memory

static doc_t TheDoc = {
    0, // cur_filename_r
    0, // cur_filename_c
    0, // cursor_r
    0, // cursor_c
    0, // offset_r
    0, // last_row
    0, // last_col
    false, // dirty
    {0}, //filename
    doc_rows
}; // the one and only

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
doc_t * GetDoc(void)
{
    return &TheDoc;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void ClearDoc(bool save_filename)
{
    uint16_t r;
    uint8_t c;
    TheDoc.cur_filename_r = 0;
    TheDoc.cur_filename_c = 0;
    TheDoc.cursor_r = 0;
    TheDoc.cursor_c = 0;
    TheDoc.offset_r = 0;
    TheDoc.last_row = 0;
    TheDoc.last_col = 0;
    TheDoc.dirty = false;
    if (!save_filename) {
        memset(TheDoc.filename, 0, MAX_FILENAME+1);
    }
    TheDoc.rows = doc_rows;
    for (r = 0 ; r < DOC_ROWS; r++) {
        TheDoc.rows[r].ptxt = (void*)(DOC_MEM_START + sizeof(uint8_t)*(DOC_COLS*(r+1)));
        TheDoc.rows[r].len = 0;
        RIA.addr0 = (uint16_t)TheDoc.rows[r].ptxt;
        RIA.step0 = 1;
        for (c = 0; c < DOC_COLS; c++) {
            RIA.rw0 = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// copy extended mem row to local working buffer
// ---------------------------------------------------------------------------
bool ReadStr(void * addr, char * str, uint8_t len)
{
    uint8_t c;
    if (str != NULL) {
        RIA.addr0 = (uint16_t)addr;
        RIA.step0 = 1;
        for (c = 0; c < len; c++) {
            str[c] = RIA.rw0;
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// copy local working buffer to extended memory
// ---------------------------------------------------------------------------
bool WriteStr(void * addr, char * str, uint8_t len)
{
    uint8_t c;
    if (str != NULL) {
        RIA.addr0 = (uint16_t)addr;
        RIA.step0 = 1;
        for (c = 0; c < len; c++) {
            RIA.rw0 = str[c];
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Try to add ASCII char to doc, shifting data if necessary
// ---------------------------------------------------------------------------
bool AddChar(char chr, bool insert_mode)
{
    if (chr != 0) {
        // is there room to add another char?
        if ((!insert_mode && TheDoc.cursor_c < DOC_COLS-1) ||
            (TheDoc.rows[TheDoc.cursor_r].len+1 < DOC_COLS)) {
            char row[DOC_COLS] = {0};
            int16_t cur_r = TheDoc.cursor_r;
            ReadStr(TheDoc.rows[cur_r].ptxt, row, TheDoc.rows[cur_r].len+1);
            // if INSERT mode, need to shift chars right if not at line end
            if (insert_mode) {
                int16_t n = ((TheDoc.rows[cur_r].len) -
                                (TheDoc.cursor_c)*sizeof(char));
                if (n > 0) {
                    memmove(row+TheDoc.cursor_c+1, row+TheDoc.cursor_c, n);
                }
                row[TheDoc.cursor_c++] = chr; //OK, now insert new char
                row[++TheDoc.rows[cur_r].len] = '\n'; // just making sure
            } else { // just overwrite what is there, if anything
                row[TheDoc.cursor_c++] = chr;
                if (TheDoc.rows[cur_r].len < TheDoc.cursor_c) {
                    TheDoc.rows[cur_r].len = TheDoc.cursor_c;
                }
                row[TheDoc.rows[cur_r].len] = '\n'; // just making sure
            }
            WriteStr(TheDoc.rows[cur_r].ptxt, row, DOC_COLS);
            if (TheDoc.last_row < cur_r) {
                TheDoc.last_row = cur_r;
            }
            if (TheDoc.last_col < TheDoc.rows[cur_r].len) {
                TheDoc.last_col = TheDoc.rows[cur_r].len;
            }
            TheDoc.dirty = true;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Delete ASCII char from doc, shifting data if necessary
// ---------------------------------------------------------------------------
bool DeleteChar(bool backspace)
{
    bool retval = false;
        char row[DOC_COLS] = {0};
        char target_row[DOC_COLS] = {0};
        int16_t cur_r = TheDoc.cursor_r;
        int16_t cur_c = TheDoc.cursor_c;
        ReadStr(TheDoc.rows[cur_r].ptxt, row, TheDoc.rows[cur_r].len); // no '\n'
        if (backspace) { // delete char to left of cursor (if one), then ...
            if (TheDoc.cursor_c > 0) { // ... just shift remaining text left
                memmove(row + cur_c-1, row + cur_c, DOC_COLS - cur_c-1);
                TheDoc.cursor_c--;
                TheDoc.rows[cur_r].len--;
                WriteStr(TheDoc.rows[TheDoc.cursor_r].ptxt, row, DOC_COLS);
                TheDoc.dirty = true;
                retval = true;
            } else { // ... at row start, so append current row to row above and delete current row
                uint8_t target_row_len = TheDoc.rows[cur_r-1].len;
                if (cur_r != 0 && AppendString(row, cur_r-1)) {
                    retval = DeleteRow(cur_r);
                    TheDoc.cursor_r = cur_r-1;
                    TheDoc.cursor_c = target_row_len;
                }
            }
        } else { // delete char at cursor (if one), then ...
            if (TheDoc.cursor_c < TheDoc.rows[cur_r].len) { // ... just shift remaining text left
                memmove(row + cur_c, row + cur_c+1, DOC_COLS - cur_c);
                TheDoc.rows[cur_r].len--;
                WriteStr(TheDoc.rows[TheDoc.cursor_r].ptxt, row, DOC_COLS);
                TheDoc.dirty = true;
                retval = true;
            } else { // ... at row end, so append row below to current row, and delete row below
                ReadStr(TheDoc.rows[cur_r+1].ptxt, target_row, TheDoc.rows[cur_r+1].len);
                if (AppendString(target_row, cur_r)) {
                    retval = DeleteRow(cur_r+1);
                }
            }
        }
    return retval;
}

// ---------------------------------------------------------------------------
// Handle CR (newline) in doc, splitting text if necessary
// ---------------------------------------------------------------------------
bool AddNewLine(void)
{
    char row[DOC_COLS] = {0};
    char new_row[DOC_COLS] = {0};
    if (AddRow(TheDoc.cursor_r)) {
        uint16_t cur_r = TheDoc.cursor_r;
        uint16_t cur_c = TheDoc.cursor_c;
        // Move the relavent part of the current row to the new row:
        // copy current row from extended ram
        memset(row, 0, DOC_COLS);
        ReadStr(TheDoc.rows[cur_r].ptxt, row, TheDoc.rows[cur_r].len+1);

        // copy the text after the cursor to the new line
        memcpy(new_row, row + cur_c, TheDoc.rows[cur_r].len - cur_c + 1);
        WriteStr(TheDoc.rows[cur_r+1].ptxt, new_row, DOC_COLS);
        TheDoc.rows[cur_r+1].len = TheDoc.rows[cur_r].len - cur_c;

        // clear the text after the cursor on the current line
        memset(row + cur_c, 0, DOC_COLS - cur_c);
        row[cur_c] = '\n';

        WriteStr(TheDoc.rows[cur_r].ptxt, row, DOC_COLS);
        TheDoc.rows[cur_r].len = cur_c;

        // finally, position the cursor at the beginning of the new line
        TheDoc.cursor_r++;
        TheDoc.cursor_c = 0;
        if (TheDoc.cursor_r >= TheDoc.offset_r + (canvas_rows()-2)) {
            TheDoc.offset_r++;
        }
        TheDoc.dirty = true;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Adds a row below row_index to doc, then
// copies all row data (including row[row_index]) to next row.
// ---------------------------------------------------------------------------
bool AddRow(uint16_t row_index)
{
    if (row_index < DOC_ROWS-1) {
        if (TheDoc.last_row+1 < DOC_ROWS) { // check if new row is OK
            int16_t r;
            char row[DOC_COLS] = {0};
            // move all rows below current row down 1
            for (r = (int16_t)TheDoc.last_row; r >= (int16_t)row_index; r--) {
                memset(row, 0, DOC_COLS);
                ReadStr(TheDoc.rows[r].ptxt, row, TheDoc.rows[r].len+1);
                WriteStr(TheDoc.rows[r+1].ptxt, row, DOC_COLS);
                TheDoc.rows[r+1].len = TheDoc.rows[r].len;
            }
            TheDoc.last_row++;
            TheDoc.dirty = true;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Deletes row[row_index] in doc, by shifting up data from rows below.
// ---------------------------------------------------------------------------
bool DeleteRow(uint16_t row_index)
{
    if (row_index < DOC_ROWS-1 && row_index <= TheDoc.last_row)  {
        int16_t r;
        char row[DOC_COLS] = {0};
        // move all rows below current row up 1
        for (r = (int16_t)row_index; r <= (int16_t)TheDoc.last_row; r++) {
            memset(row, 0, DOC_COLS);
            ReadStr(TheDoc.rows[r+1].ptxt, row, TheDoc.rows[r+1].len+1);
            WriteStr(TheDoc.rows[r].ptxt, row, DOC_COLS);
            TheDoc.rows[r].len = TheDoc.rows[r+1].len;
        }
        // clear previous last row
        memset(row, 0, DOC_COLS);
        WriteStr(TheDoc.rows[TheDoc.last_row].ptxt, row, DOC_COLS);
        TheDoc.rows[TheDoc.last_row].len = 0;
        TheDoc.last_row--;
        TheDoc.dirty = true;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Append a string to row[row_index], if the result isn't too long
// NOTE: the str should have no tailing '\n'
// ---------------------------------------------------------------------------
bool AppendString(char * str, uint16_t row_index)
{
    if (str != NULL && row_index < DOC_ROWS-1) {
        uint8_t len_str = strlen(str);
        if (len_str == 0) {
            return true; // nothing to do
        } else {
            uint8_t len_row = TheDoc.rows[row_index].len;
            uint16_t len_result = len_row + len_str;
            if (len_result < DOC_COLS) { // fits?
                char row[DOC_COLS] = {0};
                ReadStr(TheDoc.rows[row_index].ptxt, row, TheDoc.rows[row_index].len);
                strncat(row, str, DOC_COLS-TheDoc.rows[row_index].len);
                row[len_result] = '\n';
                WriteStr(TheDoc.rows[row_index].ptxt, row, DOC_COLS);
                TheDoc.rows[row_index].len = len_result;
                if (TheDoc.last_col < TheDoc.rows[row_index].len) {
                    TheDoc.last_col = TheDoc.rows[row_index].len;
                }
                TheDoc.dirty = true;
                return true;
            }
        }
    }
    return false;
}