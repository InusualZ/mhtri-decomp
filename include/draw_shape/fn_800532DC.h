/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `fn_800532DC` (0x800532DC), the 3x4 matrix copy that `draw_shape.cpp`'s range owns: the SE cluster
 * (`sound/fn_800D7F54.cpp`) and the emitter object layer (`ef/ef_emitter.cpp`) call it, and neither can include the owner's full header.
 */
#ifndef MHTRI_DRAW_SHAPE_FN_800532DC_H
#define MHTRI_DRAW_SHAPE_FN_800532DC_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_800532DC(Mtx34* dst, Mtx34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAW_SHAPE_FN_800532DC_H */
