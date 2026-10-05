/* sound/snd_reloc_state_rebase.h - the declaration of `snd_reloc_state_rebase`, which `sound/fn_800E46E8.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_SOUND_SND_RELOC_STATE_REBASE_H
#define MHTRI_SOUND_SND_RELOC_STATE_REBASE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void snd_reloc_state_rebase(struct RelocState* state, struct RelocTable* table);
#ifdef __cplusplus
}
#endif

#endif
