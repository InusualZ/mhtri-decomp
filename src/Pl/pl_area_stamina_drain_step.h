/* Leaf header (docs/plan.md 6.5 rule 2): `pl_area_stamina_drain_step` (0x80276E08), defined by `src/Pl/pl_act.cpp`. */
#ifndef MHTRI_PL_PL_AREA_STAMINA_DRAIN_STEP_H
#define MHTRI_PL_PL_AREA_STAMINA_DRAIN_STEP_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void pl_area_stamina_drain_step(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_AREA_STAMINA_DRAIN_STEP_H */
