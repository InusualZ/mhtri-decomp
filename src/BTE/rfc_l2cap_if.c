/*
 * BTE/rfc_l2cap_if.c - the RFCOMM L2CAP interface: the L2CAP callback registration, connect/config/disconnect
 *   indications, data and congestion callbacks, and the LCID-to-multiplexer table store.
 *
 * RANGE. .text 0x8049A9E0-0x8049B314 (10 functions, 0x934 B); .data 0x80617578-0x806176B8. The .text holds the BTE
 *   rfc_l2cap_if.c functions rfcomm_l2cap_if_init..RFCOMM_CongestionStatusInd and rfc_save_lcid_mcb, which closes the
 *   BTE file; rfc_mx_fsm.c's rfc_mx_sm_execute follows at 0x8049B314. The .data is the trace strings, read only by this
 *   range's functions; no pad beyond 4-byte alignment.
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's. GUESS names from the BTE source order and the callback slots
 *   rfcomm_l2cap_if_init stores them in: 0x8049AA5C RFCOMM_ConnectInd,
 *   0x8049ABEC RFCOMM_ConfigInd, 0x8049ACF4 RFCOMM_ConfigCnf, 0x8049ADFC RFCOMM_QoSViolationInd (an empty body).
 * RESIDUALS. none: .text and the claimed data are byte-identical to the target (the .data tail is alignment fill).
 * SHAPES. plain C; rfc_find_lcid_mcb is static and inlined into every callback, rfc_save_lcid_mcb (defined last) into
 *   RFCOMM_ConnectInd.
 */

#include "BTE/rfc_l2cap_if.h"
#include "BTE/port_api.h"
#include "BTE/port_utils.h"
#include "BTE/rfc_mx_fsm.h"
#include "BTE/rfc_port_fsm.h"
#include "BTE/rfc_ts_frames.h"
#include "BTE/rfc_utils.h"

/* The L2CAP connect result that refuses a channel for lack of resources. */
#define L2CAP_CONN_NO_RESOURCES 4

static tRFC_MCB* rfc_find_lcid_mcb(u16 lcid);

/* 0x8049A9E0 (0x7C): Fills RFCOMM's L2CAP callback set and registers it for the RFCOMM PSM. */
void rfcomm_l2cap_if_init(void) {
    tL2CAP_APPL_INFO* p_l2c = &rfc_cb.rfc.reg_info;

    p_l2c->pL2CA_ConnectInd_Cb = RFCOMM_ConnectInd;
    p_l2c->pL2CA_ConnectCfm_Cb = RFCOMM_ConnectCnf;
    p_l2c->pL2CA_ConnectPnd_Cb = NULL;
    p_l2c->pL2CA_ConfigInd_Cb = RFCOMM_ConfigInd;
    p_l2c->pL2CA_ConfigCfm_Cb = RFCOMM_ConfigCnf;
    p_l2c->pL2CA_DisconnectInd_Cb = RFCOMM_DisconnectInd;
    p_l2c->pL2CA_DisconnectCfm_Cb = NULL;
    p_l2c->pL2CA_QoSViolationInd_Cb = RFCOMM_QoSViolationInd;
    p_l2c->pL2CA_DataInd_Cb = RFCOMM_BufDataInd;
    p_l2c->pL2CA_CongestionStatus_Cb = RFCOMM_CongestionStatusInd;
    L2CA_Register(BT_PSM_RFCOMM, p_l2c);
}

/* 0x8049AA5C (0x8C): Takes a multiplexer for the incoming channel, or refuses the channel when none is free. */
void RFCOMM_ConnectInd(u8* bd_addr, u16 lcid, u16 psm, u8 id) {
    tRFC_MCB* p_mcb = rfc_alloc_multiplexer_channel(bd_addr, FALSE);

    rfc_save_lcid_mcb(p_mcb, lcid);
    if (p_mcb == NULL) {
        L2CA_ConnectRsp(bd_addr, id, lcid, L2CAP_CONN_NO_RESOURCES, 0);
        return;
    }
    p_mcb->lcid = lcid;
    rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_CONN_IND, &id);
}

/* 0x8049AAE8 (0x104): Passes the result of an outgoing channel to its multiplexer. */
void RFCOMM_ConnectCnf(u16 lcid, u16 result) {
    tRFC_MCB* p_mcb = rfc_find_lcid_mcb(lcid);

    if (!p_mcb) {
        RFCOMM_TRACE_ERROR1("RFCOMM_ConnectCnf LCID:0x%x", lcid);
        return;
    }
    p_mcb->lcid = lcid;
    rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_CONN_CNF, &result);
}

/* 0x8049ABEC (0x108): Passes the peer's channel configuration to its multiplexer. */
void RFCOMM_ConfigInd(u16 lcid, tL2CAP_CFG_INFO* p_cfg) {
    tRFC_MCB* p_mcb = rfc_find_lcid_mcb(lcid);

    if (!p_mcb) {
        RFCOMM_TRACE_ERROR1("RFCOMM_ConfigInd LCID:0x%x", lcid);
        return;
    }
    rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_CONF_IND, p_cfg);
}

/* 0x8049ACF4 (0x108): Passes the answer to our channel configuration to its multiplexer. */
void RFCOMM_ConfigCnf(u16 lcid, tL2CAP_CFG_INFO* p_cfg) {
    tRFC_MCB* p_mcb = rfc_find_lcid_mcb(lcid);

    if (!p_mcb) {
        RFCOMM_TRACE_ERROR1("RFCOMM_ConfigCnf LCID:0x%x", lcid);
        return;
    }
    rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_CONF_CNF, p_cfg);
}

