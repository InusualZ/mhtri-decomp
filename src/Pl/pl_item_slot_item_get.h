/* Leaf header (docs/plan.md 6.5 rule 2): symbols `Pl/pl_act.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_PL_PL_ITEM_SLOT_ITEM_GET_H
#define MHTRI_PL_PL_ITEM_SLOT_ITEM_GET_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80272C80: the item id in item slot `slot` (bit 0x80 picks a spare slot). */
u16 pl_item_slot_item_get(_PLW* plw, u8 slot);
/* 0x80272CC8: the count in item slot `slot` (0 for 0xFF). */
s16 pl_item_slot_count_get(_PLW* plw, u8 slot);
/* 0x80273044: the slot holding `item` (0x80 | index for a spare slot), 0xFFFF when none. */
u16 pl_item_slot_find(_PLW* plw, u16 item);
/* 0x80274B20: the icon kind of `item` from the item/kind pair table, 0xFF when absent. */
u8 pl_item_icon_kind_get(u16 item);
/* 0x8027993C: the next (`forward`) or previous item slot on item page 2, 0xFF otherwise. */
u16 pl_item_slot_next_get(_PLW* plw, u16 slot, s32 forward);
/* 0x8027A4EC: whether the player is in an aiming act. */
u32 pl_aim_act_ck(_PLW* plw);
/* 0x8027AC18: whether the player's item menu byte (+0x5BB) is set. */
u32 pl_item_menu_open_ck(_PLW* plw);
/* 0x8027C208: whether `item` can be used in the player's current act. */
s32 pl_item_now_usable_ck(_PLW* plw, u16 item);
/* 0x8027CE74: whether the player is in a reload motion on item page 2. */
s32 pl_reload_motion_ck(_PLW* plw);
/* 0x8027CF24: whether item page 2 shows the gauge icon. */
s32 pl_gauge_icon_ck(_PLW* plw);
/* 0x8027DC64: whether the shared player work flag 0 is clear. */
u32 pl_work_flag0_clear_ck(void);
/* 0x8027DC78: whether the shared player work flag 8 is clear. */
u32 pl_work_flag8_clear_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ITEM_SLOT_ITEM_GET_H */
