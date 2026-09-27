/*
 * homebutton/fn_80555374.cpp - the 0x80555374-0x8055C894 band (213 functions, 29984 B) of the
 * home-button (Wii HOME menu overlay) software-keyboard block, plus its 3 `.ctors` words at
 * 0x8056F404-0x8056F410.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py lookup for every row of the range - all `zz_`/`FUN_` placeholders - and
 * with a scan of the whole DOL's string pool: no bare source-file name is referenced by any
 * function of the range; the pools it does reference hold the software-keyboard layout vocabulary
 * `P_SGNkey_01`..`12`, `B_SGNkey_close` (0x8057BD18), `T_SGN_pageNumber`, `P_BT_cancel`,
 * `N_UP`/`N_DOWN` and `%d/%d`).
 *
 * Language, seam and module (brief section 2):
 *   - C++: the band is free `fn_XXXXXXXX` functions operating on a class hierarchy only - the
 *     `.ctors` initialisers, the deleting-destructor forms (`__dl__FPv`) and the `subi r3, r3,
 *     0x14/0x1C/0x24/0xC4/0xCC` adjustor thunks in the band settle it; the lib's cflags carry
 *     exceptions, and the target object has `extab`/`extabindex`.
 *   - module `homebutton`: the right link neighbour is the registered `homebutton/keyboard_ui.cpp`
 *     (0x8055C894), the band calls into that range (fn_8055C968), and the data it references is the
 *     same software-keyboard layout pool that neighbour documents.  Evidence class 3.
 *   - the seam is unproven: discovery cut the run at `--max-bytes`, and
 *     `tools/splits/tudiscover.py at 0x80555374` finds no must-link anchor at either edge (its
 *     closure is the one function fn_80555374; its left candidates are weak codegen fingerprints at
 *     0x80554DC8/0x805547F4, its right ones at 0x8055664C/0x80556840).  The right edge is the
 *     registered `homebutton/keyboard_ui.cpp` boundary, so the run is worked as one unit.
 *
 * Residuals:
 *   - partial reconstruction: the bodies below are the ones proved from the disassembly; the rest of
 *     the band is still the target object's bytes.
 *   - the layouts are *views* of the objects the band operates on (the band spans more than one
 *     class: a few bodies use one offset with two meanings, and each such body gets its own view).
 *     Sizes are marked approximate where the band never touches the object's tail.
 *   - the vtables the band dispatches through belong to another unit (`lbl_80650A48`, `.data`
 *     0x80650A48-0x80650B4C), so they are typed slot structs here - declaring the classes would make
 *     MWCC emit their tables into this object.
 */
#include "types.h"

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

struct HbmWidget;

/* `offsetof` for this band's types (MWCC's `stddef.h` is off the include path): the adjustor thunks
 * below convert a base-subobject pointer back to the complete object with it. */
#define HBM_OFFSET_OF(type, field) ((u32)(&((type*)0)->field))

/* The allocator the band's buffers come from (`.0x08` slot + MEMFreeToAllocator).
 * size: 0x4 */
struct HbmAllocator {
    /* +0x00 */ void** vtable;
};

/* The widget's tail flag bytes (fn_80558CD8 reads +0xD0, fn_80558638 +0xD1).  size: 0x4 */
struct HbmTailFlags {
    /* +0x00 */ u8 flag_D0;
    /* +0x01 */ u8 flag_D1;
    /* +0x02 */ u8 pad_0x02[2];
};

/* The widget vtable group the band dispatches through - `lbl_80650A48` in .data, owned by another
 * unit.  size: 0xDC (the declared view; the real group is 0x104 B - three tables at +0x00,
 * +0x24 and +0x98 - and stops where the band stops reading it).  Slots the band never calls keep their offset as `unused_0xNN`. */
