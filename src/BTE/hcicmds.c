/*
 * BTE/hcicmds.c - the HCI command builders: the btsnd_hcic_* functions, each filling a GKI buffer with one HCI command
 *   and sending it through btu_hcif_send_cmd.
 *
 * RANGE. .text 0x8048D44C-0x8048FB68 (59 functions, 0x271C B); .rodata 0x80573860-0x80573870. Strong by family:
 *   btsnd_hcic_inquiry..btsnd_hcic_vendor_spec_cmd (called by BTM_VendorSpecificCommand); gap_convert_btm_status
 *   precedes it and hidd_api.c's HID_DevInit follows. No .data; the .rodata is the channel map initialiser of
 *   btsnd_hcic_set_afh_channels, right after the last BTE/gki_time.c .rodata object.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's, and the BTE source's for the 25 unnamed rows, each from the HCI opcode it stores (OGF/OCF):
 *   inq_cancel 0x0402, link_key_neg_reply 0x040C, auth_request 0x0411, rmt_name_req_cancel 0x041A,
 *   rmt_features_req 0x041B, rmt_ver_req 0x041D, read_rmt_clk_offset 0x041F, reject_esco_conn 0x042A,
 *   switch_role 0x080B, write_policy_set 0x080D, set_event_filter 0x0C05, write_pin_type 0x0C0A,
 *   read_stored_key 0x0C0D, delete_stored_key 0x0C12, write_page_tout 0x0C18, write_scan_enable 0x0C1A,
 *   write_pagescan_cfg 0x0C1C, write_inqscan_cfg 0x0C1E, write_encr_mode 0x0C22, write_link_super_tout 0x0C37,
 *   write_cur_iac_lap 0x0C3A, read_bd_addr 0x1009, get_link_quality 0x1403, write_inqscan_type 0x0C43,
 *   vendor_spec_cmd (OGF 0x3F).
 * RESIDUALS. btsnd_hcic_change_name allocates p_buf to r30 and p_name to r31 (retail: the reverse); four declaration
 *   orders, a char* name, a local copy of p_buf or of the name, `register` on either parameter, pp from the BT_HDR
 *   idiom, the length computed later, and a scoped scheduling/propagation/common-subs/global-optimizer/level 2-3
 *   pragma left it. Wii/1.3 schedules the builders differently. The .rodata channel map is emitted as a 10-byte
 *   @-pool object (retail label 0x80573860, 0x10 B with the pad).
 * SHAPES. plain C; the builders that take a caller's buffer return nothing and tail-call btu_hcif_send_cmd.
 */

#include "BTE/hcicmds.h"
#include "BTE/btu_hcif.h"
#include "MSL/strlen.h"

/* The GKI pool HCI commands are taken from. */
#define HCI_CMD_POOL_ID 2

/* The opcode (2 bytes) and parameter length (1 byte) in front of every command's parameters. */
#define HCIC_PREAMBLE_SIZE 3

#define BD_ADDR_LEN 6
#define DEV_CLASS_LEN 3
#define LAP_LEN 3
#define LINK_KEY_LEN 16
#define PIN_CODE_LEN 16
#define HCI_MAX_NUM_OF_LINK_KEYS_PER_CMD 11
#define AFH_CHANNEL_MAP_LEN 10
#define AFH_CHANNEL_MAX 78

/* The HCI opcode groups. */
#define HCI_GRP_LINK_CONTROL_CMDS (0x01 << 10)
#define HCI_GRP_LINK_POLICY_CMDS (0x02 << 10)
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS (0x03 << 10)
#define HCI_GRP_INFORMATIONAL_PARAMS (0x04 << 10)
#define HCI_GRP_STATUS_PARAMS (0x05 << 10)
#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10)

/* The HCI commands. */
#define HCI_INQUIRY (0x0001 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_INQUIRY_CANCEL (0x0002 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_PERIODIC_INQUIRY_MODE (0x0003 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_CREATE_CONNECTION (0x0005 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_DISCONNECT (0x0006 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_ADD_SCO_CONNECTION (0x0007 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_ACCEPT_CONNECTION_REQUEST (0x0009 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_REJECT_CONNECTION_REQUEST (0x000A | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_LINK_KEY_REQUEST_REPLY (0x000B | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_LINK_KEY_REQUEST_NEG_REPLY (0x000C | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_PIN_CODE_REQUEST_REPLY (0x000D | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_PIN_CODE_REQUEST_NEG_REPLY (0x000E | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_CHANGE_CONN_PACKET_TYPE (0x000F | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_AUTHENTICATION_REQUESTED (0x0011 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_SET_CONN_ENCRYPTION (0x0013 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_RMT_NAME_REQUEST (0x0019 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_RMT_NAME_REQUEST_CANCEL (0x001A | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_READ_RMT_FEATURES (0x001B | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_READ_RMT_VERSION_INFO (0x001D | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_READ_RMT_CLOCK_OFFSET (0x001F | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_SETUP_ESCO_CONNECTION (0x0028 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_ACCEPT_ESCO_CONNECTION (0x0029 | HCI_GRP_LINK_CONTROL_CMDS)
#define HCI_REJECT_ESCO_CONNECTION (0x002A | HCI_GRP_LINK_CONTROL_CMDS)

