/*
 * BTE/bta_hh_utils.c - the BTA HID host utilities: control-block lookup, device clean-up, the device list and the
 *   trace helpers.
 *
 * RANGE. .text 0x804806E4-0x80480A90 (5 functions, 0x3AC B); .data 0x806138A0-0x80613B60; .sbss2
 *   0x8079D7E8-0x8079D7F0. Medium start: bta_hh_find_cb opens the bta_hh_* utility names after bta_hh_evt_code; the
 *   .data 0x806138A0 is read only by bta_hh_find_cb and bta_hh_trace_dev_db, and 0x80613924 only by
 *   bta_hh_add_device_to_list. Strong end: btm_acl_init opens BTE/btm_acl.c, whose .data starts at 0x80613B60. The
 *   .sbss2 all-zero address 0x8079D7E8 (bd_addr_null) is read only by bta_hh_find_cb. The .data between the
 *   add-device and the device-table strings holds the strings of the keyboard and mouse report parsers, which the
 *   retail link dead-strips.
 * FLAGS. the target is GC/3.0a5.2 + cflags_nw4r code, as BTE/bta_hh_main.c.
 * NAMES. the BTE source's. GUESS: bd_addr_null (.sbss2 0x8079D7E8); the parser names bta_hh_parse_keybd_rpt and
 *   bta_hh_parse_mice_rpt come from their own trace strings.
 * RESIDUALS. the string relocations name the compiler's pool where the map has lbl_806138A0/lbl_80613924.
 *   The parsers add .text and relocations the target object does not carry (the link must
 *   dead-strip them at a flip, unmeasured). The two report parsers are GUESS bodies built around their retail
 *   strings (no retail code survives the dead-strip).
 * SHAPES. plain C.
 */

#include "BTE/bta_hh_utils.h"
#include "BTE/bta_hh_main.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "BTE/bd.h"   /* bdcmp */

/* The all-zero device address. */
const u8 bd_addr_null[BD_ADDR_LEN] = {0, 0, 0, 0, 0, 0};

/* 0x804806E4 (0x13C): Returns the control-block index of the device at `bda`, taking a free block for a new one. */
u8 bta_hh_find_cb(u8* bda) {
    u8 xx;

    for (xx = 0; xx < BTA_HH_MAX_KNOWN; xx++) {
        if (!bdcmp(bda, bta_hh_cb.kdev[xx].addr) && bdcmp(bda, bd_addr_null) != 0) {
            APPL_TRACE_DEBUG2("found kdev_cb[%d] hid_handle = %d ", xx, bta_hh_cb.kdev[xx].hid_handle);
            return xx;
        } else {
            APPL_TRACE_DEBUG4("in_use ? [%d] kdev[%d].hid_handle = %d state = [%d]", bta_hh_cb.kdev[xx].in_use, xx,
                              bta_hh_cb.kdev[xx].hid_handle, bta_hh_cb.kdev[xx].state);
        }
    }

    for (xx = 0; xx < BTA_HH_MAX_KNOWN; xx++) {
        if (!bta_hh_cb.kdev[xx].in_use) {
            bdcpy(bta_hh_cb.kdev[xx].addr, bda);
            break;
        }
    }

    APPL_TRACE_DEBUG2("bta_hh_find_cb:: index = %d while max = %d", xx, BTA_HH_MAX_KNOWN);

    return xx;
}

/* 0x80480820 (0x80): Resets a device control block, keeping its index and dropping its handle mapping. */
void bta_hh_clean_up_kdev(tBTA_HH_DEV_CB* p_cb) {
    u8 index;

    if (p_cb->hid_handle != BTA_HH_INVALID_HANDLE) {
        bta_hh_cb.cb_index[p_cb->hid_handle] = BTA_HH_MAX_KNOWN;
    }

    index = p_cb->index;

    utl_freebuf((void**)&p_cb->dscp_info.descriptor);

    memset(p_cb, 0, sizeof(tBTA_HH_DEV_CB));

    p_cb->index = index;
    p_cb->state = BTA_HH_IDLE_ST;
    p_cb->hid_handle = BTA_HH_INVALID_HANDLE;
}

