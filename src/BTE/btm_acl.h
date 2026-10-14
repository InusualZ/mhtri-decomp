/* BTE/btm_acl.h - the ACL link entry points `BTE/btm_acl.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_ACL_H
#define MHTRI_BTE_BTM_ACL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80481D90 - the largest ACL packet the link to `bd_addr` carries, 0 when there is no link. */
u16 btm_get_max_packet_size(u8* bd_addr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_ACL_H */
