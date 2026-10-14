/* BTE/port_int.h - the RFCOMM port layer's types and constants: the port control block, its data queues and line
 *   parameters, the multiplexer control block and the pool of both the nine BTE port and RFCOMM units share; each unit's
 *   own entry points are declared in its own header (BTE/port_rfc.h, BTE/rfc_utils.h, ...). */
#ifndef MHTRI_BTE_PORT_INT_H
#define MHTRI_BTE_PORT_INT_H

#include "types.h"
#include "BTE/gki_time.h"

/* The pool sizes the retail bodies loop over (5 ports of 0xA4 B, 1 multiplexer of 0x78 B). */
#define MAX_RFC_PORTS 5
#define MAX_BD_CONNECTIONS 1
#define RFCOMM_MAX_DLCI 61
#define BD_ADDR_LEN 6

/* The port (application) states. */
#define PORT_STATE_CLOSED 0
#define PORT_STATE_OPENING 1
#define PORT_STATE_OPENED 2
#define PORT_STATE_CLOSING 3

/* The RFCOMM DLC states of a port (tRFC_PORT.state). */
#define RFC_STATE_CLOSED 0
#define RFC_STATE_SABME_WAIT_UA 1
#define RFC_STATE_ORIG_WAIT_SEC_CHECK 2
#define RFC_STATE_TERM_WAIT_SEC_CHECK 3
#define RFC_STATE_OPENED 4
#define RFC_STATE_DISC_WAIT_UA 5

/* The responses a port waits for (tRFC_PORT.expected_rsp). */
#define RFC_RSP_PN 0x01
#define RFC_RSP_RPN_REPLY 0x02
#define RFC_RSP_RPN 0x04
#define RFC_RSP_MSC 0x08
#define RFC_RSP_RLS 0x10

/* The control-exchange progress bits of a port (tPORT.port_ctrl). */
#define PORT_CTRL_REQ_SENT 0x01
#define PORT_CTRL_REQ_CONFIRMED 0x02
#define PORT_CTRL_IND_RECEIVED 0x04
#define PORT_CTRL_IND_RESPONDED 0x08

/* The flow control modes of a multiplexer (tRFC_MCB.flow). */
#define PORT_FC_UNDEFINED 0
#define PORT_FC_TS710 1
#define PORT_FC_CREDIT 2

/* The modem signals of a control exchange (tPORT_CTRL.modem_signal). */
#define PORT_DTRDSR_ON 0x01
#define PORT_CTSRTS_ON 0x02
#define PORT_RING_ON 0x04
#define PORT_DCD_ON 0x08

/* The line status bits (tPORT.line_status). */
#define LINE_STATUS_OVERRUN 0x02
#define LINE_STATUS_RXFLAG 0x01
#define LINE_STATUS_FAILED 0x10

/* The events a port reports to its callback (tPORT.ev_mask). */
#define PORT_EV_RXCHAR 0x00000001
#define PORT_EV_RXFLAG 0x00000002
#define PORT_EV_TXEMPTY 0x00000004
#define PORT_EV_CTS 0x00000008
#define PORT_EV_DSR 0x00000010
#define PORT_EV_RLSD 0x00000020
#define PORT_EV_BREAK 0x00000040
#define PORT_EV_ERR 0x00000080
#define PORT_EV_RING 0x00000100
#define PORT_EV_CTSS 0x00000400
#define PORT_EV_DSRS 0x00000800
#define PORT_EV_RLSDS 0x00001000
#define PORT_EV_OVERRUN 0x00002000
#define PORT_EV_TXCHAR 0x00004000
#define PORT_EV_CONNECTED 0x00000200
#define PORT_EV_CONNECT_ERR 0x00008000
#define PORT_EV_FC 0x00010000
#define PORT_EV_FCS 0x00020000

/* The results a port reports to its management callback. */
#define PORT_SUCCESS 0
#define PORT_START_FAILED 12
#define PORT_PAR_NEG_FAILED 13
#define PORT_PORT_NEG_FAILED 14
#define PORT_PEER_CONNECTION_FAILED 16
#define PORT_PEER_TIMEOUT 18
#define PORT_CLOSED 19
#define PORT_PAGE_TIMEOUT 24

/* The data queue watermarks. */
#define PORT_RX_HIGH_WM 8000
#define PORT_RX_LOW_WM 5000
#define PORT_RX_CRITICAL_WM 12000
#define PORT_RX_BUF_LOW_WM 8
#define PORT_RX_BUF_HIGH_WM 16
#define PORT_TX_HIGH_WM 8000
#define PORT_TX_BUF_HIGH_WM 16
#define PORT_CREDIT_RX_MAX 16
#define PORT_CREDIT_RX_LOW 8

/* The service UUIDs a port opens its control exchange with a port negotiation for. */
#define UUID_SERVCLASS_SERIAL_PORT 0x1101
#define UUID_SERVCLASS_DIALUP_NETWORKING 0x1103
#define UUID_SERVCLASS_FAX 0x1111

/* The events a port reports to its callback, and the result codes of its management callback. */
typedef void (tPORT_CALLBACK)(u32 code, u16 port_handle);
/* The callback that receives a port's data in place of its receive queue. */
typedef s32 (tPORT_DATA_CALLBACK)(u16 port_handle, u8* p_data, u16 len);

