/* ef/eft_state_flags_set.h - the declaration of `eft_state_flags_set`, which `ef/effect.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_EFT_STATE_FLAGS_SET_H
#define MHTRI_EF_EFT_STATE_FLAGS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void eft_state_flags_set(struct EftFrameState* self, u8 a, u8 b);
#ifdef __cplusplus
}
#endif

#endif
