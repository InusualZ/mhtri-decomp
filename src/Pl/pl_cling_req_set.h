/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_cling_req_set`, the declaration `enemy/em_act_step.cpp`
 * needs.  C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_CLING_REQ_SET_H
#define MHTRI_PL_PL_CLING_REQ_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D3F0 - sets bit `group` of the player's +0x3AC cling request mask */
void pl_cling_req_set(struct _PLW* self, u8 group);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_CLING_REQ_SET_H */
