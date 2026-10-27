/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/player_control.cpp`'s `pl_players_draw`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_PL_PL_PLAYERS_DRAW_H
#define MHTRI_PL_PL_PLAYERS_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802680C4 (0x110): draws every live player work. GUESS name. */
void pl_players_draw(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_PLAYERS_DRAW_H */
