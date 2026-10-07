/*
 * AX/AXVPB.c - the AX voice parameter blocks: the voice table, the per-frame voice service and PB sync, and the
 *    AXSetVoice* setters.
 *
 * RANGE. .text 0x804709B0-0x80471960 (17 functions, 0xFB0 B); .bss 0x806FB6E0-0x8070CDE0; .sbss
 *    0x80794FD8-0x80794FF8; .sdata2 0x8079CF48-0x8079CF68.  Cut from the old ARC/AX block between `AX/AXSPB.c`
 *    (0x804709B0) and `AX/AXProf.c` (0x80471960).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AXVPB*`, `__AXServiceVPB`, `__AXSyncPBs`, `__AXGetPBs` and `__AXSetPBDefault` names are the map's; the setters
 *    `AXSetVoice*` (0x80471550..0x80471830), `AXGetLpfCoefs` (0x80471890), `AXGetMaxVoices` (0x80471950) and
 *    `__AXVPBInitVoices` (0x804712A0) are GUESSes from the public AX API and what each does; the `AX_SYNC_*` flag names,
 *    the AXPB field names, `__AXPBData`, `__AXItdData`, `__AXVPBData` and the `__AX*` state words are GUESSes from how
 *    the code uses them; the file name `AXVPB.c` is a GUESS.
 * EVIDENCE. `.bss` 0x806FB6E0 (0x7800 B: 96 PBs), 0x80702EE0 (0x1800 B: 96 ITD buffers) and 0x807046E0 (0x8700 B: 96 voices)
 *    are reached through the `.sbss` pointers 0x80794FD8..0x80794FE0 that `__AXVPBInit` fills; `.sdata2`
 *    0x8079CF48..0x8079CF68 holds the float constants of `AXSetVoiceSrcRatio` and `AXGetLpfCoefs`.
 * RESIDUALS. AXSetVoiceLpf 0x804717C0 and AXSetVoiceAdpcmLoop 0x80471760: the target stores the first halfword before
 *    it loads `sync`, ours hoists the load; __AXSyncPBs 0x80470F10: add association and register colouring of the cycle
 *    sums; __AXVPBInitVoices 0x804712A0:
 *    register colouring; __AXServiceVPB 0x804709C0: ITD copy temporaries take r3/r0 swapped.
 */

#include "types.h"

#include "AX/AXAlloc.h"
#include "AX/AXCL.h"
#include "AX/AXSPB.h"
#include "AX/AXVPB.h"
#include "MSL/s_cos.h"
#include "MSL/sqrt.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OS.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memcpy.h"

#define AX_MAX_VOICES 96

/* Update flag bits of `AXVPB::sync`, one per PB group the DSP copy is refreshed from. */
#define AX_SYNC_ALL 0x80000000
#define AX_SYNC_SRC_SELECT 0x00000001
#define AX_SYNC_STATE_WORD 0x00000002
#define AX_SYNC_RUNNING 0x00000004
#define AX_SYNC_STATE_FLAGS 0x00000008
#define AX_SYNC_MIX 0x00000010
#define AX_SYNC_ITD 0x00000020
#define AX_SYNC_ITD_TARGET 0x00000040
#define AX_SYNC_DPOP 0x00000080
#define AX_SYNC_VE 0x00000100
#define AX_SYNC_VE_READ 0x00000200
#define AX_SYNC_ADDR_ALL 0x00000400
#define AX_SYNC_ADDR_FORMAT 0x00000800
#define AX_SYNC_ADDR_LOOP 0x00001000
#define AX_SYNC_ADDR_END 0x00002000
#define AX_SYNC_ADDR_CURRENT 0x00004000
#define AX_SYNC_ADPCM 0x00008000
#define AX_SYNC_SRC_ALL 0x00010000
#define AX_SYNC_SRC_RATIO 0x00020000
#define AX_SYNC_ADPCM_LOOP 0x00040000
#define AX_SYNC_LPF_ALL 0x00080000
#define AX_SYNC_LPF_COEFS 0x00100000
#define AX_SYNC_BIQUAD_ALL 0x00200000
#define AX_SYNC_BIQUAD_COEFS 0x00400000
#define AX_SYNC_RMT_ON 0x00800000
#define AX_SYNC_RMT_SELECT 0x01000000
#define AX_SYNC_RMT_MIX 0x02000000
#define AX_SYNC_RMT_DPOP 0x04000000
#define AX_SYNC_RMT_SRC 0x08000000
#define AX_SYNC_RMT_IIR_ALL 0x10000000
#define AX_SYNC_RMT_IIR_GAIN 0x20000000
#define AX_SYNC_RMT_IIR_COEFS 0x40000000

