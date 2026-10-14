/*
 * BTE/btm_inq.c - the BTM inquiry: discoverability and connectability, the inquiry/page scan types, inquiry mode,
 *   start and cancel inquiry, remote name requests, the inquiry database and the inquiry result/complete handlers.
 *
 * RANGE. .text 0x8048495C-0x804863F4 (25 functions, 0x1A98 B); .data 0x806141E0-0x806143F0; .sdata2
 *   0x8079D0B8-0x8079D0C0. Strong start: the .data starts after a 4-byte pad beyond 4-byte alignment (0x806141DC ->
 *   0x806141E0) and BTM_SetDiscoverability, the first function of the BTE source's btm_inq.c, is its first reader.
 *   The end is medium: btm_inq_rmt_name_failed (the last btm_inq.c function in the BTE source) before btm_init; the
 *   next .data pad seam 0x806143F0 allows any cut in [0x8048610C, 0x80486EBC]. The .sdata2 words 0x8079D0B8 and
 *   0x8079D0BC (the GIAC and LIAC access codes) are read only by BTM_SetDiscoverability and btm_event_filter_complete.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`, as the btm band (BTE/btm_pm.c's FLAGS line).
 * NAMES. the BTE source's. GUESS: 0x80484BE0 BTM_SetInquiryScanType and
 *   0x80484C90 BTM_SetPageScanType (twin bodies; the first reads the interlaced inquiry scan feature bit and calls the
 *   inquiry scan type builder, the second the page scan ones), general_inq_lap/limited_inq_lap for the two .sdata2
 *   access codes. btm_clr_inq_db, btm_clr_inq_result_flt, btm_inq_db_find and btm_initiate_inquiry are static helpers
 *   the compiler inlines into their callers.
 * RESIDUALS. BTM_StartInquiry: retail materialises the CLR_FILT state constant 1 with the copy's zero ahead of the
 *   parameter copy (the case order, an if chain and the store order measured the same or worse).
 *   btm_process_inq_results: register assignment of the result fields (the declaration order is the best of a search;
 *   btm_cb's inquiry variables read directly measured the same).
 * SHAPES. plain C; the HCI command buffer is taken success-first (`if ((p_buf = GKI_getpoolbuf(..)) != NULL) {..}
 *   else return BTM_NO_RESOURCES;`), as every scan setting's branch layout shows. btm_clr_inq_db reads btm_cb's
 *   inquiry variables directly (no p_inq local), which gives retail's counter/pointer order and the separate
 *   lis/addi of the btm_cb base in its two inlined copies.
 */

#include "BTE/btm_inq.h"
#include "BTE/btm_main.h"
#include "BTE/hcicmds.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_sec.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTM status codes. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3
#define BTM_MODE_UNSUPPORTED 4
#define BTM_ILLEGAL_VALUE 5
#define BTM_WRONG_MODE 6
#define BTM_BAD_VALUE_RET 9
#define BTM_ERR_PROCESSING 10
#define BTM_DEV_RESET 12

/* The discoverability and connectability modes and their scan defaults and limits (0.625 ms slots). */
#define BTM_NON_DISCOVERABLE 0
#define BTM_LIMITED_DISCOVERABLE 1
#define BTM_GENERAL_DISCOVERABLE 2
#define BTM_NON_CONNECTABLE 0
#define BTM_CONNECTABLE 1
#define BTM_DEFAULT_SCAN_WINDOW 0x0012
#define BTM_DEFAULT_SCAN_INTERVAL 0x0800
#define HCI_MIN_SCAN_SLOTS 0x0012
#define HCI_MAX_SCAN_SLOTS 0x1000
#define BTM_SCAN_TYPE_STANDARD 0
#define BTM_SCAN_TYPE_INTERLACED 1
#define HCI_INQUIRY_SCAN_ENABLED 0x01
#define HCI_PAGE_SCAN_ENABLED 0x02

/* The inquiry modes, the inquiry activity flags and the inquiry state machine. */
#define BTM_GENERAL_INQUIRY 0
#define BTM_LIMITED_INQUIRY 1
#define BTM_INQUIRY_INACTIVE 0
#define BTM_LIMITED_INQUIRY_ACTIVE 1
#define BTM_GENERAL_INQUIRY_ACTIVE 2
#define BTM_PERIODIC_INQUIRY_ACTIVE 4
#define BTM_INQ_INACTIVE_STATE 0
#define BTM_INQ_CLR_FILT_STATE 1
#define BTM_INQ_SET_FILT_STATE 2
#define BTM_INQ_ACTIVE_STATE 3
#define BTM_INQ_RESULT_STANDARD 0
#define BTM_INQ_RES_IGNORE_RSSI 0x7F
#define BTM_CLOCK_OFFSET_VALID 0x8000

/* The inquiry event filter conditions. */
#define BTM_CLR_INQUIRY_FILTER 0
#define BTM_FILTER_COND_DEVICE_CLASS 1
#define BTM_FILTER_COND_BD_ADDR 2
#define HCI_FILTER_INQUIRY_RESULT 1

/* The remote name request origins and timeouts (seconds), the name length and the timer type. */
#define BTM_RMT_NAME_EXT 0x01
#define BTM_EXT_RMT_NAME_TIMEOUT 40
#define BD_NAME_LEN 248
#define BTU_TTYPE_BTM_RMT_NAME 10
#define HCI_PAGE_SCAN_REP_MODE_R1 1
#define HCI_MANDATARY_PAGE_SCAN_MODE 0

