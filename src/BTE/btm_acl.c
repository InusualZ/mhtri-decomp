/*
 * BTE/btm_acl.c - the BTM ACL link management: the ACL table, link creation and removal, role switch, link policy,
 *   remote version/features, RSSI and link quality reads and the packet-type changes.
 *
 * RANGE. .text 0x80480A90-0x804827D4 (29 functions, 0x1D44 B); .data 0x80613B60-0x80613F80. Medium-strong start: the
 *   btm_* family opens with btm_acl_init after bta_hh_utils.c's last function, and BTE/gki_time.c's .data ends with
 *   the bta_hh tables exactly at 0x80613B60, the first object 0x80480B30 reads. It ends with
 *   btm_chg_all_acl_pkt_types, the last function of the BTE source's btm_acl.c; btm_dev.c's BTM_SecAddDevice follows.
 *   Every .data object is read only by this range's functions; the .data end 0x80613F80 is 8-aligned with no pad, so
 *   that seam is where the readers change, not a pad.
 * FLAGS. GC/3.0a5.2 with `cflags_nw4r`, as the matched BTE units (see BTE/l2c_utils.c); btm_bda_to_acl,
 *   btm_set_packet_types, BTM_SetLinkSuperTout, btm_handle_to_acl_index and BTM_GetHCIConnHandle inline into their
 *   callers (`-ipa file`).
 * NAMES. the map's, which are the BTE source's; 0x804819F8 btm_get_acl_disc_reason_code is a GUESS (returns the btm_cb
 *   disconnect reason, between BTM_GetNumAclLinks and BTM_GetHCIConnHandle as in the BTE source). BTM_ReadLinkPolicy and
 *   BTM_SetQoS are dead-stripped from the retail link; their bodies here are GUESSes that keep their trace strings in
 *   the .data order.
 * RESIDUALS. btm_acl_created, btm_read_remote_features_complete: in the inlined BTM_SetLinkSuperTout the target puts
 *   the loop counter, the timeout and the entry in r29/r28/r30 (r30/r28/r29), this source in r30/r29/r28 (r29/r30/r28);
 *   every declaration order of btm_acl_created and both orders in btm_bda_to_acl measured the same or worse.
 * SHAPES. plain C; the table is btm_cb.acl_db (BTE/btm_main.h), the entry type is BTE/btm_acl.h's. btm_bda_to_acl,
 *   btm_set_packet_types and BTM_SetLinkSuperTout are static here: every retail call inlines them and no copy survives.
 */

#include "BTE/btm_acl.h"
#include "BTE/btm_main.h"
#include "BTE/btm_pm.h"
#include "BTE/btm_sco.h"
#include "BTE/l2c_link.h"
#include "MSL_C/alloc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The BTU timer type of the ACL timers and their timeout in seconds. */
#define BTU_TTYPE_BTM_ACL 9
#define BTM_ACL_REPLY_TIMEOUT 3

/* The `param` of the link policy read timer. */
#define TT_DEV_RLNKP 4

/* The default link supervision timeout (slots). */
#define BTM_DEFAULT_LINK_SUPER_TOUT 0x7D00

/* The ACL packet type bits the stack may ask for, and the EDR bits that disable a packet type when set. */
#define BTM_ACL_SUPPORTED_PKTS_MASK 0xCC18
#define BTM_ACL_EXCEPTION_PKTS_MASK 0x3306
#define BTM_ACL_PKT_TYPES_MASK_DM1 0x0008
#define BTM_ACL_PKT_TYPES_MASK_DH1 0x0010
#define BTM_ACL_PKT_TYPES_MASK_NO_2_DH1 0x0002
#define BTM_ACL_PKT_TYPES_MASK_NO_3_DH1 0x0004
#define BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 0x0100
#define BTM_ACL_PKT_TYPES_MASK_NO_3_DH3 0x0200
#define BTM_ACL_PKT_TYPES_MASK_DM3 0x0400
#define BTM_ACL_PKT_TYPES_MASK_DH3 0x0800
#define BTM_ACL_PKT_TYPES_MASK_NO_2_DH5 0x1000
#define BTM_ACL_PKT_TYPES_MASK_NO_3_DH5 0x2000
#define BTM_ACL_PKT_TYPES_MASK_DM5 0x4000
#define BTM_ACL_PKT_TYPES_MASK_DH5 0x8000

/* The largest payload of each ACL packet type. */
#define HCI_DM1_PACKET_SIZE 17
#define HCI_DH1_PACKET_SIZE 27
#define HCI_DM3_PACKET_SIZE 121
#define HCI_DH3_PACKET_SIZE 183
#define HCI_DM5_PACKET_SIZE 224
#define HCI_DH5_PACKET_SIZE 339
#define HCI_EDR2_DH1_PACKET_SIZE 54
#define HCI_EDR2_DH3_PACKET_SIZE 367
#define HCI_EDR2_DH5_PACKET_SIZE 679
#define HCI_EDR3_DH1_PACKET_SIZE 83
#define HCI_EDR3_DH3_PACKET_SIZE 552
#define HCI_EDR3_DH5_PACKET_SIZE 1021

/* The ACL data size of the Broadcom controller that cannot carry a full 3-DH5 packet. */
#define BTM_BRCM_ACL_DATA_SIZE 1017

/* The HCI version that defines the EDR packet type bits. */
#define HCI_PROTO_VERSION_2_0 3

/* The link roles. */
#define BTM_ROLE_MASTER 0

