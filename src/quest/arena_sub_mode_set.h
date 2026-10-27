/* Leaf header (docs/plan.md 6.5 rule 2): `quest/arenatask.cpp`'s `arena_sub_mode_set`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_QUEST_ARENA_SUB_MODE_SET_H
#define MHTRI_QUEST_ARENA_SUB_MODE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804463B0 (0x14): sets the arena task's sub-mode. */
void arena_sub_mode_set(u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_ARENA_SUB_MODE_SET_H */
