/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_frame_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_STAGE_STAGE_FRAME_UPDATE_H
#define MHTRI_STAGE_STAGE_FRAME_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802ADB98 (0x1E0): the stage's per-frame update. */
void stage_frame_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_FRAME_UPDATE_H */
