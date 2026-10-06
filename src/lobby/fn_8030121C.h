/* Declarations owned by `lobby/fn_8030121C.cpp`: the single-effect spawners the enemy action bands call. */
#ifndef MHTRI_LOBBY_FN_8030121C_H
#define MHTRI_LOBBY_FN_8030121C_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {

/* 0x80304508 - spawns effect `id` for the enemy work at `joint` (`pos` an optional offset, `scale` the size); its body
 * reads only `id` before tail-calling the effect allocator. */
void eft_em_spawn(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale);
}
#endif

#endif /* MHTRI_LOBBY_FN_8030121C_H */
