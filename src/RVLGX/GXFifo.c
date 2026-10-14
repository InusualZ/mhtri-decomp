/*
 * RVLGX/GXFifo.c - the SDK GX FIFO management (CP/GP link check, FIFO bases, breakpoint callback).
 *
 * RANGE. `.text` 0x804B4460-0x804B4E40 (9 functions / 0x9BC B); `.data` 0x8061A7A0-0x8061A7D0 (the `CPUFifo:` /
 *   `GP Fifo:` report strings of `CPGPLinkCheck`); `.bss` 0x807472A0-0x80747300 (GPFifo, CPUFifo); `.sbss`
 *   0x807951F0-0x80795210 (CPUFifoReady, GPFifoReady, __GXOverflowCount, gpFifoCleanState, BreakPointCB,
 *   GXOverflowSuspendInProgress, __GXCurrentThread, CPGPLinked).
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (`-O4,p`); no float operations.
 * NAMES. SDK names: the RVL SDK GXFifo.c function set, order and statics. GUESS: `gpFifoCleanState` (only
 *   `__GXCleanGPFifo` writes it, with 0). The two report strings are `@NNN` locals here and `lbl_8061A7A0` /
 *   `lbl_8061A7B8` in the map (relocation names only).
 * RESIDUALS. `.bss` emits 0x48 B against the 0x60 B claim; the target's 0x18-byte zero tail is fill up to
 *   IPC/ipcclt's 32-aligned `.bss` at 0x80747300, and splits.txt gives that line no `align:` - this is the flipcheck
 *   blocker.
 * SHAPES. `GXInitFifoPtrs`, `GXInitFifoLimits`, `__GXFifoReadEnable`/`Disable`, `__GXWriteFifoIntEnable`/`Reset` and
 *   `__GXFifoLink` are absent from the DOL: static inline helpers above their callers. They insert with `GX_SET_REG`
 *   straight into the `__GXData` word (a constant 1 folds to `ori`, a 0 to `rlwinm`); a local copy of the word
 *   colours `GXSetCPUFifo`'s unlinked branch differently. The FIFO descriptor is read
 *   through the private `GXFifoObjPriv` view (0x24 B, the size `__GXFifoInit` clears). The statics are declared in
 *   reverse of their address order (MWCC emits them reversed).
 */

#pragma function_align 16

#include "types.h"
#include "gx.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXFifo.h"
#include "OS/OSInterrupt.h"
#include "OS/OSContext.h"
#include "OS/OSThread.h"
#include "OS/OSError.h"
#include "OS/PPCArch.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The private view of a `GXFifoObj`: the buffer, its watermarks and the read/write pointers. size: 0x24 */
typedef struct GXFifoObjPriv {
    /* +0x00 */ u8* base;
    /* +0x04 */ u8* top;
    /* +0x08 */ u32 size;
    /* +0x0C */ u32 hiWatermark;
    /* +0x10 */ u32 loWatermark;
    /* +0x14 */ u8* rdPtr;
    /* +0x18 */ u8* wrPtr;
    /* +0x1C */ s32 count;
    /* +0x20 */ u8 wrap;
    /* +0x21 */ u8 bindCpu;
    /* +0x22 */ u8 bindGp;
    /* +0x23 */ u8 pad_0x23;
} GXFifoObjPriv;

static u8 CPGPLinked;
static OSThread* __GXCurrentThread;
static BOOL GXOverflowSuspendInProgress;
static GXBreakPtCallback BreakPointCB;
static u32 gpFifoCleanState;
static u32 __GXOverflowCount;
static u8 GPFifoReady;
static u8 CPUFifoReady;

static GXFifoObjPriv CPUFifo;
static GXFifoObjPriv GPFifo;

/* Sets the FIFO watermark interrupt enables in the CP control register. */
static inline void __GXWriteFifoIntEnable(u32 hiWatermarkEn, u32 loWatermarkEn)
{
    GX_SET_REG(__GXData->cpEnable.word, hiWatermarkEn, 29, 29);
    GX_SET_REG(__GXData->cpEnable.word, loWatermarkEn, 28, 28);
    __cpReg[1] = __GXData->cpEnable.word;
}

