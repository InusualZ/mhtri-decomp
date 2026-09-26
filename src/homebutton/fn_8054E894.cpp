// homebutton software-keyboard band, slice 0x8054E894-0x80555374 (199 functions / 27360 B) plus the
// two `.ctors` words at 0x8056F3FC-0x8056F404 (the static initialisers fn_8054F550 / fn_80554600).
//
// Evidence for module and name (brief section 2 classes): no `__FILE__` string covers the range and
// the shared runtime dump answers only `zz_054e894_`/`FUN_` placeholders (class 4), so the file keeps
// the symbol map's `fn_8054E894` stem.  The module is evidence class 3: the range's `.data`
// vocabulary is the home-button software-keyboard layout pool the registered siblings
// `homebutton/keyboard.cpp` / `homebutton/keyboard_ui.cpp` / `homebutton/fn_80555374.cpp` document
// (`fs_VK_*.brlyt`, `T_2l_TextBox`, `P_txtScrll_UP/DOWN`, `T_prdc_Text_00..19`, `B_CPkey_00..11`,
// `P_SGNkey_00..19`, `P_key_00..49`), its right edge is the registered `homebutton/fn_80555374.cpp`,
// and its vtables point into the sibling bands.  Language C++ from the object's own structure
// (adjustor thunks `addi r3, r3, -0x04/-0x10/-0x14/-0x17F4/-0x189C/-0x1A04`, the deleting
// destructors' `__dl__FPv`, the `.ctors` words).
//
// rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
// `python tools/symbols/dumpmap.py lookup 0x8054E894` -> `zz_054e894_` and a `__FILE__` scan of the
// range's `.data` pool 0x8064E760-0x8064F600, which holds only layout/pane names).
//
// Seam: unproven, and the range is *not* one object.  Both edges are the brief's `--max-bytes` cut
// (the left neighbour fn_8054E858 ends exactly at 0x8054E894, the right edge is the registered
// `homebutton/fn_80555374.cpp`), and dtk's own auto-object fragmentation places further object starts
// inside it: `auto_fn_8054F550` (0x8054F550), `auto_03_8054F6AC`, `auto_fn_80554600` (0x80554600)
// and `auto_03_80554664`.  The two `auto_fn_*` boundaries coincide with the `.ctors` words at
// 0x8056F3FC / 0x8056F400, so the run carries at least five objects.  It is registered as one unit
// because that is what the campaign's proposal is; the boundaries are recorded here as a hint for
// whoever splits it, and they are why the views in the header are declared per function group.
//
// Residuals:
//   - partial reconstruction: the bodies below are the ones proved from the disassembly; the rest of
//     the band is still the target object's bytes (see the outbox for the inventory of what is left).
//   - the classes are *views* (header): the band spans more than one class and one offset is reused
//     with two meanings, so a body that needs the other meaning gets its own view struct.
//   - the widget is declared as a polymorphic class in the header purely for its *dispatch* shape: a
//     struct of function pointers stages the table through a temporary, a `virtual` declaration gives
//     retail's `lwz r12, 0(r3); lwz r12, <slot>(r12)`.  No method is defined and nothing is
//     instantiated, so MWCC emits no table into this object (rule 10); the tables belong to the units
//     above.  A call through a *base* subobject (`lwzu r12, 24(r3)`) still needs real C++ bases and
//     is the recorded blocker for fn_80553670 / fn_80554F18 / fn_80555014.
/* The band was built with C++ exceptions off: every registered target object of the home-button
 * keyboard block (`homebutton/fn_80555374.o`, `keyboard_ui.o`, `keyboard.o`, `tiHKBManager.o`, and
 * the `auto_*` objects this slice's symbols are split into) carries `.text` + `.comment` only, while
 * the `main` lib's `cflags` pass `-Cpp_exceptions on` for `main.cpp`'s benefit.  With the pass on our
 * object gains extab 0x80 + extabindex 0xC0 the target does not have; the pragma removes both and
 * leaves `.text` byte-identical (measured at the state it was probed in: 109/199 functions
 * exact, unit 10.0398 % with and without it).
 * Playbook row 30's per-file lever; the band-wide fix is a `cflags_homebutton` group (outbox). */
