/*
 * BTE/l2c_link.c - the L2CAP link layer: HCI connection events, link timeouts, the ACL send path and buffer allocation,
 *   packet-start/end checks, role changes and the PIN code request.
 *
 * RANGE. .text 0x80494F3C-0x804960C8 (17 functions, 0x118C B); .data 0x80616AA8-0x80616D18. The start is pinned by the
 *   .data readers; the end is medium confidence: the 4-byte pad before 0x80616D18 (0x80616D14 -> 0x80616D18) allows any
 *   cut in [0x80496084, 0x804961DC], and the BTE source keeps l2c_pin_code_request in l2c_link.c and l2c_init in
 *   l2c_main.c. The 4-byte pad inside the .data at 0x80616AF0 is not a seam: the string at 0x80616AA8 is read by both
 *   l2c_link_hci_conn_req and l2c_link_check_send_pkts, one pooled literal.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, per object in configure.py; the evidence is on BTE/l2c_utils.c's FLAGS
 *   line. The function starts of this band are packed to 4 bytes.
 * NAMES. the BTE source's.
 * RESIDUALS. l2c_link_sec_comp: the target keeps -(status == 0) in r31 and adds 8 inside the loop; this source
 *   folds the whole event out of the loop (an event local and an inline ternary measured the same).
 *   l2c_link_adjust_allocation: yy and p_lcb take r28/r29 swapped against the target (every declaration
 *   order, a combined for-init and a separate first-loop counter measured the same). flipcheck also
 *   refuses on LogMsg_6, btm_acl_created, btm_cb, btsnd_hcic_accept_conn and btu_cb: the map rows the source names are
 *   still generated (requests bte-l2cap#2/#3/#5/#6/#7/#8/#13/#23/#26), so a flip links against undefined symbols.
 * SHAPES. plain C; l2c_link_sec_comp is the security manager's callback (its middle parameter is the reference the
 *   caller registered, unused here).
 */

#include "BTE/l2c_link.h"
#include "BTE/l2c_csm.h"
#include "BTE/l2c_main.h"
#include "BTE/l2c_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The HCI reasons this layer refuses an incoming connection with. */
#define HCI_ERR_PEER_LOW_RESOURCES 0x14
#define HCI_ERR_PEER_POWER_OFF 0x15
/* The security reject reason a disconnect must not overwrite. */
#define HCI_ERR_HOST_REJECT_SECURITY 0x0E
/* The largest reassembled ACL packet. */
#define L2CAP_MAX_RCV_ACL_LEN 0x069F
/* The congestion start threshold in percent of the link's quota. */
#define L2CAP_CONGESTION_PERCENT 120

/* 0x80494F3C (0x154): Accepts or refuses an incoming ACL connection from `bd_addr`. */
void l2c_link_hci_conn_req(u8* bd_addr) {
    tL2C_LCB* p_lcb;
    BT_HDR* p_buf = (BT_HDR*)GKI_getpoolbuf(L2CAP_CMD_POOL_ID);

    if (!p_buf) {
        return;
    }

    p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr);

    if (!p_lcb) {
        p_lcb = l2cu_allocate_lcb(bd_addr);

        if (!p_lcb) {
            btsnd_hcic_reject_conn(p_buf, bd_addr, HCI_ERR_PEER_LOW_RESOURCES);
            L2CAP_TRACE_ERROR0("L2CAP failed to allocate LCB");
            return;
        }

        p_lcb->link_role = l2cu_get_conn_role(p_lcb);
        btsnd_hcic_accept_conn(p_buf, bd_addr, p_lcb->link_role);

        p_lcb->link_state = LST_CONNECTING;

        btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_LINK_CONNECT_TOUT);
        return;
    }

    if (p_lcb->link_state == LST_CONNECTING || p_lcb->link_state == LST_CONNECT_HOLDING) {
        btsnd_hcic_accept_conn(p_buf, bd_addr, p_lcb->link_role);
        p_lcb->link_state = LST_CONNECTING;
    } else if (p_lcb->link_state == LST_DISCONNECTING) {
        btsnd_hcic_reject_conn(p_buf, bd_addr, HCI_ERR_PEER_POWER_OFF);
    } else {
        L2CAP_TRACE_ERROR0("L2CAP got conn_req while connected");
        GKI_freebuf(p_buf);
    }
}

