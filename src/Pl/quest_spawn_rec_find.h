/* Leaf header (docs/plan.md 6.5 rule 2): the `Pl/pl_motion.cpp` symbols `enemy/em_pop.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_PL_QUEST_SPAWN_REC_FIND_H
#define MHTRI_PL_QUEST_SPAWN_REC_FIND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028EF7C - the move work's spawn record with spawn order `order` (NULL when none); the owner names the
 * record `PlRootEntry`.  GUESS name. */
struct QuestSpawnRec;
struct QuestSpawnRec* quest_spawn_rec_find(u32 order);
/* 0x8028F0B4 - the same search inside area list `index` only.  GUESS name. */
struct QuestSpawnRec* quest_spawn_rec_find_in(u32 order, u32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_QUEST_SPAWN_REC_FIND_H */
