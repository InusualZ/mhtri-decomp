/*
 * BTE/l2c_utils.c - the L2CAP utilities: link and channel control block allocation, release and lookup, the peer
 *   signalling builders, configuration processing and connection creation.
 *
 * RANGE. .text 0x804970F8-0x80498FBC (33 functions, 0x1EC4 B); .data 0x80617018-0x80617100; .sdata
 *   0x80793DA8-0x80793DB8. Strong: the .data starts after a 4-byte pad beyond 4-byte alignment (0x80617014 ->
 *   0x80617018) and the l2cu_* family starts at 0x804970F8; it ends with l2cu_lcb_disconnecting, before port_api.c's
 *   RFCOMM_Init. The .data and .sdata (the TRUE/FALSE strings) are read only by this range's functions.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, per object in configure.py (as BTE/sdp_*.c). Evidence, measured on scratch
 *   compiles of the five l2c sources: under GC/3.0a5.2 every row compiles to the target up to relocation names
 *   except the residuals in l2c_link.c and l2c_main.c; Wii/1.3 `cflags_os` emits none of l2c_csm.c's ten switch
 *   jump tables (its .data short of the target's) and reschedules l2cu_build_header, the CCB list code and the
 *   loop counters. The function starts of this band are packed to 4 bytes.
 * NAMES. the BTE source's, including l2cu_find_lcb_by_handle, l2cu_get_conn_role, l2cu_release_rcb and
 *   l2cu_find_lcb_by_state.
 * RESIDUALS. flipcheck refuses only on btm_cb, btu_cb and btsnd_hcic_write_auto_flush_tout: the map rows the source
 *   names are still generated (requests bte-l2cap#2/#3/#5/#6/#7/#8/#13/#23/#26), so a flip links against undefined
 *   symbols.
 * SHAPES. plain C.
 */

#include "BTE/l2c_utils.h"
#include "BTE/l2c_link.h"
#include "BTE/l2c_main.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The size of an L2CAP command pool buffer, the cap on an echo response. */
#define L2CAP_CMD_BUF_SIZE 660
/* The longest flush timeout (ms) the controller's 0.625 ms slot counter can express. */
#define L2CAP_MAX_FLUSH_TOUT_MS 1279
/* The information response result for an unsupported type. */
#define L2CAP_INFO_RESP_RESULT_NOT_SUPPORTED 1
/* The QoS service types. */
#define SVC_TYPE_BEST_EFFORT 1
#define SVC_TYPE_GUARANTEED 2

/* 0x804970F8 (0xD4): Takes a free link control block for the peer at `p_bd_addr` and rebalances the ACL buffers. */
tL2C_LCB* l2cu_allocate_lcb(u8* p_bd_addr) {
    int xx;
    tL2C_LCB* p_lcb = &l2cb.lcb_pool[0];

    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (!p_lcb->in_use) {
            memset(p_lcb, 0, sizeof(tL2C_LCB));

            p_lcb->in_use = TRUE;
            memcpy(p_lcb->remote_bd_addr, p_bd_addr, 6);

            p_lcb->link_state = LST_DISCONNECTED;
            p_lcb->handle = 0xFFFF;
            p_lcb->p_hcit_rcv_acl = NULL;
            p_lcb->link_flush_tout = 0xFFFF;
            p_lcb->timer_entry.param = (u32)p_lcb;
            p_lcb->sent_not_acked = 0;
            p_lcb->idle_timeout = l2cb.idle_timeout;
            p_lcb->id = 0;

            l2cb.num_links_active++;

            l2c_link_adjust_allocation();
            return p_lcb;
        }
    }

    return NULL;
}

/* 0x804971CC (0x100): Releases a link control block, its channels and queued data, and ends a pending echo. */
void l2cu_release_lcb(tL2C_LCB* p_lcb) {
    tL2C_CCB* p_ccb;
    tL2CA_ECHO_RSP_CB* p_cb;

    p_lcb->in_use = FALSE;

    btu_stop_timer(&p_lcb->timer_entry);

    if (p_lcb->p_hcit_rcv_acl) {
        GKI_freebuf(p_lcb->p_hcit_rcv_acl);
        p_lcb->p_hcit_rcv_acl = NULL;
    }

    btm_remove_sco_links(p_lcb->remote_bd_addr);

    p_ccb = p_lcb->p_first_ccb;
    while (p_ccb) {
        l2cu_release_ccb(p_ccb);
        p_ccb = p_lcb->p_first_ccb;
    }

    if (p_lcb->link_state == LST_CONNECTED || p_lcb->link_state == LST_DISCONNECTING) {
        btm_acl_removed(p_lcb->remote_bd_addr);
    }

    while (p_lcb->link_xmit_data_q.p_first) {
        GKI_freebuf(GKI_dequeue(&p_lcb->link_xmit_data_q));
    }

    if (l2cb.num_links_active >= 1) {
        l2cb.num_links_active--;
    }

    l2cb.controller_xmit_window += p_lcb->sent_not_acked;

    l2c_link_adjust_allocation();

    if (p_lcb->p_echo_rsp_cb) {
        p_cb = p_lcb->p_echo_rsp_cb;
        p_lcb->p_echo_rsp_cb = NULL;
        (*p_cb)(L2CAP_PING_RESULT_NO_LINK);
    }
}

