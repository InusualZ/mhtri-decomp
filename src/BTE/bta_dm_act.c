/*
 * BTE/bta_dm_act.c - the BTA device manager actions: enable and disable, bonding and PIN/authorisation replies, the
 *   inquiry and remote-name search, the SDP service discovery, the BTM security and ACL callbacks, link quality,
 *   compression and the resource-manager callback.
 *
 * RANGE. .text 0x8047B354-0x8047D690 (55 functions, 0x233C B); .rodata 0x805735E8-0x80573690; .data
 *   0x80612F08-0x80613048; .bss 0x80739288-0x807392B8; .sdata2 0x8079D098-0x8079D0A0. Strong start: bta_dm_enable is
 *   the first reader of the bta_dm data and Broadcom's first bta_dm_act.c function. Strong end: BTA_EnableBluetooth
 *   opens the API run, and BTE/bta_dm_pm.c's .data starts after an 8-byte pad (0x80613043 -> 0x80613048). Every .data
 *   object (the trace strings 0x80612F08-0x80613043) is read only by this range. The .rodata holds the three const
 *   tables bta_service_id_to_uuid_lkup_tbl, bta_service_id_to_btm_srv_id_lkup_tbl (also read by BTA_DmAddDevice) and
 *   bta_security. The .bss object 0x80739288 (0x2D B) is bta_dm_compress_srvcs, read only by bta_dm_enable and the two
 *   compression callbacks. The .sdata2 pair 0x8079D098 is the attribute list bta_dm_find_services copies to its stack.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`, as the other BTE units: Wii/1.3 `cflags_os` reschedules BTA_DmSetDeviceName,
 *   BTA_DmSearch and BTA_DmPinReply where GC reproduces them.
 * NAMES. Broadcom's bta_dm_act.c names. GUESS: bta_dm_set_dev_name 0x8047B5D0, bta_dm_set_visibility 0x8047B5D8,
 *   bta_dm_search_cmpl 0x8047C054, bta_dm_disc_result 0x8047C06C, bta_dm_free_sdp_db 0x8047C190, bta_dm_queue_search
 *   0x8047C1D4, bta_dm_queue_disc 0x8047C21C, bta_dm_search_clear_queue 0x8047C264, bta_dm_search_cancel_cmpl
 *   0x8047C2A8, bta_dm_search_cancel_notify 0x8047C348, bta_dm_cancel_rmt_name 0x8047C858 (the search action table
 *   order), bta_dm_authorize_cback 0x8047C85C, bta_dm_link_key_request_cback 0x8047CB14 (bta_security slots 0 and 3),
 *   bta_dm_signal_strength 0x8047CC64, bta_dm_signal_strength_timer_cback 0x8047CCA0, bta_dm_disable_conn_down_timer_cback
 *   0x8047D084, bta_dm_rssi_cback 0x8047D0A0, bta_dm_reset_cmpl_cback 0x8047D63C (the empty callback
 *   bta_dm_send_hci_reset passes to BTM_SendHciReset).
 * RESIDUALS. L2CA_RegisterCompression is declared with a void(void) callback (BTE/l2c_api.c's type), so
 *   bta_dm_enable casts its callback. String relocations name the compiler's pool where the map has
 *   lbl_80612F08..lbl_80613008.
 * SHAPES. plain C. The callbacks are static; bta_dm_find_services and bta_dm_discover_next_device are defined after
 *   their first callers, as retail orders them. inq_cmpl walks the inquiry database with a for loop (the
 *   BTM_InqDbNext call sits at the loop bottom), rmt_name with a while that advances first. The three discovery
 *   results take the peer name through Broadcom's static bta_dm_get_remname (inlined, no retail copy), which places
 *   the message and the name in retail's registers; bta_dm_rm_cback reads the configured role into Broadcom's `role`
 *   local, which gives retail's cmplw/ble operand order.
 */

#include "BTE/bta_dm_act.h"
#include "BTE/bta_dm_main.h"
#include "BTE/bta_dm_pm.h"
#include "BTE/btm_sec.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The SDP attributes and the PnP service class the discovery reads. */
#define ATTR_ID_SERVICE_CLASS_ID_LIST 0x0001
#define ATTR_ID_EXT_PNP_VERSION 0x8001
#define UUID_SERVCLASS_PNP_INFORMATION 0x1200
#define LEN_UUID_16 2
#define BTA_DM_SDP_DB_SIZE 250

/* The answers to an authorization request, and the BTA status values of a discovery result. */
#define BTA_DM_AUTH_PERM 0
#define BTA_DM_NOT_AUTH 2
#define BTA_SUCCESS 0
#define BTA_FAILURE 1

/* The BTM inquiry modes the disable leaves the controller in, and the GKI pool of HCI commands. */
#define BTM_NON_DISCOVERABLE 0
#define BTM_NON_CONNECTABLE 0
#define HCI_CMD_POOL_ID 2

/* The L2CAP idle timeouts the device manager sets (0xFFFF keeps the links). */
#define L2CAP_NO_IDLE_TIMEOUT 0xFFFF
#define BTA_DM_LINK_IDLE_TIMEOUT 2

/* The longest page timeout the remote name timer covers. */
#define BTA_DM_MAX_RNR_TIMEOUT 30000

static void bta_dm_disable_timer_cback(TIMER_LIST_ENT* p_tle);
static void bta_dm_disable_conn_down_timer_cback(TIMER_LIST_ENT* p_tle);
static void bta_dm_find_services(u8* bd_addr);
static void bta_dm_discover_next_device(void);
static void bta_dm_sdp_callback(u16 sdp_status);
static void bta_dm_inq_results_cb(tBTM_INQ_INFO* p_inq);
static void bta_dm_inq_cmpl_cb(tBTM_INQUIRY_CMPL* p_result);
static void bta_dm_service_search_remname_cback(u8* bd_addr, u8* dc, u8* bd_name);
static void bta_dm_remname_cback(tBTM_REMOTE_DEV_NAME* p_remote_name);
static void bta_dm_disc_remname_cback(tBTM_REMOTE_DEV_NAME* p_remote_name);
static void bta_dm_search_timer_cback(TIMER_LIST_ENT* p_tle);
static u8 bta_dm_authorize_cback(u8* bd_addr, u8* dev_class, u8* bd_name, char* service_name, u8 service_id,
                                 u8 is_originator);
static u8 bta_dm_pin_cback(u8* bd_addr, u8* dev_class, u8* bd_name);
static u8 bta_dm_link_key_request_cback(u8* bd_addr, u8* key);
static u8 bta_dm_new_link_key_cback(u8* bd_addr, u8* dev_class, u8* bd_name, u8* key, u8 key_type);
static u8 bta_dm_authentication_complete_cback(u8* bd_addr, u8* dev_class, u8* bd_name, int result);
static void bta_dm_local_addr_cback(u8* bd_addr);
static void bta_dm_signal_strength_timer_cback(TIMER_LIST_ENT* p_tle);
static void bta_dm_acl_change_cback(u8* p_bda, u8* dc, u8* bdn, u8* features, u8 is_new);
static void bta_dm_rssi_cback(tBTM_RSSI_RESULTS* p_result);
static void bta_dm_link_quality_cback(tBTM_LINK_QUALITY_RESULTS* p_result);
static u8 bta_dm_l2cap_server_compress_cback(u8* bd_addr, u16 psm, u8 id, u8 direction, u8 pe_type, u8 mem_level,
                                             u8 wbits, u8** memory_p, u32* memory_size);
static void bta_dm_compress_cback(u8 status, u8 id, u8 app_id, u8* peer_addr);
static void bta_dm_rm_cback(u8 status, u8 id, u8 app_id, u8* peer_addr);
static void bta_dm_reset_cmpl_cback(void* p_result); /* untyped: caller-owned payload, BTM's command result */

