/* VI/vi.cpp - the video interface library: retrace handling, mode configuration, framebuffer flush and the TV getters.
 * RANGE. .text 0x804E6710-0x804E91C0 (28 functions); .data 0x8062B3C8-0x8062B920; .bss 0x8075B110-0x8075B280;
 *   .sdata 0x80794160-0x80794180; .sbss 0x807955D8-0x80795688.
 *   Edges: the `vi.c` __FILE__ string (.sdata 0x80794178) and the "<< RVL_SDK - VI ... (0x4302_145) >>" build
 *   string (.data 0x8062B3C8) are read by the function 0x804E7F60 (it calls setFbbRegs and setVerticalRegs); the TU's state (.bss 0x8075B110/0x8075B188/
 *   0x8075B200/0x8075B258, .sbss 0x80795608..0x80795684) is read only from 0x804E6710..0x804E91B0.  The right edge
 *   0x804E91C0 starts `WaitMicroTime` (i2c).  The one data object read from both sides of this range is the setup
 *   function 0x804E68B0 reading the vi3in1 global at .sbss 0x80795690 (an extern).
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment): every start is 16-aligned.
 * NAMES. the map's names (`VIWaitForRetrace`, `VIFlush`, `VISetNextFrameBuffer`, `VISetBlack`, `VIGetNextField`,
 *   `VIGetCurrentLine`, `VIGetTvFormat`, `VIGetDTVStatus`, `__VIDisplayPositionToXY`, `VIEnableDimming`,
 *   `VIResetDimmingCount`, `setFbbRegs`, `setVerticalRegs`); `VISetPreRetraceCallback`/`VISetPostRetraceCallback`
 *   (0x804E70C0/0x804E7110, GUESS: they swap the callbacks the retrace handler calls and return the old one),
 *   `VISetDimmingMode` (GUESS, 0x804E9080), `VIGetRetraceCount`, `VIGetScanMode` (GUESS), `__VIResetDimmingControlA` and `__VIResetDimmingControlB` (GUESS: each clears one control word
 *   the dimming code reads) and the variables `viCurrTiming`, `viPostRetraceCallback`, `viPreRetraceCallback`,
 *   `viRetraceQueue`, `viRetraceCount`, `viHorVer`, `viFrameBufferChanged`, `viDimming`, `viDimmingControlA/B` (GUESSES).
 * RESIDUALS. Flip blockers: .bss object 0x80 vs claimed 0x170 and .sdata 0x20 claimed but not emitted (the unwritten bodies own them);
 *   `VIGetTvFormat` lacks the retail jump table `@4022` (if chain instead).  Written: the callback setters, `VIWaitForRetrace`, the getters, `VISetNextFrameBuffer`, `VISetBlack`,
 *   `__VIDisplayPositionToXY` and the three reset helpers.  Not attempted (the register-programming bodies):
 *   0x804E6710, 0x804E68B0 (retrace handler), 0x804E7160, 0x804E7280, 0x804E7480, setFbbRegs 0x804E7A30, 0x804E7CE0,
 *   setVerticalRegs 0x804E7DC0, 0x804E7F60, 0x804E8630, VIFlush (needs a 64-bit count-leading-zeros inline the
 *   compiler has no intrinsic for), `VISetDimmingMode` is 71% because it inlines `VIGetTvFormat` with its own jump table `jumptable_8062B8FC` (ours: if chain, no table).  `VIGetTvFormat`: the target switches through a nine-entry jump table whose cases share
 *   blocks, ours compiles an if chain.  `VIGetScanMode`: the target normalises the extracted bit with neg/or/srwi.
 *   Register numbering differs in `VIGetNextField`, `VIGetCurrentLine`, `VIWaitForRetrace` (the retrace count load
 *   is scheduled before the interrupt-state move) and `__VIDisplayPositionToXY` (r9/r10 swap).
 *   Data: only the state used by the written functions is defined; the .data tables, jump tables and the other .sbss
 *   words are not emitted yet, so the data sections do not compare.
 */
#include "VI/vi.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "OS/OSThread.h"
#include "OS/OSSleepThread.h"
#include "SC/SCGetScreenSaverMode.h"