/* 0x8049ADFC (0x4): Ignores a QoS violation. */
void RFCOMM_QoSViolationInd(u8* bd_addr) {
}

/* 0x8049AE00 (0x124): Acknowledges the peer's disconnect when asked and passes it to the channel's multiplexer. */
void RFCOMM_DisconnectInd(u16 lcid, u8 is_conf_needed) {
    tRFC_MCB* p_mcb = rfc_find_lcid_mcb(lcid);

    if (is_conf_needed) {
        L2CA_DisconnectRsp(lcid);
    }
    if (!p_mcb) {
        RFCOMM_TRACE_WARNING1("RFCOMM_DisconnectInd LCID:0x%x", lcid);
        return;
    }
    rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_DISC_IND, NULL);
}

/* 0x8049AF24 (0x2A0): Parses a received frame and passes it to the multiplexer or to its DLC's port. */
void RFCOMM_BufDataInd(u16 lcid, BT_HDR* p_buf) {
    tRFC_MCB* p_mcb = rfc_find_lcid_mcb(lcid);
    tPORT* p_port;
    u8 event;

    if (!p_mcb) {
        RFCOMM_TRACE_WARNING1("RFCOMM_BufDataInd LCID:0x%x", lcid);
        GKI_freebuf(p_buf);
        return;
    }

    event = rfc_parse_data(p_mcb, &rfc_cb.rfc.rx_frame, p_buf);
    if (event == RFC_EVENT_BAD_FRAME) {
        GKI_freebuf(p_buf);
        return;
    }

    if (rfc_cb.rfc.rx_frame.dlci == RFCOMM_MX_DLCI) {
        if (event == RFC_EVENT_UIH) {
            rfc_process_mx_message(p_mcb, p_buf);
            return;
        }
        rfc_mx_sm_execute(p_mcb, event, NULL);
        GKI_freebuf(p_buf);
        return;
    }

    if ((p_port = port_find_mcb_dlci_port(p_mcb, rfc_cb.rfc.rx_frame.dlci)) == NULL || !p_port->rfc.p_mcb) {
        if (event != RFC_EVENT_SABME) {
            if ((p_mcb->is_initiator && !rfc_cb.rfc.rx_frame.cr) || (!p_mcb->is_initiator && rfc_cb.rfc.rx_frame.cr)) {
                rfc_send_dm(p_mcb, rfc_cb.rfc.rx_frame.dlci, rfc_cb.rfc.rx_frame.pf);
            }
            GKI_freebuf(p_buf);
            return;
        }

        if ((p_port = port_find_dlci_port(rfc_cb.rfc.rx_frame.dlci)) == NULL) {
            rfc_send_dm(p_mcb, rfc_cb.rfc.rx_frame.dlci, TRUE);
            GKI_freebuf(p_buf);
            return;
        }
        p_mcb->port_inx[rfc_cb.rfc.rx_frame.dlci] = p_port->inx;
        p_port->rfc.p_mcb = p_mcb;
    }

    if (event == RFC_EVENT_UIH) {
        if (p_buf->len > 0) {
            rfc_port_sm_execute(p_port, event, p_buf);
        } else {
            GKI_freebuf(p_buf);
        }
        if (rfc_cb.rfc.rx_frame.credit != 0) {
            rfc_inc_credit(p_port, rfc_cb.rfc.rx_frame.credit);
        }
        return;
    }
    rfc_port_sm_execute(p_port, event, NULL);
    GKI_freebuf(p_buf);
}

/* 0x8049B1C4 (0x134): Passes L2CAP's congestion change of the channel to its multiplexer. */
void RFCOMM_CongestionStatusInd(u16 lcid, u8 is_congested) {
    tRFC_MCB* p_mcb = rfc_find_lcid_mcb(lcid);

    if (!p_mcb) {
        RFCOMM_TRACE_ERROR1("RFCOMM_CongestionStatusInd dropped LCID:0x%x", lcid);
        return;
    } else {
        RFCOMM_TRACE_EVENT1("RFCOMM_CongestionStatusInd LCID:0x%x", lcid);
    }
    rfc_process_l2cap_congestion(p_mcb, is_congested);
}

/* Returns the multiplexer of the L2CAP channel `lcid`, or NULL (also when the channel id was reused). */
static tRFC_MCB* rfc_find_lcid_mcb(u16 lcid) {
    tRFC_MCB* p_mcb;

    if (lcid - L2CAP_BASE_APPL_CID >= MAX_L2CAP_CHANNELS) {
        RFCOMM_TRACE_ERROR1("rfc_find_lcid_mcb LCID:0x%x", lcid);
        return NULL;
    } else {
        if ((p_mcb = rfc_cb.rfc.p_rfc_lcid_mcb[lcid - L2CAP_BASE_APPL_CID]) != NULL) {
            if (p_mcb->lcid != lcid) {
                RFCOMM_TRACE_WARNING2("rfc_find_lcid_mcb LCID reused LCID:0x%x current:0x%x", lcid, p_mcb->lcid);
                return NULL;
            }
        }
    }
    return p_mcb;
}

/* 0x8049B2F8 (0x1C): Stores the multiplexer of the L2CAP channel `lcid`. */
void rfc_save_lcid_mcb(tRFC_MCB* p_mcb, u16 lcid) {
    rfc_cb.rfc.p_rfc_lcid_mcb[lcid - L2CAP_BASE_APPL_CID] = p_mcb;
}
