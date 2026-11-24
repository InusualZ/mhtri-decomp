/* sound/snd_voice_pool.h - the declarations `sound/snd_voice_pool.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_VOICE_POOL_H
#define MHTRI_SOUND_SND_VOICE_POOL_H

#include "types.h"

struct SndNode2;
struct SndVoiceMgr;
#ifdef __cplusplus
class SndVoicePool;
#endif

#ifdef __cplusplus
extern "C" {
#endif
void snd_voice_free(SndNode2* list, SndNode2* node);
void* snd_voice_first(SndVoiceMgr* self);
void snd_voice_pool_frame_update(struct StreamWork* table);
void snd_bank1_install(struct StreamWork* table, s32 kind);
void snd_bank1_install_idx(struct StreamWork* table, s32 kind, u32 idx);
void snd_bank2_install(struct StreamWork* table, s32 kind);
void snd_bank3_install(struct StreamWork* table, s32 kind);
extern SndVoicePool g_snd_voice_pool; /* the 96-voice pool `snd_bank_table_get` returns */
#ifdef __cplusplus
}
#endif

#endif
