/* BTE/rfc_int.h - the RFCOMM layer's types, constants and trace statements: the received frame, the RFCOMM and
 *   port control block, the state machine events and the frame codes the nine BTE port and RFCOMM units share; each
 *   unit's own entry points are declared in its own header (BTE/rfc_mx_fsm.h, BTE/rfc_ts_frames.h, ...). */
#ifndef MHTRI_BTE_RFC_INT_H
#define MHTRI_BTE_RFC_INT_H

#include "types.h"
#include "BTE/port_int.h"
#include "BTE/l2c_api.h"

/* The multiplexer states (tRFC_MCB.state). */
#define RFC_MX_STATE_IDLE 0
#define RFC_MX_STATE_WAIT_CONN_CNF 1
#define RFC_MX_STATE_CONFIGURE 2
#define RFC_MX_STATE_SABME_WAIT_UA 3
#define RFC_MX_STATE_WAIT_SABME 4
#define RFC_MX_STATE_CONNECTED 5
#define RFC_MX_STATE_DISC_WAIT_UA 6

/* The frame events both state machines take, and the multiplexer's own. */
#define RFC_EVENT_SABME 0
#define RFC_EVENT_UA 1
#define RFC_EVENT_DM 2
#define RFC_EVENT_DISC 3
#define RFC_EVENT_UIH 4
#define RFC_EVENT_TIMEOUT 5
#define RFC_EVENT_BAD_FRAME 50

#define RFC_MX_EVENT_START_REQ 6
#define RFC_MX_EVENT_START_RSP 7
#define RFC_MX_EVENT_CLOSE_REQ 8
#define RFC_MX_EVENT_CONN_CNF 9
#define RFC_MX_EVENT_CONN_IND 10
#define RFC_MX_EVENT_CONF_CNF 11
#define RFC_MX_EVENT_CONF_IND 12
#define RFC_MX_EVENT_QOS_VIOLATION_IND 13
#define RFC_MX_EVENT_DISC_IND 14

/* The port state machine's own events. */
#define RFC_EVENT_OPEN 9
#define RFC_EVENT_ESTABLISH_RSP 11
#define RFC_EVENT_CLOSE 12
#define RFC_EVENT_CLEAR 13
#define RFC_EVENT_DATA 14
#define RFC_EVENT_SEC_COMPLETE 15

/* The BTU timer types and the timeouts (seconds) of the multiplexer and port timers. */
#define BTU_TTYPE_RFCOMM_MFC 11
#define BTU_TTYPE_RFCOMM_PORT 12
#define RFC_T1_TIMEOUT 20
#define RFC_PORT_T1_TIMEOUT 60
#define RFC_T2_TIMEOUT 60
#define RFC_DISC_TIMEOUT 3
#define RFC_CLOSE_TIMEOUT 10
#define RFCOMM_CONN_TIMEOUT 120
#define RFC_MCB_INIT_INACT_TIMER 60
#define RFC_MCB_RELEASE_INACT_TIMER 2

/* The results of an RFCOMM request. */
#define RFCOMM_SUCCESS 0
#define RFCOMM_ERROR 1

/* The frame control and multiplexer command codes, and the frame layout constants. */
#define RFCOMM_SABME 0x2F
#define RFCOMM_UA 0x63
#define RFCOMM_DM 0x0F
#define RFCOMM_DISC 0x43
#define RFCOMM_UIH 0xEF
#define RFCOMM_PF 0x10
#define RFCOMM_EA 1
#define RFCOMM_CR_MASK 2
#define RFCOMM_SHIFT_CR 1
#define RFCOMM_SHIFT_DLCI 2
#define RFCOMM_SHIFT_DLCI2 6
#define RFCOMM_SHIFT_LENGTH1 1
#define RFCOMM_SHIFT_LENGTH2 7
#define RFCOMM_SHIFT_MX_CTRL_TYPE 2
#define RFCOMM_PN_DLCI_MASK 0x3F
#define RFCOMM_PN_PRIORITY_MASK 0x3F
#define RFCOMM_PN_CONV_LAYER_MASK 0xF0
#define RFCOMM_PN_FRAM_TYPE_MASK 0x0F
#define RFCOMM_PN_K_MASK 0x07
#define RFCOMM_PN_CONV_LAYER_TYPE_1 0
#define RFCOMM_PN_CONV_LAYER_CBFC_I 0xF0
#define RFCOMM_PN_CONV_LAYER_CBFC_R 0xE0
#define RFCOMM_MIN_MTU 23
#define RFCOMM_MAX_MTU 32767
#define RFCOMM_K_MAX 7
#define RFCOMM_CMD_POOL_ID 2
#define RFCOMM_CTRL_FRAME_LEN 3
#define RFCOMM_MIN_OFFSET 5
#define RFCOMM_DATA_OVERHEAD (RFCOMM_MIN_OFFSET + 1)
#define L2CAP_MIN_OFFSET 9
#define L2CAP_BASE_APPL_CID 0x0040
#define MAX_L2CAP_CHANNELS 10
#define BT_PSM_RFCOMM 0x0003
#define RFCOMM_MX_DLCI 0
#define RFCOMM_MAX_DLCI_VALID 61

