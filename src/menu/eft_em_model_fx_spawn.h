/* menu/eft_em_model_fx_spawn.h - the declaration of `eft_em_model_fx_spawn`, which `ef/eft044.cpp` owns (docs/plan.md 6.5
 * rule 2).  GUESS name (the owner's body). */
#ifndef MHTRI_MENU_EFT_EM_MODEL_FX_SPAWN_H
#define MHTRI_MENU_EFT_EM_MODEL_FX_SPAWN_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8031E57C: takes effect slot 8 for `owner`, installs its callbacks and loads its models. */
void eft_em_model_fx_spawn(struct _ENEMY_WORK* owner);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EFT_EM_MODEL_FX_SPAWN_H */
