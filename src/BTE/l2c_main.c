/*
 * BTE/l2c_main.c - the L2CAP main module: l2c_init, the ACL receive path, the signalling command processor, the timeout
 *   dispatcher and the held-packet queue.
 *
 * RANGE. .text 0x804960C8-0x804970F8 (5 functions, 0x1030 B); .data 0x80616D18-0x80617018; .bss 0x8073C760-0x8073CF48.
 *   The start is medium confidence (BTE/l2c_link.c's header); the end is strong: the .data ends at the 4-byte pad
 *   before 0x80617018, whose readers are the l2cu_* family. The .bss is the L2CAP control block (0x7E8 B, read by every
 *   l2c unit and bta); its owner is a GUESS from the BTE source (l2c_main.c defines it) - any l2c unit gives the same
 *   .bss order.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, per object in configure.py; the evidence is on BTE/l2c_utils.c's FLAGS
 *   line. The function starts of this band are packed to 4 bytes.
 * NAMES. the BTE source's (l2cb is the control block). GUESS: BTU_TTYPE_L2CAP_INFO for the timer type 73
 *   l2c_process_timeout ignores.
 * RESIDUALS. process_l2cap_cmd: the constant 0 kept in r31 is materialised a few instructions later than the
 *   target's. flipcheck also refuses on LogMsg_5 and btu_cb: the map rows the source names are still generated
 *   (requests bte-l2cap#2/#3/#5/#6/#7/#8/#13/#23/#26), so a flip links against undefined symbols.
 * SHAPES. plain C.
 */

#include "BTE/l2c_main.h"
#include "BTE/l2c_csm.h"
#include "BTE/l2c_link.h"
#include "BTE/l2c_utils.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The timer type of an information request; this build ignores it. */
#define BTU_TTYPE_L2CAP_INFO 73
/* The hold time of a packet that arrives before its link, in seconds. */
#define L2CAP_HOLD_TOUT 1
/* The layer_specific mark of a held packet: how many hold timeouts it survives. */
#define L2CAP_HOLD_COUNT 2

/* .bss 0x8073C760 - the L2CAP control block. */
tL2C_CB l2cb;

static void process_l2cap_cmd(tL2C_LCB* p_lcb, u8* p, u16 pkt_len);

/* 0x804960C8 (0x114): Clears the control block and chains every channel into the free list. */
void l2c_init(void) {
    int xx;

    memset(&l2cb, 0, sizeof(tL2C_CB));

    for (xx = 0; xx < MAX_L2CAP_CHANNELS - 1; xx++) {
        l2cb.ccb_pool[xx].p_next_ccb = &l2cb.ccb_pool[xx + 1];
    }

    l2cb.p_free_ccb_first = &l2cb.ccb_pool[0];
    l2cb.p_free_ccb_last = &l2cb.ccb_pool[MAX_L2CAP_CHANNELS - 1];

    l2cb.desire_role = HCI_ROLE_MASTER;
    l2cb.idle_timeout = L2CAP_LINK_INACTIVITY_TOUT;

    l2cb.l2cb_trace_level = 0;
}