/* The local feature bits of the interlaced scans and the inquiry result with RSSI. */
#define HCI_LMP_INTERLACED_INQ_SCAN_SUPPORTED(f) ((f)[3] & 0x10)
#define HCI_LMP_INTERLACED_PAGE_SCAN_SUPPORTED(f) ((f)[3] & 0x20)
#define HCI_LMP_INQ_RSSI_SUPPORTED(f) ((f)[3] & 0x40)

/* The class of device service bit of limited discoverability and the field accessors. */
#define BTM_COD_SERVICE_LMTD_DISCOVER 0x0020
#define BTM_COD_SERVICE_CLASS_LO_B 0x00E0
#define BTM_COD_MINOR_CLASS_MASK 0xFC
#define BTM_COD_MAJOR_CLASS_MASK 0x1F

/* The reset-sequence state from which the scan settings are sent, and the GKI pools and buffer size the inquiry
 * uses. */
#define BTM_DEV_STATE_SCAN_READY 3
#define HCI_CMD_POOL_ID 2
#define GKI_MAX_BUF_SIZE 0x708
#define HCI_SUCCESS 0
#define HCI_ERR_UNSPECIFIED 0x1F

/* The trace levels and trace masks of the BTM layer. */
#define BT_TRACE_LEVEL_WARNING 2
#define BT_TRACE_LEVEL_API 3
#define BT_TRACE_LEVEL_DEBUG 5
#define TRACE_BTM_WARNING 0x000D0001
#define TRACE_BTM_API 0x000D0002
#define TRACE_BTM_DEBUG 0x000D0004

/* The byte offset of `member` in `type`. */
#define offsetof(type, member) ((u32) & ((type*)0)->member)

/* The general and limited inquiry access codes. */
const LAP general_inq_lap = {0x9E, 0x8B, 0x33};
const LAP limited_inq_lap = {0x9E, 0x8B, 0x00};

/* 0x8048495C (0x284): Sets the inquiry scan (discoverable) mode with its window and interval, and keeps the limited
 * discoverable bit of the class of device in step. */
u8 BTM_SetDiscoverability(u16 inq_mode, u16 window, u16 interval) {
    u8 scan_mode = 0;
    u16 service_class;
    u8* p_cod;
    u8 major;
    u8 minor;
    u8 cod[3];
    LAP temp_lap[2];
    u8 is_limited;
    u8 cod_limited;
    BT_HDR* p_buf;

    if (inq_mode != BTM_NON_DISCOVERABLE && inq_mode != BTM_LIMITED_DISCOVERABLE && inq_mode != BTM_GENERAL_DISCOVERABLE) {
        return BTM_ILLEGAL_VALUE;
    }

    if (btm_cb.devcb_state < BTM_DEV_STATE_SCAN_READY) {
        return BTM_DEV_RESET;
    }

    if (!window) {
        window = BTM_DEFAULT_SCAN_WINDOW;
    }
    if (!interval) {
        interval = BTM_DEFAULT_SCAN_INTERVAL;
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_3(TRACE_BTM_API, "BTM_SetDiscoverability: mode %d [NonDisc-0, Lim-1, Gen-2], window 0x%04x, interval 0x%04x",
                 inq_mode, window, interval);
    }

    if (inq_mode != BTM_NON_DISCOVERABLE) {
        if (window < HCI_MIN_SCAN_SLOTS || window > HCI_MAX_SCAN_SLOTS || interval < HCI_MIN_SCAN_SLOTS ||
            interval > HCI_MAX_SCAN_SLOTS || window > interval) {
            return BTM_ILLEGAL_VALUE;
        }
    }

    if (inq_mode != BTM_NON_DISCOVERABLE) {
        if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
            if (inq_mode & BTM_LIMITED_DISCOVERABLE) {
                memcpy(temp_lap[0], limited_inq_lap, 3);
                memcpy(temp_lap[1], general_inq_lap, 3);
                btsnd_hcic_write_cur_iac_lap(p_buf, 2, temp_lap);
            } else {
                btsnd_hcic_write_cur_iac_lap(p_buf, 1, (LAP*)&general_inq_lap);
            }
            scan_mode |= HCI_INQUIRY_SCAN_ENABLED;
        } else {
            return BTM_NO_RESOURCES;
        }
    }

    if (window != btm_cb.btm_inq_vars.inq_scan_window || interval != btm_cb.btm_inq_vars.inq_scan_period) {
        if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
            btm_cb.btm_inq_vars.inq_scan_window = window;
            btm_cb.btm_inq_vars.inq_scan_period = interval;
            btsnd_hcic_write_inqscan_cfg(p_buf, interval, window);
        } else {
            return BTM_NO_RESOURCES;
        }
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        if (btm_cb.btm_inq_vars.connectable_mode) {
            scan_mode |= HCI_PAGE_SCAN_ENABLED;
        }
        btm_cb.btm_inq_vars.discoverable_mode = inq_mode;
        btsnd_hcic_write_scan_enable(p_buf, scan_mode);
    } else {
        return BTM_NO_RESOURCES;
    }

    p_cod = BTM_ReadDeviceClass();
    service_class = p_cod[0];
    service_class <<= 8;
    service_class += p_cod[1] & BTM_COD_SERVICE_CLASS_LO_B;
    is_limited = (inq_mode & BTM_LIMITED_DISCOVERABLE) ? TRUE : FALSE;
    cod_limited = (service_class & BTM_COD_SERVICE_LMTD_DISCOVER) ? TRUE : FALSE;

    if (is_limited ^ cod_limited) {
        minor = p_cod[2] & BTM_COD_MINOR_CLASS_MASK;
        major = p_cod[1] & BTM_COD_MAJOR_CLASS_MASK;
        if (is_limited) {
            service_class |= BTM_COD_SERVICE_LMTD_DISCOVER;
        } else {
            service_class &= ~BTM_COD_SERVICE_LMTD_DISCOVER;
        }

        cod[2] = minor;
        cod[1] = major + (service_class & BTM_COD_SERVICE_CLASS_LO_B);
        cod[0] = service_class >> 8;
        BTM_SetDeviceClass(cod);
    }

    return BTM_SUCCESS;
}

