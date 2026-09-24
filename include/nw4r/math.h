/*
 * The nw4r math types the symbol map's mangling encodes (`Q34nw4r4math4VEC3`, `Q34nw4r4math5MTX34`).
 * They are the engine's own spellings: a function whose map name is `foo__FPQ34nw4r4math4VEC3` has to
 * take one of these types to mangle to the same symbol.
 *
 * The layout is defined ONCE.  The C++ spells it `nw4r::math::VEC3` / `MTX34`; a C unit cannot name a
 * namespace, so the same two layouts are also reachable under the C-visible spellings `VEC3` / `MTX34`
 * (used by the C units of `ef/`, `enemy/`, `sound/`) and `Vec3` / `Mtx34` (used by the ef emitter units).
 * Those are typedefs of the namespace type, not second definitions: there is a single layout and the
 * spellings alias it, which is why a mangled callee declared with the C spelling still pairs with the
 * map.  AGENTS.md -> Conventions rule 1; docs/plan.md 6.5.
 *
 * This header is includable from C and C++ (the include counts in the tree put this file in both, once
 * the C units that carry their own `VEC3` copy include it in wave 2).
 */
#ifndef MHTRI_NW4R_MATH_H
#define MHTRI_NW4R_MATH_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {

/* size: 0xC */
struct VEC3 {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ f32 z;
};

/* A 3x4 row-major float matrix; the translation column is `m[0][3]` / `m[1][3]` / `m[2][3]` at
 * +0x0C / +0x1C / +0x2C. size: 0x30 */
struct MTX34 {
    /* +0x00 */ f32 m[3][4];
};

}  // namespace math
}  // namespace nw4r

/* The C-visible spellings of the same two layouts, for the C units and for C++ units that reach the
 * type through a mangled `fn_` name. */
typedef nw4r::math::VEC3 VEC3;
typedef nw4r::math::MTX34 MTX34;
typedef nw4r::math::VEC3 Vec3;
typedef nw4r::math::MTX34 Mtx34;

#else /* !__cplusplus */

/* The C spelling.  A single layout, reachable under both the `VEC3`/`MTX34` and `Vec3`/`Mtx34` names the
 * tree uses. */
typedef struct VEC3 {
    f32 x; /* +0x0 */
    f32 y; /* +0x4 */
    f32 z; /* +0x8 */
} VEC3; /* size: 0xC */

typedef struct MTX34 {
    f32 m[3][4]; /* +0x00 */
} MTX34; /* size: 0x30 */

typedef VEC3 Vec3;
typedef MTX34 Mtx34;

#endif /* __cplusplus */

#endif /* MHTRI_NW4R_MATH_H */
