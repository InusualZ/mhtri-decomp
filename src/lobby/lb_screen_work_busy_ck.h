/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_801E7530.cpp`'s `lb_screen_work_busy_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SCREEN_WORK_BUSY_CK_H
#define MHTRI_LOBBY_LB_SCREEN_WORK_BUSY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801EC7E4 - 1 when the lobby screen work (lobby_w+0xAC) has its +4 byte set */
u32 lb_screen_work_busy_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCREEN_WORK_BUSY_CK_H */
