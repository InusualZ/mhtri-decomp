/*
 * BTE/hidh_conn.c - the HID host connection layer: L2CAP registration, the security-check completions, the L2CAP
 *   callbacks (connect, config, disconnect, congestion, data), data send and connection origination.
 *
 * RANGE. .text 0x80490E54-0x80492E94 (16 functions, 0x2040 B); .rodata 0x80573870-0x80573898; .data
 *   0x806156F8-0x80615AE0. The .text holds the BTE hidh_conn.c functions hidh_conn_reg..hidh_conn_dereg; both ends are
 *   pinned by the .data readers (the start by hidh_api.c's last reader, the end by l2c_api.c's first). The .rodata is
 *   the L2CAP callback table hidh_conn_reg registers; the .data (trace strings) is read only by this range's functions
 *   and ends at the 4-byte pad before 0x80615AE0.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) twelve of its sixteen rows do not match.
 * NAMES. the map's. GUESS hst_reg_info (hst_reg_info, the BTE source's name for the L2CAP callback table).
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C; find_conn_by_cid is inlined into every L2CAP callback, hidh_conn_disconnect into the security
 *   completions and the callbacks that close a device, hidh_conn_initiate into hidh_proc_repage_timeout. The
 *   payload pointers read through BT_BUF, except hidh_conn_snd_data's memcpy source: spelled through BT_BUF it is
 *   hoisted into a saved register out of the loop and the row no longer matches.
 */

#include "BTE/hidh_conn.h"
#include "BTE/hidh_api.h"
#include "BTE/btm_sec.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "BTE/btm_acl.h"   /* btm_get_acl_disc_reason_code */

/* The L2CAP callback set registered for both HID PSMs. */
static const tL2CAP_APPL_INFO hst_reg_info = {
    hidh_l2cif_connect_ind,
    hidh_l2cif_connect_cfm,
    NULL,
    hidh_l2cif_config_ind,
    hidh_l2cif_config_cfm,
    hidh_l2cif_disconnect_ind,
    hidh_l2cif_disconnect_cfm,
    NULL,
    hidh_l2cif_data_ind,
    hidh_l2cif_cong_ind,
};

/* Returns the handle of the device one of whose channels is `cid`, or HID_HOST_MAX_DEVICES. */
static u8 find_conn_by_cid(u16 cid) {
    u8 xx;

    for (xx = 0; xx < HID_HOST_MAX_DEVICES; xx++) {
        if (hh_cb.devices[xx].in_use && hh_cb.devices[xx].conn.conn_state != HID_CONN_STATE_UNUSED
            && (hh_cb.devices[xx].conn.ctrl_cid == cid || hh_cb.devices[xx].conn.intr_cid == cid)) {
            break;
        }
    }
    return xx;
}

/* 0x80490E54 (0x158): Sets the channel configuration and registers the two HID PSMs; returns a HID result. */
u8 hidh_conn_reg(void) {
    int xx;

    hh_cb.l2cap_cfg.mtu_present = TRUE;
    hh_cb.l2cap_cfg.mtu = HID_HOST_MTU;
    hh_cb.l2cap_cfg.flush_to_present = TRUE;
    hh_cb.l2cap_cfg.flush_to = HID_HOST_FLUSH_TO;

    if (!(u8)L2CA_Register(HID_PSM_CONTROL, (tL2CAP_APPL_INFO*)&hst_reg_info)) {
        HIDH_TRACE_ERROR0("HID Control Registration failed");
        return HID_ERR_L2CAP_FAILED;
    }
    if (!(u8)L2CA_Register(HID_PSM_INTERRUPT, (tL2CAP_APPL_INFO*)&hst_reg_info)) {
        L2CA_Deregister(HID_PSM_CONTROL);
        HIDH_TRACE_ERROR0("HID Interrupt Registration failed");
        return HID_ERR_L2CAP_FAILED;
    }

    for (xx = 0; xx < HID_HOST_MAX_DEVICES; xx++) {
        hh_cb.devices[xx].in_use = FALSE;
        hh_cb.devices[xx].conn.conn_state = HID_CONN_STATE_UNUSED;
    }
    return HID_SUCCESS;
}

