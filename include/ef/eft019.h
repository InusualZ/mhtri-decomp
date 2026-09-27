/*
 * Declarations for the symbols `src/ef/eft019.cpp` owns (docs/plan.md 6.5 rule 2: a declaration lives
 * with the TU that owns the symbol).  The owner's own body is the source of the signature here.
 */
#ifndef MHTRI_EF_EFT019_H
#define MHTRI_EF_EFT019_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* 0x80112610 - the family's core spawner the sibling effect units reuse
 * (`eft019_set_core__FPQ34nw4r4math4VEC3UcUcff`): the position, the area byte, the parameter id and
 * the two scale factors.  The owner defines it at C++ scope (`src/ef/eft019.cpp:136`), so the
 * declaration belongs there too (rule 9).  Added with `ef/eft035.cpp`. */
void eft019_set_core(nw4r::math::VEC3* pos, u8 area, u8 param, f32 scale_a, f32 scale_b);

/* 0x80111A34 - the family's short entry point (`eft019_set__FPQ34nw4r4math4VEC3UcUc`): the position,
 * the area byte and the parameter id, no scales.  Declared at C++ scope like its sibling above
 * (rule 9: the map name is this signature's mangling).  Added with `enemy/em035_prog.cpp`, the
 * first consumer to need the three-argument form. */
void eft019_set(nw4r::math::VEC3* pos, u8 area, u8 param);

#endif

#endif /* MHTRI_EF_EFT019_H */
