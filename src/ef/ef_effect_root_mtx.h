/* ef/ef_effect_root_mtx.h - leaf header for `ef_effect_root_mtx`, which `src/ef/ef_effectsystem.cpp` defines with
 * C linkage (docs/plan.md 6.5 rule 2).  A leaf because `ef/ef_emitter.cpp` declares it over its own manager view
 * and includes `ef/ef_effectsystem.h`. */
#ifndef MHTRI_EF_EF_EFFECT_ROOT_MTX_H
#define MHTRI_EF_EF_EFFECT_ROOT_MTX_H

#include "types.h"
#include "nw4r/math.h"

namespace nw4r { namespace ef { struct Effect; } }

extern "C" {
/* 0x800A60C0 - the effect's root matrix (`Effect::mRootMtx`, +0x58). */
nw4r::math::MTX34* ef_effect_root_mtx(nw4r::ef::Effect* effect);
}

#endif /* MHTRI_EF_EF_EFFECT_ROOT_MTX_H */
