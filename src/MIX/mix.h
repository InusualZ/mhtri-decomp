/*
 * MIX/mix.h - declarations of the symbols owned by `MIX/mix.c` that other units call.  The names are GUESSES from
 * how the synthesizer (`SYN/syn.c`, `SYN/synvoice.c`) drives them: one channel per AX voice, with input level, two
 * auxiliary sends, pan and fader.
 */
#ifndef MIX_MIX_H
#define MIX_MIX_H

#include "types.h"

#include "AX/AXVPB.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C2840 - attaches a mixer channel to an AX voice with its initial levels. */
void MIXInitChannel(AXVPB* axVoice, u32 mode, s32 input, s32 auxA, s32 auxB, s32 auxC, s32 pan, s32 span, s32 fader);
/* 0x804C3F10 - detaches the mixer channel of an AX voice. */
void MIXReleaseChannel(AXVPB* axVoice);
/* 0x804C3F30 / 0x804C3F60 / 0x804C3F90 / 0x804C3FC0 / 0x804C40A0 - set one level of the voice's channel. */
void MIXSetInput(AXVPB* axVoice, s32 input);
void MIXSetAuxA(AXVPB* axVoice, s32 auxA);
void MIXSetAuxB(AXVPB* axVoice, s32 auxB);
void MIXSetPan(AXVPB* axVoice, s32 pan);
void MIXSetFader(AXVPB* axVoice, s32 fader);

#ifdef __cplusplus
}
#endif

#endif
