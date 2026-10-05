/* ef/eft_res_slot_get.h - the declaration of `eft_res_slot_get`, which `ef/eft_res.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_EF_EFT_RES_SLOT_GET_H
#define MHTRI_EF_EFT_RES_SLOT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
struct EftResSlot* eft_res_slot_get(u32 size);
#ifdef __cplusplus
}
#endif

#endif
