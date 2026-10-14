/*
 * BTE/btm_sec.c - the BTM security manager: registration, PIN type and security levels, PIN reply, authorization,
 *   bonding, the L2CAP/multiplexer access checks and the authentication, encryption, link key and PIN code events.
 *
 * RANGE. .text 0x80487F1C-0x8048AEE0 (31 functions, 0x2FC4 B); .data 0x80614730-0x80615140; .sdata2
 *   0x8079D0C8-0x8079D0D8. Strong start: the .data pad seam 0x80614730 (BTE/btm_sco.c's header) and BTM_SecRegister,
 *   its first reader. It ends with btm_read_trusted_mask; BTE/btu_hcif.c follows (its header pins that cut). Every
 *   .data object is read only by this range's functions, and the .sdata2 bytes 0x8079D0C8-0x8079D0CD only by
 *   btm_sec_mkey_comp_event. The 8-byte all-ones address at 0x8079D0D0 is read by bta_dm_disable, bta_dm_keep_acl and
 *   L2CA_SetIdleTimeoutByBdAddr, never by a btm function; its owner is a GUESS from the data order - it follows
 *   btm_sec's .sdata2 and precedes DVD/dvdFatal.c's, so any unit from here to BTE/sdp_utils.c gives the same order,
 *   and BTE/gki_time.c cannot keep it without a second .sdata2 run.
 * FLAGS. GC/3.0a5.2 + `cflags_nw4r`, as the btm band (BTE/btm_pm.c's FLAGS line).
 * NAMES. the map's. GUESS (dead-stripped in retail, so no row): BTM_SetSecurityMode and BTM_SetEncryption - their
 *   bodies are reconstructed from the trace strings the .data keeps and the BTE source; mwld strips them again.
 *   btm_sec_find_first_serv, btm_sec_find_next_serv, btm_sec_find_mx_serv, btm_sec_start_get_name,
 *   btm_sec_start_authentication, btm_sec_start_encryption, btm_sec_are_all_trusted, btm_restore_mode and the two
 *   callback helpers are static helpers the compiler inlines.
 * RESIDUALS. none in the code: the rows that read below 100 differ only in relocation names (the string pool and
 *   the .sdata2 address bytes pair as `@NNN` against the map's lbl_ rows). btm_sec_pin_code_request_timeout,
 *   btm_sec_execute_procedure, btm_sec_start_authorization and btm_sec_collision_timeout are static here (global
 *   rows in the map; no other unit names them). The two GUESS bodies
 *   add .text the target does not keep (mwld strips them).
 * SHAPES. plain C; BT_BD_ANY is defined after the functions so the .sdata2 keeps the mkey event's local address
 *   initializer first; the outgoing-RFCOMM callback release is a static helper except in BTM_DeviceAuthorized (which
 *   passes its own bd_addr), btm_sec_mx_access_request, btm_sec_abort_access_req and the collision timeout (which
 *   reloads btm_cb.p_collided_dev_rec each time); btm_sec_pin_code_request and its timeout reach most
 *   fields through a `tBTM_CB*` local, as their register use shows.
 */

#include "BTE/btm_sec.h"
#include "BTE/btm_main.h"
#include "BTE/btm_acl.h"
#include "BTE/btm_dev.h"
#include "BTE/btm_devctl.h"
#include "BTE/btm_inq.h"
#include "BTE/hcicmds.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_link.h"
#include "BTE/l2c_main.h"
#include "BTE/l2c_utils.h"
#include "MSL_C/alloc.h"
#include "MSL_C/strstr.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTM status codes (BTM_SUCCESS and BTM_CMD_STARTED are BTE/l2c_int.h's). */
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3
#define BTM_MODE_UNSUPPORTED 4
#define BTM_ILLEGAL_VALUE 5
#define BTM_WRONG_MODE 6
#define BTM_UNKNOWN_ADDR 7
#define BTM_DEVICE_TIMEOUT 8
#define BTM_ERR_PROCESSING 10

/* The security modes. */
#define BTM_SEC_MODE_SERVICE 2
#define BTM_SEC_MODE_LINK 3

/* The security flags of a device record. */
#define BTM_SEC_AUTHORIZED 0x01
#define BTM_SEC_AUTHENTICATED 0x02
#define BTM_SEC_ENCRYPTED 0x04
#define BTM_SEC_NAME_KNOWN 0x08
#define BTM_SEC_LINK_KEY_KNOWN 0x10
#define BTM_SEC_IN_USE 0x80

/* The security a service (or the current check) requires. */
#define BTM_SEC_IN_AUTHORIZE 0x01
#define BTM_SEC_IN_AUTHENTICATE 0x02
#define BTM_SEC_IN_ENCRYPT 0x04
#define BTM_SEC_OUT_AUTHORIZE 0x08
#define BTM_SEC_OUT_AUTHENTICATE 0x10
#define BTM_SEC_OUT_ENCRYPT 0x20
#define BTM_SEC_BOND 0x40
#define BTM_SEC_PROCEDURE_MASK 0xC0

/* The security procedure states of a device record. */
#define BTM_SEC_STATE_IDLE 0
#define BTM_SEC_STATE_AUTHENTICATING 1
#define BTM_SEC_STATE_ENCRYPTING 2
#define BTM_SEC_STATE_GETTING_NAME 3
#define BTM_SEC_STATE_AUTHORIZING 4
#define BTM_SEC_STATE_DISCONNECTING 5

/* The invalid HCI handle, the RFCOMM multiplexer id, the trusted-service bitmap and the service and name sizes. */
#define BTM_SEC_INVALID_HANDLE 0xFFFF
#define BTM_SEC_PROTO_RFCOMM 3
#define BTM_SEC_TRUST_ALL 0xFFFFFFFF
#define BTM_SEC_MAX_SERVICES 46
#define BTM_SEC_ARRAY_BITS 32
#define BTM_SEC_IS_SERVICE_TRUSTED(p, service)                                                                            (((p)[((u32)(service)) / BTM_SEC_ARRAY_BITS]) & (u32)(((u32)1 << (((u32)(service)) % BTM_SEC_ARRAY_BITS))))
#define BTM_SEC_SERVICE_NAME_LEN 21
#define BTM_NO_AVAIL_SEC_SERVICES 0xFFFF
#define BTM_RMT_NAME_SEC 0x02
#define BTM_SEC_RMT_NAME_TIMEOUT 30
#define BTM_SEC_PIN_TIMEOUT 60
#define BTU_TTYPE_USER_FUNC 0x16
#define BT_1SEC_TIMEOUT 1

/* The PIN types, the encryption modes, the HCI status codes and the device class of a headset that needs a fixed
 * PIN. */
#define HCI_PIN_TYPE_FIXED 1
#define HCI_ENCRYPT_MODE_DISABLED 0
#define HCI_ENCRYPT_MODE_POINT_TO_POINT 1
#define HCI_VERSION_1_1 1
#define HCI_SUCCESS 0
#define HCI_ERR_PAGE_TIMEOUT_STATUS 0x04
#define HCI_ERR_AUTH_FAILURE 0x05
#define HCI_ERR_KEY_MISSING 0x06
#define HCI_ERR_CONNECTION_TOUT 0x08
#define HCI_ERR_COMMAND_DISALLOWED 0x0C
#define HCI_ERR_HOST_REJECT_SECURITY 0x0E
#define HCI_ERR_REPEATED_ATTEMPTS 0x17
#define HCI_ERR_PAIRING_NOT_ALLOWED 0x18
#define HCI_ERR_INSUFFCIENT_SECURITY 0x22
#define HCI_ERR_LMP_ERR_TRANS_COLLISION 0x23
#define HCI_ERR_ENCRY_MODE_NOT_ACCEPTABLE 0x25
#define HCI_ERR_UNIT_KEY_USED 0x26
#define HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED 0x29
#define HCI_PAGE_SCAN_REP_MODE_R1 1
#define HCI_MANDATARY_PAGE_SCAN_MODE 0
#define BTM_COD_MAJOR_AUDIO 0x05
#define BTM_COD_MAJOR_CLASS_MASK 0x1F
#define BTM_COD_MINOR_FIXED_PIN 0x40
#define HCI_SWITCH_SUPPORTED(f) (((f)[0] >> 5) & 1)
#define HCI_CMD_POOL_ID 2

/* The trace masks of the BTM layer (the levels are BTE/l2c_int.h's). */
#define TRACE_BTM_ERROR 0x000D0000
#define TRACE_BTM_WARNING 0x000D0001
#define TRACE_BTM_API 0x000D0002
#define TRACE_BTM_EVENT 0x000D0003
#define TRACE_BTM_DEBUG 0x000D0004

static u8 btm_sec_execute_procedure(tBTM_SEC_DEV_REC* p_dev_rec);
static u8 btm_sec_start_authorization(tBTM_SEC_DEV_REC* p_dev_rec);
static void btm_sec_collision_timeout(void);
static void btm_sec_pin_code_request_timeout(void);

