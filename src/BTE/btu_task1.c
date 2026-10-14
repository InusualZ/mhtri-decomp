/*
 * BTE/btu_task1.c - the BTU task of the Broadcom BTE Bluetooth stack: its initialisation, the task body that serves
 *   the HCI and protocol mailboxes and the timer list, and the BTU timer start and stop.
 *
 * RANGE. .text 0x8047A780-0x8047ABC4 (4 functions); .bss 0x80739170-0x807391F8; .sdata 0x80793D78-0x80793D80;
 *   .sbss 0x80795030-0x80795038. Strong start: btu_task_init (0x8047A780) initialises the .sdata 0x80793D78 /
 *   0x80793D7C and the .sbss 0x80795030 / 0x80795034 the task body 0x8047A7C0 reads. Strong end: 0x8047ABC4 is the
 *   first function start of the BTE band that is not 16-aligned (BTE/bd.c). The BTU control block btu_cb (.bss
 *   0x80739170, read by the task body, the timer functions and BTE/hcisu_h2.c, cleared by BTE_Init in
 *   BTE/btu_init.c) lies after BTE/bta_init.c's alarm and before the bta_sys control block 0x807391F8 of the files
 *   that follow, so it is this file's.
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's btu_cb, btu_task_init, btu_start_timer and btu_stop_timer. GUESS: 0x8047A7C0 btu_task (the
 *   fiber body BTE/bta_init.c runs), 0x80793D7C btu_task_idle (cleared while a pass runs, so an alarm that fires
 *   during one returns at once), 0x80793D78 btu_task_overrun (counts those alarms), 0x80795030 btu_task_ticks (the
 *   passes run) and 0x80795034 btu_timer_ticks (the pass of the last one-second timer event). The BTU_TTYPE values
 *   are the timer types the protocol layers start (L2CAP 2-4, SDP 5, RFCOMM 11-12 are their headers' values);
 *   the other names are the BTE source's for the callees each value reaches.
 * RESIDUALS. none known.
 * SHAPES. plain C; btu_cb, its type and the task declarations live in BTE/btu_task1.h.
 */

#include "BTE/btu_task1.h"
#include "BTE/gki_ppc.h"
#include "BTE/bte_hcisu.h"
#include "BTE/l2c_main.h"
#include "BTE/l2c_link.h"
#include "BTE/sdp_main.h"
#include "BTE/rfc_utils.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_inq.h"
#include "BTE/btm_sco.h"
#include "OS/OSTime.h"
#include "BTE/btm_discovery.h"   /* btm_discovery_timeout */
#include "BTE/hidh_conn.h"   /* hidh_proc_repage_timeout */
#include "BTE/bta_sys_main.h"   /* bta_sys_init, bta_sys_event, bta_sys_timer_update */
#include "BTE/btu_init.h"   /* btu_init_core */
#include "BTE/btu_hcif.h"

/* The GKI events of the BTU task: its mailboxes, its two timers and the stop request. */
#define TASK_MBOX_0_EVT_MASK 0x0001
#define TASK_MBOX_2_EVT_MASK 0x0004
#define TIMER_0_EVT_MASK 0x0010
#define TIMER_1_EVT_MASK 0x0020
#define EVENT_MASK_APPL_EVT_7 0x8000

#define BTU_HCI_RCV_MBOX 0
#define BTA_MBOX 2

/* The passes of the 2 ms alarm per one-second timer event. */
#define BTU_TIMER_PASSES 500

/* The message classes of the HCI mailbox (the high byte of the event). */
#define BT_EVT_MASK 0xFF00
#define BT_EVT_TO_BTU_HCI_EVT 0x1000
#define BT_EVT_TO_BTU_HCI_ACL 0x1100
#define BT_EVT_TO_BTU_HCI_SCO 0x1200
#define BT_EVT_TO_BTU_HCI_CMD 0x1600
#define BT_EVT_TO_BTU_L2C_SEG_XMIT 0x1900

