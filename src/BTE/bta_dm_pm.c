/*
 * BTE/bta_dm_pm.c - the BTA device manager power management: init, the connection-status callback, the power mode
 *   selection and the BTM power-mode and timer callbacks.
 *
 * RANGE. .text 0x8047DC5C-0x8047E678 (8 functions, 0xA1C B); .data 0x80613048-0x80613098; .bss 0x80739438-0x80739468.
 *   Strong start: the .data starts after an 8-byte pad (0x80613043 -> 0x80613048) and bta_dm_init_pm is the first
 *   bta_dm_pm.c function. Strong end: BTE/bta_hh_act.c's .data starts after an 8-byte pad (0x80613091 -> 0x80613098),
 *   and bta_dm_pm_timer, which forwards to bta_dm_pm_set_mode, is the last function before bta_hh_api_enable. Both
 *   .data strings are read only by bta_dm_pm_cback and bta_dm_pm_set_mode. The .bss object 0x80739438 (0x2E B, read
 *   by bta_dm_enable, the compression callback and this range) is GUESS Broadcom's bta_dm_conn_srvcs, a bta_dm_pm.c
 *   global; BTE/bta_dm_main.c or BTE/bta_dm_act.c would give the same layout.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`; BTE/bta_dm_act.c's header has the compiler evidence.
 * NAMES. Broadcom's bta_dm_pm.c names. GUESS: bta_dm_disable_pm (0x8047DCC4, deregisters from BTM), bta_dm_pm_cback
 *   (0x8047DCDC, the connection-status callback bta_dm_init_pm registers), bta_dm_pm_timer (0x8047E66C, the action of
 *   BTA_DM_PM_TIMER_EVT).
 * RESIDUALS. bta_dm_pm_set_mode colors the volatile registers of the configuration search and the spec lookup
 *   differently (r7/r8/r3/r6 for the table, entry, app id and count where we use r3/r7/r4/r8); operand order and a
 *   pointer-free spelling do not move it. String relocations name the compiler's pool where the map has
 *   lbl_80613048 / lbl_80613074.
 * SHAPES. plain C. The pm_cback timer stop loop uses its own block-scoped index (the register order of an inlined
 *   helper); bta_dm_pm_park, bta_dm_pm_sniff and bta_dm_pm_active are static helpers
 *   the compiler inlines into their callers (retail has no out-of-line copy).
 */

#include "BTE/bta_dm_pm.h"
#include "BTE/bta_dm_main.h"
#include "BTE/btm_pm.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTM power manager registration requests and the BTM power modes. */
#define BTM_PM_REG_SET 3
#define BTM_PM_DEREG 4
#define BTM_PM_MD_ACTIVE 0
#define BTM_PM_MD_SNIFF 2
#define BTM_PM_MD_PARK 3
#define BTM_PM_STS_ACTIVE 0

static void bta_dm_pm_cback(u8 status, u8 id, u8 app_id, u8* peer_addr);
static void bta_dm_pm_set_mode(u8* peer_addr, u8 timed_out);
static void bta_dm_pm_timer_cback(TIMER_LIST_ENT* p_tle);
static void bta_dm_pm_btm_cback(const u8* bd_addr, u32 status, u16 value, u8 hci_status);

tBTA_DM_CONNECTED_SRVCS bta_dm_conn_srvcs;

/* 0x8047DC5C (0x68): Clears the connected services and registers the power manager when it has a configuration. */
void bta_dm_init_pm(void) {
    memset(&bta_dm_conn_srvcs, 0, sizeof(bta_dm_conn_srvcs));

    if (p_bta_dm_pm_cfg[0].app_id != 0) {
        bta_sys_pm_register(bta_dm_pm_cback);
        BTM_PmRegister(BTM_PM_REG_SET, &bta_dm_cb.pm_id, bta_dm_pm_btm_cback);
    }
}

/* 0x8047DCC4 (0x18): Deregisters the power manager from BTM. */
void bta_dm_disable_pm(void) {
    BTM_PmRegister(BTM_PM_DEREG, &bta_dm_cb.pm_id, NULL);
}