/* Calls the record's completion callback with `res` and forgets it. */
static void btm_sec_dev_rec_cback_event(tBTM_SEC_DEV_REC* p_dev_rec, u8 res) {
    tBTM_SEC_CALLBACK* p_callback = p_dev_rec->p_callback;

    if (p_callback != NULL) {
        (*p_callback)(p_dev_rec->bd_addr, p_dev_rec->p_ref_data, res);
        p_dev_rec->p_callback = NULL;
    }
}

/* Calls the record's completion callback with `res` and forgets it, first releasing the RFCOMM channel an outgoing
 * check reserved. */
static void btm_sec_dev_rec_mx_cback_event(tBTM_SEC_DEV_REC* p_dev_rec, u8 res) {
    if (p_dev_rec->p_callback != NULL) {
        if (p_dev_rec->is_originator && p_dev_rec->p_cur_service != NULL &&
            p_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
            p_dev_rec->p_cur_service->orig_mx_chan_id = 0;
        }
        (*p_dev_rec->p_callback)(p_dev_rec->bd_addr, p_dev_rec->p_ref_data, res);
        p_dev_rec->p_callback = NULL;
    }
}

/* Restores the authentication setting a bonding changed. */
static void btm_restore_mode(void) {
    if (btm_cb.security_mode_changed) {
        btm_cb.security_mode_changed = FALSE;
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
            LogMsg_1(TRACE_BTM_DEBUG, "btm_restore_mode: Authen Enable -> %d",
                     (btm_cb.security_mode == BTM_SEC_MODE_LINK));
        }
        btsnd_hcic_write_auth_enable((u8)(btm_cb.security_mode == BTM_SEC_MODE_LINK));
    }
}

/* 0x80487F1C (0x78): Registers the application's security callbacks. */
u8 BTM_SecRegister(tBTM_APPL_INFO* p_cb_info) {
    btm_cb.api = *p_cb_info;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_0(TRACE_BTM_EVENT, "BTM_Sec: application registered");
    }
    return TRUE;
}

/* 0x80487F94 (0x50): Adds a remote name notification callback; returns whether a slot was free. */
u8 BTM_SecAddRmtNameNotifyCallback(tBTM_RMT_NAME_CALLBACK* p_callback) {
    int i;

    for (i = 0; i < BTM_SEC_MAX_CALLBACKS; i++) {
        if (btm_cb.p_rmt_name_callback[i] == NULL) {
            btm_cb.p_rmt_name_callback[i] = p_callback;
            return TRUE;
        }
    }
    return FALSE;
}

/* 0x80487FE4 (0x54): Removes a remote name notification callback; returns whether it was registered. */
u8 BTM_SecDeleteRmtNameNotifyCallback(tBTM_RMT_NAME_CALLBACK* p_callback) {
    int i;

    for (i = 0; i < BTM_SEC_MAX_CALLBACKS; i++) {
        if (btm_cb.p_rmt_name_callback[i] == p_callback) {
            btm_cb.p_rmt_name_callback[i] = NULL;
            return TRUE;
        }
    }
    return FALSE;
}

/* Sets the security mode, switching link-level authentication and encryption on or off (GUESS: dead-stripped in
 * retail; its trace strings survive in the .data). */
void BTM_SetSecurityMode(u8 sec_mode) {
    u8 old_mode = btm_cb.security_mode;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_1(TRACE_BTM_API, "BTM_SetSecurityMode: mode:%d", sec_mode);
    }
    btm_cb.security_mode = sec_mode;

    if (old_mode == BTM_SEC_MODE_LINK && sec_mode != BTM_SEC_MODE_LINK) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
            LogMsg_0(TRACE_BTM_DEBUG, "BTM_SetSecurityMode: Authen Enable -> FALSE");
        }
        btsnd_hcic_write_auth_enable(FALSE);
        btsnd_hcic_write_encr_mode(HCI_ENCRYPT_MODE_DISABLED);
    } else if (sec_mode == BTM_SEC_MODE_LINK) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
            LogMsg_0(TRACE_BTM_DEBUG, "BTM_SetSecurityMode: Authen Enable -> TRUE");
        }
        btsnd_hcic_write_auth_enable(TRUE);
        btsnd_hcic_write_encr_mode(HCI_ENCRYPT_MODE_POINT_TO_POINT);
    }
}

/* 0x80488038 (0xB0): Sets the PIN type and the fixed PIN code. */
void BTM_SetPinType(u8 pin_type, u8* pin_code, u8 pin_code_len) {
    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_3(TRACE_BTM_API, "BTM_SetPinType: pin type %d [variable-0, fixed-1], code %s, length %d", pin_type,
                 (u32)pin_code, pin_code_len);
    }

    if (btm_cb.devcb_state > 1) {
        btsnd_hcic_write_pin_type(pin_type);
    }

    btm_cb.pin_type = pin_type;
    btm_cb.pin_code_len = pin_code_len;
    memcpy(btm_cb.pin_code, pin_code, pin_code_len);
}

/* 0x804880E8 (0x228): Registers (or updates) the security level of a service on `psm` and its multiplexer channel;
 * returns whether a service record was available. */
u8 BTM_SetSecurityLevel(u8 is_originator, char* p_name, u8 service_id, u8 sec_level, u16 psm, u32 mx_proto_id,
                        u32 mx_chan_id) {
    tBTM_SEC_SERV_REC* p_srec;
    u16 index;
    u16 first_unused_record = BTM_NO_AVAIL_SEC_SERVICES;
    u8 record_allocated = FALSE;

    p_srec = &btm_cb.sec_serv_rec[0];

    for (index = 0; index < BTM_SEC_MAX_SERVICE_RECORDS; index++, p_srec++) {
        if (p_srec->security_flags & BTM_SEC_IN_USE) {
            if (p_srec->psm == psm && p_srec->mx_proto_id == mx_proto_id && service_id == p_srec->service_id &&
                (!strncmp(p_name, p_srec->orig_service_name, BTM_SEC_SERVICE_NAME_LEN) ||
                 !strncmp(p_name, p_srec->term_service_name, BTM_SEC_SERVICE_NAME_LEN))) {
                record_allocated = TRUE;
                break;
            }
        } else if (!record_allocated) {
            record_allocated = TRUE;
            first_unused_record = index;
        }
    }

    if (record_allocated) {
        if (index >= BTM_SEC_MAX_SERVICE_RECORDS) {
            index = first_unused_record;
            p_srec = &btm_cb.sec_serv_rec[index];
        }

        p_srec->psm = psm;
        p_srec->service_id = service_id;
        p_srec->mx_proto_id = mx_proto_id;

        if (is_originator) {
            p_srec->orig_mx_chan_id = mx_chan_id;
            p_srec->security_flags &= ~(BTM_SEC_OUT_AUTHORIZE | BTM_SEC_OUT_ENCRYPT | BTM_SEC_OUT_AUTHENTICATE);
            strncpy(p_srec->orig_service_name, p_name, BTM_SEC_SERVICE_NAME_LEN);
        } else {
            p_srec->term_mx_chan_id = mx_chan_id;
            p_srec->security_flags &= ~(BTM_SEC_IN_AUTHORIZE | BTM_SEC_IN_ENCRYPT | BTM_SEC_IN_AUTHENTICATE);
            strncpy(p_srec->term_service_name, p_name, BTM_SEC_SERVICE_NAME_LEN);
        }

        p_srec->security_flags |= (u8)(sec_level | BTM_SEC_IN_USE);

        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_6(TRACE_BTM_API, "BTM_SEC_REG[%d]: id %d, is_orig %d, psm 0x%04x, proto_id %d, chan_id %d", index,
                     service_id, is_originator, psm, mx_proto_id, mx_chan_id);
        }
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
            LogMsg_3(TRACE_BTM_API, "               : sec: 0x%x, service name [%s] (up to %d chars saved)",
                     p_srec->security_flags, (u32)p_name, BTM_SEC_SERVICE_NAME_LEN);
        }
    } else if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
        LogMsg_1(TRACE_BTM_WARNING, "BTM_SEC_REG: Out of Service Records (%d)", BTM_SEC_MAX_SERVICE_RECORDS);
    }

    return record_allocated;
}

/* 0x80488310 (0xC4): Answers the pending PIN request of `bd_addr` with `p_pin` (or refuses it) and records the
 * trusted services. */
void BTM_PINCodeReply(u8* bd_addr, u8 res, u8 pin_len, u8* p_pin, u32* trusted_mask) {
    tBTM_SEC_DEV_REC* p_dev_rec;

    if (btm_cb.pin_code_req_tle.param == 0) {
        return;
    }

    btu_stop_timer(&btm_cb.pin_code_req_tle);
    btm_cb.pin_code_req_tle.param = 0;
    memset(btm_cb.pin_code_req_bda, 0xFF, 6);

    if (res != BTM_SUCCESS) {
        btm_cb.acl_disc_reason = HCI_ERR_HOST_REJECT_SECURITY;
        btsnd_hcic_pin_code_neg_reply(bd_addr);
        return;
    }

    btm_cb.acl_disc_reason = HCI_SUCCESS;
    btsnd_hcic_pin_code_req_reply(bd_addr, pin_len, p_pin);

    if ((p_dev_rec = btm_find_dev(bd_addr)) != NULL) {
        p_dev_rec->trusted_mask[0] = trusted_mask[0];
        p_dev_rec->trusted_mask[1] = trusted_mask[1];
    }
}

