/* The lobby menu-position unit `lobby/lb_menu_pos_tbl.cpp`: the lobby's own guard the NPC control band polls. */
#ifndef MHTRI_LOBBY_LB_MENU_POS_TBL_H
#define MHTRI_LOBBY_LB_MENU_POS_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021F238 - the item list's selected row (`lb_item_list_state` +0x38); the session callback stores it in
 * `lobby_w` +0x162 (GUESS name, from the body). */
s32 getItemListSelection(void);

/* 0x8021F020 - hands the item list the clock: the stamp, the row count (and its per-tick step) and the
 * scroll depth (GUESS name: the network control's schedule sync is the caller). */
void syncItemListClock(u32 stamp, u32 count, f32 depth);

/* 0x8021F0DC - steps the lobby's event schedule (GUESS name). */
void lb_event_schedule_step(void);

#ifdef __cplusplus
}
#endif

/* Declarations `enemy/em020_prog.cpp` calls (its lobby/quest bands). */
#ifdef __cplusplus
namespace nw4r { namespace math { struct VEC3; } }
extern "C" {
void lobby_player_place(u8 mode, nw4r::math::VEC3* pos, u16 angle);
void lobby_player_place_ex(u8 mode, nw4r::math::VEC3* pos, u16 angle, s32 kind);
}
#endif

/* The lobby trade tables (.data 0x805BA660-0x805BA750), rows of halfwords that `lobby/fn_801E0ADC.cpp`'s
 * trade-goods sale reads. */
#ifdef __cplusplus
extern "C" {
#endif
/* .data 0x805BA660 (0x74) - item id -> price rows (three halfwords each), in groups of six rows. */
extern u16 lb_trade_price_rows[58];
/* .data 0x805BA6D4 (0x44) - the single trade goods and their prices (three halfwords each). */
extern u16 lb_trade_flag_rows[34];
/* .data 0x805BA718 (0x38) - the paired trade goods and their two prices (four halfwords each). */
extern u16 lb_trade_pair_rows[28];
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_POS_TBL_H */
