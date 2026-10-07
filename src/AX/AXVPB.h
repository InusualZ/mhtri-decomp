/*
 * AX/AXVPB.h - the AX voice parameter block types and the declarations of the symbols owned by `AX/AXVPB.c`.
 *
 * The AXPB layout is read from the sync-flag groups of `__AXServiceVPB` (one group per update bit) and from the
 * setters; the group names are GUESSes from the DSP voice model each group matches (state, mix, ITD, depop,
 * volume envelope, address, ADPCM, SRC, loop, LPF, biquad, remote mix).
 */
#ifndef AX_AXVPB_H
#define AX_AXVPB_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* size: 0x4; a 32-bit DSP address split into two halfwords. */
typedef struct AXPBAddr {
    /* +0x0 */ u16 hi;
    /* +0x2 */ u16 lo;
} AXPBAddr;

/* size: 0x4; two related halfwords the DSP reads together. */
typedef struct AXPBPair {
    /* +0x0 */ u16 first;
    /* +0x2 */ u16 second;
} AXPBPair;

/* size: 0x6; the ADPCM loop context. */
typedef struct AXPBAdpcmLoop {
    /* +0x0 */ u16 predScale;
    /* +0x2 */ u16 yn1;
    /* +0x4 */ u16 yn2;
} AXPBAdpcmLoop;

/* size: 0x8; the low-pass filter state and coefficients. */
typedef struct AXPBLpf {
    /* +0x0 */ u16 on;
    /* +0x2 */ u16 yn1;
    /* +0x4 */ u16 a0;
    /* +0x6 */ u16 b0;
} AXPBLpf;

/* size: 0x140; the DSP voice parameter block. */
typedef struct AXPB {
    /* +0x000 */ AXPBAddr next;
    /* +0x004 */ AXPBAddr self;
    /* +0x008 */ u16 srcSelect;
    /* +0x00A */ u16 coefSelect;
    /* +0x00C */ u32 stateWord;
    /* +0x010 */ u16 running;
    /* +0x012 */ u16 stateFlags;
    /* +0x014 */ u16 mix[24];
    /* +0x044 */ u16 itdOn;
    /* +0x046 */ AXPBAddr itdBuffer;
    /* +0x04A */ u16 itdShift[2];
    /* +0x04E */ u16 itdTargetShift[2];
    /* +0x052 */ s16 dpop[12];
    /* +0x06A */ u16 veVolume;
    /* +0x06C */ s16 veDelta;
    /* +0x06E */ AXPBPair addrFormat; /* first: loop flag, second: format */
    /* +0x072 */ AXPBAddr addrLoop;
    /* +0x076 */ AXPBAddr addrEnd;
    /* +0x07A */ AXPBAddr addrCurrent;
    /* +0x07E */ AXPBPair adpcm[10];
    /* +0x0A6 */ u16 srcRatio[2];
    /* +0x0AA */ u16 srcFraction;
    /* +0x0AC */ u16 srcLast[4];
    /* +0x0B4 */ AXPBAdpcmLoop adpcmLoop;
    /* +0x0BA */ AXPBLpf lpf;
    /* +0x0C2 */ u16 biquadOn;
    /* +0x0C4 */ u16 biquadState[4];
    /* +0x0CC */ u16 biquadCoef[5];
    /* +0x0D6 */ u16 rmtOn;
    /* +0x0D8 */ u16 rmtSelect;
    /* +0x0DA */ u16 rmtMix[16];
    /* +0x0FA */ s16 rmtDpop[8];
    /* +0x10A */ u16 rmtSrc[5];
    /* +0x114 */ u16 rmtIir[10]; /* [0]: filter type, [1..9]: state and coefficients */
    /* +0x128 */ u8 pad_0x128[0x18];
} AXPB;

struct AXVPB;
typedef void (*AXVPBCallback)(struct AXVPB* vpb);

/* size: 0x168; one voice: its list links, bookkeeping and the PB the DSP reads. */
typedef struct AXVPB {
    /* +0x00 */ struct AXVPB* next;
    /* +0x04 */ struct AXVPB* prev;
    /* +0x08 */ struct AXVPB* nextCallback;
    /* +0x0C */ u32 priority;
    /* +0x10 */ AXVPBCallback callback;
    /* +0x14 */ u32 userContext;
    /* +0x18 */ u32 index;
    /* +0x1C */ u32 sync;
    /* +0x20 */ u32 depop;
    /* +0x24 */ u32* itdBufferData;
    /* +0x28 */ AXPB pb;
} AXVPB;

void __AXSetPBDefault(AXVPB* vpb);
void __AXSyncPBs(u32 cycles);
u32 __AXGetNumVoices(void);
AXPB* __AXGetPBs(void);
void __AXServiceVPB(AXVPB* vpb);
void __AXVPBInit(void);
void __AXVPBQuit(void);
void AXSetVoiceSrcType(AXVPB* vpb, u32 type);
void AXSetVoiceState(AXVPB* vpb, u16 state);
void AXSetVoiceCurrentAddr(AXVPB* vpb, u32 address);
void AXSetVoiceSrcRatio(AXVPB* vpb, f32 ratio);
void AXSetVoiceAdpcmLoop(AXVPB* vpb, const AXPBAdpcmLoop* loop);
void AXSetVoiceLpf(AXVPB* vpb, const AXPBLpf* lpf);
void AXSetVoiceLpfCoefs(AXVPB* vpb, u16 a0, u16 b0);
void AXGetLpfCoefs(u16 frequency, u16* a0, u16* b0);
u32 AXGetMaxVoices(void);

#ifdef __cplusplus
}
#endif

#endif
