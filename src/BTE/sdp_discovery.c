/*
 * BTE/sdp_discovery.c - the SDP client discovery: request building, the server response dispatch and the response
 *   parsers that fill the discovery database.
 *
 * RANGE. .text 0x804A0F80-0x804A21F8 (10 functions, 0x1278 B); .data 0x80617EE8-0x80618048. The .text holds the BTE
 *   sdp_discovery.c functions sdpu_build_uuid_seq..add_attr; sdp_main.c's sdp_init follows. The .data is the trace
 *   format strings and add_attr's jump table; no pad beyond 4-byte alignment.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`), per object in configure.py;
 *   the evidence is in BTE/sdp_utils.c's header. The function starts of this band are packed to 4 bytes.
 * NAMES. the map's, which are the BTE source's; sdp_disc_connected (0x804A119C) is a GUESS from the BTE source's
 *   name and place (sdp_main.c's configuration callbacks call it once both sides are configured).
 * RESIDUALS. string and jump-table relocations name the compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C. The three response parsers take the reply length as a third parameter none of them reads (the
 *   callers pass it); add_attr carries the "additional protocol list" flag in bit 7 of its nest level.
 */

#include "BTE/sdp_discovery.h"
#include "BTE/sdp_main.h"
#include "BTE/sdp_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The nest-level bit add_attr sets inside an additional protocol descriptor list, and that list's attribute id. */
#define SDP_ADDITIONAL_LIST_MASK 0x80
#define ATTR_ID_ADDITION_PROTO_DESC_LISTS 0x000D
#define SDP_MAX_CONTINUATION_LEN 16
#define SDP_DISC_ATTR_LEN_MASK 0x0FFF
#define BD_ADDR_LEN 6

static void process_service_search_rsp(tCONN_CB* p_ccb, u8* p_reply, u16 reply_len);
static void process_service_attr_rsp(tCONN_CB* p_ccb, u8* p_reply, u16 reply_len);
static void process_service_search_attr_rsp(tCONN_CB* p_ccb, u8* p_reply, u16 reply_len);
static u8* save_attr_seq(tCONN_CB* p_ccb, u8* p, u8* p_msg_end);
static tSDP_DISC_REC* add_record(tSDP_DISCOVERY_DB* p_db, u8* p_bda);
static u8* add_attr(u8* p, tSDP_DISCOVERY_DB* p_db, tSDP_DISC_REC* p_rec, u16 attr_id, tSDP_DISC_ATTR* p_parent_attr,
                    u8 nest_level);

/* 0x804A0F80 (0xEC): Writes the request's UUID sequence; returns the end. */
static u8* sdpu_build_uuid_seq(u8* p_out, u16 num_uuids, tSDP_UUID* p_uuid_list) {
    u16 xx;
    u8* p_len;

    UINT8_TO_BE_STREAM(p_out, (DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);

    p_len = p_out;
    p_out += 1;

    for (xx = 0; xx < num_uuids; xx++, p_uuid_list++) {
        if (p_uuid_list->len == 2) {
            UINT8_TO_BE_STREAM(p_out, (UUID_DESC_TYPE << 3) | SIZE_TWO_BYTES);
            UINT16_TO_BE_STREAM(p_out, p_uuid_list->uu.uuid16);
        } else if (p_uuid_list->len == 4) {
            UINT8_TO_BE_STREAM(p_out, (UUID_DESC_TYPE << 3) | SIZE_FOUR_BYTES);
            UINT32_TO_BE_STREAM(p_out, p_uuid_list->uu.uuid32);
        } else {
            UINT8_TO_BE_STREAM(p_out, (UUID_DESC_TYPE << 3) | SIZE_SIXTEEN_BYTES);
            ARRAY_TO_BE_STREAM(p_out, p_uuid_list->uu.uuid128, p_uuid_list->len);
        }
    }

    *p_len = (u8)(p_out - p_len - 1);

    return p_out;
}

/* 0x804A106C (0x130): Sends a service search request (with the continuation state, if any). */
static void sdp_snd_service_search_req(tCONN_CB* p_ccb, u8 cont_len, u8* p_cont) {
    u8* p;
    u8* p_start;
    u8* p_param_len;
    BT_HDR* p_cmd;
    u16 param_len;

    if ((p_cmd = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID)) == NULL) {
        sdp_disconnect(p_ccb, SDP_NO_RESOURCES);
        return;
    }

    p_cmd->offset = L2CAP_MIN_OFFSET;
    p = p_start = &((BT_BUF*)p_cmd)->data[L2CAP_MIN_OFFSET];

    UINT8_TO_BE_STREAM(p, SDP_PDU_SERVICE_SEARCH_REQ);
    UINT16_TO_BE_STREAM(p, p_ccb->transaction_id);
    p_ccb->transaction_id++;

    p_param_len = p;
    p += 2;

    p = sdpu_build_uuid_seq(p, p_ccb->p_db->num_uuid_filters, p_ccb->p_db->uuid_filters);

    UINT16_TO_BE_STREAM(p, sdp_cb.max_recs_per_search);

    UINT8_TO_BE_STREAM(p, cont_len);

    if (cont_len && p_cont) {
        memcpy(p, p_cont, cont_len);
        p += cont_len;
    }

    param_len = (u16)(p - p_param_len - 2);
    UINT16_TO_BE_STREAM(p_param_len, param_len);

    p_ccb->disc_state = SDP_DISC_WAIT_HANDLES;

    p_cmd->len = (u16)(p - p_start);

    L2CA_DataWrite(p_ccb->connection_id, p_cmd);

    btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_SDP, SDP_INACT_TIMEOUT);
}

