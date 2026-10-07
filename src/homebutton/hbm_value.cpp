/*
 * homebutton/hbm_value.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x80558EB4..0x8055EAF0 (176 functions, 77 reconstructed).
 *
 * Phase 4 fold: 34 function(s) of 0x80558EB4..0x8055C894 from the former registered unit `homebutton/fn_80555374.cpp`.
 * Phase 4 fold: 43 function(s) of 0x8055C894..0x8055EAF0 from the former registered unit `homebutton/keyboard_ui.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the shown-value / dirty / timer helpers, the u16 array and the observer interface callbacks, from the dominant function descriptions
 * below; the band has no `__FILE__` string and the runtime dump names no function, so every function keeps its `fn_<addr>`
 * placeholder.  The band was built with C++ exceptions off (no extab/extabindex in the target objects), so the scoped
 * `#pragma exceptions off` below is carried from the absorbed sources and applied to every piece.
 *
 * Residuals: partial reconstruction - the functions not defined below keep the target object's bytes; the evidence, type views and
 * per-function residuals of the absorbed sources are in docs/splits/phase4/homebutton-carried-notes.md; the type views are in
 * `homebutton/fn_8054E894.h`, `homebutton/hbm_widget.h`, `homebutton/hbm_vu_object.h`, `homebutton/hbm_kb_object.h` and `homebutton/gui_manager.h`.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_vu_object.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"
#include "homebutton/fn_8052B004.h"
#include "homebutton/fn_80533474.h"
#include "homebutton/fn_8053E808.h"
#include "homebutton/fn_8056D814.h"
#include "homebutton/hbm_anim_record.h"
#include "homebutton/hbm_kb_child.h"
#include "homebutton/hbm_kb_list.h"
#include "homebutton/hbm_kb_widget.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"
#include "nw4r/fn_805012C4.h"
#include "nw4r/fn_80502828.h"

extern "C" void fn_8055C638(void* self);

extern "C" void fn_8055BBAC(void* node);

extern "C" u32 fn_8055BEDC(void* current);

extern "C" u32 fn_8055BEF0(void* self);

extern "C" void wmemset(void* dst, int value, u16 size);

extern "C" void* fn_8055C494(void* self, int flag);

extern "C" void* fn_8055B898(void* self, int flag);

extern f32 lbl_8079D700;

/* ---------------------------------------------------------------------------------------------------
 * helpers that live in other splits.  They sit in the unsplit `main` band (fn_80501xxx / fn_8054xxx /
 * fn_8053xxx), so no registered unit owns them - rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"), the same way the neighbouring `homebutton/gui.cpp` declares them.
 * ------------------------------------------------------------------------------------------------ */
