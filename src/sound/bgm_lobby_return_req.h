/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_lobby_return_req`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_SOUND_BGM_LOBBY_RETURN_REQ_H
#define MHTRI_SOUND_BGM_LOBBY_RETURN_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F5AC8 - sets BGM request byte 73 at +0x1CC of the sound work and starts bank 2 (fn_800F3054(2)) */
void bgm_lobby_return_req(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_LOBBY_RETURN_REQ_H */
