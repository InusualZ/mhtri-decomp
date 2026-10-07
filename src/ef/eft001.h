/* ef/eft001.h - the declarations of the `ef/eft001.cpp` symbols its ef siblings call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT001_H
#define MHTRI_EF_EFT001_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Writes a VEC3 into an MTX34's translation column (`m[0][3]`, `m[1][3]`, `m[2][3]`); the family's
 * spawn handlers call it after they build a rotation matrix for a placed model. */
void mtx34_set_trans(nw4r::math::MTX34* m, nw4r::math::VEC3* v);

/* Copies a three-word rotation vector into a `_CP_VECTOR` (`ef/eft007.cpp` and `ef/eft019.cpp` are the
 * consumers; the owner defines it over the same two pointers). */
struct _CP_VECTOR;
void eft_rot_vec_copy(struct _CP_VECTOR* dst, struct _CP_VECTOR* src);

/* 0x800FC0F0 - the effect spawn the player's act frame step issues from the player's joint position
 * (`Pl/fn_80273B14.cpp`). */
void fn_800FC0F0(nw4r::math::VEC3* pos, u32 type, u32 field_08, u32 area, _CP_VECTOR* rot,
                 f32 scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT001_H */
