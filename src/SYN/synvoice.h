/*
 * SYN/synvoice.h - declarations of the symbols owned by `SYN/synvoice.c` that `SYN/syn.c` calls, and the lookup
 * tables the three SYN units read.
 */
#ifndef SYN_SYNVOICE_H
#define SYN_SYNVOICE_H

#include "types.h"
#include "SYN/syn.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8062A3A0 - 128 volume levels (16.16 centibels) indexed by a MIDI controller or velocity value. */
extern s32 SYNVolumeTable[128];

/* 0x804E0E70 - AX voice-freed callback: unhooks the record from its synthesizer. */
void SYNVoiceFreedCallback(AXVPB* axVoice);
/* 0x804E0F20 - starts the release phase of a voice and lowers its priority. */
void SYNVoiceRelease(SYNVoice* voice, u32 priority);
/* 0x804E0F40 - runs one frame of the voice record with the given index. */
void SYNVoiceRun(u32 index);
/* 0x804E1090 - binds a voice to the layer, region, sample and ADPCM context of its note; 0 = no layer. */
s32 SYNVoiceBindLayer(SYNVoice* voice);
/* 0x804E0590, 0x804E05C0, 0x804E0420, 0x804E0890 - initial voice state from the bound records. */
void SYNVoiceInitBaseVolume(SYNVoice* voice);
void SYNVoiceInitPan(SYNVoice* voice);
void SYNVoiceInitLfo(SYNVoice* voice);
void SYNVoiceInitPitch(SYNVoice* voice);
/* 0x804E0600, 0x804E0620 - the voice's and its channel's output levels (centibels). */
s32 SYNVoiceVolume(SYNVoice* voice);
s32 SYNVoiceChannelVolume(SYNVoice* voice);
/* 0x804E0E30, 0x804E08F0 - programs the AX voice's sample addressing and its first pitch. */
void SYNVoiceSetupSample(SYNVoice* voice);
void SYNVoiceStartPitch(SYNVoice* voice);

#ifdef __cplusplus
}
#endif

#endif