/* 0x804961DC (0x330): Routes one received ACL packet to the signalling code or its channel; holds early requests. */
void l2c_rcv_acl_data(BT_HDR* p_msg) {
    u8* p = (u8*)&p_msg[1] + p_msg->offset;
    u16 handle;
    u16 hci_len;
    u8 pkt_type;
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb = NULL;
    u16 l2cap_len;
    u16 rcv_cid;
    u16 psm;
    u8 cmd_code;

    STREAM_TO_UINT16(handle, p);
    pkt_type = (handle >> HCI_DATA_EVENT_OFFSET) & HCI_DATA_EVENT_MASK;
    handle = handle & HCI_DATA_HANDLE_MASK;

    if (pkt_type != L2CAP_PKT_START) {
        L2CAP_TRACE_WARNING1("L2CAP - expected pkt start, got: %d", pkt_type);
        GKI_freebuf(p_msg);
        return;
    }

    if ((p_lcb = l2cu_find_lcb_by_handle(handle)) == NULL) {
        STREAM_TO_UINT16(hci_len, p);
        STREAM_TO_UINT16(l2cap_len, p);
        STREAM_TO_UINT16(rcv_cid, p);
        STREAM_TO_UINT8(cmd_code, p);

        if (p_msg->layer_specific == 0 && rcv_cid == L2CAP_SIGNALLING_CID &&
            (cmd_code == L2CAP_CMD_INFO_REQ || cmd_code == L2CAP_CMD_CONN_REQ) && !btm_sec_is_bonding(handle)) {
            L2CAP_TRACE_WARNING5("L2CAP - holding ACL for unknown handle:%d ls:%d cid:%d opcode:%d cur count:%d",
                                 handle, p_msg->layer_specific, rcv_cid, cmd_code, l2cb.rcv_hold_q.count);
            p_msg->layer_specific = L2CAP_HOLD_COUNT;
            GKI_enqueue(&l2cb.rcv_hold_q, p_msg);

            if (l2cb.rcv_hold_q.count == 1) {
                btu_start_timer(&l2cb.rcv_hold_tle, BTU_TTYPE_L2CAP_HOLD, L2CAP_HOLD_TOUT);
            }
        } else {
            GKI_freebuf(p_msg);
        }
        return;
    }

    STREAM_TO_UINT16(hci_len, p);
    p_msg->offset += HCI_DATA_PREAMBLE_SIZE;

    STREAM_TO_UINT16(l2cap_len, p);
    STREAM_TO_UINT16(rcv_cid, p);

    if (rcv_cid > L2CAP_CONNECTIONLESS_CID) {
        if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, rcv_cid)) == NULL) {
            L2CAP_TRACE_WARNING1("L2CAP - unknown CID: %d", rcv_cid);
            GKI_freebuf(p_msg);
            return;
        }
    }

    if (hci_len >= L2CAP_PKT_OVERHEAD) {
        p_msg->len = hci_len - L2CAP_PKT_OVERHEAD;
        p_msg->offset += L2CAP_PKT_OVERHEAD;
    } else {
        L2CAP_TRACE_WARNING0("L2CAP - got incorrect hci header");
        GKI_freebuf(p_msg);
        return;
    }

    if (l2cap_len != p_msg->len) {
        L2CAP_TRACE_WARNING2("L2CAP - bad length in pkt. Exp: %d  Act: %d", l2cap_len, p_msg->len);
        GKI_freebuf(p_msg);
        return;
    }

    if (rcv_cid == L2CAP_SIGNALLING_CID) {
        process_l2cap_cmd(p_lcb, p, l2cap_len);
        GKI_freebuf(p_msg);
    } else if (rcv_cid == L2CAP_CONNECTIONLESS_CID) {
        STREAM_TO_UINT16(psm, p);
        p_msg->offset += 2;
        p_msg->len -= 2;

        L2CAP_TRACE_DEBUG1("GOT CONNECTIONLESS DATA PSM:%d", psm);
        GKI_freebuf(p_msg);
    } else if (p_ccb == NULL) {
        GKI_freebuf(p_msg);
    } else {
        l2c_csm_execute(p_ccb, L2CEVT_L2CAP_DATA, p_msg);
    }
}

