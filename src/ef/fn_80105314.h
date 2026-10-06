/* ef/fn_80105314.h - the declarations of `ef/fn_80105314.cpp`'s symbols the enemy/effect units call (docs/plan.md
 * 6.5 rule 2), with the callers' `void*` views of the record and the position (the same pointer registers). */
#ifndef MHTRI_EF_FN_80105314_H
#define MHTRI_EF_FN_80105314_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* The per-frame handler of the enemy-effect controller (state 0 of `eft009`'s dispatcher). */
void fn_80105314(void* self);

/* `state_0x05++` and the destroy hook of the same controller. */
void fn_80105550(void* self);
void fn_80105560(void* self);

/* The enemy effect setter (position, scale, trailing id) and the id-only setter. */
void fn_801057A4(void* self, u32 a, void* v, f32 scale, u32 id);
void fn_8010A7D4(void* self, u32 a);
/* 0x8010562C - this unit's joint-effect spawner: r3 (`self`), r4/r5 two scalars, r6 the `VEC3*` and f1 the scale
 * (`enemy/em009_act.cpp` calls it). */
void eft_spawn_type10(struct _ENEMY_WORK* self, u32 type, u32 a, nw4r::math::VEC3* b, f32 scale);
/* 0x80106694 - the second spawner of the same band: r3 (`self`), r4 the `VEC3*`, r5 a byte kind and f1 the scale
 * (`enemy/em009_act.cpp` calls it). */
void eft_spawn_type11(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u8 kind, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_80105314_H */