/* 0x804883D4 (0x1DC): Takes the application's authorization answer for `bd_addr` and continues (or fails) the
 * pending security check. */
void BTM_DeviceAuthorized(u8* bd_addr, u8 res, u32* trusted_mask) {
    tBTM_SEC_DEV_REC* p_dev_rec;

    if ((p_dev_rec = btm_find_dev(bd_addr)) == NULL) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
            LogMsg_6(TRACE_BTM_WARNING,
                     "Security Manager: Attempting Authorization of Unknown Device Address [%02x%02x%02x%02x%02x%02x]",
                     bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
        }
        return;
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "Security Manager: authorized status:%d State:%d", res,
                 (p_dev_rec) ? p_dev_rec->sec_state : 0);
    }

    if (res == BTM_SUCCESS) {
        p_dev_rec->sec_flags |= BTM_SEC_AUTHORIZED;
        p_dev_rec->trusted_mask[0] = trusted_mask[0];
        p_dev_rec->trusted_mask[1] = trusted_mask[1];
    }

    if (p_dev_rec->sec_state != BTM_SEC_STATE_AUTHORIZING) {
        return;
    }

    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;

    if (res != BTM_SUCCESS) {
        if (p_dev_rec->p_callback != NULL) {
            if (p_dev_rec->is_originator && p_dev_rec->p_cur_service != NULL &&
                p_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
                p_dev_rec->p_cur_service->orig_mx_chan_id = 0;
            }
            (*p_dev_rec->p_callback)(bd_addr, p_dev_rec->p_ref_data, res);
            p_dev_rec->p_callback = NULL;
        }
        return;
    }

    if ((res = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        if (p_dev_rec->p_callback != NULL) {
            if (p_dev_rec->is_originator && p_dev_rec->p_cur_service != NULL &&
                p_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
                p_dev_rec->p_cur_service->orig_mx_chan_id = 0;
            }
            (*p_dev_rec->p_callback)(bd_addr, p_dev_rec->p_ref_data, res);
            p_dev_rec->p_callback = NULL;
        }
    }
}

/* 0x804885B0 (0x3CC): Bonds with `bd_addr` using the PIN `p_pin`: authenticates the existing link, or creates one
 * (with authentication enabled) so the controller asks for the PIN. */
u8 BTM_SecBond(u8* bd_addr, u8 pin_len, u8* p_pin, u32* trusted_mask) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    tBTM_INQ_INFO* p_inq_info;
    u8 allow_switch;
    u8* p_features;
    u8 status;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_6(TRACE_BTM_API, "BTM_SecBond BDA: %02x:%02x:%02x:%02x:%02x:%02x", bd_addr[0], bd_addr[1], bd_addr[2],
                 bd_addr[3], bd_addr[4], bd_addr[5]);
    }

    if (pin_len > BTM_MAX_PIN_LEN) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) {
            LogMsg_1(TRACE_BTM_ERROR, "BTM_SecBond: Illegal Pin len:%d", pin_len);
        }
        return BTM_ILLEGAL_VALUE;
    }

    if ((p_dev_rec = btm_find_dev(bd_addr)) == NULL) {
        if ((p_dev_rec = btm_sec_alloc_dev(bd_addr)) == NULL) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) {
                LogMsg_0(TRACE_BTM_ERROR, "BTM_SecBond: no device block");
            }
            return BTM_NO_RESOURCES;
        }
    }

    if (p_dev_rec->hci_handle != BTM_SEC_INVALID_HANDLE && (p_dev_rec->sec_flags & BTM_SEC_AUTHENTICATED)) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
            LogMsg_0(TRACE_BTM_WARNING, "BTM_SecBond -> Already Paired");
        }
        return BTM_SUCCESS;
    }

    if (BTM_DeleteStoredLinkKey(bd_addr, NULL) != BTM_SUCCESS) {
        return BTM_NO_RESOURCES;
    }

    p_dev_rec->security_required = BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_BOND;
    p_dev_rec->is_originator = TRUE;
    p_dev_rec->trusted_mask[0] = trusted_mask[0];
    p_dev_rec->trusted_mask[1] = trusted_mask[1];

    btm_cb.sec_pin_code_len = pin_len;
    if (pin_len) {
        memcpy(btm_cb.sec_pin_code, p_pin, pin_len);
    }

    p_dev_rec->sec_flags &= ~(BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED | BTM_SEC_LINK_KEY_KNOWN);

    if ((p_dev_rec->dev_class[1] & BTM_COD_MAJOR_CLASS_MASK) == BTM_COD_MAJOR_AUDIO &&
        (p_dev_rec->dev_class[2] & BTM_COD_MINOR_FIXED_PIN) && btm_cb.pin_type != HCI_PIN_TYPE_FIXED) {
        btm_cb.pin_type_changed = TRUE;
        btsnd_hcic_write_pin_type(HCI_PIN_TYPE_FIXED);
    }

    btm_cb.bond_disc_reason = 0;

    if (p_dev_rec->hci_handle != BTM_SEC_INVALID_HANDLE) {
        p_dev_rec->sec_flags &= ~BTM_SEC_LINK_KEY_KNOWN;
        p_dev_rec->sec_state = BTM_SEC_STATE_AUTHENTICATING;
        if (!btsnd_hcic_auth_request(p_dev_rec->hci_handle)) {
            return BTM_NO_RESOURCES;
        }
        btm_cb.bond_created_connection = FALSE;
        return BTM_CMD_STARTED;
    }

    if (btm_cb.security_mode != BTM_SEC_MODE_LINK) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
            LogMsg_0(TRACE_BTM_DEBUG, "BTM_SecBond: Authen Enable -> TRUE");
        }
        if (!btsnd_hcic_write_auth_enable(TRUE)) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) {
                LogMsg_0(TRACE_BTM_ERROR, "BTM_SecBond: no buffer");
            }
            return BTM_NO_RESOURCES;
        }
        btm_cb.security_mode_changed = TRUE;
    }

    btm_cb.bond_created_connection = TRUE;

    p_features = BTM_ReadLocalFeatures();
    allow_switch = HCI_SWITCH_SUPPORTED(p_features);
    p_dev_rec->acl_connected = FALSE;

    if ((p_inq_info = BTM_InqDbRead(bd_addr)) != NULL) {
        status = btsnd_hcic_create_conn(bd_addr, HCI_PKT_TYPES_MASK_DM1 | HCI_PKT_TYPES_MASK_DH1,
                                        p_inq_info->page_scan_rep_mode, p_inq_info->page_scan_mode,
                                        (u16)(p_inq_info->clock_offset | BTM_CLOCK_OFFSET_VALID), allow_switch);
    } else {
        status = btsnd_hcic_create_conn(bd_addr, HCI_PKT_TYPES_MASK_DM1 | HCI_PKT_TYPES_MASK_DH1,
                                        HCI_PAGE_SCAN_REP_MODE_R1, HCI_MANDATARY_PAGE_SCAN_MODE,
                                        p_dev_rec->clock_offset, allow_switch);
    }

    if (status) {
        return BTM_CMD_STARTED;
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) {
        LogMsg_0(TRACE_BTM_ERROR, "BTM_SecBond: no buffer");
    }
    btm_restore_mode();
    return BTM_NO_RESOURCES;
}

/* Encrypts the link to `bd_addr` and calls `p_callback` with the result (GUESS: dead-stripped in retail; its trace
 * strings survive in the .data). */
/* untyped: a caller-owned payload - the reference handed back to the callback */
u8 BTM_SetEncryption(u8* bd_addr, tBTM_SEC_CALLBACK* p_callback, void* p_ref_data) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    u8 rc;

    p_dev_rec = btm_find_dev(bd_addr);
    if (p_dev_rec == NULL || p_dev_rec->hci_handle == BTM_SEC_INVALID_HANDLE) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
            LogMsg_0(TRACE_BTM_WARNING, "Security Manager: BTM_SetEncryption not connected");
        }
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_ref_data, BTM_WRONG_MODE);
        }
        return BTM_WRONG_MODE;
    }

    if (p_dev_rec->sec_flags & BTM_SEC_ENCRYPTED) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_0(TRACE_BTM_EVENT, "Security Manager: BTM_SetEncryption already encrypted");
        }
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_ref_data, BTM_SUCCESS);
        }
        return BTM_SUCCESS;
    }

    if (p_dev_rec->p_callback != NULL) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
            LogMsg_0(TRACE_BTM_WARNING, "Security Manager: BTM_SetEncryption busy");
        }
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_ref_data, BTM_BUSY);
        }
        return BTM_BUSY;
    }

    p_dev_rec->p_callback = p_callback;
    p_dev_rec->p_ref_data = p_ref_data;
    p_dev_rec->security_required |= BTM_SEC_IN_AUTHENTICATE | BTM_SEC_IN_ENCRYPT;
    p_dev_rec->is_originator = FALSE;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_API) {
        LogMsg_4(TRACE_BTM_API, "Security Manager: BTM_SetEncryption Handle:%d State:%d Flags:0x%x Required:0x%x",
                 p_dev_rec->hci_handle, p_dev_rec->sec_state, p_dev_rec->sec_flags, p_dev_rec->security_required);
    }

    if ((rc = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_ref_data, rc);
        }
    }
    return rc;
}

