/* sound/snd_reverb_unit_set.h - the declaration of `snd_reverb_unit_set`, which `sound/fn_800E46E8.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_SOUND_SND_REVERB_UNIT_SET_H
#define MHTRI_SOUND_SND_REVERB_UNIT_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void snd_reverb_unit_set(struct ReverbMgr* mgr, struct ReverbCfg* cfg);
#ifdef __cplusplus
}
#endif

#endif