/* The power modes BTM_ReadPowerMode reports, and the power manager id that only sets a mode. */
#define BTM_PM_MD_ACTIVE 0
#define BTM_PM_MD_SNIFF 2
#define BTM_PM_MD_PARK 3
#define BTM_PM_SET_ONLY_ID 0x80

/* The HCI reasons this unit sends or reports. */
#define HCI_SUCCESS 0
#define HCI_ERR_HW_FAILURE 0x03
#define HCI_ERR_PEER_USER 0x13

/* The status codes these functions return. */
#define BTM_SUCCESS 0
#define BTM_CMD_STARTED 1
#define BTM_BUSY 2
#define BTM_NO_RESOURCES 3
#define BTM_MODE_UNSUPPORTED 4
#define BTM_WRONG_MODE 6
#define BTM_UNKNOWN_ADDR 7
#define BTM_ERR_PROCESSING 10

/* The local feature bits BTM_SetLinkPolicy checks. */
#define HCI_SWITCH_SUPPORTED(x) ((x)[0] & 0x20)
#define HCI_HOLD_MODE_SUPPORTED(x) ((x)[0] & 0x40)
#define HCI_SNIFF_MODE_SUPPORTED(x) ((x)[0] & 0x80)
#define HCI_PARK_MODE_SUPPORTED(x) ((x)[1] & 0x01)

/* The link policy bits. */
#define HCI_ENABLE_MASTER_SLAVE_SWITCH 0x0001
#define HCI_ENABLE_HOLD_MODE 0x0002
#define HCI_ENABLE_SNIFF_MODE 0x0004
#define HCI_ENABLE_PARK_MODE 0x0008

/* Returns the ACL entry of the device at `bda`, or NULL. */
static tACL_CONN* btm_bda_to_acl(u8* bda) {
    tACL_CONN* p = &btm_cb.acl_db[0];
    u16 xx;

    for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
        if ((p->in_use) && (!memcmp(p->remote_addr, bda, 6))) {
            return p;
        }
    }

    return NULL;
}

/* Asks for the packet types `pkt_types` on the link `p`, limited to what the controller supports. */
static u8 btm_set_packet_types(tACL_CONN* p, u16 pkt_types) {
    u16 temp_pkt_types;

    temp_pkt_types = (pkt_types & BTM_ACL_SUPPORTED_PKTS_MASK & btm_cb.btm_acl_pkt_types_supported);

    if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
        temp_pkt_types |= ((pkt_types & BTM_ACL_EXCEPTION_PKTS_MASK) |
                           (btm_cb.btm_acl_pkt_types_supported & BTM_ACL_EXCEPTION_PKTS_MASK));
    } else {
        temp_pkt_types &= (~BTM_ACL_EXCEPTION_PKTS_MASK);
    }

    BTM_TRACE_EVENT1("SetPacketType Mask -> 0x%04x", temp_pkt_types);

    if (!btsnd_hcic_change_conn_type(p->hci_handle, temp_pkt_types)) {
        return BTM_NO_RESOURCES;
    }

    p->pkt_types_mask = temp_pkt_types;

    return BTM_CMD_STARTED;
}

/* Stores the supervision timeout `timeout` of the link to `remote_bda` and sends it while this side is master. */
static u8 BTM_SetLinkSuperTout(u8* remote_bda, u16 timeout) {
    tACL_CONN* p = btm_bda_to_acl(remote_bda);

    if (p != NULL) {
        p->link_super_tout = timeout;

        if (p->link_role == BTM_ROLE_MASTER) {
            if (!btsnd_hcic_write_link_super_tout(p->hci_handle, timeout)) {
                return BTM_NO_RESOURCES;
            }
            return BTM_CMD_STARTED;
        }
        return BTM_SUCCESS;
    }

    return BTM_UNKNOWN_ADDR;
}

/* 0x80480A90 (0x1C): Sets the default supervision timeout and clears the disconnect reason. */
void btm_acl_init(void) {
    btm_cb.btm_def_link_super_tout = BTM_DEFAULT_LINK_SUPER_TOUT;
    btm_cb.acl_disc_reason = 0xFF;
}

/* 0x80480AAC (0x84): Returns the ACL table index of the link `hci_handle`, 4 when there is none. */
u8 btm_handle_to_acl_index(u16 hci_handle) {
    tACL_CONN* p = &btm_cb.acl_db[0];
    u8 xx;

    for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
        if ((p->in_use) && (p->hci_handle == hci_handle)) {
            break;
        }
    }

    return xx;
}

/* 0x80480B30 (0x314): Records a new ACL link, reads its version and clock offset, and applies the packet types, link
 * policy and supervision timeout once its features are known. */
