/*
 * BTE/bta_dm_api.c - the BTA device manager API: BTA_EnableBluetooth and BTA_DisableBluetooth, device state, name
 *   and visibility, search and cancel, PIN reply and adding and removing devices.
 *
 * RANGE. .text 0x8047D690-0x8047DAFC (11 functions, 0x46C B); .sdata2 0x8079D0A0-0x8079D0B0. Strong: the BTA_Dm*
 *   run; every function builds a GKI message and posts it through bta_sys_sendmsg. BTA_DmSendHciReset (event 0x10C)
 *   is the last poster before bta_dm_sm_execute. The .sdata2 holds the two tBTA_SYS_REG records only
 *   BTA_EnableBluetooth reads (bta_dm_sm_execute / bta_dm_search_sm_execute with no disable function).
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`; BTE/bta_dm_act.c's header has the compiler evidence.
 * NAMES. Broadcom's bta_dm_api.c names. GUESS BTA_DisableBluetooth (0x8047D718) is marked in BTE/gki_time.c's header,
 *   where it was named. GUESS bta_dm_reg / bta_dm_search_reg for the .sdata2 records (.sdata2 0x8079D0A0 /
 *   0x8079D0A8).
 * RESIDUALS. BTA_DmSearch keeps
 *   BTE/gki_time.h's BtaDmSearchCallback signature and casts it to the device manager's tBTA_DM_SEARCH_CBACK.
 * SHAPES. plain C; the messages are the tBTA_DM_* records of BTE/bta_dm_int.h, sized by GKI_getbuf.
 */

#include "BTE/bta_dm_api.h"
#include "BTE/bta_dm_act.h"
#include "BTE/bta_dm_main.h"
#include "BTE/btm_devctl.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTA subsystem ids of the device manager and its search. */
#define BTA_ID_DM 1
#define BTA_ID_DM_SEARCH 2

/* The BTA status values. */
#define BTA_SUCCESS 0
#define BTA_FAILURE 1

static const tBTA_SYS_REG bta_dm_reg = {bta_dm_sm_execute, NULL};
static const tBTA_SYS_REG bta_dm_search_reg = {bta_dm_search_sm_execute, NULL};

/* 0x8047D690 (0x88): Clears the control block, registers the two state machines and posts the enable with `p_cback`. */
u8 BTA_EnableBluetooth(BtaDmSecCallback p_cback) {
    tBTA_DM_API_ENABLE* p_msg;

    memset(&bta_dm_cb, 0, sizeof(bta_dm_cb));

    GKI_sched_lock();
    bta_sys_register(BTA_ID_DM, &bta_dm_reg);
    bta_sys_register(BTA_ID_DM_SEARCH, &bta_dm_search_reg);
    GKI_sched_unlock();

    if ((p_msg = (tBTA_DM_API_ENABLE*)GKI_getbuf(sizeof(tBTA_DM_API_ENABLE))) != NULL) {
        p_msg->hdr.event = BTA_DM_API_ENABLE_EVT;
        p_msg->p_sec_cback = p_cback;
        bta_sys_sendmsg(&p_msg->hdr);
        return BTA_SUCCESS;
    }
    return BTA_FAILURE;
}

