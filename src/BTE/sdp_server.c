/*
 * BTE/sdp_server.c - the SDP server: the client request dispatch and the service search, attribute and
 *   search-attribute handlers.
 *
 * RANGE. .text 0x804A2D64-0x804A3A74 (4 functions, 0xD10 B); .data 0x806183A8-0x806183E8. The .text holds the BTE
 *   sdp_server.c functions sdp_server_handle_client_req..process_service_search_attr_req; sdp_utils.c's
 *   sdpu_find_ccb_by_cid follows. The .data (the two trace format strings) starts after a 4-byte pad beyond 4-byte
 *   alignment (0x806183A4 -> 0x806183A8).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off` + `-ipa file`), per object in configure.py;
 *   the evidence is in BTE/sdp_utils.c's header. The function starts of this band are packed to 4 bytes.
 * NAMES. the map's, which are the BTE source's.
 * RESIDUALS. string relocations name the compiler's local pool where the map has lbl_ labels.
 * SHAPES. plain C. A request's response is built in the CCB's rsp_list and sent in MTU-sized pieces with a 2-byte
 *   continuation offset.
 */

#include "BTE/sdp_server.h"
#include "BTE/sdp_db.h"
#include "BTE/sdp_main.h"
#include "BTE/sdp_utils.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The length of the server's continuation state (a 2-byte offset). */
#define SDP_CONTINUATION_LEN 2

static void process_service_search(tCONN_CB* p_ccb, u16 trans_num, u16 param_len, u8* p_req, u8* p_req_end);
static void process_service_attr_req(tCONN_CB* p_ccb, u16 trans_num, u16 param_len, u8* p_req, u8* p_req_end);
static void process_service_search_attr_req(tCONN_CB* p_ccb, u16 trans_num, u16 param_len, u8* p_req,
                                            u8* p_req_end);

/* 0x804A2D64 (0x154): Validates a client request's header and dispatches it to the handler of its PDU. */
void sdp_server_handle_client_req(tCONN_CB* p_ccb, BT_HDR* p_msg) {
    u8* p_req = &((BT_BUF*)p_msg)->data[p_msg->offset];
    u8* p_req_end = p_req + p_msg->len;
    u8 pdu_id;
    u16 trans_num;
    u16 param_len;

    btu_start_timer(&p_ccb->timer_entry, BTU_TTYPE_SDP, SDP_INACT_TIMEOUT);

    pdu_id = *p_req++;

    BE_STREAM_TO_UINT16(trans_num, p_req);
    BE_STREAM_TO_UINT16(param_len, p_req);

    if ((p_req + param_len) > p_req_end) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_PDU_SIZE, NULL);
        return;
    }

    switch (pdu_id) {
    case SDP_PDU_SERVICE_SEARCH_REQ:
        process_service_search(p_ccb, trans_num, param_len, p_req, p_req_end);
        break;

    case SDP_PDU_SERVICE_ATTR_REQ:
        process_service_attr_req(p_ccb, trans_num, param_len, p_req, p_req_end);
        break;

    case SDP_PDU_SERVICE_SEARCH_ATTR_REQ:
        process_service_search_attr_req(p_ccb, trans_num, param_len, p_req, p_req_end);
        break;

    default:
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_REQ_SYNTAX, NULL);
        SDP_TRACE_WARNING1("SDP - server got unknown PDU: 0x%x", pdu_id);
        break;
    }
}

