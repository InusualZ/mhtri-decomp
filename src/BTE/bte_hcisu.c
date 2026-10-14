/*
 * BTE/bte_hcisu.c - the glue between the BTU task and the HCI services of the Broadcom BTE Bluetooth stack: sends,
 *   polls and closes through the installed transport interface, and posts received messages to the BTU task.
 *
 * RANGE. .text 0x8047A370-0x8047A470 (5 functions); .sbss 0x80795020-0x80795028. Strong start: bte_hcisu_send
 *   (0x8047A370) is the first reader of the interface pointer 0x80795020 and its context 0x80795024 (written by
 *   BTA_Init), which follow BTE/uusb_ppc.c's .sbss. Strong end: 0x8047A470 is the formatter the LogMsg_* functions
 *   tail-call, whose buffer and "%s\n" are BTE/bte_logmsg.c's data. 0x8047A430 (posts a message to the BTU task's
 *   mailbox 0) is the receive callback whose address the USB transport takes.
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's bte_hcisu_send, bte_hcisu_task, bte_hcisu_close and BTE_InitStack. GUESS: 0x8047A430
 *   bte_hcisu_rcv_msg (posts a received message to mailbox 0 of the BTU task), 0x80795020 bte_hcisu_if (the
 *   installed transport, BTA_Init stores BTE/hcisu_h2.c's table) and 0x80795024 bte_hcisu_cfg (the device BTA_Init
 *   asks it to open).
 * RESIDUALS. BTE_InitStack (0x8047A440) is Broadcom's bte_init.c function; it references no data, so a separate cut
 *   0x8047A440-0x8047A470 is neither proven nor refuted by the bytes.
 * SHAPES. plain C; the declarations and the interface type live in BTE/bte_hcisu.h.
 */

#include "BTE/bte_hcisu.h"
#include "BTE/gki_ppc.h"
#include "BTE/port_api.h"
#include "BTE/gap_api.h"   /* GAP_Init */
#include "BTE/hidd_api.h"   /* HID_DevInit */
#include "BTE/hidh_api.h"   /* HID_HostInit */

/* The mailbox of the BTU task that receives HCI messages. */
#define BTU_HCI_RCV_MBOX 0
/* The mailbox the transport may use for its own events, and the BTU event that signals it. */
#define BTU_HCISU_MBOX 1
#define BTU_HCISU_EVT 0x800

/* .sbss 0x80795024 - the device the transport opens (defined first: MWCC emits .sbss backwards). */
tHCISU_CFG* bte_hcisu_cfg;

/* .sbss 0x80795020 - the installed HCI services transport. */
const tHCISU_IF* bte_hcisu_if;

/* 0x8047A370 (0x20): Sends `p_msg` with the event `event` through the transport, or frees it when none is installed. */
void bte_hcisu_send(BT_HDR* p_msg, u16 event) {
    p_msg->event = event;
    if (bte_hcisu_if != NULL) {
        bte_hcisu_if->send(p_msg);
        return;
    }
    GKI_freebuf(p_msg);
}

/* 0x8047A390 (0x64): Sets up the transport's receive path for the BTU task and opens the device. */
void bte_hcisu_task(u32 param) {
    if (bte_hcisu_if != NULL) {
        if (bte_hcisu_if->init != NULL) {
            bte_hcisu_if->init(BTU_TASK, BTU_HCISU_MBOX, BTU_HCISU_EVT);
        }
        if (bte_hcisu_if->open != NULL) {
            bte_hcisu_if->open(bte_hcisu_cfg);
        }
    }
}

/* 0x8047A400 (0x24): Closes the transport. */
void bte_hcisu_close(void) {
    if (bte_hcisu_if != NULL && bte_hcisu_if->close != NULL) {
        bte_hcisu_if->close();
    }
}

/* 0x8047A430 (0x10): Posts a received HCI message to the BTU task. */
void bte_hcisu_rcv_msg(BT_HDR* p_msg) {
    GKI_send_msg(BTU_TASK, BTU_HCI_RCV_MBOX, p_msg);
}

/* 0x8047A440 (0x2C): Initialises RFCOMM, GAP and the HID device and host layers. */
void BTE_InitStack(void) {
    RFCOMM_Init();
    GAP_Init();
    HID_DevInit();
    HID_HostInit();
}