void btm_acl_created(u8* bda, u8* dc, u8* bdn, u16 hci_handle, u8 link_role) {
    tBTM_SEC_DEV_REC* p_dev_rec;
    u8 yy;
    tACL_CONN* p;
    u8 xx;

    p = btm_bda_to_acl(bda);
    if (p != NULL) {
        p->hci_handle = hci_handle;
        p->link_role = link_role;
        BTM_TRACE_EVENT6("Duplicate btm_acl_created: RemBdAddr: %02x%02x%02x%02x%02x%02x", bda[0], bda[1], bda[2],
                         bda[3], bda[4], bda[5]);
        return;
    }

    for (xx = 0, p = &btm_cb.acl_db[0]; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
        if (!p->in_use) {
            p->in_use = TRUE;
            p->hci_handle = hci_handle;
            p->link_role = link_role;
            p->restore_pkt_types = 0;

            btm_pm_sm_alloc(xx);

            memcpy(p->remote_addr, bda, 6);

            if (dc) {
                memcpy(p->remote_dc, dc, 3);
            }

            if (bdn) {
                memcpy(p->remote_name, bdn, BTM_ACL_NAME_LEN);
            }

            btsnd_hcic_read_rmt_clk_offset(p->hci_handle);
            btsnd_hcic_rmt_ver_req(p->hci_handle);
            p_dev_rec = btm_find_dev_by_handle(hci_handle);

            if (p_dev_rec) {
                for (yy = 0; yy < 8; yy++) {
                    if (p_dev_rec->features[yy]) {
                        memcpy(p->features, p_dev_rec->features, 8);

                        btm_set_packet_types(p, btm_cb.btm_acl_pkt_types_supported);

                        if (btm_cb.btm_def_link_policy) {
                            BTM_SetLinkPolicy(p->remote_addr, &btm_cb.btm_def_link_policy);
                        }

                        BTM_SetLinkSuperTout(p->remote_addr, btm_cb.btm_def_link_super_tout);

                        if (btm_cb.p_acl_changed_cb) {
                            (*btm_cb.p_acl_changed_cb)(p->remote_addr, p->remote_dc, p->remote_name, p->features,
                                                       TRUE);
                        }
                        return;
                    }
                }
            }

            btsnd_hcic_rmt_features_req(p->hci_handle);
            return;
        }
    }
}

/* 0x80480E44 (0xC4): Forgets the ACL link to `bda` and reports it down. */
void btm_acl_removed(u8* bda) {
    tACL_CONN* p;

    p = btm_bda_to_acl(bda);
    if (p != NULL) {
        p->in_use = FALSE;

        if (btm_cb.p_acl_changed_cb) {
            (*btm_cb.p_acl_changed_cb)(bda, NULL, NULL, NULL, FALSE);
        }
    }
}

/* 0x80480F08 (0x64): Reports every ACL link disconnected by a hardware failure. */
void btm_acl_device_down(void) {
    tACL_CONN* p = &btm_cb.acl_db[0];
    u16 xx;

    for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
        if (p->in_use) {
            l2c_link_hci_disc_comp(p->hci_handle, HCI_ERR_HW_FAILURE);
        }
    }
}

/* 0x80480F6C (0x21C): Starts a switch to the role `new_role` on the link to `remote_bd_addr`, waking the link or
 * turning its encryption off first. */
u8 BTM_SwitchRole(u8* remote_bd_addr, u8 new_role, BtmCompleteCallback p_cb) {
    tACL_CONN* p;
    tBTM_SEC_DEV_REC* p_dev_rec = NULL;
    BtmPmPwrMode settings;
    u8 status;
    u8 pwr_mode;

    if (!HCI_SWITCH_SUPPORTED(btm_cb.local_features)) {
        return BTM_MODE_UNSUPPORTED;
    }

    if ((p = btm_bda_to_acl(remote_bd_addr)) == NULL) {
        return BTM_UNKNOWN_ADDR;
    }

    if (p->link_role == new_role) {
        return BTM_SUCCESS;
    }

    if (btm_is_sco_active_by_bdaddr(remote_bd_addr) == TRUE) {
        return BTM_NO_RESOURCES;
    }

    if (p->switch_role_state != BTM_ACL_SWKEY_STATE_IDLE) {
        BTM_TRACE_DEBUG0("Role change request declined since the previous request for this device is not completed ");
        return BTM_BUSY;
    }

    memset(&btm_cb.switch_role_ref_data, 0, sizeof(tBTM_ROLE_SWITCH_CMPL));

    if ((status = BTM_ReadPowerMode(p->remote_addr, &pwr_mode)) != BTM_SUCCESS) {
        return status;
    }

    if (pwr_mode == BTM_PM_MD_PARK || pwr_mode == BTM_PM_MD_SNIFF) {
        settings.mode = BTM_PM_MD_ACTIVE;
        status = BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p->remote_addr, &settings);
        if (status != BTM_CMD_STARTED) {
            return BTM_WRONG_MODE;
        }

        p->switch_role_state = BTM_ACL_SWKEY_STATE_MODE_CHANGE;
    } else {
        if (((p_dev_rec = btm_find_dev(remote_bd_addr)) != NULL) && ((p_dev_rec->sec_flags & BTM_SEC_ENCRYPTED) != 0)) {
            if (!btsnd_hcic_set_conn_encrypt(p->hci_handle, FALSE)) {
                return BTM_NO_RESOURCES;
            } else {
                p->switch_role_state = BTM_ACL_SWKEY_STATE_ENCRYPTION_OFF;
            }
        } else {
            if (!btsnd_hcic_switch_role(remote_bd_addr, new_role)) {
                return BTM_NO_RESOURCES;
            }

            p->switch_role_state = BTM_ACL_SWKEY_STATE_IN_PROGRESS;
        }
    }

    memcpy(btm_cb.switch_role_ref_data.remote_bd_addr, remote_bd_addr, 6);
    btm_cb.switch_role_ref_data.role = new_role;
    btm_cb.p_switch_role_cb = p_cb;
    return BTM_CMD_STARTED;
}

/* 0x80481188 (0x170): Continues a role switch on `handle` once its encryption is off, or finishes one once it is back
 * on. */
