/*
 * BTE/bta_hh_act.c - the BTA HID host actions: enable and disable, the SDP discovery, open and close, data,
 *   handshake and control, the ACL queue information, device writes and the HID host callback.
 *
 * RANGE. .text 0x8047E678-0x8047FDC0 (18 functions, 0x1748 B); .data 0x80613098-0x80613590; .sdata
 *   0x80793D80-0x80793D88. Strong start: the .data starts after an 8-byte pad (0x80613091 -> 0x80613098) and
 *   bta_hh_api_enable is the first reader of the bta_hh control block. Strong end: BTA_HhEnable opens the API run.
 *   The .data is this file's strings and the six jump tables of bta_hh_handsk_act, bta_hh_ctrl_dat_act and
 *   bta_hh_cback (each function's tables follow its strings); the .sdata is the "" service name bta_hh_api_enable and
 *   bta_hh_sdp_cmpl pass to HID_HostSetSecurityLevel.
 * FLAGS. the target is GC/3.0a5.2 + cflags_nw4r code, as BTE/bta_hh_main.c: `-ipa file` inlines bta_hh_disc_cmpl
 *   into bta_hh_api_disable and bta_hh_close_act, and the static name and status helpers into their callers.
 * NAMES. the BTE source's. GUESS: bta_hh_get_dscp_act (0x8047F6C4) and bta_hh_maint_dev_act (0x8047F6E0), the action
 *   table's slots 9 and 10 in the BTE action order.
 * RESIDUALS. the string relocations name the compiler's pool where the map has lbl_80613098.. labels.
 * SHAPES. plain C.
 */

#include "BTE/bta_hh_act.h"
#include "BTE/bta_hh_main.h"
#include "BTE/bta_hh_utils.h"
#include "BTE/l2c_main.h"
#include "BTE/l2c_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x8047E678 (0x174): Initialises the HID layer, registers the callback and the device control blocks, and reports. */
void bta_hh_api_enable(tBTA_HH_DATA* p_data) {
    u8 status = BTA_HH_ERR;
    u8 xx;

    HID_HostInit();

    memset(&bta_hh_cb, 0, sizeof(tBTA_HH_CB));

    HID_HostSetSecurityLevel("", p_data->api_enable.sec_mask);

    if (HID_HostRegister(bta_hh_cback) == HID_SUCCESS) {
        bta_hh_cb.p_cback = p_data->api_enable.p_cback;

        status = BTA_HH_OK;
        for (xx = 0; xx < BTA_HH_MAX_DEVICE; xx++) {
            bta_hh_cb.kdev[xx].state = BTA_HH_IDLE_ST;
            bta_hh_cb.kdev[xx].hid_handle = BTA_HH_INVALID_HANDLE;
            bta_hh_cb.kdev[xx].index = xx;
            bta_hh_cb.cb_index[xx] = BTA_HH_MAX_KNOWN;
        }
    }

    (*bta_hh_cb.p_cback)(BTA_HH_ENABLE_EVT, (tBTA_HH*)&status);
}

/* 0x8047E7EC (0x104): Disables at once without connections, or closes every connected device first. */
void bta_hh_api_disable(void) {
    u8 xx;

    if (bta_hh_cb.p_cback == NULL) {
        return;
    }

    if (!bta_hh_cb.cnt_num) {
        bta_hh_disc_cmpl();
    } else {
        bta_hh_cb.w4_disable = TRUE;

        for (xx = 0; xx < BTA_HH_MAX_DEVICE; xx++) {
            if (bta_hh_cb.kdev[xx].state == BTA_HH_CONN_ST) {
                bta_hh_sm_execute(&bta_hh_cb.kdev[xx], BTA_HH_API_CLOSE_EVT, NULL);
            }
        }
    }
}

