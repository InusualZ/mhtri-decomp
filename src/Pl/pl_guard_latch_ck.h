/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_guard_latch_ck`, the declaration `enemy/em_act_step.cpp`
 * needs.  C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_GUARD_LATCH_CK_H
#define MHTRI_PL_PL_GUARD_LATCH_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027D51C - 1 while the player's +0x3B5 byte (`pl_guard_latch_set`) is set */
u32 pl_guard_latch_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GUARD_LATCH_CK_H */
