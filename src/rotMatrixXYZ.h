/* rotMatrixXYZ.h - leaf header: `rotMatrixXYZ` (0x80050620), owned by `fn_8004CAD8.cpp` (the nw4r math band at the
 * root of src/). C linkage: the map carries the plain name. */
#ifndef MHTRI_ROTMATRIXXYZ_H
#define MHTRI_ROTMATRIXXYZ_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Applies the X, Y and Z rotations of the angle triple `ang` to `mtx`. */
void rotMatrixXYZ(_CP_VECTOR* ang, MTX34* mtx);
#ifdef __cplusplus
}
#endif

#endif
