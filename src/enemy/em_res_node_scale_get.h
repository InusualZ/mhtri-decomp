/* Leaf header (docs/plan.md 6.5 rule 2): `em_res_node_scale_get` (0x80136144), which `enemy/em_common` owns.  C linkage
 * (the map row is a plain name). */
#ifndef MHTRI_ENEMY_EM_RES_NODE_SCALE_GET_H
#define MHTRI_ENEMY_EM_RES_NODE_SCALE_GET_H

#include "types.h"
#include "g3d/g3d_resnode.h"

#ifdef __cplusplus
extern "C" {

/* 0x80136144 - the scale of the resource node `node`. */
const f32* em_res_node_scale_get(nw4r::g3d::ResNode* node);

}
#endif

#endif /* MHTRI_ENEMY_EM_RES_NODE_SCALE_GET_H */
