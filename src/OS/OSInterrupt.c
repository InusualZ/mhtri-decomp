/*
 * OS/OSInterrupt.c - the OS interrupt layer: enable/disable/restore, the handler table, mask/unmask and the external-
 *    interrupt entry.
 * RANGE. .text 0x804D0C70-0x804D1440 (11 functions); .data 0x8061D3D0-0x8061D400; .sbss 0x80795358-0x80795370.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: `InterruptHandlerTable` (.sbss 0x80795368) and the
 *    last-interrupt record (0x80795358..0x80795364) are read by `__OSInterruptInit`, the dispatcher and
 *    `__OSUnhandledException`; the priority table .data 0x8061D3D0 by the dispatcher only; `__OSModuleInit`
 *    (0x804D1440) is not part of it.
 * FLAGS. `cflags_base` per object (configure.py): the target's function starts are all 16-aligned.
 * NAMES. map names (GUESS: `__OSGetInterruptHandler`, `__OSInterruptInit`, which the dump has as other names or placeholders);
 *    GUESS: `__OSDispatchInterrupt` (the exception-handler body that walks the priority table and calls the
 *    handler, a map `fn_` stem before), `InterruptPriorityTable` (the 12 source masks in priority order), `SetInterruptMask`
 *    and `ExternalInterruptHandler` are the map's statics.
 * RESIDUALS. every body is a raw asm transcription (nofralloc) of the target; none is reconstructed as C yet.
 * SHAPES. asm functions with `nofralloc` (playbook 104); the dispatcher and the mask writer carry the register-offset hardware
 *    accesses the source spelled as casts.
 */

#include "types.h"

#include "OS/OSContext.h"
#include "OS/OSInterrupt.h"
#include "OS/OS.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"
#include "Runtime.PPCEABI.H/memset.h"

static u32 InterruptPriorityTable[12] = {
    0x00000100, 0x00000040, 0xF8000000, 0x00000200, 0x00000080, 0x00000010,
    0x00003000, 0x00000020, 0x03FF8C00, 0x04000000, 0x00004000, 0xFFFFFFFF,
};

static __OSInterruptHandler* InterruptHandlerTable;
s64 __OSLastInterruptTime;
s16 __OSLastInterrupt;
u32 __OSLastInterruptSrr0;

static asm u32 SetInterruptMask(u32 type, u32 mask);
void __OSDispatchInterrupt(u8 exception, OSContext* context);
static asm void ExternalInterruptHandler(u8 exception, OSContext* context);

/* Clears the MSR external-interrupt enable bit and returns its previous state. */
asm BOOL OSDisableInterrupts(void)
{
    nofralloc
    mfmsr r3
    rlwinm r4, r3, 0, 17, 15
    mtmsr r4
    rlwinm r3, r3, 17, 31, 31
    blr
}

/* Sets the MSR external-interrupt enable bit and returns its previous state. */
asm BOOL OSEnableInterrupts(void)
{
    nofralloc
    mfmsr r3
    ori r4, r3, 32768
    mtmsr r4
    rlwinm r3, r3, 17, 31, 31
    blr
}

/* Sets the MSR external-interrupt enable bit to `level` and returns its previous state. */
asm BOOL OSRestoreInterrupts(BOOL level)
{
    nofralloc
    cmpwi r3, 0
    mfmsr r4
    beq L54
    ori r5, r4, 32768
    b L58
L54:
    rlwinm r5, r4, 0, 17, 15
L58:
    mtmsr r5
    rlwinm r3, r4, 17, 31, 31
    blr
}

/* Installs a handler for `interrupt` and returns the previous one. */
asm __OSInterruptHandler __OSSetInterruptHandler(s16 interrupt, __OSInterruptHandler handler)
{
    nofralloc
    lwz r5, InterruptHandlerTable(r0)
    slwi r0, r3, 2
    lwzx r3, r5, r0
    stwx r4, r5, r0
    blr
}

