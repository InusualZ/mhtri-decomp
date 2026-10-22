/* sound/pl_voice_bank_load.h - leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s voice bank load, for `menu/movie.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_SOUND_PL_VOICE_BANK_LOAD_H
#define MHTRI_SOUND_PL_VOICE_BANK_LOAD_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EFC68 - queues the voice bank files of `plw`'s voice. */
void pl_voice_bank_load(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PL_VOICE_BANK_LOAD_H */
