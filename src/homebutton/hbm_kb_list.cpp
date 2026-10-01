/*
 * homebutton/hbm_kb_list.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x805546B4..0x80555B0C (37 functions, 20 reconstructed).
 *
 * Phase 4 fold: 12 function(s) of 0x805546B4..0x80555374 from the former registered unit `homebutton/fn_8054E894.cpp`.
 * Phase 4 fold: 8 function(s) of 0x80555374..0x80555B0C from the former registered unit `homebutton/fn_80555374.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the keyboard widget list/state-selector methods and layout-name setters, from the dominant function descriptions
 * below; the band has no `__FILE__` string and the runtime dump names no function, so every function keeps its `fn_<addr>`
 * placeholder.  The band was built with C++ exceptions off (no extab/extabindex in the target objects), so the scoped
 * `#pragma exceptions off` below is carried from the absorbed sources and applied to every piece.
 *
 * Residuals: partial reconstruction - the functions not defined below keep the target object's bytes; the evidence, type views and
 * per-function residuals of the absorbed sources are in docs/splits/phase4/homebutton-carried-notes.md; the type views are in
 * `homebutton/fn_8054E894.h`, `homebutton/hbm_widget.h`, `homebutton/hbm_vu_object.h`, `homebutton/hbm_kb_object.h` and `homebutton/gui_manager.h`.
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "homebutton/fn_8054E894.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"

extern "C" void fn_80554714(void* self);

extern "C" void fn_80554BA8(void* self);

extern "C" void fn_80461780(void* dst, int value, u16 size);

/* The layout strings the band passes to its widgets (`.sdata` words; their band is unregistered). */
extern char lbl_80794610[5];     /* "N_UP" */

extern char lbl_80794618[7];     /* "N_DOWN" */

extern void* lbl_80794608;       /* widget-string slot the static initialiser copies */

extern void* lbl_8079460C;       /* widget-string slot the static initialiser copies */

extern HbmWidget lbl_806501A0;   /* the band's static widget instance */

/* Stores the object's +0x04 flag, then re-runs the child's +0x68 slot and commits it. */
extern "C" void fn_805546B4(HkbFlagChild* self, u8 flag) {
    void* value;
    self->flag_04 = flag;
    value = self->child_10->v_0x068();
    if (value != 0) {
        fn_8054CDCC(self->child_10->v_0x068());
    }
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_805547F4(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Stores the keyboard widget's +0x0C word. */
extern "C" void fn_80554AD8(HkbWidget* self, u32 value) {
    self->value_0C = value;
}

/* Runs the destructor entry, then releases the storage, and returns the pointer. */
extern "C" void* fn_80554AE0(void* self, int flag) {
    if (self != 0) {
        fn_8055C494(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Empty virtual slot. */
extern "C" void fn_80554DBC(void) {
}

/* The keyboard widget's +0x08 word. */
extern "C" u32 fn_80554DC0(HkbWidget* self) {
    return self->value_08;
}

/* The keyboard widget's state selector. */
extern "C" u32 fn_80554F00(HkbWidget* self) {
    return self->state;
}

/* Stores the state selector and re-sorts the widget's list. */
extern "C" void fn_80554F08(HkbWidget* self, u32 state) {
    self->state = state;
    fn_8055C638(self);
}

/* The child the widget is currently dispatching to. */
extern "C" void* fn_80554F10(HkbWidget* self) {
    return self->current;
}

/* Stores the +0x08 word, then dispatches the widget's own +0x0C slot with it. */
extern "C" void fn_80555110(HkbWidget* self, u32 value) {
    self->value_08 = value;
    self->v_0x00C(value);
}

/* Stores the keyboard widget's +0x08 word. */
extern "C" void fn_80555124(HkbWidget* self, u32 value) {
    self->value_08 = value;
}

/* Re-targets the widget's pane and then the widget itself. */
extern "C" void fn_8055512C(HkbWidget* self, void* value) {
    if (self->v_0x010() == value) {
        return;
    }
    if (value != 0) {
        self->pane_28->v_0x014(22);
    } else {
        self->pane_28->v_0x014(23);
    }
    self->v_0x014((u32)value);
}

/* --------------------------------------------------------------------------------------------- */
/* Bodies (address order)                                                                         */
/* --------------------------------------------------------------------------------------------- */

/* Re-targets the widget's own child pane with a layout name. */
extern "C" void fn_8055540C(HbmWidget* self) {
    HbmWidget* child = self->owner->child;
    child->vtable->setText(child, lbl_80794610, 1);
}

/* Re-targets the widget's own child pane with a layout name. */
extern "C" void fn_8055542C(HbmWidget* self) {
    HbmWidget* child = self->owner->child;
    child->vtable->setText(child, lbl_80794618, 1);
}

/* Adjustor thunk: base #1 -> fn_8055BBAC. */
extern "C" void fn_80555A0C(HbmWidget* self) {
    fn_8055BBAC(&self->vtable_14);
}

/* Clears the widget's state selector. */
extern "C" void fn_80555A14(HbmWidget* self) {
    self->mode.state = 0;
}

/* .ctors: copies the static widget's own layout strings into its instance. */
extern "C" void fn_80555AD8(void) {
    lbl_806501A0.node_4C = (HbmWidget*)lbl_80794608;
    lbl_806501A0.allocator = (HbmAllocator*)lbl_8079460C;
}

/* Adjustor thunk: base #1 -> fn_80554714. */
extern "C" void fn_80555AF4(void* base) {
    fn_80554714((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_14)));
}

/* Adjustor thunk: base #1 -> fn_80555A0C. */
extern "C" void fn_80555AFC(void* base) {
    fn_80555A0C((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_14)));
}

/* Adjustor thunk: base #1 -> fn_80554BA8. */
extern "C" void fn_80555B04(void* base) {
    fn_80554BA8((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_14)));
}
