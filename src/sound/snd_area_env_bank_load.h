/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s `snd_area_env_bank_load`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_SND_AREA_ENV_BANK_LOAD_H
#define MHTRI_SOUND_SND_AREA_ENV_BANK_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F1700 (0x1F0): requests the area's sound bank pair for `map`/`area` at `rank`. GUESS name. */
void snd_area_env_bank_load(u8 map, u8 area, u8 rank);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_AREA_ENV_BANK_LOAD_H */