extern "C" {

void  fn_8055B7D4(void* sub, s32 flag);

s32   wcschr(void* base, void* key);

s32   strlen(const char* s);

s32   strncmp(const char* a, const char* b, u32 n);

char* strncpy(char* dst, const char* src, u32 n);

void* memset(void* dst, s32 c, u32 n);

}

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata2` float pool and `.sdata` globals: other splits own them, so they are
 * declared, never defined (playbook 29).
 * ------------------------------------------------------------------------------------------------ */

/* Bounds-checked store into the widget's u16 array. */
extern "C" void fn_805594C0(HbmU16Array* self, u16 index, u16 value) {
    if (index < self->capacity) {
        self->data[index] = value;
    }
}

/* Clears the widget's u16 array and its counters. */
extern "C" void fn_80559568(HbmU16Array* self) {
    wmemset(self->data, 0, self->capacity);
    self->length = 0;
    self->offset = 0;
}

/* The last used entry of the widget's u16 array. */
extern "C" u16 fn_805595AC(HbmU16Array* self) {
    s16 index = (s16)(self->length - 1);
    if (index < 0) {
        return 0;
    }
    return self->data[index];
}

/* Recomputes the byte offset of the last u16 entry. */
extern "C" void fn_805595D0(HbmU16Array* self) {
    u16 i;
    self->offset = 0;
    for (i = 0; i < self->length; i++) {
        self->offset += 2;
    }
}

/* Virtual slot at +0x5C: sets the widget's id. */
extern "C" void fn_80559814(HbmWidget* self) {
    self->vtable->setId(self, 10);
}

/* Steps the pending value down one step and shows it. */
extern "C" u32 fn_80559C1C(HbmTextValue* self) {
    if (self->value == 0) {
        return 0;
    }
    self->value--;
    if (self->dirty == 0) {
        self->shown = self->value;
    }
    return 1;
}

/* Stores a new value; while not dirty it is shown immediately. */
extern "C" void fn_80559C50(HbmTextValue* self, u32 value) {
    u8 dirty = self->dirty;
    self->value = value;
    if (dirty == 0) {
        self->shown = value;
    }
}

/* Marks the pending value dirty and shows it. */
extern "C" void fn_80559C68(HbmTextValue* self) {
    self->shown = self->value;
    self->dirty = 1;
}

/* Clears the dirty flag. */
extern "C" void fn_80559C7C(HbmTextValue* self) {
    self->dirty = 0;
}

/* Reads the value and the shown value out. */
extern "C" void fn_80559C88(HbmTextValue* self, u32* value, u32* shown) {
    *value = self->value;
    *shown = self->shown;
}

/* Writes the two values ordered, smaller first. */
extern "C" void fn_80559D78(HbmTextValue* self, u32* first, u32* second) {
    if (self->value > self->shown) {
        *first = self->shown;
        *second = self->value;
    } else {
        *first = self->value;
        *second = self->shown;
    }
}

/* Stores the callback cluster of the band's observer interface. */
extern "C" void fn_8055A3FC(HbmWidget* self, u32 a, void* b, void* c) {
    self->arg_50 = a;
    self->ptr_BC = b;
    self->ptr_C0 = c;
}

/* Loads a timer from its two key positions. */
extern "C" void fn_8055A40C(HbmTimer* self, u32 id, u32 kind, f32 a, f32 b, f32 c, f32 d) {
    f32 base = self->base;
    self->id = id;
    self->kind = kind;
    self->a = a;
    self->b = b - base;
    self->start = c;
    self->end = d - base;
    self->active = 1;
}

/* Stores the two callback pointers of the band's observer interface. */
extern "C" void fn_8055A43C(HbmWidget* self, HbmAllocator* allocator, u32 tail) {
    self->allocator = allocator;
    self->tail.word_D0 = tail;
}

/* A null slot of the band's observer interface. */
extern "C" u32 fn_8055A594(void) {
    return 0;
}

/* Counts one child of the widget. */
extern "C" void fn_8055A870(HbmWidget* self) {
    self->count_A0++;
}

/* Empty virtual slot. */
extern "C" void fn_8055A880(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055A884(void) {
}

/* Reports the widget's own completion code. */
extern "C" u32 fn_8055A888(HbmWidget* self, int arg) {
    if (self->flag_C8 == 0) {
        return fn_805041F4(self, arg);
    }
    return fn_805041F4(self, 0x2A);
}

/* Empty virtual slot. */
extern "C" void fn_8055AD98(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055AD9C(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055B434(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8055B438(void) {
}

/* Runs the widget's own destructor, then releases its storage. */
extern "C" void* fn_8055B898(void* self, int flag) {
    if (self != 0) {
        fn_8052B5F8(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Stores the widget's layout-name string. */
extern "C" void fn_8055BAE4(HbmWidget* self, const void* name) {
    self->owner = (HbmWidget*)name;
}

/* Virtual slot at +0x14: hides the widget. */
extern "C" void fn_8055BB88(HbmWidget* self) {
    self->vtable->hide(self);
}

/* Virtual slot at +0x10 on the widget's active child. */
extern "C" void fn_8055BB98(HbmWidget* self) {
    HbmWidget* child = self->child;
    child->vtable->update(child);
}

/* The widget's collection entry at an index. */
extern "C" void* fn_8055BE48(HbmWidget* self, u32 index) {
    return self->items[index];
}

/* The widget's type tag. */
extern "C" u32 fn_8055BE58(void) {
    return 5;
}

/* Whether the widget's active child is finished. */
extern "C" u32 fn_8055BEDC(void* current) {
    HbmWidget* child = ((HbmWidget*)current)->child;
    return child->vtable->isDone(child);
}

/* A null slot. */
extern "C" u32 fn_8055BEF0(void* self) {
    (void)self;
    return 0;
}

/* Re-targets the widget's collection entry with a layout name. */
extern "C" void fn_8055C0E8(HbmWidget* self, const char* text) {
    HbmWidget* entry = (HbmWidget*)self->items[0];
    HbmWidget* child = entry->child;
    child->vtable->setText(child, text, 1);
}

/* Releases the widget, then the storage, and returns it. */
extern "C" void* fn_8055C494(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Reads the list node's id and value out, or the empty pair. */
extern "C" void fn_8055C85C(HbmWidgetNodeView* self, HbmIdValue* out) {
    HbmListNode* node = self->node;
    if (node != 0) {
        out->value = node->value;
        out->id = node->id;
    } else {
        out->id = -1;
        out->value = lbl_8079D700;
    }
}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * tail-call thunks: the retail body is `...; b <victim>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
void fn_8055CAB0(VuObject* self, void* node) { fn_80501A64(&self->list_0x08, node); }

/* ---------------------------------------------------------------------------------------------------
 * no-op virtuals: the retail body is a bare `blr`
 * ------------------------------------------------------------------------------------------------ */
void fn_8055CAB8(VuObject* self) { (void)self; }

void fn_8055CABC(VuObject* self) { (void)self; }

void fn_8055CAC0(VuObject* self) { (void)self; }

void fn_8055CAC4(VuObject* self) { (void)self; }

void fn_8055CAC8(VuObject* self) { (void)self; }

void fn_8055CACC(VuObject* self) { (void)self; }

