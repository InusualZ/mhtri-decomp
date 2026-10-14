/*
 * BTE/rfc_ts_frames.c - the RFCOMM frame builders and parser: SABME/UA/DM/DISC/UIH frames, the multiplexer commands,
 *   rfc_parse_data and rfc_process_mx_message.
 *
 * RANGE. .text 0x8049D624-0x8049EC6C (15 functions, 0x1648 B); .data 0x80617C98-0x80617D70; .sdata
 *   0x80793DB8-0x80793DC8. The .text starts at 0x8049D624 with rfc_send_sabme and ends with rfc_process_mx_message;
 *   rfc_utils.c's rfc_calc_fcs follows. The .data and .sdata (the trace strings, the two 7-byte ones small enough for
 *   .sdata) are read only by this range's functions; no pad beyond 4-byte alignment.
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's. GUESS names: 0x8049D624
 *   rfc_send_sabme, 0x8049D6D4 rfc_send_ua and 0x8049D848 rfc_send_disc by their control bytes (0x3F SABME|P, 0x73
 *   UA|F, 0x53 DISC|P), 0x8049DF6C rfc_send_test by the BTE source order and its 0x21 TEST command byte.
 * RESIDUALS. rfc_send_msc: the target schedules the command byte, the length and the DLCI byte in another order and
 *   with other registers (same instructions); moving the length decision before the buffer setup or after the command
 *   byte measures worse. rfc_send_test: the target reuses r4 (is_command, dead after the CR bit) for offset - 2 where
 *   this source takes r6. The rest of .text, the .data and the .sdata are byte-identical to the target.
 * SHAPES. plain C; rfc_send_nsc is static and inlined into rfc_process_mx_message, its only caller. The frame bytes
 *   are reached through BT_BUF's data, except in rfc_send_buf_uih, which spells the payload `(u8*)&p_buf[1]` (the
 *   byte after the header): there the data[] spelling precomputes the +8 and stores with stbux, where the target adds
 *   the offset first and folds the 8 into each store's displacement.
 */

#include "BTE/rfc_ts_frames.h"
#include "BTE/port_api.h"
#include "BTE/rfc_port_fsm.h"
#include "BTE/rfc_utils.h"

/* The command/response bit of a frame address (from the multiplexer's role) and of a multiplexer command. */
#define RFCOMM_CR(is_initiator, is_command) \
    ((((is_initiator) && (is_command)) || (!(is_initiator) && !(is_command))) << 1)
#define RFCOMM_I_CR(is_command) ((is_command) ? 2 : 0)

/* Whether a received frame is a command or a response from the multiplexer's role, and a valid DLCI. */
#define RFCOMM_FRAME_IS_CMD(initiator, cr) (((initiator) && !(cr)) || (!(initiator) && (cr)))
#define RFCOMM_FRAME_IS_RSP(initiator, cr) (((initiator) && (cr)) || (!(initiator) && !(cr)))
#define RFCOMM_VALID_DLCI(dlci) (((dlci) == 0) || (((dlci) >= 2) && ((dlci) <= RFCOMM_MAX_DLCI)))

/* The fields of the multiplexer commands. */
#define RFCOMM_EA_MASK 0x01
#define RFCOMM_SHIFT_PF 4
#define RFCOMM_PN_FRAM_TYPE_UIH 0x00
#define RFCOMM_PN_PRIORITY_0 0x00
#define RFCOMM_T1_DSEC 0
#define RFCOMM_N2 0
#define RFCOMM_MSC_FC 0x02
#define RFCOMM_MSC_RTC 0x04
#define RFCOMM_MSC_RTR 0x08
#define RFCOMM_MSC_IC 0x40
#define RFCOMM_MSC_DV 0x80
#define RFCOMM_BREAK 0x02
#define RFCOMM_BREAK_SHIFT 4
#define RFCOMM_MSC_BREAK_PRESENT_MASK 0x02
#define RFCOMM_MSC_BREAK_MASK 0xF0
#define RFCOMM_MSC_SHIFT_BREAK 4
#define RFCOMM_RLS_ERROR 0x01
#define RFCOMM_RPN_BITS_MASK 0x03
#define RFCOMM_RPN_BITS_SHIFT 0
#define RFCOMM_RPN_STOP_BITS_MASK 0x01
#define RFCOMM_RPN_STOP_BITS_SHIFT 2
#define RFCOMM_RPN_PARITY_MASK 0x01
#define RFCOMM_RPN_PARITY_SHIFT 3
#define RFCOMM_RPN_PARITY_TYPE_MASK 0x03
#define RFCOMM_RPN_PARITY_TYPE_SHIFT 4
#define RFCOMM_FC_MASK 0x3F
#define RFCOMM_MX_RPN_REQ_LEN 1