/* 0x80490FAC (0xA4): Closes both channels of the device `dhandle`; returns a HID result. */
u8 hidh_conn_disconnect(u8 dhandle) {
    tHID_CONN* p_hcon = &hh_cb.devices[dhandle].conn;

    HIDH_TRACE_EVENT0("HID - disconnect");

    if (p_hcon->ctrl_cid != 0 || p_hcon->intr_cid != 0) {
        p_hcon->conn_state = HID_CONN_STATE_DISCONNECTING;
        if (p_hcon->intr_cid) {
            L2CA_DisconnectReq(p_hcon->intr_cid);
        }
        if (p_hcon->ctrl_cid) {
            L2CA_DisconnectReq(p_hcon->ctrl_cid);
        }
    } else {
        p_hcon->conn_state = HID_CONN_STATE_UNUSED;
    }
    return HID_SUCCESS;
}

/* 0x80491050 (0x130): Accepts the incoming control channel once its security check passed, or closes the device. */
void hidh_sec_check_complete_term(u8* bd_addr, void* p_ref_data, u8 res) { /* untyped: a caller-owned payload */
    tHID_HOST_DEV_CTB* p_dev = (tHID_HOST_DEV_CTB*)p_ref_data;
    u8 dhandle = p_dev - hh_cb.devices;

    if (res == BTM_SUCCESS && p_dev->conn.conn_state == HID_CONN_STATE_SECURITY) {
        p_dev->conn.disc_reason = HID_SUCCESS;
        p_dev->conn.conn_state = HID_CONN_STATE_CONNECTING_INTR;
        L2CA_ConnectRsp(p_dev->addr, p_dev->conn.ctrl_id, p_dev->conn.ctrl_cid, L2CAP_CONN_OK, L2CAP_CONN_OK);
        L2CA_ConfigReq(p_dev->conn.ctrl_cid, &hh_cb.l2cap_cfg);
    } else if (res != BTM_SUCCESS) {
        p_dev->conn.disc_reason = HID_ERR_AUTH_FAILED;
        hidh_conn_disconnect(dhandle);
    }
}

/* 0x80491180 (0x258): Takes or refuses an incoming control or interrupt channel of a known device. */
void hidh_l2cif_connect_ind(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id) {
    tHID_CONN* p_hcon;
    u8 bAccept = TRUE;
    int i;
    tHID_HOST_DEV_CTB* p_dev;

    HIDH_TRACE_EVENT2("HID - Rcvd L2CAP conn ind, PSM: 0x%04x  CID 0x%x", psm, l2cap_cid);

    for (i = 0; i < HID_HOST_MAX_DEVICES; i++) {
        if (hh_cb.devices[i].in_use && !memcmp(bd_addr, hh_cb.devices[i].addr, 6)) {
            break;
        }
    }

    if (i == HID_HOST_MAX_DEVICES) {
        L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_SECURITY_BLOCK, 0);
        return;
    }

    p_dev = &hh_cb.devices[i];
    p_hcon = &hh_cb.devices[i].conn;

    if (psm == HID_PSM_INTERRUPT) {
        if (p_hcon->ctrl_cid == 0) {
            HIDH_TRACE_WARNING0("HID - Rcvd INTR L2CAP conn ind, but no CTL channel");
            bAccept = FALSE;
        }
        if (p_hcon->conn_state != HID_CONN_STATE_CONNECTING_INTR) {
            HIDH_TRACE_WARNING1("HID - Rcvd INTR L2CAP conn ind, wrong state: %d", p_hcon->conn_state);
            bAccept = FALSE;
        }
    } else if (p_hcon->conn_state != HID_CONN_STATE_UNUSED) {
        HIDH_TRACE_WARNING1("HID - Rcvd CTL L2CAP conn ind, wrong state: %d", p_hcon->conn_state);
        bAccept = FALSE;
    }

    if (!bAccept) {
        L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_NO_RESOURCES, 0);
        return;
    }

    if (psm == HID_PSM_CONTROL) {
        p_hcon->conn_flags = 0;
        p_hcon->ctrl_cid = l2cap_cid;
        p_hcon->ctrl_id = l2cap_id;
        p_hcon->disc_reason = HID_L2CAP_CONN_FAIL;
        p_hcon->conn_state = HID_CONN_STATE_SECURITY;
        btm_sec_mx_access_request(p_dev->addr, HID_PSM_CONTROL, FALSE, BTM_SEC_PROTO_HID,
                                  (p_dev->attr_mask & HID_SEC_REQUIRED) ? HID_SEC_CHN : HID_NOSEC_CHN,
                                  hidh_sec_check_complete_term, p_dev);
        return;
    }

    p_hcon->conn_state = HID_CONN_STATE_CONFIG;
    p_hcon->intr_cid = l2cap_cid;
    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_OK, L2CAP_CONN_OK);
    L2CA_ConfigReq(l2cap_cid, &hh_cb.l2cap_cfg);

    HIDH_TRACE_EVENT2("HID - Rcvd L2CAP conn ind, sent config req, PSM: 0x%04x  CID 0x%x", psm, l2cap_cid);
}

