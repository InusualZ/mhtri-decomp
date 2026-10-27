/* `ai_npc_level_get`, defined by `ai/ai_npc.cpp` (leaf header, rule 2). */
#ifndef MHTRI_AI_AI_NPC_LEVEL_GET_H
#define MHTRI_AI_AI_NPC_LEVEL_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the level (1-30) the points value `points` reaches in the threshold table. */
u8 ai_npc_level_get(u16 points);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_LEVEL_GET_H */
