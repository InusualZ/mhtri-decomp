/* Leaf header (docs/plan.md 6.5 rule 2): `quest/quest_entry`'s `quest_move_sub_state_ck`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_QUEST_QUEST_MOVE_SUB_STATE_CK_H
#define MHTRI_QUEST_QUEST_MOVE_SUB_STATE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803B0D5C (quest/quest_entry): whether the quest move work's sub-state machine runs. */
u32 quest_move_sub_state_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_MOVE_SUB_STATE_CK_H */
