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

/* A linear volume that moves from `current` to `target` over one audio frame. size: 0x4 */
typedef struct MIXRamp {
    /* +0x0 */ u16 current; /* the volume the voice is mixed with now */
    /* +0x2 */ u16 target;  /* the volume computed from the levels, applied on the next update */
} MIXRamp;

/* One mixing channel: the levels a voice is mixed with and the volumes derived from them. size: 0x70 */
typedef struct MIXChannel {
    /* +0x00 */ AXVPB* axvpb;  /* the voice, NULL when the channel is free */
    /* +0x04 */ u32 mode;      /* MIX_MODE_* option bits (low nibble) and pending-update flags (top nibble) */
    /* +0x08 */ s32 input;     /* input level, in tenths of a decibel */
    /* +0x0C */ s32 auxA;      /* auxiliary send A level, in tenths of a decibel */
    /* +0x10 */ s32 auxB;      /* auxiliary send B level, in tenths of a decibel */
    /* +0x14 */ s32 auxC;      /* auxiliary send C level, in tenths of a decibel */
    /* +0x18 */ s32 pan;       /* 0..127, 64 is the centre */
    /* +0x1C */ s32 span;      /* surround pan, 0..127 */
    /* +0x20 */ s32 fader;     /* channel fader, in tenths of a decibel */
    /* +0x24 */ s32 l;         /* left attenuation from the pan tables */
    /* +0x28 */ s32 r;         /* right attenuation */
    /* +0x2C */ s32 f;         /* front attenuation from the surround pan */
    /* +0x30 */ s32 b;         /* back attenuation from the surround pan */
    /* +0x34 */ s32 l1;        /* rear-left attenuation (Dolby Pro Logic II only) */
    /* +0x38 */ s32 r1;        /* rear-right attenuation (Dolby Pro Logic II only) */
    /* +0x3C */ MIXRamp v;     /* input volume */
    /* +0x40 */ MIXRamp vL;    /* main left */
    /* +0x44 */ MIXRamp vR;    /* main right */
    /* +0x48 */ MIXRamp vS;    /* main surround (rear left under Pro Logic II) */
    /* +0x4C */ MIXRamp vAL;   /* aux A left */
    /* +0x50 */ MIXRamp vAR;   /* aux A right */
    /* +0x54 */ MIXRamp vAS;   /* aux A surround */
    /* +0x58 */ MIXRamp vBL;   /* aux B left */
    /* +0x5C */ MIXRamp vBR;   /* aux B right */
    /* +0x60 */ MIXRamp vBS;   /* aux B surround */
    /* +0x64 */ MIXRamp vCL;   /* aux C left (main rear right under Pro Logic II) */
    /* +0x68 */ MIXRamp vCR;   /* aux C right (aux A rear right under Pro Logic II) */
    /* +0x6C */ MIXRamp vCS;   /* aux C surround (aux B rear right under Pro Logic II) */
} MIXChannel;

/* The remote-speaker mix of one voice: a main and an aux level for each of the four remotes. size: 0x44 */
typedef struct MIXAuxState {
    /* +0x00 */ u32 flags;        /* 0x80000000 apply targets, 0x40000000 recompute, 1/2/4/8 the aux of remote n is pre-fader */
    /* +0x04 */ s32 level[4];     /* main level of each remote, in tenths of a decibel */
    /* +0x14 */ s32 offset[4];    /* aux level of each remote, in tenths of a decibel */
    /* +0x24 */ MIXRamp ramp[8];  /* per remote: [2n] the main volume, [2n+1] the aux volume */
} MIXAuxState;

/* 0x804C2840 - attaches a mixer channel to an AX voice with its initial levels. */
void MIXInitChannel(AXVPB* axvpb, u32 mode, s32 input, s32 auxA, s32 auxB, s32 auxC, s32 pan, s32 span, s32 fader);

/* 0x804C25E0 - fills `channel`'s pan attenuations from the pan tables for the current sound mode. */
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

/* 0x804C40D0 - applies every channel's pending level and pan changes to its AX voice; runs once per audio frame. */
void MIXUpdateSettings(void);

#ifdef __cplusplus
}
#endif

#endif