struct HbmWidgetVtable {
    /* +0x00 */ void* unused_0x00;
    /* +0x04 */ void* unused_0x04;
    /* +0x08 */ void (*release)(void* self);
    /* +0x0C */ void* unused_0x0C;
    /* +0x10 */ void (*update)(void* self);
    /* +0x14 */ void (*hide)(void* self);
    /* +0x18 */ void* unused_0x18;
    /* +0x1C */ void* unused_0x1C;
    /* +0x20 */ void* unused_0x20;
    /* +0x24 */ void* unused_0x24;
    /* +0x28 */ void* unused_0x28;
    /* +0x2C */ void* unused_0x2C;
    /* +0x30 */ s32 (*isDone)(void* self);
    /* +0x34 */ void* unused_0x34;
    /* +0x38 */ void* unused_0x38;
    /* +0x3C */ void (*setText)(void* self, const char* text, int on);
    /* +0x40 */ void* unused_0x40;
    /* +0x44 */ void* unused_0x44;
    /* +0x48 */ void* unused_0x48;
    /* +0x4C */ void* unused_0x4C;
    /* +0x50 */ void (*setAnm)(void* self, const char* name, int mode);
    /* +0x54 */ void* unused_0x54;
    /* +0x58 */ void* unused_0x58;
    /* +0x5C */ void (*setId)(void* self, int id);
    /* +0x60 */ void* unused_0x60;
    /* +0x64 */ void* unused_0x64;
    /* +0x68 */ void* unused_0x68;
    /* +0x6C */ void* unused_0x6C;
    /* +0x70 */ void* unused_0x70;
    /* +0x74 */ void* unused_0x74;
    /* +0x78 */ void* unused_0x78;
    /* +0x7C */ void* unused_0x7C;
    /* +0x80 */ void* unused_0x80;
    /* +0x84 */ void* unused_0x84;
    /* +0x88 */ void* unused_0x88;
    /* +0x8C */ void* unused_0x8C;
    /* +0x90 */ void* unused_0x90;
    /* +0x94 */ void* unused_0x94;
    /* +0x98 */ void* unused_0x98;
    /* +0x9C */ void* unused_0x9C;
    /* +0xA0 */ void* unused_0xA0;
    /* +0xA4 */ void* unused_0xA4;
    /* +0xA8 */ void* unused_0xA8;
    /* +0xAC */ void* unused_0xAC;
    /* +0xB0 */ void* unused_0xB0;
    /* +0xB4 */ void* unused_0xB4;
    /* +0xB8 */ void* unused_0xB8;
    /* +0xBC */ void* unused_0xBC;
    /* +0xC0 */ void* unused_0xC0;
    /* +0xC4 */ void* unused_0xC4;
    /* +0xC8 */ void* unused_0xC8;
    /* +0xCC */ void* unused_0xCC;
    /* +0xD0 */ void* unused_0xD0;
    /* +0xD4 */ void (*onTick)(void* self);
    /* +0xD8 */ void* unused_0xD8;
};

/* The widget class the band's state machines run on: one complete object whose base subobjects sit
 * at +0x14, +0x1C, +0x24, +0xC4 and +0xCC (each owns a vtable set by the constructor of the unit
 * that owns that base).  size: 0xD4 (approximate - the largest offset the band touches is +0xD0). */
