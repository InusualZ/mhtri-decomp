/*
 * BTE/rfc_utils.c - the RFCOMM utilities: FCS calculation and check, multiplexer channel allocation, timers, security
 *   completion, port close and credit flow control.
 *
 * RANGE. .text 0x8049EC6C-0x8049F44C (15 functions, 0x7E0 B); .rodata 0x80573898-0x80573998; .data
 *   0x80617D70-0x80617E00. The .text holds the BTE rfc_utils.c functions rfc_calc_fcs..rfc_check_send_cmd; sdp_api.c's
 *   SDP_InitDiscoveryDb follows. The .rodata is the 0x100 B FCS table rfc_calc_fcs reads; the .data is read only by this
 *   range's functions, no pad beyond 4-byte alignment.
 * FLAGS. see BTE/port_api.c.
 * NAMES. the map's, which are the BTE source's; rfc_crctable (.rodata 0x80573898) is the BTE source's name for the FCS
 *   table.
 * RESIDUALS. none: .text and the claimed data are byte-identical to the target (the .data tail is alignment fill).
 * SHAPES. plain C; rfc_timer_start/stop and rfc_port_timer_stop are defined after their first callers and inlined there.
 */

#include "BTE/rfc_utils.h"
#include "BTE/port_api.h"
#include "BTE/port_rfc.h"
#include "BTE/rfc_mx_fsm.h"
#include "BTE/rfc_port_fsm.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* .rodata 0x80573898 - the reversed CRC-8 table of the RFCOMM frame check sequence. */
static const u8 rfc_crctable[] = {
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF,
};

/* 0x8049EC6C (0x38): Returns the frame check sequence of `len` bytes at `p`. */
u8 rfc_calc_fcs(u16 len, u8* p) {
    u8 fcs = 0xFF;

    while (len--) {
        fcs = rfc_crctable[fcs ^ *p++];
    }
    return 0xFF - fcs;
}

/* 0x8049ECA4 (0x4C): Returns whether `received_fcs` is the frame check sequence of `len` bytes at `p`. */
u8 rfc_check_fcs(u16 len, u8* p, u8 received_fcs) {
    u8 fcs = 0xFF;

    while (len--) {
        fcs = rfc_crctable[fcs ^ *p++];
    }
    fcs = rfc_crctable[fcs ^ received_fcs];
    return fcs == 0xCF;
}

/* 0x8049ECF0 (0x158): Returns the open multiplexer of the device, or a new one with its inactivity timer running. */
tRFC_MCB* rfc_alloc_multiplexer_channel(u8* bd_addr, u8 is_initiator) {
    int i, j;
    tRFC_MCB* p_mcb;

    for (i = 0; i < MAX_BD_CONNECTIONS; i++) {
        if (rfc_cb.port.rfc_mcb[i].state != RFC_MX_STATE_IDLE &&
            memcmp(rfc_cb.port.rfc_mcb[i].bd_addr, bd_addr, BD_ADDR_LEN) == 0) {
            rfc_timer_stop(&rfc_cb.port.rfc_mcb[i]);
            return &rfc_cb.port.rfc_mcb[i];
        }
    }

    for (i = 0, j = rfc_cb.rfc.last_mux + 1; i < MAX_BD_CONNECTIONS; i++, j++) {
        if (j >= MAX_BD_CONNECTIONS) {
            j = 0;
        }
        p_mcb = &rfc_cb.port.rfc_mcb[j];
        if (rfc_cb.port.rfc_mcb[j].state == RFC_MX_STATE_IDLE) {
            memset(p_mcb, 0, sizeof(tRFC_MCB));
            memcpy(p_mcb->bd_addr, bd_addr, BD_ADDR_LEN);
            GKI_init_q(&p_mcb->cmd_q);
            p_mcb->is_initiator = is_initiator;
            rfc_timer_start(p_mcb, RFC_MCB_INIT_INACT_TIMER);
            rfc_cb.rfc.last_mux = (u8)j;
            return p_mcb;
        }
    }
    return NULL;
}

/* 0x8049EE48 (0x88): Stops the multiplexer's timer, frees its queued commands and clears it. */
void rfc_release_multiplexer_channel(tRFC_MCB* p_mcb) {
    BT_HDR* p_buf;

    rfc_timer_stop(p_mcb);
    while ((p_buf = GKI_dequeue(&p_mcb->cmd_q)) != NULL) {
        GKI_freebuf(p_buf);
    }
    memset(p_mcb, 0, sizeof(tRFC_MCB));
    p_mcb->state = RFC_MX_STATE_IDLE;
}

/* 0x8049EED0 (0x74): Starts the multiplexer's timer for `timeout` seconds. */
void rfc_timer_start(tRFC_MCB* p_mcb, u16 timeout) {
    TIMER_LIST_ENT* p_tle = &p_mcb->tle;

    RFCOMM_TRACE_EVENT1("rfc_timer_start - timeout:%d", timeout);
    p_tle->param = (u32)p_mcb;
    btu_start_timer(p_tle, BTU_TTYPE_RFCOMM_MFC, timeout);
}

/* 0x8049EF44 (0x58): Stops the multiplexer's timer. */
void rfc_timer_stop(tRFC_MCB* p_mcb) {
    RFCOMM_TRACE_EVENT0("rfc_timer_stop");
    btu_stop_timer(&p_mcb->tle);
}

