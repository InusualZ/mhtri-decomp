/*
 * BTE/port_rfc.c - the RFCOMM port layer's indications and confirmations from the RFCOMM state machines (PORT_StartCnf
 *   .. port_rfc_closed).
 *
 * RANGE. .text 0x8049900C-0x8049A408 (18 functions, 0x13FC B); .data 0x80617100-0x80617430. The .text holds the BTE
 *   port_rfc.c functions PORT_StartCnf..port_rfc_closed; port_utils.c's port_select_mtu follows. The .data (trace
 *   strings) starts after the last BTE/gki_time.c string with no pad beyond 4-byte alignment: the seam is where the
 *   referrers change (every object is read only by this range's functions).
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. port_open_continue (0xE4 B in this object) is dead-stripped in retail (its only caller is the stripped
 *   PORT_* API); its body is a GUESS from the BTE source with the IDLE-state RFCOMM_StartReq branch left out (that
 *   callee is stripped too and has no declaration here). It is written for its two trace strings, which open the
 *   target's .data pool (0x38 B) and set PORT_PortNegCnf's pool-relative string offsets (+0x150/+0x178).
 *   flipcheck refuses on the .text size (0x14E0 vs 0x13FC); a trial link with this unit Matching gives the retail DOL
 *   hash, so mwld strips the function.
 * SHAPES. plain C; PORT_PortNegInd stores the peer's parameters before answering even when no port takes the DLCI
 *   (the retail body writes them through the NULL port).
 */

#include "BTE/port_rfc.h"
#include "BTE/port_api.h"
#include "BTE/port_utils.h"
#include "BTE/rfc_port_if.h"
#include "BTE/rfc_ts_frames.h"
#include "BTE/rfc_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The L2CAP result PORT_StartCnf maps to a page timeout. */
#define HCI_ERR_PAGE_TIMEOUT 4

/* The flow control a multiplexer uses until the peer decides. */
#define PORT_FC_DEFAULT PORT_FC_CREDIT

/* The line status error bits a line status indication carries. */
#define PORT_ERR_BREAK 0x01
#define PORT_ERR_OVERRUN 0x02

/* The result port_open_continue returns when no multiplexer can be allocated. */
#define PORT_NO_RESOURCES 6

/* Attaches a port to its device's multiplexer and starts the parameter negotiation once the multiplexer is up. */
int port_open_continue(tPORT* p_port);

int port_open_continue(tPORT* p_port) {
    tRFC_MCB* p_mcb;

    RFCOMM_TRACE_EVENT0("port_open_continue");

    p_mcb = rfc_alloc_multiplexer_channel(p_port->bd_addr, TRUE);
    if (p_mcb == NULL) {
        RFCOMM_TRACE_WARNING0("port_open_continue no mx channel");
        port_release_port(p_port);
        return PORT_NO_RESOURCES;
    }

    p_port->rfc.p_mcb = p_mcb;
    p_mcb->port_inx[p_port->dlci] = p_port->inx;

    port_select_mtu(p_port);

    if (p_mcb->state == RFC_MX_STATE_CONNECTED) {
        RFCOMM_ParNegReq(p_mcb, p_port->dlci, p_port->mtu);
    }
    return PORT_SUCCESS;
}

/* 0x8049900C (0x160): Starts the parameter negotiation of every port of the multiplexer, or fails them. */
void PORT_StartCnf(tRFC_MCB* p_mcb, u16 result) {
    tPORT* p_port;
    int i;
    u8 no_ports_up = TRUE;

    RFCOMM_TRACE_EVENT1("PORT_StartCnf result:%d", result);

    p_port = &rfc_cb.port.port[0];
    for (i = 0; i < MAX_RFC_PORTS; i++, p_port++) {
        if (p_port->rfc.p_mcb == p_mcb) {
            no_ports_up = FALSE;

            if (result == RFCOMM_SUCCESS) {
                RFCOMM_ParNegReq(p_mcb, p_port->dlci, p_port->mtu);
            } else {
                RFCOMM_TRACE_WARNING1("PORT_StartCnf failed result:%d", result);

                if (result == HCI_ERR_PAGE_TIMEOUT) {
                    p_port->error = PORT_PAGE_TIMEOUT;
                } else {
                    p_port->error = PORT_START_FAILED;
                }

                rfc_release_multiplexer_channel(p_mcb);
                p_port->rfc.p_mcb = NULL;

                if (p_port->p_callback && (PORT_EV_CONNECT_ERR & p_port->ev_mask)) {
                    (p_port->p_callback)(PORT_EV_CONNECT_ERR, p_port->inx);
                }

                if (p_port->p_mgmt_callback) {
                    p_port->p_mgmt_callback(PORT_START_FAILED, p_port->inx);
                }

                port_release_port(p_port);
            }
        }
    }

    if (no_ports_up) {
        rfc_check_mcb_active(p_mcb);
    }
}