/* 0x8047E8F0 (0xA4): Deregisters from the HID layer, frees the descriptors, reports the disable and clears the block. */
void bta_hh_disc_cmpl(void) {
    u8 xx;
    u8 status = BTA_HH_OK;

    if (HID_HostDeregister() != HID_SUCCESS) {
        status = BTA_HH_ERR;
    }

    for (xx = 0; xx < BTA_HH_MAX_DEVICE; xx++) {
        utl_freebuf((void**)&bta_hh_cb.kdev[xx].dscp_info.descriptor);
    }
    utl_freebuf((void**)&bta_hh_cb.p_disc_db);

    (*bta_hh_cb.p_cback)(BTA_HH_DISABLE_EVT, (tBTA_HH*)&status);

    memset(&bta_hh_cb, 0, sizeof(tBTA_HH_CB));
}

/* 0x8047E994 (0x134): The SDP result: registers a supported device with the HID layer and runs SDP_CMPL. */
static void bta_hh_sdp_cback(u16 result, u16 attr_mask, tHID_DEV_SDP_INFO* sdp_rec) {
    tBTA_HH_DEV_CB* p_cb = bta_hh_cb.p_cur;
    u8 hdl;
    u8 status = HID_ERR_SDP_BUSY;

    if (result == SDP_SUCCESS) {
        if (p_cb->sec_mask) {
            attr_mask |= HID_SEC_REQUIRED;
        }

        APPL_TRACE_EVENT3("bta_hh_sdp_cback: p_cb: %d result 0x%02x,                             attr_mask 0x%02x",
                          p_cb, result, attr_mask);

        if (bta_hh_tod_spt(p_cb, sdp_rec->sub_class)) {
            if (p_cb->hid_handle == BTA_HH_INVALID_HANDLE) {
                if ((status = HID_HostAddDev(p_cb->addr, attr_mask, &hdl)) == HID_SUCCESS) {
                    bta_hh_cb.cb_index[hdl] = p_cb->index;
                    bta_hh_add_device_to_list(p_cb, hdl, attr_mask, &sdp_rec->dscp_info, sdp_rec->sub_class,
                                              p_cb->app_id);
                } else {
                    p_cb->app_id = 0;
                }
            }
        } else {
            status = HID_SDP_TOD_UNSPT;
        }
    }

    utl_freebuf((void**)&bta_hh_cb.p_disc_db);

    bta_hh_sm_execute(p_cb, BTA_HH_SDP_CMPL_EVT, (tBTA_HH_DATA*)&status);
}

/* 0x8047EAC8 (0x194): Registers a known device without SDP, or starts the SDP query of a new one. */
void bta_hh_start_sdp(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    u8 status = HID_ERR_SDP_BUSY;
    u8 hdl;

    p_cb->sec_mask = p_data->api_conn.sec_mask;
    p_cb->mode = p_data->api_conn.mode;

    if (p_cb->app_id) {
        status = BTA_HH_OK;

        APPL_TRACE_DEBUG0("bta_hh_start_sdp:: skip SDP for known devices");

        if (p_cb->hid_handle == BTA_HH_INVALID_HANDLE) {
            if ((status = HID_HostAddDev(p_cb->addr, p_cb->attr_mask, &hdl)) == HID_SUCCESS) {
                bta_hh_add_device_to_list(p_cb, hdl, p_cb->attr_mask, NULL, p_cb->sub_class, p_cb->app_id);
                bta_hh_cb.cb_index[hdl] = p_cb->index;
            }
        }
        bta_hh_sm_execute(p_cb, BTA_HH_SDP_CMPL_EVT, (tBTA_HH_DATA*)&status);

        return;
    } else if (!bta_hh_cb.p_disc_db) {
        bta_hh_cb.p_disc_db = (tSDP_DISCOVERY_DB*)GKI_getbuf(p_bta_hh_cfg->sdp_db_size);

        bta_hh_cb.p_cur = p_cb;
        if ((status = HID_HostGetSDPRecord(p_data->api_conn.bd_addr, bta_hh_cb.p_disc_db, p_bta_hh_cfg->sdp_db_size,
                                           bta_hh_sdp_cback)) != HID_SUCCESS) {
            APPL_TRACE_DEBUG1("bta_hh_start_sdp:  HID_HostGetSDPRecord failed:                 Status 0x%2X", status);
            status = BTA_HH_ERR_SDP;
            utl_freebuf((void**)&bta_hh_cb.p_disc_db);
        }
    }

    if (status != BTA_HH_OK) {
        bta_hh_sm_execute(p_cb, BTA_HH_SDP_CMPL_EVT, (tBTA_HH_DATA*)&status);
    }
}