/* Returns the handler installed for `interrupt`. */
asm __OSInterruptHandler __OSGetInterruptHandler(s16 interrupt)
{
    nofralloc
    lwz r4, InterruptHandlerTable(r0)
    slwi r0, r3, 2
    lwzx r3, r4, r0
    blr
}

/* Clears the handler table, masks every interrupt source and installs the external-interrupt handler. */
asm void __OSInterruptInit(void)
{
    nofralloc
    stwu r1, -32(r1)
    mflr r0
    li r4, 0
    li r5, 128
    stw r0, 36(r1)
    stw r31, 28(r1)
    lis r31, -32768
    addi r3, r31, 12352
    stw r30, 24(r1)
    stw r29, 20(r1)
    stw r3, InterruptHandlerTable(r0)
    bl memset
    li r0, 0
    stw r0, 196(r31)
    lis r4, -13312
    li r5, 240
    stw r0, 200(r31)
    lis r3, -13056
    lis r0, 16384
    li r30, -16
    stw r5, 12292(r4)
    stw r0, 52(r3)
    bl OSDisableInterrupts
    lwz r0, 196(r31)
    mr r29, r3
    lwz r4, 200(r31)
    or r30, r30, r0
    nor r0, r0, r4
    stw r30, 196(r31)
    clrrwi r3, r0, 4
    or r30, r30, r4
    b L128
L120:
    mr r4, r30
    bl SetInterruptMask
L128:
    cmpwi r3, 0
    bne L120
    mr r3, r29
    bl OSRestoreInterrupts
    lis r4, ExternalInterruptHandler@ha
    li r3, 4
    addi r4, r4, ExternalInterruptHandler@l
    bl __OSSetExceptionHandler
    lwz r0, 36(r1)
    lwz r31, 28(r1)
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    mtlr r0
    addi r1, r1, 32
    blr
}

