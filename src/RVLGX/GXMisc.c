/*
 * RVLGX/GXMisc.c - the SDK GX miscellaneous control (misc setter, abort, draw-done, PE pokes and peeks, token/finish
 *   interrupts).
 *
 * RANGE. `.text` 0x804B5DB0-0x804B65F0 (20 functions / 0x798 B); `.sbss` 0x80795210-0x80795228 (FinishQueue +0x0,
 *   DrawDone +0x8, DrawDoneCB +0xC, TokenCB +0x10; only this unit reads them).
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (`-O4,p`); no float operations.
 * NAMES. SDK names: the RVL SDK GXMisc.c function set and order; the two interrupt handlers are file statics.
 * RESIDUALS. `__GXAbort` (0x804B5E30): the PE-copy counter loop colours the hi/hi-copy pair mirrored (r0/r3,
 *   r0/r4); the inlined copy in `GXAbortFrame` shares the allocation, so any shape that matches `__GXAbort`
 *   mismatches `GXAbortFrame` (measured); the `GXAbortFrame`-matching order is kept.
 * SHAPES. `GXFlush`, `GXSetDrawDone`, `GXWaitDrawDone`, `__GXAbortWait`, `__GXReadMEMCounterU32` and
 *   `__GXAbortWaitPECopyDone` are absent from the DOL: static inline helpers. The `.sbss` statics are declared in
 *   SDK order TokenCB, DrawDoneCB, DrawDone, FinishQueue (MWCC emits them reversed); `.sbss` is 0x14 B against the
 *   target's 0x18 B, the tail being the next object's 8-byte alignment. PI reset and EFB access use absolute
 *   addresses (0xCC003018, 0xC8000000 with `GX_SET_REG` x/y inserts).
 */
#include "types.h"
#include "gx.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXFifo.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXAbortFrame.h"
#include "OS/OSInterrupt.h"
#include "OS/OSContext.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"
#include "OS/PPCArch.h"

static GXDrawSyncCallback TokenCB;
static GXDrawDoneCallback DrawDoneCB;
static u8 DrawDone;
static OSThreadQueue FinishQueue;

void GXSetMisc(s32 token, u32 val)
{
    switch (token) {
    case 1:
        __GXData->vNum = val;
        __GXData->vNumNot = !__GXData->vNum;
        __GXData->xfWritePending = 1;
        if (__GXData->vNum) {
            __GXData->dirtyState |= 8;
        }
        break;
    case 2:
        __GXData->dlSaveContext = (val != 0);
        break;
    case 3:
        __GXData->abtWaitPECopy = (val != 0);
        break;
    }
}

static inline void __GXAbortWait(u32 clocks)
{
    s64 time0;
    s64 time1;
    time0 = OSGetTime();
    do {
        time1 = OSGetTime();
    } while (time1 - time0 <= (s64)(clocks / 4));
}

static inline u32 __GXReadMEMCounterU32(void)
{
    u32 hi0;
    u32 hi1;
    u32 lo;
    hi0 = __memReg[39];
    do {
        hi1 = hi0;
        lo = __memReg[40];
        hi0 = __memReg[39];
    } while (hi0 != hi1);
    return (hi0 << 16) | lo;
}

static inline void __GXAbortWaitPECopyDone(void)
{
    u32 cnt0;
    u32 cnt1;
    cnt0 = __GXReadMEMCounterU32();
    do {
        cnt1 = cnt0;
        __GXAbortWait(32);
        cnt0 = __GXReadMEMCounterU32();
    } while (cnt0 != cnt1);
}

void __GXAbort(void)
{
    if (__GXData->abtWaitPECopy && __GXIsGPFifoReady()) {
        __GXAbortWaitPECopyDone();
    }
    *(volatile u32*)0xCC003018 = 1;
    __GXAbortWait(200);
    *(volatile u32*)0xCC003018 = 0;
    __GXAbortWait(20);
}

static inline void GXFlush(void)
{
    if (__GXData->dirtyState) {
        __GXSetDirtyState();
    }
    GXWGFifo.u32 = 0; GXWGFifo.u32 = 0; GXWGFifo.u32 = 0; GXWGFifo.u32 = 0;
    GXWGFifo.u32 = 0; GXWGFifo.u32 = 0; GXWGFifo.u32 = 0; GXWGFifo.u32 = 0;
    PPCSync();
}

void GXAbortFrame(void)
{
    __GXAbort();
    if (__GXIsGPFifoReady()) {
        __GXCleanGPFifo();
        __GXInitRevisionBits();
        __GXData->dirtyState = 0;
        GXFlush();
    }
}

static inline void GXSetDrawDone(void)
{
    BOOL enabled = OSDisableInterrupts();
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0x45000002;
    GXFlush();
    DrawDone = 0;
    OSRestoreInterrupts(enabled);
}

