/*
 * BTE/hidd_api.c - the HID device API: HID_DevInit, which clears the HID device control block and arms its repage timer
 *   callback.
 *
 * RANGE. .text 0x8048FB68-0x8048FBD0 (1 function, 0x68 B); .bss 0x8073C210-0x8073C358. Medium confidence: HID_DevInit
 *   alone between the last btsnd_hcic_* builder and hidd_conn_initiate, no data on either side. The .bss is the HID
 *   device control block (0x148 B, read by the three hidd units); its owner is a GUESS from the BTE source (hidd_api.c
 *   defines it) - any hidd unit gives the same .bss order.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) the row matches as well.
 * NAMES. the map's. GUESS hd_cb (hd_cb, the BTE source's name for the HID device control block).
 * RESIDUALS. none.
 * SHAPES. plain C.
 */

#include "BTE/hidd_api.h"
#include "BTE/hidd_conn.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The MTU of both HID device channels. */
#define HID_DEV_MTU_SIZE 64

tHID_DEV_CTB hd_cb;

/* 0x8048FB68 (0x68): Clears the control block, sets the default channel MTUs and arms the repage timer handler. */
void HID_DevInit(void) {
    memset(&hd_cb, 0, sizeof(tHID_DEV_CTB));
    hd_cb.l2cap_cfg.mtu_present = TRUE;
    hd_cb.l2cap_cfg.mtu = HID_DEV_MTU_SIZE;
    hd_cb.l2cap_intr_cfg.mtu_present = TRUE;
    hd_cb.l2cap_intr_cfg.mtu = HID_DEV_MTU_SIZE;
    hd_cb.conn.timer_entry.param = (u32)hidd_proc_repage_timeout;
    hd_cb.trace_level = 0;
}