#define HCI_HOLD_MODE (0x0001 | HCI_GRP_LINK_POLICY_CMDS)
#define HCI_SNIFF_MODE (0x0003 | HCI_GRP_LINK_POLICY_CMDS)
#define HCI_EXIT_SNIFF_MODE (0x0004 | HCI_GRP_LINK_POLICY_CMDS)
#define HCI_PARK_MODE (0x0005 | HCI_GRP_LINK_POLICY_CMDS)
#define HCI_EXIT_PARK_MODE (0x0006 | HCI_GRP_LINK_POLICY_CMDS)
#define HCI_SWITCH_ROLE (0x000B | HCI_GRP_LINK_POLICY_CMDS)
#define HCI_WRITE_POLICY_SETTINGS (0x000D | HCI_GRP_LINK_POLICY_CMDS)

#define HCI_RESET (0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_SET_EVENT_FILTER (0x0005 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_PIN_TYPE (0x000A | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_READ_STORED_LINK_KEY (0x000D | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_STORED_LINK_KEY (0x0011 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_DELETE_STORED_LINK_KEY (0x0012 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_CHANGE_LOCAL_NAME (0x0013 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_PAGE_TOUT (0x0018 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_SCAN_ENABLE (0x001A | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_PAGESCAN_CFG (0x001C | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_INQUIRYSCAN_CFG (0x001E | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_AUTHENTICATION_ENABLE (0x0020 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_ENCRYPTION_MODE (0x0022 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_CLASS_OF_DEVICE (0x0024 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_AUTO_FLUSH_TOUT (0x0028 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_SET_HC_TO_HOST_BUF_SIZE (0x0033 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_LINK_SUPER_TOUT (0x0037 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_CURRENT_IAC_LAP (0x003A | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_SET_AFH_CHANNELS (0x003F | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_INQSCAN_TYPE (0x0043 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_INQUIRY_MODE (0x0045 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_PAGESCAN_TYPE (0x0047 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)

#define HCI_READ_LOCAL_VERSION_INFO (0x0001 | HCI_GRP_INFORMATIONAL_PARAMS)
#define HCI_READ_LOCAL_FEATURES (0x0003 | HCI_GRP_INFORMATIONAL_PARAMS)
#define HCI_READ_BUFFER_SIZE (0x0005 | HCI_GRP_INFORMATIONAL_PARAMS)
#define HCI_READ_BD_ADDR (0x0009 | HCI_GRP_INFORMATIONAL_PARAMS)

#define HCI_GET_LINK_QUALITY (0x0003 | HCI_GRP_STATUS_PARAMS)
#define HCI_READ_RSSI (0x0005 | HCI_GRP_STATUS_PARAMS)

/* The set_event_filter condition types that carry a class pair or an address. */
#define HCI_FILTER_COND_DEVICE_CLASS 1
#define HCI_FILTER_COND_BD_ADDR 2

/* The parameter lengths of the fixed-size commands. */
#define HCIC_PARAM_SIZE_INQUIRY 5
#define HCIC_PARAM_SIZE_PER_INQ_MODE 9
#define HCIC_PARAM_SIZE_CREATE_CONN 13
#define HCIC_PARAM_SIZE_DISCONNECT 3
#define HCIC_PARAM_SIZE_ADD_SCO_CONN 4
#define HCIC_PARAM_SIZE_ACCEPT_CONN 7
#define HCIC_PARAM_SIZE_REJECT_CONN 7
#define HCIC_PARAM_SIZE_LINK_KEY_REQ_REPLY 22
#define HCIC_PARAM_SIZE_LINK_KEY_NEG_REPLY 6
#define HCIC_PARAM_SIZE_PIN_CODE_REQ_REPLY 23
#define HCIC_PARAM_SIZE_PIN_CODE_NEG_REPLY 6
#define HCIC_PARAM_SIZE_CHANGE_CONN_TYPE 4
#define HCIC_PARAM_SIZE_CMD_HANDLE 2
#define HCIC_PARAM_SIZE_SET_CONN_ENCRYPT 3
#define HCIC_PARAM_SIZE_RMT_NAME_REQ 10
#define HCIC_PARAM_SIZE_RMT_NAME_REQ_CANCEL 6
#define HCIC_PARAM_SIZE_SETUP_ESCO 17
#define HCIC_PARAM_SIZE_ACCEPT_ESCO 21
#define HCIC_PARAM_SIZE_REJECT_ESCO 7
#define HCIC_PARAM_SIZE_HOLD_MODE 6
#define HCIC_PARAM_SIZE_SNIFF_MODE 10
#define HCIC_PARAM_SIZE_EXIT_SNIFF_MODE 2
#define HCIC_PARAM_SIZE_PARK_MODE 6
#define HCIC_PARAM_SIZE_EXIT_PARK_MODE 2
#define HCIC_PARAM_SIZE_SWITCH_ROLE 7
#define HCIC_PARAM_SIZE_WRITE_POLICY_SET 4
#define HCIC_PARAM_SIZE_WRITE_PARAM1 1
#define HCIC_PARAM_SIZE_WRITE_PARAM2 2
#define HCIC_PARAM_SIZE_WRITE_PARAM3 3
#define HCIC_PARAM_SIZE_READ_STORED_KEY 7
#define HCIC_PARAM_SIZE_DELETE_STORED_KEY 7
#define HCIC_PARAM_SIZE_CHANGE_NAME 248
#define HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG 4
#define HCIC_PARAM_SIZE_WRITE_INQSCAN_CFG 4
#define HCIC_PARAM_SIZE_WRITE_AUTO_FLUSH_TOUT 4
#define HCIC_PARAM_SIZE_SET_HOST_BUF_SIZE 7
#define HCIC_PARAM_SIZE_WRITE_LINK_SUPER_TOUT 4
#define HCIC_PARAM_SIZE_SET_AFH_CHANNELS 10
#define HCIC_PARAM_SIZE_READ_CMD 0