void btm_acl_encrypt_change(u16 handle, u8 status, u8 encr_enable) {
    u8 xx;
    tACL_CONN* p;

    xx = btm_handle_to_acl_index(handle);
    if (xx < BTM_MAX_ACL_LINKS) {
        p = &btm_cb.acl_db[xx];
    } else {
        return;
    }

    if (p->switch_role_state == BTM_ACL_SWKEY_STATE_ENCRYPTION_OFF) {
        if (encr_enable) {
            p->switch_role_state = BTM_ACL_SWKEY_STATE_IDLE;
        } else {
            p->switch_role_state = BTM_ACL_SWKEY_STATE_SWITCHING;
        }

        if (!btsnd_hcic_switch_role(p->remote_addr, (u8)!p->link_role)) {
            p->switch_role_state = BTM_ACL_SWKEY_STATE_IDLE;
            if (btm_cb.p_switch_role_cb) {
                (*btm_cb.p_switch_role_cb)(&btm_cb.switch_role_ref_data);
                btm_cb.p_switch_role_cb = NULL;
            }
        }
    } else if (p->switch_role_state == BTM_ACL_SWKEY_STATE_ENCRYPTION_ON) {
        p->switch_role_state = BTM_ACL_SWKEY_STATE_IDLE;
        if (btm_cb.p_switch_role_cb) {
            (*btm_cb.p_switch_role_cb)(&btm_cb.switch_role_ref_data);
            btm_cb.p_switch_role_cb = NULL;
        }
    }
}

/* 0x804812F8 (0x1F0): Drops the modes the controller lacks from `*settings` and sends the policy to the link to
 * `remote_bda`. */
u8 BTM_SetLinkPolicy(u8* remote_bda, u16* settings) {
    tACL_CONN* p;
    u8* localFeatures = BTM_ReadLocalFeatures();

    if (*settings) {
        if ((*settings & HCI_ENABLE_MASTER_SLAVE_SWITCH) && (!HCI_SWITCH_SUPPORTED(localFeatures))) {
            *settings &= (~HCI_ENABLE_MASTER_SLAVE_SWITCH);
            BTM_TRACE_API1("BTM_SetLinkPolicy switch not supported (settings: 0x%04x)", *settings);
        }
        if ((*settings & HCI_ENABLE_HOLD_MODE) && (!HCI_HOLD_MODE_SUPPORTED(localFeatures))) {
            *settings &= (~HCI_ENABLE_HOLD_MODE);
            BTM_TRACE_API1("BTM_SetLinkPolicy hold not supported (settings: 0x%04x)", *settings);
        }
        if ((*settings & HCI_ENABLE_SNIFF_MODE) && (!HCI_SNIFF_MODE_SUPPORTED(localFeatures))) {
            *settings &= (~HCI_ENABLE_SNIFF_MODE);
            BTM_TRACE_API1("BTM_SetLinkPolicy sniff not supported (settings: 0x%04x)", *settings);
        }
        if ((*settings & HCI_ENABLE_PARK_MODE) && (!HCI_PARK_MODE_SUPPORTED(localFeatures))) {
            *settings &= (~HCI_ENABLE_PARK_MODE);
            BTM_TRACE_API1("BTM_SetLinkPolicy park not supported (settings: 0x%04x)", *settings);
        }
    }

    if ((p = btm_bda_to_acl(remote_bda)) != NULL) {
        return btsnd_hcic_write_policy_set(p->hci_handle, *settings) ? BTM_CMD_STARTED : BTM_NO_RESOURCES;
    }

    return BTM_UNKNOWN_ADDR;
}

/* 0x804814E8 (0x10): Sets the link policy new links get. */
void BTM_SetDefaultLinkPolicy(u16 settings) {
    btm_cb.btm_def_link_policy = settings;
}

/* Starts a link policy read on the link to `remote_bda`; the linker strips it (no caller), the body is a GUESS that
 * keeps its trace string. */
u8 BTM_ReadLinkPolicy(u8* remote_bda, BtmCompleteCallback p_cb) {
    tACL_CONN* p;

    BTM_TRACE_API6("BTM_ReadLinkPolicy: RemBdAddr: %02x%02x%02x%02x%02x%02x", remote_bda[0], remote_bda[1],
                   remote_bda[2], remote_bda[3], remote_bda[4], remote_bda[5]);

    if (btm_cb.p_rlinkp_cmpl_cb) {
        return BTM_BUSY;
    }

    p = btm_bda_to_acl(remote_bda);
    if (p != NULL) {
        btu_start_timer(&btm_cb.rlinkp_timer, BTU_TTYPE_BTM_ACL, BTM_ACL_REPLY_TIMEOUT);
        btm_cb.p_rlinkp_cmpl_cb = p_cb;
        return BTM_CMD_STARTED;
    }

    return BTM_UNKNOWN_ADDR;
}

/* 0x804814F8 (0x104): Reports the link policy read result, with the remote address of the link, to the waiting
 * callback. */
