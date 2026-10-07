/*
 * TRK/dolphin_trk.c - the Dolphin/Revolution TRK start-up: `InitMetroTRK`, `InitMetroTRK_BBA`, the comm table set-
 *    up, the EXI UART wrappers and the program-end trap.
 *
 * RANGE. .text 0x80468D4C..0x8046940C (20 functions in the map, 0x6C0 B); .data 0x8060F5A8..0x8060F6D0; .bss
 *    0x806F5540..0x806F5568; .sbss 0x80794E48..0x80794E58; .sdata2 0x8079CF40..0x8079CF48.
 * FLAGS. the `OS` lib's `cflags_os` plus `-str reuse,pool` (configure.py `extra_cflags`: the target packs the six strings
 *    contiguously behind `@stringBase0`, with the 4 B `%s` format inside the pool; plain `-str reuse` aligns each string and
 *    puts the `%s` format in `.sdata`); `#pragma use_lmw_stmw on` (the vector copy loop saves r22..r31 with stmw).
 * NAMES. file name GUESS (MetroTRK `dolphin_trk`); `TRKTranslateAddress`, `TRKOverrideInterruptVectors` (caller
 *    TRKDoOverride), `EnableEXI2Interrupts`, `gTRKInterruptVectorOffsets`, `gTRKLockedCache` and `gTRKProgramEndTrap`
 *    are GUESS (the map rows were fn_/lbl_ names): the translate function maps a low-memory offset to the cached or
 *    uncached view unless the locked-cache BAT is valid; the vector table lists the 15 exception offsets; the 8 B
 *    `.sbss` object holds the locked cache base (stored 0xE0000000 by `TRKInitializeTarget`), its second word is never
 *    referenced; the 8 B `.sdata2` object is the 0x00454E44 trap word copied over `PPCHalt`. `DBCommTable` and its slot
 *    names follow the `gdev_cc_*` functions the table is filled with.
 * EVIDENCE. `.data` 0x8060F5A8 (vector table, read by one function), the string base 0x8060F5E8 (read by
 *    `InitMetroTRKCommTable` and `TRK_board_display`), `.sbss` 0x80794E48 / 0x80794E50 and `.bss` 0x806F5540
 *    (`gDBCommTable`, read by the whole run); `.sdata2` 0x8079CF40 is read only by the program-end trap, closing
 *    the pool.
 * RESIDUALS. `InitMetroTRK` reads 97.3: the trailing `blr` the asm functions end with is a 4 B gap row in the target, inside the
 *    row here (bytes and section size match). `TRKTranslateAddress` 85.5 and `TRKOverrideInterruptVectors` 92.0: the
 *    target tests `>= 0x10000000` and `< 0x1C000000` as two compares; every spelling measured (`&&`, `||`, nested, sequential
 *    returns, `<= 0x1BFFFFFF`) either merges them into the `subis`/`subi` range check or hoists the conversion above the
 *    second compare (sequential returns: Translate 76.5, Override 95.0).
 * SHAPES. `InitMetroTRK`, `InitMetroTRK_BBA` and `TRKLoadContext` are `nofralloc` assembly (the register image is
 *    stored and reloaded by hand); the rest is C.
 */
#include "TRK/dolphin_trk.h"
#include "TRK/gdev_cc.h"
#include "TRK/main_TRK.h"
#include "TRK/mem_TRK.h"
#include "TRK/TRK_flush_cache.h"
#include "TRK/targcont.h"
#include "TRK/targimpl.h"
#include "OS/OSCache.h"
#include "OS/OSError.h"
#include "OS/OSReset.h"
#include "OS/OSThread.h"
#include "OS/PPCHalt.h"
#include "Runtime.PPCEABI.H/TRK_interrupt_vectors.h"

#pragma use_lmw_stmw on

#define TRK_HARDWARE_NDEV 1
#define TRK_HARDWARE_BBA 2
#define TRK_VECTOR_COUNT 15
#define TRK_VECTOR_SIZE 0x100
#define TRK_EXTERNAL_INTERRUPT_VECTOR 4
#define TRK_DEBUG_EXCEPTION_MASK_OFFSET 0x44

/* size: 0x28 - the debugger channel's function table, filled by `InitMetroTRKCommTable`. */
typedef struct DBCommTable {
    /* +0x00 */ s32 (*initialize)(u32* inputPendingPtrRef, void (*handler)(void));
    /* +0x04 */ s32 (*initinterrupts)(void);
    /* +0x08 */ s32 (*shutdown)(void);
    /* +0x0C */ s32 (*peek)(void);
    /* +0x10 */ s32 (*read)(u8* dst, s32 len);
    /* +0x14 */ s32 (*write)(const u8* src, s32 len);
    /* +0x18 */ s32 (*open)(void);
    /* +0x1C */ s32 (*close)(void);
    /* +0x20 */ s32 (*pre_continue)(void);
    /* +0x24 */ s32 (*post_stop)(void);
} DBCommTable; /* size: 0x28 */

