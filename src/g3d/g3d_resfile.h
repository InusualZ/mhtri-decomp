/* g3d/g3d_resfile.h - the cross-unit declarations of `g3d/g3d_resfile.cpp`. */
#ifndef MHTRI_G3D_G3D_RESFILE_H
#define MHTRI_G3D_G3D_RESFILE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80094464 - calls display list `pList` of `size` bytes by writing the call command straight into the GX FIFO. */
void GXFastCallDisplayList(const void* pList, u32 size); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESFILE_H */