/* 0x804808A0 (0xBC): Records a registered device's handle, attributes and a copy of its report descriptor. */
void bta_hh_add_device_to_list(tBTA_HH_DEV_CB* p_cb, u8 handle, u16 attr_mask, tBTA_HH_DEV_DSCP_INFO* p_dscp_info,
                               u8 sub_class, u8 app_id) {
    APPL_TRACE_DEBUG1("subclass = 0x%2x", sub_class);

    p_cb->hid_handle = handle;
    p_cb->in_use = TRUE;
    p_cb->attr_mask = attr_mask;

    p_cb->sub_class = sub_class;
    p_cb->app_id = app_id;

    if (p_dscp_info) {
        utl_freebuf((void**)&p_cb->dscp_info.descriptor);
        if (p_dscp_info->length && (p_cb->dscp_info.descriptor = (u8*)GKI_getbuf(p_dscp_info->length)) != NULL) {
            p_cb->dscp_info.length = p_dscp_info->length;
            memcpy((u8*)p_cb->dscp_info.descriptor, p_dscp_info->descriptor, p_dscp_info->length);
        }
    }
}

/* 0x8048095C (0x54): Returns whether the device subclass is a configured type, storing that type's application id. */
u8 bta_hh_tod_spt(tBTA_HH_DEV_CB* p_cb, u8 sub_class) {
    u8 xx;
    u8 cod = (sub_class >> 2);

    for (xx = 0; xx < p_bta_hh_cfg->max_devt_spt; xx++) {
        if (cod == (u8)p_bta_hh_cfg->p_devt_list[xx].tod) {
            p_cb->app_id = p_bta_hh_cfg->p_devt_list[xx].app_id;
            return TRUE;
        }
    }
    return FALSE;
}

/* Parses a boot-mode keyboard report into its modifier byte and key codes (dead-stripped in retail). */
void bta_hh_parse_keybd_rpt(u8* p_data, u8* p_report, u16 report_len) {
    tBTA_HH_KB_CB* p_kb = &bta_hh_cb.kb_cb;
    u8 this_char;
    u8 ctl_shift;
    u16 xx;

    APPL_TRACE_DEBUG2("bta_hh_parse_keybd_rpt:  (report=%p, report_len=%d) called", p_report, report_len);

    if (report_len < 2) {
        return;
    }

    ctl_shift = *p_report++;
    report_len--;

    if (report_len > sizeof(p_kb->last_report)) {
        report_len = sizeof(p_kb->last_report);
    }

    if (ctl_shift & 0x44) {
        APPL_TRACE_DEBUG0("Alt key pressed");
        p_kb->mod_key[2] = TRUE;
    } else {
        APPL_TRACE_DEBUG0("Alt key not pressed");
        p_kb->mod_key[2] = FALSE;
    }

    memset(p_data, 0, sizeof(p_kb->last_report));

    for (xx = 0; xx < report_len; xx++) {
        this_char = p_report[xx];
        APPL_TRACE_DEBUG1("this_char = %02x", this_char);
        if (this_char == 0) {
            continue;
        }
        if (this_char < 4) {
            APPL_TRACE_DEBUG1("BTA_HhParseKeybdRpt:  Cannot interpret scan code                 0x%02x", this_char);
            continue;
        }
        p_data[xx] = this_char;
    }

    memcpy(p_kb->last_report, p_report, report_len);
}

/* Parses a boot-mode mouse report into its buttons and movement (dead-stripped in retail). */
void bta_hh_parse_mice_rpt(u8* p_data, u8* p_report, u16 report_len) {
    APPL_TRACE_DEBUG2("bta_hh_parse_mice_rpt:  bta_keybd_rpt_rcvd(report=%p,                 report_len=%d) called",
                      p_report, report_len);

    if (report_len < 3) {
        return;
    }

    p_data[0] = p_report[0];
    p_data[1] = p_report[1];
    p_data[2] = p_report[2];

    APPL_TRACE_DEBUG1("mice button: 0x%2x", p_data[0]);
    APPL_TRACE_DEBUG2("mice move: x = %d y = %d", p_data[1], p_data[2]);
}

/* 0x804809B0 (0xE0): Prints the device table. */
void bta_hh_trace_dev_db(void) {
    u8 xx;

    APPL_TRACE_DEBUG0("bta_hh_trace_dev_db:: Device DB list********************");

    for (xx = 0; xx < BTA_HH_MAX_DEVICE; xx++) {
        APPL_TRACE_DEBUG3("kdev[%d] in_use[%d]  handle[%d] ", xx, bta_hh_cb.kdev[xx].in_use,
                          bta_hh_cb.kdev[xx].hid_handle);

        APPL_TRACE_DEBUG4("\t\t\t attr_mask[%04x] state [%d] sub_class[%02x] index = %d", bta_hh_cb.kdev[xx].attr_mask,
                          bta_hh_cb.kdev[xx].state, bta_hh_cb.kdev[xx].sub_class, bta_hh_cb.kdev[xx].index);
    }
    APPL_TRACE_DEBUG0("*********************************************************");
}