struct HbmWidget {
    /* +0x00 */ HbmWidgetVtable* vtable;
    /* +0x04 */ void* items[1];         /* indexed as `items[i]` (fn_8055BE48) */
    /* +0x08 */ u32 pad_0x08;
    /* +0x0C */ u32 pad_0x0C;
    /* +0x10 */ HbmWidget* child;       /* the widget's active child */
    /* +0x14 */ void* vtable_14;        /* base #1 (adjustor thunk `subi 0x14`) */
    /* +0x18 */ HbmWidget* owner;       /* the widget that owns this one's base #1 */
    /* +0x1C */ void* vtable_1C;        /* base #2 (adjustor thunk `subi 0x1C`) */
    /* +0x20 */ u32 pad_0x20;
    /* +0x24 */ void* vtable_24;        /* base #3 (adjustor thunk `subi 0x24`) */
    /* +0x28 */ u32 pad_0x28;
    /* +0x2C */ union {
                    u32 state;          /* the state-machine selector */
                    HbmWidget* current; /* the child the accessors dispatch to */
                } mode;
    /* +0x30 */ void* resource;         /* released by the destructor */
    /* +0x34 */ u32 pad_0x34[6];
    /* +0x4C */ HbmWidget* node_4C;     /* fn_80555AD8's static initialiser target */
    /* +0x50 */ u32 arg_50;
    /* +0x54 */ u32 pad_0x54[0x0D];
    /* +0x88 */ void* list;             /* intrusive list head (fn_80501C60/fn_80501BF4) */
    /* +0x8C */ u32 pad_0x8C[4];
    /* +0x9C */ u8 flag_9C;
    /* +0x9D */ u8 pad_0x9D[3];
    /* +0xA0 */ u32 count_A0;
    /* +0xA4 */ u32 pad_A4[6];
    /* +0xBC */ void* ptr_BC;
    /* +0xC0 */ void* ptr_C0;
    /* +0xC4 */ void* vtable_C4;        /* base #4 (adjustor thunk `subi 0xC4`) */
    /* +0xC8 */ u8 flag_C8;
    /* +0xC9 */ u8 flag_C9;
    /* +0xCA */ u8 pad_0xCA[2];
    /* +0xCC */ HbmAllocator* allocator;/* freed through MEMFreeToAllocator */
    /* +0xD0 */ union {                  /* one word in fn_8055A43C, single bytes for the flags */
                    u32 word_D0;
                    HbmTailFlags bytes;
                } tail;
};

/* The value / shown-value / dirty cluster fn_80559C50 and its siblings work on.
 * size: 0x28 (approximate) */
struct HbmTextValue {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ u32 value;
    /* +0x20 */ u32 shown;
    /* +0x24 */ u8 dirty;
};

/* The u16-counted array fn_805594C0 writes into.  size: 0x10 */
struct HbmU16Array {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ u16 capacity;
    /* +0x06 */ u16 length;
    /* +0x08 */ u16 offset;              /* a byte offset into `data` */
    /* +0x0A */ u16 pad_0x0A;
    /* +0x0C */ u16* data;
};

/* A second view of the widget where +0x14 holds the band's list node (fn_8055C85C).
 * size: 0x18 */
struct HbmWidgetNodeView {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ struct HbmListNode* node;
};

/* A node of the band's sorted list (fn_8055C85C).  size: 0xC */
struct HbmListNode {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ s32 id;
    /* +0x08 */ f32 value;
};

/* The (id, value) pair fn_8055C85C writes out.  size: 0x8 */
struct HbmIdValue {
    /* +0x00 */ s32 id;
    /* +0x04 */ f32 value;
};

/* The timer cluster fn_8055A40C loads (offsets +0xE8..+0x104).  size: 0x108 (approximate) */
struct HbmTimer {
    /* +0x00 */ u8 pad_0x00[0xE8];
    /* +0xE8 */ f32 start;
    /* +0xEC */ f32 end;
    /* +0xF0 */ f32 base;
    /* +0xF4 */ u32 id;
    /* +0xF8 */ u32 kind;
    /* +0xFC */ f32 a;
    /* +0x100 */ f32 b;
    /* +0x104 */ u8 active;
};

/* The animation-record cluster fn_80556FC4 initialises.  size: 0x1C (approximate) */
struct HbmAnimRecord {
    /* +0x00 */ u8 pad_0x00[0x15];
    /* +0x15 */ u8 flag_15;
    /* +0x16 */ u8 flag_16;
    /* +0x17 */ u8 flag_17;
    /* +0x18 */ const u8* table;   /* table[0] is the record count */
};

/* --------------------------------------------------------------------------------------------- */
/* Callees (their bodies live in their own units)                                                 */
/* --------------------------------------------------------------------------------------------- */

