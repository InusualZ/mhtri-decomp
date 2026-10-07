/*
 * AX/AXAux.c - the AX auxiliary effect buses: aux buffer init and quit, the A/B/C input and output getters, aux
 *    processing and the aux callbacks.
 *
 * RANGE. .text 0x8046EA90-0x8046F310 (21 functions, 0x880 B); .bss 0x806F75E0-0x806FA760; .sbss
 *    0x80794ED8-0x80794F20.  Cut from the old ARC/AX block between `AX/AXAlloc.c` (0x8046EA90) and `AX/AXCL.c`
 *    (0x8046F310).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. the `__AXGetAux*`, `__AXProcessAux` and `AXRegisterAuxACallback` names are the map's;
 *    `AXRegisterAuxBCallback` (0x8046F210), `AXRegisterAuxCCallback` (0x8046F280), `__AXAuxBufferA/B/C` (.bss 0x806F75E0)
 *    and the `__AXAux*` state words are GUESSes named from how the code uses them; the file name `AXAux.c` is a GUESS.
 * EVIDENCE. `.bss` 0x806F75E0, 0x806F87E0 (three 0x600 B frames each, aux A and B) and 0x806F99E0 (three 0x480 B frames, aux
 *    C) are addressed from one base by `__AXProcessAux`; `.sbss` 0x80794ED8..0x80794F20 are the frame indices,
 *    buffer pointers, callbacks and clear flags, first written by `__AXAuxInit`. 0x80794F38 (the mode word) is defined
 *    in `AX/AXCL.c` and read here.
 * RESIDUALS. __AXProcessAux 0x8046EDF0: the frame index loads take swapped registers (r8/r9 vs r7/r9), the C callback test
 *    reloads the pointer; register colouring only.
 * SHAPES. three frame arrays; each aux has a callback, a context word and three clear
 *    flags (one per frame).
 */

#include "types.h"

#include "AX/AXAux.h"
#include "AX/AXCL.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memset.h"

#define AX_AUX_FRAMES 3
#define AX_AUX_CHANNEL_WORDS 0x60
#define AX_AUX_FRAME_BYTES_AB 0x600
#define AX_AUX_FRAME_BYTES_C 0x480

u8 __AXAuxAClear[AX_AUX_FRAMES];
u8 __AXAuxBClear[AX_AUX_FRAMES];
u8 __AXAuxCClear[AX_AUX_FRAMES];
AXAuxCallback __AXAuxACallback;
AXAuxCallback __AXAuxBCallback;
AXAuxCallback __AXAuxCCallback;
void* __AXAuxAContext;
void* __AXAuxBContext;
void* __AXAuxCContext;
s32* __AXAuxAInput;
s32* __AXAuxAOutput;
s32* __AXAuxBInput;
s32* __AXAuxBOutput;
s32* __AXAuxCInput;
s32* __AXAuxCOutput;
u32 __AXAuxFrameIn;
u32 __AXAuxFrameOut;
u32 __AXAuxFrameProc;
/* size: 0xD80; three frames of three channels. */
s32 __AXAuxBufferC[AX_AUX_FRAMES][3][AX_AUX_CHANNEL_WORDS];
/* size: 0x1200; three frames of four channels. */
s32 __AXAuxBufferB[AX_AUX_FRAMES][4][AX_AUX_CHANNEL_WORDS];
/* size: 0x1200; three frames of four channels. */
s32 __AXAuxBufferA[AX_AUX_FRAMES][4][AX_AUX_CHANNEL_WORDS];

