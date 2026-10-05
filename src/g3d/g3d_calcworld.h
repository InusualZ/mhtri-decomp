/* The g3d world-matrix unit `g3d/g3d_calcworld.cpp`.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_G3D_G3D_CALCWORLD_H
#define MHTRI_G3D_G3D_CALCWORLD_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void addVec3To(void* dst, const void* src);
void addVec3To(void* dst, const void* src);

/* The matrix-id flag helpers `fn_800737CC` defines (they set/clear the mode bits of a matrix id).
 * Consumers: `src/g3d/fn_800D77B0.cpp` (the per-node transform), from docs/plan.md 6.5 rule 2. */
u32 fn_800737AC(u32 value);
u32 fn_800737B4(u32 value);
u32 fn_800737BC(u32 value);
u32 fn_800737C4(u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCWORLD_H */
