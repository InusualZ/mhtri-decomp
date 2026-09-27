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

#pragma peephole off

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
extern "C" void fn_800E9E54(void* arg);
extern "C" SndAbVoice* fn_800EA7DC(SndAbVoiceHost* self);
extern "C" void fn_800EAB18(void* self, SndAbHost* host);
extern "C" void* fn_800EBA48(SndBitTable* self);
extern "C" void fn_800EBAA0(SndBitTable* self, u32 item);
extern "C" u32 fn_800EBD18(SndLookupHost* self, u32 idx);
extern "C" void fn_800EBD58(SndBdRec* dst, SndBdRec* src);
extern "C" u32 fn_800EC344(SndLookupHost* self, u32 idx);
extern "C" u32 fn_800EC380(SndLookupHost* self, u32 idx);
extern "C" SndEdRec* fn_800ED72C(SndEdRec* self);
extern "C" void* dtor_800ED780(SndEdHost* self, s16 flags);
extern "C" void* dtor_800ED7EC(void* self, s16 flags);
extern "C" void* fn_800ED848(void* self, s16 flags);
extern "C" void fn_800ED88C(SndPool2* self);
extern "C" SndNode2* fn_800ED8DC(SndPool2* self);
extern "C" SndNode2* fn_800ED954(SndNode2* self);
extern "C" void fn_800ED9B8(SndPool2* self, SndNode2* src, u32 count);
extern "C" void fn_800EDA10(SndNode2* list, SndNode2* src, u32 count);
extern "C" void* fn_800EDAA8(SndNode2* list, SndNode2* node);
extern "C" SndPool2* fn_800EDF1C(SndPool2* self);
extern "C" SndNode2* fn_800EDF74(SndNode2* self);
extern "C" SndPool2* fn_800EDFDC(SndPool2* self);
extern "C" void fn_800EE524(SndCopy28* dst, SndCopy28* src);
extern "C" u32 fn_800EE868(SndModeWord* self, u32 v);
extern "C" u32 fn_800EE8B4(SndModeWord* self, u32 v);
extern "C" void* fn_800EE9EC(u32 size);
extern "C" SndListNode* fn_800EDB48(SndVoiceMgr* self);
extern "C" void fn_800ED6A4(void* obj);
extern "C" void* dtor_800E9DD0(void* self, s16 flags);

/* --- the node storage the pool functions index ----------------------------------------------------- */

/* A block with two destructed sub-objects (`dtor_800E9854`). */
typedef struct SndDtorHost {
    /* +0x00 */ SndListNode member_0x00;
    /* +0x4C */ SndListNode member_0x4C;
} SndDtorHost; /* size: 0x98 */

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

/* Rounds `a` up to the next multiple of `b` (zero when `b` is zero). */
extern "C" u32 fn_800EA7B8(u32 a, u32 b)
{
    return b != 0 ? (a + b - 1) / b * b : 0;
}

/* The block at +0x1C of `self` (the voice state fn_800EAB18 writes). */
extern "C" SndAbVoice* fn_800EA7DC(SndAbVoiceHost* self)
{
    return &self->voice;
}

/* Returns item `idx` of the table, or zero when it is out of range. */
extern "C" u32 fn_800EBCF4(SndU32Table* self, u32 idx)
{
    if (self->count <= idx) {
        return 0;
    }
    return self->items[idx];
}

/* Copies one two-word record. */
extern "C" void fn_800EBDC8(Pair32* dst, Pair32* src)
{
    *dst = *src;
}

/* The relocation state at +0x08 of `self`. */
extern "C" void* fn_800EC93C(SndRelocHost* self, u32 idx)
{
    return fn_800E7578(&self->state, idx);
}

/* The stream manager's constructor entry point. */
extern "C" void fn_800ED698(void)
{
    fn_800ED6A4(lbl_8069A810);
}

/* Inserts `node` at the head of `list` (the 0x110 pool's lists). */
extern "C" void fn_800ED93C(SndNode2* list, SndNode2* node)
{
    SndNode2* first = list->next;

    node->next = first;
    first->prev = node;
    node->prev = list;
    list->next = node;
}