/* size: 0x09 - the line parameters of a port negotiation. */
typedef struct tPORT_STATE {
    /* +0x00 */ u8 baud_rate;
    /* +0x01 */ u8 byte_size;
    /* +0x02 */ u8 stop_bits;
    /* +0x03 */ u8 parity;
    /* +0x04 */ u8 parity_type;
    /* +0x05 */ u8 fc_type;
    /* +0x06 */ u8 rx_char1;
    /* +0x07 */ u8 xon_char;
    /* +0x08 */ u8 xoff_char;
} tPORT_STATE; /* size: 0x09 */

/* size: 0x05 - the modem signals and break state of a control exchange. */
typedef struct tPORT_CTRL {
    /* +0x00 */ u8 modem_signal;
    /* +0x01 */ u8 break_signal;
    /* +0x02 */ u8 discard_buffers;
    /* +0x03 */ u8 break_signal_seq;
    /* +0x04 */ u8 fc;
} tPORT_CTRL; /* size: 0x05 */

/* size: 0x18 - one direction of a port's data: its queue, flow control flags and byte count. */
typedef struct tPORT_DATA {
    /* +0x00 */ BUFFER_Q queue;
    /* +0x0C */ u8 peer_fc;
    /* +0x0D */ u8 user_fc;
    /* +0x10 */ u32 queue_size;
    /* +0x14 */ u32 unused_0x14;
} tPORT_DATA; /* size: 0x18 */

/* size: 0x78 - a multiplexer (one L2CAP channel to one device). */
typedef struct tRFC_MCB {
    /* +0x00 */ TIMER_LIST_ENT tle;
    /* +0x18 */ BUFFER_Q cmd_q;
    /* +0x24 */ u8 port_inx[RFCOMM_MAX_DLCI + 1];
    /* +0x62 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x68 */ u16 lcid;
    /* +0x6A */ u16 peer_l2cap_mtu;
    /* +0x6C */ u8 state;
    /* +0x6D */ u8 is_initiator;
    /* +0x6E */ u8 local_cfg_sent;
    /* +0x6F */ u8 peer_cfg_rcvd;
    /* +0x70 */ u8 restart_required;
    /* +0x71 */ u8 peer_ready;
    /* +0x72 */ u8 flow;
    /* +0x73 */ u8 l2cap_congested;
    /* +0x74 */ u8 is_disc_initiator;
    /* +0x75 */ u8 pad_0x75[3];
} tRFC_MCB; /* size: 0x78 */

/* size: 0x20 - the RFCOMM side of a port: the DLC state, the responses it waits for, its multiplexer and timer. */
typedef struct tRFC_PORT {
    /* +0x00 */ u8 state;
    /* +0x01 */ u8 expected_rsp;
    /* +0x04 */ tRFC_MCB* p_mcb;
    /* +0x08 */ TIMER_LIST_ENT tle;
} tRFC_PORT; /* size: 0x20 */

/* size: 0xA4 - a port (one DLC of a multiplexer). */
typedef struct tPORT {
    /* +0x00 */ u8 inx;
    /* +0x01 */ u8 in_use;
    /* +0x02 */ u8 state;
    /* +0x03 */ u8 scn;
    /* +0x04 */ u16 uuid;
    /* +0x06 */ u8 bd_addr[BD_ADDR_LEN];
    /* +0x0C */ u8 is_server;
    /* +0x0D */ u8 dlci;
    /* +0x0E */ u8 error;
    /* +0x0F */ u8 line_status;
    /* +0x10 */ u8 default_signal_state;
    /* +0x12 */ u16 mtu;
    /* +0x14 */ u16 peer_mtu;
    /* +0x18 */ tPORT_DATA tx;
    /* +0x30 */ tPORT_DATA rx;
    /* +0x48 */ tPORT_STATE user_port_pars;
    /* +0x51 */ tPORT_STATE peer_port_pars;
    /* +0x5A */ tPORT_CTRL local_ctrl;
    /* +0x5F */ tPORT_CTRL peer_ctrl;
    /* +0x64 */ u8 port_ctrl;
    /* +0x65 */ u8 rx_flag_ev_pending;
    /* +0x68 */ tRFC_PORT rfc;
    /* +0x88 */ u32 ev_mask;
    /* +0x8C */ tPORT_CALLBACK* p_callback;
    /* +0x90 */ tPORT_CALLBACK* p_mgmt_callback;
    /* +0x94 */ tPORT_DATA_CALLBACK* p_data_callback;
    /* +0x98 */ u16 credit_tx;
    /* +0x9A */ u16 credit_rx;
    /* +0x9C */ u16 credit_rx_max;
    /* +0x9E */ u16 credit_rx_low;
    /* +0xA0 */ u16 rx_buf_critical;
    /* +0xA2 */ u8 unused_0xA2[2];
} tPORT; /* size: 0xA4 */

/* size: 0x3AC - the port and multiplexer pools. */
typedef struct tPORT_CB {
    /* +0x000 */ tPORT port[MAX_RFC_PORTS];
    /* +0x334 */ tRFC_MCB rfc_mcb[MAX_BD_CONNECTIONS];
} tPORT_CB; /* size: 0x3AC */

#endif /* MHTRI_BTE_PORT_INT_H */