extern "C" u32 fn_805041F4(void* self, int arg);
extern "C" void* fn_80501BF4(void* list, void* node);
extern "C" void* fn_80501C60(void* list, void* node);
extern "C" void fn_80554714(void* self);
extern "C" void fn_80554BA8(void* self);
extern "C" void fn_8055C638(void* self);

/* Callees inside the band, declared ahead of their bodies (address order). */
extern "C" void fn_80555B0C(HbmWidget* self, int flag);
extern "C" void fn_8055BBAC(void* node);
extern "C" void fn_80555FB4(HbmWidget* self, int flag);
extern "C" void fn_805563D0(HbmWidget* self, int flag);
extern "C" void fn_80557228(void* self);
extern "C" void fn_80557744(void* self);
extern "C" void fn_80557E8C(void* self);
extern "C" u32 fn_8055BEDC(void* current);
extern "C" u32 fn_8055BEF0(void* self);
extern "C" void fn_80461780(void* dst, int value, u16 size);
extern "C" void fn_8052B5F8(void* self, int flag);
extern "C" void* fn_8055C494(void* self, int flag);
extern "C" void* fn_80555F5C(void* self, int flag);
extern "C" void* fn_805576EC(void* self, int flag);
extern "C" void* fn_8055B898(void* self, int flag);
extern "C" void* fn_80555BF4(void* self, int flag);
extern "C" void* fn_80557310(void* self, int flag);
extern "C" void* fn_80557350(void* self, int flag);
extern f32 lbl_8079D700;

/* The layout strings the band passes to its widgets (`.sdata` words; their band is unregistered). */
extern char lbl_80794610[5];     /* "N_UP" */
extern char lbl_80794618[7];     /* "N_DOWN" */
extern void* lbl_80794608;       /* widget-string slot the static initialiser copies */
extern void* lbl_8079460C;       /* widget-string slot the static initialiser copies */
extern HbmWidget lbl_806501A0;   /* the band's static widget instance */
extern const u8 lbl_8057BD60[];  /* the animation-record table of fn_80556FC4 */

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

/* Bounds-checked store into the widget's u16 array. */
extern "C" void fn_805594C0(HbmU16Array* self, u16 index, u16 value) {
    if (index < self->capacity) {
        self->data[index] = value;
    }
}

/* Virtual slot at +0x5C: sets the widget's id. */
extern "C" void fn_80559814(HbmWidget* self) {
    self->vtable->setId(self, 10);
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

/* Stores the callback cluster of the band's observer interface. */
extern "C" void fn_8055A3FC(HbmWidget* self, u32 a, void* b, void* c) {
    self->arg_50 = a;
    self->ptr_BC = b;
    self->ptr_C0 = c;
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


/* Sets the animation record's index, wrapping to the first entry. */
extern "C" void fn_80558C98(HbmAnimRecord* self, u8 index) {
    self->flag_17 = index;
    if (index < self->table[0]) {
        return;
    }
    self->flag_17 = 0;
}

/* Releases the widget, then the storage, and returns it. */
extern "C" void* fn_80555BF4(void* self, int flag) {
    if (self != 0) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
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

/* Releases the widget, then the storage, and returns it. */
extern "C" void* fn_8055C494(void* self, int flag) {
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

/* The last used entry of the widget's u16 array. */
extern "C" u16 fn_805595AC(HbmU16Array* self) {
    s16 index = (s16)(self->length - 1);
    if (index < 0) {
        return 0;
    }
    return self->data[index];
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

/* Recomputes the byte offset of the last u16 entry. */
extern "C" void fn_805595D0(HbmU16Array* self) {
    u16 i;
    self->offset = 0;
    for (i = 0; i < self->length; i++) {
        self->offset += 2;
    }
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

/* Clears the widget's u16 array and its counters. */
extern "C" void fn_80559568(HbmU16Array* self) {
    fn_80461780(self->data, 0, self->capacity);
    self->length = 0;
    self->offset = 0;
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
