/*
 * SYN/syn.h - the synthesizer's record types and the declarations of the symbols owned by `SYN/syn.c` that other
 * units call or read.  Every field name is a GUESS read off how the three SYN units use the offset.
 */
#ifndef SYN_SYN_H
#define SYN_SYN_H

#include "types.h"

#include "AX/AXVPB.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SYNSynth;

/* size: 0x100 - a program's key map: for each of the 128 keys the index of the layer record that plays it
 * (0xFFFF = none). */
typedef struct SYNKeyMap {
    /* +0x00 */ u16 layer[128];
} SYNKeyMap;

/* size: 0x18 - one layer of a program: the sample's root key and tuning plus where it loops. */
typedef struct SYNLayer {
    /* +0x00 */ u8 rootKey;
    /* +0x01 */ u8 exclusiveClass; /* a note of the same class on the channel cuts the previous one */
    /* +0x02 */ s16 fineTune;      /* cents */
    /* +0x04 */ s32 volumeOffset;
    /* +0x08 */ u32 loopStart;
    /* +0x0C */ u32 loopLength;    /* 0 = one shot */
    /* +0x10 */ u32 regionIndex;
    /* +0x14 */ u32 sampleIndex;
} SYNLayer;

/* size: 0x50 - the LFO and envelope parameters a voice plays with (timecents, 0x80000000 = not set). */
typedef struct SYNRegion {
    /* +0x00 */ s32 lfoRate;
    /* +0x04 */ s32 lfoDelay;
    /* +0x08 */ s32 lfoVolumeDepth;
    /* +0x0C */ s32 lfoPitchDepth;
    /* +0x10 */ s32 lfoVolumeModDepth; /* added per unit of the modulation wheel */
    /* +0x14 */ s32 lfoPitchModDepth;
    /* +0x18 */ s32 volAttack;
    /* +0x1C */ s32 volDecay;
    /* +0x20 */ s32 volSustain;
    /* +0x24 */ s32 volRelease;
    /* +0x28 */ s32 volAttackKeyScale;
    /* +0x2C */ s32 volDecayKeyScale;
    /* +0x30 */ s32 modAttack;
    /* +0x34 */ s32 modDecay;
    /* +0x38 */ s32 modSustain;
    /* +0x3C */ s32 modRelease;
    /* +0x40 */ s32 modAttackKeyScale;
    /* +0x44 */ s32 modDecayKeyScale;
    /* +0x48 */ s32 modPeak;
    /* +0x4C */ s32 drumPan; /* the pan a percussion note plays at */
} SYNRegion;

/* size: 0x10 - one sample: its format, rate and where its data sits in the sample area. */
typedef struct SYNSample {
    /* +0x00 */ u16 format; /* 0 = ADPCM, 1 = 16-bit PCM, 2 = 8-bit PCM */
    /* +0x02 */ u16 sampleRate;
    /* +0x04 */ u32 dataOffset;
    /* +0x08 */ u32 length;
    /* +0x0C */ u16 adpcmIndex;
    /* +0x0E */ u8 pad_0x0E[2];
} SYNSample;

/* size: 0x2E - the ADPCM decoder context of a sample. */
typedef struct SYNAdpcm {
    /* +0x00 */ AXPBPair coef[10];
    /* +0x28 */ AXPBAdpcmLoop loop;
} SYNAdpcm;

/* size: 0x94 - one voice record (the 0x3798 B array of `SYN/syn.c`). */
typedef struct SYNVoice {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ AXVPB* axVoice;
    /* +0x08 */ struct SYNSynth* synth; /* NULL = the record is free */
    /* +0x0C */ u8 channel;
    /* +0x0D */ u8 key;
    /* +0x0E */ u8 velocity;
    /* +0x0F */ u8 pan;
    /* +0x10 */ u8 exclusiveClass;
    /* +0x11 */ u8 pad_0x11[0x03];
    /* +0x14 */ SYNLayer* layer;
    /* +0x18 */ SYNRegion* region;
    /* +0x1C */ SYNSample* sample;
    /* +0x20 */ SYNAdpcm* adpcm;
    /* +0x24 */ u32 releasePending; /* the key was released while the sustain pedal was down */
    /* +0x28 */ u32 looped;
    /* +0x2C */ f32 sampleGain;
    /* +0x30 */ s32 basePitch; /* cents, 16.16 */
    /* +0x34 */ s32 baseVolume;
    /* +0x38 */ s32 lfoPhase;
    /* +0x3C */ s32 lfoVolumeOut;
    /* +0x40 */ s32 lfoPitchOut;
    /* +0x44 */ s32 lfoRate;
    /* +0x48 */ s32 lfoDelay;
    /* +0x4C */ s32 lfoVolumeDepth;
    /* +0x50 */ s32 lfoPitchDepth;
    /* +0x54 */ s32 lfoVolumeModDepth;
    /* +0x58 */ s32 lfoPitchModDepth;
    /* +0x5C */ u32 volPhase;
    /* +0x60 */ s32 volLevel;
    /* +0x64 */ s32 volAttackBase;
    /* +0x68 */ s32 volAttackRate;
    /* +0x6C */ s32 volDecayRate;
    /* +0x70 */ s32 volSustainLevel;
    /* +0x74 */ s32 volReleaseRate;
    /* +0x78 */ u32 modPhase;
    /* +0x7C */ s32 modLevel;
    /* +0x80 */ s32 modAttackRate;
    /* +0x84 */ s32 modDecayRate;
    /* +0x88 */ s32 modSustainLevel;
    /* +0x8C */ s32 modReleaseRate;
    /* +0x90 */ s32 modPeak;
} SYNVoice;

