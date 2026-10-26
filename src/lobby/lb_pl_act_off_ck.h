/* lobby/lb_pl_act_off_ck.h - leaf header: `lb_pl_act_off_ck` (0x80208AB8), owned by `lobby/lb_npc.cpp`. C linkage. */
#ifndef MHTRI_LOBBY_LB_PL_ACT_OFF_CK_H
#define MHTRI_LOBBY_LB_PL_ACT_OFF_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* The lobby act check that always answers "no". */
u32 lb_pl_act_off_ck(struct _PLW* self);
#ifdef __cplusplus
}
#endif

#endif
