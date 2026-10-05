/*
 * The `g3d/g3d_cpu.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d/g3d_cpu.cpp` (`.text` 0x8009A748-0x8009AA78) owns the two CPU-side display-list block
 * primitives: the 32-byte copy (`fn_8009A748`) and the 32-byte 0.0f fill (`fn_8009A910`).  The copy is
 * called from the g3d render/resource band (fn_80075DCC, g3d_state.cpp, g3d_resfile.cpp and the
 * g3d_resmat accessor cluster), so it is declared once here (the owner's header) and those consumers
 * include it.
 *
 * It keeps C linkage (its map names are plain `fn_XXXXXXXX` stems).
 */
#ifndef MHTRI_G3D_G3D_CPU_H
#define MHTRI_G3D_G3D_CPU_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_8009A748(void* pDst, const void* pSrc, u32 size); /* 0x8009A748 - copy size bytes as 32-byte blocks */
void fn_8009A910(void* pDst, u32 size);                   /* 0x8009A910 - fill size bytes with 0.0f */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CPU_H */
