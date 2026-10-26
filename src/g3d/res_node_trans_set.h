/* Leaf header (docs/plan.md 6.5 rule 2): `res_node_trans_set`, `res_node_rot_set`, which `g3d/g3d_resnode` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_G3D_RES_NODE_TRANS_SET_H
#define MHTRI_G3D_RES_NODE_TRANS_SET_H

#include "types.h"
#include "g3d/g3d_resnode.h"

#ifdef __cplusplus
extern "C" {

/* 0x80099178 - writes the translation of the resource node `node`. */
void res_node_trans_set(nw4r::g3d::ResNode* node, f32 x, f32 y, f32 z);

/* 0x80099278 - writes the rotation (degrees) of the resource node `node`. */
void res_node_rot_set(nw4r::g3d::ResNode* node, f32 x, f32 y, f32 z);

}
#endif

#endif /* MHTRI_G3D_RES_NODE_TRANS_SET_H */
