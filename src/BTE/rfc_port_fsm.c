/*
 * BTE/rfc_port_fsm.c - the RFCOMM port state machine: rfc_port_sm_execute, its per-state handlers and the
 *   PN/RPN/MSC/RLS/FCON/FCOFF processing, plus L2CAP congestion and port state set.
 *
 * RANGE. .text 0x8049BF58-0x8049D0F8 (17 functions, 0x11A0 B); .data 0x80617970-0x80617C98. The .text holds the BTE
 *   rfc_port_fsm.c functions rfc_port_sm_execute..rfc_set_port_state; rfc_port_if.c's RFCOMM_StartRsp follows. The .data
 *   (jump tables and trace strings) is read only by this range's functions; no pad beyond 4-byte alignment.
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's. GUESS names from the BTE source order:
 *   0x8049CEF8 rfc_process_nsc (an empty body) and 0x8049CEFC rfc_process_test_rsp (frees the
 *   buffer).
 * RESIDUALS. none: .text and the claimed data are byte-identical to the target (the .data tail is alignment fill).
 * SHAPES. plain C; the state handlers are static, and the uplink data path calls PORT_DataInd directly.
 */

#include "BTE/rfc_port_fsm.h"
#include "BTE/btm_sec.h"
#include "BTE/port_api.h"
#include "BTE/port_rfc.h"
#include "BTE/port_utils.h"
#include "BTE/rfc_ts_frames.h"
#include "BTE/rfc_utils.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The security protocol id and result the port's security checks use. */
#define BTM_SEC_PROTO_RFCOMM 3
#define BTM_SUCCESS 0
#define RFCOMM_SECURITY_ERR 112
#define PORT_SEC_FAILED 15

/* The modem status bits of an MSC frame and the break sequencing a received one reports. */
#define RFCOMM_MSC_FC 0x02
#define RFCOMM_MSC_RTC 0x04
#define RFCOMM_MSC_RTR 0x08
#define RFCOMM_MSC_IC 0x40
#define RFCOMM_MSC_DV 0x80
#define RFCOMM_CTRL_BREAK_IN_SEQ 1

/* The flow control types and parameter mask bits of an RPN frame. */
#define RFCOMM_FC_OFF 0x00
#define RFCOMM_FC_RTR_ON_INPUT 0x04
#define RFCOMM_FC_RTR_ON_OUTPUT 0x08
#define RFCOMM_FC_RTC_ON_INPUT 0x10
#define RFCOMM_FC_RTC_ON_OUTPUT 0x20
#define RFCOMM_RPN_PM_BIT_RATE 0x0001
#define RFCOMM_RPN_PM_DATA_BITS 0x0002
#define RFCOMM_RPN_PM_STOP_BITS 0x0004
#define RFCOMM_RPN_PM_PARITY 0x0008
#define RFCOMM_RPN_PM_PARITY_TYPE 0x0010
#define RFCOMM_RPN_PM_XON_CHAR 0x0020
#define RFCOMM_RPN_PM_XOFF_CHAR 0x0040
#define RFCOMM_RPN_PM_XONXOFF_ON_INPUT 0x0100
#define RFCOMM_RPN_PM_XONXOFF_ON_OUTPUT 0x0200
#define RFCOMM_RPN_PM_RTR_ON_INPUT 0x0400
#define RFCOMM_RPN_PM_RTR_ON_OUTPUT 0x0800
#define RFCOMM_RPN_PM_RTC_ON_INPUT 0x1000
#define RFCOMM_RPN_PM_RTC_ON_OUTPUT 0x2000