/* 0x8049650C (0xA70): Runs each signalling command of a packet: rejects, connect/config/disconnect, echo and info. */
static void process_l2cap_cmd(tL2C_LCB* p_lcb, u8* p, u16 pkt_len) {
    u8* p_pkt_end;
    u8* p_next_cmd;
    u8* p_cfg_end;
    u8* p_cfg_start;
    u8 cmd_code;
    u8 cfg_code;
    u8 cfg_len;
    u8 id;
    tL2C_CONN_INFO con_info;
    tL2CAP_CFG_INFO cfg_info;
    u16 rej_reason;
    u16 rej_mtu;
    u16 lcid;
    u16 rcid;
    u16 info_type;
    tL2C_CCB* p_ccb;
    tL2C_RCB* p_rcb;
    u8 cfg_rej;
    u16 cfg_rej_len;
    u16 cmd_len;
    tL2CA_ECHO_RSP_CB* p_cb;

    p_next_cmd = p;
    p_pkt_end = p + pkt_len;

    while (TRUE) {
        if ((p = p_next_cmd) > p_pkt_end - 4) {
            break;
        }

        STREAM_TO_UINT8(cmd_code, p);
        STREAM_TO_UINT8(id, p);
        STREAM_TO_UINT16(cmd_len, p);

        if ((p_next_cmd = p + cmd_len) > p_pkt_end) {
            L2CAP_TRACE_WARNING3("Command len bad  pkt_len: %d  cmd_len: %d  code: %d", pkt_len, cmd_len, cmd_code);
            break;
        }

        switch (cmd_code) {
        case L2CAP_CMD_REJECT:
            STREAM_TO_UINT16(rej_reason, p);
            if (rej_reason == L2CAP_CMD_REJ_MTU_EXCEEDED) {
                STREAM_TO_UINT16(rej_mtu, p);
                L2CAP_TRACE_WARNING2("L2CAP - MTU rej Handle: %d MTU: %d", p_lcb->handle, rej_mtu);
            }
            if (rej_reason == L2CAP_CMD_REJ_INVALID_CID) {
                STREAM_TO_UINT16(rcid, p);
                STREAM_TO_UINT16(lcid, p);

                L2CAP_TRACE_WARNING2("L2CAP - rej with CID invalid, LCID: %d RCID: %d", lcid, rcid);

                if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) != NULL && p_ccb->remote_cid == rcid) {
                    l2c_csm_execute(p_ccb, L2CEVT_LP_DISCONNECT_IND, NULL);
                }
            }
            break;

        case L2CAP_CMD_CONN_REQ:
            STREAM_TO_UINT16(con_info.psm, p);
            STREAM_TO_UINT16(rcid, p);

            if ((p_rcb = l2cu_find_rcb_by_psm(con_info.psm)) == NULL) {
                L2CAP_TRACE_WARNING1("L2CAP - rcvd conn req for unknown PSM: %d", con_info.psm);
                l2cu_reject_connection(p_lcb, rcid, id, L2CAP_CONN_NO_PSM);
                break;
            }

            if ((p_ccb = l2cu_allocate_ccb(p_lcb)) == NULL) {
                L2CAP_TRACE_ERROR0("L2CAP - unable to allocate CCB");
                l2cu_reject_connection(p_lcb, rcid, id, L2CAP_CONN_NO_RESOURCES);
                break;
            }

            p_ccb->remote_id = id;
            p_ccb->p_rcb = p_rcb;
            p_ccb->remote_cid = rcid;

            l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_REQ, &con_info);
            break;

        case L2CAP_CMD_CONN_RSP:
            STREAM_TO_UINT16(con_info.remote_cid, p);
            STREAM_TO_UINT16(lcid, p);
            STREAM_TO_UINT16(con_info.l2cap_result, p);
            STREAM_TO_UINT16(con_info.l2cap_status, p);

            if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) == NULL) {
                L2CAP_TRACE_WARNING2("L2CAP - no CCB for conn rsp, LCID: %d RCID: %d", lcid, con_info.remote_cid);
                break;
            }
            if (p_ccb->local_id != id) {
                L2CAP_TRACE_WARNING2("L2CAP - con rsp - bad ID. Exp: %d Got: %d", p_ccb->local_id, id);
                break;
            }

            if (con_info.l2cap_result == L2CAP_CONN_OK) {
                l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP, &con_info);
            } else if (con_info.l2cap_result == L2CAP_CONN_PENDING) {
                l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP_PND, &con_info);
            } else {
                l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONNECT_RSP_NEG, &con_info);
            }
            break;

        case L2CAP_CMD_CONFIG_REQ:
            p_cfg_end = p + cmd_len;
            cfg_rej = FALSE;
            cfg_rej_len = 0;

            STREAM_TO_UINT16(lcid, p);
            STREAM_TO_UINT16(cfg_info.flags, p);

            p_cfg_start = p;

            cfg_info.flush_to_present = cfg_info.mtu_present = cfg_info.qos_present =
                cfg_info.fcr_present = FALSE;

            while (p < p_cfg_end) {
                STREAM_TO_UINT8(cfg_code, p);
                STREAM_TO_UINT8(cfg_len, p);

                switch (cfg_code & 0x7F) {
                case L2CAP_CFG_TYPE_MTU:
                    cfg_info.mtu_present = TRUE;
                    STREAM_TO_UINT16(cfg_info.mtu, p);
                    break;

                case L2CAP_CFG_TYPE_FLUSH_TOUT:
                    cfg_info.flush_to_present = TRUE;
                    STREAM_TO_UINT16(cfg_info.flush_to, p);
                    break;

                case L2CAP_CFG_TYPE_QOS:
                    cfg_info.qos_present = TRUE;
                    STREAM_TO_UINT8(cfg_info.qos.qos_flags, p);
                    STREAM_TO_UINT8(cfg_info.qos.service_type, p);
                    STREAM_TO_UINT32(cfg_info.qos.token_rate, p);
                    STREAM_TO_UINT32(cfg_info.qos.token_bucket_size, p);
                    STREAM_TO_UINT32(cfg_info.qos.peak_bandwidth, p);
                    STREAM_TO_UINT32(cfg_info.qos.latency, p);
                    STREAM_TO_UINT32(cfg_info.qos.delay_variation, p);
                    break;

                case L2CAP_CFG_TYPE_FCR:
                    cfg_info.fcr_present = TRUE;
                    STREAM_TO_UINT8(cfg_info.fcr.mode, p);
                    STREAM_TO_UINT8(cfg_info.fcr.tx_win_sz, p);
                    STREAM_TO_UINT8(cfg_info.fcr.max_transmit, p);
                    STREAM_TO_UINT16(cfg_info.fcr.rtrans_tout, p);
                    STREAM_TO_UINT16(cfg_info.fcr.mon_tout, p);
                    STREAM_TO_UINT16(cfg_info.fcr.mps, p);
                    break;

                default:
                    if (cfg_len + L2CAP_CFG_OPTION_OVERHEAD <= cmd_len) {
                        p += cfg_len;
                        if ((cfg_code & 0x80) == 0) {
                            cfg_rej_len += cfg_len + L2CAP_CFG_OPTION_OVERHEAD;
                            cfg_rej = TRUE;
                        }
                    } else {
                        p = p_cfg_end;
                    }
                    break;
                }
            }

            if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) != NULL) {
                p_ccb->remote_id = id;
                if (cfg_rej) {
                    l2cu_send_peer_config_rej(p_ccb, p_cfg_start, (u16)(cmd_len - L2CAP_CONFIG_REQ_LEN), cfg_rej_len);
                } else {
                    l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONFIG_REQ, &cfg_info);
                }
            } else {
                l2cu_send_peer_cmd_reject(p_lcb, L2CAP_CMD_REJ_INVALID_CID, id, 0, 0);
            }
            break;

        case L2CAP_CMD_CONFIG_RSP:
            p_cfg_end = p + cmd_len;

            STREAM_TO_UINT16(lcid, p);
            STREAM_TO_UINT16(cfg_info.flags, p);
            STREAM_TO_UINT16(cfg_info.result, p);

            cfg_info.fcr_present = cfg_info.flush_to_present = cfg_info.mtu_present =
                cfg_info.qos_present = FALSE;

            while (p < p_cfg_end) {
                STREAM_TO_UINT8(cfg_code, p);
                STREAM_TO_UINT8(cfg_len, p);

                switch (cfg_code & 0x7F) {
                case L2CAP_CFG_TYPE_MTU:
                    cfg_info.mtu_present = TRUE;
                    STREAM_TO_UINT16(cfg_info.mtu, p);
                    break;

                case L2CAP_CFG_TYPE_FLUSH_TOUT:
                    cfg_info.flush_to_present = TRUE;
                    STREAM_TO_UINT16(cfg_info.flush_to, p);
                    break;

                case L2CAP_CFG_TYPE_QOS:
                    cfg_info.qos_present = TRUE;
                    STREAM_TO_UINT8(cfg_info.qos.qos_flags, p);
                    STREAM_TO_UINT8(cfg_info.qos.service_type, p);
                    STREAM_TO_UINT32(cfg_info.qos.token_rate, p);
                    STREAM_TO_UINT32(cfg_info.qos.token_bucket_size, p);
                    STREAM_TO_UINT32(cfg_info.qos.peak_bandwidth, p);
                    STREAM_TO_UINT32(cfg_info.qos.latency, p);
                    STREAM_TO_UINT32(cfg_info.qos.delay_variation, p);
                    break;

                case L2CAP_CFG_TYPE_FCR:
                    cfg_info.fcr_present = TRUE;
                    STREAM_TO_UINT8(cfg_info.fcr.mode, p);
                    STREAM_TO_UINT8(cfg_info.fcr.tx_win_sz, p);
                    STREAM_TO_UINT8(cfg_info.fcr.max_transmit, p);
                    STREAM_TO_UINT16(cfg_info.fcr.rtrans_tout, p);
                    STREAM_TO_UINT16(cfg_info.fcr.mon_tout, p);
                    STREAM_TO_UINT16(cfg_info.fcr.mps, p);
                    break;
                }
            }

            if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) != NULL) {
                if (p_ccb->local_id != id) {
                    L2CAP_TRACE_WARNING2("L2CAP - cfg rsp - bad ID. Exp: %d Got: %d", p_ccb->local_id, id);
                    break;
                }
                if (cfg_info.result == L2CAP_CFG_OK) {
                    l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONFIG_RSP, &cfg_info);
                } else {
                    l2c_csm_execute(p_ccb, L2CEVT_L2CAP_CONFIG_RSP_NEG, &cfg_info);
                }
            } else {
                L2CAP_TRACE_WARNING1("L2CAP - rcvd cfg rsp for unknown CID: %d", lcid);
            }
            break;

        case L2CAP_CMD_DISC_REQ:
            STREAM_TO_UINT16(lcid, p);
            STREAM_TO_UINT16(rcid, p);

            if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) != NULL) {
                if (p_ccb->remote_cid == rcid) {
                    p_ccb->remote_id = id;
                    l2c_csm_execute(p_ccb, L2CEVT_L2CAP_DISCONNECT_REQ, &con_info);
                }
            } else {
                l2cu_send_peer_disc_rsp(p_lcb, id, lcid, rcid);
            }
            break;

        case L2CAP_CMD_DISC_RSP:
            STREAM_TO_UINT16(rcid, p);
            STREAM_TO_UINT16(lcid, p);

            if ((p_ccb = l2cu_find_ccb_by_cid(p_lcb, lcid)) != NULL) {
                if (p_ccb->remote_cid == rcid && p_ccb->local_id == id) {
                    l2c_csm_execute(p_ccb, L2CEVT_L2CAP_DISCONNECT_RSP, &con_info);
                }
            }
            break;

        case L2CAP_CMD_ECHO_REQ:
            if (cmd_len < btu_cb.hcit_acl_pkt_size - (L2CAP_PKT_OVERHEAD + L2CAP_CMD_OVERHEAD + L2CAP_ECHO_RSP_LEN +
                                                       HCI_DATA_PREAMBLE_SIZE)) {
                l2cu_send_peer_echo_rsp(p_lcb, id, p, cmd_len);
            } else {
                l2cu_send_peer_echo_rsp(p_lcb, id, NULL, 0);
            }
            break;

        case L2CAP_CMD_ECHO_RSP:
            if (p_lcb->p_echo_rsp_cb) {
                p_cb = p_lcb->p_echo_rsp_cb;
                p_lcb->p_echo_rsp_cb = NULL;
                (*p_cb)(L2CAP_PING_RESULT_OK);
            }
            break;

        case L2CAP_CMD_INFO_REQ:
            STREAM_TO_UINT16(info_type, p);
            l2cu_send_peer_info_rsp(p_lcb, id, info_type);
            break;

        case L2CAP_CMD_INFO_RSP:
            break;

        default:
            L2CAP_TRACE_WARNING1("L2CAP - bad cmd code: %d", cmd_code);
            l2cu_send_peer_cmd_reject(p_lcb, L2CAP_CMD_REJ_NOT_UNDERSTOOD, id, 0, 0);
            return;
        }
    }
}

