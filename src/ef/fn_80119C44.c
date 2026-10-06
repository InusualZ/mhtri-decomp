/* ef/fn_80119C44.c - an effect record's constructor and its three hooks, dispatching into `ef/eft029.cpp`'s handlers.
 * RANGE. .text 0x80119C44-0x80119DEC (5 functions); extab 0x8000C554-0x8000C56C, extabindex 0x800266E8-0x8002670C,
 *   .sdata2 0x80796B2C-0x80796B30.  `fn_80119C44` takes a 0x48-byte `_EFT_HEAP` record with a 0x18-byte work block
 *   from `eft_res_slot_get`, installs `fn_80119D10` (pool release, +0x40) and `fn_80119D9C` (the four-state dispatch,
 *   +0x34); `fn_80119D9C` runs the type-2 start `fn_80119DEC` or the type handlers in `ef/eft029.cpp`.
 * NAMES. The map has only `fn_` stems for this range.  The file is C: the unit's own symbols are plain (a mangled
 *   callee is not evidence of a C++ unit); its extab/extabindex come from `cflags_main`'s `-Cpp_exceptions on`.
 * RESIDUALS. 2 partial rows:
 *  - `fn_80119C44`: the two handler stores use `r3` for the `addi` where retail uses `r0` (sizes and relocations
 *    agree); no spelling of the assignments moves it (docs/ef.md, "Handler stores through r0");
 *  - `fn_80119D9C`: retail places case 0's nested type switch after the function's exit block; MWCC puts it in
 *    case 0's slot for every spelling, so the source is the `case 0: break;` plus after-switch form, which keeps
 *    the target's size and order but leaves a dead `blr` (docs/ef.md, "The case-0 nested dispatch").
 *   flipcheck: `.sdata2` claimed, not emitted; `.text` differs in 26 bytes.
 */

#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(struct _EFT_HEAP*, u8, u8))eft_state_flags_set)

/* ---------------------------------------------------------------------------------------------------
 * the 0x48-byte effect record and its pool block
 * ------------------------------------------------------------------------------------------------- */

struct _EFT_HEAP_WORK;

/* The effect object `eft_res_slot_get` hands out and the two installed handlers drive. size: 0x48 */
struct _EFT_HEAP {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;   /* the effect type the pool tables are indexed by */
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;  /* the `fn_80119D9C` state index */
    /* +0x06 */ u8 unused_0x06[0x34 - 0x06];
    /* +0x34 */ void (*dispatch_0x34)(struct _EFT_HEAP*); /* the per-frame state machine */
    /* +0x38 */ struct _EFT_HEAP_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(struct _EFT_HEAP*);  /* the pool-release hook */
    /* +0x44 */ u8 area_0x44;   /* the area the record was spawned in */
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};
/* size: 0x48 */

/* The pool block `eft_res_slot_get(sizeof(struct _EFT_HEAP_WORK))` attaches: the effects it owns and their
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
extern struct _EFT_HEAP* eft_res_slot_get(u32 block_size);
extern void fn_80119DEC(struct _EFT_HEAP* self);
extern void fn_8011A2A0(struct _EFT_HEAP* self);
extern void fn_8011A34C(struct _EFT_HEAP* self);
extern void fn_8011ACF0(struct _EFT_HEAP* self);
extern void fn_8011AD00(struct _EFT_HEAP* self);

extern void push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(void** effects, long count);

extern f32 lbl_80796B2C; /* 1.0f */

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

void fn_80119D10(struct _EFT_HEAP* self);
void fn_80119D24(struct _EFT_HEAP* self);
void fn_80119D60(struct _EFT_HEAP* self);
void fn_80119D9C(struct _EFT_HEAP* self);

/* Creates the area's effect record and installs its two handlers. */
struct _EFT_HEAP* fn_80119C44(u32 type, u32 area, u32 count)
{
    struct _EFT_HEAP* effect;
    struct _EFT_HEAP_WORK* work;

    if ((u8)area != (u8)get_now_areano__Fv()) {
        return 0;
    }
    effect = eft_res_slot_get(sizeof(struct _EFT_HEAP_WORK));
    if (effect == 0) {
        return 0;
    }
    work = effect->work_0x38;
    work->count = (u8)count;
    work->scale = lbl_80796B2C;
    effect->field_0x03 = 0x1C;
    effect->type_0x02 = type;
    effect->area_0x44 = area;
    eft_state_flags_set_c1(effect, 0, 0);
    effect->release_0x40 = fn_80119D10;
    effect->dispatch_0x34 = fn_80119D9C;
    return effect;
}

/* Runs the release body the record's type asks for. */
void fn_80119D10(struct _EFT_HEAP* self)
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
void fn_80119D24(struct _EFT_HEAP* self)
{
    struct _EFT_HEAP_WORK* work = self->work_0x38;

    push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(&work->effects[0], work->count);
    work->count = 0;
}

/* Hands the record's pooled effects back and clears its count. */
void fn_80119D60(struct _EFT_HEAP* self)
{
    struct _EFT_HEAP_WORK* work = self->work_0x38;

    push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(&work->effects[0], work->count);
    work->count = 0;
}

/* Runs the state handler the record's `state_0x05` selects. */
void fn_80119D9C(struct _EFT_HEAP* self)
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
