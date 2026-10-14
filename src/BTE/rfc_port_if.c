/*
 * BTE/rfc_port_if.c - the RFCOMM port interface: the RFCOMM_* requests and responses the port layer sends into the
 *   state machines.
 *
 * RANGE. .text 0x8049D0F8-0x8049D624 (12 functions, 0x52C B). The .text holds the BTE rfc_port_if.c functions
 *   RFCOMM_StartRsp..RFCOMM_DlcReleaseReq and RFCOMM_DataReq at 0x8049D5EC; rfc_ts_frames.c starts at 0x8049D624 with
 *   the SABME sender. No data.
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's; 0x8049D5EC RFCOMM_DataReq is a GUESS from the BTE source order (it
 *   sends RFC_EVENT_DATA with the caller's buffer).
 * RESIDUALS. none: .text is byte-identical to the target.
 * SHAPES. plain C.
 */

#include "BTE/rfc_port_if.h"
#include "BTE/port_rfc.h"
#include "BTE/port_utils.h"
#include "BTE/rfc_mx_fsm.h"
#include "BTE/rfc_port_fsm.h"
#include "BTE/rfc_ts_frames.h"
#include "BTE/rfc_utils.h"

/* The flow control a multiplexer uses until the peer decides. */
#define PORT_FC_DEFAULT PORT_FC_CREDIT

/* 0x8049D0F8 (0x2C): Passes the port layer's answer to a multiplexer start to the multiplexer state machine. */
void RFCOMM_StartRsp(tRFC_MCB* p_mcb, u16 result) {
    rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_START_RSP, &result);
}

/* 0x8049D124 (0x68): Opens the DLC, or fails it at once while the multiplexer is not connected. */
void RFCOMM_DlcEstablishReq(tRFC_MCB* p_mcb, u8 dlci, u16 mtu) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_mcb->state != RFC_MX_STATE_CONNECTED) {
        PORT_DlcEstablishCnf(p_mcb, dlci, 0, RFCOMM_ERROR);
        return;
    }
    rfc_port_sm_execute(p_port, RFC_EVENT_OPEN, NULL);
}

/* 0x8049D18C (0x70): Answers the peer's DLC open, or releases the DLC while the multiplexer is not connected. */
void RFCOMM_DlcEstablishRsp(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u16 result) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_mcb->state != RFC_MX_STATE_CONNECTED && result == RFCOMM_SUCCESS) {
        PORT_DlcReleaseInd(p_mcb, dlci);
        return;
    }
    rfc_port_sm_execute(p_port, RFC_EVENT_ESTABLISH_RSP, &result);
}

/* 0x8049D1FC (0xE0): Sends a parameter negotiation for the DLC with the multiplexer's flow control and credits. */
void RFCOMM_ParNegReq(tRFC_MCB* p_mcb, u8 dlci, u16 mtu) {
    u8 flow;
    u8 cl;
    u8 k;
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_mcb->state != RFC_MX_STATE_CONNECTED) {
        p_port->error = PORT_PAR_NEG_FAILED;
        return;
    }
    flow = (p_mcb->flow == PORT_FC_UNDEFINED) ? PORT_FC_DEFAULT : p_mcb->flow;
    if (flow == PORT_FC_CREDIT) {
        cl = RFCOMM_PN_CONV_LAYER_CBFC_I;
        k = (p_port->credit_rx_max < RFCOMM_K_MAX) ? p_port->credit_rx_max : RFCOMM_K_MAX;
        p_port->credit_rx = k;
    } else {
        cl = RFCOMM_PN_CONV_LAYER_TYPE_1;
        k = 0;
    }
    p_port->rfc.expected_rsp |= RFC_RSP_PN;
    rfc_send_pn(p_mcb, dlci, TRUE, mtu, cl, k);
    rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
}

