/*
 * BTE/sdp_main.c - the SDP main module: sdp_init and the L2CAP callbacks (connect, config, disconnect, data),
 *   connection origination and the connection timeout.
 *
 * RANGE. .text 0x804A21F8-0x804A2D64 (11 functions, 0xB6C B); .data 0x80618048-0x806183A8; .bss 0x8073D360-0x80741998.
 *   The .text holds the BTE sdp_main.c functions sdp_init..sdp_conn_timeout; sdp_server.c's sdp_server_handle_client_req
 *   follows. The .data (the security service name and the trace format strings) starts after a 4-byte pad beyond 4-byte
 *   alignment (0x80618044 -> 0x80618048). The .bss is the SDP control block (0x4638 B, the last BTE .bss object,
 *   cleared by sdp_init).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`), per object in configure.py;
 *   the evidence is in BTE/sdp_utils.c's header. The function starts of this band are packed to 4 bytes.
 * NAMES. the map's, which are the BTE source's; sdp_cb (.bss 0x8073D360) is a GUESS from the BTE source's control
 *   block name (every SDP unit reads its trace level at +0x4630).
 * RESIDUALS. sdp_init: the target stores the three NULL callbacks from the r29 zero it keeps for the trace level; this
 *   source stores them from a rematerialised `li r0,0` (0x1A4 B for 0x1A0 B), and a u32 zero that covers the pointers
 *   too swaps r30/r31 (string base and sdp_cb). flipcheck refuses on that 4 B .text gap. String relocations name the
 *   compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C; the L2CAP callbacks are static and reach L2CAP only through the registration in sdp_init.
 *   sdp_init: the trace level is a u8 local set before the security registrations under a scoped
 *   `opt_propagation off` (it holds the zero in r29 across the calls); L2CA_Register's result is tested as a u8.
 */

#include "BTE/sdp_main.h"
#include "BTE/sdp_discovery.h"
#include "BTE/sdp_server.h"
#include "BTE/sdp_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The HCI disconnect reasons sdp_connect_cfm maps to an SDP result. */
#define HCI_ERR_AUTH_FAILURE 0x05
#define HCI_ERR_KEY_MISSING 0x06
#define HCI_ERR_HOST_REJECT_SECURITY 0x0E
#define HCI_ERR_HOST_REJECT_DEVICE 0x0F
#define HCI_ERR_PAIRING_NOT_ALLOWED 0x18
#define HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED 0x29

/* The security service id and levels sdp_init registers. */
#define BTM_SEC_SERVICE_SDP_SERVER 0
#define BTM_SEC_NONE 0
#define SDP_PROTOCOL_ID 0

/* .bss 0x8073D360 - the SDP control block. */
tSDP_CB sdp_cb;

