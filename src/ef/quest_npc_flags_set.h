/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `quest_npc_flags_set`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_QUEST_NPC_FLAGS_SET_H
#define MHTRI_EF_QUEST_NPC_FLAGS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D06F0 (0x18): raises `system_w`'s +0x7CC/+0x7CD bytes, which the quest scene copies into its NPC flags. GUESS name. */
void quest_npc_flags_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_QUEST_NPC_FLAGS_SET_H */
