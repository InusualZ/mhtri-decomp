/*
 * BTE/sdp_utils.c - the SDP utilities: CCB lookup, allocation and release, attribute sequence building, error responses,
 *   UID and attribute sequence extraction, UUID comparison and attribute list sorting.
 *
 * RANGE. .text 0x804A3A74-0x804A4AC0 (13 functions, 0x104C B); .rodata 0x80573998-0x805739A8; .data
 *   0x806183E8-0x806184E0. The .text holds the BTE sdp_utils.c functions sdpu_find_ccb_by_cid..sdpu_sort_attr_list and
 *   a 12 B link pad before DB/db.c's 16-aligned DBInit (0x804A4AC0). The .rodata is the 16 B Bluetooth base UUID; the
 *   .data opens with a jump table and is read only by this range's functions, no pad beyond 4-byte alignment.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`), per object in configure.py,
 *   for all six SDP units. Wii/1.3 `cflags_os` lowers the switches here to compare chains (no jump tables: .data 116 B
 *   for the target's 248 B) and unrolls differently (sdp_db .text 4476 B for 3276 B); Wii/1.0, Wii/1.1 and GC/2.7
 *   measure no better. `-ipa file` is what lets sdp_main.c's sdp_config_cfm inline sdp_disconnect, defined after it;
 *   `#pragma defer_codegen` inlines it too but emits the functions in reverse order.
 * NAMES. the map's, which are the BTE source's: sdpu_find_ccb_by_cid (0x804A3A74) opens the BTE sdp_utils.c and every
 *   L2CAP callback of sdp_main.c calls it with the channel id; sdp_base_uuid (.rodata 0x80573998) is a GUESS from the
 *   BTE source's name for the base UUID.
 * RESIDUALS. jump-table and string relocations name the compiler's local pool where the map has jumptable_/lbl_
 *   labels; the 12 B tail is link padding.
 * SHAPES. plain C; the trace statements and stream macros are BTE/sdp_int.h's.
 */

#include "BTE/sdp_utils.h"
#include "BTE/sdp_main.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* .rodata 0x80573998 - the Bluetooth base UUID a 16- or 32-bit UUID is short for. */
const u8 sdp_base_uuid[MAX_UUID_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                                         0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

/* 0x804A3A74 (0x90): Returns the open CCB of the L2CAP channel `cid`, or NULL. */
tCONN_CB* sdpu_find_ccb_by_cid(u16 cid) {
    u16 xx;
    tCONN_CB* p_ccb;

    for (xx = 0, p_ccb = sdp_cb.ccb; xx < SDP_MAX_CONNECTIONS; xx++, p_ccb++) {
        if (p_ccb->con_state != SDP_STATE_IDLE && p_ccb->connection_id == cid) {
            return p_ccb;
        }
    }
    return NULL;
}

/* 0x804A3B04 (0x74): Returns a cleared idle CCB, or NULL when all are in use. */
tCONN_CB* sdpu_allocate_ccb(void) {
    u16 xx;
    tCONN_CB* p_ccb;

    for (xx = 0, p_ccb = sdp_cb.ccb; xx < SDP_MAX_CONNECTIONS; xx++, p_ccb++) {
        if (p_ccb->con_state == SDP_STATE_IDLE) {
            memset(p_ccb, 0, sizeof(tCONN_CB));
            p_ccb->timer_entry.param = (u32)p_ccb;
            return p_ccb;
        }
    }
    return NULL;
}

/* 0x804A3B78 (0x38): Stops the CCB's timer and marks it idle. */
void sdpu_release_ccb(tCONN_CB* p_ccb) {
    btu_stop_timer(&p_ccb->timer_entry);
    p_ccb->con_state = SDP_STATE_IDLE;
}

/* 0x804A3BB0 (0x1D0): Writes the attribute id list of a request (every id when `p_attr` is NULL); returns the end. */
u8* sdpu_build_attrib_seq(u8* p_out, u16* p_attr, u16 num_attrs) {
    u16 xx;

    if (p_attr) {
        xx = num_attrs * 3;
    } else {
        xx = 5;
    }

    if (xx > 255) {
        UINT8_TO_BE_STREAM(p_out, (DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_WORD);
        UINT16_TO_BE_STREAM(p_out, xx);
    } else {
        UINT8_TO_BE_STREAM(p_out, (DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
        UINT8_TO_BE_STREAM(p_out, xx);
    }

    if (p_attr == NULL) {
        UINT8_TO_BE_STREAM(p_out, (UINT_DESC_TYPE << 3) | SIZE_FOUR_BYTES);
        UINT16_TO_BE_STREAM(p_out, 0);
        UINT16_TO_BE_STREAM(p_out, 0xFFFF);
    } else {
        for (xx = 0; xx < num_attrs; xx++, p_attr++) {
            UINT8_TO_BE_STREAM(p_out, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
            UINT16_TO_BE_STREAM(p_out, *p_attr);
        }
    }

    return p_out;
}

/* 0x804A3D80 (0x154): Writes one attribute (id and typed value) of a local record; returns the end. */
u8* sdpu_build_attrib_entry(u8* p_out, tSDP_ATTRIBUTE* p_attr) {
    UINT8_TO_BE_STREAM(p_out, (UINT_DESC_TYPE << 3) | SIZE_TWO_BYTES);
    UINT16_TO_BE_STREAM(p_out, p_attr->id);

    switch (p_attr->type) {
    case TEXT_STR_DESC_TYPE:
    case DATA_ELE_SEQ_DESC_TYPE:
    case DATA_ELE_ALT_DESC_TYPE:
    case URL_DESC_TYPE:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_IN_NEXT_BYTE);
        UINT8_TO_BE_STREAM(p_out, p_attr->len);
        ARRAY_TO_BE_STREAM(p_out, p_attr->value_ptr, (int)p_attr->len);
        return p_out;
    }

    switch (p_attr->len) {
    case 1:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_ONE_BYTE);
        break;
    case 2:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_TWO_BYTES);
        break;
    case 4:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_FOUR_BYTES);
        break;
    case 8:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_EIGHT_BYTES);
        break;
    case 16:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_SIXTEEN_BYTES);
        break;
    default:
        UINT8_TO_BE_STREAM(p_out, (p_attr->type << 3) | SIZE_IN_NEXT_BYTE);
        UINT8_TO_BE_STREAM(p_out, p_attr->len);
        break;
    }

    ARRAY_TO_BE_STREAM(p_out, p_attr->value_ptr, (int)p_attr->len);
    return p_out;
}

