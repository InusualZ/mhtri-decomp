/* BTE/bta_sys_int.h - the BTA system manager's types: the subsystem registration, the connection callback and the system
 *   control block BTE/bta_sys_main.c defines and BTE/bta_sys_conn.c reads. */
#ifndef MHTRI_BTE_BTA_SYS_INT_H
#define MHTRI_BTE_BTA_SYS_INT_H

#include "types.h"
#include "BTE/gki_buffer.h"
#include "BTE/ptim.h"

/* The number of BTA subsystem ids (the high byte of a BTA event). */
#define BTA_ID_MAX 26

/* The connection status values the connection callbacks receive. */
#define BTA_SYS_CONN_OPEN 0
#define BTA_SYS_CONN_CLOSE 1
#define BTA_SYS_APP_OPEN 2
#define BTA_SYS_APP_CLOSE 3
#define BTA_SYS_SCO_OPEN 4
#define BTA_SYS_SCO_CLOSE 5
#define BTA_SYS_CONN_IDLE 6
#define BTA_SYS_CONN_BUSY 7

/* A subsystem's event handler: returns whether the message may be freed. */
typedef u8 tBTA_SYS_EVT_HDLR(BT_HDR* p_msg);

/* A subsystem's disable function. */
typedef void tBTA_SYS_DISABLE(void);

/* A connection status callback: `status` is a BTA_SYS_* value for the subsystem `id`, its `app_id` and the peer. */
typedef void tBTA_SYS_CONN_CBACK(u8 status, u8 id, u8 app_id, u8* peer_addr);

/* size: 0x08 - a subsystem's registration: its event handler and its disable function. */
typedef struct tBTA_SYS_REG {
    /* +0x00 */ tBTA_SYS_EVT_HDLR* evt_hdlr;
    /* +0x04 */ tBTA_SYS_DISABLE* disable;
} tBTA_SYS_REG; /* size: 0x08 */

/* size: 0x8C - the BTA system control block (bta_sys_init clears 0x8C bytes). */
typedef struct tBTA_SYS_CB {
    /* +0x00 */ const tBTA_SYS_REG* reg[BTA_ID_MAX];
    /* +0x68 */ tPTIM_CB ptim_cb;
    /* +0x7C */ u8 timers_disabled;
    /* +0x7D */ u8 task_id;
    /* +0x7E */ u8 events_disabled;
    /* +0x7F */ u8 pad_0x7F;
    /* +0x80 */ tBTA_SYS_CONN_CBACK* prm_cb;
    /* +0x84 */ tBTA_SYS_CONN_CBACK* ppm_cb;
    /* +0x88 */ tBTA_SYS_CONN_CBACK* p_compress_cback;
} tBTA_SYS_CB; /* size: 0x8C */

#endif /* MHTRI_BTE_BTA_SYS_INT_H */
