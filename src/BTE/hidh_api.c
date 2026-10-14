/*
 * BTE/hidh_api.c - the HID host API: the SDP record query and its search callback, init, register/deregister,
 *   add/remove/open/write/close device and the security level.
 *
 * RANGE. .text 0x804900D4-0x80490E54 (11 functions, 0xD80 B); .data 0x806155E0-0x806156F8; .bss 0x8073C358-0x8073C760.
 *   Strong: the .data starts after a 4-byte pad beyond 4-byte alignment (0x806155DC -> 0x806155E0) and the BTE
 *   hidh_api.c opens with HID_HostGetSDPRecord and its static hidh_search_callback; it ends with
 *   HID_HostSetSecurityLevel, pinned by the .data (hidh_conn.c's first reader is hidh_conn_reg at 0x80490E54). The .bss
 *   is the HID host control block (0x408 B, read by both hidh units and bta_hh); its owner is a GUESS from the BTE
 *   source (hidh_api.c defines it) - either hidh unit gives the same .bss order.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) six of its eleven rows do not match.
 * NAMES. the map's. GUESS hh_cb (hh_cb, the BTE source's name for the HID host control block).
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_806155E0.
 * SHAPES. plain C; hidh_get_str_attr is inlined into hidh_search_callback, HID_HostCloseDev into HID_HostRemoveDev and
 *   both into HID_HostDeregister.
 */

#include "BTE/hidh_api.h"
#include "BTE/hidh_conn.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

tHID_HOST_CB hh_cb;

/* 0x804900D4 (0xC8): Starts the HID record query of the device at `addr` into `p_db`; returns a HID result. */
u8 HID_HostGetSDPRecord(u8* addr, tSDP_DISCOVERY_DB* p_db, u32 db_len, tHID_HOST_SDP_CALLBACK* sdp_cback) {
    tSDP_UUID uuid_list;

    if (hh_cb.sdp_busy) {
        return HID_ERR_SDP_BUSY;
    }

    uuid_list.len = 2;
    uuid_list.uu.uuid16 = UUID_SERVCLASS_HUMAN_INTERFACE;

    hh_cb.p_sdp_db = p_db;
    SDP_InitDiscoveryDb(p_db, db_len, 1, &uuid_list, 0, NULL);

    if (SDP_ServiceSearchRequest(addr, p_db, hidh_search_callback)) {
        hh_cb.sdp_cback = sdp_cback;
        hh_cb.sdp_busy = TRUE;
        return HID_SUCCESS;
    }
    return HID_ERR_NO_RESOURCES;
}

/* Copies the text attribute `attr_id` of `p_rec` into `str`; a value cut to `max_len` - 1 characters is terminated at
 * `str[max_len]`. */
static void hidh_get_str_attr(tSDP_DISC_REC* p_rec, u16 attr_id, u16 max_len, char* str) {
    tSDP_DISC_ATTR* p_attr;
    u16 name_len;

    if ((p_attr = SDP_FindAttributeInRec(p_rec, attr_id)) != NULL) {
        if ((name_len = SDP_DISC_ATTR_LEN(p_attr->attr_len_type)) < max_len) {
            memcpy(str, p_attr->attr_value.array, name_len);
            str[name_len] = '\0';
        } else {
            memcpy(str, p_attr->attr_value.array, max_len - 1);
            str[max_len] = '\0';
        }
    } else {
        str[0] = '\0';
    }
}

