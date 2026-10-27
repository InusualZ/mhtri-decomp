/* sound/snd_heap_free.h - leaf header (docs/plan.md 6.5 rule 2): `snd_heap_free`, which `sound/quest_snd.cpp` owns. */
#ifndef MHTRI_SOUND_SND_HEAP_FREE_H
#define MHTRI_SOUND_SND_HEAP_FREE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EEA2C (0x18): frees a `snd_heap_alloc` block back to `snd_exp_heap`. */
/* untyped: byte range - a raw heap block */
void snd_heap_free(void* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_HEAP_FREE_H */