/* 0x8047DCDC (0x380): Tracks the services connected to `peer_addr` and reselects the power mode of the link. */
static void bta_dm_pm_cback(u8 status, u8 id, u8 app_id, u8* peer_addr) {
    u8 i, j;
    u16 policy_setting;
    BtmVersionInfo vers;

    if (BTM_ReadLocalVersion(&vers) == BTM_SUCCESS && vers.manufacturer == LMP_COMPID_BROADCOM &&
        vers.hciVersion < 3) {
        if (status == BTA_SYS_SCO_OPEN) {
            policy_setting = bta_dm_cfg.policy_settings & HCI_ENABLE_SNIFF_MODE_MASK;
            BTM_SetLinkPolicy(peer_addr, &policy_setting);
        } else if (status == BTA_SYS_SCO_CLOSE) {
            policy_setting = bta_dm_cfg.policy_settings;
            BTM_SetLinkPolicy(peer_addr, &policy_setting);
        }
    }

    for (i = 1; i <= p_bta_dm_pm_cfg[0].app_id; i++) {
        if (p_bta_dm_pm_cfg[i].id == id &&
            (p_bta_dm_pm_cfg[i].app_id == BTA_ALL_APP_ID || p_bta_dm_pm_cfg[i].app_id == app_id)) {
            break;
        }
    }

    if (i > p_bta_dm_pm_cfg[0].app_id) {
        return;
    }

    {
        u8 k;
        for (k = 0; k < BTA_DM_NUM_PM_TIMER; k++) {
            if (bta_dm_cb.pm_timer[k].in_use && !bdcmp(bta_dm_cb.pm_timer[k].peer_bdaddr, peer_addr)) {
                bta_sys_stop_timer(&bta_dm_cb.pm_timer[k].timer);
                bta_dm_cb.pm_timer[k].in_use = FALSE;
                break;
            }
        }
    }

    if (p_bta_dm_pm_spec[p_bta_dm_pm_cfg[i].spec_idx].actn_tbl[status][0].power_mode == BTA_DM_PM_NO_ACTION) {
        return;
    }

    for (j = 0; j < bta_dm_conn_srvcs.count; j++) {
        if (bta_dm_conn_srvcs.conn_srvc[j].id == id && bta_dm_conn_srvcs.conn_srvc[j].app_id == app_id &&
            !bdcmp(bta_dm_conn_srvcs.conn_srvc[j].peer_bdaddr, peer_addr)) {
            break;
        }
    }

    if (p_bta_dm_pm_spec[p_bta_dm_pm_cfg[i].spec_idx].actn_tbl[status][0].power_mode == BTA_DM_PM_NO_PREF) {
        if (j != bta_dm_conn_srvcs.count) {
            for (; j < bta_dm_conn_srvcs.count; j++) {
                memcpy(&bta_dm_conn_srvcs.conn_srvc[j], &bta_dm_conn_srvcs.conn_srvc[j + 1], sizeof(tBTA_DM_SRVCS));
            }
            bta_dm_conn_srvcs.count--;
        }
    } else if (j == bta_dm_conn_srvcs.count) {
        if (bta_dm_conn_srvcs.count == BTA_DM_NUM_CONN_SRVS) {
            APPL_TRACE_WARNING0("bta_dm_act no more connected service cbs");
            return;
        }
        bta_dm_conn_srvcs.conn_srvc[j].id = id;
        bta_dm_conn_srvcs.conn_srvc[j].app_id = app_id;
        bdcpy(bta_dm_conn_srvcs.conn_srvc[j].peer_bdaddr, peer_addr);
        bta_dm_conn_srvcs.count++;
    }

    for (i = 0; i < bta_dm_cb.device_list.count; i++) {
        if (!bdcmp(bta_dm_cb.device_list.peer_device[i].peer_bdaddr, peer_addr)) {
            bta_dm_cb.device_list.peer_device[i].pm_mode_attempted = 0;
            bta_dm_cb.device_list.peer_device[i].pm_mode_failed = 0;
            break;
        }
    }

    bta_dm_conn_srvcs.conn_srvc[j].state = status;
    bta_dm_pm_set_mode(peer_addr, FALSE);
}

/* Parks the link to `peer_addr` unless it is parked. */
static void bta_dm_pm_park(u8* peer_addr) {
    u8 mode;

    BTM_ReadPowerMode(peer_addr, &mode);
    if (mode != BTM_PM_MD_PARK) {
        BTM_SetPowerMode(bta_dm_cb.pm_id, peer_addr, &p_bta_dm_pm_md[1]);
    }
}

/* Puts the link to `peer_addr` in sniff mode unless it is sniffing. */
static void bta_dm_pm_sniff(u8* peer_addr) {
    u8 mode;

    BTM_ReadPowerMode(peer_addr, &mode);
    if (mode != BTM_PM_MD_SNIFF) {
        BTM_SetPowerMode(bta_dm_cb.pm_id, peer_addr, &p_bta_dm_pm_md[0]);
    }
}

