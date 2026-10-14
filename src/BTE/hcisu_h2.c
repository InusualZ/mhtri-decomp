/*
 * BTE/hcisu_h2.c - the H2 (USB) HCI services of the Broadcom BTE Bluetooth stack: reassembles incoming HCI messages,
 *   sends outgoing ones through the USB transport, and exports its entry points as the transport interface table.
 *
 * RANGE. .text 0x80479030-0x804796F0 (8 functions); .rodata 0x80573530-0x80573548; .data 0x80612E60-0x80612ED0;
 *   .bss 0x807358C0-0x80735900; .sdata2 0x8079D080-0x8079D090. Strong start: 0x80479030 is a static callback whose
 *   address 0x804795D0 takes. Strong end: 0x804796F0 opens BTE/uusb_ppc.c (it reads the USB .bss 0x80735900).
 *   The .rodata 0x80573530 is the interface table of this file's five entry points (0x804795A0, 0x804795D0,
 *   0x80479640, 0x80479680, 0x804796B0, then a zero word), which BTA_Init installs; the .data are the two "HCIS: ..."
 *   messages, the .bss the reassembly state and the .sdata2 two tables, all read by 0x80479050 and the entry points.
 * FLAGS. Wii/1.3 with `cflags_base` (-O4,p, 16-byte function alignment), as BTE/gki_time.c measured; the lib default
 *   `cflags_os` packs functions to 4 bytes.
 * NAMES. the map's hcisu_h2_init, hcisu_h2_send, hcisu_h2_handle_event and hcisu_h2_send_msg_now. GUESS, from the
 *   bodies and the BTE source's hcisu_h4.c scheme: 0x80479030 hcisu_h2_usb_cback (the USB event callback, reports a
 *   close to BTA), 0x80479050 hcisu_h2_receive_msg (the reassembly state machine), 0x804795D0 hcisu_h2_open,
 *   0x80479640 hcisu_h2_close, 0x80573530 hcisu_h2_interface, 0x807358C0 hcisu_h2_cb, 0x8079D080
 *   hcisu_preamble_table (the preamble lengths of the four H4 message types) and 0x8079D088 hcisu_msg_evt_table
 *   (their BTU events).
 * RESIDUALS. the .bss object is 0x40 bytes in the map while the code reaches only 0x21 bytes of it, so the
 *   control block's tail 0x21..0x40 is an unused approximation. The two "HCIS:" messages are string literals; the
 *   map labels them lbl_80612E60 / lbl_80612E9C.
 * SHAPES. plain C; the receive path keeps one reassembly state per USB pipe (event, ACL, SCO), indexed by the pipe
 *   below the port number, and falls from the message type state into the preamble state as the BTE source does.
 */

#include "BTE/hcisu_h2.h"
#include "BTE/bta_init.h"
#include "BTE/bte_logmsg.h"
#include "BTE/btu_task1.h"
#include "BTE/l2c_link.h"
#include "BTE/uusb_ppc.h"

/* The H4 message types and the BTU events of the messages the HCI services pass on. */
#define HCIT_TYPE_COMMAND 1
#define HCIT_TYPE_ACL_DATA 2
#define HCIT_TYPE_SCO_DATA 3
#define HCIT_TYPE_EVENT 4

#define BT_EVT_TO_LM_HCI_CMD 0x2000
#define BT_EVT_TO_LM_HCI_ACL 0x2100
#define BT_EVT_TO_LM_HCI_SCO 0x2200
#define BT_EVT_TO_BTU_L2C_SEG_XMIT 0x1900

#define BTU_HCI_RCV_MBOX 0

/* The receive states of one pipe. */
#define H2_RCV_MSG_TYPE_ST 0
#define H2_RCV_PREAMBLE_ST 1
#define H2_RCV_PREAMBLE_NO_BUF_ST 2
#define H2_RCV_PAYLOAD_ST 3
#define H2_RCV_IGNORE_ST 4

/* The open state of the transport. */
#define HCISU_H2_CLOSED 0
#define HCISU_H2_OPEN 2

#define HCISU_H2_NUM_PIPES 3

/* The trace mask of the HCI services messages (layer 7, error). */
#define HCISU_TRACE_ERROR 0x70000

/* size: 0x40 - the H2 transport state: one reassembly per receive pipe, the receiving task and the open state
 *   (approximation: the code reaches 0x21 bytes; the map's object is 0x40). */
