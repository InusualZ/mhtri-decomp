/* ef/eft_em_slot_attach.h - the declaration of `eft_em_slot_attach`, which `ef/eft_model_slot.cpp` owns (docs/plan.md 6.5
 * rule 2).  GUESS name (the owner's body). */
#ifndef MHTRI_EF_EFT_EM_SLOT_ATTACH_H
#define MHTRI_EF_EFT_EM_SLOT_ATTACH_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800FACF0: registers `owner` with effect pool slot 1 and its handler. */
void eft_em_slot_attach(struct _ENEMY_WORK* owner);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_EM_SLOT_ATTACH_H */
