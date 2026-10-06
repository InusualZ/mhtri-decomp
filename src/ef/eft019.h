/* ef/eft019.h - the declarations of `ef/eft019.cpp`'s symbols, from the owner's own bodies (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT019_H
#define MHTRI_EF_EFT019_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* 0x80112610 - the family's core spawner the sibling effect units reuse (`eft019_set_core__FPQ34nw4r4math4VEC3UcUcff`):
 * the position, the area byte, the parameter id and the two scale factors; C++ scope (rule 9).  `ef/eft035.cpp`
 * calls it. */
void eft019_set_core(nw4r::math::VEC3* pos, u8 area, u8 param, f32 scale_a, f32 scale_b);

/* 0x801121DC - the family's short entry point (`eft019_set__FPQ34nw4r4math4VEC3UcUc`): the position, the area byte
 * and the parameter id; C++ scope (rule 9).  `enemy/em035_prog.cpp` calls it. */
void eft019_set(nw4r::math::VEC3* pos, u8 area, u8 param);

/* 0x80112664 - the vector-carrying spawner (`eft019_set_vec__FPQ34nw4r4math4VEC3P10_CP_VECTORUcUcf`): the position,
 * the rotation triple, the area byte, the parameter id and one scale.  `enemy/em024_ai.cpp` calls it. */
struct _CP_VECTOR;
void eft019_set_vec(nw4r::math::VEC3* pos, struct _CP_VECTOR* rot, u8 area, u8 type, f32 scale);

#endif

#endif /* MHTRI_EF_EFT019_H */
