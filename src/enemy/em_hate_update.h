/* enemy/em_hate_update.h - the declaration of `em_hate_update`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HATE_UPDATE_H
#define MHTRI_ENEMY_EM_HATE_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_hate_update(struct _ENEMY_WORK* work);
#ifdef __cplusplus
}
#endif

#endif
