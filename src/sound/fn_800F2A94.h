#ifndef MHTRI_SOUND_FN_800F2A94_H
#define MHTRI_SOUND_FN_800F2A94_H

#include "types.h"

/* Declarations for the symbols `src/sound/fn_800F2A94.cpp` owns (docs/plan.md 6.5 rule 2).
 * `sound/fn_800EF7D8.cpp` called fn_800F48F4 through its own extern until this unit registered - the two
 * ranges are adjacent, so the ownership only settled when both were landed.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern "C" void snd_bgm_hold_set(void);
/* 0x800F4734 - switches the BGM for the quest scene (`quest_play_state_ck` picks the stream); the quest
 * entry's notify path and the lobby's entry dispatch call it.  GUESS name from those uses. */
extern "C" void snd_quest_scene_set(void);
/* 0x800F486C / 0x800F4538 - BGM request flags of the quest result / quest start scenes (set the quest
 * work's stream flags and re-arm the stream).  GUESS names from the scene they are called for. */
extern "C" void snd_quest_result_bgm_set(void);
extern "C" void snd_quest_start_bgm_set(void);
/* 0x800F46D8 - sets the bit of player `player` in the BGM work's +0x1D7 mask (a no-op without the BGM work).
 * GUESS name. */
extern "C" void snd_player_mask_set(u8 player);
/* 0x800F4704 - clears the bit of player `player` in the same +0x1D7 mask (a no-op without the BGM work).
 * GUESS name, the pair of `snd_player_mask_set`. */
extern "C" void snd_player_mask_clear(u8 player);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800F6334 - stops every BGM stream; the map's `bgm_stop_all__Fv`.  Added with `quest/arenatask.cpp`
 * (rule 2: this range owns the address). */
void bgm_stop_all(void);
#endif

/* 0x800F4A90 / 0x800F3F98 - the quest scene's sound frame around the local quest step: its start (mode, phase,
 * sub-phase) and its end (GUESS names). */
#ifdef __cplusplus
extern "C" {
#endif
void snd_quest_frame_begin(s32 mode, u8 phase, u8 sub);
void snd_quest_frame_end(void);
/* 0x800F4A68 / 0x800F4560 - whether the BGM hold flag is set, and restarts the hunt streams after a large kill
 * (GUESS names). */
int snd_bgm_hold_ck(void);
void snd_hunt_stream_start(s32 mode);

/* 0x800F2A94 / 0x800F5290 - set the BGM control work up and run its frame (GUESS names). */
void bgm_ctrl_init(void);
void bgm_ctrl_frame(void);
/* 0x800F590C - clears the BGM control's "behind the scene" flag (GUESS name). */
void bgm_behind_flag_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800F2A94_H */