void btm_read_link_policy_complete(u8* p) {
    BtmCompleteCallback p_cb = btm_cb.p_rlinkp_cmpl_cb;
    tBTM_LNK_POLICY_RESULTS lnkpol;
    u16 handle;
    tACL_CONN* p_acl_cb = &btm_cb.acl_db[0];
    u16 index;

    btu_stop_timer(&btm_cb.rlinkp_timer);

    btm_cb.p_rlinkp_cmpl_cb = NULL;

    if (p_cb) {
        STREAM_TO_UINT8(lnkpol.hci_status, p);

        if (lnkpol.hci_status == HCI_SUCCESS) {
            lnkpol.status = BTM_SUCCESS;

            STREAM_TO_UINT16(handle, p);

            STREAM_TO_UINT16(lnkpol.settings, p);

            for (index = 0; index < BTM_MAX_ACL_LINKS; index++, p_acl_cb++) {
                if ((p_acl_cb->in_use) && (handle == p_acl_cb->hci_handle)) {
                    memcpy(lnkpol.rem_bda, p_acl_cb->remote_addr, 6);
                    break;
                }
            }
        } else {
            lnkpol.status = BTM_ERR_PROCESSING;
        }

        (*p_cb)(&lnkpol);
    }
}

/* 0x804815FC (0x8C): Stores the remote version the controller read for its link. */
void btm_read_remote_version_complete(u8* p) {
    tACL_CONN* p_acl_cb = &btm_cb.acl_db[0];
    u8 status;
    u16 handle;
    int xx;

    STREAM_TO_UINT8(status, p);
    if (status == HCI_SUCCESS) {
        STREAM_TO_UINT16(handle, p);

        for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p_acl_cb++) {
            if ((p_acl_cb->in_use) && (p_acl_cb->hci_handle == handle)) {
                STREAM_TO_UINT8(p_acl_cb->lmp_version, p);
                STREAM_TO_UINT16(p_acl_cb->manufacturer, p);
                STREAM_TO_UINT16(p_acl_cb->lmp_subversion, p);
                break;
            }
        }
    }
}

/* 0x80481688 (0x22C): Stores the remote features the controller read, then applies the packet types, link policy and
 * supervision timeout and reports the link up. */
void btm_read_remote_features_complete(u8* p) {
    tACL_CONN* p_acl_cb = &btm_cb.acl_db[0];
    u8 status;
    u16 handle;
    int xx, yy;
    tBTM_SEC_DEV_REC* p_dev_rec;

    STREAM_TO_UINT8(status, p);
    if (status == HCI_SUCCESS) {
        STREAM_TO_UINT16(handle, p);

        for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p_acl_cb++) {
            if ((p_acl_cb->in_use) && (p_acl_cb->hci_handle == handle)) {
                for (yy = 0; yy < 8; yy++) {
                    STREAM_TO_UINT8(p_acl_cb->features[yy], p);
                }

                p_dev_rec = btm_find_dev_by_handle(handle);
                if (p_dev_rec) {
                    memcpy(p_dev_rec->features, p_acl_cb->features, 8);
                }

                btm_set_packet_types(p_acl_cb, btm_cb.btm_acl_pkt_types_supported);

                if (btm_cb.btm_def_link_policy) {
                    BTM_SetLinkPolicy(p_acl_cb->remote_addr, &btm_cb.btm_def_link_policy);
                }

                BTM_SetLinkSuperTout(p_acl_cb->remote_addr, btm_cb.btm_def_link_super_tout);

                if (btm_cb.p_acl_changed_cb) {
                    (*btm_cb.p_acl_changed_cb)(p_acl_cb->remote_addr, p_acl_cb->remote_dc, p_acl_cb->remote_name,
                                               p_acl_cb->features, TRUE);
                }
                break;
            }
        }
    }
}

/* 0x804818B4 (0x10): Sets the supervision timeout new links get. */
void BTM_SetDefaultLinkSuperTout(u16 timeout) {
    btm_cb.btm_def_link_super_tout = timeout;
}

/* 0x804818C4 (0xD8): Returns whether a link to `remote_bda` is up. */
u8 BTM_IsAclConnectionUp(u8* remote_bda) {
    tACL_CONN* p;

    BTM_TRACE_API6("BTM_ReadClockOffset: RemBdAddr: %02x%02x%02x%02x%02x%02x", remote_bda[0], remote_bda[1],
                   remote_bda[2], remote_bda[3], remote_bda[4], remote_bda[5]);

    p = btm_bda_to_acl(remote_bda);
    if (p != NULL) {
        return TRUE;
    }

    return FALSE;
}

/* 0x8048199C (0x5C): Returns the number of ACL links in use. */
u16 BTM_GetNumAclLinks(void) {
    tACL_CONN* p = &btm_cb.acl_db[0];
    u16 xx, yy;

    for (xx = yy = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
        if (p->in_use) {
            yy++;
        }
    }

    return yy;
}

/* 0x804819F8 (0x10): Returns the reason of the last disconnect. */
u16 btm_get_acl_disc_reason_code(void) {
    return btm_cb.acl_disc_reason;
}

/* 0x80481A08 (0x9C): Returns the handle of the link to `remote_bda`, 0xFFFF when there is none. */
u16 BTM_GetHCIConnHandle(u8* remote_bda) {
    tACL_CONN* p;

    p = btm_bda_to_acl(remote_bda);
    if (p != NULL) {
        return p->hci_handle;
    }

    return 0xFFFF;
}

/* 0x80481AA4 (0x9C): Stores the clock offset the controller read for the link `hci_handle`. */
void btm_process_clk_off_comp_evt(u16 hci_handle, u16 clock_offset) {
    u8 xx;

    if ((xx = btm_handle_to_acl_index(hci_handle)) < BTM_MAX_ACL_LINKS) {
        btm_cb.acl_db[xx].clock_offset = clock_offset;
    }
}

/* 0x80481B40 (0x1F4): Records a finished role switch, turns the encryption back on when the switch turned it off, and
 * reports the result. */
