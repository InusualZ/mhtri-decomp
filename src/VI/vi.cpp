/* VI/vi.cpp - the video interface library: retrace handling, mode configuration, framebuffer flush and the TV getters.
 * RANGE. .text 0x804E6710-0x804E91C0 (28 functions); .data 0x8062B3C8-0x8062B920; .bss 0x8075B110-0x8075B280;
 *   .sdata 0x80794160-0x80794180; .sbss 0x807955D8-0x80795688.  The `vi.c` __FILE__ string (.sdata 0x80794178) and the
 *   "<< RVL_SDK - VI ... (0x4302_145) >>" build string (.data 0x8062B3C8) are this TU's; the right edge 0x804E91C0 starts
 *   `WaitMicroTime` (VI/i2c).  .data is emitted in the target's order and bytes: version string, `timing`, `taps`,
 *   `viRenderModes`, `viShutdownFunctionInfo`, the jump tables and the VIConfigure strings; .sdata/.sbss/.bss follow the
 *   declaration order of the statics.
 * FLAGS. The `OS` library block of configure.py (Wii/1.3) with `cflags_base` (-O4,p): every function start is 16-aligned.
 * NAMES. SDK names: the VI* entry points, `__VIDisplayPositionToXY`, `setFbbRegs`, `setVerticalRegs`, `CurrTvMode`,
 *   `__VIVersion`, `timing`, `taps`.  GUESS: `__VIShutdown` (the shutdown-list function), `__VIRetraceHandler` (the
 *   interrupt-24 handler `VIInit` installs), `getTiming`, `__VIInit`, `setHorizontalRegs`, `VISetPreRetraceCallback`,
 *   `VISetPostRetraceCallback`, `VISetDimmingMode`, `VIGetScanMode`, `__VIResetDimmingControlA/B`, `viRenderModes`,
 *   `viShutdownFunctionInfo` and every `vi*` variable (named from what it holds and where it is read).
 * RESIDUALS. `__VIShutdown`: the target returns its `final` argument (0) for an unknown event and keeps the event 1..3
 *   retrace-count compare as two return blocks ahead of the gamma case; written with one shared `return FALSE`, the
 *   compare is inverted and the FALSE block sits after the gamma case (the explicit-return shape turns it branchless).
 *   `VIConfigure`: the clock-select word is a `u16` (the target keeps it in r10 but masks it as a 32-bit value; the
 *   `u32` spelling allocates r0), 4 bytes longer.
 * SHAPES. The SDK's inline helpers are written above their first caller; `VIGetTvFormat`, `VIGetScanMode` and
 *   `VIGetDTVStatus` are expanded through `getTvFormat`/`getScanMode`/`getDTVStatus`.  `VIInit` expands the copy
 *   `enableDimming`; `VIEnableDimming` keeps its own body (as a wrapper its two saved registers swap).
 *   `__VIShutdown` expands the inline copy `flushRegs` instead of calling `VIFlush` (defined after it), and
 *   `getCurrentDisplayPosition` is a third copy of the counter-read loop.
 *   `viEncoderDirtyFlags` is read through a volatile view until `VI/vi3in1.h` declares it volatile.
 *   `getTvFormat` returns `CurrTvMode` for the two modes it leaves alone, which is what makes MWCC emit the nine-entry
 *   jump table; the ternary on the bit-field read (`VI_DISP_CONFIG.nin ? 1 : 0`) keeps the neg/or/srwi normalisation.
 *   The register file is the address-specified `__VIRegs` array (a plain pointer cast reloads `taps` and does not hoist
 *   0xCC000000).  The dimming state is volatile.  `cntlzd`/`cntlzdFlush` and `getCurrentHalfLine`/`getNextFieldHalfLine`
 *   differ only in their local declaration order, which decides the register allocation once inlined.
 *   `x = x + (2 * acv - 2)` (not `+=`) keeps the per-sum u16 truncation in `setVerticalRegs`; `(nonInter & 1) << 2` is
 *   what makes `__VIInit` insert the bit with rlwimi; `__VIShutdown` tests `final` last (its TRUE block closes the
 *   function).
 */
#include "VI/vi.h"
#include "VI/vi3in1.h"
#include "OS/OS.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/OSStateTM.h"
#include "OS/OSThread.h"
#include "SC/SCApi.h"
#include "SC/SCGetScreenSaverMode.h"
#include "SI/SIBios.h"
#include "DVD/dvd.h"

/* The video interface registers, as 16-bit words. */
volatile u16 __VIRegs[59] : 0xCC002000;
#define VI_REGS __VIRegs
#define VI_REG16(offset) (__VIRegs[(offset) / 2])

/* The TV format the boot ROM left in low memory (OSPhysicalToCached(0xCC)). */
#define VI_BOOT_TV_FORMAT (*(u32*)0x800000CC)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, l, h) (((x) > (h)) ? (h) : (((x) < (l)) ? (l) : (x)))

/* `VI/vi3in1.cpp`'s pending encoder updates, read as the volatile word the retrace handler polls. */
#define viEncoderDirtyFlagsVolatile (*(volatile u32*)&viEncoderDirtyFlags)

/* The change-mask bit of one register index. */
#define VI_BITMASK(index) (1ULL << (63 - (index)))

/* The display configuration register (VI_REG16(0x02)); size: 0x02 */
typedef struct VIDispConfig {
    /* +0x00 */ u16 reserved : 6;
    /* +0x00 */ u16 format : 2;
    /* +0x00 */ u16 le1 : 2;
    /* +0x00 */ u16 le0 : 2;
    /* +0x00 */ u16 dlr : 1;
    /* +0x00 */ u16 nin : 1; /* non-interlaced */
    /* +0x00 */ u16 reset : 1;
    /* +0x00 */ u16 enable : 1;
} VIDispConfig; /* size: 0x02 */
#define VI_DISP_CONFIG (*(volatile VIDispConfig*)(0xCC002002))

/* One video timing: the vertical and horizontal timing numbers the registers are programmed from. */
struct VITimingInfo {
    /* +0x00 */ u8 equ;
    /* +0x02 */ u16 acv;
    /* +0x04 */ u16 prbOdd;
    /* +0x06 */ u16 prbEven;
    /* +0x08 */ u16 psbOdd;
    /* +0x0A */ u16 psbEven;
    /* +0x0C */ u8 bs1;
    /* +0x0D */ u8 bs2;
    /* +0x0E */ u8 bs3;
    /* +0x0F */ u8 bs4;
    /* +0x10 */ u16 be1;
    /* +0x12 */ u16 be2;
    /* +0x14 */ u16 be3;
    /* +0x16 */ u16 be4;
    /* +0x18 */ u16 nhlines;
    /* +0x1A */ u16 hlw;
    /* +0x1C */ u8 hsy;
    /* +0x1D */ u8 hcs;
    /* +0x1E */ u8 hce;
    /* +0x1F */ u8 hbe640;
    /* +0x20 */ u16 hbs640;
    /* +0x22 */ u8 hbeCCIR656;
    /* +0x24 */ u16 hbsCCIR656;
}; /* size: 0x26 */

/* The display state the registers are programmed from. size: 0x58 */
struct VIHorVer {
    /* +0x00 */ u16 dispPosX;
    /* +0x02 */ u16 dispPosY;
    /* +0x04 */ u16 dispSizeX;
    /* +0x06 */ u16 dispSizeY;
    /* +0x08 */ u16 adjustedDispPosX;
    /* +0x0A */ u16 adjustedDispPosY;
    /* +0x0C */ u16 adjustedDispSizeY;
    /* +0x0E */ u16 adjustedPanPosY;
    /* +0x10 */ u16 adjustedPanSizeY;
    /* +0x12 */ u16 fbSizeX;
    /* +0x14 */ u16 fbSizeY;
    /* +0x16 */ u16 panPosX;
    /* +0x18 */ u16 panPosY;
    /* +0x1A */ u16 panSizeX;
    /* +0x1C */ u16 panSizeY;
    /* +0x20 */ u32 fbMode;
    /* +0x24 */ u32 scanMode;
    /* +0x28 */ u32 tvFormat;
    /* +0x2C */ u8 wordPerLine;
    /* +0x2D */ u8 std;
    /* +0x2E */ u8 wpl;
    /* +0x30 */ u32 bufAddr;
    /* +0x34 */ u32 tfbb;
    /* +0x38 */ u32 bfbb;
    /* +0x3C */ u8 xof;
    /* +0x40 */ u32 black;
    /* +0x44 */ u32 threeD;
    /* +0x48 */ u32 rbufAddr;
    /* +0x4C */ u32 rtfbb;
    /* +0x50 */ u32 rbfbb;
    /* +0x54 */ VITimingInfo* timing;
}; /* size: 0x58 */

typedef void (*VIPositionCallback)(s16 x, s16 y);

const char* __VIVersion = "<< RVL_SDK - VI \trelease build: Feb 27 2009 10:04:46 (0x4302_145) >>";

static VITimingInfo timing[11] = {
    {6, 240, 24, 25, 3, 2, 12, 13, 12, 13, 520, 519, 520, 519, 525, 429, 64, 71, 105, 162, 373, 122, 412},
    {6, 240, 24, 24, 4, 4, 12, 12, 12, 12, 520, 520, 520, 520, 526, 429, 64, 71, 105, 162, 373, 122, 412},
    {5, 287, 35, 36, 1, 0, 13, 12, 11, 10, 619, 618, 617, 620, 625, 432, 64, 75, 106, 172, 380, 133, 420},
    {5, 287, 33, 33, 2, 2, 13, 11, 13, 11, 619, 621, 619, 621, 624, 432, 64, 75, 106, 172, 380, 133, 420},
    {6, 240, 24, 25, 3, 2, 16, 15, 14, 13, 518, 517, 516, 519, 525, 429, 64, 78, 112, 162, 373, 122, 412},
    {6, 240, 24, 24, 4, 4, 16, 14, 16, 14, 518, 520, 518, 520, 526, 429, 64, 78, 112, 162, 373, 122, 412},
    {12, 480, 48, 48, 6, 6, 24, 24, 24, 24, 1038, 1038, 1038, 1038, 1050, 429, 64, 71, 105, 162, 373, 122, 412},
    {12, 480, 44, 44, 10, 10, 24, 24, 24, 24, 1038, 1038, 1038, 1038, 1050, 429, 64, 71, 105, 168, 379, 122, 412},
    {6, 241, 24, 25, 1, 0, 12, 13, 12, 13, 520, 519, 520, 519, 525, 429, 64, 71, 105, 159, 370, 122, 412},
    {12, 480, 48, 48, 6, 6, 24, 24, 24, 24, 1038, 1038, 1038, 1038, 1050, 429, 64, 71, 105, 180, 391, 122, 412},
    {10, 576, 62, 62, 6, 6, 20, 20, 20, 20, 1240, 1240, 1240, 1240, 1250, 432, 64, 75, 106, 172, 380, 122, 412},
};

static u16 taps[25] = {
    496, 476, 430, 372, 297, 219, 142, 70, 12, 226, 203, 192, 196, 207, 222, 236, 252, 8, 15, 19, 19, 15, 12, 8, 1,
};

