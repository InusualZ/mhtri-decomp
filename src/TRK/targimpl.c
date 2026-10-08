/*
 * TRK/targimpl.c - the MetroTRK target implementation: MSR access, memory/register access, the exception and
 *    interrupt handlers, step control, the PPC special-register access, the input-pending setter and the
 *    address/thread stubs.
 *
 * RANGE. .text 0x8046BBEC..0x8046D420 (36 functions in the map, 0x1834 B); .rodata 0x805734A8..0x80573530; .data
 *    0x8060F810..0x8060F820; .bss 0x806F6F38..0x806F74E0; .sbss 0x80794E78..0x80794E88.
 * FLAGS. the `OS` lib's `cflags_os`. `#pragma use_lmw_stmw on` (the register-heavy bodies save r24..r31 with stmw).
 * NAMES. The file name targimpl is a GUESS (MetroTRK `targimpl`). gTRKSpecialRegBuffer (map row lbl_806F6FF8) is a GUESS,
 *    gTRKStepState (lbl_806F6F48) is a GUESS, gTRKUseSerialIO (lbl_80794E80) is a GUESS, gTRKMemoryRange (lbl_805734A8) is a
 *    GUESS. TRKLowMemory and TRK_LOMEM (the OS globals at 0x800000D4..0x800000E4) are a GUESS. TRKMemoryRange, TRKStopInfo,
 *    TRKExceptionInfo, TRKExceptionStatus and their fields are a GUESS. TRKTargetCheckMemoryRange (fn_8046BBFC) is a GUESS,
 *    TRKTargetCopyBytes (fn_8046BCF4) is a GUESS, TRKTargetAccessSPR (fn_8046CF6C) is a GUESS, TRKTargetAccessPairedSingle
 *    (fn_8046D048) is a GUESS, TRKTargetAccessFPRegister (fn_8046D138) is a GUESS (named from the instruction templates and the
 *    callers); TRKTargetArmStep is a GUESS (the shared tail of the step requests).
 * EVIDENCE. `.data` 0x8060F810 (`gTRKExceptionStatus`), `.bss` 0x806F6F38.. (restore flags, step state, save
 *    state, `gTRKState`, `gTRKCPUState`), `.sbss` 0x80794E78 / 0x80794E80 and `.rodata` 0x805734A8..0x80573530
 *    are read by functions on both sides of every internal candidate boundary, so the run is one TU unless a
 *    boundary moves the globals.
 * RESIDUALS. 32 of 36 rows at 100. TRKTargetAccessDefault 88 (the target keeps the register count in r31 and the
 *    `last - first` difference in r4; a `u16` register count scores 94 but changes the arithmetic, not applied);
 *    TRKTargetAccessFPRegister 96 (first template copy scheduling);
 *    TRKTargetCopyBytes 97 (param registers r26..r30 in a different order); TRKTargetAccessExtended1 99 (`slwi` of the second
 *    flag test is recomputed, ours reuses the stored byte count). `.rodata`: the target pools the second template of
 *    TRKTargetAccessFPRegister with the first one of TRKTargetAccessSPR, ours emits four copies. `.bss` order: the target
 *    lays gTRKRestoreFlags, gTRKStepState, gTRKSaveState, gTRKSpecialRegBuffer, gTRKState, gTRKCPUState in that address
 *    order; MWCC emits them in first-use order (gTRKCPUState first), so the object does not match and the unit cannot flip.
 *    `.sbss`: the target puts gTRKUseSerialIO at +8 behind TRK_saved_exceptionID (+0), ours packs it at +0x1.
 * SHAPES. a template array initialised from `.rodata` is a stack copy; TRKTargetSingleStep and StepOutOfRange inline the same
 *    static helper as TRKTargetCheckStep (the unreachable b/bne pair of the target comes from its constant-folded kind test);
 *    the special-register accessors build a ten-word instruction template on the stack, patch in the register
 *    number, flush the cache and call it; the exception, interrupt and context-swap entries are assembly because they
 *    run with no stack frame and swap r0-r3 and the SPRs by hand.
 */
#include "TRK/targimpl.h"
#include "TRK/dolphin_trk.h"
#include "TRK/mem_TRK.h"
#include "TRK/support.h"
#include "TRK/targcont.h"
#include "TRK/TRK_flush_cache.h"
#include "OS/OSThread.h"

