/*
 * BTE/btu_hcif.c - the BTU HCI interface: the HCI event dispatcher, command send and queueing, and the per-event
 *   handlers (connection, QoS, eSCO, command complete/status, command timeout, link key notification).
 *
 * RANGE. .text 0x8048AEE0-0x8048C13C (12 functions, 0x125C B); .data 0x80615140-0x806151D0. The .text holds the BTE
 *   btu_hcif.c functions btu_hcif_process_event..btu_hcif_link_key_notification_evt; btu_init.c's btu_init_core
 *   follows. The start is pinned by the .data: the last BTE/gki_time.c .data reader is 0x8048ADCC and
 *   btu_hcif_process_event is the first reader of 0x80615140 (no pad beyond 4-byte alignment, so the seam is where the
 *   readers change). Every .data object is read only by this range's functions (the four trace strings).
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, the compiler of every measured BTE unit (BTE/l2c_utils.c FLAGS line).
 * NAMES. the map's and the BTE source's. The small per-event handlers (disconnection, authentication, remote name,
 *   encryption, master key, hardware error, role change, mode change, return link keys, PIN and link key requests,
 *   clock offset, QoS violation, eSCO change) are static and inlined into btu_hcif_process_event, as in retail.
 * RESIDUALS. none known; Wii/1.3 does not reproduce the dispatcher and the handlers.
 * SHAPES. plain C.
 */

#include "BTE/btu_hcif.h"
#include "BTE/bd.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_main.h"
#include "BTE/btm_sco.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_api.h"
#include "BTE/l2c_link.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "BTE/hidd_pm.h"   /* hidd_pm_proc_mode_change */
#include "BTE/btm_link_key.h"   /* btm_return_link_keys_evt, btm_read_stored_link_key_complete, btm_write_stored_link_key_complete, btm_delete_stored_link_key_complete, btm_report_device_status */

#define DEV_CLASS_LEN 3
#define LINK_KEY_LEN 16

/* The HCI event codes this dispatcher handles. */
#define HCI_INQUIRY_COMP_EVT 0x01
#define HCI_INQUIRY_RESULT_EVT 0x02
#define HCI_CONNECTION_COMP_EVT 0x03
#define HCI_CONNECTION_REQUEST_EVT 0x04
#define HCI_DISCONNECTION_COMP_EVT 0x05
#define HCI_AUTHENTICATION_COMP_EVT 0x06
#define HCI_RMT_NAME_REQUEST_COMP_EVT 0x07
#define HCI_ENCRYPTION_CHANGE_EVT 0x08
#define HCI_MASTER_LINK_KEY_COMP_EVT 0x0A
#define HCI_READ_RMT_FEATURES_COMP_EVT 0x0B
#define HCI_READ_RMT_VERSION_COMP_EVT 0x0C
#define HCI_QOS_SETUP_COMP_EVT 0x0D
#define HCI_COMMAND_COMPLETE_EVT 0x0E
#define HCI_COMMAND_STATUS_EVT 0x0F
#define HCI_HARDWARE_ERROR_EVT 0x10
#define HCI_ROLE_CHANGE_EVT 0x12
#define HCI_NUM_COMPL_DATA_PKTS_EVT 0x13
#define HCI_MODE_CHANGE_EVT 0x14
#define HCI_RETURN_LINK_KEYS_EVT 0x15
#define HCI_PIN_CODE_REQUEST_EVT 0x16
#define HCI_LINK_KEY_REQUEST_EVT 0x17
#define HCI_LINK_KEY_NOTIFICATION_EVT 0x18
#define HCI_READ_CLOCK_OFF_COMP_EVT 0x1C
#define HCI_QOS_VIOLATION_EVT 0x1E
#define HCI_FLOW_SPECIFICATION_COMP_EVT 0x21
#define HCI_INQUIRY_RSSI_RESULT_EVT 0x22
#define HCI_ESCO_CONNECTION_COMP_EVT 0x2C
#define HCI_ESCO_CONNECTION_CHANGED_EVT 0x2D
#define HCI_VENDOR_EVT_EF 0xEF
#define HCI_VENDOR_SPECIFIC_EVT 0xFF