/* Acknowledges the FIFO watermark interrupts in the CP clear register. */
static inline void __GXWriteFifoIntReset(u32 hiWatermarkClr, u32 loWatermarkClr)
{
    GX_SET_REG(__GXData->cpClr.word, hiWatermarkClr, 31, 31);
    GX_SET_REG(__GXData->cpClr.word, loWatermarkClr, 30, 30);
    __cpReg[2] = __GXData->cpClr.word;
}

/* Links or unlinks the CPU write FIFO to the GP read FIFO. */
static inline void __GXFifoLink(u32 enable)
{
    GX_SET_REG(__GXData->cpEnable.word, enable, 27, 27);
    __cpReg[1] = __GXData->cpEnable.word;
}

/* Starts the GP reading its FIFO. */
static inline void __GXFifoReadEnable(void)
{
    GX_SET_REG(__GXData->cpEnable.word, 1, 31, 31);
    __cpReg[1] = __GXData->cpEnable.word;
}

/* Stops the GP reading its FIFO. */
static inline void __GXFifoReadDisable(void)
{
    GX_SET_REG(__GXData->cpEnable.word, 0, 31, 31);
    __cpReg[1] = __GXData->cpEnable.word;
}

/* 0x804B4460 (0x160): handles the CP watermark and breakpoint interrupts. */
static void GXCPInterruptHandler(s16 interrupt, OSContext* context)
{
    OSContext exceptionContext;

    __GXData->cpStatus.word = __cpReg[0];

    if (__GXData->cpEnable.f.underflowIntEnable && __GXData->cpStatus.f.underflow) {
        OSResumeThread(__GXCurrentThread);
        GXOverflowSuspendInProgress = FALSE;
        __GXWriteFifoIntReset(1, 1);
        __GXWriteFifoIntEnable(1, 0);
    }

    if (__GXData->cpEnable.f.overflowIntEnable && __GXData->cpStatus.f.overflow) {
        __GXOverflowCount++;
        __GXWriteFifoIntEnable(0, 1);
        __GXWriteFifoIntReset(1, 0);
        GXOverflowSuspendInProgress = TRUE;
        OSSuspendThread(__GXCurrentThread);
    }

    if (__GXData->cpEnable.f.bpIntEnable && __GXData->cpStatus.f.bpInterrupt) {
        GX_SET_REG(__GXData->cpEnable.word, 0, 26, 26);
        __cpReg[1] = __GXData->cpEnable.word;
        if (BreakPointCB != NULL) {
            OSClearContext(&exceptionContext);
            OSSetCurrentContext(&exceptionContext);
            BreakPointCB();
            OSClearContext(&exceptionContext);
            OSSetCurrentContext(context);
        }
    }
}

/* Sets the FIFO watermarks. */
static inline void GXInitFifoLimits(GXFifoObjPriv* fifo, u32 hiWatermark, u32 loWatermark)
{
    fifo->hiWatermark = hiWatermark;
    fifo->loWatermark = loWatermark;
}

/* Sets the FIFO read and write pointers and the byte count between them. */
static inline void GXInitFifoPtrs(GXFifoObjPriv* fifo, u8* readPtr, u8* writePtr)
{
    BOOL enabled = OSDisableInterrupts();
    fifo->rdPtr = readPtr;
    fifo->wrPtr = writePtr;
    fifo->count = writePtr - readPtr;
    if (fifo->count < 0) {
        fifo->count += fifo->size;
    }
    OSRestoreInterrupts(enabled);
}

/* 0x804B45C0 (0x8C): fills a FIFO descriptor for the buffer `base`..`base + size` and resets its pointers. */
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size) /* untyped: byte range */
{
    GXFifoObjPriv* realFifo = (GXFifoObjPriv*)fifo;

    realFifo->base = base;
    realFifo->top = (u8*)base + size - 4;
    realFifo->size = size;
    realFifo->count = 0;
    GXInitFifoLimits(realFifo, size - 0x4000, (size >> 1) & ~0x1F);
    GXInitFifoPtrs(realFifo, base, base);
}