/* 0x80495090 (0x238): Completes a link: on success connects its channels, otherwise holds or drops it. */
u8 l2c_link_hci_conn_comp(u8 status, u16 handle, u8* p_bda) {
    tL2C_CONN_INFO ci;
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;
    tL2C_CCB* p_next_ccb;
    tBTM_SEC_DEV_REC* p_dev_info;

    ci.status = status;
    memcpy(ci.bd_addr, p_bda, 6);

    p_lcb = l2cu_find_lcb_by_bd_addr(ci.bd_addr);

    if (!p_lcb) {
        if (!btm_sec_is_bonding(handle)) {
            L2CAP_TRACE_WARNING0("L2CAP got conn_comp for unknown BD_ADDR");
        }
        return FALSE;
    }

    if (p_lcb->link_state != LST_CONNECTING) {
        L2CAP_TRACE_ERROR2("L2CAP got conn_comp in bad state: %d  status: 0x%d", p_lcb->link_state, status);

        if (status != 0) {
            l2c_link_hci_disc_comp(p_lcb->handle, status);
        }
        return FALSE;
    }

    p_lcb->handle = handle;

    if (ci.status == 0) {
        p_lcb->link_state = LST_CONNECTED;

        if ((p_dev_info = btm_find_dev(p_bda)) != NULL) {
            btm_acl_created(ci.bd_addr, p_dev_info->dev_class, p_dev_info->sec_bd_name, handle, p_lcb->link_role);
        } else {
            btm_acl_created(ci.bd_addr, NULL, NULL, handle, p_lcb->link_role);
        }

        l2c_process_held_packets(FALSE);

        btu_stop_timer(&p_lcb->timer_entry);

        for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_ccb->p_next_ccb) {
            l2c_csm_execute(p_ccb, L2CEVT_LP_CONNECT_CFM, &ci);
        }

        if (p_lcb->p_echo_rsp_cb) {
            l2cu_send_peer_echo_req(p_lcb, NULL, 0);
            btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_ECHO_RSP_TOUT);
        } else if (!p_lcb->p_first_ccb) {
            btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_LINK_STARTUP_TOUT);
        }
    } else if (ci.status == HCI_ERR_MAX_NUM_OF_CONNECTIONS && l2cu_lcb_disconnecting()) {
        p_lcb->link_state = LST_CONNECT_HOLDING;
        p_lcb->handle = 0xFFFF;
    } else {
        for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_next_ccb) {
            p_next_ccb = p_ccb->p_next_ccb;
            l2c_csm_execute(p_ccb, L2CEVT_LP_CONNECT_CFM_NEG, &ci);
        }

        l2cu_release_lcb(p_lcb);
    }

    return TRUE;
}

/* 0x804952C8 (0xB4): Passes the security result for the link to `p_bda` to each of its channels. */
/* untyped: an opaque handle passed through - the reference the security manager was given */
void l2c_link_sec_comp(u8* p_bda, void* p_ref_data, u8 status) {
    u8 event;
    tL2C_CONN_INFO ci;
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;
    tL2C_CCB* p_next_ccb;

    ci.status = status;
    memcpy(ci.bd_addr, p_bda, 6);

    p_lcb = l2cu_find_lcb_by_bd_addr(p_bda);

    if (!p_lcb) {
        L2CAP_TRACE_WARNING0("L2CAP got sec_comp for unknown BD_ADDR");
        return;
    }

    for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_next_ccb) {
        p_next_ccb = p_ccb->p_next_ccb;
        event = (status == 0) ? L2CEVT_SEC_COMP : L2CEVT_SEC_COMP_NEG;
        l2c_csm_execute(p_ccb, event, &ci);
    }
}

/* 0x8049537C (0xBC): A link went down: tells its channels, releases it and restarts a held connection. */
u8 l2c_link_hci_disc_comp(u16 handle, u8 reason) {
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;
    u8 status = TRUE;

    p_lcb = l2cu_find_lcb_by_handle(handle);

    if (!p_lcb) {
        status = FALSE;
    } else {
        if (btm_cb.acl_disc_reason != HCI_ERR_HOST_REJECT_SECURITY) {
            btm_cb.acl_disc_reason = reason;
        }

        for (p_ccb = p_lcb->p_first_ccb; p_ccb;) {
            tL2C_CCB* pn = p_ccb->p_next_ccb;

            l2c_csm_execute(p_ccb, L2CEVT_LP_DISCONNECT_IND, &reason);
            p_ccb = pn;
        }

        btm_sco_acl_removed(p_lcb->remote_bd_addr);

        l2cu_release_lcb(p_lcb);
    }

    if ((p_lcb = l2cu_find_lcb_by_state(LST_CONNECT_HOLDING)) != NULL) {
        l2cu_create_conn(p_lcb);
    }

    return status;
}

