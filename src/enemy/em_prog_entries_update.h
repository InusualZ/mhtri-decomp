/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_prog_support.cpp`'s `em_prog_entries_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_ENEMY_EM_PROG_ENTRIES_UPDATE_H
#define MHTRI_ENEMY_EM_PROG_ENTRIES_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8038575C (0x60): steps the five enemy-program entries and the NPC talk. GUESS name. */
void em_prog_entries_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PROG_ENTRIES_UPDATE_H */
