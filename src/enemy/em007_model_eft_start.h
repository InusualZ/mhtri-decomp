/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em007_act.cpp`'s `em007_model_eft_start`, the one declaration
 * `ai/ai_npc.cpp` needs. */
#ifndef MHTRI_ENEMY_EM007_MODEL_EFT_START_H
#define MHTRI_ENEMY_EM007_MODEL_EFT_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E0240 (0x58): starts the model effect for a work record (mode 0, the record's +0x420 kind) and arms it. */
void em007_model_eft_start(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM007_MODEL_EFT_START_H */