/* The timer types of the BTU timer list. */
#define BTU_TTYPE_BTM_DEV_CTL 1
#define BTU_TTYPE_L2CAP_LINK 2
#define BTU_TTYPE_L2CAP_CHNL 3
#define BTU_TTYPE_L2CAP_HOLD 4
#define BTU_TTYPE_SDP 5
#define BTU_TTYPE_BTM_DISC 8
#define BTU_TTYPE_BTM_ACL 9
#define BTU_TTYPE_BTM_RMT_NAME 10
#define BTU_TTYPE_RFCOMM_MFC 11
#define BTU_TTYPE_RFCOMM_PORT 12
#define BTU_TTYPE_USER_FUNC 22
#define BTU_TTYPE_BTU_CMD_CMPL 60
#define BTU_TTYPE_HID_HOST_REPAGE_TO 66

/* .bss 0x80739170 - the BTU control block. */
tBTU_CB btu_cb;

/* .sdata 0x80793D78 - the alarms that found a pass still running. */
u8 btu_task_overrun = 1;

/* .sdata 0x80793D7C - set while no pass runs. */
BOOL btu_task_idle = 1;

/* .sbss 0x80795034 - the pass of the last one-second timer event (defined first: MWCC emits .sbss backwards). */
u32 btu_timer_ticks;

/* .sbss 0x80795030 - the passes the alarm started. */
u32 btu_task_ticks;

/* 0x8047A780 (0x40): Resets the task's bookkeeping and initialises the core, the protocol layers and BTA. */
void btu_task_init(void) {
    btu_task_overrun = 1;
    btu_task_idle = 1;
    btu_task_ticks = 0;
    btu_timer_ticks = 0;
    btu_init_core();
    BTE_InitStack();
    bta_sys_init();
}

