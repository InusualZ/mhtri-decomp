/*
 * Declarations owned by `ef/fn_80105314.cpp` (docs/plan.md 6.5 rule 2): the symbols the enemy/effect
 * units call that this unit defines.  A consumer includes this header instead of declaring them itself.
 *
 * The parameters are the callers' views (`void*` for the effect/enemy record, `void*` for the position
 * triple); the ABI is the same pointer register, so nothing but the header changes at the call sites.
 */
#ifndef MHTRI_EF_FN_80105314_H
#define MHTRI_EF_FN_80105314_H

#include "types.h"

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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_80105314_H */
