/* enemy/em_ailment_reset.h - the declaration of `em_ailment_reset`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_AILMENT_RESET_H
#define MHTRI_ENEMY_EM_AILMENT_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_ailment_reset(struct _ENEMY_WORK* self, u32 kind);
#ifdef __cplusplus
}
#endif

#endif
