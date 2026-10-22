/* ef/eft_def_model_block.h - the declaration of `eft_def_model_block`, which `ef/eft_slot.cpp` owns (docs/plan.md 6.5
 * rule 2), with the callers' byte view of the block (the owner's `EftShapeBlock` is private to its source). */
#ifndef MHTRI_EF_EFT_DEF_MODEL_BLOCK_H
#define MHTRI_EF_EFT_DEF_MODEL_BLOCK_H

#include "types.h"

struct EftSlot;

#ifdef __cplusplus
extern "C" {
#endif
/* The 0x20-byte model block of entry `slot`'s motion `index` (its first byte nonzero while live). */
u8* eft_def_model_block(struct EftSlot* slot, u8 index);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_DEF_MODEL_BLOCK_H */