typedef struct tHCISU_H2_CB {
    /* +0x00 */ BT_HDR* p_rcv_msg[HCISU_H2_NUM_PIPES];
    /* +0x0C */ u16 rcv_len[HCISU_H2_NUM_PIPES];
    /* +0x12 */ u16 rcv_evt;
    /* +0x14 */ u8 rcv_msg_type[HCISU_H2_NUM_PIPES];
    /* +0x17 */ u8 rcv_len_lsb[HCISU_H2_NUM_PIPES];
    /* +0x1A */ u8 rcv_state[HCISU_H2_NUM_PIPES];
    /* +0x1D */ u8 pad_0x1D;
    /* +0x1E */ u8 state;
    /* +0x1F */ u8 task_id;
    /* +0x20 */ u8 mbox;
    /* +0x21 */ u8 unused_0x21[0x1F];
} tHCISU_H2_CB; /* size: 0x40 */

/* .sdata2 0x8079D080 - the preamble length of each H4 message type. */
static const u8 hcisu_preamble_table[] = {3, 4, 3, 2};

/* .sdata2 0x8079D088 - the BTU event of each H4 message type. */
static const u16 hcisu_msg_evt_table[] = {0x1300, 0x1100, 0x1200, 0x1000};

/* .bss 0x807358C0 - the H2 transport state. */
tHCISU_H2_CB hcisu_h2_cb;

/* 0x80479030 (0x14): Reports the close of the USB device to BTA. */
void hcisu_h2_usb_cback(s32 event, s8 status) {
    if (event == UUSB_EVT_CLOSED) {
        bta_usb_close_evt(status);
    }
}

/* 0x80479050 (0x398): Reads the bytes waiting on pipe `chan`, reassembling HCI messages and posting each complete one
 * to the receiving task; returns the bytes read. */
