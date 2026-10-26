/* `setScaleMat34W` (0x80050718), defined by `fn_8004CAD8.cpp` (leaf header, rule 2).  C++ linkage: the map spells
 * it `setScaleMat34W__FPQ34nw4r4math5MTX34PQ34nw4r4math4VEC3`. */
#ifndef MHTRI_SETSCALEMAT34W_H
#define MHTRI_SETSCALEMAT34W_H

#include "types.h"
#include "nw4r/math.h"

/* 0x80050718 - scales the rotation part of `mtx` by `scale`. */
void setScaleMat34W(nw4r::math::MTX34* mtx, nw4r::math::VEC3* scale);

#endif /* MHTRI_SETSCALEMAT34W_H */
