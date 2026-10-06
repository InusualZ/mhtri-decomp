/* ef/ef_util.h - the symbols `ef/ef_util.cpp` owns: the matrix-axis scale helper `ef_mtx34_column_length` at the range's
 * tail (0x8009CD64-0x8009CDBC), which `ef/ef_emitter.cpp` calls. */
#ifndef MHTRI_EF_EF_UTIL_H
#define MHTRI_EF_EF_UTIL_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 ef_mtx34_column_length(const f32* mtx, s32 index);

/* The rotation record ef_vec3_from_rotation reads: three angles. */
struct EfRotation {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
}; /* size: 0xC */

/* 0x8009B448 - builds into `mtx` an orthonormal frame whose Y axis is the unit direction `dir`. */
void ef_mtx34_from_y_axis(f32* mtx, const f32* dir);

/* 0x8009C7D4 - writes the unit direction the rotation angles of `rot` give into `out` and returns `out`. */
VEC3* ef_vec3_from_rotation(const struct EfRotation* rot, VEC3* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_UTIL_H */