/* The little-endian stream writers; each advances `p`. Addresses, classes, LAPs and keys go out reversed. */
#define UINT8_TO_STREAM(p, v) {*(p)++ = (u8)(v);}
#define UINT16_TO_STREAM(p, v) {*(p)++ = (u8)(v); *(p)++ = (u8)((v) >> 8);}
#define UINT32_TO_STREAM(p, v) {*(p)++ = (u8)(v); *(p)++ = (u8)((v) >> 8); \
    *(p)++ = (u8)((v) >> 16); *(p)++ = (u8)((v) >> 24);}
#define BDADDR_TO_STREAM(p, a) {int ijk; for (ijk = 0; ijk < BD_ADDR_LEN; ijk++) *(p)++ = (u8)(a)[BD_ADDR_LEN - 1 - ijk];}
#define DEVCLASS_TO_STREAM(p, a) {int ijk; for (ijk = 0; ijk < DEV_CLASS_LEN; ijk++) \
    *(p)++ = (u8)(a)[DEV_CLASS_LEN - 1 - ijk];}
#define LAP_TO_STREAM(p, a) {int ijk; for (ijk = 0; ijk < LAP_LEN; ijk++) *(p)++ = (u8)(a)[LAP_LEN - 1 - ijk];}
#define ARRAY16_TO_STREAM(p, a) {int ijk; for (ijk = 0; ijk < 16; ijk++) *(p)++ = (u8)(a)[15 - ijk];}
#define ARRAY_TO_STREAM(p, a, len) {int ijk; for (ijk = 0; ijk < (len); ijk++) *(p)++ = (u8)(a)[ijk];}

/* The head of a builder that takes its own buffer: returns FALSE when the pool is empty. */
#define HCIC_GETBUF(p) if ((p = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) == NULL) return FALSE

/* 0x8048D44C (0xA8): Starts an inquiry on the access code `inq_lap`. */
u8 btsnd_hcic_inquiry(const u8* inq_lap, u8 duration, u8 response_cnt) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_INQUIRY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_INQUIRY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_INQUIRY);

    LAP_TO_STREAM(pp, inq_lap);
    UINT8_TO_STREAM(pp, duration);
    UINT8_TO_STREAM(pp, response_cnt);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048D4F4 (0x60): Cancels the running inquiry. */
u8 btsnd_hcic_inq_cancel(void) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_INQUIRY_CANCEL);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048D554 (0xC0): Starts periodic inquiries every `min_period`..`max_period` (1.28 s units). */
u8 btsnd_hcic_per_inq_mode(u16 max_period, u16 min_period, const u8* inq_lap, u8 duration, u8 response_cnt) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_PER_INQ_MODE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_PERIODIC_INQUIRY_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_PER_INQ_MODE);

    UINT16_TO_STREAM(pp, max_period);
    UINT16_TO_STREAM(pp, min_period);
    LAP_TO_STREAM(pp, inq_lap);
    UINT8_TO_STREAM(pp, duration);
    UINT8_TO_STREAM(pp, response_cnt);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048D614 (0xE0): Pages the device at `dest`. */
