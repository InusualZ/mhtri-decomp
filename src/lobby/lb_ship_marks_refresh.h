/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s ship-mark refresh, which the spot departure of
 * `enemy/em_prog_support.cpp` runs.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SHIP_MARKS_REFRESH_H
#define MHTRI_LOBBY_LB_SHIP_MARKS_REFRESH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FC2F0 - sets the ship marks and resets the menu layer. */
void lb_ship_marks_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SHIP_MARKS_REFRESH_H */
