/*
 * The nw4r g3d `ResNode` records that **more than one unit** reaches (docs/plan.md 6.5 rule 1).
 *
 * `ResNodeData` is the node's resource block: `g3d/g3d_calcworld.cpp` already reads its +0x18 matrix-id
 * word through `fn_8005D0C4`, and `g3d/g3d_resnode.cpp` (this file's first consumer) reads the +0x14
 * flag word and the +0x20/+0x2C/+0x38 vectors.  The nw4r name is `g3d_resnode.h`'s, so the record lives
 * here once and is included where needed.  `g3d/g3d_calcworld.cpp` still carries its own partial 0x1C
 * view of the same name in its source (it pre-dates this header); folding it onto this definition is a
 * residual for the conformance sweep, exactly as `include/nw4r/g3d/res_anm.h` records for its two
 * records - the layouts agree at +0x18, so the fold is mechanical.
 *
 * `AnmResult` is the animation-result record the node reads and writes (`g3d_calcworld.cpp` builds the
 * same record under its own `NodeMtxRec` name).  Its layout is the target's: the three scale floats at
 * +0x04, the three Euler angles at +0x10, and the 3x4 matrix at +0x1C whose translation column is
 * `mtx[3]`/`mtx[7]`/`mtx[11]` (+0x28/+0x38/+0x48).
 */
#ifndef MHTRI_G3D_G3D_RESNODE_H
#define MHTRI_G3D_G3D_RESNODE_H

#include "types.h"

/* The animation result one node's pass fills in.  `flags` selects which channels the record carries;
 * the target tests 0x80 (scale), 0x100, 0x200 and writes 0x20/0x40/0x8000/0x400/0x800 beside them. */
struct AnmResult {
    /* +0x00 */ u32 flags;
    /* +0x04 */ f32 scale[3];
    /* +0x10 */ f32 rotate[3];
    /* +0x1C */ f32 mtx[12]; /* the rotation matrix; the translation column is mtx[3]/mtx[7]/mtx[11] */
}; /* size: 0x4C */

/* The node resource block `fn_8005D0C4`/`fn_8005D218` hand back.  Only the fields the reconstructed
 * bodies reach are named; the untouched runs keep their offsets as padding. */
struct ResNodeData {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u32 mFlags;   /* the channel flags fn_80099134/9178/9278 set and clear */
    /* +0x18 */ u32 mMtxID;   /* the matrix slot the node was assigned (read by g3d_calcworld.cpp) */
    /* +0x1C */ u8 pad_0x1C[0x20 - 0x1C];
    /* +0x20 */ f32 mScale[3];
    /* +0x2C */ f32 mRotate[3];
    /* +0x38 */ f32 mTranslate[3];
    /* +0x44 */ u8 pad_0x44[0x6C - 0x44];
    /* +0x6C */ u32 mSubResOfs; /* the offset fn_80099378 resolves a sub-resource through */
}; /* size: 0x70 (a lower bound: only the fields above are reached) */

#endif /* MHTRI_G3D_G3D_RESNODE_H */