/* size: 0x8 - the locked cache window the address translation tests against. */
typedef struct TRKLockedCache {
    /* +0x0 */ u32 base;          /* start of the 0x4000 B window, set by TRKInitializeTarget */
    /* +0x4 */ u32 unused_0x04;   /* never read or written */
} TRKLockedCache; /* size: 0x8 */

/* size: 0x8 - the trap word `InitializeProgramEndTrap` copies over the halt routine's second instruction. */
typedef struct TRKProgramEndTrap {
    /* +0x0 */ u32 instruction;   /* 0x00454E44, an illegal opcode */
    /* +0x4 */ u32 unused_0x04;   /* never read */
} TRKProgramEndTrap; /* size: 0x8 */

extern u8 _db_stack_addr[];

u32 gTRKInterruptVectorOffsets[16] = {
    0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0x900, 0xC00, 0xD00, 0xF00, 0x1300, 0x1400, 0x1700, 0
};

DBCommTable gDBCommTable;
TRKLockedCache gTRKLockedCache;
u8 TRK_Use_BBA;
const TRKProgramEndTrap gTRKProgramEndTrap = { 0x00454E44, 0 };

asm void InitMetroTRK(void)
{
    nofralloc
    addi r1, r1, -4
    stw r3, 0(r1)
    lis r3, gTRKCPUState@h
    ori r3, r3, gTRKCPUState@l
    stmw r0, 0(r3)
    lwz r4, 0(r1)
    addi r1, r1, 4
    stw r1, 4(r3)
    stw r4, 12(r3)
    mflr r4
    stw r4, 132(r3)
    stw r4, 128(r3)
    mfcr r4
    stw r4, 136(r3)
    mfmsr r4
    ori r3, r4, 0x8000
    xori r3, r3, 0x8000
    mtmsr r3
    mtspr 27, r4
    bl TRKSaveExtended1Block
    lis r3, gTRKCPUState@h
    ori r3, r3, gTRKCPUState@l
    lmw r0, 0(r3)
    li r0, 0
    mtspr 1010, r0
    mtspr 1013, r0
    lis r1, _db_stack_addr@h
    ori r1, r1, _db_stack_addr@l
    mr r3, r5
    bl InitMetroTRKCommTable
    cmpwi r3, 1
    bne fail
    lwz r4, 132(r3)
    mtlr r4
    lmw r0, 0(r3)
    blr
fail:
    b TRK_main
    blr
}

asm void InitMetroTRK_BBA(void)
{
    nofralloc
    addi r1, r1, -4
    stw r3, 0(r1)
    lis r3, gTRKCPUState@h
    ori r3, r3, gTRKCPUState@l
    stmw r0, 0(r3)
    lwz r4, 0(r1)
    addi r1, r1, 4
    stw r1, 4(r3)
    stw r4, 12(r3)
    mflr r4
    stw r4, 132(r3)
    stw r4, 128(r3)
    mfcr r4
    stw r4, 136(r3)
    mfmsr r4
    ori r3, r4, 0x8000
    mtmsr r3
    mtspr 27, r4
    bl TRKSaveExtended1Block
    lis r3, gTRKCPUState@h
    ori r3, r3, gTRKCPUState@l
    lmw r0, 0(r3)
    li r0, 0
    mtspr 1010, r0
    mtspr 1013, r0
    lis r1, _db_stack_addr@h
    ori r1, r1, _db_stack_addr@l
    li r3, 2
    bl InitMetroTRKCommTable
    cmpwi r3, 1
    bne fail
    lwz r4, 132(r3)
    mtlr r4
    lmw r0, 0(r3)
    blr
fail:
    b TRK_main
    blr
}

void EnableMetroTRKInterrupts(void)
{
    EnableEXI2Interrupts();
}

u32 TRKTranslateAddress(u32 address)
{
    if (address >= gTRKLockedCache.base && address < gTRKLockedCache.base + 0x4000
        && (gTRKCPUState.dbat3_upper & 3) != 0) {
        return address;
    }
    if (address < 0x3000000) {
        return (address & 0x3FFFFFFF) | 0x80000000;
    }
    if (address >= 0x10000000 && address < 0x1C000000) {
        return (address & 0x3FFFFFFF) | 0x90000000;
    }
    return address;
}

void TRKOverrideInterruptVectors(void)
{
    s32 i;
    u32 exception_mask = *(u32*)TRKTranslateAddress(TRK_DEBUG_EXCEPTION_MASK_OFFSET);

    for (i = 0; i <= TRK_VECTOR_COUNT - 1; i++) {
        if ((exception_mask & (1 << i)) != 0 && i != TRK_EXTERNAL_INTERRUPT_VECTOR) {
            u32 offset = gTRKInterruptVectorOffsets[i];
            u32 destination = TRKTranslateAddress(offset);

            TRK_memcpy((void*)destination, (u8*)gTRKInterruptVectorTable + offset, TRK_VECTOR_SIZE);
            TRK_flush_cache((void*)destination, TRK_VECTOR_SIZE);
        }
    }
}

