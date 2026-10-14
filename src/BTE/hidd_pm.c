/*
 * BTE/hidd_pm.c - the HID device power management: hidd_pm_set_now, the mode-change handler and the inactivity timeout.
 *
 * RANGE. .text 0x8048FD58-0x804900D4 (3 functions, 0x37C B); no data. Medium confidence: the hidd_pm_* family between
 *   hidd_proc_repage_timeout and HID_HostGetSDPRecord; no data on either side.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) hidd_pm_set_now and
 *   hidd_pm_proc_mode_change do not match.
 * NAMES. the map's.
 * RESIDUALS. none.
 * SHAPES. plain C; hidd_pm_start is inlined into hidd_pm_proc_mode_change and stripped.
 */

#include "BTE/hidd_pm.h"
#include "BTE/hidd_api.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* 0x8048FD58 (0x1D8): Requests the power mode `p_req` for the link to the host; returns whether the request stands. */
u8 hidd_pm_set_now(BtmPmPwrMode* p_req) {
    BtmPmPwrMode act_pm = {0};
    u8 st = BTM_SUCCESS;

    if (hd_cb.pm_ctrl_busy
        || (p_req->mode == hd_cb.curr_pm.mode
            && (p_req->mode == HCI_MODE_ACTIVE
                || (hd_cb.curr_pm.interval >= p_req->minInterval && hd_cb.curr_pm.interval <= p_req->maxInterval)))) {
        hd_cb.final_pm.mode = 0xFF;
        return TRUE;
    }

    switch (p_req->mode) {
    case HCI_MODE_ACTIVE:
        if (hd_cb.curr_pm.mode == HCI_MODE_SNIFF) {
            st = BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, hd_cb.host_addr, &act_pm);
            hd_cb.pm_ctrl_busy = TRUE;
        } else if (hd_cb.curr_pm.mode == HCI_MODE_PARK) {
            st = BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, hd_cb.host_addr, &act_pm);
            hd_cb.pm_ctrl_busy = TRUE;
        }
        break;
    case HCI_MODE_SNIFF:
        if (hd_cb.curr_pm.mode != HCI_MODE_ACTIVE) {
            hidd_pm_set_now(&act_pm);
        } else {
            st = BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, hd_cb.host_addr, p_req);
            hd_cb.pm_ctrl_busy = TRUE;
        }
        break;
    case HCI_MODE_PARK:
        if (hd_cb.curr_pm.mode != HCI_MODE_ACTIVE) {
            hidd_pm_set_now(&act_pm);
        } else {
            st = BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, hd_cb.host_addr, p_req);
            hd_cb.pm_ctrl_busy = TRUE;
        }
        break;
    }

    if (st == BTM_SUCCESS || st == BTM_CMD_STARTED) {
        return TRUE;
    }

    st += HID_DEV_PM_ERR_BASE;
    if (hd_cb.callback) {
        hd_cb.callback(HID_DEV_EVT_PM_FAILED, hd_cb.conn_substate, &st);
    }
    return FALSE;
}

/* Copies the busy power mode to the final one, requests it and starts the inactivity timer. */
static void hidd_pm_start(void) {
    memcpy(&hd_cb.final_pm, &hd_cb.pm_params[HID_DEV_BUSY_CONN_ST], sizeof(BtmPmPwrMode));
    hidd_pm_set_now(&hd_cb.pm_params[HID_DEV_BUSY_CONN_ST]);
    hd_cb.conn_substate = HID_DEV_BUSY_CONN_ST;
    hd_cb.idle_tle.param = (u32)hidd_pm_inact_timeout;
    btu_start_timer(&hd_cb.idle_tle, BTU_TTYPE_USER_FUNC, HID_DEV_INACT_TIMEOUT);
}

/* 0x8048FF30 (0x158): Takes the link's new power mode and moves on towards the final one. */
void hidd_pm_proc_mode_change(u8 hci_status, u8 mode, u16 interval) {
    if (!hd_cb.reg_flag) {
        return;
    }

    hd_cb.pm_ctrl_busy = FALSE;

    if (hci_status != 0) {
        if (hd_cb.callback) {
            hd_cb.callback(HID_DEV_EVT_PM_FAILED, hd_cb.conn_substate, &hci_status);
        }
        return;
    }

    hd_cb.curr_pm.mode = mode;
    hd_cb.curr_pm.interval = interval;

    if (hd_cb.final_pm.mode != 0xFF) {
        if (hd_cb.final_pm.mode != hd_cb.curr_pm.mode
            || (hd_cb.final_pm.mode != HCI_MODE_ACTIVE
                && (hd_cb.curr_pm.interval < hd_cb.final_pm.minInterval
                    || hd_cb.curr_pm.interval > hd_cb.final_pm.maxInterval))) {
            hidd_pm_set_now(&hd_cb.final_pm);
        } else {
            hd_cb.final_pm.mode = 0xFF;
        }
    } else if (hd_cb.curr_pm.mode == HCI_MODE_ACTIVE) {
        hidd_pm_start();
    }

    if (hd_cb.callback) {
        hd_cb.callback(HID_DEV_EVT_MODE_CHG, mode, (u8*)&interval);
    }
}

/* 0x80490088 (0x4C): Switches the link to the idle power mode once the inactivity timer runs out. */
void hidd_pm_inact_timeout(TIMER_LIST_ENT* p_tle) {
    memcpy(&hd_cb.final_pm, &hd_cb.pm_params[HID_DEV_IDLE_CONN_ST], sizeof(BtmPmPwrMode));
    hidd_pm_set_now(&hd_cb.pm_params[HID_DEV_IDLE_CONN_ST]);
    hd_cb.conn_substate = HID_DEV_IDLE_CONN_ST;
}