#pragma use_lmw_stmw on

#define TRK_BLR_INSTRUCTION 0x4E800020
#define TRK_SUPPORT_TRAP_INSTRUCTION 0x0FE00000

/* The OS globals at the bottom of the cached address space that the thread walk reads. */
typedef struct TRKLowMemory {
    /* +0x00 */ u8 pad_0x00[0xD4];
    /* +0xD4 */ u32 current_context_phys;   /* physical address of the running context */
    /* +0xD8 */ u32 current_context;        /* running context */
    /* +0xDC */ OSThread* active_thread_head;    /* first thread of the active list */
    /* +0xE0 */ OSThread* active_thread_tail;
    /* +0xE4 */ OSThread* current_thread;
} TRKLowMemory; /* size: 0xE8 */

#define TRK_LOMEM ((TRKLowMemory*)0x80000000)

/* One range of target memory the debugger may access. */
typedef struct TRKMemoryRange {
    /* +0x0 */ u32 start;
    /* +0x4 */ u32 end;           /* inclusive */
    /* +0x8 */ s32 readable;
    /* +0xC */ s32 writable;
} TRKMemoryRange; /* size: 0x10 */

/* What the exception handler recorded about the last exception the nub itself took. */
typedef struct TRKExceptionStatus {
    /* +0x0 */ u32 srr0;               /* address of the faulting instruction */
    /* +0x4 */ u32 pad_0x4;
    /* +0x8 */ u16 exception_id;       /* vector of the exception */
    /* +0xA */ u16 pad_0xA;
    /* +0xC */ u8 nub_active;          /* 1 while the nub runs, 0 while the target runs */
    /* +0xD */ u8 exception_occurred;  /* set by the exception handler, polled by the access routines */
    /* +0xE */ u16 pad_0xE;
} TRKExceptionStatus; /* size: 0x10 */

/* The stop notification record: command 0x90 payload ahead of the register image. */
typedef struct TRKStopInfo {
    /* +0x00 */ u32 length;            /* bytes of the whole notification */
    /* +0x04 */ u8 command;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u32 pc;
    /* +0x0C */ u32 instruction;       /* the instruction at pc */
    /* +0x10 */ u32 exception_id;
    /* +0x14 */ u32 thread_count;
    /* +0x18 */ u32 current_thread;
    /* +0x1C */ u8 pad_0x1C[0x24];
} TRKStopInfo; /* size: 0x40 */

/* The exception notification record. */
typedef struct TRKExceptionInfo {
    /* +0x00 */ u32 length;
    /* +0x04 */ u8 command;
    /* +0x05 */ u8 pad_0x05[3];
    /* +0x08 */ u32 srr0;              /* address of the faulting instruction */
    /* +0x0C */ u32 instruction;
    /* +0x10 */ u32 exception_id;
    /* +0x14 */ u8 pad_0x14[0x2C];
} TRKExceptionInfo; /* size: 0x40 */

TRKRestoreFlags gTRKRestoreFlags;
TRKStepState gTRKStepState;
TRKSaveState gTRKSaveState;
u32 gTRKSpecialRegBuffer[4];
TRKState gTRKState;
TRKCPUState gTRKCPUState;
static TRKExceptionStatus gTRKExceptionStatus = {0, 0, 0, 0, 1, 0, 0};
static u16 TRK_saved_exceptionID;
u8 gTRKUseSerialIO;

extern TRKMemoryRange gTRKMemoryRange[1];

static void TRKTargetAccessSPR(u32* data, u32 spr, s32 is_read);
static s32 TRKTargetAccessPairedSingle(u64* data, u32 reg, s32 is_read);
static s32 TRKTargetAccessFPRegister(u64* data, u32 reg, s32 is_read);

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

