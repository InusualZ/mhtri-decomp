/*
 * homebutton/hbm_anim_record.cpp - HOME-button (HBM) software-keyboard code, `.text` 0x80556FC4..0x80558EB4 (61 functions, 30 reconstructed).
 *
 * Phase 4 recut: 30 function(s) of 0x80556FC4..0x80558EB4 from the former registered unit `homebutton/fn_80555374.cpp`.
 *
 * Name: GUESS - `hbm_` (the HOME-button menu library, keyboard part) + the animation record that steps an index through its table, and the widget methods around it, from the dominant function descriptions
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

extern "C" void fn_80557228(void* self);

extern "C" void fn_80557744(void* self);

extern "C" void fn_80557E8C(void* self);

extern "C" void wmemset(void* dst, int value, u16 size);

extern "C" void* fn_805576EC(void* self, int flag);

extern "C" void* fn_80557310(void* self, int flag);

extern "C" void* fn_80557350(void* self, int flag);

extern const u8 lbl_8057BD60[];  /* the animation-record table of fn_80556FC4 */

/* Resets one animation record to its table. */
extern "C" void fn_80556FC4(HbmAnimRecord* self) {
    self->flag_17 = 0;
    self->flag_15 = 0;
    self->flag_16 = 0;
    self->table = lbl_8057BD60;
}

/* Resets one animation record to its table. */
extern "C" void fn_80556FE4(HbmAnimRecord* self) {
    self->flag_17 = 0;
    self->flag_15 = 0;
    self->flag_16 = 0;
    self->table = lbl_8057BD60;
}

/* Empty virtual slot. */
extern "C" void fn_805571CC(void) {
}

/* Steps the animation record's index down, wrapping to the last entry. */
extern "C" void fn_805571D0(HbmAnimRecord* self) {
    u8 index = self->flag_17;
    if (index == 0) {
        index = self->table[0];
    }
    self->flag_17 = index - 1;
}

/* Steps the animation record's index up, wrapping to the first entry. */
extern "C" void fn_805571FC(HbmAnimRecord* self) {
    u8 next = self->flag_17 + 1;
    self->flag_17 = next;
    if (next >= self->table[0]) {
        self->flag_17 = 0;
    }
}

/* Releases the widget, then the storage, and returns it. */
extern "C" void* fn_80557310(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the widget, then the storage, and returns it. */
extern "C" void* fn_80557350(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Runs the widget's own destructor, then releases its storage. */
extern "C" void* fn_805576EC(void* self, int flag) {
    if (self != 0) {
        fn_8055C494(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The child the widget is currently dispatching to. */
extern "C" void* fn_8055805C(HbmWidget* self) {
    return self->mode.current;
}

/* Clears the widget's state selector. */
extern "C" void fn_805580E4(HbmWidget* self) {
    self->mode.state = 0;
}

/* Sets the widget's state selector to its "showing" state. */
extern "C" void fn_805581B4(HbmWidget* self) {
    self->mode.state = 6;
}

/* The widget's completion test, when it has one. */
extern "C" u32 fn_80558638(HbmWidget* self) {
    if (self->tail.bytes.flag_D1 != 0) {
        return fn_8055BEDC(&self->vtable_24);
    }
    return 0;
}

/* The widget's completion test, when it has one. */
extern "C" u32 fn_80558654(HbmWidget* self) {
    if (self->tail.bytes.flag_D1 != 0) {
        return fn_8055BEF0(&self->vtable_24);
    }
    return 0;
}

/* Whether the widget is flagged. */
extern "C" u8 fn_805589A0(HbmWidget* self) {
    return ((HbmAnimRecord*)self)->flag_15;
}

/* Stores the child the widget dispatches to and re-sorts its list. */
extern "C" void fn_80558A44(HbmWidget* self, HbmWidget* current) {
    self->mode.current = current;
    fn_8055C638(self);
}

/* Walks the widget's child pane. */
extern "C" void fn_80558C8C(HbmWidget* self) {
    fn_8055BBAC(&self->vtable_24);
}

/* Empty virtual slot. */
extern "C" void fn_80558C94(void) {
}

/* Sets the animation record's index, wrapping to the first entry. */
extern "C" void fn_80558C98(HbmAnimRecord* self, u8 index) {
    self->flag_17 = index;
    if (index < self->table[0]) {
        return;
    }
    self->flag_17 = 0;
}

/* Stores the widget's active child, then updates it. */
extern "C" void fn_80558CBC(HbmWidget* self, HbmWidget* child) {
    self->child = child;
    self->vtable->update(self);
}

/* The widget's type tag. */
extern "C" u32 fn_80558CD0(void) {
    return 2;
}

/* The widget's first tail flag. */
extern "C" u8 fn_80558CD8(HbmWidget* self) {
    return self->tail.bytes.flag_D0;
}

/* Clears the widget's state selector. */
extern "C" void fn_80558D3C(HbmWidget* self) {
    self->mode.state = 0;
}

/* Clears the widget's state selector. */
extern "C" void fn_80558D48(HbmWidget* self) {
    self->mode.state = 0;
}

/* Clears the widget's state selector. */
extern "C" void fn_80558DB0(HbmWidget* self) {
    self->mode.state = 0;
}

/* Adjustor thunk: base #3 -> fn_80557228. */
extern "C" void fn_80558E84(void* base) {
    fn_80557228((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_24)));
}

/* Adjustor thunk: base #3 -> fn_80558638. */
extern "C" void fn_80558E8C(void* base) {
    fn_80558638((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_24)));
}

/* Adjustor thunk: base #3 -> fn_80558654. */
extern "C" void fn_80558E94(void* base) {
    fn_80558654((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_24)));
}

/* Adjustor thunk: base #3 -> fn_80558C8C. */
extern "C" void fn_80558E9C(void* base) {
    fn_80558C8C((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_24)));
}

/* Adjustor thunk: base #3 -> fn_80557744. */
extern "C" void fn_80558EA4(void* base) {
    fn_80557744((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_24)));
}

/* Adjustor thunk: base #5 -> fn_80557E8C. */
extern "C" void fn_80558EAC(HbmAllocator* base) {
    fn_80557E8C((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, allocator)));
}
