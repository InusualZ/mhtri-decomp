/* enemy/em_reaction_req.h - the declaration of `em_reaction_req`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_REACTION_REQ_H
#define MHTRI_ENEMY_EM_REACTION_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_reaction_req(struct _ENEMY_WORK *self, u32 a, u32 b, u32 c);
#ifdef __cplusplus
}
#endif

#endif
