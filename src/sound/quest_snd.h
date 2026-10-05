/* sound/quest_snd.h - the declarations `sound/quest_snd.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_QUEST_SND_H
#define MHTRI_SOUND_QUEST_SND_H

#include "types.h"

struct AxHandle;
struct AxVoice;

#ifdef __cplusplus
extern "C" {
#endif
void fn_800EE014(AxHandle* handle, AxVoice* voice, s16 b, u16 c);
#ifdef __cplusplus
}
#endif

#endif
