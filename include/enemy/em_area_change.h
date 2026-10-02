/* enemy/em_area_change.h - the declaration of `em_area_change`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_AREA_CHANGE_H
#define MHTRI_ENEMY_EM_AREA_CHANGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_area_change(struct _ENEMY_WORK *self, u32 a);
#ifdef __cplusplus
}
#endif

#endif