/* Returns the first registered service record for `psm`, or NULL. */
static tBTM_SEC_SERV_REC* btm_sec_find_first_serv(u16 psm) {
    tBTM_SEC_SERV_REC* p_serv_rec = &btm_cb.sec_serv_rec[0];
    int i;

    for (i = 0; i < BTM_SEC_MAX_SERVICE_RECORDS; i++, p_serv_rec++) {
        if ((p_serv_rec->security_flags & BTM_SEC_IN_USE) && p_serv_rec->psm == psm) {
            return p_serv_rec;
        }
    }
    return NULL;
}

/* Returns the next registered service record after `p_cur` with the same PSM, or NULL. */
static tBTM_SEC_SERV_REC* btm_sec_find_next_serv(tBTM_SEC_SERV_REC* p_cur) {
    tBTM_SEC_SERV_REC* p_serv_rec = &btm_cb.sec_serv_rec[0];
    u8 found_cur = FALSE;
    int i;

    for (i = 0; i < BTM_SEC_MAX_SERVICE_RECORDS; i++, p_serv_rec++) {
        if ((p_serv_rec->security_flags & BTM_SEC_IN_USE) && p_serv_rec->psm == p_cur->psm) {
            if (!found_cur) {
                if (p_cur == p_serv_rec) {
                    found_cur = TRUE;
                }
            } else {
                return p_serv_rec;
            }
        }
    }
    return NULL;
}

/* Returns the registered service record of the multiplexer channel `mx_chan_id` on `psm`, or NULL. */
static tBTM_SEC_SERV_REC* btm_sec_find_mx_serv(u8 is_originator, u16 psm, u32 mx_proto_id, u32 mx_chan_id) {
    tBTM_SEC_SERV_REC* p_serv_rec = &btm_cb.sec_serv_rec[0];
    int i;

    for (i = 0; i < BTM_SEC_MAX_SERVICE_RECORDS; i++, p_serv_rec++) {
        if ((p_serv_rec->security_flags & BTM_SEC_IN_USE) && p_serv_rec->psm == psm &&
            p_serv_rec->mx_proto_id == mx_proto_id &&
            ((is_originator && p_serv_rec->orig_mx_chan_id == mx_chan_id) ||
             (!is_originator && p_serv_rec->term_mx_chan_id == mx_chan_id))) {
            return p_serv_rec;
        }
    }
    return NULL;
}

/* 0x8048897C (0x464): Checks the security an L2CAP channel on `psm` needs and starts the missing procedures;
 * `p_callback` reports the result unless the check is still running. */
u8 btm_sec_l2cap_access_req(u8* bd_addr, u16 psm, u16 handle, u8 is_originator, tBTM_SEC_CALLBACK* p_callback) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    tBTM_SEC_SERV_REC* p_serv_rec;
    u8 security_required;
    u8 rc;

    if ((p_dev_rec = btm_find_dev(bd_addr)) == NULL) {
        if ((p_dev_rec = btm_sec_alloc_dev(bd_addr)) == NULL) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
                LogMsg_1(TRACE_BTM_WARNING, "Security Manager: l2cap_access_req PSM:%d no resources", psm);
            }
            if (p_callback != NULL) {
                (*p_callback)(bd_addr, NULL, BTM_NO_RESOURCES);
            }
            return BTM_NO_RESOURCES;
        }
    }

    p_dev_rec->hci_handle = handle;

    p_serv_rec = btm_sec_find_first_serv(psm);

    if (p_serv_rec == NULL) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
            LogMsg_1(TRACE_BTM_WARNING, "Security Manager: l2cap_access_req PSM:%d no application registerd", psm);
        }
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, NULL, BTM_MODE_UNSUPPORTED);
        }
        return BTM_MODE_UNSUPPORTED;
    }

    if (!is_originator || (is_originator && p_dev_rec->p_cur_service == NULL)) {
        p_dev_rec->p_cur_service = p_serv_rec;
    }

    security_required = p_serv_rec->security_flags;

    if (btm_sec_find_next_serv(p_serv_rec) != NULL) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_1(TRACE_BTM_EVENT, "Security Manager: l2cap_access_req PSM:%d postponed for multiplexer", psm);
        }
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, NULL, BTM_SUCCESS);
        }
        return BTM_SUCCESS;
    }

    p_dev_rec->p_callback = p_callback;
    p_dev_rec->p_ref_data = NULL;
    p_dev_rec->security_required = security_required;
    p_dev_rec->is_originator = is_originator;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_5(TRACE_BTM_EVENT, "Security Manager: l2cap_access_req PSM:%d Handle:%d State:%d Flags:0x%x Required:0x%x",
                 psm, handle, p_dev_rec->sec_state, p_dev_rec->sec_flags, security_required);
    }

    if (!is_originator && (security_required & BTM_SEC_IN_AUTHORIZE)) {
        p_dev_rec->sec_flags &= ~BTM_SEC_AUTHORIZED;
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_3(TRACE_BTM_EVENT, "Security Manager: trusted:0x%04x%04x Flags:0x%x", p_dev_rec->trusted_mask[1],
                     p_dev_rec->trusted_mask[0], p_dev_rec->sec_flags);
        }
    }

    if ((rc = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_dev_rec->p_ref_data, rc);
            p_dev_rec->p_callback = NULL;
        }
    }

    return rc;
}

/* 0x80488DE0 (0x29C): Checks the security a multiplexed channel needs and starts the missing procedures;
 * `p_callback` reports the result with `p_ref_data` unless the check is still running. */
/* untyped: a caller-owned payload - the reference handed back to the callback */
void btm_sec_mx_access_request(u8* bd_addr, u16 psm, u8 is_originator, u32 mx_proto_id, u32 mx_chan_id,
                               tBTM_SEC_CALLBACK* p_callback, void* p_ref_data) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    tBTM_SEC_SERV_REC* p_serv_rec;
    u8 rc;

    if ((p_dev_rec = btm_find_dev(bd_addr)) == NULL && (p_dev_rec = btm_sec_alloc_dev(bd_addr)) == NULL) {
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_ref_data, BTM_NO_RESOURCES);
        }
        return;
    }

    p_serv_rec = btm_sec_find_mx_serv(is_originator, psm, mx_proto_id, mx_chan_id);

    if (p_serv_rec == NULL) {
        if (p_callback != NULL) {
            (*p_callback)(bd_addr, p_ref_data, BTM_MODE_UNSUPPORTED);
        }
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_ERROR) {
            LogMsg_3(TRACE_BTM_ERROR, "Security Manager: MX service not found PSM:%d Proto:%d SCN:%d", psm,
                     mx_proto_id, mx_chan_id);
        }
        return;
    }

    p_dev_rec->p_cur_service = p_serv_rec;
    p_dev_rec->security_required = p_serv_rec->security_flags;
    p_dev_rec->is_originator = is_originator;
    p_dev_rec->p_callback = p_callback;
    p_dev_rec->p_ref_data = p_ref_data;

    p_dev_rec->sec_flags &= ~BTM_SEC_AUTHORIZED;

    if ((rc = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        if (p_callback != NULL) {
            if (p_dev_rec->is_originator && p_dev_rec->p_cur_service != NULL &&
                p_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
                p_dev_rec->p_cur_service->orig_mx_chan_id = 0;
            }
            (*p_callback)(bd_addr, p_ref_data, rc);
            p_dev_rec->p_callback = NULL;
        }
    }

}

/* 0x8048907C (0x12C): Handles an incoming ACL connection request from `bda`: rejects unpaired or filtered devices,
 * otherwise records the peer and hands the request to L2CAP. */
void btm_sec_conn_req(u8* bda, u8* dc) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    BT_HDR* p_buf;

    if (btm_cb.connect_only_paired &&
        ((p_dev_rec = btm_find_dev(bda)) == NULL || !(p_dev_rec->sec_flags & BTM_SEC_LINK_KEY_KNOWN))) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_0(TRACE_BTM_EVENT, "Security Manager: connect request from not paired device");
        }
        if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
            btsnd_hcic_reject_conn(p_buf, bda, HCI_ERR_HOST_REJECT_DEVICE);
        }
        return;
    }

    if (btm_cb.p_conn_filter_cb != NULL) {
        if (!(*btm_cb.p_conn_filter_cb)(bda, dc)) {
            if ((p_buf = (BT_HDR*)GKI_getpoolbuf(HCI_CMD_POOL_ID)) != NULL) {
                btsnd_hcic_reject_conn(p_buf, bda, HCI_ERR_HOST_REJECT_DEVICE);
            }
            return;
        }
    }

    memcpy(btm_cb.connecting_bda, bda, 6);
    memcpy(btm_cb.connecting_dc, dc, 3);

    l2c_link_hci_conn_req(bda);
}