/* 0x8049916C (0x9C): Accepts the peer's multiplexer start while a port is free or already on it. */
void PORT_StartInd(tRFC_MCB* p_mcb) {
    tPORT* p_port;
    int i;

    RFCOMM_TRACE_EVENT0("PORT_StartInd");

    p_port = &rfc_cb.port.port[0];
    for (i = 0; i < MAX_RFC_PORTS; i++, p_port++) {
        if (p_port->rfc.p_mcb == NULL || p_port->rfc.p_mcb == p_mcb) {
            RFCOMM_StartRsp(p_mcb, RFCOMM_SUCCESS);
            return;
        }
    }
    RFCOMM_StartRsp(p_mcb, RFCOMM_ERROR);
}

/* 0x80499208 (0x1B0): Attaches the DLC to a waiting port, settles MTU and flow control and answers the negotiation. */
void PORT_ParNegInd(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u8 cl, u8 k) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);
    u8 our_cl;
    u8 our_k;

    RFCOMM_TRACE_EVENT2("PORT_ParNegInd dlci:%d mtu:%d", dlci, mtu);

    if (!p_port) {
        p_port = port_find_dlci_port(dlci);
        if (!p_port) {
            rfc_send_dm(p_mcb, dlci, FALSE);
            rfc_check_mcb_active(p_mcb);
            RFCOMM_TRACE_EVENT0("PORT_ParNegInd: port not found");
            return;
        }
        p_mcb->port_inx[dlci] = p_port->inx;
    }

    memcpy(p_port->bd_addr, p_mcb->bd_addr, BD_ADDR_LEN);

    port_select_mtu(p_port);

    p_port->rfc.p_mcb = p_mcb;
    p_port->mtu = (p_port->mtu < mtu) ? p_port->mtu : mtu;
    p_port->peer_mtu = p_port->mtu;

    if (p_mcb->flow == PORT_FC_UNDEFINED) {
        if ((PORT_FC_DEFAULT == PORT_FC_TS710) || (cl == RFCOMM_PN_CONV_LAYER_TYPE_1)) {
            p_mcb->flow = PORT_FC_TS710;
        } else {
            p_mcb->flow = PORT_FC_CREDIT;
        }
    }

    if (cl == RFCOMM_PN_CONV_LAYER_TYPE_1) {
        our_cl = RFCOMM_PN_CONV_LAYER_TYPE_1;
        our_k = 0;
    } else if (p_mcb->flow == PORT_FC_CREDIT) {
        p_port->credit_tx = k;
        if (p_port->credit_tx == 0) {
            p_port->tx.peer_fc = TRUE;
        }

        our_cl = RFCOMM_PN_CONV_LAYER_CBFC_R;
        our_k = (p_port->credit_rx_max < RFCOMM_K_MAX) ? p_port->credit_rx_max : RFCOMM_K_MAX;
        p_port->credit_rx = our_k;
    } else {
        our_cl = RFCOMM_PN_CONV_LAYER_TYPE_1;
        our_k = 0;
    }
    RFCOMM_ParNegRsp(p_mcb, dlci, p_port->mtu, our_cl, our_k);
}

/* 0x804993B8 (0x104): Takes the answer to our parameter negotiation and opens the DLC of an opening port. */
void PORT_ParNegCnf(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u8 cl, u8 k) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    RFCOMM_TRACE_EVENT4("PORT_ParNegCnf dlci:%d mtu:%d cl: %d k: %d", dlci, mtu, cl, k);

    if (!p_port) {
        return;
    }

    if (p_mcb->flow == PORT_FC_UNDEFINED) {
        if (cl == RFCOMM_PN_CONV_LAYER_CBFC_R) {
            p_mcb->flow = PORT_FC_CREDIT;
        } else {
            p_mcb->flow = PORT_FC_TS710;
        }
    }

    p_port->mtu = (p_port->mtu < mtu) ? p_port->mtu : mtu;
    p_port->peer_mtu = p_port->mtu;

    if (p_mcb->flow == PORT_FC_CREDIT) {
        p_port->credit_tx = k;
        if (p_port->credit_tx == 0) {
            p_port->tx.peer_fc = TRUE;
        }
    }

    if (p_port->state == PORT_STATE_OPENING) {
        RFCOMM_DlcEstablishReq(p_mcb, p_port->dlci, p_port->mtu);
    }
}

