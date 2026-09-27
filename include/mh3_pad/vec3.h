/*
 * The three 3-float-record helpers `src/mh3_pad.cpp` owns - 0x80041E40 (`copyVec3`),
 * 0x80041E8C (`setVec3`) and 0x80043EA8 (`VEC3_ctor`).  `include/mh3_pad.h` includes this file, so
 * a consumer that can take the whole owner header needs nothing else; it is separate only so that a
 * unit which carries its OWN scalar typedefs can reach the helpers without `types.h`.
 *
 * That unit is `src/ef/ef_cube.cpp`, a legacy `auto/` file whose `u32` is `unsigned int` where
 * `types.h` spells it `unsigned long` - and its function manglings encode that (`fn_800C9DD0__FUiP4Vec3...`
 * in the target object; `Ul` there would not pair).  Hence `float` rather than `f32` below: the same
 * type, spelled without the typedef.
 *
 * docs/plan.md 6.5 rule 2: the declaration lives in the owner's header, once.
 */
#ifndef MHTRI_MH3_PAD_VEC3_H
#define MHTRI_MH3_PAD_VEC3_H

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80043EA8 - a 4-byte `blr`: it does nothing.  The call is in the retail bytes at every record
 * declaration (1,873 call sites), so it is kept as the record's constructor-shaped no-op. */
void VEC3_ctor(void *out);
/* 0x80041E40 - copies the 0xC-byte record and returns `dst` (`mr r3,r31` in the target's body). */
void* copyVec3(void *dst, const void *src);
/* 0x80041E8C - builds a record from three floats and returns it (`mr r4,r3` at the call sites). */
void* setVec3(void *out, float x, float y, float z);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_VEC3_H */
