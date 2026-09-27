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
 * SECTION.  `.text` only.  The unit's data references (the `.data` jumptable at 0x8062FF90, the
 * `.bss`/`.sbss`/`.sdata` objects) are declared, never defined, so nothing is claimed here - dtk names
 * an unclaimed relocation target from the map, and playbook 23 measures a claim before and after.
 *
 * FLAGS (open question, reported).  The whole 0x80507C40..0x80512490 band has every function start
 * 16-byte aligned (152/152) with 4/8/12-byte zero `gap_*` runs between neighbours, which is the
 * `-func_align 16` layout; the sibling `DWCi_sendControlFrame` unit is registered under `cflags_dwc`'s
 * `-func_align 4`.  This unit copies `cflags_dwc` (the task's instruction: a copied flag is safe, a
 * changed flag needs its own evidence); the 16-alignment is recorded for the flip pass, where it is
 * the difference between an object that links at the right address and one that does not.
 *
 * NAMING (rule 7).  Six of the 15 functions have bodies here and are named from their code, callers
 * and the fields they touch (each is a GUESS - the runtime dump answers `zz_0512xxx_` for the whole
 * band, so there is no map name to recover; the unit header records that):
 *   0x805091F0 -> DWCi_GetStatus       reads the first word of the DWCi state block DWCi_stateBlock
 *   0x805091D0 -> DWCi_IsStatusReady   that word == 1
 *   0x80509200 -> DWCi_GetWorkBuffer   address of the 0x190-B DWCi work buffer DWCi_workBuffer
 *   0x80509180 -> DWCi_AdvanceStatus   the 0x19 -> 0x1A state ladder over DWCi_state
 *   0x80509250 -> DWCi_SetResult       publishes flag + value into DWCi_runtime
 *   0x80508750 -> DWCi_FreeList        drains the DWCi_freeListHead list through DWCi_freeNode
 * `DWCi_Np_CPUCopyFast` itself and the other eight keep their map names (they have no body here).
 *
 * The one helper it calls that no registered unit owns, `DWCi_freeNode` (0x805076F0, the middle band
 * 0x80509DB0..0x805113B0), is a GUESS derived from this file's own call site - `DWCi_FreeList`
 * drains its list by calling it once per node with the 0xC kind tag - and is declared in the band
 * header `include/unsplit/DWCi.h`; it is a marker for a later reconstruction to confirm.
 *
 * BODY (probe).  Six of the 15 functions, chosen as the small ones the disassembly pins.  All six are
 * byte-identical at 100 %.
 */

#include "types.h"
#include "unsplit/DWCi.h"   /* the band's unowned data and helpers (rule 2) */

/* The DWCi runtime block `DWCi_runtime` (declared in `include/unsplit/DWCi.h`) points at; only the
 * two result words the state machine publishes are modelled. size: 0x59C0 */
typedef struct DWCiRuntime {
    /* +0x0000 */ u8 pad_0x0000[0x59B8];
    /* +0x59B8 */ u32 field_0x59B8;
    /* +0x59BC */ u32 field_0x59BC;
} DWCiRuntime;

/* A node of the DWCi list `DWCi_FreeList` drains; the link sits at +0x18. size: 0x1C */
typedef struct DWCiListNode {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ struct DWCiListNode* next;
} DWCiListNode;


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
    DWCi_runtime->field_0x59B8 = 1;
    DWCi_runtime->field_0x59BC = arg;
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