/* 0x80484BE0 (0xB0): Sets the inquiry scan type (standard or interlaced) when the controller supports it. */
u8 BTM_SetInquiryScanType(u16 scan_type) {
    BT_HDR* p_buf;

    if (scan_type != BTM_SCAN_TYPE_STANDARD && scan_type != BTM_SCAN_TYPE_INTERLACED) {
        return BTM_ILLEGAL_VALUE;
    }

    if (!HCI_LMP_INTERLACED_INQ_SCAN_SUPPORTED(btm_cb.local_features)) {
        return BTM_MODE_UNSUPPORTED;
    }

    if (scan_type != btm_cb.btm_inq_vars.inq_scan_type) {
        if (BTM_IsDeviceUp()) {
            if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
                btsnd_hcic_write_inqscan_type(p_buf, (u8)scan_type);
                btm_cb.btm_inq_vars.inq_scan_type = scan_type;
            } else {
                return BTM_NO_RESOURCES;
            }
        } else {
            return BTM_WRONG_MODE;
        }
    }

    return BTM_SUCCESS;
}

/* 0x80484C90 (0xB0): Sets the page scan type (standard or interlaced) when the controller supports it. */
u8 BTM_SetPageScanType(u16 scan_type) {
    BT_HDR* p_buf;

    if (scan_type != BTM_SCAN_TYPE_STANDARD && scan_type != BTM_SCAN_TYPE_INTERLACED) {
        return BTM_ILLEGAL_VALUE;
    }

    if (!HCI_LMP_INTERLACED_PAGE_SCAN_SUPPORTED(btm_cb.local_features)) {
        return BTM_MODE_UNSUPPORTED;
    }

    if (scan_type != btm_cb.btm_inq_vars.page_scan_type) {
        if (BTM_IsDeviceUp()) {
            if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
                btsnd_hcic_write_pagescan_type(p_buf, (u8)scan_type);
                btm_cb.btm_inq_vars.page_scan_type = scan_type;
            } else {
                return BTM_NO_RESOURCES;
            }
        } else {
            return BTM_WRONG_MODE;
        }
    }

    return BTM_SUCCESS;
}

/* 0x80484D40 (0x98): Sets the inquiry result mode (standard or with RSSI) when the controller supports it. */
u8 BTM_SetInquiryMode(u8 mode) {
    BT_HDR* p_buf;

    if (mode != BTM_INQ_RESULT_STANDARD && mode != 1) {
        return BTM_ILLEGAL_VALUE;
    }

    if (!HCI_LMP_INQ_RSSI_SUPPORTED(btm_cb.local_features)) {
        return BTM_MODE_UNSUPPORTED;
    }

    if (!BTM_IsDeviceUp()) {
        return BTM_WRONG_MODE;
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        btsnd_hcic_write_inquiry_mode(p_buf, mode);
    } else {
        return BTM_NO_RESOURCES;
    }

    return BTM_SUCCESS;
}

/* 0x80484DD8 (0x188): Sets the page scan (connectable) mode with its window and interval. */
u8 BTM_SetConnectability(u16 page_mode, u16 window, u16 interval) {
    u8 scan_mode = 0;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    BT_HDR* p_buf;

    if (page_mode != BTM_NON_CONNECTABLE && page_mode != BTM_CONNECTABLE) {
        return BTM_ILLEGAL_VALUE;
    }

    if (btm_cb.devcb_state < BTM_DEV_STATE_SCAN_READY) {
        return BTM_DEV_RESET;
    }

    if (!window) {
        window = BTM_DEFAULT_SCAN_WINDOW;
    }
    if (!interval) {
        interval = BTM_DEFAULT_SCAN_INTERVAL;
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_3(TRACE_BTM_API, "BTM_SetConnectability: mode %d [NonConn-0, Conn-1], window 0x%04x, interval 0x%04x",
                 page_mode, window, interval);
    }

    if (page_mode == BTM_CONNECTABLE) {
        if (window < HCI_MIN_SCAN_SLOTS || window > HCI_MAX_SCAN_SLOTS || interval < HCI_MIN_SCAN_SLOTS ||
            interval > HCI_MAX_SCAN_SLOTS || window > interval) {
            return BTM_ILLEGAL_VALUE;
        }
        scan_mode |= HCI_PAGE_SCAN_ENABLED;
    }

    if (window != p_inq->page_scan_window || interval != p_inq->page_scan_period) {
        if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
            p_inq->page_scan_window = window;
            p_inq->page_scan_period = interval;
            btsnd_hcic_write_pagescan_cfg(p_buf, interval, window);
        } else {
            return BTM_NO_RESOURCES;
        }
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        if (p_inq->discoverable_mode) {
            scan_mode |= HCI_INQUIRY_SCAN_ENABLED;
        }
        p_inq->connectable_mode = page_mode;
        btsnd_hcic_write_scan_enable(p_buf, scan_mode);
    } else {
        return BTM_NO_RESOURCES;
    }

    return BTM_SUCCESS;
}

/* 0x80484F60 (0x10): The inquiry activity flags (0 when no inquiry runs). */
u16 BTM_IsInquiryActive(void) {
    return btm_cb.btm_inq_vars.inq_active;
}

