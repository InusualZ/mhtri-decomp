/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the resource file's texture-SRT accessors, owned by
 * `g3d/g3d_resfile.cpp` (C linkage there).  GUESS names `res_file_has_anm_tex_srt`, `res_file_get_res_anm_tex_srt_num`,
 * `res_file_get_res_anm_tex_srt`: the owner documents them as the `AnmTexSrt` category test,
 * `ResFile::GetResAnmTexSrtNumEntries()` and `ResFile::GetResAnmTexSrt(u32)` (siblings `res_file_has_anm_chr`,
 * `res_file_get_res_anm_chr_num`); `nw_resource.cpp`'s animation builders call them.
 */
#ifndef MHTRI_G3D_RES_FILE_HAS_ANM_TEX_SRT_H
#define MHTRI_G3D_RES_FILE_HAS_ANM_TEX_SRT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80093794 (0x78): Tells whether the file has an `AnmTexSrt` category. */
/* untyped: opaque handle - the `ResFile` handle, as the owner defines it */
u32 res_file_has_anm_tex_srt(void* self);
/* 0x80093394 (0x8C): Returns the number of texture-SRT animations in the file. */
/* untyped: opaque handle - the `ResFile` handle, as the owner defines it */
u32 res_file_get_res_anm_tex_srt_num(void* self);
/* 0x80093390 (0x4): Returns the data of the file's texture-SRT animation `i`. */
/* untyped: opaque handle - the `ResFile` handle and the animation's data, as the owner defines them */
void* res_file_get_res_anm_tex_srt(void* self, u32 i);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_FILE_HAS_ANM_TEX_SRT_H */