/* 0x8049D624 (0xB0): Sends SABME (P=1) on the DLCI. */
void rfc_send_sabme(tRFC_MCB* p_mcb, u8 dlci) {
    BT_HDR* p_buf;
    u8* p_data;
    u8 cr = RFCOMM_CR(p_mcb->is_initiator, TRUE);

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET;
    p_data = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    *p_data++ = RFCOMM_EA | cr | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_SABME | RFCOMM_PF;
    *p_data++ = RFCOMM_EA | 0;
    *p_data = rfc_calc_fcs(RFCOMM_CTRL_FRAME_LEN, &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET]);

    p_buf->len = 4;
    rfc_check_send_cmd(p_mcb, p_buf);
}

/* 0x8049D6D4 (0xB0): Sends UA (F=1) on the DLCI. */
void rfc_send_ua(tRFC_MCB* p_mcb, u8 dlci) {
    BT_HDR* p_buf;
    u8* p_data;
    u8 cr = RFCOMM_CR(p_mcb->is_initiator, FALSE);

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET;
    p_data = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    *p_data++ = RFCOMM_EA | cr | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_UA | RFCOMM_PF;
    *p_data++ = RFCOMM_EA | 0;
    *p_data = rfc_calc_fcs(RFCOMM_CTRL_FRAME_LEN, &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET]);

    p_buf->len = 4;
    rfc_check_send_cmd(p_mcb, p_buf);
}

/* 0x8049D784 (0xC4): Sends DM on the DLCI, with the final bit when `pf` is set. */
void rfc_send_dm(tRFC_MCB* p_mcb, u8 dlci, u8 pf) {
    BT_HDR* p_buf;
    u8* p_data;
    u8 cr = RFCOMM_CR(p_mcb->is_initiator, FALSE);

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET;
    p_data = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    *p_data++ = RFCOMM_EA | cr | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_DM | ((pf) ? RFCOMM_PF : 0);
    *p_data++ = RFCOMM_EA | 0;
    *p_data = rfc_calc_fcs(RFCOMM_CTRL_FRAME_LEN, &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET]);

    p_buf->len = 4;
    rfc_check_send_cmd(p_mcb, p_buf);
}

/* 0x8049D848 (0xB0): Sends DISC (P=1) on the DLCI. */
void rfc_send_disc(tRFC_MCB* p_mcb, u8 dlci) {
    BT_HDR* p_buf;
    u8* p_data;
    u8 cr = RFCOMM_CR(p_mcb->is_initiator, TRUE);

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET;
    p_data = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    *p_data++ = RFCOMM_EA | cr | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_DISC | RFCOMM_PF;
    *p_data++ = RFCOMM_EA | 0;
    *p_data = rfc_calc_fcs(RFCOMM_CTRL_FRAME_LEN, &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET]);

    p_buf->len = 4;
    rfc_check_send_cmd(p_mcb, p_buf);
}