/* Writes the mask registers for the sources in `type` and returns the sources still to be set. */
static asm u32 SetInterruptMask(u32 type, u32 mask)
{
    nofralloc
    cntlzw r0, r3
    cmpwi r0, 12
    bge L19c
    cmpwi r0, 8
    beq L24c
    bge L278
    cmpwi r0, 5
    bge L20c
    cmpwi r0, 0
    bge L1bc
    blr
L19c:
    cmpwi r0, 17
    bge L1b0
    cmpwi r0, 15
    bge L300
    b L2bc
L1b0:
    cmpwi r0, 28
    bgelr
    b L334
L1bc:
    clrrwi. r0, r4, 31
    li r5, 0
    bne L1cc
    ori r5, r5, 1
L1cc:
    rlwinm. r0, r4, 0, 1, 1
    bne L1d8
    ori r5, r5, 2
L1d8:
    rlwinm. r0, r4, 0, 2, 2
    bne L1e4
    ori r5, r5, 4
L1e4:
    rlwinm. r0, r4, 0, 3, 3
    bne L1f0
    ori r5, r5, 8
L1f0:
    rlwinm. r0, r4, 0, 4, 4
    bne L1fc
    ori r5, r5, 16
L1fc:
    lis r4, -13312
    clrlwi r3, r3, 5
    sth r5, 16412(r4)
    blr
L20c:
    lis r5, -13312
    rlwinm. r0, r4, 0, 5, 5
    lhz r5, 20490(r5)
    rlwinm r5, r5, 0, 29, 22
    bne L224
    ori r5, r5, 16
L224:
    rlwinm. r0, r4, 0, 6, 6
    bne L230
    ori r5, r5, 64
L230:
    rlwinm. r0, r4, 0, 7, 7
    bne L23c
    ori r5, r5, 256
L23c:
    lis r4, -13312
    rlwinm r3, r3, 0, 8, 4
    sth r5, 20490(r4)
    blr
L24c:
    rlwinm. r0, r4, 0, 8, 8
    lis r4, -13056
    lwz r5, 27648(r4)
    li r0, -45
    and r5, r5, r0
    bne L268
    ori r5, r5, 4
L268:
    lis r4, -13056
    rlwinm r3, r3, 0, 9, 7
    stw r5, 27648(r4)
    blr
L278:
    rlwinm. r0, r4, 0, 9, 9
    lis r5, -13056
    lwz r5, 26624(r5)
    li r0, -11280
    and r5, r5, r0
    bne L294
    ori r5, r5, 1
L294:
    rlwinm. r0, r4, 0, 10, 10
    bne L2a0
    ori r5, r5, 4
L2a0:
    rlwinm. r0, r4, 0, 11, 11
    bne L2ac
    ori r5, r5, 1024
L2ac:
    lis r4, -13056
    rlwinm r3, r3, 0, 12, 8
    stw r5, 26624(r4)
    blr
L2bc:
    rlwinm. r0, r4, 0, 12, 12
    lis r5, -13056
    lwz r5, 26644(r5)
    li r0, -3088
    and r5, r5, r0
    bne L2d8
    ori r5, r5, 1
L2d8:
    rlwinm. r0, r4, 0, 13, 13
    bne L2e4
    ori r5, r5, 4
L2e4:
    rlwinm. r0, r4, 0, 14, 14
    bne L2f0
    ori r5, r5, 1024
L2f0:
    lis r4, -13056
    rlwinm r3, r3, 0, 15, 11
    stw r5, 26644(r4)
    blr
L300:
    lis r5, -13056
    rlwinm. r0, r4, 0, 15, 15
    lwz r5, 26664(r5)
    clrrwi r5, r5, 4
    bne L318
    ori r5, r5, 1
L318:
    rlwinm. r0, r4, 0, 16, 16
    bne L324
    ori r5, r5, 4
L324:
    lis r4, -13056
    rlwinm r3, r3, 0, 17, 14
    stw r5, 26664(r4)
    blr
L334:
    rlwinm. r0, r4, 0, 17, 17
    li r5, 240
    bne L344
    ori r5, r5, 2048
L344:
    rlwinm. r0, r4, 0, 20, 20
    bne L350
    ori r5, r5, 8
L350:
    rlwinm. r0, r4, 0, 21, 21
    bne L35c
    ori r5, r5, 4
L35c:
    rlwinm. r0, r4, 0, 22, 22
    bne L368
    ori r5, r5, 2
L368:
    rlwinm. r0, r4, 0, 23, 23
    bne L374
    ori r5, r5, 1
L374:
    rlwinm. r0, r4, 0, 24, 24
    bne L380
    ori r5, r5, 256
L380:
    rlwinm. r0, r4, 0, 25, 25
    bne L38c
    ori r5, r5, 4096
L38c:
    rlwinm. r0, r4, 0, 18, 18
    bne L398
    ori r5, r5, 512
L398:
    rlwinm. r0, r4, 0, 19, 19
    bne L3a4
    ori r5, r5, 1024
L3a4:
    rlwinm. r0, r4, 0, 26, 26
    bne L3b0
    ori r5, r5, 8192
L3b0:
    rlwinm. r0, r4, 0, 27, 27
    bne L3bc
    ori r5, r5, 16384
L3bc:
    lis r4, -13312
    rlwinm r3, r3, 0, 28, 16
    stw r5, 12292(r4)
    blr
}

/* Masks the given interrupt sources and returns the previous mask. */
asm u32 __OSMaskInterrupts(u32 mask)
{
    nofralloc
    stwu r1, -32(r1)
    mflr r0
    stw r0, 36(r1)
    stw r31, 28(r1)
    mr r31, r3
    stw r30, 24(r1)
    stw r29, 20(r1)
    bl OSDisableInterrupts
    lis r4, -32768
    mr r30, r3
    lwz r29, 196(r4)
    lwz r5, 200(r4)
    or r0, r29, r5
    andc r3, r31, r0
    or r31, r31, r29
    stw r31, 196(r4)
    or r31, r31, r5
    b L420
L418:
    mr r4, r31
    bl SetInterruptMask
L420:
    cmpwi r3, 0
    bne L418
    mr r3, r30
    bl OSRestoreInterrupts
    lwz r31, 28(r1)
    mr r3, r29
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    lwz r0, 36(r1)
    mtlr r0
    addi r1, r1, 32
    blr
}

