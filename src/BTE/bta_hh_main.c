/*
 * BTE/bta_hh_main.c - the BTA HID host state machine: bta_hh_sm_execute, the event handler and the event-code
 *   trace names, with the state tables.
 *
 * RANGE. .text 0x80480190-0x804806E4 (3 functions, 0x554 B); .rodata 0x805737B8-0x80573840; .data
 *   0x806135C0-0x806138A0; .bss 0x80739468-0x80739698. Strong start: bta_hh_sm_execute is the only reader of the
 *   .rodata state tables and, with bta_hh_hdl_event and bta_hh_evt_code, of the .data from 0x806135C0. Medium end:
 *   bta_hh_find_cb opens the utility run and its .data 0x806138A0. The .bss control block 0x80739468 (0x230 B) is
 *   bta_hh_cb, read by every bta_hh file but the API.
 * FLAGS. the target is GC/3.0a5.2 + cflags_nw4r code (`-ipa file` inlines the static bta_hh_state_code twice into
 *   bta_hh_sm_execute, as the target shows); the `OS` lib default (Wii/1.3 + cflags_os) scores lower on
 *   every bta_hh unit.
 * NAMES. the BTE source's. GUESS: bta_hh_action, bta_hh_st_idle, bta_hh_st_w4_conn, bta_hh_st_connected, bta_hh_st_tbl
 *   (.rodata 0x805737B8, 0x805737E8, 0x80573800, 0x80573818, 0x80573830), bta_hh_cb (.bss 0x80739468).
 * RESIDUALS. the string relocations name the compiler's pool where the map has lbl_806135C0/lbl_8061369C.
 * SHAPES. plain C. The state-change trace passes the event after `event &= 0xff`, as the target does.
 */

#include "BTE/bta_hh_main.h"
#include "BTE/bta_hh_act.h"
#include "BTE/bta_hh_utils.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The state-machine actions, indexed by BTA_HH_*_ACT. */
const tBTA_HH_ACTION bta_hh_action[] = {
    bta_hh_api_disc_act,
    bta_hh_open_act,
    bta_hh_close_act,
    bta_hh_data_act,
    bta_hh_ctrl_dat_act,
    bta_hh_handsk_act,
    bta_hh_start_sdp,
    bta_hh_sdp_cmpl,
    bta_hh_write_dev_act,
    bta_hh_get_dscp_act,
    bta_hh_maint_dev_act,
    bta_hh_open_cmpl_act,
};

/* The idle state: {action, next state} per event. */
const u8 bta_hh_st_idle[BTA_HH_NUM_EVENTS][BTA_HH_NUM_COLS] = {
    /* API_OPEN   */ {BTA_HH_START_SDP, BTA_HH_W4_CONN_ST},
    /* API_CLOSE  */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* INT_OPEN   */ {BTA_HH_OPEN_ACT, BTA_HH_W4_CONN_ST},
    /* INT_CLOSE  */ {BTA_HH_CLOSE_ACT, BTA_HH_IDLE_ST},
    /* INT_DATA   */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* INT_CTRL   */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* INT_HANDSK */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* SDP_CMPL   */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* WRITE_DEV  */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* GET_DSCP   */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* MAINT_DEV  */ {BTA_HH_MAINT_DEV_ACT, BTA_HH_IDLE_ST},
    /* OPEN_CMPL  */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
};

