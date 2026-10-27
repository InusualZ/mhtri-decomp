/* Leaf header (docs/plan.md 6.5 rule 2): `res_node_get_parent` (0x8005D124), owned by `g3d/g3d_anmchr.cpp`.  C linkage
 * (the map row is a plain name). */
#ifndef MHTRI_G3D_RES_NODE_GET_PARENT_H
#define MHTRI_G3D_RES_NODE_GET_PARENT_H

#include "types.h"

struct ResHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005D124 - the handle word of the node's parent. */
u32 res_node_get_parent(struct ResHandle* pSelf);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_NODE_GET_PARENT_H */