/* 0x80495438 (0x70): Tells every channel of the link `handle` whose client listens that its QoS was broken. */
u8 l2c_link_hci_qos_violation(u16 handle) {
    tL2C_LCB* p_lcb;
    tL2C_CCB* p_ccb;

    p_lcb = l2cu_find_lcb_by_handle(handle);

    if (!p_lcb) {
        return FALSE;
    }

    for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_ccb->p_next_ccb) {
        if (p_ccb->p_rcb->api.pL2CA_QoSViolationInd_Cb) {
            l2c_csm_execute(p_ccb, L2CEVT_LP_QOS_VIOLATION_IND, NULL);
        }
    }

    return TRUE;
}

/* 0x804954A8 (0x174): Handles the link timer: drops a link stuck connecting, ends a ping, or idles an unused link out. */
void l2c_link_timeout(tL2C_LCB* p_lcb) {
    tL2C_CCB* p_ccb;
    tL2C_CCB* p_next_ccb;
    u16 timeout;
    u8 rc;
    tL2CA_ECHO_RSP_CB* p_cb;

    if (p_lcb->link_state == LST_CONNECTING_WAIT_SWITCH || p_lcb->link_state == LST_CONNECTING ||
        p_lcb->link_state == LST_CONNECT_HOLDING || p_lcb->link_state == LST_DISCONNECTING) {
        for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_next_ccb) {
            p_next_ccb = p_ccb->p_next_ccb;
            l2c_csm_execute(p_ccb, L2CEVT_LP_DISCONNECT_IND, NULL);
        }

        l2cu_release_lcb(p_lcb);
    }

    if (p_lcb->link_state == LST_CONNECTED) {
        if (p_lcb->p_echo_rsp_cb) {
            p_cb = p_lcb->p_echo_rsp_cb;
            p_lcb->p_echo_rsp_cb = NULL;
            (*p_cb)(L2CAP_PING_RESULT_NO_RESP);

            L2CAP_TRACE_WARNING0("L2CAP - ping timeout");

            for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_next_ccb) {
                p_next_ccb = p_ccb->p_next_ccb;
                l2c_csm_execute(p_ccb, L2CEVT_LP_DISCONNECT_IND, NULL);
            }
        }

        if (!p_lcb->p_first_ccb) {
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

            if (timeout != 0xFFFF) {
                btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, timeout);
            }
        } else {
            l2c_link_check_send_pkts(p_lcb, NULL, NULL);
        }
    }
}

/* 0x8049561C (0x108): Hands one ACL buffer to the transport, charging the link and controller windows per segment. */
static u8 l2c_link_send_to_lower(tL2C_LCB* p_lcb, BT_HDR* p_buf) {
    u16 num_segs;
    u16 acl_data_size;

    if (p_buf->len <= btu_cb.hcit_acl_pkt_size) {
        p_lcb->link_xmit_window--;
        p_lcb->sent_not_acked++;
        l2cb.controller_xmit_window--;

        bte_hcisu_send(p_buf, BT_EVT_TO_LM_HCI_ACL);
    } else {
        acl_data_size = btu_cb.hcit_acl_data_size;
        num_segs = (p_buf->len - HCI_DATA_PREAMBLE_SIZE + acl_data_size - 1) / acl_data_size;

        if (num_segs > l2cb.controller_xmit_window) {
            num_segs = l2cb.controller_xmit_window;
            p_buf->layer_specific = num_segs;
            p_lcb->partial_segment_being_sent = TRUE;
        }

        if (num_segs > p_lcb->link_xmit_window) {
            num_segs = p_lcb->link_xmit_window;
            p_buf->layer_specific = num_segs;
            p_lcb->partial_segment_being_sent = TRUE;
        }

        l2cb.controller_xmit_window -= num_segs;
        p_lcb->link_xmit_window -= num_segs;
        p_lcb->sent_not_acked += num_segs;

        bte_hcisu_send(p_buf, BT_EVT_TO_LM_HCI_ACL);
    }

    return TRUE;
}