/* Whether the list is empty. */
extern "C" u32 fn_800ED9A0(SndNode2* self)
{
    return self->next == self->prev;
}

/* Makes `self` an empty list. */
extern "C" void fn_800EDA7C(SndNode2* self)
{
    self->prev = self;
    self->next = self;
}

/* Resets a voice record and hands it to the state machine. */
extern "C" void fn_800EDA88(SndNode2* list, SndNode2* node)
{
    node->field_0x0C = 0;
    node->field_0x10 = 0;
    node->field_0x18 = 255;
    node->field_0x14 = 0;
    node->field_0x19 = 0;
    fn_800EDAA8(list, node);
}

/* Unlinks `node` from whatever list holds it. */
extern "C" void fn_800EDB08(SndNode2* node)
{
    node->prev->next = node->next;
    node->prev->next->prev = node->prev;
}

/* The first voice of the manager's voice list. */
extern "C" void* fn_800EDB24(SndVoiceMgr* self)
{
    return fn_800E614C(fn_800EDB48(self));
}

/* The manager's voice-list sentinel. */
extern "C" SndListNode* fn_800EDB48(SndVoiceMgr* self)
{
    return &self->voice_list;
}

/* The manager's voice list walked through the other list helper. */
extern "C" void* fn_800EDB50(SndVoiceMgr* self)
{
    return fn_800E7264(fn_800EDB48(self));
}

/* The sound table's constructor entry point. */
extern "C" void fn_800EDF10(void)
{
    fn_800EDF1C(&lbl_8069A8E8);
}

/* The 0x110-node base constructor with the base vtable. */
extern "C" void fn_800EDFC4(SndNode2* self)
{
    self->vtable = lbl_8059809C;
    self->next = self;
    self->prev = self;
}

/* Copies one three-half-word record. */
extern "C" void fn_800EE508(SndTri16* dst, SndTri16* src)
{
    dst->a = src->a;
    dst->b = src->b;
    dst->c = src->c;
}

/* Frees an allocation back to the expand heap. */
extern "C" void fn_800EEA2C(void* p)
{
    if (p == 0) {
        return;
    }
    MEMFreeToExpHeap((void*)lbl_80794A20, p);
}

/* Releases a resource of kind 4. */
extern "C" void fn_800EEEF4(void* p)
{
    fn_804C2380(p, 4);
}

/* The Chacha companion's "kamen" voice bank. */
s32 get_chacha_kamen_bank(void)
{
    return 36;
}

/* The Chacha companion's first dance voice bank. */
s32 get_chacha_dance1_bank(void)
{
    return 32;
}

