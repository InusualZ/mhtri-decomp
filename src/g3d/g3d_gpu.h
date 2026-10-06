/* g3d/g3d_gpu.h - the fifo matrix writers `g3d/g3d_gpu.cpp` defines and the records they read (caller:
 *   g3d/g3d_state.cpp). */
#ifndef MHTRI_G3D_G3D_GPU_H
#define MHTRI_G3D_G3D_GPU_H

#include "types.h"

/* The eight-setting record GDSetCurrentMtx packs.  Its own assert message names it ("Array8(=%p) is not
 * valid pointer."); the caller `g3d_tex_mtx_current_set` fills the eight words from a per-texgen table (the values
 * it writes - 0, 30, 60 - all fit the six bits the packing leaves each field).  The name of each field
 * is its slot in the two emitted command words: `lo_*` goes to the word at bits 0/6/12/18, `hi_*` to
 * the one at bits 6/12/18/24.
 * size: 0x20 (all eight words are read) */
struct Array8 {
    /* +0x00 */ u32 hi_bits_6;   /* -> second command word bits 6-11  */
    /* +0x04 */ u32 hi_bits_12;  /* -> second command word bits 12-17 */
    /* +0x08 */ u32 hi_bits_18;  /* -> second command word bits 18-23 */
    /* +0x0C */ u32 hi_bits_24;  /* -> second command word bits 24-29 */
    /* +0x10 */ u32 lo_bits_0;   /* -> first command word bits 0-5    */
    /* +0x14 */ u32 lo_bits_6;   /* -> first command word bits 6-11   */
    /* +0x18 */ u32 lo_bits_12;  /* -> first command word bits 12-17  */
    /* +0x1C */ u32 lo_bits_18;  /* -> first command word bits 18-23  */
};

/* The stored 3x3 rotation GDLoadTexMtxImm3x3 expands.  The caller `g3d_tex_mtx_current_set` passes a view normal matrix
 * (`g3d_state_get_nrm_mtx`) and the body reads all nine floats as one contiguous 0x24 block, so it
 * is a 3x3 and not the 3x4 `MTX34` the expanded form uses.
 * size: 0x24 */
struct Mat33 {
    /* +0x00 */ f32 m[3][3];
};

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009B140 - writes the eight texgen matrix indices of `self` as the XF 0x1018 pair. */
void GDSetCurrentMtx(Array8* self);
/* 0x8009B2CC - loads the 3x3 `pSrc`, expanded to 3x4, as texture matrix `id`. */
void GDLoadTexMtxImm3x3(const Mat33* pSrc, u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_GPU_H */