/* 0x804972CC (0x88): Returns the in-use link to the peer at `p_bd_addr`, or NULL. */
tL2C_LCB* l2cu_find_lcb_by_bd_addr(u8* p_bd_addr) {
    int xx;
    tL2C_LCB* p_lcb = &l2cb.lcb_pool[0];

    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (p_lcb->in_use && !memcmp(p_lcb->remote_bd_addr, p_bd_addr, 6)) {
            return p_lcb;
        }
    }

    return NULL;
}

/* 0x80497354 (0x90): Returns the in-use link with the HCI handle `handle`, or NULL. */
tL2C_LCB* l2cu_find_lcb_by_handle(u16 handle) {
    int xx;
    tL2C_LCB* p_lcb = &l2cb.lcb_pool[0];

    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (p_lcb->in_use && p_lcb->handle == handle) {
            return p_lcb;
        }
    }

    return NULL;
}

/* 0x804973E4 (0x10): Returns the role a new link asks for. */
u8 l2cu_get_conn_role(tL2C_LCB* p_this_lcb) {
    return l2cb.desire_role;
}

/* 0x804973F4 (0xC4): Takes a signalling buffer and writes the HCI, L2CAP and command headers of a `len`-byte command. */
BT_HDR* l2cu_build_header(u16 handle, u16 len, u8 cmd, u8 id) {
    BT_HDR* p_buf = (BT_HDR*)GKI_getpoolbuf(L2CAP_CMD_POOL_ID);
    u8* p;

    if (!p_buf) {
        return NULL;
    }

    p_buf->offset = L2CAP_SEND_CMD_OFFSET;
    p_buf->len = len + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD;
    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET];

    UINT16_TO_STREAM(p, handle | L2CAP_PKT_START_FLAG_MASK);
    UINT16_TO_STREAM(p, len + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD);
    UINT16_TO_STREAM(p, len + L2CAP_CMD_OVERHEAD);
    UINT16_TO_STREAM(p, L2CAP_SIGNALLING_CID);
    UINT8_TO_STREAM(p, cmd);
    UINT8_TO_STREAM(p, id);
    UINT16_TO_STREAM(p, len);

    return p_buf;
}

