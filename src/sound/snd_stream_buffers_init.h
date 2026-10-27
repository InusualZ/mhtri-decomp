/* sound/snd_stream_buffers_init.h - leaf header (docs/plan.md 6.5 rule 2): `snd_stream_buffers_init`, which `sound/fn_800E46E8.cpp` owns. */
#ifndef MHTRI_SOUND_SND_STREAM_BUFFERS_INIT_H
#define MHTRI_SOUND_SND_STREAM_BUFFERS_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E4A24 (0x58): resets the volume block and sets the stream buffers up over [base, base + size). */
void snd_stream_buffers_init(u32 base, u32 size, u32 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_STREAM_BUFFERS_INIT_H */