static void sdp_connect_ind(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id);
static void sdp_connect_cfm(u16 l2cap_cid, u16 result);
static void sdp_config_ind(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
static void sdp_config_cfm(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg);
static void sdp_disconnect_ind(u16 l2cap_cid, u8 ack_needed);
static void sdp_data_ind(u16 l2cap_cid, BT_HDR* p_msg);
static void sdp_disconnect_cfm(u16 l2cap_cid, u16 result);

/* 0x804A21F8 (0x1A0): Clears the control block, registers SDP's security levels and its L2CAP PSM. */
#pragma push
#pragma opt_propagation off
void sdp_init(void) {
    u8 trace_level;

    memset(&sdp_cb, 0, sizeof(tSDP_CB));

    sdp_cb.l2cap_my_cfg.mtu_present = TRUE;
    sdp_cb.l2cap_my_cfg.mtu = SDP_MTU_SIZE;
    sdp_cb.l2cap_my_cfg.flush_to_present = TRUE;
    sdp_cb.l2cap_my_cfg.flush_to = SDP_FLUSH_TO;

    sdp_cb.max_attr_list_size = SDP_MTU_SIZE - 16;
    sdp_cb.max_recs_per_search = SDP_MAX_DISC_SERVER_RECS;
    trace_level = 0; /* no traces */

    if (!BTM_SetSecurityLevel(FALSE, "Service Discovery", BTM_SEC_SERVICE_SDP_SERVER, BTM_SEC_NONE, SDP_PSM,
                              SDP_PROTOCOL_ID, 0)) {
        SDP_TRACE_ERROR0("Security Registration Server failed");
        return;
    }

    if (!BTM_SetSecurityLevel(TRUE, "Service Discovery", BTM_SEC_SERVICE_SDP_SERVER, BTM_SEC_NONE, SDP_PSM,
                              SDP_PROTOCOL_ID, 0)) {
        SDP_TRACE_ERROR0("Security Registration for Client failed");
        return;
    }

    sdp_cb.trace_level = trace_level;

    sdp_cb.reg_info.pL2CA_ConnectInd_Cb = sdp_connect_ind;
    sdp_cb.reg_info.pL2CA_ConnectCfm_Cb = sdp_connect_cfm;
    sdp_cb.reg_info.pL2CA_ConnectPnd_Cb = NULL;
    sdp_cb.reg_info.pL2CA_ConfigInd_Cb = sdp_config_ind;
    sdp_cb.reg_info.pL2CA_ConfigCfm_Cb = sdp_config_cfm;
    sdp_cb.reg_info.pL2CA_DisconnectInd_Cb = sdp_disconnect_ind;
    sdp_cb.reg_info.pL2CA_DisconnectCfm_Cb = sdp_disconnect_cfm;
    sdp_cb.reg_info.pL2CA_QoSViolationInd_Cb = NULL;
    sdp_cb.reg_info.pL2CA_DataInd_Cb = sdp_data_ind;
    sdp_cb.reg_info.pL2CA_CongestionStatus_Cb = NULL;

    if (!(u8)L2CA_Register(SDP_PSM, &sdp_cb.reg_info)) {
        SDP_TRACE_ERROR0("SDP Registration failed");
    }
}
#pragma pop

/* 0x804A2398 (0xC4): Accepts an incoming L2CAP connection on a new server CCB and sends our configuration. */
static void sdp_connect_ind(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id) {
    tCONN_CB* p_ccb;

    if ((p_ccb = sdpu_allocate_ccb()) == NULL) {
        return;
    }

    p_ccb->con_state = SDP_STATE_CFG_SETUP;

    memcpy(&p_ccb->device_address[0], bd_addr, 6);
    p_ccb->connection_id = l2cap_cid;

    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, 0, 0);

    L2CA_ConfigReq(l2cap_cid, &sdp_cb.l2cap_my_cfg);

    SDP_TRACE_EVENT1("SDP - Rcvd L2CAP conn ind, sent config req, CID 0x%x", p_ccb->connection_id);
}

/* 0x804A245C (0x190): Handles the answer to our connect request: configures the channel or reports the failure. */
static void sdp_connect_cfm(u16 l2cap_cid, u16 result) {
    tCONN_CB* p_ccb;

    if ((p_ccb = sdpu_find_ccb_by_cid(l2cap_cid)) == NULL) {
        SDP_TRACE_WARNING1("SDP - Rcvd conn cnf for unknown CID 0x%x", l2cap_cid);
        return;
    }

    if (result == 0 && p_ccb->con_state == SDP_STATE_CONN_SETUP) {
        p_ccb->con_state = SDP_STATE_CFG_SETUP;

        L2CA_ConfigReq(l2cap_cid, &sdp_cb.l2cap_my_cfg);

        SDP_TRACE_EVENT1("SDP - got conn cnf, sent cfg req, CID: 0x%x", p_ccb->connection_id);
    } else {
        SDP_TRACE_WARNING2("SDP - Rcvd conn cnf with error: 0x%x  CID 0x%x", result, p_ccb->connection_id);

        if (p_ccb->p_cb) {
            if (result == HCI_ERR_HOST_REJECT_SECURITY || result == HCI_ERR_AUTH_FAILURE ||
                result == HCI_ERR_PAIRING_NOT_ALLOWED || result == HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED ||
                result == HCI_ERR_KEY_MISSING) {
                (*p_ccb->p_cb)(SDP_SECURITY_ERR);
            } else if (result == HCI_ERR_HOST_REJECT_DEVICE) {
                (*p_ccb->p_cb)(SDP_CONN_REJECTED);
            } else {
                (*p_ccb->p_cb)(SDP_CONN_FAILED);
            }
        }
        sdpu_release_ccb(p_ccb);
    }
}

/* 0x804A25EC (0x13C): Accepts the peer's configuration, notes its MTU and starts the exchange once both sides are done. */
static void sdp_config_ind(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg) {
    tCONN_CB* p_ccb;

    if ((p_ccb = sdpu_find_ccb_by_cid(l2cap_cid)) == NULL) {
        SDP_TRACE_WARNING1("SDP - Rcvd L2CAP cfg ind, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    if (!p_cfg->mtu_present) {
        p_ccb->rem_mtu_size = SDP_MTU_SIZE;
    } else {
        if (p_cfg->mtu > SDP_MTU_SIZE) {
            p_ccb->rem_mtu_size = SDP_MTU_SIZE;
        } else {
            p_ccb->rem_mtu_size = p_cfg->mtu;
        }
    }

    p_cfg->flush_to_present = FALSE;
    p_cfg->mtu_present = FALSE;
    p_cfg->result = 0;

    L2CA_ConfigRsp(l2cap_cid, p_cfg);

    SDP_TRACE_EVENT1("SDP - Rcvd cfg ind, sent cfg cfm, CID: 0x%x", l2cap_cid);

    p_ccb->con_flags |= SDP_FLAGS_HIS_CFG_DONE;

    if (p_ccb->con_flags & SDP_FLAGS_MY_CFG_DONE) {
        p_ccb->con_state = SDP_STATE_CONNECTED;

        if (p_ccb->con_flags & SDP_FLAGS_IS_ORIG) {
            sdp_disc_connected(p_ccb);
        } else {
            btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_SDP, SDP_INACT_TIMEOUT);
        }
    }
}

/* 0x804A2728 (0x17C): Handles the peer's answer to our configuration; a refusal closes or fails the connection. */
static void sdp_config_cfm(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg) {
    tCONN_CB* p_ccb;

    SDP_TRACE_EVENT2("SDP - Rcvd cfg cfm, CID: 0x%x  Result: %d", l2cap_cid, p_cfg->result);

    if ((p_ccb = sdpu_find_ccb_by_cid(l2cap_cid)) == NULL) {
        SDP_TRACE_WARNING1("SDP - Rcvd L2CAP cfg ind, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    if (p_cfg->result == 0) {
        p_ccb->con_flags |= SDP_FLAGS_MY_CFG_DONE;

        if (p_ccb->con_flags & SDP_FLAGS_HIS_CFG_DONE) {
            p_ccb->con_state = SDP_STATE_CONNECTED;

            if (p_ccb->con_flags & SDP_FLAGS_IS_ORIG) {
                sdp_disc_connected(p_ccb);
            } else {
                btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_SDP, SDP_INACT_TIMEOUT);
            }
        }
    } else {
        sdp_disconnect(p_ccb, SDP_CFG_FAILED);
    }
}

