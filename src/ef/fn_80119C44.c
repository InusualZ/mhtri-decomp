/* auto/80119C44_fn_80119C44.c - the effect-record constructor and its three hooks,
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * `.text` 0x80119C44..0x80119DEC (5 functions, in address order).
 *
 * What it is.  `fn_80119C44` is the constructor of a 0x48-byte effect record (`_EFT`, the same object
 * `auto/800FCED4_fn_800FCED4.cpp` and `auto/800FD520_fn_800FD520.c` drive): it gates on the current
 * area, takes a record from the `fn_800F8788` pool with a 0x18-byte work block attached, seeds the work
 * block's effect count and scale, stamps the type/area, and installs the two handlers that travel with
 * the record - `fn_80119D10` as the pool-release hook at +0x40 and `fn_80119D9C` as the per-frame
 * dispatch at +0x34.  `fn_80119D10` picks between the two identical pool-release bodies
 * (`fn_80119D24`/`fn_80119D60`), and `fn_80119D9C` is the four-state machine that runs the type-2
 * start (`fn_80119DEC`) or the type handler (`fn_8011A2A0`/`fn_8011A34C`/`fn_8011ACF0`/`fn_8011AD00`).
 *
 * Language.  The unit's own symbols are plain (`fn_80119C44`, `fn_80119D10`, ...), so the file is C and
 * the mangled callee is declared by its map spelling - the same finding as `auto/800FD520_fn_800FD520.c`
 * (a mangled *callee* is not evidence of a C++ unit; only a mangled definition or a `.cpp` `__FILE__`
 * string would be).  The `extab`/`extabindex` fragment the object carries comes from `cflags_main`'s
 * `-Cpp_exceptions on`, not from a C++ source.
 *
 * Result.  `fn_80119D10`, `fn_80119D24` and `fn_80119D60` are byte-identical; `.text` (0x1A8), `extab`
 * (0x18) and `extabindex` (0x24) are the target's sizes and all six fragment records pair.  Two
 * residuals remain, both measured (objdiff per-symbol `match_percent`):
 *
 *   * `fn_80119C44` (99.61 %).  The two handler stores materialise the address as
 *     `lis r3, sym@ha; addi r3, r3, sym@l; stw r3, off(r31)` where the target uses **r0** for the `addi`
 *     (`addi r0, r3, sym@l; stw r0, off(r31)`) - the instructions, sizes and relocations are otherwise
 *     identical.  It is not a source shape: the same two assignments in `eft002_set` (C++,
 *     `auto/800FCED4_fn_800FCED4.cpp`) *do* compile to `addi r0` with this flag set, and no spelling of
 *     the assignment here moves it (tried: a typed local, an explicit cast, `void*` fields, a comma
 *     expression, the two stores swapped, the stores moved before the `fn_800F9DF4` call, a nested
 *     block-scoped declaration, `#pragma peephole off`) - so it is the allocator's preference for this
 *     function's IR and belongs to the residual, not to the source.
 *   * `fn_80119D9C` (89.00 %).  The instructions are the target's, in the target's order, and the
 *     function is the target's 0x50 bytes - the difference is **block layout**.  Retail is
 *     `[chain][default blr][case0: b N][case1][case2][case3][exit blr][N: nested dispatch]`, i.e. case
 *     0's block is a bare jump and the nested switch's blocks sit after the function's exit block (its
 *     `break` shares that exit, which is what makes the nested switch a case-0 body).  Every shape that
 *     MWCC will emit for that source puts the nested blocks in case 0's slot instead
 *     (`[chain][default blr][N: nested dispatch][case1][case2][case3][exit blr]`, 54.25 %), so the
 *     landed source is the equivalent **`case 0: break;` + after-switch switch** form, which at least
 *     reproduces the target's size and instruction order and scores 89.00 %; its own residual is a
 *     trailing dead `blr` (the nested switch's private exit) and case 0's merged slot.  The shapes tried
 *     for the retail layout, all measured, are in `.pi/notes/80119c44-fn-80119c44-b297.md` - the nested
 *     switch as a case-0 body with `break`/`return`/neither, with the cases in either source order, with
 *     an explicit `default` before/after, with braces, with an explicit trailing `return`, with a
 *     table-lookup or a `s8` operand, and with `#pragma peephole off`; MWCC never emits the retail
 *     layout for any of them, and the sibling dispatchers of the same family (`fn_800FC428`,
 *     `fn_8011B180`, `fn_80107DDC`) all carry it in retail.
 *
 * Data.  The unit owns no pool section: its one float (`lbl_80796B2C`, 1.0f) lives in a shared
 * `.sdata2` pool, so it is `extern`-declared by its map name and never defined (playbook 29).  The
 * `extab`/`extabindex` fragments travel with the code and are claimed in `splits.txt` with its `.text`.
 *
 * Types.  `_EFT` and `_EFT_HEAP_WORK` are reconstructed minimally (only the offsets this unit touches)
 * and are copies of the neighbours' definitions (`auto/800FCED4_fn_800FCED4.cpp`'s `_EFT`/`_EFT_WORK`);
 * all of them belong in one shared header, which does not exist yet.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80119C44_fn_80119C44.c`.
 */