/* The video interface registers. */
#define VI_REG16(offset) (*(volatile u16*)(0xCC002000 + (offset)))

/* One video timing: the vertical timing numbers and the half-line geometry. size: 0x28 (the tail is unwritten) */
struct VITimingInfo {
    /* +0x00 */ u8 equ;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u16 acv;
    /* +0x04 */ u16 prbOdd;
    /* +0x06 */ u16 prbEven;
    /* +0x08 */ u16 psbOdd;
    /* +0x0A */ u16 psbEven;
    /* +0x0C */ u8 pad_0x0C[0xC];
    /* +0x18 */ u16 nhlines;
    /* +0x1A */ u16 hlw;
};

/* The display state the retrace handler programs the registers from. size: 0x58 */
struct VIHorVer {
    /* +0x00 */ u8 pad_0x00[6];
    /* +0x06 */ u16 dispSizeY;
    /* +0x08 */ u8 pad_0x08[2];
    /* +0x0A */ u16 dispPosY;
    /* +0x0C */ u8 pad_0x0C[0x18];
    /* +0x24 */ u32 threeD;
    /* +0x28 */ u8 pad_0x28[8];
    /* +0x30 */ u32 bufAddr;
    /* +0x34 */ u32 tfbb;
    /* +0x38 */ u32 bfbb;
    /* +0x3C */ u8 pad_0x3C[4];
    /* +0x40 */ u32 black;
    /* +0x44 */ u8 pad_0x44[8];
    /* +0x4C */ u32 rtfbb;
    /* +0x50 */ u32 rbfbb;
    /* +0x54 */ VITimingInfo* timing;
};

u32 CurrTvMode;

static VITimingInfo* viCurrTiming;
static VIRetraceCallback viPostRetraceCallback;
static VIRetraceCallback viPreRetraceCallback;
static OSThreadQueue viRetraceQueue;
static u32 viRetraceCount;
static VIHorVer viHorVer;
static u32 viFrameBufferChanged;
static u32 viDimmingControlA;
static u32 viDimmingControlB;

/* The dimming state of the screen saver. size: 0x28 (the tail is unwritten) */
struct VIDimming {
    /* +0x00 */ u32 count;
    /* +0x04 */ u8 pad_0x04[0x24];
};
static VIDimming viDimming;
static u32 viDimmingEnabled;
static u32 viDimmingTimeout;
static u32 viDimmingMode;

extern "C" {
void setFbbRegs(VIHorVer* horVer, u32* tfbb, u32* bfbb, u32* rtfbb, u32* rbfbb);
void setVerticalRegs(u16 dispPosY, u16 dispSizeY, u8 equ, u16 acv, u16 prbOdd, u16 prbEven, u16 psbOdd, u16 psbEven,
                     BOOL black);
}

/* Reads the current half-line of the frame (vertical counter doubled plus the horizontal position). */
static inline u32 getCurrentHalfLine(void)
{
    u32 vcount;
    u32 vcountPrev;
    u32 hcount;

    vcount = VI_REG16(0x2C) & 0x7FF;
    do {
        vcountPrev = vcount;
        hcount = VI_REG16(0x2E) & 0x7FF;
        vcount = VI_REG16(0x2C) & 0x7FF;
    } while (vcountPrev != vcount);
    return ((vcount - 1) << 1) + (hcount - 1) / viCurrTiming->hlw;
}

/* Tells which field (0 or 1) the beam is in. */
static inline u32 getCurrentFieldEvenOdd(void)
{
    return getCurrentHalfLine() < viCurrTiming->nhlines ? 1 : 0;
}

/* Sets the pre-retrace callback and returns the previous one. */
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback callback)
{
    BOOL enabled;
    VIRetraceCallback old = viPreRetraceCallback;

    enabled = OSDisableInterrupts();
    viPreRetraceCallback = callback;
    OSRestoreInterrupts(enabled);
    return old;
}

/* Sets the post-retrace callback and returns the previous one. */
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback)
{
    BOOL enabled;
    VIRetraceCallback old = viPostRetraceCallback;

    enabled = OSDisableInterrupts();
    viPostRetraceCallback = callback;
    OSRestoreInterrupts(enabled);
    return old;
}

