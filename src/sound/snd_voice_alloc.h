/* sound/snd_voice_alloc.h - leaf: the voice-pool functions `sound/snd_voice_pool.cpp` defines that its own header does
 * not declare yet (docs/plan.md 6.5 rule 2), with the signatures `sound/snd_stream_mgr.cpp` calls them through. */
#ifndef MHTRI_SOUND_SND_VOICE_ALLOC_H
#define MHTRI_SOUND_SND_VOICE_ALLOC_H

#include "types.h"

#ifdef __cplusplus
struct StreamNode;
struct SndPool2;

extern "C" {
#endif

StreamNode* snd_voice_alloc(SndPool2* pool);
StreamNode* snd_voice_last(SndPool2* pool);

#ifdef __cplusplus
}
#endif

#endif
