/*
 * homebutton/hbm_kb_child.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x80555B0C..0x80556FC4 (36 functions, 16 reconstructed).
 *
 * Phase 4 recut: 16 function(s) of 0x80555B0C..0x80556FC4 from the former registered unit `homebutton/fn_80555374.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the widget child-dispatch methods (active child, per-frame tick, adjustor thunks), from the dominant function descriptions
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

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_value.h"

/* Callees inside the band, declared ahead of their bodies (address order). */
extern "C" void fn_80555B0C(HbmWidget* self, int flag);

extern "C" void fn_80555FB4(HbmWidget* self, int flag);

extern "C" void fn_805563D0(HbmWidget* self, int flag);

extern "C" void fn_80461780(void* dst, int value, u16 size);

extern "C" void* fn_80555F5C(void* self, int flag);

extern "C" void* fn_80555BF4(void* self, int flag);

/* Releases the widget, then the storage, and returns it. */
extern "C" void* fn_80555BF4(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Runs the widget's own destructor, then releases its storage. */
extern "C" void* fn_80555F5C(void* self, int flag) {
    if (self != 0) {
        fn_8055C494(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Stores the widget's active child. */
extern "C" void fn_8055614C(HbmWidget* self, HbmWidget* child) {
    self->child = child;
}

/* Flags the widget and updates the child it is dispatching to. */
extern "C" void fn_80556154(HbmWidget* self) {
    HbmWidget* child;
    self->flag_C9 = 1;
    child = self->mode.current;
    child->vtable->update(child);
}

/* Whether the widget is flagged. */
extern "C" u8 fn_80556A5C(HbmWidget* self) {
    return self->flag_C9;
}

/* Stores the child the widget dispatches to and re-sorts its list. */
extern "C" void fn_80556A90(HbmWidget* self, HbmWidget* current) {
    self->mode.current = current;
    fn_8055C638(self);
}

/* Empty virtual slot. */
extern "C" void fn_80556DBC(void) {
}

/* Adjustor thunk: base #2 -> fn_8055BBAC. */
extern "C" void fn_80556DC0(HbmWidget* self) {
    fn_8055BBAC(&self->vtable_1C);
}

/* Virtual slot at +0xD4: the widget's per-frame tick. */
extern "C" void fn_80556DC8(HbmWidget* self) {
    self->vtable->onTick(self);
}

/* Whether the widget has been initialised. */
extern "C" u8 fn_80556DD8(HbmWidget* self) {
    return self->flag_C8;
}

/* The child the widget is currently dispatching to. */
extern "C" void* fn_80556DE0(HbmWidget* self) {
    return self->mode.current;
}

/* Clears the widget's state selector. */
extern "C" void fn_80556DE8(HbmWidget* self) {
    self->mode.state = 0;
}

/* Adjustor thunk: base #3 -> fn_80555B0C. */
extern "C" void fn_80556FA4(void* base, int flag) {
    fn_80555B0C((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_1C)), flag);
}

/* Adjustor thunk: base #3 -> fn_80556DC0. */
extern "C" void fn_80556FAC(void* base) {
    fn_80556DC0((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_1C)));
}

/* Adjustor thunk: base #3 -> fn_80555FB4. */
extern "C" void fn_80556FB4(void* base, int flag) {
    fn_80555FB4((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_1C)), flag);
}

/* Adjustor thunk: base #4 -> fn_805563D0. */
extern "C" void fn_80556FBC(void* base, int flag) {
    fn_805563D0((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_C4)), flag);
}
