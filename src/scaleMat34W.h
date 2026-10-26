/* scaleMat34W.h - leaf header: `scaleMat34W` (0x80050674), owned by `fn_8004CAD8.cpp` (the nw4r math band at the
 * root of src/). C++ linkage: the map carries `scaleMat34W__FPQ34nw4r4math5MTX34PQ34nw4r4math4VEC3`. */
#ifndef MHTRI_SCALEMAT34W_H
#define MHTRI_SCALEMAT34W_H

#include "types.h"
#include "nw4r/math.h"

/* Scales the three rows of `mtx` by the components of `scale`. */
void scaleMat34W(MTX34* mtx, VEC3* scale);

#endif
