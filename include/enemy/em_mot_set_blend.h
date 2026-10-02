/* enemy/em_mot_set_blend.h - the declaration of `em_mot_set_blend`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOT_SET_BLEND_H
#define MHTRI_ENEMY_EM_MOT_SET_BLEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_mot_set_blend(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d);
#ifdef __cplusplus
}
#endif

#endif
