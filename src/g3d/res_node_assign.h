/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_anmchr.cpp`'s `res_node_assign`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_G3D_RES_NODE_ASSIGN_H
#define MHTRI_G3D_RES_NODE_ASSIGN_H

#include "types.h"

struct ResHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005D0CC - assigns resource node handle `src` to `dst`. */
void res_node_assign(struct ResHandle* dst, const struct ResHandle* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_RES_NODE_ASSIGN_H */
