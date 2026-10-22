/* ef/eft_slot_shape_hit_ck.h - leaf header (docs/plan.md 6.5 rule 2): `ef/eft_slot.cpp`'s `eft_slot_shape_hit_ck`,
 *   for `lobby/fn_802FA9A0.cpp`.  C linkage; the signature is the owner's definition. */
#ifndef MHTRI_EF_EFT_SLOT_SHAPE_HIT_CK_H
#define MHTRI_EF_EFT_SLOT_SHAPE_HIT_CK_H

#include "types.h"
#include "nw4r/math.h"

struct EftSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80345A6C: whether `pos` stands inside shape `index` of the slot, grown by `margin`. */
u32 eft_slot_shape_hit_ck(struct EftSlot* slot, u8 index, VEC3* pos, f32 margin);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_SLOT_SHAPE_HIT_CK_H */
