/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_prog_support.cpp`'s `lb_spot_screen_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_ENEMY_LB_SPOT_SCREEN_OPEN_H
#define MHTRI_ENEMY_LB_SPOT_SCREEN_OPEN_H

#include "types.h"

struct _PLW;
struct LbSpot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80383148 - clears the lobby screen work and opens the spot screen for `spot` */
void lb_spot_screen_open(struct _PLW* self, struct LbSpot* spot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_LB_SPOT_SCREEN_OPEN_H */