/* 0x804A2EB8 (0x410): Answers a service search request with the handles of the matching local records. */
static void process_service_search(tCONN_CB* p_ccb, u16 trans_num, u16 param_len, u8* p_req, u8* p_req_end) {
    u16 max_replies;
    u16 cur_handles;
    u16 rem_handles;
    u16 cont_offset;
    tSDP_UUID_SEQ uid_seq;
    u8* p_rsp;
    u8* p_rsp_start;
    u8* p_rsp_param_len;
    u16 rsp_param_len;
    u16 num_rsp_handles;
    u16 xx;
    u32 rsp_handles[SDP_MAX_RECORDS];
    tSDP_RECORD* p_rec = NULL;
    BT_HDR* p_buf;
    u8 is_cont = FALSE;

    p_req = sdpu_extract_uid_seq(p_req, param_len, &uid_seq);

    if (!p_req || !uid_seq.num_uids) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_REQ_SYNTAX, NULL);
        return;
    }

    BE_STREAM_TO_UINT16(max_replies, p_req);

    if (max_replies > SDP_MAX_RECORDS) {
        max_replies = SDP_MAX_RECORDS;
    }

    for (num_rsp_handles = 0; num_rsp_handles < max_replies;) {
        p_rec = sdp_db_service_search(p_rec, &uid_seq);

        if (p_rec) {
            rsp_handles[num_rsp_handles++] = p_rec->record_handle;
        } else {
            break;
        }
    }

    if (*p_req) {
        if (*p_req++ != SDP_CONTINUATION_LEN) {
            sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_CONT_STATE, NULL);
            return;
        }
        BE_STREAM_TO_UINT16(cont_offset, p_req);

        if (cont_offset != p_ccb->cont_offset) {
            sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_CONT_STATE, NULL);
            return;
        }

        rem_handles = num_rsp_handles - cont_offset;
    } else {
        rem_handles = num_rsp_handles;
        cont_offset = 0;
    }

    cur_handles = (u16)((p_ccb->rem_mtu_size - SDP_MAX_SERVICE_RSPHDR_LEN) / 4);

    if (rem_handles <= cur_handles) {
        cur_handles = rem_handles;
    } else {
        p_ccb->cont_offset += cur_handles;
        is_cont = TRUE;
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID)) == NULL) {
        SDP_TRACE_ERROR0("SDP - no buf for search rsp");
        return;
    }
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_rsp = p_rsp_start = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    UINT8_TO_BE_STREAM(p_rsp, SDP_PDU_SERVICE_SEARCH_RSP);
    UINT16_TO_BE_STREAM(p_rsp, trans_num);

    p_rsp_param_len = p_rsp;
    p_rsp += 2;

    UINT16_TO_BE_STREAM(p_rsp, num_rsp_handles);
    UINT16_TO_BE_STREAM(p_rsp, cur_handles);

    for (xx = cont_offset; xx < cont_offset + cur_handles; xx++) {
        UINT32_TO_BE_STREAM(p_rsp, rsp_handles[xx]);
    }

    if (is_cont) {
        UINT8_TO_BE_STREAM(p_rsp, SDP_CONTINUATION_LEN);
        UINT16_TO_BE_STREAM(p_rsp, p_ccb->cont_offset);
    } else {
        UINT8_TO_BE_STREAM(p_rsp, 0);
    }

    rsp_param_len = p_rsp - p_rsp_param_len - 2;
    UINT16_TO_BE_STREAM(p_rsp_param_len, rsp_param_len);

    p_buf->len = p_rsp - p_rsp_start;

    L2CA_DataWrite(p_ccb->connection_id, p_buf);
}

