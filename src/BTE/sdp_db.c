/*
 * BTE/sdp_db.c - the SDP server database: service search, record and attribute lookup, record creation and deletion,
 *   attribute adds and deletes.
 *
 * RANGE. .text 0x804A02B4-0x804A0F80 (10 functions, 0xCCC B); .data 0x80617E50-0x80617EE8. The .text holds the BTE
 *   sdp_db.c functions sdp_db_service_search..SDP_DeleteAttribute; sdp_discovery.c's sdpu_build_uuid_seq follows. The
 *   .data is the three trace format strings of SDP_AddAttribute, SDP_AddUuidSequence and SDP_DeleteAttribute.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`), per object in configure.py;
 *   the evidence is in BTE/sdp_utils.c's header. The function starts of this band are packed to 4 bytes.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C; the server database is sdp_cb.server_db (BTE/sdp_int.h). SDP_DeleteAttribute reuses its record
 *   index for the attribute walk, as the BTE source does.
 */

#include "BTE/sdp_db.h"
#include "BTE/sdp_main.h"
#include "BTE/sdp_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

static u8 find_uuid_in_seq(u8* p, u32 seq_len, u8* p_uuid, u16 uuid_len, int nest_level);

/* 0x804A02B4 (0x13C): Returns the next record after `p_rec` that holds every UUID of `p_seq`, or NULL. */
tSDP_RECORD* sdp_db_service_search(tSDP_RECORD* p_rec, tSDP_UUID_SEQ* p_seq) {
    u16 xx;
    u16 yy;
    tSDP_ATTRIBUTE* p_attr;
    tSDP_RECORD* p_end = &sdp_cb.server_db.record[sdp_cb.server_db.num_records];

    if (!p_rec) {
        p_rec = &sdp_cb.server_db.record[0];
    } else {
        p_rec++;
    }

    for (; p_rec < p_end; p_rec++) {
        for (yy = 0; yy < p_seq->num_uids; yy++) {
            p_attr = &p_rec->attribute[0];
            for (xx = 0; xx < p_rec->num_attributes; xx++, p_attr++) {
                if (p_attr->type == UUID_DESC_TYPE) {
                    if (sdpu_compare_uuid_arrays(p_attr->value_ptr, p_attr->len, &p_seq->uuid_entry[yy].value[0],
                                                 p_seq->uuid_entry[yy].len)) {
                        break;
                    }
                } else if (p_attr->type == DATA_ELE_SEQ_DESC_TYPE) {
                    if (find_uuid_in_seq(p_attr->value_ptr, p_attr->len, &p_seq->uuid_entry[yy].value[0],
                                         p_seq->uuid_entry[yy].len, 0)) {
                        break;
                    }
                }
            }

            if (xx == p_rec->num_attributes) {
                break;
            }
        }

        if (yy == p_seq->num_uids) {
            return p_rec;
        }
    }

    return NULL;
}

/* 0x804A03F0 (0xD8): Returns whether a data element sequence, or one nested in it, holds the UUID. */
static u8 find_uuid_in_seq(u8* p, u32 seq_len, u8* p_uuid, u16 uuid_len, int nest_level) {
    u8* p_end = p + seq_len;
    u8 type;
    u32 len;

    if (nest_level > 3) {
        return FALSE;
    }

    while (p < p_end) {
        type = *p++;
        p = sdpu_get_len_from_type(p, type, &len);
        type = type >> 3;
        if (type == UUID_DESC_TYPE) {
            if (sdpu_compare_uuid_arrays(p, len, p_uuid, uuid_len)) {
                return TRUE;
            }
        } else if (type == DATA_ELE_SEQ_DESC_TYPE) {
            if (find_uuid_in_seq(p, len, p_uuid, uuid_len, nest_level + 1)) {
                return TRUE;
            }
        }
        p = p + len;
    }

    return FALSE;
}

/* 0x804A04C8 (0x5C): Returns the local record with the handle, or NULL. */
tSDP_RECORD* sdp_db_find_record(u32 handle) {
    tSDP_RECORD* p_rec;
    tSDP_RECORD* p_end = &sdp_cb.server_db.record[sdp_cb.server_db.num_records];

    for (p_rec = &sdp_cb.server_db.record[0]; p_rec < p_end; p_rec++) {
        if (p_rec->record_handle == handle) {
            return p_rec;
        }
    }

    return NULL;
}