/* Unmasks the given interrupt sources and returns the previous mask. */
asm u32 __OSUnmaskInterrupts(u32 mask)
{
    nofralloc
    stwu r1, -32(r1)
    mflr r0
    stw r0, 36(r1)
    stw r31, 28(r1)
    mr r31, r3
    stw r30, 24(r1)
    stw r29, 20(r1)
    bl OSDisableInterrupts
    lis r4, -32768
    mr r30, r3
    lwz r29, 196(r4)
    lwz r5, 200(r4)
    or r0, r29, r5
    and r3, r31, r0
    andc r31, r29, r31
    stw r31, 196(r4)
    or r31, r31, r5
    b L4a0
L498:
    mr r4, r31
    bl SetInterruptMask
L4a0:
    cmpwi r3, 0
    bne L498
    mr r3, r30
    bl OSRestoreInterrupts
    lwz r31, 28(r1)
    mr r3, r29
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    lwz r0, 36(r1)
    mtlr r0
    addi r1, r1, 32
    blr
}

/* Finds the highest-priority pending interrupt, runs its handler and resumes the interrupted context. */
asm void __OSDispatchInterrupt(u8 exception, OSContext* context)
{
    nofralloc
    stwu r1, -32(r1)
    mflr r0
    lis r3, -13312
    stw r0, 36(r1)
    stw r31, 28(r1)
    stw r30, 24(r1)
    mr r30, r4
    stw r29, 20(r1)
    lwz r31, 12288(r3)
    lwz r0, 12292(r3)
    rlwinm. r31, r31, 0, 16, 14
    beq L508
    and. r0, r31, r0
    bne L510
L508:
    mr r3, r30
    bl OSLoadContext
L510:
    rlwinm. r0, r31, 0, 24, 24
    li r0, 0
    beq L560
    lis r3, -13312
    lhz r4, 16414(r3)
    clrlwi. r3, r4, 31
    beq L530
    oris r0, r0, 32768
L530:
    rlwinm. r3, r4, 0, 30, 30
    beq L53c
    oris r0, r0, 16384
L53c:
    rlwinm. r3, r4, 0, 29, 29
    beq L548
    oris r0, r0, 8192
L548:
    rlwinm. r3, r4, 0, 28, 28
    beq L554
    oris r0, r0, 4096
L554:
    rlwinm. r3, r4, 0, 27, 27
    beq L560
    oris r0, r0, 2048
L560:
    rlwinm. r3, r31, 0, 25, 25
    beq L594
    lis r3, -13312
    lhz r4, 20490(r3)
    rlwinm. r3, r4, 0, 28, 28
    beq L57c
    oris r0, r0, 1024
L57c:
    rlwinm. r3, r4, 0, 26, 26
    beq L588
    oris r0, r0, 512
L588:
    rlwinm. r3, r4, 0, 24, 24
    beq L594
    oris r0, r0, 256
L594:
    rlwinm. r3, r31, 0, 26, 26
    beq L5b0
    lis r3, -13056
    lwz r3, 27648(r3)
    rlwinm. r3, r3, 0, 28, 28
    beq L5b0
    oris r0, r0, 128
L5b0:
    rlwinm. r3, r31, 0, 27, 27
    beq L630
    lis r3, -13056
    lwz r4, 26624(r3)
    rlwinm. r3, r4, 0, 30, 30
    beq L5cc
    oris r0, r0, 64
L5cc:
    rlwinm. r3, r4, 0, 28, 28
    beq L5d8
    oris r0, r0, 32
L5d8:
    rlwinm. r3, r4, 0, 20, 20
    beq L5e4
    oris r0, r0, 16
L5e4:
    lis r3, -13056
    lwz r4, 26644(r3)
    rlwinm. r3, r4, 0, 30, 30
    beq L5f8
    oris r0, r0, 8
L5f8:
    rlwinm. r3, r4, 0, 28, 28
    beq L604
    oris r0, r0, 4
L604:
    rlwinm. r3, r4, 0, 20, 20
    beq L610
    oris r0, r0, 2
L610:
    lis r3, -13056
    lwz r4, 26664(r3)
    rlwinm. r3, r4, 0, 30, 30
    beq L624
    oris r0, r0, 1
L624:
    rlwinm. r3, r4, 0, 28, 28
    beq L630
    ori r0, r0, 32768
L630:
    rlwinm. r3, r31, 0, 18, 18
    beq L63c
    ori r0, r0, 32
L63c:
    rlwinm. r3, r31, 0, 19, 19
    beq L648
    ori r0, r0, 64
L648:
    rlwinm. r3, r31, 0, 21, 21
    beq L654
    ori r0, r0, 4096
L654:
    rlwinm. r3, r31, 0, 22, 22
    beq L660
    ori r0, r0, 8192
L660:
    rlwinm. r3, r31, 0, 23, 23
    beq L66c
    ori r0, r0, 128
L66c:
    rlwinm. r3, r31, 0, 28, 28
    beq L678
    ori r0, r0, 2048
L678:
    rlwinm. r3, r31, 0, 29, 29
    beq L684
    ori r0, r0, 1024
L684:
    rlwinm. r3, r31, 0, 30, 30
    beq L690
    ori r0, r0, 512
L690:
    rlwinm. r3, r31, 0, 20, 20
    beq L69c
    ori r0, r0, 16384
L69c:
    clrlwi. r3, r31, 31
    beq L6a8
    ori r0, r0, 256
L6a8:
    rlwinm. r3, r31, 0, 17, 17
    beq L6b4
    ori r0, r0, 16
L6b4:
    lis r3, -32768
    lwz r4, 196(r3)
    lwz r3, 200(r3)
    or r3, r4, r3
    andc. r3, r0, r3
    beq L754
    lis r4, InterruptPriorityTable@ha
    addi r4, r4, InterruptPriorityTable@l
    nop
L6d8:
    lwz r0, 0(r4)
    and. r0, r3, r0
    beq L6f0
    cntlzw r0, r0
    extsh r29, r0
    b L6f8
L6f0:
    addi r4, r4, 4
    b L6d8
L6f8:
    lwz r3, InterruptHandlerTable(r0)
    slwi r0, r29, 2
    lwzx r31, r3, r0
    cmpwi r31, 0
    beq L754
    cmpwi r29, 4
    ble L72c
    sth r29, __OSLastInterrupt(r0)
    bl OSGetTime
    stw r4, __OSLastInterruptTime+0x4(r0)
    stw r3, __OSLastInterruptTime(r0)
    lwz r0, 408(r30)
    stw r0, __OSLastInterruptSrr0(r0)
L72c:
    bl OSDisableScheduler
    mr r12, r31
    mr r3, r29
    mr r4, r30
    mtctr r12
    bctrl
    bl OSEnableScheduler
    bl __OSReschedule
    mr r3, r30
    bl OSLoadContext
L754:
    mr r3, r30
    bl OSLoadContext
    lwz r0, 36(r1)
    lwz r31, 28(r1)
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    mtlr r0
    addi r1, r1, 32
    blr
}

/* Saves the interrupted registers and branches to the dispatcher. */
static asm void ExternalInterruptHandler(u8 exception, OSContext* context)
{
    nofralloc
    stw r0, 0(r4)
    stw r1, 4(r4)
    stw r2, 8(r4)
    stmw r6, 24(r4)
    mfspr r0, GQR1
    stw r0, 424(r4)
    mfspr r0, GQR2
    stw r0, 428(r4)
    mfspr r0, GQR3
    stw r0, 432(r4)
    mfspr r0, GQR4
    stw r0, 436(r4)
    mfspr r0, GQR5
    stw r0, 440(r4)
    mfspr r0, GQR6
    stw r0, 444(r4)
    mfspr r0, GQR7
    stw r0, 448(r4)
    stwu r1, -8(r1)
    b __OSDispatchInterrupt
}