void btm_acl_role_changed(u8 hci_status, u8* bd_addr, u8 new_role) {
    u8* p_bda = (bd_addr) ? bd_addr : btm_cb.connecting_bda;
    tACL_CONN* p = btm_bda_to_acl(p_bda);
    tBTM_ROLE_SWITCH_CMPL* p_data = &btm_cb.switch_role_ref_data;

    if (p == NULL) {
        return;
    }

    p_data->hci_status = hci_status;

    if (hci_status == HCI_SUCCESS) {
        p_data->role = new_role;
        memcpy(p_data->remote_bd_addr, p_bda, 6);

        p->link_role = new_role;
    }

    if (bd_addr) {
        BTM_SetLinkSuperTout(p->remote_addr, p->link_super_tout);
    }

    if (p->switch_role_state == BTM_ACL_SWKEY_STATE_SWITCHING) {
        if (btsnd_hcic_set_conn_encrypt(p->hci_handle, TRUE)) {
            p->switch_role_state = BTM_ACL_SWKEY_STATE_ENCRYPTION_ON;
            return;
        }
    }

    if (p && p->switch_role_state == BTM_ACL_SWKEY_STATE_IN_PROGRESS) {
        p->switch_role_state = BTM_ACL_SWKEY_STATE_IDLE;
    }

    if (btm_cb.p_switch_role_cb) {
        (*btm_cb.p_switch_role_cb)(p_data);
        btm_cb.p_switch_role_cb = NULL;
    }

    BTM_TRACE_EVENT2("Role Switch Event: new_role 0x%02x, HCI Status 0x%02x", p_data->role, p_data->hci_status);
}

/* 0x80481D34 (0x5C): Fails a link policy read whose timer expired. */
void btm_acl_timeout(TIMER_LIST_ENT* p_tle) {
    u32 timer_type = p_tle->param;

    if (timer_type == TT_DEV_RLNKP) {
        BtmCompleteCallback p_cb = btm_cb.p_rlinkp_cmpl_cb;
        tBTM_LNK_POLICY_RESULTS lnkpol;

        lnkpol.status = BTM_ERR_PROCESSING;
        lnkpol.settings = 0;

        btm_cb.p_rlinkp_cmpl_cb = NULL;

        if (p_cb) {
            (*p_cb)(&lnkpol);
        }
    }
}

/* 0x80481D90 (0x1A0): Returns the largest ACL payload the packet types of the link to `addr` (or of the local device)
 * allow. */
u16 btm_get_max_packet_size(u8* addr) {
    tACL_CONN* p = btm_bda_to_acl(addr);
    u16 pkt_types = 0;
    u16 pkt_size = 0;

    if (p != NULL) {
        pkt_types = p->pkt_types_mask;
    } else {
        if (memcmp(btm_cb.local_addr, addr, 6) == 0) {
            pkt_types = btm_cb.btm_acl_pkt_types_supported;
        }
    }

    if (pkt_types) {
        if (!(pkt_types & BTM_ACL_PKT_TYPES_MASK_NO_3_DH5)) {
            pkt_size = HCI_EDR3_DH5_PACKET_SIZE;
        } else if (!(pkt_types & BTM_ACL_PKT_TYPES_MASK_NO_2_DH5)) {
            pkt_size = HCI_EDR2_DH5_PACKET_SIZE;
        } else if (!(pkt_types & BTM_ACL_PKT_TYPES_MASK_NO_3_DH3)) {
            pkt_size = HCI_EDR3_DH3_PACKET_SIZE;
        } else if (pkt_types & BTM_ACL_PKT_TYPES_MASK_DH5) {
            pkt_size = HCI_DH5_PACKET_SIZE;
        } else if (!(pkt_types & BTM_ACL_PKT_TYPES_MASK_NO_2_DH3)) {
            pkt_size = HCI_EDR2_DH3_PACKET_SIZE;
        } else if (pkt_types & BTM_ACL_PKT_TYPES_MASK_DM5) {
            pkt_size = HCI_DM5_PACKET_SIZE;
        } else if (pkt_types & BTM_ACL_PKT_TYPES_MASK_DH3) {
            pkt_size = HCI_DH3_PACKET_SIZE;
        } else if (pkt_types & BTM_ACL_PKT_TYPES_MASK_DM3) {
            pkt_size = HCI_DM3_PACKET_SIZE;
        } else if (!(pkt_types & BTM_ACL_PKT_TYPES_MASK_NO_3_DH1)) {
            pkt_size = HCI_EDR3_DH1_PACKET_SIZE;
        } else if (!(pkt_types & BTM_ACL_PKT_TYPES_MASK_NO_2_DH1)) {
            pkt_size = HCI_EDR2_DH1_PACKET_SIZE;
        } else if (pkt_types & BTM_ACL_PKT_TYPES_MASK_DH1) {
            pkt_size = HCI_DH1_PACKET_SIZE;
        } else if (pkt_types & BTM_ACL_PKT_TYPES_MASK_DM1) {
            pkt_size = HCI_DM1_PACKET_SIZE;
        }
    }

    if ((pkt_size == HCI_EDR3_DH5_PACKET_SIZE) && (btu_cb.hcit_acl_data_size == BTM_BRCM_ACL_DATA_SIZE)) {
        pkt_size = BTM_BRCM_ACL_DATA_SIZE;
    }

    return pkt_size;
}