#pragma exceptions off

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "homebutton/fn_8054E894.h"
#include "homebutton/fn_80555374.h"
#include "homebutton/keyboard_ui.h"

/* --------------------------------------------------------------------------------------------- */
/* Bodies (address order)                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* The keyboard widget's +0x1AB4 byte. */
extern "C" u8 fn_8054EEBC(HkbWidget* self) {
    return self->flag_1AB4;
}

/* The keyboard widget's +0x1AB5 byte. */
extern "C" u8 fn_8054EEC4(HkbWidget* self) {
    return self->flag_1AB5;
}

/* The keyboard widget's state selector. */
extern "C" u32 fn_8054EECC(HkbWidget* self) {
    return self->state;
}

/* Stores the state selector and re-sorts the widget's list. */
extern "C" void fn_8054F120(HkbWidget* self, u32 state) {
    self->state = state;
    fn_8055C638(self);
}

/* Empty virtual slot. */
extern "C" void fn_8054F128(void* self) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F12C(void* self) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F130(void* self) {
}

/* Adjustor thunk: base #1 -> fn_80501A64. */
extern "C" void fn_8054F134(HkbWidget* self) {
    fn_80501A64(&self->vtable_04);
}

/* The keyboard widget's +0xCC word. */
extern "C" u32 fn_8054F13C(HkbWidget* self) {
    return self->field_CC;
}

/* The keyboard widget's +0x104 byte. */
extern "C" u8 fn_8054F144(HkbWidget* self) {
    return self->flag_104;
}

/* The subobject at +0xBC. */
extern "C" void* fn_8054F14C(HkbWidget* self) {
    return &self->field_BC;
}

/* The keyboard widget's +0x98 timer. */
extern "C" void fn_8054F154(HkbWidget* self, f32 value) {
    self->value_98 = value;
}

/* Stores base #1's table pointer. */
extern "C" void fn_8054F15C(HkbWidget* self, void* value) {
    self->vtable_04 = value;
}

/* Virtual slot +0x124 of the sub-widget at +0x1940. */
extern "C" void fn_8054F164(HkbWidget* self) {
    HkbWidget* child = self->child_1940;
    child->v_0x124();
}

/* Stores the keyboard widget's +0x19FA byte and +0x19FC word from the caller's pair. */
extern "C" void fn_8054F178(HkbWidget* self, u8* flag, u32* value) {
    self->flag_19FA = *flag;
    self->value_19FC = *value;
}

/* Stores the +0x1998 word, then re-targets the widget when the value is "shown". */
extern "C" void fn_8054F3DC(HkbWidget* self, u32 value) {
    self->value_1998 = value;
    if (value != 1) {
        return;
    }
    self->v_0x108(0);
}

/* Stores the keyboard widget's +0x1994 word. */
extern "C" void fn_8054F400(HkbWidget* self, u32 value) {
    self->value_1994 = value;
}

/* Stores the keyboard widget's +0x1990 counter. */
extern "C" void fn_8054F408(HkbWidget* self, u32 value) {
    self->count_1990 = value;
}

/* Virtual slot +0x50 of the sub-widget at +0x193C. */
extern "C" void fn_8054F410(HkbWidget* self) {
    HkbWidget* child = self->child_193C;
    child->v_0x050();
}

/* Virtual slot +0x48 of the sub-widget at +0x193C. */
extern "C" void fn_8054F424(HkbWidget* self) {
    HkbWidget* child = self->child_193C;
    child->v_0x048();
}

/* The child the widget is currently dispatching to. */
extern "C" void* fn_8054F438(HkbWidget* self) {
    return self->current;
}

/* Clears the widget's state selector. */
extern "C" void fn_8054F440(HkbWidget* self) {
    self->state = 0;
}

