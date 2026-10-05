/*
 * The three 3-float-record helpers `src/mh3_pad.cpp` owns - 0x80041E40 (`copyVec3`),
 * 0x80041E8C (`setVec3`) and 0x80043EA8 (`VEC3_ctor`).  `include/mh3_pad.h` includes this file, so
 * a consumer that can take the whole owner header needs nothing else; it is separate only so that a
 * unit which carries its OWN scalar typedefs can reach the helpers without `types.h`.
 *
 * That unit is `src/ef/ef_cube.cpp`, a legacy file whose `u32` is `unsigned int` where `types.h`
 * spells it `unsigned long` - and its function manglings encode that
 * (`fn_800C9DD0__FUiP4Vec3...` in the target object; `Ul` there would not pair).  Hence the
 * forward declaration below rather than `#include "nw4r/math.h"`: the record type is named without
 * pulling in a typedef set, and the name is spelled in full so this header introduces no global
 * `VEC3` typedef (several units still carry their own local copy of that name).
 *
 * The record is the game's own 3-float vector, 0xC with x/y/z at +0/+4/+8.  It is the type the call
 * sites pass: the three helpers are the game's (`mh3_pad.cpp` is game-root code, not nw4r), their
 * callers across `ef`/`enemy`/`Pl`/`g3d` hand them the engine's vector, and the map's neighbouring
 * `vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec` (0x8004FFB8) is exactly the bridge from nw4r's `VEC3`
 * to it - so a `Vec*` and a `VEC3*` are NOT the same argument.
 *
 * docs/plan.md 6.5 rules 2 and 11: the declaration lives in the owner's header, once, and carries its
 * real type - the parameters were `void*` until the type-fix pass (the erased form the rule bans).
 */
#ifndef MHTRI_MH3_PAD_VEC3_H
#define MHTRI_MH3_PAD_VEC3_H

/* The record.  `struct Vec` is tagged in `include/ef.h`, which owns the definition; naming it here
 * costs no typedef and no layout. */
#ifdef __cplusplus
namespace nw4r { namespace math { struct VEC3; } }
#define MHTRI_PAD_VEC3 nw4r::math::VEC3
#else
struct VEC3;
#define MHTRI_PAD_VEC3 struct VEC3
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80043EA8 - a 4-byte `blr`: it does nothing.  The call is in the retail bytes at every record
 * declaration (1,873 call sites), so it is kept as the record's constructor-shaped no-op. */
void VEC3_ctor(MHTRI_PAD_VEC3* out);
/* 0x80041E40 - copies the 0xC-byte record and returns `dst` (`mr r3,r31` in the target's body). */
MHTRI_PAD_VEC3* copyVec3(MHTRI_PAD_VEC3* dst, const MHTRI_PAD_VEC3* src);
/* 0x80041E8C - builds a record from three floats and returns it (`mr r4,r3` at the call sites). */
MHTRI_PAD_VEC3* setVec3(MHTRI_PAD_VEC3* out, float x, float y, float z);

#ifdef __cplusplus
}
#endif

#undef MHTRI_PAD_VEC3

#endif /* MHTRI_MH3_PAD_VEC3_H */