/* 0x8047D718 (0x38): Posts the disable. */
void BTA_DisableBluetooth(void) {
    BT_HDR* p_msg;

    if ((p_msg = (BT_HDR*)GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        p_msg->event = BTA_DM_API_DISABLE_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/* 0x8047D750 (0x38): Returns whether the controller is up, asked with the scheduler locked. */
u8 BTA_DmIsDeviceUp(void) {
    u8 status;

    GKI_sched_lock();
    status = BTM_IsDeviceUp();
    GKI_sched_unlock();
    return status;
}

/* 0x8047D788 (0x64): Posts the new local device name. */
void BTA_DmSetDeviceName(const char* p_name) {
    tBTA_DM_API_SET_NAME* p_msg;

    if ((p_msg = (tBTA_DM_API_SET_NAME*)GKI_getbuf(sizeof(tBTA_DM_API_SET_NAME))) != NULL) {
        p_msg->hdr.event = BTA_DM_API_SET_NAME_EVT;
        strncpy(p_msg->name, p_name, 32);
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047D7EC (0x58): Posts the discoverable and connectable modes. */
void BTA_DmSetVisibility(u8 disc_mode, u8 conn_mode) {
    tBTA_DM_API_SET_VISIBILITY* p_msg;

    if ((p_msg = (tBTA_DM_API_SET_VISIBILITY*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
        p_msg->hdr.event = BTA_DM_API_SET_VISIBILITY_EVT;
        p_msg->disc_mode = disc_mode;
        p_msg->conn_mode = conn_mode;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047D844 (0x84): Posts a device search with the inquiry `p_dm_inq` for the services `services`. */
void BTA_DmSearch(const BtaDmInq* p_dm_inq, u32 services, BtaDmSearchCallback p_cback) {
    tBTA_DM_API_SEARCH* p_msg;

    if ((p_msg = (tBTA_DM_API_SEARCH*)GKI_getbuf(sizeof(tBTA_DM_API_SEARCH))) != NULL) {
        p_msg->hdr.event = BTA_DM_API_SEARCH_EVT;
        memcpy(&p_msg->inq_params, p_dm_inq, sizeof(BtaDmInq));
        p_msg->services = services;
        p_msg->p_cback = (tBTA_DM_SEARCH_CBACK)p_cback;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047D8C8 (0x38): Posts the search cancel. */
void BTA_DmSearchCancel(void) {
    BT_HDR* p_msg;

    if ((p_msg = (BT_HDR*)GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        p_msg->event = BTA_DM_API_SEARCH_CANCEL_EVT;
        bta_sys_sendmsg(p_msg);
    }
}

/* 0x8047D900 (0x8C): Posts the answer to a PIN request; the PIN is copied only when it is accepted. */
void BTA_DmPinReply(const u8* bd_addr, u8 accept, u8 pin_len, const u8* p_pin) {
    tBTA_DM_API_PIN_REPLY* p_msg;

    if ((p_msg = (tBTA_DM_API_PIN_REPLY*)GKI_getbuf(sizeof(tBTA_DM_API_PIN_REPLY))) != NULL) {
        p_msg->hdr.event = BTA_DM_API_PIN_REPLY_EVT;
        bdcpy(p_msg->bd_addr, bd_addr);
        p_msg->accept = accept;
        if (accept) {
            p_msg->pin_len = pin_len;
            memcpy(p_msg->p_pin, p_pin, pin_len);
        }
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047D98C (0xF0): Adds the device with its link key and its trusted services to the security database. */
u8 BTA_DmAddDevice(const u8* bd_addr, const u8* link_key, u32 trusted_mask, u8 is_trusted) {
    u8 status;
    u32 trusted_services_mask[2];
    u8 index = 0;

    memset(trusted_services_mask, 0, sizeof(trusted_services_mask));

    if (is_trusted) {
        while (trusted_mask && index < BTA_MAX_SERVICE_ID) {
            if (trusted_mask & (1 << index)) {
                trusted_mask &= ~(1 << index);
                BTM_SEC_SET_SERVICE(trusted_services_mask, bta_service_id_to_btm_srv_id_lkup_tbl[index]);
            }
            index++;
        }
    }

    GKI_sched_lock();
    status = BTM_SecAddDevice(bd_addr, NULL, NULL, NULL, trusted_services_mask, link_key);
    GKI_sched_unlock();

    return status == FALSE;
}

/* 0x8047DA7C (0x48): Removes the device from the security database. */
u8 BTA_DmRemoveDevice(const u8* bd_addr) {
    u8 status;

    GKI_sched_lock();
    status = BTM_SecDeleteDevice(bd_addr);
    GKI_sched_unlock();

    return status == FALSE;
}

/* 0x8047DAC4 (0x38): Posts the controller reset. */
void BTA_DmSendHciReset(void) {
    BT_HDR* p_msg;

    if ((p_msg = (BT_HDR*)GKI_getbuf(sizeof(BT_HDR))) != NULL) {
        p_msg->event = BTA_DM_API_SEND_HCI_RESET_EVT;
        bta_sys_sendmsg(p_msg);
    }
}
