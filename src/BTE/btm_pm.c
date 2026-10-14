/*
 * BTE/btm_pm.c - the BTM power management: client registration, set/read power mode, the mode state machine, the HCI
 *   mode request and the command status and mode change handlers.
 *
 * RANGE. .text 0x80486450-0x804870E4 (10 functions, 0xC94 B); .rodata 0x80573840-0x80573850; .data
 *   0x806143F0-0x80614430; .sdata2 0x8079D0C0-0x8079D0C8. The start is medium: the .data starts after a 4-byte pad
 *   beyond 4-byte alignment (0x806143EC -> 0x806143F0), allowing a cut in [0x8048610C, 0x80486EBC]; the BTM_Pm*
 *   family and the BTE source order put it at BTM_PmRegister, after btm_init. The end is pinned by the readers:
 *   btm_pm_proc_mode_change (the last function) reads 0x806143F0 and btm_sco_init reads the next .rodata object
 *   0x80573850, so 0x804870E4 is the only cut that keeps both sections in order. The .rodata table is read only by
 *   btm_pm_compare_modes and the .sdata2 words only by BTM_SetPowerMode.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`. Measured on scratch compiles: under GC/3.0a5.2 +
 *   `cflags_nw4r` (as BTE/l2c_*.c, BTE/sdp_*.c) every row compiles to the target; Wii/1.3 `cflags_os` shortens
 *   BTM_SetPowerMode, btm_pm_compare_modes, btm_pm_get_set_mode and btm_pm_snd_md_req.
 * NAMES. the map's; the static tables btm_pm_md_comp_matrix, btm_pm_mode_off and btm_pm_mode_msk are the BTE
 *   source's names.
 * RESIDUALS. none known.
 * SHAPES. plain C; the ACL index search is a static helper the compiler inlines into its two callers.
 */

#include "BTE/btm_pm.h"
#include "BTE/btm_main.h"
#include "BTE/btm_acl.h"
#include "BTE/hcicmds.h"
#include "BTE/btm_sco.h"
#include "BTE/btm_devctl.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTM status codes. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_NO_RESOURCES 3
#define BTM_MODE_UNSUPPORTED 4
#define BTM_ILLEGAL_VALUE 5
#define BTM_UNKNOWN_ADDR 7
#define BTM_CMD_STORED 13

/* The power modes; the force bit asks for a mode as given, without merging the other clients' requests. */
#define BTM_PM_MD_ACTIVE 0
#define BTM_PM_MD_HOLD 1
#define BTM_PM_MD_SNIFF 2
#define BTM_PM_MD_PARK 3
#define BTM_PM_MD_FORCE 0x10
#define BTM_PM_NUM_SET_MODES 3

/* The power mode states the mode change callbacks report. */
#define BTM_PM_STS_PENDING 4
#define BTM_PM_STS_ERROR 5
#define BTM_DEV_RESET 12

/* The registration mask bits and the id of a request from a party that did not register. */
#define BTM_PM_REC_NOT_USED 0
#define BTM_PM_REG_SET 1
#define BTM_PM_REG_NOTIF 2
#define BTM_PM_DEREG 4
#define BTM_PM_SET_ONLY_ID 0x80

/* The results of comparing two requested modes. */
#define BTM_PM_GET_MD1 1
#define BTM_PM_GET_MD2 2
#define BTM_PM_GET_COMP 3

/* The role switch states of an ACL link. */
#define BTM_ACL_SWKEY_STATE_IDLE 0
#define BTM_ACL_SWKEY_STATE_MODE_CHANGE 1
#define BTM_ACL_SWKEY_STATE_ENCRYPTION_OFF 2

/* The security record flag of an encrypted link. */
#define BTM_SEC_ENCRYPTED 0x04

/* The GKI pool HCI commands are built in. */
#define HCI_CMD_POOL_ID 2

/* The trace levels and trace masks of the BTM layer. */
#define BT_TRACE_LEVEL_DEBUG 5
#define TRACE_BTM_DEBUG 0x000D0004


/* Which of two requested modes wins, indexed by (mode1 - 1) * 3 + (mode2 - 1). */
static const u8 btm_pm_md_comp_matrix[BTM_PM_NUM_SET_MODES * BTM_PM_NUM_SET_MODES] = {
    BTM_PM_GET_COMP, BTM_PM_GET_MD2, BTM_PM_GET_MD2,
    BTM_PM_GET_MD1, BTM_PM_GET_COMP, BTM_PM_GET_MD1,
    BTM_PM_GET_MD1, BTM_PM_GET_MD2, BTM_PM_GET_COMP,
};

