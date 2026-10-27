/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/player_control.cpp`'s `pl_players_ready_ck`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_PL_PL_PLAYERS_READY_CK_H
#define MHTRI_PL_PL_PLAYERS_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802673E8 (0x160): 1 when every present player's ready byte (+0x267) is set. GUESS name. */
s32 pl_players_ready_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_PLAYERS_READY_CK_H */
