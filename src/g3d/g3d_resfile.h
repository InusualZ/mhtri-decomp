/* g3d/g3d_resfile.h - the cross-unit declarations of `g3d/g3d_resfile.cpp`. */
#ifndef MHTRI_G3D_G3D_RESFILE_H
#define MHTRI_G3D_G3D_RESFILE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80094464 - calls display list `pList` of `size` bytes by writing the call command straight into the GX FIFO. */
void GXFastCallDisplayList(const void* pList, u32 size); /* untyped: byte range */

/* The material display-list blocks' range stores (a nonzero flag waits for the store) and 0x20-aligned copies;
 * each takes the block's one-word handle. */
void res_mat_pix_dc_store(struct ResHandle* pSelf, s32 flag);
void res_mat_tev_color_dc_store(struct ResHandle* pSelf, s32 flag);
void res_mat_ind_mtx_dc_store(struct ResHandle* pSelf, s32 flag);
void res_mat_tex_coord_gen_dc_store(struct ResHandle* pSelf, s32 flag);
u32 res_mat_pix_copy_to(struct ResHandle* pSelf, u32 pDst);
u32 res_mat_tev_color_copy_to(struct ResHandle* pSelf, u32 pDst);
u32 res_mat_ind_mtx_copy_to(struct ResHandle* pSelf, u32 pDst);
u32 res_mat_tex_coord_gen_copy_to(struct ResHandle* pSelf, u32 pDst);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESFILE_H */