/* 0x804913D8 (0x14C): Retries the connection of the device whose handle the timer carries and reports the retry. */
void hidh_proc_repage_timeout(TIMER_LIST_ENT* p_tle) {
    hidh_conn_initiate((u8)p_tle->param);
    hh_cb.devices[p_tle->param].conn_tries++;
    hh_cb.callback((u8)p_tle->param, HID_HDEV_EVT_RETRYING, hh_cb.devices[p_tle->param].conn_tries, NULL);
}

/* 0x80491524 (0x214): Opens the interrupt channel once the outgoing control channel passed its security check, or
 * closes the device. */
void hidh_sec_check_complete_orig(u8* bd_addr, void* p_ref_data, u8 res) { /* untyped: a caller-owned payload */
    tHID_HOST_DEV_CTB* p_dev = (tHID_HOST_DEV_CTB*)p_ref_data;
    u8 dhandle = p_dev - hh_cb.devices;
    u32 reason;

    if (res == BTM_SUCCESS && p_dev->conn.conn_state == HID_CONN_STATE_SECURITY) {
        HIDH_TRACE_EVENT0("HID - Originator security pass.");
        p_dev->conn.disc_reason = HID_SUCCESS;

        if ((p_dev->conn.intr_cid = L2CA_ConnectReq(HID_PSM_INTERRUPT, hh_cb.devices[dhandle].addr)) == 0) {
            HIDH_TRACE_WARNING0("HID - INTR Originate failed");
            reason = HID_L2CAP_REQ_FAIL;
            hidh_conn_disconnect(dhandle);
            hh_cb.callback(dhandle, HID_HDEV_EVT_CLOSE, reason, NULL);
            return;
        }
        p_dev->conn.conn_state = HID_CONN_STATE_CONNECTING_INTR;
    }

    if (res != BTM_SUCCESS && p_dev->conn.conn_state == HID_CONN_STATE_SECURITY) {
        p_dev->conn.disc_reason = HID_ERR_AUTH_FAILED;
        hidh_conn_disconnect(dhandle);
    }
}

/* 0x80491738 (0x348): Takes the result of an outgoing channel: checks the control channel's security or configures
 * the interrupt channel. */
