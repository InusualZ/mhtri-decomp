/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_push_clear`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_PL_PL_PUSH_CLEAR_H
#define MHTRI_PL_PL_PUSH_CLEAR_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027346C - clears the four words at +0x634..+0x640 */
void pl_push_clear(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_PUSH_CLEAR_H */
