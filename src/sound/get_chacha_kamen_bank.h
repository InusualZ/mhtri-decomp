/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s chacha mask bank getter, a C++ free function (`get_chacha_kamen_bank__Fv`). */
#ifndef MHTRI_SOUND_GET_CHACHA_KAMEN_BANK_H
#define MHTRI_SOUND_GET_CHACHA_KAMEN_BANK_H

#include "types.h"

#ifdef __cplusplus
/* 0x800EF554 - the sound bank of chacha's current mask. */
s32 get_chacha_kamen_bank(void);
#endif

#endif /* MHTRI_SOUND_GET_CHACHA_KAMEN_BANK_H */
