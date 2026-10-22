/* stage/stage_area_objects_refresh.h - leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s
 *   `stage_area_objects_refresh` (0x802B4C5C), for `lobby/fn_802FA9A0.cpp`.  C linkage.  GUESS name (registry). */
#ifndef MHTRI_STAGE_STAGE_AREA_OBJECTS_REFRESH_H
#define MHTRI_STAGE_STAGE_AREA_OBJECTS_REFRESH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B4C5C: re-drives every live area object's own per-frame update (outside game mode 1). */
void stage_area_objects_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_OBJECTS_REFRESH_H */