/* The waiting-for-connection state. */
const u8 bta_hh_st_w4_conn[BTA_HH_NUM_EVENTS][BTA_HH_NUM_COLS] = {
    /* API_OPEN   */ {BTA_HH_IGNORE, BTA_HH_W4_CONN_ST},
    /* API_CLOSE  */ {BTA_HH_IGNORE, BTA_HH_IDLE_ST},
    /* INT_OPEN   */ {BTA_HH_OPEN_ACT, BTA_HH_W4_CONN_ST},
    /* INT_CLOSE  */ {BTA_HH_CLOSE_ACT, BTA_HH_IDLE_ST},
    /* INT_DATA   */ {BTA_HH_IGNORE, BTA_HH_W4_CONN_ST},
    /* INT_CTRL   */ {BTA_HH_IGNORE, BTA_HH_W4_CONN_ST},
    /* INT_HANDSK */ {BTA_HH_IGNORE, BTA_HH_W4_CONN_ST},
    /* SDP_CMPL   */ {BTA_HH_SDP_CMPL, BTA_HH_W4_CONN_ST},
    /* WRITE_DEV  */ {BTA_HH_IGNORE, BTA_HH_W4_CONN_ST},
    /* GET_DSCP   */ {BTA_HH_IGNORE, BTA_HH_W4_CONN_ST},
    /* MAINT_DEV  */ {BTA_HH_MAINT_DEV_ACT, BTA_HH_IDLE_ST},
    /* OPEN_CMPL  */ {BTA_HH_OPEN_CMPL_ACT, BTA_HH_CONN_ST},
};

/* The connected state. */
const u8 bta_hh_st_connected[BTA_HH_NUM_EVENTS][BTA_HH_NUM_COLS] = {
    /* API_OPEN   */ {BTA_HH_IGNORE, BTA_HH_CONN_ST},
    /* API_CLOSE  */ {BTA_HH_API_DISC_ACT, BTA_HH_CONN_ST},
    /* INT_OPEN   */ {BTA_HH_OPEN_ACT, BTA_HH_CONN_ST},
    /* INT_CLOSE  */ {BTA_HH_CLOSE_ACT, BTA_HH_IDLE_ST},
    /* INT_DATA   */ {BTA_HH_DATA_ACT, BTA_HH_CONN_ST},
    /* INT_CTRL   */ {BTA_HH_CTRL_DAT_ACT, BTA_HH_CONN_ST},
    /* INT_HANDSK */ {BTA_HH_HANDSK_ACT, BTA_HH_CONN_ST},
    /* SDP_CMPL   */ {BTA_HH_IGNORE, BTA_HH_CONN_ST},
    /* WRITE_DEV  */ {BTA_HH_WRITE_DEV_ACT, BTA_HH_CONN_ST},
    /* GET_DSCP   */ {BTA_HH_GET_DSCP_ACT, BTA_HH_CONN_ST},
    /* MAINT_DEV  */ {BTA_HH_MAINT_DEV_ACT, BTA_HH_CONN_ST},
    /* OPEN_CMPL  */ {BTA_HH_IGNORE, BTA_HH_CONN_ST},
};

/* The state tables, indexed by state - 1. */
const u8 (*const bta_hh_st_tbl[])[BTA_HH_NUM_COLS] = {
    bta_hh_st_idle,
    bta_hh_st_w4_conn,
    bta_hh_st_connected,
};

tBTA_HH_CB bta_hh_cb;

/* Returns the trace name of a state. */
static char* bta_hh_state_code(u8 state_code) {
    switch (state_code) {
    case BTA_HH_NULL_ST:
        return "BTA_HH_NULL_ST";
    case BTA_HH_IDLE_ST:
        return "BTA_HH_IDLE_ST";
    case BTA_HH_W4_CONN_ST:
        return "BTA_HH_W4_CONN_ST";
    case BTA_HH_CONN_ST:
        return "BTA_HH_CONN_ST";
    default:
        return "unknown HID Host state";
    }
}