/* 0x80495724 (0x2CC): Queues `p_buf` on the link (reporting congestion), then sends what the windows allow. */
void l2c_link_check_send_pkts(tL2C_LCB* p_lcb, tL2C_CCB* p_ccb, BT_HDR* p_buf) {
    u8 mode;
    BtmPmPwrMode pm;

    if (p_buf) {
        if (p_lcb->link_xmit_data_q.count >= p_lcb->cong_start_thresh && !p_lcb->link_congested) {
            for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_ccb->p_next_ccb) {
                L2CAP_TRACE_WARNING1("L2CAP - Congested(), CID: 0x%04x, Congested", p_ccb->local_cid);

                if (p_ccb->p_rcb->api.pL2CA_CongestionStatus_Cb) {
                    L2CAP_TRACE_API1("L2CAP - Calling CongestionStatus_Cb(), CID: 0x%04x, Congested",
                                     p_ccb->local_cid);
                    (*p_ccb->p_rcb->api.pL2CA_CongestionStatus_Cb)(p_ccb->local_cid, TRUE);
                }
            }

            p_lcb->link_congested = TRUE;
        }

        if (p_lcb->link_xmit_data_q.count <= p_lcb->discard_thresh) {
            p_buf->layer_specific = 0;
            GKI_enqueue(&p_lcb->link_xmit_data_q, p_buf);
        } else {
            GKI_freebuf(p_buf);
            L2CAP_TRACE_WARNING0("!!! L2CAP - buffer dropped");
        }
    }

    if (p_lcb->partial_segment_being_sent) {
        return;
    }

    if (BTM_ReadPowerMode(p_lcb->remote_bd_addr, &mode) == BTM_SUCCESS && mode == BTM_PM_STS_SNIFF &&
        p_lcb->link_xmit_data_q.count) {
        pm.mode = BTM_PM_MD_ACTIVE;
        BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p_lcb->remote_bd_addr, &pm);
        btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, 1);
        return;
    }

    while (l2cb.controller_xmit_window != 0 && p_lcb->link_xmit_window != 0) {
        if (!p_lcb->link_xmit_data_q.p_first || ((BT_HDR*)p_lcb->link_xmit_data_q.p_first)->layer_specific != 0) {
            break;
        }

        if (p_lcb->partial_segment_being_sent) {
            return;
        }

        p_buf = (BT_HDR*)GKI_dequeue(&p_lcb->link_xmit_data_q);
        if (!p_buf || !l2c_link_send_to_lower(p_lcb, p_buf)) {
            break;
        }
    }

    if (p_lcb->partial_segment_being_sent) {
        return;
    }

    if (p_lcb->link_xmit_data_q.count && p_lcb->link_xmit_window) {
        btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_LINK_FLOW_CONTROL_TOUT);
    }

    if (p_lcb->link_congested && p_lcb->link_xmit_data_q.count <= p_lcb->cong_end_thresh) {
        p_lcb->link_congested = FALSE;

        for (p_ccb = p_lcb->p_first_ccb; p_ccb; p_ccb = p_ccb->p_next_ccb) {
            if (p_ccb->p_rcb->api.pL2CA_CongestionStatus_Cb) {
                L2CAP_TRACE_API1("L2CAP - Calling CongestionStatus_Cb(), CID: 0x%04x, Uncongested", p_ccb->local_cid);
                (*p_ccb->p_rcb->api.pL2CA_CongestionStatus_Cb)(p_ccb->local_cid, FALSE);

                if (p_lcb->link_congested) {
                    return;
                }
            }
        }
    }
}

/* 0x804959F0 (0x198): Splits the controller's ACL buffers between the active links, high priority first. */
void l2c_link_adjust_allocation(void) {
    u16 yy;
    tL2C_LCB* p_lcb;
    u16 quota;
    int num_hipri_links = 0;

    if (!l2cb.num_links_active) {
        return;
    }

    p_lcb = &l2cb.lcb_pool[0];
    for (yy = 0; yy < MAX_L2CAP_LINKS; yy++, p_lcb++) {
        if (p_lcb->in_use && p_lcb->acl_priority == L2CAP_PRIORITY_HIGH) {
            num_hipri_links++;
        }
    }

    quota = (l2cb.num_lm_acl_bufs / l2cb.num_links_active) + 1;

    p_lcb = &l2cb.lcb_pool[0];
    for (yy = 0; yy < MAX_L2CAP_LINKS; yy++, p_lcb++) {
        if (p_lcb->in_use) {
            if (!num_hipri_links || p_lcb->acl_priority == L2CAP_PRIORITY_HIGH) {
                p_lcb->link_xmit_window = p_lcb->link_xmit_quota = quota;
            } else {
                p_lcb->link_xmit_window = p_lcb->link_xmit_quota = L2CAP_SEC_LINK_WINDOW;
            }

            p_lcb->cong_start_thresh = (p_lcb->link_xmit_quota * L2CAP_CONGESTION_PERCENT) / 100;
            p_lcb->cong_end_thresh = (p_lcb->cong_start_thresh > L2CAP_SEC_MIN_CONGESTION)
                                         ? p_lcb->cong_start_thresh - L2CAP_SEC_MIN_CONGESTION
                                         : 0;
            p_lcb->discard_thresh = p_lcb->cong_start_thresh + 2;

            L2CAP_TRACE_DEBUG6("LCB %d Priority:%d XmitWindow:%d Congestion Start:%d End:%d Discard:%d", yy,
                               p_lcb->acl_priority, p_lcb->link_xmit_window, p_lcb->cong_start_thresh,
                               p_lcb->cong_end_thresh, p_lcb->discard_thresh);
        }
    }
}