/* Blocks the calling thread until the next vertical retrace. */
void VIWaitForRetrace(void)
{
    BOOL enabled;
    u32 count;

    enabled = OSDisableInterrupts();
    count = viRetraceCount;
    do {
        OSSleepThread(&viRetraceQueue);
    } while (count == viRetraceCount);
    OSRestoreInterrupts(enabled);
}

/* Returns the number of retraces since the library started. */
u32 VIGetRetraceCount(void)
{
    return viRetraceCount;
}

/* Returns the field (0 top, 1 bottom) the next frame buffer flip will draw. */
u32 VIGetNextField(void)
{
    BOOL enabled;
    u32 nextField;

    enabled = OSDisableInterrupts();
    nextField = getCurrentFieldEvenOdd() ^ 1;
    OSRestoreInterrupts(enabled);
    return nextField ^ (viHorVer.dispPosY & 1);
}

/* Returns the line the beam is on, within the field. */
u32 VIGetCurrentLine(void)
{
    BOOL enabled;
    VITimingInfo* timing = viCurrTiming;
    u32 halfLine;

    enabled = OSDisableInterrupts();
    halfLine = getCurrentHalfLine();
    OSRestoreInterrupts(enabled);
    if (halfLine >= timing->nhlines) {
        halfLine -= timing->nhlines;
    }
    return halfLine >> 1;
}

/* Returns the TV format (NTSC, PAL, MPAL, EURGB60) of the current mode. */
u32 VIGetTvFormat(void)
{
    BOOL enabled;
    u32 format;

    enabled = OSDisableInterrupts();
    format = CurrTvMode;
    switch (format) {
    case 0:
    case 3:
    case 6:
    case 7:
    case 8:
        format = 0;
        break;
    case 1:
    case 4:
        format = 1;
        break;
    case 2:
    case 5:
        break;
    }
    OSRestoreInterrupts(enabled);
    return format;
}

/* Returns the scan mode: 2 when the progressive bit is set, else 1 for the non-interlaced flag and 0 for interlaced. */
u32 VIGetScanMode(void)
{
    BOOL enabled;
    u32 mode;
    u32 progressive;

    enabled = OSDisableInterrupts();
    progressive = VI_REG16(0x6C) & 1;
    if (progressive == 1) {
        mode = 2;
    } else {
        mode = (VI_REG16(0x02) >> 2 & 1) != 0;
    }
    OSRestoreInterrupts(enabled);
    return mode;
}

/* Returns the DTV status bit. */
u8 VIGetDTVStatus(void)
{
    BOOL enabled;
    u32 status;

    enabled = OSDisableInterrupts();
    status = VI_REG16(0x6E) & 3;
    OSRestoreInterrupts(enabled);
    return status & 1;
}

/* Points the display at a new frame buffer on the next retrace. */
/* untyped: caller-owned frame buffer, passed through as an address */
void VISetNextFrameBuffer(void* fb)
{
    BOOL enabled;

    enabled = OSDisableInterrupts();
    viHorVer.bufAddr = (u32)fb;
    viFrameBufferChanged = 1;
    setFbbRegs(&viHorVer, &viHorVer.tfbb, &viHorVer.bfbb, &viHorVer.rtfbb, &viHorVer.rbfbb);
    OSRestoreInterrupts(enabled);
}

/* Blanks or shows the display. */
void VISetBlack(BOOL black)
{
    BOOL enabled;
    VITimingInfo* timing;

    enabled = OSDisableInterrupts();
    timing = viHorVer.timing;
    viHorVer.black = black;
    setVerticalRegs(viHorVer.dispPosY, viHorVer.dispSizeY, timing->equ, timing->acv, timing->prbOdd, timing->prbEven,
                    timing->psbOdd, timing->psbEven, black);
    OSRestoreInterrupts(enabled);
}

/* Clears the dimming counter. */
u32 VIResetDimmingCount(void)
{
    viDimming.count = 0;
    return 1;
}

