/* Declarations owned by `lobby/fn_801E0ADC.cpp`: the enemy joint-effect spawner the action bands call. */
#ifndef MHTRI_LOBBY_FN_801E0ADC_H
#define MHTRI_LOBBY_FN_801E0ADC_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {

/* 0x801E2D04 - spawns effect `id` at `joint`'s world position when it passes the height test, with `pos` and `scale`
 * in its work block and `life` in the record (the float precedes the integer: the callers load it first). */
void eft_em_spawn_joint(struct _ENEMY_WORK* self, u32 joint, u32 id, nw4r::math::VEC3* pos, f32 scale, u32 life);
}
#endif

#endif /* MHTRI_LOBBY_FN_801E0ADC_H */
