/* BTE/l2c_int.h - the L2CAP layer's types, constants and trace statements the five BTE/l2c_*.c units share: the link,
 *   channel and registration control blocks and the L2CAP control block l2cb; each unit's own entry points are declared
 *   in its own header (BTE/l2c_csm.h, BTE/l2c_link.h, BTE/l2c_main.h, BTE/l2c_utils.h). */
#ifndef MHTRI_BTE_L2C_INT_H
#define MHTRI_BTE_L2C_INT_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_api.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_dev.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_inq.h"
#include "BTE/btm_main.h"
#include "BTE/btm_pm.h"
#include "BTE/btm_sco.h"
#include "BTE/btm_sec.h"
#include "BTE/hcicmds.h"

/* The pool sizes the retail loops and the control block's layout count. */
#define MAX_L2CAP_LINKS 4
#define MAX_L2CAP_CHANNELS 10
#define MAX_L2CAP_CLIENTS 8

/* The first dynamic CID and the fixed signalling and connectionless CIDs. */
#define L2CAP_BASE_APPL_CID 0x0040
#define L2CAP_SIGNALLING_CID 1
#define L2CAP_CONNECTIONLESS_CID 2

/* The signalling command codes. */
#define L2CAP_CMD_REJECT 0x01
#define L2CAP_CMD_CONN_REQ 0x02
#define L2CAP_CMD_CONN_RSP 0x03
#define L2CAP_CMD_CONFIG_REQ 0x04
#define L2CAP_CMD_CONFIG_RSP 0x05
#define L2CAP_CMD_DISC_REQ 0x06
#define L2CAP_CMD_DISC_RSP 0x07
#define L2CAP_CMD_ECHO_REQ 0x08
#define L2CAP_CMD_ECHO_RSP 0x09
#define L2CAP_CMD_INFO_REQ 0x0A
#define L2CAP_CMD_INFO_RSP 0x0B

/* The command-reject reasons and the parameter lengths of each command. */
#define L2CAP_CMD_REJ_NOT_UNDERSTOOD 0
#define L2CAP_CMD_REJ_MTU_EXCEEDED 1
#define L2CAP_CMD_REJ_INVALID_CID 2
#define L2CAP_CMD_OVERHEAD 4
#define L2CAP_PKT_OVERHEAD 4
#define L2CAP_CONN_REQ_LEN 4
#define L2CAP_CONN_RSP_LEN 8
#define L2CAP_CONFIG_REQ_LEN 4
#define L2CAP_CONFIG_RSP_LEN 6
#define L2CAP_DISC_REQ_LEN 4
#define L2CAP_DISC_RSP_LEN 4
#define L2CAP_ECHO_RSP_LEN 0
#define L2CAP_INFO_RSP_LEN 4
#define L2CAP_CMD_REJECT_LEN 2
#define HCI_DATA_PREAMBLE_SIZE 4
#define L2CAP_SEND_CMD_OFFSET 0

/* The configuration option types and lengths. */
#define L2CAP_CFG_TYPE_MTU 0x01
#define L2CAP_CFG_TYPE_FLUSH_TOUT 0x02
#define L2CAP_CFG_TYPE_QOS 0x03
#define L2CAP_CFG_TYPE_FCR 0x04
#define L2CAP_CFG_MTU_OPTION_LEN 2
#define L2CAP_CFG_FLUSH_OPTION_LEN 2
#define L2CAP_CFG_QOS_OPTION_LEN 22
#define L2CAP_CFG_FCR_OPTION_LEN 9
#define L2CAP_CFG_OPTION_OVERHEAD 2
#define L2CAP_CFG_OPTIONS_OVERHEAD 2

