/* sound/snd_voice_pool.h - the declarations `sound/snd_voice_pool.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_VOICE_POOL_H
#define MHTRI_SOUND_SND_VOICE_POOL_H

#include "types.h"

struct SndNode2;
struct SndVoiceMgr;

#ifdef __cplusplus
extern "C" {
#endif
void fn_800EDA88(SndNode2* list, SndNode2* node);
void* fn_800EDB24(SndVoiceMgr* self);
void fn_800EDB74(void* table);
void snd_bank1_install(struct StreamWork* table, u32 id);
void fn_800EDD00(void* table, u32 id, u32 idx);
void snd_bank2_install(struct StreamWork* table, u32 id);
void snd_bank3_install(struct StreamWork* table, u32 id);
extern u8 lbl_8069A8E8[0x6828]; /* the sound table `snd_bank1_install`/`fn_800EDD00` install into */
#ifdef __cplusplus
}
#endif

#endif
