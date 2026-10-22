/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_resanmscn.cpp`'s `res_file_handle_copy`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_RES_FILE_HANDLE_COPY_H
#define MHTRI_G3D_RES_FILE_HANDLE_COPY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80091364 - copies the one-word resource handle `src` into `dst` (GUESS name). */
void res_file_handle_copy(u32* dst, const u32* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_FILE_HANDLE_COPY_H */