/* Frees the duplicate filter of the current inquiry. */
static void btm_clr_inq_result_flt(void) {
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;

    if (p_inq->p_bd_db != NULL) {
        GKI_freebuf(p_inq->p_bd_db);
        p_inq->p_bd_db = NULL;
    }
    p_inq->num_bd_entries = 0;
    p_inq->max_bd_entries = 0;
}

/* 0x80484F70 (0x108): Cancels a running (non-periodic) inquiry. */
u8 BTM_CancelInquiry(void) {
    u8 status = BTM_SUCCESS;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_0(TRACE_BTM_API, "BTM_CancelInquiry called");
    }

    if (!BTM_IsDeviceUp()) {
        return BTM_WRONG_MODE;
    }

    if (p_inq->inq_active && !(p_inq->inq_active & BTM_PERIODIC_INQUIRY_ACTIVE)) {
        p_inq->inq_active = BTM_INQUIRY_INACTIVE;
        p_inq->state = BTM_INQ_INACTIVE_STATE;
        p_inq->p_inq_results_cb = NULL;
        p_inq->p_inq_cmpl_cb = NULL;

        if (p_inq->inqfilt_active) {
            p_inq->inqfilt_active = FALSE;
            p_inq->pending_filt_complete_event++;
        } else if (!btsnd_hcic_inq_cancel()) {
            status = BTM_NO_RESOURCES;
        }

        p_inq->inq_counter++;
        btm_clr_inq_result_flt();
    }

    return status;
}

/* 0x80485078 (0x1B4): Starts an inquiry: saves the parameters and first sets (or clears) the inquiry event filter;
 * the filter's completion starts the inquiry itself. */
u8 BTM_StartInquiry(tBTM_INQ_PARMS* p_inqparms, tBTM_INQ_RESULTS_CB* p_results_cb, BtmCompleteCallback p_cmpl_cb) {
    u8 status;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_4(TRACE_BTM_API, "BTM_StartInquiry: mode: %d, dur: %d, rsps: %d, flt: %d", p_inqparms->mode,
                 p_inqparms->duration, p_inqparms->max_resps, p_inqparms->filter_cond_type);
    }

    if (p_inq->inq_active || p_inq->inqfilt_active) {
        return BTM_BUSY;
    }

    if (p_inqparms->mode != BTM_GENERAL_INQUIRY && p_inqparms->mode != BTM_LIMITED_INQUIRY) {
        return BTM_ILLEGAL_VALUE;
    }

    if (!BTM_IsDeviceUp()) {
        return BTM_WRONG_MODE;
    }

    p_inq->inqparms = *p_inqparms;
    p_inq->state = BTM_INQ_ACTIVE_STATE;
    p_inq->p_inq_cmpl_cb = p_cmpl_cb;
    p_inq->p_inq_results_cb = p_results_cb;
    p_inq->inq_cmpl_info.num_resp = 0;
    p_inq->inq_active =
        (p_inqparms->mode == BTM_LIMITED_INQUIRY) ? BTM_LIMITED_INQUIRY_ACTIVE : BTM_GENERAL_INQUIRY_ACTIVE;

    switch (p_inqparms->filter_cond_type) {
    case BTM_CLR_INQUIRY_FILTER:
        p_inq->state = BTM_INQ_SET_FILT_STATE;
        break;

    case BTM_FILTER_COND_DEVICE_CLASS:
    case BTM_FILTER_COND_BD_ADDR:
        p_inq->state = BTM_INQ_CLR_FILT_STATE;
        p_inqparms->filter_cond_type = BTM_CLR_INQUIRY_FILTER;
        break;

    default:
        return BTM_ILLEGAL_VALUE;
    }

    if ((status = btm_set_inq_event_filter(p_inqparms->filter_cond_type, &p_inqparms->filter_cond)) !=
        BTM_CMD_STARTED) {
        p_inq->state = BTM_INQ_INACTIVE_STATE;
    }

    return status;
}

/* Returns the inquiry database entry of the device at `p_bda`, or NULL. */
static tINQ_DB_ENT* btm_inq_db_find(u8* p_bda) {
    u16 xx;
    tINQ_DB_ENT* p_ent = btm_cb.btm_inq_vars.inq_db;

    for (xx = 0; xx < BTM_INQ_DB_SIZE; xx++, p_ent++) {
        if (p_ent->in_use && !memcmp(p_ent->inq_info.remote_bd_addr, p_bda, 6)) {
            return p_ent;
        }
    }
    return NULL;
}

/* 0x8048522C (0xE8): Asks for the name of the device at `remote_bda`, paging it with the database's clock offset when
 * it is known; `p_cb` receives the name. */
u8 BTM_ReadRemoteDeviceName(u8* remote_bda, tBTM_NAME_CMPL_CB p_cb) {
    tBTM_INQ_INFO* p_cur = NULL;
    tINQ_DB_ENT* p_i;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_6(TRACE_BTM_API, "BTM_ReadRemoteDeviceName: bd addr [%02x%02x%02x%02x%02x%02x]", remote_bda[0],
                 remote_bda[1], remote_bda[2], remote_bda[3], remote_bda[4], remote_bda[5]);
    }

    if ((p_i = btm_inq_db_find(remote_bda)) != NULL) {
        p_cur = &p_i->inq_info;
    }
    return btm_initiate_rem_name(remote_bda, p_cur, BTM_RMT_NAME_EXT, BTM_EXT_RMT_NAME_TIMEOUT, (BtmCompleteCallback)p_cb);
}

/* 0x80485314 (0x78): Cancels the application's remote name request. */
u8 BTM_CancelRemoteDeviceName(void) {
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_0(TRACE_BTM_API, "BTM_CancelRemoteDeviceName()");
    }

    if (p_inq->remname_active & BTM_RMT_NAME_EXT) {
        if (btsnd_hcic_rmt_name_req_cancel(p_inq->remname_bda)) {
            return BTM_CMD_STARTED;
        }
        return BTM_NO_RESOURCES;
    }
    return BTM_WRONG_MODE;
}

