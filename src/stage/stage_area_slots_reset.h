/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_area_slots_reset`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_STAGE_STAGE_AREA_SLOTS_RESET_H
#define MHTRI_STAGE_STAGE_AREA_SLOTS_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802AFA40 (0x3C): frees every stage area slot. */
void stage_area_slots_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_SLOTS_RESET_H */