/* 0x804A3ED4 (0x13C): Sends an SDP error response with `error_code` and the optional text. */
void sdpu_build_n_send_error(tCONN_CB* p_ccb, u16 trans_num, u16 error_code, char* p_error_text) {
    u8* p_rsp;
    u8* p_rsp_start;
    u8* p_rsp_param_len;
    u16 rsp_param_len;
    BT_HDR* p_buf;

    SDP_TRACE_WARNING2("SDP - sdpu_build_n_send_error  code: 0x%x  CID: 0x%x", error_code, p_ccb->connection_id);

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID)) == NULL) {
        SDP_TRACE_ERROR0("SDP - no buf for err msg");
        return;
    }
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_rsp = p_rsp_start = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    UINT8_TO_BE_STREAM(p_rsp, SDP_PDU_ERROR_RESPONSE);
    UINT16_TO_BE_STREAM(p_rsp, trans_num);

    p_rsp_param_len = p_rsp;
    p_rsp += 2;

    UINT16_TO_BE_STREAM(p_rsp, error_code);

    if (p_error_text) {
        ARRAY_TO_BE_STREAM(p_rsp, p_error_text, (int)strlen(p_error_text));
    }

    rsp_param_len = p_rsp - p_rsp_param_len - 2;
    UINT16_TO_BE_STREAM(p_rsp_param_len, rsp_param_len);

    p_buf->len = p_rsp - p_rsp_start;

    L2CA_DataWrite(p_ccb->connection_id, p_buf);
}

