// ---------------------------------------------------------------------------
// doc.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "doc.h"

// for the file dialog single row textbox, at 0x11300
static doc_row_t dlg_row; // single row

// for the main window's document textbox, starting at 0x11380
static doc_row_t doc_rows[DOC_ROWS]; // array of pointers to extended memory

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
bool InitDoc(doc_t * doc, bool for_dlg)
{
    if (doc != NULL) {
        doc->cursor_r = 0;
        doc->cursor_c = 0;
        doc->offset_r = 0;
        doc->last_row = 0;
        doc->last_col = 0;
        doc->dirty = false;
        memset(doc->filename, 0, sizeof(char)*MAX_FILENAME+1);
        if (for_dlg) {
            char row[DOC_COLS] = {0};
            doc->rows = &dlg_row;
            doc->rows->ptxt = (void *)(DOC_MEM_START - sizeof(uint8_t)*DOC_COLS);
            doc->rows->len = 0;
            doc->rows->dirty = false;
            RIA.addr0 = (uint16_t)doc->rows[0].ptxt;
            RIA.step0 = 1;
            for (uint8_t c = 0; c < DOC_COLS; c++) {
                RIA.rw0 = 0;
            }
        } else {
            doc->rows = doc_rows;
            for (uint16_t r = 0; r < DOC_ROWS; r++) {
                doc->rows[r].ptxt = (void*)(DOC_MEM_START + sizeof(uint8_t)*(DOC_COLS*r));
                doc->rows[r].len = 0;
                doc->rows[r].dirty = true;
                RIA.addr0 = (uint16_t)doc->rows[r].ptxt;
                RIA.step0 = 1;
                for (uint8_t c = 0; c < DOC_COLS; c++) {
                    RIA.rw0 = 0;
                }
            }
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// copy extended mem row to local working buffer
// ---------------------------------------------------------------------------
bool ReadStr(void * addr, char * str, uint8_t len)
{
    if (str != NULL) {
        RIA.addr0 = (uint16_t)addr;
        RIA.step0 = 1;
        for (uint8_t c = 0; c < len; c++) {
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
    if (str != NULL) {
        RIA.addr0 = (uint16_t)addr;
        RIA.step0 = 1;
        for (uint8_t c = 0; c < len; c++) {
            RIA.rw0 = str[c];
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Try to add ASCII char to doc, shifting data if necessary
// ---------------------------------------------------------------------------
bool AddChar(doc_t * doc, char chr, bool insert_mode, uint8_t max_cols)
{
    bool retval = false;
    if (doc != NULL) {
        if (chr != 0) {
            // is there room to add another char?
            if ((!insert_mode && doc->cursor_c < (max_cols-1)) ||
                (doc->rows[doc->cursor_r].len < (max_cols-1))) {
                char row[DOC_COLS] = {0};
                int16_t cur_r = doc->cursor_r;
                ReadStr(doc->rows[cur_r].ptxt, row, doc->rows[cur_r].len+1);
                // if INSERT mode, need to shift chars right if not at line end
                if (insert_mode) {
                    int16_t n = ((doc->rows[cur_r].len) -
                                 (doc->cursor_c)*sizeof(char));
                    if (n > 0) {
                        memmove(row+doc->cursor_c+1, row+doc->cursor_c, n);
                    }
                    row[doc->cursor_c++] = chr; //OK, now insert new char
                    row[++doc->rows[cur_r].len] = '\n'; // just making sure
                } else { // just overwrite what is there, if anything
                    row[doc->cursor_c++] = chr;
                    if (doc->rows[cur_r].len < doc->cursor_c) {
                        doc->rows[cur_r].len = doc->cursor_c;
                    }
                    row[doc->rows[cur_r].len] = '\n'; // just making sure
                }
                WriteStr(doc->rows[cur_r].ptxt, row, DOC_COLS);
                if (doc->last_row < cur_r) {
                    doc->last_row = cur_r;
                }
                if (doc->last_col < doc->rows[cur_r].len) {
                    doc->last_col = doc->rows[cur_r].len;
                }
                doc->rows[cur_r].dirty = true;
                doc->dirty = true;
                retval = true;
            }
        }
    }
    return retval;
}

// ---------------------------------------------------------------------------
// Delete ASCII char from doc, shifting data if necessary
// ---------------------------------------------------------------------------
bool DeleteChar(doc_t * doc, bool backspace)
{
    bool retval = false;
    if (doc != NULL) {
        char row[DOC_COLS] = {0};
        char target_row[DOC_COLS] = {0};
        int16_t cur_r = doc->cursor_r;
        int16_t cur_c = doc->cursor_c;
        ReadStr(doc->rows[cur_r].ptxt, row, doc->rows[cur_r].len); // no '\n'
        if (backspace) { // delete char to left of cursor (if one), then ...
            if (doc->cursor_c > 0) { // ... just shift remaining text left
                memmove(row + cur_c-1, row + cur_c, DOC_COLS - cur_c-1);
                doc->cursor_c--;
                doc->rows[cur_r].len--;
                WriteStr(doc->rows[doc->cursor_r].ptxt, row, DOC_COLS);
                doc->rows[doc->cursor_r].dirty = true;
                doc->dirty = true;
                retval = true;
            } else { // ... at row start, so append current row to row above and delete current row
                uint8_t target_row_len = doc->rows[cur_r-1].len;
                if (cur_r != 0 && AppendString(doc, row, cur_r-1)) {
                    retval = DeleteRow(doc, cur_r);
                    doc->cursor_r = cur_r-1;
                    doc->cursor_c = target_row_len;
                }
            }
        } else { // delete char at cursor (if one), then ...
            if (doc->cursor_c < doc->rows[cur_r].len) { // ... just shift remaining text left
                memmove(row + cur_c, row + cur_c+1, DOC_COLS - cur_c);
                doc->rows[cur_r].len--;
                WriteStr(doc->rows[doc->cursor_r].ptxt, row, DOC_COLS);
                doc->rows[doc->cursor_r].dirty = true;
                doc->dirty = true;
                retval = true;
            } else { // ... at row end, so append row below to current row, and delete row below
                ReadStr(doc->rows[cur_r+1].ptxt, target_row, doc->rows[cur_r+1].len);
                if (AppendString(doc, target_row, cur_r)) {
                    retval = DeleteRow(doc, cur_r+1);
                }
            }
        }
    }
    return retval;
}

// ---------------------------------------------------------------------------
// Handle CR (newline) in doc, splitting text if necessary
// ---------------------------------------------------------------------------
bool AddNewLine(doc_t * doc)
{
    bool retval = false;
    char row[DOC_COLS] = {0};
    char new_row[DOC_COLS] = {0};

    if (AddRow(doc, doc->cursor_r)) {
        // Move the relavent part of the current row to the new row:
        // copy current row from extended ram
        memset(row, 0, DOC_COLS);
        uint16_t cur_r = doc->cursor_r;
        uint16_t cur_c = doc->cursor_c;
        ReadStr(doc->rows[cur_r].ptxt, row, doc->rows[cur_r].len+1);

        // copy the text after the cursor to the new line
        memcpy(new_row, row + cur_c, doc->rows[cur_r].len - cur_c + 1);
        WriteStr(doc->rows[cur_r+1].ptxt, new_row, DOC_COLS);
        doc->rows[cur_r+1].len = doc->rows[cur_r].len - cur_c;
        doc->rows[cur_r+1].dirty = true;

        // clear the text after the cursor on the current line
        memset(row + cur_c, 0, DOC_COLS - cur_c);
        row[cur_c] = '\n';

        WriteStr(doc->rows[cur_r].ptxt, row, DOC_COLS);
        doc->rows[cur_r].len = cur_c;
        doc->rows[cur_r].dirty = true;

        // finally, position the cursor at the beginning of the new line
        doc->cursor_r++;
        doc->cursor_c = 0;
        doc->dirty = true;
        return true;
    }
    return retval;
}

// ---------------------------------------------------------------------------
// Adds a row below row_index to doc, then
// copies all row data (including row[row_index]) to next row.
// ---------------------------------------------------------------------------
bool AddRow(doc_t * doc, uint16_t row_index)
{
    bool retval = false;
    if (doc != NULL && row_index < DOC_ROWS-1) {
        if (doc->last_row+1 < DOC_ROWS) { // check if new row is OK
            char row[DOC_COLS] = {0};
            // move all rows below current row down 1
            for (int16_t r = (int16_t)doc->last_row; r >= (int16_t)row_index; r--) {
                memset(row, 0, DOC_COLS);
                ReadStr(doc->rows[r].ptxt, row, doc->rows[r].len+1);
                WriteStr(doc->rows[r+1].ptxt, row, DOC_COLS);
                doc->rows[r+1].len = doc->rows[r].len;
                doc->rows[r+1].dirty = true;
            }
            doc->last_row++;
            doc->dirty = true;
            return true;
        }
    }
    return retval;
}

// ---------------------------------------------------------------------------
// Deletes row[row_index] in doc, by shifting up data from rows below.
// ---------------------------------------------------------------------------
bool DeleteRow(doc_t * doc, uint16_t row_index)
{
    bool retval = false;
    if (doc != NULL && row_index < DOC_ROWS-1 && row_index <= doc->last_row)  {
        char row[DOC_COLS] = {0};
        // move all rows below current row up 1
        for (int16_t r = (int16_t)row_index; r <= (int16_t)doc->last_row; r++) {
            memset(row, 0, DOC_COLS);
            ReadStr(doc->rows[r+1].ptxt, row, doc->rows[r+1].len+1);
            WriteStr(doc->rows[r].ptxt, row, DOC_COLS);
            doc->rows[r].len = doc->rows[r+1].len;
            doc->rows[r].dirty = true;
        }
        // clear previous last row
        memset(row, 0, DOC_COLS);
        WriteStr(doc->rows[doc->last_row].ptxt, row, DOC_COLS);
        doc->rows[doc->last_row].len = 0;
        doc->rows[doc->last_row].dirty = true;
        doc->last_row--;
        doc->dirty = true;
        return true;
    }
    return retval;
}

// ---------------------------------------------------------------------------
// Append a string to row[row_index], if the result isn't too long
// NOTE: the str should have no tailing '\n'
// ---------------------------------------------------------------------------
bool AppendString(doc_t * doc, char * str, uint16_t row_index)
{
    bool retval = false;
    if (doc != NULL && str != NULL && row_index < DOC_ROWS-1) {
        uint8_t len_str = strlen(str);
        if (len_str == 0) {
            retval = true; // nothing to do
        } else {
            uint8_t len_row = doc->rows[row_index].len;
            uint16_t len_result = len_row + len_str;
            if (len_result < DOC_COLS) { // fits?
                char row[DOC_COLS] = {0};
                ReadStr(doc->rows[row_index].ptxt, row, doc->rows[row_index].len);
                strncat(row, str, DOC_COLS-doc->rows[row_index].len);
                row[len_result] = '\n';
                WriteStr(doc->rows[row_index].ptxt, row, DOC_COLS);
                doc->rows[row_index].len = len_result;
                doc->rows[row_index].dirty = true;
                if (doc->last_col < doc->rows[row_index].len) {
                    doc->last_col = doc->rows[row_index].len;
                }
                doc->dirty = true;
                retval = true;
            }
        }
    }
    return retval;
}