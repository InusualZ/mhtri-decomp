/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `PSVECSubtract` (0x80050CF4), the SDK vector subtract that `fn_8004CAD8.cpp`'s range owns, in the
 * `Vec*` spelling the `ef` draw-strategy units call it with (the owner's full header `fn_8004CAD8.h` spells it with `f32*`).
 */
#ifndef MHTRI_FN_8004CAD8_PSVECSUBTRACT_H
#define MHTRI_FN_8004CAD8_PSVECSUBTRACT_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void PSVECSubtract(Vec* out, Vec* a, Vec* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_8004CAD8_PSVECSUBTRACT_H */
