/* ef/eft_res_models_spawn.h - the declaration of `eft_res_models_spawn`, which `ef/eft_res.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_EFT_RES_MODELS_SPAWN_H
#define MHTRI_EF_EFT_RES_MODELS_SPAWN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* untyped: an opaque handle passed through (the callee only hands the pointer on) */
void eft_res_models_spawn(struct _EFT* self, void** models, s32 mode, s32 count, void* arg);
#ifdef __cplusplus
}
#endif

#endif
