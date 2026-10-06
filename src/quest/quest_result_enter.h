/*
 * Leaf header (docs/plan.md 6.5 rule 2): the three quest entry-band functions `lobby/lb_companion_ui.cpp` calls,
 * all defined by `src/quest/quest_entry.cpp`.  It is separate from `quest/quest_entry.h` because that header
 * carries the band's whole declaration set, which clashes with the lobby band's own view of
 * `get_move_work_adrs` and `GetItemData`; the owner's header includes this one, so there is one declaration.
 */
#ifndef MHTRI_QUEST_QUEST_RESULT_ENTER_H
#define MHTRI_QUEST_QUEST_RESULT_ENTER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Enters the quest result state (0x803AFE4C) and the quest start state (0x803AFF34); `item` is the item work
 * the caller holds, `work` the slot's move work, `kind` the result message selector. */
void quest_result_enter(struct Q_ItemWork* item, struct Q_MoveWork* work, u8 kind);
void quest_start_enter(struct Q_ItemWork* item, struct Q_MoveWork* work);
/* Charges one faint of `player` against the quest reward and announces it (0x803AF98C). */
void quest_reward_faint_penalty(u8 player);
/* Hands `player`'s arena data step (`id`, `value`) to the quest work (0x803B3074). */
void quest_arena_data_step(u8 player, u16 id, s16 value);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* MHTRI_QUEST_QUEST_RESULT_ENTER_H */
