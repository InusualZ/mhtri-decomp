/*
 * DWCi_Np_CPUCopyFast.c - the Nintendo Wi-Fi Connection (DWCi) SDK band, `.text` 0x80507C40..0x80509DB0.
 *
 * REGISTRATION (recon lane, 2026-09-27).  One `DWCi` unit for the named function `DWCi_Np_CPUCopyFast`
 * (0x80507C40, 0x9E4 B) and its 14 following neighbours (15 functions, 8560 B).  Right edge is a
 * STRONG cut: `tudiscover.py at 0x80507C40` reports `cut 18505 0x80509DB0 strong x2`
 * (.sdata run jump 0x807942D8 -> 0x807942FC and 0x80794328 -> 0x80794330; the second admits
 * only cuts in [18505,18508], and the first pins 18505).  The left edge at 0x80507C40 is a weak cut
 * (only the .data jumptable run jump jumptable_8062FD00 -> jumptable_8062FF90, a 14-function interval)
 * - 0x80507C40 is taken because it is the named symbol's own (16-aligned) start.  The function
 * `0x805078F0` immediately below it ends at 0x80507C38.
 *
 * SECTIONS.  `.text` plus two of the band's data objects, claimed 2026-09-28 so that the range the
 * unit's own code stores into is the unit's (rule 12; the declarations lived in
 * `include/unsplit/DWCi.h`, which is rule 12's finding):
 *   .sbss 0x807957D0..0x807957F8 - the state machine's words: the free-list head, the runtime/result
 *     block and the state-ladder word.  Every symbol in the run is stored by *this* object
 *     (`DWCi_FreeList`, `DWCi_SetResult`, `DWCi_AdvanceStatus`) and by no other registered unit, so
 *     the definer test - a store, not a load (`DWCi_runtime` 57 loads, 2 stores; `DWCi_state` 28
 *     stores) - picks this unit.  0x807957F8 (not this unit's) is the first symbol past the run.
 *   .sdata 0x80794200..0x80794210 - the word `fn_80508A70` loads and hands to its
 *     `NANDPrivateOpenAsync`/`NANDPrivateDeleteAsync` calls as the session's path, then the two
 *     all-zero name strings the opener is handed (0x80794204/0x80794208) and the 3-byte "//" at
 *     0x8079420C.  The four object bytes at 0x80794200 read zero only because the word is a
 *     *relocation*: the target object's single `.rela.sdata` entry is an `R_PPC_ADDR32` to
 *     `lbl_80630020`, the "/shared2/DWC_AUTHDATA" path string.  All four words are read only here.
 * Both boundaries are symbol-aligned and 8-byte aligned, which `dtk dol split` requires of a claim
 * (an 8-misaligned boundary makes it die with `Invalid alignment for split`).  One run per section:
 * a second run whose gap no registered unit owns makes the split die with a link-order cycle
 * (playbook 53), so nothing else here is claimed.  Still unowned and declared in the band header:
 * the `.data` jumptable at 0x8062FF90, `DWCi_protocolMagic`, `DWCi_addressFormat*`, the digit class
 * table, `DWCi_addressRingIndex`, `DWCi_stateBlock` and `DWCi_workBuffer` - all read here but stored
 * elsewhere or nowhere.  `DWCi_addressRing` (0x807625C0) is *not* in that list: the word falls
 * inside the NATNEG unit's `.bss` run, so rule 2 declares it in `include/DWCi/DWCi_NatNeg.h`,
 * which this file reaches through the band header.
 *
 * FLAGS (open question, reported).  The whole 0x80507C40..0x80512490 band has every function start
 * 16-byte aligned (152/152) with 4/8/12-byte zero `gap_*` runs between neighbours, which is the
 * `-func_align 16` layout; the sibling `DWCi_sendControlFrame` unit is registered under `cflags_dwc`'s
 * `-func_align 4`.  This unit copies `cflags_dwc` (the task's instruction: a copied flag is safe, a
 * changed flag needs its own evidence); the 16-alignment is recorded for the flip pass, where it is
 * the difference between an object that links at the right address and one that does not.
 *
 * NAMING (rule 7, no exemption).  The unit owns 15 functions; ten have bodies here and are named
 * from their code, callers and the fields they touch (each is a GUESS - the runtime dump answers
 * `zz_0512xxx_` for the whole band, so there is no map name to recover; the unit header records
 * that):
 *   0x805091F0 -> DWCi_GetStatus       reads the first word of the DWCi state block DWCi_stateBlock
 *   0x805091D0 -> DWCi_IsStatusReady   that word == 1
 *   0x80509200 -> DWCi_GetWorkBuffer   address of the 0x190-B DWCi work buffer DWCi_workBuffer
 *   0x80509180 -> DWCi_AdvanceStatus   the 0x19 -> 0x1A state ladder over DWCi_state
 *   0x80509250 -> DWCi_SetResult       publishes flag + value into DWCi_runtime
 *   0x80508750 -> DWCi_FreeList        drains the DWCi_freeListHead list through DWCi_freeNode
 *   0x805087A0 -> DWCi_npStart         the reset a session begins with: clears the state block and
 *                                      its two trailing regions, DWCi_state / DWCi_runtime, and
 *                                      remembers the argument in DWCi_initArgument
 *   0x805089F0 -> DWCi_npSetup         opens the session: calls the host callback below with the two
 *                                      argument words, zeroes the work buffer, copies the 5-byte tag
 *   0x80509210 -> DWCi_npSetValue      issues the runtime block's command 13 with its argument
 *   0x80509230 -> DWCi_npSetValueEx    the three-argument form of that same command
 * One row is named from a call site instead of a body: 0x80508830 -> `DWCi_initRuntime`, the
 * six-argument host callback `DWCi_npSetup` invokes with the band's two empty-name defaults.
 * Criterion, stated once for both DWCi units: no auto-generated name is spelled in source, and a row
 * that no written body reaches keeps its stem - here `fn_80508630`, `fn_80508A70` and `fn_80509270`;
 * the unit's own row `DWCi_Np_CPUCopyFast` carries the map's name and has no body yet either.
 *
 * The one helper it calls that no registered unit owns, `DWCi_freeNode` (0x805076F0, the middle band
 * 0x80509DB0..0x805113B0), is a GUESS derived from this file's own call site - `DWCi_FreeList`
 * drains its list by calling it once per node with the 0xC kind tag - and is declared in the band
 * header `include/unsplit/DWCi.h`; it is a marker for a later reconstruction to confirm.
 *
 * BODIES (this pass).  Ten of the 15 functions are written; the unit is 6.2027 % over its 8464 B of
 * `.text` (was 2.5992 %).  Byte-identical (100 %): `DWCi_FreeList`, `DWCi_AdvanceStatus`,
 * `DWCi_IsStatusReady`, `DWCi_GetStatus`, `DWCi_GetWorkBuffer`, `DWCi_SetResult`, `DWCi_npSetValue`,
 * `DWCi_npSetValueEx`.
 *   DWCi_npSetup  99.6774  124 B
 *   DWCi_npStart  95.1471  136 B
 * Residuals: `DWCi_npSetup` is one instruction off - the host callback takes six arguments, and
 * only that arity reproduces retail's `mr r7,r4`/`mr r8,r5` argument shuffles; `DWCi_npStart`'s
 * frame is 0x10 and settles one instruction short (`memset(state, ...)` gives `mr r3,r31` where
 * retail's `addi r3,r31,0` is the same address written as a computation).  Unwritten, with sizes:
 * `fn_80508630` (0x118), `DWCi_initRuntime` (0x1B8), `fn_80508A70` (0x708), `DWCi_Np_CPUCopyFast`
 * (0x9E4), `fn_80509270` (0xB40) - the three large ones are the DWCi save/HTTP state machine and
 * the two `DWCi_*_Fast` copy loops, and `DWCi_initRuntime` is the runtime-block initialiser whose
 * callee graph (`fn_8051C554`, `fn_8051A4E8`, `fn_805078F0`) is entirely outside this unit.
 * DATA still unclaimed, same reason as `DWCi_NatNeg.c`: the bodies that emit `.sdata`/`.bss`/`.sbss`
 * are not all written, so a claim would move every un-emitted object (playbook 23/58).
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"    /* memset: the runtime unit's header (rule 2) */
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers, and - through it - */
                                         /* the two owners' headers for the data they now own       */