/* 0x804A4010 (0x350): Parses a request's UUID sequence into `p_seq`; returns the byte after it, or NULL. */
u8* sdpu_extract_uid_seq(u8* p, u16 param_len, tSDP_UUID_SEQ* p_seq) {
    u8 type;
    u8 size;
    u32 seq_len;
    u32 uuid_len;
    u8* p_seq_end;

    p_seq->num_uids = 0;

    BE_STREAM_TO_UINT8(type, p);
    size = type & 7;
    type = type >> 3;
    if (type != DATA_ELE_SEQ_DESC_TYPE) {
        return NULL;
    }

    switch (size) {
    case SIZE_TWO_BYTES:
        seq_len = 2;
        break;
    case SIZE_FOUR_BYTES:
        seq_len = 4;
        break;
    case SIZE_SIXTEEN_BYTES:
        seq_len = 16;
        break;
    case SIZE_IN_NEXT_BYTE:
        BE_STREAM_TO_UINT8(seq_len, p);
        break;
    case SIZE_IN_NEXT_WORD:
        BE_STREAM_TO_UINT16(seq_len, p);
        break;
    case SIZE_IN_NEXT_LONG:
        BE_STREAM_TO_UINT32(seq_len, p);
        break;
    default:
        return NULL;
    }

    if (seq_len >= param_len) {
        return NULL;
    }

    p_seq_end = p + seq_len;

    while (p < p_seq_end) {
        BE_STREAM_TO_UINT8(type, p);
        size = type & 7;
        type = type >> 3;
        if (type != UUID_DESC_TYPE) {
            return NULL;
        }

        switch (size) {
        case SIZE_TWO_BYTES:
            uuid_len = 2;
            break;
        case SIZE_FOUR_BYTES:
            uuid_len = 4;
            break;
        case SIZE_SIXTEEN_BYTES:
            uuid_len = 16;
            break;
        case SIZE_IN_NEXT_BYTE:
            BE_STREAM_TO_UINT8(uuid_len, p);
            break;
        case SIZE_IN_NEXT_WORD:
            BE_STREAM_TO_UINT16(uuid_len, p);
            break;
        case SIZE_IN_NEXT_LONG:
            BE_STREAM_TO_UINT32(uuid_len, p);
            break;
        default:
            return NULL;
        }

        if (uuid_len == 2 || uuid_len == 4 || uuid_len == 16) {
            p_seq->uuid_entry[p_seq->num_uids].len = uuid_len;
            BE_STREAM_TO_ARRAY(p, p_seq->uuid_entry[p_seq->num_uids].value, (int)uuid_len);
            p_seq->num_uids++;
        } else {
            return NULL;
        }

        if (p_seq->num_uids >= MAX_UUIDS_PER_SEQ) {
            return NULL;
        }
    }

    if (p != p_seq_end) {
        return NULL;
    }

    return p;
}

/* 0x804A4360 (0x250): Parses a request's attribute id list into `p_seq`; returns the byte after it, or NULL. */
u8* sdpu_extract_attr_seq(u8* p, u16 param_len, tSDP_ATTR_SEQ* p_seq) {
    u8* p_end_list;
    u8 descr;
    u8 type;
    u8 size;
    u32 list_len;
    u32 attr_len;

    p_seq->num_attr = 0;

    BE_STREAM_TO_UINT8(descr, p);
    type = descr >> 3;
    size = descr & 7;

    if (type != DATA_ELE_SEQ_DESC_TYPE) {
        return p;
    }

    switch (size) {
    case SIZE_IN_NEXT_BYTE:
        BE_STREAM_TO_UINT8(list_len, p);
        break;
    case SIZE_IN_NEXT_WORD:
        BE_STREAM_TO_UINT16(list_len, p);
        break;
    case SIZE_IN_NEXT_LONG:
        BE_STREAM_TO_UINT32(list_len, p);
        break;
    default:
        return p;
    }

    if (list_len > param_len) {
        return p;
    }

    p_end_list = p + list_len;

    while (p < p_end_list) {
        BE_STREAM_TO_UINT8(descr, p);
        type = descr >> 3;
        size = descr & 7;

        if (type != UINT_DESC_TYPE) {
            return p;
        }

        switch (size) {
        case SIZE_TWO_BYTES:
            attr_len = 2;
            break;
        case SIZE_FOUR_BYTES:
            attr_len = 4;
            break;
        case SIZE_IN_NEXT_BYTE:
            BE_STREAM_TO_UINT8(attr_len, p);
            break;
        case SIZE_IN_NEXT_WORD:
            BE_STREAM_TO_UINT16(attr_len, p);
            break;
        case SIZE_IN_NEXT_LONG:
            BE_STREAM_TO_UINT32(attr_len, p);
            break;
        default:
            return NULL;
        }

        if (attr_len == 2) {
            BE_STREAM_TO_UINT16(p_seq->attr_entry[p_seq->num_attr].start, p);
            p_seq->attr_entry[p_seq->num_attr].end = p_seq->attr_entry[p_seq->num_attr].start;
        } else if (attr_len == 4) {
            BE_STREAM_TO_UINT16(p_seq->attr_entry[p_seq->num_attr].start, p);
            BE_STREAM_TO_UINT16(p_seq->attr_entry[p_seq->num_attr].end, p);
        } else {
            return NULL;
        }

        if (++p_seq->num_attr >= MAX_ATTR_PER_SEQ) {
            return NULL;
        }
    }

    return p;
}

/* 0x804A45B0 (0xD0): Stores the length a type byte's size index gives; returns the byte after the length field. */
u8* sdpu_get_len_from_type(u8* p, u8 type, u32* p_len) {
    u8 len8;
    u16 len16;
    u32 len32;

    switch (type & 7) {
    case SIZE_ONE_BYTE:
        *p_len = 1;
        break;
    case SIZE_TWO_BYTES:
        *p_len = 2;
        break;
    case SIZE_FOUR_BYTES:
        *p_len = 4;
        break;
    case SIZE_EIGHT_BYTES:
        *p_len = 8;
        break;
    case SIZE_SIXTEEN_BYTES:
        *p_len = 16;
        break;
    case SIZE_IN_NEXT_BYTE:
        BE_STREAM_TO_UINT8(len8, p);
        *p_len = len8;
        break;
    case SIZE_IN_NEXT_WORD:
        BE_STREAM_TO_UINT16(len16, p);
        *p_len = len16;
        break;
    case SIZE_IN_NEXT_LONG:
        BE_STREAM_TO_UINT32(len32, p);
        *p_len = (u16)len32;
        break;
    }

    return p;
}

