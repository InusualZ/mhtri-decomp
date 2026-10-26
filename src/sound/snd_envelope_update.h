/* sound/snd_envelope_update.h - leaf: the voice envelope steps `sound/fn_800E8E60.cpp` defines (docs/plan.md 6.5
 * rule 2), with the signatures `sound/snd_stream_mgr.cpp` calls them through. */
#ifndef MHTRI_SOUND_SND_ENVELOPE_UPDATE_H
#define MHTRI_SOUND_SND_ENVELOPE_UPDATE_H

#include "types.h"

#ifdef __cplusplus
class StreamMgr;
struct StreamNode;
struct AxVoiceBlock;

extern "C" {
#endif

void snd_envelope_update(StreamMgr* player, StreamNode* node, AxVoiceBlock* env);
void snd_envelope_release(StreamMgr* player, AxVoiceBlock* env);

#ifdef __cplusplus
}
#endif

#endif
