/* sound/snd_audio_init.h - leaf header (docs/plan.md 6.5 rule 2): `snd_audio_init`, which `sound/snd_stream_reloc.cpp` owns. */
#ifndef MHTRI_SOUND_SND_AUDIO_INIT_H
#define MHTRI_SOUND_SND_AUDIO_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E8730 (0xB0): brings the AI/AX/MIX/SYN/SEQ audio stack up. */
void snd_audio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_AUDIO_INIT_H */