/* Called when a note starts, with the voice, the synthesizer and the channel. */
typedef void (*SYNNoteCallback)(AXVPB* axVoice, struct SYNSynth* synth, u8 channel);
/* Called every frame for a playing voice, with the voice and its channel. */
typedef void (*SYNFrameCallback)(AXVPB* axVoice, u8 channel);

/* size: 0x313C - one synthesizer instance embedded in a sequence (`SEQSequence.synth`): the sound tables, 16 MIDI
 * channels of controller state, the event queue and the voice slots. */
typedef struct SYNSynth {
    /* +0x0000 */ struct SYNSynth* next;
    /* +0x0004 */ SYNKeyMap* drumKeyMaps;
    /* +0x0008 */ SYNKeyMap* melodicKeyMaps;
    /* +0x000C */ SYNLayer* layers;
    /* +0x0010 */ SYNRegion* regions;
    /* +0x0014 */ SYNSample* samples;
    /* +0x0018 */ SYNAdpcm* adpcms;
    /* +0x001C */ u32 sampleBasePcm16;
    /* +0x0020 */ u32 sampleBasePcm8;
    /* +0x0024 */ u32 sampleBaseAdpcm;
    /* +0x0028 */ u32 acquirePriority;
    /* +0x002C */ u32 voicePriority;
    /* +0x0030 */ u32 releasePriority;
    /* +0x0034 */ SYNKeyMap* keyMap[16]; /* the program each channel plays */
    /* +0x0074 */ s32 masterVolume;
    /* +0x0078 */ u8 controller[16][128];
    /* +0x0878 */ u8 rpnSelected[16]; /* 1 = RPN selected, 0 = NRPN */
    /* +0x0888 */ u8 pad_0x0888[0x20];
    /* +0x08A8 */ s32 pitchBendRange[16];
    /* +0x08E8 */ s32 pitchBend[16];
    /* +0x0928 */ s32 channelVolume[16];
    /* +0x0968 */ s32 expression[16];
    /* +0x09A8 */ s32 reverbSend[16];
    /* +0x09E8 */ s32 chorusSend[16];
    /* +0x0A28 */ u8 eventQueue[0x300]; /* 256 three-byte MIDI messages */
    /* +0x0D28 */ u8* eventWrite;
    /* +0x0D2C */ s32 eventCount;
    /* +0x0D30 */ s32 voiceCount;
    /* +0x0D34 */ SYNVoice* exclusiveVoice[16][16];
    /* +0x1134 */ SYNVoice* noteVoice[16][128];
    /* +0x3134 */ SYNNoteCallback noteCallback;
    /* +0x3138 */ SYNFrameCallback frameCallback;
} SYNSynth;

/* 0x80795488 - the voice record array (`SYNVoicePool`, one record per AX voice), indexed by AX voice number. */
extern SYNVoice* SYNVoices;

/* 0x804DF1B0 - feeds one MIDI message (status byte plus its data bytes) to the synthesizer.  NAME: a GUESS
 * from its two callers, which pass the event bytes they just decoded. */
void SYNMIDIInput(SYNSynth* synth, u8* message);

/* 0x804DF0A0 / 0x804DF130 / 0x804DF140 - initialise the voice records, drop them, and run one audio frame.
 * NAMES: GUESSes. */
void SYNInit(void);
void SYNQuit(void);
void SYNRunAudio(void);

#ifdef __cplusplus
}
#endif

#endif