/* The connection and configuration results. */
#define L2CAP_CONN_OK 0
#define L2CAP_CONN_PENDING 1
#define L2CAP_CONN_NO_PSM 2
#define L2CAP_CONN_SECURITY_BLOCK 3
#define L2CAP_CONN_NO_RESOURCES 4
#define L2CAP_CONN_TIMEOUT 0xEEEE
#define L2CAP_CFG_OK 0
#define L2CAP_CFG_UNACCEPTABLE_PARAMS 1
#define L2CAP_CFG_FAILED_NO_REASON 2
#define L2CAP_CFG_UNKNOWN_OPTIONS 3
#define L2CAP_DEFAULT_MTU 672
#define L2CAP_MTU_SIZE 1691
#define L2CAP_MIN_MTU 48
#define L2CAP_DEFAULT_FLUSH_TO 0xFFFF
#define L2CAP_NO_AUTOMATIC_FLUSH 0xFFFF
#define L2CAP_PING_RESULT_OK 0
#define L2CAP_PING_RESULT_NO_LINK 1
#define L2CAP_PING_RESULT_NO_RESP 2
#define L2CAP_DW_SUCCESS 1
#define L2CAP_DW_CONGESTED 2
#define L2CAP_DW_FAILED 0

/* The ACL packet boundary flags and the HCI handle mask. */
#define L2CAP_PKT_START 2
#define L2CAP_PKT_CONTINUE 1
#define L2CAP_PKT_START_FLAG_MASK 0x2000
#define HCI_DATA_HANDLE_MASK 0x0FFF
#define HCI_DATA_EVENT_MASK 3
#define HCI_DATA_EVENT_OFFSET 12
#define BT_EVT_TO_LM_HCI_ACL 0x2100

/* The timers L2CAP runs: link, channel and held-packet. */
#define BTU_TTYPE_L2CAP_LINK 2
#define BTU_TTYPE_L2CAP_CHNL 3
#define BTU_TTYPE_L2CAP_HOLD 4
#define L2CAP_LINK_CONNECT_TOUT 60
#define L2CAP_LINK_CONNECT_TOUT_EXT 120
#define L2CAP_ECHO_RSP_TOUT 30
#define L2CAP_LINK_FLOW_CONTROL_TOUT 2
#define L2CAP_LINK_STARTUP_TOUT 60
#define L2CAP_LINK_ROLE_SWITCH_TOUT 10
#define L2CAP_LINK_DISCONNECT_TOUT 30
#define L2CAP_CHNL_CONNECT_TOUT 60
#define L2CAP_CHNL_CONNECT_TOUT_EXT 120
#define L2CAP_CHNL_CFG_TIMEOUT 30
#define L2CAP_CHNL_DISCONNECT_TOUT 30
#define L2CAP_LINK_INACTIVITY_TOUT 2

/* The pool and HCI values the link layer passes on. */
#define L2CAP_CMD_POOL_ID 2
#define L2CAP_MAX_ACL_PKTS 0x069F
#define HCI_ROLE_MASTER 0
#define HCI_ROLE_SLAVE 1
#define HCI_ERR_HOST_REJECT_RESOURCES 0x0D
#define HCI_ERR_PEER_USER 0x13
#define HCI_ERR_CONNECTION_EXISTS 0x0B
#define HCI_ERR_MAX_NUM_OF_CONNECTIONS 0x09
#define HCI_ERR_HOST_REJECT_DEVICE 0x0F
#define HCI_ERR_CONN_CAUSE_LOCAL_HOST 0x16
#define HCI_ERR_HOST_TIMEOUT 0x10
#define HCI_ERR_UNSPECIFIED 0x1F
#define HCI_ERR_PAGE_TIMEOUT 0x04
#define HCI_ERR_UNACCEPT_CONN_INTERVAL 0x14
#define HCI_ERR_REJ_NO_RESOURCES 0x14
#define HCI_ERR_REJ_SECURITY 0x15
#define HCI_PKT_TYPES_MASK_DM1 0x0008
#define HCI_PKT_TYPES_MASK_DH1 0x0010
#define BTM_CLOCK_OFFSET_VALID 0x8000
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_PM_STS_SNIFF 3
#define BTM_PM_MD_ACTIVE 0
#define BTM_PM_SET_ONLY_ID 0x80
#define L2CAP_PRIORITY_HIGH 1
#define L2CAP_SEC_MIN_CONGESTION 3
#define L2CAP_SEC_LINK_WINDOW 3