static GXRenderModeObj viRenderModes[3] = {
    {6, 640, 528, 528, 40, 23, 640, 528, 0, 0, 0, 0,
     {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
     {0, 0, 21, 22, 21, 0, 0}},
    {6, 640, 528, 528, 40, 23, 640, 528, 0, 0, 0, 0,
     {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
     {8, 8, 10, 12, 10, 8, 8}},
    {6, 640, 264, 524, 40, 23, 640, 524, 0, 0, 0, 1,
     {{3, 2}, {9, 6}, {3, 10}, {3, 2}, {9, 6}, {3, 10}, {9, 2}, {3, 6}, {9, 10}, {9, 2}, {3, 6}, {9, 10}},
     {4, 8, 12, 16, 12, 8, 4}},
};

extern "C" {
static BOOL __VIShutdown(BOOL final, u32 event);
}

static OSShutdownFunctionInfo viShutdownFunctionInfo = {__VIShutdown, 127, NULL, NULL};

static BOOL viShutdownFlushPending = TRUE;
static u32 viPrevDTVStatus = 999;
static u32 viPrevTvFormat = 999;
static BOOL viPrevDimmingEnabled = TRUE;
static BOOL viPrevDvdStopMotorEnabled = TRUE;

static BOOL viInitialized;
static u32 viRetraceLineMisses;
static BOOL viRetraceLineCheck;
static volatile BOOL viDimmingResetPending;
static volatile u32 viDimmingThreshold;
volatile u32 viDimmingTimeout;
static volatile u32 viDvdStopMotorTimeout;
static volatile u32 viDimmingIdleCount;
static volatile u32 viDvdIdleCount;
static volatile BOOL viDimmed;
static VIPositionCallback viPositionCallback;
static s16 viDisplayOffsetH;
static s16 viDisplayOffsetV;
volatile u32 viChangeMode;
volatile u64 viChanged;
volatile u32 viShdwChangeMode;
volatile u64 viShdwChanged;
BOOL viFrameBufferChanged;
static VITimingInfo* viUserTiming;
static u32 viShutdownRetraceCount;
static BOOL viDimRequested;
static volatile BOOL viUndimRequested;
static BOOL viDebugPalCautionShown;
static u32 viCurrBufAddr;
u32 viNextBufAddr;
static u32 CurrTvMode;
VITimingInfo* viCurrTiming;
static BOOL viKeepCCIR656Regs;
VIRetraceCallback viPostRetraceCallback;
VIRetraceCallback viPreRetraceCallback;
OSThreadQueue viRetraceQueue;
volatile u32 viDimmingControlB;
volatile u32 viDimmingControlA;
volatile s32 viDimmingMode;
static volatile BOOL viDvdStopMotorEnabled;
volatile BOOL viDimmingEnabled;
u32 viFlushPending;
u32 viFlushDone;
volatile u32 viRetraceCount;

u16 viRegs[0x3C];
u16 viShdwRegs[0x3C];
VIHorVer viHorVer;
u32 viDimming[10];

extern "C" {
static void __VIRetraceHandler(s16 interrupt, OSContext* context);
static VITimingInfo* getTiming(u32 mode);
static void __VIInit(u32 mode);
static void setFbbRegs(VIHorVer* horVer, u32* tfbb, u32* bfbb, u32* rtfbb, u32* rbfbb);
static void setHorizontalRegs(VITimingInfo* tm, u16 dispPosX, u16 dispSizeX);
static void setVerticalRegs(u16 dispPosY, u16 dispSizeY, u8 equ, u16 acv, u16 prbOdd, u16 prbEven, u16 psbOdd,
                            u16 psbEven, BOOL black);
}

/* Counts the leading zero bits of a 64-bit value. */
static inline s32 cntlzd(u64 bit)
{
    s32 value;
    u32 hi;
    u32 lo;

    hi = (u32)(bit >> 32);
    lo = (u32)(bit & 0xFFFFFFFF);
    value = __cntlzw(hi);
    if (value < 32) {
        return value;
    }
    return 32 + __cntlzw(lo);
}

/* Counts the leading zero bits of a 64-bit value. */
static inline s32 cntlzdFlush(u64 bit)
{
    u32 hi;
    u32 lo;
    s32 value;

    hi = (u32)(bit >> 32);
    lo = (u32)(bit & 0xFFFFFFFF);
    value = __cntlzw(hi);
    if (value < 32) {
        return value;
    }
    return 32 + __cntlzw(lo);
}

/* Reads the beam position (horizontal and vertical counters) consistently. */
static inline void getCurrentDisplayPosition(u32* hct, u32* vct)
{
    u32 hcount;
    u32 vcount0;
    u32 vcount;

    vcount = VI_REG16(0x2C) & 0x7FF;
    do {
        vcount0 = vcount;
        hcount = VI_REG16(0x2E) & 0x7FF;
        vcount = VI_REG16(0x2C) & 0x7FF;
    } while (vcount0 != vcount);
    *hct = hcount;
    *vct = vcount;
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

/* Reads the current half-line of the frame (vertical counter doubled plus the horizontal position). */
static inline u32 getNextFieldHalfLine(void)
{
    u32 hcount;
    u32 vcountPrev;
    u32 vcount;

    vcount = VI_REG16(0x2C) & 0x7FF;
    do {
        vcountPrev = vcount;
        hcount = VI_REG16(0x2E) & 0x7FF;
        vcount = VI_REG16(0x2C) & 0x7FF;
    } while (vcountPrev != vcount);
    return ((vcount - 1) << 1) + (hcount - 1) / viCurrTiming->hlw;
}

/* Tells which field (0 or 1) the beam is in, through `getNextFieldHalfLine`. */
static inline u32 getNextFieldEvenOdd(void)
{
    return getNextFieldHalfLine() < viCurrTiming->nhlines ? 1 : 0;
}

/* Tells which field (0 or 1) the beam is in. */
static inline u32 getCurrentFieldEvenOdd(void)
{
    return getCurrentHalfLine() < viCurrTiming->nhlines ? 1 : 0;
}

/* Returns the TV format (NTSC, PAL, MPAL, EURGB60) of the current mode. */
static inline u32 getTvFormat(void)
{
    BOOL enabled;
    u32 format;

    enabled = OSDisableInterrupts();
    switch (CurrTvMode) {
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
        format = CurrTvMode;
        break;
    }
    OSRestoreInterrupts(enabled);
    return format;
}

/* Returns the scan mode: 2 when the progressive bit is set, else 1 for the non-interlaced flag and 0 for interlaced. */
static inline u32 getScanMode(void)
{
    BOOL enabled;
    u32 mode;
    u32 progressive;

    enabled = OSDisableInterrupts();
    progressive = VI_REG16(0x6C) & 1;
    if (progressive == 1) {
        mode = 2;
    } else {
        mode = VI_DISP_CONFIG.nin ? 1 : 0;
    }
    OSRestoreInterrupts(enabled);
    return mode;
}

/* Returns the DTV status bit. */
static inline u8 getDTVStatus(void)
{
    BOOL enabled;
    u32 status;

    enabled = OSDisableInterrupts();
    status = VI_REG16(0x6E) & 3;
    OSRestoreInterrupts(enabled);
    return status & 1;
}

/* Turns screen dimming on or off; it stays off when the console's screen saver setting is off. Returns the previous setting. */
static inline u32 enableDimming(s32 enable)
{
    u32 old = viDimmingEnabled;

    if (enable == 1 && SCGetScreenSaverMode() == 0) {
        enable = 0;
    }
    viDimmingEnabled = enable;
    return old;
}

/* Copies the pending shadow registers to the hardware unless a mode change waits for the second field. */
static inline BOOL setRegs(void)
{
    s32 regIndex;

    if (!(viShdwChangeMode == 1 && getCurrentFieldEvenOdd() == 0)) {
        while (viShdwChanged) {
            regIndex = cntlzd(viShdwChanged);
            VI_REGS[regIndex] = viRegs[regIndex];
            viShdwChanged &= ~VI_BITMASK(regIndex);
        }
        viShdwChangeMode = 0;
        viCurrTiming = viHorVer.timing;
        CurrTvMode = viHorVer.tvFormat;
        viCurrBufAddr = viNextBufAddr;
        return TRUE;
    }
    return FALSE;
}

/* Copies every changed shadow register into the register file and publishes the pending frame buffer. */
static inline void flushRegs(void)
{
    BOOL enabled;
    s32 regIndex;

    enabled = OSDisableInterrupts();
    viShdwChangeMode |= viChangeMode;
    viChangeMode = 0;
    viShdwChanged |= viChanged;
    while (viChanged) {
        regIndex = cntlzd(viChanged);
        viRegs[regIndex] = viShdwRegs[regIndex];
        viChanged &= ~VI_BITMASK(regIndex);
    }
    viFlushDone = 1;
    viFlushPending = 1;
    viNextBufAddr = viHorVer.bufAddr;
    OSRestoreInterrupts(enabled);
}

/* Flushes the video state on a shutdown event; returns TRUE once the display is ready for it. */
static BOOL __VIShutdown(BOOL final, u32 event)
{
    if (!final) {
        switch (event) {
        case 1:
        case 2:
        case 3:
            if (viShutdownFlushPending) {
                __VIRequestEncoderUpdate();
                flushRegs();
                viShutdownRetraceCount = viRetraceCount;
                viShutdownFlushPending = FALSE;
                return FALSE;
            }
            if (viShutdownRetraceCount != viRetraceCount) {
                return TRUE;
            }
            break;
        case 0:
        case 4:
        case 5:
        case 6:
            __VISetLinearGamma();
            return TRUE;
        }
        return FALSE;
    } else {
        return TRUE;
    }
}

/* Handles the display interrupt: programs the flushed registers, runs the callbacks and drives the screen dimming. */
static void __VIRetraceHandler(s16 interrupt, OSContext* context)
{
    OSContext exceptionContext;
    u16 reg;
    u32 inter;
    u32 tvFormat;
    u8 dtvStatus;
    u32 bit;
    s32 i;

    inter = 0;
    reg = VI_REG16(0x30);
    if (reg & 0x8000) {
        VI_REG16(0x30) = reg & ~0x8000;
        inter |= 1;
    }
    reg = VI_REG16(0x34);
    if (reg & 0x8000) {
        VI_REG16(0x34) = reg & ~0x8000;
        inter |= 2;
    }
    reg = VI_REG16(0x38);
    if (reg & 0x8000) {
        VI_REG16(0x38) = reg & ~0x8000;
        inter |= 4;
    }
    reg = VI_REG16(0x3C);
    if (reg & 0x8000) {
        VI_REG16(0x3C) = reg & ~0x8000;
        inter |= 8;
    }
    reg = VI_REG16(0x3C);
    if ((inter & 4) || (inter & 8)) {
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(&exceptionContext);
        if (viPositionCallback) {
            s16 x;
            s16 y;
            u32 h;
            u32 v;

            getCurrentDisplayPosition(&h, &v);
            __VIDisplayPositionToXY(h, v, &x, &y);
            viPositionCallback(x, y);
        }
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(context);
        return;
    }

    viRetraceCount++;
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(&exceptionContext);
    if (viPreRetraceCallback) {
        viPreRetraceCallback(viRetraceCount);
    }

    if (viRetraceLineCheck) {
        u32 h;
        u32 v;

        getCurrentDisplayPosition(&h, &v);
        if (v != 1 && v != (u32)viCurrTiming->nhlines / 2 + 1) {
            viRetraceLineMisses++;
        }
    }

    if (viFlushDone) {
        if (setRegs()) {
            viFlushDone = 0;
            SIRefreshSamplingRate();
        }
    }

    dtvStatus = getDTVStatus();
    if (dtvStatus != viPrevDTVStatus) {
        __VISetEncoderMode(dtvStatus);
    }
    viPrevDTVStatus = dtvStatus;

    tvFormat = getTvFormat();
    if (tvFormat != viPrevTvFormat) {
        if (tvFormat == 5) {
            __VISetFilter4EURGB60(1);
        } else {
            __VISetFilter4EURGB60(0);
        }
        if (tvFormat == 1) {
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
            viDvdStopMotorTimeout = 90000;
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
            viDvdStopMotorTimeout = 108000;
        }
        viDimmingIdleCount = 0;
        viDvdIdleCount = 0;
    }
    viPrevTvFormat = tvFormat;

    if (viFlushPending) {
        while (viEncoderDirtyFlagsVolatile) {
            bit = 1 << (31 - __cntlzw(viEncoderDirtyFlagsVolatile));
            switch (bit) {
            case 1:
                __VISetCGMS();
                break;
            case 2:
                __VISetWSS();
                break;
            case 4:
                __VISetClosedCaption();
                break;
            case 8:
                __VISendEncoderRegs();
                break;
            case 16:
                __VISetGamma();
                break;
            case 32:
                __VISetTrapFilter();
                break;
            case 64:
                __VISendEncoderReg0A();
                break;
            case 128:
                __VISetRGBModeImm();
                break;
            }
            viEncoderDirtyFlagsVolatile &= ~bit;
        }
        viFlushPending = 0;
    }

    if (viPostRetraceCallback) {
        OSClearContext(&exceptionContext);
        viPostRetraceCallback(viRetraceCount);
    }
    OSWakeupThread(&viRetraceQueue);
    OSClearContext(&exceptionContext);
    OSSetCurrentContext(context);

    if (viDimmingResetPending == 1) {
        if (__OSSetVIForceDimming(0, 0, 0) == 1) {
            viDimmingResetPending = 0;
            viDimmingIdleCount = 0;
        }
    }
    for (i = 0; i < 10; i++) {
        if (viDimming[i] == 0) {
            viDimming[0] = 0;
            break;
        }
    }
    if (viDimmingControlA && viDimmingControlB && viDimming[0]) {
        if (viDimmingEnabled == 1 && viDimmingIdleCount < 0xFFFFFFFF) {
            viDimmingIdleCount++;
        }
        if (viDvdStopMotorEnabled == 1 && viDvdIdleCount < 0xFFFFFFFF) {
            viDvdIdleCount++;
        }
    } else {
        if (viDimmingIdleCount >= viDimmingThreshold) {
            viUndimRequested = 1;
        }
        if (viDvdIdleCount >= viDvdStopMotorTimeout) {
            __DVDRestartMotor();
        }
        viDimmingIdleCount = 0;
        viDvdIdleCount = 0;
        viDimmingThreshold = viDimmingTimeout;
    }
    if (viPrevDimmingEnabled != viDimmingEnabled) {
        if (viDimmingEnabled == 0) {
            if (viDimmingIdleCount >= viDimmingThreshold) {
                viUndimRequested = 1;
            }
        }
        viDimmingIdleCount = 0;
        viDimmingThreshold = viDimmingTimeout;
    }
    if (viDimmingIdleCount == viDimmingThreshold) {
        viDimRequested = 1;
    }
    if (viUndimRequested) {
        if (__OSSetVIForceDimming(0, 2, 2) == 1) {
            viUndimRequested = 0;
            viDimmed = 0;
        }
    }
    if (viDimRequested) {
        if (__OSSetVIForceDimming(1, 2, 2) == 1) {
            viDimRequested = 0;
            viDimmed = 1;
        }
    }
    if (viPrevDvdStopMotorEnabled != viDvdStopMotorEnabled) {
        if (viDvdStopMotorEnabled == 0) {
            if (viDvdIdleCount >= viDvdStopMotorTimeout) {
                __DVDRestartMotor();
            }
        }
        viDvdIdleCount = 0;
    }
    if (viDvdIdleCount == viDvdStopMotorTimeout) {
        __DVDStopMotorAsync(&__DVDStopMotorCommandBlock, NULL);
    }
    viDimmingControlA = 1;
    viDimmingControlB = 1;
    for (i = 0; i < 10; i++) {
        viDimming[i] = 1;
    }
    viPrevDimmingEnabled = viDimmingEnabled;
    viPrevDvdStopMotorEnabled = viDvdStopMotorEnabled;
    if (viDimmingTimeout > viDimmingIdleCount && !viDimmed) {
        viDimmingThreshold = viDimmingTimeout;
    }
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

/* Returns the timing record of a TV mode (format << 2 | scan mode), the user timing for the custom modes, or NULL. */
static VITimingInfo* getTiming(u32 mode)
{
    switch (mode) {
    case 0:
        return &timing[0];
    case 1:
        return &timing[1];
    case 4:
        return &timing[2];
    case 5:
        return &timing[3];
    case 20:
        return &timing[0];
    case 21:
        return &timing[1];
    case 8:
        return &timing[4];
    case 9:
        return &timing[5];
    case 2:
    case 10:
    case 22:
        return &timing[6];
    case 3:
        return &timing[7];
    case 16:
        return &timing[2];
    case 17:
        return &timing[3];
    case 24:
        return &timing[8];
    case 26:
        return &timing[9];
    case 6:
        return &timing[10];
    case 28:
    case 29:
    case 30:
    case 34:
        return viUserTiming;
    }
    return NULL;
}

/* Resets the video interface and programs every timing register for a TV mode. */
static void __VIInit(u32 mode)
{
    VITimingInfo* tm;
    u32 nonInter;
    volatile u32 delay;
    u32 tv;
    u32 tvForReg;
    u16 hct;
    u16 vct;

    nonInter = mode & 3;
    tv = mode >> 2;
    VI_BOOT_TV_FORMAT = tv;
    tm = getTiming(mode);
    VI_REG16(0x02) = 2;
    for (delay = 0; delay < 1000; delay++) {
    }
    VI_REG16(0x02) = 0;
    VI_REG16(0x06) = tm->hlw;
    VI_REG16(0x04) = tm->hce | (tm->hcs << 8);
    VI_REG16(0x0A) = tm->hsy | (tm->hbe640 << 7);
    VI_REG16(0x08) = (tm->hbe640 >> 9) | (tm->hbs640 << 1);
    if (!viKeepCCIR656Regs) {
        VI_REG16(0x72) = tm->hbeCCIR656 | 0x8000;
        VI_REG16(0x74) = tm->hbsCCIR656;
    }
    VI_REG16(0x00) = tm->equ;
    VI_REG16(0x0E) = tm->prbOdd + tm->acv * 2 - 2;
    VI_REG16(0x0C) = tm->psbOdd + 2;
    VI_REG16(0x12) = tm->prbEven + tm->acv * 2 - 2;
    VI_REG16(0x10) = tm->psbEven + 2;
    VI_REG16(0x16) = tm->bs1 | (tm->be1 << 5);
    VI_REG16(0x14) = tm->bs3 | (tm->be3 << 5);
    VI_REG16(0x1A) = tm->bs2 | (tm->be2 << 5);
    VI_REG16(0x18) = tm->bs4 | (tm->be4 << 5);
    VI_REG16(0x48) = 40 | (40 << 8);
    VI_REG16(0x36) = 1;
    VI_REG16(0x34) = 1 | (1 << 12);
    hct = tm->hlw + 1;
    vct = tm->nhlines / 2 + 1;
    VI_REG16(0x32) = hct;
    VI_REG16(0x30) = vct | (1 << 12);
    switch (tv) {
    case 1:
    case 2:
    case 3:
        tvForReg = tv;
        break;
    default:
        tvForReg = 0;
        break;
    }
    if (nonInter == 0 || nonInter == 1) {
        VI_REG16(0x02) = 1 | ((nonInter & 1) << 2) | (tvForReg << 8);
        VI_REG16(0x6C) = 0;
    } else {
        VI_REG16(0x02) = 1 | 4 | (tvForReg << 8);
        VI_REG16(0x6C) = 1;
    }
}

/* Clamps the display window to the screen and derives the adjusted position, size and pan of the picture. */
static inline void adjustPosition(u16 acv)
{
    s32 coeff;
    s32 frac;

    viHorVer.adjustedDispPosX = (u16)CLAMP((s16)viHorVer.dispPosX + viDisplayOffsetH, 0, 720 - viHorVer.dispSizeX);
    coeff = (viHorVer.fbMode == 0) ? 2 : 1;
    frac = viHorVer.dispPosY & 1;
    viHorVer.adjustedDispPosY = (u16)MAX((s16)viHorVer.dispPosY + viDisplayOffsetV, frac);
    viHorVer.adjustedDispSizeY =
        (u16)(viHorVer.dispSizeY + MIN((s16)viHorVer.dispPosY + viDisplayOffsetV - frac, 0) -
              MAX((s16)viHorVer.dispPosY + (s16)viHorVer.dispSizeY + viDisplayOffsetV - ((s16)acv * 2 - frac), 0));
    viHorVer.adjustedPanPosY = (u16)(viHorVer.panPosY - MIN((s16)viHorVer.dispPosY + viDisplayOffsetV - frac, 0) / coeff);
    viHorVer.adjustedPanSizeY =
        (u16)(viHorVer.panSizeY + MIN((s16)viHorVer.dispPosY + viDisplayOffsetV - frac, 0) / coeff -
              MAX((s16)viHorVer.dispPosY + (s16)viHorVer.dispSizeY + viDisplayOffsetV - ((s16)acv * 2 - frac), 0) / coeff);
}

/* Reads the console's display offset setting. */
static inline void importAdjustingValues(void)
{
    viDisplayOffsetH = SCGetDisplayOffsetH();
    viDisplayOffsetV = 0;
}

/* Turns stopping the disc motor on idle on or off; returns the previous setting. */
static inline BOOL enableDvdStopMotor(BOOL enable)
{
    BOOL old = viDvdStopMotorEnabled;

    viDvdStopMotorEnabled = enable;
    return old;
}

/* Brings the video interface up: programs the filter taps, reads the boot mode and installs the retrace handler. */
void VIInit(void)
{
    u16 dspCfg;
    u32 value;
    u32 tv;
    u32 tvInBootrom;
    u32 tvFormat;

    if (viInitialized) {
        return;
    }
    OSRegisterVersion(__VIVersion);
    viInitialized = TRUE;
    if (!(VI_REG16(0x02) & 1)) {
        __VIInit(0);
    }
    viRetraceCount = 0;
    viChanged = 0;
    viShdwChanged = 0;
    viChangeMode = 0;
    viShdwChangeMode = 0;
    viFlushDone = 0;
    viFlushPending = 0;
    VI_REG16(0x4E) = taps[0] | ((taps[1] & 0x3F) << 10);
    VI_REG16(0x4C) = (taps[1] >> 6) | (taps[2] << 4);
    VI_REG16(0x52) = taps[3] | ((taps[4] & 0x3F) << 10);
    VI_REG16(0x50) = (taps[4] >> 6) | (taps[5] << 4);
    VI_REG16(0x56) = taps[6] | ((taps[7] & 0x3F) << 10);
    VI_REG16(0x54) = (taps[7] >> 6) | (taps[8] << 4);
    VI_REG16(0x5A) = taps[9] | (taps[10] << 8);
    VI_REG16(0x58) = taps[11] | (taps[12] << 8);
    VI_REG16(0x5E) = taps[13] | (taps[14] << 8);
    VI_REG16(0x5C) = taps[15] | (taps[16] << 8);
    VI_REG16(0x62) = taps[17] | (taps[18] << 8);
    VI_REG16(0x60) = taps[19] | (taps[20] << 8);
    VI_REG16(0x66) = taps[21] | (taps[22] << 8);
    VI_REG16(0x64) = taps[23] | (taps[24] << 8);
    VI_REG16(0x70) = 640;
    importAdjustingValues();
    tvInBootrom = VI_BOOT_TV_FORMAT;
    dspCfg = VI_REG16(0x02);
    viHorVer.scanMode = getScanMode();
    viHorVer.tvFormat = (dspCfg >> 8) & 3;
    if (tvInBootrom == 5 || (tvInBootrom == 1 && viHorVer.tvFormat == 0)) {
        viHorVer.tvFormat = 5;
    }
    tv = (viHorVer.tvFormat == 3) ? 0 : viHorVer.tvFormat;
    viHorVer.timing = getTiming((tv << 2) + viHorVer.scanMode);
    viShdwRegs[1] = dspCfg;
    viCurrTiming = viHorVer.timing;
    CurrTvMode = viHorVer.tvFormat;
    viHorVer.dispSizeX = 640;
    viHorVer.dispSizeY = viCurrTiming->acv * 2;
    viHorVer.dispPosX = (720 - viHorVer.dispSizeX) / 2;
    viHorVer.dispPosY = 0;
    adjustPosition(viCurrTiming->acv);
    viHorVer.fbSizeX = 640;
    viHorVer.fbSizeY = viCurrTiming->acv * 2;
    viHorVer.panPosX = 0;
    viHorVer.panPosY = 0;
    viHorVer.panSizeX = 640;
    viHorVer.panSizeY = viCurrTiming->acv * 2;
    viHorVer.fbMode = 0;
    viHorVer.wordPerLine = 40;
    viHorVer.std = 40;
    viHorVer.wpl = 40;
    viHorVer.xof = 0;
    viHorVer.black = TRUE;
    viHorVer.threeD = FALSE;
    OSInitThreadQueue(&viRetraceQueue);
    value = VI_REG16(0x30);
    value &= ~0x8000;
    VI_REG16(0x30) = value;
    value = VI_REG16(0x34);
    value &= ~0x8000;
    VI_REG16(0x34) = value;
    viPreRetraceCallback = NULL;
    viPostRetraceCallback = NULL;
    __OSSetInterruptHandler(24, __VIRetraceHandler);
    __OSUnmaskInterrupts(0x80);
    OSRegisterShutdownFunction(&viShutdownFunctionInfo);
    tvFormat = getTvFormat();
    if (tvFormat == 1) {
        viDimmingTimeout = viDimmingThreshold = 15000;
        viDvdStopMotorTimeout = 90000;
    } else {
        viDimmingTimeout = viDimmingThreshold = 18000;
        viDvdStopMotorTimeout = 108000;
    }
    viDimmingIdleCount = 0;
    viDvdIdleCount = 0;
    viDimmingMode = 0;
    viDimmingResetPending = 1;
    viDimmed = 0;
    enableDimming(TRUE);
    enableDvdStopMotor(FALSE);
    __VIInitEncoder();
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

/* Computes the top and bottom field base addresses of a frame buffer. */
static inline void calcFbbs(u32 bufAddr, u16 panPosX, u16 panPosY, u8 wordPerLine, u32 xfbMode, u16 dispPosY,
                            u32* tfbb, u32* bfbb)
{
    u32 bytesPerLine;
    u32 xoffInWords;
    u32 tmp;

    xoffInWords = (u32)panPosX / 16;
    bytesPerLine = (u32)wordPerLine * 32;
    *tfbb = bufAddr + xoffInWords * 32 + bytesPerLine * panPosY;
    *bfbb = (xfbMode == 0) ? *tfbb : (*tfbb + bytesPerLine);
    if (dispPosY % 2 == 1) {
        tmp = *tfbb;
        *tfbb = *bfbb;
        *bfbb = tmp;
    }
    *tfbb = *tfbb & 0x3FFFFFFF;
    *bfbb = *bfbb & 0x3FFFFFFF;
}

/* Programs the field base registers of the frame buffer (and of the right-eye buffer in 3D). */
static void setFbbRegs(VIHorVer* horVer, u32* tfbb, u32* bfbb, u32* rtfbb, u32* rbfbb)
{
    u32 shifted;

    calcFbbs(horVer->bufAddr, horVer->panPosX, horVer->adjustedPanPosY, horVer->wordPerLine, horVer->fbMode,
             horVer->adjustedDispPosY, tfbb, bfbb);
    if (horVer->threeD) {
        calcFbbs(horVer->rbufAddr, horVer->panPosX, horVer->adjustedPanPosY, horVer->wordPerLine, horVer->fbMode,
                 horVer->adjustedDispPosY, rtfbb, rbfbb);
    }
    if (*tfbb < 0x01000000 && *bfbb < 0x01000000 && *rtfbb < 0x01000000 && *rbfbb < 0x01000000) {
        shifted = 0;
    } else {
        shifted = 1;
    }
    if (shifted) {
        *tfbb >>= 5;
        *bfbb >>= 5;
        *rtfbb >>= 5;
        *rbfbb >>= 5;
    }
    viShdwRegs[15] = *tfbb & 0xFFFF;
    viChanged |= VI_BITMASK(15);
    viShdwRegs[14] = (*tfbb >> 16) | (horVer->xof << 8) | (shifted << 12);
    viChanged |= VI_BITMASK(14);
    viShdwRegs[19] = *bfbb & 0xFFFF;
    viChanged |= VI_BITMASK(19);
    viShdwRegs[18] = *bfbb >> 16;
    viChanged |= VI_BITMASK(18);
    if (horVer->threeD) {
        viShdwRegs[17] = *rtfbb & 0xFFFF;
        viChanged |= VI_BITMASK(17);
        viShdwRegs[16] = *rtfbb >> 16;
        viChanged |= VI_BITMASK(16);
        viShdwRegs[21] = *rbfbb & 0xFFFF;
        viChanged |= VI_BITMASK(21);
        viShdwRegs[20] = *rbfbb >> 16;
        viChanged |= VI_BITMASK(20);
    }
}

/* Programs the horizontal timing registers for a display position and width. */
static void setHorizontalRegs(VITimingInfo* tm, u16 dispPosX, u16 dispSizeX)
{
    u32 hbe;
    u32 hbs;
    u32 hbeLo;
    u32 hbeHi;

    viShdwRegs[3] = tm->hlw;
    viChanged |= VI_BITMASK(3);
    viShdwRegs[2] = tm->hce | (tm->hcs << 8);
    viChanged |= VI_BITMASK(2);
    if (viHorVer.tvFormat == 8) {
        hbe = tm->hbe640 + 172;
        hbs = tm->hbs640;
    } else {
        hbe = tm->hbe640 - 40 + dispPosX;
        hbs = tm->hbs640 + 40 + dispPosX - (720 - dispSizeX);
    }
    hbeLo = hbe & 0x1FF;
    hbeHi = hbe >> 9;
    viShdwRegs[5] = tm->hsy | (hbeLo << 7);
    viChanged |= VI_BITMASK(5);
    viShdwRegs[4] = hbeHi | (hbs << 1);
    viChanged |= VI_BITMASK(4);
}

/* Programs the vertical timing registers for a display position and height (blanked when black). */
static void setVerticalRegs(u16 dispPosY, u16 dispSizeY, u8 equ, u16 acv, u16 prbOdd, u16 prbEven, u16 psbOdd,
                            u16 psbEven, BOOL black)
{
    u16 actualPrbOdd;
    u16 actualPrbEven;
    u16 actualPsbOdd;
    u16 actualPsbEven;
    u16 actualAcv;
    s32 c;
    s32 d;

    if (viHorVer.scanMode == 2 || viHorVer.scanMode == 3) {
        c = 1;
        d = 2;
    } else {
        c = 2;
        d = 1;
    }
    if (dispPosY % 2 == 0) {
        actualPrbOdd = prbOdd + d * dispPosY;
        actualPsbOdd = psbOdd + d * ((c * acv - dispSizeY) - dispPosY);
        actualPrbEven = prbEven + d * dispPosY;
        actualPsbEven = psbEven + d * ((c * acv - dispSizeY) - dispPosY);
    } else {
        actualPrbOdd = prbEven + d * dispPosY;
        actualPsbOdd = psbEven + d * ((c * acv - dispSizeY) - dispPosY);
        actualPrbEven = prbOdd + d * dispPosY;
        actualPsbEven = psbOdd + d * ((c * acv - dispSizeY) - dispPosY);
    }
    actualAcv = dispSizeY / c;
    if (black) {
        actualPrbOdd = actualPrbOdd + (2 * actualAcv - 2);
        actualPsbOdd = actualPsbOdd + 2;
        actualPrbEven = actualPrbEven + (2 * actualAcv - 2);
        actualPsbEven = actualPsbEven + 2;
        actualAcv = 0;
    }
    viShdwRegs[0] = equ | (actualAcv << 4);
    viChanged |= VI_BITMASK(0);
    viShdwRegs[7] = actualPrbOdd;
    viChanged |= VI_BITMASK(7);
    viShdwRegs[6] = actualPsbOdd;
    viChanged |= VI_BITMASK(6);
    viShdwRegs[9] = actualPrbEven;
    viChanged |= VI_BITMASK(9);
    viShdwRegs[8] = actualPsbEven;
    viChanged |= VI_BITMASK(8);
}

/* Programs the display interrupt to fire at the start of the second field. */
static inline void setInterruptRegs(VITimingInfo* tm)
{
    u16 vct;
    u16 hct;
    u16 borrow;

    vct = tm->nhlines / 2;
    borrow = tm->nhlines % 2;
    hct = borrow ? tm->hlw : (u16)0;
    vct++;
    hct++;
    viShdwRegs[25] = hct;
    viChanged |= VI_BITMASK(25);
    viShdwRegs[24] = vct | 0x1000;
    viChanged |= VI_BITMASK(24);
}

/* Programs the horizontal scaler for a pan width shown on a display width. */
static inline void setScalingRegs(u16 panSizeX, u16 dispSizeX, BOOL threeD)
{
    u32 scale;

    panSizeX = threeD ? panSizeX * 2 : panSizeX;
    if (panSizeX < dispSizeX) {
        scale = (256 * (u32)panSizeX + (u32)dispSizeX - 1) / (u32)dispSizeX;
        viShdwRegs[37] = scale | 0x1000;
        viChanged |= VI_BITMASK(37);
        viShdwRegs[56] = panSizeX;
        viChanged |= VI_BITMASK(56);
    } else {
        viShdwRegs[37] = 256;
        viChanged |= VI_BITMASK(37);
    }
}

/* Programs the color burst blanking interval registers. */
static inline void setBBIntervalRegs(VITimingInfo* tm)
{
    u16 val;

    val = tm->bs1 | (tm->be1 << 5);
    viShdwRegs[11] = val;
    viChanged |= VI_BITMASK(11);
    val = tm->bs3 | (tm->be3 << 5);
    viShdwRegs[10] = val;
    viChanged |= VI_BITMASK(10);
    val = tm->bs2 | (tm->be2 << 5);
    viShdwRegs[13] = val;
    viChanged |= VI_BITMASK(13);
    val = tm->bs4 | (tm->be4 << 5);
    viShdwRegs[12] = val;
    viChanged |= VI_BITMASK(12);
}

/* Derives the frame buffer stride and pan offset, and programs the picture configuration register. */
static inline void setPicConfig(u16 fbSizeX, u32 xfbMode, u16 panPosX, u16 panSizeX, u8* wordPerLine, u8* std,
                                u8* wpl, u8* xof)
{
    *wordPerLine = (fbSizeX + 15) / 16;
    *std = (xfbMode == 0) ? *wordPerLine : (u8)(*wordPerLine * 2);
    *xof = panPosX % 16;
    *wpl = (*xof + panSizeX + 15) / 16;
    viShdwRegs[36] = *std | (*wpl << 8);
    viChanged |= VI_BITMASK(36);
}

/* Programs the video interface for a render mode. */
void VIConfigure(const GXRenderModeObj* rm)
{
    VITimingInfo* tm;
    u32 regDspCfg;
    u16 regClksel;
    BOOL enabled;
    u32 newNonInter;
    u32 tvInBootrom;
    u32 tvInGame;

    enabled = OSDisableInterrupts();
    newNonInter = rm->viTVmode & 3;
    if (viHorVer.scanMode != newNonInter) {
        viChangeMode = 1;
        viHorVer.scanMode = newNonInter;
    }
    tvInGame = rm->viTVmode >> 2;
    tvInBootrom = VI_BOOT_TV_FORMAT;
    if (tvInGame == 4) {
        if (!viDebugPalCautionShown) {
            viDebugPalCautionShown = TRUE;
            OSReport("***************************************\n");
            OSReport(" ! ! ! C A U T I O N ! ! !             \n");
            OSReport("This TV format \"DEBUG_PAL\" is only for \n");
            OSReport("temporary solution until PAL DAC board \n");
            OSReport("is available. Please do NOT use this   \n");
            OSReport("mode in real games!!!                  \n");
            OSReport("***************************************\n");
        }
    }
    if (((tvInBootrom != 1 && tvInBootrom != 5) && (tvInGame == 1 || tvInGame == 5)) ||
        ((tvInBootrom == 1 || tvInBootrom == 5) && (tvInGame != 1 && tvInGame != 5))) {
        OSPanic("vi.c", 2651, "VIConfigure(): Tried to change mode from (%d) to (%d), which is forbidden\n",
                tvInBootrom, tvInGame);
    }
    if (tvInGame == 0 || tvInGame == 2) {
        viHorVer.tvFormat = tvInBootrom;
    } else {
        viHorVer.tvFormat = tvInGame;
    }
    viHorVer.dispPosX = rm->viXOrigin;
    viHorVer.dispPosY = (viHorVer.scanMode == 1) ? (u16)(rm->viYOrigin * 2) : rm->viYOrigin;
    viHorVer.dispSizeX = rm->viWidth;
    viHorVer.fbSizeX = rm->fbWidth;
    viHorVer.fbSizeY = rm->xfbHeight;
    viHorVer.fbMode = rm->xfbMode;
    viHorVer.panSizeX = viHorVer.fbSizeX;
    viHorVer.panSizeY = viHorVer.fbSizeY;
    viHorVer.panPosX = 0;
    viHorVer.panPosY = 0;
    viHorVer.dispSizeY = (viHorVer.scanMode == 2)   ? viHorVer.panSizeY
                         : (viHorVer.scanMode == 3) ? viHorVer.panSizeY
                         : (viHorVer.fbMode == 0)   ? (u16)(viHorVer.panSizeY * 2)
                                                    : viHorVer.panSizeY;
    viHorVer.threeD = (viHorVer.scanMode == 3) ? TRUE : FALSE;
    tm = getTiming((viHorVer.tvFormat << 2) + viHorVer.scanMode);
    viHorVer.timing = tm;
    adjustPosition(tm->acv);
    setInterruptRegs(tm);
    regDspCfg = viShdwRegs[1];
    regClksel = viShdwRegs[54];
    if (viHorVer.scanMode == 2 || viHorVer.scanMode == 3) {
        regDspCfg = (regDspCfg & ~4) | 4;
        if (viHorVer.tvFormat == 8) {
            regClksel = regClksel & ~1;
        } else {
            regClksel = (regClksel & ~1) | 1;
        }
    } else {
        regDspCfg = (regDspCfg & ~4) | ((viHorVer.scanMode & 1) << 2);
        regClksel = regClksel & ~1;
    }
    regDspCfg = (regDspCfg & ~8) | (viHorVer.threeD << 3);
    regDspCfg = regDspCfg & ~0x300;
    if (viHorVer.tvFormat == 1 || viHorVer.tvFormat == 2 || viHorVer.tvFormat == 3) {
        regDspCfg |= viHorVer.tvFormat << 8;
    }
    viShdwRegs[1] = regDspCfg;
    viChanged |= VI_BITMASK(1);
    viShdwRegs[54] = regClksel;
    viChanged |= VI_BITMASK(54);
    setScalingRegs(viHorVer.panSizeX, viHorVer.dispSizeX, viHorVer.threeD);
    setHorizontalRegs(tm, viHorVer.adjustedDispPosX, viHorVer.dispSizeX);
    setBBIntervalRegs(tm);
    setPicConfig(viHorVer.fbSizeX, viHorVer.fbMode, viHorVer.panPosX, viHorVer.panSizeX, &viHorVer.wordPerLine,
                 &viHorVer.std, &viHorVer.wpl, &viHorVer.xof);
    if (viFrameBufferChanged) {
        setFbbRegs(&viHorVer, &viHorVer.tfbb, &viHorVer.bfbb, &viHorVer.rtfbb, &viHorVer.rbfbb);
    }
    setVerticalRegs(viHorVer.adjustedDispPosY, viHorVer.adjustedDispSizeY, tm->equ, tm->acv, tm->prbOdd, tm->prbEven,
                    tm->psbOdd, tm->psbEven, viHorVer.black);
    OSRestoreInterrupts(enabled);
}

/* Sets the pan window (the part of the frame buffer shown) and reprograms the dependent registers. */
void VIConfigurePan(u16 xOrg, u16 yOrg, u16 width, u16 height)
{
    BOOL enabled;
    VITimingInfo* tm;

    enabled = OSDisableInterrupts();
    viHorVer.panPosX = xOrg;
    viHorVer.panPosY = yOrg;
    viHorVer.panSizeX = width;
    viHorVer.panSizeY = height;
    viHorVer.dispSizeY = (viHorVer.scanMode == 2)   ? viHorVer.panSizeY
                         : (viHorVer.scanMode == 3) ? viHorVer.panSizeY
                         : (viHorVer.fbMode == 0)   ? (u16)(viHorVer.panSizeY * 2)
                                                    : viHorVer.panSizeY;
    tm = viHorVer.timing;
    adjustPosition(tm->acv);
    setScalingRegs(viHorVer.panSizeX, viHorVer.dispSizeX, viHorVer.threeD);
    setPicConfig(viHorVer.fbSizeX, viHorVer.fbMode, viHorVer.panPosX, viHorVer.panSizeX, &viHorVer.wordPerLine,
                 &viHorVer.std, &viHorVer.wpl, &viHorVer.xof);
    if (viFrameBufferChanged) {
        setFbbRegs(&viHorVer, &viHorVer.tfbb, &viHorVer.bfbb, &viHorVer.rtfbb, &viHorVer.rbfbb);
    }
    setVerticalRegs(viHorVer.adjustedDispPosY, viHorVer.dispSizeY, tm->equ, tm->acv, tm->prbOdd, tm->prbEven,
                    tm->psbOdd, tm->psbEven, viHorVer.black);
    OSRestoreInterrupts(enabled);
}

/* Copies every changed shadow register into the register file and publishes the pending frame buffer. */
void VIFlush(void)
{
    BOOL enabled;
    s32 regIndex;

    enabled = OSDisableInterrupts();
    viShdwChangeMode |= viChangeMode;
    viChangeMode = 0;
    viShdwChanged |= viChanged;
    while (viChanged) {
        regIndex = cntlzdFlush(viChanged);
        viRegs[regIndex] = viShdwRegs[regIndex];
        viChanged &= ~VI_BITMASK(regIndex);
    }
    viFlushDone = 1;
    viFlushPending = 1;
    viNextBufAddr = viHorVer.bufAddr;
    OSRestoreInterrupts(enabled);
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
    setVerticalRegs(viHorVer.adjustedDispPosY, viHorVer.dispSizeY, timing->equ, timing->acv, timing->prbOdd, timing->prbEven,
                    timing->psbOdd, timing->psbEven, black);
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
    nextField = getNextFieldEvenOdd() ^ 1;
    OSRestoreInterrupts(enabled);
    return nextField ^ (viHorVer.adjustedDispPosY & 1);
}

/* Returns the line the beam is on, within the field. */
u32 VIGetCurrentLine(void)
{
    BOOL enabled;
    u32 halfLine;
    VITimingInfo* timing = viCurrTiming;

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
    return getTvFormat();
}

/* Returns the scan mode: 2 when the progressive bit is set, else 1 for the non-interlaced flag and 0 for interlaced. */
u32 VIGetScanMode(void)
{
    return getScanMode();
}

/* Returns the DTV status bit. */
u8 VIGetDTVStatus(void)
{
    return getDTVStatus();
}

/* Converts a beam position (horizontal and vertical counter values) to a screen column and line, or -1 outside the picture. */
void __VIDisplayPositionToXY(u32 hcount, u32 vcount, s16* x, s16* y)
{
    u32 halfLine;

    halfLine = ((vcount - 1) << 1) + (hcount - 1) / viCurrTiming->hlw;
    if (viHorVer.scanMode == 0) {
        if (halfLine < viCurrTiming->nhlines) {
            if (halfLine < viCurrTiming->equ * 3 + viCurrTiming->prbOdd) {
                *y = -1;
            } else if (halfLine >= viCurrTiming->nhlines - viCurrTiming->psbOdd) {
                *y = -1;
            } else {
                *y = (halfLine - viCurrTiming->equ * 3 - viCurrTiming->prbOdd) & ~1;
            }
        } else {
            halfLine -= viCurrTiming->nhlines;
            if (halfLine < viCurrTiming->equ * 3 + viCurrTiming->prbEven) {
                *y = -1;
            } else if (halfLine >= viCurrTiming->nhlines - viCurrTiming->psbEven) {
                *y = -1;
            } else {
                *y = ((halfLine - viCurrTiming->equ * 3 - viCurrTiming->prbEven) & ~1) + 1;
            }
        }
    } else if (viHorVer.scanMode == 1) {
        if (halfLine >= viCurrTiming->nhlines) {
            halfLine -= viCurrTiming->nhlines;
        }
        if (halfLine < viCurrTiming->equ * 3 + viCurrTiming->prbOdd) {
            *y = -1;
        } else if (halfLine >= viCurrTiming->nhlines - viCurrTiming->psbOdd) {
            *y = -1;
        } else {
            *y = (halfLine - viCurrTiming->equ * 3 - viCurrTiming->prbOdd) & ~1;
        }
    } else if (viHorVer.scanMode == 2) {
        if (halfLine < viCurrTiming->nhlines) {
            if (halfLine < viCurrTiming->equ * 3 + viCurrTiming->prbOdd) {
                *y = -1;
            } else if (halfLine >= viCurrTiming->nhlines - viCurrTiming->psbOdd) {
                *y = -1;
            } else {
                *y = halfLine - viCurrTiming->equ * 3 - viCurrTiming->prbOdd;
            }
        } else {
            halfLine -= viCurrTiming->nhlines;
            if (halfLine < viCurrTiming->equ * 3 + viCurrTiming->prbEven) {
                *y = -1;
            } else if (halfLine >= viCurrTiming->nhlines - viCurrTiming->psbEven) {
                *y = -1;
            } else {
                *y = (halfLine - viCurrTiming->equ * 3 - viCurrTiming->prbEven) & ~1;
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
s32 VISetDimmingMode(s32 mode)
{
    s32 old = viDimmingMode;

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

/* Clears the dimming counter. */
u32 VIResetDimmingCount(void)
{
    viDimming[0] = 0;
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
