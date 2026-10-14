/*
 * BTE/l2c_csm.c - the L2CAP channel state machine: l2c_csm_execute, one handler per channel state, and the peer-data
 *   forwarder.
 *
 * RANGE. .text 0x80493A60-0x80494F3C (11 functions, 0x14DC B); .data 0x80616270-0x80616AA8. The .text holds the BTE
 *   l2c_csm.c functions l2c_csm_execute..forward_peer_data; the end is pinned by the .data (forward_peer_data is the
 *   last reader below 0x80616AA8, l2c_link_hci_conn_req the first above). The .data (state switch tables and trace
 *   strings) starts with no pad beyond 4-byte alignment where the readers change.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, per object in configure.py; the evidence is on BTE/l2c_utils.c's FLAGS
 *   line. The function starts of this band are packed to 4 bytes.
 * NAMES. the BTE source's. GUESS: tL2C_CCB.sec_pending (+0x37, set by l2cu_allocate_ccb, cleared once the security
 *   check passed and the connect request went out).
 * RESIDUALS. flipcheck refuses only on btu_cb: the map rows the source names are still generated (requests
 *   bte-l2cap#2/#3/#5/#6/#7/#8/#13/#23/#26), so a flip links against undefined symbols.
 * SHAPES. plain C; the state handlers are static and reached only through l2c_csm_execute's switch.
 */

#include "BTE/l2c_csm.h"
#include "BTE/l2c_link.h"
#include "BTE/l2c_main.h"
#include "BTE/l2c_utils.h"

/* The HCI status a refused security check reports to the client. */
#define HCI_ERR_AUTH_FAILURE 0x05
/* The smallest buffer offset that leaves room for the HCI and L2CAP headers. */
#define L2CAP_MIN_OFFSET 9

