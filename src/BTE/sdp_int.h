/* BTE/sdp_int.h - the SDP layer's types, constants, trace statements and stream macros the six BTE/sdp_*.c units
 *   share; each unit's own entry points are declared in its own header (BTE/sdp_main.h, BTE/sdp_db.h, ...). */
#ifndef MHTRI_BTE_SDP_INT_H
#define MHTRI_BTE_SDP_INT_H

#include "types.h"
#include "BTE/gki_time.h"

/* The data element type descriptors (the upper five bits of a type byte) and their size indices (the lower three). */
#define UINT_DESC_TYPE 1
#define TWO_COMP_INT_DESC_TYPE 2
#define UUID_DESC_TYPE 3
#define TEXT_STR_DESC_TYPE 4
#define BOOLEAN_DESC_TYPE 5
#define DATA_ELE_SEQ_DESC_TYPE 6
#define DATA_ELE_ALT_DESC_TYPE 7
#define URL_DESC_TYPE 8

#define SIZE_ONE_BYTE 0
#define SIZE_TWO_BYTES 1
#define SIZE_FOUR_BYTES 2
#define SIZE_EIGHT_BYTES 3
#define SIZE_SIXTEEN_BYTES 4
#define SIZE_IN_NEXT_BYTE 5
#define SIZE_IN_NEXT_WORD 6
#define SIZE_IN_NEXT_LONG 7

/* The PDU ids of the SDP protocol. */
#define SDP_PDU_ERROR_RESPONSE 0x01
#define SDP_PDU_SERVICE_SEARCH_REQ 0x02
#define SDP_PDU_SERVICE_SEARCH_RSP 0x03
#define SDP_PDU_SERVICE_ATTR_REQ 0x04
#define SDP_PDU_SERVICE_ATTR_RSP 0x05
#define SDP_PDU_SERVICE_SEARCH_ATTR_REQ 0x06
#define SDP_PDU_SERVICE_SEARCH_ATTR_RSP 0x07

/* The result codes of the SDP API and the error codes of the protocol. */
#define SDP_SUCCESS 0x0000
#define SDP_INVALID_VERSION 0x0001
#define SDP_INVALID_SERV_REC_HDL 0x0002
#define SDP_INVALID_REQ_SYNTAX 0x0003
#define SDP_INVALID_PDU_SIZE 0x0004
#define SDP_INVALID_CONT_STATE 0x0005
#define SDP_NO_RESOURCES 0x0006
#define SDP_DI_REG_FAILED 0x0007
#define SDP_DI_DISC_FAILED 0x0008
#define SDP_NO_DI_RECORD_FOUND 0x0009
#define SDP_ERR_ATTR_NOT_PRESENT 0x000A
#define SDP_ILLEGAL_PARAMETER 0x000B
#define SDP_NO_RECS_MATCH 0xFFF0
#define SDP_CONN_FAILED 0xFFF1
#define SDP_CFG_FAILED 0xFFF2
#define SDP_GENERIC_ERROR 0xFFF3
#define SDP_DB_FULL 0xFFF4
#define SDP_INVALID_PDU 0xFFF5
#define SDP_SECURITY_ERR 0xFFF6
#define SDP_CONN_REJECTED 0xFFF7

/* The connection states and flags of a CCB, and the discovery states. */
#define SDP_STATE_IDLE 0
#define SDP_STATE_CONN_SETUP 1
#define SDP_STATE_CFG_SETUP 2
#define SDP_STATE_CONNECTED 3

#define SDP_FLAGS_IS_ORIG 0x01
#define SDP_FLAGS_HIS_CFG_DONE 0x02
#define SDP_FLAGS_MY_CFG_DONE 0x04

#define SDP_DISC_WAIT_CONN 0
#define SDP_DISC_WAIT_HANDLES 1
#define SDP_DISC_WAIT_ATTR 2
#define SDP_DISC_WAIT_SEARCH_ATTR 3

