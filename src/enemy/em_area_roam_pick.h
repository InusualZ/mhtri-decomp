/* enemy/em_area_roam_pick.h - the declaration of `em_area_roam_pick`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_AREA_ROAM_PICK_H
#define MHTRI_ENEMY_EM_AREA_ROAM_PICK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_area_roam_pick(struct _ENEMY_WORK* self, u16 a);
#ifdef __cplusplus
}
#endif

#endif