#include "types.h"

/* ---------------------------------------------------------------------------------------------------
 * the 0x48-byte effect record and its pool block
 * ------------------------------------------------------------------------------------------------- */

struct _EFT_HEAP_WORK;

/* The effect object `fn_800F8788` hands out and the two installed handlers drive. size: 0x48 */
struct _EFT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;   /* the effect type the pool tables are indexed by */
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;  /* the `fn_80119D9C` state index */
    /* +0x06 */ u8 unused_0x06[0x34 - 0x06];
    /* +0x34 */ void (*dispatch_0x34)(struct _EFT*); /* the per-frame state machine */
    /* +0x38 */ struct _EFT_HEAP_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(struct _EFT*);  /* the pool-release hook */
    /* +0x44 */ u8 area_0x44;   /* the area the record was spawned in */
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};
/* size: 0x48 */

/* The pool block `fn_800F8788(sizeof(struct _EFT_HEAP_WORK))` attaches: the effects it owns and their
 * scale.  The scale sits at +0x10, so at most three effect pointers fit in front of it.
 * size: 0x18 */
struct _EFT_HEAP_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ void* effects[3];
    /* +0x10 */ f32 scale;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
};

/* ---------------------------------------------------------------------------------------------------
 * externs - the callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

extern u32 get_now_areano__Fv(void);
extern struct _EFT* fn_800F8788(u32 block_size);
extern void fn_800F9DF4(struct _EFT* self, u8 a, u8 b);
extern void fn_80119DEC(struct _EFT* self);
extern void fn_8011A2A0(struct _EFT* self);
extern void fn_8011A34C(struct _EFT* self);
extern void fn_8011ACF0(struct _EFT* self);
extern void fn_8011AD00(struct _EFT* self);

extern void push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(void** effects, long count);

extern f32 lbl_80796B2C; /* 1.0f */

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

void fn_80119D10(struct _EFT* self);
void fn_80119D24(struct _EFT* self);
void fn_80119D60(struct _EFT* self);
void fn_80119D9C(struct _EFT* self);

/* Creates the area's effect record and installs its two handlers. */
struct _EFT* fn_80119C44(u32 type, u32 area, u32 count)
{
    struct _EFT* effect;
    struct _EFT_HEAP_WORK* work;

    if ((u8)area != (u8)get_now_areano__Fv()) {
        return 0;
    }
    effect = fn_800F8788(sizeof(struct _EFT_HEAP_WORK));
    if (effect == 0) {
        return 0;
    }
    work = effect->work_0x38;
    work->count = (u8)count;
    work->scale = lbl_80796B2C;
    effect->field_0x03 = 0x1C;
    effect->type_0x02 = type;
    effect->area_0x44 = area;
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_80119D10;
    effect->dispatch_0x34 = fn_80119D9C;
    return effect;
}

/* Runs the release body the record's type asks for. */
void fn_80119D10(struct _EFT* self)
{
    switch (self->type_0x02) {
    default:
        fn_80119D24(self);
        break;
    case 2:
        fn_80119D60(self);
        break;
    }
}

/* Hands the record's pooled effects back and clears its count. */
void fn_80119D24(struct _EFT* self)
{
    struct _EFT_HEAP_WORK* work = self->work_0x38;

    push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(&work->effects[0], work->count);
    work->count = 0;
}

/* Hands the record's pooled effects back and clears its count. */
void fn_80119D60(struct _EFT* self)
{
    struct _EFT_HEAP_WORK* work = self->work_0x38;

    push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(&work->effects[0], work->count);
    work->count = 0;
}

/* Runs the state handler the record's `state_0x05` selects. */
void fn_80119D9C(struct _EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        fn_8011A34C(self);
        return;
    case 2:
        fn_8011ACF0(self);
        return;
    case 3:
        fn_8011AD00(self);
        return;
    default:
        return;
    }

    switch (self->type_0x02) {
    default:
        fn_80119DEC(self);
        break;
    case 2:
        fn_8011A2A0(self);
        break;
    }
}