/* The trace levels and the L2CAP layer's trace masks (the layer in bits 16-23, the type in the low bits). */
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define BT_TRACE_LEVEL_DEBUG 5
#define TRACE_L2CAP_ERROR 0x00080000
#define TRACE_L2CAP_WARNING 0x00080001
#define TRACE_L2CAP_API 0x00080002
#define TRACE_L2CAP_EVENT 0x00080003
#define TRACE_L2CAP_DEBUG 0x00080004

/* The link states of an LCB. */
typedef enum tL2C_LINK_STATE {
    LST_DISCONNECTED,
    LST_CONNECT_HOLDING,
    LST_CONNECTING_WAIT_SWITCH,
    LST_CONNECTING,
    LST_CONNECTED,
    LST_DISCONNECTING
} tL2C_LINK_STATE;

/* The channel states of a CCB. */
typedef enum tL2C_CHNL_STATE {
    CST_CLOSED,
    CST_ORIG_W4_SEC_COMP,
    CST_TERM_W4_SEC_COMP,
    CST_W4_L2CAP_CONNECT_RSP,
    CST_W4_L2CA_CONNECT_RSP,
    CST_CONFIG,
    CST_OPEN,
    CST_W4_L2CAP_DISCONNECT_RSP,
    CST_W4_L2CA_DISCONNECT_RSP
} tL2C_CHNL_STATE;

/* The events of the channel state machine (l2c_csm_execute). */
#define L2CEVT_LP_CONNECT_CFM 0
#define L2CEVT_LP_CONNECT_CFM_NEG 1
#define L2CEVT_LP_CONNECT_IND 2
#define L2CEVT_LP_DISCONNECT_IND 3
#define L2CEVT_LP_QOS_CFM 4
#define L2CEVT_LP_QOS_CFM_NEG 5
#define L2CEVT_LP_QOS_VIOLATION_IND 6
#define L2CEVT_SEC_COMP 7
#define L2CEVT_SEC_COMP_NEG 8
#define L2CEVT_L2CAP_CONNECT_REQ 10
#define L2CEVT_L2CAP_CONNECT_RSP 11
#define L2CEVT_L2CAP_CONNECT_RSP_PND 12
#define L2CEVT_L2CAP_CONNECT_RSP_NEG 13
#define L2CEVT_L2CAP_CONFIG_REQ 14
#define L2CEVT_L2CAP_CONFIG_RSP 15
#define L2CEVT_L2CAP_CONFIG_RSP_NEG 16
#define L2CEVT_L2CAP_DISCONNECT_REQ 17
#define L2CEVT_L2CAP_DISCONNECT_RSP 18
#define L2CEVT_L2CAP_DATA 19
#define L2CEVT_L2CA_CONNECT_REQ 20
#define L2CEVT_L2CA_CONNECT_RSP 21
#define L2CEVT_L2CA_CONNECT_RSP_NEG 22
#define L2CEVT_L2CA_CONFIG_REQ 23
#define L2CEVT_L2CA_CONFIG_RSP 24
#define L2CEVT_L2CA_CONFIG_RSP_NEG 25
#define L2CEVT_L2CA_DISCONNECT_REQ 26
#define L2CEVT_L2CA_DISCONNECT_RSP 27
#define L2CEVT_L2CA_DATA_READ 28
#define L2CEVT_L2CA_DATA_WRITE 29
#define L2CEVT_TIMEOUT 30

/* The configuration-done bits of a CCB. */
#define IB_CFG_DONE 0x01
#define OB_CFG_DONE 0x02

/* size: 0x10 - the connection data the link and signalling code hand the channel state machine. */
typedef struct tL2C_CONN_INFO {
    /* +0x00 */ u8 bd_addr[6];
    /* +0x06 */ u8 status;
    /* +0x08 */ u16 psm;
    /* +0x0A */ u16 l2cap_result;
    /* +0x0C */ u16 l2cap_status;
    /* +0x0E */ u16 remote_cid;
} tL2C_CONN_INFO; /* size: 0x10 */

/* The callback of an echo request (L2CA_Ping). */
typedef void (tL2CA_ECHO_RSP_CB)(u16 result);

