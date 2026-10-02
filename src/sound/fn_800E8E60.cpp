/* sound/fn_800E8E60.cpp - the head of the quest/challenge sound work system (voice-slot lists and pools)
 *
 * `.text` 0x800E8E60..0x800E9D00, 33 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit; the functions of the neighbouring units were cut out of this file.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 */

/* Retired header of `sound/fn_800E8E60.cpp` (kept for its notes and residuals): */
/* sound/fn_800E8E60.cpp - the quest/challenge sound work system, `.text` 0x800E8E60..0x800EF7D8
 * (0x6978 B, 149 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 144 of this range's 149 symbols (checked by
 * reading config/RMHE08/symbols.txt: `fn_800E8E60` .. `fn_800EF690` are the map's bare `.text` entries);
 * the five named symbols carry the map's own C++ mangling and are defined under their real identifiers.
 *
 * Language: C++.  Five of the range's map names are mangled (`quest_snd_wk_init__Fv`,
 * `get_chacha_kamen_bank__Fv`, `get_chacha_dance1_bank__Fv`, `get_chacha_dance2_bank__Fv`), which is the
 * plan's evidence that the original TU is C++ (docs/plan.md, "The language comes from the symbol").  No
 * `__FILE__` string for this unit survives in the .data/.sdata pool, so the map's `fn_800E8E60` stem is
 * kept as the file name (docs/plan.md 12, "Register once, at the final home": the stem is a legitimate
 * outcome when nothing supports a better name).
 *
 * Module: `sound`.  The immediately preceding `.text` run (0x800E46E8..0x800E8E60) is
 * `sound/fn_800E46E8.cpp`, and this range's data pool is the sound bank/stream table
 * (`16/tsb/*.tsb`, `16/whd/*.whd`, `16/srt/**`), so the unit is registered in the `sound` lib.
 *
 * Flags: `sound` / `Wii/1.3` / `cflags_main` + `#pragma peephole off`.  `tools/flags/infer.py` reports
 * peephole off (48 folds, `li r0,N; psq_lx/psq_stx` epilogues and 33 kept `clrlwi`s), `-inline noauto`
 * (35 kept `bl` to tiny same-object functions, already in `cflags_main`) and no `stmw`/`lmw`; the
 * neighbouring `sound/fn_800E46E8.cpp` carries the same file-wide pragma.
 *
 * Status: partial (phase B first pass).  82 of the 149 functions have bodies: 81 measure >= 80 % on the
 * official report metric (`fuzzy_match_percent`), 79 of them 100 %, and `fn_800EE524` (a 0x28-byte record
 * copy) is written but scores 74.40 % - MWCC schedules its load/store pairs differently from the target
 * and no source shape tried recovers it.  The 67 unwritten functions score 0 %; the biggest are
 * `fn_800EB314` (1844 B), `fn_800E9E94` (1456 B, MWCC-vectorised paired-single) and `fn_800EE014`
 * (1268 B).  Two further residual families are below the bar or structurally stuck: `fn_800E96BC`
 * (81.12 %) outlines its loop-machine cases to a helper whose argument move (`mr r4,r5`) the inlined
 * body here does not reproduce, and `fn_800E960C` (93.68 %) differs only in branch displacements.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit sound/fn_800E8E60.cpp`.
 */

#include "types.h"
#include "sound/sound_work.h"

/* --- the unit's own functions, declared so each has one signature ---------------------------------- */