/* The HCI commands whose completion or status this layer handles. */
#define HCI_GRP_VENDOR_SPECIFIC (0x3F << 10)
#define HCI_COMMAND_NONE 0x0000
#define HCI_INQUIRY 0x0401
#define HCI_INQUIRY_CANCEL 0x0402
#define HCI_CREATE_CONNECTION 0x0405
#define HCI_RMT_NAME_REQUEST 0x0419
#define HCI_SETUP_ESCO_CONNECTION 0x0428
#define HCI_HOLD_MODE 0x0801
#define HCI_SNIFF_MODE 0x0803
#define HCI_EXIT_SNIFF_MODE 0x0804
#define HCI_PARK_MODE 0x0805
#define HCI_EXIT_PARK_MODE 0x0806
#define HCI_SWITCH_ROLE 0x080B
#define HCI_READ_POLICY_SETTINGS 0x080C
#define HCI_RESET 0x0C03
#define HCI_SET_EVENT_FILTER 0x0C05
#define HCI_READ_STORED_LINK_KEY 0x0C0D
#define HCI_WRITE_STORED_LINK_KEY 0x0C11
#define HCI_DELETE_STORED_LINK_KEY 0x0C12
#define HCI_READ_LOCAL_NAME 0x0C14
#define HCI_HOST_NUM_PACKETS_DONE 0x0C35
#define HCI_READ_LOCAL_VERSION_INFO 0x1001
#define HCI_READ_LOCAL_FEATURES 0x1003
#define HCI_READ_BUFFER_SIZE 0x1005
#define HCI_READ_BD_ADDR 0x1009
#define HCI_GET_LINK_QUALITY 0x1403
#define HCI_READ_RSSI 0x1405
/* The vendor command whose completion may arrive ahead of an older queued command. */
#define HCI_VSC_OUT_OF_ORDER 0xFC4C

#define HCI_SUCCESS 0x00
#define HCI_ERR_UNSPECIFIED 0x1F
#define HCI_LINK_TYPE_ACL 0x01
#define HCI_INVALID_HANDLE 0xFFFF
#define BTM_ROLE_UNDEFINED 0xFF
#define BTM_INQ_RESULT_STANDARD 0
#define BTM_INQ_RESULT_WITH_RSSI 1
#define BTM_CB_EVT_RETURN_LINK_KEYS 1
#define BTM_DEV_STATUS_CMD_TOUT 2
#define BTM_DEV_STATE_WAIT_RESET_CMPLT 0

/* The GKI pool a sent command's copy is taken from, and the transport event of an HCI command. */
#define HCI_CMD_POOL_ID 2
#define BT_EVT_TO_LM_HCI_CMD 0x2000

/* The command complete timer: its timer type and its timeout (s). */
#define BTU_TTYPE_HCI_CMD_CMPL 60
#define BTU_CMD_CMPL_TIMEOUT 8

/* The HCI layer's trace masks. */
#define TRACE_HCI_ERROR 0x70000
#define TRACE_HCI_WARNING 0x70001

/* An HCI connection handle without its packet boundary and broadcast flags. */
#define HCID_GET_HANDLE(u16v) ((u16)((u16v) & 0x0FFF))

/* The little-endian stream readers; each advances `p`. Addresses, classes and keys arrive reversed. */
#define STREAM_TO_UINT8(u8v, p) {u8v = (u8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16v, p) {u16v = ((u16)(*(p)) + (((u16)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32v, p) {u32v = (((u32)(*(p))) + ((((u32)(*((p) + 1)))) << 8) + \
    ((((u32)(*((p) + 2)))) << 16) + ((((u32)(*((p) + 3)))) << 24)); (p) += 4;}
#define STREAM_TO_BDADDR(a, p) {int ijk; u8* pbda = (u8*)(a) + BD_ADDR_LEN - 1; \
    for (ijk = 0; ijk < BD_ADDR_LEN; ijk++) *pbda-- = *(p)++;}
#define STREAM_TO_DEVCLASS(a, p) {int ijk; u8* _pa = (u8*)(a) + DEV_CLASS_LEN - 1; \
    for (ijk = 0; ijk < DEV_CLASS_LEN; ijk++) *_pa-- = *(p)++;}
#define STREAM_TO_ARRAY16(a, p) {int ijk; u8* _pa = (u8*)(a) + 15; for (ijk = 0; ijk < 16; ijk++) *_pa-- = *(p)++;}

