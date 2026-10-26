/* g3d/res_file_tex_by_index.h - leaf header (docs/plan.md 6.5 rule 2) for two `g3d/g3d_resfile.cpp` lookups:
 * `res_file_tex_by_index` (0x80092A54, `ResFile::GetResTex(int)`) and `res_file_pltt_by_index` (0x80092790,
 * `ResFile::GetResPltt(int)`) return the `index`-th texture / palette handle word of the file whose handle word
 * `file` points at (0 when the file has no such dictionary).  C linkage: the owner defines them `extern "C"`. */
#ifndef MHTRI_G3D_RES_FILE_TEX_BY_INDEX_H
#define MHTRI_G3D_RES_FILE_TEX_BY_INDEX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 res_file_tex_by_index(const u32* file, u32 index);
u32 res_file_pltt_by_index(const u32* file, u32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_FILE_TEX_BY_INDEX_H */