/* 0x804A119C (0x34): Starts the first request once the client channel is configured. */
void sdp_disc_connected(tCONN_CB* p_ccb) {
    if (p_ccb->is_attr_search) {
        p_ccb->disc_state = SDP_DISC_WAIT_SEARCH_ATTR;

        process_service_search_attr_rsp(p_ccb, NULL, 0);
    } else {
        p_ccb->num_handles = 0;
        sdp_snd_service_search_req(p_ccb, 0, NULL);
    }
}

/* 0x804A11D0 (0x140): Dispatches a server response to the parser its PDU and the discovery state expect. */
void sdp_disc_server_rsp(tCONN_CB* p_ccb, BT_HDR* p_msg) {
    u8* p;
    u8 rsp_pdu;
    u8 invalid_pdu = TRUE;

    btu_stop_timer(&p_ccb->timer_entry);

    p = &((BT_BUF*)p_msg)->data[p_msg->offset];

    BE_STREAM_TO_UINT8(rsp_pdu, p);

    p_msg->len--;

    switch (rsp_pdu) {
    case SDP_PDU_SERVICE_SEARCH_RSP:
        if (p_ccb->disc_state == SDP_DISC_WAIT_HANDLES) {
            process_service_search_rsp(p_ccb, p, p_msg->len);
            invalid_pdu = FALSE;
        }
        break;

    case SDP_PDU_SERVICE_ATTR_RSP:
        if (p_ccb->disc_state == SDP_DISC_WAIT_ATTR) {
            process_service_attr_rsp(p_ccb, p, p_msg->len);
            invalid_pdu = FALSE;
        }
        break;

    case SDP_PDU_SERVICE_SEARCH_ATTR_RSP:
        if (p_ccb->disc_state == SDP_DISC_WAIT_SEARCH_ATTR) {
            process_service_search_attr_rsp(p_ccb, p, p_msg->len);
            invalid_pdu = FALSE;
        }
        break;
    }

    if (invalid_pdu) {
        SDP_TRACE_WARNING2("SDP - Unexp. PDU: %d in state: %d", rsp_pdu, p_ccb->disc_state);
        sdp_disconnect(p_ccb, SDP_GENERIC_ERROR);
    }
}

/* 0x804A1310 (0x148): Stores the record handles of a search response and asks for more or for the first attributes. */
static void process_service_search_rsp(tCONN_CB* p_ccb, u8* p_reply, u16 reply_len) {
    u16 xx;
    u16 total;
    u16 cur_handles;
    u16 orig;
    u8 cont_len;

    p_reply += 4;
    BE_STREAM_TO_UINT16(total, p_reply);
    BE_STREAM_TO_UINT16(cur_handles, p_reply);

    orig = p_ccb->num_handles;
    p_ccb->num_handles += cur_handles;
    if (p_ccb->num_handles == 0) {
        SDP_TRACE_WARNING0("SDP - Rcvd ServiceSearchRsp, no matches");
        sdp_disconnect(p_ccb, SDP_NO_RECS_MATCH);
        return;
    }

    if (total > sdp_cb.max_recs_per_search) {
        total = sdp_cb.max_recs_per_search;
    }
    if (p_ccb->num_handles > sdp_cb.max_recs_per_search) {
        p_ccb->num_handles = sdp_cb.max_recs_per_search;
    }

    for (xx = orig; xx < p_ccb->num_handles; xx++) {
        BE_STREAM_TO_UINT32(p_ccb->handles[xx], p_reply);
    }

    BE_STREAM_TO_UINT8(cont_len, p_reply);
    if (cont_len != 0) {
        if (cont_len > SDP_MAX_CONTINUATION_LEN) {
            sdp_disconnect(p_ccb, SDP_INVALID_CONT_STATE);
            return;
        }
        sdp_snd_service_search_req(p_ccb, cont_len, p_reply);
    } else {
        p_ccb->disc_state = SDP_DISC_WAIT_ATTR;

        process_service_attr_rsp(p_ccb, NULL, 0);
    }
}

