// ---------------------------------------------------------------------------
// statusbar.c
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "ezpsg.h"
#include "colors.h"
#include "display.h"
#include "textbox.h"
#include "statusbar.h"

#define MAX_CUR_POS 18

static uint16_t row = 29;
static uint16_t col = 0;
static uint8_t w = 80;
static uint8_t h = 1;
static uint8_t bg = DARK_BLUE;
static uint8_t fg = LIGHT_GRAY;
static uint8_t fg_info = LIGHT_GRAY;
static uint8_t fg_warning = YELLOW;
static uint8_t fg_error = RED;

static char msg[MAX_STATUS_MSG+1] = {0};
static char pos[MAX_CUR_POS+1] = {0};

#define wait(duration) (duration)
#define trumpet(note, duration) (-1), (note), (duration)
#define end() (0)

static const uint8_t beep_info[] = {trumpet(g4, 1), wait(1), end()};
static const uint8_t beep_warning[] = {trumpet(g3, 1), wait(3),
                                       trumpet(g3, 1), wait(3), end()};
static const uint8_t beep_error[] = {trumpet(g2, 1), wait(3),
                                     trumpet(g2, 1), wait(3),
                                     trumpet(g2, 1), wait(3),end()};

static uint16_t status_timer = 0;

// ---------------------------------------------------------------------------
/*
FROM errno.h: ----------------------------------------------------------------
#define ERANGE 1
#define EDOM 2
#define EILSEQ 3
#define EINVAL 4            // no error, operation succeeded
#define ENOMEM 5
#define ELAST 5

FROM rp6502.h, and ff.h (FAT filesys library): -------------------------------
typedef enum {
  FR_OK = 32,               // 32: Succeeded
  FR_DISK_ERR,              // 33: A hard error occurred in the low level disk I/O layer
  FR_INT_ERR,               // 34: Assertion failed
  FR_NOT_READY,             // 35: The physical drive cannot work
  FR_NO_FILE,               // 36: Could not find the file
  FR_NO_PATH,               // 36: Could not find the path
  FR_INVALID_NAME,          // 37: The path name format is invalid
  FR_DENIED,                // 38: Access denied due to prohibited access or directory full
  FR_EXIST,                 // Access denied due to prohibited access
  FR_INVALID_OBJECT,        // The file/directory object is invalid
  FR_WRITE_PROTECTED,       // The physical drive is write protected
  FR_INVALID_DRIVE,         // The logical drive number is invalid
  FR_NOT_ENABLED,           // The volume has no work area
  FR_NO_FILESYSTEM,         // There is no valid FAT volume
  FR_MKFS_ABORTED,          // The f_mkfs() aborted due to any problem
  FR_TIMEOUT,               // Could not get a grant to access the volume within defined period
  FR_LOCKED,                // The operation is rejected according to the file sharing policy
  FR_NOT_ENOUGH_CORE,       // LFN working buffer could not be allocated
  FR_TOO_MANY_OPEN_FILES,   // Number of open files > FF_FS_LOCK
  FR_INVALID_PARAMETER      // Given parameter is invalid
} FRESULT;
*/
// ---------------------------------------------------------------------------
#ifdef __CC65__
static const char * ERRNO_STR[] = {"ENOENT", "ENOMEM", "EACCES", "ENODEV", "EMFILE",
                                   "EBUSY", "EINVAL", "ENOSPC", "EEXIST", "EAGAIN",
                                   "EIO", "EINTR", "ENOSYS", "ESPIPE", "ERANGE", "EBADF",
                                   "ENOEXEC", "EUNKNOWN"};
#else
static const char * ERRNO_STR[] = {"ERANGE", "EDOM", "EILSEQ", "EINVAL", "ENOMEM"};
#endif
#define errno_str(err) ERRNO_STR[(err)-1];

static const char * FR_STR[] = {"FR_OK","FR_DISK_ERR","FR_INT_ERR","FR_NOT_READY",
                                "FR_NO_FILE","FR_NO_PATH","FR_INVALID_NAME","FR_DENIED",
                                "FR_EXIST","FR_INVALID_OBJECT","FR_WRITE_PROTECTED",
                                "FR_INVALID_DRIVE","FR_NOT_ENABLED","FR_NO_FILESYSTEM",
                                "FR_MKFS_ABORTED","FR_TIMEOUT:","FR_LOCKED",
                                "FR_NOT_ENOUGH_CORE","FR_TOO_MANY_OPEN_FILES"};
