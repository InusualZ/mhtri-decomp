/*
 * The two `src/fn_8004CAD8.cpp` symbols that other bands reach without needing that unit's whole
 * header: 0x8005050C (`MTX34_ctor`) and 0x8004CAD8 (`set_slot_none`).  `fn_8004CAD8.h`
 * includes this file; it is split out because its own declaration set (`res_tex_ctor`,
 * `vec3_normalize_into`, `mtx34_identity`, ...) still disagrees with several consumers' local copies, so
 * pulling the whole header into the `ef`/`g3d`/`Pl` bands fails to compile (measured: 30 TUs,
 * MWCC `(10505)`/`(10197)` on the *other* symbols).
 *
 * docs/plan.md 6.5 rule 2: the declaration lives in the owner's header, once.
 */
#ifndef MHTRI_FN_8004CAD8_MTX_H
#define MHTRI_FN_8004CAD8_MTX_H

#include "types.h"

/* The record `MTX34_ctor` constructs.  Forward-declared by name so this header needs no typedef
 * set of its own: `nw4r/math.h` (and the units that still carry the type locally) define it. */
#ifdef __cplusplus
namespace nw4r { namespace math { struct MTX34; } }
#define MHTRI_MTX34 nw4r::math::MTX34
#else
struct MTX34;
#define MHTRI_MTX34 struct MTX34
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8005050C - a 4-byte `blr`: it does nothing.  Every call site passes the address of an
 * `nw4r::math::MTX34` local right after its declaration (`MTX34 mtx; MTX34_ctor(&mtx);`), so it is
 * kept as the record's constructor-shaped no-op - the same reading as `mh3_pad/vec3.h`'s
 * `VEC3_ctor`, whose 0x80043EA8 is the 3-float twin of this one (365 call sites in the DOL).  The
 * parameter carries that type rather than the erased `void*` the name pass left here: the owner's
 * band is the nw4r math band (`rotLocalMatX__FUlPQ34nw4r4math5MTX34` is the code right before it),
 * so the library spelling is the evidenced one (rule 11).
 *
 * GUESS (naming): the body is empty, so the name comes from the call context only.  Confirm when the
 * owner's band is written. */
void MTX34_ctor(MHTRI_MTX34* out);

/* 0x8004CAD8 - writes the `0xFFFF` "none" sentinel into one of six `s16` fields of `rec`
 * (rec+0x92, 0x94, 0x96, 0x98, 0x9A, 0x9C) selected by a 1-based `kind`, through a 16-way jump table
 * at 0x80581584, and returns 1 for kinds 1..6 / 0 for the rest.  The lobby menu panel is the only
 * caller (`fn_801F1874`/`fn_801F2040` in `lobby/fn_801EC9F8.cpp` and `lobby/fn_801F3294.cpp`:
 * `if (entry->field_0x10 == 0xFFFF) set_slot_none(rec, 3)`) over the global pointer at 0x806DBA60.
 *
 * GUESS (naming): the six fields and the kind code are read as item/equipment slots; the record's
 * type is not reconstructed yet, hence `void*`.  Confirm when the owner's band is written. */
s32 set_slot_none(void *rec, u32 kind);

#ifdef __cplusplus
}
#endif

#undef MHTRI_MTX34

#endif /* MHTRI_FN_8004CAD8_MTX_H */