/* 0x804A28A4 (0xF8): Handles the peer closing the channel: acknowledges it and reports the result. */
static void sdp_disconnect_ind(u16 l2cap_cid, u8 ack_needed) {
    tCONN_CB* p_ccb;

    if ((p_ccb = sdpu_find_ccb_by_cid(l2cap_cid)) == NULL) {
        SDP_TRACE_WARNING1("SDP - Rcvd L2CAP disc, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    if (ack_needed) {
        L2CA_DisconnectRsp(l2cap_cid);
    }

    SDP_TRACE_EVENT1("SDP - Rcvd L2CAP disc, CID: 0x%x", l2cap_cid);

    if (p_ccb->p_cb) {
        (*p_ccb->p_cb)((u16)((p_ccb->con_state == SDP_STATE_CONNECTED) ? SDP_SUCCESS : SDP_CONN_FAILED));
    }

    sdpu_release_ccb(p_ccb);
}

/* 0x804A299C (0xD4): Passes a received packet to the client or server side of its connection, then frees it. */
static void sdp_data_ind(u16 l2cap_cid, BT_HDR* p_msg) {
    tCONN_CB* p_ccb;

    if ((p_ccb = sdpu_find_ccb_by_cid(l2cap_cid)) != NULL) {
        if (p_ccb->con_state == SDP_STATE_CONNECTED) {
            if (p_ccb->con_flags & SDP_FLAGS_IS_ORIG) {
                sdp_disc_server_rsp(p_ccb, p_msg);
            } else {
                sdp_server_handle_client_req(p_ccb, p_msg);
            }
        } else {
            SDP_TRACE_WARNING2("SDP - Ignored L2CAP data while in state: %d, CID: 0x%x", p_ccb->con_state,
                               l2cap_cid);
        }
    } else {
        SDP_TRACE_WARNING1("SDP - Rcvd L2CAP data, unknown CID: 0x%x", l2cap_cid);
    }

    GKI_freebuf(p_msg);
}

/* 0x804A2A70 (0x114): Allocates a client CCB and opens the L2CAP channel to `p_bd_addr`; NULL when that fails. */
tCONN_CB* sdp_conn_originate(u8* p_bd_addr) {
    tCONN_CB* p_ccb;
    u16 cid;

    if ((p_ccb = sdpu_allocate_ccb()) == NULL) {
        SDP_TRACE_WARNING0("SDP - no spare CCB for orig");
        return NULL;
    }

    SDP_TRACE_EVENT0("SDP - Originate started");

    p_ccb->con_flags |= SDP_FLAGS_IS_ORIG;

    memcpy(&p_ccb->device_address[0], p_bd_addr, 6);

    p_ccb->con_state = SDP_STATE_CONN_SETUP;

    cid = L2CA_ConnectReq(SDP_PSM, p_bd_addr);

    if (cid != 0) {
        p_ccb->connection_id = cid;
        return p_ccb;
    } else {
        SDP_TRACE_WARNING0("SDP - Originate failed");
        sdpu_release_ccb(p_ccb);
        return NULL;
    }
}

/* 0x804A2B84 (0x9C): Closes the CCB's channel, or reports `reason` at once while it is still connecting. */
void sdp_disconnect(tCONN_CB* p_ccb, u16 reason) {
    SDP_TRACE_EVENT1("SDP - disconnect  CID: 0x%x", p_ccb->connection_id);

    if (p_ccb->con_state != SDP_STATE_CONN_SETUP) {
        L2CA_DisconnectReq(p_ccb->connection_id);
        p_ccb->disconnect_reason = reason;
    } else {
        if (p_ccb->p_cb) {
            (*p_ccb->p_cb)(reason);
        }

        sdpu_release_ccb(p_ccb);
    }
}

/* 0x804A2C20 (0xBC): Handles the confirmation of our disconnect: reports the stored reason and frees the CCB. */
static void sdp_disconnect_cfm(u16 l2cap_cid, u16 result) {
    tCONN_CB* p_ccb;

    if ((p_ccb = sdpu_find_ccb_by_cid(l2cap_cid)) == NULL) {
        SDP_TRACE_WARNING1("SDP - Rcvd L2CAP disc cfm, unknown CID: 0x%x", l2cap_cid);
        return;
    }

    SDP_TRACE_EVENT1("SDP - Rcvd L2CAP disc cfm, CID: 0x%x", l2cap_cid);

    if (p_ccb->p_cb) {
        (*p_ccb->p_cb)(p_ccb->disconnect_reason);
    }

    sdpu_release_ccb(p_ccb);
}

/* 0x804A2CDC (0x88): Disconnects a CCB whose inactivity timer ran out and reports a failed connection. */
void sdp_conn_timeout(tCONN_CB* p_ccb) {
    SDP_TRACE_EVENT2("SDP - CCB timeout in state: %d  CID: 0x%x", p_ccb->con_state, p_ccb->connection_id);

    L2CA_DisconnectReq(p_ccb->connection_id);

    if (p_ccb->p_cb) {
        (*p_ccb->p_cb)(SDP_CONN_FAILED);
    }

    sdpu_release_ccb(p_ccb);
}