/* 0x8046EA90 (0x114): clears the aux frames and drops the callbacks. */
void __AXAuxInit(void)
{
    s32 i;
    s32* a;
    s32* b;
    s32* c;

    __AXAuxCCallback = NULL;
    a = &__AXAuxBufferA[0][0][0];
    b = &__AXAuxBufferB[0][0][0];
    __AXAuxBCallback = NULL;
    c = &__AXAuxBufferC[0][0][0];
    __AXAuxACallback = NULL;
    __AXAuxCContext = NULL;
    __AXAuxBContext = NULL;
    __AXAuxAContext = NULL;
    __AXAuxFrameIn = 0;
    __AXAuxFrameOut = 1;
    __AXAuxFrameProc = 2;
    i = 288;
    while (i--) {
        *a++ = 0;
        *b++ = 0;
        *c++ = 0;
    }
    memset(__AXAuxAClear, 0, AX_AUX_FRAMES);
    memset(__AXAuxBClear, 0, AX_AUX_FRAMES);
    memset(__AXAuxCClear, 0, AX_AUX_FRAMES);
}

/* 0x8046EBB0 (0x14): drops the callbacks. */
void __AXAuxQuit(void)
{
    __AXAuxCCallback = NULL;
    __AXAuxBCallback = NULL;
    __AXAuxACallback = NULL;
}

/* 0x8046EBD0 (0x34): returns the aux A input frame, or 0 while no callback is registered. */
void __AXGetAuxAInput(u32* address)
{
    if (__AXAuxACallback != NULL) {
        *address = (u32)__AXAuxBufferA[__AXAuxFrameIn];
    } else {
        *address = 0;
    }
}

/* 0x8046EC10 (0x1C): returns the aux A output frame. */
void __AXGetAuxAOutput(u32* address)
{
    *address = (u32)__AXAuxBufferA[__AXAuxFrameOut];
}

/* 0x8046EC30 (0x20): returns the surround channel of the aux A input frame. */
void __AXGetAuxAInputDpl2(u32* address)
{
    *address = (u32)__AXAuxBufferA[__AXAuxFrameIn][3];
}

/* 0x8046EC50 (0x20): returns the right channel of the aux A output frame. */
void __AXGetAuxAOutputDpl2R(u32* address)
{
    *address = (u32)__AXAuxBufferA[__AXAuxFrameOut][1];
}

/* 0x8046EC70 (0x20): returns the left surround channel of the aux A output frame. */
void __AXGetAuxAOutputDpl2Ls(u32* address)
{
    *address = (u32)__AXAuxBufferA[__AXAuxFrameOut][2];
}

/* 0x8046EC90 (0x20): returns the right surround channel of the aux A output frame. */
void __AXGetAuxAOutputDpl2Rs(u32* address)
{
    *address = (u32)__AXAuxBufferA[__AXAuxFrameOut][3];
}

/* 0x8046ECB0 (0x34): returns the aux B input frame, or 0 while no callback is registered. */
void __AXGetAuxBInput(u32* address)
{
    if (__AXAuxBCallback != NULL) {
        *address = (u32)__AXAuxBufferB[__AXAuxFrameIn];
    } else {
        *address = 0;
    }
}

/* 0x8046ECF0 (0x1C): returns the aux B output frame. */
void __AXGetAuxBOutput(u32* address)
{
    *address = (u32)__AXAuxBufferB[__AXAuxFrameOut];
}

/* 0x8046ED10 (0x20): returns the surround channel of the aux B input frame. */
void __AXGetAuxBInputDpl2(u32* address)
{
    *address = (u32)__AXAuxBufferB[__AXAuxFrameIn][3];
}

/* 0x8046ED30 (0x20): returns the right channel of the aux B output frame. */
void __AXGetAuxBOutputDpl2R(u32* address)
{
    *address = (u32)__AXAuxBufferB[__AXAuxFrameOut][1];
}

/* 0x8046ED50 (0x20): returns the left surround channel of the aux B output frame. */
void __AXGetAuxBOutputDpl2Ls(u32* address)
{
    *address = (u32)__AXAuxBufferB[__AXAuxFrameOut][2];
}

/* 0x8046ED70 (0x20): returns the right surround channel of the aux B output frame. */
void __AXGetAuxBOutputDpl2Rs(u32* address)
{
    *address = (u32)__AXAuxBufferB[__AXAuxFrameOut][3];
}

