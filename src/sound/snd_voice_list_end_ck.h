/* sound/snd_voice_list_end_ck.h - leaf: the voice-list end test and the slot flag test `sound/sound_obj.cpp` defines
 * (docs/plan.md 6.5 rule 2), with the signatures `sound/snd_voice_pool.cpp` calls them through. */
#ifndef MHTRI_SOUND_SND_VOICE_LIST_END_CK_H
#define MHTRI_SOUND_SND_VOICE_LIST_END_CK_H

#include "types.h"

struct SndPool2;
struct SndNode2;

#ifdef __cplusplus
extern "C" {
#endif
u32 snd_voice_list_end_ck(SndPool2* pool, SndNode2* node);
u32 snd_slot_flag_test(SndNode2* node, u32 mask);
#ifdef __cplusplus
}
#endif

#endif