#include "unsplit/Runtime.PPCEABI.H.h"   /* strncpy */

/* The DWCi runtime block `DWCi_runtime` (this unit's own object, declared in
 * `include/DWCi/DWCi_Np_CPUCopyFast.h`) points at.  Only the
 * fields this unit's bodies read or write are named; every other offset is an unmodelled run of the
 * original's own layout, so the size is the last named field's end and a lower bound.  The block is
 * handed out by the host callback `DWCi_initRuntime` registers (`DWCi_runtime->allocCallback`),
 * which is why its size (0x5B30) is the callback's argument there. size: >= 0x5B1C */
typedef struct DWCiRuntime {
    /* +0x0000 */ u8 pad_0x0000[0x419E];
    /* +0x419E */ char tag_0x419E[5];   /* the 5-byte tag DWCi_npSetup copies; no terminator is written */
    /* +0x41A3 */ u8 pad_0x41A3[0x1815];
    /* +0x59B8 */ u32 resultFlag;       /* DWCi_SetResult publishes a pending result here */
    /* +0x59BC */ u32 resultValue;
    /* +0x59C0 */ u8 pad_0x59C0[0x08];
    /* +0x59C8 */ u32 mode_0x59C8;      /* 2 while DWCi_npSetup's session is open */
    /* +0x59CC */ u8 pad_0x59CC[0x148];
    /* +0x5B14 */ struct DWCiRuntime* (*allocCallback)(u32 command, u32 size);
    /* +0x5B18 */ void (*commandCallback)(u32 command, u32 arg1, u32 arg2);
} DWCiRuntime; /* size: 0x5B1C */

