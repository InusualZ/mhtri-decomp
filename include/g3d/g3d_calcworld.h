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

void fn_80073F68(void* dst, const void* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CALCWORLD_H */