/* 0x80481F30 (0x44): Registers (or with NULL clears) the ACL change callback; refuses a second one. */
u8 BTM_AclRegisterForChanges(tBTM_ACL_DB_CHANGE_CB* p_cb) {
    if (!p_cb) {
        btm_cb.p_acl_changed_cb = NULL;
    } else if (btm_cb.p_acl_changed_cb) {
        return BTM_BUSY;
    } else {
        btm_cb.p_acl_changed_cb = p_cb;
    }

    return BTM_SUCCESS;
}

/* Starts a QoS setup on the link to `bd`; the linker strips it (no caller), the body is a GUESS that keeps its trace
 * string. */
u8 BTM_SetQoS(u8* bd, FLOW_SPEC* p_flow, BtmCompleteCallback p_cb) {
    tACL_CONN* p;

    BTM_TRACE_API6("BTM_SetQoS: BdAddr: %02x%02x%02x%02x%02x%02x", bd[0], bd[1], bd[2], bd[3], bd[4], bd[5]);

    if (btm_cb.p_qossu_cmpl_cb) {
        return BTM_BUSY;
    }

    if ((p = btm_bda_to_acl(bd)) != NULL) {
        btu_start_timer(&btm_cb.qossu_timer, BTU_TTYPE_BTM_ACL, BTM_ACL_REPLY_TIMEOUT);
        btm_cb.p_qossu_cmpl_cb = p_cb;
        return BTM_CMD_STARTED;
    }

    return BTM_UNKNOWN_ADDR;
}

/* 0x80481F74 (0xD8): Reports the QoS setup result to the waiting callback. */
void btm_qos_setup_complete(u8 status, u16 handle, FLOW_SPEC* p_flow) {
    BtmCompleteCallback p_cb = btm_cb.p_qossu_cmpl_cb;
    tBTM_QOS_SETUP_CMPL qossu;

    btu_stop_timer(&btm_cb.qossu_timer);

    btm_cb.p_qossu_cmpl_cb = NULL;

    if (p_cb) {
        qossu.status = status;
        qossu.handle = handle;
        if (p_flow != NULL) {
            qossu.flow.qos_flags = p_flow->qos_flags;
            qossu.flow.service_type = p_flow->service_type;
            qossu.flow.token_rate = p_flow->token_rate;
            qossu.flow.peak_bandwidth = p_flow->peak_bandwidth;
            qossu.flow.latency = p_flow->latency;
            qossu.flow.delay_variation = p_flow->delay_variation;
        }
        BTM_TRACE_DEBUG1("BTM: p_flow->delay_variation: 0x%02x", p_flow->delay_variation);
        (*p_cb)(&qossu);
    }
}

/* 0x8048204C (0x13C): Starts an RSSI read on the link to `remote_bda`; `p_cb` gets the result. */
u8 BTM_ReadRSSI(u8* remote_bda, BtmCompleteCallback p_cb) {
    tACL_CONN* p;

    BTM_TRACE_API6("BTM_ReadRSSI: RemBdAddr: %02x%02x%02x%02x%02x%02x", remote_bda[0], remote_bda[1],
                   remote_bda[2], remote_bda[3], remote_bda[4], remote_bda[5]);

    if (btm_cb.p_rssi_cmpl_cb) {
        return BTM_BUSY;
    }

    p = btm_bda_to_acl(remote_bda);
    if (p != NULL) {
        btu_start_timer(&btm_cb.rssi_timer, BTU_TTYPE_BTM_ACL, BTM_ACL_REPLY_TIMEOUT);
        btm_cb.p_rssi_cmpl_cb = p_cb;

        if (!btsnd_hcic_read_rssi(p->hci_handle)) {
            btm_cb.p_rssi_cmpl_cb = NULL;
            btu_stop_timer(&btm_cb.rssi_timer);
            return BTM_NO_RESOURCES;
        } else {
            return BTM_CMD_STARTED;
        }
    }

    return BTM_UNKNOWN_ADDR;
}

/* 0x80482188 (0x13C): Starts a link quality read on the link to `remote_bda`; `p_cb` gets the result. */
u8 BTM_ReadLinkQuality(u8* remote_bda, BtmCompleteCallback p_cb) {
    tACL_CONN* p;

    BTM_TRACE_API6("BTM_ReadLinkQuality: RemBdAddr: %02x%02x%02x%02x%02x%02x", remote_bda[0], remote_bda[1],
                   remote_bda[2], remote_bda[3], remote_bda[4], remote_bda[5]);

    if (btm_cb.p_lnk_qual_cmpl_cb) {
        return BTM_BUSY;
    }

    p = btm_bda_to_acl(remote_bda);
    if (p != NULL) {
        btu_start_timer(&btm_cb.lnk_quality_timer, BTU_TTYPE_BTM_ACL, BTM_ACL_REPLY_TIMEOUT);
        btm_cb.p_lnk_qual_cmpl_cb = p_cb;

        if (!btsnd_hcic_get_link_quality(p->hci_handle)) {
            btu_stop_timer(&btm_cb.lnk_quality_timer);
            btm_cb.p_lnk_qual_cmpl_cb = NULL;
            return BTM_NO_RESOURCES;
        } else {
            return BTM_CMD_STARTED;
        }
    }

    return BTM_UNKNOWN_ADDR;
}