/* 0x804994BC (0x140): Accepts the peer's DLC open on a waiting port and reports the connection. */
void PORT_DlcEstablishInd(tRFC_MCB* p_mcb, u8 dlci, u16 mtu) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    RFCOMM_TRACE_EVENT2("PORT_DlcEstablishInd dlci:%d mtu:%d", dlci, mtu);

    if (!p_port) {
        p_port = port_find_dlci_port(dlci);
        if (!p_port) {
            RFCOMM_DlcEstablishRsp(p_mcb, dlci, 0, RFCOMM_ERROR);
            return;
        }
        p_mcb->port_inx[dlci] = p_port->inx;
    }

    if (mtu && (mtu < p_port->peer_mtu)) {
        p_port->peer_mtu = mtu;
    }

    rfc_timer_stop(p_mcb);

    RFCOMM_DlcEstablishRsp(p_mcb, dlci, p_port->mtu, RFCOMM_SUCCESS);

    if (p_port->p_callback && (p_port->ev_mask & PORT_EV_CONNECTED)) {
        (p_port->p_callback)(PORT_EV_CONNECTED, p_port->inx);
    }

    if (p_port->p_mgmt_callback) {
        p_port->p_mgmt_callback(PORT_SUCCESS, p_port->inx);
    }

    p_port->state = PORT_STATE_OPENED;
}

/* 0x804995FC (0x144): Reports our DLC open's result and starts the port's control exchange. */
void PORT_DlcEstablishCnf(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u16 result) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    RFCOMM_TRACE_EVENT3("PORT_DlcEstablishCnf dlci:%d mtu:%d result:%d", dlci, mtu, result);

    if (!p_port) {
        return;
    }

    if (result != RFCOMM_SUCCESS) {
        p_port->error = PORT_START_FAILED;
        port_rfc_closed(p_port, PORT_START_FAILED);
        return;
    }

    if (mtu && (mtu < p_port->peer_mtu)) {
        p_port->peer_mtu = mtu;
    }

    rfc_timer_stop(p_mcb);

    if (p_port->p_callback && (p_port->ev_mask & PORT_EV_CONNECTED)) {
        (p_port->p_callback)(PORT_EV_CONNECTED, p_port->inx);
    }

    if (p_port->p_mgmt_callback) {
        p_port->p_mgmt_callback(PORT_SUCCESS, p_port->inx);
    }

    p_port->state = PORT_STATE_OPENED;

    if ((p_port->uuid == UUID_SERVCLASS_DIALUP_NETWORKING) || (p_port->uuid == UUID_SERVCLASS_FAX)) {
        RFCOMM_PortNegReq(p_port->rfc.p_mcb, p_port->dlci, NULL);
    } else {
        RFCOMM_ControlReq(p_port->rfc.p_mcb, p_port->dlci, &p_port->local_ctrl);
    }
}

/* 0x80499740 (0x150): Stores the peer's port parameters and answers its port negotiation. */
void PORT_PortNegInd(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars, u16 param_mask) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    RFCOMM_TRACE_EVENT0("PORT_PortNegInd");

    if (!p_port) {
        p_port = port_find_dlci_port(dlci);
        if (!p_port) {
            p_port->peer_port_pars = *p_pars;
            RFCOMM_PortNegRsp(p_mcb, dlci, p_pars, 0);
            return;
        }
        p_mcb->port_inx[dlci] = p_port->inx;
    }

    p_port->peer_port_pars = *p_pars;
    RFCOMM_PortNegRsp(p_mcb, dlci, p_pars, param_mask);
}

