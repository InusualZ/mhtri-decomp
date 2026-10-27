/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s `snd_bank_loader_init`. */
#ifndef MHTRI_SOUND_SND_BANK_LOADER_INIT_H
#define MHTRI_SOUND_SND_BANK_LOADER_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EEAE0 (0x414): brings the sound system up (the work record, the sound heap, the audio stack, the bank tables). */
void snd_bank_loader_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_BANK_LOADER_INIT_H */
