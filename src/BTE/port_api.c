/*
 * BTE/port_api.c - the RFCOMM port API: RFCOMM_Init, which clears the RFCOMM control block and calls
 *   rfcomm_l2cap_if_init.
 *
 * RANGE. .text 0x80498FBC-0x8049900C (1 function, 0x50 B); .bss 0x8073CF48-0x8073D360. RFCOMM_Init is the last
 *   function of the BTE port_api.c and the only one retail keeps (the PORT_* API is dead-stripped); port_rfc.c's
 *   PORT_StartCnf follows. The .bss is the RFCOMM control block (0x418 B, read by every RFCOMM unit); its owner is a
 *   GUESS - any RFCOMM unit before sdp_main gives the same .bss order.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`) is the compiler of all nine
 *   RFCOMM units, as of the six SDP units (BTE/sdp_utils.c): under it every .text size equals the target's, while the
 *   registered Wii/1.3 `cflags_os` differs in register allocation, loop shapes and switch lowering (rfc_ts_frames .text
 *   0x1678 B for 0x1648 B); -ipa file is what inlines the later-defined rfc_timer_start/stop into
 *   rfc_alloc_multiplexer_channel and rfc_find_lcid_mcb/rfc_save_lcid_mcb into the L2CAP callbacks. configure.py builds
 *   the nine objects with it, per object. The function starts of this band are packed to 4 bytes.
 * NAMES. the map's, which are the BTE source's; rfc_cb (.bss 0x8073CF48) is a GUESS from the BTE source's control
 *   block name (every RFCOMM unit reads its trace level at +0x414).
 * RESIDUALS. none: .text and .bss are byte-identical to the target.
 * SHAPES. plain C; the PORT_* API (and port_open_continue in BTE/port_rfc.c) is not reconstructed: retail
 *   dead-strips it.
 */

#include "BTE/port_api.h"
#include "BTE/rfc_l2cap_if.h"
#include "Runtime.PPCEABI.H/memset.h"

/* .bss 0x8073CF48 - the RFCOMM control block. */
tRFC_CB rfc_cb;

/* 0x80498FBC (0x50): Clears the RFCOMM control block, sets its defaults and registers RFCOMM with L2CAP. */
void RFCOMM_Init(void) {
    memset(&rfc_cb, 0, sizeof(tRFC_CB));
    rfc_cb.rfc.last_mux = MAX_BD_CONNECTIONS;
    rfc_cb.trace_level = BT_TRACE_LEVEL_DEBUG;
    rfcomm_l2cap_if_init();
}
