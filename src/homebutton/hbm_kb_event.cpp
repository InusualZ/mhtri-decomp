/*
 * homebutton/hbm_kb_event.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x8056083C..0x80566440 (97 functions, 59 reconstructed).
 *
 * Phase 4 fold: 18 function(s) of 0x8056083C..0x805632BC from the former registered unit `homebutton/keyboard_ui.cpp`.
 * Phase 4 fold: 41 function(s) of 0x805632BC..0x80566440 from the former registered unit `homebutton/keyboard.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the event forwarders, state dispatchers and the nibble dispatcher of the keyboard object, from the dominant function descriptions
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

#include "homebutton/hbm_vu_object.h"
#include "homebutton/hbm_kb_object.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_value.h"
#include "nw4r/fn_805012C4.h"

/* ---------------------------------------------------------------------------------------------------
 * helpers that live in other splits.  They sit in the unsplit `main` band (fn_80501xxx / fn_8054xxx /
 * fn_8053xxx), so no registered unit owns them - rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"), the same way the neighbouring `homebutton/gui.cpp` declares them.
 * ------------------------------------------------------------------------------------------------ */
extern "C" {

s32   wcschr(void* base, void* key);

s32   strlen(const char* s);

s32   strncmp(const char* a, const char* b, u32 n);

char* strncpy(char* dst, const char* src, u32 n);

void* memset(void* dst, s32 c, u32 n);

/* The two dispatchers of the registered neighbour `homebutton/keyboard.cpp` that fn_8056329C tail-calls. */
void fn_805632BC(void* self);

void fn_805634A4(void* self);

}

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata2` float pool and `.sdata` globals: other splits own them, so they are
 * declared, never defined (playbook 29).
 * ------------------------------------------------------------------------------------------------ */
extern "C" {

extern f32 lbl_807946D8[2];  /* a small-data record whose float at +4 is written by fn_8056167C */

}


/* The compiler's own operator delete: the retail object jumps to `__dl__FPv`. */
void operator delete(void* ptr) throw();

/* Character/keystroke-name strings the keyboard searches its layout with (the two `.sdata` tables at
 * 0x80794728/0x80794738) and the pane name `T_Header` the memo button is looked up by. */

/* The unit's own `.sdata2` pool: another split owns it, so it is declared, never defined
 * (playbook 29). */

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * chained-sub-object calls
 * ------------------------------------------------------------------------------------------------ */
void fn_8056083C(VuObject* self, VuSub* arg)
{
    self->field_0x0C = arg;
    ((void (*)(VuSub*, VuObject*))arg->vt->slot_01C)(arg, self);
}

void fn_8056085C(VuObject* self, void* a, void* b)
{
    VuSub* node = (VuSub*)fn_80501C60(&self->list_0x04, 0);

    while (node != 0) {
        ((void (*)(VuSub*, void*, void*))node->vt->slot_01C)(node, a, b);
        node = (VuSub*)fn_80501C60(&self->list_0x04, node);
    }
}

void fn_805608EC(VuObject* self) { MEMInitList(&self->list_0x04, 4); }

/* A deleting destructor whose base sub-object sits at +4 of the object. */
void* fn_80560B68(VuObject* self, s32 flag)
{
    if (self != 0) {
        fn_8055B7D4((void*)&self->list_0x04, 0);
        if (flag > 0)
            operator delete(self);
    }
    return self;
}

void fn_80560FB4(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_020)(self->sub_0x40); }

void fn_80560FC8(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_018)(self->sub_0x40); }

void fn_80560FDC(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_01C)(self->sub_0x40); }

void fn_80560FF0(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_024)(self->sub_0x40); }

void fn_80561004(VuObject* self) { ((void (*)(VuSub*))self->sub_0x40->vt->slot_028)(self->sub_0x40); }

/* Event forwarders: a state change pushes the wrapped object's value through a virtual slot. */
void fn_80561018(VuObject* self, s32 state)
{
    if (state != 1)
        return;
    void* value = ((void* (*)(VuSub*))self->sub_0x40->vt->slot_00C)(self->sub_0x40);
    ((void (*)(VuObject*, void*))self->vt->slot_0EC)(self, value);
}

/* ---------------------------------------------------------------------------------------------------
 * stores into the unit's small-data globals and the float pools
 * ------------------------------------------------------------------------------------------------ */
void fn_8056167C(f32 value) { lbl_807946D8[1] = value; }

void fn_80562244(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_114)(self);
    self->field_0x44 = 0xB;
    ((void (*)(VuSub*, s32))self->sub_0x1C->vt->slot_23C)(self->sub_0x1C, 1);
}

void fn_80562BFC(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_114)(self);
    self->field_0x44 = 8;
    ((void (*)(VuSub*, s32))((VuSub*)self->field_0x18)->vt->slot_104)((VuSub*)self->field_0x18, 1);
}

void fn_80562C54(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_128)(self);
    self->field_0x44 = 9;
}

void fn_80562FE4(VuObject* self)
{
    ((void (*)(VuObject*))self->vt->slot_100)(self);
    self->field_0x44 = 0xD;
    ((void (*)(VuSub*, s32))((VuSub*)self->field_0x18)->vt->slot_108)((VuSub*)self->field_0x18, 0);
    ((void (*)(VuSub*, s32))((VuSub*)self->field_0x14)->vt->slot_120)((VuSub*)self->field_0x14, 0);
}

/* ---------------------------------------------------------------------------------------------------
 * guarded forwarders: `if (state < 1 || state >= 3) return; <wrapped>->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563054(VuObject* self)
{
    if (self->field_0x44 >= 3)
        return;
    if (self->field_0x44 < 1)
        return;
    ((void (*)(VuSub*))self->sub_0x1C->vt->slot_258)(self->sub_0x1C);
}

