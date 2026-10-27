#ifndef MHTRI_SOUND_FN_800F2A94_H
#define MHTRI_SOUND_FN_800F2A94_H

#include "types.h"

/* Declarations for the symbols `src/sound/fn_800F2A94.cpp` (the BGM controller) owns (docs/plan.md 6.5 rule 2). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F2A94 / 0x800F5290: sets the BGM control work up and runs its frame. */
void bgm_ctrl_init(void);
void bgm_ctrl_frame(void);
/* 0x800F3054: requests stream command `id` (an id >= 0x32 is a track, a small id a play/stop command) on `channel`. */
void bgm_ch_req(s32 channel, u32 id);
/* 0x800F3F98: the quest scene's monster-BGM frame (picks the large monster tracks for channels 1 and 2). */
void snd_quest_frame_end(void);
/* 0x800F4538 / 0x800F486C: BGM request flags of the quest start and quest result scenes. */
void snd_quest_start_bgm_set(void);
void snd_quest_result_bgm_set(void);
/* 0x800F4560: restarts the hunt streams after a large kill. */
void snd_hunt_stream_start(s32 mode);
/* 0x800F46D8 / 0x800F4704: set / clear the bit of player `player` in the BGM work's player mask. */
void snd_player_mask_set(u8 player);
void snd_player_mask_clear(u8 player);
/* 0x800F4734: switches the BGM for the quest scene. */
void snd_quest_scene_set(void);
/* 0x800F48F4 / 0x800F4A68: hold the field BGM for the quest, and whether it is held. */
void snd_bgm_hold_set(void);
int snd_bgm_hold_ck(void);
/* 0x800F4A90: the quest scene's area BGM frame (the selection, the phase and the area). */
void snd_quest_frame_begin(s32 mode, u8 phase, u8 sub);
/* 0x800F590C: clears the BGM control's "behind the scene" flag. */
void bgm_behind_flag_clear(void);
/* 0x800F5924 / 0x800F5998: stops the lobby's channel 1 and arms the departure BGM. */
void bgm_lobby_ch1_stop(void);
void bgm_lobby_depart_set(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800F6334: stops every BGM stream; the map's `bgm_stop_all__Fv`. */
void bgm_stop_all(void);
#endif

#endif /* MHTRI_SOUND_FN_800F2A94_H */
