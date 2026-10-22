/* menu/eft_em_model_pair_attach.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `eft_em_model_pair_attach` (0x803C32B0), which `menu/get_pop_dat_ptr.cpp` owns;
 * it attaches the two-model effect slot (kind 0x38) to the enemy. */
#ifndef MHTRI_MENU_EFT_EM_MODEL_PAIR_ATTACH_H
#define MHTRI_MENU_EFT_EM_MODEL_PAIR_ATTACH_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void eft_em_model_pair_attach(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif
