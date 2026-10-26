/* enemy/lobby_friend_accept.h - leaf header (docs/plan.md 6.5 rule 2) for `enemy/em020_prog.cpp`'s recent-list and
 *   quest-info entries `menu/menu_item_sub.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_ENEMY_LOBBY_FRIEND_ACCEPT_H
#define MHTRI_ENEMY_LOBBY_FRIEND_ACCEPT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _mh_ivec2_;

/* 0x80378A2C (0x5C): accepts recent row `index`'s friend request: 0 out of range, 1 sent, 2 failed. */
s32 lobby_friend_accept(s32 index);
/* 0x80378A88 (0xA4): drops recent row `index`. */
void lobby_recent_drop(s32 index);
/* 0x80378F7C (0x20): the address of arena row `index` (31 bytes) in the lobby state block. */
u8* em020_row_ptr(u8 index);
/* 0x8036CF64 (0x744): draws the quest info sheet for board mode `mode`. */
void quest_info_sheet_draw(s32 mode);
/* 0x8036D6A8 (0x370): draws the quest info detail page. */
void quest_info_detail_draw(s32 mode);
/* 0x8036DA18 (0x168): draws the quest info summary page. */
void quest_info_summary_draw(s32 mode);
/* 0x8036DB80 (0x150): draws the quest info objective page. */
void quest_info_objective_draw(s32 mode);
/* 0x8036E1D4 (0x98): draws quest `index`'s name at `pos`. */
void quest_info_name_draw(u8 index, const struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LOBBY_FRIEND_ACCEPT_H */