/* 0x804A32C8 (0x390): Answers an attribute request for one local record, continuing a long answer across requests. */
static void process_service_attr_req(tCONN_CB* p_ccb, u16 trans_num, u16 param_len, u8* p_req, u8* p_req_end) {
    u16 max_list_len;
    u16 len_to_send;
    u16 cont_offset;
    u16 rem_len;
    tSDP_ATTR_SEQ attr_seq;
    u8* p_rsp;
    u8* p_rsp_start;
    u8* p_rsp_param_len;
    u16 rsp_param_len;
    u16 xx;
    u32 rec_handle;
    tSDP_RECORD* p_rec;
    tSDP_ATTRIBUTE* p_attr;
    BT_HDR* p_buf;
    u8 is_cont = FALSE;

    BE_STREAM_TO_UINT32(rec_handle, p_req);

    if (p_req > p_req_end) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_SERV_REC_HDL, NULL);
        return;
    }

    BE_STREAM_TO_UINT16(max_list_len, p_req);

    if (max_list_len > (p_ccb->rem_mtu_size - SDP_MAX_ATTR_RSPHDR_LEN)) {
        max_list_len = p_ccb->rem_mtu_size - SDP_MAX_ATTR_RSPHDR_LEN;
    }

    p_req = sdpu_extract_attr_seq(p_req, param_len, &attr_seq);

    if (!p_req || !attr_seq.num_attr || p_req > p_req_end) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_REQ_SYNTAX, NULL);
        return;
    }

    p_rec = sdp_db_find_record(rec_handle);
    if (!p_rec) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_SERV_REC_HDL, NULL);
        return;
    }

    if (*p_req) {
        if (*p_req++ != SDP_CONTINUATION_LEN) {
            sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_CONT_STATE, NULL);
            return;
        }

        BE_STREAM_TO_UINT16(cont_offset, p_req);

        if (cont_offset != p_ccb->cont_offset) {
            sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_CONT_STATE, NULL);
            return;
        }
    } else {
        p_rsp = &p_ccb->rsp_list[3];
        p_ccb->cont_offset = 0;

        for (xx = 0; xx < attr_seq.num_attr; xx++) {
            p_attr = sdp_db_find_attr_in_rec(p_rec, attr_seq.attr_entry[xx].start, attr_seq.attr_entry[xx].end);

            if (p_attr) {
                rem_len = SDP_MAX_LIST_BYTE_COUNT - (u16)(p_rsp - &p_ccb->rsp_list[0]);
                if (p_attr->len > (u32)(rem_len - 6)) {
                    break;
                }

                p_rsp = sdpu_build_attrib_entry(p_rsp, p_attr);

                if (attr_seq.attr_entry[xx].start != attr_seq.attr_entry[xx].end) {
                    attr_seq.attr_entry[xx].start = p_attr->id + 1;
                    xx--;
                }
            }
        }

        p_ccb->list_len = (u16)(p_rsp - &p_ccb->rsp_list[0]);
        if (p_ccb->list_len > 255) {
            p_ccb->rsp_list[0] = (u8)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_WORD);
            p_ccb->rsp_list[1] = (u8)((p_ccb->list_len - 3) >> 8);
            p_ccb->rsp_list[2] = (u8)(p_ccb->list_len - 3);
        } else {
            p_ccb->cont_offset = 1;
            p_ccb->rsp_list[1] = (u8)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
            p_ccb->rsp_list[2] = (u8)(p_ccb->list_len - 3);
            p_ccb->list_len--;
        }
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID)) == NULL) {
        SDP_TRACE_ERROR0("SDP - no buf for search rsp");
        return;
    }
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_rsp = p_rsp_start = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    UINT8_TO_BE_STREAM(p_rsp, SDP_PDU_SERVICE_ATTR_RSP);
    UINT16_TO_BE_STREAM(p_rsp, trans_num);

    p_rsp_param_len = p_rsp;
    p_rsp += 2;

    if (p_ccb->list_len <= max_list_len) {
        len_to_send = p_ccb->list_len;
    } else {
        len_to_send = max_list_len;
    }

    UINT16_TO_BE_STREAM(p_rsp, len_to_send);

    memcpy(p_rsp, &p_ccb->rsp_list[p_ccb->cont_offset], len_to_send);
    p_rsp += len_to_send;

    p_ccb->cont_offset += len_to_send;
    p_ccb->list_len -= len_to_send;

    if (p_ccb->list_len) {
        is_cont = TRUE;

        UINT8_TO_BE_STREAM(p_rsp, SDP_CONTINUATION_LEN);
        UINT16_TO_BE_STREAM(p_rsp, p_ccb->cont_offset);
    } else {
        UINT8_TO_BE_STREAM(p_rsp, 0);
    }

    rsp_param_len = p_rsp - p_rsp_param_len - 2;
    UINT16_TO_BE_STREAM(p_rsp_param_len, rsp_param_len);

    p_buf->len = p_rsp - p_rsp_start;

    L2CA_DataWrite(p_ccb->connection_id, p_buf);
}

