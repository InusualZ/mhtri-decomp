/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800F2A94.cpp`'s `bgm_quest_clear_frame`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_BGM_QUEST_CLEAR_FRAME_H
#define MHTRI_SOUND_BGM_QUEST_CLEAR_FRAME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F32EC (0x200): the BGM work's per-frame step. GUESS name. */
void bgm_quest_clear_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_BGM_QUEST_CLEAR_FRAME_H */
