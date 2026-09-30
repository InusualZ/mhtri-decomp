/*
 * Declarations owned by `lobby/fn_8030121C.cpp` (docs/plan.md 6.5 rule 2).  The band (0x8030121C..0x803066F0)
 * also holds the single-effect spawners the enemy action bands call.
 */
#ifndef MHTRI_LOBBY_FN_8030121C_H
#define MHTRI_LOBBY_FN_8030121C_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {

/* 0x80304508 - the effect spawner beside `eft_em_spawn_param`: r3 the enemy work, r4 the effect id (the only
 * argument its body reads before tail-calling the effect allocator), `joint` the attach joint, `pos` an
 * optional offset and `scale` the size. */
void eft_em_spawn(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale);
}
#endif

#endif /* MHTRI_LOBBY_FN_8030121C_H */
