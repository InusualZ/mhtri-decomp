/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft007.cpp`'s `eft007_set` (0x80102994, C++ linkage). */
#ifndef MHTRI_EF_EFT007_SET_H
#define MHTRI_EF_EFT007_SET_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
/* Starts an eft007 player-weapon effect of `type` at `vec` with the scale `scale`. */
void eft007_set(struct _PLW* self, u8 type, u8 colour, unsigned long id, nw4r::math::VEC3* vec, f32 scale);
#endif

#endif /* MHTRI_EF_EFT007_SET_H */
