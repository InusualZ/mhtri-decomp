/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s stream buffer getter (registry name; GUESS). */
#ifndef MHTRI_SOUND_SND_STREAM_DMA_GET_H
#define MHTRI_SOUND_SND_STREAM_DMA_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F2900 - the loaded stream buffer's address, or 0 while it is not loaded. */
u32 snd_stream_dma_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_STREAM_DMA_GET_H */