/* 0x8048538C (0xC0): The inquiry database entry of the device at `p_bda`, or NULL. */
tBTM_INQ_INFO* BTM_InqDbRead(u8* p_bda) {
    u16 xx;
    tINQ_DB_ENT* p_ent = btm_cb.btm_inq_vars.inq_db;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_6(TRACE_BTM_API, "BTM_InqDbRead: bd addr [%02x%02x%02x%02x%02x%02x]", p_bda[0], p_bda[1], p_bda[2],
                 p_bda[3], p_bda[4], p_bda[5]);
    }

    for (xx = 0; xx < BTM_INQ_DB_SIZE; xx++, p_ent++) {
        if (p_ent->in_use && !memcmp(p_ent->inq_info.remote_bd_addr, p_bda, 6)) {
            return &p_ent->inq_info;
        }
    }
    return NULL;
}

/* 0x8048544C (0xB8): The first used inquiry database entry, or NULL. */
tBTM_INQ_INFO* BTM_InqDbFirst(void) {
    u16 xx;
    tINQ_DB_ENT* p_ent = btm_cb.btm_inq_vars.inq_db;

    for (xx = 0; xx < BTM_INQ_DB_SIZE; xx++, p_ent++) {
        if (p_ent->in_use) {
            return &p_ent->inq_info;
        }
    }
    return NULL;
}

/* 0x80485504 (0x13C): The used inquiry database entry after `p_cur` (the first one for NULL), or NULL. */
tBTM_INQ_INFO* BTM_InqDbNext(tBTM_INQ_INFO* p_cur) {
    tINQ_DB_ENT* p_ent;
    u16 inx;

    if (p_cur != NULL) {
        p_ent = (tINQ_DB_ENT*)((u8*)p_cur - offsetof(tINQ_DB_ENT, inq_info));
        inx = (u16)((p_ent - btm_cb.btm_inq_vars.inq_db) + 1);

        for (p_ent = &btm_cb.btm_inq_vars.inq_db[inx]; inx < BTM_INQ_DB_SIZE; inx++, p_ent++) {
            if (p_ent->in_use) {
                return &p_ent->inq_info;
            }
        }
        return NULL;
    }
    return BTM_InqDbFirst();
}

/* Forgets the inquiry database entry of `p_bda` (every entry for NULL), telling the change callback. */
static void btm_clr_inq_db(u8* p_bda) {
    tINQ_DB_ENT* p_ent = btm_cb.btm_inq_vars.inq_db;
    u16 xx;

    for (xx = 0; xx < BTM_INQ_DB_SIZE; xx++, p_ent++) {
        if (p_ent->in_use) {
            if (p_bda == NULL || !memcmp(p_ent->inq_info.remote_bd_addr, p_bda, 6)) {
                p_ent->in_use = FALSE;
                if (btm_cb.btm_inq_vars.p_inq_change_cb != NULL) {
                    (*btm_cb.btm_inq_vars.p_inq_change_cb)(&p_ent->inq_info, FALSE);
                }
            }
        }
    }
}

/* 0x80485640 (0xD0): Clears the inquiry database entry of `p_bda` (every entry for NULL) unless an inquiry, a name
 * request or a filter update is running. */
u8 BTM_ClearInqDb(u8* p_bda) {
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;

    if (p_inq->inq_active != BTM_INQUIRY_INACTIVE || p_inq->remname_active || p_inq->inqfilt_active) {
        return BTM_BUSY;
    }

    btm_clr_inq_db(p_bda);
    return BTM_SUCCESS;
}

/* 0x80485710 (0x198): Resets the inquiry state after a controller reset, failing the pending inquiry, name request
 * and filter update to their callbacks. */
void btm_inq_db_reset(void) {
    tBTM_REMOTE_DEV_NAME rem_name;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    u8 num_responses;
    u8 temp_inq_active;
    u8 status;

    btu_stop_timer(&p_inq->inq_timer_ent);
    if (p_inq->inq_active != BTM_INQUIRY_INACTIVE) {
        temp_inq_active = p_inq->inq_active;
        p_inq->inq_active = BTM_INQUIRY_INACTIVE;

        if (temp_inq_active == BTM_LIMITED_INQUIRY_ACTIVE || temp_inq_active == BTM_GENERAL_INQUIRY_ACTIVE) {
            if (p_inq->p_inq_cmpl_cb != NULL) {
                num_responses = 0;
                (*p_inq->p_inq_cmpl_cb)(&num_responses);
            }
        }
    }

    if (p_inq->remname_active & BTM_RMT_NAME_EXT) {
        btu_stop_timer(&p_inq->rmt_name_timer_ent);
        p_inq->remname_active = FALSE;
        memset(p_inq->remname_bda, 0, 6);

        if (p_inq->p_remname_cmpl_cb != NULL) {
            rem_name.status = BTM_DEV_RESET;
            (*p_inq->p_remname_cmpl_cb)(&rem_name);
            p_inq->p_remname_cmpl_cb = NULL;
        }
    }

    if (p_inq->inqfilt_active) {
        p_inq->inqfilt_active = FALSE;

        if (p_inq->p_inqfilter_cmpl_cb != NULL) {
            status = BTM_DEV_RESET;
            (*p_inq->p_inqfilter_cmpl_cb)(&status);
        }
    }

    p_inq->state = BTM_INQ_INACTIVE_STATE;
    p_inq->pending_filt_complete_event = 0;
    p_inq->p_inq_results_cb = NULL;
    btm_clr_inq_db(NULL);
    btm_clr_inq_result_flt();

    p_inq->discoverable_mode = BTM_NON_DISCOVERABLE;
    p_inq->connectable_mode = BTM_NON_CONNECTABLE;
    p_inq->page_scan_type = BTM_SCAN_TYPE_STANDARD;
    p_inq->inq_scan_type = BTM_SCAN_TYPE_STANDARD;
}