/* The local feature byte and bit of the hold, sniff and park modes. */
static const u8 btm_pm_mode_off[BTM_PM_NUM_SET_MODES] = {0, 0, 1};
static const u8 btm_pm_mode_msk[BTM_PM_NUM_SET_MODES] = {0x40, 0x80, 0x01};

/* 0x80486450 (0xBC): Registers a power mode client (or removes the one at `*p_pm_id`) and returns its id. */
u8 BTM_PmRegister(u8 mask, u8* p_pm_id, BtmPmCallback p_cb) {
    int xx;

    if (mask & BTM_PM_DEREG) {
        if (*p_pm_id >= BTM_MAX_PM_RECORDS) {
            return BTM_ILLEGAL_VALUE;
        }
        btm_cb.pm_reg_db[*p_pm_id].mask = BTM_PM_REC_NOT_USED;
        return BTM_SUCCESS;
    }

    for (xx = 0; xx < BTM_MAX_PM_RECORDS; xx++) {
        if (btm_cb.pm_reg_db[xx].mask == BTM_PM_REC_NOT_USED) {
            if (mask & BTM_PM_REG_NOTIF) {
                if (p_cb == NULL) {
                    return BTM_ILLEGAL_VALUE;
                }
                btm_cb.pm_reg_db[xx].cback = p_cb;
            }
            btm_cb.pm_reg_db[xx].mask = mask;
            *p_pm_id = xx;
            return BTM_SUCCESS;
        }
    }

    return BTM_NO_RESOURCES;
}

/* Returns the index of the ACL link to `remote_bda`, or BTM_MAX_ACL_LINKS. */
static u8 btm_pm_find_acl_ind(const u8* remote_bda) {
    tACL_CONN* p = &btm_cb.acl_db[0];
    u8 xx;

    for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
        if (p->in_use && !memcmp(p->remote_addr, remote_bda, 6)) {
            break;
        }
    }
    return xx;
}

/* 0x8048650C (0x1D0): Records client `pm_id`'s requested mode for the link to `remote_bda` and asks the controller
 * for the merged mode unless a request is already pending. */
u8 BTM_SetPowerMode(u8 pm_id, const u8* remote_bda, BtmPmPwrMode* p_mode) {
    u8 mode;
    int acl_ind;
    tBTM_PM_MCB* p_cb;
    int ind;
    u8* p_features;

    if (pm_id >= BTM_MAX_PM_RECORDS) {
        pm_id = BTM_PM_SET_ONLY_ID;
    }

    if (p_mode == NULL) {
        return BTM_ILLEGAL_VALUE;
    }

    mode = p_mode->mode & ~BTM_PM_MD_FORCE;

    acl_ind = btm_pm_find_acl_ind(remote_bda);
    if (acl_ind == BTM_MAX_ACL_LINKS) {
        return BTM_UNKNOWN_ADDR;
    }

    p_cb = &btm_cb.pm_mode_db[acl_ind];

    if (mode != BTM_PM_MD_ACTIVE) {
        ind = mode - BTM_PM_MD_HOLD;
        p_features = BTM_ReadLocalFeatures();
        if (!(p_features[btm_pm_mode_off[ind]] & btm_pm_mode_msk[ind])) {
            return BTM_MODE_UNSUPPORTED;
        }
    }

    if (mode == p_cb->state) {
        if (mode == BTM_PM_MD_ACTIVE ||
            (p_mode->maxInterval >= p_cb->interval && p_mode->minInterval <= p_cb->interval)) {
            return BTM_SUCCESS;
        }
    }

    if (pm_id != BTM_PM_SET_ONLY_ID && (btm_cb.pm_reg_db[pm_id].mask & BTM_PM_REG_SET)) {
        p_cb->req_mode[pm_id] = *p_mode;
        p_cb->chg_ind = TRUE;
    }

    if (p_cb->state == BTM_PM_MD_HOLD || p_cb->state == BTM_PM_STS_PENDING ||
        btm_cb.pm_pend_link != BTM_MAX_ACL_LINKS) {
        return BTM_CMD_STORED;
    }

    return btm_pm_snd_md_req(pm_id, acl_ind, p_mode);
}

