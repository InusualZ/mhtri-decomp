/* BTE/gap_int.h - the GAP layer's types, constants and trace statements: the GAP control block the three BTE gap_*.c
 *   units share; each unit's own entry points are declared in its own header (BTE/gap_api.h, BTE/gap_conn.h,
 *   BTE/gap_utils.h). */
#ifndef MHTRI_BTE_GAP_INT_H
#define MHTRI_BTE_GAP_INT_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_api.h"
#include "BTE/btm_inq.h"

/* The trace layer of GAP (bits 16-23 of a trace mask) and its trace types. */
#define TRACE_GAP_ERROR 0x000E0000
#define TRACE_GAP_WARNING 0x000E0001
#define TRACE_GAP_API 0x000E0002
#define TRACE_GAP_EVENT 0x000E0003
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define BT_TRACE_LEVEL_DEBUG 5

/* The GAP control block sizes: the request blocks and the connection blocks. */
#define GAP_MAX_BLOCKS 2
#define GAP_MAX_CONNECTIONS 8

/* The GAP request events (tGAP_INFO.event, the first argument of a request's callback). */
#define GAP_EVT_INQUIRY_RESULTS 0x0001
#define GAP_EVT_INQUIRY_COMPLETE 0x0002
#define GAP_EVT_DISCOVERY_COMPLETE 0x0003
#define GAP_EVT_REM_NAME_COMPLETE 0x0004
#define GAP_EVT_FIND_ADDR_COMPLETE 0x0005

/* The GAP results: success and the error group a BTM status converts to. */
#define BT_PASS 0
#define GAP_ERR_BUSY 0x0103
#define GAP_NO_DATA_AVAIL 0x0102
#define GAP_ERR_ILL_PARM 0x0109
#define GAP_CMD_INITIATED 0x010B
#define GAP_DEVICE_NOT_UP 0x010C
#define GAP_ERR_UNKNOWN_BD_ADDR 0x010D
#define GAP_EOINQDB 0x0111
#define GAP_ERR_PROCESSING 0x0114
#define GAP_ERR_TIMEOUT 0x0115

/* The GAP connection events a connection's callback receives. */
#define GAP_EVT_CONN_OPENED 0x0100
#define GAP_EVT_CONN_CLOSED 0x0101
#define GAP_EVT_CONN_DATA_AVAIL 0x0102
#define GAP_EVT_CONN_CONGESTED 0x0103
#define GAP_EVT_CONN_UNCONGESTED 0x0104

/* The connection block states (tGAP_CCB.con_state) and flags (tGAP_CCB.con_flags). */
#define GAP_CCB_STATE_IDLE 0
#define GAP_CCB_STATE_LISTENING 1
#define GAP_CCB_STATE_CONN_SETUP 2
#define GAP_CCB_STATE_CFG_SETUP 3
#define GAP_CCB_STATE_CONNECTED 4
#define GAP_CCB_FLAGS_IS_ORIG 0x01
#define GAP_CCB_FLAGS_HIS_CFG_DONE 0x02
#define GAP_CCB_FLAGS_MY_CFG_DONE 0x04

/* The BTM statuses the conversion to a GAP result distinguishes. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3
#define BTM_MODE_UNSUPPORTED 4
#define BTM_ILLEGAL_VALUE 5
#define BTM_WRONG_MODE 6
#define BTM_UNKNOWN_ADDR 7
#define BTM_DEVICE_TIMEOUT 8

/* The L2CAP connect and configure results, and the largest remote MTU a GAP connection keeps. */
#define L2CAP_CONN_OK 0
#define L2CAP_CFG_OK 0
#define GAP_MAX_REM_MTU 1691

/* The callback of a GAP request: the event and its payload, whose type depends on the event. */
/* untyped: a caller-owned payload - one type per event */
typedef void (tGAP_CALLBACK)(u16 event, void* p_data);
/* The callback of a GAP connection: the connection's handle and the event. */
typedef void (tGAP_CONN_CALLBACK)(u16 gap_handle, u16 event);

/* size: 0x02 - the outcome of an inquiry, as a BTM inquiry reports it. */
typedef struct tBTM_INQUIRY_CMPL {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 num_resp;
} tBTM_INQUIRY_CMPL; /* size: 0x02 */

/* size: 0x04 - the outcome of a remote name request, as BTM reports it; the name follows (approximation: the head
 * only). */
typedef struct tBTM_REMOTE_DEV_NAME {
    /* +0x00 */ u16 status;
    /* +0x02 */ u16 length;
    /* +0x04 */ u8 remote_bd_name[1];
} tBTM_REMOTE_DEV_NAME; /* size: 0x06 - approximation: the name runs on */

