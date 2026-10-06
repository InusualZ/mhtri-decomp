/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbol `menu/multi_result.cpp` calls.  C++ scope: the map row
 * is the mangling `ai_torch_ck__FP8_AINPC_W`. */
#ifndef MHTRI_AI_AI_TORCH_CK_H
#define MHTRI_AI_AI_TORCH_CK_H

#include "types.h"

struct _AINPC_W;

#ifdef __cplusplus
/* 0x802D8414 - whether the AI companion is carrying a lit torch. */
u32 ai_torch_ck(struct _AINPC_W* self);
#endif

#endif /* MHTRI_AI_AI_TORCH_CK_H */