/* 0x80480190 (0x354): Runs an event through a device's state machine, answering the application for a missing one. */
void bta_hh_sm_execute(tBTA_HH_DEV_CB* p_cb, u16 event, tBTA_HH_DATA* p_data) {
    const u8 (*state_table)[BTA_HH_NUM_COLS];
    u8 action;
    tBTA_HH cback_data;
    u16 cback_event = 0;
    u8 in_state;

    memset(&cback_data, 0, sizeof(tBTA_HH));

    if (p_cb == NULL) {
        if (bta_hh_cb.p_cback != NULL) {
            switch (event) {
            case BTA_HH_API_OPEN_EVT:
                cback_event = BTA_HH_OPEN_EVT;
                bdcpy(cback_data.conn.bda, p_data->api_conn.bd_addr);
                cback_data.conn.status = BTA_HH_ERR_DB_FULL;
                cback_data.conn.handle = BTA_HH_INVALID_HANDLE;
                break;
            case BTA_HH_API_MAINT_DEV_EVT:
                cback_event = p_data->api_maintdev.sub_event;

                if (p_data->api_maintdev.sub_event == BTA_HH_ADD_DEV_EVT) {
                    bdcpy(cback_data.dev_info.bda, p_data->api_maintdev.bda);
                    cback_data.dev_info.status = BTA_HH_ERR_DB_FULL;
                    cback_data.dev_info.handle = BTA_HH_INVALID_HANDLE;
                } else {
                    cback_data.dev_info.status = BTA_HH_ERR_HDL;
                    cback_data.dev_info.handle = (u8)p_data->api_maintdev.hdr.layer_specific;
                }
                break;
            case BTA_HH_API_WRITE_DEV_EVT:
                cback_event = (p_data->api_sndcmd.t_type - BTA_HH_FST_BTE_TRANS_EVT) + BTA_HH_FST_TRANS_CB_EVT;
                if (p_data->api_sndcmd.t_type == HID_TRANS_SET_PROTOCOL ||
                    p_data->api_sndcmd.t_type == HID_TRANS_SET_REPORT ||
                    p_data->api_sndcmd.t_type == HID_TRANS_SET_IDLE) {
                    cback_data.dev_status.status = BTA_HH_ERR_HDL;
                    cback_data.dev_status.handle = (u8)p_data->api_sndcmd.hdr.layer_specific;
                } else if (p_data->api_sndcmd.t_type != HID_TRANS_DATA &&
                           p_data->api_sndcmd.t_type != HID_TRANS_CONTROL) {
                    cback_data.hs_data.handle = (u8)p_data->api_sndcmd.hdr.layer_specific;
                    cback_data.hs_data.status = BTA_HH_ERR_HDL;
                }
                break;
            case BTA_HH_API_CLOSE_EVT:
                cback_event = BTA_HH_CLOSE_EVT;

                cback_data.dev_status.status = BTA_HH_ERR_HDL;
                cback_data.dev_status.handle = (u8)p_data->api_sndcmd.hdr.layer_specific;
                break;
            default:
                APPL_TRACE_ERROR1("wrong device handle: [%d]", p_data->hdr.layer_specific);
                break;
            }
            if (cback_event) {
                (*bta_hh_cb.p_cback)(cback_event, &cback_data);
            }
        }
    } else {
        in_state = p_cb->state;
        APPL_TRACE_EVENT3("bta_hh_sm_execute: State 0x%02x [%s], Event [%s]", in_state, bta_hh_state_code(in_state),
                          bta_hh_evt_code(event));

        state_table = bta_hh_st_tbl[p_cb->state - 1];

        event &= 0xff;

        p_cb->state = state_table[event][BTA_HH_NEXT_STATE];

        if ((action = state_table[event][BTA_HH_ACTION]) != BTA_HH_IGNORE) {
            (*bta_hh_action[action])(p_cb, p_data);
        }

        if (in_state != p_cb->state) {
            APPL_TRACE_DEBUG3("HH State Change: [%s] -> [%s] after Event [%s]", bta_hh_state_code(in_state),
                              bta_hh_state_code(p_cb->state), bta_hh_evt_code(event));
        }
    }
}

