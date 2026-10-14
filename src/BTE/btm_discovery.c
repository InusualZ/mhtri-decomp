/*
 * BTE/btm_discovery.c - the BTM discovery database: init, reset and the discovery timeout.
 *
 * RANGE. .text 0x80484828-0x8048495C (3 functions, 0x134 B); no data. Weak: the three btm_discovery_* functions read
 *   no data, and the .data pad seam 0x806141E0 allows any cut in [0x8048453C, 0x8048495C], so they may be the tail of
 *   BTE/btm_link_key.c or the head of BTE/btm_inq.c; only the name family separates them.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as the matched BTE units (see BTE/l2c_utils.c); btm_discovery_db_init inlines
 *   into btm_discovery_db_reset (`-ipa file`).
 * NAMES. the map's. The file name is a GUESS from the btm_discovery_* family; tBTM_DISC_VAR is a GUESS name.
 * RESIDUALS. none; a trial flip links byte-identically (DOL hash unchanged).
 * SHAPES. plain C; the state is btm_cb.disc_var (BTE/btm_discovery.h's type).
 */

#include "BTE/btm_discovery.h"
#include "BTE/btm_main.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x80484828 (0x78): Clears the discovery state and sets the database up with the public browse group filter. */
void btm_discovery_db_init(void) {
    btm_cb.p_disc_db = (tSDP_DISCOVERY_DB*)btm_cb.disc_var.db_mem;
    memset(&btm_cb.disc_var, 0, sizeof(tBTM_DISC_VAR));

    btm_cb.disc_var.num_uuid = 1;
    btm_cb.disc_var.uuid.len = 2;
    btm_cb.disc_var.uuid.uu.uuid16 = BTM_DISC_UUID_SERVCLASS;

    SDP_InitDiscoveryDb(btm_cb.p_disc_db, BTM_DISC_DB_SIZE, 1, &btm_cb.disc_var.uuid, 0, NULL);
}

/* 0x804848A0 (0xB8): Stops the discovery timer, reports a running discovery as finished and sets the database up
 * again. */
void btm_discovery_db_reset(void) {
    tBTM_DISC_CMPL_CB* p_cb;

    btu_stop_timer(&btm_cb.disc_var.disc_timer);

    if (btm_cb.disc_var.busy) {
        p_cb = btm_cb.disc_var.p_cmpl_cb;
        btm_cb.disc_var.busy = FALSE;
        btm_cb.disc_var.p_cmpl_cb = NULL;
        if (p_cb) {
            (*p_cb)(0);
        }
    }

    btm_discovery_db_init();
}

/* 0x80484958 (0x4): Handles the discovery timer expiry; nothing is pending on it. */
void btm_discovery_timeout(void) {
}
