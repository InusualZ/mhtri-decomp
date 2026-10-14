/*
 * BTE/btm_devctl.c - the BTM device control: device init and reset, the HCI reset sequence and its completions, AFH
 *   channels, local name/address/version/features/class, the device status notification and the vendor-specific
 *   commands and events.
 *
 * RANGE. .text 0x80482EA0-0x80484224 (24 functions, 0x1384 B); .data 0x80613F80-0x806140E0; .sdata
 *   0x80793D88-0x80793D8C. The start is medium: btm_dev_init after btm_dev.c in the BTE source order; the .data start
 *   0x80613F80 is 8-aligned with no pad (first readers BTM_SetAfhChannels and btm_read_local_features_complete). The
 *   end is strong: the next .data object starts after a 4-byte pad beyond 4-byte alignment (0x806140DC ->
 *   0x806140E0), and its first reader BTM_WritePageTimeout is the first function after btm_vendor_specific_evt, the
 *   last reader of this range's .data - the cut is pinned exactly at 0x80484224. The .sdata word 0x80793D88 is the
 *   default class literal btm_dev_init copies.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as the matched BTE units (see BTE/l2c_utils.c); the static reset-sequence
 *   steps and BTM_SetDeviceClass/BTM_SetLocalDeviceName inline into their callers (`-ipa file`).
 * NAMES. the map's, which are the BTE source's; 0x80483FB4 BTM_ReadDeviceClass is a GUESS (returns the address of the
 *   btm_cb device class, at its BTE source position).
 * RESIDUALS. btm_read_local_features_complete: the target keeps the control block pointer in r31 and the string base
 *   in r30, this source the other way round (every order of the six declarations, a source without p_devcb and one
 *   with p_devcb everywhere measured the same or worse). BTM_SetLocalDeviceName: p_buf and the name pointer take
 *   r31/r30 swapped; the other declaration order fixes it and costs its two inlined copies (btm_reset_complete,
 *   btm_dev_timeout).
 * SHAPES. plain C; the device control block is flattened into btm_cb (BTE/btm_main.h). Each reset-sequence step
 *   decrements the retry count itself, so a caller that starts a step stores one more than the target's folded value.
 *   btm_vsc_complete copies `p` and the param_buf address into locals before the callback test (retail's mr r7 and
 *   hoisted addi).
 */

#include "BTE/btm_devctl.h"
#include "BTE/btm_main.h"
#include "BTE/l2c_link.h"
#include "BTE/l2c_utils.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The GKI pool the HCI commands are built in. */
#define HCI_CMD_POOL_ID 2

/* The BTU timer type of the device control timers and their timeouts in seconds. */
#define BTU_TTYPE_BTM_DEV_CTL 1
#define BTM_DEV_RESET_TIMEOUT 4
#define BTM_DEV_REPLY_TIMEOUT 1

/* The tries of the reset and of each later step of the reset sequence. */
#define BTM_DEV_RESET_RETRIES 5
#define BTM_DEV_REPLY_RETRIES 5

/* The default page timeout (slots), and the ACL and SCO packet types before the controller reports its own. */
#define BTM_DEFAULT_PAGE_TIMEOUT 0x1400
#define BTM_ACL_PKT_TYPES_DEFAULT 0xCC18
#define BTM_SCO_PKT_TYPES_DEFAULT 0x003F

/* The class of device the stack starts with. */
#define BTM_INIT_CLASS_OF_DEVICE "\x00\x1F\x00"

/* The host buffer sizes the stack reports to the controller. */
#define BTM_HOST_ACL_BUF_SIZE 0x69B
#define BTM_HOST_SCO_BUF_SIZE 30
#define BTM_HOST_ACL_BUF_NUM 20
#define BTM_HOST_SCO_BUF_NUM 10

/* The length of an ACL data packet header. */
#define HCI_DATA_PREAMBLE_SIZE 4

/* The HCI version that defines the EDR packet type bits. */
#define HCI_PROTO_VERSION_2_0 3