/* 0x804822C4 (0x11C): Reports the RSSI read result, with the remote address of the link, to the waiting callback. */
void btm_read_rssi_complete(u8* p) {
    BtmCompleteCallback p_cb = btm_cb.p_rssi_cmpl_cb;
    tBTM_RSSI_RESULTS results;
    u16 handle;
    tACL_CONN* p_acl_cb = &btm_cb.acl_db[0];
    u16 index;

    btu_stop_timer(&btm_cb.rssi_timer);
    btm_cb.p_rssi_cmpl_cb = NULL;

    if (p_cb) {
        STREAM_TO_UINT8(results.hci_status, p);

        if (results.hci_status == HCI_SUCCESS) {
            results.status = BTM_SUCCESS;

            STREAM_TO_UINT16(handle, p);

            STREAM_TO_UINT8(results.rssi, p);
            BTM_TRACE_EVENT2("BTM RSSI Complete: rssi %d, hci status 0x%02x", results.rssi, results.hci_status);

            for (index = 0; index < BTM_MAX_ACL_LINKS; index++, p_acl_cb++) {
                if ((p_acl_cb->in_use) && (handle == p_acl_cb->hci_handle)) {
                    memcpy(results.rem_bda, p_acl_cb->remote_addr, 6);
                    break;
                }
            }
        } else {
            results.status = BTM_ERR_PROCESSING;
        }

        (*p_cb)(&results);
    }
}

/* 0x804823E0 (0x118): Reports the link quality read result, with the remote address of the link, to the waiting
 * callback. */
void btm_read_link_quality_complete(u8* p) {
    BtmCompleteCallback p_cb = btm_cb.p_lnk_qual_cmpl_cb;
    tBTM_LINK_QUALITY_RESULTS results;
    u16 handle;
    tACL_CONN* p_acl_cb = &btm_cb.acl_db[0];
    u16 index;

    btu_stop_timer(&btm_cb.rssi_timer);
    btm_cb.p_lnk_qual_cmpl_cb = NULL;

    if (p_cb) {
        STREAM_TO_UINT8(results.hci_status, p);

        if (results.hci_status == HCI_SUCCESS) {
            results.status = BTM_SUCCESS;

            STREAM_TO_UINT16(handle, p);

            STREAM_TO_UINT8(results.link_quality, p);
            BTM_TRACE_EVENT2("BTM Link Quality Complete: Link Quality %d, hci status 0x%02x", results.link_quality,
                             results.hci_status);

            for (index = 0; index < BTM_MAX_ACL_LINKS; index++, p_acl_cb++) {
                if ((p_acl_cb->in_use) && (handle == p_acl_cb->hci_handle)) {
                    memcpy(results.rem_bda, p_acl_cb->remote_addr, 6);
                    break;
                }
            }
        } else {
            results.status = BTM_ERR_PROCESSING;
        }

        (*p_cb)(&results);
    }
}

/* 0x804824F8 (0xBC): Disconnects the link to `bd_addr` as a remote user termination. */
u8 btm_remove_acl(u8* bd_addr) {
    u16 hci_handle = BTM_GetHCIConnHandle(bd_addr);

    if (!btsnd_hcic_disconnect(hci_handle, HCI_ERR_PEER_USER)) {
        return BTM_NO_RESOURCES;
    }

    return BTM_SUCCESS;
}

/* 0x804825B4 (0x220): Lowers every link to one-slot packets while a SCO link runs (`enable` non-zero), or restores
 * the saved packet types, waking a sniffing link first. */
void btm_chg_all_acl_pkt_types(u8 enable) {
    tACL_CONN* p = &btm_cb.acl_db[0];
    u16 xx;
    u8 mode;
    BtmPmPwrMode pm;
    u16 pkt_types;

    if (enable) {
        for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
            if (p->in_use) {
                BTM_TRACE_DEBUG1("btm BEFORE SCO setting to 1 slot; hci hdl 0x%x", p->hci_handle);

                p->restore_pkt_types = p->pkt_types_mask;

                pkt_types = BTM_ACL_PKT_TYPES_MASK_DH1 | BTM_ACL_PKT_TYPES_MASK_DM1;
                if (btm_cb.local_version.hciVersion >= HCI_PROTO_VERSION_2_0) {
                    pkt_types |= BTM_ACL_PKT_TYPES_MASK_NO_2_DH3 | BTM_ACL_PKT_TYPES_MASK_NO_3_DH3 |
                                 BTM_ACL_PKT_TYPES_MASK_NO_2_DH5 | BTM_ACL_PKT_TYPES_MASK_NO_3_DH5;
                }
                btm_set_packet_types(p, pkt_types);
            }
        }
    } else {
        for (xx = 0; xx < BTM_MAX_ACL_LINKS; xx++, p++) {
            if (p->in_use && p->restore_pkt_types) {
                if (BTM_ReadPowerMode(p->remote_addr, &mode) == BTM_SUCCESS) {
                    if (mode == BTM_PM_MD_SNIFF) {
                        BTM_TRACE_DEBUG1("btm last SCO removed; unsniffing hci hdl 0x%x", p->hci_handle);
                        pm.mode = BTM_PM_MD_ACTIVE;
                        BTM_SetPowerMode(BTM_PM_SET_ONLY_ID, p->remote_addr, &pm);
                    } else {
                        BTM_TRACE_DEBUG2("btm last SCO removed; hci hdl 0x%x, types 0x%02x", p->hci_handle,
                                         p->pkt_types_mask);
                        btm_set_packet_types(p, p->restore_pkt_types);
                        p->restore_pkt_types = 0;
                    }
                }
            }
        }
    }
}
