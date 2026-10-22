/* enemy/em_hook9_call.h - the declaration of `em_hook9_call`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_HOOK9_CALL_H
#define MHTRI_ENEMY_EM_HOOK9_CALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_hook9_call(struct _ENEMY_WORK* self, u8 idx);
#ifdef __cplusplus
}
#endif

#endif
