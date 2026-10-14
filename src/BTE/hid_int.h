/* BTE/hid_int.h - the HID device and HID host layers' types, constants and trace statements: the two control blocks
 *   the BTE hidd_*.c and hidh_*.c units share; each unit's own entry points are declared in its own header
 *   (BTE/hidd_api.h, BTE/hidh_conn.h, ...). */
#ifndef MHTRI_BTE_HID_INT_H
#define MHTRI_BTE_HID_INT_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_api.h"
#include "BTE/sdp_int.h"

/* The trace layer of HID (bits 16-23 of a trace mask) and its trace types. */
#define TRACE_HID_ERROR 0x001E0000
#define TRACE_HID_WARNING 0x001E0001
#define TRACE_HID_API 0x001E0002
#define TRACE_HID_EVENT 0x001E0003
#define TRACE_HID_DEBUG 0x001E0004
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define BT_TRACE_LEVEL_DEBUG 5

/* The L2CAP PSMs of the HID control and interrupt channels. */
#define HID_PSM_CONTROL 0x0011
#define HID_PSM_INTERRUPT 0x0013

/* The HID results (tHID_STATUS). */
#define HID_SUCCESS 0
#define HID_ERR_NOT_REGISTERED 1
#define HID_ERR_ALREADY_REGISTERED 2
#define HID_ERR_NO_RESOURCES 3
#define HID_ERR_NO_CONNECTION 4
#define HID_ERR_INVALID_PARAM 5
#define HID_ERR_UNSUPPORTED 6
#define HID_ERR_UNKNOWN_COMMAND 7
#define HID_ERR_CONGESTED 8
#define HID_ERR_CONN_IN_PROCESS 9
#define HID_ERR_ALREADY_CONN 10
#define HID_ERR_DISCONNECTING 11
#define HID_ERR_SET_CONNABLE_FAIL 12
#define HID_ERR_HOST_UNKNOWN 13
#define HID_ERR_L2CAP_FAILED 14
#define HID_ERR_AUTH_FAILED 15
#define HID_ERR_SDP_BUSY 16

/* The base a failed BTM power mode status is offset by before the HID device callback reports it (GUESS name). */
#define HID_DEV_PM_ERR_BASE 55

/* The HID connection states (tHID_CONN.conn_state) and flags (tHID_CONN.conn_flags). */
#define HID_CONN_STATE_UNUSED 0
#define HID_CONN_STATE_CONNECTING_CTRL 1
#define HID_CONN_STATE_CONNECTING_INTR 2
#define HID_CONN_STATE_CONFIG 3
#define HID_CONN_STATE_CONNECTED 4
#define HID_CONN_STATE_DISCONNECTING 5
#define HID_CONN_STATE_SECURITY 6
#define HID_CONN_FLAGS_IS_ORIG 0x01

/* The HCI power modes (tHID_DEV_PWR_MD.mode) and the BTU timer type of a timer whose `param` is its handler. */
#define HCI_MODE_ACTIVE 0
#define HCI_MODE_HOLD 1
#define HCI_MODE_SNIFF 2
#define HCI_MODE_PARK 3
#define BTU_TTYPE_USER_FUNC 22

/* The HID device events its callback receives. */
#define HID_DEV_EVT_OPEN 0
#define HID_DEV_EVT_CLOSE 1
#define HID_DEV_EVT_RETRYING 2
#define HID_DEV_EVT_MODE_CHG 3
#define HID_DEV_EVT_PM_FAILED 4

/* The HID device's connection sub-states (tHID_DEV_CTB.conn_substate); each indexes tHID_DEV_CTB.pm_params. */
#define HID_DEV_BUSY_CONN_ST 0
#define HID_DEV_IDLE_CONN_ST 1

/* The HID device's retry limit, repage window and inactivity timeout (seconds), and its BTM power manager id. */
#define HID_DEV_MAX_CONN_RETRY 15
#define HID_DEV_REPAGE_WIN 1
#define HID_DEV_INACT_TIMEOUT 60
#define BTM_PM_SET_ONLY_ID 0x80

/* The BTM results a power mode request returns. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1

/* size: 0x24 - one HID connection: the control and interrupt channels and the timer of the connection attempt. */
typedef struct tHID_CONN {
    /* +0x00 */ u8 conn_state;
    /* +0x01 */ u8 conn_flags;
    /* +0x02 */ u8 ctrl_id;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u16 ctrl_cid;
    /* +0x06 */ u16 intr_cid;
    /* +0x08 */ u16 rem_mtu_size;
    /* +0x0A */ u16 disc_reason;
    /* +0x0C */ TIMER_LIST_ENT timer_entry;
} tHID_CONN; /* size: 0x24 */