/* 0x804B4650 (0xF8): returns whether the CPU and GP FIFOs are the same buffer, reporting a partial overlap. */
static u8 CPGPLinkCheck(void)
{
    u32 check = 0;
    BOOL overlap;
    s32 diff1;
    s32 diff2;

    if (!CPUFifoReady || !GPFifoReady) {
        return FALSE;
    }

    if (CPUFifo.base == GPFifo.base) {
        check++;
    }
    if (CPUFifo.top == GPFifo.top) {
        check++;
    }
    if (check == 2) {
        return TRUE;
    }

    diff1 = CPUFifo.top - GPFifo.base;
    diff2 = GPFifo.top - CPUFifo.base;
    overlap = FALSE;
    if ((diff1 > 0 && diff2 > 0) || (diff1 < 0 && diff2 < 0)) {
        overlap = TRUE;
    }
    if (overlap) {
        OSReport("CPUFifo: %08X - %08X\n", CPUFifo.base, CPUFifo.top);
        OSReport("GP Fifo: %08X - %08X\n", GPFifo.base, GPFifo.top);
    }
    return FALSE;
}

/* 0x804B4750 (0x1E0): makes `fifo` the CPU write FIFO (NULL detaches it). */
void GXSetCPUFifo(const GXFifoObj* fifo)
{
    BOOL enabled = OSDisableInterrupts();
    u32 reg;

    if (fifo == NULL) {
        CPUFifoReady = FALSE;
        CPGPLinked = FALSE;
        CPUFifo.bindGp = FALSE;
        CPUFifo.bindCpu = FALSE;
        OSRestoreInterrupts(enabled);
        return;
    }

    CPUFifo = *(const GXFifoObjPriv*)fifo;
    CPUFifoReady = TRUE;
    CPUFifo.bindCpu = TRUE;

    if (CPGPLinkCheck()) {
        CPGPLinked = TRUE;
        CPUFifo.bindGp = TRUE;
        __piReg[3] = (u32)CPUFifo.base & 0x3FFFFFFF;
        __piReg[4] = (u32)CPUFifo.top & 0x3FFFFFFF;
        reg = 0;
        GX_SET_REG(reg, (u32)CPUFifo.wrPtr >> 5, 3, 26);
        __piReg[5] = reg;
        __GXWriteFifoIntReset(1, 1);
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    } else {
        CPUFifo.bindGp = FALSE;
        if (CPGPLinked) {
            __GXFifoLink(0);
            CPGPLinked = FALSE;
        }
        __GXWriteFifoIntEnable(0, 0);
        reg = 0;
        __piReg[3] = (u32)CPUFifo.base & 0x3FFFFFFF;
        __piReg[4] = (u32)CPUFifo.top & 0x3FFFFFFF;
        GX_SET_REG(reg, (u32)CPUFifo.wrPtr >> 5, 3, 26);
        __piReg[5] = reg;
    }

    PPCSync();
    OSRestoreInterrupts(enabled);
}

/* 0x804B4930 (0x290): makes `fifo` the GP read FIFO (NULL detaches it). */
void GXSetGPFifo(const GXFifoObj* fifo)
{
    BOOL enabled = OSDisableInterrupts();
    u32 reg;

    __GXFifoReadDisable();
    __GXWriteFifoIntEnable(0, 0);

    if (fifo == NULL) {
        GPFifoReady = FALSE;
        CPGPLinked = FALSE;
        __GXFifoLink(0);
        GPFifo.bindCpu = FALSE;
        GPFifo.bindGp = FALSE;
        OSRestoreInterrupts(enabled);
        return;
    }

    GPFifo = *(const GXFifoObjPriv*)fifo;
    GPFifoReady = TRUE;
    GPFifo.bindGp = TRUE;

    __cpReg[16] = (u32)GPFifo.base;
    __cpReg[18] = (u32)GPFifo.top;
    __cpReg[24] = GPFifo.count;
    __cpReg[26] = (u32)GPFifo.wrPtr;
    __cpReg[28] = (u32)GPFifo.rdPtr;
    __cpReg[20] = GPFifo.hiWatermark;
    __cpReg[22] = GPFifo.loWatermark;
    __cpReg[17] = ((u32)GPFifo.base & 0x3FFFFFFF) >> 16;
    __cpReg[19] = ((u32)GPFifo.top & 0x3FFFFFFF) >> 16;
    __cpReg[25] = GPFifo.count >> 16;
    __cpReg[27] = ((u32)GPFifo.wrPtr & 0x3FFFFFFF) >> 16;
    __cpReg[29] = ((u32)GPFifo.rdPtr & 0x3FFFFFFF) >> 16;
    __cpReg[21] = GPFifo.hiWatermark >> 16;
    __cpReg[23] = GPFifo.loWatermark >> 16;

    PPCSync();

    if (CPGPLinkCheck()) {
        CPGPLinked = TRUE;
        GPFifo.bindCpu = TRUE;
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    } else {
        CPGPLinked = FALSE;
        GPFifo.bindCpu = FALSE;
        __GXWriteFifoIntEnable(0, 0);
        __GXFifoLink(0);
    }

    reg = __GXData->cpEnable.word;
    GX_SET_REG(reg, 0, 30, 30);
    GX_SET_REG(reg, 0, 26, 26);
    __cpReg[1] = reg;
    __cpReg[1] = __GXData->cpEnable.word;

    __GXWriteFifoIntReset(1, 1);
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}

