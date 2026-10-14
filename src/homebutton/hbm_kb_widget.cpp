/*
 * homebutton/hbm_kb_widget.cpp - software-keyboard code of the tiHKBManager band: the keyboard widget's state
 * selector, property handlers, page stepping, flag accessors, deleting destructors and adjustor thunks.
 *
 * RANGE.     .text 0x8054F7BC..0x805546B4 (102 functions), .ctors 0x8056F400..0x8056F404, .rodata 0x8057B2F0..0x8057B668,
 *            .data 0x8064F360..0x80650160; the functions not defined below keep the target object's bytes.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (see src/homebutton/hbm_kb_event.cpp); no extab/extabindex in the target, so
 *            `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the keyboard widget class; no `__FILE__` string and no
 *            runtime-dump name.
 * NAMES.     GUESS type HkbWidgetProperty (the record the widget's slot +0x18 fills and applies).
 * NAMES.     GUESS function names (map rows renamed): hkbKbWidgetSetPropertyFlag (stores +0x17E8 and applies it as
 *            property 0x1D), hkbKbWidgetHandleByteProperty (property ids 0x22/0x23 through slots +0x40/+0x44).
 * SHAPES.    Signed compares (`cmpwi`, `ble`) read the widget's words as `s32`: fn_80551314 casts +0x1A90,
 *            fn_80551E5C switches on `(s32)state`, fn_80553F84 takes its selector as `s32`.
 * RESIDUALS. The constructors (fn_80553F50, fn_80553FC8, fn_80554278) store the unit's own `.data` vtables at +0x00
 *            and +0x14: they need their classes reconstructed (rule 10), not hand-written stores.
 * RESIDUALS. fn_80551404, fn_8055136C, fn_80550BA4, fn_80550C30 and fn_80550CCC read widget words (+0x17F0, +0x18F0,
 *            +0x1A94) that the shared view in homebutton/hkb_widget.h does not name yet.
 * RESIDUALS. fn_805542BC is declared `(HkbWidget*)` in homebutton/hbm_kb_widget.h; its body takes
 *            (self, s32, u32 id, HkbFlagRecord*), so it waits for the declaration.
 * RESIDUALS. datagap: the claimed `.data` 0x8064F360..0x80650160 spans at least 7 TUs (emission-order seams at
 *            0x8064FFC4, 0x8064FFF0, 0x8065001C, 0x80650038, 0x80650080, 0x806500A0) - a seam question for the
 *            registration.
 * RESIDUALS. 41 functions are unwritten: 0x8054F90C-0x8054F9F4, 0x8054FBB4-0x80550124, 0x80550128-0x80550488,
 *            0x805504EC-0x80550768, 0x80550770-0x805508F4, 0x80550910-0x80550ACC, 0x80550BA4-0x80550CBC,
 *            0x80550CCC-0x805512B4, 0x8055136C-0x8055148C, 0x805514E0-0x80551680, 0x805516E4-0x80551964,
 *            0x80551970-0x80551E5C, 0x80551F4C-0x80553670, 0x80553680-0x80553F84, 0x80553FC8-0x80554268,
 *            0x80554278-0x805544C0, 0x80554600-0x80554664.
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "homebutton/hkb_widget.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_kb_widget.h"
#include "homebutton/hbm_value.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

/* Empty virtual slot. */
extern "C" void fn_8054F7BC(void) {
}

/* The widget's own update, then the +0x10 virtual slot. */
extern "C" void fn_8054F7C0(HkbWidget* self) {
    ((void (*)(HkbWidget*))fn_8056083C)(self);
    self->v_0x010();
}

/* Empty virtual slot. */
extern "C" void fn_8054F800(void) {
}

/* Copies the caller's keyboard-layout block over the record's own. */
extern "C" void fn_8054F804(HkbLayoutRecord* self, void* source) {
    memcpy(self->buffer_10, source, 0x17D8);
}

/* The property record the widget's slot +0x18 fills (id 0x1F) and applies (id 0x1D): a flag byte at +0x04.
 * size: 0x08 */
struct HkbWidgetProperty {
    /* +0x00 */ u32 pad_00;
    /* +0x04 */ u8 flag_04;
    /* +0x05 */ u8 pad_05[3];
};

/* Stores the +0x17E8 flag, then reads property 0x1F, sets its flag and applies it as property 0x1D. */
extern "C" void hkbKbWidgetSetPropertyFlag(HkbWidget* self, u8 flag) {
    HkbWidgetProperty prop;
    self->flag_17E8 = flag;
    self->v_0x018(0x1F, &prop);
    prop.flag_04 = flag;
    self->v_0x018(0x1D, &prop);
}