u16 hcisu_h2_receive_msg(u16 chan, tHCISU_H2_CB* p_cb) {
    u16 count = 0;
    u8 byte;
    u16 msg_len;
    u16 len;
    BOOL msg_received;
    u16 port;

    if (p_cb->state != HCISU_H2_OPEN) {
        return 0;
    }

    port = chan + 1;

    for (;;) {
        if (UUSB_Read((u8)port, &byte, 1) == 0) {
            break;
        }
        msg_received = FALSE;
        count++;

        switch (p_cb->rcv_state[chan]) {
        case H2_RCV_MSG_TYPE_ST:
            if (port == UUSB_PORT_EVT) {
                p_cb->p_rcv_msg[chan] = (BT_HDR*)GKI_getpoolbuf(2);
                p_cb->rcv_msg_type[chan] = HCIT_TYPE_EVENT;
            } else if (port == UUSB_PORT_ACL) {
                p_cb->p_rcv_msg[chan] = (BT_HDR*)GKI_getpoolbuf(3);
                p_cb->rcv_msg_type[chan] = HCIT_TYPE_ACL_DATA;
            } else if (port == UUSB_PORT_SCO) {
                p_cb->p_rcv_msg[chan] = (BT_HDR*)GKI_getpoolbuf(1);
                p_cb->rcv_msg_type[chan] = HCIT_TYPE_SCO_DATA;
            } else {
                break;
            }

            if (p_cb->p_rcv_msg[chan] != NULL) {
                p_cb->p_rcv_msg[chan]->len = 0;
                p_cb->p_rcv_msg[chan]->event = hcisu_msg_evt_table[p_cb->rcv_msg_type[chan] - 1];
                p_cb->p_rcv_msg[chan]->offset = 0;
                p_cb->rcv_state[chan] = H2_RCV_PREAMBLE_ST;
            } else {
                LogMsg_0(HCISU_TRACE_ERROR, "HCIS: Unable to allocate buffer for incoming HCI message.");
                p_cb->rcv_state[chan] = H2_RCV_PREAMBLE_NO_BUF_ST;
            }

            p_cb->rcv_len[chan] = hcisu_preamble_table[p_cb->rcv_msg_type[chan] - 1];
            /* fall through */
        case H2_RCV_PREAMBLE_ST:
            *(((BT_BUF*)p_cb->p_rcv_msg[chan])->data + p_cb->p_rcv_msg[chan]->len++) = byte;

            if (--p_cb->rcv_len[chan] == 0) {
                msg_len = byte;
                if (p_cb->rcv_msg_type[chan] == HCIT_TYPE_ACL_DATA) {
                    msg_len = (u16)((msg_len << 8) + p_cb->rcv_len_lsb[chan]);
                    p_cb->p_rcv_msg[chan] = l2cap_link_chk_pkt_start(p_cb->p_rcv_msg[chan]);
                    if (p_cb->p_rcv_msg[chan] == NULL) {
                        p_cb->rcv_len[chan] = msg_len;
                        if (msg_len == 0) {
                            p_cb->rcv_state[chan] = H2_RCV_MSG_TYPE_ST;
                        } else {
                            p_cb->rcv_state[chan] = H2_RCV_IGNORE_ST;
                        }
                        break;
                    }
                }

                p_cb->rcv_len[chan] = msg_len;

                if (msg_len + hcisu_preamble_table[p_cb->rcv_msg_type[chan] - 1] + sizeof(BT_HDR) >
                    GKI_get_buf_size(p_cb->p_rcv_msg[chan])) {
                    GKI_freebuf(p_cb->p_rcv_msg[chan]);
                    p_cb->p_rcv_msg[chan] = NULL;
                    p_cb->rcv_state[chan] = H2_RCV_IGNORE_ST;
                    LogMsg_0(HCISU_TRACE_ERROR, "HCIS: Invalid length for incoming HCI message.");
                } else if (msg_len != 0) {
                    p_cb->rcv_state[chan] = H2_RCV_PAYLOAD_ST;
                } else {
                    p_cb->rcv_state[chan] = H2_RCV_MSG_TYPE_ST;
                    msg_received = TRUE;
                }
            } else {
                p_cb->rcv_len_lsb[chan] = byte;
            }
            break;

        case H2_RCV_PREAMBLE_NO_BUF_ST:
            if (--p_cb->rcv_len[chan] == 0) {
                msg_len = byte;
                if (p_cb->rcv_msg_type[chan] == HCIT_TYPE_ACL_DATA) {
                    msg_len = (u16)((msg_len << 8) + p_cb->rcv_len_lsb[chan]);
                }
                p_cb->rcv_len[chan] = msg_len;
                p_cb->rcv_state[chan] = H2_RCV_IGNORE_ST;
            } else {
                p_cb->rcv_len_lsb[chan] = byte;
            }
            break;

        case H2_RCV_PAYLOAD_ST:
            *(((BT_BUF*)p_cb->p_rcv_msg[chan])->data + p_cb->p_rcv_msg[chan]->len++) = byte;
            p_cb->rcv_len[chan]--;

            len = UUSB_Read((u8)(chan + 1), ((BT_BUF*)p_cb->p_rcv_msg[chan])->data + p_cb->p_rcv_msg[chan]->len,
                            p_cb->rcv_len[chan]);
            count += len;
            p_cb->p_rcv_msg[chan]->len += len;
            p_cb->rcv_len[chan] -= len;

            if (p_cb->rcv_len[chan] == 0) {
                if (p_cb->rcv_msg_type[chan] == HCIT_TYPE_ACL_DATA && !l2cap_link_chk_pkt_end()) {
                    p_cb->rcv_state[chan] = H2_RCV_MSG_TYPE_ST;
                } else {
                    p_cb->rcv_state[chan] = H2_RCV_MSG_TYPE_ST;
                    msg_received = TRUE;
                }
            }
            break;

        case H2_RCV_IGNORE_ST:
            if (--p_cb->rcv_len[chan] == 0) {
                p_cb->rcv_state[chan] = H2_RCV_MSG_TYPE_ST;
            }
            break;
        }

        if (msg_received) {
            GKI_send_msg(hcisu_h2_cb.task_id, BTU_HCI_RCV_MBOX, p_cb->p_rcv_msg[chan]);
            p_cb->p_rcv_msg[chan] = NULL;
        }
    }

    return count;
}

/* 0x804793F0 (0x1AC): Writes `p_msg` to its USB pipe, splitting an ACL packet longer than the controller takes into
 * continuation packets that reuse the buffer. */
