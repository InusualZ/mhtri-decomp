/* Leaf header for the two joint-matrix accessors `enemy/fn_80138074.c` owns (0x80139A64, 0x80139A7C); the
 * owner is a C file whose declarations take `void*`.  They read and write the matrix of one joint of a character's model.
 */
struct MtxHolder;

#ifndef MHTRI_ENEMY_JOINT_MTX_STORE_H
#define MHTRI_ENEMY_JOINT_MTX_STORE_H

#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80139A64 - stores the matrix `mtx` into the joint `holder` names. */
void joint_mtx_store(struct MtxHolder* holder, nw4r::math::MTX34* mtx);
/* 0x80139A7C - reads the joint's matrix into `mtx`. */
void joint_mtx_load(struct MtxHolder* holder, nw4r::math::MTX34* mtx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_JOINT_MTX_STORE_H */