/* Checks that `length` bytes at `address` lie in readable (`is_write` 0) or writable memory; returns 0 or 0x700. */
static s32 TRKTargetCheckMemoryRange(u32 address, u32 length, s32 is_write)
{
    u32 last;
    s32 err = 0x700;
    s32 i;

    length += address;
    last = length - 1;

    if (last < address) {
        return 0x700;
    }
    for (i = 0; i < 1; i++) {
        if (address > gTRKMemoryRange[i].end || last < gTRKMemoryRange[i].start) {
            continue;
        }
        if ((is_write == 0 && gTRKMemoryRange[i].readable == 0) || (is_write == 1 && gTRKMemoryRange[i].writable == 0)) {
            err = 0x700;
        } else {
            err = 0;
            if (address < gTRKMemoryRange[i].start) {
                err = TRKTargetCheckMemoryRange(address, gTRKMemoryRange[i].start - address, is_write);
            }
            if (err == 0 && last > gTRKMemoryRange[i].end) {
                err = TRKTargetCheckMemoryRange(gTRKMemoryRange[i].end, last - gTRKMemoryRange[i].end, is_write);
            }
        }
        break;
    }
    return err;
}

__declspec(section ".rodata") TRKMemoryRange gTRKMemoryRange[1] = {{0, 0xFFFFFFFF, 1, 1}};

/* Copies `length` bytes one at a time through aligned word accesses, switching the MSR around each read and write. */
static void TRKTargetCopyBytes(u8* dst, const u8* src, u32 length, u32 dst_msr, u32 src_msr)
{
    u32 saved_msr = __TRK_get_MSR();

    while (length != 0) {
        u32* src_word;
        u32* dst_word;
        u32 value;
        u32 mask;

        __TRK_set_MSR(src_msr);
        src_word = (u32*)((u32)src & ~3);
        value = (u8)(*src_word >> ((3 - ((u32)src - (u32)src_word)) * 8));
        asm { sync }
        __TRK_set_MSR(dst_msr);
        dst_word = (u32*)((u32)dst & ~3);
        mask = 0xFF << ((3 - ((u32)dst - (u32)dst_word)) * 8);
        *dst_word = (*dst_word & ~mask) | ((value << ((3 - ((s32)dst - (s32)dst_word)) * 8)) & mask);
        asm { sync }
        src++;
        dst++;
        length--;
    }
    __TRK_set_MSR(saved_msr);
}

/* untyped: byte range */
s32 TRKTargetAccessMemory(void* data, u32 address, u32* length, s32 options, s32 is_read)
{
    TRKExceptionStatus saved = gTRKExceptionStatus;
    s32 err = 0;
    u32 translated;
    u32 msr;
    u32 target_msr;

    gTRKExceptionStatus.exception_occurred = 0;
    translated = TRKTranslateAddress(address);
    err = TRKTargetCheckMemoryRange(translated, *length, is_read == 0);
    if (err != 0) {
        *length = 0;
    } else {
        msr = __TRK_get_MSR();
        target_msr = msr | (gTRKCPUState.srr1 & 0x10);
        if (is_read != 0) {
            TRKTargetCopyBytes(data, (u8*)translated, *length, msr, target_msr);
        } else {
            TRKTargetCopyBytes((u8*)translated, data, *length, target_msr, msr);
            TRK_flush_cache((void*)translated, *length);
            if (address != translated) {
                TRK_flush_cache((void*)address, *length);
            }
        }
    }
    if (gTRKExceptionStatus.exception_occurred != 0) {
        *length = 0;
        err = 0x702;
    }
    gTRKExceptionStatus = saved;
    return err;
}

s32 TRKTargetAccessDefault(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read)
{
    TRKExceptionStatus saved;
    u32* registers;
    u32 registers_count;
    s32 err;

    if (last > 36) {
        return 0x701;
    }
    saved = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_occurred = 0;
    registers_count = last - first + 1;
    registers = &gTRKCPUState.gpr[first];
    *count = registers_count * 4;
    if (is_read != 0) {
        err = TRKAppendBuffer_ui32(message, registers, registers_count);
    } else {
        err = TRKReadBuffer_ui32(message, registers, registers_count);
    }
    if (gTRKExceptionStatus.exception_occurred != 0) {
        *count = 0;
        err = 0x702;
    }
    gTRKExceptionStatus = saved;
    return err;
}

