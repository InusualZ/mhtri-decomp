/* The callees `lobby/lb_npc.cpp`'s control band takes from other units (`enemy/em020_prog.cpp`,
 * `lobby/fn_801E7530.cpp`, `lobby/fn_802FA9A0.cpp`), typed as the callees' own bodies show. */
#ifndef MHTRI_LOBBY_LB_NPC_CALLEES_H
#define MHTRI_LOBBY_LB_NPC_CALLEES_H

#include "types.h"
#include "lobby/lb_npc.h"   /* LbNpcUserData */

#ifdef __cplusplus
extern "C" {
#endif

/* The lobby user-data block pointer (`lobby_world_block`, `.sbss` 0x80794880, 4 bytes).  `lb_item_db_toggle`
 * toggles the block's byte 0x3E00. */
extern LbNpcUserData* lobby_world_block;

u32 game_ready_ck(void);
void fn_801E9888(void);
void fn_801E9C58(void);
void lb_kujira_day_step(void);
u32 lobby_net_busy_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_CALLEES_H */