/* The reset value of `AXVPB::sync` for a fresh voice. */
#define AX_SYNC_DEFAULT 0x18A80024

AXVPB __AXVPBData[AX_MAX_VOICES];
u32 __AXItdData[AX_MAX_VOICES][0x10];
AXPB __AXPBData[AX_MAX_VOICES];

u32 __AXCycleLimit;
u32 __AXCycles;
s32 __AXNumVoices;
u32 __AXMaxVoices;
AXVPB* __AXVPBs;
u32* __AXItdBuffers;
AXPB* __AXPBs;

/* 0x804709B0 (0x8): returns how many voices the last sync serviced. */
u32 __AXGetNumVoices(void)
{
    return __AXNumVoices;
}

/* 0x804709C0 (0x550): copies the changed PB groups of a voice into the DSP's copy. */
void __AXServiceVPB(AXVPB* vpb)
{
    AXPB* dst;
    AXPB* pb = &vpb->pb;
    u32 sync;
    u32 i;
    u32* itd;

    __AXNumVoices++;
    dst = &__AXPBs[vpb->index];
    sync = vpb->sync;
    if (sync == 0) {
        pb->running = dst->running;
        pb->veVolume = dst->veVolume;
        pb->addrCurrent.hi = dst->addrCurrent.hi;
        pb->addrCurrent.lo = dst->addrCurrent.lo;
        return;
    }
    if (sync & AX_SYNC_ALL) {
        memcpy(dst, pb, sizeof(AXPB));
        return;
    }
    if (sync & AX_SYNC_SRC_SELECT) {
        dst->srcSelect = pb->srcSelect;
        dst->coefSelect = pb->coefSelect;
    }
    if (sync & AX_SYNC_STATE_WORD) {
        dst->stateWord = pb->stateWord;
    }
    if (sync & AX_SYNC_RUNNING) {
        dst->running = pb->running;
    } else {
        pb->running = dst->running;
    }
    if (sync & AX_SYNC_STATE_FLAGS) {
        dst->stateFlags = pb->stateFlags;
    }
    if (sync & AX_SYNC_MIX) {
        memcpy(dst->mix, pb->mix, sizeof(pb->mix));
    }
    if (sync & AX_SYNC_ITD_TARGET) {
        dst->itdTargetShift[0] = pb->itdTargetShift[0];
        dst->itdTargetShift[1] = pb->itdTargetShift[1];
    } else if (sync & AX_SYNC_ITD) {
        dst->itdOn = pb->itdOn;
        dst->itdBuffer.hi = pb->itdBuffer.hi;
        dst->itdBuffer.lo = pb->itdBuffer.lo;
        dst->itdShift[0] = pb->itdShift[0];
        dst->itdShift[1] = pb->itdShift[1];
        dst->itdTargetShift[0] = pb->itdTargetShift[0];
        dst->itdTargetShift[1] = pb->itdTargetShift[1];
        itd = vpb->itdBufferData;
        for (i = 0; i < 16; i++) {
            itd[i] = 0;
        }
    }
    if (sync & AX_SYNC_DPOP) {
        memcpy(dst->dpop, pb->dpop, sizeof(pb->dpop));
    }
    if (sync & AX_SYNC_VE_READ) {
        pb->veVolume = dst->veVolume;
        dst->veDelta = pb->veDelta;
    } else if (sync & AX_SYNC_VE) {
        dst->veVolume = pb->veVolume;
        dst->veDelta = pb->veDelta;
    }
    if (sync & (AX_SYNC_ADDR_FORMAT | AX_SYNC_ADDR_LOOP | AX_SYNC_ADDR_END | AX_SYNC_ADDR_CURRENT)) {
        if (sync & AX_SYNC_ADDR_FORMAT) {
            dst->addrFormat.first = pb->addrFormat.first;
        }
        if (sync & AX_SYNC_ADDR_LOOP) {
            dst->addrLoop = pb->addrLoop;
        }
        if (sync & AX_SYNC_ADDR_END) {
            dst->addrEnd = pb->addrEnd;
        }
        if (sync & AX_SYNC_ADDR_CURRENT) {
            dst->addrCurrent = pb->addrCurrent;
        } else {
            pb->addrCurrent = dst->addrCurrent;
        }
    } else if (sync & AX_SYNC_ADDR_ALL) {
        dst->addrFormat = pb->addrFormat;
        dst->addrLoop = pb->addrLoop;
        dst->addrEnd = pb->addrEnd;
        dst->addrCurrent = pb->addrCurrent;
    } else {
        pb->addrCurrent.hi = dst->addrCurrent.hi;
        pb->addrCurrent.lo = dst->addrCurrent.lo;
    }
    if (sync & AX_SYNC_ADPCM) {
        for (i = 0; i < 10; i++) {
            dst->adpcm[i] = pb->adpcm[i];
        }
    }
    if (sync & AX_SYNC_SRC_RATIO) {
        dst->srcRatio[0] = pb->srcRatio[0];
        dst->srcRatio[1] = pb->srcRatio[1];
    } else if (sync & AX_SYNC_SRC_ALL) {
        dst->srcRatio[0] = pb->srcRatio[0];
        dst->srcRatio[1] = pb->srcRatio[1];
        dst->srcFraction = pb->srcFraction;
        dst->srcLast[0] = pb->srcLast[0];
        dst->srcLast[1] = pb->srcLast[1];
        dst->srcLast[2] = pb->srcLast[2];
        dst->srcLast[3] = pb->srcLast[3];
    }
    if (sync & AX_SYNC_ADPCM_LOOP) {
        dst->adpcmLoop.predScale = pb->adpcmLoop.predScale;
        dst->adpcmLoop.yn1 = pb->adpcmLoop.yn1;
        dst->adpcmLoop.yn2 = pb->adpcmLoop.yn2;
    }
    if (sync & AX_SYNC_LPF_COEFS) {
        dst->lpf.a0 = pb->lpf.a0;
        dst->lpf.b0 = pb->lpf.b0;
    } else if (sync & AX_SYNC_LPF_ALL) {
        dst->lpf.on = pb->lpf.on;
        dst->lpf.yn1 = pb->lpf.yn1;
        dst->lpf.a0 = pb->lpf.a0;
        dst->lpf.b0 = pb->lpf.b0;
    }
    if (sync & AX_SYNC_BIQUAD_COEFS) {
        for (i = 0; i < 5; i++) {
            dst->biquadCoef[i] = pb->biquadCoef[i];
        }
    } else if (sync & AX_SYNC_BIQUAD_ALL) {
        dst->biquadOn = pb->biquadOn;
        for (i = 0; i < 4; i++) {
            dst->biquadState[i] = pb->biquadState[i];
        }
        for (i = 0; i < 5; i++) {
            dst->biquadCoef[i] = pb->biquadCoef[i];
        }
    }
    if (sync & AX_SYNC_RMT_ON) {
        dst->rmtOn = pb->rmtOn;
    }
    if (sync & AX_SYNC_RMT_SELECT) {
        dst->rmtSelect = pb->rmtSelect;
    }
    if (sync & AX_SYNC_RMT_MIX) {
        memcpy(dst->rmtMix, pb->rmtMix, sizeof(pb->rmtMix));
    }
    if (sync & AX_SYNC_RMT_DPOP) {
        memcpy(dst->rmtDpop, pb->rmtDpop, sizeof(pb->rmtDpop));
    }
    if (sync & AX_SYNC_RMT_SRC) {
        memcpy(dst->rmtSrc, pb->rmtSrc, sizeof(pb->rmtSrc));
    }
    if (sync & AX_SYNC_RMT_IIR_GAIN) {
        dst->rmtIir[2] = pb->rmtIir[2];
        dst->rmtIir[3] = pb->rmtIir[3];
    } else if (sync & AX_SYNC_RMT_IIR_COEFS) {
        dst->rmtIir[5] = pb->rmtIir[5];
        dst->rmtIir[6] = pb->rmtIir[6];
        dst->rmtIir[7] = pb->rmtIir[7];
        dst->rmtIir[8] = pb->rmtIir[8];
        dst->rmtIir[9] = pb->rmtIir[9];
    } else if (sync & AX_SYNC_RMT_IIR_ALL) {
        for (i = 0; i < 10; i++) {
            dst->rmtIir[i] = pb->rmtIir[i];
        }
    }
}

