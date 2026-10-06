/* Leaf declaration of `Pl/pl_act_step.cpp`'s hit-effect spawn (0x802462FC), which the net receivers replay: `pos`
 * is the impact position, `param` the effect parameter, `kind`/`area` the actor's kind and area bytes and `extra`
 * the effect variant flag.
 */
#ifndef MHTRI_PL_FN_802430E8_H
#define MHTRI_PL_FN_802430E8_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void pl_hit_effect_spawn(struct _PLW* self, nw4r::math::VEC3* pos, u16 param, u8 kind, u8 area, u8 extra);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_802430E8_H */
