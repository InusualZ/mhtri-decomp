/* enemy/em_max_hp_init.h - the declaration of `em_max_hp_init`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_MAX_HP_INIT_H
#define MHTRI_ENEMY_EM_MAX_HP_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_max_hp_init(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