/* Runs the destructor entry, then releases the storage, and returns the pointer. */
extern "C" void* fn_8054F44C(void* self, int flag) {
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

/* Whether the widget's node list is empty. */
extern "C" u32 fn_8054F4A8(HkbWidget* self) {
    return fn_80526F00(&self->state, 0, 0) == 0;
}

/* Empty virtual slot. */
extern "C" void fn_8054F4E0(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F4E4(void) {
}

/* Empty virtual slot returning "no". */
extern "C" u32 fn_8054F4E8(void) {
    return 0;
}

/* Empty virtual slot returning "no". */
extern "C" u32 fn_8054F4F0(void) {
    return 0;
}

/* Empty virtual slot. */
extern "C" void fn_8054F4F8(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F4FC(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F500(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F504(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F508(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F50C(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F510(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F514(void) {
}

/* Empty virtual slot returning "no". */
extern "C" u32 fn_8054F518(void) {
    return 0;
}

/* Empty virtual slot returning "no". */
extern "C" u32 fn_8054F520(void) {
    return 0;
}

/* Empty virtual slot. */
extern "C" void fn_8054F528(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F52C(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F530(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F534(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F538(void) {
}

/* Empty virtual slot. */
extern "C" void fn_8054F53C(void) {
}

/* The object's +0x15 byte. */
extern "C" u8 fn_8054F540(HkbFlagView* self) {
    return self->flag_15;
}

/* Stores the object's +0x15 byte. */
extern "C" void fn_8054F548(HkbFlagView* self, u8 flag) {
    self->flag_15 = flag;
}

/* Adjustor thunk: base #2 -> fn_80545DAC. */
extern "C" void fn_8054F6AC(HkbWidget* self) {
    fn_80545DAC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_80545C44. */
extern "C" void fn_8054F6B4(HkbWidget* self) {
    fn_80545C44((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #7 -> fn_8054CF38. */
extern "C" void fn_8054F6BC(HkbWidget* self) {
    fn_8054CF38((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #2 -> fn_80547BE8. */
extern "C" void fn_8054F6C4(HkbWidget* self) {
    fn_80547BE8((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_80545DCC. */
extern "C" void fn_8054F6CC(HkbWidget* self) {
    fn_80545DCC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_8054734C. */
extern "C" void fn_8054F6D4(HkbWidget* self) {
    fn_8054734C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_805472C4. */
extern "C" void fn_8054F6DC(HkbWidget* self) {
    fn_805472C4((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_8054F130. */
extern "C" void fn_8054F6E4(HkbWidget* self) {
    fn_8054F130((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_80546BCC. */
extern "C" void fn_8054F6EC(HkbWidget* self) {
    fn_80546BCC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_8054F12C. */
extern "C" void fn_8054F6F4(HkbWidget* self) {
    fn_8054F12C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_80547114. */
extern "C" void fn_8054F6FC(HkbWidget* self) {
    fn_80547114((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_8054E180. */
extern "C" void fn_8054F704(HkbWidget* self) {
    fn_8054E180((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_80546658. */
extern "C" void fn_8054F70C(HkbWidget* self) {
    fn_80546658((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_8054D818. */
extern "C" void fn_8054F714(HkbWidget* self) {
    fn_8054D818((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> fn_8054F128. */
extern "C" void fn_8054F71C(HkbWidget* self) {
    fn_8054F128((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #7 -> fn_8054E858. */
extern "C" void fn_8054F724(HkbWidget* self) {
    fn_8054E858((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> fn_8054E81C. */
extern "C" void fn_8054F72C(HkbWidget* self) {
    fn_8054E81C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> fn_8054DDD8. */
extern "C" void fn_8054F734(HkbWidget* self) {
    fn_8054DDD8((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> fn_8054E05C. */
extern "C" void fn_8054F73C(HkbWidget* self) {
    fn_8054E05C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> fn_8054D9F0. */
extern "C" void fn_8054F744(HkbWidget* self) {
    fn_8054D9F0((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> fn_8054D818. */
extern "C" void fn_8054F74C(HkbWidget* self) {
    fn_8054D818((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> fn_8054D634. */
extern "C" void fn_8054F754(HkbWidget* self) {
    fn_8054D634((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Dispatches the target's own update with the node's argument. */
extern "C" void fn_8054F75C(HkbNode* self) {
    HkbWidget* target = self->target;
    if (target == 0) {
        return;
    }
    target->v_0x020(&self->argument);
}

/* Clears the node's argument and dispatches the target's own reset when one is installed. */
extern "C" void fn_8054F788(HkbNode* self) {
    HkbWidget* target = self->target;
    self->argument = 0;
    if (target == 0) {
        return;
    }
    target->v_0x030(&self->argument);
}

/* Empty virtual slot. */
extern "C" void fn_8054F7BC(void) {
}

/* The widget's own update, then the +0x10 virtual slot. */
extern "C" void fn_8054F7C0(HkbWidget* self) {
    fn_8056083C(self);
    self->v_0x010();
}

/* Empty virtual slot. */
extern "C" void fn_8054F800(void) {
}

/* Copies the caller's keyboard-layout block over the record's own. */
extern "C" void fn_8054F804(HkbLayoutRecord* self, void* source) {
    memcpy(self->buffer_10, source, 0x17D8);
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
    if (self->value_1A90 > 0) {
        fn_80553074(self->sub_18B8, -1);
        self->child_1808->v_0x014(11);
    }
}

/* Empty virtual slot. */
extern "C" void fn_805514DC(void) {
}

/* Virtual slot +0x4C of base #5, then the widget's own +0x11C slot. */
extern "C" void fn_8055148C(HkbWidget* self) {
    HkbWidget* base = (HkbWidget*)&self->vtable_17F4;
    base->v_0x04C();
    self->v_0x11C();
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

/* The width the two +0x186C/+0x1874 timers span. */
extern "C" void fn_80551EF4(HkbWidget* self) {
    self->value_1AAC = self->value_1874 - self->value_186C;
}

/* Virtual slot +0x10 on base #5, then the widget's own +0x110 slot. */
extern "C" void fn_80551EB0(HkbWidget* self) {
    fn_8055C1D4(&self->vtable_17F4);
    self->v_0x110();
}

/* Virtual slot +0x10 on base #5, then the widget's own +0x110 slot. */
extern "C" void fn_80551F08(HkbWidget* self) {
    fn_8055C2CC(&self->vtable_17F4);
    self->v_0x110();
}

/* Stores the keyboard widget's +0x0C word. */
extern "C" void fn_80554AD8(HkbWidget* self, u32 value) {
    self->value_0C = value;
}

/* Stores the keyboard widget's +0x08 word. */
extern "C" void fn_80555124(HkbWidget* self, u32 value) {
    self->value_08 = value;
}

/* Dispatches the base subobject at +0x18 with the caller's argument. */
extern "C" void fn_80553670(HkbWidget* self, u32 arg) {
    ((HkbWidget*)&self->table_18)->v_0x014(arg);
}

/* Whether the sub-object's +0x10 child is set, then its +0x08 slot. */
extern "C" void fn_80553F84(void* self, u32 arg1, u32 arg2, HkbFlagRecord* rec) {
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

/* Adjustor thunk: base #3 -> fn_805536FC. */
extern "C" void fn_80554664(HkbWidget* self) {
    fn_805536FC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)));
}

/* Adjustor thunk: base #3 -> fn_805542BC. */
extern "C" void fn_8055466C(HkbWidget* self) {
    fn_805542BC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)));
}

/* Adjustor thunk: base #3 -> fn_80553F84. */
extern "C" void fn_80554674(HkbWidget* self, u32 arg1, u32 arg2, HkbFlagRecord* rec) {
    fn_80553F84((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)), arg1, arg2, rec);
}

/* Adjustor thunk: base #5 -> fn_8054F90C. */
extern "C" void fn_8055467C(HkbWidget* self) {
    fn_8054F90C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
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

/* Releases the object's storage and returns the pointer. */
extern "C" void* fn_805547F4(void* self, int flag) {
    if (self != 0) {
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

/* Stores the object's +0x04 flag, then re-runs the child's +0x68 slot and commits it. */
extern "C" void fn_805546B4(HkbFlagChild* self, u8 flag) {
    void* value;
    self->flag_04 = flag;
    value = self->child_10->v_0x068();
    if (value != 0) {
        fn_8054CDCC(self->child_10->v_0x068());
    }
}

/* Steps the widget's page one way or the other for the caller's event. */
extern "C" void fn_80551E5C(HkbWidget* self, u32 event) {
    if (self->state == 12) {
        if (event != 4) {
            return;
        }
        self->v_0x014(0);
        return;
    }
    if (self->state < 11) {
        return;
    }
    if (self->state > 12) {
        return;
    }
    if (event != 4) {
        return;
    }
    self->v_0x014(10);
}
