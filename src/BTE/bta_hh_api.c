/*
 * BTE/bta_hh_api.c - the BTA HID host API: enable and disable, open and close, send data, add and remove a device
 *   and the ACL queue information.
 *
 * RANGE. .text 0x8047FDC0-0x80480190 (8 functions, 0x3D0 B); .data 0x80613590-0x806135C0; .sdata2
 *   0x8079D0B0-0x8079D0B8. Strong: the BTA_Hh* run, every function posting a GKI message through bta_sys_sendmsg;
 *   bta_hh_sm_execute follows. The .data string 0x80613590 is read only by BTA_HhOpen; the .sdata2 pair is the
 *   bta_sys registration record (bta_hh_hdl_event, BTA_HhDisable) only BTA_HhEnable reads.
 * FLAGS. the target is GC/3.0a5.2 + cflags_nw4r code, as BTE/bta_hh_main.c: every row matches under it; the `OS` lib
 *   default (Wii/1.3 + cflags_os) scores lower.
 * NAMES. the BTE source's. GUESS: BTA_HhDisable (0x8047FE48, posts event 0x170D with no payload; the registration
 *   record's second slot), bta_hh_reg (.sdata2 0x8079D0B0).
 * RESIDUALS. the string relocation names the compiler's pool where the map has lbl_80613590.
 * SHAPES. plain C. BTA_HhSendData builds its message through the static helper bta_hh_snd_write_dev.
 */

#include "BTE/bta_hh_api.h"
#include "BTE/bta_hh_int.h"
#include "BTE/bta_hh_main.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The handlers the HID host registers with the BTA system. */
static const tBTA_SYS_REG bta_hh_reg = {bta_hh_hdl_event, BTA_HhDisable};

/* 0x8047FDC0 (0x88): Registers the HID host and posts the enable request with the security mask and callback. */
void BTA_HhEnable(u8 sec_mask, BtaHhCallback p_cback) {
    tBTA_HH_API_ENABLE* p_buf;

    GKI_sched_lock();
    bta_sys_register(BTA_ID_HH, &bta_hh_reg);
    GKI_sched_unlock();

    p_buf = (tBTA_HH_API_ENABLE*)GKI_getbuf(sizeof(tBTA_HH_API_ENABLE));

    if (p_buf != NULL) {
        memset(p_buf, 0, sizeof(tBTA_HH_API_ENABLE));

        p_buf->hdr.event = BTA_HH_API_ENABLE_EVT;
        p_buf->p_cback = (tBTA_HH_CBACK)p_cback;
        p_buf->sec_mask = sec_mask;

        bta_sys_sendmsg(p_buf);
    }
}