s32 TRKInitializeTarget(void)
{
    gTRKState.stopped = 1;
    gTRKState.msr = __TRK_get_MSR();
    gTRKLockedCache.base = 0xE0000000;
    return 0;
}

void __TRKreset(void)
{
    OSResetSystem(0, 0, 0);
}

asm void TRKLoadContext(TRKCPUState* context, u32 vector)
{
    nofralloc
    lwz r0, 0(r3)
    lwz r1, 4(r3)
    lwz r2, 8(r3)
    lhz r5, 418(r3)
    rlwinm. r6, r5, 0, 30, 30
    beq skip_volatile
    rlwinm r5, r5, 0, 31, 29
    sth r5, 418(r3)
    lmw r5, 20(r3)
    b restore_rest
skip_volatile:
    lmw r13, 52(r3)
restore_rest:
    mr r31, r3
    mr r3, r4
    lwz r4, 128(r31)
    mtcrf 0xFF, r4
    lwz r4, 132(r31)
    mtlr r4
    lwz r4, 136(r31)
    mtctr r4
    lwz r4, 140(r31)
    mtxer r4
    mfmsr r4
    rlwinm r4, r4, 0, 17, 15
    rlwinm r4, r4, 0, 31, 29
    mtmsr r4
    mtspr 273, r2
    lwz r4, 12(r31)
    mtspr 274, r4
    lwz r4, 16(r31)
    mtspr 275, r4
    lwz r2, 408(r31)
    lwz r4, 412(r31)
    lwz r31, 124(r31)
    b TRKInterruptHandler
}

void TRKEXICallBack(s16 unused, TRKCPUState* context)
{
    OSEnableScheduler();
    TRKLoadContext(context, 0x500);
}

s32 InitMetroTRKCommTable(s32 hardware)
{
    OSReport("Devkit set to : %ld\n", hardware);
    OSReport("MetroTRK : Sizeof Reply - %ld bytes\n", 64);
    TRK_Use_BBA = 0;
    if (hardware == TRK_HARDWARE_BBA) {
        return 0;
    }
    if (hardware == TRK_HARDWARE_NDEV) {
        OSReport("MetroTRK : Set to NDEV hardware\n");
        gDBCommTable.initialize = gdev_cc_initialize;
        gDBCommTable.open = gdev_cc_open;
        gDBCommTable.close = gdev_cc_close;
        gDBCommTable.read = gdev_cc_read;
        gDBCommTable.write = gdev_cc_write;
        gDBCommTable.shutdown = gdev_cc_shutdown;
        gDBCommTable.peek = gdev_cc_peek;
        gDBCommTable.pre_continue = gdev_cc_pre_continue;
        gDBCommTable.post_stop = gdev_cc_post_stop;
        gDBCommTable.initinterrupts = gdev_cc_initinterrupts;
        return 0;
    }
    OSReport("MetroTRK : Set to UNKNOWN hardware. (%ld)\n", hardware);
    OSReport("MetroTRK : Invalid hardware ID passed from OS\n");
    OSReport("MetroTRK : Defaulting to GDEV Hardware\n");
    return 1;
}

void TRKUARTInterruptHandler(void)
{
}

s32 TRKInitializeIntDrivenUART(u32 mode, u32 option, u8** inputPendingPtrRef)
{
    gDBCommTable.initialize((u32*)inputPendingPtrRef, (void (*)(void))TRKEXICallBack);
    gDBCommTable.open();
    return 0;
}

void EnableEXI2Interrupts(void)
{
    if (TRK_Use_BBA == 0) {
        if (gDBCommTable.initinterrupts != 0) {
            gDBCommTable.initinterrupts();
        }
    }
}

s32 TRKPollUART(void)
{
    return gDBCommTable.peek();
}

/* untyped: byte range */
s32 TRKReadUARTN(void* bytes, u32 length)
{
    return gDBCommTable.read(bytes, length) != 0 ? -1 : 0;
}

/* untyped: byte range */
s32 TRK_WriteUARTN(const void* buffer, u32 size)
{
    return gDBCommTable.write(buffer, size) != 0 ? -1 : 0;
}

void ReserveEXI2Port(void)
{
    gDBCommTable.post_stop();
}

void UnreserveEXI2Port(void)
{
    gDBCommTable.pre_continue();
}

void TRK_board_display(const char* message)
{
    OSReport("%s\n", message);
}

void InitializeProgramEndTrap(void)
{
    u32* halt = (u32*)PPCHalt;

    TRK_memcpy(halt + 1, &gTRKProgramEndTrap, 4);
    ICInvalidateRange(halt + 1, 4);
    DCFlushRange(halt + 1, 4);
}
