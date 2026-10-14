/*
 * BTE/rfc_mx_fsm.c - the RFCOMM multiplexer state machine: rfc_mx_sm_execute, its per-state handlers and the L2CAP
 *   config replies.
 *
 * RANGE. .text 0x8049B314-0x8049BF58 (10 functions, 0xC44 B); .data 0x806176B8-0x80617970. The .text holds the BTE
 *   rfc_mx_fsm.c functions rfc_mx_sm_execute..rfc_mx_conf_ind; rfc_port_fsm.c's rfc_port_sm_execute follows. The .data
 *   holds the state machine's jump tables and trace strings, read only by this range's functions; no pad beyond 4-byte
 *   alignment.
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. none: .text and the claimed data are byte-identical to the target (the .data tail is alignment fill).
 * SHAPES. plain C; the state handlers and the config replies are static, rfc_mx_send_config_req is inlined into its two
 *   callers.
 */

#include "BTE/rfc_mx_fsm.h"
#include "BTE/port_api.h"
#include "BTE/port_rfc.h"
#include "BTE/rfc_l2cap_if.h"
#include "BTE/rfc_ts_frames.h"
#include "BTE/rfc_utils.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The L2CAP results and sizes the multiplexer's channel setup uses. */
#define L2CAP_CONN_OK 0
#define L2CAP_CONN_PENDING 1
#define L2CAP_CFG_OK 0
#define L2CAP_DEFAULT_MTU 672
#define L2CAP_MTU_SIZE 1691