static void btu_hcif_inquiry_comp_evt(u8* p);
static void btu_hcif_inquiry_result_evt(u8* p);
static void btu_hcif_inquiry_rssi_result_evt(u8* p);
void btu_hcif_connection_comp_evt(u8* p);
void btu_hcif_connection_request_evt(u8* p);
static void btu_hcif_disconnection_comp_evt(u8* p);
static void btu_hcif_authentication_comp_evt(u8* p);
static void btu_hcif_rmt_name_request_comp_evt(u8* p, u16 evt_len);
static void btu_hcif_encryption_change_evt(u8* p);
static void btu_hcif_master_link_key_comp_evt(u8* p);
static void btu_hcif_read_rmt_features_comp_evt(u8* p);
static void btu_hcif_read_rmt_version_comp_evt(u8* p);
void btu_hcif_qos_setup_comp_evt(u8* p);
void btu_hcif_command_complete_evt(u8* p, u16 evt_len);
void btu_hcif_command_status_evt(u8* p);
static void btu_hcif_hardware_error_evt(u8* p);
static void btu_hcif_role_change_evt(u8* p);
static void btu_hcif_num_compl_data_pkts_evt(u8* p);
static void btu_hcif_mode_change_evt(u8* p);
static void btu_hcif_return_link_keys_evt(u8* p);
static void btu_hcif_pin_code_request_evt(u8* p);
static void btu_hcif_link_key_request_evt(u8* p);
void btu_hcif_link_key_notification_evt(u8* p);
static void btu_hcif_read_clock_off_comp_evt(u8* p);
static void btu_hcif_qos_violation_evt(u8* p);
static void btu_hcif_flow_spec_comp_evt(u8* p, u16 evt_len);
void btu_hcif_esco_connection_comp_evt(u8* p);
static void btu_hcif_esco_connection_chg_evt(u8* p);
static void btu_hcif_vendor_specific_evt(u8* p, u8 evt_len);
void btu_hcif_hdl_command_complete(u16 opcode, u8* p, u16 evt_len);
void btu_hcif_hdl_command_status(u16 opcode, u8 status, u8* p_cmd);

/* 0x8048AEE0 (0x5B8): Dispatches the HCI event in `p_msg` to its handler; before the reset completes only command
 * complete and command status events pass. */
void btu_hcif_process_event(BT_HDR* p_msg) {
    u8* p = &((BT_BUF*)p_msg)->data[p_msg->offset];
    u8 hci_evt_code;
    u8 hci_evt_len;

    STREAM_TO_UINT8(hci_evt_code, p);
    STREAM_TO_UINT8(hci_evt_len, p);

    if (btm_cb.devcb_state == BTM_DEV_STATE_WAIT_RESET_CMPLT && hci_evt_code != HCI_COMMAND_COMPLETE_EVT &&
        hci_evt_code != HCI_COMMAND_STATUS_EVT) {
        return;
    }

    switch (hci_evt_code) {
    case HCI_INQUIRY_COMP_EVT:
        btu_hcif_inquiry_comp_evt(p);
        break;
    case HCI_INQUIRY_RESULT_EVT:
        btu_hcif_inquiry_result_evt(p);
        break;
    case HCI_INQUIRY_RSSI_RESULT_EVT:
        btu_hcif_inquiry_rssi_result_evt(p);
        break;
    case HCI_CONNECTION_COMP_EVT:
        btu_hcif_connection_comp_evt(p);
        break;
    case HCI_CONNECTION_REQUEST_EVT:
        btu_hcif_connection_request_evt(p);
        break;
    case HCI_DISCONNECTION_COMP_EVT:
        btu_hcif_disconnection_comp_evt(p);
        break;
    case HCI_AUTHENTICATION_COMP_EVT:
        btu_hcif_authentication_comp_evt(p);
        break;
    case HCI_RMT_NAME_REQUEST_COMP_EVT:
        btu_hcif_rmt_name_request_comp_evt(p, hci_evt_len);
        break;
    case HCI_ENCRYPTION_CHANGE_EVT:
        btu_hcif_encryption_change_evt(p);
        break;
    case HCI_MASTER_LINK_KEY_COMP_EVT:
        btu_hcif_master_link_key_comp_evt(p);
        break;
    case HCI_READ_RMT_FEATURES_COMP_EVT:
        btu_hcif_read_rmt_features_comp_evt(p);
        break;
    case HCI_READ_RMT_VERSION_COMP_EVT:
        btu_hcif_read_rmt_version_comp_evt(p);
        break;
    case HCI_QOS_SETUP_COMP_EVT:
        btu_hcif_qos_setup_comp_evt(p);
        break;
    case HCI_COMMAND_COMPLETE_EVT:
        btu_hcif_command_complete_evt(p, hci_evt_len);
        break;
    case HCI_COMMAND_STATUS_EVT:
        btu_hcif_command_status_evt(p);
        break;
    case HCI_HARDWARE_ERROR_EVT:
        btu_hcif_hardware_error_evt(p);
        break;
    case HCI_ROLE_CHANGE_EVT:
        btu_hcif_role_change_evt(p);
        break;
    case HCI_NUM_COMPL_DATA_PKTS_EVT:
        btu_hcif_num_compl_data_pkts_evt(p);
        break;
    case HCI_MODE_CHANGE_EVT:
        btu_hcif_mode_change_evt(p);
        break;
    case HCI_RETURN_LINK_KEYS_EVT:
        btu_hcif_return_link_keys_evt(p);
        break;
    case HCI_PIN_CODE_REQUEST_EVT:
        btu_hcif_pin_code_request_evt(p);
        break;
    case HCI_LINK_KEY_REQUEST_EVT:
        btu_hcif_link_key_request_evt(p);
        break;
    case HCI_LINK_KEY_NOTIFICATION_EVT:
        btu_hcif_link_key_notification_evt(p);
        break;
    case HCI_READ_CLOCK_OFF_COMP_EVT:
        btu_hcif_read_clock_off_comp_evt(p);
        break;
    case HCI_QOS_VIOLATION_EVT:
        btu_hcif_qos_violation_evt(p);
        break;
    case HCI_FLOW_SPECIFICATION_COMP_EVT:
        btu_hcif_flow_spec_comp_evt(p, hci_evt_len);
        break;
    case HCI_ESCO_CONNECTION_COMP_EVT:
        btu_hcif_esco_connection_comp_evt(p);
        break;
    case HCI_ESCO_CONNECTION_CHANGED_EVT:
        btu_hcif_esco_connection_chg_evt(p);
        break;
    case HCI_VENDOR_EVT_EF:
    case HCI_VENDOR_SPECIFIC_EVT:
        btu_hcif_vendor_specific_evt(p, hci_evt_len);
        break;
    }
}

