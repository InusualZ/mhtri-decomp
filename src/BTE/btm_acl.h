/* BTE/btm_acl.h - the ACL link table entry `BTE/btm_acl.c` keeps in btm_cb, the results its completions report and the
 *   ACL entry points it owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_ACL_H
#define MHTRI_BTE_BTM_ACL_H

#include "types.h"
#include "BTE/gki_time.h"
#include "BTE/l2c_api.h"

/* The number of ACL links the stack tracks (the BTE source's MAX_L2CAP_LINKS). */
#define BTM_MAX_ACL_LINKS 4

/* The length of a remote device name the ACL table stores (the BTE source's BD_NAME_LEN). */
#define BTM_ACL_NAME_LEN 248

/* The role switch states of an ACL link (`switch_role_state`). */
#define BTM_ACL_SWKEY_STATE_IDLE 0
#define BTM_ACL_SWKEY_STATE_MODE_CHANGE 1
#define BTM_ACL_SWKEY_STATE_ENCRYPTION_OFF 2
#define BTM_ACL_SWKEY_STATE_SWITCHING 3
#define BTM_ACL_SWKEY_STATE_ENCRYPTION_ON 4
#define BTM_ACL_SWKEY_STATE_IN_PROGRESS 5

/* size: 0x11C - one ACL link: its handle, packet types, the remote device's address, class, name, version and
 * features, and the link's role. */
typedef struct tACL_CONN {
    /* +0x000 */ u16 hci_handle;
    /* +0x002 */ u16 pkt_types_mask;
    /* +0x004 */ u16 restore_pkt_types;
    /* +0x006 */ u16 clock_offset;
    /* +0x008 */ u8 remote_addr[6];
    /* +0x00E */ u8 remote_dc[3];
    /* +0x011 */ u8 remote_name[BTM_ACL_NAME_LEN];
    /* +0x109 */ u8 pad_0x109;
    /* +0x10A */ u16 manufacturer;
    /* +0x10C */ u16 lmp_subversion;
    /* +0x10E */ u16 link_super_tout;
    /* +0x110 */ u8 features[8];
    /* +0x118 */ u8 lmp_version;
    /* +0x119 */ u8 in_use;
    /* +0x11A */ u8 link_role;
    /* +0x11B */ u8 switch_role_state;
} tACL_CONN; /* size: 0x11C */

/* The ACL change callback: the remote address, class, name and features and whether the link is up. */
typedef void (tBTM_ACL_DB_CHANGE_CB)(u8* p_bda, u8* p_dc, u8* p_bdn, u8* features, u8 is_new);

/* size: 0x08 - the result a role switch reports: the HCI status, the new role and the remote address. */
typedef struct tBTM_ROLE_SWITCH_CMPL {
    /* +0x00 */ u8 hci_status;
    /* +0x01 */ u8 role;
    /* +0x02 */ u8 remote_bd_addr[6];
} tBTM_ROLE_SWITCH_CMPL; /* size: 0x08 */

/* size: 0x0A - the result of a link policy read: the BTM and HCI status, the remote address and the policy bits. */
typedef struct tBTM_LNK_POLICY_RESULTS {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 hci_status;
    /* +0x02 */ u8 rem_bda[6];
    /* +0x08 */ u16 settings;
} tBTM_LNK_POLICY_RESULTS; /* size: 0x0A */

/* size: 0x09 - the result of an RSSI read: the BTM and HCI status, the signed RSSI and the remote address. */
typedef struct tBTM_RSSI_RESULTS {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 hci_status;
    /* +0x02 */ s8 rssi;
    /* +0x03 */ u8 rem_bda[6];
} tBTM_RSSI_RESULTS; /* size: 0x09 */

/* size: 0x09 - the result of a link quality read: the BTM and HCI status, the quality and the remote address. */
typedef struct tBTM_LINK_QUALITY_RESULTS {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 hci_status;
    /* +0x02 */ u8 link_quality;
    /* +0x03 */ u8 rem_bda[6];
} tBTM_LINK_QUALITY_RESULTS; /* size: 0x09 */

