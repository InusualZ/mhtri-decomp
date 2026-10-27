/* sound/snd_whd_install.h - leaf header (docs/plan.md 6.5 rule 2): `snd_whd_install`, which `sound/snd_stream_reloc.cpp` owns. */
#ifndef MHTRI_SOUND_SND_WHD_INSTALL_H
#define MHTRI_SOUND_SND_WHD_INSTALL_H

#include "types.h"

struct RelocDesc;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E7DE4 (0xB0): installs a chunk's `whd` descriptor tables into stream handle `idx`. */
void snd_whd_install(u32 idx, struct RelocDesc* desc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_WHD_INSTALL_H */
