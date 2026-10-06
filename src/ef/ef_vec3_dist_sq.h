/*
 * Leaf header (docs/plan.md 6.5 rule 2) for two helpers `ef/ef_emitter.cpp` owns, spelled as the owner defines them.
 */
#ifndef MHTRI_EF_EF_VEC3_DIST_SQ_H
#define MHTRI_EF_EF_VEC3_DIST_SQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A8300 (0x4C): the squared distance between two positions. */
f32 ef_vec3_dist_sq(void* a, void* b); /* untyped: opaque handle - the owner reads them as packed position records */
/* 0x800A89A0 (0x58): copies the twelve words of a 3x4 matrix and returns `dst`. */
void* ef_mtx34_copy(void* dst, const void* src); /* untyped: byte range - a 3x4 matrix copied word by word */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_VEC3_DIST_SQ_H */