/* size: 0x04 - the outcome of an inquiry, as a GAP request reports it. */
typedef struct tGAP_INQ_CMPL {
    /* +0x00 */ u16 status;
    /* +0x02 */ u8 num_results;
    /* +0x03 */ u8 pad_0x03;
} tGAP_INQ_CMPL; /* size: 0x04 */

/* size: 0x10 - one GAP request block: the caller's callbacks and the event its BTM completion reports. */
typedef struct tGAP_INFO {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ tGAP_CALLBACK* gap_cback;
    /* +0x08 */ tGAP_CALLBACK* gap_inq_rslt_cback;
    /* +0x0C */ u16 event;
    /* +0x0E */ u8 unused_0x0E;
    /* +0x0F */ u8 in_use;
} tGAP_INFO; /* size: 0x10 */

/* size: 0x4A - the outcome of a find-address-by-name request: the status, the address found and the name looked for. */
typedef struct tGAP_FINDADDR_RESULTS {
    /* +0x00 */ u16 status;
    /* +0x02 */ u8 bd_addr[6];
    /* +0x08 */ char devname[0x42];
} tGAP_FINDADDR_RESULTS; /* size: 0x4A */

/* size: 0x54 - the find-address-by-name request: its callback, the inquiry database entry being named and the
 * outcome. */
typedef struct tGAP_FINDADDR_CB {
    /* +0x00 */ tGAP_CALLBACK* p_cback;
    /* +0x04 */ tBTM_INQ_INFO* p_cur_inq;
    /* +0x08 */ tGAP_FINDADDR_RESULTS results;
    /* +0x52 */ u8 in_use;
    /* +0x53 */ u8 pad_0x53;
} tGAP_FINDADDR_CB; /* size: 0x54 */

/* size: 0x60 - one GAP connection block: its L2CAP channel, the received buffers and the caller's callback. */
typedef struct tGAP_CCB {
    /* +0x00 */ u8 con_state;
    /* +0x01 */ u8 con_flags;
    /* +0x02 */ u8 unused_0x02[2];
    /* +0x04 */ u16 gap_handle;
    /* +0x06 */ u16 connection_id;
    /* +0x08 */ u8 rem_addr_specified;
    /* +0x09 */ u8 rem_dev_address[6];
    /* +0x0F */ u8 pad_0x0F;
    /* +0x10 */ u16 psm;
    /* +0x12 */ u16 rem_mtu_size;
    /* +0x14 */ BUFFER_Q rx_queue;
    /* +0x20 */ tGAP_CONN_CALLBACK* p_callback;
    /* +0x24 */ tL2CAP_CFG_INFO cfg;
} tGAP_CCB; /* size: 0x60 */

/* size: 0x328 - the GAP connection layer: the L2CAP callback set and the connection blocks. */
typedef struct tGAP_CONN {
    /* +0x000 */ tL2CAP_APPL_INFO reg_info;
    /* +0x028 */ tGAP_CCB ccb_pool[GAP_MAX_CONNECTIONS];
} tGAP_CONN; /* size: 0x328 */

/* size: 0x3AC - the GAP control block (the .bss row is 0x3B0 B with its alignment). */
typedef struct tGAP_CB {
    /* +0x000 */ tGAP_INFO blk[GAP_MAX_BLOCKS];
    /* +0x020 */ BtmCompleteCallback btm_cback[GAP_MAX_BLOCKS];
    /* +0x028 */ u8 trace_level;
    /* +0x029 */ u8 pad_0x029[3];
    /* +0x02C */ tGAP_FINDADDR_CB findaddr_cb;
    /* +0x080 */ u32 unused_0x080;
    /* +0x084 */ tGAP_CONN conn;
} tGAP_CB; /* size: 0x3AC */

#define GAP_TRACE_WARNING0(m) {if (gap_cb.trace_level >= BT_TRACE_LEVEL_WARNING) LogMsg_0(TRACE_GAP_WARNING, (m));}
#define GAP_TRACE_EVENT0(m) {if (gap_cb.trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_GAP_EVENT, (m));}
#define GAP_TRACE_EVENT1(m, p1) {if (gap_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_GAP_EVENT, (m), (u32)(p1));}
#define GAP_TRACE_EVENT2(m, p1, p2) {if (gap_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_2(TRACE_GAP_EVENT, (m), (u32)(p1), (u32)(p2));}
#define GAP_TRACE_EVENT4(m, p1, p2, p3, p4) {if (gap_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_4(TRACE_GAP_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4));}
#define GAP_TRACE_EVENT6(m, p1, p2, p3, p4, p5, p6) {if (gap_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_6(TRACE_GAP_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4), (u32)(p5), (u32)(p6));}

#endif /* MHTRI_BTE_GAP_INT_H */
