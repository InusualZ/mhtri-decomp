/*
 * AI_SDK/ai.c - the Revolution SDK AI (audio interface) library: DMA init/start/stop, the DMA byte counter,
 *    `AIInit` and its interrupt handler with the callback stack switch.
 *
 * RANGE. .text 0x8046D420..0x8046D9F0 (12 functions in the map, 0x5D0 B); .data 0x8060F820..0x8060F868; .sdata
 *    0x80793D00..0x80793D08; .sbss 0x80794E88..0x80794EC8.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): the run is 16-aligned (`-O4,p` function
 *    alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. directory `AI_SDK` because `src/ai/` already holds the game's monster AI and the paths collide on a case-
 *    insensitive checkout; file name `ai` is the library's. `AIRegisterDMACallback` is the dump's name (map row
 *    renamed from fn_8046D420); `AIGetDMAStartAddr` (fn_8046D540: the start address read back from the two DMA address
 *    registers) and `__AI_SRC_INIT` (fn_8046D820: the sample-counter timing loop `AIInit` runs once) are GUESS, as are
 *    the `gAI*` variables (`gAIWindow*` hold the five calibration windows in ticks, named for their nanosecond values;
 *    map rows renamed from lbl_, the 4 B halves merged into 8 B rows) and the register block types.
 * EVIDENCE. its text is a 16-aligned run inside the 4-byte-packed TRK band (0x8046D420 starts after
 *    `GetUseSerialIO`); `.data` 0x8060F820 is the build string `<< RVL_SDK - AI ...` read through `.sdata`
 *    0x80793D00 by `AIInit`; `.sbss` 0x80794E88..0x80794EC8 are read only by `AIInit`, `__AIDHandler`,
 *    `__AICallbackStackSwitch` and the first function.
 * RESIDUALS. `__AI_SRC_INIT` 99.0: the 64-bit window compares number their scratch registers differently (r12/r0 against
 *    r8/r5 around the `diff` temporaries); `.data` is 69 B against the target's 72 B range (its trailing pad is dtk's `gap`).
 * SHAPES. the DMA registers are 16-bit read-modify-writes at 0xCC005030..; the interrupt handler runs the user callback
 *    on a private stack through `__AICallbackStackSwitch`.
 */
#include "AI_SDK/ai.h"
#include "OS/OS.h"
#include "OS/OSContext.h"
#include "OS/OSInterrupt.h"
#include "OS/OSTime.h"

/* size: 0xC - the AI DMA register block at 0xCC005030 (halfwords at +0, +2, +6 and +0xA are used). */
typedef struct AIDMARegs {
    /* +0x0 */ volatile u16 start_high;     /* address bits 16..28 */
    /* +0x2 */ volatile u16 start_low;      /* address bits 5..15 */
    /* +0x4 */ volatile u16 pad_0x04;
    /* +0x6 */ volatile u16 control;        /* bit 15 starts the transfer, bits 0..14 the length in 32 B blocks */
    /* +0x8 */ volatile u16 pad_0x08;
    /* +0xA */ volatile u16 blocks_left;    /* blocks still to transfer */
} AIDMARegs; /* size: 0xC */

#define AI_DMA_REGS ((AIDMARegs*)0xCC005030)
#define AI_DSP_CSR ((volatile u16*)0xCC00500A)
#define AI_REGS ((volatile u32*)0xCD006C00)

/* size: 0x8 - the pointer to the library's build string (the second word is never read). */
typedef struct AIVersionSlot {
    /* +0x0 */ const char* text;
    /* +0x4 */ u32 unused_0x04;
} AIVersionSlot; /* size: 0x8 */

/* size: 0x8 - the slot AIRegisterDMACallback swaps (the second word is never read). */
typedef struct AICallbackSlot {
    /* +0x0 */ AIDMACallback callback;
    /* +0x4 */ u32 unused_0x04;
} AICallbackSlot; /* size: 0x8 */

AICallbackSlot gAIDMACallback;
u8* gAICallbackStack;
u8* gAIOldStack;
s64 gAIWindow31524;
s64 gAIWindow42024;
s64 gAIWindow42000;
s64 gAIWindow63000;
s64 gAIWindow3000;
s32 gAIInCallback;
s32 gAIInitFlag;
AIVersionSlot gAIVersion = { "<< RVL_SDK - AI 	release build: Feb 27 2009 10:01:30 (0x4302_145) >>", 0 };

AIDMACallback AIRegisterDMACallback(AIDMACallback callback)
{
    AIDMACallback old = gAIDMACallback.callback;
    BOOL level = OSDisableInterrupts();

    gAIDMACallback.callback = callback;
    OSRestoreInterrupts(level);
    return old;
}

void AIInitDMA(u32 address, u32 length)
{
    BOOL level = OSDisableInterrupts();

    AI_DMA_REGS->start_high = (AI_DMA_REGS->start_high & ~0x1FFF) | (address >> 16);
    AI_DMA_REGS->start_low = (AI_DMA_REGS->start_low & ~0xFFE0) | (address & 0xFFFF);
    AI_DMA_REGS->control = (AI_DMA_REGS->control & ~0x7FFF) | ((length >> 5) & 0xFFFF);
    OSRestoreInterrupts(level);
}

