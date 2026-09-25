/* The g3d camera-animation unit `g3d/g3d_resanmcamera.cpp` (0x8008A220-0x8008A664; the earlier
 * `.c` spelling covered only 0x8008A220-0x8008A28C and was merged into this one TU 2026-09-24).
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_G3D_G3D_RESANMCAMERA_H
#define MHTRI_G3D_G3D_RESANMCAMERA_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_8008A220(void* out, s32 arg1);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESANMCAMERA_H */
