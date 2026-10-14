/*
 * BTE/sdp_api.c - the SDP client API: discovery database setup, service search requests, record and attribute lookups,
 *   the device identification record and the trace level.
 *
 * RANGE. .text 0x8049F44C-0x804A02B4 (9 functions, 0xE68 B); .data 0x80617E00-0x80617E50. The .text holds the BTE
 *   sdp_api.c functions SDP_InitDiscoveryDb..SDP_SetTraceLevel; sdp_db.c's sdp_db_service_search follows. The .data
 *   (SDP_InitDiscoveryDb's error format) starts after a 4-byte pad beyond 4-byte alignment (0x80617DFC -> 0x80617E00).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`), per object in configure.py;
 *   the evidence is in BTE/sdp_utils.c's header. The function starts of this band are packed to 4 bytes.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_80617E00.
 * SHAPES. plain C. The device identification specification id the record carries is 0x0009 (the bytes 00 09 the
 *   target stores).
 */

#include "BTE/sdp_api.h"
#include "BTE/sdp_db.h"
#include "BTE/sdp_main.h"
#include "BTE/sdp_utils.h"
#include "MSL/strlen.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x8049F44C (0x378): Clears the caller's discovery database and stores its UUID and attribute filters. */
u8 SDP_InitDiscoveryDb(tSDP_DISCOVERY_DB* p_db, u32 len, u16 num_uuid, tSDP_UUID* p_uuid_list, u16 num_attr,
                       u16* p_attr_list) {
    u16 xx;

    if (p_db == NULL || sizeof(tSDP_DISCOVERY_DB) > len || num_attr > SDP_MAX_ATTR_FILTERS ||
        num_uuid > SDP_MAX_UUID_FILTERS) {
        SDP_TRACE_ERROR4("SDP_InitDiscoveryDb Illegal param: p_db 0x%x, len %d, num_uuid %d, num_attr %d", (u32)p_db, len,
                         num_uuid, num_attr);
        return FALSE;
    }

    memset(p_db, 0, len);

    p_db->mem_size = len - sizeof(tSDP_DISCOVERY_DB);
    p_db->mem_free = p_db->mem_size;
    p_db->p_first_rec = NULL;
    p_db->p_free_mem = (u8*)p_db + sizeof(tSDP_DISCOVERY_DB);

    for (xx = 0; xx < num_uuid; xx++) {
        p_db->uuid_filters[xx] = *p_uuid_list++;
    }

    p_db->num_uuid_filters = num_uuid;

    for (xx = 0; xx < num_attr; xx++) {
        p_db->attr_filters[xx] = *p_attr_list++;
    }

    sdpu_sort_attr_list(num_attr, p_db);

    p_db->num_attr_filters = num_attr;
    return TRUE;
}

/* 0x8049F7C4 (0x5C): Connects to the device and starts a service search into `p_db`. */
u8 SDP_ServiceSearchRequest(u8* p_bd_addr, tSDP_DISCOVERY_DB* p_db, tSDP_DISC_CMPL_CB* p_cb) {
    tCONN_CB* p_ccb;

    p_ccb = sdp_conn_originate(p_bd_addr);
    if (!p_ccb) {
        return FALSE;
    }

    p_ccb->disc_state = SDP_DISC_WAIT_CONN;
    p_ccb->p_db = p_db;
    p_ccb->p_cb = p_cb;

    return TRUE;
}

/* 0x8049F820 (0x64): Connects to the device and starts a combined service search and attribute request. */
u8 SDP_ServiceSearchAttributeRequest(u8* p_bd_addr, tSDP_DISCOVERY_DB* p_db, tSDP_DISC_CMPL_CB* p_cb) {
    tCONN_CB* p_ccb;

    p_ccb = sdp_conn_originate(p_bd_addr);
    if (!p_ccb) {
        return FALSE;
    }

    p_ccb->disc_state = SDP_DISC_WAIT_CONN;
    p_ccb->p_db = p_db;
    p_ccb->p_cb = p_cb;
    p_ccb->is_attr_search = TRUE;

    return TRUE;
}