/* Property handler: id 0x22 sets the widget's byte through slot +0x40, id 0x23 reads it back through slot +0x44. */
extern "C" void hkbKbWidgetHandleByteProperty(HkbWidget* self, s32 id, u8* value) {
    if (id == 0x22) {
        self->v_0x040(*value);
    }
    if (id == 0x23) {
        *value = self->v_0x044();
    }
}

/* Stores the widget's +0x17E9 flag. */
extern "C" void fn_8054F8FC(HkbWidget* self, u8 flag) {
    self->flag_17E9 = flag;
}

/* The widget's +0x17E9 flag. */
extern "C" u8 fn_8054F904(HkbWidget* self) {
    return self->flag_17E9;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054F9F4(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054FA34(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054FA74(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054FAB4(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054FAF4(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054FB34(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_8054FB74(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Empty virtual slot. */
extern "C" void fn_80550124(void) {
}

/* Clears the widget's state selector. */
extern "C" void fn_80550488(HkbWidget* self) {
    self->state = 0;
}

/* Runs the destructor entry, then releases the storage, and returns the pointer. */
extern "C" void* fn_80550494(void* self, int flag) {
    if (self != 0) {
        fn_8055C494(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Stores the keyboard widget's +0x1B88 byte. */
extern "C" void fn_80550768(HkbWidget* self, u8 flag) {
    self->flag_1B88 = flag;
}

/* Stores the keyboard widget's +0x17EC word, then dispatches slot +0x10 on itself. */
extern "C" void fn_805508F4(HkbWidget* self, u32 value) {
    self->value_17EC = value;
    self->v_0x010();
}

/* Stores the keyboard widget's +0x17EC word. */
extern "C" void fn_80550908(HkbWidget* self, u32 value) {
    self->value_17EC = value;
}

/* Turns the widget's two mode flags off and back on in order. */
extern "C" void fn_80550ACC(HkbWidget* self) {
    self->v_0x03C(0);
    self->v_0x0E0(0);
    self->v_0x03C(1);
}

/* Turns the widget's two mode flags on and back off in order. */
extern "C" void fn_80550B38(HkbWidget* self) {
    self->v_0x03C(1);
    self->v_0x0E0(0);
    self->v_0x03C(0);
}

/* Stores the keyboard widget's +0x1A88 word. */
extern "C" void fn_80550CBC(HkbWidget* self, u32 value) {
    self->value_1A88 = value;
}

/* Stores the keyboard widget's +0x1A8C word. */
extern "C" void fn_80550CC4(HkbWidget* self, u32 value) {
    self->value_1A8C = value;
}

/* Advances the widget's page and re-targets it. */
extern "C" void fn_805512B4(HkbWidget* self) {
    if (self->index_1AA4 < self->count_1A98 - 1) {
        fn_80553074(self->sub_18B8, 1);
        self->child_1808->v_0x014(11);
    }
}

/* Steps the widget's page back and re-targets it. */
extern "C" void fn_80551314(HkbWidget* self) {
    if ((s32)self->value_1A90 > 0) {
        fn_80553074(self->sub_18B8, -1);
        self->child_1808->v_0x014(11);
    }
}

/* Virtual slot +0x4C of base #5, then the widget's own +0x11C slot. */
extern "C" void fn_8055148C(HkbWidget* self) {
    HkbWidget* base = (HkbWidget*)&self->vtable_17F4;
    base->v_0x04C();
    self->v_0x11C();
}

/* Empty virtual slot. */
extern "C" void fn_805514DC(void) {
}

/* The keyboard widget's +0x30 child. */
extern "C" void* fn_80551680(HkbWidget* self) {
    return self->current;
}

/* The keyboard widget's state selector. */
extern "C" u32 fn_80551688(HkbWidget* self) {
    return self->state;
}

/* Stores the state selector and re-sorts the widget's list. */
extern "C" void fn_80551690(HkbWidget* self, u32 state) {
    self->state = state;
    fn_8055C638(self);
}

/* Stores the +0x17E9 flag and re-targets the pane it selects. */
extern "C" void fn_80551698(HkbWidget* self, u8 flag) {
    if (flag == self->flag_17E9) {
        return;
    }
    self->flag_17E9 = flag;
    if (flag != 0) {
        self->pane_1B84->v_0x014(11);
        return;
    }
    self->pane_1B84->v_0x014(12);
}

/* The keyboard widget's +0x1A8C word. */
extern "C" u32 fn_80551964(HkbWidget* self) {
    return self->value_1A8C;
}

/* Empty virtual slot. */
extern "C" void fn_8055196C(void) {
}

/* Steps the widget's page one way or the other for the caller's event. */
extern "C" void fn_80551E5C(HkbWidget* self, s32 event) {
    switch ((s32)self->state) {
    case 11:
        if (event == 4) {
            self->v_0x014(10);
        }
        break;
    case 12:
        if (event == 4) {
            self->v_0x014(0);
        }
        break;
    }
}

/* Virtual slot +0x10 on base #5, then the widget's own +0x110 slot. */
extern "C" void fn_80551EB0(HkbWidget* self) {
    fn_8055C1D4(&self->vtable_17F4);
    self->v_0x110();
}

/* The width the two +0x186C/+0x1874 timers span. */
extern "C" void fn_80551EF4(HkbWidget* self) {
    self->value_1AAC = self->value_1874 - self->value_186C;
}

/* Virtual slot +0x10 on base #5, then the widget's own +0x110 slot. */
extern "C" void fn_80551F08(HkbWidget* self) {
    fn_8055C2CC(&self->vtable_17F4);
    self->v_0x110();
}

/* Dispatches the base subobject at +0x18 with the caller's argument. */
extern "C" void fn_80553670(HkbWidget* self, u32 arg) {
    ((HkbWidget*)&self->table_18)->v_0x014(arg);
}

/* Whether the sub-object's +0x10 child is set, then its +0x08 slot. */
extern "C" void fn_80553F84(void* self, s32 arg1, u32 arg2, HkbFlagRecord* rec) {
    HkbWidget* child;
    (void)arg2;
    if (rec->flag_04 != 0) {
        return;
    }
    if (arg1 != 1) {
        return;
    }
    child = ((HkbChild10*)self)->child_10;
    if (child == 0) {
        return;
    }
    child->v_0x008(self, 1024, 0);
}

/* The u16 element at index `index` of the array at +0x80. */
extern "C" u16 fn_80554268(HkbU16Array* self, u32 index) {
    return self->values_80[index];
}

/* The low half of the keyboard widget's +0x1A90 word. */
extern "C" u16 fn_805544C0(HkbWidget* self) {
    return (u16)self->value_1A90;
}

/* Forwards the caller's pair to the widget's +0x18 slot. */
extern "C" void fn_805544CC(HkbWidget* self, u32 id, void* arg) {
    self->v_0x018(id, arg);
}

/* Empty virtual slot. */
extern "C" void fn_805544DC(void) {
}

/* Empty virtual slot. */
extern "C" void fn_805544E0(void) {
}

/* Empty virtual slot. */
extern "C" void fn_805544E4(void) {
}

/* Empty virtual slot. */
extern "C" void fn_805544E8(void) {
}

/* Runs the destructor entry, then releases the storage, and returns the pointer. */
extern "C" void* fn_805544EC(void* self, int flag) {
    if (self != 0) {
        if (self != 0) {
            fn_8055C494(self, 0);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Runs the destructor entry, then releases the storage, and returns the pointer. */
extern "C" void* fn_80554548(void* self, int flag) {
    if (self != 0) {
        if (self != 0) {
            fn_8055C494(self, 0);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Runs the destructor entry, then releases the storage, and returns the pointer. */
extern "C" void* fn_805545A4(void* self, int flag) {
    if (self != 0) {
        if (self != 0) {
            fn_8055C494(self, 0);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Adjustor thunk: base #3 -> fn_805536FC. */
extern "C" void fn_80554664(HkbWidget* self) {
    fn_805536FC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)));
}

/* Adjustor thunk: base #3 -> fn_805542BC. */
extern "C" void fn_8055466C(HkbWidget* self) {
    fn_805542BC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)));
}

/* Adjustor thunk: base #3 -> fn_80553F84. */
extern "C" void fn_80554674(HkbWidget* self, s32 arg1, u32 arg2, HkbFlagRecord* rec) {
    fn_80553F84((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)), arg1, arg2, rec);
}

/* Adjustor thunk: base #5 -> fn_8054F90C. */
extern "C" void fn_8055467C(HkbWidget* self, s32 flag) {
    fn_8054F90C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)), flag);
}

/* Adjustor thunk: base #5 -> fn_80551F08. */
extern "C" void fn_80554684(HkbWidget* self) {
    fn_80551F08((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> fn_80551EB0. */
extern "C" void fn_8055468C(HkbWidget* self) {
    fn_80551EB0((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> fn_80550804. */
extern "C" void fn_80554694(HkbWidget* self) {
    fn_80550804((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> fn_80550770. */
extern "C" void fn_8055469C(HkbWidget* self) {
    fn_80550770((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> fn_805504EC. */
extern "C" void fn_805546A4(HkbWidget* self) {
    fn_805504EC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #6 -> fn_805516E4. */
extern "C" void fn_805546AC(HkbWidget* self) {
    fn_805516E4((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_189C)));
}