#define RFCOMM_MX_PN 0x80
#define RFCOMM_MX_PSC 0x40
#define RFCOMM_MX_CLD 0xC0
#define RFCOMM_MX_TEST 0x20
#define RFCOMM_MX_FCON 0xA0
#define RFCOMM_MX_FCOFF 0x60
#define RFCOMM_MX_MSC 0xE0
#define RFCOMM_MX_NSC 0x10
#define RFCOMM_MX_RPN 0x90
#define RFCOMM_MX_RLS 0x50
#define RFCOMM_MX_SNC 0xD0

#define RFCOMM_MX_PN_LEN 8
#define RFCOMM_MX_MSC_LEN_NO_BREAK 2
#define RFCOMM_MX_MSC_LEN_WITH_BREAK 3
#define RFCOMM_MX_NSC_LEN 1
#define RFCOMM_MX_RPN_REQ_LEN 1
#define RFCOMM_MX_RPN_LEN 8
#define RFCOMM_MX_RLS_LEN 2
#define RFCOMM_MX_FCON_LEN 0
#define RFCOMM_MX_FCOFF_LEN 0

#define RFCOMM_RPN_PM_MASK 0x3F7F

/* The trace layer of RFCOMM (bits 16-23 of a trace mask) and its trace types. */
#define TRACE_RFCOMM_ERROR 0x00090000
#define TRACE_RFCOMM_WARNING 0x00090001
#define TRACE_RFCOMM_API 0x00090002
#define TRACE_RFCOMM_EVENT 0x00090003
#define TRACE_RFCOMM_DEBUG 0x00090004
#define BT_TRACE_LEVEL_ERROR 1
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_EVENT 4
#define BT_TRACE_LEVEL_DEBUG 5

/* size: 0x14 - a received frame: its address and control fields and the decoded multiplexer command. */
typedef struct MX_FRAME {
    /* +0x00 */ u8 dlci;
    /* +0x01 */ u8 type;
    /* +0x02 */ u8 cr;
    /* +0x03 */ u8 ea;
    /* +0x04 */ u8 pf;
    /* +0x05 */ u8 credit;
    /* +0x08 */ union {
        struct { /* size: 0x0A */
            /* +0x08 */ u8 dlci;
            /* +0x09 */ u8 frame_type;
            /* +0x0A */ u8 conv_layer;
            /* +0x0B */ u8 priority;
            /* +0x0C */ u8 t1;
            /* +0x0E */ u16 mtu;
            /* +0x10 */ u8 n2;
            /* +0x11 */ u8 k;
        } pn;
        struct { /* size: 0x08 */
            /* +0x08 */ u8* p_data;
            /* +0x0C */ u16 data_len;
        } test;
        struct { /* size: 0x04 */
            /* +0x08 */ u8 dlci;
            /* +0x09 */ u8 signals;
            /* +0x0A */ u8 break_present;
            /* +0x0B */ u8 break_duration;
        } msc;
        struct { /* size: 0x03 */
            /* +0x08 */ u8 ea;
            /* +0x09 */ u8 cr;
            /* +0x0A */ u8 type;
        } nsc;
        struct { /* size: 0x0C */
            /* +0x08 */ u8 dlci;
            /* +0x09 */ u8 is_request;
            /* +0x0A */ u8 baud_rate;
            /* +0x0B */ u8 byte_size;
            /* +0x0C */ u8 stop_bits;
            /* +0x0D */ u8 parity;
            /* +0x0E */ u8 parity_type;
            /* +0x0F */ u8 fc_type;
            /* +0x10 */ u8 xon_char;
            /* +0x11 */ u8 xoff_char;
            /* +0x12 */ u16 param_mask;
        } rpn;
        struct { /* size: 0x02 */
            /* +0x08 */ u8 dlci;
            /* +0x09 */ u8 line_status;
        } rls;
    } u;
} MX_FRAME; /* size: 0x14 */

