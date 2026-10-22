/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_area_event_dispatch`, which `Pl/pl_act_step.cpp`'s gesture
 * act calls. */
#ifndef MHTRI_STAGE_STAGE_AREA_EVENT_DISPATCH_H
#define MHTRI_STAGE_STAGE_AREA_EVENT_DISPATCH_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B4680 - runs area event `index` on the local stage, or sends it to the lobby for the master (registry name). */
void stage_area_event_dispatch(struct _PLW* plw, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_EVENT_DISPATCH_H */