/* The default link policy bits. */
#define HCI_ENABLE_MASTER_SLAVE_SWITCH 0x0001
#define HCI_ENABLE_HOLD_MODE 0x0002
#define HCI_ENABLE_SNIFF_MODE 0x0004
#define HCI_ENABLE_PARK_MODE 0x0008

/* The ACL packet type bits. */
#define BTM_ACL_PKT_TYPES_MASK_DM1 0x0008
#define BTM_ACL_PKT_TYPES_MASK_DH1 0x0010
#define BTM_ACL_PKT_TYPES_MASK_DM3 0x0400
#define BTM_ACL_PKT_TYPES_MASK_DH3 0x0800
#define BTM_ACL_PKT_TYPES_MASK_DM5 0x4000
#define BTM_ACL_PKT_TYPES_MASK_DH5 0x8000
#define BTM_ACL_PKT_TYPES_MASK_NO_2_DH1 0x0002
#define BTM_ACL_PKT_TYPES_MASK_NO_3_DH1 0x0004
#define BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 0x0100
#define BTM_ACL_PKT_TYPES_MASK_NO_3_DH3 0x0200
#define BTM_ACL_PKT_TYPES_MASK_NO_2_DH5 0x1000
#define BTM_ACL_PKT_TYPES_MASK_NO_3_DH5 0x2000

/* The SCO packet type bits. */
#define BTM_SCO_PKT_TYPES_MASK_HV1 0x0001
#define BTM_SCO_PKT_TYPES_MASK_HV2 0x0002
#define BTM_SCO_PKT_TYPES_MASK_HV3 0x0004
#define BTM_SCO_PKT_TYPES_MASK_EV3 0x0008
#define BTM_SCO_PKT_TYPES_MASK_EV4 0x0010
#define BTM_SCO_PKT_TYPES_MASK_EV5 0x0020
#define BTM_SCO_PKT_TYPES_MASK_NO_2_EV3 0x0040
#define BTM_SCO_PKT_TYPES_MASK_NO_3_EV3 0x0080
#define BTM_SCO_PKT_TYPES_MASK_NO_2_EV5 0x0100
#define BTM_SCO_PKT_TYPES_MASK_NO_3_EV5 0x0200
#define BTM_ESCO_LINK_ONLY_MASK 0x0038

/* The page and inquiry scan defaults a controller reset restores. */
#define HCI_DEF_PAGESCAN_WINDOW 0x12
#define HCI_DEF_PAGESCAN_INTERVAL 0x800
#define HCI_DEF_INQUIRYSCAN_WINDOW 0x12
#define HCI_DEF_INQUIRYSCAN_INTERVAL 0x800
#define HCI_DEF_SCAN_TYPE 0

/* The scan type and inquiry mode the stack switches to once the controller is up. */
#define BTM_SCAN_TYPE_INTERLACED 1
#define BTM_INQ_RESULT_WITH_RSSI 1

/* The manufacturer whose vendor events are expected. */
#define BTM_VSE_MANUFACTURER 18

/* The status codes these functions return. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3
#define BTM_MODE_UNSUPPORTED 4
#define BTM_ILLEGAL_VALUE 5
#define BTM_DEV_RESET 12

/* The size of a vendor specific command buffer beyond its parameters: the buffer header and the command preamble. */
#define BTM_VSC_BUF_OVERHEAD 11

/* Resets the controller: restarts the reset sequence and its timer. */
static void btm_dev_reset(void) {
    btm_cb.devcb_state = BTM_DEV_STATE_WAIT_RESET_CMPLT;
    btm_cb.retry_count = BTM_DEV_RESET_RETRIES;
    btu_start_timer(&btm_cb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_RESET_TIMEOUT);
    btsnd_hcic_reset();
}

/* Asks the controller for its buffer sizes, or resets it when the step ran out of tries. */
static void btm_get_hci_buf_size(void) {
    BT_HDR* p_buf;

    if (--btm_cb.retry_count == 0) {
        btm_dev_reset();
    } else {
        btu_start_timer(&btm_cb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
        if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
            btsnd_hcic_read_buffer_size(p_buf);
        }
    }
}

