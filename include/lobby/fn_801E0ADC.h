/*
 * Declarations owned by `lobby/fn_801E0ADC.cpp` (docs/plan.md 6.5 rule 2).  The band (0x801E0ADC..0x801E7530)
 * also holds the enemy joint-effect spawner the action bands call, so the declaration lives here.
 */
#ifndef MHTRI_LOBBY_FN_801E0ADC_H
#define MHTRI_LOBBY_FN_801E0ADC_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {

/* 0x801E2D04 - spawns effect `id` for the enemy work: it reads the world position of `joint`, and when the id
 * passes the height test builds the effect with `pos` copied into its work block, `life` stored in the
 * effect record and `scale` in the work block.  The float sits before the
 * integer because the callers load it first. */
void eft_em_spawn_joint(struct _ENEMY_WORK* self, u32 joint, u32 id, nw4r::math::VEC3* pos, f32 scale, u32 life);
}
#endif

#endif /* MHTRI_LOBBY_FN_801E0ADC_H */
