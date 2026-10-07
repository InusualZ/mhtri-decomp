/* ef/change_color_scale_eff.h - leaf header: `change_color_scale_eff` (0x800F99D4), owned by `ef/effect.cpp`. */
#ifndef MHTRI_EF_CHANGE_COLOR_SCALE_EFF_H
#define MHTRI_EF_CHANGE_COLOR_SCALE_EFF_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Hands a colour/scale request to every particle manager of `effect`. */
void change_color_scale_eff(nw4r::ef::Effect* effect, u8 mode, _GXColor* color, _GXColor* color2,
                            nw4r::math::VEC3* pos, u8 flag, f32 scale);
#ifdef __cplusplus
}
#endif

#endif
