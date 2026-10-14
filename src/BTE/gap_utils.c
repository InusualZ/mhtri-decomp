/*
 * BTE/gap_utils.c - the GAP utilities: the BTM event callback and its two registered thunks, the inquiry-database
 *   name/address search and the BTM status conversion.
 *
 * RANGE. .text 0x8048CE30-0x8048D44C (6 functions, 0x61C B); .data 0x806152A0-0x80615590. Strong: the .data starts
 *   after a 4-byte pad beyond 4-byte alignment (0x8061529C -> 0x806152A0), and its first reader btm_cback is the first
 *   function after gap_congestion_ind, the last gap_conn.c .data reader - the cut is pinned exactly at 0x8048CE30. It
 *   ends with gap_convert_btm_status; hcicmds.c's btsnd_hcic_inquiry follows. Every .data object (strings and six
 *   switch tables) is read only by this range's functions.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as BTE/sdp_utils.c: every row of the eight gap/hid units compiles to the
 *   target under it; under the lib default (Wii/1.3 `cflags_os`) three of its six rows do not match.
 * NAMES. the map's; btm_cback is the map's name for the GAP BTM callback. gap_btm_cback0/gap_btm_cback1 (gap_btm_cback0,
 *   gap_btm_cback1: the thunks GAP_Init installs, calling btm_cback with block 0 and 1) and gap_find_addr_name_cb
 *   (gap_find_addr_name_cb: the remote name callback gap_find_addr_inq_cb hands BTM_ReadRemoteDeviceName) are the BTE source's.
 * RESIDUALS. gap_inq_results_cb is a GUESS body (the BTE source's) for the dead-stripped function whose two trace
 *   strings sit at .data +0xF0; it adds 0x100 B of .text the retail link strips again. String and jump-table
 *   relocations name the compiler's local pool where the map has lbl_/jumptable_ labels.
 * SHAPES. plain C; gap_convert_btm_status (defined last) is inlined into the three callbacks, gap_free_cb into
 *   btm_cback.
 */

#include "BTE/gap_utils.h"
#include "BTE/gap_api.h"
#include "MSL/strlen.h"
#include "MSL_C/strstr.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* Clears the request block `p_cb`. */
static void gap_free_cb(tGAP_INFO* p_cb) {
    if (p_cb) {
        p_cb->gap_cback = NULL;
        p_cb->in_use = FALSE;
    }
}

/* 0x8048CE30 (0x1D8): Reports the BTM completion `p_data` of the request block `index` to its callback and frees the
 * block. */
void btm_cback(u16 index, void* p_data) { /* untyped: a caller-owned payload */
    tGAP_INFO* p_cb;
    tGAP_INQ_CMPL inq_cmpl;

    if (index < GAP_MAX_BLOCKS && gap_cb.blk[index].in_use) {
        p_cb = &gap_cb.blk[index];

        switch (p_cb->event) {
        case GAP_EVT_INQUIRY_COMPLETE:
            inq_cmpl.num_results = ((tBTM_INQUIRY_CMPL*)p_data)->num_resp;
            inq_cmpl.status = (((tBTM_INQUIRY_CMPL*)p_data)->status == BTM_SUCCESS) ? BT_PASS : GAP_ERR_PROCESSING;
            p_data = &inq_cmpl;
            GAP_TRACE_EVENT2("   GAP Inquiry Complete Event (Status 0x%04x, Result(s) %d)", inq_cmpl.status,
                             inq_cmpl.num_results);
            break;
        case GAP_EVT_DISCOVERY_COMPLETE:
            if (*(u16*)p_data) {
                GAP_TRACE_EVENT1("   GAP Discovery Complete Event(SDP Result: 0x%04x)", *(u16*)p_data);
            } else {
                GAP_TRACE_EVENT0("   GAP Discovery Successfully Completed");
            }
            break;
        case GAP_EVT_REM_NAME_COMPLETE:
            ((tBTM_REMOTE_DEV_NAME*)p_data)->status =
                gap_convert_btm_status((u8)((tBTM_REMOTE_DEV_NAME*)p_data)->status);
            GAP_TRACE_EVENT1("   GAP Remote Name Complete Event (status 0x%04x)",
                             ((tBTM_REMOTE_DEV_NAME*)p_data)->status);
            break;
        }

        if (p_cb->gap_cback) {
            p_cb->gap_cback(p_cb->event, p_data);
        }

        gap_free_cb(p_cb);
    }
}

/* 0x8048D008 (0xC): Reports a BTM completion to request block 0. */
void gap_btm_cback0(void* p1) { /* untyped: a caller-owned payload */
    btm_cback(0, p1);
}

/* 0x8048D014 (0xC): Reports a BTM completion to request block 1. */
void gap_btm_cback1(void* p1) { /* untyped: a caller-owned payload */
    btm_cback(1, p1);
}

