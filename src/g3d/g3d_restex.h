/* g3d/g3d_restex.h - the C helpers `g3d/g3d_restex.cpp` defines over the nw4r g3d `ResTex` and `ResPltt`
 *   handles (the classes themselves are declared in `g3d/g3d_resshp.h`). */
#ifndef MHTRI_G3D_G3D_RESTEX_H
#define MHTRI_G3D_G3D_RESTEX_H

#include "types.h"

struct ResHandle;

extern "C" {
/* The palette block's data-cache store (`flag` != 0: with sync), its revision word and whether that
 * revision is one the library reads (1 or 3); each takes the resource's one-word handle. */
void res_pltt_dc_store(struct ResHandle* pSelf, s32 flag);
u32 res_pltt_check_revision(struct ResHandle* pSelf);
u32 res_pltt_get_revision(struct ResHandle* pSelf);
/* The texture block's revision word, its revision check (1 or 3) and the main-memory flush of its `mSize`
 * bytes. */
u32 res_tex_check_revision(struct ResHandle* pSelf);
u32 res_tex_get_revision(struct ResHandle* pSelf);
void res_tex_dc_flush(struct ResHandle* pSelf);
}

#endif /* MHTRI_G3D_G3D_RESTEX_H */