/* 0x804974B8 (0xFC): Sends a command reject with `reason` and its one or two parameters. */
void l2cu_send_peer_cmd_reject(tL2C_LCB* p_lcb, u16 reason, u8 rem_id, u16 p1, u16 p2) {
    u16 param_len;
    BT_HDR* p_buf;
    u8* p;

    if (reason == L2CAP_CMD_REJ_MTU_EXCEEDED) {
        param_len = 2;
    } else if (reason == L2CAP_CMD_REJ_INVALID_CID) {
        param_len = 4;
    } else {
        param_len = 0;
    }

    if ((p_buf = l2cu_build_header(p_lcb->handle, (u16)(L2CAP_CMD_REJECT_LEN + param_len), L2CAP_CMD_REJECT,
                                   rem_id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer cmd_rej");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, reason);

    if (param_len >= 2) {
        UINT16_TO_STREAM(p, p1);
    }

    if (param_len >= 4) {
        UINT16_TO_STREAM(p, p2);
    }

    l2c_link_check_send_pkts(p_lcb, NULL, p_buf);
}

/* 0x804975B4 (0xC4): Sends a connect request for the channel's PSM with a new command id. */
void l2cu_send_peer_connect_req(tL2C_CCB* p_ccb) {
    BT_HDR* p_buf;
    u8* p;

    p_ccb->local_id = ++p_ccb->p_lcb->id;

    if ((p_buf = l2cu_build_header(p_ccb->p_lcb->handle, L2CAP_CONN_REQ_LEN, L2CAP_CMD_CONN_REQ, p_ccb->local_id)) ==
        NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, p_ccb->p_rcb->psm);
    UINT16_TO_STREAM(p, p_ccb->local_cid);

    l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, p_buf);
}

/* 0x80497678 (0xD8): Answers the peer's connect request on the channel with `result` and `status`. */
void l2cu_send_peer_connect_rsp(tL2C_CCB* p_ccb, u16 result, u16 status) {
    BT_HDR* p_buf;
    u8* p;

    if ((p_buf = l2cu_build_header(p_ccb->p_lcb->handle, L2CAP_CONN_RSP_LEN, L2CAP_CMD_CONN_RSP, p_ccb->remote_id)) ==
        NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, p_ccb->local_cid);
    UINT16_TO_STREAM(p, p_ccb->remote_cid);
    UINT16_TO_STREAM(p, result);
    UINT16_TO_STREAM(p, status);

    l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, p_buf);
}

/* 0x80497750 (0xC0): Refuses a connect request that has no channel with `result`. */
void l2cu_reject_connection(tL2C_LCB* p_lcb, u16 remote_cid, u8 rem_id, u16 result) {
    BT_HDR* p_buf;
    u8* p;

    if ((p_buf = l2cu_build_header(p_lcb->handle, L2CAP_CONN_RSP_LEN, L2CAP_CMD_CONN_RSP, rem_id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, 0);
    UINT16_TO_STREAM(p, remote_cid);
    UINT16_TO_STREAM(p, result);
    UINT16_TO_STREAM(p, 0);

    l2c_link_check_send_pkts(p_lcb, NULL, p_buf);
}

/* 0x80497810 (0x2F8): Sends our configuration request with the options `p_cfg` marks present. */
void l2cu_send_peer_config_req(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg) {
    BT_HDR* p_buf;
    u16 cfg_len = 0;
    u8* p;

    p_ccb->local_id = ++p_ccb->p_lcb->id;

    if (p_cfg->mtu_present) {
        cfg_len += L2CAP_CFG_MTU_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }
    if (p_cfg->flush_to_present) {
        cfg_len += L2CAP_CFG_FLUSH_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }
    if (p_cfg->qos_present) {
        cfg_len += L2CAP_CFG_QOS_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }
    if (p_cfg->fcr_present) {
        cfg_len += L2CAP_CFG_FCR_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }

    if ((p_buf = l2cu_build_header(p_ccb->p_lcb->handle, (u16)(cfg_len + L2CAP_CONFIG_REQ_LEN), L2CAP_CMD_CONFIG_REQ,
                                   p_ccb->local_id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, p_ccb->remote_cid);
    UINT16_TO_STREAM(p, 0);

    if (p_cfg->mtu_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_MTU);
        UINT8_TO_STREAM(p, L2CAP_CFG_MTU_OPTION_LEN);
        UINT16_TO_STREAM(p, p_cfg->mtu);
    }

    if (p_cfg->flush_to_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_FLUSH_TOUT);
        UINT8_TO_STREAM(p, L2CAP_CFG_FLUSH_OPTION_LEN);
        UINT16_TO_STREAM(p, p_cfg->flush_to);
    }

    if (p_cfg->qos_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_QOS);
        UINT8_TO_STREAM(p, L2CAP_CFG_QOS_OPTION_LEN);
        UINT8_TO_STREAM(p, p_cfg->qos.qos_flags);
        UINT8_TO_STREAM(p, p_cfg->qos.service_type);
        UINT32_TO_STREAM(p, p_cfg->qos.token_rate);
        UINT32_TO_STREAM(p, p_cfg->qos.token_bucket_size);
        UINT32_TO_STREAM(p, p_cfg->qos.peak_bandwidth);
        UINT32_TO_STREAM(p, p_cfg->qos.latency);
        UINT32_TO_STREAM(p, p_cfg->qos.delay_variation);
    }

    if (p_cfg->fcr_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_FCR);
        UINT8_TO_STREAM(p, L2CAP_CFG_FCR_OPTION_LEN);
        UINT8_TO_STREAM(p, p_cfg->fcr.mode);
        UINT8_TO_STREAM(p, p_cfg->fcr.tx_win_sz);
        UINT8_TO_STREAM(p, p_cfg->fcr.max_transmit);
        UINT16_TO_STREAM(p, p_cfg->fcr.rtrans_tout);
        UINT16_TO_STREAM(p, p_cfg->fcr.mon_tout);
        UINT16_TO_STREAM(p, p_cfg->fcr.mps);
    }

    l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, p_buf);
}

