/* BTE/bta_hh_int.h - the BTA HID host's message types, control blocks, events and trace statements the four
 *   BTE/bta_hh_*.c units share; each unit's own entry points are declared in its own header (BTE/bta_hh_act.h,
 *   BTE/bta_hh_api.h, BTE/bta_hh_main.h, BTE/bta_hh_utils.h). */
#ifndef MHTRI_BTE_BTA_HH_INT_H
#define MHTRI_BTE_BTA_HH_INT_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/sdp_int.h"
#include "BTE/hidh_api.h"
#include "BTE/bd.h"
#include "BTE/bta_hh_cfg.h"
#include "BTE/bta_sys_conn.h"
#include "BTE/bta_sys_main.h"
#include "BTE/utl.h"
#include "WUD/WUDHidHost.h"

/* The length of a device address. */
#ifndef BD_ADDR_LEN
#define BD_ADDR_LEN 6
#endif

/* The BTA subsystem id of the HID host; its internal events are (BTA_ID_HH << 8) + n. */
#define BTA_ID_HH 0x17

/* The internal events the state machine and bta_hh_hdl_event dispatch. */
#define BTA_HH_API_OPEN_EVT 0x1700
#define BTA_HH_API_CLOSE_EVT 0x1701
#define BTA_HH_INT_OPEN_EVT 0x1702
#define BTA_HH_INT_CLOSE_EVT 0x1703
#define BTA_HH_INT_DATA_EVT 0x1704
#define BTA_HH_INT_CTRL_DATA 0x1705
#define BTA_HH_INT_HANDSK_EVT 0x1706
#define BTA_HH_SDP_CMPL_EVT 0x1707
#define BTA_HH_API_WRITE_DEV_EVT 0x1708
#define BTA_HH_API_GET_DSCP_EVT 0x1709
#define BTA_HH_API_MAINT_DEV_EVT 0x170A
#define BTA_HH_OPEN_CMPL_EVT 0x170B
#define BTA_HH_API_ENABLE_EVT 0x170C
#define BTA_HH_API_DISABLE_EVT 0x170D
#define BTA_HH_API_GET_ACL_Q_EVT 0x170E
#define BTA_HH_DISC_CMPL_EVT 0x170F
#define BTA_HH_INVALID_EVT 0x1710

/* The events the application callback receives. */
#define BTA_HH_ENABLE_EVT 0
#define BTA_HH_DISABLE_EVT 1
#define BTA_HH_OPEN_EVT 2
#define BTA_HH_CLOSE_EVT 3
#define BTA_HH_GET_RPT_EVT 4
#define BTA_HH_SET_RPT_EVT 5
#define BTA_HH_GET_PROTO_EVT 6
#define BTA_HH_SET_PROTO_EVT 7
#define BTA_HH_GET_IDLE_EVT 8
#define BTA_HH_SET_IDLE_EVT 9
#define BTA_HH_GET_DSCP_EVT 10
#define BTA_HH_ADD_DEV_EVT 11
#define BTA_HH_RMV_DEV_EVT 12
#define BTA_HH_VC_UNPLUG_EVT 13
#define BTA_HH_GET_ACL_Q_EVT 15

/* The first HID transaction type and the first application event it maps to (one-to-one). */
#define BTA_HH_FST_BTE_TRANS_EVT HID_TRANS_GET_REPORT
#define BTA_HH_FST_TRANS_CB_EVT BTA_HH_GET_RPT_EVT

/* The status codes the application callback receives. */
#define BTA_HH_OK 0
#define BTA_HH_HS_TRANS_NOT_SPT 3
#define BTA_HH_HS_ERROR 5
#define BTA_HH_ERR 6
#define BTA_HH_ERR_SDP 7
#define BTA_HH_ERR_PROTO 8
#define BTA_HH_ERR_DB_FULL 9
#define BTA_HH_ERR_TOD_UNSPT 10
#define BTA_HH_ERR_AUTH_FAILED 12
#define BTA_HH_ERR_HDL 13

/* The control-block states and the state-table actions. */
#define BTA_HH_NULL_ST 0
#define BTA_HH_IDLE_ST 1
#define BTA_HH_W4_CONN_ST 2
#define BTA_HH_CONN_ST 3

#define BTA_HH_API_DISC_ACT 0
#define BTA_HH_OPEN_ACT 1
#define BTA_HH_CLOSE_ACT 2
#define BTA_HH_DATA_ACT 3
#define BTA_HH_CTRL_DAT_ACT 4
#define BTA_HH_HANDSK_ACT 5
#define BTA_HH_START_SDP 6
#define BTA_HH_SDP_CMPL 7
#define BTA_HH_WRITE_DEV_ACT 8
#define BTA_HH_GET_DSCP_ACT 9
#define BTA_HH_MAINT_DEV_ACT 10
#define BTA_HH_OPEN_CMPL_ACT 11
#define BTA_HH_NUM_ACTIONS 12
#define BTA_HH_IGNORE BTA_HH_NUM_ACTIONS