/* 0x804866DC (0xB8): Writes the power mode of the link to `remote_bda` to `p_mode`; returns the BTM status. */
u8 BTM_ReadPowerMode(u8* remote_bda, u8* p_mode) {
    int acl_ind;

    acl_ind = btm_pm_find_acl_ind(remote_bda);
    if (acl_ind == BTM_MAX_ACL_LINKS) {
        return BTM_UNKNOWN_ADDR;
    }

    *p_mode = btm_cb.pm_mode_db[acl_ind].state;
    return BTM_SUCCESS;
}

/* 0x80486794 (0x78): Clears the pending request and the client registrations after a controller reset, telling the
 * pending client. */
void btm_pm_reset(void) {
    int xx;
    BtmPmCallback cb = NULL;

    if (btm_cb.pm_pend_id != BTM_PM_SET_ONLY_ID && (btm_cb.pm_reg_db[btm_cb.pm_pend_id].mask & BTM_PM_REG_NOTIF)) {
        cb = btm_cb.pm_reg_db[btm_cb.pm_pend_id].cback;
    }

    btm_cb.pm_pend_link = BTM_MAX_ACL_LINKS;

    for (xx = 0; xx < BTM_MAX_PM_RECORDS; xx++) {
        btm_cb.pm_reg_db[xx].mask = BTM_PM_REC_NOT_USED;
    }

    if (cb != NULL) {
        (*cb)(btm_cb.acl_db[btm_cb.pm_pend_link].remote_addr, BTM_PM_STS_ERROR, BTM_DEV_RESET, 0);
    }
}

/* 0x8048680C (0x50): Clears the power mode state of the link at `ind`. */
void btm_pm_sm_alloc(u8 ind) {
    tBTM_PM_MCB* p_db = &btm_cb.pm_mode_db[ind];

    memset(p_db, 0, sizeof(tBTM_PM_MCB));
    p_db->state = BTM_PM_MD_ACTIVE;
}

/* 0x8048685C (0x21C): Merges two requested modes into `p_res`; returns the winner, `p_res` for a merge, or NULL when
 * they conflict. */
BtmPmPwrMode* btm_pm_compare_modes(BtmPmPwrMode* p_md1, BtmPmPwrMode* p_md2, BtmPmPwrMode* p_res) {
    u8 res;

    if (p_md1 == NULL) {
        *p_res = *p_md2;
        return p_md2;
    }

    if (p_md2->mode == BTM_PM_MD_ACTIVE || p_md1->mode == BTM_PM_MD_ACTIVE) {
        return NULL;
    }

    if (p_md1->mode & BTM_PM_MD_FORCE) {
        *p_res = *p_md1;
        p_res->mode &= ~BTM_PM_MD_FORCE;
        return p_res;
    }

    if (p_md2->mode & BTM_PM_MD_FORCE) {
        *p_res = *p_md2;
        p_res->mode &= ~BTM_PM_MD_FORCE;
        return p_res;
    }

    res = (p_md1->mode - 1) * BTM_PM_NUM_SET_MODES + (p_md2->mode - 1);
    res = btm_pm_md_comp_matrix[res];
    switch (res) {
    case BTM_PM_GET_MD1:
        *p_res = *p_md1;
        return p_md1;

    case BTM_PM_GET_MD2:
        *p_res = *p_md2;
        return p_md2;

    case BTM_PM_GET_COMP:
        p_res->mode = p_md1->mode;
        p_res->maxInterval = (p_md1->maxInterval < p_md2->maxInterval) ? p_md1->maxInterval : p_md2->maxInterval;
        p_res->minInterval = (p_md1->minInterval > p_md2->minInterval) ? p_md1->minInterval : p_md2->minInterval;

        if (p_res->maxInterval < p_res->minInterval) {
            return NULL;
        }

        if (p_res->mode == BTM_PM_MD_SNIFF) {
            p_res->attempt = (p_md1->attempt > p_md2->attempt) ? p_md1->attempt : p_md2->attempt;
            p_res->timeout = (p_md1->timeout > p_md2->timeout) ? p_md1->timeout : p_md2->timeout;
        }
        return p_res;
    }
    return NULL;
}

/* 0x80486A78 (0x160): Computes the mode to ask the controller for from the registered clients' requests and
 * `p_mode`. */