/* The sizes the retail bodies compare against. */
#define SDP_MAX_CONNECTIONS 4
#define SDP_MAX_RECORDS 20
#define SDP_MAX_REC_ATTR 25
#define SDP_MAX_PAD_LEN 350
#define SDP_MAX_ATTR_LEN 80
#define SDP_MAX_UUID_FILTERS 3
#define SDP_MAX_ATTR_FILTERS 12
#define SDP_MAX_DISC_SERVER_RECS 21
#define SDP_MAX_LIST_BYTE_COUNT 1000
#define MAX_UUIDS_PER_SEQ 16
#define MAX_ATTR_PER_SEQ 16
#define MAX_UUID_SIZE 16
#define MAX_NEST_LEVELS 5
#define SDP_MAX_LIST_ELEMS 3
#define SDP_MAX_SERVICE_RSPHDR_LEN 12
#define SDP_MAX_ATTR_RSPHDR_LEN 10
#define SDP_PSM 0x0001
#define SDP_POOL_ID 2
#define L2CAP_MIN_OFFSET 9
#define SDP_INACT_TIMEOUT 30
#define BTU_TTYPE_SDP 5
#define SDP_MTU_SIZE 256
#define SDP_FLUSH_TO 0xFFFF

/* The attribute ids the device identification record and the discovery code name. */
#define ATTR_ID_SERVICE_CLASS_ID_LIST 0x0001
#define ATTR_ID_SERVICE_ID 0x0003
#define ATTR_ID_PROTOCOL_DESC_LIST 0x0004
#define ATTR_ID_DOCUMENTATION_URL 0x000A
#define ATTR_ID_CLIENT_EXE_URL 0x000B
#define ATTR_ID_EXT_BRCM_VERSION 0x8001
#define ATTR_ID_SERVICE_DESCRIPTION 0x0101
#define ATTR_ID_SPECIFICATION_ID 0x0200
#define ATTR_ID_VENDOR_ID 0x0201
#define ATTR_ID_PRODUCT_ID 0x0202
#define ATTR_ID_PRODUCT_VERSION 0x0203
#define ATTR_ID_PRIMARY_RECORD 0x0204
#define ATTR_ID_VENDOR_ID_SOURCE 0x0205
#define ATTR_ID_RECORD_HANDLE 0x0000
#define UUID_SERVCLASS_PNP_INFORMATION 0x1200
#define BLUETOOTH_DI_SPECIFICATION 0x0009

/* The trace levels and the SDP layer's trace masks (the layer in bits 16-23, the type in the low bits). */
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define TRACE_SDP_ERROR 0x000A0000
#define TRACE_SDP_WARNING 0x000A0001
#define TRACE_SDP_API 0x000A0002
#define TRACE_SDP_EVENT 0x000A0003

/* size: 0x14 - a UUID of 2, 4 or 16 bytes. */
typedef struct tSDP_UUID {
    /* +0x00 */ u16 len;
    /* +0x04 */ union {
        u16 uuid16;
        u32 uuid32;
        u8 uuid128[MAX_UUID_SIZE];
    } uu;
} tSDP_UUID; /* size: 0x14 */

/* size: 0x0C - one attribute value of a discovered record (more bytes follow for a long array value). */
typedef struct tSDP_DISC_ATTR {
    /* +0x00 */ struct tSDP_DISC_ATTR* p_next_attr;
    /* +0x04 */ u16 attr_id;
    /* +0x06 */ u16 attr_len_type;
    /* +0x08 */ union {
        u8 uint8;
        u16 uint16;
        u32 uint32;
        u8 array[4];
        struct tSDP_DISC_ATTR* p_sub_attr;
    } attr_value;
} tSDP_DISC_ATTR; /* size: 0x0C */

/* size: 0x14 - a discovered service record. */
typedef struct tSDP_DISC_REC {
    /* +0x00 */ tSDP_DISC_ATTR* p_first_attr;
    /* +0x04 */ struct tSDP_DISC_REC* p_next_rec;
    /* +0x08 */ u32 time_read;
    /* +0x0C */ u8 remote_bd_addr[6];
} tSDP_DISC_REC; /* size: 0x14 */

/* size: 0x6C - the discovery database header; the caller's memory after it holds the records and attributes. */
typedef struct tSDP_DISCOVERY_DB {
    /* +0x00 */ u32 mem_size;
    /* +0x04 */ u32 mem_free;
    /* +0x08 */ tSDP_DISC_REC* p_first_rec;
    /* +0x0C */ u16 num_uuid_filters;
    /* +0x10 */ tSDP_UUID uuid_filters[SDP_MAX_UUID_FILTERS];
    /* +0x4C */ u16 num_attr_filters;
    /* +0x4E */ u16 attr_filters[SDP_MAX_ATTR_FILTERS];
    /* +0x68 */ u8* p_free_mem;
} tSDP_DISCOVERY_DB; /* size: 0x6C */