/* Traces an inquiry result and passes it to the result callback of every request block running an inquiry. */
void gap_inq_results_cb(tBTM_INQ_INFO* p_results) {
    tGAP_INFO* p_cb;
    u8 index;

    GAP_TRACE_EVENT6("GAP Inquiry Results Callback (bdaddr [%02x%02x%02x%02x%02x%02x])",
                     p_results->remote_bd_addr[0], p_results->remote_bd_addr[1], p_results->remote_bd_addr[2],
                     p_results->remote_bd_addr[3], p_results->remote_bd_addr[4], p_results->remote_bd_addr[5]);
    GAP_TRACE_EVENT4("                             (COD [%02x%02x%02x], clkoff 0x%04x)", p_results->dev_class[0],
                     p_results->dev_class[1], p_results->dev_class[2], p_results->clock_offset);

    for (index = 0, p_cb = &gap_cb.blk[0]; index < GAP_MAX_BLOCKS; index++, p_cb++) {
        if (p_cb->in_use && p_cb->event == GAP_EVT_INQUIRY_COMPLETE) {
            if (p_cb->gap_inq_rslt_cback) {
                p_cb->gap_inq_rslt_cback(GAP_EVT_INQUIRY_RESULTS, p_results);
            }
        }
    }
}

/* 0x8048D020 (0x218): Takes a remote name: ends the find-address-by-name request on a match, else names the next
 * inquiry result. */
void gap_find_addr_name_cb(tBTM_REMOTE_DEV_NAME* p) {
    tGAP_FINDADDR_CB* p_cb = &gap_cb.findaddr_cb;
    tGAP_FINDADDR_RESULTS* p_results = &p_cb->results;

    if (p_cb->in_use) {
        if (p->status == BTM_SUCCESS) {
            GAP_TRACE_EVENT2("   GAP: FindAddrByName Rem Name Cmpl Evt (Status 0x%04x, Name [%s])", p->status,
                             p->remote_bd_name);

            if (!strncmp(p_results->devname, (char*)p->remote_bd_name, strlen(p_results->devname))) {
                memcpy(p_results->bd_addr, p_cb->p_cur_inq->remote_bd_addr, 6);
                p_results->status = BT_PASS;
            } else if ((p_cb->p_cur_inq = BTM_InqDbNext(p_cb->p_cur_inq)) != NULL) {
                if (BTM_ReadRemoteDeviceName(p_cb->p_cur_inq->remote_bd_addr, (BtmCompleteCallback)gap_find_addr_name_cb)
                    == BTM_CMD_STARTED) {
                    return;
                }
                p_results->status = gap_convert_btm_status((u8)p->status);
            } else {
                p_results->status = GAP_NO_DATA_AVAIL;
            }
        } else {
            GAP_TRACE_EVENT1("   GAP: FindAddrByName Rem Name Cmpl Evt (Status 0x%04x)", p->status);
            p_results->status = gap_convert_btm_status((u8)p->status);
        }

        if (p_cb->p_cback) {
            p_cb->p_cback(GAP_EVT_FIND_ADDR_COMPLETE, p_results);
        }

        p_cb->in_use = FALSE;
        p_cb->p_cback = NULL;
    }
}

/* 0x8048D238 (0x1B4): Takes the end of the find-address-by-name request's inquiry and names its first result. */
void gap_find_addr_inq_cb(tBTM_INQUIRY_CMPL* p) {
    tGAP_FINDADDR_CB* p_cb = &gap_cb.findaddr_cb;
    tGAP_FINDADDR_RESULTS* p_results = &p_cb->results;

    if (p_cb->in_use) {
        GAP_TRACE_EVENT2("   GAP: FindAddrByName Inq Cmpl Evt (Status 0x%04x, Result(s) %d)", p->status, p->num_resp);

        if (p->status == BTM_SUCCESS) {
            p_results->status = GAP_EOINQDB;
            if ((p_cb->p_cur_inq = BTM_InqDbFirst()) != NULL) {
                if (BTM_ReadRemoteDeviceName(p_cb->p_cur_inq->remote_bd_addr, (BtmCompleteCallback)gap_find_addr_name_cb)
                    == BTM_CMD_STARTED) {
                    return;
                }
                p_results->status = gap_convert_btm_status(p->status);
            } else {
                p_results->status = GAP_NO_DATA_AVAIL;
            }
        } else {
            p_results->status = gap_convert_btm_status(p->status);
        }

        if (p_cb->p_cback) {
            p_cb->p_cback(GAP_EVT_FIND_ADDR_COMPLETE, p_results);
        }

        p_cb->in_use = FALSE;
        p_cb->p_cback = NULL;
    }
}

/* 0x8048D3EC (0x60): Converts a BTM status to a GAP result. */
u16 gap_convert_btm_status(u8 btm_status) {
    switch (btm_status) {
    case BTM_SUCCESS:
        return BT_PASS;
    case BTM_CMD_STARTED:
        return GAP_CMD_INITIATED;
    case BTM_BUSY:
        return GAP_ERR_BUSY;
    case BTM_MODE_UNSUPPORTED:
    case BTM_ILLEGAL_VALUE:
        return GAP_ERR_ILL_PARM;
    case BTM_WRONG_MODE:
        return GAP_DEVICE_NOT_UP;
    case BTM_UNKNOWN_ADDR:
        return GAP_ERR_UNKNOWN_BD_ADDR;
    case BTM_DEVICE_TIMEOUT:
        return GAP_ERR_TIMEOUT;
    default:
        return GAP_ERR_PROCESSING;
    }
}
