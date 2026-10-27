/* sound/snd_heap_allocator.h - leaf header (docs/plan.md 6.5 rule 2): `snd_heap_allocator`, which `sound/quest_snd.cpp` owns. */
#ifndef MHTRI_SOUND_SND_HEAP_ALLOCATOR_H
#define MHTRI_SOUND_SND_HEAP_ALLOCATOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x806A1110 (.bss, 0x10): the allocator record over `snd_exp_heap`. */
extern u8 snd_heap_allocator[0x10];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_HEAP_ALLOCATOR_H */