/* The columns of a state-table row and the number of state-machine events (the low byte of the event). */
#define BTA_HH_ACTION 0
#define BTA_HH_NEXT_STATE 1
#define BTA_HH_NUM_COLS 2
#define BTA_HH_NUM_EVENTS 12

/* The device table size, the unused handle and the protocol modes. */
#define BTA_HH_MAX_KNOWN 16
#define BTA_HH_MAX_DEVICE 16
#define BTA_HH_INVALID_HANDLE 0xFF
#define BTA_HH_PROTO_RPT_MODE 0
#define BTA_HH_MAX_ACL_Q_DEV 4

/* The HID layer's results, transaction types and control parameters the actions pass through. */
#define HID_SUCCESS 0
#define HID_ERR_L2CAP_FAILED 14
#define HID_ERR_AUTH_FAILED 15
#define HID_ERR_SDP_BUSY 16
#define HID_L2CAP_CONN_FAIL 0x0100
#define HID_L2CAP_REQ_FAIL 0x0200
#define HID_L2CAP_CFG_FAIL 0x0400
#define HID_SEC_REQUIRED 0x8000
#define HID_SDP_TOD_UNSPT 0xFF
#define HID_TRANS_CONTROL 1
#define HID_TRANS_GET_REPORT 4
#define HID_TRANS_SET_REPORT 5
#define HID_TRANS_GET_PROTOCOL 6
#define HID_TRANS_SET_PROTOCOL 7
#define HID_TRANS_GET_IDLE 8
#define HID_TRANS_SET_IDLE 9
#define HID_TRANS_DATA 10
#define HID_TRANS_DATAC 11
#define HID_PAR_REP_TYPE_OUTPUT 2
#define BTA_HH_CTRL_SUSPEND 3
#define BTA_HH_CTRL_EXIT_SUSPEND 4
#define BTA_HH_CTRL_VIRTUAL_CABLE_UNPLUG 5

/* The HID layer's events to bta_hh_cback. */
#define HID_HDEV_EVT_OPEN 0
#define HID_HDEV_EVT_CLOSE 1
#define HID_HDEV_EVT_RETRYING 2
#define HID_HDEV_EVT_INTR_DATA 3
#define HID_HDEV_EVT_INTR_DATC 4
#define HID_HDEV_EVT_CTRL_DATA 5
#define HID_HDEV_EVT_CTRL_DATC 6
#define HID_HDEV_EVT_HANDSHAKE 7
#define HID_HDEV_EVT_VC_UNPLUG 8

/* The handshake result codes a HID device answers a transaction with. */
#define HID_PAR_HANDSHAKE_RSP_SUCCESS 0
#define HID_PAR_HANDSHAKE_RSP_NOT_READY 1
#define HID_PAR_HANDSHAKE_RSP_ERR_INVALID_REP_ID 2
#define HID_PAR_HANDSHAKE_RSP_ERR_UNSUPPORTED_REQ 3
#define HID_PAR_HANDSHAKE_RSP_ERR_INVALID_PARAM 4
#define HID_PAR_HANDSHAKE_RSP_ERR_UNKNOWN 14
#define HID_PAR_HANDSHAKE_RSP_ERR_FATAL 15

/* The application layer's trace statements: each prints only while appl_trace_level reaches its own. */
#ifndef BT_TRACE_LEVEL_DEBUG
#define BT_TRACE_LEVEL_DEBUG 5
#endif
#define TRACE_APPL_ERROR 0x00000500
#define TRACE_APPL_EVENT 0x00000503
#define TRACE_APPL_DEBUG 0x00000504
#define APPL_TRACE_ERROR0(m) {if (appl_trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg_0(TRACE_APPL_ERROR, (m));}
#define APPL_TRACE_ERROR1(m, p1) {if (appl_trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_1(TRACE_APPL_ERROR, (m), (u32)(p1));}
#define APPL_TRACE_EVENT1(m, p1) {if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_APPL_EVENT, (m), (u32)(p1));}
#define APPL_TRACE_EVENT3(m, p1, p2, p3) {if (appl_trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_3(TRACE_APPL_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3));}
#define APPL_TRACE_DEBUG0(m) {if (appl_trace_level >= BT_TRACE_LEVEL_DEBUG) LogMsg_0(TRACE_APPL_DEBUG, (m));}
#define APPL_TRACE_DEBUG1(m, p1) {if (appl_trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_1(TRACE_APPL_DEBUG, (m), (u32)(p1));}
#define APPL_TRACE_DEBUG2(m, p1, p2) {if (appl_trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_2(TRACE_APPL_DEBUG, (m), (u32)(p1), (u32)(p2));}
#define APPL_TRACE_DEBUG3(m, p1, p2, p3) {if (appl_trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_3(TRACE_APPL_DEBUG, (m), (u32)(p1), (u32)(p2), (u32)(p3));}
#define APPL_TRACE_DEBUG4(m, p1, p2, p3, p4) {if (appl_trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_4(TRACE_APPL_DEBUG, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4));}

