/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s `snd_result_banks_load`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_SND_RESULT_BANKS_LOAD_H
#define MHTRI_SOUND_SND_RESULT_BANKS_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EF690 (0x148): requests the result screen's two sound bank pairs. GUESS name. */
void snd_result_banks_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_RESULT_BANKS_LOAD_H */
