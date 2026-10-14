/*
 * BTE/gap_conn.c - the GAP connection layer: gap_conn_init and the L2CAP callbacks it registers (connect, config,
 *   disconnect, data and congestion indications/confirmations).
 *
 * RANGE. .text 0x8048C2F4-0x8048CE30 (8 functions, 0xB3C B); .data 0x806151D0-0x806152A0; .sdata 0x80793DA0-0x80793DA8.
 *   The .text holds the BTE gap_conn.c functions gap_conn_init..gap_congestion_ind; gap_utils.c's BTM callback follows
 *   at 0x8048CE30, pinned by the .data. The .data starts with no pad beyond 4-byte alignment where the readers change;
 *   the .sdata (an 8-byte string read by gap_connect_ind) starts after a 4-byte pad beyond 4-byte alignment (0x80793D9C
 *   -> 0x80793DA0).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) five of its eight rows do not match.
 * NAMES. the map's. gap_data_ind (gap_data_ind) is the data-indication slot of the L2CAP callback set gap_conn_init
 *   fills, the BTE source's name.
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C; gap_find_ccb_by_cid is inlined into every L2CAP callback but gap_connect_ind, gap_release_ccb into
 *   every callback that drops a connection.
 */

#include "BTE/gap_conn.h"
#include "BTE/gap_api.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* 0x8048C2F4 (0x70): Fills the GAP connection layer's L2CAP callback set. */
void gap_conn_init(void) {
    gap_cb.conn.reg_info.pL2CA_ConnectInd_Cb = gap_connect_ind;
    gap_cb.conn.reg_info.pL2CA_ConnectCfm_Cb = gap_connect_cfm;
    gap_cb.conn.reg_info.pL2CA_ConnectPnd_Cb = NULL;
    gap_cb.conn.reg_info.pL2CA_ConfigInd_Cb = gap_config_ind;
    gap_cb.conn.reg_info.pL2CA_ConfigCfm_Cb = gap_config_cfm;
    gap_cb.conn.reg_info.pL2CA_DisconnectInd_Cb = gap_disconnect_ind;
    gap_cb.conn.reg_info.pL2CA_DisconnectCfm_Cb = NULL;
    gap_cb.conn.reg_info.pL2CA_QoSViolationInd_Cb = NULL;
    gap_cb.conn.reg_info.pL2CA_DataInd_Cb = gap_data_ind;
    gap_cb.conn.reg_info.pL2CA_CongestionStatus_Cb = gap_congestion_ind;
}

/* Returns the connection block whose channel is `cid`, or NULL. */
static tGAP_CCB* gap_find_ccb_by_cid(u16 cid) {
    u16 xx;
    tGAP_CCB* p_ccb;

    for (xx = 0, p_ccb = gap_cb.conn.ccb_pool; xx < GAP_MAX_CONNECTIONS; xx++, p_ccb++) {
        if (p_ccb->con_state != GAP_CCB_STATE_IDLE && p_ccb->connection_id == cid) {
            return p_ccb;
        }
    }
    return NULL;
}

/* Drops the received buffers of `p_ccb`, frees it and deregisters its PSM when no other block uses it. */
static void gap_release_ccb(tGAP_CCB* p_ccb) {
    u16 xx;
    u16 psm = p_ccb->psm;

    while (p_ccb->rx_queue.p_first) {
        GKI_freebuf(GKI_dequeue(&p_ccb->rx_queue));
    }

    p_ccb->con_state = GAP_CCB_STATE_IDLE;

    for (xx = 0, p_ccb = gap_cb.conn.ccb_pool; xx < GAP_MAX_CONNECTIONS; xx++, p_ccb++) {
        if (p_ccb->con_state != GAP_CCB_STATE_IDLE && p_ccb->psm == psm) {
            return;
        }
    }

    L2CA_Deregister(psm);
}

/* 0x8048C364 (0x18C): Takes an incoming channel for a listening block, or refuses an unexpected one. */
void gap_connect_ind(u8* bd_addr, u16 l2cap_cid, u16 psm, u8 l2cap_id) {
    u16 xx;
    tGAP_CCB* p_ccb;

    for (xx = 0, p_ccb = gap_cb.conn.ccb_pool; xx < GAP_MAX_CONNECTIONS; xx++, p_ccb++) {
        if (p_ccb->con_state == GAP_CCB_STATE_LISTENING && p_ccb->psm == psm
            && (p_ccb->rem_addr_specified == FALSE || !memcmp(bd_addr, p_ccb->rem_dev_address, 6))) {
            break;
        }
    }

    if (xx == GAP_MAX_CONNECTIONS) {
        GAP_TRACE_WARNING0("*******");
        GAP_TRACE_WARNING0("WARNING: GAP Conn Indication for Unexpected Bd Addr...Disconnecting");
        GAP_TRACE_WARNING0("*******");
        L2CA_DisconnectReq(l2cap_cid);
        return;
    }

    p_ccb->con_state = GAP_CCB_STATE_CFG_SETUP;
    memcpy(p_ccb->rem_dev_address, bd_addr, 6);
    p_ccb->connection_id = l2cap_cid;

    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_OK, L2CAP_CONN_OK);

    GAP_TRACE_EVENT1("GAP_CONN - Rcvd L2CAP conn ind, CID: 0x%x", p_ccb->connection_id);

    L2CA_ConfigReq(l2cap_cid, &p_ccb->cfg);
}

