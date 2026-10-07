/*
 * AX/AXSPB.c - the AX studio parameter block: the studio getter and print, the main and remote depop fades, the SPB
 *    init and quit and the voice depop.
 *
 * RANGE. .text 0x80470570-0x804709B0 (7 functions, 0x440 B); .bss 0x806FB660-0x806FB6E0; .sbss 0x80794F88-0x80794FD8.
 *    Cut from the old ARC/AX block between `AX/AXOut.c` (0x80470570) and `AX/AXVPB.c` (0x804709B0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AX*` names are the map's; the file name `AXSPB.c` is a GUESS; `__AXStudio` (.bss), the twenty
 *    `__AXDepopMain<N>` / `__AXDepopRmt<N>` accumulators and the `AXStudio` / `AXStudioFade` types are GUESSes named
 *    from how the print and depop code use them.
 * EVIDENCE. `.bss` 0x806FB660 (0x80 B) is read by `__AXGetStudio` and `__AXPrintStudio` only; `.sbss`
 *    0x80794F88..0x80794FD8 (twenty words) is read by `__AXPrintStudio`, `__AXSPBInit` and `__AXDepopVoice`
 *    and by nothing else; `__AXGetNumVoices` reads the next group (0x80794FE8).
 * RESIDUALS. none recorded yet.
 * SHAPES. the studio is a packed array of 6-byte (level, step) records: 12 main records then 8 remote records.
 */

#include "types.h"

#include "AX/AXSPB.h"
#include "AX/AXVPB.h"
#include "OS/OSCache.h"

/* The depop accumulators, defined from the highest address down (the compiler emits them in reverse). */
s32 __AXDepopMain0;
s32 __AXDepopMain1;
s32 __AXDepopMain2;
s32 __AXDepopMain3;
s32 __AXDepopMain4;
s32 __AXDepopMain5;
s32 __AXDepopMain6;
s32 __AXDepopMain7;
s32 __AXDepopMain8;
s32 __AXDepopMain9;
s32 __AXDepopMain10;
s32 __AXDepopMain11;
s32 __AXDepopRmt0;
s32 __AXDepopRmt2;
s32 __AXDepopRmt4;
s32 __AXDepopRmt6;
s32 __AXDepopRmt1;
s32 __AXDepopRmt3;
s32 __AXDepopRmt5;
s32 __AXDepopRmt7;

AXStudio __AXStudio __attribute__((aligned(32)));

/* 0x80470570 (0xC): returns the studio fade block. */
AXStudio* __AXGetStudio(void)
{
    return &__AXStudio;
}

#pragma dont_inline on

/* 0x80470580 (0x6C): steps the accumulated main depop level toward zero, 96 units per step. */
void __AXDepopFadeMain(s32* level, s32* studioLevel, s16* studioStep)
{
    s32 step = *level / 96;

    if (step != 0) {
        if (step > 20) {
            step = 20;
        }
        if (step < -20) {
            step = -20;
        }
        *studioLevel = *level;
        *level -= step * 96;
        *studioStep = -step;
    } else {
        *level = 0;
        *studioLevel = 0;
        *studioStep = 0;
    }
}

/* 0x804705F0 (0x6C): steps the accumulated remote depop level toward zero, 18 units per step. */
void __AXDepopFadeRmt(s32* level, s32* studioLevel, s16* studioStep)
{
    s32 step = *level / 18;

    if (step != 0) {
        if (step > 20) {
            step = 20;
        }
        if (step < -20) {
            step = -20;
        }
        *studioLevel = *level;
        *level -= step * 18;
        *studioStep = -step;
    } else {
        *level = 0;
        *studioLevel = 0;
        *studioStep = 0;
    }
}

#pragma dont_inline reset