/* 0x8049019C (0x450): Reads the HID record out of the finished query and reports it to the query's callback. */
void hidh_search_callback(u16 sdp_result) {
    tSDP_DISCOVERY_DB* p_db = hh_cb.p_sdp_db;
    tSDP_DISC_REC* p_rec;
    tSDP_DISC_ATTR* p_attr;
    tSDP_DISC_ATTR* p_subattr1;
    tSDP_DISC_ATTR* p_subattr2;
    tSDP_DISC_ATTR* p_repdesc;
    tSDP_UUID hid_uuid;
    tHID_DEV_SDP_INFO* p_nvi = &hh_cb.sdp_rec;
    u16 attr_mask = 0;

    hid_uuid.len = 2;
    hid_uuid.uu.uuid16 = UUID_SERVCLASS_HUMAN_INTERFACE;

    hh_cb.sdp_busy = FALSE;

    if (sdp_result != SDP_SUCCESS) {
        hh_cb.sdp_cback(sdp_result, 0, NULL);
        return;
    }

    if ((p_rec = SDP_FindServiceUUIDInDb(p_db, &hid_uuid, NULL)) == NULL) {
        hh_cb.sdp_cback(HID_SDP_NO_SERV_UUID, 0, NULL);
        return;
    }

    memset(&hh_cb.sdp_rec, 0, sizeof(tHID_DEV_SDP_INFO));

    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_DESCRIPTOR_LIST)) == NULL
        || SDP_DISC_ATTR_TYPE(p_attr->attr_len_type) != DATA_ELE_SEQ_DESC_TYPE
        || (p_subattr1 = p_attr->attr_value.p_sub_attr) == NULL
        || SDP_DISC_ATTR_TYPE(p_subattr1->attr_len_type) != DATA_ELE_SEQ_DESC_TYPE
        || (p_subattr2 = p_subattr1->attr_value.p_sub_attr) == NULL
        || (p_repdesc = p_subattr2->p_next_attr) == NULL
        || SDP_DISC_ATTR_TYPE(p_repdesc->attr_len_type) != TEXT_STR_DESC_TYPE) {
        hh_cb.sdp_cback(HID_SDP_MANDATORY_MISSING, 0, NULL);
        return;
    }

    if ((p_nvi->dscp_info.length = SDP_DISC_ATTR_LEN(p_repdesc->attr_len_type)) != 0) {
        p_nvi->dscp_info.descriptor = (u8*)&p_repdesc->attr_value;
    }

    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_VIRTUAL_CABLE)) != NULL && p_attr->attr_value.uint8) {
        attr_mask |= HID_VIRTUAL_CABLE;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_RECONNECT_INITIATE)) != NULL && p_attr->attr_value.uint8) {
        attr_mask |= HID_RECONN_INIT;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_NORMALLY_CONNECTABLE)) != NULL
        && p_attr->attr_value.uint8) {
        attr_mask |= HID_NORMALLY_CONNECTABLE;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_SDP_DISABLE)) != NULL && p_attr->attr_value.uint8) {
        attr_mask |= HID_SDP_DISABLE;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_BATTERY_POWER)) != NULL && p_attr->attr_value.uint8) {
        attr_mask |= HID_BATTERY_POWER;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_REMOTE_WAKE)) != NULL && p_attr->attr_value.uint8) {
        attr_mask |= HID_REMOTE_WAKE;
    }

    hidh_get_str_attr(p_rec, ATTR_ID_SERVICE_NAME, HID_MAX_SVC_NAME_LEN, p_nvi->svc_name);
    hidh_get_str_attr(p_rec, ATTR_ID_SERVICE_DESCRIPTION, HID_MAX_SVC_DESCR_LEN, p_nvi->svc_descr);
    hidh_get_str_attr(p_rec, ATTR_ID_PROVIDER_NAME, HID_MAX_PROV_NAME_LEN, p_nvi->prov_name);

    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_DEVICE_RELNUM)) != NULL) {
        p_nvi->rel_num = p_attr->attr_value.uint16;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_COUNTRY_CODE)) != NULL) {
        p_nvi->ctry_code = p_attr->attr_value.uint8;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_DEVICE_SUBCLASS)) != NULL) {
        p_nvi->sub_class = p_attr->attr_value.uint8;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_PARSER_VERSION)) != NULL) {
        p_nvi->hpars_ver = p_attr->attr_value.uint16;
    }
    if ((p_attr = SDP_FindAttributeInRec(p_rec, ATTR_ID_HID_LINK_SUPERVISION_TO)) != NULL) {
        attr_mask |= HID_SUP_TOUT_AVLBL;
        p_nvi->sup_timeout = p_attr->attr_value.uint16;
    }

    hh_cb.sdp_rec.p_sdp_layer_rec = p_rec;
    hh_cb.sdp_cback(SDP_SUCCESS, attr_mask, &hh_cb.sdp_rec);
}

/* 0x804905EC (0x44): Clears the control block and silences the trace. */
void HID_HostInit(void) {
    memset(&hh_cb, 0, sizeof(tHID_HOST_CB));
    hh_cb.trace_level = 0;
}