/* A node of the DWCi list `DWCi_FreeList` drains; the link sits at +0x18. size: 0x1C */
typedef struct DWCiListNode {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ struct DWCiListNode* next;
} DWCiListNode;

/* The DWCi state block `DWCi_stateBlock` (declared `u32[]` in `include/unsplit/DWCi.h` because its
 * unsized form is what makes MWCC address it absolutely), as `DWCi_npStart` clears it: three
 * regions zeroed in place and one count word cleared.  The regions' own contents belong to the
 * band's helpers, so they stay unnamed runs. size: >= 0x380 */
/* This unit's own entry points, in address order. */
u32 DWCi_npStart(u32 arg);
u32 DWCi_npSetup(char* tag, u32 arg1, u32 arg2);
void DWCi_npSetValue(u32 value);
void DWCi_npSetValueEx(u32 value);
void DWCi_initRuntime(char* playerName, char* friendCode, u32 a, u32 b, u32 c, u32 d);

typedef struct DWCiStateBlock {
    /* +0x000 */ u8 pad_0x000[0x1B8];
    /* +0x1B8 */ u32 cleared_0x1B8;    /* the count word DWCi_npStart clears on its own */
    /* +0x1BC */ u8 pad_0x1BC[0x14];
    /* +0x1D0 */ u8 region_0x1D0[0x174];
    /* +0x344 */ u8 pad_0x344[0x1C];
    /* +0x360 */ u8 region_0x360[0x20];
} DWCiStateBlock; /* size: 0x380 */


/* 0x805091F0 (0xC): the DWCi state word. */
u32 DWCi_GetStatus(void) {
    return DWCi_stateBlock[0];
}

/* 0x805091D0 (0x18): `state == 1`. */
u32 DWCi_IsStatusReady(void) {
    return DWCi_stateBlock[0] == 1;
}

/* 0x80509200 (0xC): address of the DWCi work buffer. */
u8* DWCi_GetWorkBuffer(void) {
    return DWCi_workBuffer;
}

/* 0x80509180 (0x44): the state ladder 0x19 -> 0x1A; 1 while the state is 0, 0x1A or just advanced. */
u32 DWCi_AdvanceStatus(void) {
    if (DWCi_state == 0x19) {
        DWCi_state = 0x1A;
        return 1;
    }
    if (DWCi_state == 0 || DWCi_state == 0x1A) {
        return 1;
    }
    return 0;
}

/* 0x80509250 (0x18): publish a flag + the argument into the runtime block. */
void DWCi_SetResult(u32 arg) {
    DWCi_runtime->resultFlag = 1;
    DWCi_runtime->resultValue = arg;
}

/* 0x80508750 (0x50): drain the request list, calling DWCi_freeNode(0xC, node, 0) per node. */
u32 DWCi_FreeList(void) {
    DWCiListNode* node = DWCi_freeListHead;
    while (node != 0) {
        DWCiListNode* next = node->next;
        DWCi_freeNode(0xC, node, 0);
        DWCi_freeListHead = next;
        node = next;
    }
    return 0;
}

/* 0x805087A0 (0x88): clear the whole DWCi state block and its trailing regions, drop the runtime
 * block, remember the argument and return 1.  The reset every session begins with. */
u32 DWCi_npStart(u32 arg) {
    DWCiStateBlock* state = (DWCiStateBlock*)DWCi_stateBlock;

    memset(state, 0, 0x1D0);
    memset(state->region_0x1D0, 0, 0x174);
    memset(state->region_0x360, 0, 0x20);
    DWCi_runtime = 0;
    DWCi_state = 0;
    DWCi_initArgument = arg;
    state->cleared_0x1B8 = 0;
    DWCi_useStoredConfig = 0;
    return 1;
}

/* 0x805089F0 (0x7C): open a session with the caller's two values over the band's two empty-name
 * defaults, clear the work buffer, copy the 5-byte tag into the runtime block and set its mode. */
u32 DWCi_npSetup(char* tag, u32 arg1, u32 arg2) {
    DWCi_initRuntime(DWCi_npEmptyPlayerName, DWCi_npEmptyFriendCode, 0, 0, arg1, arg2);
    memset(DWCi_workBuffer, 0, 0x174);
    strncpy(DWCi_runtime->tag_0x419E, tag, 5);
    DWCi_runtime->mode_0x59C8 = 2;
    return 1;
}

/* 0x80509210 (0x18): issue the host callback's command 13 with `value`. */
void DWCi_npSetValue(u32 value) {
    DWCi_runtime->allocCallback(13, value);
}

/* 0x80509230 (0x1C): the three-argument form of that same command. */
void DWCi_npSetValueEx(u32 value) {
    DWCi_runtime->commandCallback(13, value, 0);
}
