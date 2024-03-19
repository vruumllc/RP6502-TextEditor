// ---------------------------------------------------------------------------
// statusbar.h
// ---------------------------------------------------------------------------

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_STATUS_MSG 60

typedef enum {STATUS_INFO, STATUS_WARNING, STATUS_ERROR} status_level_t;

void InitStatusBar(void);
void UpdateStatusBarMsg(const char * status_msg, status_level_t level);
void UpdateStatusBarPos(void);
void ReportFileError(void);

void incr_status_timer(void);
uint16_t get_status_timer(void);

#endif // STATUSBAR_H