/* 0x8048C4F0 (0x1DC): Configures an outgoing channel once it is up, or reports the close and frees its block. */
void gap_connect_cfm(u16 l2cap_cid, u16 result) {
    tGAP_CCB* p_ccb;

    if ((p_ccb = gap_find_ccb_by_cid(l2cap_cid)) == NULL) {
        return;
    }

    if (result == L2CAP_CONN_OK && p_ccb->con_state == GAP_CCB_STATE_CONN_SETUP) {
        p_ccb->con_state = GAP_CCB_STATE_CFG_SETUP;
        L2CA_ConfigReq(l2cap_cid, &p_ccb->cfg);
    } else {
        if (p_ccb->p_callback) {
            p_ccb->p_callback(p_ccb->gap_handle, GAP_EVT_CONN_CLOSED);
        }
        gap_release_ccb(p_ccb);
    }
}

/* 0x8048C6CC (0x13C): Accepts the peer's configuration of a channel and reports it open once ours is accepted too. */
void gap_config_ind(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg) {
    tGAP_CCB* p_ccb;

    if ((p_ccb = gap_find_ccb_by_cid(l2cap_cid)) == NULL) {
        return;
    }

    if (!p_cfg->mtu_present || p_cfg->mtu > GAP_MAX_REM_MTU) {
        p_ccb->rem_mtu_size = GAP_MAX_REM_MTU;
    } else {
        p_ccb->rem_mtu_size = p_cfg->mtu;
    }

    p_cfg->flush_to_present = FALSE;
    p_cfg->mtu_present = FALSE;
    p_cfg->result = L2CAP_CFG_OK;

    L2CA_ConfigRsp(l2cap_cid, p_cfg);

    p_ccb->con_flags |= GAP_CCB_FLAGS_HIS_CFG_DONE;
    if (p_ccb->con_flags & GAP_CCB_FLAGS_MY_CFG_DONE) {
        p_ccb->con_state = GAP_CCB_STATE_CONNECTED;
        p_ccb->p_callback(p_ccb->gap_handle, GAP_EVT_CONN_OPENED);
    }
}

/* 0x8048C808 (0x1EC): Takes the answer to our configuration: reports the channel open once both sides are configured,
 * or reports the close and frees its block. */
void gap_config_cfm(u16 l2cap_cid, tL2CAP_CFG_INFO* p_cfg) {
    tGAP_CCB* p_ccb;

    if ((p_ccb = gap_find_ccb_by_cid(l2cap_cid)) == NULL) {
        return;
    }

    if (p_cfg->result == L2CAP_CFG_OK) {
        p_ccb->con_flags |= GAP_CCB_FLAGS_MY_CFG_DONE;
        if (p_ccb->con_flags & GAP_CCB_FLAGS_HIS_CFG_DONE) {
            p_ccb->con_state = GAP_CCB_STATE_CONNECTED;
            p_ccb->p_callback(p_ccb->gap_handle, GAP_EVT_CONN_OPENED);
        }
    } else {
        p_ccb->p_callback(p_ccb->gap_handle, GAP_EVT_CONN_CLOSED);
        gap_release_ccb(p_ccb);
    }
}

/* 0x8048C9F4 (0x1F8): Acknowledges the peer's close when asked, reports it and frees the channel's block. */
void gap_disconnect_ind(u16 l2cap_cid, u8 ack_needed) {
    tGAP_CCB* p_ccb;

    GAP_TRACE_EVENT1("GAP_CONN - Rcvd L2CAP disc, CID: 0x%x", l2cap_cid);

    if ((p_ccb = gap_find_ccb_by_cid(l2cap_cid)) == NULL) {
        return;
    }

    if (ack_needed) {
        L2CA_DisconnectRsp(l2cap_cid);
    }

    p_ccb->p_callback(p_ccb->gap_handle, GAP_EVT_CONN_CLOSED);
    gap_release_ccb(p_ccb);
}

/* 0x8048CBEC (0x110): Queues a received buffer on a connected channel and reports it, or frees it. */
void gap_data_ind(u16 l2cap_cid, BT_HDR* p_msg) {
    tGAP_CCB* p_ccb;

    if ((p_ccb = gap_find_ccb_by_cid(l2cap_cid)) == NULL) {
        GKI_freebuf(p_msg);
        return;
    }

    if (p_ccb->con_state == GAP_CCB_STATE_CONNECTED) {
        GKI_enqueue(&p_ccb->rx_queue, p_msg);
        p_ccb->p_callback(p_ccb->gap_handle, GAP_EVT_CONN_DATA_AVAIL);
    } else {
        GKI_freebuf(p_msg);
    }
}

/* 0x8048CCFC (0x134): Reports a channel's congestion change to its block's callback. */
void gap_congestion_ind(u16 lcid, u8 is_congested) {
    tGAP_CCB* p_ccb;
    u16 event;

    GAP_TRACE_EVENT2("GAP_CONN - Rcvd L2CAP Is Congested (%d), CID: 0x%x", is_congested, lcid);

    if ((p_ccb = gap_find_ccb_by_cid(lcid)) == NULL) {
        return;
    }

    event = is_congested ? GAP_EVT_CONN_CONGESTED : GAP_EVT_CONN_UNCONGESTED;
    p_ccb->p_callback(p_ccb->gap_handle, event);
}