void hidh_l2cif_connect_cfm(u16 l2cap_cid, u16 result) {
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;
    u32 reason;
    tHID_HOST_DEV_CTB* p_dev = NULL;

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_dev = &hh_cb.devices[dhandle];
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL || !(p_hcon->conn_flags & HID_CONN_FLAGS_IS_ORIG)
        || (l2cap_cid == p_hcon->ctrl_cid && p_hcon->conn_state != HID_CONN_STATE_CONNECTING_CTRL)
        || (l2cap_cid == p_hcon->intr_cid && p_hcon->conn_state != HID_CONN_STATE_CONNECTING_INTR)) {
        HIDH_TRACE_WARNING1("HID - Rcvd unexpected conn cnf, CID 0x%x ", l2cap_cid);
        return;
    }

    if (result != L2CAP_CONN_OK) {
        if (l2cap_cid == p_hcon->ctrl_cid) {
            p_hcon->ctrl_cid = 0;
        } else {
            p_hcon->intr_cid = 0;
        }
        hidh_conn_disconnect(dhandle);
        reason = HID_L2CAP_CONN_FAIL | (u32)result;
        hh_cb.callback(dhandle, HID_HDEV_EVT_CLOSE, reason, NULL);
        return;
    }

    if (l2cap_cid == p_hcon->ctrl_cid) {
        p_hcon->conn_state = HID_CONN_STATE_SECURITY;
        p_hcon->disc_reason = HID_L2CAP_CONN_FAIL;
        btm_sec_mx_access_request(p_dev->addr, HID_PSM_CONTROL, TRUE, BTM_SEC_PROTO_HID,
                                  (p_dev->attr_mask & HID_SEC_REQUIRED) ? HID_SEC_CHN : HID_NOSEC_CHN,
                                  hidh_sec_check_complete_orig, p_dev);
    } else {
        p_hcon->conn_state = HID_CONN_STATE_CONFIG;
    }

    L2CA_ConfigReq(l2cap_cid, &hh_cb.l2cap_cfg);

    HIDH_TRACE_EVENT1("HID - got CTRL conn cnf, sent cfg req, CID: 0x%x", l2cap_cid);
}

/* 0x80491A80 (0x2A8): Accepts the peer's configuration of a channel and reports the device open once both channels
 * are configured both ways. */