/* 0x804858A8 (0x4): Initialises the inquiry database (btm_init already cleared it). */
void btm_inq_db_init(void) {
}

/* 0x804858AC (0xDC): Whether `p_bda` already answered the current inquiry; records it in the duplicate filter when
 * not. */
u8 btm_inq_find_bdaddr(u8* p_bda) {
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    u16 num_bd_entries;
    tINQ_BDADDR* p_db = &p_inq->p_bd_db[0];
    u16 xx;

    if ((p_inq->inq_active & BTM_PERIODIC_INQUIRY_ACTIVE) || p_db == NULL) {
        return FALSE;
    }

    num_bd_entries = p_inq->num_bd_entries;
    for (xx = 0; xx < num_bd_entries; xx++, p_db++) {
        if (!memcmp(p_db->bd_addr, p_bda, 6) && p_db->inq_count == p_inq->inq_counter) {
            return TRUE;
        }
    }

    if (xx < p_inq->max_bd_entries) {
        p_db->inq_count = p_inq->inq_counter;
        memcpy(p_db->bd_addr, p_bda, 6);
        p_inq->num_bd_entries++;
    }

    return FALSE;
}

/* 0x80485988 (0x104): Takes a free inquiry database entry for `p_bda`, or reuses the oldest one. */
tINQ_DB_ENT* btm_inq_db_new(u8* p_bda) {
    u16 xx;
    tINQ_DB_ENT* p_ent = btm_cb.btm_inq_vars.inq_db;
    tINQ_DB_ENT* p_old = btm_cb.btm_inq_vars.inq_db;
    u32 ot = 0xFFFFFFFF;

    for (xx = 0; xx < BTM_INQ_DB_SIZE; xx++, p_ent++) {
        if (!p_ent->in_use) {
            memset(p_ent, 0, sizeof(tINQ_DB_ENT));
            memcpy(p_ent->inq_info.remote_bd_addr, p_bda, 6);
            p_ent->in_use = TRUE;
            return p_ent;
        }

        if (p_ent->time_of_resp < ot) {
            p_old = p_ent;
            ot = p_ent->time_of_resp;
        }
    }

    if (btm_cb.btm_inq_vars.p_inq_change_cb != NULL) {
        (*btm_cb.btm_inq_vars.p_inq_change_cb)(&p_old->inq_info, FALSE);
    }

    memset(p_old, 0, sizeof(tINQ_DB_ENT));
    memcpy(p_old->inq_info.remote_bd_addr, p_bda, 6);
    p_old->in_use = TRUE;

    return p_old;
}

/* 0x80485A8C (0xE4): Sets (or clears) the controller's inquiry result filter for the condition `p_filt_cond`. */
u8 btm_set_inq_event_filter(u8 filter_cond_type, tBTM_INQ_FILT_COND* p_filt_cond) {
    u8 condition_buf[3 * 2];
    BT_HDR* p_buf;
    u8 condition_length = 3 * 2;
    u8* p_cond = condition_buf;

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        switch (filter_cond_type) {
        case BTM_FILTER_COND_DEVICE_CLASS:
            memcpy(condition_buf, p_filt_cond->cod_cond.dev_class, 3);
            memcpy(&condition_buf[3], p_filt_cond->cod_cond.dev_class_mask, 3);
            break;

        case BTM_FILTER_COND_BD_ADDR:
            p_cond = p_filt_cond->bdaddr_cond;
            break;

        case BTM_CLR_INQUIRY_FILTER:
            condition_length = 0;
            break;

        default:
            return BTM_ILLEGAL_VALUE;
        }

        btm_cb.btm_inq_vars.inqfilt_active = TRUE;

        btsnd_hcic_set_event_filter(p_buf, HCI_FILTER_INQUIRY_RESULT, filter_cond_type, p_cond, condition_length);
        return BTM_CMD_STARTED;
    } else {
        return BTM_NO_RESOURCES;
    }
}

/* Starts the inquiry (or periodic inquiry) the saved parameters describe. */
static void btm_initiate_inquiry(tBTM_INQUIRY_VAR_ST* p_inq) {
    const u8* lap;
    tBTM_INQ_PARMS* p_inqparms = &p_inq->inqparms;

    p_inqparms->max_resps = (u8)((p_inqparms->max_resps <= BTM_INQ_DB_SIZE) ? p_inqparms->max_resps : BTM_INQ_DB_SIZE);

    lap = (p_inq->inq_active & BTM_LIMITED_INQUIRY_ACTIVE) ? limited_inq_lap : general_inq_lap;

    if (p_inq->inq_active & BTM_PERIODIC_INQUIRY_ACTIVE) {
        if (!btsnd_hcic_per_inq_mode(p_inq->per_max_delay, p_inq->per_min_delay, lap, p_inqparms->duration,
                                     p_inqparms->max_resps)) {
            btm_process_inq_complete(BTM_NO_RESOURCES);
        }
    } else {
        btm_clr_inq_result_flt();

        if ((p_inq->p_bd_db = (tINQ_BDADDR*)GKI_getbuf(GKI_MAX_BUF_SIZE)) != NULL) {
            p_inq->max_bd_entries = (u16)(GKI_MAX_BUF_SIZE / sizeof(tINQ_BDADDR));
            memset(p_inq->p_bd_db, 0, GKI_MAX_BUF_SIZE);
        }

        if (!btsnd_hcic_inquiry(lap, p_inqparms->duration, 0)) {
            btm_process_inq_complete(BTM_NO_RESOURCES);
        }
    }
}

