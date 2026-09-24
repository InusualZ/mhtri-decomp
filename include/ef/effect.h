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
void fn_800F9DF4(struct _EFT* self, u8 a, u8 b);

/* Calls vtable slot 6 of the pooled effect with `flag != 0` (its retire/flag hook). */
void fn_800F996C(nw4r::ef::Effect* effect, u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFFECT_H */
