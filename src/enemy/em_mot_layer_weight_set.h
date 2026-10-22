/* enemy/em_mot_layer_weight_set.h - the declaration of `em_mot_layer_weight_set`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MOT_LAYER_WEIGHT_SET_H
#define MHTRI_ENEMY_EM_MOT_LAYER_WEIGHT_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_mot_layer_weight_set(struct _ENEMY_WORK* self, u16 mot, u32 frame, f32 weight);
#ifdef __cplusplus
}
#endif

#endif
