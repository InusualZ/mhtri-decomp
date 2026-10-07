/*
 * OS/OSMemory.c - the OS memory layout: physical/simulated sizes, the memory-protection interrupt handler, BAT setup
 *    and `__OSInitMemoryProtection`.
 * RANGE. .text 0x804D1740-0x804D1EE0 (15 functions); .data 0x8061D400-0x8061D410; .sbss 0x80795370-0x80795378.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: its `ShutdownFunctionInfo` (.data 0x8061D400) is
 *    address-taken by `__OSInitMemoryProtection` (0x804D1E94, the OSAlarm one is 0x8061C0C8) and its
 *    `initialized` flag (.sbss 0x80795370) by the same function.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. `OSGetPhysicalMem2Size`, `BATConfig`, `__OSInitMemoryProtection` are the map's names; `MEMIntrruptHandler`
 *    is the map's (sic).  GUESS: `MEMShutdown` (the registered shutdown function: clears the memory-controller
 *    interrupt and masks the memory-protection sources), `RealModeJump` (rfi to a physical address with IR/DR off) and the
 *    six `BATSetup*` helpers (the BAT register sets for a 24 MB MEM1 and for each simulated-MEM2 size / arena-end
 *    class `BATConfig` tests); the map had them as `fn_804D1770`..`fn_804D1C80`.
 * RESIDUALS. none recorded yet.
 * SHAPES. the BAT helpers and `RealModeJump` are asm functions with `nofralloc` (mtspr to BAT/SRR registers, isync,
 *    rfi: C cannot spell them; playbook 104); the rest is C.
 */

#include "types.h"

#include "OS/DCInvalidateRange.h"
#include "OS/OSContext.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSMemory.h"
#include "OS/OSReset.h"

/* The memory controller's 16-bit registers (0xCC004000 block) and the low-memory words the layout code reads. */
#define MEM_INTERRUPT_MASK (*(volatile u16*)0xCC004010)
#define MEM_PROTECT_CAUSE (*(volatile u16*)0xCC00401E)
#define MEM_INTERRUPT_CLEAR (*(volatile u16*)0xCC004020)
#define MEM_ADDRESS_LO (*(volatile u16*)0xCC004022)
#define MEM_ADDRESS_HI (*(volatile u16*)0xCC004024)
#define MEM_SIMULATE_24MB (*(volatile u16*)0xCC004028)

#define OS_PHYSICAL_MEM1_SIZE (*(u32*)0x80003100)
#define OS_SIMULATED_MEM1_SIZE (*(u32*)0x80003104)
#define OS_PHYSICAL_MEM2_SIZE (*(u32*)0x80003118)
#define OS_SIMULATED_MEM2_SIZE (*(u32*)0x8000311C)
#define OS_MEM2_ARENA_END (*(u32*)0x80003120)

#define OS_ERROR_MEMORY_PROTECTION 15

BOOL MEMShutdown(BOOL final, u32 event);
static void MEMIntrruptHandler(s16 interrupt, OSContext* context);
asm void BATSetupMem1_24MB(void);
asm void BATSetupMem2_64MB_Hi52MB(void);
asm void BATSetupMem2_64MB_Hi56MB(void);
asm void BATSetupMem2_64MB_HiOver56MB(void);
asm void BATSetupMem2_128MB_Hi112MB(void);
asm void BATSetupMem2_128MB_HiOver112MB(void);
asm void RealModeJump(u32 target);
static void BATConfig(u32 magic);

static OSShutdownFunctionInfo ShutdownFunctionInfo = { MEMShutdown, 127 };

/* Returns the size of MEM2. */
u32 OSGetPhysicalMem2Size(void)
{
    return OS_PHYSICAL_MEM2_SIZE;
}

/* Returns the simulated size of MEM1. */
u32 OSGetConsoleSimulatedMem1Size(void)
{
    return OS_SIMULATED_MEM1_SIZE;
}

/* Returns the simulated size of MEM2. */
u32 OSGetConsoleSimulatedMem2Size(void)
{
    return OS_SIMULATED_MEM2_SIZE;
}