/* 0x804A0524 (0x40): Returns the record's first attribute with an id in the range, or NULL. */
tSDP_ATTRIBUTE* sdp_db_find_attr_in_rec(tSDP_RECORD* p_rec, u16 start_attr, u16 end_attr) {
    tSDP_ATTRIBUTE* p_at;
    u16 xx;

    for (xx = 0, p_at = &p_rec->attribute[0]; xx < p_rec->num_attributes; xx++, p_at++) {
        if (p_at->id >= start_attr && p_at->id <= end_attr) {
            return p_at;
        }
    }

    return NULL;
}

/* 0x804A0564 (0xE0): Adds an empty local record with the next free handle; returns the handle, 0 when the database is full. */
u32 SDP_CreateRecord(void) {
    u32 handle;
    u8 buf[4];
    tSDP_DB* p_db = &sdp_cb.server_db;

    if (p_db->num_records < SDP_MAX_RECORDS) {
        memset(&p_db->record[p_db->num_records], 0, sizeof(tSDP_RECORD));

        if (p_db->num_records) {
            handle = p_db->record[p_db->num_records - 1].record_handle + 1;
        } else {
            handle = 0x10000;
        }

        p_db->record[p_db->num_records].record_handle = handle;
        p_db->num_records++;

        buf[0] = (u8)(handle >> 24);
        buf[1] = (u8)(handle >> 16);
        buf[2] = (u8)(handle >> 8);
        buf[3] = (u8)handle;
        SDP_AddAttribute(handle, ATTR_ID_RECORD_HANDLE, UINT_DESC_TYPE, 4, buf);

        return p_db->record[p_db->num_records - 1].record_handle;
    }

    return 0;
}