/* 0x8048B498 (0x18C): Sends queued HCI commands while the controller has credits; `p_buf` joins the queue first, and
 * each sent command waits on the command complete queue under the command timer. */
void btu_hcif_send_cmd(BT_HDR* p_buf) {
    BT_HDR* p_cmd;
    u16 opcode;

    if (p_buf != NULL && btu_cb.cmd_xmit_q.count != 0) {
        GKI_enqueue(&btu_cb.cmd_xmit_q, p_buf);
        p_buf = NULL;
    }

    if (btu_cb.hci_cmd_window == 0 && btm_cb.devcb_state == BTM_DEV_STATE_WAIT_RESET_CMPLT) {
        btu_cb.hci_cmd_window = btu_cb.cmd_xmit_q.count + 1;
    }

    while (btu_cb.hci_cmd_window != 0) {
        if (p_buf == NULL) {
            p_buf = (BT_HDR*)GKI_dequeue(&btu_cb.cmd_xmit_q);
        }

        if (p_buf == NULL) {
            break;
        }

        opcode = ((BT_BUF*)p_buf)->data[p_buf->offset] + (((BT_BUF*)p_buf)->data[p_buf->offset + 1] << 8);

        if (opcode != HCI_RESET && opcode != HCI_HOST_NUM_PACKETS_DONE) {
            if ((p_cmd = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
                memcpy(p_cmd, p_buf, sizeof(BT_HDR));
                memcpy(&((BT_BUF*)p_cmd)->data[p_cmd->offset], &((BT_BUF*)p_buf)->data[p_buf->offset], p_buf->len);
                GKI_enqueue(&btu_cb.cmd_cmpl_q, p_cmd);
                btu_start_timer(&btu_cb.cmd_cmpl_timer, BTU_TTYPE_HCI_CMD_CMPL, BTU_CMD_CMPL_TIMEOUT);
            }
        }

        btu_cb.hci_cmd_window--;
        bte_hcisu_send(p_buf, BT_EVT_TO_LM_HCI_CMD);
        p_buf = NULL;
    }

    if (p_buf != NULL) {
        GKI_enqueue(&btu_cb.cmd_xmit_q, p_buf);
    }
}

/* Ends the inquiry with the event's status. */
static void btu_hcif_inquiry_comp_evt(u8* p) {
    u8 status;

    STREAM_TO_UINT8(status, p);
    btm_process_inq_complete(status);
}

/* Passes a standard inquiry result to the device manager. */
static void btu_hcif_inquiry_result_evt(u8* p) {
    btm_process_inq_results(p, BTM_INQ_RESULT_STANDARD);
}

/* Passes an inquiry result with RSSI to the device manager. */
static void btu_hcif_inquiry_rssi_result_evt(u8* p) {
    btm_process_inq_results(p, BTM_INQ_RESULT_WITH_RSSI);
}

/* 0x8048B624 (0xDC): Hands a connection complete to the security manager and L2CAP (ACL) or the SCO manager. */
void btu_hcif_connection_comp_evt(u8* p) {
    u8 status;
    u16 handle;
    BD_ADDR bda;
    u8 link_type;
    u8 enc_mode;
    tBTM_ESCO_DATA esco_data;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_BDADDR(bda, p);
    STREAM_TO_UINT8(link_type, p);
    STREAM_TO_UINT8(enc_mode, p);

    handle = HCID_GET_HANDLE(handle);

    if (link_type == HCI_LINK_TYPE_ACL) {
        btm_sec_connected(bda, handle, status, enc_mode);
        l2c_link_hci_conn_comp(status, handle, bda);
    } else {
        memset(&esco_data, 0, sizeof(tBTM_ESCO_DATA));
        memcpy(esco_data.bd_addr, bda, BD_ADDR_LEN);
        btm_sco_connected(status, bda, handle, &esco_data);
    }
}

/* 0x8048B700 (0x8C): Hands an incoming connection request to the security manager (ACL) or the SCO manager. */
void btu_hcif_connection_request_evt(u8* p) {
    BD_ADDR bda;
    u8 dc[DEV_CLASS_LEN];
    u8 link_type;

    STREAM_TO_BDADDR(bda, p);
    STREAM_TO_DEVCLASS(dc, p);
    STREAM_TO_UINT8(link_type, p);

    if (link_type == HCI_LINK_TYPE_ACL) {
        btm_sec_conn_req(bda, dc);
    } else {
        btm_sco_conn_req(bda, dc, link_type);
    }
}

/* Drops the disconnected link from L2CAP or the SCO manager and the security manager. */
static void btu_hcif_disconnection_comp_evt(u8* p) {
    u8 status;
    u16 handle;
    u8 reason;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT8(reason, p);

    handle = HCID_GET_HANDLE(handle);

    if (!l2c_link_hci_disc_comp(handle, reason)) {
        btm_sco_removed(handle, reason);
    }

    btm_sec_disconnected(handle, reason);
}

/* Passes an authentication complete to the security manager. */
static void btu_hcif_authentication_comp_evt(u8* p) {
    u8 status;
    u16 handle;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);

    btm_sec_auth_complete(handle, status);
}

