/* sound/snd_voice_pool.cpp - the sound table object and its pool destructors
 *
 * `.text` 0x800ED780..0x800EE014, 22 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit, built from `sound/fn_800E8E60.cpp`.
 * Name is a GUESS: the unit owns `lbl_8069A8E8` (0x6828 B `.bss`, the table `snd_bank1_install`/`fn_800EDD00` install into) and `dtor_800ED780`/`dtor_800ED7EC`.
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "types.h"
#include "sound/sound_work.h"

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

extern "C" SndListNode* fn_800EDB48(SndVoiceMgr* self);

#pragma peephole off

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
