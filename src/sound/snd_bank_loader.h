/* sound/snd_bank_loader.h - the declarations `sound/snd_bank_loader.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_BANK_LOADER_H
#define MHTRI_SOUND_SND_BANK_LOADER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void fn_800F0F9C(u8 id);
#ifdef __cplusplus
}
#endif

#endif
