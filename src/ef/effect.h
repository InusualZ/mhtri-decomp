/*
 * Declarations owned by `ef/effect.cpp` (docs/plan.md 6.5 rule 2): the two symbols the effect manager
 * publishes to its ef siblings.  A consumer includes this header instead of declaring them itself.
 *
 * The owner's pointer parameters are its private `EftFrameState*`/`EftHandle*`; the declarations below
 * take the callers' views (`struct _EFT*` from ef.h, `nw4r::ef::Effect*`), which is the same pointer
 * register through the extern "C" name, so nothing but the header changes at the call sites.
 */
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
void fn_800F996C(nw4r::ef::Effect* effect, u32 arg);

/* Places/moves the pooled effect at a world position (the enemy/emitter state-0 handlers and
 * eft019's creation path), and reports whether the pooled effect is still alive. */
void fn_800F975C(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
s32 fn_800F9884(nw4r::ef::Effect* effect);
/* 0x800F9D80 - the effect's state/frame report the kind-1 state-0 body switches on (it compares it
 * against 1).  Added with `ef/fn_8030681C.cpp`. */
s32 fn_800F9D80(_EFT* self);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800F9920 - steps the pooled effect one frame and reports whether it is still alive; the map spells
 * it `effect_move__FPQ34nw4r2ef6Effect`, so it is a C++ free function (rule 9);
 * `stage/shell.cpp`'s `shell_draw_set_eff` drives it. */
u32 effect_move(nw4r::ef::Effect* effect);
#endif

#endif /* MHTRI_EF_EFFECT_H */
