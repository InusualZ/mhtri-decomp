/*
 * homebutton/hbm_kb_cursor.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x80566440..0x80569DAC (85 functions, 30 reconstructed).
 *
 * Phase 4 recut: 30 function(s) of 0x80566440..0x80569DAC from the former registered unit `homebutton/keyboard.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the keyboard cursor position and the file-input-stream destructors, from the dominant function descriptions
 * below; the band has no `__FILE__` string and the runtime dump names no function, so every function keeps its `fn_<addr>`
 * placeholder.  The band was built with C++ exceptions off (no extab/extabindex in the target objects), so the scoped
 * `#pragma exceptions off` below is carried from the absorbed sources and applied to every piece.
 *
 * Residuals: partial reconstruction - the functions not defined below keep the target object's bytes; the evidence, type views and
 * per-function residuals of the absorbed sources are in docs/splits/phase4/homebutton-carried-notes.md; the type views are in
 * `homebutton/fn_8054E894.h`, `homebutton/hbm_widget.h`, `homebutton/hbm_vu_object.h`, `homebutton/hbm_kb_object.h` and `homebutton/gui_manager.h`.
 */

#include "types.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_kb_object.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"


/* The compiler's own operator delete: the retail object jumps to `__dl__FPv`. */
void operator delete(void* ptr) throw();

/* Character/keystroke-name strings the keyboard searches its layout with (the two `.sdata` tables at
 * 0x80794728/0x80794738) and the pane name `T_Header` the memo button is looked up by. */
extern "C" {

extern char lbl_80794728[8];

extern char lbl_80794738[8];

extern char lbl_80657E14[];

}

/* The unit's own `.sdata2` pool: another split owns it, so it is declared, never defined
 * (playbook 29). */
extern "C" {

extern f32 lbl_8079D750;

extern f32 lbl_8079D754;

extern f32 lbl_8079D774;

extern f32 lbl_8079D784;

}