/* The HID device event callback: the event, its value and a pointer to its payload. */
typedef void (tHID_DEV_CBACK)(u8 event, u32 data, u8* p_data);

/* size: 0x04 - the current power mode of the HID device's link. */
typedef struct tHID_DEV_CURR_PM {
    /* +0x00 */ u8 mode;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 interval;
} tHID_DEV_CURR_PM; /* size: 0x04 */

/* size: 0x144 - the HID device control block (the .bss row is 0x148 B with its alignment). */
typedef struct tHID_DEV_CTB {
    /* +0x000 */ u8 host_addr[6];
    /* +0x006 */ u8 unused_0x006[2];
    /* +0x008 */ u8 dev_state;
    /* +0x009 */ u8 conn_tries;
    /* +0x00A */ u8 unused_0x00A[6];
    /* +0x010 */ tHID_CONN conn;
    /* +0x034 */ TIMER_LIST_ENT idle_tle;
    /* +0x04C */ BtmPmPwrMode pm_params[2];
    /* +0x060 */ u8 unused_0x060[10];
    /* +0x06A */ tHID_DEV_CURR_PM curr_pm;
    /* +0x06E */ u8 pm_ctrl_busy;
    /* +0x06F */ u8 conn_substate;
    /* +0x070 */ BtmPmPwrMode final_pm;
    /* +0x07A */ u8 unused_0x07A[0x4A];
    /* +0x0C4 */ tHID_DEV_CBACK* callback;
    /* +0x0C8 */ tL2CAP_CFG_INFO l2cap_cfg;
    /* +0x104 */ tL2CAP_CFG_INFO l2cap_intr_cfg;
    /* +0x140 */ u8 reg_flag;
    /* +0x141 */ u8 trace_level;
    /* +0x142 */ u8 pad_0x142[2];
} tHID_DEV_CTB; /* size: 0x144 */

/* The HID host events its device callback receives. */
#define HID_HDEV_EVT_OPEN 0
#define HID_HDEV_EVT_CLOSE 1
#define HID_HDEV_EVT_RETRYING 2
#define HID_HDEV_EVT_INTR_DATA 3
#define HID_HDEV_EVT_INTR_DATC 4
#define HID_HDEV_EVT_CTRL_DATA 5
#define HID_HDEV_EVT_CTRL_DATC 6
#define HID_HDEV_EVT_HANDSHAKE 7
#define HID_HDEV_EVT_VC_UNPLUG 8

/* The HID host's device count, the device states and the close reasons a failed L2CAP step reports. */
#define HID_HOST_MAX_DEVICES 16
#define HID_HOST_MAX_CONN_RETRY 0
#define HID_DEV_NO_CONN 0
#define HID_DEV_CONNECTED 1
#define HID_L2CAP_CONN_FAIL 0x0100
#define HID_L2CAP_REQ_FAIL 0x0200
#define HID_L2CAP_CFG_FAIL 0x0400

/* The HID host channel configuration: the MTU and the flush timeout it asks for. */
#define HID_HOST_MTU 640
#define HID_HOST_FLUSH_TO 0xFFFF

/* The HID host connection flags past HID_CONN_FLAGS_IS_ORIG. */
#define HID_CONN_FLAGS_HIS_CTRL_CFG_DONE 0x02
#define HID_CONN_FLAGS_MY_CTRL_CFG_DONE 0x04
#define HID_CONN_FLAGS_HIS_INTR_CFG_DONE 0x08
#define HID_CONN_FLAGS_MY_INTR_CFG_DONE 0x10
#define HID_CONN_FLAGS_ALL_CONFIGURED 0x1E
#define HID_CONN_FLAGS_CONGESTED 0x20

/* The device attribute bits (tHID_HOST_DEV_CTB.attr_mask, the SDP query's attribute mask). */
#define HID_VIRTUAL_CABLE 0x0001
#define HID_NORMALLY_CONNECTABLE 0x0002
#define HID_RECONN_INIT 0x0004
#define HID_SDP_DISABLE 0x0008
#define HID_BATTERY_POWER 0x0010
#define HID_REMOTE_WAKE 0x0020
#define HID_SUP_TOUT_AVLBL 0x0040
#define HID_SEC_REQUIRED 0x8000

