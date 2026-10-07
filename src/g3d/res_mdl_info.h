/* g3d/res_mdl_info.h - the model info block `res_mdl_info_data` (g3d/g3d_calcview.cpp) returns. */
#ifndef MHTRI_G3D_RES_MDL_INFO_H
#define MHTRI_G3D_RES_MDL_INFO_H

#include "types.h"

/* The model info block `res_mdl_info_data` returns: the node and matrix counts, the envelope mode and the offset
 * of the position/normal matrix table.  size: 0x28 (a lower bound: only these fields are reached) */
struct ResMdlInfoData {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ s32 mNumNode;
    /* +0x0C */ u8 pad_0x0C[0x10];
    /* +0x1C */ u32 mNumMtx;
    /* +0x20 */ u16 mUnk20;
    /* +0x22 */ u8 mUnk22;
    /* +0x23 */ u8 mEnvelopeMtxMode;
    /* +0x24 */ u32 mToPosNrmMtxTable; /* offset from this block to the position/normal matrix table */
};

#endif
