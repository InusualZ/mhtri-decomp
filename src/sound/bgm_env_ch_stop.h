/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_env_ch_stop`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_BGM_ENV_CH_STOP_H
#define MHTRI_SOUND_BGM_ENV_CH_STOP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F6338 (0x7C): sends the stop command to every playing stream but stream 1. GUESS name. */
void bgm_env_ch_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_ENV_CH_STOP_H */
