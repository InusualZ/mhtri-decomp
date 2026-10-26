/* ef/change_color_eff.h - leaf header: `change_color_eff` (0x800F9988), owned by `ef/effect.cpp`. C++ linkage: the map
 * carries `change_color_eff__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC38_GXColor`. */
#ifndef MHTRI_EF_CHANGE_COLOR_EFF_H
#define MHTRI_EF_CHANGE_COLOR_EFF_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"

namespace nw4r { namespace ef { struct Effect; } }

/* Recolours `effect` at `pos` with `color`. */
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);

#endif
