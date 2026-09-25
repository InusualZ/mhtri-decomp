/*
 * Declarations owned by `sound/fn_800D7F54.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.  Keep it minimal.
 */
#ifndef MHTRI_SOUND_FN_800D7F54_H
#define MHTRI_SOUND_FN_800D7F54_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The sound-module frame entry point `main.cpp` calls. */
void fn_800D8438(void);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void* fn_800DA72C(s32 kind, s32 id, Vec3* pos);
void fn_800DCF0C(s32 handle, Vec3* pos);
void fn_800DA864(Vec3* pos);
void fn_800DA8F4(Vec3* pos);
void fn_800DA93C(Vec3* pos);
void fn_800DA95C(Vec3* pos);
void fn_800DCB18(s32 id, Vec3* pos);
void fn_800DB608(u8 flag, Vec3* pos, u8 arg);
/* Merged 2026-09-25 (ef/fn_80105314 batch): the two per-position sound requests the enemy-effect
 * dispatch (`fn_80105564`) tail-calls. */
void fn_800DC60C(nw4r::math::VEC3* pos, u32 mode);
void fn_800DB964(nw4r::math::VEC3* pos);
/* Merged 2026-09-25 (ef/fn_80105314 batch): the per-position sound requests the enemy-effect
 * state-0 handlers call. */
void fn_800DC6D8(nw4r::math::VEC3* pos, u32 mode);
void fn_800DB974(void* src, nw4r::math::VEC3* pos);

/* Merged 2026-09-24: the per-material impact calls `ef/eft019.cpp`'s creation tail dispatches into. */
void fn_800DA8AC(Vec3* pos);
void fn_800DA9A4(Vec3* pos);
void fn_800DA9B4(Vec3* pos);
void fn_800DA9C4(Vec3* pos);
void fn_800DA9D4(Vec3* pos);
void fn_800DA9E4(Vec3* pos);
void fn_800DA9F4(Vec3* pos);
void fn_800DC46C(Vec3* pos);
void fn_800DC4B4(Vec3* pos);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800D7F54_H */
