/*
 * BTE/port_utils.c - the RFCOMM port utilities: MTU selection, port release and lookup, user and peer flow control.
 *
 * RANGE. .text 0x8049A408-0x8049A9E0 (7 functions, 0x5D8 B); .data 0x80617430-0x80617578. The .text holds the BTE
 *   port_utils.c functions port_select_mtu..port_flow_control_peer; rfc_l2cap_if.c's rfcomm_l2cap_if_init follows. The
 *   .data starts after a 4-byte pad beyond 4-byte alignment (0x8061742C -> 0x80617430: a new 8-aligned contribution).
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. none: .text and the claimed data are byte-identical to the target (the .data tail is alignment fill).
 * SHAPES. plain C.
 */

#include "BTE/port_utils.h"
#include "BTE/btm_acl.h"
#include "BTE/port_api.h"
#include "BTE/rfc_port_if.h"
#include "BTE/rfc_ts_frames.h"
#include "BTE/rfc_utils.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The L2CAP MTU and packet overhead an RFCOMM frame's size is chosen against, and the MTU without a link. */
#define L2CAP_MTU_SIZE 1691
#define L2CAP_PKT_OVERHEAD 4
#define RFCOMM_DEFAULT_MTU 127

/* 0x8049A408 (0x17C): Chooses the port's MTU from the link's packet size and derives its receive credits. */
void port_select_mtu(tPORT* p_port) {
    u16 packet_size;

    if (p_port->mtu == 0) {
        packet_size = btm_get_max_packet_size(p_port->bd_addr);
        if (packet_size == 0) {
            RFCOMM_TRACE_WARNING0("port_select_mtu bad packet size");
            p_port->mtu = RFCOMM_DEFAULT_MTU;
        } else if ((L2CAP_MTU_SIZE + L2CAP_PKT_OVERHEAD) >= packet_size) {
            p_port->mtu = ((L2CAP_MTU_SIZE + L2CAP_PKT_OVERHEAD) / packet_size * packet_size) - RFCOMM_DATA_OVERHEAD -
                          L2CAP_PKT_OVERHEAD;
            RFCOMM_TRACE_DEBUG1("port_select_mtu selected %d based on connection speed", p_port->mtu);
        } else {
            p_port->mtu = L2CAP_MTU_SIZE - RFCOMM_DATA_OVERHEAD;
            RFCOMM_TRACE_DEBUG1("port_select_mtu selected %d based on l2cap PDU size", p_port->mtu);
        }
    } else {
        RFCOMM_TRACE_DEBUG1("port_select_mtu application selected %d", p_port->mtu);
    }
    p_port->credit_rx_max = PORT_RX_HIGH_WM / p_port->mtu;
    p_port->credit_rx_low = PORT_RX_LOW_WM / p_port->mtu;
    p_port->rx_buf_critical = PORT_RX_CRITICAL_WM / p_port->mtu;
    RFCOMM_TRACE_DEBUG3("port_select_mtu credit_rx_max %d, credit_rx_low %d, rx_buf_critical %d",
                        p_port->credit_rx_max, p_port->credit_rx_low, p_port->rx_buf_critical);
}

/* 0x8049A584 (0xDC): Frees the port's queued data and, once its DLC is closed, frees the port itself. */
void port_release_port(tPORT* p_port) {
    BT_HDR* p_buf;

    while ((p_buf = GKI_dequeue(&p_port->rx.queue)) != NULL) {
        GKI_freebuf(p_buf);
    }
    p_port->rx.queue_size = 0;
    while ((p_buf = GKI_dequeue(&p_port->tx.queue)) != NULL) {
        GKI_freebuf(p_buf);
    }
    p_port->tx.queue_size = 0;
    p_port->state = PORT_STATE_CLOSED;
    if (p_port->rfc.state == RFC_STATE_CLOSED) {
        RFCOMM_TRACE_DEBUG0("rfc_port_closed DONE");
        if (p_port->rfc.p_mcb) {
            p_port->rfc.p_mcb->port_inx[p_port->dlci] = 0;
            rfc_check_mcb_active(p_port->rfc.p_mcb);
        }
        rfc_port_timer_stop(p_port);
        memset(p_port, 0, sizeof(tPORT));
    }
}

/* 0x8049A660 (0x54): Returns the port of the multiplexer's DLCI, or NULL. */
tPORT* port_find_mcb_dlci_port(tRFC_MCB* p_mcb, u8 dlci) {
    u8 inx;

    if (!p_mcb) {
        return NULL;
    }
    if (dlci > RFCOMM_MAX_DLCI) {
        return NULL;
    }
    inx = p_mcb->port_inx[dlci];
    if (inx == 0) {
        return NULL;
    }
    return &rfc_cb.port.port[inx - 1];
}

