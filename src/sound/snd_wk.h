/* sound/snd_wk.h - leaf header (docs/plan.md 6.5 rule 2): `snd_wk`, which `sound/snd_bank_loader.cpp` owns. */
#ifndef MHTRI_SOUND_SND_WK_H
#define MHTRI_SOUND_SND_WK_H

#include "types.h"
#include "sound/snd_work.h"

/* 0x80794A2C (.sbss): the sound bank loader's work record. */
extern SndWork* snd_wk;

#endif /* MHTRI_SOUND_SND_WK_H */
