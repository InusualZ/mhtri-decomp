/* enemy/em_turn_seq_start.h - the declaration of `em_turn_seq_start`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_TURN_SEQ_START_H
#define MHTRI_ENEMY_EM_TURN_SEQ_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_turn_seq_start(struct _ENEMY_WORK* self, void* tbl, s32 a, s32 b, s32 c);
#ifdef __cplusplus
}
#endif

#endif
