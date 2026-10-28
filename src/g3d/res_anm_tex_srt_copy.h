/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the texture-SRT animation handle's copy and validity test, owned by
 * `g3d/g3d_anmtexsrt.cpp` (C linkage there).  GUESS names `res_anm_tex_srt_copy`/`res_anm_tex_srt_is_valid`: the
 * first word-copies the handle and returns `self` (`AnmObjTexSrtRes`'s constructor copies `mRes` with it), the second
 * returns whether the handle's word is set (`AnmObjTexSrtRes::Construct` tests the resource with it).
 */
#ifndef MHTRI_G3D_RES_ANM_TEX_SRT_COPY_H
#define MHTRI_G3D_RES_ANM_TEX_SRT_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006D000 (0x30): Copies the handle `src` into `self` and returns `self`. */
/* untyped: opaque handle - the two `ResAnmTexSrt` handles, as the owner defines them */
void* res_anm_tex_srt_copy(void* self, void* src);
/* 0x8006CE34 (0x14): Tells whether the handle is set. */
/* untyped: opaque handle - the `ResAnmTexSrt` handle, as the owner defines it */
u32 res_anm_tex_srt_is_valid(void* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_ANM_TEX_SRT_COPY_H */