/* 0x80499890 (0x118): Takes the answer to our port negotiation and sends our modem status, or closes the port. */
void PORT_PortNegCnf(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars, u16 result) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    RFCOMM_TRACE_EVENT0("PORT_PortNegCnf");

    if (!p_port) {
        RFCOMM_TRACE_WARNING0("PORT_PortNegCnf no port");
        return;
    }

    if (result != RFCOMM_SUCCESS) {
        p_port->error = PORT_PORT_NEG_FAILED;
        RFCOMM_DlcReleaseReq(p_mcb, p_port->dlci);
        port_rfc_closed(p_port, PORT_PORT_NEG_FAILED);
        return;
    }

    if (!(p_port->port_ctrl & PORT_CTRL_REQ_SENT)) {
        RFCOMM_ControlReq(p_port->rfc.p_mcb, p_port->dlci, &p_port->local_ctrl);
    } else {
        RFCOMM_TRACE_WARNING0("PORT_PortNegCnf Control Already sent");
    }
}

/* 0x804999A8 (0x178): Stores the peer's modem status, answers it with ours and reports the signal changes. */
void PORT_ControlInd(tRFC_MCB* p_mcb, u8 dlci, tPORT_CTRL* p_pars) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);
    u32 event;
    u8 old_signals;

    RFCOMM_TRACE_EVENT0("PORT_ControlInd");

    if (!p_port) {
        return;
    }

    old_signals = p_port->peer_ctrl.modem_signal;

    event = port_get_signal_changes(p_port, old_signals, p_pars->modem_signal);

    p_port->peer_ctrl = *p_pars;

    if (!(p_port->port_ctrl & PORT_CTRL_REQ_SENT)) {
        RFCOMM_ControlReq(p_port->rfc.p_mcb, p_port->dlci, &p_port->local_ctrl);
    } else {
        if (!(p_port->port_ctrl & PORT_CTRL_IND_RECEIVED)) {
            event |= (PORT_EV_CONNECTED & p_port->ev_mask);
        }

        if (p_port->port_ctrl & PORT_CTRL_REQ_CONFIRMED) {
            event |= port_rfc_send_tx_data(p_port);
        }
    }

    p_port->port_ctrl |= (PORT_CTRL_IND_RECEIVED | PORT_CTRL_IND_RESPONDED);

    if (p_pars->break_signal) {
        event |= (PORT_EV_BREAK & p_port->ev_mask);
    }

    if (event && p_port->p_callback) {
        (p_port->p_callback)(event, p_port->inx);
    }

    RFCOMM_TRACE_EVENT4("PORT_ControlInd DTR_DSR : %d, RTS_CTS : %d, RI : %d, DCD : %d",
                        ((p_port->peer_ctrl.modem_signal & PORT_DTRDSR_ON) ? 1 : 0),
                        ((p_port->peer_ctrl.modem_signal & PORT_CTSRTS_ON) ? 1 : 0),
                        ((p_port->peer_ctrl.modem_signal & PORT_RING_ON) ? 1 : 0),
                        ((p_port->peer_ctrl.modem_signal & PORT_DCD_ON) ? 1 : 0));
}

/* 0x80499B20 (0xC4): Takes the answer to our modem status and sends the queued data once both sides exchanged. */
void PORT_ControlCnf(tRFC_MCB* p_mcb, u8 dlci, tPORT_CTRL* p_pars) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);
    u32 event = 0;

    RFCOMM_TRACE_EVENT0("PORT_ControlCnf");

    if (!p_port) {
        return;
    }

    if (!(p_port->port_ctrl & PORT_CTRL_REQ_CONFIRMED)) {
        p_port->port_ctrl |= PORT_CTRL_REQ_CONFIRMED;

        if (p_port->port_ctrl & PORT_CTRL_IND_RECEIVED) {
            event = (p_port->ev_mask & PORT_EV_CONNECTED);
        }
    }

    if (p_port->port_ctrl & PORT_CTRL_IND_RECEIVED) {
        event |= port_rfc_send_tx_data(p_port);
    }

    if (event && p_port->p_callback) {
        (p_port->p_callback)(event, p_port->inx);
    }
}

/* 0x80499BE4 (0xC8): Records the peer's line status and reports its errors. */
void PORT_LineStatusInd(tRFC_MCB* p_mcb, u8 dlci, u8 line_status) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);
    u32 event = 0;

    RFCOMM_TRACE_EVENT0("PORT_LineStatusInd");

    if (!p_port) {
        return;
    }

    p_port->line_status |= line_status;

    if (line_status & PORT_ERR_OVERRUN) {
        event |= PORT_EV_OVERRUN;
    }

    if (line_status & PORT_ERR_BREAK) {
        event |= PORT_EV_BREAK;
    }

    if (line_status & ~(PORT_ERR_OVERRUN | PORT_ERR_BREAK)) {
        event |= PORT_EV_ERR;
    }

    if ((p_port->p_callback != NULL) && (p_port->ev_mask & event)) {
        p_port->p_callback((p_port->ev_mask & event), p_port->inx);
    }
}

