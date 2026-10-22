/* stage/stg_ground_impact_set.h - the declarations of `stg_ground_impact_set` and `stg_area_ceiling_get`, which
 * `stage/stg_w.cpp` owns (docs/plan.md 6.5 rule 2); the signatures are the owner's definitions. */
#ifndef MHTRI_STAGE_STG_GROUND_IMPACT_SET_H
#define MHTRI_STAGE_STG_GROUND_IMPACT_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x802B0430 */
f32 stg_area_ceiling_get(u8 id);
/* 0x802B43A8 */
void stg_ground_impact_set(struct MHchar* chr, u8 kind, u16 flags);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STG_GROUND_IMPACT_SET_H */