/* Cycle cost of the eight 2-bit remote mix select fields of a PB. */
#define AX_RMT_FIELD(select, shift) __AXCycleTableB[((select) >> (shift)) & 3]

/* 0x80470F10 (0x310): services every voice by priority within the DSP cycle budget and flushes the PBs. */
void __AXSyncPBs(u32 baseCycles)
{
    u32 cycles;
    u32 priority;
    AXVPB* vpb;
    AXPB* dst;

    __AXNumVoices = 0;
    DCInvalidateRange(__AXPBs, __AXMaxVoices * sizeof(AXPB));
    DCInvalidateRange(__AXItdBuffers, __AXMaxVoices << 6);
    cycles = __AXGetCommandListCycles() + __AXMaxVoices * 600;
    cycles = cycles + baseCycles + 32;
    for (priority = 31; priority != 0; priority--) {
        for (vpb = __AXGetStackHead(priority); vpb != NULL; vpb = vpb->next) {
            if (vpb->pb.itdOn == 1) {
                cycles += 129;
            }
            if (vpb->depop != 0) {
                __AXDepopVoice(&__AXPBs[vpb->index]);
            }
            if (vpb->pb.running == 1) {
                u32 ratio;
                u32 stateWord;
                u32 srcCycles;
                u32 select;

                cycles += 387;
                if (vpb->pb.lpf.on != 0) {
                    cycles += 309;
                }
                if (vpb->pb.biquadOn != 0) {
                    cycles += 1024;
                }
                if (vpb->pb.itdOn == 1) {
                    cycles += 27;
                }
                ratio = (vpb->pb.srcRatio[1] & 0xFFFF) | (vpb->pb.srcRatio[0] << 16);
                if (vpb->pb.srcSelect == 0) {
                    srcCycles = ((u32)((ratio << 9) + 0x8000) >> 16) + 1561;
                } else {
                    srcCycles = 605;
                    if (vpb->pb.srcSelect == 1) {
                        srcCycles = ((u32)((ratio << 9) + 0x8000) >> 16) + 1466;
                    }
                }
                cycles += srcCycles;
                stateWord = vpb->pb.stateWord;
                cycles = cycles + __AXCycleTableA[(stateWord >> 26) & 0x1F]
                         + (__AXCycleTableA[(stateWord >> 16) & 0x1F]
                            + (__AXCycleTableA[(stateWord >> 21) & 0x1F] + __AXCycleTableA[stateWord & 0x1F]));
                if (vpb->pb.rmtOn == 1) {
                    cycles += 613;
                    if (vpb->pb.rmtIir[0] == 1) {
                        cycles += 118;
                    } else if (vpb->pb.rmtIir[0] == 2) {
                        cycles += 834;
                    }
                    select = vpb->pb.rmtSelect;
                    cycles = (AX_RMT_FIELD(select, 14) + AX_RMT_FIELD(select, 12))
                             + (cycles + AX_RMT_FIELD(select, 10))
                             + (AX_RMT_FIELD(select, 8) + AX_RMT_FIELD(select, 6)
                                + (AX_RMT_FIELD(select, 2) + (AX_RMT_FIELD(select, 4) + AX_RMT_FIELD(select, 0))));
                }
                if (__AXCycleLimit > cycles) {
                    __AXServiceVPB(vpb);
                } else {
                    dst = &__AXPBs[vpb->index];
                    if (dst->running == 1) {
                        __AXDepopVoice(dst);
                    }
                    vpb->pb.running = 0;
                    dst->running = 0;
                    __AXPushCallbackStack(vpb);
                }
            } else {
                __AXServiceVPB(vpb);
            }
            vpb->sync = 0;
            vpb->depop = 0;
        }
    }
    __AXCycles = cycles;
    for (vpb = __AXGetStackHead(0); vpb != NULL; vpb = vpb->next) {
        if (vpb->depop != 0) {
            __AXDepopVoice(&__AXPBs[vpb->index]);
        }
        vpb->depop = 0;
        __AXPBs[vpb->index].running = 0;
    }
    DCFlushRange(__AXPBs, __AXMaxVoices * sizeof(AXPB));
    DCFlushRange(__AXItdBuffers, __AXMaxVoices << 6);
}