/* 0x8049EF9C (0x80): Starts the port's timer for `timeout` seconds. */
void rfc_port_timer_start(tPORT* p_port, u16 timeout) {
    TIMER_LIST_ENT* p_tle = &p_port->rfc.tle;

    RFCOMM_TRACE_EVENT1("rfc_port_timer_start - timeout:%d", timeout);
    p_tle->param = (u32)p_port;
    btu_start_timer(p_tle, BTU_TTYPE_RFCOMM_PORT, timeout);
}

/* 0x8049F01C (0x58): Stops the port's timer. */
void rfc_port_timer_stop(tPORT* p_port) {
    RFCOMM_TRACE_EVENT0("rfc_port_timer_stop");
    btu_stop_timer(&p_port->rfc.tle);
}

/* 0x8049F074 (0xC4): Closes the multiplexer (or times it out) once none of its DLCs is in use. */
void rfc_check_mcb_active(tRFC_MCB* p_mcb) {
    u16 i;

    for (i = 0; i < RFCOMM_MAX_DLCI; i++) {
        if (p_mcb->port_inx[i] != 0) {
            p_mcb->is_disc_initiator = FALSE;
            return;
        }
    }
    if (p_mcb->is_disc_initiator) {
        p_mcb->is_disc_initiator = FALSE;
        rfc_mx_sm_execute(p_mcb, RFC_MX_EVENT_CLOSE_REQ, NULL);
    } else {
        rfc_timer_start(p_mcb, RFC_MCB_RELEASE_INACT_TIMER);
    }
}

/* 0x8049F138 (0x3C): Sends the timeout event of an expired multiplexer or port timer to its state machine. */
void rfcomm_process_timeout(TIMER_LIST_ENT* p_tle) {
    switch (p_tle->event) {
    case BTU_TTYPE_RFCOMM_MFC:
        rfc_mx_sm_execute((tRFC_MCB*)p_tle->param, RFC_EVENT_TIMEOUT, NULL);
        break;
    case BTU_TTYPE_RFCOMM_PORT:
        rfc_port_sm_execute((tPORT*)p_tle->param, RFC_EVENT_TIMEOUT, NULL);
        break;
    default:
        break;
    }
}

/* 0x8049F174 (0x54): Passes the security check result to the port while it still waits for one. */
/* untyped: a caller-owned payload - the reference handed to btm_sec_mx_access_request */
void rfc_sec_check_complete(u8* bd_addr, void* p_ref_data, u8 res) {
    tPORT* p_port = (tPORT*)p_ref_data;

    if (!p_port->in_use || (p_port->rfc.state != RFC_STATE_ORIG_WAIT_SEC_CHECK &&
                            p_port->rfc.state != RFC_STATE_TERM_WAIT_SEC_CHECK)) {
        return;
    }
    rfc_port_sm_execute(p_port, RFC_EVENT_SEC_COMPLETE, &res);
}

/* 0x8049F1C8 (0x158): Marks the port's DLC closed, frees it in its multiplexer and reports the close. */
void rfc_port_closed(tPORT* p_port) {
    tRFC_MCB* p_mcb = p_port->rfc.p_mcb;

    RFCOMM_TRACE_DEBUG0("rfc_port_closed");
    rfc_port_timer_stop(p_port);
    p_port->rfc.state = RFC_STATE_CLOSED;
    if (p_mcb) {
        p_mcb->port_inx[p_port->dlci] = 0;
        p_port->dlci = 0;
        rfc_check_mcb_active(p_mcb);
    }
    port_rfc_closed(p_port, PORT_CLOSED);
}

/* 0x8049F320 (0x8C): Adds received credits to the port and resumes its sending if it was out of credits. */
void rfc_inc_credit(tPORT* p_port, u8 credit) {
    if (p_port->rfc.p_mcb->flow == PORT_FC_CREDIT) {
        p_port->credit_tx += credit;
        RFCOMM_TRACE_EVENT1("rfc_inc_credit:%d", p_port->credit_tx);
        if (p_port->tx.peer_fc == TRUE) {
            PORT_FlowInd(p_port->rfc.p_mcb, p_port->dlci, TRUE);
        }
    }
}

/* 0x8049F3AC (0x3C): Spends one credit of the port and stops its sending when none is left. */
void rfc_dec_credit(tPORT* p_port) {
    if (p_port->rfc.p_mcb->flow == PORT_FC_CREDIT) {
        if (p_port->credit_tx > 0) {
            p_port->credit_tx--;
        }
        if (p_port->credit_tx == 0) {
            p_port->tx.peer_fc = TRUE;
        }
    }
}

/* 0x8049F3E8 (0x64): Queues the command and writes queued commands to L2CAP while it is not congested. */
void rfc_check_send_cmd(tRFC_MCB* p_mcb, BT_HDR* p_buf) {
    BT_HDR* p;

    if (p_buf != NULL) {
        GKI_enqueue(&p_mcb->cmd_q, p_buf);
    }
    while (!p_mcb->l2cap_congested) {
        if ((p = GKI_dequeue(&p_mcb->cmd_q)) == NULL) {
            break;
        }
        L2CA_DataWrite(p_mcb->lcid, p);
    }
}
