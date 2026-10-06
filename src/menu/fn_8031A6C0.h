/*
 * menu/fn_8031A6C0.h - the selection screen's records, traced from the target: `MenuSel` is the units' view of the
 *   0x330-byte working record `menu/menu_item.h` names `MenuSlot` (with the +0x1EC cursor `MenuSelCursor` and the +0x210
 *   slot pairs the item layer leaves unnamed), `MenuSelEntry` the 0x18-byte entries at +0x24 (3) and +0x6C (8);
 *   `self[1]` is the second record (+0x330).  Read by `menu/fn_8031A6C0.cpp` and `menu/menu_item_effect.cpp`.
 */
#ifndef MHTRI_MENU_FN_8031A6C0_H
#define MHTRI_MENU_FN_8031A6C0_H

#include "types.h"

struct MenuSel;

/* One 0x18-byte menu entry as this band views it (the `+0x24` and `+0x6C` arrays). */
struct MenuSelEntry {
    /* +0x00 */ u8  field_0x00;
    /* +0x01 */ u8  selected_0x01;   /* 1 for the entry the caller's index names */
    /* +0x02 */ u8  available_0x02;  /* 1 while the entry's item is selectable */
    /* +0x03 */ u8  pad_0x03[0x03];
    /* +0x06 */ s16 slot_0x06;       /* the slot the entry draws, -1 for none */
    /* +0x08 */ u32 data_0x08;
    /* +0x0C */ u32 color_a_0x0C;
    /* +0x10 */ u32 color_b_0x10;
    /* +0x14 */ s32 color_c_0x14;
};
/* size: 0x18 */

/* The selection state the screen embeds at +0x1EC; its methods take the cursor in r3. */
struct MenuSelCursor {
    /* +0x00 */ void* record_0x00;   /* the equip record the current pair resolved to */
    /* +0x04 */ u16   item_a_0x04;   /* left item id */
    /* +0x06 */ u16   item_b_0x06;   /* right item id */
    /* +0x08 */ s16   slot_a_0x08;   /* left resolved slot, -1 for none */
    /* +0x0A */ s16   slot_b_0x0A;   /* right resolved slot, -1 for none */
    /* +0x0C */ s16   value_0x0C;    /* a displayed amount (0x178 sentinel) */
    /* +0x0E */ u8    kind_0x0E;     /* 0/1 (no record) or 2 (record) */
    /* +0x0F */ u8    timer_0x0F;    /* countdown before the value is applied */
    /* +0x10 */ s8    status_0x10;   /* negative while the pair is still resolving */
    /* +0x11 */ u8    flag_0x11;
    /* +0x12 */ u8    flag_0x12;
    /* +0x13 */ u8    flag_0x13;
    /* +0x14 */ s8    applied_0x14;  /* the applied amount */
    /* +0x15 */ u8    anim_0x15;     /* draw-animation selector */
    /* +0x16 */ u8    mode_0x16;     /* the draw state `fn_8031CE08` switches on */
    /* +0x17 */ u8    timer_0x17;
    /* +0x18 */ u8    state_0x18;    /* the sub state `fn_8031C338` advances */
    /* +0x19 */ u8    pad_0x19[0x03];
    /* +0x1C */ MenuSel* owner_0x1C;
    /* +0x20 */ u8    blend_0x20;
    /* +0x21 */ u8    blend_0x21;
    /* +0x22 */ u8    counter_0x22;
    /* +0x23 */ u8    counter_0x23;
};
/* size: 0x24 */

/* One 4-byte entry of the `+0x210` slot-pair array (8 entries). */
struct MenuSelSlot {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 amount_0x02;
};
/* size: 0x04 */

/* The shared slot-id table `lobby_world_block` points at: this band reads the eight `u16` ids at +0x39A2
 * (the menu library's own view of the same block).  Declared here as this band's view. */
struct MenuSharedSlots {
    /* +0x39A2 */ u16 ids_0x39A2[8];
};
/* size: 0x39B2 (approximate: only the field this band touches is named) */

/* This band's view of the 0x330-byte menu working record.  The first 0x330 bytes are the `MenuSlot`
 * `menu_item.h` defines; the extra fields are the ones this band's own code uses. */
