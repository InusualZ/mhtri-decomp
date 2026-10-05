/* enemy/em_turn_seq_step.h - the declaration of `em_turn_seq_step`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_TURN_SEQ_STEP_H
#define MHTRI_ENEMY_EM_TURN_SEQ_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_turn_seq_step(struct _ENEMY_WORK* self, void* tbl);
#ifdef __cplusplus
}
#endif

#endif
