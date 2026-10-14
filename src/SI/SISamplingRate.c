/*
 * SI/SISamplingRate.c - the SI polling rate: the per-TV-format line/count tables and the call that programs them.
 *
 * RANGE. .text 0x804DEFB0-0x804DF0A0 (2 functions); .data 0x8062A008-0x8062A0A0; .sbss 0x80795480-0x80795488.
 *    Cut from the tail of `SI/SIBios.c`, before `SYN/syn.c` (0x804DF0A0).
 * FLAGS. `cflags_base`, the same group as `SI/SIBios.c` (16-aligned function starts).
 * NAMES. `SISetSamplingRate`, `SIRefreshSamplingRate`, `XYNTSC` and the file name `SISamplingRate.c` are the RVL SDK's;
 *    `XYPAL` (map row `lbl_8062A038`, which also covers the string), `SamplingRate` and `SISamplingXY` are GUESSes.
 * EVIDENCE. The seam: the target `SISetSamplingRate` addresses `XYNTSC`, `XYPAL` and its `OSReport` string off one
 *    base equal to `XYNTSC` (+0x0/+0x30/+0x60); MWCC pools off `...data.0`, so `XYNTSC` opens its own TU's .data.
 *    Compiled inside `SI/SIBios.c` the base is the SI build string (+0x70/+0xA0/+0xD0).
 * RESIDUALS. `XYPAL` and the string are one map row (`lbl_8062A038`, 0x68 B); our object names `...data.0`, so the
 *    relocation names differ from the target's.
 * SHAPES. `SISetSamplingRate` sits in a `dont_inline` bracket: the target's `SIRefreshSamplingRate` is a tail call,
 *    and `-O4,p` otherwise inlines it.  `SamplingRate` is clamped to 11 before interrupts are disabled; an unknown TV
 *    format reports and falls back to row 0 of `XYNTSC`; the line count doubles when the VI runs non-interlaced
 *    (`0xCC00206C` bit 0).
 */

#include "types.h"

#include "SI/SISamplingRate.h"
#include "SI/SIBios.h"
#include "OS/OSInterrupt.h"
#include "OS/OSError.h"
#include "VI/vi.h"

/* One sampling-rate row: the polling line and the sample count. */
typedef struct SISamplingXY {
    /* +0x00 */ u16 line;
    /* +0x02 */ u8 count;
    /* +0x03 */ u8 pad_0x03;
} SISamplingXY; /* size: 0x04 */

static SISamplingXY XYNTSC[12] = {
    {0x00F6, 2, 0}, {0x000E, 19, 0}, {0x001E, 9, 0}, {0x002C, 6, 0}, {0x0034, 5, 0}, {0x0041, 4, 0},
    {0x0057, 3, 0}, {0x0057, 3, 0}, {0x0057, 3, 0}, {0x0083, 2, 0}, {0x0083, 2, 0}, {0x0083, 2, 0},
};
static SISamplingXY XYPAL[12] = {
    {0x0128, 2, 0}, {0x000F, 21, 0}, {0x001D, 11, 0}, {0x002D, 7, 0}, {0x0034, 6, 0}, {0x003F, 5, 0},
    {0x004E, 4, 0}, {0x0068, 3, 0}, {0x0068, 3, 0}, {0x0068, 3, 0}, {0x0068, 3, 0}, {0x009C, 2, 0},
};

static s32 SamplingRate;

#pragma dont_inline on
/* 0x804DEFB0 (0xD4): Stores the polling rate (clamped to 11 ms) and programs the SI polling line and count. */
void SISetSamplingRate(u32 msec)
{
    BOOL enabled;
    SISamplingXY* xy;

    if (msec > 11) {
        msec = 11;
    }
    enabled = OSDisableInterrupts();
    SamplingRate = msec;
    switch (VIGetTvFormat()) {
    case 0:
    case 2:
    case 5:
        xy = XYNTSC;
        break;
    case 1:
        xy = XYPAL;
        break;
    default:
        OSReport("SISetSamplingRate: unknown TV format. Use default.");
        msec = 0;
        xy = XYNTSC;
        break;
    }
    SISetXY(((*(volatile u16*)0xCC00206C & 1) + 1) * xy[msec].line, xy[msec].count);
    OSRestoreInterrupts(enabled);
}
#pragma dont_inline reset

/* 0x804DF090 (0x8): Reapplies the stored polling rate. */
void SIRefreshSamplingRate(void)
{
    SISetSamplingRate(SamplingRate);
}