/* Passes a remote name to the inquiry and security managers. */
static void btu_hcif_rmt_name_request_comp_evt(u8* p, u16 evt_len) {
    u8 status;
    BD_ADDR bd_addr;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_BDADDR(bd_addr, p);

    evt_len -= (1 + BD_ADDR_LEN);

    btm_process_remote_name(bd_addr, p, evt_len, status);
    btm_sec_rmt_name_request_complete(bd_addr, p, status);
}

/* Passes an encryption change to the ACL and security managers. */
static void btu_hcif_encryption_change_evt(u8* p) {
    u8 status;
    u16 handle;
    u8 encr_enable;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT8(encr_enable, p);

    btm_acl_encrypt_change(handle, status, encr_enable);
    btm_sec_encrypt_change(handle, status, encr_enable);
}

/* Passes a master link key complete to the security manager. */
static void btu_hcif_master_link_key_comp_evt(u8* p) {
    u8 status;
    u16 handle;
    u8 key_flg;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT8(key_flg, p);

    btm_sec_mkey_comp_event(handle, status, key_flg);
}

/* Passes the peer's features to the ACL manager. */
static void btu_hcif_read_rmt_features_comp_evt(u8* p) {
    btm_read_remote_features_complete(p);
}

/* Passes the peer's version to the ACL manager. */
static void btu_hcif_read_rmt_version_comp_evt(u8* p) {
    btm_read_remote_version_complete(p);
}

/* 0x8048B78C (0x100): Passes a QoS setup complete and its flow specification to the ACL manager. */
void btu_hcif_qos_setup_comp_evt(u8* p) {
    u8 status;
    u16 handle;
    FLOW_SPEC flow;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT8(flow.qos_flags, p);
    STREAM_TO_UINT8(flow.service_type, p);
    STREAM_TO_UINT32(flow.token_rate, p);
    STREAM_TO_UINT32(flow.peak_bandwidth, p);
    STREAM_TO_UINT32(flow.latency, p);
    STREAM_TO_UINT32(flow.delay_variation, p);

    btm_qos_setup_complete(status, handle, &flow);
}

/* 0x8048B88C (0xE0): Passes an eSCO connection complete and its link parameters to the SCO manager. */
void btu_hcif_esco_connection_comp_evt(u8* p) {
    tBTM_ESCO_DATA data;
    u16 handle;
    BD_ADDR bda;
    u8 status;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_BDADDR(bda, p);

    STREAM_TO_UINT8(data.link_type, p);
    STREAM_TO_UINT8(data.tx_interval, p);
    STREAM_TO_UINT8(data.retrans_window, p);
    STREAM_TO_UINT16(data.rx_pkt_len, p);
    STREAM_TO_UINT16(data.tx_pkt_len, p);
    STREAM_TO_UINT8(data.air_mode, p);

    memcpy(data.bd_addr, bda, BD_ADDR_LEN);
    btm_sco_connected(status, bda, handle, &data);
}

