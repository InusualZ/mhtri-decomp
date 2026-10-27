/* sound/snd_chunk_install.h - leaf header (docs/plan.md 6.5 rule 2): `snd_chunk_install`, which `sound/quest_snd.cpp` owns. */
#ifndef MHTRI_SOUND_SND_CHUNK_INSTALL_H
#define MHTRI_SOUND_SND_CHUNK_INSTALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EEA44 (0x9C): installs a loaded chunk's `whd`/`tsb` pair into stream handle `ctx`. */
void snd_chunk_install(u32 ctx, u32 chunk);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_CHUNK_INSTALL_H */
