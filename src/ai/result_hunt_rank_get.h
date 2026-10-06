/* Leaf header (docs/plan.md 6.5 rule 2): the `ai/ai_npc.cpp` symbols `quest/quest_entry.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_AI_RESULT_HUNT_RANK_GET_H
#define MHTRI_AI_RESULT_HUNT_RANK_GET_H

#include "types.h"

/* The quest result's rank helpers (GUESS names from `quest/quest_entry.cpp`'s result fill): the hunt rank and
 * the rank points of the two count runs, the rank of the elapsed minutes, and the rank breakdown rows. */
struct Q_CountSet;
#ifdef __cplusplus
extern "C" {
#endif
s16 result_hunt_rank_get(struct Q_CountSet* counts_c, struct Q_CountSet* counts_d);
s16 result_time_rank_get(u16 minutes);
void result_rank_rows_fill(u8* rows);
s32 result_rank_points_get(struct Q_CountSet* counts_c, struct Q_CountSet* counts_d);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_AI_RESULT_HUNT_RANK_GET_H */
