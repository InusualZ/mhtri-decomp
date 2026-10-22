/* ef/eft_slot_shape_block_get.h - the declaration of `eft_slot_shape_block_get` (0x80345894), which `ef/eft_slot.cpp`
 * owns (docs/plan.md 6.5 rule 2), with the callers' view of the shape block it returns (the owner's `EftShapeBlock`
 * is private to its source). */
#ifndef MHTRI_EF_EFT_SLOT_SHAPE_BLOCK_GET_H
#define MHTRI_EF_EFT_SLOT_SHAPE_BLOCK_GET_H

#include "types.h"
#include "nw4r/math.h"

struct EftSlot;

/* One shape block of a slot definition: a sphere (`kind_0x00` 0, centre `a_0x04`) or a capsule (1, `a_0x04` to
 * `b_0x10`), with its radius.
 * size: 0x20 */
struct EftShapeBlockView {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[0x3];
    /* +0x04 */ nw4r::math::VEC3 a_0x04;
    /* +0x10 */ nw4r::math::VEC3 b_0x10;
    /* +0x1C */ f32 radius_0x1C;
};

extern "C" {
/* Returns shape block `index` of the slot's definition for key `key` in the current map kind, NULL when none. */
EftShapeBlockView* eft_slot_shape_block_get(struct EftSlot* slot, u8 key, u8 index);
}

#endif /* MHTRI_EF_EFT_SLOT_SHAPE_BLOCK_GET_H */
