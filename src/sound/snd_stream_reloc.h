/* sound/snd_stream_reloc.h - the declarations `sound/snd_stream_reloc.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_STREAM_RELOC_H
#define MHTRI_SOUND_SND_STREAM_RELOC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s16 fn_800E852C(void);
s16 fn_800E8594(u32 idx);
u32 snd_handle_get(void);
s32 snd_level_volume_get(void);
/* 0x800E89E0 - records whether the network transfer mode is 1 (GUESS name: the network transfer-mode switch
 * is the caller). */
void setStreamTransferMode(u32 mode);
/* 0x800E8D40 - resets the reverb work area (its 0x8000-byte buffer at 0x90003F60 and the cursor words). */
void clearReverbWorkArea(void);
/* 0x800E8CC8 - copies `count` halfwords into the ring the reverb time counter walks (GUESS name: the network layer
 * callback hands it the record layer command 39 delivers). */
void pushReverbSamples(s16* src, s32 count);
/* 0x800E8D74 - advances the reverb time counter by 0xB with interrupts off (GUESS name). */
void advanceReverbClock(void);
/* 0x800E84F0 / 0x800E8558 - store the sound object's SE level word and its u8 fade level (GUESS names). */
void snd_se_level_set(s32 level);
void snd_fade_level_set(u8 level);
/* 0x800E85E8 / 0x800E8634 - advance reverb state `idx`, apply reverb config `entry_idx` of bank `idx`, each with
 * interrupts off (GUESS names). */
void snd_reverb_advance(u32 idx);
void snd_reverb_apply(u32 idx, u32 entry_idx);

#ifdef __cplusplus
}
#endif

#endif