/* Passes an eSCO connection change to the SCO manager. */
static void btu_hcif_esco_connection_chg_evt(u8* p) {
    u16 handle;
    u16 tx_pkt_len;
    u16 rx_pkt_len;
    u8 status;
    u8 tx_interval;
    u8 retrans_window;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT8(tx_interval, p);
    STREAM_TO_UINT8(retrans_window, p);
    STREAM_TO_UINT16(rx_pkt_len, p);
    STREAM_TO_UINT16(tx_pkt_len, p);

    btm_esco_proc_conn_chg(status, handle, tx_interval, retrans_window, rx_pkt_len, tx_pkt_len);
}

/* 0x8048B96C (0x158): Passes the result of the completed command `opcode` to the manager that sent it. */
void btu_hcif_hdl_command_complete(u16 opcode, u8* p, u16 evt_len) {
    switch (opcode) {
    case HCI_RESET:
        btm_reset_complete();
        break;
    case HCI_INQUIRY_CANCEL:
        btm_process_inq_complete(HCI_SUCCESS);
        break;
    case HCI_SET_EVENT_FILTER:
        btm_event_filter_complete(p);
        break;
    case HCI_READ_STORED_LINK_KEY:
        btm_read_stored_link_key_complete(p);
        break;
    case HCI_WRITE_STORED_LINK_KEY:
        btm_write_stored_link_key_complete(p);
        break;
    case HCI_DELETE_STORED_LINK_KEY:
        btm_delete_stored_link_key_complete(p);
        break;
    case HCI_READ_LOCAL_VERSION_INFO:
        btm_read_local_version_complete(p, evt_len);
        break;
    case HCI_READ_POLICY_SETTINGS:
        btm_read_link_policy_complete(p);
        break;
    case HCI_READ_BUFFER_SIZE:
        btm_read_hci_buf_size_complete(p, evt_len);
        break;
    case HCI_READ_LOCAL_FEATURES:
        btm_read_local_features_complete(p, evt_len);
        break;
    case HCI_READ_LOCAL_NAME:
        btm_read_local_name_complete(p, evt_len);
        break;
    case HCI_READ_BD_ADDR:
        btm_read_local_addr_complete(p, evt_len);
        break;
    case HCI_GET_LINK_QUALITY:
        btm_read_link_quality_complete(p);
        break;
    case HCI_READ_RSSI:
        btm_read_rssi_complete(p);
        break;
    default:
        if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC) {
            btm_vsc_complete(p, opcode, evt_len);
        }
        break;
    }
}

/* 0x8048BAC4 (0x118): Takes the command complete's credits, retires the matching queued command and handles the
 * result. */
void btu_hcif_command_complete_evt(u8* p, u16 evt_len) {
    u16 cc_opcode;
    BT_HDR* p_cmd;
    u16 cmd_opcode;

    STREAM_TO_UINT8(btu_cb.hci_cmd_window, p);
    STREAM_TO_UINT16(cc_opcode, p);

    evt_len -= 3;

    if (cc_opcode != HCI_RESET && cc_opcode != HCI_HOST_NUM_PACKETS_DONE && cc_opcode != HCI_COMMAND_NONE) {
        if ((p_cmd = (BT_HDR*)GKI_dequeue(&btu_cb.cmd_cmpl_q)) != NULL) {
            cmd_opcode = ((BT_BUF*)p_cmd)->data[0] + (((BT_BUF*)p_cmd)->data[1] << 8);

            if (cc_opcode == HCI_VSC_OUT_OF_ORDER && cmd_opcode != cc_opcode) {
                GKI_enqueue_head(&btu_cb.cmd_cmpl_q, p_cmd);
                return;
            }

            GKI_freebuf(p_cmd);
        }

        if (!GKI_queue_is_empty(&btu_cb.cmd_cmpl_q)) {
            btu_start_timer(&btu_cb.cmd_cmpl_timer, BTU_TTYPE_HCI_CMD_CMPL, BTU_CMD_CMPL_TIMEOUT);
        } else {
            btu_stop_timer(&btu_cb.cmd_cmpl_timer);
        }
    }

    btu_hcif_hdl_command_complete(cc_opcode, p, evt_len);

    btu_hcif_send_cmd(NULL);
}

/* 0x8048BBDC (0x200): Passes a command status to the manager that sent `opcode`; `p_cmd` is the sent command's
 * parameters, from their length byte. */
