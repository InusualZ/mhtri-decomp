/* sound/chacha_dance_bank_find.h - leaf header (docs/plan.md 6.5 rule 2): `chacha_dance_bank_find`, which `sound/snd_bank_loader.cpp` owns. */
#ifndef MHTRI_SOUND_CHACHA_DANCE_BANK_FIND_H
#define MHTRI_SOUND_CHACHA_DANCE_BANK_FIND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EF56C (0x5C): the chunk (0x20/0x21) Chacha's dance `id` is loaded in, -1 when neither dance slot holds it. */
s32 chacha_dance_bank_find(u8 slot, u8 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_CHACHA_DANCE_BANK_FIND_H */
