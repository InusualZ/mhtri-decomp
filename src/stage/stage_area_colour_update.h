/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w`'s `stage_area_colour_update`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_STAGE_STAGE_AREA_COLOUR_UPDATE_H
#define MHTRI_STAGE_STAGE_AREA_COLOUR_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B2E2C (stage/stg_w): copies the current area's staged colour record out of the stage block and drives it. */
void stage_area_colour_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_COLOUR_UPDATE_H */
