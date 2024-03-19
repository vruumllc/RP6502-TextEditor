// ---------------------------------------------------------------------------
// msg_dlg.h
// ---------------------------------------------------------------------------

#ifndef MSG_DLG_H
#define MSG_DLG_H

#include <stdint.h>
#include <stdbool.h>
#include "panel.h"

#define MAX_MSG_LEN 70

typedef enum {INVALID_MSG_DLG_TYPE, OK, YES_NO, OK_CANCEL, YES_NO_CANCEL} msg_dlg_type_t;

typedef struct msg_dlg {
    msg_dlg_type_t msg_dlg_type;
    char dlg_msg[MAX_MSG_LEN+1];
    panel_t panel;
} msg_dlg_t;

msg_dlg_t * NewMsgDlg(char * msg, msg_dlg_type_t type,
                      panel_btn_action Yes,
                      panel_btn_action No,
                      panel_btn_action Cancel);

bool ShowMsgDlg(msg_dlg_t * pmsg_dlg, uint8_t row, uint8_t col);
void DeleteMsgDlg(msg_dlg_t * pmsg_dlg);

void MsgDlgButtonPressed(msg_dlg_t * pmsg_dlg, uint8_t index);
bool IsMsgDlgButtonPressed(msg_dlg_t * pmsg_dlg, int16_t row, int16_t col);

#endif // MSG_DLG_H