extern "C" SndRef* fn_800E9604(SndRef** slot);
extern "C" u32 fn_800E960C(SndRefTable* self, u32 index);
extern "C" SndRef* fn_800E96A4(SndRef** slot);
extern "C" SndRef* fn_800E96AC(SndRef** slot);
extern "C" SndRef* fn_800E96B4(SndRef** slot);
extern "C" void fn_800E96BC(Sound2* self, SoundSlot* slot, AxVoiceBlock* ax);
extern "C" void fn_800E9848(void* self, AxVoiceBlock* ax);
extern "C" void* dtor_800E9854(void* self, s16 flags);
extern "C" void* dtor_800E98C0(void* self, s16 flags);
extern "C" void* fn_800E991C(void* self, s16 flags);
extern "C" void fn_800E9A10(SndListNode* list, SndListNode* node);
extern "C" SndListNode* fn_800E9A28(SndListNode* self);
extern "C" u32 fn_800E9A74(SndListNode* self);
extern "C" void fn_800E9A8C(SndPool* self, SndListNode* src, u32 count);
extern "C" void fn_800E9AE4(SndListNode* list, SndListNode* src, u32 count);
extern "C" void fn_800E9B50(SndListNode* self);
extern "C" SndListNode* fn_800E9B5C(SndListNode* list, SndListNode* node);
extern "C" SndListNode* fn_800E9B60(SndListNode* list, SndListNode* node);
extern "C" void fn_800E9BC0(SndListNode* node);
extern "C" SndListNode* fn_800E9BDC(SndListNode* self);
extern "C" SndListNode* fn_800E9BE4(SndPool* self);
extern "C" SndListNode* fn_800E9C08(SndPool* self);
extern "C" void fn_800E9C10(void);
extern "C" SndPool* fn_800E9C1C(SndPool* self);
extern "C" SndListNode* fn_800E9C74(SndListNode* self);
extern "C" void fn_800E9CB0(SndListNode* self);
extern "C" SndPool* fn_800E9CC8(SndPool* self);
extern "C" void fn_800E8F78(f32* a, f32* b, f32* out);
extern "C" s32 fn_800E9324(s16 arg, f32 a, f32 b);
extern "C" s32 fn_800E9390(f32 a, f32 b);
extern "C" u32 fn_800E95C4(SndLookupHost* self, u32 idx);
extern "C" void fn_800E9960(SndPool* self);
extern "C" SndListNode* fn_800E99B0(SndPool* self);

/* --- the node storage the pool functions index ----------------------------------------------------- */

/* A block with two destructed sub-objects (`dtor_800E9854`). */
typedef struct SndDtorHost {
    /* +0x00 */ SndListNode member_0x00;
    /* +0x4C */ SndListNode member_0x4C;
} SndDtorHost; /* size: 0x98 */

#pragma peephole off

/* --- bodies, in address order --------------------------------------------------------------------- */

/* Returns the reference the slot holds (four accessors share this shape). */
extern "C" SndRef* fn_800E9604(SndRef** slot)
{
    return *slot;
}

/* Selects one of five reference slots by index and returns its value. */
extern "C" u32 fn_800E960C(SndRefTable* self, u32 index)
{
    u32 value = 0;

    switch (index) {
    case 0:
        value = fn_800E96B4(&self->ref_40)->value;
        break;
    case 1:
        value = fn_800E96B4(&self->ref_18)->value;
        break;
    case 2:
        value = fn_800E96AC(&self->ref_00)->value;
        break;
    case 3:
        value = fn_800E96A4(&self->ref_08)->value;
        break;
    case 4:
        value = fn_800E96B4(&self->ref_10)->value;
        break;
    }
    return value;
}

/* Returns the reference the slot holds. */
extern "C" SndRef* fn_800E96A4(SndRef** slot)
{
    return *slot;
}

/* Returns the reference the slot holds. */
extern "C" SndRef* fn_800E96AC(SndRef** slot)
{
    return *slot;
}

/* Returns the reference the slot holds. */
extern "C" SndRef* fn_800E96B4(SndRef** slot)
{
    return *slot;
}