/* size: 0x2C - a registered PSM and its client's callbacks. */
typedef struct tL2C_RCB {
    /* +0x00 */ u8 in_use;
    /* +0x02 */ u16 psm;
    /* +0x04 */ tL2CAP_APPL_INFO api;
} tL2C_RCB; /* size: 0x2C */

struct tL2C_LCB;

/* size: 0x7C - one L2CAP channel. */
typedef struct tL2C_CCB {
    /* +0x00 */ u8 in_use;
    /* +0x04 */ tL2C_CHNL_STATE chnl_state;
    /* +0x08 */ struct tL2C_CCB* p_next_ccb;
    /* +0x0C */ struct tL2C_CCB* p_prev_ccb;
    /* +0x10 */ struct tL2C_LCB* p_lcb;
    /* +0x14 */ u16 local_cid;
    /* +0x16 */ u16 remote_cid;
    /* +0x18 */ TIMER_LIST_ENT timer_entry;
    /* +0x30 */ tL2C_RCB* p_rcb;
    /* +0x34 */ u8 config_done;
    /* +0x35 */ u8 local_id;
    /* +0x36 */ u8 remote_id;
    /* +0x37 */ u8 sec_pending;
    /* +0x38 */ u16 our_mtu;
    /* +0x3A */ u16 peer_mtu;
    /* +0x3C */ u16 our_flush_to;
    /* +0x40 */ FLOW_SPEC peer_qos;
    /* +0x58 */ FLOW_SPEC our_qos;
    /* +0x70 */ BUFFER_Q xmit_hold_q;
} tL2C_CCB; /* size: 0x7C */

/* size: 0x5C - one ACL link and the channels on it. */
typedef struct tL2C_LCB {
    /* +0x00 */ u8 in_use;
    /* +0x04 */ tL2C_LINK_STATE link_state;
    /* +0x08 */ tL2C_CCB* p_first_ccb;
    /* +0x0C */ tL2C_CCB* p_last_ccb;
    /* +0x10 */ TIMER_LIST_ENT timer_entry;
    /* +0x28 */ u16 handle;
    /* +0x2A */ u8 remote_bd_addr[6];
    /* +0x30 */ u8 link_role;
    /* +0x31 */ u8 id;
    /* +0x32 */ u16 link_flush_tout;
    /* +0x34 */ u16 link_xmit_quota;
    /* +0x36 */ u16 link_xmit_window;
    /* +0x38 */ u16 sent_not_acked;
    /* +0x3A */ u16 cong_start_thresh;
    /* +0x3C */ u16 cong_end_thresh;
    /* +0x3E */ u16 discard_thresh;
    /* +0x40 */ u8 partial_segment_being_sent;
    /* +0x41 */ u8 link_congested;
    /* +0x44 */ BUFFER_Q link_xmit_data_q;
    /* +0x50 */ BT_HDR* p_hcit_rcv_acl;
    /* +0x54 */ tL2CA_ECHO_RSP_CB* p_echo_rsp_cb;
    /* +0x58 */ u16 idle_timeout;
    /* +0x5A */ u8 acl_priority;
} tL2C_LCB; /* size: 0x5C */

/* size: 0x7E8 - the L2CAP control block (.bss 0x8073C760). */
typedef struct tL2C_CB {
    /* +0x000 */ u8 l2cb_trace_level;
    /* +0x001 */ u8 desire_role;
    /* +0x002 */ u16 num_lm_acl_bufs;
    /* +0x004 */ u16 controller_xmit_window;
    /* +0x008 */ tL2C_LCB lcb_pool[MAX_L2CAP_LINKS];
    /* +0x178 */ tL2C_CCB ccb_pool[MAX_L2CAP_CHANNELS];
    /* +0x650 */ tL2C_RCB rcb_pool[MAX_L2CAP_CLIENTS];
    /* +0x7B0 */ tL2C_CCB* p_free_ccb_first;
    /* +0x7B4 */ tL2C_CCB* p_free_ccb_last;
    /* +0x7B8 */ u16 num_links_active;
    /* +0x7BA */ u16 idle_timeout;
    /* +0x7BC */ tL2C_LCB* p_cur_hcit_lcb;
    /* +0x7C0 */ BUFFER_Q rcv_hold_q;
    /* +0x7CC */ TIMER_LIST_ENT rcv_hold_tle;
    /* +0x7E4 */ u8 unused_0x7E4[4];
} tL2C_CB; /* size: 0x7E8 */