/* 0x8047EC5C (0x168): Opens the HID connection after a good SDP result, or reports the open failure. */
void bta_hh_sdp_cmpl(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_CONN conn_dat;
    u8 status;

    APPL_TRACE_DEBUG1("bta_hh_sdp_cmpl:  status 0x%2X", p_data->status);

    memset(&conn_dat, 0, sizeof(tBTA_HH_CONN));
    conn_dat.handle = p_cb->hid_handle;
    bdcpy(conn_dat.bda, p_cb->addr);

    if ((status = p_data->status) == BTA_HH_OK) {
        if (!p_cb->incoming_conn) {
            HID_HostSetSecurityLevel("", p_cb->sec_mask);

            if ((status = HID_HostOpenDev(p_cb->hid_handle)) != HID_SUCCESS) {
                APPL_TRACE_DEBUG1("bta_hh_sdp_cmpl:  HID_HostOpenDev failed:                     Status 0x%2X",
                                  status);
                HID_HostRemoveDev(p_cb->hid_handle);
            }
        } else {
            bta_hh_sm_execute(p_cb, BTA_HH_OPEN_CMPL_EVT, NULL);
        }
    }

    if (status != BTA_HH_OK) {
        if (p_data->status == HID_SDP_TOD_UNSPT) {
            conn_dat.status = BTA_HH_ERR_TOD_UNSPT;
        } else {
            conn_dat.status = BTA_HH_ERR_SDP;
        }

        (*bta_hh_cb.p_cback)(BTA_HH_OPEN_EVT, (tBTA_HH*)&conn_dat);

        bta_hh_sm_execute(p_cb, BTA_HH_API_CLOSE_EVT, NULL);

        if (p_cb->app_id == 0 && !p_cb->incoming_conn) {
            bta_hh_clean_up_kdev(p_cb);
        }
        bta_hh_trace_dev_db();
    }
}

/* 0x8047EDC4 (0x68): Closes the HID connection, reporting a close failure. */
void bta_hh_api_disc_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_CBDATA disc_dat;
    u8 status;

    disc_dat.handle = p_data ? (u8)p_data->hdr.layer_specific : p_cb->hid_handle;
    disc_dat.status = BTA_HH_ERR;

    status = HID_HostCloseDev(disc_dat.handle);

    if (status) {
        (*bta_hh_cb.p_cback)(BTA_HH_CLOSE_EVT, (tBTA_HH*)&disc_dat);
    }
}

/* 0x8047EE2C (0x128): Completes an open: notifies the callouts and the power manager and sets a non-report mode. */
void bta_hh_open_cmpl_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_CONN conn;
    u8 dev_handle = p_data ? (u8)p_data->hid_cback.hdr.layer_specific : p_cb->hid_handle;

    memset(&conn, 0, sizeof(tBTA_HH_CONN));
    conn.handle = dev_handle;
    bdcpy(conn.bda, p_cb->addr);

    bta_hh_cb.cnt_num++;

    bta_hh_co_open(p_cb->hid_handle, p_cb->sub_class, p_cb->attr_mask, p_cb->app_id);

    bta_sys_conn_open(BTA_ID_HH, p_cb->app_id, p_cb->addr);

    if (p_cb->mode != BTA_HH_PROTO_RPT_MODE) {
        if ((conn.status = HID_HostWriteDev(dev_handle, HID_TRANS_SET_PROTOCOL, p_cb->mode, 0, 0, NULL)) !=
            HID_SUCCESS) {
            conn.status = BTA_HH_ERR_PROTO;
            (*bta_hh_cb.p_cback)(BTA_HH_OPEN_EVT, (tBTA_HH*)&conn);
        } else {
            p_cb->w4_evt = BTA_HH_OPEN_EVT;
        }
    } else {
        p_cb->opened = TRUE;
        (*bta_hh_cb.p_cback)(BTA_HH_OPEN_EVT, (tBTA_HH*)&conn);
    }

    p_cb->incoming_conn = FALSE;
}

