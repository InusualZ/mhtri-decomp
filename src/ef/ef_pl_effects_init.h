/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft018.cpp`'s `ef_pl_effects_init`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_EF_EF_PL_EFFECTS_INIT_H
#define MHTRI_EF_EF_PL_EFFECTS_INIT_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8010D608 - initialises the player's effect set (GameMode_ck, eft_light_spawn) */
void ef_pl_effects_init(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PL_EFFECTS_INIT_H */