/* 0x80471220 (0x8): returns the DSP's PB array. */
AXPB* __AXGetPBs(void)
{
    return __AXPBs;
}

/* 0x80471230 (0x40): resets a voice's PB to its power-on state. */
void __AXSetPBDefault(AXVPB* vpb)
{
    vpb->pb.running = 0;
    vpb->pb.itdOn = 0;
    vpb->sync = AX_SYNC_DEFAULT;
    vpb->pb.lpf.on = 0;
    vpb->pb.biquadOn = 0;
    vpb->pb.rmtOn = 0;
    vpb->pb.rmtIir[0] = 0;
    vpb->pb.rmtSrc[0] = 0;
    vpb->pb.rmtSrc[1] = 0;
    vpb->pb.rmtSrc[2] = 0;
    vpb->pb.rmtSrc[3] = 0;
    vpb->pb.rmtSrc[4] = 0;
}

/* Clears `count` words from `address`. */
static inline void __AXClearWords(u32* address, u32 count)
{
    while (count--) {
        *address++ = 0;
    }
}

void __AXVPBInitVoices(void);

/* 0x80471270 (0x30): points the voice tables at the static arrays and builds the voice list. */
void __AXVPBInit(void)
{
    __AXMaxVoices = AX_MAX_VOICES;
    __AXPBs = __AXPBData;
    __AXItdBuffers = __AXItdData[0];
    __AXVPBs = __AXVPBData;
    __AXVPBInitVoices();
}