void* fn_8055CD20(void* self, s32 flag)
{
    if (self != 0) {
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* The same three-call chain through slot 0x138 instead of 0x13C. */
void fn_8055D300(VuObject* self, void* a, void* b)
{
    VuObject* first = (VuObject*)((void* (*)(VuObject*))self->vt->slot_068)(self);
    VuObject* second = (VuObject*)((void* (*)(VuObject*))first->vt->slot_0E0)(first);

    ((void (*)(VuObject*, void*, void*))second->vt->slot_138)(second, a, b);
}

/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `return self->sub->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_8055DB78(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_0DC)(self->sub_0x1C); }

void fn_8055DB8C(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_240)(self->sub_0x1C); }

void fn_8055DBA0(VuObject* self) { fn_80546660(self->sub_0x1C); }

void fn_8055DBA8(VuObject* self, void* value) { self->field_0x38 = value; }

void fn_8055DBB0(VuObject* self, void* value) { self->field_0x34 = value; }

void fn_8055DBB8(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_0FC)(self->sub_0x1C); }

void fn_8055DBCC(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_100)(self->sub_0x1C); }

void fn_8055DBE0(VuObject* self) { ((void (*)(VuSub*))self->sub_0x1C->vt->slot_104)(self->sub_0x1C); }

/* A byte-flag setter that dispatches on the new value. */
void fn_8055DCD0(VuObject* self, u8 flag)
{
    self->field_0x3C = flag;
    if (flag == 1)
        ((void (*)(VuObject*))self->vt->slot_0D0)(self);
    else
        ((void (*)(VuObject*))self->vt->slot_0CC)(self);
}

/* Two factories: allocate a sub-object from the owner's allocator, then build or construct it. */
VuHeader* fn_8055DF3C(VuOwner* self)
{
    VuHeader* obj = (VuHeader*)MEMAllocFromAllocator((MEMAllocator*)self->allocator, 0x14);

    if (obj != 0) {
        obj->vt = (void*)lbl_8064F0B0;
        obj->field_0x04 = 0;
        obj->field_0x08 = 0;
        obj->field_0x0C = 0;
    }
    return obj;
}

void fn_8055DF88(VuOwner* self)
{
    void* obj = MEMAllocFromAllocator((MEMAllocator*)self->allocator, 0x28);

    if (obj != 0)
        fn_8056D814(obj, self);
}

/* A plain deleting destructor with no base-destructor call. */
void* fn_8055E59C(void* self, s32 flag)
{
    if (self != 0) {
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

/* ---------------------------------------------------------------------------------------------------
 * accessors
 * ------------------------------------------------------------------------------------------------ */
void* fn_8055E694(VuObject* self) { return self->field_0x20; }

void* fn_8055E778(VuObject* self) { return self->sub_0x1C; }

void* fn_8055E780(VuObject* self) { return self->field_0x24; }

void* fn_8055E788(VuObject* self) { return self->field_0x18; }

void* fn_8055E790(VuObject* self) { return self->field_0x28; }

void* fn_8055E798(VuObject* self) { return self->field_0x28; }

void* fn_8055E7A0(VuObject* self) { return self->field_0x2C; }

void* fn_8055E7A8(VuObject* self) { return self->field_0x14; }

void* fn_8055E7B0(VuObject* self) { return self->field_0x10; }

void fn_8055E7B8(VuObject* self) { (void)self; }

void fn_8055E7BC(VuObject* self) { (void)self; }

/* Three chained virtual calls: slot 0x68 -> slot 0xE0 -> slot 0x13C(a, b). */
void fn_8055E7C0(VuObject* self, void* a, void* b)
{
    VuObject* first = (VuObject*)((void* (*)(VuObject*))self->vt->slot_068)(self);
    VuObject* second = (VuObject*)((void* (*)(VuObject*))first->vt->slot_0E0)(first);

    ((void (*)(VuObject*, void*, void*))second->vt->slot_13C)(second, a, b);
}

void* fn_8055E82C(VuObject* self) { return self->field_0x38; }

void fn_8055E834(VuObject* self) { (void)self; }

void fn_8055E838(VuObject* self, void* value) { self->field_0x14 = value; }

/* ---------------------------------------------------------------------------------------------------
 * deleting destructors: `if (self && flag > 0) delete self;` returns the object (the compiler's own
 * deleting-destructor shape, dispatching into the class's non-deleting destructor first)
 * ------------------------------------------------------------------------------------------------ */
void* fn_8055E840(void* self, s32 flag)
{
    if (self != 0) {
        fn_805385D0(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E898(void* self, s32 flag)
{
    if (self != 0) {
        fn_8053F65C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E8F0(void* self, s32 flag)
{
    if (self != 0) {
        fn_8054F90C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E948(void* self, s32 flag)
{
    if (self != 0) {
        fn_8054CF38(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E9A0(void* self, s32 flag)
{
    if (self != 0) {
        fn_80554714(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055E9F8(void* self, s32 flag)
{
    if (self != 0) {
        fn_80555B0C(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void* fn_8055EA50(void* self, s32 flag)
{
    if (self != 0) {
        fn_80557228(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}
}