void hidh_l2cif_config_ind(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg) {
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL) {
        HIDH_TRACE_WARNING1("HID - Rcvd L2CAP cfg ind, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    HIDH_TRACE_EVENT1("HID - Rcvd cfg ind, sent cfg cfm, CID: 0x%x", l2cap_cid);

    if (!p_cfg->mtu_present || p_cfg->mtu > HID_HOST_MTU) {
        p_hcon->rem_mtu_size = HID_HOST_MTU;
    } else {
        p_hcon->rem_mtu_size = p_cfg->mtu;
    }

    p_cfg->flush_to_present = FALSE;
    p_cfg->mtu_present = FALSE;
    p_cfg->result = L2CAP_CFG_OK;

    L2CA_ConfigRsp(l2cap_cid, p_cfg);

    if (l2cap_cid == p_hcon->ctrl_cid) {
        p_hcon->conn_flags |= HID_CONN_FLAGS_HIS_CTRL_CFG_DONE;
    } else {
        p_hcon->conn_flags |= HID_CONN_FLAGS_HIS_INTR_CFG_DONE;
    }

    if ((p_hcon->conn_flags & HID_CONN_FLAGS_ALL_CONFIGURED) == HID_CONN_FLAGS_ALL_CONFIGURED
        && p_hcon->conn_state == HID_CONN_STATE_CONFIG) {
        p_hcon->conn_state = HID_CONN_STATE_CONNECTED;
        hh_cb.devices[dhandle].state = HID_DEV_CONNECTED;
        hh_cb.callback(dhandle, HID_HDEV_EVT_OPEN, 0, NULL);
    }
}

/* 0x80491D28 (0x30C): Takes the answer to our configuration of a channel: closes the device on a refusal, else reports
 * it open once both channels are configured both ways. */
void hidh_l2cif_config_cfm(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg) {
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;
    u32 reason;

    HIDH_TRACE_EVENT2("HID - Rcvd cfg cfm, CID: 0x%x  Result: %d", l2cap_cid, p_cfg->result);

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL) {
        HIDH_TRACE_WARNING1("HID - Rcvd L2CAP cfg ind, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    if (p_cfg->result != L2CAP_CFG_OK) {
        hidh_conn_disconnect(dhandle);
        reason = HID_L2CAP_CFG_FAIL | (u32)p_cfg->result;
        hh_cb.callback(dhandle, HID_HDEV_EVT_CLOSE, reason, NULL);
        return;
    }

    if (l2cap_cid == p_hcon->ctrl_cid) {
        p_hcon->conn_flags |= HID_CONN_FLAGS_MY_CTRL_CFG_DONE;
    } else {
        p_hcon->conn_flags |= HID_CONN_FLAGS_MY_INTR_CFG_DONE;
    }

    if ((p_hcon->conn_flags & HID_CONN_FLAGS_ALL_CONFIGURED) == HID_CONN_FLAGS_ALL_CONFIGURED
        && p_hcon->conn_state == HID_CONN_STATE_CONFIG) {
        p_hcon->conn_state = HID_CONN_STATE_CONNECTED;
        hh_cb.devices[dhandle].state = HID_DEV_CONNECTED;
        hh_cb.callback(dhandle, HID_HDEV_EVT_OPEN, 0, NULL);
    }
}

/* 0x80492034 (0x2C4): Takes the peer's close of a channel and reports the device closed once both are gone. */
void hidh_l2cif_disconnect_ind(u16 l2cap_cid, u8 ack_needed) {
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;
    u16 disc_res = 0;
    u16 hid_close_evt_reason;

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL) {
        HIDH_TRACE_WARNING1("HID - Rcvd L2CAP disc, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    if (ack_needed) {
        L2CA_DisconnectRsp(l2cap_cid);
    }

    HIDH_TRACE_EVENT1("HID - Rcvd L2CAP disc, CID: 0x%x", l2cap_cid);

    p_hcon->conn_state = HID_CONN_STATE_DISCONNECTING;

    if (l2cap_cid == p_hcon->ctrl_cid) {
        p_hcon->ctrl_cid = 0;
    } else {
        p_hcon->intr_cid = 0;
    }

    if (p_hcon->ctrl_cid == 0 && p_hcon->intr_cid == 0) {
        hh_cb.devices[dhandle].state = HID_DEV_NO_CONN;
        p_hcon->conn_state = HID_CONN_STATE_UNUSED;

        if (!ack_needed) {
            disc_res = btm_get_acl_disc_reason_code();
        }

        hid_close_evt_reason = p_hcon->disc_reason;
        if (disc_res == HCI_ERR_AUTH_FAILURE || disc_res == HCI_ERR_KEY_MISSING
            || disc_res == HCI_ERR_HOST_REJECT_SECURITY || disc_res == HCI_ERR_PAIRING_NOT_ALLOWED
            || disc_res == HCI_ERR_UNIT_KEY_USED || disc_res == HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED
            || disc_res == HCI_ERR_ENCRY_MODE_NOT_ACCEPTABLE || disc_res == HCI_ERR_REPEATED_ATTEMPTS) {
            hid_close_evt_reason = HID_ERR_AUTH_FAILED;
        }

        hh_cb.callback(dhandle, HID_HDEV_EVT_CLOSE, hid_close_evt_reason, NULL);
    }
}

/* 0x804922F8 (0x248): Takes the confirmation of our close of a channel and reports the device closed once both are
 * gone. */
void hidh_l2cif_disconnect_cfm(u16 l2cap_cid, u16 result) {
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL) {
        HIDH_TRACE_WARNING1("HID - Rcvd L2CAP disc cfm, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    HIDH_TRACE_EVENT1("HID - Rcvd L2CAP disc cfm, CID: 0x%x", l2cap_cid);

    if (l2cap_cid == p_hcon->ctrl_cid) {
        p_hcon->ctrl_cid = 0;
    } else {
        p_hcon->intr_cid = 0;
    }

    if (p_hcon->ctrl_cid == 0 && p_hcon->intr_cid == 0) {
        hh_cb.devices[dhandle].state = HID_DEV_NO_CONN;
        p_hcon->conn_state = HID_CONN_STATE_UNUSED;
        hh_cb.callback(dhandle, HID_HDEV_EVT_CLOSE, p_hcon->disc_reason, NULL);
    }
}

/* 0x80492540 (0x1F8): Records whether a channel is congested. */
void hidh_l2cif_cong_ind(u16 l2cap_cid, u8 congested) {
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL) {
        HIDH_TRACE_WARNING1("HID - Rcvd L2CAP congestion status, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    HIDH_TRACE_EVENT2("HID - Rcvd L2CAP congestion status, CID: 0x%x  Cong: %d", l2cap_cid, congested);

    if (congested) {
        p_hcon->conn_flags |= HID_CONN_FLAGS_CONGESTED;
    } else {
        p_hcon->conn_flags &= ~HID_CONN_FLAGS_CONGESTED;
    }
}

/* 0x80492738 (0x398): Strips the transaction header of a received buffer and passes it to the device callback. */
void hidh_l2cif_data_ind(u16 l2cap_cid, BT_HDR* p_msg) {
    u8* p_data = ((BT_BUF*)p_msg)->data + p_msg->offset;
    u8 ttype;
    u8 param;
    u8 rep_type;
    u8 evt;
    u8 dhandle;
    tHID_CONN* p_hcon = NULL;

    if ((dhandle = find_conn_by_cid(l2cap_cid)) != HID_HOST_MAX_DEVICES) {
        p_hcon = &hh_cb.devices[dhandle].conn;
    }

    if (p_hcon == NULL) {
        HIDH_TRACE_WARNING1("HID - Rcvd L2CAP data, unknown CID: 0x%x", l2cap_cid);
        GKI_freebuf(p_msg);
        return;
    }

    ttype = (*p_data >> 4) & 0x0F;
    param = *p_data & 0x0F;
    rep_type = param & HID_PAR_REP_TYPE_MASK;
    p_data++;

    p_msg->len--;
    p_msg->offset++;

    switch (ttype) {
    case HID_TRANS_HANDSHAKE:
        hh_cb.callback(dhandle, HID_HDEV_EVT_HANDSHAKE, param, NULL);
        GKI_freebuf(p_msg);
        break;
    case HID_TRANS_CONTROL:
        switch (param) {
        case HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG:
            hidh_conn_disconnect(dhandle);
            hh_cb.callback(dhandle, HID_HDEV_EVT_VC_UNPLUG, 0, NULL);
            break;
        default:
            break;
        }
        GKI_freebuf(p_msg);
        break;
    case HID_TRANS_DATA:
        evt = (hh_cb.devices[dhandle].conn.intr_cid == l2cap_cid) ? HID_HDEV_EVT_INTR_DATA : HID_HDEV_EVT_CTRL_DATA;
        hh_cb.callback(dhandle, evt, rep_type, p_msg);
        break;
    case HID_TRANS_DATAC:
        evt = (hh_cb.devices[dhandle].conn.intr_cid == l2cap_cid) ? HID_HDEV_EVT_INTR_DATC : HID_HDEV_EVT_CTRL_DATC;
        hh_cb.callback(dhandle, evt, rep_type, p_msg);
        break;
    default:
        GKI_freebuf(p_msg);
        break;
    }
}

/* 0x80492AD0 (0x29C): Sends a transaction to the device `dhandle`, segmenting `buf` to the channel MTU; returns a HID
 * result. */
u8 hidh_conn_snd_data(u8 dhandle, u8 trans_type, u8 param, u16 data, u8 report_id, BT_HDR* buf) {
    tHID_CONN* p_hcon = &hh_cb.devices[dhandle].conn;
    BT_HDR* p_buf;
    u8* p_out;
    u16 bytes_copied;
    u8 seg_req = FALSE;
    u16 data_size;
    u16 cid;
    u8 pool_id;
    u8 use_data = 0;
    u8 blank_datc = FALSE;

    if (p_hcon->conn_flags & HID_CONN_FLAGS_CONGESTED) {
        if (buf) {
            GKI_freebuf(buf);
        }
        return HID_ERR_CONGESTED;
    }

    switch (trans_type) {
    case HID_TRANS_CONTROL:
    case HID_TRANS_GET_REPORT:
    case HID_TRANS_SET_REPORT:
    case HID_TRANS_GET_PROTOCOL:
    case HID_TRANS_SET_PROTOCOL:
    case HID_TRANS_GET_IDLE:
    case HID_TRANS_SET_IDLE:
        cid = p_hcon->ctrl_cid;
        pool_id = HID_CONTROL_POOL_ID;
        break;
    case HID_TRANS_DATA:
        cid = p_hcon->intr_cid;
        pool_id = HID_INTERRUPT_POOL_ID;
        break;
    default:
        return HID_ERR_INVALID_PARAM;
    }

    if (trans_type == HID_TRANS_SET_IDLE) {
        use_data = 1;
    } else if (trans_type == HID_TRANS_GET_REPORT && (param & HID_PAR_GET_REP_BUFSIZE_FOLLOWS)) {
        use_data = 2;
    }

    do {
        if (buf == NULL || blank_datc) {
            if ((p_buf = (BT_HDR*)GKI_getpoolbuf(pool_id)) == NULL) {
                return HID_ERR_NO_RESOURCES;
            }
            p_buf->offset = L2CAP_MIN_OFFSET;
            seg_req = FALSE;
            data_size = 0;
            bytes_copied = 0;
            blank_datc = FALSE;
        } else if (buf->len > p_hcon->rem_mtu_size - 1) {
            if ((p_buf = (BT_HDR*)GKI_getpoolbuf(pool_id)) == NULL) {
                return HID_ERR_NO_RESOURCES;
            }
            p_buf->offset = L2CAP_MIN_OFFSET;
            seg_req = TRUE;
            data_size = buf->len;
            bytes_copied = p_hcon->rem_mtu_size - 1;
        } else {
            p_buf = buf;
            p_buf->offset -= 1;
            seg_req = FALSE;
            data_size = buf->len;
            bytes_copied = buf->len;
        }

        p_out = ((BT_BUF*)p_buf)->data + p_buf->offset;
        *p_out++ = (trans_type << 4) | (param & 0x0F);

        if (trans_type == HID_TRANS_GET_REPORT && report_id) {
            *p_out = report_id;
            data_size = bytes_copied = 1;
        }

        if (seg_req) {
            memcpy(p_out, (u8*)(buf + 1) + buf->offset, bytes_copied);
            buf->offset += bytes_copied;
            buf->len -= bytes_copied;
        } else if (use_data == 1) {
            *(p_out + bytes_copied) = data & 0xFF;
        } else if (use_data == 2) {
            *(p_out + bytes_copied) = data & 0xFF;
            *(p_out + bytes_copied + 1) = (data >> 8) & 0xFF;
        }

        p_buf->len = bytes_copied + 1 + use_data;
        data_size -= bytes_copied;

        if ((p_hcon->conn_flags & HID_CONN_FLAGS_CONGESTED) || !L2CA_DataWrite(cid, p_buf)) {
            return HID_ERR_CONGESTED;
        }

        if (data_size) {
            trans_type = HID_TRANS_DATAC;
        } else if (bytes_copied == p_hcon->rem_mtu_size - 1) {
            trans_type = HID_TRANS_DATAC;
            blank_datc = TRUE;
        }
    } while (data_size != 0 || blank_datc);

    return HID_SUCCESS;
}

/* 0x80492D6C (0xFC): Opens the control channel to the device `dhandle`, or reports the device closed when L2CAP
 * refuses; returns a HID result. */
u8 hidh_conn_initiate(u8 dhandle) {
    tHID_HOST_DEV_CTB* p_dev = &hh_cb.devices[dhandle];

    if (p_dev->conn.conn_state != HID_CONN_STATE_UNUSED) {
        return HID_ERR_CONN_IN_PROCESS;
    }

    p_dev->conn.ctrl_cid = 0;
    p_dev->conn.intr_cid = 0;
    p_dev->conn.disc_reason = HID_L2CAP_CONN_FAIL;
    p_dev->conn.conn_flags = HID_CONN_FLAGS_IS_ORIG;

    if ((p_dev->conn.ctrl_cid = L2CA_ConnectReq(HID_PSM_CONTROL, p_dev->addr)) == 0) {
        HIDH_TRACE_WARNING0("HID - Originate failed");
        dhandle = (p_dev - &hh_cb.devices[0]) / sizeof(tHID_HOST_DEV_CTB);
        hh_cb.callback(dhandle, HID_HDEV_EVT_CLOSE, HID_ERR_L2CAP_FAILED, NULL);
    } else {
        p_dev->conn.conn_state = HID_CONN_STATE_CONNECTING_CTRL;
    }
    return HID_SUCCESS;
}

/* 0x80492E68 (0x2C): Deregisters the two HID PSMs from L2CAP. */
void hidh_conn_dereg(void) {
    L2CA_Deregister(HID_PSM_CONTROL);
    L2CA_Deregister(HID_PSM_INTERRUPT);
}