/* 0x804712A0 (0x28C): clears the PB, ITD and voice arrays, links the PBs into a chain and frees every voice. */
void __AXVPBInitVoices(void)
{
    u32 i;
    AXPB* dsp;
    AXVPB* vpb;
    u32* itd;

    __AXCycles = 0;
    __AXCycleLimit = OS_BUS_CLOCK / 667;
    __AXClearWords((u32*)__AXPBs, __AXMaxVoices * 0x50);
    __AXClearWords(__AXItdBuffers, __AXMaxVoices * 0x10);
    __AXClearWords((u32*)__AXVPBs, __AXMaxVoices * 0x5A);
    for (i = 0; i < __AXMaxVoices; i++) {
        vpb = &__AXVPBs[i];
        dsp = &__AXPBs[i];
        itd = &__AXItdBuffers[i * 0x10];
        vpb->index = i;
        vpb->itdBufferData = itd;
        __AXSetPBDefault(vpb);
        if (i == __AXMaxVoices - 1) {
            dsp->next.lo = 0;
            dsp->next.hi = 0;
            vpb->pb.next.lo = 0;
            vpb->pb.next.hi = 0;
        } else {
            u32 next = (u32)(dsp + 1);

            vpb->pb.next.hi = next >> 16;
            vpb->pb.next.lo = next;
            dsp->next.hi = next >> 16;
            dsp->next.lo = next;
        }
        vpb->pb.self.hi = (u32)dsp >> 16;
        vpb->pb.self.lo = (u32)dsp;
        dsp->self.hi = (u32)dsp >> 16;
        dsp->self.lo = (u32)dsp;
        vpb->pb.itdBuffer.hi = (u32)itd >> 16;
        vpb->pb.itdBuffer.lo = (u32)itd;
        dsp->itdBuffer.hi = (u32)itd >> 16;
        dsp->itdBuffer.lo = (u32)itd;
        vpb->priority = 1;
        __AXPushFreeStack(vpb);
    }
    DCFlushRange(__AXPBs, __AXMaxVoices * sizeof(AXPB));
}

/* 0x80471530 (0x18): forgets the voice tables. */
void __AXVPBQuit(void)
{
    __AXPBs = NULL;
    __AXItdBuffers = NULL;
    __AXVPBs = NULL;
    __AXMaxVoices = 0;
}

