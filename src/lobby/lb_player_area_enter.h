/* `lb_player_area_enter`, defined by `lobby/lb_npc.cpp` (leaf header, docs/plan.md 6.5 rule 2).  C linkage (the map
 * row is a plain name). */
#ifndef MHTRI_LOBBY_LB_PLAYER_AREA_ENTER_H
#define MHTRI_LOBBY_LB_PLAYER_AREA_ENTER_H

#include "types.h"
#include "lobby/lb_npc.h" /* LbPlaceRec */

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Places `self` at the area entry `place` (`quiet` skips the area's place info). */
void lb_player_area_enter(struct _PLW* self, LbPlaceRec* place, u32 quiet);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PLAYER_AREA_ENTER_H */