/* 0x8047FE48 (0x38): Posts the disable request. */
void BTA_HhDisable(void) {
    BT_HDR* p_buf;

    if ((p_buf = GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        p_buf->event = BTA_HH_API_DISABLE_EVT;
        bta_sys_sendmsg(p_buf);
    }
}

/* 0x8047FE80 (0x64): Posts the close request for a device. */
void BTA_HhClose(u8 dev_handle) {
    BT_HDR* p_buf;

    if ((p_buf = GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        memset(p_buf, 0, sizeof(BT_HDR));
        p_buf->event = BTA_HH_API_CLOSE_EVT;
        p_buf->layer_specific = (u16)dev_handle;

        bta_sys_sendmsg(p_buf);
    }
}

/* 0x8047FEE4 (0xB4): Posts the open request for the device at `dev_bda`. */
void BTA_HhOpen(const u8* dev_bda, u8 mode, u8 sec_mask) {
    tBTA_HH_API_CONN* p_buf;

    p_buf = (tBTA_HH_API_CONN*)GKI_getbuf(sizeof(tBTA_HH_API_CONN));

    if (p_buf != NULL) {
        memset(p_buf, 0, sizeof(tBTA_HH_API_CONN));

        p_buf->hdr.event = BTA_HH_API_OPEN_EVT;
        p_buf->hdr.layer_specific = BTA_HH_INVALID_HANDLE;
        p_buf->sec_mask = sec_mask;
        p_buf->mode = mode;
        bdcpy(p_buf->bd_addr, dev_bda);

        bta_sys_sendmsg(p_buf);
    } else {
        APPL_TRACE_ERROR0("No resource to send HID host Connect request.");
    }
}

/* Posts one HID transaction for a device. */
static void bta_hh_snd_write_dev(u8 dev_handle, u8 t_type, u8 param, u16 data, u8 rpt_id, BT_HDR* p_data) {
    tBTA_HH_CMD_DATA* p_buf;
    u16 len = (u16)sizeof(tBTA_HH_CMD_DATA);

    if ((p_buf = (tBTA_HH_CMD_DATA*)GKI_getbuf(len)) != NULL) {
        memset(p_buf, 0, sizeof(tBTA_HH_CMD_DATA));

        p_buf->hdr.event = BTA_HH_API_WRITE_DEV_EVT;
        p_buf->hdr.layer_specific = (u16)dev_handle;
        p_buf->t_type = t_type;
        p_buf->data = data;
        p_buf->param = param;
        p_buf->p_data = p_data;
        p_buf->rpt_id = rpt_id;

        bta_sys_sendmsg(p_buf);
    }
}

/* 0x8047FF98 (0x90): Posts an output report for a device. */
void BTA_HhSendData(u8 dev_handle, BT_HDR* p_data) {
    bta_hh_snd_write_dev(dev_handle, HID_TRANS_DATA, HID_PAR_REP_TYPE_OUTPUT, 0, 0, p_data);
}

/* 0x80480028 (0xA8): Posts the request that adds a device with its attributes and report descriptor. */
void BTA_HhAddDev(const u8* bda, u16 attr_mask, u8 sub_class, u8 app_id, BtaHhDscpInfo dscp_info) {
    tBTA_HH_MAINT_DEV* p_buf;

    p_buf = (tBTA_HH_MAINT_DEV*)GKI_getbuf(sizeof(tBTA_HH_MAINT_DEV));

    if (p_buf != NULL) {
        memset(p_buf, 0, sizeof(tBTA_HH_MAINT_DEV));

        p_buf->hdr.event = BTA_HH_API_MAINT_DEV_EVT;
        p_buf->sub_event = BTA_HH_ADD_DEV_EVT;
        p_buf->hdr.layer_specific = BTA_HH_INVALID_HANDLE;

        p_buf->attr_mask = (u16)attr_mask;
        p_buf->sub_class = sub_class;
        p_buf->app_id = app_id;
        memcpy(&p_buf->dscp_info, &dscp_info, sizeof(tBTA_HH_DEV_DSCP_INFO));
        bdcpy(p_buf->bda, bda);

        bta_sys_sendmsg(p_buf);
    }
}

/* 0x804800D0 (0x6C): Posts the request that removes a device. */
void BTA_HhRemoveDev(u8 dev_handle) {
    tBTA_HH_MAINT_DEV* p_buf;

    p_buf = (tBTA_HH_MAINT_DEV*)GKI_getbuf(sizeof(tBTA_HH_MAINT_DEV));

    if (p_buf != NULL) {
        memset(p_buf, 0, sizeof(tBTA_HH_MAINT_DEV));

        p_buf->hdr.event = BTA_HH_API_MAINT_DEV_EVT;
        p_buf->sub_event = BTA_HH_RMV_DEV_EVT;
        p_buf->hdr.layer_specific = (u16)dev_handle;

        bta_sys_sendmsg(p_buf);
    }
}

/* 0x8048013C (0x54): Posts the request that reports the ACL queue information. */
void BTA_HhGetAclQueueInfo(void) {
    BT_HDR* p_buf;

    if ((p_buf = GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        memset(p_buf, 0, sizeof(BT_HDR));
        p_buf->event = BTA_HH_API_GET_ACL_Q_EVT;

        bta_sys_sendmsg(p_buf);
    }
}