u8 btsnd_hcic_create_conn(u8* dest, u16 packet_types, u8 page_scan_rep_mode, u8 page_scan_mode, u16 clock_offset,
                          u8 allow_switch) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CREATE_CONN;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_CREATE_CONNECTION);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CREATE_CONN);

    BDADDR_TO_STREAM(pp, dest);
    UINT16_TO_STREAM(pp, packet_types);
    UINT8_TO_STREAM(pp, page_scan_rep_mode);
    UINT8_TO_STREAM(pp, page_scan_mode);
    UINT16_TO_STREAM(pp, clock_offset);
    UINT8_TO_STREAM(pp, allow_switch);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048D6F4 (0x88): Disconnects the link `handle` with `reason`. */
u8 btsnd_hcic_disconnect(u16 handle, u8 reason) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_DISCONNECT;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_DISCONNECT);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_DISCONNECT);
    UINT16_TO_STREAM(pp, handle);
    UINT8_TO_STREAM(pp, reason);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048D77C (0x8C): Adds an SCO link with `packet_types` to the ACL link `handle`. */
u8 btsnd_hcic_add_SCO_conn(u16 handle, u16 packet_types) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_ADD_SCO_CONN;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_ADD_SCO_CONNECTION);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_ADD_SCO_CONN);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, packet_types);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048D808 (0x60): Accepts the incoming connection from `dest` with `role`, in `p`. */
void btsnd_hcic_accept_conn(BT_HDR* p, u8* dest, u8 role) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_ACCEPT_CONN;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_ACCEPT_CONNECTION_REQUEST);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_ACCEPT_CONN);
    BDADDR_TO_STREAM(pp, dest);
    UINT8_TO_STREAM(pp, role);

    btu_hcif_send_cmd(p);
}

/* 0x8048D868 (0x5C): Rejects the incoming connection from `dest` with `reason`, in `p`. */
void btsnd_hcic_reject_conn(BT_HDR* p, u8* dest, u8 reason) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REJECT_CONN;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_REJECT_CONNECTION_REQUEST);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_REJECT_CONN);

    BDADDR_TO_STREAM(pp, dest);
    UINT8_TO_STREAM(pp, reason);

    btu_hcif_send_cmd(p);
}

/* 0x8048D8C4 (0x1A0): Answers a link key request of `bd_addr` with `link_key`. */
u8 btsnd_hcic_link_key_req_reply(u8* bd_addr, u8* link_key) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_LINK_KEY_REQ_REPLY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_LINK_KEY_REQUEST_REPLY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_LINK_KEY_REQ_REPLY);

    BDADDR_TO_STREAM(pp, bd_addr);
    ARRAY16_TO_STREAM(pp, link_key);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DA64 (0xA0): Answers a link key request of `bd_addr` with no key. */
u8 btsnd_hcic_link_key_neg_reply(u8* bd_addr) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_LINK_KEY_NEG_REPLY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_LINK_KEY_REQUEST_NEG_REPLY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_LINK_KEY_NEG_REPLY);

    BDADDR_TO_STREAM(pp, bd_addr);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DB04 (0x1E4): Answers a PIN request of `bd_addr` with the `pin_code_len` bytes of `p_pin_code`, zero-padded. */
u8 btsnd_hcic_pin_code_req_reply(u8* bd_addr, u8 pin_code_len, u8* p_pin_code) {
    BT_HDR* p;
    u8* pp;
    int i;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_PIN_CODE_REQ_REPLY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_PIN_CODE_REQUEST_REPLY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_PIN_CODE_REQ_REPLY);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT8_TO_STREAM(pp, pin_code_len);

    for (i = 0; i < pin_code_len; i++) {
        *pp++ = *p_pin_code++;
    }

    for (; i < PIN_CODE_LEN; i++) {
        *pp++ = 0;
    }

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DCE8 (0xA0): Refuses a PIN request of `bd_addr`. */
u8 btsnd_hcic_pin_code_neg_reply(u8* bd_addr) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_PIN_CODE_NEG_REPLY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_PIN_CODE_REQUEST_NEG_REPLY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_PIN_CODE_NEG_REPLY);

    BDADDR_TO_STREAM(pp, bd_addr);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DD88 (0x90): Changes the packet types of the link `handle`. */
u8 btsnd_hcic_change_conn_type(u16 handle, u16 packet_types) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CHANGE_CONN_TYPE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_CHANGE_CONN_PACKET_TYPE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CHANGE_CONN_TYPE);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, packet_types);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DE18 (0x7C): Starts the authentication of the link `handle`. */
u8 btsnd_hcic_auth_request(u16 handle) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CMD_HANDLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_AUTHENTICATION_REQUESTED);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CMD_HANDLE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DE94 (0x8C): Turns the encryption of the link `handle` on or off. */
u8 btsnd_hcic_set_conn_encrypt(u16 handle, u8 enable) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SET_CONN_ENCRYPT;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SET_CONN_ENCRYPTION);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_SET_CONN_ENCRYPT);

    UINT16_TO_STREAM(pp, handle);
    UINT8_TO_STREAM(pp, enable);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DF20 (0xD8): Requests the name of the device at `bd_addr`. */
