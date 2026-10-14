/*
 * BTE/gap_api.c - the GAP API: GAP_Init, which clears the GAP control block, registers the two BTM callbacks and calls
 *   gap_conn_init.
 *
 * RANGE. .text 0x8048C294-0x8048C2F4 (1 function, 0x60 B); .bss 0x8073BE60-0x8073C210. GAP_Init is the only GAP API
 *   function retail keeps; gap_conn.c's gap_conn_init follows. Medium confidence on the .text (no data on either side).
 *   The .bss is the GAP control block (0x3B0 B, read by GAP_Init and the gap_conn/gap_utils functions, after the BTM
 *   control block and before the HID device one); its owner is a GUESS from the BTE source (gap_api.c defines it) - any
 *   GAP unit gives the same .bss order.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) GAP_Init does not match.
 * NAMES. the map's. GUESS gap_cb (gap_cb, the BTE source's name for the GAP control block).
 * RESIDUALS. none.
 * SHAPES. plain C.
 */

#include "BTE/gap_api.h"
#include "BTE/gap_conn.h"
#include "BTE/gap_utils.h"
#include "Runtime.PPCEABI.H/memset.h"

tGAP_CB gap_cb;

/* 0x8048C294 (0x60): Clears the control block, installs the two BTM completion callbacks and starts the connection
 * layer. */
void GAP_Init(void) {
    memset(&gap_cb, 0, sizeof(tGAP_CB));
    gap_cb.btm_cback[0] = gap_btm_cback0;
    gap_cb.btm_cback[1] = gap_btm_cback1;
    gap_cb.trace_level = BT_TRACE_LEVEL_DEBUG;
    gap_conn_init();
}