/* 0x804804E4 (0x14C): Dispatches a HID host message to its handler or to its device's state machine; returns TRUE. */
u8 bta_hh_hdl_event(BT_HDR* p_msg) {
    u8 index = BTA_HH_MAX_KNOWN;
    tBTA_HH_DEV_CB* p_cb = NULL;

    switch (p_msg->event) {
    case BTA_HH_API_ENABLE_EVT:
        bta_hh_api_enable((tBTA_HH_DATA*)p_msg);
        break;
    case BTA_HH_API_DISABLE_EVT:
        bta_hh_api_disable();
        break;
    case BTA_HH_DISC_CMPL_EVT:
        bta_hh_disc_cmpl();
        break;
    case BTA_HH_API_GET_ACL_Q_EVT:
        bta_hh_get_acl_q_info();
        break;
    default:
        if (p_msg->event == BTA_HH_API_OPEN_EVT) {
            index = bta_hh_find_cb(((tBTA_HH_API_CONN*)p_msg)->bd_addr);
        } else if (p_msg->event == BTA_HH_API_MAINT_DEV_EVT) {
            if (((tBTA_HH_MAINT_DEV*)p_msg)->sub_event == BTA_HH_ADD_DEV_EVT) {
                index = bta_hh_find_cb(((tBTA_HH_MAINT_DEV*)p_msg)->bda);
            } else {
                index = bta_hh_cb.cb_index[p_msg->layer_specific];
            }
        } else if (p_msg->layer_specific < BTA_HH_MAX_KNOWN) {
            index = bta_hh_cb.cb_index[p_msg->layer_specific];
        }

        if (index != BTA_HH_MAX_KNOWN) {
            p_cb = &bta_hh_cb.kdev[index];
        }

        APPL_TRACE_DEBUG2("bta_hh_hdl_event:: handle = %d dev_cb[%d] ", p_msg->layer_specific, index);
        bta_hh_sm_execute(p_cb, p_msg->event, (tBTA_HH_DATA*)p_msg);
    }
    return TRUE;
}

/* 0x80480630 (0xB4): Returns the trace name of an internal event. */
char* bta_hh_evt_code(u16 evt_code) {
    switch (evt_code) {
    case BTA_HH_API_DISABLE_EVT:
        return "BTA_HH_API_DISABLE_EVT";
    case BTA_HH_API_ENABLE_EVT:
        return "BTA_HH_API_ENABLE_EVT";
    case BTA_HH_API_OPEN_EVT:
        return "BTA_HH_API_OPEN_EVT";
    case BTA_HH_API_CLOSE_EVT:
        return "BTA_HH_API_CLOSE_EVT";
    case BTA_HH_INT_OPEN_EVT:
        return "BTA_HH_INT_OPEN_EVT";
    case BTA_HH_INT_CLOSE_EVT:
        return "BTA_HH_INT_CLOSE_EVT";
    case BTA_HH_INT_HANDSK_EVT:
        return "BTA_HH_INT_HANDSK_EVT";
    case BTA_HH_INT_DATA_EVT:
        return "BTA_HH_INT_DATA_EVT";
    case BTA_HH_INT_CTRL_DATA:
        return "BTA_HH_INT_CTRL_DATA";
    case BTA_HH_API_WRITE_DEV_EVT:
        return "BTA_HH_API_WRITE_DEV_EVT";
    case BTA_HH_SDP_CMPL_EVT:
        return "BTA_HH_SDP_CMPL_EVT";
    case BTA_HH_DISC_CMPL_EVT:
        return "BTA_HH_DISC_CMPL_EVT";
    case BTA_HH_API_MAINT_DEV_EVT:
        return "BTA_HH_API_MAINT_DEV_EVT";
    case BTA_HH_API_GET_DSCP_EVT:
        return "BTA_HH_API_GET_DSCP_EVT";
    case BTA_HH_OPEN_CMPL_EVT:
        return "BTA_HH_OPEN_CMPL_EVT";
    case BTA_HH_API_GET_ACL_Q_EVT:
        return "BTA_HH_API_GET_ACL_Q_EVT";
    default:
        return "unknown HID Host event code";
    }
}
