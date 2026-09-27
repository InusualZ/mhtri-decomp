/*
 * DWCi_Np_CPUCopyFast.c - the Nintendo Wi-Fi Connection (DWCi) SDK band, `.text` 0x80507C40..0x80509DB0.
 *
 * REGISTRATION (recon lane, 2026-09-27).  One `DWCi` unit for the named function `DWCi_Np_CPUCopyFast`
 * (0x80507C40, 0x9E4 B) and its 14 following neighbours (15 functions, 8560 B).  Right edge is a
 * STRONG cut: `tudiscover.py at 0x80507C40` reports `cut 18505 0x80509DB0 strong x2`
 * (.sdata run jump lbl_807942D8 -> lbl_807942FC and lbl_80794328 -> lbl_80794330; the second admits
 * only cuts in [18505,18508], and the first pins 18505).  The left edge at 0x80507C40 is a weak cut
 * (only the .data jumptable run jump jumptable_8062FD00 -> jumptable_8062FF90, a 14-function interval)
 * - 0x80507C40 is taken because it is the named symbol's own (16-aligned) start.  The function
 * `fn_805078F0` immediately below it ends at 0x80507C38.
 *
 * SECTION.  `.text` only.  The unit's data references (the `.data` jumptable at 0x8062FF90, the
 * `.bss`/`.sbss`/`.sdata` objects) are declared, never defined, so nothing is claimed here - dtk names
 * an unclaimed relocation target from the map, and playbook 23 measures a claim before and after.
 *
 * FLAGS (open question, reported).  The whole 0x80507C40..0x80512490 band has every function start
 * 16-byte aligned (152/152) with 4/8/12-byte zero `gap_*` runs between neighbours, which is the
 * `-func_align 16` layout; the sibling `DWCi/fn_805113B0.c` is registered under `cflags_dwc`'s
 * `-func_align 4`.  This unit copies `cflags_dwc` (the task's instruction: a copied flag is safe, a
 * changed flag needs its own evidence); the 16-alignment is recorded for the flip pass, where it is
 * the difference between an object that links at the right address and one that does not.
 *
 * NAMING (rule 7).  Six of the 15 functions have bodies here and are named from their code, callers
 * and the fields they touch (each is a GUESS - the runtime dump answers `zz_0512xxx_` for the whole
 * band, so there is no map name to recover; the unit header records that):
 *   fn_805091F0 -> DWCi_GetStatus       reads the first word of the DWCi state block lbl_80760F00
 *   fn_805091D0 -> DWCi_IsStatusReady   that word == 1
 *   fn_80509200 -> DWCi_GetWorkBuffer   address of the 0x190-B DWCi work buffer lbl_807610D0
 *   fn_80509180 -> DWCi_AdvanceStatus   the 0x19 -> 0x1A state ladder over lbl_807957F4
 *   fn_80509250 -> DWCi_SetResult       publishes flag + value into lbl_807957F0
 *   fn_80508750 -> DWCi_FreeList        drains the lbl_807957E0 list through fn_805076F0
 * `DWCi_Np_CPUCopyFast` itself and the other eight keep their map names (they have no body here).
 *
 * rule 7 deferred: the only `fn_` this file still names is `fn_805076F0` (0x805076F0), a *reference* -
 * it sits outside this unit's range (the DWCi middle band 0x80509DB0..0x805113B0) and is owned by
 * another, still-unnamed unit, so naming it here would be a guess about someone else's TU.  Every
 * name this file *defines* is derived above.
 *
 * BODY (probe).  Six of the 15 functions, chosen as the small ones the disassembly pins.  All six are
 * byte-identical at 100 %.
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef signed char    s8;
typedef signed short   s16;
typedef signed int     s32;

/* The DWCi runtime block `lbl_807957F0` points at; only the two result words the state machine
 * publishes are modelled. size: 0x59C0 */
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

extern u32 lbl_80760F00[];        /* .bss - the DWCi state block (0x1D0) */
extern u8  lbl_807610D0[];        /* .bss - the DWCi work buffer (0x190) */
extern DWCiRuntime* lbl_807957F0; /* .sbss - the runtime/result block */
extern DWCiListNode* lbl_807957E0;/* .sbss - the list head DWCi_FreeList drains */
extern volatile s32 lbl_807957F4; /* .sbss - the state-ladder word (retail reloads it) */
extern void fn_805076F0(u32 kind, void* node, u32 arg); /* other unit's (deferred, see header) */

/* 0x805091F0 (0xC): the DWCi state word. */
u32 DWCi_GetStatus(void) {
    return lbl_80760F00[0];
}

/* 0x805091D0 (0x18): `state == 1`. */
u32 DWCi_IsStatusReady(void) {
    return lbl_80760F00[0] == 1;
}

/* 0x80509200 (0xC): address of the DWCi work buffer. */
u8* DWCi_GetWorkBuffer(void) {
    return lbl_807610D0;
}

/* 0x80509180 (0x44): the state ladder 0x19 -> 0x1A; 1 while the state is 0, 0x1A or just advanced. */
u32 DWCi_AdvanceStatus(void) {
    if (lbl_807957F4 == 0x19) {
        lbl_807957F4 = 0x1A;
        return 1;
    }
    if (lbl_807957F4 == 0 || lbl_807957F4 == 0x1A) {
        return 1;
    }
    return 0;
}

/* 0x80509250 (0x18): publish a flag + the argument into the runtime block. */
void DWCi_SetResult(u32 arg) {
    lbl_807957F0->field_0x59B8 = 1;
    lbl_807957F0->field_0x59BC = arg;
}

/* 0x80508750 (0x50): drain the request list, calling fn_805076F0(0xC, node, 0) per node. */
u32 DWCi_FreeList(void) {
    DWCiListNode* node = lbl_807957E0;
    while (node != 0) {
        DWCiListNode* next = node->next;
        fn_805076F0(0xC, node, 0);
        lbl_807957E0 = next;
        node = next;
    }
    return 0;
}
