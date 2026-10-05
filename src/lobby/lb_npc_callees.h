/* The callees `lobby/lb_npc.cpp`'s lobby control band (0x8020C588..0x80212760) takes from other units.
 *
 * Declared here (not in their owners' headers) because no header of theirs does it yet - `fn_803768F8` is
 * `enemy/em020_ai.cpp`'s, `fn_801E9888`/`fn_801E9C58` are `lobby/fn_801E7530.cpp`'s and have no header, and
 * `fn_802FF2C0` is `lobby/fn_802FA9A0.cpp`'s; the argument types are the ones the callees' own bodies show.
 */
#ifndef MHTRI_LOBBY_LB_NPC_CALLEES_H
#define MHTRI_LOBBY_LB_NPC_CALLEES_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The lobby item-database pointer (`lobby_world_block`, `.sbss` 0x80794880, 4 bytes).  `fn_802125C8`
 * toggles the byte 0x3E00 of the block it points at. */
extern u8* lobby_world_block;

s32 game_ready_ck(void);
void fn_801E9888(void);
void fn_801E9C58(void);
void fn_802FF2C0(void);
u32 fn_803768F8(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_CALLEES_H */