/* 0x804A0644 (0x114): Deletes the local record with the handle (every record for handle 0); returns whether one went. */
u8 SDP_DeleteRecord(u32 handle) {
    u16 xx;
    u16 yy;
    u16 zz;
    tSDP_RECORD* p_rec = &sdp_cb.server_db.record[0];

    if (handle == 0 || sdp_cb.server_db.num_records == 0) {
        sdp_cb.server_db.num_records = 0;
        sdp_cb.server_db.di_primary_handle = 0;
        sdp_cb.server_db.brcm_di_registered = 0;
        return TRUE;
    } else {
        for (xx = 0; xx < sdp_cb.server_db.num_records; xx++, p_rec++) {
            if (p_rec->record_handle == handle) {
                for (yy = xx; yy < sdp_cb.server_db.num_records; yy++, p_rec++) {
                    *p_rec = *(p_rec + 1);

                    for (zz = 0; zz < p_rec->num_attributes; zz++) {
                        p_rec->attribute[zz].value_ptr -= sizeof(tSDP_RECORD);
                    }
                }

                sdp_cb.server_db.num_records--;

                if (sdp_cb.server_db.di_primary_handle == handle) {
                    sdp_cb.server_db.di_primary_handle = 0;
                    sdp_cb.server_db.brcm_di_registered = 0;
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* 0x804A0758 (0x398): Adds or replaces an attribute of a local record, keeping the attributes in id order. */
u8 SDP_AddAttribute(u32 handle, u16 attr_id, u8 attr_type, u32 attr_len, u8* p_val) {
    u16 xx;
    u16 yy;
    u16 zz;
    tSDP_RECORD* p_rec = &sdp_cb.server_db.record[0];

    for (zz = 0; zz < sdp_cb.server_db.num_records; zz++, p_rec++) {
        if (p_rec->record_handle == handle) {
            tSDP_ATTRIBUTE* p_attr = &p_rec->attribute[0];

            for (xx = 0; xx < p_rec->num_attributes; xx++, p_attr++) {
                if (p_attr->id == attr_id) {
                    SDP_DeleteAttribute(handle, attr_id);
                    break;
                }
                if (p_attr->id > attr_id) {
                    break;
                }
            }

            if (p_rec->num_attributes == SDP_MAX_REC_ATTR) {
                return FALSE;
            }

            if (xx == p_rec->num_attributes) {
                p_attr = &p_rec->attribute[p_rec->num_attributes];
            } else {
                for (yy = p_rec->num_attributes; yy > xx; yy--) {
                    p_rec->attribute[yy] = p_rec->attribute[yy - 1];
                }
            }

            p_rec->num_attributes++;

            p_attr->id = attr_id;
            p_attr->len = attr_len;
            p_attr->type = attr_type;

            if (p_rec->free_pad_ptr + attr_len >= SDP_MAX_PAD_LEN) {
                SDP_TRACE_WARNING2("SDP_AddAttribute: attr_len:%d too long. truncate to (%d)", attr_len,
                                   SDP_MAX_PAD_LEN - p_rec->free_pad_ptr);
                attr_len = SDP_MAX_PAD_LEN - p_rec->free_pad_ptr;
                p_val[SDP_MAX_PAD_LEN - p_rec->free_pad_ptr] = '\0';
                p_val[SDP_MAX_PAD_LEN - p_rec->free_pad_ptr + 1] = '\0';
            }

            if (attr_len > 0 && p_val != NULL) {
                memcpy(&p_rec->attr_pad[p_rec->free_pad_ptr], p_val, attr_len);
                p_attr->value_ptr = &p_rec->attr_pad[p_rec->free_pad_ptr];
                p_rec->free_pad_ptr += attr_len;
            }

            if (handle == sdp_cb.server_db.di_primary_handle && attr_id == ATTR_ID_EXT_BRCM_VERSION) {
                sdp_cb.server_db.brcm_di_registered = TRUE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

/* 0x804A0AF0 (0xD8): Adds a sequence of 16-bit UUIDs as one attribute of a local record. */
u8 SDP_AddUuidSequence(u32 handle, u16 attr_id, u16 num_uuids, u16* p_uuids) {
    u16 xx;
    u8 buff[SDP_MAX_ATTR_LEN * 2];
    u8* p = buff;
    s32 max_len = SDP_MAX_ATTR_LEN - 3;

    for (xx = 0; xx < num_uuids; xx++, p_uuids++) {
        UINT8_TO_BE_STREAM(p, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
        UINT16_TO_BE_STREAM(p, *p_uuids);

        if ((p - buff) > max_len) {
            SDP_TRACE_WARNING2("SDP_AddUuidSequence - too long, add %d uuids of %d", xx, num_uuids);
            break;
        }
    }

    return SDP_AddAttribute(handle, attr_id, DATA_ELE_SEQ_DESC_TYPE, (u32)(p - buff), buff);
}

/* 0x804A0BC8 (0x17C): Adds the service class id list attribute (16-bit UUIDs) to a local record. */
u8 SDP_AddServiceClassIdList(u32 handle, u16 num_services, u16* p_service_uuids) {
    u16 xx;
    u8 buff[SDP_MAX_ATTR_LEN * 2];
    u8* p = buff;

    for (xx = 0; xx < num_services; xx++, p_service_uuids++) {
        UINT8_TO_BE_STREAM(p, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
        UINT16_TO_BE_STREAM(p, *p_service_uuids);
    }

    return SDP_AddAttribute(handle, ATTR_ID_SERVICE_CLASS_ID_LIST, DATA_ELE_SEQ_DESC_TYPE, (u32)(p - buff), buff);
}

/* 0x804A0D44 (0x23C): Deletes an attribute of a local record and compacts the record's value pad. */
u8 SDP_DeleteAttribute(u32 handle, u16 attr_id) {
    u16 xx;
    u16 yy;
    tSDP_RECORD* p_rec = &sdp_cb.server_db.record[0];
    u8* pad_ptr;
    u32 len;

    for (xx = 0; xx < sdp_cb.server_db.num_records; xx++, p_rec++) {
        if (p_rec->record_handle == handle) {
            tSDP_ATTRIBUTE* p_attr = &p_rec->attribute[0];

            SDP_TRACE_API2("Deleting attr_id 0x%04x for handle 0x%x", attr_id, handle);

            for (xx = 0; xx < p_rec->num_attributes; xx++, p_attr++) {
                if (p_attr->id == attr_id) {
                    pad_ptr = p_attr->value_ptr;
                    len = p_attr->len;

                    if (len) {
                        for (yy = 0; yy < p_rec->num_attributes; yy++) {
                            if (p_rec->attribute[yy].value_ptr > pad_ptr) {
                                p_rec->attribute[yy].value_ptr -= len;
                            }
                        }
                    }

                    p_rec->num_attributes--;

                    for (yy = xx; yy < p_rec->num_attributes; yy++, p_attr++) {
                        *p_attr = *(p_attr + 1);
                    }

                    if (len) {
                        xx = (p_rec->free_pad_ptr - ((pad_ptr + len) - (u8*)&p_rec->attr_pad[0]));
                        for (yy = 0; yy < xx; yy++, pad_ptr++) {
                            *pad_ptr = *(pad_ptr + len);
                        }
                        p_rec->free_pad_ptr -= len;
                    }
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}