u8 btsnd_hcic_rmt_name_req(u8* bd_addr, u8 page_scan_rep_mode, u8 page_scan_mode, u16 clock_offset) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_RMT_NAME_REQ;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_RMT_NAME_REQUEST);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_RMT_NAME_REQ);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT8_TO_STREAM(pp, page_scan_rep_mode);
    UINT8_TO_STREAM(pp, page_scan_mode);
    UINT16_TO_STREAM(pp, clock_offset);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048DFF8 (0xA0): Cancels the name request to `bd_addr`. */
u8 btsnd_hcic_rmt_name_req_cancel(u8* bd_addr) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_RMT_NAME_REQ_CANCEL;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_RMT_NAME_REQUEST_CANCEL);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_RMT_NAME_REQ_CANCEL);

    BDADDR_TO_STREAM(pp, bd_addr);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E098 (0x7C): Requests the supported features of the peer on `handle`. */
u8 btsnd_hcic_rmt_features_req(u16 handle) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CMD_HANDLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_RMT_FEATURES);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CMD_HANDLE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E114 (0x7C): Requests the version information of the peer on `handle`. */
u8 btsnd_hcic_rmt_ver_req(u16 handle) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CMD_HANDLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_RMT_VERSION_INFO);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CMD_HANDLE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E190 (0x7C): Requests the clock offset of the peer on `handle`. */
u8 btsnd_hcic_read_rmt_clk_offset(u16 handle) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CMD_HANDLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_RMT_CLOCK_OFFSET);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CMD_HANDLE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E20C (0xFC): Sets up an eSCO link on the ACL link `handle`. */
u8 btsnd_hcic_setup_esco_conn(u16 handle, u32 tx_bw, u32 rx_bw, u16 max_latency, u16 voice, u8 retrans_effort,
                              u16 packet_types) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SETUP_ESCO;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SETUP_ESCO_CONNECTION);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_SETUP_ESCO);

    UINT16_TO_STREAM(pp, handle);
    UINT32_TO_STREAM(pp, tx_bw);
    UINT32_TO_STREAM(pp, rx_bw);
    UINT16_TO_STREAM(pp, max_latency);
    UINT16_TO_STREAM(pp, voice);
    UINT8_TO_STREAM(pp, retrans_effort);
    UINT16_TO_STREAM(pp, packet_types);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E308 (0xE8): Accepts the incoming eSCO connection from `bd_addr`, in `p`. */
void btsnd_hcic_accept_esco_conn(BT_HDR* p, u8* bd_addr, u32 tx_bw, u32 rx_bw, u16 max_latency, u16 content_fmt,
                                 u8 retrans_effort, u16 packet_types) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_ACCEPT_ESCO;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_ACCEPT_ESCO_CONNECTION);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_ACCEPT_ESCO);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT32_TO_STREAM(pp, tx_bw);
    UINT32_TO_STREAM(pp, rx_bw);
    UINT16_TO_STREAM(pp, max_latency);
    UINT16_TO_STREAM(pp, content_fmt);
    UINT8_TO_STREAM(pp, retrans_effort);
    UINT16_TO_STREAM(pp, packet_types);

    btu_hcif_send_cmd(p);
}

/* 0x8048E3F0 (0x60): Rejects the incoming eSCO connection from `bd_addr` with `reason`, in `p`. */
void btsnd_hcic_reject_esco_conn(BT_HDR* p, u8* bd_addr, u8 reason) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REJECT_ESCO;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_REJECT_ESCO_CONNECTION);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_REJECT_ESCO);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT8_TO_STREAM(pp, reason);

    btu_hcif_send_cmd(p);
}

/* 0x8048E450 (0xB4): Puts the link `handle` in hold mode, in `p` or a new buffer. */
u8 btsnd_hcic_hold_mode(BT_HDR* p, u16 handle, u16 max_hold_period, u16 min_hold_period) {
    u8* pp;

    if (p == NULL) {
        HCIC_GETBUF(p);
    }
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_HOLD_MODE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_HOLD_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_HOLD_MODE);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, max_hold_period);
    UINT16_TO_STREAM(pp, min_hold_period);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E504 (0xCC): Puts the link `handle` in sniff mode, in `p` or a new buffer. */
u8 btsnd_hcic_sniff_mode(BT_HDR* p, u16 handle, u16 max_sniff_period, u16 min_sniff_period, u16 sniff_attempt,
                         u16 sniff_timeout) {
    u8* pp;

    if (p == NULL) {
        HCIC_GETBUF(p);
    }
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SNIFF_MODE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SNIFF_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_SNIFF_MODE);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, max_sniff_period);
    UINT16_TO_STREAM(pp, min_sniff_period);
    UINT16_TO_STREAM(pp, sniff_attempt);
    UINT16_TO_STREAM(pp, sniff_timeout);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E5D0 (0x84): Takes the link `handle` out of sniff mode, in `p` or a new buffer. */
