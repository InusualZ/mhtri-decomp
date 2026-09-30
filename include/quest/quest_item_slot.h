/*
 * `quest/quest_item_slot.cpp`'s declarations (docs/plan.md 6.5 rule 2): the head of the quest entry band
 * (`.text` 0x803AA4A4..0x803AB3BC) - record counters, the item-pair and item-slot helpers and the small quest
 * state probes.  The records they read are `quest/quest_types.h`'s.
 *
 * C linkage: the units that already call into this band (`menu/menu_item_page.cpp`,
 * `lobby/lb_companion_ui.cpp`) declare these inside their own `extern "C"` blocks, so the original symbols
 * were C-linkage names.
 */
#ifndef MHTRI_QUEST_QUEST_ITEM_SLOT_H
#define MHTRI_QUEST_QUEST_ITEM_SLOT_H

#include "types.h"
#include "quest/quest_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The recorded count of `kind` in the save block's first/second record set plus the live item work's copy;
 * the `_wide` forms take the index wider than a byte. */
s32 quest_record_a_count_get(u32 kind);
s32 quest_record_a_count_get_wide(s32 kind);
s32 quest_record_b_count_get(u32 kind);
s32 quest_record_b_count_get_wide(s32 kind);

/* The three item-record copies the pair tables are filled with (0x803AA67C, 0x803AA774, 0x803AA898). */
void quest_item_pair_copy_block(Q_ItemPair* dst, u16 id, u8 kind);
void quest_item_pair_copy_row(Q_ItemPair* dst, u16 id, s8 row, u8 kind);
void quest_item_pair_copy_cell(Q_ItemPair* dst, u16 id, s8 col);

/* The five-slot list's entry for `id`, sign-extended; the callers compare it against 0. */
s16 quest_item_slot_find(Q_ItemCount* slots, u16 id);
/* Adds `count` of `id` to the list; the result byte is what the menu switches on. */
s32 quest_item_slot_add(Q_ItemCount* slots, u8* rot, u16 id, s16 count);
/* Records the item a caller hands over in the local slot's move work, then merges it into the item
 * work's own five-slot list with the negated value (0x803AAB80).  The target never reads its first
 * argument, which the only caller passes as the player work pointer. */
void quest_item_work_merge(struct _PLW* owner, u16 id, s16 value);
/* Rolls and applies the end-of-quest use of the carried item slots, then clears them (0x803AAC0C). */
void quest_item_slots_use(struct _PLW* plw);

/* The item-id range test the pair tables are selected with (0x803AAF1C). */
u32 quest_item_id_low_ck(u16 id);
/* The local slot's item work handed to the pick gate with `idx` and 1 (0x803AB190); 0 when the slot has
 * no item work. */
u32 quest_item_work_notify(s32 idx);

/* The per-slot state probes (0x803AAE7C, 0x803AAEC0, 0x803AB028, 0x803AB070, 0x803AB15C): the move work's
 * +0x22DC flag, whether a quest is selected, the +0x22D4 state byte's bit 7 ("a state code follows") and
 * low seven bits, and the play-state gate. */
u32 quest_move_flag_ck(void);
u32 quest_select_ready_ck(void);
u32 quest_move_state_valid_ck(void);
u32 quest_move_state_get(void);
u32 quest_play_state_ck(void);
/* The three quest-id range probes the bands below gate on (0x803AAF3C, 0x803AAF88, 0x803AAFE0). */
u32 quest_id_low_get(void);
u32 quest_id_head_ck(void);
u32 quest_id_tail_ck(void);

/* The band's static initialiser (0x803AB1F0, the `.ctors` word's target): fills the quest zone tables. */
void quest_zone_tbl_init(void);

/* Warps to the hub in the entry state, else to the quest's start position (0x803AB0B8). */
void quest_warp_by_state(struct _PLW* plw);
/* The quest entry's notify path: a player's entry is announced to the lobby, charged against the quest
 * reward (0x803AB0FC). */
void quest_entry_notify(u8 player);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* MHTRI_QUEST_QUEST_ITEM_SLOT_H */