/* size: 0x08 - a HID report descriptor: its length and a GKI buffer holding it (BTE/gki_time.h's type). */
typedef BtaHhDscpInfo tBTA_HH_DEV_DSCP_INFO;

union tBTA_HH;

/* The application callback: the event (BTA_HH_*_EVT, a 16-bit value) and its payload. */
typedef void (*tBTA_HH_CBACK)(u16 event, union tBTA_HH* p_data);

/* size: 0x34 - BTA_HH_API_ENABLE_EVT: the security mask and the application callback. */
typedef struct tBTA_HH_API_ENABLE {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 sec_mask;
    /* +0x09 */ u8 unused_0x09[0x27];
    /* +0x30 */ tBTA_HH_CBACK p_cback;
} tBTA_HH_API_ENABLE; /* size: 0x34 */

/* size: 0x10 - BTA_HH_API_OPEN_EVT: the device to connect to, the security mask and the protocol mode. */
typedef struct tBTA_HH_API_CONN {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0E */ u8 sec_mask;
    /* +0x0F */ u8 mode;
} tBTA_HH_API_CONN; /* size: 0x10 */

/* size: 0x14 - BTA_HH_API_WRITE_DEV_EVT: one HID transaction to the device `hdr.layer_specific`. */
typedef struct tBTA_HH_CMD_DATA {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 t_type;
    /* +0x09 */ u8 param;
    /* +0x0A */ u8 rpt_id;
    /* +0x0B */ u8 pad_0x0B;
    /* +0x0C */ u16 data;
    /* +0x0E */ u16 pad_0x0E;
    /* +0x10 */ BT_HDR* p_data;
} tBTA_HH_CMD_DATA; /* size: 0x14 */

/* size: 0x10 - an event bta_hh_cback forwards from the HID layer: its data word and its report buffer. */
typedef struct tBTA_HH_CBACK_DATA {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u32 data;
    /* +0x0C */ BT_HDR* p_data;
} tBTA_HH_CBACK_DATA; /* size: 0x10 */

/* size: 0x1C - BTA_HH_API_MAINT_DEV_EVT: add (with its attributes and descriptor) or remove a device. */
typedef struct tBTA_HH_MAINT_DEV {
    /* +0x00 */ BT_HDR hdr;
    /* +0x08 */ u8 bda[BD_ADDR_LEN];
    /* +0x0E */ u16 attr_mask;
    /* +0x10 */ u16 sub_event;
    /* +0x12 */ u8 sub_class;
    /* +0x13 */ u8 app_id;
    /* +0x14 */ tBTA_HH_DEV_DSCP_INFO dscp_info;
} tBTA_HH_MAINT_DEV; /* size: 0x1C */

/* size: 0x34 - every message the HID host state machine receives. */
typedef union tBTA_HH_DATA {
    /* +0x00 */ BT_HDR hdr;
    /* +0x00 */ tBTA_HH_API_ENABLE api_enable;
    /* +0x00 */ tBTA_HH_API_CONN api_conn;
    /* +0x00 */ tBTA_HH_CMD_DATA api_sndcmd;
    /* +0x00 */ tBTA_HH_CBACK_DATA hid_cback;
    /* +0x00 */ u8 status;
    /* +0x00 */ tBTA_HH_MAINT_DEV api_maintdev;
} tBTA_HH_DATA; /* size: 0x34 */

/* size: 0x08 - BTA_HH_OPEN_EVT (and the device events): the device, the result and its handle. */
typedef struct tBTA_HH_CONN {
    /* +0x00 */ u8 bda[BD_ADDR_LEN];
    /* +0x06 */ u8 status;
    /* +0x07 */ u8 handle;
} tBTA_HH_CONN; /* size: 0x08 */

/* size: 0x08 - BTA_HH_ADD_DEV_EVT / BTA_HH_RMV_DEV_EVT: the device, the result and its handle. */
typedef tBTA_HH_CONN tBTA_HH_DEV_INFO;

