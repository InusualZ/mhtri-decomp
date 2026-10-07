/* g3d/g3d_resnode.h - the nw4r g3d `ResNode` records more than one unit reads: `ResNodeData` (the +0x14 flag word,
 *   the +0x18 matrix id, the +0x20/+0x2C/+0x38 vectors) and `AnmResult` (scale at +0x04, Euler angles at +0x10, the
 *   3x4 matrix at +0x1C).  `g3d/g3d_calcworld.cpp` still defines its own `NodeMtxRec`. */
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
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ s32 mToResName;  /* offset from this block to the node name's characters (the length word is 4 before) */
    /* +0x0C */ u32 mNodeID;  /* the node's index in its model */
    /* +0x10 */ u8 pad_0x10[0x4];
    /* +0x14 */ u32 mFlags;   /* the channel flags fn_80099134/9178/9278 set and clear */
    /* +0x18 */ u32 mMtxID;   /* the matrix slot the node was assigned (read by g3d_calcworld.cpp) */
    /* +0x1C */ u32 mBillboardRefNodeID; /* the node a billboard reference follows (flag 0x400) */
    /* +0x20 */ f32 mScale[3];
    /* +0x2C */ f32 mRotate[3];
    /* +0x38 */ f32 mTranslate[3];
    /* +0x44 */ u8 pad_0x44[0x5C - 0x44];
    /* +0x5C */ s32 mToParentNode;  /* offset from this block to the parent's, 0 for none */
    /* +0x60 */ s32 mToChildNode;
    /* +0x64 */ s32 mToNextSibling;
    /* +0x68 */ s32 mToPrevSibling;
    /* +0x6C */ u32 mSubResOfs; /* the offset fn_80099378 resolves a sub-resource through */
}; /* size: 0x70 (a lower bound: only the fields above are reached) */

#ifdef __cplusplus
#include "g3d/g3d_rescommon.h"

namespace nw4r {
namespace g3d {

/* A one-word handle on a `ResNodeData`.  size: 0x4 */
class ResNode : public ResCommon<ResNodeData> {
public:
    /* untyped: opaque handle */
    explicit ResNode(void* pData);
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_RESNODE_H */