void btu_hcif_hdl_command_status(u16 opcode, u8 status, u8* p_cmd) {
    BD_ADDR bd_addr;
    u16 handle;
    tBTM_ESCO_DATA esco_data;

    switch (opcode) {
    case HCI_HOLD_MODE:
    case HCI_SNIFF_MODE:
    case HCI_EXIT_SNIFF_MODE:
    case HCI_PARK_MODE:
    case HCI_EXIT_PARK_MODE:
        btm_pm_proc_cmd_status(status);
        break;

    default:
        if (status != HCI_SUCCESS) {
            switch (opcode) {
            case HCI_INQUIRY:
                btm_process_inq_complete(status);
                break;

            case HCI_RMT_NAME_REQUEST:
                btm_process_remote_name(NULL, NULL, 0, status);
                btm_sec_rmt_name_request_complete(NULL, NULL, status);
                break;

            case HCI_QOS_SETUP_COMP_EVT:
                btm_qos_setup_complete(status, 0, NULL);
                break;

            case HCI_SWITCH_ROLE:
                btm_acl_role_changed(status, NULL, BTM_ROLE_UNDEFINED);
                l2c_link_role_change_failed();
                break;

            case HCI_CREATE_CONNECTION:
                if (p_cmd != NULL) {
                    p_cmd++;
                    STREAM_TO_BDADDR(bd_addr, p_cmd);
                    btm_sec_connected(bd_addr, HCI_INVALID_HANDLE, status, 0);
                    l2c_link_hci_conn_comp(status, HCI_INVALID_HANDLE, bd_addr);
                }
                break;

            case HCI_SETUP_ESCO_CONNECTION:
                if (p_cmd != NULL) {
                    p_cmd++;
                    STREAM_TO_UINT16(handle, p_cmd);

                    if (btm_is_sco_active(handle)) {
                        btm_esco_proc_conn_chg(status, handle, 0, 0, 0, 0);
                    } else {
                        btm_sco_connected(status, NULL, handle, &esco_data);
                    }
                }
                break;

            default:
                if ((opcode & HCI_GRP_VENDOR_SPECIFIC) == HCI_GRP_VENDOR_SPECIFIC) {
                    btm_vsc_complete(p_cmd, opcode, 1);
                }
                break;
            }
        }
        break;
    }
}

/* 0x8048BDDC (0x128): Takes the command status's credits, retires the matching queued command and handles the
 * status. */
void btu_hcif_command_status_evt(u8* p) {
    u8 status;
    u16 opcode;
    u16 cmd_opcode;
    BT_HDR* p_cmd = NULL;
    u8* p_data = NULL;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT8(btu_cb.hci_cmd_window, p);
    STREAM_TO_UINT16(opcode, p);

    if (opcode != HCI_RESET && opcode != HCI_HOST_NUM_PACKETS_DONE && opcode != HCI_COMMAND_NONE) {
        if ((p_cmd = (BT_HDR*)GKI_dequeue(&btu_cb.cmd_cmpl_q)) != NULL) {
            p_data = &((BT_BUF*)p_cmd)->data[p_cmd->offset];
            STREAM_TO_UINT16(cmd_opcode, p_data);

            if (cmd_opcode != opcode) {
                p_data = NULL;
                LogMsg_2(TRACE_HCI_WARNING, "Event mismatch opcode=%X cmd opcode=%X", opcode, cmd_opcode);
            }
        }

        if (!GKI_queue_is_empty(&btu_cb.cmd_cmpl_q)) {
            btu_start_timer(&btu_cb.cmd_cmpl_timer, BTU_TTYPE_HCI_CMD_CMPL, BTU_CMD_CMPL_TIMEOUT);
        } else {
            btu_stop_timer(&btu_cb.cmd_cmpl_timer);
        }
    }

    btu_hcif_hdl_command_status(opcode, status, p_data);

    if (p_cmd != NULL) {
        GKI_freebuf(p_cmd);
    }

    btu_hcif_send_cmd(NULL);
}

/* 0x8048BF04 (0x158): Fails the oldest unanswered command with an unspecified error and reports the timeout. */
void btu_hcif_cmd_timeout(void) {
    BT_HDR* p_cmd;
    u8* p;
    u16 cmd_opcode;

    btu_cb.hci_cmd_window = 1;

    if ((p_cmd = (BT_HDR*)GKI_dequeue(&btu_cb.cmd_cmpl_q)) == NULL) {
        LogMsg_0(TRACE_HCI_WARNING, "Cmd timeout; no cmd in queue");
        return;
    }

    if (!GKI_queue_is_empty(&btu_cb.cmd_cmpl_q)) {
        btu_start_timer(&btu_cb.cmd_cmpl_timer, BTU_TTYPE_HCI_CMD_CMPL, BTU_CMD_CMPL_TIMEOUT);
    }

    p = &((BT_BUF*)p_cmd)->data[p_cmd->offset];
    STREAM_TO_UINT16(cmd_opcode, p);

    LogMsg_1(TRACE_HCI_WARNING, "BTU HCI command timeout - cmd opcode = 0x%02x", cmd_opcode);

    switch (cmd_opcode) {
    case HCI_INQUIRY:
    case HCI_QOS_SETUP_COMP_EVT:
    case HCI_RMT_NAME_REQUEST:
    case HCI_CREATE_CONNECTION:
    case HCI_HOLD_MODE:
    case HCI_SNIFF_MODE:
    case HCI_EXIT_SNIFF_MODE:
    case HCI_PARK_MODE:
    case HCI_EXIT_PARK_MODE:
        btu_hcif_hdl_command_status(cmd_opcode, HCI_ERR_UNSPECIFIED, p);
        break;

    default: {
        u8 status = HCI_ERR_UNSPECIFIED;
        btu_hcif_hdl_command_complete(cmd_opcode, &status, 1);
        break;
    }
    }

    GKI_freebuf(p_cmd);

    btm_report_device_status(BTM_DEV_STATUS_CMD_TOUT);

    btu_hcif_send_cmd(NULL);
}

