/*
 * BTE/hidd_conn.c - the HID device connection layer: hidd_conn_initiate and the repage timeout that retries it.
 *
 * RANGE. .text 0x8048FBD0-0x8048FD58 (2 functions, 0x188 B); .data 0x80615590-0x806155E0. Medium confidence on the
 *   .text start (HID_DevInit reads no data); the end is hidd_pm.c's hidd_pm_set_now. The .data (trace strings read only
 *   by these two functions) starts with no pad beyond 4-byte alignment where the readers change and ends at the 4-byte
 *   pad before 0x806155E0. Both starts are 16-aligned with no padding before either (0x8048FBD0 + 0xC0 = 0x8048FC90).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) both rows match as well.
 * NAMES. the map's.
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C; hidd_proc_repage_timeout sits in a `#pragma dont_inline on` scope, which keeps the call to
 *   hidd_conn_initiate that `-ipa file` would otherwise inline.
 */

#include "BTE/hidd_conn.h"
#include "BTE/hidd_api.h"

/* 0x8048FBD0 (0xC0): Opens the control channel to the host; returns a HID result. */
u8 hidd_conn_initiate(void) {
    tHID_CONN* p_hcon = &hd_cb.conn;

    HIDD_TRACE_EVENT0("HID - Originate started");

    p_hcon->ctrl_cid = 0;
    p_hcon->intr_cid = 0;
    p_hcon->conn_flags = HID_CONN_FLAGS_IS_ORIG;

    if ((p_hcon->ctrl_cid = L2CA_ConnectReq(HID_PSM_CONTROL, hd_cb.host_addr)) == 0) {
        HIDD_TRACE_WARNING0("HID - Originate failed");
        return HID_ERR_L2CAP_FAILED;
    }

    p_hcon->conn_state = HID_CONN_STATE_CONNECTING_CTRL;
    return HID_SUCCESS;
}

#pragma push
#pragma dont_inline on

/* 0x8048FC90 (0xC8): Retries the connection to the host, or reports the close after the retry limit. */
void hidd_proc_repage_timeout(TIMER_LIST_ENT* p_tle) {
    HIDD_TRACE_DEBUG0("hidd_proc_repage_timeout");

    hd_cb.conn_tries++;
    if (hidd_conn_initiate()) {
        if (hd_cb.conn_tries > HID_DEV_MAX_CONN_RETRY) {
            hd_cb.dev_state = 0;
            hd_cb.callback(HID_DEV_EVT_CLOSE, 0, NULL);
        } else {
            btu_start_timer(&hd_cb.conn.timer_entry, BTU_TTYPE_USER_FUNC, HID_DEV_REPAGE_WIN);
        }
    } else {
        hd_cb.callback(HID_DEV_EVT_RETRYING, hd_cb.conn_tries, NULL);
    }
}

#pragma pop
