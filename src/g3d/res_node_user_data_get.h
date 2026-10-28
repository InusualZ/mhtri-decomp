/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_resnode.cpp`'s `res_node_user_data_get` (`ResNode::GetResUserData`).
 * C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_RES_NODE_USER_DATA_GET_H
#define MHTRI_G3D_RES_NODE_USER_DATA_GET_H

#include "types.h"

struct ResHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80099378 (0x3C): returns the word of the node's user-data handle (the block at the node's +0x6C offset). */
u32 res_node_user_data_get(struct ResHandle* pSelf);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_NODE_USER_DATA_GET_H */