/* 0x80490630 (0x7C): Registers the HID PSMs with L2CAP and stores the device callback; returns a HID result. */
u8 HID_HostRegister(tHID_HOST_DEV_CALLBACK* dev_cback) {
    u8 st;

    if (hh_cb.reg_flag) {
        return HID_ERR_ALREADY_REGISTERED;
    }
    if (dev_cback == NULL) {
        return HID_ERR_INVALID_PARAM;
    }
    if ((st = hidh_conn_reg()) != HID_SUCCESS) {
        return st;
    }

    hh_cb.callback = dev_cback;
    hh_cb.reg_flag = TRUE;
    return HID_SUCCESS;
}

/* 0x804906AC (0xE4): Removes every device and deregisters the HID PSMs; returns a HID result. */
u8 HID_HostDeregister(void) {
    u8 i;

    if (!hh_cb.reg_flag) {
        return HID_ERR_NOT_REGISTERED;
    }

    for (i = 0; i < HID_HOST_MAX_DEVICES; i++) {
        HID_HostRemoveDev(i);
    }

    hidh_conn_dereg();
    hh_cb.reg_flag = FALSE;
    return HID_SUCCESS;
}

/* 0x80490790 (0x190): Finds or takes a device slot for `addr` and stores its attributes; returns a HID result. */
u8 HID_HostAddDev(u8* addr, u16 attr_mask, u8* handle) {
    int i;

    if (!hh_cb.reg_flag) {
        return HID_ERR_NOT_REGISTERED;
    }

    for (i = 0; i < HID_HOST_MAX_DEVICES; i++) {
        if (!memcmp(addr, hh_cb.devices[i].addr, 6)) {
            break;
        }
    }

    if (i == HID_HOST_MAX_DEVICES) {
        for (i = 0; i < HID_HOST_MAX_DEVICES; i++) {
            if (!hh_cb.devices[i].in_use) {
                break;
            }
        }
    }

    if (i == HID_HOST_MAX_DEVICES) {
        return HID_ERR_NO_RESOURCES;
    }

    if (!hh_cb.devices[i].in_use) {
        hh_cb.devices[i].in_use = TRUE;
        memcpy(hh_cb.devices[i].addr, addr, 6);
        hh_cb.devices[i].state = HID_DEV_NO_CONN;
        hh_cb.devices[i].conn_tries = 0;
    }

    hh_cb.devices[i].attr_mask = attr_mask;
    *handle = i;
    return HID_SUCCESS;
}

/* 0x80490920 (0xC8): Closes the device `dev_handle` and frees its slot; returns a HID result. */
u8 HID_HostRemoveDev(u8 dev_handle) {
    if (!hh_cb.reg_flag) {
        return HID_ERR_NOT_REGISTERED;
    }
    if (dev_handle > HID_HOST_MAX_DEVICES || !hh_cb.devices[dev_handle].in_use) {
        return HID_ERR_INVALID_PARAM;
    }

    HID_HostCloseDev(dev_handle);
    hh_cb.devices[dev_handle].in_use = FALSE;
    hh_cb.devices[dev_handle].conn.conn_state = HID_CONN_STATE_UNUSED;
    hh_cb.devices[dev_handle].conn.ctrl_cid = hh_cb.devices[dev_handle].conn.intr_cid = 0;
    return HID_SUCCESS;
}

/* 0x804909E8 (0x64): Opens the connection to the device `dev_handle`; returns a HID result. */
u8 HID_HostOpenDev(u8 dev_handle) {
    if (!hh_cb.reg_flag) {
        return HID_ERR_NOT_REGISTERED;
    }
    if (dev_handle > HID_HOST_MAX_DEVICES || !hh_cb.devices[dev_handle].in_use) {
        return HID_ERR_INVALID_PARAM;
    }
    if (hh_cb.devices[dev_handle].state != HID_DEV_NO_CONN) {
        return HID_ERR_ALREADY_CONN;
    }

    hh_cb.devices[dev_handle].conn_tries = 1;
    return hidh_conn_initiate(dev_handle);
}

/* 0x80490A4C (0x140): Sends a transaction to the device `dev_handle`, or frees `pbuf` when it cannot; returns a HID
 * result. */