/* Asks the controller for its version and address, or resets it when the step ran out of tries. */
static void btm_get_local_version(void) {
    if (--btm_cb.retry_count == 0) {
        btm_dev_reset();
    } else {
        btu_start_timer(&btm_cb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
        btsnd_hcic_read_local_ver();
        btsnd_hcic_read_bd_addr();
        btm_pm_reset();
    }
}

/* Asks the controller for its features, or resets it when the step ran out of tries. */
static void btm_get_local_features(void) {
    if (--btm_cb.retry_count == 0) {
        btm_dev_reset();
    } else {
        btu_start_timer(&btm_cb.reset_timer, BTU_TTYPE_BTM_DEV_CTL, BTM_DEV_REPLY_TIMEOUT);
        btsnd_hcic_read_local_features();
    }
}

/* Starts the reset sequence after the reset completed and sends the stored class, name and PIN type. */
static void btm_after_reset_hold_complete(void) {
    btm_cb.devcb_state = BTM_DEV_STATE_1ST_HCI_CMD_SENT;
    btm_cb.retry_count = BTM_DEV_REPLY_RETRIES;
    btm_get_hci_buf_size();

    BTM_SetDeviceClass(btm_cb.dev_class);
    BTM_SetLocalDeviceName(btm_cb.bd_name);
    BTM_SetPinType(btm_cb.pin_type, btm_cb.pin_code, btm_cb.pin_code_len);
}

/* 0x80482EA0 (0xA0): Sets the device control defaults and resets the controller. */
void btm_dev_init(void) {
    memcpy(btm_cb.dev_class, BTM_INIT_CLASS_OF_DEVICE, 3);

    btm_cb.page_timeout = BTM_DEFAULT_PAGE_TIMEOUT;

    btm_cb.reset_timer.param = TT_DEV_RESET;
    btm_cb.rln_timer.param = TT_DEV_RLN;
    btm_cb.rlinkp_timer.param = TT_DEV_RLNKP;

    btm_cb.btm_acl_pkt_types_supported = BTM_ACL_PKT_TYPES_DEFAULT;
    btm_cb.btm_sco_pkt_types_supported = BTM_SCO_PKT_TYPES_DEFAULT;

    btm_cb.first_disabled_channel = 0xFF;
    btm_cb.last_disabled_channel = 0xFF;

    btm_dev_reset();
}

/* 0x80482F40 (0xB0): Clears the inquiry and discovery state and reports every pending device request as reset. */
void btm_db_reset(void) {
    BtmCompleteCallback p_cb;
    u8 status = BTM_DEV_RESET;

    btm_inq_db_reset();
    btm_discovery_db_reset();

    if (btm_cb.p_rln_cmpl_cb) {
        p_cb = btm_cb.p_rln_cmpl_cb;
        btm_cb.p_rln_cmpl_cb = NULL;
        if (p_cb) {
            (*p_cb)(NULL);
        }
    }

    if (btm_cb.p_rlinkp_cmpl_cb) {
        p_cb = btm_cb.p_rlinkp_cmpl_cb;
        btm_cb.p_rlinkp_cmpl_cb = NULL;
        if (p_cb) {
            (*p_cb)(&status);
        }
    }

    if (btm_cb.p_rssi_cmpl_cb) {
        p_cb = btm_cb.p_rssi_cmpl_cb;
        btm_cb.p_rssi_cmpl_cb = NULL;
        if (p_cb) {
            (*p_cb)(&status);
        }
    }
}

/* 0x80482FF0 (0x9C): Drops every link and pending request and resets the controller; `p_cb` reports the end of the
 * reset sequence. */
void BTM_DeviceReset(void (*p_cb)(void)) {
    if (!btm_cb.p_reset_cmpl_cb || btm_cb.p_reset_cmpl_cb == (BtmCompleteCallback)p_cb) {
        btm_acl_device_down();
        btm_db_reset();

        btm_cb.p_reset_cmpl_cb = (BtmCompleteCallback)p_cb;
        btm_dev_reset();

        if (btm_cb.p_dev_status_cb) {
            (*btm_cb.p_dev_status_cb)(BTM_DEV_STATUS_DOWN);
        }
    }
}

/* 0x8048308C (0xA8): Drops every link and pending request and resets the controller; `p_cb` reports the reset. */
void BTM_SendHciReset(BtmCompleteCallback p_cb) {
    if (!btm_cb.p_reset_cmpl_cb || btm_cb.p_reset_cmpl_cb == p_cb || btm_cb.p_hci_reset_cmpl_cb == p_cb) {
        btm_acl_device_down();
        btm_db_reset();

        btm_cb.p_hci_reset_cmpl_cb = p_cb;
        btm_dev_reset();

        if (btm_cb.p_dev_status_cb) {
            (*btm_cb.p_dev_status_cb)(BTM_DEV_STATUS_DOWN);
        }
    }
}

/* 0x80483134 (0x1C): Returns whether the reset sequence finished. */
u8 BTM_IsDeviceUp(void) {
    return btm_cb.devcb_state == BTM_DEV_STATE_READY;
}

/* 0x80483150 (0xF0): Disables the AFH channels `first` to `last` when the controller supports AFH. */
u8 BTM_SetAfhChannels(u8 first, u8 last) {
    BTM_TRACE_API4("BTM_SetAfhChannels first: %d (%d) last: %d (%d)", first, btm_cb.first_disabled_channel, last,
                   btm_cb.last_disabled_channel);

    if (!(btm_cb.local_features[5] & 0x08) && !(btm_cb.local_features[4] & 0x10) &&
        !(btm_cb.local_features[5] & 0x10)) {
        return BTM_MODE_UNSUPPORTED;
    }

    if ((btm_cb.first_disabled_channel != first) || (btm_cb.last_disabled_channel != last)) {
        if (btsnd_hcic_set_afh_channels(first, last)) {
            btm_cb.first_disabled_channel = first;
            btm_cb.last_disabled_channel = last;
        } else {
            return BTM_NO_RESOURCES;
        }
    }
    return BTM_SUCCESS;
}

/* 0x80483240 (0x298): Retries the step of the reset sequence that timed out, or fails the local name read. */
void btm_dev_timeout(TIMER_LIST_ENT* p_tle) {
    u32 timer_type = p_tle->param;
    BtmCompleteCallback p_cb;

    if (timer_type == TT_DEV_RESET) {
        switch (btm_cb.devcb_state) {
        case BTM_DEV_STATE_WAIT_RESET_CMPLT:
            btm_dev_reset();
            break;
        case BTM_DEV_STATE_WAIT_AFTER_RESET:
            btm_after_reset_hold_complete();
            break;
        case BTM_DEV_STATE_1ST_HCI_CMD_SENT:
            btm_get_hci_buf_size();
            break;
        case BTM_DEV_STATE_2ND_HCI_CMD_SENT:
            btm_get_local_version();
            break;
        }
    } else if (timer_type == TT_DEV_RLN) {
        p_cb = btm_cb.p_rln_cmpl_cb;
        btm_cb.p_rln_cmpl_cb = NULL;
        if (p_cb) {
            (*p_cb)(NULL);
        }
    }
}

/* 0x804834D8 (0x1C4): Reports a requested HCI reset, or continues the reset sequence. */
void btm_reset_complete(void) {
    BtmCompleteCallback p_cb;

    BTM_TRACE_EVENT0("btm_reset_complete");

    if ((p_cb = btm_cb.p_hci_reset_cmpl_cb) != NULL) {
        btm_cb.p_hci_reset_cmpl_cb = NULL;
        (*p_cb)(NULL);
    } else if (btm_cb.devcb_state == BTM_DEV_STATE_WAIT_RESET_CMPLT) {
        l2cu_device_reset();

        btm_cb.btm_inq_vars.inq_scan_window = HCI_DEF_INQUIRYSCAN_WINDOW;
        btm_cb.btm_inq_vars.inq_scan_period = HCI_DEF_INQUIRYSCAN_INTERVAL;
        btm_cb.btm_inq_vars.inq_scan_type = HCI_DEF_SCAN_TYPE;
        btm_cb.btm_inq_vars.page_scan_window = HCI_DEF_PAGESCAN_WINDOW;
        btm_cb.btm_inq_vars.page_scan_period = HCI_DEF_PAGESCAN_INTERVAL;
        btm_cb.btm_inq_vars.page_scan_type = HCI_DEF_SCAN_TYPE;

        btm_after_reset_hold_complete();
    }
}

/* 0x8048369C (0xC8): Takes the controller's ACL buffer size and count, sends the host's, and asks for the version. */
void btm_read_hci_buf_size_complete(u8* p, u16 evt_len) {
    u8 status;
    u8 lm_sco_buf_size;
    u16 lm_num_acl_bufs;

    STREAM_TO_UINT8(status, p);
    if (status == 0) {
        STREAM_TO_UINT16(btu_cb.hcit_acl_data_size, p);
        STREAM_TO_UINT8(lm_sco_buf_size, p);
        STREAM_TO_UINT16(lm_num_acl_bufs, p);

        btu_cb.hcit_acl_pkt_size = btu_cb.hcit_acl_data_size + HCI_DATA_PREAMBLE_SIZE;

        l2c_link_processs_num_bufs(lm_num_acl_bufs);

        btsnd_hcic_set_host_buf_size(BTM_HOST_ACL_BUF_SIZE, BTM_HOST_SCO_BUF_SIZE, BTM_HOST_ACL_BUF_NUM,
                                     BTM_HOST_SCO_BUF_NUM);
    }

    btm_cb.devcb_state = BTM_DEV_STATE_2ND_HCI_CMD_SENT;
    btm_cb.retry_count = BTM_DEV_REPLY_RETRIES;
    btm_get_local_version();
}

/* 0x80483764 (0xBC): Stores the controller's version and asks for its features. */
void btm_read_local_version_complete(u8* p, u16 evt_len) {
    BtmVersionInfo* p_vi = &btm_cb.local_version;
    u8 status;

    STREAM_TO_UINT8(status, p);
    if (status == 0) {
        STREAM_TO_UINT8(p_vi->hciVersion, p);
        STREAM_TO_UINT16(p_vi->hciRevision, p);
        STREAM_TO_UINT8(p_vi->lmpVersion, p);
        STREAM_TO_UINT16(p_vi->manufacturer, p);
        STREAM_TO_UINT16(p_vi->lmpSubversion, p);
    }

    btm_cb.devcb_state = BTM_DEV_STATE_3RD_HCI_CMD_SENT;
    btm_cb.retry_count = BTM_DEV_REPLY_RETRIES;
    btm_get_local_features();
}

/* 0x80483820 (0x4FC): Stores the controller's features, derives the supported packet types and link policy, and
 * reports the device up. */
void btm_read_local_features_complete(u8* p, u16 evt_len) {
    tBTM_CB* p_devcb;
    BtmCompleteCallback p_cb;
    u8 status;
    u16 xx;
    u8 last;
    u8 first;

    p_devcb = &btm_cb;
    p_cb = p_devcb->p_reset_cmpl_cb;
    p_devcb->p_reset_cmpl_cb = NULL;

    STREAM_TO_UINT8(status, p);
    if (status == 0) {
        p_devcb->devcb_state = BTM_DEV_STATE_READY;

        for (xx = 0; xx < 8; xx++) {
            STREAM_TO_UINT8(p_devcb->local_features[xx], p);
        }

        btm_cb.btm_acl_pkt_types_supported = BTM_ACL_PKT_TYPES_MASK_DH1 + BTM_ACL_PKT_TYPES_MASK_DM1;

        if (p_devcb->local_features[0] & 0x01) {
            btm_cb.btm_acl_pkt_types_supported |= BTM_ACL_PKT_TYPES_MASK_DH3 + BTM_ACL_PKT_TYPES_MASK_DM3;
        }

        if (p_devcb->local_features[0] & 0x02) {
            btm_cb.btm_acl_pkt_types_supported |= BTM_ACL_PKT_TYPES_MASK_DH5 + BTM_ACL_PKT_TYPES_MASK_DM5;
        }

        if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
            if (!(p_devcb->local_features[3] & 0x02)) {
                btm_cb.btm_acl_pkt_types_supported |= BTM_ACL_PKT_TYPES_MASK_NO_2_DH1 +
                                                      BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 +
                                                      BTM_ACL_PKT_TYPES_MASK_NO_2_DH5;
            }

            if (!(p_devcb->local_features[3] & 0x04)) {
                btm_cb.btm_acl_pkt_types_supported |= BTM_ACL_PKT_TYPES_MASK_NO_3_DH1 +
                                                      BTM_ACL_PKT_TYPES_MASK_NO_3_DH3 +
                                                      BTM_ACL_PKT_TYPES_MASK_NO_3_DH5;
            }

            if ((p_devcb->local_features[3] & 0x02) || (p_devcb->local_features[3] & 0x04)) {
                if (!(p_devcb->local_features[4] & 0x80)) {
                    btm_cb.btm_acl_pkt_types_supported |= BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 +
                                                          BTM_ACL_PKT_TYPES_MASK_NO_3_DH3;
                }
                if (!(p_devcb->local_features[5] & 0x01)) {
                    btm_cb.btm_acl_pkt_types_supported |= BTM_ACL_PKT_TYPES_MASK_NO_2_DH5 +
                                                          BTM_ACL_PKT_TYPES_MASK_NO_3_DH5;
                }
            }
        }

        BTM_TRACE_DEBUG1("Local supported ACL packet types: 0x%04x", btm_cb.btm_acl_pkt_types_supported);

        btm_cb.btm_sco_pkt_types_supported = 0;
        btm_cb.sco_cb.esco_supported = FALSE;

        if (p_devcb->local_features[1] & 0x08) {
            btm_cb.btm_sco_pkt_types_supported = BTM_SCO_PKT_TYPES_MASK_HV1;

            if (p_devcb->local_features[1] & 0x10) {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_HV2;
            }

            if (p_devcb->local_features[1] & 0x20) {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_HV3;
            }
        }

        if (p_devcb->local_features[3] & 0x80) {
            btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_EV3;
        }

        if (p_devcb->local_features[4] & 0x01) {
            btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_EV4;
        }

        if (p_devcb->local_features[4] & 0x02) {
            btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_EV5;
        }

        if (btm_cb.btm_sco_pkt_types_supported & BTM_ESCO_LINK_ONLY_MASK) {
            btm_cb.sco_cb.esco_supported = TRUE;

            if (p_devcb->local_features[5] & 0x20) {
                if (!(p_devcb->local_features[5] & 0x80)) {
                    btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_NO_2_EV5;
                }
            } else {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_NO_2_EV3 +
                                                      BTM_SCO_PKT_TYPES_MASK_NO_2_EV5;
            }

            if (p_devcb->local_features[5] & 0x40) {
                if (!(p_devcb->local_features[5] & 0x80)) {
                    btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_NO_3_EV5;
                }
            } else {
                btm_cb.btm_sco_pkt_types_supported |= BTM_SCO_PKT_TYPES_MASK_NO_3_EV3 +
                                                      BTM_SCO_PKT_TYPES_MASK_NO_3_EV5;
            }
        }

        BTM_TRACE_DEBUG1("Local supported SCO packet types: 0x%04x", btm_cb.btm_sco_pkt_types_supported);

        if (p_devcb->local_features[0] & 0x20) {
            btm_cb.btm_def_link_policy |= HCI_ENABLE_MASTER_SLAVE_SWITCH;
        } else {
            btm_cb.btm_def_link_policy &= ~HCI_ENABLE_MASTER_SLAVE_SWITCH;
        }

        if (p_devcb->local_features[0] & 0x40) {
            btm_cb.btm_def_link_policy |= HCI_ENABLE_HOLD_MODE;
        } else {
            btm_cb.btm_def_link_policy &= ~HCI_ENABLE_HOLD_MODE;
        }

        if (p_devcb->local_features[0] & 0x80) {
            btm_cb.btm_def_link_policy |= HCI_ENABLE_SNIFF_MODE;
        } else {
            btm_cb.btm_def_link_policy &= ~HCI_ENABLE_SNIFF_MODE;
        }

        if (p_devcb->local_features[1] & 0x01) {
            btm_cb.btm_def_link_policy |= HCI_ENABLE_PARK_MODE;
        } else {
            btm_cb.btm_def_link_policy &= ~HCI_ENABLE_PARK_MODE;
        }

        btm_sec_dev_reset();

        if (btm_cb.last_disabled_channel != 0xFF) {
            last = btm_cb.last_disabled_channel;
            first = btm_cb.first_disabled_channel;
            btm_cb.last_disabled_channel = 0xFF;
            btm_cb.first_disabled_channel = 0xFF;
            BTM_SetAfhChannels(first, last);
        }

        if (p_devcb->local_features[3] & 0x40) {
            BTM_SetInquiryMode(BTM_INQ_RESULT_WITH_RSSI);
        }

        BTM_SetPageScanType(BTM_SCAN_TYPE_INTERLACED);
        BTM_SetInquiryScanType(BTM_SCAN_TYPE_INTERLACED);

        if (btm_cb.p_dev_status_cb) {
            (*btm_cb.p_dev_status_cb)(BTM_DEV_STATUS_UP);
        }

        if (p_cb) {
            (*p_cb)(NULL);
        }
    }
}

