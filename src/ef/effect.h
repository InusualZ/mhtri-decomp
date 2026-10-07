/* ef/effect.h - the two symbols `ef/effect.cpp` publishes to its ef siblings (docs/plan.md 6.5 rule 2), declared
 * with the callers' pointer views (`struct _EFT*`, `nw4r::ef::Effect*`); the ABI is the same pointer register. */
#ifndef MHTRI_EF_EFFECT_H
#define MHTRI_EF_EFFECT_H

#include "types.h"
#include "nw4r/math.h"

struct _EFT;

#ifdef __cplusplus
namespace nw4r {
namespace ef {
struct Effect;
}  // namespace ef
}  // namespace nw4r

extern "C" {
#endif

/* ORs the two flag bytes into the effect state's `flags_0x04` (the pool's per-effect state updater). */
void eft_state_flags_set(struct _EFT* self, u8 a, u8 b);

/* Calls vtable slot 6 of the pooled effect with `flag != 0` (its retire/flag hook). */
void effect_retire(nw4r::ef::Effect* effect, u32 arg);

/* Places/moves the pooled effect at a world position (the enemy/emitter state-0 handlers and
 * eft019's creation path), and reports whether the pooled effect is still alive. */
void fn_800F975C(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
s32 fn_800F9884(nw4r::ef::Effect* effect);
/* 0x800F9D80 - the effect's state/frame report the kind-1 state-0 body of `ef/fn_8030681C.cpp` switches on (it
 * compares it against 1). */
s32 eft_water_state_ck(_EFT* self);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800F9920 - steps the pooled effect one frame and reports whether it is still alive; the map spells
 * it `effect_move__FPQ34nw4r2ef6Effect`, so it is a C++ free function (rule 9);
 * `stage/shell.cpp`'s `shell_draw_set_eff` drives it. */
u32 effect_move(nw4r::ef::Effect* effect);
/* 0x800F96E0 - sets the pooled effect's root matrix translation to `pos` (`SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3`). */
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
/* 0x800F9AC0 - sets the pooled effect's parameter scale (`change_paramscale_eff__FPQ34nw4r2ef6Effectf`). */
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
#endif

#endif /* MHTRI_EF_EFFECT_H */
