/* BTE/btm_sco.h - the SCO link entry points `BTE/btm_sco.c` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTM_SCO_H
#define MHTRI_BTE_BTM_SCO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804878C8 / 0x80487A60 - drop the SCO links to `bda`, or forget them after its ACL link went down. */
void btm_remove_sco_links(u8* bda);
void btm_sco_acl_removed(u8* bda);

/* 0x80487E94 - whether a SCO link to `remote_bda` is up. */
u8 btm_is_sco_active_by_bdaddr(u8* remote_bda);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTM_SCO_H */
