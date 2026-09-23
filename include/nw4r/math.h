/*
 * The nw4r math types the symbol map's mangling encodes (`Q34nw4r4math4VEC3`, ...). They are the engine's
 * own spellings, so a function whose map name is `foo__FPQ34nw4r4math4VEC3` has to take one of these
 * types to mangle to the same symbol.
 *
 * `Pl/pl_act.cpp` carries a private copy of `VEC3`; that copy moves here the next time that unit is
 * touched (a type more than one unit uses lives in one header, AGENTS.md -> Conventions).
 */
#ifndef MHTRI_NW4R_MATH_H
#define MHTRI_NW4R_MATH_H

#include "types.h"

namespace nw4r {
namespace math {

/* size: 0xC */
struct VEC3 {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ f32 z;
};

}  // namespace math
}  // namespace nw4r

#endif /* MHTRI_NW4R_MATH_H */
