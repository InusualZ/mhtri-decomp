/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `GXSetTexCoordGen2`: the map's address for the SDK function falls in `EXI/ProbeBarnacle.c`'s
 * registered range (an attribution the symbol map carries), so that unit is its owner here; the units that call it (the GX pipe helpers of
 * `fn_80047398.cpp`, `userdata_item.cpp` and `draw_shape_arm.cpp`) include this declaration instead of spelling it.
 */
#ifndef MHTRI_EXI_GXSETTEXCOORDGEN2_H
#define MHTRI_EXI_GXSETTEXCOORDGEN2_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void GXSetTexCoordGen2(u32 a, u32 b, u32 c, u32 d, u32 e, u32 f);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EXI_GXSETTEXCOORDGEN2_H */