/* Brings the link to `peer_addr` back to active mode. */
static void bta_dm_pm_active(u8* peer_addr) {
    BtmPmPwrMode pm;

    pm.mode = BTM_PM_MD_ACTIVE;
    BTM_SetPowerMode(bta_dm_cb.pm_id, peer_addr, &pm);
}

/* 0x8047E05C (0x380): Picks the power mode the services connected to `peer_addr` allow and applies it, after their
 * delay unless `timed_out`. */
static void bta_dm_pm_set_mode(u8* peer_addr, u8 timed_out) {
    u8 pm_action = BTA_DM_PM_NO_ACTION;
    u16 timeout = 0;
    u8 i, j;
    u8 failed_pm = 0;
    tBTA_DM_PEER_DEVICE* p_peer_device = NULL;
    u8 allowed_modes = 0;
    u8 pref_modes = 0;
    tBTA_DM_SRVCS* p_srvcs;
    tBTA_DM_PM_SPEC* p_pm_spec;
    tBTA_DM_PM_ACTN* p_act0;
    tBTA_DM_PM_ACTN* p_act1;

    if (!bta_dm_cb.device_list.count) {
        return;
    }

    for (i = 0; i < bta_dm_cb.device_list.count; i++) {
        if (!bdcmp(bta_dm_cb.device_list.peer_device[i].peer_bdaddr, peer_addr)) {
            p_peer_device = &bta_dm_cb.device_list.peer_device[i];
            failed_pm = p_peer_device->pm_mode_failed;
            break;
        }
    }

    if (p_peer_device == NULL) {
        return;
    }

    for (i = 0; i < bta_dm_conn_srvcs.count; i++) {
        if (!bdcmp(bta_dm_conn_srvcs.conn_srvc[i].peer_bdaddr, peer_addr)) {
            p_srvcs = &bta_dm_conn_srvcs.conn_srvc[i];
            for (j = 1; j <= p_bta_dm_pm_cfg[0].app_id; j++) {
                if (p_srvcs->id == p_bta_dm_pm_cfg[j].id &&
                    (p_bta_dm_pm_cfg[j].app_id == BTA_ALL_APP_ID || p_srvcs->app_id == p_bta_dm_pm_cfg[j].app_id)) {
                    break;
                }
            }

            p_pm_spec = &p_bta_dm_pm_spec[p_bta_dm_pm_cfg[j].spec_idx];
            p_act0 = &p_pm_spec->actn_tbl[p_srvcs->state][0];
            p_act1 = &p_pm_spec->actn_tbl[p_srvcs->state][1];

            allowed_modes |= p_pm_spec->allow_mask;

            if (!(failed_pm & p_act0->power_mode)) {
                pref_modes |= p_pm_spec->actn_tbl[bta_dm_conn_srvcs.conn_srvc[i].state][0].power_mode;
                if (p_pm_spec->actn_tbl[bta_dm_conn_srvcs.conn_srvc[i].state][0].power_mode > pm_action) {
                    pm_action = p_act0->power_mode;
                    timeout = p_pm_spec->actn_tbl[bta_dm_conn_srvcs.conn_srvc[i].state][0].timeout;
                }
            } else if (!(failed_pm & p_act1->power_mode)) {
                pref_modes |= p_pm_spec->actn_tbl[bta_dm_conn_srvcs.conn_srvc[i].state][1].power_mode;
                if (p_pm_spec->actn_tbl[bta_dm_conn_srvcs.conn_srvc[i].state][1].power_mode > pm_action) {
                    pm_action = p_act1->power_mode;
                    timeout = p_pm_spec->actn_tbl[bta_dm_conn_srvcs.conn_srvc[i].state][1].timeout;
                }
            }
        }
    }

    if ((pm_action & (BTA_DM_PM_PARK | BTA_DM_PM_SNIFF)) && !(allowed_modes & pm_action)) {
        pm_action = allowed_modes & (BTA_DM_PM_PARK | BTA_DM_PM_SNIFF) & pref_modes;
        if (!pm_action) {
            timeout = 0;
        }
    }

    if (!timed_out && timeout) {
        for (i = 0; i < BTA_DM_NUM_PM_TIMER; i++) {
            if (!bta_dm_cb.pm_timer[i].in_use) {
                bta_dm_cb.pm_timer[i].in_use = TRUE;
                bdcpy(bta_dm_cb.pm_timer[i].peer_bdaddr, peer_addr);
                bta_dm_cb.pm_timer[i].timer.p_cback = bta_dm_pm_timer_cback;
                bta_sys_start_timer(&bta_dm_cb.pm_timer[i].timer, 0, timeout);
                return;
            }
        }

        if (i == BTA_DM_NUM_PM_TIMER) {
            APPL_TRACE_WARNING0("bta_dm_act no more pm timers");
            return;
        }
    }

    if (pm_action == BTA_DM_PM_NO_ACTION) {
    } else if (pm_action == BTA_DM_PM_PARK) {
        p_peer_device->pm_mode_attempted = BTA_DM_PM_PARK;
        bta_dm_pm_park(peer_addr);
    } else if (pm_action == BTA_DM_PM_SNIFF) {
        p_peer_device->pm_mode_attempted = BTA_DM_PM_SNIFF;
        bta_dm_pm_sniff(peer_addr);
    } else if (pm_action == BTA_DM_PM_ACTIVE) {
        bta_dm_pm_active(peer_addr);
    }
}

