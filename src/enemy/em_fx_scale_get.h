/* enemy/em_fx_scale_get.h - the declaration of `em_fx_scale_get`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_FX_SCALE_GET_H
#define MHTRI_ENEMY_EM_FX_SCALE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
f32 em_fx_scale_get(void* enemy); /* target references the plain name (relocaudit) */
#ifdef __cplusplus
}
#endif

#endif
