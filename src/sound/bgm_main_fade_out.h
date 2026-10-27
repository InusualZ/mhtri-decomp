/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_main_fade_out`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_BGM_MAIN_FADE_OUT_H
#define MHTRI_SOUND_BGM_MAIN_FADE_OUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F6514 (0xC): sends the end command to stream 0. GUESS name. */
void bgm_main_fade_out(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_MAIN_FADE_OUT_H */
