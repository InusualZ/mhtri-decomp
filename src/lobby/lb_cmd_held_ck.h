/* `lb_cmd_held_ck`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_CMD_HELD_CK_H
#define MHTRI_LOBBY_LB_CMD_HELD_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Whether the command bits are held, in the view the lobby page units call it with. */
u32 lb_cmd_held_ck(s32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CMD_HELD_CK_H */