/* 0x80499CAC (0x68): Closes the port of the DLC the peer released. */
void PORT_DlcReleaseInd(tRFC_MCB* p_mcb, u8 dlci) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    RFCOMM_TRACE_EVENT0("PORT_DlcReleaseInd");

    if (!p_port) {
        return;
    }

    port_rfc_closed(p_port, PORT_CLOSED);
}

/* 0x80499D14 (0xA0): Closes every port of the multiplexer whose L2CAP channel went down, then frees it. */
void PORT_CloseInd(tRFC_MCB* p_mcb) {
    tPORT* p_port;
    int i;

    RFCOMM_TRACE_EVENT0("PORT_CloseInd");

    p_port = &rfc_cb.port.port[0];
    for (i = 0; i < MAX_RFC_PORTS; i++, p_port++) {
        if (p_port->rfc.p_mcb == p_mcb) {
            port_rfc_closed(p_port, PORT_PEER_CONNECTION_FAILED);
        }
    }
    rfc_release_multiplexer_channel(p_mcb);
}

/* 0x80499DB4 (0x98): Closes every port of the multiplexer that timed out. */
void Port_TimeOutCloseMux(tRFC_MCB* p_mcb) {
    tPORT* p_port;
    int i;

    RFCOMM_TRACE_EVENT0("Port_TimeOutCloseMux");

    p_port = &rfc_cb.port.port[0];
    for (i = 0; i < MAX_RFC_PORTS; i++, p_port++) {
        if (p_port->rfc.p_mcb == p_mcb) {
            port_rfc_closed(p_port, PORT_PEER_TIMEOUT);
        }
    }
}

/* 0x80499E4C (0x200): Delivers received data to the port's data callback or queue, with flow control and events. */
void PORT_DataInd(tRFC_MCB* p_mcb, u8 dlci, BT_HDR* p_buf) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);
    u8 rx_char1;
    u32 events = 0;
    u8* p;
    int i;

    RFCOMM_TRACE_EVENT1("PORT_DataInd with data length %d", p_buf->len);

    if (!p_port) {
        GKI_freebuf(p_buf);
        return;
    }

    if (p_port->p_data_callback) {
        port_flow_control_peer(p_port, TRUE, 1);
        p_port->p_data_callback(p_port->inx, &((BT_BUF*)p_buf)->data[p_buf->offset], p_buf->len);
        GKI_freebuf(p_buf);
        return;
    }

    if ((p_port->rx.queue_size + p_buf->len > PORT_RX_CRITICAL_WM) ||
        (p_port->rx.queue.count + 1 > p_port->rx_buf_critical)) {
        RFCOMM_TRACE_EVENT0("PORT_DataInd. Buffer over run. Dropping the buffer");
        GKI_freebuf(p_buf);

        RFCOMM_LineStatusReq(p_mcb, dlci, LINE_STATUS_OVERRUN);
        return;
    }

    if (((rx_char1 = p_port->user_port_pars.rx_char1) != 0) && (p_port->ev_mask & PORT_EV_RXFLAG)) {
        for (i = 0, p = &((BT_BUF*)p_buf)->data[p_buf->offset]; i < p_buf->len; i++) {
            if (*p++ == rx_char1) {
                events |= PORT_EV_RXFLAG;
                break;
            }
        }
    }

    GKI_enqueue(&p_port->rx.queue, p_buf);
    p_port->rx.queue_size += p_buf->len;

    port_flow_control_peer(p_port, FALSE, 0);

    if (p_port->rx.user_fc) {
        if (events & PORT_EV_RXFLAG) {
            p_port->rx_flag_ev_pending = TRUE;
        }
        return;
    }

    events |= PORT_EV_RXCHAR;
    events &= p_port->ev_mask;

    if (p_port->p_callback && events) {
        p_port->p_callback(events, p_port->inx);
    }
}

