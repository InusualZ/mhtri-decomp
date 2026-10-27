/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_act_held_class_get`, the one declaration `ef/eft053.cpp`
 * needs.  GUESS name, from the body (listed on the NAMES line of ef/eft053.cpp). */
#ifndef MHTRI_PL_PL_ACT_HELD_CLASS_GET_H
#define MHTRI_PL_PL_ACT_HELD_CLASS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027DFE4 (0x64): the class of the player's held act (1 for acts 0xAB/0xAE, 3 for 0xA7 in group 0, 2 for act 0xC in
 * group 12, 0 otherwise) */
s32 pl_act_held_class_get(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_HELD_CLASS_GET_H */
