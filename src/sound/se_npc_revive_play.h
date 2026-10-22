/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s `se_npc_revive_play`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_SOUND_SE_NPC_REVIVE_PLAY_H
#define MHTRI_SOUND_SE_NPC_REVIVE_PLAY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DCC18 (0xC): plays the companion's revive system sound (group 28, id 35) */
void se_npc_revive_play(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_NPC_REVIVE_PLAY_H */
