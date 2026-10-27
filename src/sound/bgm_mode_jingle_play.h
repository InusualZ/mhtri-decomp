/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_mode_jingle_play`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_BGM_MODE_JINGLE_PLAY_H
#define MHTRI_SOUND_BGM_MODE_JINGLE_PLAY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F5208 (0x88): plays the quest-end jingle for end kinds 4, 6 and 7 on stream 2. GUESS name. */
void bgm_mode_jingle_play(u8 end_kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_MODE_JINGLE_PLAY_H */
