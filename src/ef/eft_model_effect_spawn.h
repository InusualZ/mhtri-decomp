/* Leaf header (docs/plan.md 6.5 rule 2): `eft_model_effect_spawn` (0x800FC0F0), defined by `src/ef/eft002.cpp`. */
#ifndef MHTRI_EF_EFT_MODEL_EFFECT_SPAWN_H
#define MHTRI_EF_EFT_MODEL_EFFECT_SPAWN_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void eft_model_effect_spawn(nw4r::math::VEC3* pos, u32 type, u8 field_08, u8 area, struct _CP_VECTOR* rot, f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_MODEL_EFFECT_SPAWN_H */
