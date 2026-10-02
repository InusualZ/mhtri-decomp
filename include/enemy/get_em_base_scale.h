/* enemy/get_em_base_scale.h - the declaration of `get_em_base_scale`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_GET_EM_BASE_SCALE_H
#define MHTRI_ENEMY_GET_EM_BASE_SCALE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
f32 get_em_base_scale(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
