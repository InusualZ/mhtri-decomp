/*
 * The `g3d/g3d_state.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d/g3d_state.cpp` (`.text` 0x8008452C-0x800898B0) owns the g3d render-state cluster.
 * `g3d/g3d_resfile.cpp`'s accessor cluster calls its resource-range store helper, so it is declared
 * once here (the owner's header) and that consumer includes it.
 *
 * It keeps C linkage (its map name is a plain `fn_XXXXXXXX` stem).
 */
#ifndef MHTRI_G3D_G3D_STATE_H
#define MHTRI_G3D_G3D_STATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_80089690(void* pBase, u32 size); /* 0x80089690 - the resource-range store */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_H */