/* The L2CAP layer's trace statements: each prints only while l2cb's trace level reaches its own. */
#define L2CAP_TRACE_ERROR0(m) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg_0(TRACE_L2CAP_ERROR, (m));}
#define L2CAP_TRACE_ERROR1(m, p1) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_1(TRACE_L2CAP_ERROR, (m), (u32)(p1));}
#define L2CAP_TRACE_ERROR2(m, p1, p2) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_2(TRACE_L2CAP_ERROR, (m), (u32)(p1), (u32)(p2));}
#define L2CAP_TRACE_WARNING0(m) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_0(TRACE_L2CAP_WARNING, (m));}
#define L2CAP_TRACE_WARNING1(m, p1) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_1(TRACE_L2CAP_WARNING, (m), (u32)(p1));}
#define L2CAP_TRACE_WARNING2(m, p1, p2) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_2(TRACE_L2CAP_WARNING, (m), (u32)(p1), (u32)(p2));}
#define L2CAP_TRACE_WARNING3(m, p1, p2, p3) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_3(TRACE_L2CAP_WARNING, (m), (u32)(p1), (u32)(p2), (u32)(p3));}
#define L2CAP_TRACE_WARNING5(m, p1, p2, p3, p4, p5) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_5(TRACE_L2CAP_WARNING, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4), (u32)(p5));}
#define L2CAP_TRACE_API0(m) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_API) LogMsg_0(TRACE_L2CAP_API, (m));}
#define L2CAP_TRACE_API1(m, p1) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_1(TRACE_L2CAP_API, (m), (u32)(p1));}
#define L2CAP_TRACE_API2(m, p1, p2) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_2(TRACE_L2CAP_API, (m), (u32)(p1), (u32)(p2));}
#define L2CAP_TRACE_API3(m, p1, p2, p3) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_3(TRACE_L2CAP_API, (m), (u32)(p1), (u32)(p2), (u32)(p3));}
#define L2CAP_TRACE_API6(m, p1, p2, p3, p4, p5, p6) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_API) \
    LogMsg_6(TRACE_L2CAP_API, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4), (u32)(p5), (u32)(p6));}
#define L2CAP_TRACE_EVENT1(m, p1) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_L2CAP_EVENT, (m), (u32)(p1));}
#define L2CAP_TRACE_DEBUG1(m, p1) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_1(TRACE_L2CAP_DEBUG, (m), (u32)(p1));}
#define L2CAP_TRACE_DEBUG6(m, p1, p2, p3, p4, p5, p6) {if (l2cb.l2cb_trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_6(TRACE_L2CAP_DEBUG, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4), (u32)(p5), (u32)(p6));}

/* The little-endian stream readers and writers of the protocol code; each advances `p`. */
#define UINT8_TO_STREAM(p, u8v) {*(p)++ = (u8)(u8v);}
#define UINT16_TO_STREAM(p, u16v) {*(p)++ = (u8)(u16v); *(p)++ = (u8)((u16v) >> 8);}
#define UINT32_TO_STREAM(p, u32v) {*(p)++ = (u8)(u32v); *(p)++ = (u8)((u32v) >> 8); \
    *(p)++ = (u8)((u32v) >> 16); *(p)++ = (u8)((u32v) >> 24);}
#define STREAM_TO_UINT8(u8v, p) {u8v = (u8)(*(p)); (p) += 1;}
#define STREAM_TO_UINT16(u16v, p) {u16v = ((u16)(*(p)) + (((u16)(*((p) + 1))) << 8)); (p) += 2;}
#define STREAM_TO_UINT32(u32v, p) {u32v = (((u32)(*(p))) + ((((u32)(*((p) + 1)))) << 8) + \
    ((((u32)(*((p) + 2)))) << 16) + ((((u32)(*((p) + 3)))) << 24)); (p) += 4;}

#endif /* MHTRI_BTE_L2C_INT_H */