/* 0x80497B08 (0x2F8): Answers the peer's configuration request with `p_cfg`'s result and options. */
void l2cu_send_peer_config_rsp(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg) {
    BT_HDR* p_buf;
    u16 cfg_len = 0;
    u8* p;

    if (p_cfg->mtu_present) {
        cfg_len += L2CAP_CFG_MTU_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }
    if (p_cfg->flush_to_present) {
        cfg_len += L2CAP_CFG_FLUSH_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }
    if (p_cfg->qos_present) {
        cfg_len += L2CAP_CFG_QOS_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }
    if (p_cfg->fcr_present) {
        cfg_len += L2CAP_CFG_FCR_OPTION_LEN + L2CAP_CFG_OPTION_OVERHEAD;
    }

    if ((p_buf = l2cu_build_header(p_ccb->p_lcb->handle, (u16)(L2CAP_CONFIG_RSP_LEN + cfg_len), L2CAP_CMD_CONFIG_RSP,
                                   p_ccb->remote_id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, p_ccb->remote_cid);
    UINT16_TO_STREAM(p, 0);
    UINT16_TO_STREAM(p, p_cfg->result);

    if (p_cfg->mtu_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_MTU);
        UINT8_TO_STREAM(p, L2CAP_CFG_MTU_OPTION_LEN);
        UINT16_TO_STREAM(p, p_cfg->mtu);
    }

    if (p_cfg->flush_to_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_FLUSH_TOUT);
        UINT8_TO_STREAM(p, L2CAP_CFG_FLUSH_OPTION_LEN);
        UINT16_TO_STREAM(p, p_cfg->flush_to);
    }

    if (p_cfg->qos_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_QOS);
        UINT8_TO_STREAM(p, L2CAP_CFG_QOS_OPTION_LEN);
        UINT8_TO_STREAM(p, p_cfg->qos.qos_flags);
        UINT8_TO_STREAM(p, p_cfg->qos.service_type);
        UINT32_TO_STREAM(p, p_cfg->qos.token_rate);
        UINT32_TO_STREAM(p, p_cfg->qos.token_bucket_size);
        UINT32_TO_STREAM(p, p_cfg->qos.peak_bandwidth);
        UINT32_TO_STREAM(p, p_cfg->qos.latency);
        UINT32_TO_STREAM(p, p_cfg->qos.delay_variation);
    }

    if (p_cfg->fcr_present) {
        UINT8_TO_STREAM(p, L2CAP_CFG_TYPE_FCR);
        UINT8_TO_STREAM(p, L2CAP_CFG_FCR_OPTION_LEN);
        UINT8_TO_STREAM(p, p_cfg->fcr.mode);
        UINT8_TO_STREAM(p, p_cfg->fcr.tx_win_sz);
        UINT8_TO_STREAM(p, p_cfg->fcr.max_transmit);
        UINT16_TO_STREAM(p, p_cfg->fcr.rtrans_tout);
        UINT16_TO_STREAM(p, p_cfg->fcr.mon_tout);
        UINT16_TO_STREAM(p, p_cfg->fcr.mps);
    }

    l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, p_buf);
}

/* 0x80497E00 (0x1B4): Rejects the peer's configuration, echoing back every option we do not know. */
void l2cu_send_peer_config_rej(tL2C_CCB* p_ccb, u8* p_data, u16 data_len, u16 rej_len) {
    BT_HDR* p_buf = (BT_HDR*)GKI_getpoolbuf(L2CAP_CMD_POOL_ID);
    u16 len;
    u16 cfg_len;
    u8* p;
    u8* p_hci_len;
    u8* p_data_end;
    u8 cfg_code;

    if (!p_buf) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for cfg_rej");
        return;
    }

    p_buf->offset = L2CAP_SEND_CMD_OFFSET;
    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET];

    UINT16_TO_STREAM(p, p_ccb->p_lcb->handle | L2CAP_PKT_START_FLAG_MASK);

    p_hci_len = p;
    p += 2;

    UINT16_TO_STREAM(p, rej_len + L2CAP_CMD_OVERHEAD + L2CAP_CONFIG_RSP_LEN);
    UINT16_TO_STREAM(p, L2CAP_SIGNALLING_CID);
    UINT8_TO_STREAM(p, L2CAP_CMD_CONFIG_RSP);
    UINT8_TO_STREAM(p, p_ccb->remote_id);
    UINT16_TO_STREAM(p, rej_len + L2CAP_CONFIG_RSP_LEN);
    UINT16_TO_STREAM(p, p_ccb->remote_cid);
    UINT16_TO_STREAM(p, 0);
    UINT16_TO_STREAM(p, L2CAP_CFG_UNKNOWN_OPTIONS);

    p_data_end = p_data + data_len;
    while (p_data < p_data_end) {
        cfg_code = *p_data;
        cfg_len = *(p_data + 1);

        switch (cfg_code & 0x7F) {
        case L2CAP_CFG_TYPE_MTU:
        case L2CAP_CFG_TYPE_FLUSH_TOUT:
        case L2CAP_CFG_TYPE_QOS:
            p_data += cfg_len + L2CAP_CFG_OPTION_OVERHEAD;
            break;

        default:
            if (cfg_len + L2CAP_CFG_OPTION_OVERHEAD <= data_len) {
                if (!(cfg_code & 0x80)) {
                    memcpy(p, p_data, cfg_len + L2CAP_CFG_OPTION_OVERHEAD);
                    p += cfg_len + L2CAP_CFG_OPTION_OVERHEAD;
                }
                p_data += cfg_len + L2CAP_CFG_OPTION_OVERHEAD;
            } else {
                p_data = p_data_end;
            }
            break;
        }
    }

    len = (u16)(p - p_hci_len - 2);
    UINT16_TO_STREAM(p_hci_len, len);

    p_buf->len = len + HCI_DATA_PREAMBLE_SIZE;

    l2c_link_check_send_pkts(p_ccb->p_lcb, p_ccb, p_buf);
}