extern "C" {

void* fn_80567004(KbObject* self, void* value)
{
    void* old = self->sub_0x04;
    self->sub_0x04 = (Wrapped*)value;
    return old;
}

/* ---------------------------------------------------------------------------------------------------
 * guarded forwarders: `if (self->tail_0x1A04.field_0x1E4 != 1) return; <wrapped>->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_8056772C(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((void (*)(Wrapped*, s32, s32))self->tail_0x1A04.field_0x1DC->vt->slot_014)(self->tail_0x1A04.field_0x1DC, 0, 7);
}

void fn_805677C0(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((void (*)(Wrapped*, s32, s32))self->tail_0x1A04.field_0x1DC->vt->slot_014)(self->tail_0x1A04.field_0x1DC, 1, 7);
}

void fn_80567D3C(void* p) { fn_8054E05C(p); }

/* ---------------------------------------------------------------------------------------------------
 * the 0x1B04.. key-frame block
 * ------------------------------------------------------------------------------------------------ */
void fn_80568004(KbObject* self, f32 value)
{
    f32 out;

    self->tail_0x1A04.float_0x100 = value;
    if (self->tail_0x1A04.field_0x1E4 == 2)
        out = -value;
    else
        out = lbl_8079D754;
    self->float_0x100 = out;
}

void fn_80568C60(KbObject* self)
{
    if (self->tail_0x1A04.float_0x110 == self->tail_0x1A04.float_0x118)
        self->tail_0x1A04.float_0x118 = self->tail_0x1A04.float_0x118 + lbl_8079D774;
    if (self->tail_0x1A04.float_0x114 == self->tail_0x1A04.float_0x11C)
        self->tail_0x1A04.float_0x114 = self->tail_0x1A04.float_0x114 + lbl_8079D774;
    (void)fn_8055B174(&self->field_0x010, &self->tail_0x1A04.float_0x110);
}

void fn_80568DBC(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        fn_80547BE8(self);
}

/* The 2D anchored position of the keyboard's cursor: the object's own pair when one is attached, the
 * shared constant pair otherwise. */
Vec2f fn_80568DD0(KbObject* self)
{
    Vec2f out;

    if (self->tail_0x1A04.field_0x0F4 == 0) {
        out.x = lbl_8079D750;
        out.y = lbl_8079D750;
        return out;
    }
    out.x = self->tail_0x1A04.field_0x0F4->float_0x44;
    out.y = self->tail_0x1A04.field_0x0F4->float_0x48;
    return out;
}

/* ---------------------------------------------------------------------------------------------------
 * the keyboard's pane/text lookups (the layout vocabulary of the `.data` pool)
 * ------------------------------------------------------------------------------------------------ */
void fn_805692DC(KbObject* self)
{
    KbTail* found = (KbTail*)((void* (*)(KbTail*, char*))self->tail_0x1A04.vt->slot_05C)(&self->tail_0x1A04, lbl_80794728);

    ((void (*)(KbTail*))found->vt->slot_018)(found);
}

void fn_8056931C(KbObject* self, void* arg)
{
    Wrapped* pane = (Wrapped*)((void* (*)(Wrapped*, char*, s32))self->tail_0x1A04.field_0x004->field_0x10->vt->slot_03C)(self->tail_0x1A04.field_0x004->field_0x10, lbl_80657E14, 1);

    ((void (*)(void*, void*, s32))pane->vt->slot_07C)(pane, arg, 0);
}

/* ---------------------------------------------------------------------------------------------------
 * text-search wrappers: the character name (lbl_80794738) is looked up in the keyboard's own list
 * ------------------------------------------------------------------------------------------------ */
void fn_8056945C(KbObject* self)
{
    KbTail* base = &self->tail_0x1A04;

    ((void (*)(KbTail*, char*))base->vt->slot_02C)(base, lbl_80794738);
}

void fn_80569470(KbObject* self)
{
    KbTail* base = &self->tail_0x1A04;

    ((void (*)(KbTail*, char*))base->vt->slot_02C)(base, lbl_80794738);
    ((void (*)(KbTail*, char*, s32))base->vt->slot_040)(base, lbl_80794738, 1);
}

f32 fn_805694B8(KbObject* self) { (void)self; return lbl_8079D754; }

f32 fn_805694C0(KbObject* self)
{
    f32 frame = ((f32 (*)(KbObject*))self->vt->slot_2A4)(self);
    return frame - lbl_8079D784;
}

void fn_8056985C(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((void (*)(Wrapped*))self->tail_0x1A04.field_0x1D0->vt->slot_018)(self->tail_0x1A04.field_0x1D0);
    fn_80546BCC(self, arg);
}

void fn_80569910(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((void (*)(Wrapped*))self->tail_0x1A04.field_0x1D0->vt->slot_010)(self->tail_0x1A04.field_0x1D0);
    fn_8055A888((HbmWidget*)&self->field_0x010, (int)arg);
}

void fn_80569970(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((void (*)(Wrapped*, s32))self->tail_0x1A04.field_0x1D0->vt->slot_010)(self->tail_0x1A04.field_0x1D0, 0);
}

void fn_80569998(KbObject* self) { (void)self; }

void fn_80569B98(void* p) { fn_8055C638(p); }

void fn_80569B9C(KbObject* self) { (void)self; }

void fn_80569BA0(KbObject* self) { (void)self; }

void* fn_80569BA4(KbObject* self) { return self->field_0x2C; }

void fn_80569BAC(KbObject* self, void* value)
{
    self->field_0x2C = value;
    fn_8055C638(self);
}

void* fn_80569BB4(KbObject* self) { return self->field_0x30; }

void  fn_80569BBC(KbObject* self) { self->field_0x2C = 0; }

/* The two file-input-stream destructors: the inner check is the class's own non-deleting destructor,
 * which the retail object keeps as a second `if (this != 0)`. */
void* fn_80569BC8(void* self, s32 flag)
{
    if (self != 0) {
        if (self != 0)
            fn_8055C494(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void fn_80569C24(KbObject* self) { (void)self; }

void* fn_80569C28(void* self, s32 flag)
{
    if (self != 0) {
        if (self != 0)
            fn_8055C494(self, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void  fn_80569C94(KbObject* self, void* value) { self->tail_0x1A04.field_0x1C8 = value; }

f32 fn_80569C9C(KbObject* self) { return self->tail_0x1A04.float_0x100; }
}