/* 0x80483D1C (0xB8): Stores the local device name `p_name` and sends it to the controller. */
u8 BTM_SetLocalDeviceName(char* p_name) {
    u8* p;
    BT_HDR* p_buf;

    if (!p_name) {
        return BTM_ILLEGAL_VALUE;
    }

    if (btm_cb.devcb_state == BTM_DEV_STATE_WAIT_RESET_CMPLT || btm_cb.devcb_state == BTM_DEV_STATE_WAIT_AFTER_RESET) {
        return BTM_DEV_RESET;
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        p = (u8*)btm_cb.bd_name;
        if (p != (u8*)p_name) {
            memset(btm_cb.bd_name, 0, sizeof(btm_cb.bd_name));
            strncpy(btm_cb.bd_name, p_name, BTM_MAX_LOC_BD_NAME_LEN);
        }

        btsnd_hcic_change_name(p_buf, p);
        return BTM_CMD_STARTED;
    }
    return BTM_NO_RESOURCES;
}

/* 0x80483DD4 (0x8C): Hands the local name read result (or NULL on failure) to the waiting callback. */
void btm_read_local_name_complete(u8* p, u16 evt_len) {
    BtmCompleteCallback p_cb = btm_cb.p_rln_cmpl_cb;
    u8 status;

    btu_stop_timer(&btm_cb.rln_timer);

    btm_cb.p_rln_cmpl_cb = NULL;

    if (p_cb) {
        STREAM_TO_UINT8(status, p);

        if (status == 0) {
            (*p_cb)(p);
        } else {
            (*p_cb)(NULL);
        }
    }
}