/* 0x804B4BC0 (0x8): returns whether a GP FIFO is attached. */
u8 __GXIsGPFifoReady(void)
{
    return GPFifoReady;
}

/* 0x804B4BD0 (0x44): installs the CP breakpoint callback and returns the previous one. */
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback cb)
{
    GXBreakPtCallback oldCallback = BreakPointCB;
    BOOL enabled = OSDisableInterrupts();
    BreakPointCB = cb;
    OSRestoreInterrupts(enabled);
    return oldCallback;
}

/* 0x804B4C20 (0x7C): installs the CP interrupt handler and clears both FIFO records. */
void __GXFifoInit(void)
{
    __OSSetInterruptHandler(17, GXCPInterruptHandler);
    __OSUnmaskInterrupts(0x4000);
    __GXCurrentThread = OSGetCurrentThread();
    GXOverflowSuspendInProgress = FALSE;
    memset(&CPUFifo, 0, sizeof(GXFifoObjPriv));
    memset(&GPFifo, 0, sizeof(GXFifoObjPriv));
    CPUFifoReady = FALSE;
    GPFifoReady = FALSE;
}

/* 0x804B4CA0 (0x1A0): discards the GP FIFO's pending commands (the read pointer catches up with the write pointer) and
 * restarts the read. */
void __GXCleanGPFifo(void)
{
    BOOL enabled;
    u32 reg;

    if (!GPFifoReady) {
        return;
    }

    enabled = OSDisableInterrupts();
    __GXFifoReadDisable();
    __GXWriteFifoIntEnable(0, 0);

    GPFifo.rdPtr = GPFifo.wrPtr;
    GPFifo.count = 0;
    __cpReg[24] = GPFifo.count;
    __cpReg[26] = (u32)GPFifo.wrPtr;
    __cpReg[28] = (u32)GPFifo.rdPtr;
    __cpReg[25] = GPFifo.count >> 16;
    __cpReg[27] = ((u32)GPFifo.wrPtr & 0x3FFFFFFF) >> 16;
    __cpReg[29] = ((u32)GPFifo.rdPtr & 0x3FFFFFFF) >> 16;
    PPCSync();

    if (CPGPLinked) {
        CPUFifo.rdPtr = GPFifo.rdPtr;
        CPUFifo.wrPtr = GPFifo.wrPtr;
        CPUFifo.count = GPFifo.count;
        reg = 0;
        GX_SET_REG(reg, (u32)CPUFifo.wrPtr >> 5, 3, 26);
        __piReg[5] = reg;
        __GXWriteFifoIntEnable(1, 0);
        __GXFifoLink(1);
    }

    GX_SET_REG(__GXData->cpEnable.word, 0, 30, 30);
    GX_SET_REG(__GXData->cpEnable.word, 0, 26, 26);
    __cpReg[1] = __GXData->cpEnable.word;
    gpFifoCleanState = 0;

    __GXWriteFifoIntReset(1, 1);
    __GXFifoReadEnable();
    OSRestoreInterrupts(enabled);
}
