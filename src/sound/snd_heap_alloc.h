/* sound/snd_heap_alloc.h - leaf header (docs/plan.md 6.5 rule 2): `snd_heap_alloc`, which `sound/quest_snd.cpp` owns. */
#ifndef MHTRI_SOUND_SND_HEAP_ALLOC_H
#define MHTRI_SOUND_SND_HEAP_ALLOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EE9EC (0x40): allocates `size` bytes from `snd_heap_allocator` (null for a zero request). */
/* untyped: byte range - a raw heap block */
void* snd_heap_alloc(u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_HEAP_ALLOC_H */