/* 0x804891A8 (0x1C): Sets the security mode and clears the pending PIN request. */
void btm_sec_init(u8 sec_mode) {
    btm_cb.security_mode = sec_mode;
    memset(btm_cb.pin_code_req_bda, 0xFF, 6);
}

/* 0x804891C4 (0x40): Re-enables link-level authentication and encryption after a controller reset in link mode. */
void btm_sec_dev_reset(void) {
    if (btm_cb.security_mode == BTM_SEC_MODE_LINK) {
        btsnd_hcic_write_auth_enable(TRUE);
        btsnd_hcic_write_encr_mode(HCI_ENCRYPT_MODE_POINT_TO_POINT);
    }
}

/* 0x80489204 (0xB4): Abandons the pending security check of `bd_addr`, telling the application. */
void btm_sec_abort_access_req(u8* bd_addr) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);

    if (p_dev_rec == NULL) {
        return;
    }

    if (btm_cb.api.p_abort_callback != NULL) {
        (*btm_cb.api.p_abort_callback)(bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name);
    }

    if (p_dev_rec->sec_state != BTM_SEC_STATE_AUTHORIZING && p_dev_rec->sec_state != BTM_SEC_STATE_AUTHENTICATING) {
        return;
    }

    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;
    if (p_dev_rec->is_originator && p_dev_rec->p_cur_service != NULL &&
        p_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
        p_dev_rec->p_cur_service->orig_mx_chan_id = 0;
    }
    p_dev_rec->p_callback = NULL;
}

/* 0x804892B8 (0x2E8): Handles the remote name result of a security name request: records the name, sends a delayed
 * link key, finishes a bonding, and continues (or fails) the pending security check. */
void btm_sec_rmt_name_request_complete(u8* p_bd_addr, u8* p_bd_name, u8 status) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    int i;

    if (p_bd_addr != NULL) {
        p_dev_rec = btm_find_dev(p_bd_addr);
    } else {
        p_dev_rec = &btm_cb.sec_dev_rec[0];

        for (i = 0; i < BTM_SEC_MAX_DEVICE_RECORDS; i++, p_dev_rec++) {
            if ((p_dev_rec->sec_flags & BTM_SEC_IN_USE) && p_dev_rec->sec_state == BTM_SEC_STATE_GETTING_NAME) {
                break;
            }
        }

        if (i == BTM_SEC_MAX_DEVICE_RECORDS) {
            p_dev_rec = NULL;
        }
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "Security Manager: rmt_name_complete status:%d State:%d", status,
                 (p_dev_rec) ? p_dev_rec->sec_state : 0);
    }

    if (p_dev_rec == NULL) {
        return;
    }

    if (status == HCI_SUCCESS) {
        memcpy(p_dev_rec->sec_bd_name, p_bd_name, BTM_MAX_REM_BD_NAME_LEN);
        p_dev_rec->sec_flags |= BTM_SEC_NAME_KNOWN;

        for (i = 0; i < BTM_SEC_MAX_CALLBACKS; i++) {
            if (btm_cb.p_rmt_name_callback[i] != NULL) {
                (*btm_cb.p_rmt_name_callback[i])(p_bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name);
            }
        }
    }

    if (p_dev_rec->link_key_not_sent) {
        if ((p_dev_rec->security_required & BTM_SEC_BOND) && !p_dev_rec->acl_connected) {
            return;
        }

        p_dev_rec->link_key_not_sent = FALSE;

        if (btm_cb.api.p_link_key_callback != NULL) {
            (*btm_cb.api.p_link_key_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                              p_dev_rec->link_key, p_dev_rec->link_key_type);
        }

        for (i = 0; i < BTM_SEC_MAX_CALLBACKS; i++) {
            if (btm_cb.p_link_key_notify_cb[i] != NULL) {
                (*btm_cb.p_link_key_notify_cb[i])(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                  p_dev_rec->link_key, p_dev_rec->link_key_type);
            }
        }
    }

    if ((p_dev_rec->security_required & BTM_SEC_BOND) && (p_dev_rec->sec_flags & BTM_SEC_AUTHENTICATED)) {
        p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_BOND);
        btsnd_hcic_disconnect(p_dev_rec->hci_handle, HCI_ERR_PEER_USER);
        return;
    }

    if (p_dev_rec->sec_state != BTM_SEC_STATE_GETTING_NAME) {
        return;
    }

    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;

    if (status != HCI_SUCCESS) {
        btm_sec_dev_rec_cback_event(p_dev_rec, BTM_ERR_PROCESSING);
        return;
    }

    if ((status = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        btm_sec_dev_rec_cback_event(p_dev_rec, status);
    }
}

/* 0x804895A0 (0x344): Handles the authentication complete event of the link `handle`: restores the bonding's
 * settings, reports the result and continues (or fails) the pending security check. */
void btm_sec_auth_complete(u16 handle, u8 status) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "Security Manager: auth_complete status:%d State:%d", status,
                 (p_dev_rec) ? p_dev_rec->sec_state : 0);
    }

    btm_restore_mode();
    if (btm_cb.pin_type_changed) {
        btm_cb.pin_type_changed = FALSE;
        btsnd_hcic_write_pin_type(btm_cb.pin_type);
    }

    if (p_dev_rec != NULL && (p_dev_rec->security_required & BTM_SEC_BOND) && !btm_cb.bond_created_connection) {
        p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_BOND);
        if (btm_cb.bond_disc_reason) {
            btm_sec_disconnect(handle, btm_cb.bond_disc_reason);
        }
    }

    if (p_dev_rec == NULL) {
        return;
    }

    if (p_dev_rec->sec_state != BTM_SEC_STATE_AUTHENTICATING) {
        if (btm_cb.api.p_auth_complete_callback != NULL && status != HCI_SUCCESS) {
            (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                   status);
        }
        return;
    }

    if (status == HCI_ERR_COMMAND_DISALLOWED &&
        (p_dev_rec->sec_flags & (BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED)) ==
            (BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED)) {
        status = HCI_SUCCESS;
    }

    if (btm_cb.api.p_auth_complete_callback != NULL) {
        (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                               status);
    }

    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;

    if (p_dev_rec->security_required & BTM_SEC_BOND) {
        p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_BOND);
        btsnd_hcic_disconnect(p_dev_rec->hci_handle, HCI_ERR_PEER_USER);
        return;
    }

    if (status != HCI_SUCCESS) {
        btm_sec_dev_rec_mx_cback_event(p_dev_rec, BTM_ERR_PROCESSING);
        btsnd_hcic_disconnect(p_dev_rec->hci_handle, HCI_ERR_AUTH_FAILURE);
        return;
    }

    p_dev_rec->sec_flags |= BTM_SEC_AUTHENTICATED;

    if ((status = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        btm_sec_dev_rec_mx_cback_event(p_dev_rec, status);
    }
}

/* 0x804898E4 (0xFC): Hands the master link key complete event to the registered callback. */
void btm_sec_mkey_comp_event(u16 handle, u8 status, u8 key_flg) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);
    u8 bd_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "Security Manager: mkey comp status:%d State:%d", status,
                 (p_dev_rec) ? p_dev_rec->sec_state : 0);
    }

    if (btm_cb.mkey_cback != NULL) {
        if (p_dev_rec == NULL) {
            (btm_cb.mkey_cback)(bd_addr, status, key_flg);
        } else {
            (btm_cb.mkey_cback)(p_dev_rec->bd_addr, status, key_flg);
        }
    }
}

/* 0x804899E0 (0x13C): Handles the encryption change event of the link `handle`: retries after a collision, and
 * continues (or fails) the pending security check. */
void btm_sec_encrypt_change(u16 handle, u8 status, u8 encr_enable) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "Security Manager: encrypt_change status:%d State:%d", status,
                 (p_dev_rec) ? p_dev_rec->sec_state : 0);
    }

    if (p_dev_rec == NULL) {
        return;
    }

    if (status == HCI_ERR_LMP_ERR_TRANS_COLLISION) {
        btm_cb.p_collided_dev_rec = p_dev_rec;
        btm_cb.sec_collision_tle.param = (u32)btm_sec_collision_timeout;
        btu_start_timer(&btm_cb.sec_collision_tle, BTU_TTYPE_USER_FUNC, BT_1SEC_TIMEOUT);
        return;
    }

    if (status != HCI_SUCCESS) {
        btm_sec_dev_rec_cback_event(p_dev_rec, BTM_ERR_PROCESSING);
        return;
    }

    p_dev_rec->sec_flags |= BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED;

    if (p_dev_rec->sec_state != BTM_SEC_STATE_ENCRYPTING) {
        return;
    }

    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;

    if ((status = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
        btm_sec_dev_rec_cback_event(p_dev_rec, status);
    }
}

