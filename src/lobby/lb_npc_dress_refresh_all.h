/* lobby/lb_npc_dress_refresh_all.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s
 *   `lb_npc_dress_refresh_all`, `lb_npc_talk_ok_npc23` and `lb_npc_talk_ok_npc73`, for `lobby/fn_802FA9A0.cpp`.  C
 *   linkage (the map rows are plain names); the signatures are the owner's definitions. */
#ifndef MHTRI_LOBBY_LB_NPC_DRESS_REFRESH_ALL_H
#define MHTRI_LOBBY_LB_NPC_DRESS_REFRESH_ALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FF60C: re-applies every live NPC's dressing. */
void lb_npc_dress_refresh_all(void);
/* 0x801FE6F0 / 0x801FE844: whether NPC 23 / NPC 73 may be talked to. */
s32 lb_npc_talk_ok_npc23(u32 id);
s32 lb_npc_talk_ok_npc73(u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_DRESS_REFRESH_ALL_H */
