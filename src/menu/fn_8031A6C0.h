/*
 * menu/fn_8031A6C0.h - the selection screen's records, traced from the target: `MenuSel` is the units' view of the
 *   0x330-byte working record `menu/menu_item.h` names `MenuSlot` (with the +0x1EC cursor `MenuSelCursor` and the +0x210
 *   slot pairs the item layer leaves unnamed), `MenuSelEntry` the 0x18-byte entries at +0x24 (3) and +0x6C (8);
 *   `self[1]` is the second record (+0x330).  Read by `menu/menu_combo.cpp` and `menu/menu_item_effect.cpp`.
 */
#ifndef MHTRI_MENU_FN_8031A6C0_H
#define MHTRI_MENU_FN_8031A6C0_H

#include "types.h"
#include "id_value.h"

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

/* One combination recipe (`lobby/lb_companion_ui.h` defines it). */
struct LbComboRecipe;

/* This unit's view of `LbComboRecipe` (the cursor reads the result through it): `lobby/lb_companion_ui.h` cannot be
 * included beside `hud/cockpit.h` (`chk_pointer` is `u32` there, `s32` here).  size: 0x8 */
struct MenuComboRecipe {
    /* +0x00 */ u16 partner_0x00;  /* the second ingredient */
    /* +0x02 */ u16 result_0x02;   /* the item the combination makes */
    /* +0x04 */ u8  unused_0x04[0x4];
};

/* The selection state the screen embeds at +0x1EC; its methods take the cursor in r3. */
struct MenuSelCursor {
    /* +0x00 */ MenuComboRecipe* record_0x00; /* the combination recipe the current pair resolved to */
    /* +0x04 */ u16   item_a_0x04;   /* left item id */
    /* +0x06 */ u16   item_b_0x06;   /* right item id */
    /* +0x08 */ u16   slot_a_0x08;   /* left resolved slot, 0xFFFF for none */
    /* +0x0A */ u16   slot_b_0x0A;   /* right resolved slot, 0xFFFF for none */
    /* +0x0C */ s16   value_0x0C;    /* a displayed amount (0x178 sentinel) */
    /* +0x0E */ u8    kind_0x0E;     /* 0/1 (no record) or 2 (record) */
    /* +0x0F */ u8    timer_0x0F;    /* countdown before the value is applied */
    /* +0x10 */ s8    status_0x10;   /* negative while the pair is still resolving */
    /* +0x11 */ u8    flag_0x11;
    /* +0x12 */ u8    flag_0x12;
    /* +0x13 */ u8    flag_0x13;
    /* +0x14 */ s8    applied_0x14;  /* the applied amount */
    /* +0x15 */ u8    anim_0x15;     /* draw-animation selector */
    /* +0x16 */ u8    mode_0x16;     /* the draw state `menu_sel_cursor_result_draw` switches on */
    /* +0x17 */ u8    timer_0x17;
    /* +0x18 */ s8    state_0x18;    /* the hold/repeat sub state `menu_sel_cursor_hold_tick` advances */
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
    /* +0x02 */ s16 amount_0x02;
};
/* size: 0x04 */

/* The save block `lobby_world_block` points at, as this band views it: the equipment box (`IdValue` records from
 * +0x180, `userdata_box_capacity` of them) and the eight shared slot ids at +0x39A2. */
struct MenuSharedSlots {
    /* +0x0000 */ u8  pad_0x0000[0x180];
    /* +0x0180 */ IdValue box_0x0180[1];  /* the box's first record; the count is `userdata_box_capacity`'s */
    /* +0x0184 */ u8  pad_0x0184[0x381E];
    /* +0x39A2 */ u16 ids_0x39A2[8];
};
/* size: 0x39B2 (approximate: a lower bound, the fields this band touches) */

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
    /* +0x00A */ u8  pad_0x00A[0x009];
    /* +0x013 */ u8  wait_0x013;      /* set while the panel holds the menu open (1) */
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
    /* +0x18C */ s32 flag_0x18C;      /* the confirm window's row (0 = yes) */
    /* +0x190 */ struct _PLW* worker_0x190; /* the player the panel works for */
    /* +0x194 */ IdValue* page_a_0x194; /* the first item page (the pouch) */
    /* +0x198 */ IdValue* page_b_0x198; /* the second item page, or null */
    /* +0x19C */ u16 value_0x19C;
    /* +0x19E */ u16 value_0x19E;
    /* +0x1A0 */ s8  col_0x1A0;
    /* +0x1A1 */ s8  row_0x1A1;
    /* +0x1A2 */ s8  sel_0x1A2;
    /* +0x1A3 */ s8  count_0x1A3;
    /* +0x1A4 */ u8  pad_0x1A4[0x003];
    /* +0x1A7 */ s8  list_row_0x1A7;  /* the recipe list's cursor row (the second record's scroll) */
    /* +0x1A8 */ u8  pad_0x1A8[0x008];
    /* +0x1B0 */ u8  flag_0x1B0;
    /* +0x1B1 */ u8  pad_0x1B1[0x037];
    /* +0x1E8 */ u16 ticket_0x1E8;    /* the cockpit message ticket */
    /* +0x1EA */ u8  pad_0x1EA[0x002];
    /* +0x1EC */ MenuSelCursor cursor_0x1EC;
    /* +0x210 */ MenuSelSlot slots_0x210[8];
    /* +0x230 */ u8  pad_0x230[0x00A];
    /* +0x23A */ u8  fade_0x23A;
    /* +0x23B */ u8  field_0x23B;
    /* +0x23C */ u8  frame_0x23C;     /* the recipe list's cursor animation frame */
    /* +0x23D */ u8  pad_0x23D[0x0F3];
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