/* size: 0x1C - the result of a QoS setup: the flow the controller granted, the link handle and the HCI status. */
typedef struct tBTM_QOS_SETUP_CMPL {
    /* +0x00 */ FLOW_SPEC flow;
    /* +0x18 */ u16 handle;
    /* +0x1A */ u8 status;
    /* +0x1B */ u8 pad_0x1B;
} tBTM_QOS_SETUP_CMPL; /* size: 0x1C */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80480A90 / 0x80480AAC - set the ACL defaults, and the ACL table index of `hci_handle` (4 when unknown). */
void btm_acl_init(void);
u8 btm_handle_to_acl_index(u16 hci_handle);

/* 0x80480B30 / 0x80480E44 / 0x80480F08 - record a new ACL link (class and name may be NULL), forget a closed one, or
 * report every link down. */
void btm_acl_created(u8* bda, u8* dc, u8* bdn, u16 hci_handle, u8 link_role);
void btm_acl_removed(u8* bda);
void btm_acl_device_down(void);

/* 0x80480F6C - asks for the role `new_role` on the link to `remote_bd_addr`; `p_cb` reports the result. */
u8 BTM_SwitchRole(u8* remote_bd_addr, u8 new_role, BtmCompleteCallback p_cb);

/* 0x80481188 - continues a role switch after the encryption of `handle` changed to `encr_enable`. */
void btm_acl_encrypt_change(u16 handle, u8 status, u8 encr_enable);

/* 0x804812F8 - sets the link policy `*settings` on the link to `remote_bda`, dropping the modes the controller lacks. */
u8 BTM_SetLinkPolicy(u8* remote_bda, u16* settings);

/* 0x804814F8 / 0x804815FC / 0x80481688 - the HCI completions of the link policy, remote version and remote features
 * reads (`p` is the event's parameter stream). */
void btm_read_link_policy_complete(u8* p);
void btm_read_remote_version_complete(u8* p);
void btm_read_remote_features_complete(u8* p);

/* 0x804818C4 / 0x8048199C / 0x804819F8 / 0x80481A08 - whether a link to `remote_bda` is up, the number of links, the
 * last disconnect reason and the handle of the link to `remote_bda` (0xFFFF when there is none). */
u8 BTM_IsAclConnectionUp(u8* remote_bda);
u16 BTM_GetNumAclLinks(void);
u16 btm_get_acl_disc_reason_code(void);
u16 BTM_GetHCIConnHandle(u8* remote_bda);

/* 0x80481AA4 / 0x80481B40 / 0x80481D34 - the clock offset event, the role change event and the ACL timer expiry. */
void btm_process_clk_off_comp_evt(u16 hci_handle, u16 clock_offset);
void btm_acl_role_changed(u8 hci_status, u8* bd_addr, u8 new_role);
void btm_acl_timeout(TIMER_LIST_ENT* p_tle);

/* 0x80481D90 - the largest ACL packet the link to `bd_addr` carries, 0 when there is no link. */
u16 btm_get_max_packet_size(u8* bd_addr);

/* 0x80481F30 - registers (or with NULL clears) the ACL change callback; refuses a second registration. */
u8 BTM_AclRegisterForChanges(tBTM_ACL_DB_CHANGE_CB* p_cb);

/* 0x80481F74 - reports the QoS setup result of the link `handle` (`p_flow` may be NULL). */
void btm_qos_setup_complete(u8 status, u16 handle, FLOW_SPEC* p_flow);

/* 0x8048204C / 0x80482188 - read the RSSI or the link quality of the link to `remote_bda`; `p_cb` gets the result. */
u8 BTM_ReadRSSI(u8* remote_bda, BtmCompleteCallback p_cb);
u8 BTM_ReadLinkQuality(u8* remote_bda, BtmCompleteCallback p_cb);

/* 0x804822C4 / 0x804823E0 - the HCI completions of the RSSI and link quality reads. */
void btm_read_rssi_complete(u8* p);
void btm_read_link_quality_complete(u8* p);

/* 0x804824F8 - disconnects the link to `bd_addr`; returns the stack's status. */
u8 btm_remove_acl(u8* bd_addr);

/* 0x804825B4 - lowers every link to one-slot packets (`enable` non-zero) or restores their packet types. */
void btm_chg_all_acl_pkt_types(u8 enable);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_ACL_H */