s32 TRKTargetAccessFP(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read)
{
    TRKExceptionStatus saved;
    u64 value;
    s32 err;

    if (last > 33) {
        return 0x701;
    }
    saved = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_occurred = 0;
    __TRK_set_MSR(__TRK_get_MSR() | 0x2000);
    *count = 0;
    err = 0;
    while (first <= last && err == 0) {
        if (is_read != 0) {
            TRKTargetAccessFPRegister(&value, first, is_read);
            err = TRKAppendBuffer1_ui64(message, value);
        } else {
            TRKReadBuffer1_ui64(message, &value);
            err = TRKTargetAccessFPRegister(&value, first, is_read);
        }
        *count += 8;
        first++;
    }
    if (gTRKExceptionStatus.exception_occurred != 0) {
        *count = 0;
        err = 0x702;
    }
    gTRKExceptionStatus = saved;
    return err;
}

s32 TRKTargetAccessExtended1(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read)
{
    TRKExceptionStatus saved;
    u32* registers;
    u32 registers_count;
    s32 err;

    if (last > 0x60) {
        return 0x701;
    }
    saved = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_occurred = 0;
    *count = 0;
    if (first <= last) {
        registers_count = last - first + 1;
        *count = registers_count * 4;
        registers = &gTRKCPUState.pad_0x1A8[first];
        if (is_read != 0) {
            err = TRKAppendBuffer_ui32(message, registers, registers_count);
        } else {
            if (registers <= &gTRKCPUState.time_base_image[1]
                && registers + registers_count - 1 >= &gTRKCPUState.time_base_image[0]) {
                gTRKRestoreFlags.restore_time_base = 1;
            }
            if (registers <= &gTRKCPUState.flagged_reg_image && registers + registers_count - 1 >= &gTRKCPUState.flagged_reg_image) {
                gTRKRestoreFlags.restore_flag_1 = 1;
            }
            err = TRKReadBuffer_ui32(message, registers, registers_count);
        }
    }
    if (gTRKExceptionStatus.exception_occurred != 0) {
        *count = 0;
        err = 0x702;
    }
    gTRKExceptionStatus = saved;
    return err;
}

s32 TRKTargetAccessExtended2(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read)
{
    TRKExceptionStatus saved;
    u32 hid2;
    u64 value;
    s32 err;

    if (last > 31) {
        return 0x701;
    }
    saved = gTRKExceptionStatus;
    gTRKExceptionStatus.exception_occurred = 0;
    TRKTargetAccessSPR(&hid2, 920, 1);
    hid2 |= 0xA0000000;
    TRKTargetAccessSPR(&hid2, 920, 0);
    hid2 = 0;
    TRKTargetAccessSPR(&hid2, 912, 0);
    *count = 0;
    err = 0;
    while (first <= last && err == 0) {
        if (is_read != 0) {
            TRKTargetAccessPairedSingle(&value, first, is_read);
            err = TRKAppendBuffer1_ui64(message, value);
        } else {
            TRKReadBuffer1_ui64(message, &value);
            err = TRKTargetAccessPairedSingle(&value, first, is_read);
        }
        *count += 8;
        first++;
    }
    if (gTRKExceptionStatus.exception_occurred != 0) {
        *count = 0;
        err = 0x702;
    }
    gTRKExceptionStatus = saved;
    return err;
}