/* The SDP service class UUID of each BTA service id. */
const u16 bta_service_id_to_uuid_lkup_tbl[BTA_MAX_SERVICE_ID + 1] = {
    0x1200, 0x1101, 0x1103, 0x1111, 0x1102, 0x1108, 0x111E, 0x1105, 0x1106, 0x1109, 0x1110, 0x1104,
    0x1118, 0x111B, 0x1115, 0x1116, 0x1117, 0x112D, 0x110B, 0x110E, 0x1124, 0x1304, 0x112F, 0x0000,
};

/* The BTM security service id of each BTA service id. */
const u32 bta_service_id_to_btm_srv_id_lkup_tbl[BTA_MAX_SERVICE_ID + 1] = {
    0, 1, 3, 11, 2, 12, 29, 6, 7, 9, 10, 4, 22, 35, 25, 27, 26, 40, 37, 39, 32, 37, 44, 0,
};

/* The security callbacks the device manager registers with BTM. */
const tBTM_APPL_INFO bta_security = {
    bta_dm_authorize_cback,
    bta_dm_pin_cback,
    bta_dm_new_link_key_cback,
    bta_dm_link_key_request_cback,
    bta_dm_authentication_complete_cback,
    NULL,
};

/* The services that opened a compressed connection. */
tBTA_DM_SRVCS bta_dm_compress_srvcs[BTA_DM_NUM_COMPRESS_SRVS];

/* 0x8047B354 (0x110): Resets the search and power manager state and registers the device manager with BTM, BTA_SYS
 * and L2CAP. */
void bta_dm_enable(tBTA_DM_MSG* p_data) {
    u8 dev_class[DEV_CLASS_LEN];

    memset(&bta_dm_search_cb, 0, sizeof(bta_dm_search_cb));
    memset(&bta_dm_conn_srvcs, 0, sizeof(bta_dm_conn_srvcs));

    memcpy(dev_class, bta_dm_cfg.dev_class, sizeof(dev_class));
    BTM_SetDeviceClass(dev_class);

    bta_dm_cb.p_sec_cback = p_data->enable.p_sec_cback;
    BTM_SecRegister((tBTM_APPL_INFO*)&bta_security);
    BTM_SetDefaultLinkSuperTout(bta_dm_cfg.link_timeout);
    BTM_WritePageTimeout(bta_dm_cfg.page_timeout);
    BTM_SetDefaultLinkPolicy(bta_dm_cfg.policy_settings);
    BTM_AclRegisterForChanges(bta_dm_acl_change_cback);
    BTM_ReadLocalDeviceAddr(bta_dm_local_addr_cback);

    bta_sys_rm_register(bta_dm_rm_cback);
    bta_sys_compress_register(bta_dm_compress_cback);
    bta_dm_init_pm();

    memset(bta_dm_compress_srvcs, 0, sizeof(bta_dm_compress_srvcs));
    bta_sys_compress_register(bta_dm_compress_cback);
    L2CA_RegisterCompression((void (*)(void))bta_dm_l2cap_server_compress_cback, FALSE);

    WBT_ExtCreateRecord();
}

/* 0x8047B464 (0xB0): Stops the subsystems and the scans, then reports the disable once the links are down. */
void bta_dm_disable(tBTA_DM_MSG* p_data) {
    L2CA_SetIdleTimeoutByBdAddr((u8*)BT_BD_ANY, 0);
    bta_sys_disable();

    BTM_SetDiscoverability(BTM_NON_DISCOVERABLE, 0, 0);
    BTM_SetConnectability(BTM_NON_CONNECTABLE, 0, 0);

    bta_dm_disable_pm();

    if (BTM_GetNumAclLinks() == 0) {
        bta_dm_cb.disable_timer.p_cback = bta_dm_disable_conn_down_timer_cback;
        bta_sys_start_timer(&bta_dm_cb.disable_timer, 0, 1000);
    } else {
        bta_dm_cb.disabling = TRUE;
        bta_dm_cb.disable_timer.p_cback = bta_dm_disable_timer_cback;
        bta_sys_start_timer(&bta_dm_cb.disable_timer, 0, 5000);
    }
}

/* 0x8047B514 (0xBC): Drops the remaining links, or reports the disable when none is left (or they are kept). */
static void bta_dm_disable_timer_cback(TIMER_LIST_ENT* p_tle) {
    u8 i;

    APPL_TRACE_EVENT0(" bta_dm_disable_timer_cback  ");

    if (BTM_GetNumAclLinks() && bta_dm_cb.keep_acl_on_shut_down == FALSE) {
        for (i = 0; i < bta_dm_cb.device_list.count; i++) {
            btm_remove_acl(bta_dm_cb.device_list.peer_device[i].peer_bdaddr);
        }
    } else {
        bta_dm_cb.disabling = FALSE;
        bta_dm_cb.p_sec_cback(BTA_DM_DISABLE_EVT, NULL);
    }
}

/* 0x8047B5D0 (0x8): Sets the local device name. */
void bta_dm_set_dev_name(tBTA_DM_MSG* p_data) {
    BTM_SetLocalDeviceName(p_data->set_name.name);
}

/* 0x8047B5D8 (0x48): Sets the discoverable and connectable modes. */
void bta_dm_set_visibility(tBTA_DM_MSG* p_data) {
    BTM_SetDiscoverability(p_data->set_visibility.disc_mode, 0, 0);
    BTM_SetConnectability(p_data->set_visibility.conn_mode, 0, 0);
}

/* 0x8047B620 (0x70): Bonds with the device, keeping its trusted services. */
void bta_dm_bond(tBTA_DM_MSG* p_data) {
    u32 trusted_services_mask[2];
    u32* p_trusted_mask;

    if ((p_trusted_mask = btm_read_trusted_mask(p_data->bond.bd_addr)) != NULL) {
        memcpy(trusted_services_mask, p_trusted_mask, sizeof(trusted_services_mask));
    } else {
        memset(trusted_services_mask, 0, sizeof(trusted_services_mask));
    }

    BTM_SecBond(p_data->bond.bd_addr, p_data->bond.pin_len, p_data->bond.p_pin, trusted_services_mask);
}

/* 0x8047B690 (0x9C): Answers a PIN request, with the PIN when it is accepted. */
void bta_dm_pin_reply(tBTA_DM_MSG* p_data) {
    u32 trusted_services_mask[2];
    u32* p_trusted_mask;

    if ((p_trusted_mask = btm_read_trusted_mask(p_data->pin_reply.bd_addr)) != NULL) {
        memcpy(trusted_services_mask, p_trusted_mask, sizeof(trusted_services_mask));
    } else {
        memset(trusted_services_mask, 0, sizeof(trusted_services_mask));
    }

    if (p_data->pin_reply.accept) {
        BTM_PINCodeReply(p_data->pin_reply.bd_addr, BTM_SUCCESS, p_data->pin_reply.pin_len, p_data->pin_reply.p_pin,
                         trusted_services_mask);
    } else {
        BTM_PINCodeReply(p_data->pin_reply.bd_addr, BTM_NOT_AUTHORIZED, 0, NULL, trusted_services_mask);
    }
}

