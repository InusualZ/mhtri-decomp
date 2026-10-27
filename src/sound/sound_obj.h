/* sound/sound_obj.h - the declarations `sound/sound_obj.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SOUND_OBJ_H
#define MHTRI_SOUND_SOUND_OBJ_H

#include "types.h"

struct ReverbMgr;
struct Sound;
struct Sound2;

#ifdef __cplusplus
extern "C" {
#endif
Sound* snd_obj_get(void);
void snd_cmd_list_run(Sound2* self);
ReverbMgr* snd_reverb_mgr_get(void);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
void snd_cmd_post(void* unused, u32 owner, u32 kind, u8 a, u8 b, u8 c, u8 d, s16 e, s16 f);
void* snd_obj_play_pan(Sound* snd, u32 a, u32 b, u32 c);
void* snd_obj_play_pos(Sound* snd, u32 a, u32 b, u32 c, u32 d);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
void* snd_line_cue_get(void* p, u32 idx);
void* snd_obj_play_pos_reset(Sound* snd, u32 a, u32 b, u32 c, u32 d);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
u32 snd_obj_owner_voice_count(void* unused, u32 key);
void snd_obj_reverb_apply(Sound* snd, u32 idx, u32 entry_idx);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
s16 snd_obj_owner_pitch_get(void* unused, u32 key);
/* untyped: an opaque handle passed through (the callee reads no field of it) */
void snd_obj_owner_lpf_set(void* unused, u32 key, u16 pitch);
#ifdef __cplusplus
}
#endif

#endif