void hcisu_h2_send_msg_now(tHCISU_H2_CB* p_cb, BT_HDR* p_msg) {
    u16 event = p_msg->event;
    u8* p = &((BT_BUF*)p_msg)->data[p_msg->offset];
    u8 port;
    u16 handle;

    if (event == BT_EVT_TO_LM_HCI_ACL) {
        port = UUSB_PORT_ACL;
    } else if (event == BT_EVT_TO_LM_HCI_SCO) {
        port = UUSB_PORT_SCO;
    } else if (event == BT_EVT_TO_LM_HCI_CMD) {
        port = UUSB_PORT_CMD;
    }

    if (event == BT_EVT_TO_LM_HCI_ACL && p_msg->len > btu_cb.hcit_acl_pkt_size) {
        handle = (u16)(p[0] + (u16)(p[1] << 8));
        p += 2;
        handle = (handle & 0xCFFF) | 0x1000;

        while (p_msg->len > btu_cb.hcit_acl_pkt_size) {
            UUSB_Write(port, p, btu_cb.hcit_acl_pkt_size, p_msg);

            p_msg->offset += btu_cb.hcit_acl_data_size;
            p_msg->len -= btu_cb.hcit_acl_data_size;

            p = (u8*)(p_msg + 1) + p_msg->offset;
            *p++ = (u8)handle;
            *p++ = (u8)(handle >> 8);

            if (p_msg->len > btu_cb.hcit_acl_pkt_size) {
                *p++ = (u8)btu_cb.hcit_acl_data_size;
                *p++ = btu_cb.hcit_acl_data_size >> 8;
            } else {
                *p++ = (u8)(p_msg->len - 4);
                *p++ = (p_msg->len - 4) >> 8;
            }

            if (p_msg->layer_specific != 0 && --p_msg->layer_specific == 0) {
                p_msg->event = BT_EVT_TO_BTU_L2C_SEG_XMIT;
                GKI_send_msg(hcisu_h2_cb.task_id, BTU_HCI_RCV_MBOX, p_msg);
                return;
            }
        }
    }

    UUSB_Write(port, p, p_msg->len, p_msg);
    GKI_freebuf(p_msg);
}

/* 0x804795A0 (0x2C): Resets the transport state and records the receiving task, its mailbox and its event. */
void hcisu_h2_init(u8 task_id, u8 mbox, u16 rcv_evt) {
    hcisu_h2_cb.state = HCISU_H2_CLOSED;
    hcisu_h2_cb.rcv_state[0] = H2_RCV_MSG_TYPE_ST;
    hcisu_h2_cb.rcv_state[1] = H2_RCV_MSG_TYPE_ST;
    hcisu_h2_cb.rcv_state[2] = H2_RCV_MSG_TYPE_ST;
    hcisu_h2_cb.task_id = task_id;
    hcisu_h2_cb.mbox = mbox;
    hcisu_h2_cb.rcv_evt = rcv_evt;
}

/* 0x804795D0 (0x68): Opens the USB device `p_cfg` names and starts its receive pipes; returns TRUE. */
u8 hcisu_h2_open(tHCISU_CFG* p_cfg) {
    tUUSB_CFG uusb_cfg;
    uusb_cfg.rcv_mode = 0;
    uusb_cfg.vendor_id = p_cfg->vendor_id;
    uusb_cfg.product_id = p_cfg->product_id;
    uusb_cfg.task_id = hcisu_h2_cb.task_id;
    hcisu_h2_cb.state = HCISU_H2_OPEN;
    UUSB_Open(&uusb_cfg);
    UUSB_Register(&uusb_cfg, hcisu_h2_usb_cback);
    return TRUE;
}

/* 0x80479640 (0x34): Closes the USB device. */
void hcisu_h2_close(void) {
    hcisu_h2_cb.state = HCISU_H2_CLOSED;
    UUSB_Close();
    UUSB_Unregister();
}

/* 0x80479680 (0x30): Sends `p_msg` at once; returns TRUE. */
u8 hcisu_h2_send(BT_HDR* p_msg) {
    hcisu_h2_send_msg_now(&hcisu_h2_cb, p_msg);
    return TRUE;
}

/* 0x804796B0 (0x3C): Reads the pipe the receive event `event` signals; returns FALSE. */
u8 hcisu_h2_handle_event(u16 event) {
    hcisu_h2_receive_msg((u16)(event - 8 - hcisu_h2_cb.rcv_evt), &hcisu_h2_cb);
    return FALSE;
}

/* .rodata 0x80573530 - the H2 transport's entry points. */
const tHCISU_IF hcisu_h2_interface = {
    hcisu_h2_init, hcisu_h2_open, hcisu_h2_close, hcisu_h2_send, hcisu_h2_handle_event, NULL,
};