u8 btsnd_hcic_exit_sniff_mode(BT_HDR* p, u16 handle) {
    u8* pp;

    if (p == NULL) {
        HCIC_GETBUF(p);
    }
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_EXIT_SNIFF_MODE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_EXIT_SNIFF_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_EXIT_SNIFF_MODE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E654 (0xB4): Puts the link `handle` in park mode, in `p` or a new buffer. */
u8 btsnd_hcic_park_mode(BT_HDR* p, u16 handle, u16 beacon_max_interval, u16 beacon_min_interval) {
    u8* pp;

    if (p == NULL) {
        HCIC_GETBUF(p);
    }
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_PARK_MODE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_PARK_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_PARK_MODE);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, beacon_max_interval);
    UINT16_TO_STREAM(pp, beacon_min_interval);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E708 (0x84): Takes the link `handle` out of park mode, in `p` or a new buffer. */
u8 btsnd_hcic_exit_park_mode(BT_HDR* p, u16 handle) {
    u8* pp;

    if (p == NULL) {
        HCIC_GETBUF(p);
    }
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_EXIT_PARK_MODE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_EXIT_PARK_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_EXIT_PARK_MODE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E78C (0xB0): Requests the master/slave role `role` on the link to `bd_addr`. */
u8 btsnd_hcic_switch_role(u8* bd_addr, u8 role) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SWITCH_ROLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SWITCH_ROLE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_SWITCH_ROLE);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT8_TO_STREAM(pp, role);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E83C (0x94): Sets the link policy `settings` of the link `handle`. */
u8 btsnd_hcic_write_policy_set(u16 handle, u16 settings) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_POLICY_SET;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_POLICY_SETTINGS);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_POLICY_SET);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, settings);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E8D0 (0x5C): Resets the controller. */
u8 btsnd_hcic_reset(void) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_RESET);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048E92C (0x1BC): Sets the event filter `filt_type` with the `filt_cond_len`-byte condition `filt_cond`, in `p`. */
void btsnd_hcic_set_event_filter(BT_HDR* p, u8 filt_type, u8 filt_cond_type, u8* filt_cond, u8 filt_cond_len) {
    u8* pp = ((BT_BUF*)p)->data;

    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SET_EVENT_FILTER);

    if (filt_type) {
        p->len = (u16)(HCIC_PREAMBLE_SIZE + 2 + filt_cond_len);
        UINT8_TO_STREAM(pp, (u8)(2 + filt_cond_len));

        UINT8_TO_STREAM(pp, filt_type);
        UINT8_TO_STREAM(pp, filt_cond_type);

        if (filt_cond_type == HCI_FILTER_COND_DEVICE_CLASS) {
            DEVCLASS_TO_STREAM(pp, filt_cond);
            filt_cond += DEV_CLASS_LEN;
            DEVCLASS_TO_STREAM(pp, filt_cond);
            filt_cond += DEV_CLASS_LEN;

            filt_cond_len -= (2 * DEV_CLASS_LEN);
        } else if (filt_cond_type == HCI_FILTER_COND_BD_ADDR) {
            BDADDR_TO_STREAM(pp, filt_cond);
            filt_cond += BD_ADDR_LEN;

            filt_cond_len -= BD_ADDR_LEN;
        }

        if (filt_cond_len) {
            ARRAY_TO_STREAM(pp, filt_cond, filt_cond_len);
        }
    } else {
        p->len = (u16)(HCIC_PREAMBLE_SIZE + 1);
        UINT8_TO_STREAM(pp, 1);

        UINT8_TO_STREAM(pp, filt_type);
    }

    btu_hcif_send_cmd(p);
}

/* 0x8048EAE8 (0x74): Sets whether the host's PIN is fixed or variable. */
u8 btsnd_hcic_write_pin_type(u8 type) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_PIN_TYPE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, type);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048EB5C (0x60): Reads the stored link key of `bd_addr` (or every key), in `p`. */
void btsnd_hcic_read_stored_key(BT_HDR* p, u8* bd_addr, u8 read_all_flag) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_STORED_KEY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_STORED_LINK_KEY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_STORED_KEY);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT8_TO_STREAM(pp, read_all_flag);

    btu_hcif_send_cmd(p);
}

/* 0x8048EBBC (0x1C0): Stores `num_keys` address/link key pairs in the controller, in `p`. */
void btsnd_hcic_write_stored_key(BT_HDR* p, u8 num_keys, u8 (*bd_addr)[BD_ADDR_LEN], u8 (*link_key)[LINK_KEY_LEN]) {
    u8* pp = ((BT_BUF*)p)->data;
    int j;

    p->len = HCIC_PREAMBLE_SIZE + 1 + (num_keys * (BD_ADDR_LEN + LINK_KEY_LEN));
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_STORED_LINK_KEY);
    UINT8_TO_STREAM(pp, p->len - HCIC_PREAMBLE_SIZE);

    if (num_keys > HCI_MAX_NUM_OF_LINK_KEYS_PER_CMD) {
        num_keys = HCI_MAX_NUM_OF_LINK_KEYS_PER_CMD;
    }

    UINT8_TO_STREAM(pp, num_keys);

    for (j = 0; j < num_keys; j++) {
        BDADDR_TO_STREAM(pp, bd_addr[j]);
        ARRAY16_TO_STREAM(pp, link_key[j]);
    }

    btu_hcif_send_cmd(p);
}