asm void TRKInterruptHandler(void)
{
    nofralloc
    mtspr SRR0, r2
    mtspr SRR1, r4
    mfsprg r4, 3
    mfcr r2
    mtsprg 3, r2
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    lwz r2, 140(r2)
    ori r2, r2, 32770
    xori r2, r2, 32770
    sync
    mtmsr r2
    sync
    lis r2, TRK_saved_exceptionID@h
    ori r2, r2, TRK_saved_exceptionID@l
    sth r3, 0(r2)
    cmpwi r3, 1280
    bne L_exception
    lis r2, gTRKCPUState@h
    ori r2, r2, gTRKCPUState@l
    mflr r3
    stw r3, 1068(r2)
    bl TRKUARTInterruptHandler
    lis r2, gTRKCPUState@h
    ori r2, r2, gTRKCPUState@l
    lwz r3, 1068(r2)
    mtlr r3
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    lwz r2, 160(r2)
    lbz r2, 0(r2)
    cmpwi r2, 0
    beq L_resume
    lis r2, gTRKExceptionStatus@h
    ori r2, r2, gTRKExceptionStatus@l
    lbz r2, 12(r2)
    cmpwi r2, 1
    beq L_resume
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    li r3, 1
    stb r3, 156(r2)
    b L_exception
L_resume:
    lis r2, gTRKSaveState@h
    ori r2, r2, gTRKSaveState@l
    lwz r3, 136(r2)
    mtcr r3
    lwz r3, 12(r2)
    lwz r2, 8(r2)
    rfi
L_exception:
    lis r2, TRK_saved_exceptionID@h
    ori r2, r2, TRK_saved_exceptionID@l
    lhz r3, 0(r2)
    lis r2, gTRKExceptionStatus@h
    ori r2, r2, gTRKExceptionStatus@l
    lbz r2, 12(r2)
    cmpwi r2, 0
    bne TRKExceptionHandler
    lis r2, gTRKCPUState@h
    ori r2, r2, gTRKCPUState@l
    stw r0, 0(r2)
    stw r1, 4(r2)
    mfsprg r0, 1
    stw r0, 8(r2)
    sth r3, 760(r2)
    sth r3, 762(r2)
    mfsprg r0, 2
    stw r0, 12(r2)
    stmw r4, 16(r2)
    mfspr r27, SRR0
    mflr r28
    mfsprg r29, 3
    mfctr r30
    mfxer r31
    stmw r27, 128(r2)
    bl TRKSaveExtended1Block
    lis r2, gTRKExceptionStatus@h
    ori r2, r2, gTRKExceptionStatus@l
    li r3, 1
    stb r3, 12(r2)
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    lwz r0, 140(r2)
    sync
    mtmsr r0
    sync
    lwz r0, 128(r2)
    mtlr r0
    lwz r0, 132(r2)
    mtctr r0
    lwz r0, 136(r2)
    mtxer r0
    lwz r0, 148(r2)
    mtspr DSISR, r0
    lwz r0, 144(r2)
    mtspr DAR, r0
    lmw r3, 12(r2)
    lwz r0, 0(r2)
    lwz r1, 4(r2)
    lwz r2, 8(r2)
    b TRKPostInterruptEvent
}

asm void TRKExceptionHandler(void)
{
    nofralloc
    lis r2, gTRKExceptionStatus@h
    ori r2, r2, gTRKExceptionStatus@l
    sth r3, 8(r2)
    mfspr r3, SRR0
    stw r3, 0(r2)
    lhz r3, 8(r2)
    cmpwi r3, 512
    beq L_skip
    cmpwi r3, 768
    beq L_skip
    cmpwi r3, 1024
    beq L_skip
    cmpwi r3, 1536
    beq L_skip
    cmpwi r3, 1792
    beq L_skip
    cmpwi r3, 2048
    beq L_skip
    cmpwi r3, 4096
    beq L_skip
    cmpwi r3, 4352
    beq L_skip
    cmpwi r3, 4608
    beq L_skip
    cmpwi r3, 4864
    beq L_skip
    b L_flag
L_skip:
    mfspr r3, SRR0
    addi r3, r3, 4
    mtspr SRR0, r3
L_flag:
    lis r2, gTRKExceptionStatus@h
    ori r2, r2, gTRKExceptionStatus@l
    li r3, 1
    stb r3, 13(r2)
    mfsprg r3, 3
    mtcr r3
    mfsprg r2, 1
    mfsprg r3, 2
    rfi
}

void TRKPostInterruptEvent(void)
{
    TRKEvent event;
    u32 instruction;
    u32 length;
    s32 type;
    s32 id;

    if (gTRKState.input_activated != 0) {
        gTRKState.input_activated = 0;
        return;
    }
    id = gTRKCPUState.exception_id & 0xFFFF;
    if (id == 0x700 || id == 0xD00) {
        length = 4;
        TRKTargetAccessMemory(&instruction, gTRKCPUState.pc, &length, 0, 1);
        if (instruction == TRK_SUPPORT_TRAP_INSTRUCTION) {
            type = TRK_EVENT_SUPPORT_REQUEST;
        } else {
            type = TRK_EVENT_BREAKPOINT;
        }
    } else {
        type = TRK_EVENT_EXCEPTION;
    }
    TRKConstructEvent(&event, type);
    TRKPostEvent(&event);
}

