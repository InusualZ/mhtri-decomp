/* The lobby menu-position unit `lobby/lb_menu_pos_tbl.cpp`: the lobby's own guard the NPC control band polls. */
#ifndef MHTRI_LOBBY_LB_MENU_POS_TBL_H
#define MHTRI_LOBBY_LB_MENU_POS_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The lobby's own guard: nonzero while a dialogue/prompt owns input. */
s32 fn_8021F238(void);

/* 0x8021F020 - hands the item list the clock: the stamp, the row count (and its per-tick step) and the
 * scroll depth (GUESS name: the network control's schedule sync is the caller). */
void syncItemListClock(u32 stamp, u32 count, f32 depth);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_POS_TBL_H */