/* 0x80495B88 (0xE4): Returns the completed packets of each listed link to its and the controller's windows. */
void l2c_link_process_num_completed_pkts(u8* p) {
    u8 num_handles;
    u8 xx;
    u16 handle;
    u16 num_sent;
    tL2C_LCB* p_lcb;

    STREAM_TO_UINT8(num_handles, p);

    for (xx = 0; xx < num_handles; xx++) {
        STREAM_TO_UINT16(handle, p);
        STREAM_TO_UINT16(num_sent, p);

        p_lcb = l2cu_find_lcb_by_handle(handle);

        if (p_lcb) {
            p_lcb->link_xmit_window += num_sent;
            if (p_lcb->link_xmit_window > p_lcb->link_xmit_quota) {
                p_lcb->link_xmit_window = p_lcb->link_xmit_quota;
            }

            if (p_lcb->sent_not_acked > num_sent) {
                p_lcb->sent_not_acked -= num_sent;
            } else {
                p_lcb->sent_not_acked = 0;
            }

            l2cb.controller_xmit_window += num_sent;

            l2c_link_check_send_pkts(p_lcb, NULL, NULL);
        }
    }
}

/* 0x80495C6C (0x14): Records the controller's ACL buffer count. */
void l2c_link_processs_num_bufs(u16 num_lm_acl_bufs) {
    l2cb.controller_xmit_window = l2cb.num_lm_acl_bufs = num_lm_acl_bufs;
}

/* 0x80495C80 (0x1D0): Starts or extends the packet being reassembled on the link; returns it, or NULL to wait. */
BT_HDR* l2cap_link_chk_pkt_start(BT_HDR* p_cur_buf) {
    u8* p;
    u16 handle;
    u16 hci_len;
    u16 pkt_type;
    tL2C_LCB* p_lcb;
    BT_HDR* p_return_buf;
    u8* p_f;
    u16 total_len;

    p = (u8*)&p_cur_buf[1] + p_cur_buf->offset;
    p_cur_buf->layer_specific = 0;

    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT16(hci_len, p);

    pkt_type = (handle >> HCI_DATA_EVENT_OFFSET) & HCI_DATA_EVENT_MASK;
    handle = handle & HCI_DATA_HANDLE_MASK;

    l2cb.p_cur_hcit_lcb = NULL;

    p_lcb = l2cu_find_lcb_by_handle(handle);

    if (!p_lcb) {
        return p_cur_buf;
    }

    if (pkt_type == L2CAP_PKT_START) {
        if (p_lcb->p_hcit_rcv_acl) {
            L2CAP_TRACE_WARNING0("L2CAP - dropping incomplete pkt");
            GKI_freebuf(p_lcb->p_hcit_rcv_acl);
        }

        p_lcb->p_hcit_rcv_acl = p_cur_buf;
        p_return_buf = p_cur_buf;
        l2cb.p_cur_hcit_lcb = p_lcb;
    } else if ((p_return_buf = p_lcb->p_hcit_rcv_acl) != NULL) {
        p_f = (u8*)&p_return_buf[1] + p_return_buf->offset + 2;
        total_len = p_f[0] + (p_f[1] << 8);

        if (total_len + hci_len <= L2CAP_MAX_RCV_ACL_LEN) {
            l2cb.p_cur_hcit_lcb = p_lcb;

            if (p_cur_buf->len > HCI_DATA_PREAMBLE_SIZE) {
                memcpy((u8*)&p_return_buf[1] + p_return_buf->offset + p_return_buf->len,
                       (u8*)&p_cur_buf[1] + p_cur_buf->offset + HCI_DATA_PREAMBLE_SIZE,
                       p_cur_buf->len - HCI_DATA_PREAMBLE_SIZE);
                p_return_buf->len += p_cur_buf->len - HCI_DATA_PREAMBLE_SIZE;
            }

            GKI_freebuf(p_cur_buf);

            total_len += hci_len;
            UINT16_TO_STREAM(p_f, total_len);
        } else {
            L2CAP_TRACE_WARNING0("L2CAP - dropping too long pkt");
            GKI_freebuf(p_lcb->p_hcit_rcv_acl);
            p_lcb->p_hcit_rcv_acl = NULL;
            p_return_buf = NULL;
        }
    } else {
        p_return_buf = NULL;
    }

    if (p_return_buf == NULL) {
        GKI_freebuf(p_cur_buf);
    }

    return p_return_buf;
}

