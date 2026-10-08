/*
 * MIX/mix.h - declarations of the symbols owned by `MIX/mix.c` that other units call or read: the AX channel mixer.
 *    The function names are GUESSes (the shared dump has only `__MIXSetPan`); the channel layout is traced from the
 *    bodies in `mix.c`.
 */
#ifndef MIX_MIX_H
#define MIX_MIX_H

#include "types.h"
#include "AX/AXVPB.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A volume that moves from `current` to `target`. size: 0x4 */
typedef struct MIXRamp {
    /* +0x0 */ u16 current;
    /* +0x2 */ u16 target;
} MIXRamp;

/* One mixing channel: the levels a voice is mixed with and the coefficients derived from them. size: 0x70 */
typedef struct MIXChannel {
    /* +0x00 */ u32 mode;      /* zero when the channel is free */
    /* +0x04 */ u32 update;    /* pending-change flags: 0x10000000 input, 0x40000000 levels/pan, 0x80000000 applied */
    /* +0x08 */ s32 input;     /* input level, in tenths of a decibel */
    /* +0x0C */ s32 auxA;      /* auxiliary send A level, in tenths of a decibel */
    /* +0x10 */ s32 auxB;      /* auxiliary send B level, in tenths of a decibel */
    /* +0x14 */ s32 auxC;      /* auxiliary send C level, in tenths of a decibel */
    /* +0x18 */ s32 pan;       /* 0..127, 64 is the centre */
    /* +0x1C */ s32 span;      /* surround pan, 0..127 */
    /* +0x20 */ s32 fader;     /* channel fader, in tenths of a decibel */
    /* +0x24 */ s32 gain[6];   /* left, right, surround left/right and the rear pair, from the pan tables */
    /* +0x3C */ MIXRamp ramp[13]; /* running mix state, `current` cleared when the channel is set up */
} MIXChannel;

/* The ramped volumes of one voice's four buses: for each bus a direct and an offset value. size: 0x44 */
typedef struct MIXAuxState {
    /* +0x00 */ u32 flags;        /* 0x80000000 applied, 0x40000000 changed, 1/2/4/8 the offset of bus n is absolute */
    /* +0x04 */ s32 level[4];     /* bus levels in tenths of a decibel */
    /* +0x14 */ s32 offset[4];    /* per-bus offsets in tenths of a decibel */
    /* +0x24 */ MIXRamp ramp[8];
} MIXAuxState;

/* 0x804C2840 - attaches a mixer channel to an AX voice with its initial levels. */
void MIXInitChannel(AXVPB* axVoice, u32 mode, s32 input, s32 auxA, s32 auxB, s32 auxC, s32 pan, s32 span, s32 fader);

/* 0x804C25E0 - fills `channel`'s gain row from the pan tables for the current sound mode. */
void __MIXSetPan(MIXChannel* channel);

/* 0x804C26A0 - turns tenths of a decibel into the table's 16-bit linear volume (0 below -90.4 dB, 0xFF64 above +6 dB). */
u16 __MIXGetVolume(s32 tenthsDb);

/* 0x804C26E0 - sets up the channel arrays for every AX voice; does nothing before AX is initialised. */
void MIXInit(void);

/* 0x804C2800 - clears the mixer state. */
void MIXQuit(void);

/* 0x804C2820 - sets the sound mode (the pan table family). */
void MIXSetSoundMode(u32 mode);

/* 0x804C2830 - returns the sound mode. */
u32 MIXGetSoundMode(void);

/* 0x804C3F10 - frees the channel of `voice`. */
void MIXReleaseChannel(AXVPB* voice);

/* 0x804C3F30 / 0x804C3F60 / 0x804C3F90 / 0x804C3FC0 / 0x804C4030 / 0x804C40A0 - setters of the voice's channel levels. */
void MIXSetInput(AXVPB* voice, s32 tenthsDb);
void MIXSetAuxA(AXVPB* voice, s32 tenthsDb);
void MIXSetAuxB(AXVPB* voice, s32 tenthsDb);
void MIXSetPan(AXVPB* voice, s32 pan);
void MIXSetSPan(AXVPB* voice, s32 span);
void MIXSetFader(AXVPB* voice, s32 tenthsDb);

/* 0x804C5BB0 - resets the aux ramp state of voice `index`. */
void __MIXResetAux(u32 index);

#ifdef __cplusplus
}
#endif

#endif
