/* ef/EftSlot.h - the 0x3C-byte effect slot record `ef/eft_slot.cpp` keeps in its ten-entry pool, shared with
 * `hud/pl_frame_sync.cpp` (through `hud/net_char_sync.h`), which packs and applies it (docs/plan.md 6.5 rule 1). */
#ifndef MHTRI_EF_EFTSLOT_H
#define MHTRI_EF_EFTSLOT_H

#include "types.h"
#include "nw4r/math.h"

/* The 0x3C-byte slot pool `lbl_806BF0A0` (10 entries) the family's spawn/reset pair walks: `key_0x00`
 * is the definition-table index (`eft_def_get`..`eft_def_handler`), `field_0x14` is stamped 255 by the
 * pool reset, and the +0x18 block holds the slot's per-instance state. size: 0x3C */
struct EftSlot {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 key_0x01;
    /* +0x02 */ u8 armed_0x02;  /* set by `eft_slot_work_bind` when the slot has no move work yet */
    /* +0x03 */ s8 work_0x03;   /* the move-work index, -1 = none */
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u16 field_0x06;
    /* +0x08 */ u8 field_0x08;  /* the state byte `eft_slot_state_set` writes */
    /* +0x09 */ u8 field_0x09;  /* the pending state `eft_slot_state_request` records when the slot is idle */
    /* +0x0A */ u8 field_0x0A;
    /* +0x0B */ u8 field_0x0B;
    /* +0x0C */ u8 field_0x0C;   /* the work index the previous tick settled on */
    /* +0x0D */ u8 field_0x0D;
    /* +0x0E */ u8 field_0x0E;
    /* +0x0F */ u8 field_0x0F;  /* the mode `eft_slot_effect_key` selects the +0x14 byte on */
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 field_0x11;
    /* +0x12 */ u8 field_0x12;
    /* +0x13 */ u8 field_0x13;   /* the map number `get_now_mapno` seeds */
    /* +0x14 */ u8 field_0x14;   /* the "slot live" byte the reset stamps 255 */
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 field_0x16;
    /* +0x17 */ u8 field_0x17;
    /* +0x18 */ u32 field_0x18;  /* `eft_slot_live_ck` gates on it being non-zero */
    /* +0x1C */ s16 field_0x1C;
    /* +0x1E */ u16 field_0x1E;
    /* +0x20 */ u16 field_0x20;
    /* +0x22 */ u16 field_0x22;
    union {   /* the position triple the net sync copies as one VEC3 */
        /* +0x24 */ nw4r::math::VEC3 pos_0x24;
        struct {
            /* +0x24 */ f32 field_0x24;
            /* +0x28 */ f32 field_0x28;
            /* +0x2C */ f32 field_0x2C;
        };
    };
    /* +0x30 */ f32 field_0x30;
    /* +0x34 */ s16 field_0x34;
    /* +0x36 */ u8 field_0x36;
    /* +0x37 */ u8 field_0x37;
    /* +0x38 */ u8 field_0x38;
    /* +0x39 */ u8 field_0x39;
    /* +0x3A */ u8 field_0x3A;
    /* +0x3B */ u8 field_0x3B;
};

#endif /* MHTRI_EF_EFTSLOT_H */