/* 0x8049D8F8 (0x198): Wraps the buffer's payload in a UIH frame (with credits from `layer_specific`) and sends it. */
void rfc_send_buf_uih(tRFC_MCB* p_mcb, u8 dlci, BT_HDR* p_buf) {
    u8* p_data;
    u8 cr = RFCOMM_CR(p_mcb->is_initiator, TRUE);
    u8 credits;

    p_buf->offset -= RFCOMM_CTRL_FRAME_LEN;
    if (p_buf->len > 127) {
        p_buf->offset--;
    }

    if (dlci) {
        credits = (u8)p_buf->layer_specific;
    } else {
        credits = 0;
    }

    if (credits) {
        p_buf->offset--;
    }

    p_data = (u8*)&p_buf[1] + p_buf->offset;

    *p_data++ = RFCOMM_EA | cr | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_UIH | ((credits) ? RFCOMM_PF : 0);
    if (p_buf->len <= 127) {
        *p_data++ = RFCOMM_EA | (p_buf->len << 1);
        p_buf->len += 3;
    } else {
        *p_data++ = (p_buf->len & 0x7F) << 1;
        *p_data++ = p_buf->len >> RFCOMM_SHIFT_LENGTH2;
        p_buf->len += 4;
    }

    if (credits) {
        *p_data++ = credits;
        p_buf->len++;
    }

    p_data = (u8*)&p_buf[1] + p_buf->offset + p_buf->len++;
    *p_data = rfc_calc_fcs(2, (u8*)&p_buf[1] + p_buf->offset);

    if (dlci == RFCOMM_MX_DLCI) {
        rfc_check_send_cmd(p_mcb, p_buf);
    } else {
        L2CA_DataWrite(p_mcb->lcid, p_buf);
    }
}

