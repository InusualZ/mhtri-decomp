/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_motion.cpp`'s `pl_quest_end_request`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_PL_PL_QUEST_END_REQUEST_H
#define MHTRI_PL_PL_QUEST_END_REQUEST_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028F2F8 (0x70): while quest flag 8 is set, stores the end reason (6 for mode 0, else 4) in the player move work */
void pl_quest_end_request(s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_QUEST_END_REQUEST_H */
