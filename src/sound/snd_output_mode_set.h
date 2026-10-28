/* sound/snd_output_mode_set.h - leaf header (docs/plan.md 6.5 rule 2): `snd_output_mode_set`, which `sound/snd_stream_reloc.cpp` owns. */
#ifndef MHTRI_SOUND_SND_OUTPUT_MODE_SET_H
#define MHTRI_SOUND_SND_OUTPUT_MODE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E87E4 (0x74): selects the AX/MIX output mode. */
void snd_output_mode_set(u16 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_OUTPUT_MODE_SET_H */
