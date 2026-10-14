/* BTE/btm_acl.h - the ACL link entry points `BTE/btm_acl.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_ACL_H
#define MHTRI_BTE_BTM_ACL_H

#include "types.h"
#include "BTE/l2c_api.h"

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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_ACL_H */
