/*
 * BTE/btm_main.c - the BTM main module: btm_init and the BTM control block.
 *
 * RANGE. .text 0x804863F4-0x80486450 (1 function, 0x5C B); .bss 0x80739698-0x8073BE60. Medium: btm_init alone,
 *   between btm_inq.c and btm_pm.c in the BTE source order. The .bss is the BTM control block btm_cb (0x27C4 B plus the
 *   8-byte alignment pad, read by every btm unit and by bta/l2c); its owner is a GUESS from the BTE source (btm_main.c
 *   defines it) - no other btm unit has .bss, so any of them gives the same .bss order.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`, as the btm band (BTE/btm_pm.c's FLAGS line); btm_init compiles to the target
 *   under Wii/1.3 `cflags_os` as well.
 * NAMES. the map's.
 * RESIDUALS. none in .text. The .bss object is 0x27C4 B against the map's 0x27C8 (the memset length is the struct
 *   size; the 4 bytes are the alignment pad of the next object).
 * SHAPES. plain C; the control block layout is BTE/btm_main.h's.
 */

#include "BTE/btm_main.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_inq.h"
#include "BTE/btm_sco.h"
#include "BTE/btm_sec.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The trace level the control block starts with (BT_TRACE_LEVEL_NONE). */
#define BTM_INITIAL_TRACE_LEVEL 0
/* The security mode the stack starts in (per-service security). */
#define BTM_SEC_MODE_SERVICE 2


tBTM_CB btm_cb;

/* 0x804863F4 (0x5C): Clears the control block and initialises the inquiry, ACL, security, SCO and device modules. */
void btm_init(void) {
    memset(&btm_cb, 0, sizeof(tBTM_CB));
    btm_cb.trace_level = BTM_INITIAL_TRACE_LEVEL;

    btm_inq_db_init();
    btm_acl_init();
    btm_sec_init(BTM_SEC_MODE_SERVICE);
    btm_sco_init();
    btm_dev_init();
}