/* 0x8049F884 (0x28): Returns the discovered record's attribute with the id, or NULL. */
tSDP_DISC_ATTR* SDP_FindAttributeInRec(tSDP_DISC_REC* p_rec, u16 attr_id) {
    tSDP_DISC_ATTR* p_attr;

    p_attr = p_rec->p_first_attr;
    while (p_attr) {
        if (p_attr->attr_id == attr_id) {
            return p_attr;
        }
        p_attr = p_attr->p_next_attr;
    }

    return NULL;
}

/* 0x8049F8AC (0xCC): Returns the next discovered record after `p_start_rec` with the 16-bit service class. */
tSDP_DISC_REC* SDP_FindServiceInDb(tSDP_DISCOVERY_DB* p_db, u16 service_uuid, tSDP_DISC_REC* p_start_rec) {
    tSDP_DISC_REC* p_rec;
    tSDP_DISC_ATTR* p_attr;
    tSDP_DISC_ATTR* p_sattr;

    if (!p_start_rec) {
        p_rec = p_db->p_first_rec;
    } else {
        p_rec = p_start_rec->p_next_rec;
    }

    while (p_rec) {
        p_attr = p_rec->p_first_attr;
        while (p_attr) {
            if (p_attr->attr_id == ATTR_ID_SERVICE_CLASS_ID_LIST &&
                (p_attr->attr_len_type >> 12) == DATA_ELE_SEQ_DESC_TYPE) {
                for (p_sattr = p_attr->attr_value.p_sub_attr; p_sattr; p_sattr = p_sattr->p_next_attr) {
                    if ((p_sattr->attr_len_type >> 12) == UUID_DESC_TYPE && (p_sattr->attr_len_type & 0xFFF) == 2 &&
                        p_sattr->attr_value.uint16 == service_uuid) {
                        return p_rec;
                    }
                }
                break;
            } else if (p_attr->attr_id == ATTR_ID_SERVICE_ID) {
                if ((p_attr->attr_len_type >> 12) == UUID_DESC_TYPE && (p_attr->attr_len_type & 0xFFF) == 2 &&
                    p_attr->attr_value.uint16 == service_uuid) {
                    return p_rec;
                }
            }

            p_attr = p_attr->p_next_attr;
        }

        p_rec = p_rec->p_next_rec;
    }

    return NULL;
}

/* 0x8049F978 (0x108): Returns the next discovered record after `p_start_rec` with the service class UUID. */
tSDP_DISC_REC* SDP_FindServiceUUIDInDb(tSDP_DISCOVERY_DB* p_db, tSDP_UUID* p_uuid, tSDP_DISC_REC* p_start_rec) {
    tSDP_DISC_REC* p_rec;
    tSDP_DISC_ATTR* p_attr;
    tSDP_DISC_ATTR* p_sattr;

    if (!p_start_rec) {
        p_rec = p_db->p_first_rec;
    } else {
        p_rec = p_start_rec->p_next_rec;
    }

    while (p_rec) {
        p_attr = p_rec->p_first_attr;
        while (p_attr) {
            if (p_attr->attr_id == ATTR_ID_SERVICE_CLASS_ID_LIST &&
                (p_attr->attr_len_type >> 12) == DATA_ELE_SEQ_DESC_TYPE) {
                for (p_sattr = p_attr->attr_value.p_sub_attr; p_sattr; p_sattr = p_sattr->p_next_attr) {
                    if ((p_sattr->attr_len_type >> 12) == UUID_DESC_TYPE) {
                        if (sdpu_compare_uuid_with_attr(p_uuid, p_sattr)) {
                            return p_rec;
                        }
                    }
                }
                break;
            } else if (p_attr->attr_id == ATTR_ID_SERVICE_ID) {
                if ((p_attr->attr_len_type >> 12) == UUID_DESC_TYPE) {
                    if (sdpu_compare_uuid_with_attr(p_uuid, p_attr)) {
                        return p_rec;
                    }
                }
            }

            p_attr = p_attr->p_next_attr;
        }

        p_rec = p_rec->p_next_rec;
    }

    return NULL;
}