/* 0x80485B70 (0x220): Handles the completion of an inquiry event filter command: reports it to an external caller,
 * or moves the inquiry on to the next filter or the inquiry itself. */
void btm_event_filter_complete(u8* p) {
    u8 hci_status;
    u8 status;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    BtmCompleteCallback p_cb = p_inq->p_inqfilter_cmpl_cb;

    if (p_inq->pending_filt_complete_event) {
        p_inq->pending_filt_complete_event--;
        return;
    }

    if (p_inq->inqfilt_active == TRUE) {
        hci_status = *p;
        if (hci_status != HCI_SUCCESS) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
                LogMsg_1(TRACE_BTM_WARNING, "BTM Warning: Set Event Filter Failed (HCI returned 0x%x)", hci_status);
            }
            status = BTM_ERR_PROCESSING;
        } else {
            status = BTM_SUCCESS;
        }

        if (p_inq->state == BTM_INQ_INACTIVE_STATE) {
            p_inq->inqfilt_active = FALSE;
            if (p_cb != NULL) {
                (*p_cb)(&status);
            }
        } else {
            if (status != BTM_SUCCESS) {
                btm_process_inq_complete(BTM_ERR_PROCESSING);
                p_inq->inqfilt_active = FALSE;
                p_inq->inq_active = BTM_INQUIRY_INACTIVE;
                p_inq->state = BTM_INQ_INACTIVE_STATE;
                return;
            }

            if (p_inq->state == BTM_INQ_CLR_FILT_STATE) {
                if ((status = btm_set_inq_event_filter(p_inq->inqparms.filter_cond_type, &p_inq->inqparms.filter_cond)) ==
                    BTM_CMD_STARTED) {
                    p_inq->state = BTM_INQ_SET_FILT_STATE;
                } else {
                    p_inq->inqfilt_active = FALSE;
                    btm_process_inq_complete(BTM_ERR_PROCESSING);
                }
            } else {
                p_inq->state = BTM_INQ_ACTIVE_STATE;
                p_inq->inqfilt_active = FALSE;
                btm_initiate_inquiry(p_inq);
            }
        }
    }
}

/* 0x80485D90 (0x278): Handles an inquiry result event: records each new responder in the database and tells the
 * inquiry's result callback and the database change callback. */
void btm_process_inq_results(u8* p, u8 inq_res_mode) {
    u8 bda[6];
    u8 page_scan_mode = 0;
    u8 page_scan_rep_mode = 0;
    u8* pbda;
    int ijk;
    u8 dc[3];
    u8 xx;
    u8 rssi = 0;
    tINQ_DB_ENT* p_i;
    tBTM_INQ_INFO* p_cur;
    u8 page_scan_per_mode = 0;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    u8 is_new = TRUE;
    tBTM_INQ_RESULTS_CB* p_inq_results_cb = p_inq->p_inq_results_cb;
    u16 clock_offset;
    u8 num_resp;

    if (!p_inq->inq_active) {
        return;
    }

    num_resp = *p++;

    for (xx = 0; xx < num_resp; xx++) {
        pbda = bda + 6 - 1;
        for (ijk = 0; ijk < 6; ijk++) {
            *pbda-- = *p++;
        }
        page_scan_rep_mode = *p++;
        page_scan_per_mode = *p++;

        if (inq_res_mode == BTM_INQ_RESULT_STANDARD) {
            page_scan_mode = *p++;
        }

        pbda = dc + 3 - 1;
        for (ijk = 0; ijk < 3; ijk++) {
            *pbda-- = *p++;
        }
        clock_offset = p[0] + (p[1] << 8);
        p += 2;

        if (inq_res_mode != BTM_INQ_RESULT_STANDARD) {
            rssi = *p++;
        }

        if (btm_inq_find_bdaddr(bda)) {
            continue;
        }

        if (p_inq->p_inq_filter_cb != NULL && !(*p_inq->p_inq_filter_cb)(bda, dc)) {
            continue;
        }

        p_i = btm_inq_db_find(bda);

        if (p_i == NULL) {
            p_i = btm_inq_db_new(bda);
            is_new = TRUE;
        } else if (p_i->inq_count == p_inq->inq_counter) {
            is_new = FALSE;
        }

        if (is_new == TRUE) {
            p_cur = &p_i->inq_info;
            p_cur->page_scan_rep_mode = page_scan_rep_mode;
            p_cur->page_scan_per_mode = page_scan_per_mode;
            p_cur->page_scan_mode = page_scan_mode;
            p_cur->dev_class[0] = dc[0];
            p_cur->dev_class[1] = dc[1];
            p_cur->dev_class[2] = dc[2];
            p_cur->clock_offset = clock_offset | BTM_CLOCK_OFFSET_VALID;
            if (inq_res_mode != BTM_INQ_RESULT_STANDARD) {
                p_cur->rssi = rssi;
            } else {
                p_cur->rssi = BTM_INQ_RES_IGNORE_RSSI;
            }

            p_i->time_of_resp = GKI_get_tick_count();
            p_i->inq_count = p_inq->inq_counter;
            p_inq->inq_cmpl_info.num_resp++;

            if (!(p_inq->inq_active & BTM_PERIODIC_INQUIRY_ACTIVE) && p_inq->inqparms.max_resps &&
                p_inq->inq_cmpl_info.num_resp == p_inq->inqparms.max_resps) {
                btsnd_hcic_inq_cancel();
            }

            p_i->inq_info.appl_knows_rem_name = FALSE;

            if (p_inq_results_cb != NULL) {
                (p_inq_results_cb)(p_cur);
            }

            if (p_inq->p_inq_change_cb != NULL) {
                (*p_inq->p_inq_change_cb)(&p_i->inq_info, TRUE);
            }
        }
    }
}