/* 0x8046ED90 (0x34): returns the aux C input frame, or 0 while no callback is registered. */
void __AXGetAuxCInput(u32* address)
{
    if (__AXAuxCCallback != NULL) {
        *address = (u32)__AXAuxBufferC[__AXAuxFrameIn];
    } else {
        *address = 0;
    }
}

/* 0x8046EDD0 (0x1C): returns the aux C output frame. */
void __AXGetAuxCOutput(u32* address)
{
    *address = (u32)__AXAuxBufferC[__AXAuxFrameOut];
}

/* 0x8046EDF0 (0x3AC): runs the aux callbacks on the current frame and advances the frame indices. */
void __AXProcessAux(void)
{
    __AXAuxAInput = __AXAuxBufferA[__AXAuxFrameIn][0];
    __AXAuxAOutput = __AXAuxBufferA[__AXAuxFrameOut][0];
    __AXAuxBInput = __AXAuxBufferB[__AXAuxFrameIn][0];
    __AXAuxBOutput = __AXAuxBufferB[__AXAuxFrameOut][0];
    __AXAuxCInput = __AXAuxBufferC[__AXAuxFrameIn][0];
    __AXAuxCOutput = __AXAuxBufferC[__AXAuxFrameOut][0];
    if (__AXAuxACallback != NULL) {
        if (__AXOutputMode == 2) {
            s32* buffers[4];

            buffers[0] = __AXAuxBufferA[__AXAuxFrameProc][0];
            buffers[1] = __AXAuxBufferA[__AXAuxFrameProc][1];
            buffers[2] = __AXAuxBufferA[__AXAuxFrameProc][2];
            buffers[3] = __AXAuxBufferA[__AXAuxFrameProc][3];
            DCInvalidateRange(buffers[0], AX_AUX_FRAME_BYTES_AB);
            __AXAuxACallback(buffers, __AXAuxAContext);
            DCFlushRangeNoSync(buffers[0], AX_AUX_FRAME_BYTES_AB);
        } else {
            s32* buffers[3];

            buffers[0] = __AXAuxBufferA[__AXAuxFrameProc][0];
            buffers[1] = __AXAuxBufferA[__AXAuxFrameProc][1];
            buffers[2] = __AXAuxBufferA[__AXAuxFrameProc][2];
            DCInvalidateRange(buffers[0], AX_AUX_FRAME_BYTES_C);
            __AXAuxACallback(buffers, __AXAuxAContext);
            DCFlushRangeNoSync(buffers[0], AX_AUX_FRAME_BYTES_C);
        }
    } else if (__AXAuxAClear[__AXAuxFrameProc] != 0) {
        s32* frame = __AXAuxBufferA[__AXAuxFrameProc][0];

        memset(frame, 0, AX_AUX_FRAME_BYTES_AB);
        DCFlushRange(frame, AX_AUX_FRAME_BYTES_AB);
        __AXAuxAClear[__AXAuxFrameProc] = 0;
    }
    if (__AXAuxBCallback != NULL) {
        if (__AXOutputMode == 2) {
            s32* buffers[4];

            buffers[0] = __AXAuxBufferB[__AXAuxFrameProc][0];
            buffers[1] = __AXAuxBufferB[__AXAuxFrameProc][1];
            buffers[2] = __AXAuxBufferB[__AXAuxFrameProc][2];
            buffers[3] = __AXAuxBufferB[__AXAuxFrameProc][3];
            DCInvalidateRange(buffers[0], AX_AUX_FRAME_BYTES_AB);
            __AXAuxBCallback(buffers, __AXAuxBContext);
            DCFlushRangeNoSync(buffers[0], AX_AUX_FRAME_BYTES_AB);
        } else {
            s32* buffers[3];

            buffers[0] = __AXAuxBufferB[__AXAuxFrameProc][0];
            buffers[1] = __AXAuxBufferB[__AXAuxFrameProc][1];
            buffers[2] = __AXAuxBufferB[__AXAuxFrameProc][2];
            DCInvalidateRange(buffers[0], AX_AUX_FRAME_BYTES_C);
            __AXAuxBCallback(buffers, __AXAuxBContext);
            DCFlushRangeNoSync(buffers[0], AX_AUX_FRAME_BYTES_C);
        }
    } else if (__AXAuxBClear[__AXAuxFrameProc] != 0) {
        s32* frame = __AXAuxBufferB[__AXAuxFrameProc][0];

        memset(frame, 0, AX_AUX_FRAME_BYTES_AB);
        DCFlushRange(frame, AX_AUX_FRAME_BYTES_AB);
        __AXAuxBClear[__AXAuxFrameProc] = 0;
    }
    if (__AXAuxCCallback != NULL && __AXOutputMode != 2) {
        s32* buffers[3];

        buffers[0] = __AXAuxBufferC[__AXAuxFrameProc][0];
        buffers[1] = __AXAuxBufferC[__AXAuxFrameProc][1];
        buffers[2] = __AXAuxBufferC[__AXAuxFrameProc][2];
        DCInvalidateRange(buffers[0], AX_AUX_FRAME_BYTES_C);
        __AXAuxCCallback(buffers, __AXAuxCContext);
        DCFlushRangeNoSync(buffers[0], AX_AUX_FRAME_BYTES_C);
    } else if (__AXAuxCCallback == NULL && __AXAuxCClear[__AXAuxFrameProc] != 0) {
        s32* frame = __AXAuxBufferC[__AXAuxFrameProc][0];

        memset(frame, 0, AX_AUX_FRAME_BYTES_C);
        DCFlushRange(frame, AX_AUX_FRAME_BYTES_C);
        __AXAuxCClear[__AXAuxFrameProc] = 0;
    }
    __AXAuxFrameIn = (__AXAuxFrameIn + 1) % AX_AUX_FRAMES;
    __AXAuxFrameOut = (__AXAuxFrameOut + 1) % AX_AUX_FRAMES;
    __AXAuxFrameProc = (__AXAuxFrameProc + 1) % AX_AUX_FRAMES;
}