struct MenuSel {
    /* +0x000 */ u8  pad_0x000[0x001];
    /* +0x001 */ u8  state_0x001;
    /* +0x002 */ u8  field_0x002;
    /* +0x003 */ u8  pad_0x003[0x001];
    /* +0x004 */ u16 input_0x004;
    /* +0x006 */ u16 input_0x006;
    /* +0x008 */ u16 input_0x008;
    /* +0x00A */ u8  pad_0x00A[0x00A];
    /* +0x014 */ u8  mode_0x014;
    /* +0x015 */ u8  pad_0x015[0x001];
    /* +0x016 */ s8  field_0x016;
    /* +0x017 */ u8  value_0x017;
    /* +0x018 */ u8  pad_0x018[0x002];
    /* +0x01A */ s8  index_0x01A;
    /* +0x01B */ u8  pad_0x01B[0x001];
    /* +0x01C */ u8  field_0x01C;
    /* +0x01D */ u8  pad_0x01D[0x007];
    /* +0x024 */ MenuSelEntry entries_a_0x024[3];
    /* +0x06C */ MenuSelEntry entries_b_0x06C[8];
    /* +0x12C */ u8  pad_0x12C[0x060];
    /* +0x18C */ u32 flag_0x18C;
    /* +0x190 */ void* worker_0x190;
    /* +0x194 */ u32 field_0x194;
    /* +0x198 */ u32 field_0x198;
    /* +0x19C */ u16 value_0x19C;
    /* +0x19E */ u16 value_0x19E;
    /* +0x1A0 */ s8  col_0x1A0;
    /* +0x1A1 */ s8  row_0x1A1;
    /* +0x1A2 */ s8  sel_0x1A2;
    /* +0x1A3 */ s8  count_0x1A3;
    /* +0x1A4 */ u8  pad_0x1A4[0x00C];
    /* +0x1B0 */ u8  flag_0x1B0;
    /* +0x1B1 */ u8  pad_0x1B1[0x037];
    /* +0x1E8 */ u16 value_0x1E8;
    /* +0x1EA */ u8  pad_0x1EA[0x002];
    /* +0x1EC */ MenuSelCursor cursor_0x1EC;
    /* +0x210 */ MenuSelSlot slots_0x210[8];
    /* +0x230 */ u8  pad_0x230[0x00A];
    /* +0x23A */ u8  fade_0x23A;
    /* +0x23B */ u8  field_0x23B;
    /* +0x23C */ u8  pad_0x23C[0x0F4];
};
/* size: 0x330 (the `menu_item.h` `MenuSlot` record; `self[1]` is the work area's second slot) */

/* The effect wrapper the band's tail drives (its +0x38 payload differs per effect type). */
struct MenuEff {
    /* +0x000 */ u8  pad_0x000[0x001];
    /* +0x001 */ u8  state_0x001;
    /* +0x002 */ u8  kind_0x002;
    /* +0x003 */ u8  type_0x003;
    /* +0x004 */ u8  pad_0x004;
    /* +0x005 */ u8  step_0x005;
    /* +0x006 */ u8  flag_0x006;
    /* +0x007 */ u8  pad_0x007[0x005];
    /* +0x00C */ s32 value_0x00C;
    /* +0x010 */ f32 scale_0x010;
    /* +0x014 */ u8  pad_0x014[0x004];
    /* +0x018 */ f32 vec_0x018[3];
    /* +0x024 */ u32 rot_x_0x024;
    /* +0x028 */ u32 rot_y_0x028;
    /* +0x02C */ u32 field_0x02C;
    /* +0x030 */ void* model_0x030;
    /* +0x034 */ void* retire_0x034;  /* callback */
    /* +0x038 */ void* payload_0x038;
    /* +0x03C */ u8  pad_0x03C[0x004];
    /* +0x040 */ void* move_0x040;    /* callback */
    /* +0x044 */ u8  area_0x044;
    /* +0x045 */ u8  pad_0x045[0x03];
};
/* size: 0x48 (approximate: the highest offset the band touches is +0x44) */

#endif /* MHTRI_MENU_FN_8031A6C0_H */
