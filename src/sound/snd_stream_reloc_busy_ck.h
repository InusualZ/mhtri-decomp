/* sound/snd_stream_reloc_busy_ck.h - leaf header (docs/plan.md 6.5 rule 2): `sound/snd_stream_reloc.cpp`'s stream checks, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_SOUND_SND_STREAM_RELOC_BUSY_CK_H
#define MHTRI_SOUND_SND_STREAM_RELOC_BUSY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E802C / 0x800E8228 - whether the stream under `key` still plays / stops it. */
u32 snd_stream_reloc_busy_ck(u32 key);
void snd_stream_reloc_stop(u32 key);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_STREAM_RELOC_BUSY_CK_H */
