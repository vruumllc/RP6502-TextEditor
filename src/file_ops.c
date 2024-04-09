// ---------------------------------------------------------------------------
// file_ops.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __CC65__
#include <fcntl.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include "doc.h"
#include "display.h"
#include "textbox.h"
#include "statusbar.h"
#include "panel.h"
#include "file_ops.h"

static char msg[MAX_STATUS_MSG+1] = {0};
static char buf[DOC_COLS] = {0};
static char row[DOC_COLS] = {0};

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void OpenFile()
{
    //printf("OpenFile: Filename = %s\n", TheDoc.filename);
    if (strlen(TheDoc.filename) > 0) {
        int16_t retval = EINVAL;
        uint16_t offset = 0;
        uint16_t wrapped_file_lines = 0;
        int16_t fd = open(TheDoc.filename, O_RDONLY);
        if (fd >= 0) {
            uint16_t r;
            for (r = 0; r < DOC_ROWS; r++) {
                memset(buf, 0, DOC_COLS);
                memset(row, 0, DOC_COLS);
                TheDoc.rows[r].ptxt = (void*)(DOC_MEM_START+ (r+1)*DOC_COLS);
                TheDoc.rows[r].len = 0;
                if ((retval = read(fd, buf, DOC_COLS)) > 0) {
                    bool IsWindows = false;
                    char * pch = (char*)strchr(buf, '\r');
                    if (pch != NULL) { // Windows
                        IsWindows = true;
                        *pch = '\n';
                        *(++pch) = 0;
                    } else { // linux
                        pch = (char*)strchr(buf, '\n');
                        if (pch != NULL) {
                            *(++pch) = 0;
                        } else { // line to too long!
                            memset(msg, 0, MAX_STATUS_MSG+1);
                            snprintf(msg, MAX_STATUS_MSG,
                                    "File line %u too long, so wrapped it!",
                                    r + 1 - wrapped_file_lines);
                            wrapped_file_lines++;
                            buf[DOC_COLS-2] = '\n';
                            buf[DOC_COLS-1] = 0;
                            TheDoc.dirty = true;
                            UpdateStatusBarMsg(msg, STATUS_WARNING);
                        }
                    }
                    strncpy(row, buf, DOC_COLS);
                    TheDoc.rows[r].len = strlen(row)-1; // don't count '\n'
                    //printf("r=%u, %s", r, row);
                    WriteStr(TheDoc.rows[r].ptxt, row, TheDoc.rows[r].len+1); // copy '\n' too
                    TheDoc.last_row = r;

                    // set file pointer to start of next row text
                    offset += TheDoc.rows[r].len+1+(IsWindows?1:0);
                    if (lseek(fd, offset, SEEK_SET) < 0) {
                        ReportFileError();
                    }
                } else {
                    if (retval < 0) {
                        ReportFileError();
                    }
                }
            }
            if (close(fd) < 0) {
                ReportFileError();
            }
            for (r = 0; r < TheTextbox.h; r++) {
                TheTextbox.row_dirty[r] = true;
            }
        } else {
            ReportFileError();
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static void SaveFile(bool fail_if_exists)
{
    int16_t fd;
    int16_t flags = fail_if_exists ? (O_WRONLY|O_CREAT|O_EXCL|O_TRUNC)
                                   : (O_WRONLY|O_TRUNC);
    //printf("SaveFile: Filename = %s\n", TheDoc.filename);
    fd = open(TheDoc.filename, flags);
    if (fd >= 0) {
        uint16_t r;
        char row[DOC_COLS];
        for (r = 0; r <= TheDoc.last_row; r++) {
            uint8_t len;
            memset(row, 0, DOC_COLS);
            ReadStr(TheDoc.rows[r].ptxt, row, TheDoc.rows[r].len+1);

            // make SURE line ends in '\n'
            len = strlen(row);
            if (row[len-1] != '\n' && len < DOC_COLS-1) {
                row[len] = '\n';
                row[len+1] = 0;
                TheDoc.rows[r].len = len;
                //puts("Had to add '\\n' to line end!\n");
            }

            //printf("r=%u %s", r, row);

            if (write(fd, row, TheDoc.rows[r].len+1) < 0) {
                ReportFileError();
            }
        }
        close(fd);
        TheDoc.dirty = false;
    } else {
        if (errno == 0 || errno == FR_EXIST) {
            // bug: open() always sets errno to 0
            if (errno == 0) {
                errno = FR_EXIST; // probably
            }
            // clear bad filename in doc
            memset(TheDoc.filename, 0, MAX_FILENAME+1);
        }
        ReportFileError();
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void SaveNewFile(void)
{
    SaveFile(true);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void ResaveFile(void)
{
    SaveFile(false);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void NOP(void)
{
    CloseAnyPopupMenu();
}