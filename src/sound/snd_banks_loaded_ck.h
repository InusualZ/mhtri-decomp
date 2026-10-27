/* Leaf header (docs/plan.md 6.5 rule 2): `sound/snd_bank_loader.cpp`'s `snd_banks_loaded_ck`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SOUND_SND_BANKS_LOADED_CK_H
#define MHTRI_SOUND_SND_BANKS_LOADED_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800EF1B4 (0xBC): 1 once the required number of sound banks is loaded. GUESS name. */
s32 snd_banks_loaded_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_BANKS_LOADED_CK_H */