/* 0x80489B1C (0x40): Whether the link `handle` is bonding. */
u8 btm_sec_is_bonding(u16 handle) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);

    if (p_dev_rec != NULL && (p_dev_rec->security_required & BTM_SEC_BOND)) {
        return TRUE;
    }
    return FALSE;
}

/* 0x80489B5C (0x480): Handles the connection complete event of an ACL link to `bda`: reports a failed bonding or
 * security check, finishes a bonding that only had a key to send, or records the link and starts its security. */
void btm_sec_connected(u8* bda, u16 handle, u8 status, u8 enc_mode) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bda);
    u8 res;
    int i;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_3(TRACE_BTM_EVENT, "Security Manager: btm_sec_connected handle:%d status:%d enc_mode:%d", handle, status,
                 enc_mode);
    }

    if (p_dev_rec == NULL) {
        if (status == HCI_SUCCESS) {
            if ((p_dev_rec = btm_sec_alloc_dev(bda)) == NULL) {
                return;
            }
        } else {
            return;
        }
    } else {
        p_dev_rec->timestamp = btm_cb.dev_rec_count++;
        p_dev_rec->acl_connected = TRUE;
    }

    if (p_dev_rec->security_required & BTM_SEC_BOND) {
        btm_restore_mode();
        if (btm_cb.pin_type_changed) {
            btm_cb.pin_type_changed = FALSE;
            btsnd_hcic_write_pin_type(btm_cb.pin_type);
        }
    }

    if (status != HCI_SUCCESS) {
        if (status == HCI_ERR_AUTH_FAILURE || status == HCI_ERR_KEY_MISSING || status == HCI_ERR_HOST_REJECT_SECURITY ||
            status == HCI_ERR_PAIRING_NOT_ALLOWED || status == HCI_ERR_UNIT_KEY_USED ||
            status == HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED || status == HCI_ERR_ENCRY_MODE_NOT_ACCEPTABLE ||
            status == HCI_ERR_REPEATED_ATTEMPTS || status == HCI_ERR_INSUFFCIENT_SECURITY ||
            (p_dev_rec->security_required & BTM_SEC_BOND)) {
            p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_BOND);
            p_dev_rec->sec_flags &= ~BTM_SEC_LINK_KEY_KNOWN;

            if (btm_cb.pin_code_req_tle.param) {
                btu_stop_timer(&btm_cb.pin_code_req_tle);
                status = HCI_ERR_HOST_REJECT_SECURITY;
                btm_cb.pin_code_req_tle.param = 0;
            }

            if (btm_cb.acl_disc_reason == HCI_ERR_HOST_REJECT_SECURITY) {
                status = HCI_ERR_HOST_REJECT_SECURITY;
            }

            if (btm_cb.api.p_auth_complete_callback != NULL) {
                (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class,
                                                       p_dev_rec->sec_bd_name, status);
            }
            return;
        }

        if (p_dev_rec->p_callback != NULL) {
            if (p_dev_rec->is_originator && p_dev_rec->p_cur_service != NULL &&
                p_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
                p_dev_rec->p_cur_service->orig_mx_chan_id = 0;
            }
            if (status == HCI_ERR_CONNECTION_TOUT || status == HCI_ERR_UNSPECIFIED ||
                status == HCI_ERR_PAGE_TIMEOUT_STATUS) {
                (*p_dev_rec->p_callback)(p_dev_rec->bd_addr, p_dev_rec->p_ref_data, BTM_DEVICE_TIMEOUT);
            } else {
                (*p_dev_rec->p_callback)(p_dev_rec->bd_addr, p_dev_rec->p_ref_data, BTM_ERR_PROCESSING);
            }
        }
        return;
    }

    if ((p_dev_rec->security_required & BTM_SEC_BOND) && p_dev_rec->link_key_not_sent) {
        p_dev_rec->link_key_not_sent = FALSE;

        if (btm_cb.api.p_link_key_callback != NULL) {
            (*btm_cb.api.p_link_key_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                              p_dev_rec->link_key, p_dev_rec->link_key_type);
        }

        for (i = 0; i < BTM_SEC_MAX_CALLBACKS; i++) {
            if (btm_cb.p_link_key_notify_cb[i] != NULL) {
                (*btm_cb.p_link_key_notify_cb[i])(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                  p_dev_rec->link_key, p_dev_rec->link_key_type);
            }
        }

        if (btm_cb.api.p_auth_complete_callback != NULL) {
            (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                   HCI_SUCCESS);
        }

        p_dev_rec->security_required &= ~(BTM_SEC_OUT_AUTHENTICATE | BTM_SEC_BOND);
        btsnd_hcic_disconnect(handle, HCI_ERR_PEER_USER);
        return;
    }

    p_dev_rec->hci_handle = handle;

    p_dev_rec->sec_flags &= ~(BTM_SEC_AUTHORIZED | BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED);
    if (enc_mode) {
        p_dev_rec->sec_flags |= BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED;
    }
    if (btm_cb.security_mode == BTM_SEC_MODE_LINK) {
        p_dev_rec->sec_flags |= BTM_SEC_AUTHENTICATED;
    }

    p_dev_rec->link_key_changed = FALSE;

    if (!(p_dev_rec->sec_flags & BTM_SEC_NAME_KNOWN) || p_dev_rec->is_originator) {
        if ((res = btm_sec_execute_procedure(p_dev_rec)) != BTM_CMD_STARTED) {
            btm_sec_dev_rec_mx_cback_event(p_dev_rec, res);
        }
    }
}

/* 0x80489FDC (0xBC): Drops the link `handle` with `reason` (later, when it is bonding); returns the BTM status. */
u8 btm_sec_disconnect(u16 handle, u8 reason) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);
    u8 old_state;

    if (p_dev_rec == NULL) {
        return BTM_ILLEGAL_VALUE;
    }

    if (p_dev_rec->security_required & BTM_SEC_BOND) {
        btm_cb.bond_disc_reason = reason;
        return BTM_SUCCESS;
    }

    old_state = p_dev_rec->sec_state;
    if (old_state == BTM_SEC_STATE_DISCONNECTING) {
        return BTM_CMD_STARTED;
    }

    p_dev_rec->sec_state = BTM_SEC_STATE_DISCONNECTING;

    if (btsnd_hcic_disconnect(handle, reason)) {
        return BTM_CMD_STARTED;
    }

    p_dev_rec->sec_state = old_state;
    return BTM_NO_RESOURCES;
}

/* 0x8048A098 (0x13C): Handles the disconnection of the link `handle`: fails a running bonding or PIN request to the
 * application and resets the record's link state. */
void btm_sec_disconnected(u16 handle, u8 reason) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev_by_handle(handle);

    if (p_dev_rec == NULL) {
        return;
    }

    if ((p_dev_rec->security_required & BTM_SEC_BOND) && btm_cb.api.p_auth_complete_callback != NULL) {
        if (reason == HCI_ERR_REPEATED_ATTEMPTS) {
            (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                   HCI_ERR_REPEATED_ATTEMPTS);
        } else {
            (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                   HCI_ERR_AUTH_FAILURE);
        }
    }

    if (btm_cb.pin_code_req_tle.param && !memcmp(btm_cb.pin_code_req_bda, p_dev_rec->bd_addr, 6)) {
        btu_stop_timer(&btm_cb.pin_code_req_tle);
        btm_cb.pin_code_req_tle.param = 0;
        memset(btm_cb.pin_code_req_bda, 0xFF, 6);

        if (btm_cb.api.p_auth_complete_callback != NULL) {
            (*btm_cb.api.p_auth_complete_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                   HCI_ERR_HOST_REJECT_SECURITY);
        }
    }

    p_dev_rec->hci_handle = BTM_SEC_INVALID_HANDLE;
    p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;
    p_dev_rec->sec_flags &= ~(BTM_SEC_AUTHORIZED | BTM_SEC_AUTHENTICATED | BTM_SEC_ENCRYPTED);
    p_dev_rec->security_required &= BTM_SEC_IN_USE;
    p_dev_rec->p_callback = NULL;
}

/* 0x8048A1D4 (0x1AC): Records the new link key of `p_bda` and hands it to the application (later, when the name is
 * not known yet). */