/* 0x804A1458 (0x2A8): Collects an attribute response, saves a complete one and asks for the next record's attributes. */
static void process_service_attr_rsp(tCONN_CB* p_ccb, u8* p_reply, u16 reply_len) {
    u8* p_start;
    u8* p_param_len;
    u16 param_len;
    u16 list_byte_count;
    u8 cont_request_needed = FALSE;

    if (p_reply) {
        p_reply += 4;
        BE_STREAM_TO_UINT16(list_byte_count, p_reply);

        if ((p_ccb->list_len + list_byte_count) > SDP_MAX_LIST_BYTE_COUNT) {
            sdp_disconnect(p_ccb, SDP_INVALID_PDU_SIZE);
            return;
        }

        memcpy(&p_ccb->rsp_list[p_ccb->list_len], p_reply, list_byte_count);
        p_ccb->list_len += list_byte_count;
        p_reply += list_byte_count;

        if (*p_reply) {
            if (*p_reply > SDP_MAX_CONTINUATION_LEN) {
                sdp_disconnect(p_ccb, SDP_INVALID_CONT_STATE);
                return;
            }
            cont_request_needed = TRUE;
        } else {
            if (!save_attr_seq(p_ccb, &p_ccb->rsp_list[0], &p_ccb->rsp_list[p_ccb->list_len])) {
                sdp_disconnect(p_ccb, SDP_DB_FULL);
                return;
            }

            p_ccb->list_len = 0;
            p_ccb->cur_handle++;
        }
    }

    if (p_ccb->cur_handle < p_ccb->num_handles) {
        BT_HDR* p_msg = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID);
        u8* p;

        if (!p_msg) {
            sdp_disconnect(p_ccb, SDP_NO_RESOURCES);
            return;
        }

        p_msg->offset = L2CAP_MIN_OFFSET;
        p = p_start = &((BT_BUF*)p_msg)->data[L2CAP_MIN_OFFSET];

        UINT8_TO_BE_STREAM(p, SDP_PDU_SERVICE_ATTR_REQ);
        UINT16_TO_BE_STREAM(p, p_ccb->transaction_id);
        p_ccb->transaction_id++;

        p_param_len = p;
        p += 2;

        UINT32_TO_BE_STREAM(p, p_ccb->handles[p_ccb->cur_handle]);

        UINT16_TO_BE_STREAM(p, sdp_cb.max_attr_list_size);

        if (p_ccb->p_db->num_attr_filters) {
            p = sdpu_build_attrib_seq(p, p_ccb->p_db->attr_filters, p_ccb->p_db->num_attr_filters);
        } else {
            p = sdpu_build_attrib_seq(p, NULL, 0);
        }

        if (cont_request_needed) {
            memcpy(p, p_reply, *p_reply + 1);
            p += *p_reply + 1;
        } else {
            UINT8_TO_BE_STREAM(p, 0);
        }

        param_len = (u16)(p - p_param_len - 2);
        UINT16_TO_BE_STREAM(p_param_len, param_len);

        p_msg->len = (u16)(p - p_start);

        L2CA_DataWrite(p_ccb->connection_id, p_msg);

        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_SDP, SDP_INACT_TIMEOUT);
    } else {
        sdp_disconnect(p_ccb, SDP_SUCCESS);
        return;
    }
}