/* 0x804A3658 (0x41C): Answers a search-attribute request with the attributes of every matching local record. */
static void process_service_search_attr_req(tCONN_CB* p_ccb, u16 trans_num, u16 param_len, u8* p_req,
                                            u8* p_req_end) {
    u16 max_list_len;
    u16 rem_len;
    u16 len_to_send;
    u16 cont_offset;
    tSDP_UUID_SEQ uid_seq;
    u8* p_rsp;
    u8* p_rsp_start;
    u8* p_rsp_param_len;
    u16 rsp_param_len;
    u16 xx;
    tSDP_RECORD* p_rec;
    tSDP_ATTR_SEQ attr_seq;
    tSDP_ATTR_SEQ attr_seq_sav;
    tSDP_ATTRIBUTE* p_attr;
    BT_HDR* p_buf;
    u8 maxxed_out = FALSE;
    u8 is_cont = FALSE;
    u8* p_seq_start;
    u16 seq_len;

    p_req = sdpu_extract_uid_seq(p_req, param_len, &uid_seq);

    if (!p_req || !uid_seq.num_uids) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_REQ_SYNTAX, NULL);
        return;
    }

    BE_STREAM_TO_UINT16(max_list_len, p_req);

    if (max_list_len > (p_ccb->rem_mtu_size - SDP_MAX_ATTR_RSPHDR_LEN)) {
        max_list_len = p_ccb->rem_mtu_size - SDP_MAX_ATTR_RSPHDR_LEN;
    }

    p_req = sdpu_extract_attr_seq(p_req, param_len, &attr_seq);

    if (!p_req || !attr_seq.num_attr) {
        sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_REQ_SYNTAX, NULL);
        return;
    }

    attr_seq_sav = attr_seq;

    if (*p_req) {
        if (*p_req++ != SDP_CONTINUATION_LEN) {
            sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_CONT_STATE, NULL);
            return;
        }

        BE_STREAM_TO_UINT16(cont_offset, p_req);

        if (cont_offset != p_ccb->cont_offset) {
            sdpu_build_n_send_error(p_ccb, trans_num, SDP_INVALID_CONT_STATE, NULL);
            return;
        }
    } else {
        p_rsp = &p_ccb->rsp_list[3];
        p_ccb->cont_offset = 0;

        for (p_rec = sdp_db_service_search(NULL, &uid_seq); p_rec; p_rec = sdp_db_service_search(p_rec, &uid_seq)) {
            attr_seq = attr_seq_sav;

            p_seq_start = p_rsp;
            p_rsp += 3;

            for (xx = 0; xx < attr_seq.num_attr; xx++) {
                p_attr = sdp_db_find_attr_in_rec(p_rec, attr_seq.attr_entry[xx].start, attr_seq.attr_entry[xx].end);

                if (p_attr) {
                    rem_len = SDP_MAX_LIST_BYTE_COUNT - (u16)(p_rsp - &p_ccb->rsp_list[0]);
                    if (p_attr->len > (u32)(rem_len - 6)) {
                        maxxed_out = TRUE;
                        break;
                    }

                    p_rsp = sdpu_build_attrib_entry(p_rsp, p_attr);

                    if (attr_seq.attr_entry[xx].start != attr_seq.attr_entry[xx].end) {
                        attr_seq.attr_entry[xx].start = p_attr->id + 1;
                        xx--;
                    }
                }
            }

            if (maxxed_out) {
                break;
            }

            seq_len = (u16)(p_rsp - p_seq_start - 3);
            if (seq_len) {
                UINT8_TO_BE_STREAM(p_seq_start, (DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_WORD);
                UINT16_TO_BE_STREAM(p_seq_start, seq_len);
            } else {
                p_rsp = p_seq_start;
            }
        }

        p_ccb->list_len = (u16)(p_rsp - &p_ccb->rsp_list[0]);
        if (p_ccb->list_len > 255) {
            p_ccb->rsp_list[0] = (u8)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_WORD);
            p_ccb->rsp_list[1] = (u8)((p_ccb->list_len - 3) >> 8);
            p_ccb->rsp_list[2] = (u8)(p_ccb->list_len - 3);
        } else {
            p_ccb->cont_offset = 1;
            p_ccb->rsp_list[1] = (u8)((DATA_ELE_SEQ_DESC_TYPE << 3) | SIZE_IN_NEXT_BYTE);
            p_ccb->rsp_list[2] = (u8)(p_ccb->list_len - 3);
            p_ccb->list_len--;
        }
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(SDP_POOL_ID)) == NULL) {
        SDP_TRACE_ERROR0("SDP - no buf for search rsp");
        return;
    }
    p_buf->offset = L2CAP_MIN_OFFSET;
    p_rsp = p_rsp_start = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    UINT8_TO_BE_STREAM(p_rsp, SDP_PDU_SERVICE_SEARCH_ATTR_RSP);
    UINT16_TO_BE_STREAM(p_rsp, trans_num);

    p_rsp_param_len = p_rsp;
    p_rsp += 2;

    if (p_ccb->list_len <= max_list_len) {
        len_to_send = p_ccb->list_len;
    } else {
        len_to_send = max_list_len;
    }

    UINT16_TO_BE_STREAM(p_rsp, len_to_send);

    memcpy(p_rsp, &p_ccb->rsp_list[p_ccb->cont_offset], len_to_send);
    p_rsp += len_to_send;

    p_ccb->cont_offset += len_to_send;
    p_ccb->list_len -= len_to_send;

    if (p_ccb->list_len) {
        is_cont = TRUE;

        UINT8_TO_BE_STREAM(p_rsp, SDP_CONTINUATION_LEN);
        UINT16_TO_BE_STREAM(p_rsp, p_ccb->cont_offset);
    } else {
        UINT8_TO_BE_STREAM(p_rsp, 0);
    }

    rsp_param_len = p_rsp - p_rsp_param_len - 2;
    UINT16_TO_BE_STREAM(p_rsp_param_len, rsp_param_len);

    p_buf->len = p_rsp - p_rsp_start;

    L2CA_DataWrite(p_ccb->connection_id, p_buf);
}
