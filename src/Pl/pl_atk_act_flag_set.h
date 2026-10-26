/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_atk_act_flag_set`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_PL_PL_ATK_ACT_FLAG_SET_H
#define MHTRI_PL_PL_ATK_ACT_FLAG_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E1B8 - sets the actor's attack-act flag bits. */
void pl_atk_act_flag_set(struct _PLW* self, u8 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ATK_ACT_FLAG_SET_H */
