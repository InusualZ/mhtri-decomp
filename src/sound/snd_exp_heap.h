/* sound/snd_exp_heap.h - leaf header (docs/plan.md 6.5 rule 2): `snd_exp_heap`, which `sound/quest_snd.cpp` owns. */
#ifndef MHTRI_SOUND_SND_EXP_HEAP_H
#define MHTRI_SOUND_SND_EXP_HEAP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794A20 (.sbss): the expand heap the bank loader init creates for the sound heap. */
extern u32 snd_exp_heap;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_EXP_HEAP_H */
