/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `pl_sub_gauge_skill_add`, which `Pl/pl_act_class3.cpp` calls. */
#ifndef MHTRI_PL_PL_SUB_GAUGE_SKILL_ADD_H
#define MHTRI_PL_PL_SUB_GAUGE_SKILL_ADD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80276B58 - adds a signed amount to the actor's sub gauge, a cost scaled by the active skills (registry name). */
void pl_sub_gauge_skill_add(struct _PLW* self, s32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SUB_GAUGE_SKILL_ADD_H */