/* 0x80496F7C (0x50): Dispatches an expired L2CAP timer to its link, channel or the hold queue. */
void l2c_process_timeout(TIMER_LIST_ENT* p_tle) {
    switch (p_tle->event) {
    case BTU_TTYPE_L2CAP_LINK:
        l2c_link_timeout((tL2C_LCB*)p_tle->param);
        break;

    case BTU_TTYPE_L2CAP_CHNL:
        l2c_csm_execute((tL2C_CCB*)p_tle->param, L2CEVT_TIMEOUT, NULL);
        break;

    case BTU_TTYPE_L2CAP_HOLD:
        l2c_process_held_packets(TRUE);
        break;

    case BTU_TTYPE_L2CAP_INFO:
        break;
    }
}

/* 0x80496FCC (0x12C): Replays the held packets whose link has come up (or whose hold ran out). */
void l2c_process_held_packets(u8 timed_out) {
    BT_HDR* p_buf;
    BT_HDR* p_buf1;
    BUFFER_Q* p_rcv_hold_q = &l2cb.rcv_hold_q;

    if (!p_rcv_hold_q->count) {
        return;
    }

    if (!timed_out) {
        btu_stop_timer(&l2cb.rcv_hold_tle);
        L2CAP_TRACE_WARNING0("L2CAP HOLD CONTINUE");
    } else {
        L2CAP_TRACE_WARNING0("L2CAP HOLD TIMEOUT");
    }

    for (p_buf = (BT_HDR*)GKI_getfirst(p_rcv_hold_q); p_buf; p_buf = p_buf1) {
        p_buf1 = (BT_HDR*)GKI_getnext(p_buf);

        if (!timed_out || !p_buf->layer_specific || --p_buf->layer_specific == 0) {
            GKI_remove_from_queue(p_rcv_hold_q, p_buf);
            p_buf->layer_specific = 0xFFFF;
            l2c_rcv_acl_data(p_buf);
        }
    }

    if (p_rcv_hold_q->count) {
        btu_start_timer(&l2cb.rcv_hold_tle, BTU_TTYPE_L2CAP_HOLD, L2CAP_HOLD_TOUT);
    }
}