/* 0x8049FA80 (0x360): Builds the local device identification record (the primary one is reused); returns an SDP result. */
u16 SDP_SetLocalDiRecord(tSDP_DI_RECORD* p_device_info, u32* p_handle) {
    u16 result = SDP_SUCCESS;
    u32 handle;
    u16 di_uuid = UUID_SERVCLASS_PNP_INFORMATION;
    u8 temp_u16[2];
    u8* p_temp;

    *p_handle = 0;
    if (p_device_info == NULL) {
        return SDP_ILLEGAL_PARAMETER;
    }

    if (p_device_info->primary_record == TRUE && sdp_cb.server_db.di_primary_handle) {
        handle = sdp_cb.server_db.di_primary_handle;
    } else {
        if ((handle = SDP_CreateRecord()) == 0) {
            return SDP_NO_RESOURCES;
        }
    }

    *p_handle = handle;

    if (!SDP_AddServiceClassIdList(handle, 1, &di_uuid)) {
        result = SDP_DI_REG_FAILED;
    }

    if (result == SDP_SUCCESS) {
        p_temp = temp_u16;
        UINT16_TO_BE_STREAM(p_temp, BLUETOOTH_DI_SPECIFICATION);
        if (!SDP_AddAttribute(handle, ATTR_ID_SPECIFICATION_ID, UINT_DESC_TYPE, sizeof(temp_u16), temp_u16)) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS && p_device_info->client_executable_url[0] != '\0') {
        if (!((strlen(p_device_info->client_executable_url) + 1 <= SDP_MAX_ATTR_LEN) &&
              SDP_AddAttribute(handle, ATTR_ID_CLIENT_EXE_URL, URL_DESC_TYPE,
                               (u32)(strlen(p_device_info->client_executable_url) + 1),
                               (u8*)p_device_info->client_executable_url))) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS && p_device_info->service_description[0] != '\0') {
        if (!((strlen(p_device_info->service_description) + 1 <= SDP_MAX_ATTR_LEN) &&
              SDP_AddAttribute(handle, ATTR_ID_SERVICE_DESCRIPTION, TEXT_STR_DESC_TYPE,
                               (u32)(strlen(p_device_info->service_description) + 1),
                               (u8*)p_device_info->service_description))) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS && p_device_info->documentation_url[0] != '\0') {
        if (!((strlen(p_device_info->documentation_url) + 1 <= SDP_MAX_ATTR_LEN) &&
              SDP_AddAttribute(handle, ATTR_ID_DOCUMENTATION_URL, URL_DESC_TYPE,
                               (u32)(strlen(p_device_info->documentation_url) + 1),
                               (u8*)p_device_info->documentation_url))) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS) {
        p_temp = temp_u16;
        UINT16_TO_BE_STREAM(p_temp, p_device_info->vendor);
        if (!SDP_AddAttribute(handle, ATTR_ID_VENDOR_ID, UINT_DESC_TYPE, sizeof(p_device_info->vendor), temp_u16)) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS) {
        p_temp = temp_u16;
        UINT16_TO_BE_STREAM(p_temp, p_device_info->product);
        if (!SDP_AddAttribute(handle, ATTR_ID_PRODUCT_ID, UINT_DESC_TYPE, sizeof(p_device_info->product), temp_u16)) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS) {
        p_temp = temp_u16;
        UINT16_TO_BE_STREAM(p_temp, p_device_info->version);
        if (!SDP_AddAttribute(handle, ATTR_ID_PRODUCT_VERSION, UINT_DESC_TYPE, sizeof(p_device_info->version),
                              temp_u16)) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS) {
        if (!SDP_AddAttribute(handle, ATTR_ID_PRIMARY_RECORD, BOOLEAN_DESC_TYPE, 1, &p_device_info->primary_record)) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result == SDP_SUCCESS) {
        p_temp = temp_u16;
        UINT16_TO_BE_STREAM(p_temp, p_device_info->vendor_id_source);
        if (!SDP_AddAttribute(handle, ATTR_ID_VENDOR_ID_SOURCE, UINT_DESC_TYPE, sizeof(p_device_info->vendor_id_source),
                              temp_u16)) {
            result = SDP_DI_REG_FAILED;
        }
    }

    if (result != SDP_SUCCESS) {
        SDP_DeleteRecord(handle);
    } else if (p_device_info->primary_record == TRUE) {
        sdp_cb.server_db.di_primary_handle = handle;
    }

    return result;
}