void btm_sec_link_key_notification(u8* p_bda, u8* p_link_key, u8 key_type) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(p_bda);
    int i;

    if (p_dev_rec == NULL && (p_dev_rec = btm_sec_alloc_dev(p_bda)) == NULL) {
        return;
    }

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_6(TRACE_BTM_EVENT, "btm_sec_link_key_notification()  BDA: %02x:%02x:%02x:%02x:%02x:%02x", p_bda[0],
                 p_bda[1], p_bda[2], p_bda[3], p_bda[4], p_bda[5]);
    }
    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_1(TRACE_BTM_EVENT, "                                TYPE: %d", key_type);
    }

    if (btm_cb.local_version.hciVersion < HCI_VERSION_1_1) {
        p_dev_rec->link_key_type = 0xFF;
    } else {
        p_dev_rec->link_key_type = key_type;
    }

    p_dev_rec->sec_flags |= BTM_SEC_LINK_KEY_KNOWN;
    memcpy(p_dev_rec->link_key, p_link_key, 16);

    if (!(p_dev_rec->sec_flags & BTM_SEC_NAME_KNOWN) ||
        ((p_dev_rec->security_required & BTM_SEC_BOND) && !p_dev_rec->acl_connected)) {
        p_dev_rec->link_key_not_sent = TRUE;
        return;
    }

    if (btm_cb.api.p_link_key_callback != NULL) {
        (*btm_cb.api.p_link_key_callback)(p_bda, p_dev_rec->dev_class, p_dev_rec->sec_bd_name, p_link_key,
                                          p_dev_rec->link_key_type);
    }

    for (i = 0; i < BTM_SEC_MAX_CALLBACKS; i++) {
        if (btm_cb.p_link_key_notify_cb[i] != NULL) {
            (*btm_cb.p_link_key_notify_cb[i])(p_bda, p_dev_rec->dev_class, p_dev_rec->sec_bd_name, p_link_key,
                                              p_dev_rec->link_key_type);
        }
    }
}

/* 0x8048A380 (0x130): Answers the controller's link key request for `p_bda` from the record or the application, or
 * says there is none. */
void btm_sec_link_key_request(u8* p_bda) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(p_bda);
    u8 link_key[16];

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_6(TRACE_BTM_EVENT, "btm_sec_link_key_request()  BDA: %02x:%02x:%02x:%02x:%02x:%02x", p_bda[0], p_bda[1],
                 p_bda[2], p_bda[3], p_bda[4], p_bda[5]);
    }

    if (p_dev_rec != NULL && (p_dev_rec->sec_flags & BTM_SEC_LINK_KEY_KNOWN)) {
        btsnd_hcic_link_key_req_reply(p_bda, p_dev_rec->link_key);
        return;
    }

    if (p_dev_rec == NULL) {
        p_dev_rec = btm_sec_alloc_dev(p_bda);
    }

    if (!(p_dev_rec->security_required & BTM_SEC_BOND) && btm_cb.api.p_link_key_req_callback != NULL) {
        if (p_dev_rec != NULL) {
            if ((*btm_cb.api.p_link_key_req_callback)(p_bda, p_dev_rec->link_key) == BTM_SUCCESS) {
                btsnd_hcic_link_key_req_reply(p_bda, p_dev_rec->link_key);
                return;
            }
        } else if ((*btm_cb.api.p_link_key_req_callback)(p_bda, link_key) == BTM_SUCCESS) {
            btsnd_hcic_link_key_req_reply(p_bda, link_key);
            return;
        }
    }

    btsnd_hcic_link_key_neg_reply(p_bda);
}

/* 0x8048A4B0 (0x74): Refuses the PIN request the application did not answer in time. */
static void btm_sec_pin_code_request_timeout(void) {
    tBTM_CB* p_cb = &btm_cb;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_0(TRACE_BTM_EVENT, "btm_sec_pin_code_request_timeout()");
    }
    p_cb->pin_code_req_tle.param = 0;
    memset(btm_cb.pin_code_req_bda, 0xFF, 6);
    btsnd_hcic_pin_code_neg_reply(p_cb->connecting_bda);
}

/* 0x8048A524 (0x35C): Handles the controller's PIN request for `p_bda`: answers with the fixed or bonding PIN, asks
 * the application (with a timeout), or refuses. */
void btm_sec_pin_code_request(u8* p_bda) {
    tBTM_CB* p_cb = &btm_cb;
    tBTM_SEC_DEV_REC* p_dev_rec;
    u8 reject = FALSE;
    int i = 0;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_6(TRACE_BTM_EVENT, "btm_sec_pin_code_request()  BDA: %02x:%02x:%02x:%02x:%02x:%02x", p_bda[0], p_bda[1],
                 p_bda[2], p_bda[3], p_bda[4], p_bda[5]);
    }

    if (!p_cb->pairing_disabled && p_cb->pin_type == HCI_PIN_TYPE_FIXED) {
        if (btm_find_dev(p_bda) == NULL) {
            for (p_dev_rec = &btm_cb.sec_dev_rec[0]; i < BTM_SEC_MAX_DEVICE_RECORDS; i++, p_dev_rec++) {
                if (!(p_dev_rec->sec_flags & BTM_SEC_IN_USE)) {
                    break;
                }
            }
        }

        if (i >= BTM_SEC_MAX_DEVICE_RECORDS) {
            btsnd_hcic_pin_code_neg_reply(p_bda);
            return;
        }

        btsnd_hcic_pin_code_req_reply(p_bda, p_cb->pin_code_len, p_cb->pin_code);
        return;
    }

    if ((p_dev_rec = btm_find_dev(p_bda)) == NULL) {
        p_dev_rec = btm_sec_alloc_dev(p_bda);
    }

    memcpy(btm_cb.pin_code_req_bda, p_bda, 6);

    if (p_dev_rec != NULL) {
        if (!memcmp(p_bda, p_cb->connecting_bda, 6)) {
            memcpy(p_dev_rec->dev_class, p_cb->connecting_dc, 3);
        }

        if (p_dev_rec->security_required & BTM_SEC_BOND) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
                LogMsg_0(TRACE_BTM_EVENT, "btm_sec_pin_code_request bonding sending reply");
            }
            btsnd_hcic_pin_code_req_reply(p_bda, p_cb->sec_pin_code_len, p_cb->sec_pin_code);

            if (btm_cb.security_mode_changed) {
                btm_cb.security_mode_changed = FALSE;
                if (btm_cb.trace_level >= BT_TRACE_LEVEL_DEBUG) {
                    LogMsg_1(TRACE_BTM_DEBUG, "btm_sec_pin_code_request: Authen Enable -> %d",
                             (btm_cb.security_mode == BTM_SEC_MODE_LINK));
                }
                btsnd_hcic_write_auth_enable((u8)(btm_cb.security_mode == BTM_SEC_MODE_LINK));
            }
        } else if (p_cb->pairing_disabled || p_cb->api.p_pin_callback == NULL ||
                   (!p_dev_rec->is_originator &&
                    (p_dev_rec->dev_class[1] & BTM_COD_MAJOR_CLASS_MASK) == BTM_COD_MAJOR_AUDIO &&
                    (p_dev_rec->dev_class[2] & BTM_COD_MINOR_FIXED_PIN))) {
            reject = TRUE;
        } else {
            memcpy(p_cb->connecting_bda, p_bda, 6);
            memcpy(p_cb->connecting_dc, p_dev_rec->dev_class, 3);

            p_cb->pin_code_req_tle.param = (u32)btm_sec_pin_code_request_timeout;
            btu_start_timer(&p_cb->pin_code_req_tle, BTU_TTYPE_USER_FUNC, BTM_SEC_PIN_TIMEOUT);
            (*p_cb->api.p_pin_callback)(p_bda, p_dev_rec->dev_class, p_dev_rec->sec_bd_name);
        }
    } else {
        reject = TRUE;
    }

    if (reject) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_WARNING) {
            LogMsg_3(TRACE_BTM_WARNING, "btm_sec_pin_code_request(): Pairing disabled:%d; PIN callback:%x, Dev Rec:%x!",
                     p_cb->pairing_disabled, (u32)p_cb->api.p_pin_callback, (u32)p_dev_rec);
        }
        btsnd_hcic_pin_code_neg_reply(p_bda);
    }
}

/* 0x8048A880 (0x50): Records the clock offset the link `handle` reported in its record and the inquiry database. */
void btm_sec_update_clock_offset(u16 handle, u16 clock_offset) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    tBTM_INQ_INFO* p_inq_info;

    if ((p_dev_rec = btm_find_dev_by_handle(handle)) == NULL) {
        return;
    }

    p_dev_rec->clock_offset = clock_offset | BTM_CLOCK_OFFSET_VALID;

    if ((p_inq_info = BTM_InqDbRead(p_dev_rec->bd_addr)) == NULL) {
        return;
    }

    p_inq_info->clock_offset = clock_offset | BTM_CLOCK_OFFSET_VALID;
}

/* Starts the security name request of the record's device; returns whether it started. */
static u8 btm_sec_start_get_name(tBTM_SEC_DEV_REC* p_dev_rec) {
    u8 tempstate = p_dev_rec->sec_state;

    p_dev_rec->sec_state = BTM_SEC_STATE_GETTING_NAME;

    if (btm_initiate_rem_name(p_dev_rec->bd_addr, NULL, BTM_RMT_NAME_SEC, BTM_SEC_RMT_NAME_TIMEOUT, NULL) !=
        BTM_CMD_STARTED) {
        p_dev_rec->sec_state = tempstate;
        return FALSE;
    }
    return TRUE;
}