/* On the final shutdown pass, clears the memory-controller interrupt mask and masks the memory-protection sources. */
BOOL MEMShutdown(BOOL final, u32 event)
{
    if (final) {
        MEM_INTERRUPT_MASK = 0xFF;
        __OSMaskInterrupts(0xF0000000);
    }
    return TRUE;
}

/* Reads the faulting address and cause, acknowledges the interrupt and runs the memory-protection error handler. */
static void MEMIntrruptHandler(s16 interrupt, OSContext* context)
{
    u32 cause = MEM_PROTECT_CAUSE;
    u32 address = (u32)(MEM_ADDRESS_HI & 0x3FF) << 16 | MEM_ADDRESS_LO;

    MEM_INTERRUPT_CLEAR = 0;
    if (__OSErrorTable[OS_ERROR_MEMORY_PROTECTION]) {
        __OSErrorTable[OS_ERROR_MEMORY_PROTECTION](OS_ERROR_MEMORY_PROTECTION, context, cause, address);
        return;
    }
    __OSUnhandledException(OS_ERROR_MEMORY_PROTECTION, context, cause, address);
}

/* Maps the 24 MB MEM1 layout in BAT pairs 0 and 2 and returns to its caller with address translation on. */
static asm void BATSetupMem1_24MB(void)
{
    nofralloc
    li r7, 0
    lis r4, 0
    addi r4, r4, 2
    lis r3, -32768
    addi r3, r3, 511
    lis r6, 256
    addi r6, r6, 2
    lis r5, -32512
    addi r5, r5, 255
    isync
    mtdbatu 0, r7
    mtdbatl 0, r4
    mtdbatu 0, r3
    isync
    mtibatu 0, r7
    mtibatl 0, r4
    mtibatu 0, r3
    isync
    mtdbatu 2, r7
    mtdbatl 2, r6
    mtdbatu 2, r5
    isync
    mtibatu 2, r7
    mtibatl 2, r6
    mtibatu 2, r5
    isync
    mfmsr r3
    ori r3, r3, 48
    mtspr SRR1,  r3
    mflr r3
    mtspr SRR0,  r3
    rfi
}

/* Maps the BATs for a simulated MEM2 of up to 64 MB whose arena ends at or below 0x93400000, then returns with translation on. */
static asm void BATSetupMem2_64MB_Hi52MB(void)
{
    nofralloc
    li r7, 0
    lis r4, 4096
    addi r4, r4, 2
    lis r3, -28672
    addi r3, r3, 1023
    lis r6, 4096
    addi r6, r6, 42
    lis r5, -12288
    addi r5, r5, 2047
    isync
    mtspr 568, r7
    mtspr 569, r4
    mtspr 568, r3
    isync
    mtspr 560, r7
    mtspr 561, r7
    isync
    mtspr 570, r7
    mtspr 571, r6
    mtspr 570, r5
    isync
    mtspr 562, r7
    mtspr 563, r7
    isync
    lis r4, 4608
    addi r4, r4, 2
    lis r3, -28160
    addi r3, r3, 511
    lis r6, 4864
    addi r6, r6, 2
    lis r5, -27904
    addi r5, r5, 127
    isync
    mtspr 572, r7
    mtspr 573, r4
    mtspr 572, r3
    isync
    mtspr 564, r7
    mtspr 565, r7
    isync
    mtspr 574, r7
    mtspr 575, r6
    mtspr 574, r5
    isync
    mtspr 566, r7
    mtspr 567, r7
    isync
    mfmsr r3
    ori r3, r3, 48
    mtspr SRR1,  r3
    mflr r3
    mtspr SRR0,  r3
    rfi
}