/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_state_closed(tPORT* p_port, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_sabme_wait_ua(tPORT* p_port, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_term_wait_sec_check(tPORT* p_port, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_orig_wait_sec_check(tPORT* p_port, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_opened(tPORT* p_port, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_disc_wait_ua(tPORT* p_port, u16 event, void* p_data);

/* 0x8049BF58 (0x90): Passes the event to the handler of the port's DLC state. */
/* untyped: a caller-owned payload - its type depends on the event */
void rfc_port_sm_execute(tPORT* p_port, u16 event, void* p_data) {
    if (!p_port) {
        RFCOMM_TRACE_WARNING1("NULL port event %d", event);
        return;
    }

    switch (p_port->rfc.state) {
    case RFC_STATE_CLOSED:
        rfc_port_sm_state_closed(p_port, event, p_data);
        break;
    case RFC_STATE_SABME_WAIT_UA:
        rfc_port_sm_sabme_wait_ua(p_port, event, p_data);
        break;
    case RFC_STATE_ORIG_WAIT_SEC_CHECK:
        rfc_port_sm_orig_wait_sec_check(p_port, event, p_data);
        break;
    case RFC_STATE_TERM_WAIT_SEC_CHECK:
        rfc_port_sm_term_wait_sec_check(p_port, event, p_data);
        break;
    case RFC_STATE_OPENED:
        rfc_port_sm_opened(p_port, event, p_data);
        break;
    case RFC_STATE_DISC_WAIT_UA:
        rfc_port_sm_disc_wait_ua(p_port, event, p_data);
        break;
    }
}

/* 0x8049BFE8 (0x174): Starts the security check of an outgoing or incoming DLC open. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_state_closed(tPORT* p_port, u16 event, void* p_data) {
    switch (event) {
    case RFC_EVENT_OPEN:
        p_port->rfc.state = RFC_STATE_ORIG_WAIT_SEC_CHECK;
        btm_sec_mx_access_request(p_port->rfc.p_mcb->bd_addr, BT_PSM_RFCOMM, TRUE, BTM_SEC_PROTO_RFCOMM,
                                  (u32)(p_port->dlci / 2), &rfc_sec_check_complete, p_port);
        return;

    case RFC_EVENT_CLOSE:
        break;

    case RFC_EVENT_CLEAR:
        return;

    case RFC_EVENT_DATA:
        GKI_freebuf(p_data);
        break;

    case RFC_EVENT_SABME:
        p_port->rfc.state = RFC_STATE_TERM_WAIT_SEC_CHECK;
        btm_sec_mx_access_request(p_port->rfc.p_mcb->bd_addr, BT_PSM_RFCOMM, FALSE, BTM_SEC_PROTO_RFCOMM,
                                  (u32)(p_port->dlci / 2), &rfc_sec_check_complete, p_port);
        return;

    case RFC_EVENT_UA:
        return;

    case RFC_EVENT_DM:
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_UIH:
        GKI_freebuf(p_data);
        rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, FALSE);
        return;

    case RFC_EVENT_DISC:
        rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, FALSE);
        return;

    case RFC_EVENT_TIMEOUT:
        Port_TimeOutCloseMux(p_port->rfc.p_mcb);
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;
    }
    RFCOMM_TRACE_WARNING1("Port state closed Event ignored %d", event);
}

/* 0x8049C15C (0x19C): Waits for the peer's UA to our SABME and reports the DLC open result. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_sabme_wait_ua(tPORT* p_port, u16 event, void* p_data) {
    switch (event) {
    case RFC_EVENT_OPEN:
    case RFC_EVENT_ESTABLISH_RSP:
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;

    case RFC_EVENT_CLOSE:
        rfc_port_timer_start(p_port, RFC_DISC_TIMEOUT);
        rfc_send_disc(p_port->rfc.p_mcb, p_port->dlci);
        p_port->rfc.expected_rsp = 0;
        p_port->rfc.state = RFC_STATE_DISC_WAIT_UA;
        return;

    case RFC_EVENT_CLEAR:
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DATA:
        GKI_freebuf(p_data);
        break;

    case RFC_EVENT_UA:
        rfc_port_timer_stop(p_port);
        p_port->rfc.state = RFC_STATE_OPENED;
        PORT_DlcEstablishCnf(p_port->rfc.p_mcb, p_port->dlci, p_port->rfc.p_mcb->peer_l2cap_mtu, RFCOMM_SUCCESS);
        return;

    case RFC_EVENT_DM:
        PORT_DlcEstablishCnf(p_port->rfc.p_mcb, p_port->dlci, p_port->rfc.p_mcb->peer_l2cap_mtu, RFCOMM_ERROR);
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DISC:
        rfc_send_ua(p_port->rfc.p_mcb, p_port->dlci);
        PORT_DlcEstablishCnf(p_port->rfc.p_mcb, p_port->dlci, p_port->rfc.p_mcb->peer_l2cap_mtu, RFCOMM_ERROR);
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_SABME:
        rfc_send_ua(p_port->rfc.p_mcb, p_port->dlci);
        return;

    case RFC_EVENT_UIH:
        GKI_freebuf(p_data);
        return;

    case RFC_EVENT_TIMEOUT:
        p_port->rfc.state = RFC_STATE_CLOSED;
        PORT_DlcEstablishCnf(p_port->rfc.p_mcb, p_port->dlci, p_port->rfc.p_mcb->peer_l2cap_mtu, RFCOMM_ERROR);
        return;
    }
    RFCOMM_TRACE_WARNING1("Port state sabme_wait_ua Event ignored %d", event);
}

/* 0x8049C2F8 (0x1BC): Waits for the security check and the port layer's answer to an incoming DLC open. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_term_wait_sec_check(tPORT* p_port, u16 event, void* p_data) {
    switch (event) {
    case RFC_EVENT_SEC_COMPLETE:
        if (*(u8*)p_data != BTM_SUCCESS) {
            if (p_port->rfc.p_mcb) {
                rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, TRUE);
                port_rfc_closed(p_port, PORT_SEC_FAILED);
            }
        } else {
            PORT_DlcEstablishInd(p_port->rfc.p_mcb, p_port->dlci, p_port->rfc.p_mcb->peer_l2cap_mtu);
        }
        return;

    case RFC_EVENT_OPEN:
    case RFC_EVENT_CLOSE:
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;

    case RFC_EVENT_CLEAR:
        btm_sec_abort_access_req(p_port->rfc.p_mcb->bd_addr);
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DATA:
        RFCOMM_TRACE_ERROR0("Port error state Term Wait Sec event Data");
        GKI_freebuf(p_data);
        return;

    case RFC_EVENT_SABME:
        return;

    case RFC_EVENT_DISC:
        btm_sec_abort_access_req(p_port->rfc.p_mcb->bd_addr);
        p_port->rfc.state = RFC_STATE_CLOSED;
        rfc_send_ua(p_port->rfc.p_mcb, p_port->dlci);
        PORT_DlcReleaseInd(p_port->rfc.p_mcb, p_port->dlci);
        return;

    case RFC_EVENT_UIH:
        GKI_freebuf(p_data);
        return;

    case RFC_EVENT_ESTABLISH_RSP:
        if (*(u8*)p_data != RFCOMM_SUCCESS) {
            if (p_port->rfc.p_mcb) {
                rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, TRUE);
            }
        } else {
            rfc_send_ua(p_port->rfc.p_mcb, p_port->dlci);
            p_port->rfc.state = RFC_STATE_OPENED;
        }
        return;
    }
    RFCOMM_TRACE_WARNING1("Port state term_wait_sec_check Event ignored %d", event);
}

/* 0x8049C4B4 (0x150): Sends SABME once the security check of an outgoing DLC open passes. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_orig_wait_sec_check(tPORT* p_port, u16 event, void* p_data) {
    switch (event) {
    case RFC_EVENT_SEC_COMPLETE:
        if (*(u8*)p_data != BTM_SUCCESS) {
            PORT_DlcEstablishCnf(p_port->rfc.p_mcb, p_port->dlci, 0, RFCOMM_SECURITY_ERR);
            rfc_port_closed(p_port);
            return;
        }
        rfc_send_sabme(p_port->rfc.p_mcb, p_port->dlci);
        rfc_port_timer_start(p_port, RFC_PORT_T1_TIMEOUT);
        p_port->rfc.state = RFC_STATE_SABME_WAIT_UA;
        return;

    case RFC_EVENT_OPEN:
    case RFC_EVENT_SABME:
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;

    case RFC_EVENT_CLOSE:
        btm_sec_abort_access_req(p_port->rfc.p_mcb->bd_addr);
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DATA:
        RFCOMM_TRACE_ERROR0("Port error state Orig Wait Sec event Data");
        GKI_freebuf(p_data);
        return;

    case RFC_EVENT_UIH:
        GKI_freebuf(p_data);
        return;
    }
    RFCOMM_TRACE_WARNING1("Port state orig_wait_sec_check Event ignored %d", event);
}

/* 0x8049C604 (0x1EC): Sends and receives data on the open DLC, and closes it on request or on the peer's DISC/DM. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_opened(tPORT* p_port, u16 event, void* p_data) {
    switch (event) {
    case RFC_EVENT_OPEN:
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;

    case RFC_EVENT_CLOSE:
        rfc_port_timer_start(p_port, RFC_DISC_TIMEOUT);
        rfc_send_disc(p_port->rfc.p_mcb, p_port->dlci);
        p_port->rfc.expected_rsp = 0;
        p_port->rfc.state = RFC_STATE_DISC_WAIT_UA;
        return;

    case RFC_EVENT_CLEAR:
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DATA:
        if (p_port->rfc.p_mcb->flow == PORT_FC_CREDIT && ((BT_HDR*)p_data)->len < p_port->peer_mtu &&
            !p_port->rx.user_fc && p_port->credit_rx_max > p_port->credit_rx) {
            ((BT_HDR*)p_data)->layer_specific = (u8)(p_port->credit_rx_max - p_port->credit_rx);
            p_port->credit_rx = p_port->credit_rx_max;
        } else {
            ((BT_HDR*)p_data)->layer_specific = 0;
        }
        rfc_send_buf_uih(p_port->rfc.p_mcb, p_port->dlci, (BT_HDR*)p_data);
        rfc_dec_credit(p_port);
        return;

    case RFC_EVENT_UA:
        return;

    case RFC_EVENT_SABME:
        rfc_send_ua(p_port->rfc.p_mcb, p_port->dlci);
        return;

    case RFC_EVENT_DM:
        PORT_DlcReleaseInd(p_port->rfc.p_mcb, p_port->dlci);
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DISC:
        p_port->rfc.state = RFC_STATE_CLOSED;
        rfc_send_ua(p_port->rfc.p_mcb, p_port->dlci);
        PORT_DlcReleaseInd(p_port->rfc.p_mcb, p_port->dlci);
        return;

    case RFC_EVENT_UIH:
        PORT_DataInd(p_port->rfc.p_mcb, p_port->dlci, (BT_HDR*)p_data);
        return;

    case RFC_EVENT_TIMEOUT:
        Port_TimeOutCloseMux(p_port->rfc.p_mcb);
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;
    }
    RFCOMM_TRACE_WARNING1("Port state opened Event ignored %d", event);
}

/* 0x8049C7F0 (0x11C): Closes the DLC once the peer answers our DISC (or it times out). */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_port_sm_disc_wait_ua(tPORT* p_port, u16 event, void* p_data) {
    switch (event) {
    case RFC_EVENT_OPEN:
    case RFC_EVENT_ESTABLISH_RSP:
        RFCOMM_TRACE_ERROR2("Port error state %d event %d", p_port->rfc.state, event);
        return;

    case RFC_EVENT_CLEAR:
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_DATA:
        GKI_freebuf(p_data);
        return;

    case RFC_EVENT_UA:
        p_port->rfc.p_mcb->is_disc_initiator = TRUE;
    case RFC_EVENT_DM:
        rfc_port_closed(p_port);
        return;

    case RFC_EVENT_SABME:
        rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, TRUE);
        return;

    case RFC_EVENT_DISC:
        rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, TRUE);
        return;

    case RFC_EVENT_UIH:
        GKI_freebuf(p_data);
        rfc_send_dm(p_port->rfc.p_mcb, p_port->dlci, FALSE);
        return;

    case RFC_EVENT_TIMEOUT:
        rfc_port_closed(p_port);
        return;
    }
    RFCOMM_TRACE_WARNING1("Port state disc_wait_ua Event ignored %d", event);
}

/* 0x8049C90C (0xE8): Passes a parameter negotiation command, or the awaited response, to the port layer. */
void rfc_process_pn(tRFC_MCB* p_mcb, u8 is_command, MX_FRAME* p_frame) {
    tPORT* p_port;
    u8 dlci = p_frame->dlci;

    if (is_command) {
        if (p_mcb->state != RFC_MX_STATE_DISC_WAIT_UA) {
            PORT_ParNegInd(p_mcb, dlci, p_frame->u.pn.mtu, p_frame->u.pn.conv_layer, p_frame->u.pn.k);
        } else {
            rfc_send_dm(p_mcb, dlci, FALSE);
            RFCOMM_TRACE_WARNING0("***** MX PN while disconnecting *****");
        }
        return;
    }

    p_port = port_find_mcb_dlci_port(p_mcb, dlci);
    if (p_port == NULL || !(p_port->rfc.expected_rsp & RFC_RSP_PN)) {
        return;
    }
    p_port->rfc.expected_rsp &= ~RFC_RSP_PN;
    rfc_port_timer_stop(p_port);
    PORT_ParNegCnf(p_mcb, dlci, p_frame->u.pn.mtu, p_frame->u.pn.conv_layer, p_frame->u.pn.k);
}

/* 0x8049C9F4 (0x30C): Answers a port negotiation, or takes its awaited response and retries with another flow
 * control. */
void rfc_process_rpn(tRFC_MCB* p_mcb, u8 is_command, u8 is_request, MX_FRAME* p_frame) {
    tPORT_STATE port_pars;
    tPORT* p_port;

    if ((p_port = port_find_mcb_dlci_port(p_mcb, p_frame->dlci)) == NULL) {
        if (is_command) {
            memset(&port_pars, 0, sizeof(tPORT_STATE));
            rfc_set_port_state(&port_pars, p_frame);
            PORT_PortNegInd(p_mcb, p_frame->dlci, &port_pars, p_frame->u.rpn.param_mask);
        }
        return;
    }

    if (is_command && is_request) {
        port_pars = p_port->peer_port_pars;
        rfc_send_rpn(p_mcb, p_frame->dlci, FALSE, &p_port->peer_port_pars, 0);
        return;
    }

    port_pars = p_port->peer_port_pars;
    rfc_set_port_state(&port_pars, p_frame);

    if (is_command) {
        PORT_PortNegInd(p_mcb, p_frame->dlci, &port_pars, p_frame->u.rpn.param_mask);
        return;
    }

    p_port = port_find_mcb_dlci_port(p_mcb, p_frame->dlci);
    if (p_port == NULL || !(p_port->rfc.expected_rsp & (RFC_RSP_RPN | RFC_RSP_RPN_REPLY))) {
        return;
    }

    rfc_port_timer_stop(p_port);

    if (p_port->rfc.expected_rsp & RFC_RSP_RPN_REPLY) {
        p_port->rfc.expected_rsp &= ~RFC_RSP_RPN_REPLY;
        p_port->peer_port_pars = port_pars;

        if (port_pars.fc_type == (RFCOMM_FC_RTR_ON_INPUT | RFCOMM_FC_RTR_ON_OUTPUT) ||
            port_pars.fc_type == (RFCOMM_FC_RTC_ON_INPUT | RFCOMM_FC_RTC_ON_OUTPUT)) {
            p_frame->u.rpn.param_mask = RFCOMM_RPN_PM_MASK;
        } else {
            p_port->peer_port_pars.fc_type = RFCOMM_FC_RTR_ON_INPUT | RFCOMM_FC_RTR_ON_OUTPUT;
            p_port->rfc.expected_rsp |= RFC_RSP_RPN;
            rfc_send_rpn(p_mcb, p_frame->dlci, TRUE, &p_port->peer_port_pars,
                         RFCOMM_RPN_PM_RTR_ON_INPUT | RFCOMM_RPN_PM_RTR_ON_OUTPUT);
            rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
            return;
        }
    } else {
        p_port->rfc.expected_rsp &= ~RFC_RSP_RPN;
    }

    if ((p_frame->u.rpn.param_mask & (RFCOMM_RPN_PM_RTR_ON_INPUT | RFCOMM_RPN_PM_RTR_ON_OUTPUT)) ==
            (RFCOMM_RPN_PM_RTR_ON_INPUT | RFCOMM_RPN_PM_RTR_ON_OUTPUT) ||
        (p_frame->u.rpn.param_mask & (RFCOMM_RPN_PM_RTC_ON_INPUT | RFCOMM_RPN_PM_RTC_ON_OUTPUT)) ==
            (RFCOMM_RPN_PM_RTC_ON_INPUT | RFCOMM_RPN_PM_RTC_ON_OUTPUT)) {
        PORT_PortNegCnf(p_mcb, p_port->dlci, &port_pars, RFCOMM_SUCCESS);
        return;
    }

    if (p_port->peer_port_pars.fc_type == (RFCOMM_FC_RTR_ON_INPUT | RFCOMM_FC_RTR_ON_OUTPUT)) {
        p_port->peer_port_pars.fc_type = RFCOMM_FC_RTC_ON_INPUT | RFCOMM_FC_RTC_ON_OUTPUT;
        p_port->rfc.expected_rsp |= RFC_RSP_RPN;
        rfc_send_rpn(p_mcb, p_frame->dlci, TRUE, &p_port->peer_port_pars,
                     RFCOMM_RPN_PM_RTC_ON_INPUT | RFCOMM_RPN_PM_RTC_ON_OUTPUT);
        rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
        return;
    }

    if (p_port->peer_port_pars.fc_type == (RFCOMM_FC_RTC_ON_INPUT | RFCOMM_FC_RTC_ON_OUTPUT)) {
        p_port->peer_port_pars.fc_type = RFCOMM_FC_OFF;
        PORT_PortNegCnf(p_mcb, p_port->dlci, &port_pars, RFCOMM_SUCCESS);
    }
}

/* 0x8049CD00 (0x16C): Answers a modem status command and reports it (with its flow control), or takes the awaited
 * response. */
void rfc_process_msc(tRFC_MCB* p_mcb, u8 is_command, MX_FRAME* p_frame) {
    tPORT_CTRL pars;
    tPORT* p_port;
    u8 modem_signals = p_frame->u.msc.signals;
    u8 new_peer_fc = FALSE;

    p_port = port_find_mcb_dlci_port(p_mcb, p_frame->dlci);
    if (p_port == NULL) {
        return;
    }

    pars.modem_signal = 0;
    if (modem_signals & RFCOMM_MSC_RTC) {
        pars.modem_signal |= PORT_DTRDSR_ON;
    }
    if (modem_signals & RFCOMM_MSC_RTR) {
        pars.modem_signal |= PORT_CTSRTS_ON;
    }
    if (modem_signals & RFCOMM_MSC_IC) {
        pars.modem_signal |= PORT_RING_ON;
    }
    if (modem_signals & RFCOMM_MSC_DV) {
        pars.modem_signal |= PORT_DCD_ON;
    }
    pars.fc = (modem_signals & RFCOMM_MSC_FC) == RFCOMM_MSC_FC;
    pars.break_signal = (p_frame->u.msc.break_present) ? p_frame->u.msc.break_duration : 0;
    pars.discard_buffers = 0;
    pars.break_signal_seq = RFCOMM_CTRL_BREAK_IN_SEQ;

    if (is_command) {
        rfc_send_msc(p_mcb, p_frame->dlci, FALSE, &pars);
        if (p_port->rfc.p_mcb->flow != PORT_FC_CREDIT) {
            p_port->peer_ctrl.fc = new_peer_fc = pars.fc;
            if (new_peer_fc != p_port->tx.peer_fc) {
                PORT_FlowInd(p_mcb, p_frame->dlci, (u8)!new_peer_fc);
            }
        }
        PORT_ControlInd(p_mcb, p_frame->dlci, &pars);
        return;
    }

    if (!(p_port->rfc.expected_rsp & RFC_RSP_MSC)) {
        return;
    }
    p_port->rfc.expected_rsp &= ~RFC_RSP_MSC;
    rfc_port_timer_stop(p_port);
    PORT_ControlCnf(p_port->rfc.p_mcb, p_port->dlci, &pars);
}

/* 0x8049CE6C (0x8C): Reports and answers a line status command, or takes the awaited response. */
void rfc_process_rls(tRFC_MCB* p_mcb, u8 is_command, MX_FRAME* p_frame) {
    tPORT* p_port;

    if (is_command) {
        PORT_LineStatusInd(p_mcb, p_frame->dlci, p_frame->u.rls.line_status);
        rfc_send_rls(p_mcb, p_frame->dlci, FALSE, p_frame->u.rls.line_status);
    } else {
        p_port = port_find_mcb_dlci_port(p_mcb, p_frame->dlci);
        if (!p_port || !(p_port->rfc.expected_rsp & RFC_RSP_RLS)) {
            return;
        }
        p_port->rfc.expected_rsp &= ~RFC_RSP_RLS;
        rfc_port_timer_stop(p_port);
    }
}

/* 0x8049CEF8 (0x4): Ignores a non-supported command response. */
void rfc_process_nsc(tRFC_MCB* p_mcb, MX_FRAME* p_frame) {
}

/* 0x8049CEFC (0x8): Frees the buffer of a test response. */
void rfc_process_test_rsp(tRFC_MCB* p_mcb, BT_HDR* p_buf) {
    GKI_freebuf(p_buf);
}

/* 0x8049CF04 (0x64): Resumes sending after the peer's flow-on command and answers it. */
void rfc_process_fcon(tRFC_MCB* p_mcb, u8 is_command) {
    if (is_command) {
        rfc_cb.rfc.peer_rx_disabled = FALSE;
        rfc_send_fcon(p_mcb, FALSE);
        if (!p_mcb->l2cap_congested) {
            PORT_FlowInd(p_mcb, 0, TRUE);
        }
    }
}

/* 0x8049CF68 (0x64): Stops sending after the peer's flow-off command and answers it. */
void rfc_process_fcoff(tRFC_MCB* p_mcb, u8 is_command) {
    if (is_command) {
        rfc_cb.rfc.peer_rx_disabled = TRUE;
        if (!p_mcb->l2cap_congested) {
            PORT_FlowInd(p_mcb, 0, FALSE);
        }
        rfc_send_fcoff(p_mcb, FALSE);
    }
}

/* 0x8049CFCC (0x88): Records L2CAP congestion, flushes the queued commands when it clears and reports it to the
 * ports. */
void rfc_process_l2cap_congestion(tRFC_MCB* p_mcb, u8 is_congested) {
    p_mcb->l2cap_congested = is_congested;
    if (!is_congested) {
        rfc_check_send_cmd(p_mcb, NULL);
    }
    if (!rfc_cb.rfc.peer_rx_disabled) {
        if (!is_congested) {
            PORT_FlowInd(p_mcb, 0, TRUE);
        } else {
            PORT_FlowInd(p_mcb, 0, FALSE);
        }
    }
}

/* 0x8049D054 (0xA4): Copies the line parameters the frame's mask selects into `port_pars`. */
void rfc_set_port_state(tPORT_STATE* port_pars, MX_FRAME* p_frame) {
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_BIT_RATE) {
        port_pars->baud_rate = p_frame->u.rpn.baud_rate;
    }
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_DATA_BITS) {
        port_pars->byte_size = p_frame->u.rpn.byte_size;
    }
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_STOP_BITS) {
        port_pars->stop_bits = p_frame->u.rpn.stop_bits;
    }
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_PARITY) {
        port_pars->parity = p_frame->u.rpn.parity;
    }
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_PARITY_TYPE) {
        port_pars->parity_type = p_frame->u.rpn.parity_type;
    }
    if (p_frame->u.rpn.param_mask & (RFCOMM_RPN_PM_XONXOFF_ON_INPUT | RFCOMM_RPN_PM_XONXOFF_ON_OUTPUT |
                                     RFCOMM_RPN_PM_RTR_ON_INPUT | RFCOMM_RPN_PM_RTR_ON_OUTPUT |
                                     RFCOMM_RPN_PM_RTC_ON_INPUT | RFCOMM_RPN_PM_RTC_ON_OUTPUT)) {
        port_pars->fc_type = p_frame->u.rpn.fc_type;
    }
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_XON_CHAR) {
        port_pars->xon_char = p_frame->u.rpn.xon_char;
    }
    if (p_frame->u.rpn.param_mask & RFCOMM_RPN_PM_XOFF_CHAR) {
        port_pars->xoff_char = p_frame->u.rpn.xoff_char;
    }
}