/* Starts authenticating the record's link; returns whether the command was queued. */
static u8 btm_sec_start_authentication(tBTM_SEC_DEV_REC* p_dev_rec) {
    p_dev_rec->sec_state = BTM_SEC_STATE_AUTHENTICATING;
    return btsnd_hcic_auth_request(p_dev_rec->hci_handle);
}

/* Starts encrypting the record's link; returns whether the command was queued. */
static u8 btm_sec_start_encryption(tBTM_SEC_DEV_REC* p_dev_rec) {
    if (!btsnd_hcic_set_conn_encrypt(p_dev_rec->hci_handle, TRUE)) {
        return FALSE;
    }
    p_dev_rec->sec_state = BTM_SEC_STATE_ENCRYPTING;
    return TRUE;
}

/* Whether the trusted-service bitmap trusts every service. */
static u8 btm_sec_are_all_trusted(u32* p_mask) {
    u32 trusted_inx;

    for (trusted_inx = 0; trusted_inx < 2; trusted_inx++) {
        if (p_mask[trusted_inx] != BTM_SEC_TRUST_ALL) {
            return FALSE;
        }
    }
    return TRUE;
}

/* 0x8048A8D0 (0x3E0): Starts the next security procedure the record still needs (ACL link, name, authentication,
 * encryption, authorization); returns BTM_CMD_STARTED while one runs, else the result. */
static u8 btm_sec_execute_procedure(tBTM_SEC_DEV_REC* p_dev_rec) {
    tL2C_LCB* p_lcb;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_3(TRACE_BTM_EVENT, "btm_sec_execute_procedure: Required:0x%x Flags:0x%x State:%d",
                 p_dev_rec->security_required, p_dev_rec->sec_flags, p_dev_rec->sec_state);
    }

    if (p_dev_rec->sec_state != BTM_SEC_STATE_IDLE) {
        return BTM_CMD_STARTED;
    }

    if (p_dev_rec->hci_handle == BTM_SEC_INVALID_HANDLE) {
        if ((p_lcb = l2cu_allocate_lcb(p_dev_rec->bd_addr)) == NULL) {
            L2CAP_TRACE_WARNING0("L2CAP - no LCB for L2CA_conn_req");
            return BTM_NO_RESOURCES;
        }
        if (!l2cu_create_conn(p_lcb)) {
            return BTM_NO_RESOURCES;
        }
        p_dev_rec->is_originator = TRUE;
        return BTM_CMD_STARTED;
    }

    if (!(p_dev_rec->sec_flags & BTM_SEC_NAME_KNOWN) && p_dev_rec->hci_handle != BTM_SEC_INVALID_HANDLE) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_0(TRACE_BTM_EVENT, "Security Manager: Start get name");
        }
        if (btm_sec_start_get_name(p_dev_rec)) {
            return BTM_CMD_STARTED;
        }
        p_dev_rec->sec_flags |= BTM_SEC_NAME_KNOWN;
    }

    if (!(p_dev_rec->sec_flags & BTM_SEC_AUTHENTICATED) &&
        ((p_dev_rec->is_originator && (p_dev_rec->security_required & BTM_SEC_OUT_AUTHENTICATE)) ||
         (!p_dev_rec->is_originator && (p_dev_rec->security_required & BTM_SEC_IN_AUTHENTICATE))) &&
        p_dev_rec->hci_handle != BTM_SEC_INVALID_HANDLE) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_0(TRACE_BTM_EVENT, "Security Manager: Start authentication");
        }
        if (!btm_sec_start_authentication(p_dev_rec)) {
            return BTM_NO_RESOURCES;
        }
        return BTM_CMD_STARTED;
    }

    if (!(p_dev_rec->sec_flags & BTM_SEC_ENCRYPTED) &&
        ((p_dev_rec->is_originator && (p_dev_rec->security_required & BTM_SEC_OUT_ENCRYPT)) ||
         (!p_dev_rec->is_originator && (p_dev_rec->security_required & BTM_SEC_IN_ENCRYPT))) &&
        p_dev_rec->hci_handle != BTM_SEC_INVALID_HANDLE) {
        if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
            LogMsg_0(TRACE_BTM_EVENT, "Security Manager: Start encryption");
        }
        if (!btm_sec_start_encryption(p_dev_rec)) {
            return BTM_NO_RESOURCES;
        }
        return BTM_CMD_STARTED;
    }

    if (!(p_dev_rec->sec_flags & BTM_SEC_AUTHORIZED) &&
        ((p_dev_rec->is_originator && (p_dev_rec->security_required & BTM_SEC_OUT_AUTHORIZE)) ||
         (!p_dev_rec->is_originator && (p_dev_rec->security_required & BTM_SEC_IN_AUTHORIZE)))) {
        if (!btm_sec_are_all_trusted(p_dev_rec->trusted_mask) &&
            p_dev_rec->p_cur_service->service_id < BTM_SEC_MAX_SERVICES &&
            !BTM_SEC_IS_SERVICE_TRUSTED(p_dev_rec->trusted_mask, p_dev_rec->p_cur_service->service_id)) {
            if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
                LogMsg_0(TRACE_BTM_EVENT, "Security Manager: Start authorization");
            }
            return btm_sec_start_authorization(p_dev_rec);
        }
    }

    p_dev_rec->security_required &= BTM_SEC_PROCEDURE_MASK;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_2(TRACE_BTM_EVENT, "Security Manager: trusted:0x%04x%04x", p_dev_rec->trusted_mask[1],
                 p_dev_rec->trusted_mask[0]);
    }
    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_0(TRACE_BTM_EVENT, "Security Manager: access granted");
    }

    return BTM_SUCCESS;
}

/* 0x8048ACB0 (0x11C): Asks the application to authorize the record's current service (after getting the device's
 * name); returns its answer or BTM_CMD_STARTED. */
static u8 btm_sec_start_authorization(tBTM_SEC_DEV_REC* p_dev_rec) {
    u8 result;
    char* p_service_name = NULL;
    u8 service_id;

    if ((p_dev_rec->sec_flags & BTM_SEC_NAME_KNOWN) || p_dev_rec->hci_handle == BTM_SEC_INVALID_HANDLE) {
        if (btm_cb.api.p_authorize_callback == NULL) {
            return BTM_MODE_UNSUPPORTED;
        }

        if (p_dev_rec->p_cur_service != NULL) {
            if (p_dev_rec->is_originator) {
                p_service_name = p_dev_rec->p_cur_service->orig_service_name;
            } else {
                p_service_name = p_dev_rec->p_cur_service->term_service_name;
            }
            service_id = p_dev_rec->p_cur_service->service_id;
        } else {
            service_id = 0;
        }

        p_dev_rec->sec_state = BTM_SEC_STATE_AUTHORIZING;
        result = (*btm_cb.api.p_authorize_callback)(p_dev_rec->bd_addr, p_dev_rec->dev_class, p_dev_rec->sec_bd_name,
                                                    p_service_name, service_id, p_dev_rec->is_originator);
        if (result == BTM_SUCCESS) {
            p_dev_rec->sec_flags |= BTM_SEC_AUTHORIZED;
            p_dev_rec->sec_state = BTM_SEC_STATE_IDLE;
        }
        return result;
    }

    btm_sec_start_get_name(p_dev_rec);
    return BTM_CMD_STARTED;
}

/* 0x8048ADCC (0xE0): Retries the security check an encryption collision interrupted. */
static void btm_sec_collision_timeout(void) {
    u8 status;

    if (btm_cb.trace_level >= BT_TRACE_LEVEL_EVENT) {
        LogMsg_0(TRACE_BTM_EVENT, "btm_sec_collision_timeout()");
    }
    btm_cb.sec_collision_tle.param = 0;

    status = btm_sec_execute_procedure(btm_cb.p_collided_dev_rec);

    if (status != BTM_CMD_STARTED) {
        if (btm_cb.p_collided_dev_rec->p_callback != NULL) {
            if (btm_cb.p_collided_dev_rec->is_originator && btm_cb.p_collided_dev_rec->p_cur_service != NULL &&
                btm_cb.p_collided_dev_rec->p_cur_service->mx_proto_id == BTM_SEC_PROTO_RFCOMM) {
                btm_cb.p_collided_dev_rec->p_cur_service->orig_mx_chan_id = 0;
            }
            (*btm_cb.p_collided_dev_rec->p_callback)(btm_cb.p_collided_dev_rec->bd_addr,
                                                     btm_cb.p_collided_dev_rec->p_ref_data, status);
            btm_cb.p_collided_dev_rec->p_callback = NULL;
        }
    }
}

/* 0x8048AEAC (0x34): The trusted-service bitmap of `bd_addr`, or NULL. */
u32* btm_read_trusted_mask(u8* bd_addr) {
    tBTM_SEC_DEV_REC* p_dev_rec = btm_find_dev(bd_addr);

    if (p_dev_rec != NULL) {
        return p_dev_rec->trusted_mask;
    }
    return NULL;
}

/* The "any device" address. */
const u8 BT_BD_ANY[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
