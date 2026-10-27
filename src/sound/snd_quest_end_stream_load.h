/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s `snd_quest_end_stream_load`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_SND_QUEST_END_STREAM_LOAD_H
#define MHTRI_SOUND_SND_QUEST_END_STREAM_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F2114 (0x98): requests the quest-end stream file. GUESS name. */
void snd_quest_end_stream_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_QUEST_END_STREAM_LOAD_H */
