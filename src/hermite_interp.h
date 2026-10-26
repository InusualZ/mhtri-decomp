/* Leaf header (docs/plan.md 6.5 rule 2): `hermite_interp`, which `fn_8004CAD8.cpp` owns.  C linkage. */
#ifndef MHTRI_HERMITE_INTERP_H
#define MHTRI_HERMITE_INTERP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80052288 (0x78): the cubic Hermite value at `t` of the curve through (p0, t0, m0) and (p1, t1, m1). */
f32 hermite_interp(f32 t, f32 p0, f32 t0, f32 m0, f32 p1, f32 t1, f32 m1);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HERMITE_INTERP_H */