/* 0x8047EF54 (0xB8): A connection came up: completes it for a known device, or runs SDP first for a new one. */
void bta_hh_open_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_API_CONN conn_data;
    u8 dev_handle = p_data ? (u8)p_data->hid_cback.hdr.layer_specific : p_cb->hid_handle;

    APPL_TRACE_EVENT1("bta_hh_open_act:  Device[%d] connected", dev_handle);

    if (p_cb->app_id != 0) {
        bta_hh_sm_execute(p_cb, BTA_HH_OPEN_CMPL_EVT, p_data);
    } else {
        p_cb->incoming_conn = TRUE;

        memset(&conn_data, 0, sizeof(tBTA_HH_API_CONN));
        bdcpy(conn_data.bd_addr, p_cb->addr);
        bta_hh_start_sdp(p_cb, (tBTA_HH_DATA*)&conn_data);
    }
}

/* 0x8047F00C (0x58): Passes an input report to the data callout and frees its buffer. */
void bta_hh_data_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    BT_HDR* pdata = p_data->hid_cback.p_data;
    u8* p_rpt = ((BT_BUF*)pdata)->data + pdata->offset;

    bta_hh_co_data((u8)p_data->hid_cback.hdr.layer_specific, p_rpt, pdata->len, p_cb->mode, p_cb->sub_class,
                   p_cb->app_id);

    utl_freebuf((void**)&pdata);
}

/* Returns the trace name of the event a device waits for. */
static char* bta_hh_get_w4_event(u16 event) {
    switch (event) {
    case BTA_HH_GET_RPT_EVT:
        return "BTA_HH_GET_RPT_EVT";
    case BTA_HH_SET_RPT_EVT:
        return "BTA_HH_SET_RPT_EVT";
    case BTA_HH_GET_PROTO_EVT:
        return "BTA_HH_GET_PROTO_EVT";
    case BTA_HH_SET_PROTO_EVT:
        return "BTA_HH_SET_PROTO_EVT";
    case BTA_HH_GET_IDLE_EVT:
        return "BTA_HH_GET_IDLE_EVT";
    case BTA_HH_SET_IDLE_EVT:
        return "BTA_HH_SET_IDLE_EVT";
    case BTA_HH_OPEN_EVT:
        return "BTA_HH_OPEN_EVT";
    default:
        return "Unknown event";
    }
}

/* Maps a handshake result to the status the application receives. */
static u8 bta_hh_get_trans_status(u32 result) {
    switch (result) {
    case HID_PAR_HANDSHAKE_RSP_SUCCESS:
        return BTA_HH_OK;
    case HID_PAR_HANDSHAKE_RSP_NOT_READY:
    case HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID:
    case HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ:
    case HID_PAR_HANDSHAKE_RSP_ERR_INVALID_PARAM:
        return (u8)result;
    case HID_PAR_HANDSHAKE_RSP_ERR_UNKNOWN:
    case HID_PAR_HANDSHAKE_RSP_ERR_FATAL:
    default:
        return BTA_HH_HS_ERROR;
    }
}