asm void TRKSwapAndGo(void)
{
    nofralloc
    lis r3, gTRKState@h
    ori r3, r3, gTRKState@l
    stmw r0, 0(r3)
    mfmsr r0
    stw r0, 140(r3)
    mflr r0
    stw r0, 128(r3)
    mfctr r0
    stw r0, 132(r3)
    mfxer r0
    stw r0, 136(r3)
    mfspr r0, DSISR
    stw r0, 148(r3)
    mfspr r0, DAR
    stw r0, 144(r3)
    li r1, -32766
    not r1, r1
    mfmsr r3
    and r3, r3, r1
    mtmsr r3
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    lwz r2, 160(r2)
    lbz r2, 0(r2)
    cmpwi r2, 0
    beq L_run
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    li r3, 1
    stb r3, 156(r2)
    b TRKInterruptHandlerEnableInterrupts
L_run:
    lis r2, gTRKExceptionStatus@h
    ori r2, r2, gTRKExceptionStatus@l
    li r3, 0
    stb r3, 12(r2)
    bl TRKRestoreExtended1Block
    lis r2, gTRKCPUState@h
    ori r2, r2, gTRKCPUState@l
    lmw r27, 128(r2)
    mtspr SRR0, r27
    mtlr r28
    mtcr r29
    mtctr r30
    mtxer r31
    lmw r3, 12(r2)
    lwz r0, 0(r2)
    lwz r1, 4(r2)
    lwz r2, 8(r2)
    rfi
}

asm void TRKInterruptHandlerEnableInterrupts(void)
{
    nofralloc
    lis r2, gTRKState@h
    ori r2, r2, gTRKState@l
    lwz r0, 140(r2)
    sync
    mtmsr r0
    sync
    lwz r0, 128(r2)
    mtlr r0
    lwz r0, 132(r2)
    mtctr r0
    lwz r0, 136(r2)
    mtxer r0
    lwz r0, 148(r2)
    mtspr DSISR, r0
    lwz r0, 144(r2)
    mtspr DAR, r0
    lmw r3, 12(r2)
    lwz r0, 0(r2)
    lwz r1, 4(r2)
    lwz r2, 8(r2)
    b TRKPostInterruptEvent
}

s32 TRKTargetInterrupt(TRKEvent* event)
{
    s32 err = 0;

    switch (event->type) {
    case TRK_EVENT_BREAKPOINT:
    case TRK_EVENT_EXCEPTION:
        if (TRKTargetCheckStep() == 0) {
            TRKTargetSetStopped(1);
            err = TRKDoNotifyStopped(0x90);
        }
        break;
    }
    return err;
}

s32 TRKTargetAddStopInfo(TRKBuffer* message)
{
    u32 instruction;
    u32 thread_count;
    u32 current_index;
    u32 page_length;
    u32 length;
    TRKStopInfo info;
    u32 page[0x100];
    u32* reg;
    s32 index;
    s32 err;

    TRK_memset(&info, 0, sizeof(info));
    info.length = 0x4E8;
    info.command = 0x90;
    info.pc = gTRKCPUState.pc;
    GetThreadInfo(&thread_count, &current_index);
    info.thread_count = thread_count;
    info.current_thread = *ConvertAddress(0xE4);
    length = 4;
    TRKTargetAccessMemory(&instruction, gTRKCPUState.pc, &length, 0, 1);
    info.instruction = instruction;
    info.exception_id = gTRKCPUState.exception_id & 0xFFFF;
    err = TRKAppendBuffer_ui8(message, &info, sizeof(info));
    if (err == 0) {
        reg = gTRKCPUState.gpr;
        for (index = 0; index < 32; index++) {
            TRKAppendBuffer1_ui32(message, *reg);
            reg++;
        }
        TRKAppendBuffer1_ui32(message, gTRKCPUState.pc);
        TRKAppendBuffer1_ui32(message, gTRKCPUState.lr);
        TRKAppendBuffer1_ui32(message, gTRKCPUState.cr);
        TRKAppendBuffer1_ui32(message, gTRKCPUState.ctr);
        err = TRKAppendBuffer1_ui32(message, gTRKCPUState.xer);
    }
    if (err == 0) {
        u32 word;

        word = *ConvertAddress(0xD4);
        TRKAppendBuffer1_ui32(message, word);
        word = *ConvertAddress(0xD8);
        TRKAppendBuffer1_ui32(message, word);
        word = *ConvertAddress(0xDC);
        TRKAppendBuffer1_ui32(message, word);
        word = *ConvertAddress(0xE0);
        TRKAppendBuffer1_ui32(message, word);
        word = *ConvertAddress(0xE4);
        err = TRKAppendBuffer1_ui32(message, word);
    }
    if (err == 0) {
        page_length = 0x400;
        err = TRKTargetAccessMemory(page, gTRKCPUState.pc & ~0x3FF, &page_length, 0, 1);
        TRK_AppendBuffer(message, page, 0x400);
    }
    return err;
}