/* 0x8046F1A0 (0x64): installs the aux A callback; clearing it marks all three aux A frames for a clear. */
/* untyped: caller-owned context word handed back to the callback */
void AXRegisterAuxACallback(AXAuxCallback callback, void* context)
{
    BOOL level = OSDisableInterrupts();

    __AXAuxACallback = callback;
    __AXAuxAContext = context;
    if (callback == NULL) {
        memset(__AXAuxAClear, 1, AX_AUX_FRAMES);
    }
    OSRestoreInterrupts(level);
}

/* 0x8046F210 (0x64): installs the aux B callback; clearing it marks all three aux B frames for a clear. */
/* untyped: caller-owned context word handed back to the callback */
void AXRegisterAuxBCallback(AXAuxCallback callback, void* context)
{
    BOOL level = OSDisableInterrupts();

    __AXAuxBCallback = callback;
    __AXAuxBContext = context;
    if (callback == NULL) {
        memset(__AXAuxBClear, 1, AX_AUX_FRAMES);
    }
    OSRestoreInterrupts(level);
}

/* 0x8046F280 (0x64): installs the aux C callback; clearing it marks all three aux C frames for a clear. */
/* untyped: caller-owned context word handed back to the callback */
void AXRegisterAuxCCallback(AXAuxCallback callback, void* context)
{
    BOOL level = OSDisableInterrupts();

    __AXAuxCCallback = callback;
    __AXAuxCContext = context;
    if (callback == NULL) {
        memset(__AXAuxCClear, 1, AX_AUX_FRAMES);
    }
    OSRestoreInterrupts(level);
}

/* 0x8046F2F0 (0x14): returns the aux A callback and its context. */
/* untyped: caller-owned context word handed back to the callback */
void AXGetAuxACallback(AXAuxCallback* callback, void** context)
{
    *callback = __AXAuxACallback;
    *context = __AXAuxAContext;
}