/* 0x80471550 (0xC0): selects the sample rate conversion type of a voice. */
void AXSetVoiceSrcType(AXVPB* vpb, u32 type)
{
    BOOL level = OSDisableInterrupts();

    switch (type) {
    case 0:
        vpb->pb.srcSelect = 2;
        break;
    case 1:
        vpb->pb.srcSelect = 1;
        break;
    case 2:
        vpb->pb.srcSelect = 0;
        vpb->pb.coefSelect = 0;
        break;
    case 3:
        vpb->pb.srcSelect = 0;
        vpb->pb.coefSelect = 1;
        break;
    case 4:
        vpb->pb.srcSelect = 0;
        vpb->pb.coefSelect = 2;
        break;
    }
    vpb->sync |= AX_SYNC_SRC_SELECT;
    OSRestoreInterrupts(level);
}

/* 0x80471610 (0x70): starts or stops a voice. */
void AXSetVoiceState(AXVPB* vpb, u16 state)
{
    BOOL level = OSDisableInterrupts();

    if (vpb->pb.running == state) {
        OSRestoreInterrupts(level);
        return;
    }
    vpb->pb.running = state;
    vpb->sync |= AX_SYNC_RUNNING;
    if (state == 0) {
        vpb->depop = 1;
    }
    OSRestoreInterrupts(level);
}

/* 0x80471680 (0x54): moves the current playback address of a voice. */
void AXSetVoiceCurrentAddr(AXVPB* vpb, u32 address)
{
    BOOL level = OSDisableInterrupts();

    vpb->pb.addrCurrent.hi = address >> 16;
    vpb->pb.addrCurrent.lo = address;
    vpb->sync |= AX_SYNC_ADDR_CURRENT;
    OSRestoreInterrupts(level);
}

/* 0x804716E0 (0x78): sets the sample rate ratio of a voice (1.0 is 0x10000). */
void AXSetVoiceSrcRatio(AXVPB* vpb, f32 ratio)
{
    BOOL level = OSDisableInterrupts();
    u32 fixed = (u32)(65536.0f * ratio);

    vpb->pb.srcRatio[0] = fixed >> 16;
    vpb->pb.srcRatio[1] = fixed;
    vpb->sync |= AX_SYNC_SRC_RATIO;
    OSRestoreInterrupts(level);
}

/* 0x80471760 (0x60): sets the ADPCM loop context of a voice. */
void AXSetVoiceAdpcmLoop(AXVPB* vpb, const AXPBAdpcmLoop* loop)
{
    BOOL level = OSDisableInterrupts();

    vpb->pb.adpcmLoop = *loop;
    vpb->sync |= AX_SYNC_ADPCM_LOOP;
    OSRestoreInterrupts(level);
}

/* 0x804717C0 (0x68): sets the low-pass filter state of a voice. */
void AXSetVoiceLpf(AXVPB* vpb, const AXPBLpf* lpf)
{
    BOOL level = OSDisableInterrupts();

    vpb->pb.lpf.on = lpf->on;
    vpb->pb.lpf.yn1 = lpf->yn1;
    vpb->pb.lpf.a0 = lpf->a0;
    vpb->pb.lpf.b0 = lpf->b0;
    vpb->sync |= AX_SYNC_LPF_ALL;
    OSRestoreInterrupts(level);
}

/* 0x80471830 (0x5C): sets the low-pass filter coefficients of a voice. */
void AXSetVoiceLpfCoefs(AXVPB* vpb, u16 a0, u16 b0)
{
    BOOL level = OSDisableInterrupts();

    vpb->pb.lpf.a0 = a0;
    vpb->pb.lpf.b0 = b0;
    vpb->sync |= AX_SYNC_LPF_COEFS;
    OSRestoreInterrupts(level);
}

#pragma fp_contract off

/* 0x80471890 (0xBC): computes the one-pole low-pass coefficients for a cutoff frequency in Hz. */
void AXGetLpfCoefs(u16 frequency, u16* a0, u16* b0)
{
    f32 cosine = (f32)cos(6.2831855f * (f32)frequency / 32000.0f);
    f32 c = 2.0f - cosine;
    s16 coef = (s16)(32768.0f * -((f32)sqrt(c * c - 1.0f) - c));

    *b0 = coef;
    *a0 = 0x7FFF - (u16)coef;
}

#pragma fp_contract on

/* 0x80471950 (0x8): returns the number of voices. */
u32 AXGetMaxVoices(void)
{
    return __AXMaxVoices;
}