/* Reports a controller hardware error and resets the controller when the device is up. */
static void btu_hcif_hardware_error_evt(u8* p) {
    LogMsg_0(TRACE_HCI_ERROR, "Ctlr H/w error event");

    if (BTM_IsDeviceUp()) {
        BTM_DeviceReset(NULL);
    }
}

/* Passes a role change to L2CAP (on success) and the ACL manager. */
static void btu_hcif_role_change_evt(u8* p) {
    u8 status;
    BD_ADDR bda;
    u8 role;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_BDADDR(bda, p);
    STREAM_TO_UINT8(role, p);

    if (status == HCI_SUCCESS) {
        l2c_link_role_changed(bda, role);
    }

    btm_acl_role_changed(status, bda, role);
}

/* Passes the completed packet counts to L2CAP. */
static void btu_hcif_num_compl_data_pkts_evt(u8* p) {
    l2c_link_process_num_completed_pkts(p);
}

/* Passes a mode change to the SCO, power and HID device managers. */
static void btu_hcif_mode_change_evt(u8* p) {
    u8 status;
    u16 handle;
    u8 current_mode;
    u16 interval;

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT8(current_mode, p);
    STREAM_TO_UINT16(interval, p);

    btm_sco_chk_pend_unpark(status, handle, current_mode);
    btm_pm_proc_mode_change(status, handle, current_mode, interval);
    hidd_pm_proc_mode_change(status, current_mode, interval);
}

/* Passes the returned link keys to the application, with the event byte written in front of the count. */
static void btu_hcif_return_link_keys_evt(u8* p) {
    u8 num_keys;

    num_keys = *p;

    if (!num_keys) {
        return;
    }

    p[-1] = BTM_CB_EVT_RETURN_LINK_KEYS;

    btm_return_link_keys_evt((tBTM_RETURN_LINK_KEYS_EVT*)(p - 1));
}

/* Passes a PIN code request to L2CAP and the security manager. */
static void btu_hcif_pin_code_request_evt(u8* p) {
    BD_ADDR bda;

    STREAM_TO_BDADDR(bda, p);

    l2c_pin_code_request(bda);
    btm_sec_pin_code_request(bda);
}

/* Passes a link key request to the security manager. */
static void btu_hcif_link_key_request_evt(u8* p) {
    BD_ADDR bda;

    STREAM_TO_BDADDR(bda, p);
    btm_sec_link_key_request(bda);
}

/* 0x8048C05C (0xE0): Passes a new link key and its type to the security manager. */
void btu_hcif_link_key_notification_evt(u8* p) {
    BD_ADDR bda;
    u8 key[LINK_KEY_LEN];
    u8 key_type;

    STREAM_TO_BDADDR(bda, p);
    STREAM_TO_ARRAY16(key, p);
    STREAM_TO_UINT8(key_type, p);

    btm_sec_link_key_notification(bda, key, key_type);
}

/* Passes a successful clock offset read to the ACL and security managers. */
static void btu_hcif_read_clock_off_comp_evt(u8* p) {
    u8 status;
    u16 handle;
    u16 clock_offset;

    STREAM_TO_UINT8(status, p);

    if (status != HCI_SUCCESS) {
        return;
    }

    STREAM_TO_UINT16(handle, p);
    STREAM_TO_UINT16(clock_offset, p);

    handle = HCID_GET_HANDLE(handle);

    btm_process_clk_off_comp_evt(handle, clock_offset);
    btm_sec_update_clock_offset(handle, clock_offset);
}

/* Passes a QoS violation to L2CAP. */
static void btu_hcif_qos_violation_evt(u8* p) {
    u16 handle;

    STREAM_TO_UINT16(handle, p);

    handle = HCID_GET_HANDLE(handle);

    l2c_link_hci_qos_violation(handle);
}

/* Ignores a flow specification complete. */
static void btu_hcif_flow_spec_comp_evt(u8* p, u16 evt_len) {
}

/* Passes a vendor specific event to the device manager. */
static void btu_hcif_vendor_specific_evt(u8* p, u8 evt_len) {
    btm_vendor_specific_evt(p, evt_len);
}
