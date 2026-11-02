/* g3d/g3d_restev.h - the C helpers `g3d/g3d_restev.cpp` defines over the nw4r g3d `ResTev` handle (the
 *   class itself is declared in `g3d/g3d_resshp.h`). */
#ifndef MHTRI_G3D_G3D_RESTEV_H
#define MHTRI_G3D_G3D_RESTEV_H

#include "types.h"

struct ResHandle;

extern "C" {
/* The tev block's 0x200-byte copy to an aligned destination (returns the copy's handle word) and its
 * data-cache store (`flag` != 0: with sync); each takes the resource's one-word handle. */
u32 res_tev_copy_to(struct ResHandle* pSelf, void* pDst); /* untyped: byte range */
void res_tev_dc_store(struct ResHandle* pSelf, s32 flag);
}

#endif /* MHTRI_G3D_G3D_RESTEV_H */
