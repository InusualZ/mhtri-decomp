/*
 * Leaf header (docs/plan.md 6.5 rule 2) for two matrix builders `ef/ef_util.cpp` owns, spelled as the owner defines
 * them (its consumers that declare them locally with other spellings do not include this header).
 */
#ifndef MHTRI_EF_EF_MTX34_SCALE_COLUMNS_H
#define MHTRI_EF_EF_MTX34_SCALE_COLUMNS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009CA30 (0x170): the rotation matrix of three Euler angles. */
void ef_mtx34_rotate_xyz(f32* mtx, f32 x, f32 y, f32 z);
/* 0x8009CC20 (0x8C): `mtx` with each column scaled by the matching component of `scale`, into `dst`. */
void ef_mtx34_scale_columns(f32* dst, const f32* scale, const f32* mtx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_MTX34_SCALE_COLUMNS_H */