/* 0x8049DA90 (0xE8): Sends a parameter negotiation command or response for the DLC. */
void rfc_send_pn(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, u16 mtu, u8 cl, u8 k) {
    BT_HDR* p_buf;
    u8* p_data;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_PN;
    *p_data++ = RFCOMM_EA | (RFCOMM_MX_PN_LEN << 1);

    *p_data++ = dlci;
    *p_data++ = RFCOMM_PN_FRAM_TYPE_UIH | cl;

    if (is_command) {
        *p_data++ = RFCOMM_PN_PRIORITY_0;
    } else {
        *p_data++ = rfc_cb.rfc.rx_frame.u.pn.priority;
    }

    *p_data++ = RFCOMM_T1_DSEC;
    *p_data++ = mtu & 0xFF;
    *p_data++ = mtu >> 8;
    *p_data++ = RFCOMM_N2;
    *p_data = k;

    p_buf->len = RFCOMM_MX_PN_LEN + 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049DB78 (0x84): Sends a flow-on command or response. */
void rfc_send_fcon(tRFC_MCB* p_mcb, u8 is_command) {
    BT_HDR* p_buf;
    u8* p_data;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_FCON;
    *p_data++ = RFCOMM_EA | (RFCOMM_MX_FCON_LEN << 1);

    p_buf->len = RFCOMM_MX_FCON_LEN + 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049DBFC (0x84): Sends a flow-off command or response. */
void rfc_send_fcoff(tRFC_MCB* p_mcb, u8 is_command) {
    BT_HDR* p_buf;
    u8* p_data;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_FCOFF;
    *p_data++ = RFCOMM_EA | (RFCOMM_MX_FCOFF_LEN << 1);

    p_buf->len = RFCOMM_MX_FCOFF_LEN + 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049DC80 (0x120): Sends a modem status command or response for the DLC (with the break when there is one). */
void rfc_send_msc(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, tPORT_CTRL* p_pars) {
    BT_HDR* p_buf;
    u8* p_data;
    u8 signals;
    u8 break_duration;
    u8 len;

    signals = p_pars->modem_signal;
    break_duration = p_pars->break_signal;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    if (break_duration) {
        len = RFCOMM_MX_MSC_LEN_WITH_BREAK;
    } else {
        len = RFCOMM_MX_MSC_LEN_NO_BREAK;
    }

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_MSC;
    *p_data++ = RFCOMM_EA | (len << 1);

    *p_data++ = RFCOMM_EA | RFCOMM_CR_MASK | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_EA | ((p_pars->fc) ? RFCOMM_MSC_FC : 0) | ((signals & PORT_DTRDSR_ON) ? RFCOMM_MSC_RTC : 0) |
                ((signals & PORT_CTSRTS_ON) ? RFCOMM_MSC_RTR : 0) | ((signals & PORT_RING_ON) ? RFCOMM_MSC_IC : 0) |
                ((signals & PORT_DCD_ON) ? RFCOMM_MSC_DV : 0);

    if (break_duration) {
        *p_data++ = RFCOMM_EA | RFCOMM_BREAK | (break_duration << RFCOMM_BREAK_SHIFT);
    }

    p_buf->len = len + 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049DDA0 (0xB0): Sends a line status command or response for the DLC. */
void rfc_send_rls(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, u8 status) {
    BT_HDR* p_buf;
    u8* p_data;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_RLS;
    *p_data++ = RFCOMM_EA | (RFCOMM_MX_RLS_LEN << 1);

    *p_data++ = RFCOMM_EA | RFCOMM_CR_MASK | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_RLS_ERROR | status;

    p_buf->len = RFCOMM_MX_RLS_LEN + 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* Answers a multiplexer command this side does not support with a non-supported command response. */
static void rfc_send_nsc(tRFC_MCB* p_mcb) {
    BT_HDR* p_buf;
    u8* p_data;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(FALSE) | RFCOMM_MX_NSC;
    *p_data++ = RFCOMM_EA | (RFCOMM_MX_NSC_LEN << 1);

    *p_data++ = rfc_cb.rfc.rx_frame.ea | (rfc_cb.rfc.rx_frame.cr << RFCOMM_SHIFT_CR) | rfc_cb.rfc.rx_frame.type;

    p_buf->len = RFCOMM_MX_NSC_LEN + 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049DE50 (0x11C): Sends a port negotiation command or response (a query when `p_pars` is NULL). */
void rfc_send_rpn(tRFC_MCB* p_mcb, u8 dlci, u8 is_command, tPORT_STATE* p_pars, u16 mask) {
    BT_HDR* p_buf;
    u8* p_data;

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_CTRL_FRAME_LEN;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_RPN;

    if (!p_pars) {
        *p_data++ = RFCOMM_EA | (RFCOMM_MX_RPN_REQ_LEN << 1);
        *p_data++ = RFCOMM_EA | RFCOMM_CR_MASK | (dlci << RFCOMM_SHIFT_DLCI);
        p_buf->len = RFCOMM_MX_RPN_REQ_LEN + 2;
    } else {
        *p_data++ = RFCOMM_EA | (RFCOMM_MX_RPN_LEN << 1);
        *p_data++ = RFCOMM_EA | RFCOMM_CR_MASK | (dlci << RFCOMM_SHIFT_DLCI);
        *p_data++ = p_pars->baud_rate;
        *p_data++ = (p_pars->byte_size << RFCOMM_RPN_BITS_SHIFT) | (p_pars->stop_bits << RFCOMM_RPN_STOP_BITS_SHIFT) |
                    (p_pars->parity << RFCOMM_RPN_PARITY_SHIFT) |
                    (p_pars->parity_type << RFCOMM_RPN_PARITY_TYPE_SHIFT);
        *p_data++ = p_pars->fc_type;
        *p_data++ = p_pars->xon_char;
        *p_data++ = p_pars->xoff_char;
        *p_data++ = (mask & 0xFF);
        *p_data++ = (mask >> 8);
        p_buf->len = RFCOMM_MX_RPN_LEN + 2;
    }
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049DF6C (0xA8): Sends the buffer's payload as a test command or response, moving it back to make room. */
void rfc_send_test(tRFC_MCB* p_mcb, u8 is_command, BT_HDR* p_buf) {
    u8* p_data;
    u16 xx;
    u8 *p_src, *p_dest;

    if (p_buf->offset < (L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET + 2)) {
        p_src = &((BT_BUF*)p_buf)->data[p_buf->offset] + p_buf->len - 1;
        p_dest = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET + 2] + p_buf->len - 1;

        for (xx = 0; xx < p_buf->len; xx++) {
            *p_dest-- = *p_src--;
        }

        p_buf->offset = L2CAP_MIN_OFFSET + RFCOMM_MIN_OFFSET + 2;
    }

    p_buf->offset -= 2;
    p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];

    *p_data++ = RFCOMM_EA | RFCOMM_I_CR(is_command) | RFCOMM_MX_TEST;
    *p_data++ = RFCOMM_EA | (p_buf->len << 1);

    p_buf->len += 2;
    rfc_send_buf_uih(p_mcb, RFCOMM_MX_DLCI, p_buf);
}

/* 0x8049E014 (0xC0): Sends `credit` credits for the DLC in an empty UIH frame. */
void rfc_send_credit(tRFC_MCB* p_mcb, u8 dlci, u8 credit) {
    BT_HDR* p_buf;
    u8* p_data;
    u8 cr = RFCOMM_CR(p_mcb->is_initiator, TRUE);

    if ((p_buf = GKI_getpoolbuf(RFCOMM_CMD_POOL_ID)) == NULL) {
        return;
    }

    p_buf->offset = L2CAP_MIN_OFFSET;
    p_data = &((BT_BUF*)p_buf)->data[L2CAP_MIN_OFFSET];

    *p_data++ = RFCOMM_EA | cr | (dlci << RFCOMM_SHIFT_DLCI);
    *p_data++ = RFCOMM_UIH | RFCOMM_PF;
    *p_data++ = RFCOMM_EA | 0;
    *p_data++ = credit;
    *p_data = rfc_calc_fcs(2, &((BT_BUF*)p_buf)->data[p_buf->offset]);

    p_buf->len = 5;
    rfc_check_send_cmd(p_mcb, p_buf);
}

/* 0x8049E0D4 (0x574): Decodes the frame header into `p_frame`, checks it and its FCS and returns its event. */
u8 rfc_parse_data(tRFC_MCB* p_mcb, MX_FRAME* p_frame, BT_HDR* p_buf) {
    u8 ead, eal, fcs;
    u8* p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];
    u8* p_start = p_data;
    u16 len;

    if (p_buf->len < RFCOMM_CTRL_FRAME_LEN) {
        RFCOMM_TRACE_ERROR1("Bad Length1: %d", p_buf->len);
        return RFC_EVENT_BAD_FRAME;
    }

    ead = *p_data & RFCOMM_EA;
    p_frame->cr = (*p_data & RFCOMM_CR_MASK) >> RFCOMM_SHIFT_CR;
    p_frame->dlci = *p_data++ >> RFCOMM_SHIFT_DLCI;
    if (!ead) {
        p_frame->dlci += *p_data++ << RFCOMM_SHIFT_DLCI2;
    }

    p_frame->type = *p_data & ~RFCOMM_PF;
    p_frame->pf = (*p_data++ & RFCOMM_PF) >> RFCOMM_SHIFT_PF;

    eal = *p_data & RFCOMM_EA;
    len = *p_data++ >> RFCOMM_SHIFT_LENGTH1;
    if (!eal) {
        len += *p_data++ << RFCOMM_SHIFT_LENGTH2;
    }

    p_buf->len -= (3 + !ead + !eal + 1);
    p_buf->offset += (3 + !ead + !eal);

    if (p_mcb->flow == PORT_FC_CREDIT && p_frame->type == RFCOMM_UIH && p_frame->dlci != RFCOMM_MX_DLCI &&
        p_frame->pf == 1) {
        p_frame->credit = *p_data++;
        p_buf->len--;
        p_buf->offset++;
    } else {
        p_frame->credit = 0;
    }

    if (p_buf->len != len) {
        RFCOMM_TRACE_ERROR2("Bad Length2 %d %d", p_buf->len, len);
        return RFC_EVENT_BAD_FRAME;
    }

    fcs = *(p_data + len);

    switch (p_frame->type) {
    case RFCOMM_SABME:
        if (RFCOMM_FRAME_IS_RSP(p_mcb->is_initiator, p_frame->cr) || !p_frame->pf || len ||
            !RFCOMM_VALID_DLCI(p_frame->dlci) || !rfc_check_fcs(RFCOMM_CTRL_FRAME_LEN, p_start, fcs)) {
            RFCOMM_TRACE_ERROR0("Bad SABME");
            return RFC_EVENT_BAD_FRAME;
        } else {
            return RFC_EVENT_SABME;
        }

    case RFCOMM_UA:
        if (RFCOMM_FRAME_IS_CMD(p_mcb->is_initiator, p_frame->cr) || !p_frame->pf || len ||
            !RFCOMM_VALID_DLCI(p_frame->dlci) || !rfc_check_fcs(RFCOMM_CTRL_FRAME_LEN, p_start, fcs)) {
            RFCOMM_TRACE_ERROR0("Bad UA");
            return RFC_EVENT_BAD_FRAME;
        } else {
            return RFC_EVENT_UA;
        }

    case RFCOMM_DM:
        if (RFCOMM_FRAME_IS_CMD(p_mcb->is_initiator, p_frame->cr) || len || !RFCOMM_VALID_DLCI(p_frame->dlci) ||
            !rfc_check_fcs(RFCOMM_CTRL_FRAME_LEN, p_start, fcs)) {
            RFCOMM_TRACE_ERROR0("Bad DM");
            return RFC_EVENT_BAD_FRAME;
        } else {
            return RFC_EVENT_DM;
        }

    case RFCOMM_DISC:
        if (RFCOMM_FRAME_IS_RSP(p_mcb->is_initiator, p_frame->cr) || !p_frame->pf || len ||
            !RFCOMM_VALID_DLCI(p_frame->dlci) || !rfc_check_fcs(RFCOMM_CTRL_FRAME_LEN, p_start, fcs)) {
            RFCOMM_TRACE_ERROR0("Bad DISC");
            return RFC_EVENT_BAD_FRAME;
        } else {
            return RFC_EVENT_DISC;
        }

    case RFCOMM_UIH:
        if (!RFCOMM_VALID_DLCI(p_frame->dlci)) {
            RFCOMM_TRACE_ERROR0("Bad UIH - invalid DLCI");
            return RFC_EVENT_BAD_FRAME;
        } else if (!rfc_check_fcs(2, p_start, fcs)) {
            RFCOMM_TRACE_ERROR0("Bad UIH - FCS");
            return RFC_EVENT_BAD_FRAME;
        } else if (RFCOMM_FRAME_IS_RSP(p_mcb->is_initiator, p_frame->cr)) {
            RFCOMM_TRACE_ERROR0("Bad UIH - response");
            return RFC_EVENT_UIH;
        } else {
            return RFC_EVENT_UIH;
        }
    }

    return RFC_EVENT_BAD_FRAME;
}

/* 0x8049E648 (0x624): Decodes a multiplexer command or response on DLCI 0 and passes it on; frees the buffer. */
void rfc_process_mx_message(tRFC_MCB* p_mcb, BT_HDR* p_buf) {
    u8* p_data = &((BT_BUF*)p_buf)->data[p_buf->offset];
    MX_FRAME* p_rx_frame = &rfc_cb.rfc.rx_frame;
    u16 length = p_buf->len;
    u8 ea, cr, mx_len;
    u8 is_command;

    p_rx_frame->ea = *p_data & RFCOMM_EA;
    p_rx_frame->cr = (*p_data & RFCOMM_CR_MASK) >> RFCOMM_SHIFT_CR;
    p_rx_frame->type = *p_data++ & ~(RFCOMM_CR_MASK | RFCOMM_EA_MASK);

    if (!p_rx_frame->ea || !length) {
        RFCOMM_TRACE_ERROR2("Illegal MX Frame ea:%d len:%d", p_rx_frame->ea, length);
        GKI_freebuf(p_buf);
        return;
    }

    length--;

    is_command = p_rx_frame->cr;

    ea = *p_data & RFCOMM_EA;

    mx_len = *p_data++ >> RFCOMM_SHIFT_LENGTH1;
    length--;

    if (!ea) {
        mx_len += *p_data++ << RFCOMM_SHIFT_LENGTH2;
        length--;
    }

    if (mx_len != length) {
        RFCOMM_TRACE_ERROR0("Bad MX frame");
        GKI_freebuf(p_buf);
        return;
    }

    switch (p_rx_frame->type) {
    case RFCOMM_MX_PN:
        if (length != RFCOMM_MX_PN_LEN) {
            break;
        }

        p_rx_frame->dlci = *p_data++ & RFCOMM_PN_DLCI_MASK;
        p_rx_frame->u.pn.frame_type = *p_data & RFCOMM_PN_FRAM_TYPE_MASK;
        p_rx_frame->u.pn.conv_layer = *p_data++ & RFCOMM_PN_CONV_LAYER_MASK;
        p_rx_frame->u.pn.priority = *p_data++ & RFCOMM_PN_PRIORITY_MASK;
        p_rx_frame->u.pn.t1 = *p_data++;
        p_rx_frame->u.pn.mtu = *p_data + (*(p_data + 1) << 8);
        p_data += 2;
        p_rx_frame->u.pn.n2 = *p_data++;
        p_rx_frame->u.pn.k = *p_data++ & RFCOMM_PN_K_MASK;

        if (!p_rx_frame->dlci || !RFCOMM_VALID_DLCI(p_rx_frame->dlci) || (p_rx_frame->u.pn.mtu < RFCOMM_MIN_MTU) ||
            (p_rx_frame->u.pn.mtu > RFCOMM_MAX_MTU)) {
            RFCOMM_TRACE_ERROR0("Bad PN frame");
            break;
        }

        GKI_freebuf(p_buf);
        rfc_process_pn(p_mcb, is_command, p_rx_frame);
        return;

    case RFCOMM_MX_TEST:
        if (!length) {
            break;
        }

        p_rx_frame->u.test.p_data = p_data;
        p_rx_frame->u.test.data_len = length;

        p_buf->offset += 2;
        p_buf->len -= 2;

        if (is_command) {
            rfc_send_test(p_mcb, FALSE, p_buf);
        } else {
            rfc_process_test_rsp(p_mcb, p_buf);
        }
        return;

    case RFCOMM_MX_FCON:
        if (length != RFCOMM_MX_FCON_LEN) {
            break;
        }

        GKI_freebuf(p_buf);
        rfc_process_fcon(p_mcb, is_command);
        return;

    case RFCOMM_MX_FCOFF:
        if (length != RFCOMM_MX_FCOFF_LEN) {
            break;
        }

        GKI_freebuf(p_buf);
        rfc_process_fcoff(p_mcb, is_command);
        return;

    case RFCOMM_MX_MSC:
        ea = *p_data & RFCOMM_EA;
        cr = (*p_data & RFCOMM_CR_MASK) >> RFCOMM_SHIFT_CR;
        p_rx_frame->dlci = *p_data++ >> RFCOMM_SHIFT_DLCI;

        if (!ea || !cr || !p_rx_frame->dlci || !RFCOMM_VALID_DLCI(p_rx_frame->dlci)) {
            RFCOMM_TRACE_ERROR0("Bad MSC frame");
            break;
        }

        p_rx_frame->u.msc.signals = *p_data++;

        if (mx_len == RFCOMM_MX_MSC_LEN_WITH_BREAK) {
            p_rx_frame->u.msc.break_present = *p_data & RFCOMM_MSC_BREAK_PRESENT_MASK;
            p_rx_frame->u.msc.break_duration = (*p_data & RFCOMM_MSC_BREAK_MASK) >> RFCOMM_MSC_SHIFT_BREAK;
        } else {
            p_rx_frame->u.msc.break_present = FALSE;
            p_rx_frame->u.msc.break_duration = 0;
        }
        GKI_freebuf(p_buf);
        rfc_process_msc(p_mcb, is_command, p_rx_frame);
        return;

    case RFCOMM_MX_NSC:
        if ((length != RFCOMM_MX_NSC_LEN) || !is_command) {
            break;
        }

        p_rx_frame->u.nsc.ea = *p_data & RFCOMM_EA;
        p_rx_frame->u.nsc.cr = (*p_data & RFCOMM_CR_MASK) >> RFCOMM_SHIFT_CR;
        p_rx_frame->u.nsc.type = *p_data++ >> RFCOMM_SHIFT_DLCI;

        GKI_freebuf(p_buf);
        rfc_process_nsc(p_mcb, p_rx_frame);
        return;

    case RFCOMM_MX_RPN:
        if ((length != RFCOMM_MX_RPN_REQ_LEN) && (length != RFCOMM_MX_RPN_LEN)) {
            break;
        }

        ea = *p_data & RFCOMM_EA;
        cr = (*p_data & RFCOMM_CR_MASK) >> RFCOMM_SHIFT_CR;
        p_rx_frame->dlci = *p_data++ >> RFCOMM_SHIFT_DLCI;

        if (!ea || !cr || !p_rx_frame->dlci || !RFCOMM_VALID_DLCI(p_rx_frame->dlci)) {
            RFCOMM_TRACE_ERROR0("Bad RPN frame");
            break;
        }

        p_rx_frame->u.rpn.is_request = (length == RFCOMM_MX_RPN_REQ_LEN);

        if (!p_rx_frame->u.rpn.is_request) {
            p_rx_frame->u.rpn.baud_rate = *p_data++;
            p_rx_frame->u.rpn.byte_size = (*p_data >> RFCOMM_RPN_BITS_SHIFT) & RFCOMM_RPN_BITS_MASK;
            p_rx_frame->u.rpn.stop_bits = (*p_data >> RFCOMM_RPN_STOP_BITS_SHIFT) & RFCOMM_RPN_STOP_BITS_MASK;
            p_rx_frame->u.rpn.parity = (*p_data >> RFCOMM_RPN_PARITY_SHIFT) & RFCOMM_RPN_PARITY_MASK;
            p_rx_frame->u.rpn.parity_type = (*p_data++ >> RFCOMM_RPN_PARITY_TYPE_SHIFT) & RFCOMM_RPN_PARITY_TYPE_MASK;

            p_rx_frame->u.rpn.fc_type = *p_data++ & RFCOMM_FC_MASK;
            p_rx_frame->u.rpn.xon_char = *p_data++;
            p_rx_frame->u.rpn.xoff_char = *p_data++;
            p_rx_frame->u.rpn.param_mask = (*p_data + (*(p_data + 1) << 8)) & RFCOMM_RPN_PM_MASK;
        }
        GKI_freebuf(p_buf);
        rfc_process_rpn(p_mcb, is_command, p_rx_frame->u.rpn.is_request, p_rx_frame);
        return;

    case RFCOMM_MX_RLS:
        if (length != RFCOMM_MX_RLS_LEN) {
            break;
        }

        ea = *p_data & RFCOMM_EA;
        cr = (*p_data & RFCOMM_CR_MASK) >> RFCOMM_SHIFT_CR;

        p_rx_frame->dlci = *p_data++ >> RFCOMM_SHIFT_DLCI;
        p_rx_frame->u.rls.line_status = (*p_data & ~0x01);

        if (!ea || !cr || !p_rx_frame->dlci || !RFCOMM_VALID_DLCI(p_rx_frame->dlci)) {
            RFCOMM_TRACE_ERROR0("Bad RPN frame");
            break;
        }

        GKI_freebuf(p_buf);
        rfc_process_rls(p_mcb, is_command, p_rx_frame);
        return;
    }

    GKI_freebuf(p_buf);

    if (is_command) {
        rfc_send_nsc(p_mcb);
    }
}
