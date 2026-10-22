/* sound/snd_master_volume_set.h - leaf header (docs/plan.md 6.5 rule 2): `sound/snd_stream_reloc.cpp`'s master volume and stream pause, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_SOUND_SND_MASTER_VOLUME_SET_H
#define MHTRI_SOUND_SND_MASTER_VOLUME_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E86E8 - sets the AX master volume with interrupts off. */
void snd_master_volume_set(u16 volume);
/* 0x800E82D4 / 0x800E8314 - pause and resume the streams (pending state 2 / 4). */
void snd_stream_reloc_pause(void);
void snd_stream_reloc_resume(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_MASTER_VOLUME_SET_H */
