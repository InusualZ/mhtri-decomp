/* light/light_point_color_get.h - the declaration of `light_point_color_get`, which `light/light.cpp` owns
 * (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_LIGHT_LIGHT_POINT_COLOR_GET_H
#define MHTRI_LIGHT_LIGHT_POINT_COLOR_GET_H

#include "types.h"
#include "nw4r/math.h"

struct _GXColor;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x802C21B8: the nearest point light's colour at `pos` in `out`; 0 when no point light is registered. */
s32 light_point_color_get(nw4r::math::VEC3* pos, struct _GXColor* out);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_POINT_COLOR_GET_H */
