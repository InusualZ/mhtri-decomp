/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbols `enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_AI_AINPC_INIT_H
#define MHTRI_AI_AINPC_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C2700 / 0x802C2860 - set the NPC work up, and load its resources (GUESS names). */
void ainpc_init(void);
void ainpc_res_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_INIT_H */
