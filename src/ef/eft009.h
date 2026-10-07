/* ef/eft009.h - the declarations of `ef/eft009.cpp`'s symbols its consumers call (docs/plan.md 6.5 rule 2); where
 * consumers' spellings differed only in parameter names the wider form is kept. */
#ifndef MHTRI_EF_EFT009_H
#define MHTRI_EF_EFT009_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;
struct _CP_VECTOR;

#ifdef __cplusplus
extern "C" {
#endif

/* The real signature, from the owner's own definition (`src/ef/eft009.cpp`:
 * `void eft009_spawn_at_pos(_ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, f32 scale,
 * nw4r::math::VEC3* pos)`; every caller loads the scale before the position).  C++ gets it because the enemy
 * programs (`enemy/em001_prog.cpp`, `enemy/em029_prog.cpp`, ...) call it with six arguments; the C consumers
 * keep the old-style declaration (they call it with six as well).  One view per TU:
 * declaring both spellings is `(10197) illegal function overloading`. */
#ifdef __cplusplus
void eft009_spawn_at_pos(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta,
                 f32 scale, nw4r::math::VEC3* pos);
#else
void eft009_spawn_at_pos();
#endif


/* The family's handlers and hooks. */
void fn_80103D28(void* self);
void fn_801041BC(void* self);
void fn_801048A0(void* self);
void fn_801048B0(void* self);
/* 0x801048B4 - the single-effect spawner (id/type/joint-delta/scale), defined `extern "C"` in `ef/eft009.cpp`;
 * `enemy/fn_801BD6C0.cpp` calls it. */
void eft009_spawn_at_joint(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, f32 scale);
#ifdef __cplusplus
}  /* the map name below is a mangling, so it sits outside the C-linkage block */
/* 0x8010494C - the owner's C++ definition `eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl`, declared at C++
 * scope so a caller never spells the mangling (rule 9); `enemy/em009_act.cpp` spawns through it. */
void eft009_set_pos(u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 id);
#endif

#endif /* MHTRI_EF_EFT009_H */