/* 0x80495E50 (0x6C): Returns whether the packet being reassembled is complete, then lets it go. */
u8 l2cap_link_chk_pkt_end(void) {
    u8* p;
    BT_HDR* p_buf;
    u16 l2cap_len;
    tL2C_LCB* p_lcb = l2cb.p_cur_hcit_lcb;

    if (!p_lcb || (p_buf = p_lcb->p_hcit_rcv_acl) == NULL) {
        return TRUE;
    }

    p = ((BT_BUF*)p_buf)->data + p_buf->offset + HCI_DATA_PREAMBLE_SIZE;
    STREAM_TO_UINT16(l2cap_len, p);

    if (l2cap_len > p_buf->len - (HCI_DATA_PREAMBLE_SIZE + L2CAP_PKT_OVERHEAD)) {
        return FALSE;
    }

    p_lcb->p_hcit_rcv_acl = NULL;
    return TRUE;
}

/* 0x80495EBC (0x80): Records the new role of the link to `bd_addr` and resumes links that waited for the switch. */
void l2c_link_role_changed(u8* bd_addr, u8 new_role) {
    tL2C_LCB* p_lcb;
    int xx;

    p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr);
    if (p_lcb) {
        p_lcb->link_role = new_role;
    }

    p_lcb = &l2cb.lcb_pool[0];
    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (p_lcb->in_use && p_lcb->link_state == LST_CONNECTING_WAIT_SWITCH) {
            l2cu_create_conn_after_switch(p_lcb);
        }
    }
}

/* 0x80495F3C (0x6C): Resumes the links that waited for a role switch that failed. */
void l2c_link_role_change_failed(void) {
    tL2C_LCB* p_lcb;
    int xx;

    p_lcb = &l2cb.lcb_pool[0];
    for (xx = 0; xx < MAX_L2CAP_LINKS; xx++, p_lcb++) {
        if (p_lcb->in_use && p_lcb->link_state == LST_CONNECTING_WAIT_SWITCH) {
            l2cu_create_conn_after_switch(p_lcb);
        }
    }
}

/* 0x80495FA8 (0xDC): Puts a partly sent buffer back at the head of its link's queue and resumes sending. */
void l2c_link_segments_xmitted(BT_HDR* p_msg) {
    u8* p;
    u16 handle;
    tL2C_LCB* p_lcb;

    p = &((BT_BUF*)p_msg)->data[p_msg->offset];
    STREAM_TO_UINT16(handle, p);
    handle = handle & HCI_DATA_HANDLE_MASK;

    if ((p_lcb = l2cu_find_lcb_by_handle(handle)) == NULL) {
        L2CAP_TRACE_WARNING1("L2CAP - rcvd segment complete, unknown handle: %d", handle);
        GKI_freebuf(p_msg);
        return;
    }

    if (p_lcb->link_state == LST_CONNECTED) {
        p_msg->layer_specific = 0;
        GKI_enqueue_head(&p_lcb->link_xmit_data_q, p_msg);

        p_lcb->partial_segment_being_sent = FALSE;

        l2c_link_check_send_pkts(p_lcb, NULL, NULL);
    } else {
        GKI_freebuf(p_msg);
    }
}

/* 0x80496084 (0x44): Extends the link timer of an unused link while the user enters a PIN. */
void l2c_pin_code_request(u8* bd_addr) {
    tL2C_LCB* p_lcb = l2cu_find_lcb_by_bd_addr(bd_addr);

    if (p_lcb && !p_lcb->p_first_ccb) {
        btu_start_timer(&p_lcb->timer_entry, BTU_TTYPE_L2CAP_LINK, L2CAP_LINK_CONNECT_TOUT_EXT);
    }
}
