/* Declarations owned by `ef/eft_em_tint.cpp`: the debris effect's tinted spawn the enemy part-break bands call. */
#ifndef MHTRI_EF_EFT_EM_TINT_H
#define MHTRI_EF_EFT_EM_TINT_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80305924 - spawns the debris effect `id` at joint `joint` of `self`, tinted `color`, for 10 frames. */
void eft_em_spawn_tinted(struct _ENEMY_WORK* self, u32 id, u32 joint, nw4r::math::VEC3* pos, f32 scale, u32 color);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_EM_TINT_H */
