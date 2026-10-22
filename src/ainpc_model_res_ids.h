/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s `ainpc_model_res_ids`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_AINPC_MODEL_RES_IDS_H
#define MHTRI_AINPC_MODEL_RES_IDS_H

#include "types.h"
#include "lobby/lb_npc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8058AA70 (0x10): the AI NPC model's resource records (the first name builds the model) */
extern LbResId ainpc_model_res_ids[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AINPC_MODEL_RES_IDS_H */
