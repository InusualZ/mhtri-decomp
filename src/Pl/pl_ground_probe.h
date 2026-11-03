/* Leaf header (docs/plan.md 6.5 rule 2): `pl_ground_probe`, which `Pl/pl_coll.cpp` owns, in the argument view
 * `enemy/em_common.cpp` calls it through (the owner defines it over an `_ENEMY_WORK*` and a `LandData*`; C linkage, so
 * the code is unaffected). */
#ifndef MHTRI_PL_PL_GROUND_PROBE_H
#define MHTRI_PL_PL_GROUND_PROBE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
s32 pl_ground_probe(nw4r::math::VEC3* pos, u16* water, f32* floor, f32* surface, f32* surface2, u8* ceiling, u32 mask);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_GROUND_PROBE_H */
