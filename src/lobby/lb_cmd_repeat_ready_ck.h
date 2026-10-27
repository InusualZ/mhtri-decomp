/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_cmd_repeat_ready_ck`.  C linkage (the map row is
 * a plain name). */
#ifndef MHTRI_LOBBY_LB_CMD_REPEAT_READY_CK_H
#define MHTRI_LOBBY_LB_CMD_REPEAT_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021DEFC - the repeat check of `buttons`, while the game is ready or progress flag 99 is set. */
s32 lb_cmd_repeat_ready_ck(u16 buttons);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CMD_REPEAT_READY_CK_H */
