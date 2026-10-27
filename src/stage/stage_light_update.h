/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_light_update`, which `fn_80056F24.cpp` calls.  C linkage. */
#ifndef MHTRI_STAGE_STAGE_LIGHT_UPDATE_H
#define MHTRI_STAGE_STAGE_LIGHT_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802AE9B4 (0x1A0): updates the stage lighting for the frame. */
void stage_light_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_LIGHT_UPDATE_H */
