/* Leaf header (docs/plan.md 6.5 rule 2): `snd_em_bank_level_load`, which `sound/snd_bank_loader` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_SOUND_SND_EM_BANK_LEVEL_LOAD_H
#define MHTRI_SOUND_SND_EM_BANK_LEVEL_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F0D88 - the enemy sound bank setup of a level. */
void snd_em_bank_level_load(u8 level);

#ifdef __cplusplus
}
#endif

#endif
