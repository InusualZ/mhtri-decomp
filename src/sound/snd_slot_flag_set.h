/* sound/snd_slot_flag_set.h - leaf: the voice-node, relocation-state and player-base functions `sound/sound_obj.cpp`
 * defines (docs/plan.md 6.5 rule 2), with the signatures `sound/snd_stream_mgr.cpp` calls them through. */
#ifndef MHTRI_SOUND_SND_SLOT_FLAG_SET_H
#define MHTRI_SOUND_SND_SLOT_FLAG_SET_H

#include "types.h"

#ifdef __cplusplus
class StreamMgr;
struct SndPlayerBase;
struct StreamNode;
struct StreamRelocState;
struct StreamGroup;
struct SndPool2;
struct ReverbMgr;

extern "C" {
#endif

SndPlayerBase* snd_player_base_construct(SndPlayerBase* base);
StreamMgr* snd_player_base_destruct(StreamMgr* obj, s32 flags);
StreamRelocState* snd_reloc_state_release(StreamRelocState* state, s32 flags);
StreamRelocState* snd_reloc_state_clear(StreamRelocState* state);
StreamGroup* snd_reloc_state_entry(StreamRelocState* state, u32 idx);
void snd_slot_flag_set(StreamNode* node, u32 mask);
void snd_slot_flag_clear(StreamNode* node, u32 mask);
u32 snd_slot_flag_test(StreamNode* node, u32 mask);
u32 snd_voice_list_end_ck(SndPool2* pool, StreamNode* node);
StreamNode* snd_voice_next(StreamNode* node);
StreamNode* snd_voice_prev(StreamNode* node);
u32 snd_voice_list_head_ck(SndPool2* pool, StreamNode* node);
u32 snd_reverb_flags_get(ReverbMgr* mgr);

#ifdef __cplusplus
}
#endif

#endif
