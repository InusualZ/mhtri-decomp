/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_802FA9A0.cpp`'s `lb_unlock_cond_last_ck`, which
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_UNLOCK_COND_LAST_CK_H
#define MHTRI_LOBBY_LB_UNLOCK_COND_LAST_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802FB4F4 - whether unlock condition `cond` of the last table is met. */
u32 lb_unlock_cond_last_ck(s32 cond);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_UNLOCK_COND_LAST_CK_H */