void fn_80563080(VuObject* self)
{
    if (self->field_0x44 >= 3)
        return;
    if (self->field_0x44 < 1)
        return;
    ((void (*)(VuSub*))self->sub_0x1C->vt->slot_25C)(self->sub_0x1C);
}

/* The nibble dispatcher: tail-calls the registered neighbour according to bits 4..7 of +0x48. */
void fn_8056329C(VuObject* self)
{
    switch ((self->field_0x48 >> 4) & 0xF) {
    case 1:
        fn_805632BC(self);
        break;
    default:
        fn_805634A4(self);
        break;
    }
}
}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * no-op virtuals: the retail body is a bare `blr`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563610(KbObject* self) { (void)self; }

/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `return self->sub->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563614(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_0F0)(self->sub_0x04); }

/* ---------------------------------------------------------------------------------------------------
 * accessors
 * ------------------------------------------------------------------------------------------------ */
void* fn_80563628(KbObject* self) { return self->field_0x44; }

/* ---------------------------------------------------------------------------------------------------
 * state dispatchers: `switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) { case 1: case 2: <body> }`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563630(KbObject* self)
{
    switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) {
    case 1:
    case 2:
        ((Wrapped* (*)(void))self->vt->slot_034)()->vt->slot_14C();
        break;
    }
}

void fn_805636A4(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_068)(self->sub_0x04); }

void fn_805636B8(KbObject* self) { (void)self; }

void fn_805636BC(KbObject* self)
{
    if (((KbStateFn)self->sub_0x04->vt->slot_0F0)() == 1 || ((KbStateFn)self->sub_0x04->vt->slot_0F0)() == 2)
        ((Wrapped* (*)(void))self->vt->slot_034)()->vt->slot_148();
}

s32 fn_80563740(KbObject* self, s32 a, f32 x, f32 y, s32 b, s32 c, s32 d)
{
    s32 ret;

    switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) {
    case 1:
    case 2: {
        /* the retail object hands the callee a copy of the argument tuple as the 8th argument */
        KbArgs args;
        self->vt->slot_034();
        args.a = a;
        args.x = x;
        args.y = y;
        args.b = b;
        args.c = c;
        args.d = d;
        ret = (s32)((s32 (*)(f32, f32, s32, s32, s32, s32, KbArgs*))self->vt->slot_224)(x, y, a, b, c, d, &args);
        break;
    }
    default:
        ret = 0;
        break;
    }
    return ret;
}

void fn_80563834(KbObject* self, void* value)
{
    switch (((KbStateFn)self->sub_0x04->vt->slot_0F0)()) {
    case 1:
    case 2:
        Wrapped* receiver;
        self->vt->slot_034();
        receiver = ((Wrapped* (*)(void))self->vt->slot_034)();
        ((void (*)(Wrapped*, void*))receiver->vt->slot_228)(receiver, value);
        break;
    }
}

void fn_80563DEC(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_078)(self->sub_0x04); }

void fn_80563E00(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_080)(self->sub_0x04); }

void fn_80563E14(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_090)(self->sub_0x04); }

void fn_80563E28(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_098)(self->sub_0x04); }

void* fn_80563E3C(KbObject* self) { return self->sub_0x04->field_0x78; }

void fn_80563E48(KbObject* self) { (void)self; }

void fn_80563E4C(KbObject* self) { (void)self; }

f32 fn_805644F8(KbObject* self) { return self->tail_0x1A04.float_0x104; }

f32 fn_80564500(KbObject* self) { return self->tail_0x1A04.float_0x108; }

/* ---------------------------------------------------------------------------------------------------
 * tail-call thunks: the retail body is `b <victim>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
void fn_805646E4(KbObject* self) { fn_8055BBAC(&self->sub_0x04); }

/* ---------------------------------------------------------------------------------------------------
 * constant returns
 * ------------------------------------------------------------------------------------------------ */
s32 fn_80564770(KbObject* self) { (void)self; return 0; }

s32 fn_80564778(KbObject* self) { (void)self; return 0; }

void fn_805647F4(KbObject* self) { (void)self; }

void fn_805647F8(KbObject* self) { (void)self; }

void fn_80564A2C(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_0A0)(self->sub_0x04); }

void fn_80564A40(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_088)(self->sub_0x04); }

void fn_805655B0(KbObject* self) { ((void (*)(Wrapped*))self->sub_0x04->vt->slot_070)(self->sub_0x04); }

void fn_805661C8(KbObject* self) { (void)self; }

void fn_805661CC(KbObject* self) { (void)self; }

void fn_805661D0(KbObject* self) { (void)self; }

void fn_805661D4(KbObject* self) { (void)self; }

s32 fn_805661D8(KbObject* self) { (void)self; return 3; }

void fn_805661E0(KbObject* self) { (void)self; }

s32 fn_805661E4(KbObject* self) { (void)self; return 2; }

/* ---------------------------------------------------------------------------------------------------
 * the five deleting destructors of the animation-widget classes: `if (self && flag > 0) delete self;`
 * returns the object (the retail body is the compiler's own deleting-destructor shape)
 * ------------------------------------------------------------------------------------------------ */
void* fn_805661EC(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_8056622C(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

s32 fn_8056626C(KbObject* self) { (void)self; return 1; }

void fn_80566274(KbObject* self) { (void)self; }

s32 fn_80566278(KbObject* self) { (void)self; return 0; }

void* fn_80566280(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_805662C0(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_80566300(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}
}