/* 0x8047E3DC (0x78): Posts the power mode change BTM reports for `bd_addr`. */
static void bta_dm_pm_btm_cback(const u8* bd_addr, u32 status, u16 value, u8 hci_status) {
    tBTA_DM_PM_BTM_STATUS* p_buf;

    if ((p_buf = (tBTA_DM_PM_BTM_STATUS*)GKI_getbuf(sizeof(tBTA_DM_PM_BTM_STATUS))) != NULL) {
        p_buf->hdr.event = BTA_DM_PM_BTM_STATUS_EVT;
        p_buf->status = status;
        p_buf->value = value;
        p_buf->hci_status = hci_status;
        bdcpy(p_buf->bd_addr, bd_addr);
        bta_sys_sendmsg(&p_buf->hdr);
    }
}

/* 0x8047E454 (0xF8): Frees the power manager timer `p_tle` and posts its expiry for its peer. */
static void bta_dm_pm_timer_cback(TIMER_LIST_ENT* p_tle) {
    tBTA_DM_PM_TIMER* p_buf;
    u8 i;

    for (i = 0; i < BTA_DM_NUM_PM_TIMER; i++) {
        if (bta_dm_cb.pm_timer[i].in_use && &bta_dm_cb.pm_timer[i].timer == p_tle) {
            bta_dm_cb.pm_timer[i].in_use = FALSE;
            break;
        }
    }

    if (i == BTA_DM_NUM_PM_TIMER) {
        return;
    }

    if ((p_buf = (tBTA_DM_PM_TIMER*)GKI_getbuf(sizeof(tBTA_DM_PM_TIMER))) != NULL) {
        p_buf->hdr.event = BTA_DM_PM_TIMER_EVT;
        bdcpy(p_buf->bd_addr, bta_dm_cb.pm_timer[i].peer_bdaddr);
        bta_sys_sendmsg(&p_buf->hdr);
    }
}

/* 0x8047E54C (0x120): Stops the peer's power manager timer; on a return to active mode, records a failed mode
 * attempt or reselects the power mode. */
void bta_dm_pm_btm_status(tBTA_DM_MSG* p_data) {
    u8 i;

    for (i = 0; i < BTA_DM_NUM_PM_TIMER; i++) {
        if (bta_dm_cb.pm_timer[i].in_use && !bdcmp(bta_dm_cb.pm_timer[i].peer_bdaddr, p_data->pm_status.bd_addr)) {
            bta_sys_stop_timer(&bta_dm_cb.pm_timer[i].timer);
            bta_dm_cb.pm_timer[i].in_use = FALSE;
            break;
        }
    }

    switch (p_data->pm_status.status) {
    case BTM_PM_STS_ACTIVE:
        if (p_data->pm_status.hci_status) {
            for (i = 0; i < bta_dm_cb.device_list.count; i++) {
                if (!bdcmp(bta_dm_cb.device_list.peer_device[i].peer_bdaddr, p_data->pm_status.bd_addr)) {
                    if (bta_dm_cb.device_list.peer_device[i].pm_mode_attempted & (BTA_DM_PM_PARK | BTA_DM_PM_SNIFF)) {
                        bta_dm_cb.device_list.peer_device[i].pm_mode_failed |=
                            bta_dm_cb.device_list.peer_device[i].pm_mode_attempted & (BTA_DM_PM_PARK | BTA_DM_PM_SNIFF);
                        bta_dm_pm_set_mode(p_data->pm_status.bd_addr, FALSE);
                    }
                    break;
                }
            }
        } else {
            bta_dm_pm_set_mode(p_data->pm_status.bd_addr, FALSE);
        }
        break;
    default:
        break;
    }
}

/* 0x8047E66C (0xC): Reselects the power mode of the peer whose timer expired. */
void bta_dm_pm_timer(tBTA_DM_MSG* p_data) {
    bta_dm_pm_set_mode(p_data->pm_timer.bd_addr, TRUE);
}
