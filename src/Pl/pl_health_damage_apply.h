/* Leaf header (docs/plan.md 6.5 rule 2): `pl_health_damage_apply` (0x8027633C), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_HEALTH_DAMAGE_APPLY_H
#define MHTRI_PL_PL_HEALTH_DAMAGE_APPLY_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

s32 pl_health_damage_apply(struct _PLW* plw, s16 delta, s8* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_HEALTH_DAMAGE_APPLY_H */
