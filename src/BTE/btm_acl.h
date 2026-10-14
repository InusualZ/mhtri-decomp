/* BTE/btm_acl.h - the ACL link entry points `BTE/btm_acl.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_ACL_H
#define MHTRI_BTE_BTM_ACL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80481D90 - the largest ACL packet the link to `bd_addr` carries, 0 when there is no link. */
u16 btm_get_max_packet_size(u8* bd_addr);

/* 0x80480B30 / 0x80480E44 - record a new ACL link (class and name may be NULL), or forget a closed one. */
void btm_acl_created(u8* bda, u8* dc, u8* bdn, u16 hci_handle, u8 link_role);
void btm_acl_removed(u8* bda);

/* 0x80480F6C - asks for the role `new_role` on the link to `remote_bd_addr`; `p_cb` reports the result. */
u8 BTM_SwitchRole(u8* remote_bd_addr, u8 new_role, void (*p_cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_ACL_H */
