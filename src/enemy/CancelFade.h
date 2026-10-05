/* enemy/CancelFade.h - the declaration of `CancelFade`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_CANCELFADE_H
#define MHTRI_ENEMY_CANCELFADE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void CancelFade(struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif
