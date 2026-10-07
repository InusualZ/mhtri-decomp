/*
 * SYN/syn.h - declarations of the symbols owned by `SYN/syn.c` that other units call or read.
 */
#ifndef SYN_SYN_H
#define SYN_SYN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x313C - one synthesizer instance embedded in a sequence (`SEQSequence.synth`); its fields are not
 * reconstructed yet, so the block is an approximation of the extent, not a layout. */
typedef struct SYNSynth {
    /* +0x00 */ u8 pad_0x00[0x313C];
} SYNSynth;

/* size: 0x4C - the sound region a voice plays (an approximation of the extent: only the fields up to +0x48 are read): the volume and modulation envelope parameters in timecents (0x80000000 = not set), sustain levels and
 * the key-scaling amounts.  Names are GUESSes from how `SYNSetupVolumeEnvelope` uses them. */
typedef struct SYNRegion {
    /* +0x00 */ u8 pad_0x00[0x18];
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
} SYNRegion;

/* size: 0x94 - one voice record (the 0x3798 B array of `SYN/syn.c`).  Only the envelope fields are named; the
 * names are GUESSes from the setup functions. */
typedef struct SYNVoice {
    /* +0x00 */ u8 pad_0x00[0x0D];
    /* +0x0D */ u8 key;
    /* +0x0E */ u8 velocity;
    /* +0x0F */ u8 pad_0x0F[0x09];
    /* +0x18 */ SYNRegion* region;
    /* +0x1C */ u8 pad_0x1C[0x40];
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

/* 0x804DF1B0 - feeds one MIDI message (status byte plus its data bytes) to the synthesizer.  NAME: a GUESS
 * from its two callers, which pass the event bytes they just decoded. */
void SYNMIDIInput(SYNSynth* synth, const u8* message);

#ifdef __cplusplus
}
#endif

#endif
