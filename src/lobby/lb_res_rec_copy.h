/* lobby/lb_res_rec_copy.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s 8-byte resource record copy,
 * for `Pl/player_control.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_LOBBY_LB_RES_REC_COPY_H
#define MHTRI_LOBBY_LB_RES_REC_COPY_H

#include "types.h"
#include "lobby/lb_npc.h"   /* LbResRec, LbResId */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FCAC8 (0x14): copies the two words of a resource id record into `dst`. */
void lb_res_rec_copy(LbResRec* dst, const LbResId* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_RES_REC_COPY_H */