/* 0x8047B72C (0xD4): Answers an authorization request; a permanent grant trusts the service. */
void bta_dm_auth_reply(tBTA_DM_MSG* p_data) {
    u32 trusted_services_mask[2];
    u32* p_trusted_mask;

    if ((p_trusted_mask = btm_read_trusted_mask(p_data->auth_reply.bd_addr)) != NULL) {
        memcpy(trusted_services_mask, p_trusted_mask, sizeof(trusted_services_mask));
    } else {
        memset(trusted_services_mask, 0, sizeof(trusted_services_mask));
    }

    if (p_data->auth_reply.response != BTA_DM_NOT_AUTH) {
        if (p_data->auth_reply.response == BTA_DM_AUTH_PERM) {
            if (p_data->auth_reply.service <= BTA_MAX_SERVICE_ID) {
                BTM_SEC_SET_SERVICE(trusted_services_mask,
                                    bta_service_id_to_btm_srv_id_lkup_tbl[p_data->auth_reply.service]);
            }
        }
        BTM_DeviceAuthorized(p_data->auth_reply.bd_addr, BTM_SUCCESS, trusted_services_mask);
    } else {
        BTM_DeviceAuthorized(p_data->auth_reply.bd_addr, BTM_NOT_AUTHORIZED, trusted_services_mask);
    }
}

/* 0x8047B800 (0x60): Starts an inquiry for the search. */
void bta_dm_search_start(tBTA_DM_MSG* p_data) {
    BTM_ClearInqDb(NULL);

    bta_dm_search_cb.p_search_cback = p_data->search.p_cback;
    bta_dm_search_cb.services = p_data->search.services;

    BTM_StartInquiry((tBTM_INQ_PARMS*)&p_data->search.inq_params, bta_dm_inq_results_cb,
                     (BtmCompleteCallback)bta_dm_inq_cmpl_cb);
}

/* 0x8047B860 (0x84): Cancels the running inquiry, or the remote name request of a name-only search. */
void bta_dm_search_cancel(tBTA_DM_MSG* p_data) {
    tBTA_DM_MSG* p_msg;

    BTM_ClearInqDb(NULL);

    if (BTM_IsInquiryActive()) {
        BTM_CancelInquiry();
        bta_dm_search_cb.p_search_cback(BTA_DM_SEARCH_CANCEL_CMPL_EVT, NULL);
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_SEARCH_CMPL_EVT;
            bta_sys_sendmsg(&p_msg->hdr);
        }
    } else if (!bta_dm_search_cb.services) {
        BTM_CancelRemoteDeviceName();
    }
}

/* 0x8047B8E4 (0x114): Discovers the services of one device, or reads its name when no service is asked for. */
void bta_dm_discover(tBTA_DM_MSG* p_data) {
    tBTA_DM_MSG* p_msg;

    bta_dm_search_cb.p_search_cback = p_data->discover.p_cback;
    bta_dm_search_cb.services = p_data->discover.services;
    bta_dm_search_cb.services_to_search = bta_dm_search_cb.services;
    bta_dm_search_cb.service_index = 0;
    bta_dm_search_cb.services_found = 0;
    bta_dm_search_cb.peer_name[0] = 0;

    BTM_SecAddRmtNameNotifyCallback(bta_dm_service_search_remname_cback);
    bdcpy(bta_dm_search_cb.peer_bdaddr, p_data->discover.bd_addr);

    if (bta_dm_search_cb.services) {
        bta_dm_find_services(bta_dm_search_cb.peer_bdaddr);
        return;
    }

    if (btm_cb.page_timeout < BTA_DM_MAX_RNR_TIMEOUT) {
        bta_sys_start_timer(&bta_dm_search_cb.rnr_timer, BTA_DM_REMT_NAME_TOUT_EVT, btm_cb.page_timeout + 100);
    }

    if (BTM_ReadRemoteDeviceName(bta_dm_search_cb.peer_bdaddr, bta_dm_disc_remname_cback) != BTM_CMD_STARTED) {
        bta_sys_stop_timer(&bta_dm_search_cb.rnr_timer);
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            bdcpy(p_msg->rem_name.result.disc_res.bd_addr, bta_dm_search_cb.peer_bdaddr);
            p_msg->rem_name.result.disc_res.bd_name[0] = 0;
            p_msg->hdr.event = BTA_DM_REMT_NAME_EVT;
            bta_sys_sendmsg(&p_msg->hdr);
        }
    }
}

/* 0x8047B9F8 (0x224): Starts the service discovery of the first device found, or reports the devices whose names
 * the inquiry gave and reads the first unknown name; then reports the inquiry complete. */
void bta_dm_inq_cmpl(tBTA_DM_MSG* p_data) {
    u8* p_bd_addr;
    tBTA_DM_MSG* p_msg;
    tBTA_DM_SEARCH data;
    tBTA_DM_SEARCH result;
    u8 search_done = TRUE;

    if ((bta_dm_search_cb.p_btm_inq_info = BTM_InqDbFirst()) != NULL) {
        search_done = FALSE;
        if (bta_dm_search_cb.services) {
            if (BTM_IsAclConnectionUp(bta_dm_search_cb.p_btm_inq_info->remote_bd_addr)) {
                bta_dm_search_cb.wait_disc = FALSE;
            } else {
                bta_dm_search_cb.wait_disc = TRUE;
            }
            bta_dm_search_cb.sdp_results = FALSE;
            BTM_SecAddRmtNameNotifyCallback(bta_dm_service_search_remname_cback);
            bta_dm_search_cb.service_index = 0;
            bta_dm_search_cb.services_found = 0;
            bta_dm_search_cb.services_to_search = bta_dm_search_cb.services;
            bta_dm_search_cb.peer_name[0] = 0;
            bdcpy(bta_dm_search_cb.peer_bdaddr, bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
            bta_dm_find_services(bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
        } else {
            search_done = TRUE;
            for (; bta_dm_search_cb.p_btm_inq_info != NULL;
                 bta_dm_search_cb.p_btm_inq_info = BTM_InqDbNext(bta_dm_search_cb.p_btm_inq_info)) {
                if (bta_dm_search_cb.p_btm_inq_info->appl_knows_rem_name) {
                    bdcpy(result.disc_res.bd_addr, bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
                    result.disc_res.bd_name[0] = 0;
                    result.disc_res.services = 0;
                    bta_dm_search_cb.p_search_cback(BTA_DM_DISC_RES_EVT, &result);
                } else {
                    if (btm_cb.page_timeout < BTA_DM_MAX_RNR_TIMEOUT) {
                        bta_sys_start_timer(&bta_dm_search_cb.rnr_timer, BTA_DM_REMT_NAME_TOUT_EVT,
                                            btm_cb.page_timeout + 100);
                    }
                    if (BTM_ReadRemoteDeviceName(bta_dm_search_cb.p_btm_inq_info->remote_bd_addr,
                                                 bta_dm_remname_cback) != BTM_CMD_STARTED) {
                        bta_sys_stop_timer(&bta_dm_search_cb.rnr_timer);
                        p_bd_addr = bta_dm_search_cb.p_btm_inq_info->remote_bd_addr;
                        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
                            bdcpy(p_msg->rem_name.result.disc_res.bd_addr, p_bd_addr);
                            p_msg->rem_name.result.disc_res.bd_name[0] = 0;
                            p_msg->hdr.event = BTA_DM_REMT_NAME_EVT;
                            bta_sys_sendmsg(&p_msg->hdr);
                        }
                    }
                    search_done = FALSE;
                    break;
                }
            }
        }
    }

    if (search_done) {
        bta_dm_search_cb.services = 0;
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_SEARCH_CMPL_EVT;
            bta_sys_sendmsg(&p_msg->hdr);
        }
    }

    data.inq_cmpl.num_resps = p_data->inq_cmpl.num;
    bta_dm_search_cb.p_search_cback(BTA_DM_INQ_CMPL_EVT, &data);
}

/* 0x8047BC1C (0x16C): Reports the devices after the named one whose names are known, reads the next unknown name,
 * then reports the name read. */
void bta_dm_rmt_name(tBTA_DM_MSG* p_data) {
    u8* p_bd_addr;
    tBTA_DM_MSG* p_msg;
    tBTA_DM_SEARCH result;
    u8 search_done = TRUE;

    while ((bta_dm_search_cb.p_btm_inq_info = BTM_InqDbNext(bta_dm_search_cb.p_btm_inq_info)) != NULL) {
        if (bta_dm_search_cb.p_btm_inq_info->appl_knows_rem_name) {
            bdcpy(result.disc_res.bd_addr, bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
            result.disc_res.bd_name[0] = 0;
            result.disc_res.services = 0;
            bta_dm_search_cb.p_search_cback(BTA_DM_DISC_RES_EVT, &result);
        } else {
            if (btm_cb.page_timeout < BTA_DM_MAX_RNR_TIMEOUT) {
                bta_sys_start_timer(&bta_dm_search_cb.rnr_timer, BTA_DM_REMT_NAME_TOUT_EVT, btm_cb.page_timeout + 100);
            }
            if (BTM_ReadRemoteDeviceName(bta_dm_search_cb.p_btm_inq_info->remote_bd_addr,
                                         bta_dm_remname_cback) != BTM_CMD_STARTED) {
                bta_sys_stop_timer(&bta_dm_search_cb.rnr_timer);
                p_bd_addr = bta_dm_search_cb.p_btm_inq_info->remote_bd_addr;
                if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
                    bdcpy(p_msg->rem_name.result.disc_res.bd_addr, p_bd_addr);
                    p_msg->rem_name.result.disc_res.bd_name[0] = 0;
                    p_msg->hdr.event = BTA_DM_REMT_NAME_EVT;
                    bta_sys_sendmsg(&p_msg->hdr);
                }
            }
            search_done = FALSE;
            break;
        }
    }

    if (search_done) {
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_SEARCH_CMPL_EVT;
            bta_sys_sendmsg(&p_msg->hdr);
        }
    }

    bta_dm_search_cb.p_search_cback(BTA_DM_DISC_RES_EVT, &p_data->rem_name.result);
}