/* 0x8047F064 (0x268): A handshake answered the pending transaction: reports its result to the application. */
void bta_hh_handsk_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_CBDATA cback_data;
    tBTA_HH_HSDATA hs_data;
    tBTA_HH_CONN conn;
    u8 status;

    APPL_TRACE_DEBUG2("HANDSHAKE received for: event = %s data= %d", bta_hh_get_w4_event(p_cb->w4_evt),
                      p_data->hid_cback.data);

    switch (p_cb->w4_evt) {
    case BTA_HH_GET_RPT_EVT:
    case BTA_HH_GET_PROTO_EVT:
    case BTA_HH_GET_IDLE_EVT:
        memset(&hs_data, 0, sizeof(tBTA_HH_HSDATA));
        hs_data.handle = p_cb->hid_handle;
        hs_data.status = bta_hh_get_trans_status(p_data->hid_cback.data);
        (*bta_hh_cb.p_cback)(p_cb->w4_evt, (tBTA_HH*)&hs_data);
        p_cb->w4_evt = 0;
        break;

    case BTA_HH_SET_RPT_EVT:
    case BTA_HH_SET_PROTO_EVT:
    case BTA_HH_SET_IDLE_EVT:
        cback_data.handle = p_cb->hid_handle;
        cback_data.status = bta_hh_get_trans_status(p_data->hid_cback.data);
        (*bta_hh_cb.p_cback)(p_cb->w4_evt, (tBTA_HH*)&cback_data);
        p_cb->w4_evt = 0;
        break;

    case BTA_HH_OPEN_EVT:
        status = BTA_HH_OK;
        if (p_data->hid_cback.data) {
            status = BTA_HH_ERR_PROTO;
        }
        conn.status = status;
        conn.handle = p_cb->hid_handle;
        bdcpy(conn.bda, p_cb->addr);
        (*bta_hh_cb.p_cback)(p_cb->w4_evt, (tBTA_HH*)&conn);
        bta_hh_trace_dev_db();
        p_cb->w4_evt = 0;
        if (conn.status == BTA_HH_OK) {
            p_cb->opened = TRUE;
        }
        break;

    default:
        APPL_TRACE_DEBUG0("unknown transaction type");
        break;
    }

    bta_sys_idle(BTA_ID_HH, p_cb->app_id, p_cb->addr);
}

/* 0x8047F2CC (0x1F8): A control-channel report answered a GET_ transaction: reports the value read back. */
void bta_hh_ctrl_dat_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    BT_HDR* pdata = p_data->hid_cback.p_data;
    u8* data = ((BT_BUF*)pdata)->data + pdata->offset;
    tBTA_HH_HSDATA hs_data;

    APPL_TRACE_DEBUG1("Ctrl DATA received w4: event[%s]", bta_hh_get_w4_event(p_cb->w4_evt));
    hs_data.status = BTA_HH_OK;
    hs_data.handle = p_cb->hid_handle;

    switch (p_cb->w4_evt) {
    case BTA_HH_GET_IDLE_EVT:
        hs_data.rsp_data.idle_rate = *data;
        break;
    case BTA_HH_GET_RPT_EVT:
        memcpy(&hs_data.rsp_data.rpt_data, pdata, sizeof(BT_HDR));
        break;
    case BTA_HH_GET_PROTO_EVT:
        hs_data.rsp_data.proto_mode = *data;
        break;
    case BTA_HH_SET_PROTO_EVT:
    case BTA_HH_SET_RPT_EVT:
    case BTA_HH_SET_IDLE_EVT:
    default:
        APPL_TRACE_DEBUG1("invalid  transaction type for DATA payload: 4_evt[%s]", bta_hh_get_w4_event(p_cb->w4_evt));
        break;
    }

    bta_sys_busy(BTA_ID_HH, p_cb->app_id, p_cb->addr);
    bta_sys_idle(BTA_ID_HH, p_cb->app_id, p_cb->addr);

    (*bta_hh_cb.p_cback)(p_cb->w4_evt, (tBTA_HH*)&hs_data);

    p_cb->w4_evt = 0;
    utl_freebuf((void**)&pdata);
}