/* 0x80483E60 (0x40): Hands the local address to `p_cb`. */
u8 BTM_ReadLocalDeviceAddr(tBTM_LOCAL_ADDR_CB p_cb) {
    if (p_cb) {
        (*p_cb)(btm_cb.local_addr);
    }
    return BTM_SUCCESS;
}

/* 0x80483EA0 (0x48): Stores the local address the controller returned. */
void btm_read_local_addr_complete(u8* p, u16 evt_len) {
    u8 status;

    STREAM_TO_UINT8(status, p);

    if (status == 0) {
        STREAM_TO_BDADDR(btm_cb.local_addr, p);
    }
}

/* 0x80483EE8 (0x4C): Copies the controller's version to `p_vers` once the sequence read it. */
u8 BTM_ReadLocalVersion(BtmVersionInfo* p_vers) {
    if (btm_cb.devcb_state < BTM_DEV_STATE_3RD_HCI_CMD_SENT) {
        return BTM_DEV_RESET;
    }

    *p_vers = btm_cb.local_version;

    return BTM_SUCCESS;
}

/* 0x80483F34 (0x80): Stores the local device class and sends it to the controller. */
u8 BTM_SetDeviceClass(const u8* dev_class) {
    BT_HDR* p_buf;

    memcpy(btm_cb.dev_class, dev_class, 3);

    if (btm_cb.devcb_state == BTM_DEV_STATE_WAIT_RESET_CMPLT || btm_cb.devcb_state == BTM_DEV_STATE_WAIT_AFTER_RESET) {
        return BTM_DEV_RESET;
    }

    if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
        btsnd_hcic_write_dev_class(p_buf, (u8*)dev_class);
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* 0x80483FB4 (0x10): Returns the local device class. */
u8* BTM_ReadDeviceClass(void) {
    return btm_cb.dev_class;
}

/* 0x80483FC4 (0x10): Returns the controller's local feature bytes. */
u8* BTM_ReadLocalFeatures(void) {
    return btm_cb.local_features;
}

/* 0x80483FD4 (0x18): Installs the device status callback and returns the previous one. */
BtmDevStatusCallback BTM_RegisterForDeviceStatusNotif(BtmDevStatusCallback p_cb) {
    BtmDevStatusCallback p_prev = btm_cb.p_dev_status_cb;

    btm_cb.p_dev_status_cb = p_cb;
    return p_prev;
}

/* 0x80483FEC (0xFC): Sends the vendor specific command `opcode` with `param_len` parameter bytes; `p_cb` gets its
 * result. */
u8 BTM_VendorSpecificCommand(u16 opcode, u8 param_len, const u8* p_param_buf, void (*p_cb)(s32 result)) {
    BT_HDR* p_buf;

    BTM_TRACE_EVENT2("BTM: BTM_VendorSpecificCommand: Opcode: 0x%04X, ParamLen: %i.", opcode, param_len);

    if (btm_cb.vsc_busy) {
        BTM_TRACE_EVENT0("BTM: Unable to send vendor specific command (controller is busy).");
        return BTM_BUSY;
    }

    if ((p_buf = ((BT_HDR * (*)(u16)) GKI_getbuf)((u16)(param_len + BTM_VSC_BUF_OVERHEAD))) != NULL) {
        btm_cb.p_vsc_cmpl_cb = (BtmCompleteCallback)p_cb;
        if (p_cb) {
            btm_cb.vsc_busy = TRUE;
        }

        btsnd_hcic_vendor_spec_cmd(p_buf, opcode, param_len, (u8*)p_param_buf);

        if (p_cb != NULL) {
            return BTM_CMD_STARTED;
        }
        return BTM_SUCCESS;
    }
    return BTM_NO_RESOURCES;
}

/* 0x804840E8 (0x6C): Hands the result of a vendor specific command to its callback. */
void btm_vsc_complete(u8* p, u16 opcode, u16 evt_len) {
    BtmCompleteCallback p_cb = btm_cb.p_vsc_cmpl_cb;
    tBTM_VSC_CMPL vcs_cplt_params;
    u8* p_src = p;
    u8* p_dst = vcs_cplt_params.param_buf;

    btm_cb.vsc_busy = FALSE;
    btm_cb.p_vsc_cmpl_cb = NULL;

    if (p_cb) {
        vcs_cplt_params.opcode = opcode;
        vcs_cplt_params.param_len = evt_len;
        memcpy(p_dst, p_src, evt_len);
        (*p_cb)(&vcs_cplt_params);
    }
}

/* 0x80484154 (0x38): Installs (or with NULL clears) the vendor specific event callback; refuses a second one. */
u8 BTM_RegisterForVSEvents(BtmVsEventCallback p_cb) {
    if (btm_cb.p_vend_spec_cb != NULL && p_cb != NULL) {
        return BTM_BUSY;
    }

    btm_cb.p_vend_spec_cb = p_cb;
    return BTM_SUCCESS;
}

/* 0x8048418C (0x98): Clears a pending vendor specific command and hands the event to the registered callback. */
void btm_vendor_specific_evt(u8* p, u8 evt_len) {
    tBTM_CB* p_devcb = &btm_cb;

    if (p_devcb->local_version.manufacturer != BTM_VSE_MANUFACTURER) {
        BTM_TRACE_EVENT0("BTM Event: Received a vendor specific event from controller");
    }

    p_devcb->p_vsc_cmpl_cb = NULL;
    p_devcb->vsc_busy = FALSE;

    if (p_devcb->p_vend_spec_cb) {
        (*p_devcb->p_vend_spec_cb)(evt_len, p);
    }
}
