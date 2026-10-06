/* g3d/g3d_cpu.h - the 32-byte block copy fn_8009A748 and 0.0f fill fn_8009A910 `g3d/g3d_cpu.cpp` owns (C linkage). */
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