/* size: 0x68 - the RFCOMM half of the control block: the frame being parsed, the L2CAP registration and the
 * multiplexer of each L2CAP channel. */
typedef struct tRFCOMM_CB {
    /* +0x00 */ MX_FRAME rx_frame;
    /* +0x14 */ tL2CAP_APPL_INFO reg_info;
    /* +0x3C */ tRFC_MCB* p_rfc_lcid_mcb[MAX_L2CAP_CHANNELS];
    /* +0x64 */ u8 peer_rx_disabled;
    /* +0x65 */ u8 last_mux;
    /* +0x66 */ u8 last_port;
} tRFCOMM_CB; /* size: 0x68 */

/* size: 0x418 - the RFCOMM control block (.bss 0x8073CF48): the RFCOMM half, the port pools and the trace level. */
typedef struct tRFC_CB {
    /* +0x000 */ tRFCOMM_CB rfc;
    /* +0x068 */ tPORT_CB port;
    /* +0x414 */ u8 trace_level;
} tRFC_CB; /* size: 0x418 */

/* The RFCOMM trace statements: each prints only while the control block's trace level reaches its own. */
#define RFCOMM_TRACE_ERROR0(m) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_ERROR) LogMsg_0(TRACE_RFCOMM_ERROR, (m));}
#define RFCOMM_TRACE_ERROR1(m, p1) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_1(TRACE_RFCOMM_ERROR, (m), (u32)(p1));}
#define RFCOMM_TRACE_ERROR2(m, p1, p2) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_ERROR) \
    LogMsg_2(TRACE_RFCOMM_ERROR, (m), (u32)(p1), (u32)(p2));}
#define RFCOMM_TRACE_WARNING0(m) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_0(TRACE_RFCOMM_WARNING, (m));}
#define RFCOMM_TRACE_WARNING1(m, p1) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_1(TRACE_RFCOMM_WARNING, (m), (u32)(p1));}
#define RFCOMM_TRACE_WARNING2(m, p1, p2) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_WARNING) \
    LogMsg_2(TRACE_RFCOMM_WARNING, (m), (u32)(p1), (u32)(p2));}
#define RFCOMM_TRACE_EVENT0(m) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) LogMsg_0(TRACE_RFCOMM_EVENT, (m));}
#define RFCOMM_TRACE_EVENT1(m, p1) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_1(TRACE_RFCOMM_EVENT, (m), (u32)(p1));}
#define RFCOMM_TRACE_EVENT2(m, p1, p2) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_2(TRACE_RFCOMM_EVENT, (m), (u32)(p1), (u32)(p2));}
#define RFCOMM_TRACE_EVENT3(m, p1, p2, p3) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_3(TRACE_RFCOMM_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3));}
#define RFCOMM_TRACE_EVENT4(m, p1, p2, p3, p4) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_EVENT) \
    LogMsg_4(TRACE_RFCOMM_EVENT, (m), (u32)(p1), (u32)(p2), (u32)(p3), (u32)(p4));}
#define RFCOMM_TRACE_DEBUG0(m) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) LogMsg_0(TRACE_RFCOMM_DEBUG, (m));}
#define RFCOMM_TRACE_DEBUG1(m, p1) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_1(TRACE_RFCOMM_DEBUG, (m), (u32)(p1));}
#define RFCOMM_TRACE_DEBUG3(m, p1, p2, p3) {if (rfc_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) \
    LogMsg_3(TRACE_RFCOMM_DEBUG, (m), (u32)(p1), (u32)(p2), (u32)(p3));}

/* The address byte of a frame: the DLCI, the command/response bit and the EA bit. */
#define RFCOMM_FORMAT_ADDR(p, dlci, cr) {*(p)++ = (u8)(((dlci) << RFCOMM_SHIFT_DLCI) | ((cr) << RFCOMM_SHIFT_CR) | RFCOMM_EA);}

#endif /* MHTRI_BTE_RFC_INT_H */