s32 TRKTargetAddExceptionInfo(TRKBuffer* message)
{
    TRKExceptionInfo info;
    u32 instruction;
    u32 length;

    TRK_memset(&info, 0, sizeof(info));
    info.length = 0x40;
    info.command = 0x91;
    info.srr0 = gTRKExceptionStatus.srr0;
    length = 4;
    TRKTargetAccessMemory(&instruction, gTRKExceptionStatus.srr0, &length, 0, 1);
    info.instruction = instruction;
    info.exception_id = gTRKExceptionStatus.exception_id;
    return TRKAppendBuffer_ui8(message, &info, sizeof(info));
}

static inline void TRKTargetArmStep(void)
{
    gTRKStepState.active = 1;
    gTRKCPUState.srr1 = (gTRKCPUState.srr1 | 0x400) & ~0x8000;
    if (gTRKStepState.kind == 0 || gTRKStepState.kind == 0x10) {
        gTRKStepState.count--;
    }
    TRKTargetSetStopped(0);
}

s32 TRKTargetCheckStep(void)
{
    s32 done;

    if (gTRKStepState.active != 0) {
        done = 1;
        gTRKCPUState.srr1 = (gTRKCPUState.srr1 & ~0x400) | 0x8000;
        if (gTRKStepState.active != 0 && (gTRKCPUState.exception_id & 0xFFFF) == 0xD00) {
            switch (gTRKStepState.kind) {
            case 0:
                if (gTRKStepState.count != 0) {
                    done = 0;
                }
                break;
            case 1:
                if (gTRKCPUState.pc >= gTRKStepState.range_start && gTRKCPUState.pc <= gTRKStepState.range_end) {
                    done = 0;
                }
                break;
            }
        }
        if (done != 0) {
            gTRKStepState.active = 0;
        } else {
            TRKTargetArmStep();
        }
    }
    return gTRKStepState.active;
}

s32 TRKTargetSingleStep(u32 count, s32 step_over)
{
    if (step_over != 0) {
        return 0x703;
    }
    gTRKStepState.kind = 0;
    gTRKStepState.count = count;
    TRKTargetArmStep();
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
    TRKTargetArmStep();
    return 0;
}

u32 TRKTargetGetPC(void)
{
    return gTRKCPUState.pc;
}