/* 0x8047BD88 (0x64): Ends the name-only discovery and reports the name read. */
void bta_dm_disc_rmt_name(tBTA_DM_MSG* p_data) {
    tBTA_DM_MSG* p_msg;

    if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
        p_msg->hdr.event = BTA_DM_SEARCH_CMPL_EVT;
        bta_sys_sendmsg(&p_msg->hdr);
    }

    p_data->rem_name.result.disc_res.services = 0;
    bta_dm_search_cb.p_search_cback(BTA_DM_DISC_RES_EVT, &p_data->rem_name.result);
}

/* Returns the peer's stored name, or the name BTM holds for it when none is stored. */
static char* bta_dm_get_remname(void) {
    char* p_name = bta_dm_search_cb.peer_name;
    char* p_temp;

    if (*p_name == '\0') {
        if ((p_temp = BTM_SecReadDevName(bta_dm_search_cb.peer_bdaddr)) != NULL) {
            p_name = p_temp;
        }
    }

    return p_name;
}

/* 0x8047BDEC (0x268): Records the service found by the last SDP search and searches the next one, or posts the
 * discovery result of the device. */
void bta_dm_sdp_result(tBTA_DM_MSG* p_data) {
    tSDP_DISC_REC* p_sdp_rec = NULL;
    tBTA_DM_MSG* p_msg;
    u8 service_found = FALSE;
    u16 service;

    if (p_data->sdp_event.sdp_result == SDP_SUCCESS || p_data->sdp_event.sdp_result == SDP_NO_RECS_MATCH ||
        p_data->sdp_event.sdp_result == SDP_DB_FULL) {
        service = bta_service_id_to_uuid_lkup_tbl[bta_dm_search_cb.service_index - 1];
        if (p_data->sdp_event.sdp_result == SDP_DB_FULL ||
            (p_sdp_rec = SDP_FindServiceInDb(bta_dm_search_cb.p_sdp_db, service, NULL)) != NULL) {
            if (service == UUID_SERVCLASS_PNP_INFORMATION) {
                if (p_sdp_rec && SDP_FindAttributeInRec(p_sdp_rec, ATTR_ID_EXT_PNP_VERSION)) {
                    service_found = TRUE;
                }
            } else {
                service_found = TRUE;
            }

            if (service_found) {
                bta_dm_search_cb.services_found |= 1 << (bta_dm_search_cb.service_index - 1);
            }
        }

        GKI_freebuf(bta_dm_search_cb.p_sdp_db);
        bta_dm_search_cb.p_sdp_db = NULL;

        if (bta_dm_search_cb.services_to_search) {
            bta_dm_find_services(bta_dm_search_cb.peer_bdaddr);
            return;
        }

        BTM_SecDeleteRmtNameNotifyCallback(bta_dm_service_search_remname_cback);
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_DISCOVERY_RESULT_EVT;
            p_msg->disc_result.result.disc_res.result = BTA_SUCCESS;
            p_msg->disc_result.result.disc_res.services = bta_dm_search_cb.services_found;
            bdcpy(p_msg->disc_result.result.disc_res.bd_addr, bta_dm_search_cb.peer_bdaddr);
            strncpy(p_msg->disc_result.result.disc_res.bd_name, bta_dm_get_remname(), 32);
            bta_sys_sendmsg(&p_msg->hdr);
        }
    } else {
        if (p_data->sdp_event.sdp_result == SDP_CONN_FAILED || p_data->sdp_event.sdp_result == SDP_SECURITY_ERR ||
            p_data->sdp_event.sdp_result == SDP_CONN_REJECTED) {
            bta_dm_search_cb.wait_disc = FALSE;
        }

        GKI_freebuf(bta_dm_search_cb.p_sdp_db);
        bta_dm_search_cb.p_sdp_db = NULL;

        BTM_SecDeleteRmtNameNotifyCallback(bta_dm_service_search_remname_cback);
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_DISCOVERY_RESULT_EVT;
            p_msg->disc_result.result.disc_res.result = BTA_FAILURE;
            p_msg->disc_result.result.disc_res.services = bta_dm_search_cb.services_found;
            bdcpy(p_msg->disc_result.result.disc_res.bd_addr, bta_dm_search_cb.peer_bdaddr);
            strncpy(p_msg->disc_result.result.disc_res.bd_name, bta_dm_get_remname(), 32);
            bta_sys_sendmsg(&p_msg->hdr);
        }
    }
}

/* 0x8047C054 (0x18): Reports the discovery complete. */
void bta_dm_search_cmpl(tBTA_DM_MSG* p_data) {
    bta_dm_search_cb.p_search_cback(BTA_DM_DISC_CMPL_EVT, NULL);
}

