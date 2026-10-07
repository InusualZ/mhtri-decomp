/* rotLocalMatX.h - leaf header: `rotLocalMatX`/`rotLocalMatY`/`rotLocalMatZ` (0x8005044C, 0x80050510, 0x80050598),
 * owned by the nw4r math band at the root of src/. C++ linkage: the map carries the `...FUlPQ34nw4r4math5MTX34`
 * manglings. */
#ifndef MHTRI_ROTLOCALMATX_H
#define MHTRI_ROTLOCALMATX_H

#include "types.h"
#include "nw4r/math.h"

/* Rotate `mtx` about the X, Y and Z axis by `angle`. */
void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatZ(u32 angle, nw4r::math::MTX34* mtx);

#endif