/* 0x8049FDE0 (0x4B0): Reads a local device identification record (the primary one for handle 0) back into `p_device_info`. */
u16 SDP_GetLocalDiRecord(tSDP_DI_GET_RECORD* p_device_info, u32* p_handle) {
    u16 result = SDP_NO_DI_RECORD_FOUND;
    tSDP_RECORD* p_rec;
    tSDP_ATTRIBUTE* p_attr;
    u8* p_temp;
    s32 templen;

    if (*p_handle == 0) {
        *p_handle = sdp_cb.server_db.di_primary_handle;
    }

    if ((p_rec = sdp_db_find_record(*p_handle)) != NULL) {
        memset(p_device_info, 0, sizeof(tSDP_DI_RECORD));

        result = SDP_SUCCESS;

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_SPECIFICATION_ID, ATTR_ID_SPECIFICATION_ID);
        if (p_attr) {
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_UINT16(p_device_info->spec_id, p_temp);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_VENDOR_ID, ATTR_ID_VENDOR_ID);
        if (p_attr) {
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_UINT16(p_device_info->rec.vendor, p_temp);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_PRODUCT_ID, ATTR_ID_PRODUCT_ID);
        if (p_attr) {
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_UINT16(p_device_info->rec.product, p_temp);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_PRODUCT_VERSION, ATTR_ID_PRODUCT_VERSION);
        if (p_attr) {
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_UINT16(p_device_info->rec.version, p_temp);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_VENDOR_ID_SOURCE, ATTR_ID_VENDOR_ID_SOURCE);
        if (p_attr) {
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_UINT16(p_device_info->rec.vendor_id_source, p_temp);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_PRIMARY_RECORD, ATTR_ID_PRIMARY_RECORD);
        if (p_attr) {
            p_device_info->rec.primary_record = *p_attr->value_ptr;
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_CLIENT_EXE_URL, ATTR_ID_CLIENT_EXE_URL);
        if (p_attr) {
            templen = (s32)((p_attr->len < SDP_MAX_ATTR_LEN) ? p_attr->len : SDP_MAX_ATTR_LEN);
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_ARRAY(p_temp, p_device_info->rec.client_executable_url, templen);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_SERVICE_DESCRIPTION, ATTR_ID_SERVICE_DESCRIPTION);
        if (p_attr) {
            templen = (s32)((p_attr->len < SDP_MAX_ATTR_LEN) ? p_attr->len : SDP_MAX_ATTR_LEN);
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_ARRAY(p_temp, p_device_info->rec.service_description, templen);
        }

        p_attr = sdp_db_find_attr_in_rec(p_rec, ATTR_ID_DOCUMENTATION_URL, ATTR_ID_DOCUMENTATION_URL);
        if (p_attr) {
            templen = (s32)((p_attr->len < SDP_MAX_ATTR_LEN) ? p_attr->len : SDP_MAX_ATTR_LEN);
            p_temp = p_attr->value_ptr;
            BE_STREAM_TO_ARRAY(p_temp, p_device_info->rec.documentation_url, templen);
        }
    } else {
        *p_handle = 0;
    }

    return result;
}

/* 0x804A0290 (0x24): Sets the SDP trace level (0xFF only reads it); returns the level. */
u8 SDP_SetTraceLevel(u8 new_level) {
    if (new_level != 0xFF) {
        sdp_cb.trace_level = new_level;
    }

    return sdp_cb.trace_level;
}
