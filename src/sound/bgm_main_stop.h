/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_main_stop`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_BGM_MAIN_STOP_H
#define MHTRI_SOUND_BGM_MAIN_STOP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F51E0 (0x28): sends the stop command to stream 0 while it plays. GUESS name. */
void bgm_main_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_MAIN_STOP_H */