/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_closed(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_orig_w4_sec_comp(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_term_w4_sec_comp(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2cap_connect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2ca_connect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_config(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_open(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2cap_disconnect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data);
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2ca_disconnect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data);
static void forward_peer_data(tL2C_CCB* p_ccb, BT_HDR* p_buf);

/* 0x80493A60 (0x4C): Runs `event` through the handler of the channel's state. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
void l2c_csm_execute(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    switch (p_ccb->chnl_state) {
    case CST_CLOSED:
        l2c_csm_closed(p_ccb, event, p_data);
        break;

    case CST_ORIG_W4_SEC_COMP:
        l2c_csm_orig_w4_sec_comp(p_ccb, event, p_data);
        break;

    case CST_TERM_W4_SEC_COMP:
        l2c_csm_term_w4_sec_comp(p_ccb, event, p_data);
        break;

    case CST_W4_L2CAP_CONNECT_RSP:
        l2c_csm_w4_l2cap_connect_rsp(p_ccb, event, p_data);
        break;

    case CST_W4_L2CA_CONNECT_RSP:
        l2c_csm_w4_l2ca_connect_rsp(p_ccb, event, p_data);
        break;

    case CST_CONFIG:
        l2c_csm_config(p_ccb, event, p_data);
        break;

    case CST_OPEN:
        l2c_csm_open(p_ccb, event, p_data);
        break;

    case CST_W4_L2CAP_DISCONNECT_RSP:
        l2c_csm_w4_l2cap_disconnect_rsp(p_ccb, event, p_data);
        break;

    case CST_W4_L2CA_DISCONNECT_RSP:
        l2c_csm_w4_l2ca_disconnect_rsp(p_ccb, event, p_data);
        break;
    }
}

/* 0x80493AAC (0x294): Handles a closed channel: starts security for a new connection or reports a failed one. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_closed(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2C_CONN_INFO* p_ci = (tL2C_CONN_INFO*)p_data;
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    tL2CA_CONNECT_CFM_CB* connect_cfm = p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: CLOSED evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_LP_CONNECT_CFM:
        p_ccb->chnl_state = CST_ORIG_W4_SEC_COMP;
        btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, p_ccb->p_lcb->handle, TRUE,
                                 &l2c_link_sec_comp);
        break;

    case L2CEVT_LP_CONNECT_CFM_NEG:
        L2CAP_TRACE_API2("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid, p_ci->status);
        l2cu_release_ccb(p_ccb);
        (*connect_cfm)(local_cid, p_ci->status);
        break;

    case L2CEVT_L2CA_CONNECT_REQ:
        if (btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, p_ccb->p_lcb->handle, TRUE,
                                     &l2c_link_sec_comp) == BTM_CMD_STARTED) {
            p_ccb->chnl_state = CST_ORIG_W4_SEC_COMP;
        }
        break;

    case L2CEVT_SEC_COMP:
        l2cu_send_peer_connect_req(p_ccb);
        p_ccb->chnl_state = CST_W4_L2CAP_CONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TOUT);
        break;

    case L2CEVT_SEC_COMP_NEG:
        L2CAP_TRACE_API2("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                         L2CAP_CONN_TIMEOUT);
        l2cu_release_ccb(p_ccb);
        (*connect_cfm)(local_cid, L2CAP_CONN_SECURITY_BLOCK);
        break;

    case L2CEVT_L2CAP_CONNECT_REQ:
        p_ccb->chnl_state = CST_TERM_W4_SEC_COMP;
        btm_sec_l2cap_access_req(p_ccb->p_lcb->remote_bd_addr, p_ccb->p_rcb->psm, p_ccb->p_lcb->handle, FALSE,
                                 &l2c_link_sec_comp);
        break;

    case L2CEVT_TIMEOUT:
        L2CAP_TRACE_API2("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                         L2CAP_CONN_TIMEOUT);
        l2cu_release_ccb(p_ccb);
        (*connect_cfm)(local_cid, L2CAP_CONN_TIMEOUT);
        break;

    case L2CEVT_L2CAP_DATA:
    case L2CEVT_L2CA_DATA_WRITE:
        GKI_freebuf(p_data);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        l2cu_release_ccb(p_ccb);
        break;
    }
}

/* 0x80493D40 (0x170): Waits for our outgoing connection's security check, then sends the connect request. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_orig_w4_sec_comp(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    tL2CA_CONNECT_CFM_CB* connect_cfm = p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: ORIG_W4_SEC_COMP evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_SEC_COMP:
        p_ccb->chnl_state = CST_W4_L2CAP_CONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TOUT);
        l2cu_send_peer_connect_req(p_ccb);
        p_ccb->sec_pending = FALSE;
        break;

    case L2CEVT_SEC_COMP_NEG:
        L2CAP_TRACE_API2("L2CAP - Calling ConnectCfm_Cb(), CID: 0x%04x  Status: %d", p_ccb->local_cid,
                         HCI_ERR_AUTH_FAILURE);
        l2cu_release_ccb(p_ccb);
        (*connect_cfm)(local_cid, HCI_ERR_AUTH_FAILURE);
        break;

    case L2CEVT_L2CA_DATA_WRITE:
    case L2CEVT_L2CAP_DATA:
        GKI_freebuf(p_data);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        btm_sec_abort_access_req(p_ccb->p_lcb->remote_bd_addr);
        l2cu_release_ccb(p_ccb);
        break;
    }
}

/* 0x80493EB0 (0x168): Waits for an incoming connection's security check, then offers it to the client. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_term_w4_sec_comp(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    L2CAP_TRACE_EVENT1("L2CAP - st: TERM_W4_SEC_COMP evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        btm_sec_abort_access_req(p_ccb->p_lcb->remote_bd_addr);
        l2cu_release_ccb(p_ccb);
        break;

    case L2CEVT_SEC_COMP:
        p_ccb->chnl_state = CST_W4_L2CA_CONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TOUT);

        L2CAP_TRACE_API1("L2CAP - Calling Connect_Ind_Cb(), CID: 0x%04x", p_ccb->local_cid);
        (*p_ccb->p_rcb->api.pL2CA_ConnectInd_Cb)(p_ccb->p_lcb->remote_bd_addr, p_ccb->local_cid, p_ccb->p_rcb->psm,
                                                 p_ccb->remote_id);
        break;

    case L2CEVT_SEC_COMP_NEG:
        l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_SECURITY_BLOCK, 0);
        l2cu_release_ccb(p_ccb);
        break;

    case L2CEVT_L2CA_DATA_WRITE:
    case L2CEVT_L2CAP_DATA:
        GKI_freebuf(p_data);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        l2cu_release_ccb(p_ccb);
        break;

    case L2CEVT_L2CAP_DISCONNECT_REQ:
        l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
        btm_sec_abort_access_req(p_ccb->p_lcb->remote_bd_addr);
        l2cu_release_ccb(p_ccb);
        break;
    }
}

/* 0x80494018 (0x244): Waits for the peer's answer to our connect request. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2cap_connect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2C_CONN_INFO* p_ci = (tL2C_CONN_INFO*)p_data;
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    tL2CA_CONNECT_CFM_CB* connect_cfm = p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: W4_L2CAP_CON_RSP evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        p_ccb->chnl_state = CST_CLOSED;
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_L2CAP_CONNECT_RSP:
        p_ccb->remote_cid = p_ci->remote_cid;
        p_ccb->chnl_state = CST_CONFIG;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT);
        L2CAP_TRACE_API1("L2CAP - Calling Connect_Cfm_Cb(), CID: 0x%04x, Success", p_ccb->local_cid);

        (*p_ccb->p_rcb->api.pL2CA_ConnectCfm_Cb)(p_ccb->local_cid, L2CAP_CONN_OK);
        break;

    case L2CEVT_L2CAP_CONNECT_RSP_PND:
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TOUT_EXT);
        if (p_ccb->p_rcb->api.pL2CA_ConnectPnd_Cb) {
            L2CAP_TRACE_API0("L2CAP - Calling Connect_Pnd_Cb()");
            (*p_ccb->p_rcb->api.pL2CA_ConnectPnd_Cb)(0);
        }
        break;

    case L2CEVT_L2CAP_CONNECT_RSP_NEG:
        L2CAP_TRACE_API2("L2CAP - Calling Connect_Cfm_Cb(), CID: 0x%04x, Failure Code: %d", p_ccb->local_cid,
                         p_ci->l2cap_result);
        l2cu_release_ccb(p_ccb);
        (*connect_cfm)(local_cid, p_ci->l2cap_result);
        break;

    case L2CEVT_TIMEOUT:
        L2CAP_TRACE_API1("L2CAP - Calling Connect_Cfm_Cb(), CID: 0x%04x, Timeout", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*connect_cfm)(local_cid, L2CAP_CONN_TIMEOUT);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        l2cu_release_ccb(p_ccb);
        break;

    case L2CEVT_L2CA_DATA_WRITE:
    case L2CEVT_L2CAP_DATA:
        GKI_freebuf(p_data);
        break;
    }
}

/* 0x8049425C (0x1D8): Waits for the client to accept, hold or refuse an incoming connection. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2ca_connect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2C_CONN_INFO* p_ci;
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: W4_L2CA_CON_RSP evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_L2CA_CONNECT_RSP:
        p_ci = (tL2C_CONN_INFO*)p_data;

        if (p_ci == NULL || p_ci->l2cap_result == L2CAP_CONN_OK) {
            l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_OK, 0);
            p_ccb->chnl_state = CST_CONFIG;
            btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT);
        } else {
            l2cu_send_peer_connect_rsp(p_ccb, p_ci->l2cap_result, p_ci->l2cap_status);
            btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CONNECT_TOUT_EXT);
        }
        break;

    case L2CEVT_L2CA_CONNECT_RSP_NEG:
        p_ci = (tL2C_CONN_INFO*)p_data;
        l2cu_send_peer_connect_rsp(p_ccb, p_ci->l2cap_result, p_ci->l2cap_status);
        l2cu_release_ccb(p_ccb);
        break;

    case L2CEVT_TIMEOUT:
        l2cu_send_peer_connect_rsp(p_ccb, L2CAP_CONN_NO_PSM, 0);
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_L2CA_DATA_WRITE:
    case L2CEVT_L2CAP_DATA:
        GKI_freebuf(p_data);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        l2cu_send_peer_disc_req(p_ccb);
        p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_DISCONNECT_TOUT);
        break;
    }
}

/* 0x80494434 (0x3C8): Runs the configuration exchange; opens the channel once both sides are configured. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_config(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2CAP_CFG_INFO* p_cfg = (tL2CAP_CFG_INFO*)p_data;
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: CONFIG evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_L2CAP_CONFIG_REQ:
        if (l2cu_process_peer_cfg_req(p_ccb, p_cfg)) {
            L2CAP_TRACE_API1("L2CAP - Calling Config_Req_Cb(), CID: 0x%04x", p_ccb->local_cid);
            (*p_ccb->p_rcb->api.pL2CA_ConfigInd_Cb)(p_ccb->local_cid, p_cfg);
        } else {
            l2cu_send_peer_config_rsp(p_ccb, p_cfg);
        }
        break;

    case L2CEVT_L2CAP_CONFIG_RSP:
        l2cu_process_peer_cfg_rsp(p_ccb, p_cfg);

        p_ccb->config_done |= OB_CFG_DONE;

        if (p_ccb->config_done & IB_CFG_DONE) {
            p_ccb->chnl_state = CST_OPEN;
            btu_stop_timer(&p_ccb->timer_entry);
            forward_peer_data(p_ccb, NULL);
        }

        L2CAP_TRACE_API1("L2CAP - Calling Config_Rsp_Cb(), CID: 0x%04x", p_ccb->local_cid);
        (*p_ccb->p_rcb->api.pL2CA_ConfigCfm_Cb)(p_ccb->local_cid, p_cfg);
        break;

    case L2CEVT_L2CAP_CONFIG_RSP_NEG:
        btu_stop_timer(&p_ccb->timer_entry);

        L2CAP_TRACE_API2("L2CAP - Calling Config_Rsp_Cb(), CID: 0x%04x, Failure: %d", p_ccb->local_cid, p_cfg->result);
        (*p_ccb->p_rcb->api.pL2CA_ConfigCfm_Cb)(p_ccb->local_cid, p_cfg);
        break;

    case L2CEVT_L2CAP_DISCONNECT_REQ:
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_DISCONNECT_TOUT);
        p_ccb->chnl_state = CST_W4_L2CA_DISCONNECT_RSP;
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  Conf Needed", p_ccb->local_cid);
        (*p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb)(p_ccb->local_cid, TRUE);
        break;

    case L2CEVT_L2CA_CONFIG_REQ:
        l2cu_process_our_cfg_req(p_ccb, p_cfg);
        l2cu_send_peer_config_req(p_ccb, p_cfg);
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT);
        break;

    case L2CEVT_L2CA_CONFIG_RSP:
        l2cu_process_our_cfg_rsp(p_ccb, p_cfg);

        p_ccb->config_done |= IB_CFG_DONE;

        if (p_ccb->config_done & OB_CFG_DONE) {
            p_ccb->chnl_state = CST_OPEN;
            btu_stop_timer(&p_ccb->timer_entry);
            forward_peer_data(p_ccb, NULL);
        }

        l2cu_send_peer_config_rsp(p_ccb, p_cfg);
        break;

    case L2CEVT_L2CA_CONFIG_RSP_NEG:
        l2cu_send_peer_config_rsp(p_ccb, p_cfg);
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        l2cu_send_peer_disc_req(p_ccb);
        p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_DISCONNECT_TOUT);
        break;

    case L2CEVT_L2CAP_DATA:
        L2CAP_TRACE_API1("L2CAP - Calling DataInd_Cb(), CID: 0x%04x", p_ccb->local_cid);
        (*p_ccb->p_rcb->api.pL2CA_DataInd_Cb)(p_ccb->local_cid, (BT_HDR*)p_data);
        break;

    case L2CEVT_L2CA_DATA_WRITE:
        if (p_ccb->config_done & OB_CFG_DONE) {
            GKI_enqueue(&p_ccb->xmit_hold_q, p_data);
        } else {
            GKI_freebuf(p_data);
        }
        break;

    case L2CEVT_TIMEOUT:
        l2cu_send_peer_disc_req(p_ccb);
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;
    }
}

/* 0x804947FC (0x25C): Handles an open channel: data both ways, reconfiguration and disconnection. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_open(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    u16 local_cid = p_ccb->local_cid;
    tL2CAP_CFG_INFO* p_cfg;
    tL2C_CHNL_STATE tempstate;
    u8 tempcfgdone;

    L2CAP_TRACE_EVENT1("L2CAP - st: OPEN evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_LP_QOS_VIOLATION_IND:
        if (p_ccb->p_rcb->api.pL2CA_QoSViolationInd_Cb) {
            (*p_ccb->p_rcb->api.pL2CA_QoSViolationInd_Cb)(p_ccb->p_lcb->remote_bd_addr);
        }
        break;

    case L2CEVT_L2CAP_CONFIG_REQ:
        p_cfg = (tL2CAP_CFG_INFO*)p_data;

        tempstate = p_ccb->chnl_state;
        tempcfgdone = p_ccb->config_done;
        p_ccb->chnl_state = CST_CONFIG;
        p_ccb->config_done = 0;

        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT);

        if (l2cu_process_peer_cfg_req(p_ccb, p_cfg)) {
            (*p_ccb->p_rcb->api.pL2CA_ConfigInd_Cb)(p_ccb->local_cid, p_cfg);
        } else {
            btu_stop_timer(&p_ccb->timer_entry);
            p_ccb->chnl_state = tempstate;
            p_ccb->config_done = tempcfgdone;
            l2cu_send_peer_config_rsp(p_ccb, p_cfg);
        }
        break;

    case L2CEVT_L2CAP_DISCONNECT_REQ:
        p_ccb->chnl_state = CST_W4_L2CA_DISCONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_DISCONNECT_TOUT);
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  Conf Needed", p_ccb->local_cid);
        (*p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb)(p_ccb->local_cid, TRUE);
        break;

    case L2CEVT_L2CAP_DATA:
        (*p_ccb->p_rcb->api.pL2CA_DataInd_Cb)(p_ccb->local_cid, (BT_HDR*)p_data);
        break;

    case L2CEVT_L2CA_DISCONNECT_REQ:
        l2cu_send_peer_disc_req(p_ccb);
        p_ccb->chnl_state = CST_W4_L2CAP_DISCONNECT_RSP;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_DISCONNECT_TOUT);
        break;

    case L2CEVT_L2CA_DATA_WRITE:
        ((BT_HDR*)p_data)->event = p_ccb->local_cid;
        forward_peer_data(p_ccb, (BT_HDR*)p_data);
        break;

    case L2CEVT_L2CA_CONFIG_REQ:
        p_cfg = (tL2CAP_CFG_INFO*)p_data;
        l2cu_process_our_cfg_req(p_ccb, p_cfg);
        l2cu_send_peer_config_req(p_ccb, p_cfg);
        p_ccb->chnl_state = CST_CONFIG;
        p_ccb->config_done = 0;
        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_L2CAP_CHNL, L2CAP_CHNL_CFG_TIMEOUT);
        break;
    }
}

/* 0x80494A58 (0x1E4): Waits for the peer to confirm our disconnect request. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2cap_disconnect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2CA_DISCONNECT_CFM_CB* disconnect_cfm = p_ccb->p_rcb->api.pL2CA_DisconnectCfm_Cb;
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: W4_L2CAP_DISC_RSP evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_L2CAP_DISCONNECT_RSP:
        l2cu_release_ccb(p_ccb);
        if (disconnect_cfm) {
            L2CAP_TRACE_API1("L2CAP - Calling DisconnectCfm_Cb(), CID: 0x%04x", local_cid);
            (*disconnect_cfm)(local_cid, L2CAP_CONN_OK);
        }
        break;

    case L2CEVT_L2CAP_DISCONNECT_REQ:
        l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
        l2cu_release_ccb(p_ccb);
        if (disconnect_cfm) {
            L2CAP_TRACE_API1("L2CAP - Calling DisconnectCfm_Cb(), CID: 0x%04x", local_cid);
            (*disconnect_cfm)(local_cid, L2CAP_CONN_OK);
        }
        break;

    case L2CEVT_TIMEOUT:
        l2cu_release_ccb(p_ccb);
        if (disconnect_cfm) {
            L2CAP_TRACE_API1("L2CAP - Calling DisconnectCfm_Cb(), CID: 0x%04x", local_cid);
            (*disconnect_cfm)(local_cid, L2CAP_CONN_TIMEOUT);
        }
        break;

    case L2CEVT_L2CAP_DATA:
    case L2CEVT_L2CA_DATA_WRITE:
        GKI_freebuf(p_data);
        break;
    }
}

/* 0x80494C3C (0x15C): Waits for the client to confirm the peer's disconnect request. */
/* untyped: a caller-owned payload - tL2C_CONN_INFO, tL2CAP_CFG_INFO or BT_HDR by event */
static void l2c_csm_w4_l2ca_disconnect_rsp(tL2C_CCB* p_ccb, u8 event, void* p_data) {
    tL2CA_DISCONNECT_IND_CB* disconnect_ind = p_ccb->p_rcb->api.pL2CA_DisconnectInd_Cb;
    u16 local_cid = p_ccb->local_cid;

    L2CAP_TRACE_EVENT1("L2CAP - st: W4_L2CA_DISC_RSP evt: %d", event);

    switch (event) {
    case L2CEVT_LP_DISCONNECT_IND:
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_TIMEOUT:
        l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
        L2CAP_TRACE_API1("L2CAP - Calling Disconnect_Ind_Cb(), CID: 0x%04x  No Conf Needed", p_ccb->local_cid);
        l2cu_release_ccb(p_ccb);
        (*disconnect_ind)(local_cid, FALSE);
        break;

    case L2CEVT_L2CA_DISCONNECT_RSP:
        l2cu_send_peer_disc_rsp(p_ccb->p_lcb, p_ccb->remote_id, p_ccb->local_cid, p_ccb->remote_cid);
        l2cu_release_ccb(p_ccb);
        break;

    case L2CEVT_L2CAP_DATA:
    case L2CEVT_L2CA_DATA_WRITE:
        GKI_freebuf(p_data);
        break;
    }
}

/* 0x80494D98 (0x1A4): Sends the channel's queued data (and `p_buf`, after it) with the L2CAP and HCI headers. */
static void forward_peer_data(tL2C_CCB* p_ccb, BT_HDR* p_buf) {
    u8* p;

    if (p_buf && p_ccb->xmit_hold_q.count) {
        GKI_enqueue(&p_ccb->xmit_hold_q, p_buf);
        p_buf = (BT_HDR*)GKI_dequeue(&p_ccb->xmit_hold_q);
    } else if (!p_buf) {
        p_buf = (BT_HDR*)GKI_dequeue(&p_ccb->xmit_hold_q);
    }

    while (p_buf) {
        if (p_buf->offset < L2CAP_MIN_OFFSET) {
            L2CAP_TRACE_ERROR1("L2CAP - cannot send buffer, offset: %d", p_buf->offset);
            GKI_freebuf(p_buf);
            p_buf = (BT_HDR*)GKI_dequeue(&p_ccb->xmit_hold_q);
            continue;
        }

        p_buf->offset -= L2CAP_PKT_OVERHEAD + HCI_DATA_PREAMBLE_SIZE;
        p_buf->len += L2CAP_PKT_OVERHEAD;

        p = (u8*)&p_buf[1] + p_buf->offset;

        UINT16_TO_STREAM(p, p_ccb->p_lcb->handle | L2CAP_PKT_START_FLAG_MASK);

        if (p_buf->len > btu_cb.hcit_acl_data_size) {
            UINT16_TO_STREAM(p, btu_cb.hcit_acl_data_size);
        } else {
            UINT16_TO_STREAM(p, p_buf->len);
        }

        UINT16_TO_STREAM(p, p_buf->len - L2CAP_PKT_OVERHEAD);
        UINT16_TO_STREAM(p, p_ccb->remote_cid);

        p_buf->len += HCI_DATA_PREAMBLE_SIZE;

        l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, p_buf);

        p_buf = (BT_HDR*)GKI_dequeue(&p_ccb->xmit_hold_q);
    }
}