u8 btm_pm_get_set_mode(u8 pm_id, tBTM_PM_MCB* p_cb, BtmPmPwrMode* p_mode, BtmPmPwrMode* p_res) {
    int xx;
    BtmPmPwrMode* p_md = NULL;

    if (p_mode != NULL && (p_mode->mode & BTM_PM_MD_FORCE)) {
        *p_res = *p_mode;
        p_res->mode &= ~BTM_PM_MD_FORCE;
        return p_res->mode;
    }

    for (xx = 0; xx < BTM_MAX_PM_RECORDS; xx++) {
        if (btm_cb.pm_reg_db[xx].mask & BTM_PM_REG_SET) {
            if (p_cb->req_mode[xx].mode == BTM_PM_MD_ACTIVE) {
                return BTM_PM_MD_ACTIVE;
            }
            if (btm_pm_compare_modes(p_md, &p_cb->req_mode[xx], p_res) == NULL) {
                return BTM_PM_MD_ACTIVE;
            }
            p_md = p_res;
        }
    }

    if (p_md == NULL) {
        if (p_mode != NULL) {
            *p_res = *p_mode;
        } else {
            return BTM_PM_MD_ACTIVE;
        }
    } else if (pm_id == BTM_PM_SET_ONLY_ID) {
        if (btm_pm_compare_modes(p_mode, p_md, p_res) == NULL) {
            return BTM_PM_MD_ACTIVE;
        }
    }

    return p_res->mode;
}

/* 0x80486BD8 (0x234): Sends the HCI command that moves the link at `link_ind` to the merged mode (waking it first
 * when needed). */
u8 btm_pm_snd_md_req(u8 pm_id, int link_ind, BtmPmPwrMode* p_mode) {
    BtmPmPwrMode md_res;
    u8 mode;
    tBTM_PM_MCB* p_cb = &btm_cb.pm_mode_db[link_ind];
    u8 chg_ind = FALSE;
    BT_HDR* p_buf;

    mode = btm_pm_get_set_mode(pm_id, p_cb, p_mode, &md_res);
    md_res.mode = mode;

    if (p_cb->state == mode) {
        if (mode == BTM_PM_MD_ACTIVE ||
            (md_res.maxInterval >= p_cb->interval && md_res.minInterval <= p_cb->interval)) {
            return BTM_CMD_STORED;
        }
        chg_ind = TRUE;
    }
    p_cb->chg_ind = chg_ind;

    if (mode != BTM_PM_MD_ACTIVE && p_cb->state != BTM_PM_MD_ACTIVE) {
        p_cb->chg_ind = TRUE;
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) == NULL) {
        return BTM_NO_RESOURCES;
    }

    if (p_cb->chg_ind == TRUE) {
        md_res.mode = BTM_PM_MD_ACTIVE;
    }

    btm_cb.pm_pend_link = link_ind;
    btm_cb.pm_pend_id = pm_id;

    switch (md_res.mode) {
    case BTM_PM_MD_ACTIVE:
        switch (p_cb->state) {
        case BTM_PM_MD_SNIFF:
            btsnd_hcic_exit_sniff_mode(p_buf, btm_cb.acl_db[link_ind].hci_handle);
            break;
        case BTM_PM_MD_PARK:
            btsnd_hcic_exit_park_mode(p_buf, btm_cb.acl_db[link_ind].hci_handle);
            break;
        default:
            btm_cb.pm_pend_link = BTM_MAX_ACL_LINKS;
            break;
        }
        break;

    case BTM_PM_MD_HOLD:
        btsnd_hcic_hold_mode(p_buf, btm_cb.acl_db[link_ind].hci_handle, md_res.maxInterval, md_res.minInterval);
        break;

    case BTM_PM_MD_SNIFF:
        btsnd_hcic_sniff_mode(p_buf, btm_cb.acl_db[link_ind].hci_handle, md_res.maxInterval, md_res.minInterval,
                              md_res.attempt, md_res.timeout);
        break;

    case BTM_PM_MD_PARK:
        btsnd_hcic_park_mode(p_buf, btm_cb.acl_db[link_ind].hci_handle, md_res.maxInterval, md_res.minInterval);
        break;

    default:
        btm_cb.pm_pend_link = BTM_MAX_ACL_LINKS;
        break;
    }

    if (btm_cb.pm_pend_link == BTM_MAX_ACL_LINKS) {
        GKI_freebuf(p_buf);
    }

    return BTM_CMD_STARTED;
}

/* 0x80486E0C (0xB0): Handles the command status of a mode request: marks the link pending and tells the requesting
 * client. */
