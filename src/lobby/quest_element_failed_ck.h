/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_quest_screen.cpp` symbols `quest/quest_entry.cpp and enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_LOBBY_QUEST_ELEMENT_FAILED_CK_H
#define MHTRI_LOBBY_QUEST_ELEMENT_FAILED_CK_H

#include "types.h"

struct QuestWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803AA2A0 - element `index` carries flag 0x20; 0x803AA2C0 - element `index` is live; 0x803AA1B8 - element
 * `index` failed; 0x803AA2F0 - the quest failed; 0x803A8FB0 - the quest clock step. */
u32 quest_element_flag20_ck(struct QuestWork* work, s32 index);
s32 quest_element_live_ck(struct QuestWork* work, s32 index);
u32 quest_element_failed_ck(struct QuestWork* work, s32 index);
u32 quest_failed_ck(struct QuestWork* work);
void quest_clock_step(s32 mode);
/* 0x803A888C - records the quest id the lobby picked. */
void quest_id_set(s32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_QUEST_ELEMENT_FAILED_CK_H */