/* Maps the BATs for a simulated MEM2 of up to 64 MB whose arena ends at or below 0x93800000, then returns with translation on. */
static asm void BATSetupMem2_64MB_Hi56MB(void)
{
    nofralloc
    li r7, 0
    lis r4, 4096
    addi r4, r4, 2
    lis r3, -28672
    addi r3, r3, 1023
    lis r6, 4096
    addi r6, r6, 42
    lis r5, -12288
    addi r5, r5, 2047
    isync
    mtspr 568, r7
    mtspr 569, r4
    mtspr 568, r3
    isync
    mtspr 560, r7
    mtspr 561, r7
    isync
    mtspr 570, r7
    mtspr 571, r6
    mtspr 570, r5
    isync
    mtspr 562, r7
    mtspr 563, r7
    isync
    lis r4, 4608
    addi r4, r4, 2
    lis r3, -28160
    addi r3, r3, 511
    lis r6, 4864
    addi r6, r6, 2
    lis r5, -27904
    addi r5, r5, 255
    isync
    mtspr 572, r7
    mtspr 573, r4
    mtspr 572, r3
    isync
    mtspr 564, r7
    mtspr 565, r7
    isync
    mtspr 574, r7
    mtspr 575, r6
    mtspr 574, r5
    isync
    mtspr 566, r7
    mtspr 567, r7
    isync
    mfmsr r3
    ori r3, r3, 48
    mtspr SRR1,  r3
    mflr r3
    mtspr SRR0,  r3
    rfi
}

/* Maps the BATs for a simulated MEM2 of up to 64 MB whose arena ends higher, then returns with translation on. */
static asm void BATSetupMem2_64MB_HiOver56MB(void)
{
    nofralloc
    li r7, 0
    lis r4, 4096
    addi r4, r4, 2
    lis r3, -28672
    addi r3, r3, 2047
    lis r6, 4096
    addi r6, r6, 42
    lis r5, -12288
    addi r5, r5, 2047
    isync
    mtspr 568, r7
    mtspr 569, r4
    mtspr 568, r3
    isync
    mtspr 560, r7
    mtspr 561, r7
    isync
    mtspr 570, r7
    mtspr 571, r6
    mtspr 570, r5
    isync
    mtspr 562, r7
    mtspr 563, r7
    isync
    mtspr 564, r7
    mtspr 565, r7
    isync
    mtspr 566, r7
    mtspr 567, r7
    isync
    mtspr 572, r7
    mtspr 573, r7
    isync
    mtspr 574, r7
    mtspr 575, r7
    isync
    mfmsr r3
    ori r3, r3, 48
    mtspr SRR1,  r3
    mflr r3
    mtspr SRR0,  r3
    rfi
}

/* Maps the BATs for a simulated MEM2 of up to 128 MB whose arena ends at or below 0x97000000, then returns with translation on. */
static asm void BATSetupMem2_128MB_Hi112MB(void)
{
    nofralloc
    li r7, 0
    lis r4, 4096
    addi r4, r4, 2
    lis r3, -28672
    addi r3, r3, 2047
    lis r6, 4096
    addi r6, r6, 42
    lis r5, -12288
    addi r5, r5, 4095
    isync
    mtspr 568, r7
    mtspr 569, r4
    mtspr 568, r3
    isync
    mtspr 560, r7
    mtspr 561, r7
    isync
    mtspr 570, r7
    mtspr 571, r6
    mtspr 570, r5
    isync
    mtspr 562, r7
    mtspr 563, r7
    isync
    lis r4, 5120
    addi r4, r4, 2
    lis r3, -27648
    addi r3, r3, 1023
    lis r6, 5632
    addi r6, r6, 2
    lis r5, -27136
    addi r5, r5, 511
    isync
    mtspr 572, r7
    mtspr 573, r4
    mtspr 572, r3
    isync
    mtspr 564, r7
    mtspr 565, r7
    isync
    mtspr 574, r7
    mtspr 575, r6
    mtspr 574, r5
    isync
    mtspr 566, r7
    mtspr 567, r7
    isync
    mfmsr r3
    ori r3, r3, 48
    mtspr SRR1,  r3
    mflr r3
    mtspr SRR0,  r3
    rfi
}