void btm_pm_proc_cmd_status(u8 status) {
    tBTM_PM_MCB* p_cb;
    u8 pm_status;

    if (btm_cb.pm_pend_link >= BTM_MAX_ACL_LINKS) {
        return;
    }

    p_cb = &btm_cb.pm_mode_db[btm_cb.pm_pend_link];

    if (status == 0) {
        p_cb->state = BTM_PM_STS_PENDING;
        pm_status = BTM_PM_STS_PENDING;
    } else {
        pm_status = BTM_PM_STS_ERROR;
    }

    if (btm_cb.pm_pend_id != BTM_PM_SET_ONLY_ID && (btm_cb.pm_reg_db[btm_cb.pm_pend_id].mask & BTM_PM_REG_NOTIF)) {
        (*btm_cb.pm_reg_db[btm_cb.pm_pend_id].cback)(btm_cb.acl_db[btm_cb.pm_pend_link].remote_addr, pm_status, 0,
                                                    status);
    }

    btm_cb.pm_pend_link = BTM_MAX_ACL_LINKS;
}

/* 0x80486EBC (0x228): Handles a mode change event: restores the packet types after unsniffing, records the new mode,
 * sends any pending request, tells the clients and continues a role switch waiting for the link to wake. */
void btm_pm_proc_mode_change(u8 hci_status, u16 hci_handle, u8 mode, u16 interval) {
    tACL_CONN* p;
    tBTM_PM_MCB* p_cb;
    tBTM_SEC_DEV_REC* p_dev_rec;
    int xx;
    int yy;
    int zz;

    if ((xx = btm_handle_to_acl_index(hci_handle)) >= BTM_MAX_ACL_LINKS) {
        return;
    }

    p = &btm_cb.acl_db[xx];

    if (mode == BTM_PM_MD_ACTIVE && p->restore_pkt_types && !btm_num_sco_links_active()) {
        p->restore_pkt_types = 0;
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
            LogMsg_2(TRACE_BTM_DEBUG, "btm mode change AFTER unsniffing; hci hdl 0x%x, types 0x%02x", hci_handle,
                     p->pkt_types_mask);
        }
        btsnd_hcic_change_conn_type(p->hci_handle, p->pkt_types_mask);
    }

    p_cb = &btm_cb.pm_mode_db[xx];
    p_cb->state = mode;
    p_cb->interval = interval;

    for (zz = 0; zz < BTM_MAX_PM_RECORDS; zz++) {
        if (mode == BTM_PM_MD_ACTIVE && p_cb->req_mode[zz].mode == BTM_PM_MD_HOLD) {
            p_cb->req_mode[zz].mode = BTM_PM_MD_ACTIVE;
        }
    }

    if (p_cb->chg_ind == TRUE) {
        btm_pm_snd_md_req(BTM_PM_SET_ONLY_ID, xx, NULL);
    }

    for (yy = 0; yy < BTM_MAX_PM_RECORDS; yy++) {
        if (btm_cb.pm_reg_db[yy].mask & BTM_PM_REG_NOTIF) {
            (*btm_cb.pm_reg_db[yy].cback)(p->remote_addr, mode, interval, hci_status);
        }
    }

    if (btm_cb.acl_db[xx].switch_role_state == BTM_ACL_SWKEY_STATE_MODE_CHANGE) {
        p_dev_rec = btm_find_dev(btm_cb.acl_db[xx].remote_addr);
        if (p_dev_rec != NULL && (p_dev_rec->sec_flags & BTM_SEC_ENCRYPTED)) {
            if (btsnd_hcic_set_conn_encrypt(hci_handle, FALSE)) {
                btm_cb.acl_db[xx].switch_role_state = BTM_ACL_SWKEY_STATE_ENCRYPTION_OFF;
                return;
            }
        } else if (btsnd_hcic_switch_role(btm_cb.acl_db[xx].remote_addr, !btm_cb.acl_db[xx].link_role)) {
            btm_cb.acl_db[xx].switch_role_state = BTM_ACL_SWKEY_STATE_IDLE;
            return;
        }

        btm_cb.acl_db[xx].switch_role_state = BTM_ACL_SWKEY_STATE_IDLE;
        if (btm_cb.p_switch_role_cb != NULL) {
            btm_cb.switch_role_ref_data.hci_status = hci_status;
            (*btm_cb.p_switch_role_cb)(&btm_cb.switch_role_ref_data);
            btm_cb.p_switch_role_cb = NULL;
        }
    }
}