/* 0x8047A7C0 (0x37C): Runs one pass of the BTU task: the HCI mailbox, the BTU timer list and the BTA mailbox and timer. */
void btu_task(void) {
    u16 event;
    u16 mask;
    BT_HDR* p_msg;
    u8 i;
    BOOL handled;
    BOOL done = FALSE;

    OSGetTime();
    btu_task_ticks++;

    GKI_disable();
    if (btu_task_idle) {
        btu_task_idle = 0;
        btu_task_overrun = 1;
    } else {
        btu_task_overrun++;
        GKI_enable();
        return;
    }
    GKI_enable();

    event = TASK_MBOX_0_EVT_MASK | TASK_MBOX_2_EVT_MASK;
    if (btu_task_ticks > btu_timer_ticks + BTU_TIMER_PASSES) {
        btu_timer_ticks = btu_task_ticks;
        event |= TIMER_0_EVT_MASK | TIMER_1_EVT_MASK;
    }

    while (!done) {
        done = TRUE;

        if (event & TASK_MBOX_0_EVT_MASK) {
            while ((p_msg = (BT_HDR*)GKI_read_mbox(BTU_HCI_RCV_MBOX)) != NULL) {
                done = FALSE;

                switch (p_msg->event & BT_EVT_MASK) {
                case BT_EVT_TO_BTU_HCI_ACL:
                    l2c_rcv_acl_data(p_msg);
                    break;
                case BT_EVT_TO_BTU_L2C_SEG_XMIT:
                    l2c_link_segments_xmitted(p_msg);
                    break;
                case BT_EVT_TO_BTU_HCI_SCO:
                    btm_route_sco_data(p_msg);
                    break;
                case BT_EVT_TO_BTU_HCI_EVT:
                    btu_hcif_process_event(p_msg);
                    GKI_freebuf(p_msg);
                    break;
                case BT_EVT_TO_BTU_HCI_CMD:
                    btu_hcif_send_cmd(p_msg);
                    break;
                default:
                    i = 0;
                    mask = (u16)(p_msg->event & BT_EVT_MASK);
                    handled = FALSE;

                    for (; !handled && i < BTU_MAX_REG_EVENT; i++) {
                        if (btu_cb.event_reg[i].event_cb == NULL) {
                            continue;
                        }

                        if (mask == btu_cb.event_reg[i].event_range) {
                            if (btu_cb.event_reg[i].event_cb) {
                                btu_cb.event_reg[i].event_cb(p_msg);
                                handled = TRUE;
                            }
                        }
                    }

                    if (handled == FALSE) {
                        GKI_freebuf(p_msg);
                    }
                    break;
                }
            }
        }

        if (event & TIMER_0_EVT_MASK) {
            TIMER_LIST_ENT* p_tle;

            GKI_update_timer_list(&btu_cb.timer_queue, 1);
            event = (u16)(event & ~TIMER_0_EVT_MASK);

            while (btu_cb.timer_queue.p_first != NULL && btu_cb.timer_queue.p_first->ticks == 0) {
                p_tle = btu_cb.timer_queue.p_first;
                done = FALSE;
                GKI_remove_from_timer_list(&btu_cb.timer_queue, p_tle);

                switch (p_tle->event) {
                case BTU_TTYPE_BTM_DEV_CTL:
                    btm_dev_timeout(p_tle);
                    break;
                case BTU_TTYPE_BTM_ACL:
                    btm_acl_timeout(p_tle);
                    break;
                case BTU_TTYPE_L2CAP_LINK:
                case BTU_TTYPE_L2CAP_CHNL:
                case BTU_TTYPE_L2CAP_HOLD:
                    l2c_process_timeout(p_tle);
                    break;
                case BTU_TTYPE_SDP:
                    sdp_conn_timeout((tCONN_CB*)p_tle->param);
                    break;
                case BTU_TTYPE_BTM_RMT_NAME:
                    btm_inq_rmt_name_failed();
                    break;
                case BTU_TTYPE_BTM_DISC:
                    btm_discovery_timeout();
                    break;
                case BTU_TTYPE_RFCOMM_MFC:
                case BTU_TTYPE_RFCOMM_PORT:
                    rfcomm_process_timeout(p_tle);
                    break;
                case BTU_TTYPE_BTU_CMD_CMPL:
                    btu_hcif_cmd_timeout();
                    break;
                case BTU_TTYPE_HID_HOST_REPAGE_TO:
                    hidh_proc_repage_timeout(p_tle);
                    break;
                case BTU_TTYPE_USER_FUNC:
                    ((void (*)(TIMER_LIST_ENT*))p_tle->param)(p_tle);
                    break;
                default:
                    i = 0;
                    handled = FALSE;

                    for (; !handled && i < BTU_MAX_REG_TIMER; i++) {
                        if (btu_cb.timer_reg[i].timer_cb == NULL) {
                            continue;
                        }

                        if (btu_cb.timer_reg[i].p_tle == p_tle) {
                            btu_cb.timer_reg[i].timer_cb(p_tle);
                            handled = TRUE;
                        }
                    }
                    break;
                }
            }
        }

        if (event & TASK_MBOX_2_EVT_MASK) {
            while ((p_msg = (BT_HDR*)GKI_read_mbox(BTA_MBOX)) != NULL) {
                done = FALSE;
                bta_sys_event(p_msg);
            }
        }

        if (event & TIMER_1_EVT_MASK) {
            event = (u16)(event & ~TIMER_1_EVT_MASK);
            bta_sys_timer_update();
        }

        if (event & EVENT_MASK_APPL_EVT_7) {
            break;
        }
    }

    btu_task_idle = 1;
}

/* 0x8047AB40 (0x70): Starts the timer entry `p_tle` of type `type` for `timeout` ticks of the BTU timer list. */
void btu_start_timer(TIMER_LIST_ENT* p_tle, u16 type, u32 timeout) {
    GKI_remove_from_timer_list(&btu_cb.timer_queue, p_tle);
    p_tle->event = type;
    p_tle->ticks = timeout;
    GKI_add_to_timer_list(&btu_cb.timer_queue, p_tle);
}

/* 0x8047ABB0 (0x14): Stops the timer entry `p_tle`. */
void btu_stop_timer(TIMER_LIST_ENT* p_tle) {
    GKI_remove_from_timer_list(&btu_cb.timer_queue, p_tle);
}