void AIStartDMA(void)
{
    AI_DMA_REGS->control |= 0x8000;
}

void AIStopDMA(void)
{
    AI_DMA_REGS->control &= 0x7FFF;
}

u32 AIGetDMABytesLeft(void)
{
    return (AI_DMA_REGS->blocks_left & 0x7FFF) << 5;
}

u32 AIGetDMAStartAddr(void)
{
    return ((AI_DMA_REGS->start_high & 0x1FFF) << 16) | (AI_DMA_REGS->start_low & 0xFFE0);
}

u32 __ARGetInterruptStatus(void)
{
    return (AI_DMA_REGS->control & 0x7FFF) << 5;
}

s32 AICheckInit(void)
{
    return gAIInitFlag;
}

void AIInit(u8* stack)
{
    u32 ticks;
    BOOL level;

    if (gAIInitFlag == 1) {
        return;
    }
    OSRegisterVersion(gAIVersion.text);
    ticks = (*(u32*)0x800000F8 >> 2) / 125000;
    gAIWindow31524 = (ticks * 31524) / 8000;
    gAIWindow42024 = (ticks * 42024) / 8000;
    gAIWindow42000 = (ticks * 42000) / 8000;
    gAIWindow63000 = (ticks * 63000) / 8000;
    gAIWindow3000 = (ticks * 3000) / 8000;
    AI_REGS[0] = AI_REGS[0] & ~0x15;
    AI_REGS[1] = 0;
    AI_REGS[3] = 0;
    AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
    if ((((AI_REGS[0] >> 6) & 1) ^ 1) != 0) {
        AI_REGS[0] = AI_REGS[0] & ~0x40;
        level = OSDisableInterrupts();
        __AI_SRC_INIT();
        AI_REGS[0] = AI_REGS[0] | 0x40;
        OSRestoreInterrupts(level);
    }
    gAIDMACallback.callback = NULL;
    gAICallbackStack = stack;
    __OSSetInterruptHandler(5, __AIDHandler);
    __OSUnmaskInterrupts(0x04000000);
    gAIInitFlag = 1;
}

void __AIDHandler(s16 interrupt, OSContext* context)
{
    OSContext scratch;

    *AI_DSP_CSR = (*AI_DSP_CSR & ~0xA0) | 0x08;
    OSClearContext(&scratch);
    OSSetCurrentContext(&scratch);
    if (gAIDMACallback.callback != NULL && gAIInCallback == 0) {
        AIDMACallback callback = gAIDMACallback.callback;
        u8* stack = gAICallbackStack;

        gAIInCallback = 1;
        if (stack != NULL) {
            __AICallbackStackSwitch(callback);
        } else {
            callback();
        }
        gAIInCallback = 0;
    }
    OSClearContext(&scratch);
    OSSetCurrentContext(context);
}

asm void __AICallbackStackSwitch(AIDMACallback callback)
{
    fralloc
    mr r31, r3
    lis r5, gAIOldStack@ha
    addi r5, r5, gAIOldStack@l
    stw r1, 0(r5)
    lis r5, gAICallbackStack@ha
    addi r5, r5, gAICallbackStack@l
    lwz r1, 0(r5)
    addi r1, r1, -8
    mtlr r31
    blrl
    lis r5, gAIOldStack@ha
    addi r5, r5, gAIOldStack@l
    lwz r1, 0(r5)
    frfree
    blr
}

/* Spin-waits until the sample counter ticks over. */
#define AI_WAIT_SAMPLE_EDGE()                                           do {                                                                    u32 sample = AI_REGS[2] & 0x7FFFFFFF;                               asm { nop }                                                         while (sample == (AI_REGS[2] & 0x7FFFFFFF)) {                       }                                                               } while (0)

void __AI_SRC_INIT(void)
{
    s64 target;
    s64 end = 0;
    s32 done = 0;
    s64 adjust = 0;
    s64 start;
    s64 diff;

    while (done == 0) {
        AI_REGS[0] = (AI_REGS[0] & ~0x20) | 0x20;
        AI_REGS[0] = AI_REGS[0] & ~0x2;
        AI_REGS[0] = (AI_REGS[0] & ~0x1) | 0x1;
        AI_WAIT_SAMPLE_EDGE();
        start = OSGetTime();
        AI_REGS[0] = (AI_REGS[0] & ~0x2) | 0x2;
        AI_REGS[0] = (AI_REGS[0] & ~0x1) | 0x1;
        AI_WAIT_SAMPLE_EDGE();
        end = OSGetTime();
        AI_REGS[0] = AI_REGS[0] & ~0x2;
        AI_REGS[0] = AI_REGS[0] & ~0x1;
        diff = end - start;
        if (diff < gAIWindow31524 - gAIWindow3000) {
            adjust = gAIWindow42000;
            done = 1;
        } else if (diff >= gAIWindow31524 + gAIWindow3000 && diff < gAIWindow42024 - gAIWindow3000) {
            adjust = gAIWindow63000;
            done = 1;
        } else {
            done = 0;
        }
    }
    target = end + adjust;
    while (OSGetTime() < target) {
    }
}