/* The Chacha companion's second dance voice bank. */
s32 get_chacha_dance2_bank(void)
{
    return 33;
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

/* Hands a parameter to the stream work's virtual method 5. */
extern "C" void fn_800E9E54(void* arg)
{
    Sound2* obj = (Sound2*)fn_800E4A18();

    obj->vtable->method_14(obj, arg);
}

/* Enables the voice and resets its two counters. */
extern "C" void fn_800EAB18(void* self, SndAbHost* host)
{
    SndAbVoice* voice = fn_800EA7DC(host->voice);

    voice->enabled_0xAB = 1;
    voice->field_0xD4 = 0xFFFF;
    voice->field_0xD6 = 0;
}

/* Returns the first free item of the bit table and marks it used. */
extern "C" void* fn_800EBA48(SndBitTable* self)
{
    u32 i;
    u32 count = self->count;

    for (i = 0; i < count; i++) {
        if ((self->mask & (1u << i)) == 0) {
            u32 item = self->items[i];

            self->mask |= (1u << i);
            return (void*)item;
        }
    }
    return 0;
}

/* Releases the bit of the item equal to `item`. */
extern "C" void fn_800EBAA0(SndBitTable* self, u32 item)
{
    u32 i;

    for (i = 0; i < self->count; i++) {
        if (self->items[i] == item) {
            self->mask &= ~(1u << i);
        }
    }
}

/* Returns item `idx` of the lookup at +0x10. */
extern "C" u32 fn_800EBD18(SndLookupHost* self, u32 idx)
{
    SndLookup* l = &self->at_0x10;

    if (l->header == 0) {
        return 0;
    }
    if (l->header->count <= idx) {
        return 0;
    }
    return l->items[idx];
}

/* Copies one 0x28-byte record (four pairs then two words). */
extern "C" void fn_800EBD58(SndBdRec* dst, SndBdRec* src)
{
    fn_800EBDC8(&dst->p_0x00, &src->p_0x00);
    fn_800EBDC8(&dst->p_0x08, &src->p_0x08);
    fn_800EBDC8(&dst->p_0x10, &src->p_0x10);
    fn_800EBDC8(&dst->p_0x18, &src->p_0x18);
    dst->field_0x20 = src->field_0x20;
    dst->field_0x24 = src->field_0x24;
}

/* Returns item `idx` of the lookup at +0x00. */
extern "C" u32 fn_800EC344(SndLookupHost* self, u32 idx)
{
    SndLookup* l = &self->at_0x00;

    if (l->header == 0) {
        return 0;
    }
    if (l->header->count <= idx) {
        return 0;
    }
    return l->items[idx];
}

/* Returns item `idx` of the lookup at +0x18. */
extern "C" u32 fn_800EC380(SndLookupHost* self, u32 idx)
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

/* Constructs the four relocation states and clears the two trailing words. */
extern "C" SndEdRec* fn_800ED72C(SndEdRec* self)
{
    fn_800E7CA4(&self->states[0]);
    fn_800E7CA4(&self->states[1]);
    fn_800E7CA4(&self->states[2]);
    fn_800E7CA4(&self->states[3]);
    self->field_0x20 = 0;
    self->field_0x24 = 0;
    return self;
}

/* Deleting destructor for the two-record host. */
extern "C" void* dtor_800ED780(SndEdHost* self, s16 flags)
{
    if (self != 0) {
        dtor_800ED7EC(&self->member_0x110, -1);
        dtor_800ED7EC(self, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Deleting destructor for one record. */
extern "C" void* dtor_800ED7EC(void* self, s16 flags)
{
    if (self != 0) {
        fn_800ED848(self, 0);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Frees the object when the flags say it is owned. */
extern "C" void* fn_800ED848(void* self, s16 flags)
{
    if (self != 0 && flags > 0) {
        operator delete(self);
    }
    return self;
}

/* Initialises the second pool on first use and allocates one node. */
extern "C" void fn_800ED88C(SndPool2* self)
{
    if (self->initialised == 0) {
        fn_800ED9B8(self, &self->nodes[0], 96);
        self->initialised = 1;
    }
    fn_800ED8DC(self);
}

/* Allocates one 0x110 node: pops the free list and pushes it onto the active list. */
extern "C" SndNode2* fn_800ED8DC(SndPool2* self)
{
    if (fn_800ED9A0(&self->free_list) == 1) {
        return 0;
    }
    {
        SndNode2* node = fn_800ED954(&self->free_list);

        fn_800ED93C(&self->active_list, node);
        return node;
    }
}

/* Removes and returns the head node, or null when the list is empty. */
extern "C" SndNode2* fn_800ED954(SndNode2* self)
{
    if (fn_800ED9A0(self) == 1) {
        return 0;
    }
    {
        SndNode2* first = self->next;
        SndNode2* second = first->next;

        self->next = second;
        second->prev = self;
        return first;
    }
}

/* Empties the active list and fills the free list with `count` nodes from `src`. */
extern "C" void fn_800ED9B8(SndPool2* self, SndNode2* src, u32 count)
{
    fn_800EDA7C(&self->active_list);
    fn_800EDA10(&self->free_list, src, count);
}

/* Inserts `count` consecutive 0x110 nodes from `src` at the head of `list`. */
extern "C" void fn_800EDA10(SndNode2* list, SndNode2* src, u32 count)
{
    u32 i = 0;
    SndNode2* node = src;

    while (i < count) {
        fn_800ED93C(list, node);
        node++;
        i++;
    }
}

/* Moves `node` to the head of `list` and returns its former successor. */
extern "C" void* fn_800EDAA8(SndNode2* list, SndNode2* node)
{
    SndNode2* next = (SndNode2*)fn_800E614C(node);

    fn_800EDB08(node);
    fn_800ED93C(list, node);
    return next;
}

/* Constructs the second pool: both lists, then the 96 nodes. */
extern "C" SndPool2* fn_800EDF1C(SndPool2* self)
{
    fn_800EDFDC(self);
    __construct_array(&self->nodes[0], (void*)fn_800EDF74, (void*)dtor_800ED7EC, 272, 96);
    self->initialised = 0;
    return self;
}

/* Constructs one 0x110 node: the base constructor then the derived vtable. */
extern "C" SndNode2* fn_800EDF74(SndNode2* self)
{
    fn_800EDFC4(self);
    self->vtable = lbl_80598090;
    self->field_0x0C = 0;
    self->field_0x10 = 0;
    self->field_0x14 = 255;
    return self;
}

/* Constructs both of the second pool's list sentinels. */
extern "C" SndPool2* fn_800EDFDC(SndPool2* self)
{
    fn_800EDF74(&self->free_list);
    fn_800EDF74(&self->active_list);
    return self;
}

/* Copies one 0x28-byte five-pair record. */
extern "C" void fn_800EE524(SndCopy28* dst, SndCopy28* src)
{
    *dst = *src;
}

/* Scales `v` by the mode in the word at +0x04. */
extern "C" u32 fn_800EE868(SndModeWord* self, u32 v)
{
    u32 r = 0;

    switch (self->mode) {
    case 0:
    case 1:
        r = v << 1;
        break;
    case 2:
    case 3:
        r = v >> 1;
        break;
    case 4:
    case 5:
        r = v;
        break;
    }
    return r;
}

/* Scales `v` by the mode in the word at +0x04 (the inverse mapping). */
extern "C" u32 fn_800EE8B4(SndModeWord* self, u32 v)
{
    u32 r = 0;

    switch (self->mode) {
    case 0:
    case 1:
        r = v >> 1;
        break;
    case 2:
    case 3:
        r = v << 1;
        break;
    case 4:
    case 5:
        r = v;
        break;
    }
    return r;
}

/* Allocates `size` bytes from the global allocator (null for a zero request). */
extern "C" void* fn_800EE9EC(u32 size)
{
    void* p = 0;

    if (size != 0) {
        p = MEMAllocFromAllocator(lbl_806A1110, size);
    }
    return p;
}

/* A host with four destructed sub-objects at +0, +8, +16 and +24. */
typedef struct SndQuadHost {
    /* +0x00 */ u8 member_0x00[8];
    /* +0x08 */ u8 member_0x08[8];
    /* +0x10 */ u8 member_0x10[8];
    /* +0x18 */ u8 member_0x18[8];
} SndQuadHost; /* size: 0x20 */

/* A host with a three-element array at +0x5C. */
typedef struct SndArrHost {
    /* +0x00 */ u8 pad_0x00[0x5C];
    /* +0x5C */ u8 array_0x5C[3 * 40];
} SndArrHost; /* size: 0xD4 */

/* Destroys a three-element array of 40-byte records. */
extern "C" void* fn_800E9D58(SndArrHost* self, s16 flags)
{
    if (self != 0) {
        __destroy_arr(&self->array_0x5C, (void*)dtor_800E9DD0, 40, 3);
        dtor_800E54A8(self, 0);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Destroys the four sub-objects of the record. */
extern "C" void* dtor_800E9DD0(void* self, s16 flags)
{
    if (self != 0) {
        fn_800E5690(&((SndQuadHost*)self)->member_0x18, -1);
        fn_800E5690(&((SndQuadHost*)self)->member_0x10, -1);
        fn_800E5690(&((SndQuadHost*)self)->member_0x08, -1);
        fn_800E5690(&((SndQuadHost*)self)->member_0x00, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}
