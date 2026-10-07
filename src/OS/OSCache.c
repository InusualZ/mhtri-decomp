/*
 * OS/OSCache.c - the OS cache control: data/instruction cache and locked-cache ranges, the DMA error handler and
 *    `__OSCacheInit`.
 * RANGE. .text 0x804CC5F0-0x804CCD40 (21 functions); .data 0x8061C158-0x8061C390.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: .data 0x8061C158..0x8061C390 holds the "L2 INVALIDATE" and DMA error
 *    strings read only by `DMAErrorHandler` and `__OSCacheInit`; the range is the DC/IC/LC accessors around them.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names throughout; GUESS: `LCLoadBlocks`, `LCLoadData`, `LCQueueLength` (the dump names both block-DMA entries
 *    `LCStoreBlocks` and both data entries `LCStoreData`; the first of each sets the DMA load bit, the second does not);
 *    GUESS: `L2Disable`, `L2GlobalInvalidate`, `L2Init`, `L2Enable` (inlined statics: their
 *    bodies are the register sequences inside `__OSCacheInit`, and the "L2 INVALIDATE" string is emitted ahead of the DMA
 *    strings, so a static owner of it precedes `DMAErrorHandler`).
 * RESIDUALS. the unit's .text is 168 bytes longer than the target: `L2Disable` and `L2GlobalInvalidate` are emitted as
 *    functions here; the target object has only their strings (`static inline` drops the code but puts the "L2 INVALIDATE"
 *    string after the DMA strings, moving every pool offset in the two C bodies).
 * SHAPES. the cache-line loops and the locked-cache mapping are asm functions with `nofralloc` (playbook 104); the HID2 and
 *    DMA SPRs are written by number (this compiler's `HID2` name is a different register).
 */

#include "types.h"

#include "DB/DBPrintf.h"
#include "OS/DCInvalidateRange.h"
#include "OS/LCEnable.h"
#include "OS/OSCache.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/PPCArch.h"
#include "OS/PPCHalt.h"

#define LC_BLOCK_BYTES 4096 /* bytes one locked-cache DMA moves: 128 blocks of 32 */

#define L2CR_ENABLE 0x80000000
#define L2CR_GLOBAL_INVALIDATE 0x00200000
#define HID0_ICE 0x8000
#define HID0_DCE 0x4000
#define MSR_TRANSLATION 0x30

void __LCEnable(void);
void DMAErrorHandler(u8 error, OSContext* context, ...);

/* Turns the L2 cache off. */
static void L2Disable(void)
{
    asm { sync }
    PPCMtl2cr(PPCMfl2cr() & ~L2CR_ENABLE);
    asm { sync }
}

/* Invalidates the whole L2 cache and waits for the invalidate to finish. */
static void L2GlobalInvalidate(void)
{
    L2Disable();
    PPCMtl2cr(PPCMfl2cr() | L2CR_GLOBAL_INVALIDATE);
    while (PPCMfl2cr() & 1) {
    }
    PPCMtl2cr(PPCMfl2cr() & ~L2CR_GLOBAL_INVALIDATE);
    while (PPCMfl2cr() & 1) {
        DBPrintf(">>> L2 INVALIDATE : SHOULD NEVER HAPPEN\n");
    }
}


/* Switches the data cache on. */
asm void DCEnable(void)
{
    nofralloc
    sync
    mfspr r3, HID0
    ori r3, r3, 16384
    mtspr HID0, r3
    blr
}

/* Invalidates the data-cache lines covering the range. */
/* untyped: byte range */
asm void DCInvalidateRange(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_3c:
    dcbi 0, r3
    addi r3, r3, 32
    bdnz loop_3c
    blr
}

/* Writes back and invalidates the data-cache lines of the range, then waits. */
/* untyped: byte range */
asm void DCFlushRange(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_6c:
    dcbf 0, r3
    addi r3, r3, 32
    bdnz loop_6c
    sc
    blr
}

/* Writes back the data-cache lines of the range, then waits. */
/* untyped: byte range */
asm void DCStoreRange(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_9c:
    dcbst 0, r3
    addi r3, r3, 32
    bdnz loop_9c
    sc
    blr
}

