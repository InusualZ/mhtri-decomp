/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s `lb_pl_act_idle_ck`, which `lobby/lb_menu_pos_tbl.cpp` calls. */
#ifndef MHTRI_LOBBY_LB_PL_ACT_IDLE_CK_H
#define MHTRI_LOBBY_LB_PL_ACT_IDLE_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80208A84 (0x34): whether the player's act layer is idle (not acts 9/11/12, and +0x30E set). */
u32 lb_pl_act_idle_ck(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PL_ACT_IDLE_CK_H */