#define fr_str(err) FR_STR[(err)-32];

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void ezpsg_instruments(const uint8_t **data)
{
    switch ((int8_t) * (*data)++) // instrument
    {
    case -1: // piano
        ezpsg_play_note(*(*data)++, // note
                        *(*data)++, // duration
                        1,          // release
                        32,         // duty
                        0x33,       // vol_attack
                        0xFD,       // vol_decay
                        0x38,       // wave_release
                        0);         // pan
        break;
    default:
        // The instrumment you just added probably isn't
        // consuming the correct number of paramaters.
        //puts("Unknown instrument.");
        exit(1);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void ReportFileError(void)
{
    char msg[MAX_STATUS_MSG+1] = {0};
    const char * err_code_str = NULL;
    int16_t err_code = errno;

#ifdef __CC65__
    if (err_code >= ENOENT && err_code <= ENOEXEC) {
#else
    if (err_code >= ERANGE && err_code <= ENOMEM) {
#endif
        err_code_str = errno_str(err_code);
    } else if (err_code >= FR_OK && err_code <= FR_TOO_MANY_OPEN_FILES) {
        err_code_str = fr_str(err_code);
    } else {
        // bug in llvm-mos API? It's what I get from open() on any error
        err_code_str = "EUNKNOWN";
    }
    snprintf(msg, MAX_STATUS_MSG,
             "File operation failed with error, %u, '%s'", err_code, err_code_str);
    UpdateStatusBarMsg(msg, STATUS_ERROR);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void InitStatusBar(void)
{
    uint8_t c;
    // draw status bar background
    for (c = 0; c < w; c++) {
        DrawChar(row, col, ' ', bg, fg);
    }
    UpdateStatusBarMsg("Welcome to TE, a text editor for the RP6502", STATUS_INFO);
    UpdateStatusBarPos();
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateStatusBarMsg(const char * status_msg, status_level_t level)
{
    uint8_t c;
    uint8_t fg_clr = fg_info;
    uint8_t msg_len = 0;

    status_timer = 0; // reset, so we display msg for a full 10 seconds

    switch((uint8_t)level) {
        case STATUS_INFO:
            fg_clr = fg_info;
            break;
        case STATUS_WARNING:
            fg_clr = fg_warning;
            break;
        case STATUS_ERROR:
            fg_clr = fg_error;
            break;
    }
    if (status_msg == NULL) {
        memset(msg, 0, MAX_STATUS_MSG+1);
    } else {
        strncpy(msg, status_msg, MAX_STATUS_MSG);
        msg_len = strlen(msg);
    }
    for (c = 0; c <= MAX_STATUS_MSG; c++) {
        DrawChar(row, c, ' ', bg, bg);
    }
    for (c = 0; c < msg_len; c++) {
        DrawChar(row, c+1, msg[c], bg, fg_clr);
    }
    if (msg_len > 0) {
        switch((uint8_t)level) {
            case STATUS_INFO:
                ezpsg_play_song(beep_info);
                break;
            case STATUS_WARNING:
                ezpsg_play_song(beep_warning);
                break;
            case STATUS_ERROR:
                ezpsg_play_song(beep_error);
                break;
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void UpdateStatusBarPos(void)
{
    uint8_t c, start;

    // add extra +1 to line, column, so we have 1,1 at start of doc
    uint16_t line = 1 + TheDoc.cursor_r;
    uint16_t column = 1 + TheDoc.cursor_c;

    snprintf(pos, MAX_CUR_POS, "Line %u Col %u ", line, column);

    start = w-MAX_CUR_POS-1;
    for (c = 0; c < MAX_CUR_POS; c++) {
        DrawChar(row, start+c, ' ', bg, fg);
    }
    start = w-1-strlen(pos);
    for (c = 0; c < strlen(pos); c++) {
        DrawChar(row, start+c, pos[c], bg, fg);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void incr_status_timer(void)
{
    status_timer++;
}
uint16_t get_status_timer(void)
{
    return status_timer;
}