/* The security services, protocol and multiplexer channels of the HID channels. */
#define BTM_SEC_SERVICE_HID_SEC_CTRL 32
#define BTM_SEC_SERVICE_HID_NOSEC_CTRL 33
#define BTM_SEC_SERVICE_HID_INTR 34
#define BTM_SEC_PROTO_HID 6
#define HID_SEC_CHN 1
#define HID_NOSEC_CHN 2
#define BTM_SEC_NONE 0

/* The L2CAP connect and configure results. */
#define L2CAP_CONN_OK 0
#define L2CAP_CONN_PENDING 1
#define L2CAP_CONN_SECURITY_BLOCK 3
#define L2CAP_CONN_NO_RESOURCES 4
#define L2CAP_CFG_OK 0

/* The HID transaction types and parameters. */
#define HID_TRANS_HANDSHAKE 0
#define HID_TRANS_CONTROL 1
#define HID_TRANS_GET_REPORT 4
#define HID_TRANS_SET_REPORT 5
#define HID_TRANS_GET_PROTOCOL 6
#define HID_TRANS_SET_PROTOCOL 7
#define HID_TRANS_GET_IDLE 8
#define HID_TRANS_SET_IDLE 9
#define HID_TRANS_DATA 10
#define HID_TRANS_DATAC 11
#define HID_PAR_CONTROL_VIRTUAL_CABLE_UNPLUG 5
#define HID_PAR_REP_TYPE_MASK 0x03
#define HID_PAR_GET_REP_BUFSIZE_FOLLOWS 0x08
#define HID_CONTROL_POOL_ID 2
#define HID_INTERRUPT_POOL_ID 2

/* The HCI disconnect reasons that report an authentication failure. */
#define HCI_ERR_AUTH_FAILURE 0x05
#define HCI_ERR_KEY_MISSING 0x06
#define HCI_ERR_HOST_REJECT_SECURITY 0x0E
#define HCI_ERR_REPEATED_ATTEMPTS 0x17
#define HCI_ERR_PAIRING_NOT_ALLOWED 0x18
#define HCI_ERR_ENCRY_MODE_NOT_ACCEPTABLE 0x25
#define HCI_ERR_UNIT_KEY_USED 0x26
#define HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED 0x29

/* The statuses the record query reports when no HID record or no descriptor list is found. */
#define HID_SDP_NO_SERV_UUID (SDP_ILLEGAL_PARAMETER + 1)
#define HID_SDP_MANDATORY_MISSING (SDP_ILLEGAL_PARAMETER + 2)

/* The HID service class and the HID record attributes the record query reads. */
#define UUID_SERVCLASS_HUMAN_INTERFACE 0x1124
#define ATTR_ID_SERVICE_NAME 0x0100
#define ATTR_ID_PROVIDER_NAME 0x0102
#define ATTR_ID_HID_DEVICE_RELNUM 0x0200
#define ATTR_ID_HID_PARSER_VERSION 0x0201
#define ATTR_ID_HID_DEVICE_SUBCLASS 0x0202
#define ATTR_ID_HID_COUNTRY_CODE 0x0203
#define ATTR_ID_HID_VIRTUAL_CABLE 0x0204
#define ATTR_ID_HID_RECONNECT_INITIATE 0x0205
#define ATTR_ID_HID_DESCRIPTOR_LIST 0x0206
#define ATTR_ID_HID_SDP_DISABLE 0x0208
#define ATTR_ID_HID_BATTERY_POWER 0x0209
#define ATTR_ID_HID_REMOTE_WAKE 0x020A
#define ATTR_ID_HID_LINK_SUPERVISION_TO 0x020C
#define ATTR_ID_HID_NORMALLY_CONNECTABLE 0x020D

/* The type and length fields of a discovered attribute's attr_len_type. */
#define SDP_DISC_ATTR_LEN(len_type) ((len_type) & 0x0FFF)
#define SDP_DISC_ATTR_TYPE(len_type) ((len_type) >> 12)

/* The longest service name, description and provider name the record query copies. */
#define HID_MAX_SVC_NAME_LEN 32
#define HID_MAX_SVC_DESCR_LEN 32
#define HID_MAX_PROV_NAME_LEN 32