/* Writes back and invalidates the data-cache lines of the range without waiting. */
/* untyped: byte range */
asm void DCFlushRangeNoSync(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_cc:
    dcbf 0, r3
    addi r3, r3, 32
    bdnz loop_cc
    blr
}

/* Writes back the data-cache lines of the range without waiting. */
/* untyped: byte range */
asm void DCStoreRangeNoSync(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_fc:
    dcbst 0, r3
    addi r3, r3, 32
    bdnz loop_fc
    blr
}

/* Zeroes the data-cache lines covering the range. */
/* untyped: byte range */
asm void DCZeroRange(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_12c:
    dcbz 0, r3
    addi r3, r3, 32
    bdnz loop_12c
    blr
}

/* Invalidates the instruction-cache lines covering the range. */
/* untyped: byte range */
asm void ICInvalidateRange(void* start, u32 nBytes)
{
    nofralloc
    cmplwi r4, 0
    blelr
    clrlwi r5, r3, 27
    add r4, r4, r5
    addi r4, r4, 31
    srwi r4, r4, 5
    mtctr r4
loop_15c:
    icbi 0, r3
    addi r3, r3, 32
    bdnz loop_15c
    sync
    isync
    blr
}

/* Invalidates the whole instruction cache. */
asm void ICFlashInvalidate(void)
{
    nofralloc
    mfspr r3, HID0
    ori r3, r3, 2048
    mtspr HID0, r3
    blr
}

/* Switches the instruction cache on. */
asm void ICEnable(void)
{
    nofralloc
    isync
    mfspr r3, HID0
    ori r3, r3, 32768
    mtspr HID0, r3
    blr
}