/* 0x8049A6B4 (0x88): Returns the unattached port waiting for the DLCI (or its server-side twin), or NULL. */
tPORT* port_find_dlci_port(u8 dlci) {
    u16 i;
    tPORT* p_port;

    for (i = 0; i < MAX_RFC_PORTS; i++) {
        p_port = &rfc_cb.port.port[i];
        if (p_port->in_use && p_port->rfc.p_mcb == NULL) {
            if (p_port->dlci == dlci) {
                return p_port;
            } else if ((dlci & 0x01) && (p_port->dlci == (dlci - 1))) {
                p_port->dlci++;
                return p_port;
            }
        }
    }
    return NULL;
}

/* 0x8049A73C (0x70): Updates the user flow control from the peer and the send queue; returns the events to report. */
u32 port_flow_control_user(tPORT* p_port) {
    u32 event = 0;
    u8 fc = p_port->tx.peer_fc || !p_port->rfc.p_mcb || !p_port->rfc.p_mcb->peer_ready ||
            (p_port->tx.queue_size > PORT_TX_HIGH_WM) || (p_port->tx.queue.count > PORT_TX_BUF_HIGH_WM);

    if (p_port->tx.user_fc == fc) {
        return 0;
    }
    p_port->tx.user_fc = fc;
    if (fc) {
        event = PORT_EV_FC;
    } else {
        event = PORT_EV_FC | PORT_EV_FCS;
    }
    return event;
}

/* 0x8049A7AC (0x68): Returns the masked events the change from `old_signals` to `signal` raises. */
u32 port_get_signal_changes(tPORT* p_port, u8 old_signals, u8 signal) {
    u8 changed_signals = signal ^ old_signals;
    u32 events = 0;

    if (changed_signals & PORT_DTRDSR_ON) {
        events |= PORT_EV_DSR;
        if (signal & PORT_DTRDSR_ON) {
            events |= PORT_EV_DSRS;
        }
    }
    if (changed_signals & PORT_CTSRTS_ON) {
        events |= PORT_EV_CTS;
        if (signal & PORT_CTSRTS_ON) {
            events |= PORT_EV_CTSS;
        }
    }
    if (changed_signals & PORT_RING_ON) {
        events |= PORT_EV_RING;
    }
    if (changed_signals & PORT_DCD_ON) {
        events |= PORT_EV_RLSD;
        if (signal & PORT_DCD_ON) {
            events |= PORT_EV_RLSDS;
        }
    }
    return p_port->ev_mask & events;
}

/* 0x8049A814 (0x1CC): Enables or disables the peer's sending to the port by credits or by TS 07.10 flow control. */
void port_flow_control_peer(tPORT* p_port, u8 enable, u16 count) {
    if (!p_port->rfc.p_mcb) {
        return;
    }
    if (p_port->rfc.p_mcb->flow == PORT_FC_CREDIT) {
        if (enable) {
            if (count > p_port->credit_rx) {
                p_port->credit_rx = 0;
            } else {
                p_port->credit_rx -= count;
            }
            if ((p_port->credit_rx <= p_port->credit_rx_low) && !p_port->rx.user_fc &&
                (p_port->credit_rx_max > p_port->credit_rx)) {
                rfc_send_credit(p_port->rfc.p_mcb, p_port->dlci, (u8)(p_port->credit_rx_max - p_port->credit_rx));
                p_port->credit_rx = p_port->credit_rx_max;
                p_port->rx.peer_fc = FALSE;
            }
        } else {
            if (p_port->p_data_callback) {
                p_port->rx.peer_fc = TRUE;
            } else if (p_port->rx.queue.count >= p_port->credit_rx_max) {
                p_port->rx.peer_fc = TRUE;
            }
        }
    } else {
        if (enable) {
            if (p_port->rx.peer_fc && (p_port->rx.queue_size < PORT_RX_LOW_WM) &&
                (p_port->rx.queue.count < PORT_RX_BUF_LOW_WM)) {
                p_port->rx.peer_fc = FALSE;
                if (!p_port->rx.user_fc) {
                    RFCOMM_FlowReq(p_port->rfc.p_mcb, p_port->dlci, TRUE);
                }
            }
        } else {
            if (p_port->p_data_callback) {
                p_port->rx.peer_fc = TRUE;
                RFCOMM_FlowReq(p_port->rfc.p_mcb, p_port->dlci, FALSE);
            } else if (((p_port->rx.queue_size > PORT_RX_HIGH_WM) || (p_port->rx.queue.count > PORT_RX_BUF_HIGH_WM)) &&
                       !p_port->rx.peer_fc) {
                RFCOMM_TRACE_EVENT0("PORT_DataInd Data reached HW. Sending FC set.");
                p_port->rx.peer_fc = TRUE;
                RFCOMM_FlowReq(p_port->rfc.p_mcb, p_port->dlci, FALSE);
            }
        }
    }
}
