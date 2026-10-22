/* Leaf header (docs/plan.md 6.5 rule 2): `nw_resource.cpp`'s `nw_res_list_load`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_NW_RES_LIST_LOAD_H
#define MHTRI_NW_RES_LIST_LOAD_H

#include "types.h"
#include "lobby/lb_npc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D5CAC (0x6C): requests every {id, name} record of the list that is not resident yet; returns the count */
s32 nw_res_list_load(LbResId* list);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NW_RES_LIST_LOAD_H */