/* 0x8049D2DC (0x28): Answers the peer's parameter negotiation while the multiplexer is connected. */
void RFCOMM_ParNegRsp(tRFC_MCB* p_mcb, u8 dlci, u16 mtu, u8 cl, u8 k) {
    if (p_mcb->state != RFC_MX_STATE_CONNECTED) {
        return;
    }
    rfc_send_pn(p_mcb, dlci, FALSE, mtu, cl, k);
}

/* 0x8049D304 (0xBC): Sends a port negotiation (a query when `p_pars` is NULL), or fails it while not connected. */
void RFCOMM_PortNegReq(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_mcb->state != RFC_MX_STATE_CONNECTED) {
        PORT_PortNegCnf(p_mcb, dlci, NULL, RFCOMM_ERROR);
        return;
    }
    if (!p_pars) {
        p_port->rfc.expected_rsp |= RFC_RSP_RPN_REPLY;
    } else {
        p_port->rfc.expected_rsp |= RFC_RSP_RPN;
    }
    rfc_send_rpn(p_mcb, dlci, TRUE, p_pars, RFCOMM_RPN_PM_MASK);
    rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
}

/* 0x8049D3C0 (0x20): Answers the peer's port negotiation while the multiplexer is connected. */
void RFCOMM_PortNegRsp(tRFC_MCB* p_mcb, u8 dlci, tPORT_STATE* p_pars, u16 param_mask) {
    if (p_mcb->state != RFC_MX_STATE_CONNECTED) {
        return;
    }
    rfc_send_rpn(p_mcb, dlci, FALSE, p_pars, param_mask);
}

/* 0x8049D3E0 (0xA4): Sends the port's modem status on an open DLC and waits for the answer. */
void RFCOMM_ControlReq(tRFC_MCB* p_mcb, u8 dlci, tPORT_CTRL* p_pars) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_port->state != PORT_STATE_OPENED || p_port->rfc.state != RFC_STATE_OPENED) {
        return;
    }
    p_port->port_ctrl |= PORT_CTRL_REQ_SENT;
    p_port->rfc.expected_rsp |= RFC_RSP_MSC;
    rfc_send_msc(p_mcb, dlci, TRUE, p_pars);
    rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
}

/* 0x8049D484 (0xA4): Sends the port's flow control state as a modem status on an open DLC. */
void RFCOMM_FlowReq(tRFC_MCB* p_mcb, u8 dlci, u8 enable) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_port->state != PORT_STATE_OPENED || p_port->rfc.state != RFC_STATE_OPENED) {
        return;
    }
    p_port->local_ctrl.fc = !enable;
    p_port->rfc.expected_rsp |= RFC_RSP_MSC;
    rfc_send_msc(p_mcb, dlci, TRUE, &p_port->local_ctrl);
    rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
}

/* 0x8049D528 (0x98): Sends the port's line status on an open DLC and waits for the answer. */
void RFCOMM_LineStatusReq(tRFC_MCB* p_mcb, u8 dlci, u8 status) {
    tPORT* p_port = port_find_mcb_dlci_port(p_mcb, dlci);

    if (p_port->state != PORT_STATE_OPENED || p_port->rfc.state != RFC_STATE_OPENED) {
        return;
    }
    p_port->rfc.expected_rsp |= RFC_RSP_RLS;
    rfc_send_rls(p_mcb, dlci, TRUE, status);
    rfc_port_timer_start(p_port, RFC_T2_TIMEOUT);
}

/* 0x8049D5C0 (0x2C): Closes the DLC. */
void RFCOMM_DlcReleaseReq(tRFC_MCB* p_mcb, u8 dlci) {
    rfc_port_sm_execute(port_find_mcb_dlci_port(p_mcb, dlci), RFC_EVENT_CLOSE, NULL);
}

/* 0x8049D5EC (0x38): Sends the buffer on the DLC. */
void RFCOMM_DataReq(tRFC_MCB* p_mcb, u8 dlci, BT_HDR* p_buf) {
    rfc_port_sm_execute(port_find_mcb_dlci_port(p_mcb, dlci), RFC_EVENT_DATA, p_buf);
}
