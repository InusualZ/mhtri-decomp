/* Leaf header (docs/plan.md 6.5 rule 2): the scene and label helpers `lobby/lb_npc.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_NPC_SCENE_START_H
#define MHTRI_LOBBY_LB_NPC_SCENE_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80207AA4 - starts the NPC's work scene (`mode` 1 offline, 0 online). */
void lb_npc_scene_start(u8 mode);
/* 0x80207AF8 - whether the NPC's work scene has finished. */
u32 lb_npc_scene_done_ck(void);
/* 0x80212760 - the label of sort order `order`. */
s8* lb_sort_order_name_get(u8 order);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_SCENE_START_H */
