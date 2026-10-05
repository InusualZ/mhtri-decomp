/* sound/snd_reverb_state_advance.h - the declaration of `snd_reverb_state_advance`, which `sound/fn_800E46E8.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_SOUND_SND_REVERB_STATE_ADVANCE_H
#define MHTRI_SOUND_SND_REVERB_STATE_ADVANCE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void snd_reverb_state_advance(struct ReverbMgr* mgr, u32 idx);
#ifdef __cplusplus
}
#endif

#endif
