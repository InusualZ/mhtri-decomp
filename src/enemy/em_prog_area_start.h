/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_prog_support.cpp`'s `em_prog_area_start`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_ENEMY_EM_PROG_AREA_START_H
#define MHTRI_ENEMY_EM_PROG_AREA_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803855D4 (0xC4): starts the enemy-program support entries the current map and area need. GUESS name. */
void em_prog_area_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PROG_AREA_START_H */