/* 0x80486008 (0x104): Handles the inquiry complete event: records the status and, for a single inquiry, ends it and
 * calls the completion callback. */
void btm_process_inq_complete(u8 status) {
    BtmCompleteCallback p_inq_cb = btm_cb.btm_inq_vars.p_inq_cmpl_cb;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    u8 cmpl;

    if (p_inq->inq_active) {
        p_inq->inq_counter++;
        cmpl = BTM_ERR_PROCESSING;
        if (status == HCI_SUCCESS) {
            cmpl = BTM_SUCCESS;
        }
        p_inq->inq_cmpl_info.status = cmpl;

        if (!(p_inq->inq_active & BTM_PERIODIC_INQUIRY_ACTIVE)) {
            p_inq->p_inq_results_cb = NULL;
            p_inq->inq_active = BTM_INQUIRY_INACTIVE;
            p_inq->state = BTM_INQ_INACTIVE_STATE;
            p_inq->p_inq_cmpl_cb = NULL;

            btm_clr_inq_result_flt();

            if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
                LogMsg_2(TRACE_BTM_DEBUG, "BTM Inq Compl Callback: status 0x%02x, num results %d",
                         p_inq->inq_cmpl_info.status, p_inq->inq_cmpl_info.num_resp);
            }

            if (p_inq_cb != NULL) {
                (p_inq_cb)(&p_inq->inq_cmpl_info);
            }
        }
    }
}

/* 0x8048610C (0x150): Starts a remote name request for `origin` (the application or the security manager), or joins
 * the one already running for the same device. */
u8 btm_initiate_rem_name(u8* remote_bda, tBTM_INQ_INFO* p_cur, u8 origin, u32 timeout, BtmCompleteCallback p_cb) {
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    u8 send = TRUE;
    u8 cmd_ok = TRUE;
    u8 status = BTM_CMD_STARTED;

    if (!BTM_IsDeviceUp()) {
        return BTM_WRONG_MODE;
    }

    if (!(p_inq->remname_active & origin)) {
        if (origin == BTM_RMT_NAME_EXT) {
            if (p_inq->remname_active) {
                if (memcmp(remote_bda, p_inq->remname_bda, 6) == 0) {
                    send = FALSE;
                } else {
                    return BTM_BUSY;
                }
            }
            p_inq->p_remname_cmpl_cb = p_cb;
        } else if (p_inq->remname_active) {
            return BTM_BUSY;
        }

        if (send) {
            memcpy(p_inq->remname_bda, remote_bda, 6);
            btu_start_timer(&p_inq->rmt_name_timer_ent, BTU_TTYPE_BTM_RMT_NAME, timeout);

            if (p_cur != NULL) {
                cmd_ok = btsnd_hcic_rmt_name_req(remote_bda, p_cur->page_scan_rep_mode, p_cur->page_scan_mode,
                                                 (u16)(p_cur->clock_offset | BTM_CLOCK_OFFSET_VALID));
            } else {
                cmd_ok = btsnd_hcic_rmt_name_req(remote_bda, HCI_PAGE_SCAN_REP_MODE_R1, HCI_MANDATARY_PAGE_SCAN_MODE, 0);
            }
        }

        if (cmd_ok) {
            p_inq->remname_active |= origin;
        } else {
            status = BTM_NO_RESOURCES;
        }
    } else {
        status = BTM_BUSY;
    }

    return status;
}

/* 0x8048625C (0x158): Handles the remote name result: stops the timer and hands the name (or the failure) to the
 * application's callback when it asked for it. */
void btm_process_remote_name(u8* bda, u8* bdn, u16 evt_len, u8 hci_status) {
    tBTM_REMOTE_DEV_NAME rem_name;
    tBTM_INQUIRY_VAR_ST* p_inq = &btm_cb.btm_inq_vars;
    BtmCompleteCallback p_cb = p_inq->p_remname_cmpl_cb;
    u8 origin = p_inq->remname_active;
    u8* p_n1;
    u16 temp_evt_len;

    btu_stop_timer(&p_inq->rmt_name_timer_ent);
    p_inq->remname_active = FALSE;

    if (hci_status == HCI_SUCCESS) {
        p_n1 = rem_name.remote_bd_name;
        rem_name.length = (evt_len < BD_NAME_LEN) ? evt_len : BD_NAME_LEN;
        rem_name.status = BTM_SUCCESS;
        temp_evt_len = rem_name.length;
        while (temp_evt_len > 0) {
            *p_n1++ = *bdn++;
            temp_evt_len--;
        }
    } else {
        rem_name.length = 0;
        rem_name.status = BTM_BAD_VALUE_RET;
        rem_name.remote_bd_name[0] = 0;
    }

    memset(p_inq->remname_bda, 0, 6);

    if (origin & BTM_RMT_NAME_EXT) {
        p_inq->p_remname_cmpl_cb = NULL;
        if (p_cb != NULL) {
            (p_cb)(&rem_name);
        }
    }
}

/* 0x804863B4 (0x40): Fails the pending remote name request (its timer expired) to the application and the security
 * manager. */
void btm_inq_rmt_name_failed(void) {
    btm_process_remote_name(NULL, NULL, 0, HCI_ERR_UNSPECIFIED);
    btm_sec_rmt_name_request_complete(NULL, NULL, HCI_ERR_UNSPECIFIED);
}