/* Clears the first dimming control word. */
u32 __VIResetDimmingControlA(void)
{
    viDimmingControlA = 0;
    return 1;
}

/* Clears the second dimming control word. */
u32 __VIResetDimmingControlB(void)
{
    viDimmingControlB = 0;
    return 1;
}

/* Converts a beam position (horizontal and vertical counter values) to a screen column and line, or -1 outside the picture. */
void __VIDisplayPositionToXY(u32 hcount, u32 vcount, s16* x, s16* y)
{
    u32 halfLine;
    u32 nhlines;
    u32 lead;
    u32 prb;

    halfLine = ((vcount - 1) << 1) + (hcount - 1) / viCurrTiming->hlw;
    if (viHorVer.threeD == 0) {
        nhlines = viCurrTiming->nhlines;
        if (halfLine < nhlines) {
            lead = viCurrTiming->equ * 3;
            prb = viCurrTiming->prbOdd;
            if (halfLine < prb + lead) {
                *y = -1;
            } else if (halfLine >= nhlines - viCurrTiming->psbOdd) {
                *y = -1;
            } else {
                *y = (halfLine - lead - prb) & ~1;
            }
        } else {
            halfLine -= nhlines;
            lead = viCurrTiming->equ * 3;
            prb = viCurrTiming->prbEven;
            if (halfLine < prb + lead) {
                *y = -1;
            } else if (halfLine >= nhlines - viCurrTiming->psbEven) {
                *y = -1;
            } else {
                *y = ((halfLine - lead - prb) & ~1) + 1;
            }
        }
    } else if (viHorVer.threeD == 1) {
        nhlines = viCurrTiming->nhlines;
        if (halfLine >= nhlines) {
            halfLine -= nhlines;
        }
        lead = viCurrTiming->equ * 3;
        prb = viCurrTiming->prbOdd;
        if (halfLine < prb + lead) {
            *y = -1;
        } else if (halfLine >= nhlines - viCurrTiming->psbOdd) {
            *y = -1;
        } else {
            *y = (halfLine - lead - prb) & ~1;
        }
    } else if (viHorVer.threeD == 2) {
        nhlines = viCurrTiming->nhlines;
        if (halfLine < nhlines) {
            lead = viCurrTiming->equ * 3;
            prb = viCurrTiming->prbOdd;
            if (halfLine < prb + lead) {
                *y = -1;
            } else if (halfLine >= nhlines - viCurrTiming->psbOdd) {
                *y = -1;
            } else {
                *y = halfLine - lead - prb;
            }
        } else {
            halfLine -= nhlines;
            lead = viCurrTiming->equ * 3;
            prb = viCurrTiming->prbEven;
            if (halfLine < prb + lead) {
                *y = -1;
            } else if (halfLine >= nhlines - viCurrTiming->psbEven) {
                *y = -1;
            } else {
                *y = (halfLine - lead - prb) & ~1;
            }
        }
    }
    *x = hcount - 1;
}

/* Turns screen dimming on or off; it stays off when the console's screen saver setting is off. Returns the previous setting. */
u32 VIEnableDimming(s32 enable)
{
    u32 old = viDimmingEnabled;

    if (enable == 1 && SCGetScreenSaverMode() == 0) {
        enable = 0;
    }
    viDimmingEnabled = enable;
    return old;
}

/* Selects the screen-dimming delay class (1, 2 or other) and derives the timeout in frames from the TV format. Returns the previous class. */
u32 VISetDimmingMode(u32 mode)
{
    u32 old = viDimmingMode;

    viDimmingMode = mode;
    if (VIGetTvFormat() == 1) {
        switch (viDimmingMode) {
        case 1:
            viDimmingTimeout = 30000;
            break;
        case 2:
            viDimmingTimeout = 45000;
            break;
        default:
            viDimmingTimeout = 15000;
            break;
        }
    } else {
        switch (viDimmingMode) {
        case 1:
            viDimmingTimeout = 36000;
            break;
        case 2:
            viDimmingTimeout = 54000;
            break;
        default:
            viDimmingTimeout = 18000;
            break;
        }
    }
    return old;
}
