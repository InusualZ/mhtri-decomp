/* Leaf header (docs/plan.md 6.5 rule 2): the two `Pl/fn_80273B14.cpp` symbols `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header (its `Psw` declaration clashes with the
 * lobby band's).  Both are C linkage (the map rows are plain names). */
#ifndef MHTRI_PL_PL_ACT_STAGE_LATCH_SET_H
#define MHTRI_PL_PL_ACT_STAGE_LATCH_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80274748 - arms the player's follow-up stage latch (+0x1F) and its 2-frame hold (+0x396) when the
 * player is the master.  GUESS name from those two stores. */
void pl_act_stage_latch_set(struct _PLW* plw, s8 stage);
/* 0x80274810 - the local player's work record (the third move-work array's `my_player_no` entry). */
struct _PLW* my_player_work_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ACT_STAGE_LATCH_SET_H */
