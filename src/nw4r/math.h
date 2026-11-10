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
 * map.  CLAUDE.md -> Conventions rule 1; docs/plan.md 6.5.
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

/* nw4r's plain 2-float record: the base of `VEC2`. size: 0x8 */
struct _VEC2 {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
};

/* size: 0x8 */
struct VEC2 : public _VEC2 {
    VEC2() __attribute__((never_inline)) {}
};

/* nw4r's plain 3-float record: the base of `VEC3`, and the type a table of vectors has (it has no
 * constructor, so an aggregate initialiser and a static object need no `__sinit`). size: 0xC */
struct _VEC3 {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ f32 z;
};

}  // namespace math
}  // namespace nw4r

struct Vec; /* the SDK vector (`MTX/mtx.h`) a `VEC3` can be built from */

/* `VEC3::operator=` calls `vec3_assign` (a free function, so that `_VEC3` itself stays trivially copyable).  Its body
 * is in `mh3_pad.cpp`, which emits the kept copy: the other units see only the declaration, so a call to
 * `VEC3::operator=` may throw for them (retail's extab ranges cover it, `g3d/g3d_scnobj.cpp`'s constructor). */
#include "mh3_pad/vec3_assign.h"

namespace nw4r {
namespace math {

/* nw4r's vector class.  The game's build never inlines its members: each one is emitted as a weak copy
 * after its first user and the linker keeps the first (`never_inline` gives that shape under the game's
 * flags; the Nintendo-built libraries, which inline them, spell their vectors `_VEC3`).  The arithmetic
 * operators return by value into the caller's object; they are defined once, at the address named. */
struct VEC3 : public _VEC3 {
    VEC3() __attribute__((never_inline)) {}
    VEC3(f32 fx, f32 fy, f32 fz) __attribute__((never_inline))
    {
        x = fx;
        y = fy;
        z = fz;
    }
    VEC3(const VEC3& v) __attribute__((never_inline)) : _VEC3(v) {}
    VEC3(const _VEC3& v) __attribute__((never_inline))
    {
        x = v.x;
        y = v.y;
        z = v.z;
    }
    VEC3(const Vec& v) __attribute__((never_inline)); /* 0x80143174, enemy/enemy_control.cpp */
    VEC3& operator=(const VEC3& v) __attribute__((never_inline))
    {
        vec3_assign(this, &v);
        return *this;
    }
    VEC3& operator+=(const VEC3& rhs);    /* 0x80073F68, g3d/g3d_calcworld.cpp */
    VEC3 operator+(const VEC3& rhs) const;  /* 0x80051378, fn_8004CAD8.cpp */
    VEC3 operator-(const VEC3& rhs) const;  /* 0x80050CA0, fn_8004CAD8.cpp */
    VEC3 operator*(f32 f) const;            /* 0x80051EE0, fn_8004CAD8.cpp */
    VEC3 operator/(f32 f) const;            /* 0x800AD9C0, ef/ef_particlemanager.cpp */
    VEC3 operator-() const;                 /* 0x80077DBC, g3d/g3d_draw.cpp */
};

/* A quaternion. size: 0x10 */
struct QUAT {
    /* +0x0 */ f32 x;
    /* +0x4 */ f32 y;
    /* +0x8 */ f32 z;
    /* +0xC */ f32 w;
};

/* nw4r's plain 3x4 record: the base of `MTX34`, for a matrix no constructor builds. size: 0x30 */
struct _MTX34 {
    /* +0x00 */ f32 m[3][4];
};

/* A 3x4 row-major float matrix; the translation column is `m[0][3]` / `m[1][3]` / `m[2][3]` at
 * +0x0C / +0x1C / +0x2C. size: 0x30 */
struct MTX34 : public _MTX34 {
    MTX34() __attribute__((never_inline)) {}
};

/* A 3x3 row-major float matrix. size: 0x24 */
struct MTX33 {
    /* +0x00 */ f32 m[3][3];

    MTX33(); /* 0x800516F0, fn_8004CAD8.cpp */
};

/* A 4x4 row-major float matrix. size: 0x40 */
struct MTX44 {
    /* +0x00 */ f32 m[4][4];

    MTX44() __attribute__((never_inline)) {}
};

/* A segment: its two end points.  Retail's out-of-line constructor is 0x800FA3E8 (`ef/effect.cpp`). size: 0x18 */
struct SEGMENT3 {
    /* +0x00 */ VEC3 start;
    /* +0x0C */ VEC3 end;

    SEGMENT3() __attribute__((never_inline)) {}
};

/* A capsule: the segment and the radius (`shell_se_req2__FP5_se_wPQ34nw4r4math7CAPSULEUcUl` mangles the type under
 * this namespace).  The segment is a base here, so the end points keep their names; the constructor (0x800FA3B8,
 * `ef/effect.cpp`) builds it through the segment's. size: 0x1C */
struct CAPSULE : public SEGMENT3 {
    /* +0x18 */ f32 radius;

    CAPSULE() __attribute__((never_inline)) {}
};

}  // namespace math
}  // namespace nw4r

/* The C-visible spellings of the same two layouts, for the C units and for C++ units that reach the
 * type through a mangled `fn_` name. */
typedef nw4r::math::VEC3 VEC3;
typedef nw4r::math::_VEC3 _VEC3;
typedef nw4r::math::MTX34 MTX34;
typedef nw4r::math::_MTX34 _MTX34;
typedef nw4r::math::VEC3 Vec3;
typedef nw4r::math::MTX34 Mtx34;

/* The nw4r math free functions the effect units call.  Their map names are GLOBAL-scope manglings
 * (`setVector3__FPQ34nw4r4math4VEC3fff`), so the real declarations sit at global scope: declaring them
 * inside `nw4r::math` would add the `Q34nw4r4math` qualifier and miss the map.  tools/units/mangle.py
 * confirms each spelling; playbook 50 and docs/plan.md 6.5 rule 9 (call the owner, never the
 * mangled identifier).  Additive: a second lane extending this list stays additive. */
void setVector3(VEC3* v, f32 x, f32 y, f32 z);
void rotVecX(VEC3* v, u32 angle);
void rotVecY(VEC3* v, u32 angle);
void rotVecZ(VEC3* v, u32 angle);
void rotVecXYZ(VEC3* v, struct _CP_VECTOR* ang);
void rotVecZXY(VEC3* v, struct _CP_VECTOR* ang);
void mulVecMat(VEC3* v, MTX34* m);
void copyMat33(MTX34* dst, MTX34* src);

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
typedef VEC3 _VEC3;
typedef MTX34 _MTX34;
typedef MTX34 Mtx34;

#endif /* __cplusplus */

#endif /* MHTRI_NW4R_MATH_H */
