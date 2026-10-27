/* Leaf header (docs/plan.md 6.5 rule 2): `ai/ai_npc.cpp`'s `ai_npc_skill_group_get`, which `lobby/lb_companion_ui.cpp`
 * calls.  C linkage (the map row is a plain name). */
#ifndef MHTRI_AI_AI_NPC_SKILL_GROUP_GET_H
#define MHTRI_AI_AI_NPC_SKILL_GROUP_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802D7688 (0xB4): the behaviour list (0..2) skill `skill` belongs to, 3 when in none.  GUESS name. */
s32 ai_npc_skill_group_get(u8 skill);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_AI_NPC_SKILL_GROUP_GET_H */