/* size: 0x02 - the result of a SET_ transaction or a close: the status and the device handle. */
typedef struct tBTA_HH_CBDATA {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 handle;
} tBTA_HH_CBDATA; /* size: 0x02 */

/* size: 0x0A - the result of a GET_ transaction: the status, the handle and the value read back. */
typedef struct tBTA_HH_HSDATA {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 handle;
    /* +0x02 */ union {
        u8 proto_mode;
        u8 idle_rate;
        BT_HDR rpt_data;
    } rsp_data;
} tBTA_HH_HSDATA; /* size: 0x0A */

/* size: 0x06 - the ACL queue of one connected device: its handle, the queued and the unacknowledged packets. */
typedef struct tBTA_HH_ACL_Q_DEV {
    /* +0x00 */ u8 handle;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 num_queued;
    /* +0x04 */ u16 num_not_acked;
} tBTA_HH_ACL_Q_DEV; /* size: 0x06 */

/* size: 0x1E - BTA_HH_GET_ACL_Q_EVT: the controller's ACL window and buffers, and the connected devices' queues. */
typedef struct tBTA_HH_ACL_Q_INFO {
    /* +0x00 */ u16 xmit_window;
    /* +0x02 */ u16 num_acl_bufs;
    /* +0x04 */ u16 num_dev;
    /* +0x06 */ tBTA_HH_ACL_Q_DEV dev[BTA_HH_MAX_ACL_Q_DEV];
} tBTA_HH_ACL_Q_INFO; /* size: 0x1E */

/* size: 0x20 - every payload the application callback receives. */
typedef union tBTA_HH {
    /* +0x00 */ tBTA_HH_CONN conn;
    /* +0x00 */ tBTA_HH_DEV_INFO dev_info;
    /* +0x00 */ tBTA_HH_CBDATA dev_status;
    /* +0x00 */ tBTA_HH_HSDATA hs_data;
    /* +0x00 */ tBTA_HH_DEV_DSCP_INFO dscp_info;
    /* +0x00 */ tBTA_HH_ACL_Q_INFO acl_q_info;
} tBTA_HH; /* size: 0x20 */

/* size: 0x20 - one known HID device. */
typedef struct tBTA_HH_DEV_CB {
    /* +0x00 */ tBTA_HH_DEV_DSCP_INFO dscp_info;
    /* +0x08 */ u8 addr[BD_ADDR_LEN];
    /* +0x0E */ u16 attr_mask;
    /* +0x10 */ u16 w4_evt;
    /* +0x12 */ u8 index;
    /* +0x13 */ u8 sub_class;
    /* +0x14 */ u8 sec_mask;
    /* +0x15 */ u8 app_id;
    /* +0x16 */ u8 hid_handle;
    /* +0x17 */ u8 vp;
    /* +0x18 */ u8 in_use;
    /* +0x19 */ u8 incoming_conn;
    /* +0x1A */ u8 opened;
    /* +0x1B */ u8 mode;
    /* +0x1C */ u8 state;
    /* +0x1D */ u8 pad_0x1D[3];
} tBTA_HH_DEV_CB; /* size: 0x20 */

/* size: 0x0E - the keyboard report state (GUESS layout: only the dead-stripped report parsers use it). */
typedef struct tBTA_HH_KB_CB {
    /* +0x00 */ u8 mod_key[4];
    /* +0x04 */ u8 num_lock;
    /* +0x05 */ u8 caps_lock;
    /* +0x06 */ u8 last_report[8];
} tBTA_HH_KB_CB; /* size: 0x0E */

/* size: 0x230 - the HID host control block (.bss 0x80739468). */
typedef struct tBTA_HH_CB {
    /* +0x000 */ tBTA_HH_KB_CB kb_cb;
    /* +0x00E */ u8 pad_0x00E[2];
    /* +0x010 */ tBTA_HH_DEV_CB kdev[BTA_HH_MAX_DEVICE];
    /* +0x210 */ tBTA_HH_DEV_CB* p_cur;
    /* +0x214 */ u8 cb_index[BTA_HH_MAX_KNOWN];
    /* +0x224 */ tBTA_HH_CBACK p_cback;
    /* +0x228 */ tSDP_DISCOVERY_DB* p_disc_db;
    /* +0x22C */ u8 trace_level;
    /* +0x22D */ u8 cnt_num;
    /* +0x22E */ u8 w4_disable;
    /* +0x22F */ u8 pad_0x22F;
} tBTA_HH_CB; /* size: 0x230 */

/* An action of the state machine. */
typedef void (*tBTA_HH_ACTION)(tBTA_HH_DEV_CB* p_cb, tBTA_HH_DATA* p_data);

#endif /* MHTRI_BTE_BTA_HH_INT_H */