/* 0x804A1700 (0x2D4): Collects a search-attribute response, asks for its continuation, then saves every record in it. */
static void process_service_search_attr_rsp(tCONN_CB* p_ccb, u8* p_reply, u16 reply_len) {
    u8* p;
    u8* p_start;
    u8* p_end;
    u8* p_param_len;
    u8 type;
    u32 seq_len;
    u16 param_len;
    u16 lists_byte_count = 0;
    u8 cont_request_needed = FALSE;

    if (p_reply) {
        p_reply += 4;
        BE_STREAM_TO_UINT16(lists_byte_count, p_reply);

        if ((p_ccb->list_len + lists_byte_count) > SDP_MAX_LIST_BYTE_COUNT) {
            sdp_disconnect(p_ccb, SDP_INVALID_PDU_SIZE);
            return;
        }

        memcpy(&p_ccb->rsp_list[p_ccb->list_len], p_reply, lists_byte_count);
        p_ccb->list_len += lists_byte_count;
        p_reply += lists_byte_count;

        if (*p_reply) {
            if (*p_reply > SDP_MAX_CONTINUATION_LEN) {
                sdp_disconnect(p_ccb, SDP_INVALID_CONT_STATE);
                return;
            }

            cont_request_needed = TRUE;
        }
    }

    if (cont_request_needed || !p_reply) {
        BT_HDR* p_msg = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID);
        u8* p;

        if (!p_msg) {
            sdp_disconnect(p_ccb, SDP_NO_RESOURCES);
            return;
        }

        p_msg->offset = L2CAP_MIN_OFFSET;
        p = p_start = &((BT_BUF*)p_msg)->data[L2CAP_MIN_OFFSET];

        UINT8_TO_BE_STREAM(p, SDP_PDU_SERVICE_SEARCH_ATTR_REQ);
        UINT16_TO_BE_STREAM(p, p_ccb->transaction_id);
        p_ccb->transaction_id++;

        p_param_len = p;
        p += 2;

        p = sdpu_build_uuid_seq(p, p_ccb->p_db->num_uuid_filters, p_ccb->p_db->uuid_filters);

        UINT16_TO_BE_STREAM(p, sdp_cb.max_attr_list_size);

        if (p_ccb->p_db->num_attr_filters) {
            p = sdpu_build_attrib_seq(p, p_ccb->p_db->attr_filters, p_ccb->p_db->num_attr_filters);
        } else {
            p = sdpu_build_attrib_seq(p, NULL, 0);
        }

        if (p_reply) {
            memcpy(p, p_reply, *p_reply + 1);
            p += *p_reply + 1;
        } else {
            UINT8_TO_BE_STREAM(p, 0);
        }

        param_len = (u16)(p - p_param_len - 2);
        UINT16_TO_BE_STREAM(p_param_len, param_len);

        p_msg->len = (u16)(p - p_start);

        L2CA_DataWrite(p_ccb->connection_id, p_msg);

        btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_SDP, SDP_INACT_TIMEOUT);

        return;
    }

    p = &p_ccb->rsp_list[0];

    type = *p++;

    if ((type >> 3) != DATA_ELE_SEQ_DESC_TYPE) {
        SDP_TRACE_WARNING1("SDP - Wrong type: 0x%02x in attr_rsp", type);
        return;
    }
    p = sdpu_get_len_from_type(p, type, &seq_len);

    p_end = &p_ccb->rsp_list[p_ccb->list_len];

    if ((p + seq_len) != p_end) {
        sdp_disconnect(p_ccb, SDP_INVALID_CONT_STATE);
        return;
    }

    while (p < p_end) {
        p = save_attr_seq(p_ccb, p, &p_ccb->rsp_list[p_ccb->list_len]);
        if (!p) {
            sdp_disconnect(p_ccb, SDP_DB_FULL);
            return;
        }
    }

    sdp_disconnect(p_ccb, SDP_SUCCESS);
}

/* 0x804A19D4 (0x1EC): Adds one record and its attributes from an attribute list; returns the byte after it, or NULL. */
static u8* save_attr_seq(tCONN_CB* p_ccb, u8* p, u8* p_msg_end) {
    u32 seq_len;
    u32 attr_len;
    u16 attr_id;
    u8 type;
    u8* p_seq_end;
    tSDP_DISC_REC* p_rec;

    type = *p++;

    if ((type >> 3) != DATA_ELE_SEQ_DESC_TYPE) {
        SDP_TRACE_WARNING1("SDP - Wrong type: 0x%02x in attr_rsp", type);
        return NULL;
    }

    p = sdpu_get_len_from_type(p, type, &seq_len);
    if ((p + seq_len) > p_msg_end) {
        SDP_TRACE_WARNING1("SDP - Bad len in attr_rsp %d", seq_len);
        return NULL;
    }

    p_rec = add_record(p_ccb->p_db, p_ccb->device_address);
    if (!p_rec) {
        SDP_TRACE_WARNING0("SDP - DB full");
        return NULL;
    }

    p_seq_end = p + seq_len;

    while (p < p_seq_end) {
        type = *p++;
        p = sdpu_get_len_from_type(p, type, &attr_len);
        if ((type >> 3) != UINT_DESC_TYPE || attr_len != 2) {
            SDP_TRACE_WARNING2("SDP - Bad type: 0x%02x or len: %d in attr_rsp", type, attr_len);
            return NULL;
        }
        BE_STREAM_TO_UINT16(attr_id, p);

        p = add_attr(p, p_ccb->p_db, p_rec, attr_id, NULL, 0);
        if (!p) {
            SDP_TRACE_WARNING0("SDP - DB full");
            return NULL;
        }
    }

    return p;
}

