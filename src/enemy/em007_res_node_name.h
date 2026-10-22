/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em007_act.cpp`'s `em007_res_node_name`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_EM007_RES_NODE_NAME_H
#define MHTRI_ENEMY_EM007_RES_NODE_NAME_H

#include "types.h"

struct ResHandle;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E0A84 - the name of resource node `node`. */
const char* em007_res_node_name(const struct ResHandle* node);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM007_RES_NODE_NAME_H */
