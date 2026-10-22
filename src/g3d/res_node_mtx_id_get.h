/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_calcview.cpp`'s `res_node_mtx_id_get`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_RES_NODE_MTX_ID_GET_H
#define MHTRI_G3D_RES_NODE_MTX_ID_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006FDCC - the matrix id of resource node `p` (GUESS name); the owner's prototype (`g3d/g3d_calcview.h`). */
u32 res_node_mtx_id_get(const void* p); /* untyped: opaque handle - the ResNode handle */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_NODE_MTX_ID_GET_H */