/* Maps the locked cache: loads the DBAT, sets HID2 and zeroes the 16 KB. */
asm void __LCEnable(void)
{
    nofralloc
    mfmsr r5
    ori r5, r5, 4096
    mtmsr r5
    lis r3, -32768
    li r4, 1024
    mtctr r4
loop_1c8:
    dcbt 0, r3
    dcbst 0, r3
    addi r3, r3, 32
    bdnz loop_1c8
    mfspr r4, 920
    oris r4, r4, 4111
    mtspr 920, r4
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    lis r3, -8192
    ori r3, r3, 2
    mtdbatl 3, r3
    ori r3, r3, 510
    mtdbatu 3, r3
    isync
    lis r3, -8192
    li r6, 512
    mtctr r6
    li r6, 0
loop_23c:
    dcbz_l r6, r3
    addi r3, r3, 32
    bdnz loop_23c
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

/* Maps the locked cache with interrupts off. */
void LCEnable(void)
{
    BOOL enabled = OSDisableInterrupts();

    __LCEnable();
    OSRestoreInterrupts(enabled);
}

/* Invalidates the locked cache lines and turns it off. */
asm void LCDisable(void)
{
    nofralloc
    lis r3, -8192
    li r4, 512
    mtctr r4
loop_2cc:
    dcbi 0, r3
    addi r3, r3, 32
    bdnz loop_2cc
    mfspr r4, 920
    rlwinm r4, r4, 0, 4, 2
    mtspr 920, r4
    blr
}

/* Queues a DMA of `blocks` 32-byte blocks into the locked cache. */
/* untyped: byte range */
asm void LCLoadBlocks(void* dst, void* src, u32 blocks)
{
    nofralloc
    rlwinm r6, r5, 30, 27, 31
    clrlwi r4, r4, 3
    or r6, r6, r4
    mtspr 922, r6
    rlwinm r6, r5, 2, 28, 29
    or r6, r6, r3
    ori r6, r6, 18
    mtspr 923, r6
    blr
}

/* Queues a DMA of `blocks` 32-byte blocks out of the locked cache. */
/* untyped: byte range */
asm void LCStoreBlocks(void* dst, void* src, u32 blocks)
{
    nofralloc
    rlwinm r6, r5, 30, 27, 31
    clrlwi r3, r3, 3
    or r6, r6, r3
    mtspr 922, r6
    rlwinm r6, r5, 2, 28, 29
    or r6, r6, r4
    ori r6, r6, 2
    mtspr 923, r6
    blr
}

/* Queues the DMAs that move `nBytes` into the locked cache and returns how many were needed. */
/* untyped: byte range */
u32 LCLoadData(void* dst, void* src, u32 nBytes)
{
    u32 nBlocks = (nBytes + 31) / 32;
    u32 nTransactions = (nBlocks + 127) / 128;

    while (nBlocks != 0) {
        if (nBlocks < 128) {
            LCLoadBlocks(dst, src, nBlocks);
            nBlocks = 0;
        } else {
            LCLoadBlocks(dst, src, 0);
            nBlocks -= 128;
            dst = (char*)dst + LC_BLOCK_BYTES;
            src = (char*)src + LC_BLOCK_BYTES;
        }
    }
    return nTransactions;
}

/* Queues the DMAs that move `nBytes` out of the locked cache and returns how many were needed. */
/* untyped: byte range */
u32 LCStoreData(void* dst, void* src, u32 nBytes)
{
    u32 nBlocks = (nBytes + 31) / 32;
    u32 nTransactions = (nBlocks + 127) / 128;

    while (nBlocks != 0) {
        if (nBlocks < 128) {
            LCStoreBlocks(dst, src, nBlocks);
            nBlocks = 0;
        } else {
            LCStoreBlocks(dst, src, 0);
            nBlocks -= 128;
            dst = (char*)dst + LC_BLOCK_BYTES;
            src = (char*)src + LC_BLOCK_BYTES;
        }
    }
    return nTransactions;
}

/* Returns the number of queued locked-cache DMAs. */
asm u32 LCQueueLength(void)
{
    nofralloc
    mfspr r4, 920
    rlwinm r3, r4, 8, 28, 31
    blr
}

/* Spins until at most `length` locked-cache DMAs are queued. */
asm void LCQueueWait(u32 length)
{
    nofralloc
    mfspr r4, 920
    rlwinm r4, r4, 8, 28, 31
    cmpw r4, r3
    bgt LCQueueWait
    blr
}

/* Reports a locked-cache DMA machine check, clears its HID2 error bits, and halts when it is some other machine check. */
void DMAErrorHandler(u8 error, OSContext* context, ...)
{
    u32 hid2 = PPCMfhid2();

    OSReport("Machine check received\n");
    OSReport("HID2 = 0x%x   SRR1 = 0x%x\n", hid2, context->srr1);
    if (!(hid2 & 0x00F00000) || !(context->srr1 & 0x00200000)) {
        OSReport("Machine check was not DMA/locked cache related\n");
        OSDumpContext(context);
        PPCHalt();
    }
    OSReport("DMAErrorHandler(): An error occurred while processing DMA.\n");
    OSReport("The following errors have been detected and cleared :\n");
    if (hid2 & 0x00800000) {
        OSReport("\t- Requested a locked cache tag that was already in the cache\n");
    }
    if (hid2 & 0x00400000) {
        OSReport("\t- DMA attempted to access normal cache\n");
    }
    if (hid2 & 0x00200000) {
        OSReport("\t- DMA missed in data cache\n");
    }
    if (hid2 & 0x00100000) {
        OSReport("\t- DMA queue overflowed\n");
    }
    PPCMthid2(hid2);
}

/* Brings the L1 instruction and data caches and the L2 cache up when they are off and installs the DMA error handler. */
void __OSCacheInit(void)
{
    u32 msr;

    if (!(PPCMfhid0() & HID0_ICE)) {
        ICEnable();
        DBPrintf("L1 i-caches initialized\n");
    }
    if (!(PPCMfhid0() & HID0_DCE)) {
        DCEnable();
        DBPrintf("L1 d-caches initialized\n");
    }
    if (!(PPCMfl2cr() & L2CR_ENABLE)) {
        msr = PPCMfmsr();
        asm { sync }
        PPCMtmsr(MSR_TRANSLATION);
        asm { sync }
        L2Disable();
        L2GlobalInvalidate();
        PPCMtmsr(msr);
        PPCMtl2cr((PPCMfl2cr() | L2CR_ENABLE) & ~L2CR_GLOBAL_INVALIDATE);
        DBPrintf("L2 cache initialized\n");
    }
    OSSetErrorHandler(1, DMAErrorHandler);
    DBPrintf("Locked cache machine check handler installed\n");
}