/* 0x8047C06C (0x50): Reports the discovery result of the device and ends the discovery. */
void bta_dm_disc_result(tBTA_DM_MSG* p_data) {
    tBTA_DM_MSG* p_msg;

    bta_dm_search_cb.p_search_cback(BTA_DM_DISC_RES_EVT, &p_data->disc_result.result);

    if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
        p_msg->hdr.event = BTA_DM_SEARCH_CMPL_EVT;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047C0BC (0x88): Reports a device that has services, then discovers the next device once its link is down. */
void bta_dm_search_result(tBTA_DM_MSG* p_data) {
    if (p_data->disc_result.result.disc_res.services) {
        bta_dm_search_cb.p_search_cback(BTA_DM_DISC_RES_EVT, &p_data->disc_result.result);
    }

    if (!bta_dm_search_cb.wait_disc) {
        bta_dm_search_cb.wait_disc = FALSE;
        bta_dm_discover_next_device();
    } else {
        bta_dm_search_cb.sdp_results = TRUE;
        bta_dm_search_cb.search_timer.p_cback = bta_dm_search_timer_cback;
        bta_sys_start_timer(&bta_dm_search_cb.search_timer, 0, 3000);
    }
}

/* 0x8047C144 (0x4C): Stops waiting for the link to go down and discovers the next device. */
static void bta_dm_search_timer_cback(TIMER_LIST_ENT* p_tle) {
    APPL_TRACE_EVENT0(" bta_dm_search_timer_cback  ");
    bta_dm_search_cb.wait_disc = FALSE;
    bta_dm_discover_next_device();
}

/* 0x8047C190 (0x44): Frees the SDP database. */
void bta_dm_free_sdp_db(tBTA_DM_MSG* p_data) {
    if (bta_dm_search_cb.p_sdp_db) {
        GKI_freebuf(bta_dm_search_cb.p_sdp_db);
        bta_dm_search_cb.p_sdp_db = NULL;
    }
}

/* 0x8047C1D4 (0x48): Keeps a search request until the cancel completes. */
void bta_dm_queue_search(tBTA_DM_MSG* p_data) {
    bta_dm_search_cb.p_search_queue = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_API_SEARCH));
    memcpy(bta_dm_search_cb.p_search_queue, p_data, sizeof(tBTA_DM_API_SEARCH));
}

/* 0x8047C21C (0x48): Keeps a discovery request until the cancel completes. */
void bta_dm_queue_disc(tBTA_DM_MSG* p_data) {
    bta_dm_search_cb.p_search_queue = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_API_DISCOVER));
    memcpy(bta_dm_search_cb.p_search_queue, p_data, sizeof(tBTA_DM_API_DISCOVER));
}

/* 0x8047C264 (0x44): Drops the queued request. */
void bta_dm_search_clear_queue(tBTA_DM_MSG* p_data) {
    if (bta_dm_search_cb.p_search_queue) {
        GKI_freebuf(bta_dm_search_cb.p_search_queue);
        bta_dm_search_cb.p_search_queue = NULL;
    }
}

/* 0x8047C2A8 (0x44): Posts the queued request now that the cancel completed. */
void bta_dm_search_cancel_cmpl(tBTA_DM_MSG* p_data) {
    if (bta_dm_search_cb.p_search_queue) {
        bta_sys_sendmsg(&bta_dm_search_cb.p_search_queue->hdr);
        bta_dm_search_cb.p_search_queue = NULL;
    }
}

/* 0x8047C2EC (0x5C): Frees the SDP database of the cancelled discovery and reports the cancel complete. */
void bta_dm_search_cancel_transac_cmpl(tBTA_DM_MSG* p_data) {
    if (bta_dm_search_cb.p_sdp_db) {
        GKI_freebuf(bta_dm_search_cb.p_sdp_db);
        bta_dm_search_cb.p_sdp_db = NULL;
    }

    bta_dm_search_cb.p_search_cback(BTA_DM_SEARCH_CANCEL_CMPL_EVT, NULL);
}

/* 0x8047C348 (0x18): Reports the cancel complete. */
void bta_dm_search_cancel_notify(tBTA_DM_MSG* p_data) {
    bta_dm_search_cb.p_search_cback(BTA_DM_SEARCH_CANCEL_CMPL_EVT, NULL);
}

/* 0x8047C360 (0x1A0): Starts the SDP search of the next service still to search on `bd_addr`, or posts the
 * discovery result when none is left. */
static void bta_dm_find_services(u8* bd_addr) {
    tSDP_UUID uuid;
    u16 attr_list[] = {ATTR_ID_SERVICE_CLASS_ID_LIST, ATTR_ID_EXT_PNP_VERSION};
    u16 num_attrs = 1;
    tBTA_DM_MSG* p_msg;

    while (bta_dm_search_cb.service_index < BTA_MAX_SERVICE_ID) {
        if (bta_dm_search_cb.services_to_search & (1 << bta_dm_search_cb.service_index)) {
            if ((bta_dm_search_cb.p_sdp_db = (tSDP_DISCOVERY_DB*)GKI_getbuf(BTA_DM_SDP_DB_SIZE)) != NULL) {
                bta_dm_search_cb.services_to_search &= ~(1 << bta_dm_search_cb.service_index);

                uuid.len = LEN_UUID_16;
                uuid.uu.uuid16 = bta_service_id_to_uuid_lkup_tbl[bta_dm_search_cb.service_index];
                if (uuid.uu.uuid16 == UUID_SERVCLASS_PNP_INFORMATION) {
                    num_attrs = 2;
                }

                SDP_InitDiscoveryDb(bta_dm_search_cb.p_sdp_db, BTA_DM_SDP_DB_SIZE, 1, &uuid, num_attrs, attr_list);

                if (!SDP_ServiceSearchAttributeRequest(bd_addr, bta_dm_search_cb.p_sdp_db, bta_dm_sdp_callback)) {
                    GKI_freebuf(bta_dm_search_cb.p_sdp_db);
                    bta_dm_search_cb.p_sdp_db = NULL;
                    bta_dm_search_cb.service_index = BTA_MAX_SERVICE_ID;
                } else {
                    bta_dm_search_cb.service_index++;
                    return;
                }
            }
        }
        bta_dm_search_cb.service_index++;
    }

    if (bta_dm_search_cb.service_index >= BTA_MAX_SERVICE_ID) {
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_DISCOVERY_RESULT_EVT;
            p_msg->disc_result.result.disc_res.services = bta_dm_search_cb.services_found;
            bdcpy(p_msg->disc_result.result.disc_res.bd_addr, bta_dm_search_cb.peer_bdaddr);
            strncpy(p_msg->disc_result.result.disc_res.bd_name, bta_dm_get_remname(), 32);
            bta_sys_sendmsg(&p_msg->hdr);
        }
    }
}

