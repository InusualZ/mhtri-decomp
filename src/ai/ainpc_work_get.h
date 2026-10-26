/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s ainpc_work_get */
#ifndef MHTRI_AI_AINPC_WORK_GET_H
#define MHTRI_AI_AINPC_WORK_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D27E0 - the address of the AI companion record `ainpc_w`. */
u8* ainpc_work_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AINPC_WORK_GET_H */