/* 0x804A4680 (0xE0): Returns whether a 128-bit UUID's last 12 bytes are the base UUID's. */
u8 sdpu_is_base_uuid(u8* p_uuid) {
    u16 xx;

    for (xx = 4; xx < MAX_UUID_SIZE; xx++) {
        if (p_uuid[xx] != sdp_base_uuid[xx]) {
            return FALSE;
        }
    }

    return TRUE;
}

/* 0x804A4760 (0x274): Compares two UUIDs of any of the three lengths, widening the shorter one. */
u8 sdpu_compare_uuid_arrays(u8* p_uuid1, u32 len1, u8* p_uuid2, u16 len2) {
    u8 nu1[MAX_UUID_SIZE];
    u8 nu2[MAX_UUID_SIZE];

    if (len1 == len2) {
        if (len1 == 2) {
            return (p_uuid1[0] == p_uuid2[0]) && (p_uuid1[1] == p_uuid2[1]);
        }
        if (len1 == 4) {
            return (p_uuid1[0] == p_uuid2[0]) && (p_uuid1[1] == p_uuid2[1]) && (p_uuid1[2] == p_uuid2[2]) &&
                   (p_uuid1[3] == p_uuid2[3]);
        } else {
            return memcmp(p_uuid1, p_uuid2, len1) == 0;
        }
    } else if (len1 > len2) {
        if (len1 == 4) {
            return (p_uuid1[0] == 0) && (p_uuid1[1] == 0) && (p_uuid1[2] == p_uuid2[0]) && (p_uuid1[3] == p_uuid2[1]);
        } else {
            memcpy(nu1, p_uuid1, MAX_UUID_SIZE);
            memcpy(nu2, sdp_base_uuid, MAX_UUID_SIZE);

            if (len2 == 4) {
                memcpy(nu2, p_uuid2, len2);
            } else {
                memcpy(nu2 + 2, p_uuid2, len2);
            }

            return memcmp(nu1, nu2, MAX_UUID_SIZE) == 0;
        }
    } else {
        if (len2 == 4) {
            return (p_uuid2[0] == 0) && (p_uuid2[1] == 0) && (p_uuid2[2] == p_uuid1[0]) && (p_uuid2[3] == p_uuid1[1]);
        } else {
            memcpy(nu2, p_uuid2, MAX_UUID_SIZE);
            memcpy(nu1, sdp_base_uuid, MAX_UUID_SIZE);

            if (len1 == 4) {
                memcpy(nu1, p_uuid1, len1);
            } else {
                memcpy(nu1 + 2, p_uuid1, len1);
            }

            return memcmp(nu1, nu2, MAX_UUID_SIZE) == 0;
        }
    }
}

/* 0x804A49D4 (0x90): Returns whether a discovered UUID attribute equals the UUID `p_btuuid`. */
u8 sdpu_compare_uuid_with_attr(tSDP_UUID* p_btuuid, tSDP_DISC_ATTR* p_attr) {
    u16 attr_len = p_attr->attr_len_type & 0xFFF;

    if (p_btuuid->len != attr_len) {
        return FALSE;
    }

    if (p_btuuid->len == 2) {
        return p_btuuid->uu.uuid16 == p_attr->attr_value.uint16;
    } else if (p_btuuid->len == 4) {
        return p_btuuid->uu.uuid32 == p_attr->attr_value.uint32;
    } else {
        return memcmp(p_btuuid->uu.uuid128, p_attr->attr_value.array, MAX_UUID_SIZE) == 0;
    }
}

/* 0x804A4A64 (0x50): Sorts the discovery database's attribute filter ids into ascending order. */
void sdpu_sort_attr_list(u16 num_attr, tSDP_DISCOVERY_DB* p_db) {
    u16 i;
    u16 x;

    if (num_attr > 1) {
        for (i = 0; i < num_attr - 1; i++) {
            if (p_db->attr_filters[i] > p_db->attr_filters[i + 1]) {
                x = p_db->attr_filters[i];
                p_db->attr_filters[i] = p_db->attr_filters[i + 1];
                p_db->attr_filters[i + 1] = x;
                i = (u16)-1;
            }
        }
    }
}