/* 0x8048ED7C (0xB0): Deletes the stored link key of `bd_addr` (or every key). */
u8 btsnd_hcic_delete_stored_key(u8* bd_addr, u8 delete_all_flag) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_DELETE_STORED_KEY;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_DELETE_STORED_LINK_KEY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_DELETE_STORED_KEY);

    BDADDR_TO_STREAM(pp, bd_addr);
    UINT8_TO_STREAM(pp, delete_all_flag);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048EE2C (0x140): Sets the host's device name to the NUL-terminated `p_name`, in `p`. */
void btsnd_hcic_change_name(BT_HDR* p, u8* p_name) {
    u8* pp = ((BT_BUF*)p)->data;
    u16 len = (u16)(strlen((char*)p_name) + 1);

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CHANGE_NAME;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_CHANGE_LOCAL_NAME);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CHANGE_NAME);

    ARRAY_TO_STREAM(pp, p_name, len);

    btu_hcif_send_cmd(p);
}

/* 0x8048EF6C (0x38): Sets the page timeout (0.625 ms slots), in `p`. */
void btsnd_hcic_write_page_tout(BT_HDR* p, u16 timeout) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM2;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_PAGE_TOUT);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM2);

    UINT16_TO_STREAM(pp, timeout);

    btu_hcif_send_cmd(p);
}

/* 0x8048EFA4 (0x30): Sets which of inquiry and page scans run, in `p`. */
void btsnd_hcic_write_scan_enable(BT_HDR* p, u8 flag) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_SCAN_ENABLE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, flag);

    btu_hcif_send_cmd(p);
}

/* 0x8048EFD4 (0x44): Sets the page scan interval and window, in `p`. */
void btsnd_hcic_write_pagescan_cfg(BT_HDR* p, u16 interval, u16 window) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_PAGESCAN_CFG);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PAGESCAN_CFG);

    UINT16_TO_STREAM(pp, interval);
    UINT16_TO_STREAM(pp, window);

    btu_hcif_send_cmd(p);
}

/* 0x8048F018 (0x44): Sets the inquiry scan interval and window, in `p`. */
void btsnd_hcic_write_inqscan_cfg(BT_HDR* p, u16 interval, u16 window) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQSCAN_CFG;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_INQUIRYSCAN_CFG);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_INQSCAN_CFG);

    UINT16_TO_STREAM(pp, interval);
    UINT16_TO_STREAM(pp, window);

    btu_hcif_send_cmd(p);
}

/* 0x8048F05C (0x74): Turns authentication of every connection on or off. */
u8 btsnd_hcic_write_auth_enable(u8 flag) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_AUTHENTICATION_ENABLE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, flag);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F0D0 (0x74): Sets the encryption mode of every connection. */
u8 btsnd_hcic_write_encr_mode(u8 mode) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_ENCRYPTION_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, mode);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F144 (0x44): Sets the host's class of device, in `p`. */
void btsnd_hcic_write_dev_class(BT_HDR* p, u8* dev_class) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM3;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_CLASS_OF_DEVICE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM3);

    DEVCLASS_TO_STREAM(pp, dev_class);

    btu_hcif_send_cmd(p);
}

/* 0x8048F188 (0x44): Sets the automatic flush timeout (0.625 ms slots) of the link `handle`, in `p`. */
void btsnd_hcic_write_auto_flush_tout(BT_HDR* p, u16 handle, u16 tout) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_AUTO_FLUSH_TOUT;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_AUTO_FLUSH_TOUT);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_AUTO_FLUSH_TOUT);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, tout);

    btu_hcif_send_cmd(p);
}

/* 0x8048F1CC (0xBC): Tells the controller the host's ACL and SCO buffer sizes and counts. */
u8 btsnd_hcic_set_host_buf_size(u16 acl_len, u8 sco_len, u16 acl_num, u16 sco_num) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SET_HOST_BUF_SIZE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SET_HC_TO_HOST_BUF_SIZE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_SET_HOST_BUF_SIZE);

    UINT16_TO_STREAM(pp, acl_len);
    UINT8_TO_STREAM(pp, sco_len);
    UINT16_TO_STREAM(pp, acl_num);
    UINT16_TO_STREAM(pp, sco_num);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F288 (0x94): Sets the supervision timeout of the link `handle`. */
