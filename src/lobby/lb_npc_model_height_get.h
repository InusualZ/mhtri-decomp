/* `lb_npc_model_height_get`, defined by `lobby/lb_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_NPC_MODEL_HEIGHT_GET_H
#define MHTRI_LOBBY_LB_NPC_MODEL_HEIGHT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the current height of NPC model kind `model` (the `npc_model_cur_height_data` table). */
f32 lb_npc_model_height_get(s16 model);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_MODEL_HEIGHT_GET_H */