/* Advances a voice's loop state machine one step; state 0 and state 5 dispatch through the owner's vtable. */
extern "C" void fn_800E96BC(Sound2* self, SoundSlot* slot, AxVoiceBlock* ax)
{
    switch (ax->field_3A) {
    case 0:
        self->vtable->method_24(self, slot);
        break;
    case 1:
        ax->loop_value = (s16)(ax->loop_value + ax->loop_rise_step);
        if (ax->loop_value >= 127) {
            ax->loop_value = 127;
            if (ax->loop_fall_base != 0) {
                ax->field_3A = 2;
            } else if (ax->loop_hold_step != 0) {
                ax->field_3A = 3;
            } else {
                ax->field_3A = 4;
            }
        }
        break;
    case 2:
        ax->loop_value = (s16)(ax->loop_fall_base - ax->loop_value);
        if (ax->loop_value <= ax->loop_fall_min) {
            ax->loop_value = ax->loop_fall_min;
            if (ax->loop_hold_step != 0) {
                ax->field_3A = 3;
            } else {
                ax->field_3A = 4;
            }
        }
        break;
    case 3:
        ax->loop_value = (s16)(ax->loop_value + ax->loop_hold_step);
        if (ax->loop_value >= 127) {
            ax->loop_value = 127;
            ax->field_3A = 4;
        } else if (ax->loop_value <= 0) {
            ax->loop_value = 0;
            ax->field_3A = 5;
        }
        break;
    case 5:
        ax->loop_value = (s16)(ax->loop_release_step - ax->loop_value);
        if (ax->loop_value <= 0) {
            ax->loop_value = 0;
            self->vtable->method_18(self, ax);
        }
        break;
    }
}

/* Forces a voice's loop state to the wrap state. */
extern "C" void fn_800E9848(void* self, AxVoiceBlock* ax)
{
    ax->field_3A = 5;
}