/* 0x8047F4C4 (0x200): The HID connection went down: reports an open failure or the close, and finishes a disable. */
void bta_hh_close_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_CONN conn_dat;
    tBTA_HH_CBDATA disc_dat = {BTA_HH_OK, 0};
    u32 reason = p_data->hid_cback.data;
    u16 event = p_cb->vp ? BTA_HH_VC_UNPLUG_EVT : BTA_HH_CLOSE_EVT;

    disc_dat.handle = p_cb->hid_handle;
    disc_dat.status = p_data->hid_cback.data;

    if ((reason & (HID_L2CAP_CONN_FAIL | HID_L2CAP_REQ_FAIL | HID_L2CAP_CFG_FAIL)) || reason == HID_ERR_AUTH_FAILED ||
        reason == HID_ERR_L2CAP_FAILED || !p_cb->opened) {
        conn_dat.handle = p_cb->hid_handle;
        conn_dat.status = (reason == HID_ERR_AUTH_FAILED) ? BTA_HH_ERR_AUTH_FAILED : BTA_HH_ERR;
        bdcpy(conn_dat.bda, p_cb->addr);
        HID_HostCloseDev(p_cb->hid_handle);

        (*bta_hh_cb.p_cback)(BTA_HH_OPEN_EVT, (tBTA_HH*)&conn_dat);

        bta_hh_trace_dev_db();
        return;
    } else {
        bta_hh_co_close(p_cb->hid_handle, p_cb->app_id);
        bta_sys_conn_close(BTA_ID_HH, p_cb->app_id, p_cb->addr);
        bta_hh_cb.cnt_num--;
        p_cb->opened = FALSE;

        if (disc_dat.status) {
            disc_dat.status = BTA_HH_ERR;
        }

        (*bta_hh_cb.p_cback)(event, (tBTA_HH*)&disc_dat);

        if (p_cb->vp) {
            HID_HostRemoveDev(p_cb->hid_handle);
            bta_hh_clean_up_kdev(p_cb);
        }

        bta_hh_trace_dev_db();
    }

    p_cb->vp = FALSE;
    p_cb->w4_evt = 0;

    if (bta_hh_cb.cnt_num == 0 && bta_hh_cb.w4_disable) {
        bta_hh_disc_cmpl();
    }
}

/* 0x8047F6C4 (0x1C): Reports the device's report descriptor. */
void bta_hh_get_dscp_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    (*bta_hh_cb.p_cback)(BTA_HH_GET_DSCP_EVT, (tBTA_HH*)&p_cb->dscp_info);
}

/* 0x8047F6E0 (0x164): Adds a device to the HID layer and the device list, or removes one, and reports the result. */
void bta_hh_maint_dev_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_MAINT_DEV* p_dev_info = &p_data->api_maintdev;
    tBTA_HH_DEV_INFO dev_info;
    u8 dev_handle;

    dev_info.status = BTA_HH_ERR;
    dev_info.handle = BTA_HH_INVALID_HANDLE;

    switch (p_dev_info->sub_event) {
    case BTA_HH_ADD_DEV_EVT:
        bdcpy(dev_info.bda, p_dev_info->bda);

        if (p_cb->hid_handle == BTA_HH_INVALID_HANDLE) {
            if (HID_HostAddDev(p_dev_info->bda, p_dev_info->attr_mask, &dev_handle) == HID_SUCCESS) {
                dev_info.status = BTA_HH_OK;
                dev_info.handle = dev_handle;

                bta_hh_add_device_to_list(p_cb, dev_handle, p_dev_info->attr_mask, NULL, p_dev_info->sub_class,
                                          p_dev_info->app_id);
                bta_hh_cb.cb_index[dev_handle] = p_cb->index;
            }
        } else {
            dev_info.handle = p_cb->hid_handle;
            dev_info.status = BTA_HH_OK;
        }
        bta_hh_trace_dev_db();
        break;

    case BTA_HH_RMV_DEV_EVT:
        dev_info.handle = (u8)p_dev_info->hdr.layer_specific;
        bdcpy(dev_info.bda, p_cb->addr);

        if (p_cb->state != BTA_HH_CONN_ST) {
            if (HID_HostRemoveDev(dev_info.handle) == HID_SUCCESS) {
                dev_info.status = BTA_HH_OK;

                bta_hh_clean_up_kdev(p_cb);
            }
        }
        break;

    default:
        APPL_TRACE_DEBUG0("invalid command");
        break;
    }

    (*bta_hh_cb.p_cback)(p_dev_info->sub_event, (tBTA_HH*)&dev_info);
}