static inline void GXWaitDrawDone(void)
{
    BOOL enabled = OSDisableInterrupts();
    while (!DrawDone) {
        OSSleepThread(&FinishQueue);
    }
    OSRestoreInterrupts(enabled);
}

void GXDrawDone(void)
{
    GXSetDrawDone();
    __GXData->xfWritePending = 0;
    GXWaitDrawDone();
}

void GXPixModeSync(void)
{
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->pixelCtrl.word;
    __GXData->xfWritePending = 0;
}

void GXPokeAlphaMode(s32 func, u8 threshold)
{
    __peReg[3] = (func << 8) | threshold;
}

void GXPokeAlphaRead(u32 mode)
{
    u32 reg = 0;
    GX_SET_REG(reg, mode, 30, 31);
    reg |= 4;
    __peReg[4] = reg;
}

void GXPokeAlphaUpdate(u8 update_enable)
{
    u32 reg = __peReg[1];
    GX_SET_REG(reg, update_enable, 27, 27);
    __peReg[1] = reg;
}

void GXPokeBlendMode(s32 type, u32 src_factor, u32 dst_factor, u32 op)
{
    u32 reg = __peReg[1];
    GX_SET_REG(reg, (type == 1) || (type == 3), 31, 31);
    GX_SET_REG(reg, (type == 3), 20, 20);
    GX_SET_REG(reg, (type == 2), 30, 30);
    GX_SET_REG(reg, op, 16, 19);
    GX_SET_REG(reg, src_factor, 21, 23);
    GX_SET_REG(reg, dst_factor, 24, 26);
    GX_SET_REG(reg, 0x41, 0, 7);
    __peReg[1] = reg;
}

void GXPokeColorUpdate(u8 update_enable)
{
    u32 reg = __peReg[1];
    GX_SET_REG(reg, update_enable, 28, 28);
    __peReg[1] = reg;
}

void GXPokeDstAlpha(u8 enable, u8 alpha)
{
    u32 reg = 0;
    GX_SET_REG(reg, alpha, 24, 31);
    GX_SET_REG(reg, enable, 23, 23);
    __peReg[2] = reg;
}

void GXPokeDither(u8 dither)
{
    u32 reg = __peReg[1];
    GX_SET_REG(reg, dither, 29, 29);
    __peReg[1] = reg;
}

void GXPokeZMode(u8 compare_enable, u32 func, u8 update_enable)
{
    u32 reg = 0;
    GX_SET_REG(reg, compare_enable, 31, 31);
    GX_SET_REG(reg, func, 28, 30);
    GX_SET_REG(reg, update_enable, 27, 27);
    __peReg[0] = reg;
}

void GXPeekARGB(u16 x, u16 y, u32* color)
{
    u32 addr = 0xC8000000;
    GX_SET_REG(addr, x, 20, 29);
    GX_SET_REG(addr, y, 10, 19);
    *color = *(volatile u32*)addr;
}

void GXPokeARGB(u16 x, u16 y, u32 color)
{
    u32 addr = 0xC8000000;
    GX_SET_REG(addr, x, 20, 29);
    GX_SET_REG(addr, y, 10, 19);
    *(volatile u32*)addr = color;
}

GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback cb)
{
    GXDrawSyncCallback old = TokenCB;
    BOOL enabled = OSDisableInterrupts();
    TokenCB = cb;
    OSRestoreInterrupts(enabled);
    return old;
}

static void GXTokenInterruptHandler(s16 interrupt, OSContext* context)
{
    u16 token;
    OSContext exceptionContext;
    token = __peReg[7];
    if (TokenCB) {
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(&exceptionContext);
        TokenCB(token);
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(context);
    }
    __peReg[5] |= 4;
}

GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback cb)
{
    GXDrawDoneCallback old = DrawDoneCB;
    BOOL enabled = OSDisableInterrupts();
    DrawDoneCB = cb;
    OSRestoreInterrupts(enabled);
    return old;
}

static void GXFinishInterruptHandler(s16 interrupt, OSContext* context)
{
    OSContext exceptionContext;
    __peReg[5] |= 8;
    DrawDone = 1;
    if (DrawDoneCB) {
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(&exceptionContext);
        DrawDoneCB();
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(context);
    }
    OSWakeupThread(&FinishQueue);
}

void __GXPEInit(void)
{
    __OSSetInterruptHandler(18, GXTokenInterruptHandler);
    __OSSetInterruptHandler(19, GXFinishInterruptHandler);
    OSInitThreadQueue(&FinishQueue);
    __OSUnmaskInterrupts(0x2000);
    __OSUnmaskInterrupts(0x1000);
    __peReg[5] |= 0xF;
}