/* 0x80470660 (0x178): fades every depop accumulator into the studio block and flushes it. */
void __AXPrintStudio(void)
{
    __AXDepopFadeMain(&__AXDepopMain0, &__AXStudio.main[0].level, &__AXStudio.main[0].step);
    __AXDepopFadeMain(&__AXDepopMain1, &__AXStudio.main[1].level, &__AXStudio.main[1].step);
    __AXDepopFadeMain(&__AXDepopMain2, &__AXStudio.main[2].level, &__AXStudio.main[2].step);
    __AXDepopFadeMain(&__AXDepopMain3, &__AXStudio.main[3].level, &__AXStudio.main[3].step);
    __AXDepopFadeMain(&__AXDepopMain4, &__AXStudio.main[4].level, &__AXStudio.main[4].step);
    __AXDepopFadeMain(&__AXDepopMain5, &__AXStudio.main[5].level, &__AXStudio.main[5].step);
    __AXDepopFadeMain(&__AXDepopMain6, &__AXStudio.main[6].level, &__AXStudio.main[6].step);
    __AXDepopFadeMain(&__AXDepopMain7, &__AXStudio.main[7].level, &__AXStudio.main[7].step);
    __AXDepopFadeMain(&__AXDepopMain8, &__AXStudio.main[8].level, &__AXStudio.main[8].step);
    __AXDepopFadeMain(&__AXDepopMain9, &__AXStudio.main[9].level, &__AXStudio.main[9].step);
    __AXDepopFadeMain(&__AXDepopMain10, &__AXStudio.main[10].level, &__AXStudio.main[10].step);
    __AXDepopFadeMain(&__AXDepopMain11, &__AXStudio.main[11].level, &__AXStudio.main[11].step);
    __AXDepopFadeRmt(&__AXDepopRmt0, &__AXStudio.rmt[0].level, &__AXStudio.rmt[0].step);
    __AXDepopFadeRmt(&__AXDepopRmt2, &__AXStudio.rmt[2].level, &__AXStudio.rmt[2].step);
    __AXDepopFadeRmt(&__AXDepopRmt4, &__AXStudio.rmt[4].level, &__AXStudio.rmt[4].step);
    __AXDepopFadeRmt(&__AXDepopRmt6, &__AXStudio.rmt[6].level, &__AXStudio.rmt[6].step);
    __AXDepopFadeRmt(&__AXDepopRmt1, &__AXStudio.rmt[1].level, &__AXStudio.rmt[1].step);
    __AXDepopFadeRmt(&__AXDepopRmt3, &__AXStudio.rmt[3].level, &__AXStudio.rmt[3].step);
    __AXDepopFadeRmt(&__AXDepopRmt5, &__AXStudio.rmt[5].level, &__AXStudio.rmt[5].step);
    __AXDepopFadeRmt(&__AXDepopRmt7, &__AXStudio.rmt[7].level, &__AXStudio.rmt[7].step);
    DCFlushRange(&__AXStudio, sizeof(AXStudio));
}

/* 0x804707E0 (0x58): clears every depop accumulator. */
void __AXSPBInit(void)
{
    __AXDepopRmt7 = 0;
    __AXDepopRmt5 = 0;
    __AXDepopRmt3 = 0;
    __AXDepopRmt1 = 0;
    __AXDepopRmt6 = 0;
    __AXDepopRmt4 = 0;
    __AXDepopRmt2 = 0;
    __AXDepopRmt0 = 0;
    __AXDepopMain11 = 0;
    __AXDepopMain10 = 0;
    __AXDepopMain9 = 0;
    __AXDepopMain8 = 0;
    __AXDepopMain7 = 0;
    __AXDepopMain6 = 0;
    __AXDepopMain5 = 0;
    __AXDepopMain4 = 0;
    __AXDepopMain3 = 0;
    __AXDepopMain2 = 0;
    __AXDepopMain1 = 0;
    __AXDepopMain0 = 0;
}

/* 0x80470840 (0x4): has nothing to release. */
void __AXSPBQuit(void)
{
}

/* 0x80470850 (0x15C): adds a voice's depop deltas to the accumulators. */
void __AXDepopVoice(AXPB* pb)
{
    __AXDepopMain0 += pb->dpop[0];
    __AXDepopMain3 += pb->dpop[1];
    __AXDepopMain6 += pb->dpop[2];
    __AXDepopMain9 += pb->dpop[3];
    __AXDepopMain1 += pb->dpop[4];
    __AXDepopMain4 += pb->dpop[5];
    __AXDepopMain7 += pb->dpop[6];
    __AXDepopMain10 += pb->dpop[7];
    __AXDepopMain2 += pb->dpop[8];
    __AXDepopMain5 += pb->dpop[9];
    __AXDepopMain8 += pb->dpop[10];
    __AXDepopMain11 += pb->dpop[11];
    __AXDepopRmt0 += pb->rmtDpop[0];
    __AXDepopRmt2 += pb->rmtDpop[1];
    __AXDepopRmt4 += pb->rmtDpop[2];
    __AXDepopRmt6 += pb->rmtDpop[3];
    __AXDepopRmt1 += pb->rmtDpop[4];
    __AXDepopRmt3 += pb->rmtDpop[5];
    __AXDepopRmt5 += pb->rmtDpop[6];
    __AXDepopRmt7 += pb->rmtDpop[7];
}
