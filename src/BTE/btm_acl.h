/* BTE/btm_acl.h - the ACL link entry points `BTE/btm_acl.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_ACL_H
#define MHTRI_BTE_BTM_ACL_H

#include "types.h"
#include "BTE/l2c_api.h"
#include "BTE/gki_time.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TIMER_LIST_ENT;
/* 0x80481D34 - handles the expiry of the ACL timer `p_tle`. */
void btm_acl_timeout(struct TIMER_LIST_ENT* p_tle);

/* 0x80481F74 - finishes a QoS setup on the link `handle` with the granted flow spec `p_flow`. */
void btm_qos_setup_complete(u8 status, u16 handle, FLOW_SPEC* p_flow);

/* 0x80481D90 - the largest ACL packet the link to `bd_addr` carries, 0 when there is no link. */
u16 btm_get_max_packet_size(u8* bd_addr);

/* 0x80480B30 / 0x80480E44 - record a new ACL link (class and name may be NULL), or forget a closed one. */
void btm_acl_created(u8* bda, u8* dc, u8* bdn, u16 hci_handle, u8 link_role);
void btm_acl_removed(u8* bda);

/* 0x80480F6C - asks for the role `new_role` on the link to `remote_bd_addr`; `p_cb` reports the result. */
u8 BTM_SwitchRole(u8* remote_bd_addr, u8 new_role, void (*p_cb)(void));

/* 0x804819F8 */
u16 btm_get_acl_disc_reason_code(void);

/* 0x80481188 */
void btm_acl_encrypt_change(u16 handle, u8 status, u8 encr_enable);
/* 0x804814F8 */
void btm_read_link_policy_complete(u8* p);
/* 0x804815FC */
void btm_read_remote_version_complete(u8* p);
/* 0x80481688 */
void btm_read_remote_features_complete(u8* p);
/* 0x80481AA4 */
void btm_process_clk_off_comp_evt(u16 hci_handle, u16 clock_offset);
/* 0x80481B40 */
void btm_acl_role_changed(u8 hci_status, u8* bd_addr, u8 new_role);
/* 0x804822C4 */
void btm_read_rssi_complete(u8* p);
/* 0x804823E0 */
void btm_read_link_quality_complete(u8* p);

/* size: 0x09 - the result BTM_ReadRSSI reports. */
typedef struct tBTM_RSSI_RESULTS {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 hci_status;
    /* +0x02 */ s8 rssi;
    /* +0x03 */ u8 rem_bda[6];
} tBTM_RSSI_RESULTS; /* size: 0x09 */

/* size: 0x09 - the result BTM_ReadLinkQuality reports. */
typedef struct tBTM_LINK_QUALITY_RESULTS {
    /* +0x00 */ u8 status;
    /* +0x01 */ u8 hci_status;
    /* +0x02 */ u8 link_quality;
    /* +0x03 */ u8 rem_bda[6];
} tBTM_LINK_QUALITY_RESULTS; /* size: 0x09 */

/* The callback BTM_AclRegisterForChanges installs: a link to `p_bda` came up (`is_new`) or went down. */
typedef void (*tBTM_ACL_DB_CHANGE_CB)(u8* p_bda, u8* dc, u8* bdn, u8* features, u8 is_new);

/* 0x80481F30 / 0x8048199C / 0x804818C4 - register the link change callback, count the links, whether a link to
 * `remote_bda` is up. */
u8 BTM_AclRegisterForChanges(tBTM_ACL_DB_CHANGE_CB p_cb);
u16 BTM_GetNumAclLinks(void);
u8 BTM_IsAclConnectionUp(u8* remote_bda);

/* 0x8048204C / 0x80482188 - read the RSSI or link quality of the link to `remote_bda`; `p_cb` gets the result. */
u8 BTM_ReadRSSI(u8* remote_bda, BtmCompleteCallback p_cb);
u8 BTM_ReadLinkQuality(u8* remote_bda, BtmCompleteCallback p_cb);

/* 0x804812F8 - sets the link policy of the link to `remote_bda`. */
u8 BTM_SetLinkPolicy(u8* remote_bda, u16* settings);


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_ACL_H */