u8 btsnd_hcic_write_link_super_tout(u16 handle, u16 timeout) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_LINK_SUPER_TOUT;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_LINK_SUPER_TOUT);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_LINK_SUPER_TOUT);

    UINT16_TO_STREAM(pp, handle);
    UINT16_TO_STREAM(pp, timeout);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F31C (0x70): Sets the `num_cur_iac` inquiry access codes the host scans for, in `p`. */
void btsnd_hcic_write_cur_iac_lap(BT_HDR* p, u8 num_cur_iac, u8 (*iac_lap)[LAP_LEN]) {
    u8* pp = ((BT_BUF*)p)->data;
    int i;

    p->len = HCIC_PREAMBLE_SIZE + 1 + (LAP_LEN * num_cur_iac);
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_CURRENT_IAC_LAP);
    UINT8_TO_STREAM(pp, p->len - HCIC_PREAMBLE_SIZE);

    UINT8_TO_STREAM(pp, num_cur_iac);

    for (i = 0; i < num_cur_iac; i++) {
        LAP_TO_STREAM(pp, iac_lap[i]);
    }

    btu_hcif_send_cmd(p);
}

/* 0x8048F38C (0x60): Reads the controller's version information. */
u8 btsnd_hcic_read_local_ver(void) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_LOCAL_VERSION_INFO);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F3EC (0x5C): Reads the controller's supported features. */
u8 btsnd_hcic_read_local_features(void) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_LOCAL_FEATURES);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F448 (0x28): Reads the controller's ACL and SCO buffer sizes, in `p`. */
void btsnd_hcic_read_buffer_size(BT_HDR* p) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_BUFFER_SIZE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd(p);
}

/* 0x8048F470 (0x60): Reads the controller's device address. */
u8 btsnd_hcic_read_bd_addr(void) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_READ_CMD;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_BD_ADDR);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_READ_CMD);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F4D0 (0x7C): Reads the link quality of the link `handle`. */
u8 btsnd_hcic_get_link_quality(u16 handle) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CMD_HANDLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_GET_LINK_QUALITY);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CMD_HANDLE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F54C (0x78): Reads the RSSI of the link `handle`. */
u8 btsnd_hcic_read_rssi(u16 handle) {
    BT_HDR* p;
    u8* pp;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CMD_HANDLE;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_READ_RSSI);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_CMD_HANDLE);

    UINT16_TO_STREAM(pp, handle);

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F5C4 (0x420): Marks the channels `first`..`last` bad in the AFH channel map and sends it. */
u8 btsnd_hcic_set_afh_channels(u8 first, u8 last) {
    BT_HDR* p;
    u8* pp;
    u8 channels[AFH_CHANNEL_MAP_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F};
    int i;

    HCIC_GETBUF(p);
    pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_SET_AFH_CHANNELS;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_SET_AFH_CHANNELS);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_SET_AFH_CHANNELS);

    if (first <= last && last <= AFH_CHANNEL_MAX) {
        for (i = first; i <= last; i++) {
            channels[i / 8] &= ~(1 << (i % 8));
        }
    }

    for (i = 0; i < AFH_CHANNEL_MAP_LEN; i++) {
        UINT8_TO_STREAM(pp, channels[i]);
    }

    btu_hcif_send_cmd(p);
    return TRUE;
}

/* 0x8048F9E4 (0x30): Sets the inquiry scan type, in `p`. */
void btsnd_hcic_write_inqscan_type(BT_HDR* p, u8 type) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_INQSCAN_TYPE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, type);

    btu_hcif_send_cmd(p);
}

/* 0x8048FA14 (0x30): Sets the inquiry result format, in `p`. */
void btsnd_hcic_write_inquiry_mode(BT_HDR* p, u8 mode) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_INQUIRY_MODE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, mode);

    btu_hcif_send_cmd(p);
}

/* 0x8048FA44 (0x30): Sets the page scan type, in `p`. */
void btsnd_hcic_write_pagescan_type(BT_HDR* p, u8 type) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_PARAM1;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_WRITE_PAGESCAN_TYPE);
    UINT8_TO_STREAM(pp, HCIC_PARAM_SIZE_WRITE_PARAM1);

    UINT8_TO_STREAM(pp, type);

    btu_hcif_send_cmd(p);
}

/* 0x8048FA74 (0xF4): Sends the vendor specific command `opcode` with the `len` parameter bytes of `p_data`, in `p`. */
void btsnd_hcic_vendor_spec_cmd(BT_HDR* p, u16 opcode, u8 len, u8* p_data) {
    u8* pp = ((BT_BUF*)p)->data;

    p->len = HCIC_PREAMBLE_SIZE + len;
    p->offset = 0;

    UINT16_TO_STREAM(pp, HCI_GRP_VENDOR_SPECIFIC | opcode);
    UINT8_TO_STREAM(pp, len);

    ARRAY_TO_STREAM(pp, p_data, len);

    btu_hcif_send_cmd(p);
}