u8 HID_HostWriteDev(u8 dev_handle, u8 t_type, u8 param, u16 data, u8 report_id, BT_HDR* pbuf) {
    u8 status = HID_SUCCESS;

    if (!hh_cb.reg_flag) {
        HIDH_TRACE_ERROR0("HID_ERR_NOT_REGISTERED");
        status = HID_ERR_NOT_REGISTERED;
    }

    if (dev_handle > HID_HOST_MAX_DEVICES || !hh_cb.devices[dev_handle].in_use) {
        HIDH_TRACE_ERROR0("HID_ERR_INVALID_PARAM");
        status = HID_ERR_INVALID_PARAM;
    }

    if (hh_cb.devices[dev_handle].state != HID_DEV_CONNECTED) {
        HIDH_TRACE_ERROR1("HID_ERR_NO_CONNECTION dev_handle %d", dev_handle);
        status = HID_ERR_NO_CONNECTION;
    }

    if (status != HID_SUCCESS) {
        if (pbuf) {
            GKI_freebuf(pbuf);
        }
    } else {
        status = hidh_conn_snd_data(dev_handle, t_type, param, data, report_id, pbuf);
    }

    return status;
}

/* 0x80490B8C (0xA0): Stops the retries of the device `dev_handle` and closes its connection; returns a HID result. */
u8 HID_HostCloseDev(u8 dev_handle) {
    if (!hh_cb.reg_flag) {
        return HID_ERR_NOT_REGISTERED;
    }
    if (dev_handle > HID_HOST_MAX_DEVICES || !hh_cb.devices[dev_handle].in_use) {
        return HID_ERR_INVALID_PARAM;
    }

    hh_cb.devices[dev_handle].conn_tries = HID_HOST_MAX_CONN_RETRY + 1;
    btu_stop_timer(&hh_cb.devices[dev_handle].conn.timer_entry);

    if (hh_cb.devices[dev_handle].state != HID_DEV_CONNECTED) {
        return HID_ERR_NO_CONNECTION;
    }

    hh_cb.devices[dev_handle].conn_tries = HID_HOST_MAX_CONN_RETRY + 1;
    return hidh_conn_disconnect(dev_handle);
}

/* 0x80490C2C (0x228): Registers the security level of the HID channels under `serv_name`; returns a HID result. */
u8 HID_HostSetSecurityLevel(char* serv_name, u8 sec_lvl) {
    if (!BTM_SetSecurityLevel(FALSE, serv_name, BTM_SEC_SERVICE_HID_SEC_CTRL, sec_lvl, HID_PSM_CONTROL,
                              BTM_SEC_PROTO_HID, HID_SEC_CHN)) {
        HIDH_TRACE_ERROR0("Security Registration 1 failed");
        return HID_ERR_NO_RESOURCES;
    }
    if (!BTM_SetSecurityLevel(TRUE, serv_name, BTM_SEC_SERVICE_HID_SEC_CTRL, sec_lvl, HID_PSM_CONTROL,
                              BTM_SEC_PROTO_HID, HID_SEC_CHN)) {
        HIDH_TRACE_ERROR0("Security Registration 2 failed");
        return HID_ERR_NO_RESOURCES;
    }
    if (!BTM_SetSecurityLevel(FALSE, serv_name, BTM_SEC_SERVICE_HID_NOSEC_CTRL, BTM_SEC_NONE, HID_PSM_CONTROL,
                              BTM_SEC_PROTO_HID, HID_NOSEC_CHN)) {
        HIDH_TRACE_ERROR0("Security Registration 3 failed");
        return HID_ERR_NO_RESOURCES;
    }
    if (!BTM_SetSecurityLevel(TRUE, serv_name, BTM_SEC_SERVICE_HID_NOSEC_CTRL, BTM_SEC_NONE, HID_PSM_CONTROL,
                              BTM_SEC_PROTO_HID, HID_NOSEC_CHN)) {
        HIDH_TRACE_ERROR0("Security Registration 4 failed");
        return HID_ERR_NO_RESOURCES;
    }
    if (!BTM_SetSecurityLevel(TRUE, serv_name, BTM_SEC_SERVICE_HID_INTR, BTM_SEC_NONE, HID_PSM_INTERRUPT,
                              BTM_SEC_PROTO_HID, 0)) {
        HIDH_TRACE_ERROR0("Security Registration 5 failed");
        return HID_ERR_NO_RESOURCES;
    }
    if (!BTM_SetSecurityLevel(FALSE, serv_name, BTM_SEC_SERVICE_HID_INTR, BTM_SEC_NONE, HID_PSM_INTERRUPT,
                              BTM_SEC_PROTO_HID, 0)) {
        HIDH_TRACE_ERROR0("Security Registration 6 failed");
        return HID_ERR_NO_RESOURCES;
    }
    return HID_SUCCESS;
}
