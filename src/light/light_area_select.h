/* Leaf header (docs/plan.md 6.5 rule 2): `light/light.cpp`'s `light_area_select`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LIGHT_LIGHT_AREA_SELECT_H
#define MHTRI_LIGHT_LIGHT_AREA_SELECT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C2230 - selects the area light objects on the scene root (LightSet::SelectLightObj) */
void light_area_select(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_AREA_SELECT_H */
