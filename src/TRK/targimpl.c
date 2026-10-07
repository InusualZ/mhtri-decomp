/*
 * TRK/targimpl.c - the MetroTRK target implementation: MSR access, memory/register access, the exception and
 *    interrupt handlers, step control, the PPC special-register access, the input-pending setter and the
 *    address/thread stubs.
 *
 * RANGE. .text 0x8046BBEC..0x8046D420 (36 functions in the map, 0x1834 B); .rodata 0x805734A8..0x80573530; .data
 *    0x8060F810..0x8060F820; .bss 0x806F6F38..0x806F74E0; .sbss 0x80794E78..0x80794E88.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `targimpl`); `gTRKSpecialRegBuffer` (map row lbl_806F6FF8), `gTRKStepState`
 *    (map row lbl_806F6F48), `gTRKUseSerialIO` (lbl_80794E80), `TRKLowMemory` and `TRK_LOMEM` (the OS globals at 0x800000DC /
 *    0x800000E4 `GetThreadInfo` walks) are GUESS.
 * EVIDENCE. `.data` 0x8060F810 (`gTRKExceptionStatus`), `.bss` 0x806F6F38.. (restore flags, step state, save
 *    state, `gTRKState`, `gTRKCPUState`), `.sbss` 0x80794E78 / 0x80794E80 and `.rodata` 0x805734A8..0x80573530
 *    are read by functions on both sides of every internal candidate boundary, so the run is one TU unless a
 *    boundary moves the globals.
 * RESIDUALS. COARSE: this is the biggest TRK unit; the PPC-access group (0x8046CF6C..) and the glue (0x8046D368..)
 *    may be separate files but the shared `gTRKState` forbids cutting before the `.bss` order is understood. Bodies
 *    written (12 rows at 100): the MSR accessors, GetPC, Stopped, SetStopped, Stop, SetInputPendingPtr, ConvertAddress,
 *    GetThreadInfo, the serial-IO flag and TRKPPCAccessSpecialReg. `TRKTargetSingleStep` 67 and `TRKTargetStepOutOfRange` 86:
 *    the target keeps an unreachable `b`/`bne` pair and a dead `count` update after arming the step (a `count - 1` reload
 *    in SingleStep) that no spelling measured here reproduces. NOT attempted: the memory/register access group
 *    (0x8046BBFC..0x8046C418), TRKInterruptHandler .. TRKTargetInterrupt (0x8046C418..0x8046C874, assembly-heavy),
 *    TRKTargetAddStopInfo / AddExceptionInfo / CheckStep (0x8046C874..0x8046CC34), TRKTargetSupportRequest (0x8046CD40),
 *    the instruction-template register access (0x8046CF6C..0x8046D0F0, 0x8046D138) and ReadFPSCR / WriteFPSCR
 *    (paired-single save of f31). The `.rodata` templates, `gTRKExceptionStatus`, the save state and the restore flags are
 *    not defined yet, so the unit's data does not match.
 * SHAPES. the special-register accessors build a ten-word instruction template on the stack, patch in the register
 *    number, flush the cache and call it.
 */
#include "TRK/targimpl.h"
#include "TRK/TRK_flush_cache.h"
#include "OS/OSThread.h"

#define TRK_BLR_INSTRUCTION 0x4E800020

/* The OS globals at the bottom of the cached address space that the thread walk reads. */
typedef struct TRKLowMemory {
    /* +0x00 */ u8 pad_0x00[0xDC];
    /* +0xDC */ OSThread* active_thread_head;    /* first thread of the active list */
    /* +0xE0 */ OSThread* active_thread_tail;
    /* +0xE4 */ OSThread* current_thread;
} TRKLowMemory; /* size: 0xE8 */

#define TRK_LOMEM ((TRKLowMemory*)0x80000000)

TRKCPUState gTRKCPUState;
TRKState gTRKState;
TRKStepState gTRKStepState;
u32 gTRKSpecialRegBuffer[4];
u8 gTRKUseSerialIO;

asm u32 __TRK_get_MSR(void)
{
    nofralloc
    mfmsr r3
    blr
}

asm void __TRK_set_MSR(u32 msr)
{
    nofralloc
    mtmsr r3
    blr
}

s32 TRKTargetSingleStep(u32 count, s32 step_over)
{
    if (step_over != 0) {
        return 0x703;
    }
    gTRKStepState.kind = 0;
    gTRKStepState.count = count;
    gTRKStepState.active = 1;
    gTRKCPUState.srr1 = (gTRKCPUState.srr1 | 0x400) & ~0x8000;
    TRKTargetSetStopped(0);
    return 0;
}

s32 TRKTargetStepOutOfRange(u32 start, u32 end, s32 step_over)
{
    if (step_over != 0) {
        return 0x703;
    }
    gTRKStepState.kind = 1;
    gTRKStepState.range_start = start;
    gTRKStepState.range_end = end;
    gTRKStepState.active = 1;
    gTRKCPUState.srr1 = (gTRKCPUState.srr1 | 0x400) & ~0x8000;
    TRKTargetSetStopped(0);
    return 0;
}

u32 TRKTargetGetPC(void)
{
    return gTRKCPUState.pc;
}

s32 TRKTargetStopped(void)
{
    return gTRKState.stopped;
}

void TRKTargetSetStopped(s32 stopped)
{
    gTRKState.stopped = stopped;
}

s32 TRKTargetStop(void)
{
    gTRKState.stopped = 1;
    return 0;
}

s32 TRKPPCAccessSpecialReg(u32* data, u32* instructions)
{
    instructions[9] = TRK_BLR_INSTRUCTION;
    TRK_flush_cache(instructions, 0x28);
    ((void (*)(u32*, u32*))instructions)(data, gTRKSpecialRegBuffer);
    return 0;
}

void TRKTargetSetInputPendingPtr(u8* inputPendingPtr)
{
    gTRKState.input_pending_ptr = inputPendingPtr;
}

u32 ConvertAddress(u32 address)
{
    return address | 0x80000000;
}

void GetThreadInfo(u32* count, u32* currentIndex)
{
    u32 index;
    OSThread* thread;

    *count = 1;
    *currentIndex = 0;
    thread = TRK_LOMEM->active_thread_head;
    if (thread == (OSThread*)-1 || thread == NULL || thread == (OSThread*)0x80000000) {
        return;
    }
    for (index = 0; thread != NULL;) {
        if (thread == TRK_LOMEM->current_thread) {
            *currentIndex = index;
        }
        thread = (OSThread*)ConvertAddress((u32)thread->linkActive.next);
        index++;
        if (thread == (OSThread*)-1 || thread == NULL || thread == (OSThread*)0x80000000) {
            break;
        }
    }
    *count = index;
}

void SetUseSerialIO(u8 useSerial)
{
    gTRKUseSerialIO = useSerial;
}

u8 GetUseSerialIO(void)
{
    return gTRKUseSerialIO;
}
