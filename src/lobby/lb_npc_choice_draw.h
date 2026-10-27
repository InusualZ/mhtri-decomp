/* `lb_npc_choice_draw`, defined by `lobby/fn_801E7530.cpp` (leaf header, rule 2).  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_LOBBY_LB_NPC_CHOICE_DRAW_H
#define MHTRI_LOBBY_LB_NPC_CHOICE_DRAW_H

#include "types.h"
#include "lobby/lb_list.h"   /* LbList */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E8994 - draws an NPC talk choice list `list` on panel 0x1D2D with the help line on 0x1877. */
void lb_npc_choice_draw(LbList* list);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_CHOICE_DRAW_H */