/* 0x8047C500 (0xE0): Starts the service discovery of the next device of the inquiry, or ends the search. */
static void bta_dm_discover_next_device(void) {
    tBTA_DM_MSG* p_msg;

    if ((bta_dm_search_cb.p_btm_inq_info = BTM_InqDbNext(bta_dm_search_cb.p_btm_inq_info)) != NULL) {
        if (BTM_IsAclConnectionUp(bta_dm_search_cb.p_btm_inq_info->remote_bd_addr)) {
            bta_dm_search_cb.wait_disc = FALSE;
        } else {
            bta_dm_search_cb.wait_disc = TRUE;
        }
        bta_dm_search_cb.sdp_results = FALSE;
        BTM_SecAddRmtNameNotifyCallback(bta_dm_service_search_remname_cback);
        bta_dm_search_cb.service_index = 0;
        bta_dm_search_cb.services_found = 0;
        bta_dm_search_cb.peer_name[0] = 0;
        bta_dm_search_cb.services_to_search = bta_dm_search_cb.services;
        bdcpy(bta_dm_search_cb.peer_bdaddr, bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
        bta_dm_find_services(bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
    } else {
        bta_dm_search_cb.services = 0;
        if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
            p_msg->hdr.event = BTA_DM_SEARCH_CMPL_EVT;
            bta_sys_sendmsg(&p_msg->hdr);
        }
    }
}

/* 0x8047C5E0 (0x48): Posts the result of an SDP search. */
static void bta_dm_sdp_callback(u16 sdp_status) {
    tBTA_DM_SDP_RESULT* p_msg;

    if ((p_msg = (tBTA_DM_SDP_RESULT*)GKI_getbuf(sizeof(tBTA_DM_SDP_RESULT))) != NULL) {
        p_msg->hdr.event = BTA_DM_SDP_RESULT_EVT;
        p_msg->sdp_result = sdp_status;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047C628 (0x90): Reports an inquiry response; a known device's name need not be read again. */
static void bta_dm_inq_results_cb(tBTM_INQ_INFO* p_inq) {
    tBTA_DM_SEARCH result;
    tBTM_INQ_INFO* p_inq_info;

    bdcpy(result.inq_res.bd_addr, p_inq->remote_bd_addr);
    memcpy(result.inq_res.dev_class, p_inq->dev_class, DEV_CLASS_LEN);
    result.inq_res.rssi = p_inq->rssi;

    if ((p_inq_info = BTM_InqDbRead(p_inq->remote_bd_addr)) != NULL) {
        result.inq_res.remt_name_not_required = FALSE;
    }

    bta_dm_search_cb.p_search_cback(BTA_DM_INQ_RES_EVT, &result);

    if (p_inq_info) {
        p_inq_info->appl_knows_rem_name = result.inq_res.remt_name_not_required;
    }
}

/* 0x8047C6B8 (0x4C): Posts the end of the inquiry with its response count. */
static void bta_dm_inq_cmpl_cb(tBTM_INQUIRY_CMPL* p_result) {
    tBTA_DM_MSG* p_msg;

    if ((p_msg = (tBTA_DM_MSG*)GKI_getbuf(sizeof(tBTA_DM_MSG))) != NULL) {
        p_msg->hdr.event = BTA_DM_INQUIRY_CMPL_EVT;
        p_msg->inq_cmpl.num = p_result->num_resp;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047C704 (0x40): Keeps the name BTM read for the device being discovered. */
static void bta_dm_service_search_remname_cback(u8* bd_addr, u8* dc, u8* bd_name) {
    strncpy(bta_dm_search_cb.peer_name, (char*)bd_name, 31);
    BTM_SecDeleteRmtNameNotifyCallback(bta_dm_service_search_remname_cback);
}

/* 0x8047C744 (0x8C): Posts the name read for the current inquiry device. */
static void bta_dm_remname_cback(tBTM_REMOTE_DEV_NAME* p_remote_name) {
    tBTA_DM_REM_NAME* p_msg;

    bta_sys_stop_timer(&bta_dm_search_cb.rnr_timer);

    if ((p_msg = (tBTA_DM_REM_NAME*)GKI_getbuf(sizeof(tBTA_DM_REM_NAME))) != NULL) {
        bdcpy(p_msg->result.disc_res.bd_addr, bta_dm_search_cb.p_btm_inq_info->remote_bd_addr);
        strncpy(p_msg->result.disc_res.bd_name, (char*)p_remote_name->remote_bd_name, 32);
        p_msg->hdr.event = BTA_DM_REMT_NAME_EVT;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047C7D0 (0x88): Posts the name read for the device being discovered. */
static void bta_dm_disc_remname_cback(tBTM_REMOTE_DEV_NAME* p_remote_name) {
    tBTA_DM_REM_NAME* p_msg;

    bta_sys_stop_timer(&bta_dm_search_cb.rnr_timer);

    if ((p_msg = (tBTA_DM_REM_NAME*)GKI_getbuf(sizeof(tBTA_DM_REM_NAME))) != NULL) {
        bdcpy(p_msg->result.disc_res.bd_addr, bta_dm_search_cb.peer_bdaddr);
        strncpy(p_msg->result.disc_res.bd_name, (char*)p_remote_name->remote_bd_name, 32);
        p_msg->hdr.event = BTA_DM_REMT_NAME_EVT;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047C858 (0x4): Cancels the remote name request whose timer expired. */
void bta_dm_cancel_rmt_name(tBTA_DM_MSG* p_data) {
    BTM_CancelRemoteDeviceName();
}

/* 0x8047C85C (0xC8): Asks the application to authorize the BTM service `service_id` for the device. */
static u8 bta_dm_authorize_cback(u8* bd_addr, u8* dev_class, u8* bd_name, char* service_name, u8 service_id,
                                 u8 is_originator) {
    tBTA_DM_SEC sec_event;
    u8 index = 1;

    bdcpy(sec_event.authorize.bd_addr, bd_addr);
    strncpy(sec_event.authorize.bd_name, (char*)bd_name, 32);

    for (; index <= BTA_MAX_SERVICE_ID; index++) {
        if (bta_service_id_to_btm_srv_id_lkup_tbl[index] == service_id) {
            sec_event.authorize.service = index;
            break;
        }
    }

    if (bta_dm_cb.p_sec_cback && index <= BTA_MAX_SERVICE_ID) {
        bta_dm_cb.p_sec_cback(BTA_DM_AUTHORIZE_EVT, &sec_event);
        return BTM_CMD_STARTED;
    }
    return BTM_NOT_AUTHORIZED;
}

/* 0x8047C924 (0xC0): Reports the PIN request of the pending device with the name just read. */
static void bta_dm_pinname_cback(tBTM_REMOTE_DEV_NAME* p_data) {
    tBTA_DM_SEC sec_event;

    bdcpy(sec_event.pin_req.bd_addr, bta_dm_cb.pin_bd_addr);
    sec_event.pin_req.dev_class[0] = bta_dm_cb.pin_dev_class[0];
    sec_event.pin_req.dev_class[1] = bta_dm_cb.pin_dev_class[1];
    sec_event.pin_req.dev_class[2] = bta_dm_cb.pin_dev_class[2];

    if (p_data && p_data->status == BTM_SUCCESS) {
        memcpy(sec_event.pin_req.bd_name, p_data->remote_bd_name, (p_data->length < 32) ? p_data->length : 32);
        sec_event.pin_req.bd_name[32] = 0;
    } else {
        sec_event.pin_req.bd_name[0] = 0;
    }

    bta_dm_cb.p_sec_cback(BTA_DM_PIN_REQ_EVT, &sec_event);
}

/* 0x8047C9E4 (0x130): Reports a PIN request; reads the device name first when BTM has none. */
static u8 bta_dm_pin_cback(u8* bd_addr, u8* dev_class, u8* bd_name) {
    tBTA_DM_SEC sec_event;

    if (!bta_dm_cb.p_sec_cback) {
        return BTM_NOT_AUTHORIZED;
    }

    if (bd_name[0] == 0) {
        bdcpy(bta_dm_cb.pin_bd_addr, bd_addr);
        bta_dm_cb.pin_dev_class[0] = dev_class[0];
        bta_dm_cb.pin_dev_class[1] = dev_class[1];
        bta_dm_cb.pin_dev_class[2] = dev_class[2];
        if (BTM_ReadRemoteDeviceName(bd_addr, bta_dm_pinname_cback) == BTM_CMD_STARTED) {
            return BTM_CMD_STARTED;
        }
        APPL_TRACE_WARNING0(" bta_dm_pin_cback() -> Failed to start Remote Name Request  ");
    }

    bdcpy(sec_event.pin_req.bd_addr, bd_addr);
    sec_event.pin_req.dev_class[0] = dev_class[0];
    sec_event.pin_req.dev_class[1] = dev_class[1];
    sec_event.pin_req.dev_class[2] = dev_class[2];
    strncpy(sec_event.pin_req.bd_name, (char*)bd_name, 32);
    sec_event.pin_req.bd_name[32] = 0;

    bta_dm_cb.p_sec_cback(BTA_DM_PIN_REQ_EVT, &sec_event);
    return BTM_CMD_STARTED;
}

/* 0x8047CB14 (0x8): Refuses the link key request; the application stores no keys here. */
static u8 bta_dm_link_key_request_cback(u8* bd_addr, u8* key) {
    return BTM_NOT_AUTHORIZED;
}

/* 0x8047CB1C (0x9C): Reports a completed authentication with its new link key. */
static u8 bta_dm_new_link_key_cback(u8* bd_addr, u8* dev_class, u8* bd_name, u8* key, u8 key_type) {
    tBTA_DM_SEC sec_event;

    bdcpy(sec_event.auth_cmpl.bd_addr, bd_addr);
    memcpy(sec_event.auth_cmpl.bd_name, bd_name, 31);
    sec_event.auth_cmpl.bd_name[31] = 0;
    memcpy(sec_event.auth_cmpl.key, key, LINK_KEY_LEN);
    sec_event.auth_cmpl.key_present = TRUE;
    sec_event.auth_cmpl.success = TRUE;

    if (bta_dm_cb.p_sec_cback) {
        bta_dm_cb.p_sec_cback(BTA_DM_AUTH_CMPL_EVT, &sec_event);
    }
    return BTM_CMD_STARTED;
}

/* 0x8047CBB8 (0x84): Reports a failed authentication. */
static u8 bta_dm_authentication_complete_cback(u8* bd_addr, u8* dev_class, u8* bd_name, int result) {
    tBTA_DM_SEC sec_event;

    if (result != BTM_SUCCESS) {
        bdcpy(sec_event.auth_cmpl.bd_addr, bd_addr);
        memcpy(sec_event.auth_cmpl.bd_name, bd_name, 31);
        sec_event.auth_cmpl.bd_name[31] = 0;
        sec_event.auth_cmpl.success = FALSE;
        sec_event.auth_cmpl.key_present = FALSE;
        if (bta_dm_cb.p_sec_cback) {
            bta_dm_cb.p_sec_cback(BTA_DM_AUTH_CMPL_EVT, &sec_event);
        }
    }
    return BTM_SUCCESS;
}

/* 0x8047CC3C (0x28): Reports the device manager enabled with the local address. */
static void bta_dm_local_addr_cback(u8* bd_addr) {
    if (bta_dm_cb.p_sec_cback) {
        bta_dm_cb.p_sec_cback(BTA_DM_ENABLE_EVT, (tBTA_DM_SEC*)bd_addr);
    }
}

/* 0x8047CC64 (0x3C): Starts reading the signal strength of the links periodically, or stops it. */
void bta_dm_signal_strength(tBTA_DM_MSG* p_data) {
    if (p_data->sig_strength.start) {
        bta_dm_cb.signal_strength_mask = p_data->sig_strength.mask;
        bta_dm_cb.signal_strength_period = p_data->sig_strength.period;
        bta_dm_signal_strength_timer_cback(NULL);
    } else {
        bta_sys_stop_timer(&bta_dm_cb.signal_strength_timer);
    }
}

/* 0x8047CCA0 (0xF4): Reads the RSSI and link quality of every link and restarts the period timer. */
static void bta_dm_signal_strength_timer_cback(TIMER_LIST_ENT* p_tle) {
    u8 i;

    if (bta_dm_cb.signal_strength_mask & BTA_SIG_STRENGTH_RSSI_MASK) {
        for (i = 0; i < bta_dm_cb.device_list.count; i++) {
            BTM_ReadRSSI(bta_dm_cb.device_list.peer_device[i].peer_bdaddr, (BtmCompleteCallback)bta_dm_rssi_cback);
        }
    }

    if (bta_dm_cb.signal_strength_mask & BTA_SIG_STRENGTH_LINK_QUALITY_MASK) {
        for (i = 0; i < bta_dm_cb.device_list.count; i++) {
            BTM_ReadLinkQuality(bta_dm_cb.device_list.peer_device[i].peer_bdaddr,
                                (BtmCompleteCallback)bta_dm_link_quality_cback);
        }
    }

    if (bta_dm_cb.signal_strength_period) {
        bta_dm_cb.signal_strength_timer.p_cback = bta_dm_signal_strength_timer_cback;
        bta_sys_start_timer(&bta_dm_cb.signal_strength_timer, 0, bta_dm_cb.signal_strength_period * 1000);
    }
}

/* 0x8047CD94 (0x70): Posts a link up or down. */
static void bta_dm_acl_change_cback(u8* p_bda, u8* dc, u8* bdn, u8* features, u8 is_new) {
    tBTA_DM_ACL_CHANGE* p_msg;

    if ((p_msg = (tBTA_DM_ACL_CHANGE*)GKI_getbuf(sizeof(tBTA_DM_ACL_CHANGE))) != NULL) {
        bdcpy(p_msg->bd_addr, p_bda);
        p_msg->is_new = is_new;
        p_msg->hdr.event = BTA_DM_ACL_CHANGE_EVT;
        bta_sys_sendmsg(&p_msg->hdr);
    }
}

/* 0x8047CE04 (0x280): Adds or removes the device in the link list and reports the link up or down; finishes a
 * pending discovery or disable that waited for the link to go down. */
void bta_dm_acl_change(tBTA_DM_MSG* p_data) {
    u8 i;
    tBTA_DM_SEC conn;
    u8* p_bda = p_data->acl_change.bd_addr;

    if (p_data->acl_change.is_new) {
        for (i = 0; i < bta_dm_cb.device_list.count; i++) {
            if (!bdcmp(bta_dm_cb.device_list.peer_device[i].peer_bdaddr, p_bda)) {
                break;
            }
        }

        if (i == bta_dm_cb.device_list.count) {
            bdcpy(bta_dm_cb.device_list.peer_device[bta_dm_cb.device_list.count].peer_bdaddr, p_bda);
            bta_dm_cb.device_list.count++;
        }

        bta_dm_cb.device_list.peer_device[i].conn_state = BTA_DM_CONNECTED;
        bta_dm_cb.device_list.peer_device[i].pref_role = 0;
        bdcpy(conn.link_up.bd_addr, p_bda);
        bta_dm_cb.p_sec_cback(BTA_DM_LINK_UP_EVT, &conn);
    } else {
        for (i = 0; i < bta_dm_cb.device_list.count; i++) {
            if (bdcmp(bta_dm_cb.device_list.peer_device[i].peer_bdaddr, p_bda)) {
                continue;
            }
            for (; i < bta_dm_cb.device_list.count; i++) {
                memcpy(&bta_dm_cb.device_list.peer_device[i], &bta_dm_cb.device_list.peer_device[i + 1],
                       sizeof(tBTA_DM_PEER_DEVICE));
            }
            break;
        }
        bta_dm_cb.device_list.count--;

        if (bta_dm_search_cb.wait_disc && !bdcmp(bta_dm_search_cb.peer_bdaddr, p_bda)) {
            bta_dm_search_cb.wait_disc = FALSE;
            if (bta_dm_search_cb.sdp_results) {
                APPL_TRACE_EVENT0(" timer stopped  ");
                bta_sys_stop_timer(&bta_dm_search_cb.search_timer);
                bta_dm_discover_next_device();
            }
        }

        if (bta_dm_cb.disabling && !BTM_GetNumAclLinks()) {
            bta_dm_cb.disabling = FALSE;
            bta_sys_stop_timer(&bta_dm_cb.disable_timer);
            bta_dm_cb.disable_timer.p_cback = bta_dm_disable_conn_down_timer_cback;
            bta_sys_start_timer(&bta_dm_cb.disable_timer, 0, 1000);
        }

        bdcpy(conn.link_down.bd_addr, p_bda);
        conn.link_down.status = btm_get_acl_disc_reason_code();
        bta_dm_cb.p_sec_cback(BTA_DM_LINK_DOWN_EVT, &conn);
    }
}

/* 0x8047D084 (0x1C): Reports the device manager disabled once the links went down. */
static void bta_dm_disable_conn_down_timer_cback(TIMER_LIST_ENT* p_tle) {
    bta_dm_cb.p_sec_cback(BTA_DM_DISABLE_EVT, NULL);
}

/* 0x8047D0A0 (0x6C): Reports the RSSI read for a link. */
static void bta_dm_rssi_cback(tBTM_RSSI_RESULTS* p_result) {
    tBTA_DM_SIG_STRENGTH sig_strength;

    if (p_result->status == BTM_SUCCESS) {
        bdcpy(sig_strength.bd_addr, p_result->rem_bda);
        sig_strength.mask = BTA_SIG_STRENGTH_RSSI_MASK;
        sig_strength.rssi_value = p_result->rssi;
        bta_dm_cb.p_sec_cback(BTA_DM_SIG_STRENGTH_EVT, (tBTA_DM_SEC*)&sig_strength);
    }
}

/* 0x8047D10C (0x6C): Reports the link quality read for a link. */
static void bta_dm_link_quality_cback(tBTM_LINK_QUALITY_RESULTS* p_result) {
    tBTA_DM_SIG_STRENGTH sig_strength;

    if (p_result->status == BTM_SUCCESS) {
        bdcpy(sig_strength.bd_addr, p_result->rem_bda);
        sig_strength.mask = BTA_SIG_STRENGTH_LINK_QUALITY_MASK;
        sig_strength.link_quality_value = p_result->link_quality;
        bta_dm_cb.p_sec_cback(BTA_DM_SIG_STRENGTH_EVT, (tBTA_DM_SEC*)&sig_strength);
    }
}

/* 0x8047D178 (0xE4): Asks the application for compression memory for a server link with an open compressed
 * service. */
static u8 bta_dm_l2cap_server_compress_cback(u8* bd_addr, u16 psm, u8 id, u8 direction, u8 pe_type, u8 mem_level,
                                             u8 wbits, u8** memory_p, u32* memory_size) {
    u8 result = FALSE;
    int i;

    for (i = 0; i < BTA_DM_NUM_COMPRESS_SRVS; i++) {
        if (bta_dm_compress_srvcs[i].state == 1 && !bdcmp(bta_dm_compress_srvcs[i].peer_bdaddr, bd_addr)) {
            APPL_TRACE_EVENT1("bta_dm_l2cap_server_compress_cback, BTA ID %d", bta_dm_compress_srvcs[i].id);
            result = bta_dm_co_get_compress_memory(bta_dm_compress_srvcs[i].id, memory_p, memory_size);
            break;
        }
    }
    return result;
}

/* 0x8047D25C (0x238): Records a compressed service opening (unless a connected service excludes compression) or
 * closing. */
static void bta_dm_compress_cback(u8 status, u8 id, u8 app_id, u8* peer_addr) {
    u8 i, j, k;
    u8 found;

    if (status == BTA_SYS_CONN_OPEN) {
        for (i = 1; i <= p_bta_dm_compress_cfg[0].app_id; i++) {
            if ((app_id == p_bta_dm_compress_cfg[i].app_id || p_bta_dm_compress_cfg[i].app_id == BTA_ALL_APP_ID) &&
                id == p_bta_dm_compress_cfg[i].id && p_bta_dm_compress_cfg[i].cfg == 1) {
                found = FALSE;
                for (j = 1; j <= p_bta_dm_compress_cfg[0].app_id; j++) {
                    if (p_bta_dm_compress_cfg[j].cfg == 2) {
                        for (k = 0; k < bta_dm_conn_srvcs.count; k++) {
                            if (bta_dm_conn_srvcs.conn_srvc[k].id == p_bta_dm_compress_cfg[j].id) {
                                found = TRUE;
                            }
                        }
                    }
                }

                if (!found) {
                    bta_dm_compress_srvcs[i - 1].app_id = app_id;
                    bta_dm_compress_srvcs[i - 1].id = id;
                    bdcpy(bta_dm_compress_srvcs[i - 1].peer_bdaddr, peer_addr);
                    bta_dm_compress_srvcs[i - 1].state = 1;
                    APPL_TRACE_EVENT3("bta_dm_compress_cback open app_id %d, BTA id %d, state %d",
                                      bta_dm_compress_srvcs[i - 1].app_id, bta_dm_compress_srvcs[i - 1].id,
                                      bta_dm_compress_srvcs[i - 1].state);
                }
                return;
            }
        }
    } else if (status == BTA_SYS_CONN_CLOSE) {
        for (i = 1; i <= p_bta_dm_compress_cfg[0].app_id; i++) {
            if ((app_id == p_bta_dm_compress_cfg[i].app_id || p_bta_dm_compress_cfg[i].app_id == BTA_ALL_APP_ID) &&
                id == p_bta_dm_compress_cfg[i].id) {
                bta_dm_compress_srvcs[i - 1].state = 0;
                APPL_TRACE_EVENT3("bta_dm_compress_cback close app_id %d, BTA id %d, state %d",
                                  bta_dm_compress_srvcs[i - 1].app_id, bta_dm_compress_srvcs[i - 1].id,
                                  bta_dm_compress_srvcs[i - 1].state);
                return;
            }
        }
    }
}

/* 0x8047D494 (0x108): Marks the device connected and raises its preferred role to the service's configured one. */
static void bta_dm_rm_cback(u8 status, u8 id, u8 app_id, u8* peer_addr) {
    u8 i, j;
    u8 role;

    if (status == BTA_SYS_CONN_OPEN) {
        for (j = 0; j < bta_dm_cb.device_list.count; j++) {
            if (!bdcmp(bta_dm_cb.device_list.peer_device[j].peer_bdaddr, peer_addr)) {
                bta_dm_cb.device_list.peer_device[j].conn_state = BTA_DM_CONNECTED;
                for (i = 1; i <= p_bta_dm_rm_cfg[0].app_id; i++) {
                    if ((app_id == p_bta_dm_rm_cfg[i].app_id || p_bta_dm_rm_cfg[i].app_id == BTA_ALL_APP_ID) &&
                        id == p_bta_dm_rm_cfg[i].id) {
                        role = p_bta_dm_rm_cfg[i].cfg;
                        if (role > bta_dm_cb.device_list.peer_device[j].pref_role) {
                            bta_dm_cb.device_list.peer_device[j].pref_role = role;
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
}

/* 0x8047D59C (0x84): Keeps the links up across the disable, or lets them time out after two seconds. */
void bta_dm_keep_acl(tBTA_DM_MSG* p_data) {
    bta_dm_cb.keep_acl_on_shut_down = p_data->hdr.layer_specific;

    if (bta_dm_cb.keep_acl_on_shut_down) {
        L2CA_SetIdleTimeoutByBdAddr((u8*)BT_BD_ANY, L2CAP_NO_IDLE_TIMEOUT);
        L2CA_SetIdleTimeout(0, L2CAP_NO_IDLE_TIMEOUT, TRUE);
    } else {
        L2CA_SetIdleTimeoutByBdAddr((u8*)BT_BD_ANY, BTA_DM_LINK_IDLE_TIMEOUT);
        L2CA_SetIdleTimeout(0, BTA_DM_LINK_IDLE_TIMEOUT, TRUE);
    }
}

/* Reports the device manager disabled at once. */
void bta_dm_immediate_disable(void) {
    bta_dm_cb.p_sec_cback(BTA_DM_DISABLE_EVT, NULL);
}

/* Ignores the controller's answer to the reset. */
/* untyped: caller-owned payload, BTM's command result */
static void bta_dm_reset_cmpl_cback(void* p_result) {
}

/* 0x8047D640 (0x50): Marks a reset pending, stops the scans and resets the controller. */
void bta_dm_send_hci_reset(tBTA_DM_MSG* p_data) {
    BT_HDR* p_buf;

    bta_sys_cb.events_disabled = TRUE;

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        btsnd_hcic_write_scan_enable(p_buf, 0);
    }

    BTM_SendHciReset(bta_dm_reset_cmpl_cback);
}