/* 0x8049A04C (0x130): Applies the peer's flow control (to one DLC or, on DLCI 0, the whole multiplexer). */
void PORT_FlowInd(tRFC_MCB* p_mcb, u8 dlci, u8 enable_data) {
    tPORT* p_port = NULL;
    u32 events = 0;
    int i;

    RFCOMM_TRACE_EVENT1("PORT_FlowInd fc:%d", enable_data);

    if (dlci == 0) {
        p_mcb->peer_ready = enable_data;
    } else {
        if ((p_port = port_find_mcb_dlci_port(p_mcb, dlci)) == NULL) {
            return;
        }
        p_port->tx.peer_fc = !enable_data;
    }

    for (i = 0; i < MAX_RFC_PORTS; i++) {
        if (dlci == 0) {
            p_port = &rfc_cb.port.port[i];
            if (!p_port->in_use || (p_port->rfc.p_mcb != p_mcb) || (p_port->rfc.state != RFC_STATE_OPENED)) {
                continue;
            }
        }
        events = 0;

        events |= port_flow_control_user(p_port);
        events |= port_rfc_send_tx_data(p_port);
        events &= p_port->ev_mask;

        if (p_port->p_callback && events) {
            (p_port->p_callback)(events, p_port->inx);
        }

        if (dlci != 0) {
            break;
        }
    }
}

/* 0x8049A17C (0xF8): Sends the port's queued data while the peer accepts it; returns the events to report. */
u32 port_rfc_send_tx_data(tPORT* p_port) {
    u32 events = 0;
    BT_HDR* p_buf;

    if (p_port->tx.queue_size > 0) {
        while (!p_port->tx.peer_fc && p_port->rfc.p_mcb && p_port->rfc.p_mcb->peer_ready) {
            if ((p_buf = GKI_dequeue(&p_port->tx.queue)) != NULL) {
                RFCOMM_TRACE_EVENT0("Sending RFCOMM_DataReq");
                p_port->tx.queue_size -= p_buf->len;

                RFCOMM_DataReq(p_port->rfc.p_mcb, p_port->dlci, p_buf);

                events |= PORT_EV_TXCHAR;

                if (p_port->tx.queue_size == 0) {
                    events |= PORT_EV_TXEMPTY;
                    break;
                }
            } else {
                events |= PORT_EV_TXEMPTY;
                break;
            }
        }
        events |= port_flow_control_user(p_port);
    }
    return events & p_port->ev_mask;
}

/* 0x8049A274 (0x194): Reports the closed DLC to the port's callbacks and releases the port (a server port still
 * opening only goes back to listening). */
void port_rfc_closed(tPORT* p_port, u8 res) {
    u8 old_signals;
    u32 events = 0;
    tRFC_MCB* p_mcb = p_port->rfc.p_mcb;

    if ((p_port->state == PORT_STATE_OPENING) && (p_port->is_server)) {
        RFCOMM_TRACE_EVENT0("port_rfc_closed in OPENING state ignored");

        rfc_port_timer_stop(p_port);
        p_port->rfc.state = RFC_STATE_CLOSED;

        if (p_mcb) {
            p_mcb->port_inx[p_port->dlci] = 0;
            rfc_check_mcb_active(p_mcb);
            p_port->rfc.p_mcb = NULL;
        }

        p_port->dlci &= 0xFE;
        return;
    }

    if ((p_port->state != PORT_STATE_CLOSING) && (p_port->state != PORT_STATE_CLOSED)) {
        p_port->line_status |= LINE_STATUS_FAILED;

        old_signals = p_port->peer_ctrl.modem_signal;

        p_port->peer_ctrl.modem_signal &= ~(PORT_DTRDSR_ON | PORT_CTSRTS_ON | PORT_DCD_ON);

        events |= port_get_signal_changes(p_port, old_signals, p_port->peer_ctrl.modem_signal);

        if (p_port->ev_mask & PORT_EV_CONNECT_ERR) {
            events |= PORT_EV_CONNECT_ERR;
        }
    }
    RFCOMM_TRACE_EVENT2("port_rfc_closed state:%d sending events:%x", p_port->state, events);

    if ((p_port->p_callback != NULL) && events) {
        p_port->p_callback(events, p_port->inx);
    }

    if (p_port->p_mgmt_callback) {
        p_port->p_mgmt_callback(res, p_port->inx);
    }

    p_port->rfc.state = RFC_STATE_CLOSED;

    port_release_port(p_port);
}