/* 0x8047F844 (0x11C): Reports the controller's ACL buffers and each connected device's interrupt-link queue. */
void bta_hh_get_acl_q_info(void) {
    u8 xx;
    u8 num_dev = 0;
    tBTA_HH_ACL_Q_INFO q_info;
    tL2C_CCB* p_ccb;
    tL2C_LCB* p_lcb;

    memset(&q_info, 0, sizeof(tBTA_HH_ACL_Q_INFO));

    for (xx = 0; xx < BTA_HH_MAX_DEVICE; xx++) {
        if (bta_hh_cb.kdev[xx].in_use && bta_hh_cb.kdev[xx].state == BTA_HH_CONN_ST) {
            q_info.dev[num_dev].handle = bta_hh_cb.kdev[xx].hid_handle;

            if ((p_ccb = l2cu_find_ccb_by_cid(NULL,
                                              hh_cb.devices[bta_hh_cb.kdev[xx].hid_handle].conn.intr_cid)) != NULL &&
                (p_lcb = p_ccb->p_lcb) != NULL && p_lcb->in_use) {
                q_info.dev[num_dev].num_queued = p_lcb->link_xmit_data_q.count;
                q_info.dev[num_dev].num_not_acked = p_lcb->sent_not_acked;
            }
            num_dev++;
        }
    }

    q_info.xmit_window = l2cb.controller_xmit_window;
    q_info.num_acl_bufs = l2cb.num_lm_acl_bufs;
    q_info.num_dev = num_dev;

    (*bta_hh_cb.p_cback)(BTA_HH_GET_ACL_Q_EVT, (tBTA_HH*)&q_info);
}

/* 0x8047F960 (0x1C8): Sends a HID transaction, reporting a failure or recording the event its answer completes. */
void bta_hh_write_dev_act(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data) {
    tBTA_HH_CBDATA cbdata = {BTA_HH_OK, 0};
    u16 event = (p_data->api_sndcmd.t_type - BTA_HH_FST_BTE_TRANS_EVT) + BTA_HH_FST_TRANS_CB_EVT;
    u8 status;

    cbdata.handle = p_cb->hid_handle;

    if ((status = HID_HostWriteDev(p_cb->hid_handle, p_data->api_sndcmd.t_type, p_data->api_sndcmd.param,
                                   p_data->api_sndcmd.data, p_data->api_sndcmd.rpt_id, p_data->api_sndcmd.p_data)) !=
        HID_SUCCESS) {
        APPL_TRACE_ERROR1("HID_HostWriteDev Error %d", status);
        cbdata.status = BTA_HH_ERR;

        if (p_data->api_sndcmd.t_type != HID_TRANS_CONTROL && p_data->api_sndcmd.t_type != HID_TRANS_DATA) {
            (*bta_hh_cb.p_cback)(event, (tBTA_HH*)&cbdata);
        } else if (p_data->api_sndcmd.param == BTA_HH_CTRL_VIRTUAL_CABLE_UNPLUG) {
            (*bta_hh_cb.p_cback)(BTA_HH_VC_UNPLUG_EVT, (tBTA_HH*)&cbdata);
        }
    } else {
        switch (p_data->api_sndcmd.t_type) {
        case HID_TRANS_SET_PROTOCOL:
        case HID_TRANS_GET_REPORT:
        case HID_TRANS_SET_REPORT:
        case HID_TRANS_GET_PROTOCOL:
        case HID_TRANS_GET_IDLE:
        case HID_TRANS_SET_IDLE:
            p_cb->w4_evt = event;
            break;
        case HID_TRANS_DATA:
        case HID_TRANS_CONTROL:
            if (p_data->api_sndcmd.param == BTA_HH_CTRL_VIRTUAL_CABLE_UNPLUG) {
                p_cb->vp = TRUE;
            }
            break;
        case HID_TRANS_DATAC:
        default:
            APPL_TRACE_DEBUG1("bta_hh_write_dev_act:: cmd type = %d", p_data->api_sndcmd.t_type);
            break;
        }

        if (p_data->api_sndcmd.t_type != HID_TRANS_CONTROL) {
            bta_sys_busy(BTA_ID_HH, p_cb->app_id, p_cb->addr);
            bta_sys_idle(BTA_ID_HH, p_cb->app_id, p_cb->addr);
        } else if (p_data->api_sndcmd.param == BTA_HH_CTRL_SUSPEND) {
            bta_sys_sco_close(BTA_ID_HH, p_cb->app_id, p_cb->addr);
        } else if (p_data->api_sndcmd.param == BTA_HH_CTRL_EXIT_SUSPEND) {
            bta_sys_busy(BTA_ID_HH, p_cb->app_id, p_cb->addr);
        }
    }
}