/* Maps the BATs for a simulated MEM2 of up to 128 MB whose arena ends higher, then returns with translation on. */
static asm void BATSetupMem2_128MB_HiOver112MB(void)
{
    nofralloc
    li r7, 0
    lis r4, 4096
    addi r4, r4, 2
    lis r3, -28672
    addi r3, r3, 4095
    lis r6, 4096
    addi r6, r6, 42
    lis r5, -12288
    addi r5, r5, 4095
    isync
    mtspr 568, r7
    mtspr 569, r4
    mtspr 568, r3
    isync
    mtspr 560, r7
    mtspr 561, r7
    isync
    mtspr 570, r7
    mtspr 571, r6
    mtspr 570, r5
    isync
    mtspr 562, r7
    mtspr 563, r7
    isync
    mtspr 564, r7
    mtspr 565, r7
    isync
    mtspr 566, r7
    mtspr 567, r7
    isync
    mtspr 572, r7
    mtspr 573, r7
    isync
    mtspr 574, r7
    mtspr 575, r7
    isync
    mfmsr r3
    ori r3, r3, 48
    mtspr SRR1,  r3
    mflr r3
    mtspr SRR0,  r3
    rfi
}

/* Jumps to the physical address in r3 with instruction and data translation off. */
static asm void RealModeJump(u32 target)
{
    nofralloc
    clrlwi r3, r3, 2
    mtspr SRR0,  r3
    mfmsr r3
    rlwinm r3, r3, 0, 28, 25
    mtspr SRR1,  r3
    rfi
}

/* Picks the BAT layout that fits the simulated memory sizes, switches to it through real mode and checks the magic. */
static void BATConfig(u32 magic)
{
    u32 mem1 = OS_SIMULATED_MEM1_SIZE;
    u32 mem2;
    u32 mem2End;

    if (mem1 < OS_PHYSICAL_MEM1_SIZE && mem1 == 0x1800000) {
        DCInvalidateRange((void*)0x81800000, 0x1800000);
        MEM_SIMULATE_24MB = 2;
    }
    if (mem1 <= 0x1800000) {
        RealModeJump((u32)BATSetupMem1_24MB);
    }

    mem2 = OS_SIMULATED_MEM2_SIZE;
    mem2End = OS_MEM2_ARENA_END;
    if (mem2 <= 0x4000000) {
        if (mem2End <= 0x93400000) {
            RealModeJump((u32)BATSetupMem2_64MB_Hi52MB);
        } else if (mem2End <= 0x93800000) {
            RealModeJump((u32)BATSetupMem2_64MB_Hi56MB);
        } else {
            RealModeJump((u32)BATSetupMem2_64MB_HiOver56MB);
        }
    } else if (mem2 <= 0x8000000) {
        if (mem2End <= 0x97000000) {
            RealModeJump((u32)BATSetupMem2_128MB_Hi112MB);
        } else {
            RealModeJump((u32)BATSetupMem2_128MB_HiOver112MB);
        }
    }

    while (magic != 0xBA2CF) {
    }
}

/* Re-maps the BATs for execution from MEM1 and checks the magic. */
void __OSRestoreCodeExecOnMEM1(u32 magic)
{
    RealModeJump((u32)BATSetupMem1_24MB);
    while (magic != 0xBA2CF) {
    }
}

/* Installs the memory-protection interrupt handlers and the BAT layout on first call. */
void __OSInitMemoryProtection(void)
{
    static BOOL initialized;
    BOOL enabled;

    if (initialized) {
        return;
    }
    enabled = OSDisableInterrupts();
    MEM_INTERRUPT_CLEAR = 0;
    MEM_INTERRUPT_MASK = 0xFF;
    __OSMaskInterrupts(0xF0000000);
    __OSSetInterruptHandler(0, MEMIntrruptHandler);
    __OSSetInterruptHandler(1, MEMIntrruptHandler);
    __OSSetInterruptHandler(2, MEMIntrruptHandler);
    __OSSetInterruptHandler(3, MEMIntrruptHandler);
    __OSSetInterruptHandler(4, MEMIntrruptHandler);
    OSRegisterShutdownFunction(&ShutdownFunctionInfo);
    BATConfig(0xBA2CF);
    __OSUnmaskInterrupts(0x8000000);
    initialized = TRUE;
    OSRestoreInterrupts(enabled);
}
