/* enemy/em_area_next.h - the declaration of `em_area_next`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_AREA_NEXT_H
#define MHTRI_ENEMY_EM_AREA_NEXT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_area_next(struct _ENEMY_WORK *self, u32 a, u32 b);
#ifdef __cplusplus
}
#endif

#endif
