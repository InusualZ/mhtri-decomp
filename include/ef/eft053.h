/*
 * `ef/eft053.cpp`'s own header.
 *
 * The seven helpers below are NOT this unit's symbols: their owners are the registered root units
 * (`fn_8004CAD8.cpp`, `draw_shape.cpp`) whose headers cannot carry these spellings yet - 16 units
 * declare local copies with mismatching parameter types (`fn_80041E40(void*, const void*)` in
 * `ef/fn_801173AC.cpp`), so adding them to the owner header breaks every one of those TUs'
 * compilation (measured: 16 `FAILED` targets, `(10197) illegal function overloading`).  Until those
 * copies move to the owner's header, this is the only home that compiles; the unit's outbox carries
 * the shared-file request.
 */
#ifndef MHTRI_EF_EFT053_H
#define MHTRI_EF_EFT053_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"

/* 0x80050F80 - squared distance between two 3-float vectors (`ai/fn_802D0F34.h` spells the same). */
f32 fn_80050F80(const void* a, const void* b);
/* 0x80041E40 / 0x80051378 - the engine vector copy and component-wise add. */
void fn_80041E40(VEC3* dst, const VEC3* src);
void fn_80051378(VEC3* out, VEC3* a, VEC3* b);
/* 0x80050850 - normalise a vector in place (`ef/eft001.cpp`/`ef/eft007.cpp` carry the same call). */
void fn_80050850(VEC3* a, VEC3* b);
/* 0x80050E70 - copy the engine's `Vec` into an nw4r `VEC3` (map mangling
 * `vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`; `ef/eft007.cpp` names `unsplit/unknown.h` but that
 * header does not actually declare it). */
void vec_to_mh_vec3(VEC3* dst, Vec* src);

#endif /* MHTRI_EF_EFT053_H */
