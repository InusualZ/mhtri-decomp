/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_ch_fade_out`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_BGM_CH_FADE_OUT_H
#define MHTRI_SOUND_BGM_CH_FADE_OUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F51B8 (0x28): sends the end command to stream `stream` while it plays. GUESS name. */
void bgm_ch_fade_out(s32 stream);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_CH_FADE_OUT_H */