/* The discovery completion callback. */
typedef void (tSDP_DISC_CMPL_CB)(u16 result);

/* size: 0x0C - one attribute of a local service record. */
typedef struct tSDP_ATTRIBUTE {
    /* +0x00 */ u32 len;
    /* +0x04 */ u8* value_ptr;
    /* +0x08 */ u16 id;
    /* +0x0A */ u8 type;
} tSDP_ATTRIBUTE; /* size: 0x0C */

/* size: 0x298 - a local service record: its attributes in id order and the pad their values are stored in. */
typedef struct tSDP_RECORD {
    /* +0x000 */ u32 record_handle;
    /* +0x004 */ u32 free_pad_ptr;
    /* +0x008 */ u16 num_attributes;
    /* +0x00C */ tSDP_ATTRIBUTE attribute[SDP_MAX_REC_ATTR];
    /* +0x138 */ u8 attr_pad[SDP_MAX_PAD_LEN];
} tSDP_RECORD; /* size: 0x298 */

/* size: 0x33E8 - the local service record database. */
typedef struct tSDP_DB {
    /* +0x00 */ u32 di_primary_handle;
    /* +0x04 */ u8 brcm_di_registered;
    /* +0x06 */ u16 num_records;
    /* +0x08 */ tSDP_RECORD record[SDP_MAX_RECORDS];
} tSDP_DB; /* size: 0x33E8 */

/* size: 0x478 - one SDP connection (client or server side). */
typedef struct tCONN_CB {
    /* +0x000 */ u8 con_state;
    /* +0x001 */ u8 con_flags;
    /* +0x002 */ u8 device_address[6];
    /* +0x008 */ TIMER_LIST_ENT timer_entry;
    /* +0x020 */ u16 rem_mtu_size;
    /* +0x022 */ u16 connection_id;
    /* +0x024 */ u16 list_len;
    /* +0x026 */ u8 rsp_list[SDP_MAX_LIST_BYTE_COUNT];
    /* +0x410 */ tSDP_DISCOVERY_DB* p_db;
    /* +0x414 */ tSDP_DISC_CMPL_CB* p_cb;
    /* +0x418 */ u32 handles[SDP_MAX_DISC_SERVER_RECS];
    /* +0x46C */ u16 num_handles;
    /* +0x46E */ u16 cur_handle;
    /* +0x470 */ u16 transaction_id;
    /* +0x472 */ u16 disconnect_reason;
    /* +0x474 */ u8 disc_state;
    /* +0x475 */ u8 is_attr_search;
    /* +0x476 */ u16 cont_offset;
} tCONN_CB; /* size: 0x478 */

/* size: 0x4634 - the SDP control block (.bss 0x8073D360, 0x4638 B with its alignment). */
typedef struct tSDP_CB {
    /* +0x0000 */ tL2CAP_CFG_INFO l2cap_my_cfg;
    /* +0x003C */ tCONN_CB ccb[SDP_MAX_CONNECTIONS];
    /* +0x121C */ tSDP_DB server_db;
    /* +0x4604 */ tL2CAP_APPL_INFO reg_info;
    /* +0x462C */ u16 max_attr_list_size;
    /* +0x462E */ u16 max_recs_per_search;
    /* +0x4630 */ u8 trace_level;
} tSDP_CB; /* size: 0x4634 */

/* size: 0x12 - one UUID of a request's UUID sequence. */
typedef struct tSDP_UUID_ENTRY {
    /* +0x00 */ u16 len;
    /* +0x02 */ u8 value[MAX_UUID_SIZE];
} tSDP_UUID_ENTRY; /* size: 0x12 */

/* size: 0x122 - the UUID sequence of a service search request. */
typedef struct tSDP_UUID_SEQ {
    /* +0x000 */ u16 num_uids;
    /* +0x002 */ tSDP_UUID_ENTRY uuid_entry[MAX_UUIDS_PER_SEQ];
} tSDP_UUID_SEQ; /* size: 0x122 */

/* size: 0x04 - one attribute id range of a request. */
typedef struct tSDP_ATTR_ID {
    /* +0x00 */ u16 start;
    /* +0x02 */ u16 end;
} tSDP_ATTR_ID; /* size: 0x04 */