/* size: 0x34 - one device known to the HID host. */
typedef struct tHID_HOST_DEV_CTB {
    /* +0x00 */ u8 in_use;
    /* +0x01 */ u8 addr[6];
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ u16 attr_mask;
    /* +0x0A */ u8 state;
    /* +0x0B */ u8 unused_0x0B;
    /* +0x0C */ u8 conn_tries;
    /* +0x0D */ u8 pad_0x0D[3];
    /* +0x10 */ tHID_CONN conn;
} tHID_HOST_DEV_CTB; /* size: 0x34 */

/* size: 0x08 - the report descriptor list of a HID record. */
typedef struct tHID_DEV_DSCP_INFO {
    /* +0x00 */ u16 dl_len;
    /* +0x04 */ u8* dsc_list;
} tHID_DEV_DSCP_INFO; /* size: 0x08 */

/* size: 0x74 - the HID record the record query reads out of a device. */
typedef struct tHID_DEV_SDP_INFO {
    /* +0x00 */ char svc_name[HID_MAX_SVC_NAME_LEN];
    /* +0x20 */ char svc_descr[HID_MAX_SVC_DESCR_LEN];
    /* +0x40 */ char prov_name[HID_MAX_PROV_NAME_LEN];
    /* +0x60 */ u16 rel_num;
    /* +0x62 */ u16 hpars_ver;
    /* +0x64 */ u8 sub_class;
    /* +0x65 */ u8 ctry_code;
    /* +0x66 */ u16 sup_timeout;
    /* +0x68 */ tHID_DEV_DSCP_INFO dscp_info;
    /* +0x70 */ tSDP_DISC_REC* p_sdp_layer_rec;
} tHID_DEV_SDP_INFO; /* size: 0x74 */

/* The HID host device event callback: the device handle, the event, its value and the received buffer. */
typedef void (tHID_HOST_DEV_CALLBACK)(u8 dev_handle, u8 event, u32 data, BT_HDR* p_buf);
/* The completion callback of the HID record query: the SDP status, the attribute mask and the record. */
typedef void (tHID_HOST_SDP_CALLBACK)(u16 result, u16 attr_mask, tHID_DEV_SDP_INFO* sdp_rec);

/* size: 0x404 - the HID host control block (the .bss row is 0x408 B with its alignment). */
typedef struct tHID_HOST_CB {
    /* +0x000 */ tHID_HOST_DEV_CTB devices[HID_HOST_MAX_DEVICES];
    /* +0x340 */ tHID_HOST_DEV_CALLBACK* callback;
    /* +0x344 */ tL2CAP_CFG_INFO l2cap_cfg;
    /* +0x380 */ u8 sdp_busy;
    /* +0x381 */ u8 pad_0x381[3];
    /* +0x384 */ tHID_HOST_SDP_CALLBACK* sdp_cback;
    /* +0x388 */ tSDP_DISCOVERY_DB* p_sdp_db;
    /* +0x38C */ tHID_DEV_SDP_INFO sdp_rec;
    /* +0x400 */ u8 reg_flag;
    /* +0x401 */ u8 trace_level;
    /* +0x402 */ u8 pad_0x402[2];
} tHID_HOST_CB; /* size: 0x404 */

#define HIDD_TRACE_WARNING0(m) {if (hd_cb.trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg_0(TRACE_HID_WARNING, (m));}
#define HIDD_TRACE_EVENT0(m) {if (hd_cb.trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_HID_EVENT, (m));}
#define HIDD_TRACE_DEBUG0(m) {if (hd_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) LogMsg_0(TRACE_HID_DEBUG, (m));}

#define HIDH_TRACE_ERROR0(m) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg_0(TRACE_HID_ERROR, (m));}
#define HIDH_TRACE_ERROR1(m, p1) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_1(TRACE_HID_ERROR, (m), (u32)(p1));}
#define HIDH_TRACE_WARNING0(m) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg_0(TRACE_HID_WARNING, (m));}
#define HIDH_TRACE_WARNING1(m, p1) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_1(TRACE_HID_WARNING, (m), (u32)(p1));}
#define HIDH_TRACE_EVENT0(m) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_HID_EVENT, (m));}
#define HIDH_TRACE_EVENT1(m, p1) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_HID_EVENT, (m), (u32)(p1));}
#define HIDH_TRACE_EVENT2(m, p1, p2) {if (hh_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_2(TRACE_HID_EVENT, (m), (u32)(p1), (u32)(p2));}

#endif /* MHTRI_BTE_HID_INT_H */
