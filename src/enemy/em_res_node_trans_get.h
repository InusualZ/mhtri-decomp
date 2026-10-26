/* Leaf header (docs/plan.md 6.5 rule 2): `em_res_node_trans_get`, `em_res_node_rot_get`, which `enemy/em_common` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_ENEMY_EM_RES_NODE_TRANS_GET_H
#define MHTRI_ENEMY_EM_RES_NODE_TRANS_GET_H

#include "types.h"
#include "g3d/g3d_resnode.h"

#ifdef __cplusplus
extern "C" {

/* 0x801360FC - the translation of the resource node `node`. */
const f32* em_res_node_trans_get(nw4r::g3d::ResNode* node);

/* 0x80136120 - the rotation of the resource node `node`. */
const f32* em_res_node_rot_get(nw4r::g3d::ResNode* node);

}
#endif

#endif /* MHTRI_ENEMY_EM_RES_NODE_TRANS_GET_H */