/* size: 0x42 - the attribute id list of an attribute request. */
typedef struct tSDP_ATTR_SEQ {
    /* +0x00 */ u16 num_attr;
    /* +0x02 */ tSDP_ATTR_ID attr_entry[MAX_ATTR_PER_SEQ];
} tSDP_ATTR_SEQ; /* size: 0x42 */

/* size: 0xFA - the device identification record SDP_SetLocalDiRecord registers. */
typedef struct tSDP_DI_RECORD {
    /* +0x00 */ u16 vendor;
    /* +0x02 */ u16 vendor_id_source;
    /* +0x04 */ u16 product;
    /* +0x06 */ u16 version;
    /* +0x08 */ u8 primary_record;
    /* +0x09 */ char client_executable_url[SDP_MAX_ATTR_LEN];
    /* +0x59 */ char service_description[SDP_MAX_ATTR_LEN];
    /* +0xA9 */ char documentation_url[SDP_MAX_ATTR_LEN];
} tSDP_DI_RECORD; /* size: 0xFA */

/* size: 0xFC - the device identification record SDP_GetLocalDiRecord reads back, with its specification id. */
typedef struct tSDP_DI_GET_RECORD {
    /* +0x00 */ u16 spec_id;
    /* +0x02 */ tSDP_DI_RECORD rec;
} tSDP_DI_GET_RECORD; /* size: 0xFC */

/* The SDP layer's trace statements: each prints only while the control block's trace level reaches its own. */
#define SDP_TRACE_ERROR0(m) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg_0(TRACE_SDP_ERROR, (m));}
#define SDP_TRACE_ERROR4(m, p1, p2, p3, p4) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_4(TRACE_SDP_ERROR, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4));}
#define SDP_TRACE_WARNING0(m) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg_0(TRACE_SDP_WARNING, (m));}
#define SDP_TRACE_WARNING1(m, p1) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_1(TRACE_SDP_WARNING, (m), (u32)(p1));}
#define SDP_TRACE_WARNING2(m, p1, p2) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_2(TRACE_SDP_WARNING, (m), (u32)(p1), (u32)(p2));}
#define SDP_TRACE_API2(m, p1, p2) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_2(TRACE_SDP_API, (m), (u32)(p1), (u32)(p2));}
#define SDP_TRACE_EVENT0(m) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_SDP_EVENT, (m));}
#define SDP_TRACE_EVENT1(m, p1) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_SDP_EVENT, (m), (u32)(p1));}
#define SDP_TRACE_EVENT2(m, p1, p2) {if (sdp_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_2(TRACE_SDP_EVENT, (m), (u32)(p1), (u32)(p2));}

/* The big-endian stream readers and writers of the protocol code; each advances `p`. */
#define UINT8_TO_BE_STREAM(p, v) {*(p)++ = (u8)(v);}
#define UINT16_TO_BE_STREAM(p, v) {*(p)++ = (u8)((v) >> 8); *(p)++ = (u8)(v);}
#define UINT32_TO_BE_STREAM(p, v) {*(p)++ = (u8)((v) >> 24); *(p)++ = (u8)((v) >> 16);     *(p)++ = (u8)((v) >> 8); *(p)++ = (u8)(v);}
#define ARRAY_TO_BE_STREAM(p, a, len) {int ijk; for (ijk = 0; ijk < (len); ijk++) *(p)++ = (u8)(a)[ijk];}
#define BE_STREAM_TO_UINT8(v, p) {v = (u8)(*(p)); (p) += 1;}
#define BE_STREAM_TO_UINT16(v, p) {v = (u16)(((u16)(*(p)) << 8) + (u16)(*((p) + 1))); (p) += 2;}
#define BE_STREAM_TO_UINT32(v, p) {v = ((u32)(*((p) + 3)) + ((u32)(*((p) + 2)) << 8) +     ((u32)(*((p) + 1)) << 16) + ((u32)(*(p)) << 24)); (p) += 4;}
#define BE_STREAM_TO_ARRAY(p, a, len) {int ijk; for (ijk = 0; ijk < (len); ijk++) ((u8*)(a))[ijk] = *(p)++;}

#endif /* MHTRI_BTE_SDP_INT_H */