/* Deleting destructor: frees both base sub-objects and, for a positive flag, the object. */
extern "C" void* dtor_800E9854(void* self, s16 flags)
{
    if (self != 0) {
        dtor_800E98C0(&((SndDtorHost*)self)->member_0x4C, -1);
        dtor_800E98C0(self, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Deleting destructor for one base sub-object. */
extern "C" void* dtor_800E98C0(void* self, s16 flags)
{
    if (self != 0) {
        fn_800E991C(self, 0);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Frees the object when the flags say it is owned. */
extern "C" void* fn_800E991C(void* self, s16 flags)
{
    if (self != 0 && flags > 0) {
        operator delete(self);
    }
    return self;
}

/* Inserts `node` at the head of `list` (the sentinel). */
extern "C" void fn_800E9A10(SndListNode* list, SndListNode* node)
{
    SndListNode* first = list->next;

    node->next = first;
    first->prev = node;
    node->prev = list;
    list->next = node;
}

/* Removes and returns the head node, or null when the list is empty. */
extern "C" SndListNode* fn_800E9A28(SndListNode* self)
{
    if (fn_800E9A74(self) == 1) {
        return 0;
    }
    {
        SndListNode* first = self->next;
        SndListNode* second = first->next;

        self->next = second;
        second->prev = self;
        return first;
    }
}

/* Whether the list is empty (its sentinel is its only node). */
extern "C" u32 fn_800E9A74(SndListNode* self)
{
    return self->next == self->prev;
}

/* Empties the active list and fills the free list with `count` nodes from `src`. */
extern "C" void fn_800E9A8C(SndPool* self, SndListNode* src, u32 count)
{
    fn_800E9B50(&self->active_list);
    fn_800E9AE4(&self->free_list, src, count);
}

/* Inserts `count` consecutive nodes from `src` at the head of `list`. */
extern "C" void fn_800E9AE4(SndListNode* list, SndListNode* src, u32 count)
{
    u32 i = 0;
    SndListNode* node = src;

    while (i < count) {
        fn_800E9A10(list, node);
        node++;
        i++;
    }
}

/* Makes `self` an empty list. */
extern "C" void fn_800E9B50(SndListNode* self)
{
    self->prev = self;
    self->next = self;
}

/* Moves `node` to the head of `list` and returns its former successor. */
extern "C" SndListNode* fn_800E9B5C(SndListNode* list, SndListNode* node)
{
    return fn_800E9B60(list, node);
}

/* Moves `node` to the head of `list` and returns its former successor. */
extern "C" SndListNode* fn_800E9B60(SndListNode* list, SndListNode* node)
{
    SndListNode* next = fn_800E9BDC(node);

    fn_800E9BC0(node);
    fn_800E9A10(list, node);
    return next;
}

/* Unlinks `node` from whatever list holds it. */
extern "C" void fn_800E9BC0(SndListNode* node)
{
    node->prev->next = node->next;
    node->prev->next->prev = node->prev;
}

/* The node after `self`. */
extern "C" SndListNode* fn_800E9BDC(SndListNode* self)
{
    return self->next;
}

/* The first node of the pool's active list. */
extern "C" SndListNode* fn_800E9BE4(SndPool* self)
{
    return fn_800E9BDC(fn_800E9C08(self));
}

/* The pool's active-list sentinel. */
extern "C" SndListNode* fn_800E9C08(SndPool* self)
{
    return &self->active_list;
}

/* The global pool's constructor entry point. */
extern "C" void fn_800E9C10(void)
{
    fn_800E9C1C(&lbl_80698AF0);
}

/* Constructs the pool: both lists, then the 96 nodes. */
extern "C" SndPool* fn_800E9C1C(SndPool* self)
{
    fn_800E9CC8(self);
    __construct_array(&self->nodes[0], (void*)fn_800E9C74, (void*)dtor_800E98C0, 76, 96);
    self->initialised = 0;
    return self;
}

/* Constructs one node: the base constructor then the derived vtable. */
extern "C" SndListNode* fn_800E9C74(SndListNode* self)
{
    fn_800E9CB0(self);
    self->vtable = lbl_80598020;
    return self;
}

/* The node base constructor: base vtable and an empty list. */
extern "C" void fn_800E9CB0(SndListNode* self)
{
    self->vtable = lbl_8059802C;
    self->next = self;
    self->prev = self;
}

/* Constructs both of the pool's list sentinels. */
extern "C" SndPool* fn_800E9CC8(SndPool* self)
{
    fn_800E9C74(&self->free_list);
    fn_800E9C74(&self->active_list);
    return self;
}

/* --- the next batch ---------------------------------------------------------------------------- */

/* Ratio of two levels, clamped to 1.0. */
extern "C" void fn_800E8F78(f32* a, f32* b, f32* out)
{
    if (lbl_807964E8 == *a || *b > *a) {
        *out = lbl_807964E8;
    } else {
        *out = lbl_807964EC - *b / *a;
    }
}

/* Scales a level, converts it to an integer and clamps it to [0, arg]. */
extern "C" s32 fn_800E9324(s16 arg, f32 a, f32 b)
{
    s32 v = (s32)((f32)arg / (lbl_807964EC + lbl_80796514 * (a * b)));

    if (v > arg) {
        v = arg;
    }
    if (v < 0) {
        v = 0;
    }
    return v;
}

/* Scales a level and clamps it to [1, 30]. */
extern "C" s32 fn_800E9390(f32 a, f32 b)
{
    s32 v = (s32)(lbl_80796518 / (lbl_807964EC + lbl_8079651C * (a * b)));

    if (v > 30) {
        v = 30;
    }
    if (v < 1) {
        v = 1;
    }
    return v;
}

/* Returns item `idx` of the lookup at +0x18, or zero when it is unset or out of range. */
extern "C" u32 fn_800E95C4(SndLookupHost* self, u32 idx)
{
    SndLookup* l = &self->at_0x18;

    if (l->header == 0) {
        return 0;
    }
    if (l->header->count <= idx) {
        return 0;
    }
    return l->items[idx];
}

/* Initialises the pool on first use and allocates one node from it. */
extern "C" void fn_800E9960(SndPool* self)
{
    if (self->initialised == 0) {
        fn_800E9A8C(self, &self->nodes[0], 96);
        self->initialised = 1;
    }
    fn_800E99B0(self);
}

/* Allocates one node: pops the free list and pushes it onto the active list. */
extern "C" SndListNode* fn_800E99B0(SndPool* self)
{
    if (fn_800E9A74(&self->free_list) == 1) {
        return 0;
    }
    {
        SndListNode* node = fn_800E9A28(&self->free_list);

        fn_800E9A10(&self->active_list, node);
        return node;
    }
}
