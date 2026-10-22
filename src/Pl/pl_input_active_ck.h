/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_input_active_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_INPUT_ACTIVE_CK_H
#define MHTRI_PL_PL_INPUT_ACTIVE_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026F888 - 1 when the master player has a lever or button input */
u32 pl_input_active_ck(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_INPUT_ACTIVE_CK_H */
