/* The effect unit `ef/eft009.cpp`.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
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
 * `void fn_801049D0(_ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, nw4r::math::VEC3* pos,
 * f32 scale)`).  C++ gets it because `enemy/fn_80147CE0.cpp` calls it with six arguments; the C
 * consumers keep the old-style declaration (they call it with six as well).  One view per TU:
 * declaring both spellings is `(10197) illegal function overloading`. */
#ifdef __cplusplus
void fn_801049D0(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta,
                 nw4r::math::VEC3* pos, f32 scale);
#else
void fn_801049D0();
#endif


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void fn_80103D28(void* self);
void fn_801041BC(void* self);
void fn_801048A0(void* self);
void fn_801048B0(void* self);
/* 0x801048B4 - the single-effect spawner (id/type/joint-delta/scale), defined `extern "C"` in
 * `src/ef/eft009.cpp`.  Added for `enemy/fn_801BD6C0.cpp` (rule 2: three consumer units used to
 * spell it locally). */
void eft009_spawn_at_joint(struct _ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, f32 scale);
#ifdef __cplusplus
}  /* the map name below is a mangling, so it sits outside the C-linkage block */
/* 0x8010494C - the owner's own C++-mangled definition `eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl`
 * (`void eft009_set_pos(u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 id)` in
 * `src/ef/eft009.cpp`), declared at C++ scope so a caller never spells the mangling (rule 9).  Added
 * with `enemy/fn_80387844.cpp`, whose effect branches spawn through it. */
void eft009_set_pos(u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 id);
#endif

#endif /* MHTRI_EF_EFT009_H */