s32 TRKTargetSupportRequest(void)
{
    u32 io_result;
    s32 status;
    TRKEvent event;
    s32 position;
    u32* count;
    s32 request = gTRKCPUState.gpr[3];

    switch (request) {
    default:
        TRKConstructEvent(&event, TRK_EVENT_EXCEPTION);
        TRKPostEvent(&event);
        return 0;
    case 0xD2:
        status = TRKSuppOpenFile((const char*)gTRKCPUState.gpr[4], gTRKCPUState.gpr[5], (u32*)gTRKCPUState.gpr[6], &io_result);
        if (io_result == 0 && status != 0) {
            io_result = 1;
        }
        gTRKCPUState.gpr[3] = io_result;
        break;
    case 0xD3:
        status = TRKSuppCloseFile(gTRKCPUState.gpr[4], &io_result);
        if (io_result == 0 && status != 0) {
            io_result = 1;
        }
        gTRKCPUState.gpr[3] = io_result;
        break;
    case 0xD4:
        position = *(s32*)gTRKCPUState.gpr[5];
        status = TRKSuppPositionFile(gTRKCPUState.gpr[4], &position, gTRKCPUState.gpr[6], &io_result);
        if (io_result == 0 && status != 0) {
            io_result = 1;
        }
        gTRKCPUState.gpr[3] = io_result;
        *(s32*)gTRKCPUState.gpr[5] = position;
        break;
    case 0xD0:
    case 0xD1:
        count = (u32*)gTRKCPUState.gpr[5];
        status = TRKSuppAccessFile(gTRKCPUState.gpr[4], (u8*)gTRKCPUState.gpr[6], count, &io_result, 1, request == 0xD1);
        if (io_result == 0 && status != 0) {
            io_result = 1;
        }
        gTRKCPUState.gpr[3] = io_result;
        if (request == 0xD1) {
            TRK_flush_cache((void*)gTRKCPUState.gpr[6], *count);
        }
        break;
    }
    gTRKCPUState.pc += 4;
    return status;
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

/* Reads (`is_read`) or writes the special-purpose register `spr` through `data`. */
static void TRKTargetAccessSPR(u32* data, u32 spr, s32 is_read)
{
    u32 instructions[10] = {
        0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
        0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
    };

    if (is_read != 0) {
        instructions[0] = (0x7C800000 | ((spr << 6) & 0x3F800)) | (((spr << 16) & 0x1F0000) | 0x2A6);
        instructions[1] = 0x90830000;
    } else {
        instructions[0] = 0x80830000;
        instructions[1] = (0x7C800000 | ((spr << 6) & 0x3F800)) | (((spr << 16) & 0x1F0000) | 0x3A6);
    }
    TRKPPCAccessSpecialReg(data, instructions, is_read);
}

/* Reads (`is_read`) or writes the paired-single register `reg` through `data`. */
static s32 TRKTargetAccessPairedSingle(u64* data, u32 reg, s32 is_read)
{
    u32 instructions[10] = {
        0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
        0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
    };

    instructions[0] = is_read != 0 ? ((reg << 21) | 0xF0030000) : ((reg << 21) | 0xE0030000);
    return TRKPPCAccessSpecialReg((u32*)data, instructions, is_read);
}

asm void ReadFPSCR(u64* value)
{
    nofralloc
    stwu r1, -64(r1)
    stfd f31, 16(r1)
    psq_st f31, 32(r1), 0, 0
    mffs f31
    stfd f31, 0(r3)
    psq_l f31, 32(r1), 0, 0
    lfd f31, 16(r1)
    addi r1, r1, 64
    blr
}

asm void WriteFPSCR(u64* value)
{
    nofralloc
    stwu r1, -64(r1)
    stfd f31, 16(r1)
    psq_st f31, 32(r1), 0, 0
    lfd f31, 0(r3)
    mtfsf 255, f31
    psq_l f31, 32(r1), 0, 0
    lfd f31, 16(r1)
    addi r1, r1, 64
    blr
}

/* Reads (`is_read`) or writes floating-point register `reg` (0-31), the FPSCR (32) or the FPECR (33) through `data`. */
static s32 TRKTargetAccessFPRegister(u64* data, u32 reg, s32 is_read)
{
    u32 instructions[10] = {
        0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
        0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
    };
    s32 err = 0;
    u32* words = (u32*)data;

    if (reg < 32) {
        u32 opcode = (reg << 21) | 0xC8030000;
        if (is_read != 0) {
            opcode = (reg << 21) | 0xD8030000;
        }
        instructions[0] = opcode;
        err = TRKPPCAccessSpecialReg(words, instructions, is_read);
    } else if (reg == 32) {
        if (is_read != 0) {
            ReadFPSCR(data);
        } else {
            WriteFPSCR(data);
        }
        *data &= 0xFFFFFFFFULL;
    } else if (reg == 33) {
        if (is_read == 0) {
            words[0] = words[1];
        }
        {
            u32 instructions2[10] = {
                0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
                0x60000000, 0x60000000, 0x60000000, 0x60000000, 0x60000000,
            };
            if (is_read != 0) {
                instructions2[0] = 0x7C9EFAA6;
                instructions2[1] = 0x90830000;
            } else {
                instructions2[0] = 0x80830000;
                instructions2[1] = 0x7C9EFBA6;
            }
            err = TRKPPCAccessSpecialReg(words, instructions2, is_read);
        }
        if (is_read != 0) {
            *data = words[0];
            *data &= 0xFFFFFFFFULL;
        }
    }
    return err;
}

s32 TRKPPCAccessSpecialReg(u32* data, u32* instructions, s32 is_read)
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

u32* ConvertAddress(u32 address)
{
    return (u32*)(address | 0x80000000);
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