/* Returns the trace name of a HID layer event. */
static char* bta_hh_hid_event_name(u16 event) {
    switch (event) {
    case HID_HDEV_EVT_OPEN:
        return "HID_HDEV_EVT_OPEN";
    case HID_HDEV_EVT_CLOSE:
        return "HID_HDEV_EVT_CLOSE";
    case HID_HDEV_EVT_RETRYING:
        return "HID_HDEV_EVT_RETRYING";
    case HID_HDEV_EVT_INTR_DATA:
        return "HID_HDEV_EVT_INTR_DATA";
    case HID_HDEV_EVT_INTR_DATC:
        return "HID_HDEV_EVT_INTR_DATC";
    case HID_HDEV_EVT_CTRL_DATA:
        return "HID_HDEV_EVT_CTRL_DATA";
    case HID_HDEV_EVT_CTRL_DATC:
        return "HID_HDEV_EVT_CTRL_DATC";
    case HID_HDEV_EVT_HANDSHAKE:
        return "HID_HDEV_EVT_HANDSHAKE";
    case HID_HDEV_EVT_VC_UNPLUG:
        return "HID_HDEV_EVT_VC_UNPLUG";
    default:
        return "Unknown HID event";
    }
}

/* 0x8047FB28 (0x298): The HID layer's device callback: posts the event as a message to the BTA task. */
void bta_hh_cback(u8 dev_handle, u8 event, u32 data, BT_HDR* pdata) {
    tBTA_HH_CBACK_DATA* p_buf;
    u16 sm_event = BTA_HH_INVALID_EVT;
    u8 xx = 0;

    APPL_TRACE_DEBUG1("bta_hh_cback::HID_event [%s]", bta_hh_hid_event_name(event));

    switch (event) {
    case HID_HDEV_EVT_OPEN:
        sm_event = BTA_HH_INT_OPEN_EVT;
        break;
    case HID_HDEV_EVT_CLOSE:
        sm_event = BTA_HH_INT_CLOSE_EVT;
        break;
    case HID_HDEV_EVT_INTR_DATA:
        sm_event = BTA_HH_INT_DATA_EVT;
        break;
    case HID_HDEV_EVT_HANDSHAKE:
        sm_event = BTA_HH_INT_HANDSK_EVT;
        break;
    case HID_HDEV_EVT_CTRL_DATA:
        sm_event = BTA_HH_INT_CTRL_DATA;
        break;
    case HID_HDEV_EVT_RETRYING:
        break;
    case HID_HDEV_EVT_INTR_DATC:
    case HID_HDEV_EVT_CTRL_DATC:
        utl_freebuf((void**)&pdata);
        break;
    case HID_HDEV_EVT_VC_UNPLUG:
        for (xx = 0; xx < BTA_HH_MAX_DEVICE; xx++) {
            if (bta_hh_cb.kdev[xx].hid_handle == dev_handle) {
                bta_hh_cb.kdev[xx].vp = TRUE;
                break;
            }
        }
        break;
    }

    if (sm_event != BTA_HH_INVALID_EVT &&
        (p_buf = (tBTA_HH_CBACK_DATA*)GKI_getbuf(sizeof(tBTA_HH_CBACK_DATA) + sizeof(BT_HDR))) != NULL) {
        p_buf->hdr.event = sm_event;
        p_buf->hdr.layer_specific = (u16)dev_handle;
        p_buf->data = data;
        p_buf->p_data = pdata;

        bta_sys_sendmsg(p_buf);
    }
}