/* 0x804A1BC0 (0xA0): Appends an empty record for the device to the discovery database; NULL when it is full. */
static tSDP_DISC_REC* add_record(tSDP_DISCOVERY_DB* p_db, u8* p_bda) {
    tSDP_DISC_REC* p_rec;

    if (p_db->mem_free < sizeof(tSDP_DISC_REC)) {
        return NULL;
    }

    p_rec = (tSDP_DISC_REC*)p_db->p_free_mem;
    p_db->p_free_mem += sizeof(tSDP_DISC_REC);
    p_db->mem_free -= sizeof(tSDP_DISC_REC);

    p_rec->p_first_attr = NULL;
    p_rec->p_next_rec = NULL;

    memcpy(p_rec->remote_bd_addr, p_bda, BD_ADDR_LEN);

    if (p_db->p_first_rec == NULL) {
        p_db->p_first_rec = p_rec;
    } else {
        tSDP_DISC_REC* p_rec1 = p_db->p_first_rec;

        while (p_rec1->p_next_rec) {
            p_rec1 = p_rec1->p_next_rec;
        }

        p_rec1->p_next_rec = p_rec;
    }

    return p_rec;
}

/* 0x804A1C60 (0x598): Adds one attribute value (recursing into sequences) to a record or a parent; returns the byte after it. */
static u8* add_attr(u8* p, tSDP_DISCOVERY_DB* p_db, tSDP_DISC_REC* p_rec, u16 attr_id, tSDP_DISC_ATTR* p_parent_attr,
                    u8 nest_level) {
    tSDP_DISC_ATTR* p_attr;
    u32 attr_len;
    u32 total_len;
    u16 attr_type;
    u16 id;
    u8 type;
    u8* p_end;
    u8 is_additional_list = nest_level & SDP_ADDITIONAL_LIST_MASK;

    nest_level &= ~(SDP_ADDITIONAL_LIST_MASK);

    type = *p++;
    p = sdpu_get_len_from_type(p, type, &attr_len);

    attr_len &= SDP_DISC_ATTR_LEN_MASK;
    attr_type = (type >> 3) & 0x0F;

    if (attr_len > 4) {
        total_len = attr_len - 4 + (u16)sizeof(tSDP_DISC_ATTR);
    } else {
        total_len = sizeof(tSDP_DISC_ATTR);
    }

    total_len = (total_len + 3) & ~3;

    if (p_db->mem_free < total_len) {
        return NULL;
    }

    p_attr = (tSDP_DISC_ATTR*)p_db->p_free_mem;
    p_attr->attr_id = attr_id;
    p_attr->attr_len_type = (u16)attr_len | (attr_type << 12);
    p_attr->p_next_attr = NULL;

    switch (attr_type) {
    case UINT_DESC_TYPE:
        if (is_additional_list != 0 && attr_len == 2) {
            BE_STREAM_TO_UINT16(id, p);
            if (id != ATTR_ID_PROTOCOL_DESC_LIST) {
                p -= 2;
            } else {
                p_db->p_free_mem += sizeof(tSDP_DISC_ATTR);
                p_db->mem_free -= sizeof(tSDP_DISC_ATTR);
                p_end = p + attr_len;
                total_len = 0;

                if (nest_level >= MAX_NEST_LEVELS) {
                    SDP_TRACE_ERROR0("SDP - attr nesting too deep");
                    return p_end;
                }

                p = add_attr(p, p_db, p_rec, ATTR_ID_PROTOCOL_DESC_LIST, p_attr, (u8)(nest_level + 1));

                break;
            }
        }
        /* fall through */

    case TWO_COMP_INT_DESC_TYPE:
        switch (attr_len) {
        case 1:
            p_attr->attr_value.uint8 = *p++;
            break;
        case 2:
            BE_STREAM_TO_UINT16(p_attr->attr_value.uint16, p);
            break;
        case 4:
            BE_STREAM_TO_UINT32(p_attr->attr_value.uint32, p);
            break;
        default:
            BE_STREAM_TO_ARRAY(p, p_attr->attr_value.array, (s32)attr_len);
            break;
        }
        break;

    case UUID_DESC_TYPE:
        switch (attr_len) {
        case 2:
            BE_STREAM_TO_UINT16(p_attr->attr_value.uint16, p);
            break;
        case 4:
            BE_STREAM_TO_UINT32(p_attr->attr_value.uint32, p);
            if (p_attr->attr_value.uint32 < 0x10000) {
                attr_len = 2;
                p_attr->attr_len_type = (u16)attr_len | (attr_type << 12);
                p_attr->attr_value.uint16 = (u16)p_attr->attr_value.uint32;
            }
            break;
        case 16:
            if (sdpu_is_base_uuid(p)) {
                if (p[0] == 0 && p[1] == 0) {
                    p_attr->attr_len_type = (p_attr->attr_len_type & ~SDP_DISC_ATTR_LEN_MASK) | 2;
                    p += 2;
                    BE_STREAM_TO_UINT16(p_attr->attr_value.uint16, p);
                    p += MAX_UUID_SIZE - 4;
                } else {
                    p_attr->attr_len_type = (p_attr->attr_len_type & ~SDP_DISC_ATTR_LEN_MASK) | 4;
                    BE_STREAM_TO_UINT32(p_attr->attr_value.uint32, p);
                    p += MAX_UUID_SIZE - 4;
                }
            } else {
                BE_STREAM_TO_ARRAY(p, p_attr->attr_value.array, (s32)attr_len);
            }
            break;
        default:
            SDP_TRACE_WARNING1("SDP - bad len in UUID attr: %d", attr_len);
            return p + attr_len;
        }
        break;

    case DATA_ELE_SEQ_DESC_TYPE:
    case DATA_ELE_ALT_DESC_TYPE:
        p_db->p_free_mem += sizeof(tSDP_DISC_ATTR);
        p_db->mem_free -= sizeof(tSDP_DISC_ATTR);
        p_end = p + attr_len;
        total_len = 0;

        if (nest_level >= MAX_NEST_LEVELS) {
            SDP_TRACE_ERROR0("SDP - attr nesting too deep");
            return p_end;
        }
        if (is_additional_list != 0 || attr_id == ATTR_ID_ADDITION_PROTO_DESC_LISTS) {
            nest_level |= SDP_ADDITIONAL_LIST_MASK;
        }

        while (p < p_end) {
            p = add_attr(p, p_db, p_rec, 0, p_attr, (u8)(nest_level + 1));
            if (!p) {
                return NULL;
            }
        }
        break;

    case TEXT_STR_DESC_TYPE:
    case URL_DESC_TYPE:
        BE_STREAM_TO_ARRAY(p, p_attr->attr_value.array, (s32)attr_len);
        break;

    case BOOLEAN_DESC_TYPE:
        switch (attr_len) {
        case 1:
            p_attr->attr_value.uint8 = *p++;
            break;
        default:
            SDP_TRACE_WARNING1("SDP - bad len in boolean attr: %d", attr_len);
            return p + attr_len;
        }
        break;

    default:
        break;
    }

    p_db->p_free_mem += total_len;
    p_db->mem_free -= total_len;

    if (!p_parent_attr) {
        if (!p_rec->p_first_attr) {
            p_rec->p_first_attr = p_attr;
        } else {
            tSDP_DISC_ATTR* p_attr1 = p_rec->p_first_attr;

            while (p_attr1->p_next_attr) {
                p_attr1 = p_attr1->p_next_attr;
            }

            p_attr1->p_next_attr = p_attr;
        }
    } else {
        if (!p_parent_attr->attr_value.p_sub_attr) {
            p_parent_attr->attr_value.p_sub_attr = p_attr;
        } else {
            tSDP_DISC_ATTR* p_attr1 = p_parent_attr->attr_value.p_sub_attr;

            while (p_attr1->p_next_attr) {
                p_attr1 = p_attr1->p_next_attr;
            }

            p_attr1->p_next_attr = p_attr;
        }
    }

    return p;
}
