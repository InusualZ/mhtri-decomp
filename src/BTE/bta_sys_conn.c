/*
 * BTE/bta_sys_conn.c - the BTA system connection manager: the resource-manager, compression and power-manager callback
 *   registrations, the connection open and close notifications and three power-manager status forwards.
 *
 * RANGE. .text 0x8047AC98-0x8047AEEC (8 functions, 0x254 B); no data. Medium start: bta_sys_rm_register,
 *   bta_sys_compress_register and bta_sys_pm_register each store one callback into the bta_sys control block (+0x80,
 *   +0x88, +0x84), the registrations Broadcom's bta_sys_conn.c opens with, after the two address helpers of BTE/bd.c,
 *   which read no data. Medium end: bta_sys_init opens BTE/bta_sys_main.c, whose .data starts after an 8-byte pad
 *   (0x80612ECB -> 0x80612ED0). Every function reads the control block bta_sys_cb, which BTE/bta_sys_main.c owns.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's and the BTE source's: bta_sys_rm_register (0x8047AC98, stores +0x80) and bta_sys_compress_register
 *   (0x8047ACA8, stores +0x88); bta_sys_sco_close, bta_sys_idle and bta_sys_busy (0x8047AE50/0x8047AE84/0x8047AEB8)
 *   pass the power manager the BTA_SYS_* status 5, 6 and 7.
 * RESIDUALS. none known.
 * SHAPES. plain C.
 */

#include "BTE/bta_sys_conn.h"
#include "BTE/bta_sys_main.h"

/* 0x8047AC98 (0x10): Registers the resource manager's connection callback. */
void bta_sys_rm_register(tBTA_SYS_CONN_CBACK* p_cback) {
    bta_sys_cb.prm_cb = p_cback;
}

/* 0x8047ACA8 (0x10): Registers the compression manager's connection callback. */
void bta_sys_compress_register(tBTA_SYS_CONN_CBACK* p_cback) {
    bta_sys_cb.p_compress_cback = p_cback;
}

/* 0x8047ACB8 (0x10): Registers the power manager's connection callback. */
void bta_sys_pm_register(tBTA_SYS_CONN_CBACK* p_cback) {
    bta_sys_cb.ppm_cb = p_cback;
}

/* 0x8047ACC8 (0xC4): Reports to every registered manager that the subsystem `id` opened a connection to `peer_addr`. */
void bta_sys_conn_open(u8 id, u8 app_id, u8* peer_addr) {
    if (bta_sys_cb.prm_cb) {
        bta_sys_cb.prm_cb(BTA_SYS_CONN_OPEN, id, app_id, peer_addr);
    }

    if (bta_sys_cb.ppm_cb) {
        bta_sys_cb.ppm_cb(BTA_SYS_CONN_OPEN, id, app_id, peer_addr);
    }

    if (bta_sys_cb.p_compress_cback) {
        bta_sys_cb.p_compress_cback(BTA_SYS_CONN_OPEN, id, app_id, peer_addr);
    }
}

/* 0x8047AD8C (0xC4): Reports to every registered manager that the subsystem `id` closed its connection to `peer_addr`. */
void bta_sys_conn_close(u8 id, u8 app_id, u8* peer_addr) {
    if (bta_sys_cb.prm_cb) {
        bta_sys_cb.prm_cb(BTA_SYS_CONN_CLOSE, id, app_id, peer_addr);
    }

    if (bta_sys_cb.ppm_cb) {
        bta_sys_cb.ppm_cb(BTA_SYS_CONN_CLOSE, id, app_id, peer_addr);
    }

    if (bta_sys_cb.p_compress_cback) {
        bta_sys_cb.p_compress_cback(BTA_SYS_CONN_CLOSE, id, app_id, peer_addr);
    }
}

/* 0x8047AE50 (0x34): Reports an SCO close of the subsystem `id` to the power manager. */
void bta_sys_sco_close(u8 id, u8 app_id, u8* peer_addr) {
    if (bta_sys_cb.ppm_cb) {
        bta_sys_cb.ppm_cb(BTA_SYS_SCO_CLOSE, id, app_id, peer_addr);
    }
}

/* 0x8047AE84 (0x34): Reports that the subsystem `id`'s connection went idle to the power manager. */
void bta_sys_idle(u8 id, u8 app_id, u8* peer_addr) {
    if (bta_sys_cb.ppm_cb) {
        bta_sys_cb.ppm_cb(BTA_SYS_CONN_IDLE, id, app_id, peer_addr);
    }
}

/* 0x8047AEB8 (0x34): Reports that the subsystem `id`'s connection is busy to the power manager. */
void bta_sys_busy(u8 id, u8 app_id, u8* peer_addr) {
    if (bta_sys_cb.ppm_cb) {
        bta_sys_cb.ppm_cb(BTA_SYS_CONN_BUSY, id, app_id, peer_addr);
    }
}