/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_idle(tRFC_MCB* p_mcb, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_wait_conn_cnf(tRFC_MCB* p_mcb, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_configure(tRFC_MCB* p_mcb, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_sabme_wait_ua(tRFC_MCB* p_mcb, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_wait_sabme(tRFC_MCB* p_mcb, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_connected(tRFC_MCB* p_mcb, u16 event, void* p_data);
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_disc_wait_ua(tRFC_MCB* p_mcb, u16 event, void* p_data);
static void rfc_mx_send_config_req(tRFC_MCB* p_mcb);
static void rfc_mx_conf_cnf(tRFC_MCB* p_mcb, tL2CAP_CFG_INFO* p_cfg);
static void rfc_mx_conf_ind(tRFC_MCB* p_mcb, tL2CAP_CFG_INFO* p_cfg);

/* 0x8049B314 (0x44): Passes the event to the handler of the multiplexer's state. */
/* untyped: a caller-owned payload - its type depends on the event */
void rfc_mx_sm_execute(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    switch (p_mcb->state) {
    case RFC_MX_STATE_IDLE:
        rfc_mx_sm_state_idle(p_mcb, event, p_data);
        break;
    case RFC_MX_STATE_WAIT_CONN_CNF:
        rfc_mx_sm_state_wait_conn_cnf(p_mcb, event, p_data);
        break;
    case RFC_MX_STATE_CONFIGURE:
        rfc_mx_sm_state_configure(p_mcb, event, p_data);
        break;
    case RFC_MX_STATE_SABME_WAIT_UA:
        rfc_mx_sm_sabme_wait_ua(p_mcb, event, p_data);
        break;
    case RFC_MX_STATE_WAIT_SABME:
        rfc_mx_sm_state_wait_sabme(p_mcb, event, p_data);
        break;
    case RFC_MX_STATE_CONNECTED:
        rfc_mx_sm_state_connected(p_mcb, event, p_data);
        break;
    case RFC_MX_STATE_DISC_WAIT_UA:
        rfc_mx_sm_state_disc_wait_ua(p_mcb, event, p_data);
        break;
    default:
        break;
    }
}

/* 0x8049B358 (0x214): Opens the L2CAP channel for a start request, or accepts and configures an incoming one. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_idle(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_state_idle - evt:%d", event);
    switch (event) {
    case RFC_MX_EVENT_START_REQ:
        p_mcb->peer_l2cap_mtu = L2CAP_DEFAULT_MTU - RFCOMM_MIN_OFFSET - 1;
        if ((p_mcb->lcid = L2CA_ConnectReq(BT_PSM_RFCOMM, p_mcb->bd_addr)) == 0) {
            PORT_StartCnf(p_mcb, RFCOMM_ERROR);
            return;
        }
        rfc_save_lcid_mcb(p_mcb, p_mcb->lcid);
        p_mcb->state = RFC_MX_STATE_WAIT_CONN_CNF;
        return;

    case RFC_MX_EVENT_START_RSP:
    case RFC_MX_EVENT_CONN_CNF:
    case RFC_MX_EVENT_CONF_IND:
    case RFC_MX_EVENT_CONF_CNF:
        RFCOMM_TRACE_ERROR2("Mx error state %d event %d", p_mcb->state, event);
        return;

    case RFC_MX_EVENT_CONN_IND:
        if (p_mcb->is_initiator) {
            L2CA_ConnectRsp(p_mcb->bd_addr, *(u8*)p_data, p_mcb->lcid, L2CAP_CONN_PENDING, 0);
            return;
        }
        rfc_timer_start(p_mcb, RFCOMM_CONN_TIMEOUT);
        L2CA_ConnectRsp(p_mcb->bd_addr, *(u8*)p_data, p_mcb->lcid, L2CAP_CONN_OK, 0);
        rfc_mx_send_config_req(p_mcb);
        p_mcb->state = RFC_MX_STATE_CONFIGURE;
        return;

    case RFC_EVENT_SABME:
        break;

    case RFC_EVENT_UA:
    case RFC_EVENT_DM:
        return;

    case RFC_EVENT_DISC:
        rfc_send_dm(p_mcb, RFCOMM_MX_DLCI, TRUE);
        return;

    case RFC_EVENT_UIH:
        rfc_send_dm(p_mcb, RFCOMM_MX_DLCI, FALSE);
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* 0x8049B56C (0x1BC): Configures the channel once L2CAP accepts it, or gives the multiplexer up. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_wait_conn_cnf(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_state_wait_conn_cnf - evt:%d", event);
    switch (event) {
    case RFC_MX_EVENT_START_REQ:
        RFCOMM_TRACE_ERROR2("Mx error state %d event %d", p_mcb->state, event);
        return;

    case RFC_MX_EVENT_CONN_CNF:
        if (*(u16*)p_data != L2CAP_CONN_OK) {
            p_mcb->state = RFC_MX_STATE_IDLE;
            PORT_StartCnf(p_mcb, *(u16*)p_data);
            return;
        }
        p_mcb->state = RFC_MX_STATE_CONFIGURE;
        rfc_mx_send_config_req(p_mcb);
        return;

    case RFC_MX_EVENT_DISC_IND:
        p_mcb->state = RFC_MX_STATE_IDLE;
        PORT_CloseInd(p_mcb);
        return;

    case RFC_EVENT_TIMEOUT:
        p_mcb->state = RFC_MX_STATE_IDLE;
        L2CA_DisconnectReq(p_mcb->lcid);
        PORT_CloseInd(p_mcb);
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* 0x8049B728 (0x128): Exchanges the channel configurations. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_configure(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_state_configure - evt:%d", event);
    switch (event) {
    case RFC_MX_EVENT_START_REQ:
    case RFC_MX_EVENT_CONN_CNF:
        RFCOMM_TRACE_ERROR2("Mx error state %d event %d", p_mcb->state, event);
        return;

    case RFC_MX_EVENT_CONF_IND:
        rfc_mx_conf_ind(p_mcb, (tL2CAP_CFG_INFO*)p_data);
        return;

    case RFC_MX_EVENT_CONF_CNF:
        rfc_mx_conf_cnf(p_mcb, (tL2CAP_CFG_INFO*)p_data);
        return;

    case RFC_MX_EVENT_DISC_IND:
        p_mcb->state = RFC_MX_STATE_IDLE;
        PORT_CloseInd(p_mcb);
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* 0x8049B850 (0x174): Waits for the peer's UA to our SABME on DLCI 0 and reports the start result. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_sabme_wait_ua(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_sabme_wait_ua - evt:%d", event);
    switch (event) {
    case RFC_MX_EVENT_START_REQ:
    case RFC_MX_EVENT_CONN_CNF:
        RFCOMM_TRACE_ERROR2("Mx error state %d event %d", p_mcb->state, event);
        return;

    case RFC_MX_EVENT_CONF_IND:
        rfc_mx_conf_ind(p_mcb, (tL2CAP_CFG_INFO*)p_data);
        return;

    case RFC_MX_EVENT_CONF_CNF:
        rfc_mx_conf_cnf(p_mcb, (tL2CAP_CFG_INFO*)p_data);
        return;

    case RFC_MX_EVENT_DISC_IND:
        p_mcb->state = RFC_MX_STATE_IDLE;
        PORT_CloseInd(p_mcb);
        return;

    case RFC_EVENT_UA:
        rfc_timer_stop(p_mcb);
        p_mcb->state = RFC_MX_STATE_CONNECTED;
        p_mcb->peer_ready = TRUE;
        PORT_StartCnf(p_mcb, RFCOMM_SUCCESS);
        return;

    case RFC_EVENT_DM:
        rfc_timer_stop(p_mcb);
    case RFC_EVENT_TIMEOUT:
        p_mcb->state = RFC_MX_STATE_IDLE;
        L2CA_DisconnectReq(p_mcb->lcid);
        PORT_StartCnf(p_mcb, RFCOMM_ERROR);
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* 0x8049B9C4 (0x120): Waits for the peer's SABME on DLCI 0 and answers it once the port layer agrees. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_wait_sabme(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_state_wait_sabme - evt:%d", event);
    switch (event) {
    case RFC_MX_EVENT_DISC_IND:
        p_mcb->state = RFC_MX_STATE_IDLE;
        PORT_CloseInd(p_mcb);
        return;

    case RFC_EVENT_SABME:
        PORT_StartInd(p_mcb);
        return;

    case RFC_MX_EVENT_START_RSP:
        if (*(u16*)p_data != RFCOMM_SUCCESS) {
            rfc_send_dm(p_mcb, RFCOMM_MX_DLCI, TRUE);
        } else {
            rfc_send_ua(p_mcb, RFCOMM_MX_DLCI);
            p_mcb->state = RFC_MX_STATE_CONNECTED;
            p_mcb->peer_ready = TRUE;
        }
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* 0x8049BAE4 (0x118): Disconnects DLCI 0 on a close request or timeout, and answers the peer's DISC. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_connected(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_state_connected - evt:%d", event);
    switch (event) {
    case RFC_EVENT_TIMEOUT:
    case RFC_MX_EVENT_CLOSE_REQ:
        rfc_timer_start(p_mcb, RFC_DISC_TIMEOUT);
        p_mcb->state = RFC_MX_STATE_DISC_WAIT_UA;
        rfc_send_disc(p_mcb, RFCOMM_MX_DLCI);
        return;

    case RFC_MX_EVENT_DISC_IND:
        p_mcb->state = RFC_MX_STATE_IDLE;
        PORT_CloseInd(p_mcb);
        return;

    case RFC_EVENT_DISC:
        rfc_send_ua(p_mcb, RFCOMM_MX_DLCI);
        if (p_mcb->is_initiator) {
            L2CA_DisconnectReq(p_mcb->lcid);
        }
        PORT_CloseInd(p_mcb);
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* 0x8049BBFC (0x178): Closes the channel once the peer answers our DISC, reopening it for a start that arrived since. */
/* untyped: a caller-owned payload - its type depends on the event */
static void rfc_mx_sm_state_disc_wait_ua(tRFC_MCB* p_mcb, u16 event, void* p_data) {
    RFCOMM_TRACE_EVENT1("rfc_mx_sm_state_disc_wait_ua - evt:%d", event);
    switch (event) {
    case RFC_EVENT_UA:
    case RFC_EVENT_DM:
    case RFC_EVENT_TIMEOUT:
        L2CA_DisconnectReq(p_mcb->lcid);
        if (p_mcb->restart_required) {
            if ((p_mcb->lcid = L2CA_ConnectReq(BT_PSM_RFCOMM, p_mcb->bd_addr)) == 0) {
                PORT_StartCnf(p_mcb, RFCOMM_ERROR);
                return;
            }
            rfc_save_lcid_mcb(p_mcb, p_mcb->lcid);
            p_mcb->restart_required = FALSE;
            p_mcb->local_cfg_sent = FALSE;
            p_mcb->peer_cfg_rcvd = FALSE;
            p_mcb->state = RFC_MX_STATE_WAIT_CONN_CNF;
            return;
        }
        rfc_release_multiplexer_channel(p_mcb);
        return;

    case RFC_EVENT_DISC:
        rfc_send_ua(p_mcb, RFCOMM_MX_DLCI);
        return;

    case RFC_EVENT_UIH:
        GKI_freebuf(p_data);
        rfc_send_dm(p_mcb, RFCOMM_MX_DLCI, FALSE);
        return;

    case RFC_MX_EVENT_START_REQ:
        p_mcb->restart_required = TRUE;
        return;

    case RFC_MX_EVENT_DISC_IND:
        p_mcb->state = RFC_MX_STATE_IDLE;
        PORT_CloseInd(p_mcb);
        return;

    case RFC_MX_EVENT_CLOSE_REQ:
        return;
    }
    RFCOMM_TRACE_EVENT2("RFCOMM MX ignored - evt:%d in state:%d", event, p_mcb->state);
}

/* Sends our channel configuration: the RFCOMM MTU, no flush timeout and no QoS. */
static void rfc_mx_send_config_req(tRFC_MCB* p_mcb) {
    tL2CAP_CFG_INFO cfg;

    RFCOMM_TRACE_EVENT0("rfc_mx_send_config_req");
    memset(&cfg, 0, sizeof(tL2CAP_CFG_INFO));
    cfg.mtu_present = TRUE;
    cfg.mtu = L2CAP_MTU_SIZE;
    cfg.flush_to_present = FALSE;
    cfg.qos_present = FALSE;
    L2CA_ConfigReq(p_mcb->lcid, &cfg);
}

/* 0x8049BD74 (0xF4): Takes the answer to our configuration and sends SABME on DLCI 0 once both sides are configured. */
static void rfc_mx_conf_cnf(tRFC_MCB* p_mcb, tL2CAP_CFG_INFO* p_cfg) {
    RFCOMM_TRACE_EVENT2("rfc_mx_conf_cnf p_cfg:%08x res:%d ", p_cfg, (p_cfg) ? p_cfg->result : 0);

    if (p_cfg->result != L2CAP_CFG_OK) {
        if (p_mcb->is_initiator) {
            PORT_StartCnf(p_mcb, p_cfg->result);
            L2CA_DisconnectReq(p_mcb->lcid);
        }
        rfc_release_multiplexer_channel(p_mcb);
        return;
    }

    p_mcb->local_cfg_sent = TRUE;
    if (p_mcb->state == RFC_MX_STATE_CONFIGURE && p_mcb->peer_cfg_rcvd) {
        if (p_mcb->is_initiator) {
            p_mcb->state = RFC_MX_STATE_SABME_WAIT_UA;
            rfc_send_sabme(p_mcb, RFCOMM_MX_DLCI);
        } else {
            p_mcb->state = RFC_MX_STATE_WAIT_SABME;
        }
    }
}

/* 0x8049BE68 (0xF0): Takes the peer's MTU from its configuration, accepts it and moves on once both sides are
 * configured. */
static void rfc_mx_conf_ind(tRFC_MCB* p_mcb, tL2CAP_CFG_INFO* p_cfg) {
    RFCOMM_TRACE_EVENT1("rfc_mx_conf_ind p_cfg:%0x", p_cfg);

    if (p_cfg->mtu_present) {
        p_mcb->peer_l2cap_mtu = p_cfg->mtu - RFCOMM_MIN_OFFSET - 1;
    } else {
        p_mcb->peer_l2cap_mtu = L2CAP_DEFAULT_MTU - RFCOMM_MIN_OFFSET - 1;
    }

    p_cfg->mtu_present = FALSE;
    p_cfg->flush_to_present = FALSE;
    p_cfg->qos_present = FALSE;
    p_cfg->result = L2CAP_CFG_OK;
    L2CA_ConfigRsp(p_mcb->lcid, p_cfg);

    p_mcb->peer_cfg_rcvd = TRUE;
    if (p_mcb->state == RFC_MX_STATE_CONFIGURE && p_mcb->local_cfg_sent) {
        if (p_mcb->is_initiator) {
            p_mcb->state = RFC_MX_STATE_SABME_WAIT_UA;
            rfc_send_sabme(p_mcb, RFCOMM_MX_DLCI);
        } else {
            p_mcb->state = RFC_MX_STATE_WAIT_SABME;
        }
    }
}