/* 0x80497FB4 (0xD4): Queues a disconnect request for the channel ahead of its data. */
void l2cu_send_peer_disc_req(tL2C_CCB* p_ccb) {
    BT_HDR* p_buf;
    u8* p;

    p_ccb->local_id = ++p_ccb->p_lcb->id;

    if ((p_buf = l2cu_build_header(p_ccb->p_lcb->handle, L2CAP_DISC_REQ_LEN, L2CAP_CMD_DISC_REQ, p_ccb->local_id)) ==
        NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, p_ccb->remote_cid);
    UINT16_TO_STREAM(p, p_ccb->local_cid);

    p_buf->layer_specific = 0;
    GKI_enqueue(&p_ccb->p_lcb->link_xmit_data_q, p_buf);

    l2c_link_check_send_pkts(p_ccb->p_lcb, NULL, NULL);
}

/* 0x80498088 (0xC0): Queues the answer to the peer's disconnect request. */
void l2cu_send_peer_disc_rsp(tL2C_LCB* p_lcb, u8 remote_id, u16 local_cid, u16 remote_cid) {
    BT_HDR* p_buf;
    u8* p;

    if ((p_buf = l2cu_build_header(p_lcb->handle, L2CAP_DISC_RSP_LEN, L2CAP_CMD_DISC_RSP, remote_id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, local_cid);
    UINT16_TO_STREAM(p, remote_cid);

    p_buf->layer_specific = 0;
    GKI_enqueue(&p_lcb->link_xmit_data_q, p_buf);

    l2c_link_check_send_pkts(p_lcb, NULL, NULL);
}

/* 0x80498148 (0x170): Sends an echo request carrying `data_len` bytes of `p_data`. */
void l2cu_send_peer_echo_req(tL2C_LCB* p_lcb, u8* p_data, u16 data_len) {
    BT_HDR* p_buf;
    u8* p;
    int xx;

    if ((p_buf = l2cu_build_header(p_lcb->handle, data_len, L2CAP_CMD_ECHO_REQ, ++p_lcb->id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for echo_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    if (data_len) {
        for (xx = 0; xx < data_len; xx++) {
            *p++ = p_data[xx];
        }
    }

    l2c_link_check_send_pkts(p_lcb, NULL, p_buf);
}

/* 0x804982B8 (0x19C): Answers an echo request, dropping the data when it does not fit one buffer. */
void l2cu_send_peer_echo_rsp(tL2C_LCB* p_lcb, u8 id, u8* p_data, u16 data_len) {
    BT_HDR* p_buf;
    u8* p;
    u16 maxlen;
    int xx;

    maxlen = (L2CAP_CMD_BUF_SIZE > btu_cb.hcit_acl_pkt_size) ? btu_cb.hcit_acl_data_size : L2CAP_CMD_BUF_SIZE;

    if (data_len > (u16)(maxlen - (HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD +
                                   L2CAP_ECHO_RSP_LEN))) {
        data_len = 0;
    }

    if ((p_buf = l2cu_build_header(p_lcb->handle, data_len, L2CAP_CMD_ECHO_RSP, id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    if (data_len) {
        for (xx = 0; xx < data_len; xx++) {
            *p++ = p_data[xx];
        }
    }

    l2c_link_check_send_pkts(p_lcb, NULL, p_buf);
}

/* 0x80498454 (0xA4): Answers an information request with "not supported". */
void l2cu_send_peer_info_rsp(tL2C_LCB* p_lcb, u8 remote_id, u16 info_type) {
    BT_HDR* p_buf;
    u8* p;

    if ((p_buf = l2cu_build_header(p_lcb->handle, L2CAP_INFO_RSP_LEN, L2CAP_CMD_INFO_RSP, remote_id)) == NULL) {
        L2CAP_TRACE_WARNING0("L2CAP - no buffer for conn_req");
        return;
    }

    p = &((BT_BUF*)p_buf)->data[L2CAP_SEND_CMD_OFFSET + HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD];

    UINT16_TO_STREAM(p, info_type);
    UINT16_TO_STREAM(p, L2CAP_INFO_RESP_RESULT_NOT_SUPPORTED);

    l2c_link_check_send_pkts(p_lcb, NULL, p_buf);
}

/* 0x804984F8 (0x10C): Takes a free channel, appends it to the link's channel list and sets the default config. */
tL2C_CCB* l2cu_allocate_ccb(tL2C_LCB* p_lcb) {
    tL2C_CCB* p_ccb;

    if (!l2cb.p_free_ccb_first) {
        return NULL;
    }

    p_ccb = l2cb.p_free_ccb_first;
    l2cb.p_free_ccb_first = p_ccb->p_next_ccb;

    p_ccb->in_use = TRUE;
    p_ccb->local_cid = L2CAP_BASE_APPL_CID + (u16)(p_ccb - l2cb.ccb_pool);
    p_ccb->p_lcb = p_lcb;

    if (!p_lcb->p_first_ccb) {
        p_lcb->p_first_ccb = p_lcb->p_last_ccb = p_ccb;
        p_ccb->p_prev_ccb = p_ccb->p_next_ccb = NULL;
    } else {
        p_ccb->p_next_ccb = NULL;
        p_ccb->p_prev_ccb = p_lcb->p_last_ccb;
        p_lcb->p_last_ccb->p_next_ccb = p_ccb;
        p_lcb->p_last_ccb = p_ccb;
    }

    p_ccb->our_flush_to = L2CAP_DEFAULT_FLUSH_TO;
    p_ccb->our_mtu = p_ccb->peer_mtu = L2CAP_DEFAULT_MTU;

    p_ccb->peer_qos.service_type = p_ccb->our_qos.service_type = SVC_TYPE_BEST_EFFORT;
    p_ccb->peer_qos.token_rate = p_ccb->our_qos.token_rate = 0;
    p_ccb->peer_qos.token_bucket_size = p_ccb->our_qos.token_bucket_size = 0;
    p_ccb->peer_qos.peak_bandwidth = p_ccb->our_qos.peak_bandwidth = 0;
    p_ccb->peer_qos.latency = p_ccb->our_qos.latency = 0xFFFFFFFF;
    p_ccb->peer_qos.delay_variation = p_ccb->our_qos.delay_variation = 0xFFFFFFFF;

    p_ccb->config_done = 0;
    p_ccb->chnl_state = CST_CLOSED;
    p_ccb->sec_pending = TRUE;
    p_ccb->timer_entry.param = (u32)p_ccb;

    return p_ccb;
}

/* 0x80498604 (0x1A4): Releases a channel to the free list; when it was the link's last, starts the idle timeout. */
void l2cu_release_ccb(tL2C_CCB* p_ccb) {
    tL2C_LCB* p_lcb = p_ccb->p_lcb;
    u16 timeout;
    u8 rc;

    if (!p_ccb->in_use) {
        return;
    }

    p_ccb->in_use = FALSE;

    btu_stop_timer(&p_ccb->timer_entry);

    while (p_ccb->xmit_hold_q.p_first) {
        GKI_freebuf(GKI_dequeue(&p_ccb->xmit_hold_q));
    }

    p_ccb->p_lcb = NULL;

    if (p_lcb->p_first_ccb == p_ccb) {
        p_lcb->p_first_ccb = p_ccb->p_next_ccb;
        if (p_lcb->p_first_ccb) {
            p_lcb->p_first_ccb->p_prev_ccb = NULL;
        }
    } else if (p_lcb->p_last_ccb == p_ccb) {
        p_lcb->p_last_ccb = p_ccb->p_prev_ccb;
        p_lcb->p_last_ccb->p_next_ccb = NULL;
    } else {
        p_ccb->p_prev_ccb->p_next_ccb = p_ccb->p_next_ccb;
        p_ccb->p_next_ccb->p_prev_ccb = p_ccb->p_prev_ccb;
    }

    if (!l2cb.p_free_ccb_first) {
        l2cb.p_free_ccb_first = p_ccb;
        l2cb.p_free_ccb_last = p_ccb;
        p_ccb->p_next_ccb = NULL;
        p_ccb->p_prev_ccb = NULL;
    } else {
        p_ccb->p_next_ccb = NULL;
        p_ccb->p_prev_ccb = l2cb.p_free_ccb_last;
        l2cb.p_free_ccb_last->p_next_ccb = p_ccb;
        l2cb.p_free_ccb_last = p_ccb;
    }

    if (p_lcb->in_use && p_lcb->link_state == LST_CONNECTED && !p_lcb->p_first_ccb) {
        timeout = p_lcb->idle_timeout;

        if (timeout == 0) {
            rc = btm_sec_disconnect(p_lcb->handle, HCI_ERR_PEER_USER);
            if (rc == BTM_CMD_STARTED) {
                p_lcb->link_state = LST_DISCONNECTING;
                timeout = L2CAP_LINK_DISCONNECT_TOUT;
            } else if (rc == BTM_SUCCESS) {
                p_lcb->link_state = LST_DISCONNECTING;
                timeout = 0xFFFF;
            } else {
                timeout = 1;
            }
        }

        if (timeout != 0xFFFF) {
            btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, timeout);
        }
    }
}

/* 0x804987A8 (0x5C): Returns the in-use channel with `local_cid`, on `p_lcb` when it is given, or NULL. */
tL2C_CCB* l2cu_find_ccb_by_cid(tL2C_LCB* p_lcb, u16 local_cid) {
    tL2C_CCB* p_ccb = NULL;

    if (local_cid >= L2CAP_BASE_APPL_CID) {
        p_ccb = &l2cb.ccb_pool[(u16)(local_cid - L2CAP_BASE_APPL_CID)];

        if (!p_ccb->in_use) {
            p_ccb = NULL;
        } else if (p_lcb && p_lcb != p_ccb->p_lcb) {
            p_ccb = NULL;
        }
    }

    return p_ccb;
}

/* 0x80498804 (0x4C): Takes a free registration record for `psm`, or returns NULL. */
tL2C_RCB* l2cu_allocate_rcb(u16 psm) {
    tL2C_RCB* p_rcb = &l2cb.rcb_pool[0];
    u16 xx;

    for (xx = 0; xx < MAX_L2CAP_CLIENTS; xx++, p_rcb++) {
        if (!p_rcb->in_use) {
            p_rcb->in_use = TRUE;
            p_rcb->psm = psm;
            return p_rcb;
        }
    }

    return NULL;
}

/* 0x80498850 (0x10): Frees a registration record. */
void l2cu_release_rcb(tL2C_RCB* p_rcb) {
    p_rcb->in_use = FALSE;
    p_rcb->psm = 0;
}

/* 0x80498860 (0xB8): Returns the registration record of `psm`, or NULL. */
tL2C_RCB* l2cu_find_rcb_by_psm(u16 psm) {
    tL2C_RCB* p_rcb = &l2cb.rcb_pool[0];
    u16 xx;

    for (xx = 0; xx < MAX_L2CAP_CLIENTS; xx++, p_rcb++) {
        if (p_rcb->in_use && p_rcb->psm == psm) {
            return p_rcb;
        }
    }

    return NULL;
}

/* 0x80498918 (0x148): Checks the peer's configuration request; on a bad value rewrites `p_cfg` into the rejection. */
u8 l2cu_process_peer_cfg_req(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg) {
    u8 mtu_ok = TRUE;
    u8 qos_type_ok = TRUE;
    u8 flush_to_ok = TRUE;
    u8 cfg_ok;

    if (p_cfg->mtu_present) {
        if (p_cfg->mtu >= L2CAP_MIN_MTU) {
            p_ccb->peer_mtu = p_cfg->mtu;
            if (p_ccb->peer_mtu > L2CAP_MTU_SIZE) {
                p_ccb->peer_mtu = p_cfg->mtu = L2CAP_MTU_SIZE;
            }
        } else {
            mtu_ok = FALSE;
            p_cfg->mtu = L2CAP_MIN_MTU;
        }
    }

    if (p_cfg->flush_to_present) {
        if (p_cfg->flush_to == 0) {
            p_cfg->flush_to = 0xFFFF;
            flush_to_ok = FALSE;
        }
    }

    if (p_cfg->qos_present) {
        if (p_cfg->qos.service_type <= SVC_TYPE_GUARANTEED) {
            p_ccb->peer_qos = p_cfg->qos;
        } else {
            p_cfg->qos.service_type = SVC_TYPE_BEST_EFFORT;
            qos_type_ok = FALSE;
        }
    }

    cfg_ok = FALSE;
    if ((mtu_ok && flush_to_ok && qos_type_ok) != FALSE) {
        cfg_ok = TRUE;
    }

    if (!cfg_ok) {
        p_cfg->result = L2CAP_CFG_UNACCEPTABLE_PARAMS;

        if (mtu_ok) {
            p_cfg->mtu_present = FALSE;
        }
        if (flush_to_ok) {
            p_cfg->flush_to_present = FALSE;
        }
        if (qos_type_ok) {
            p_cfg->qos_present = FALSE;
        }
        p_cfg->fcr_present = FALSE;
    }

    return cfg_ok;
}

/* 0x80498A60 (0x40): Takes the QoS the peer answered our configuration with. */
void l2cu_process_peer_cfg_rsp(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg) {
    if (p_cfg->qos_present) {
        p_ccb->our_qos = p_cfg->qos;
    }
}

/* 0x80498AA0 (0x110): Records our configuration request and lowers the link's automatic flush timeout to it. */
void l2cu_process_our_cfg_req(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg) {
    tL2C_LCB* p_lcb;
    u16 hci_flush_to;
    BT_HDR* p_buf;

    if (p_cfg->mtu_present) {
        p_ccb->our_mtu = p_cfg->mtu;
        if (p_ccb->our_mtu > L2CAP_MTU_SIZE) {
            p_ccb->our_mtu = p_cfg->mtu = L2CAP_MTU_SIZE;
        }
    }

    if (p_cfg->qos_present) {
        p_ccb->our_qos = p_cfg->qos;
    }

    if (p_cfg->flush_to_present && p_cfg->flush_to != 0) {
        p_ccb->our_flush_to = p_cfg->flush_to;
        p_lcb = p_ccb->p_lcb;

        if (p_cfg->flush_to < p_lcb->link_flush_tout) {
            p_lcb->link_flush_tout = p_cfg->flush_to;

            if (p_cfg->flush_to <= L2CAP_MAX_FLUSH_TOUT_MS) {
                hci_flush_to = (u16)(((p_cfg->flush_to * 8) + 3) / 5);

                if ((p_buf = (BT_HDR*)GKI_getpoolbuf(L2CAP_CMD_POOL_ID)) != NULL) {
                    btsnd_hcic_write_auto_flush_tout(p_buf, p_lcb->handle, hci_flush_to);
                }
            }
        }
    }
}

/* 0x80498BB0 (0x40): Takes the QoS we answered the peer's configuration with. */
void l2cu_process_our_cfg_rsp(tL2C_CCB* p_ccb, tL2CAP_CFG_INFO* p_cfg) {
    if (p_cfg->qos_present) {
        p_ccb->peer_qos = p_cfg->qos;
    }
}

/* 0x80498BF0 (0x6C): Drops every connected link after a controller reset. */
void l2cu_device_reset(void) {
    int xx;
    tL2C_LCB* p_lcb = &l2cb.lcb_pool[0];

    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (p_lcb->in_use && p_lcb->handle != 0xFFFF) {
            l2c_link_hci_disc_comp(p_lcb->handle, (u8)-1);
        }
    }
}

/* 0x80498C5C (0x118): Pages the peer of `p_lcb`, first switching a link where we are slave to master when possible. */
u8 l2cu_create_conn(tL2C_LCB* p_lcb) {
    int xx;
    tL2C_LCB* p_lcb_cur;
    u8 is_sco_active;

    p_lcb->link_state = LST_CONNECTING;

    p_lcb_cur = &l2cb.lcb_pool[0];

    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb_cur++) {
        if (p_lcb_cur == p_lcb) {
            continue;
        }

        if (p_lcb_cur->in_use && p_lcb_cur->link_role == HCI_ROLE_SLAVE) {
            is_sco_active = btm_is_sco_active_by_bdaddr(p_lcb_cur->remote_bd_addr);

            L2CAP_TRACE_API1("l2cu_create_conn - btm_is_sco_active_by_bdaddr() is_sco_active = %s",
                             (is_sco_active == TRUE) ? "TRUE" : "FALSE");

            if (is_sco_active == TRUE) {
                continue;
            }

            if (btm_cb.local_features[0] & 0x20) {
                p_lcb->link_state = LST_CONNECTING_WAIT_SWITCH;
                p_lcb->link_role = HCI_ROLE_MASTER;

                BTM_SwitchRole(p_lcb_cur->remote_bd_addr, HCI_ROLE_MASTER, NULL);

                btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_LINK_ROLE_SWITCH_TOUT);
                return TRUE;
            }
        }
    }

    return l2cu_create_conn_after_switch(p_lcb);
}

/* 0x80498D74 (0xD0): Sends the HCI create-connection command for `p_lcb`, with the paging data of the inquiry. */
u8 l2cu_create_conn_after_switch(tL2C_LCB* p_lcb) {
    u8 allow_switch;
    tBTM_INQ_INFO* p_inq_info;
    u8 page_scan_rep_mode;
    u8 page_scan_mode;
    u16 clock_offset;
    u8* p_features;

    p_features = BTM_ReadLocalFeatures();
    allow_switch = (p_features[0] >> 5) & 1;

    p_lcb->link_state = LST_CONNECTING;

    if ((p_inq_info = BTM_InqDbRead(p_lcb->remote_bd_addr)) != NULL) {
        page_scan_rep_mode = p_inq_info->page_scan_rep_mode;
        page_scan_mode = p_inq_info->page_scan_mode;
        clock_offset = (u16)(p_inq_info->clock_offset | BTM_CLOCK_OFFSET_VALID);
    } else {
        page_scan_rep_mode = 1;
        page_scan_mode = 0;
        clock_offset = 0;
    }

    if (!btsnd_hcic_create_conn(p_lcb->remote_bd_addr, HCI_PKT_TYPES_MASK_DM1 + HCI_PKT_TYPES_MASK_DH1,
                                page_scan_rep_mode, page_scan_mode, clock_offset, allow_switch)) {
        L2CAP_TRACE_ERROR0("L2CAP - no buffer for l2cu_create_conn");
        l2cu_release_lcb(p_lcb);
        return FALSE;
    }

    btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_LINK_CONNECT_TOUT);
    return TRUE;
}

/* 0x80498E44 (0x90): Returns the first in-use link in `state`, or NULL. */
tL2C_LCB* l2cu_find_lcb_by_state(tL2C_LINK_STATE state) {
    int xx;
    tL2C_LCB* p_lcb = &l2cb.lcb_pool[0];

    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (p_lcb->in_use && p_lcb->link_state == state) {
            return p_lcb;
        }
    }

    return NULL;
}

/* 0x80498ED4 (0xE8): Returns whether a link has no channels, is disconnecting, or is closing its last channel. */
u8 l2cu_lcb_disconnecting(void) {
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;
    u16 i;
    u8 status = FALSE;

    p_lcb = &l2cb.lcb_pool[0];
    for (i = 0; i < MAX_L2CAP_LINKS; i++, p_lcb++) {
        if (p_lcb->in_use) {
            if (!p_lcb->p_first_ccb || p_lcb->link_state == LST_DISCONNECTING) {
                status = TRUE;
                break;
            } else if (p_lcb->p_first_ccb == p_lcb->p_last_ccb) {
                p_ccb = p_lcb->p_first_ccb;
                if (p_ccb->in_use && (p_ccb->chnl_state == CST_W4_L2CAP_DISCONNECT_RSP ||
                                      p_ccb->chnl_state == CST_W4_L2CA_DISCONNECT_RSP)) {
                    status = TRUE;
                    break;
                }
            }
        }
    }

    return status;
}
