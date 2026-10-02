/* ef/eft_res_slot_release.h - the declaration of `eft_res_slot_release`, which `ef/eft_res.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_EFT_RES_SLOT_RELEASE_H
#define MHTRI_EF_EFT_RES_SLOT_RELEASE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/* untyped: an opaque handle passed through (the callee only hands the pointer on) */
void eft_res_slot_release(void* self_);
#ifdef __cplusplus
}
#endif

#endif
