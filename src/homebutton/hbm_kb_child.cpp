/*
 * homebutton/hbm_kb_child.cpp - software-keyboard code of the tiHKBManager band: the keyboard child widget (the region
 * key panes and the key-top part): open/close, region selection, its state selectors, accessors, the parts' deleting
 * destructors and the adjustor thunks.
 *
 * RANGE.     .text 0x80555B0C..0x80556FC4 (36 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the child widget; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS function names hbmKbChild* (read off the bodies: the panes they bind, the states they set), view classes
 *            HbmChildView / HbmChildBaseView / HbmChildPaneView / HbmChildEntryView (slots named by offset) and records
 *            HbmChildWidget / HbmChildPaneRecord; map rows renamed with them (request hbm-misc).
 * SHAPES.    The views are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no
 *            method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * SHAPES.    hbmKbChildStepFinished is one `&&` condition: retail's `bnelr; bnelr; bctr; blr` tail.
 * SHAPES.    hbmKbChildStepStateEx switches on `(u32)state` (retail's `cmplwi` bound check before the jump table).
 * RESIDUALS. hbmKbChildSetRegion: retail addresses its three literals off the section's first object 0x80650500
 *            (+0x3CC/+0x3DC/+0x3EC); ours sit at other offsets until the unit's .data is emitted in order.
 * RESIDUALS. Unwritten, store vtables whose classes this unit would have to emit (rule 10): fn_80555B0C (destructor),
 *            fn_80555C34 (constructor).
 * RESIDUALS. Unwritten, read this unit's own tables (0x80650810 region panes, 0x80650870 key panes, 0x80650510 key
 *            table) that the source does not emit yet: fn_80556560, fn_805565F8, fn_8055664C, fn_80556F08 (the
 *            static initialiser).
 * RESIDUALS. Unwritten, call hbm_hermite.cpp's hbmNameMatches:
 *            fn_805563D0, fn_805564B4, fn_80556840.
 */

#include "types.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_value.h"

/* Callees inside the band, declared ahead of their bodies (address order). */
extern "C" void fn_80555B0C(HbmWidget* self, int flag);


extern "C" void fn_805563D0(HbmWidget* self, int flag);

extern "C" void wmemset(void* dst, int value, u16 size);

extern "C" void* fn_80555F5C(void* self, int flag);

extern "C" void* fn_80555BF4(void* self, int flag);

/* The dispatch view of the widget and of the child it dispatches to: slot 0x10 updates, 0x14 sets the state, 0x18
 * exchanges a value by id, 0xD4 is the per-frame tick with the region.
 * Declared as virtuals so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no method is
 * defined and nothing is constructed here, so no table is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmChildView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 state);
    /* slot +0x018 */ virtual void v_0x018(s32 id, HbmWidget** value);
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020(HbmWidget* owner);
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void v_0x068();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
    /* slot +0x078 */ virtual void v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual void v_0x098();
    /* slot +0x09C */ virtual void v_0x09C();
    /* slot +0x0A0 */ virtual void v_0x0A0();
    /* slot +0x0A4 */ virtual void v_0x0A4();
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual void v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4(const char* name);
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4(s32 region);
};

/* A pane found by name through the widget's +0x1C base: slot 0x14 sets its state.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmChildPaneView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 state);
};

/* The widget's +0x1C base: slot 0x50 binds a named pane animation, slot 0x5C finds a pane by name.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmChildBaseView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050(const char* name, s32 mode);
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual HbmChildPaneView* v_0x05C(const char* name);
};

/* The widget's +0x20 entry: slot 0x30 binds the value block, slot 0x38 sets a mode.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmChildEntryView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030(HbmChildView** binding);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038(s32 mode);
};

/* The pane record the widget's +0x04 word points at: its name at +0xBC.  size: 0xDC */
struct HbmChildPaneRecord {
    /* +0x00 */ u8 unused_0x00[0xBC];
    /* +0xBC */ char name[0x20];
}; /* size: 0xDC */

/* The keyboard child widget as the bodies below read it: the pane record (+0x04), the widget base (+0x1C) and its
 * entry (+0x20) and value block (+0x34), the state selector (+0x2C), the key-top part (+0x30), the open/flag bytes
 * (+0xC8/+0xC9), the region (+0xD0) and the popup (+0xD4).  size: 0xD8 (approximation: bounded by the popup word) */
struct HbmChildWidget {
    /* +0x00 */ HbmChildView* vtable_view;
    /* +0x04 */ HbmChildPaneRecord* pane;
    /* +0x08 */ u8 unused_0x08[0x08];
    /* +0x10 */ HbmWidget* child;
    /* +0x14 */ u8 unused_0x14[0x08];
    /* +0x1C */ u32 base_1C;
    /* +0x20 */ HbmChildEntryView* entry;
    /* +0x24 */ u8 unused_0x24[0x08];
    /* +0x2C */ s32 state;
    /* +0x30 */ HbmChildPaneView* keytop;
    /* +0x34 */ HbmChildView* binding;
    /* +0x38 */ u8 unused_0x38[0x90];
    /* +0xC8 */ u8 open;
    /* +0xC9 */ u8 active;
    /* +0xCA */ u8 unused_0xCA[0x06];
    /* +0xD0 */ s32 region;
    /* +0xD4 */ HbmChildView* popup;
}; /* size: 0xD8 */

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

/* 0x80555FB4 (0xAC): resets the key-top pane and rebinds the widget base's entry to its value block. */
extern "C" void hbmKbChildUpdate(HbmChildWidget* self) {
    ((HbmChildBaseView*)&self->base_1C)->v_0x05C("N_PRDCkeytop_all")->v_0x014(0);
    ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkeytop_all", 0);
    self->entry->v_0x038(0);
    self->entry->v_0x030(&self->binding);
}

/* 0x80556060 (0xEC): opens the widget for an owner with a popup: ticks the region, binds the owner, plays the key-top
 * animation and selects the key-top part. */
extern "C" void hbmKbChildOpen(HbmChildWidget* self, HbmWidget* owner, HbmChildView* popup) {
    self->popup = popup;
    ((HbmChildView*)self)->v_0x0D4(self->region);
    ((HbmChildView*)self)->v_0x020(owner);
    self->open = 1;
    self->active = 0;
    ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkeytop_all", 1);
    ((HbmChildView*)self)->v_0x0C8();
    ((HbmChildBaseView*)&self->base_1C)->v_0x05C("N_PRDCkeytop_all")->v_0x014(5);
    self->keytop->v_0x014(19);
}

/* Stores the widget's active child. */
extern "C" void fn_8055614C(HbmWidget* self, HbmWidget* child) {
    self->child = child;
}

/* Flags the widget and updates the child it is dispatching to. */
extern "C" void fn_80556154(HbmWidget* self) {
    self->flag_C9 = 1;
    ((HbmChildView*)self->mode.current)->v_0x010();
}

/* 0x80556170 (0x60): leaves the key-top pane in state 6 and clears the active flag. */
extern "C" void hbmKbChildDeactivate(HbmChildWidget* self) {
    ((HbmChildBaseView*)&self->base_1C)->v_0x05C("N_PRDCkeytop_all")->v_0x014(6);
    self->active = 0;
}

/* 0x805561D0 (0xA8): queries value 31, hands the child back as value 29, closes the popup and stops the key-top
 * animation. */
extern "C" void hbmKbChildClose(HbmChildWidget* self) {
    HbmWidget* value;

    ((HbmChildView*)self)->v_0x018(31, &value);
    value = self->child;
    ((HbmChildView*)self)->v_0x018(29, &value);
    if (self->popup != 0) {
        self->popup->v_0x030();
    }
    self->open = 0;
    ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkeytop_all", 0);
}

/* 0x80556278 (0x158): stores the region (0 JP, 1 US, 2 EU) and shows that region's key pane only. */
extern "C" void hbmKbChildSetRegion(HbmChildWidget* self, s32 region) {
    self->region = region;
    switch (region) {
    case 0:
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_JP", 1);
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_US", 0);
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_EU", 0);
        break;
    case 1:
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_JP", 0);
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_US", 1);
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_EU", 0);
        break;
    case 2:
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_JP", 0);
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_US", 0);
        ((HbmChildBaseView*)&self->base_1C)->v_0x050("N_PRDCkey_EU", 1);
        break;
    }
}

/* Whether the widget is flagged. */
extern "C" u8 fn_80556A5C(HbmWidget* self) {
    return self->flag_C9;
}

/* 0x80556A64 (0x2C): leaves state 5 for state 0 on event 4. */
extern "C" void hbmKbChildStepFinished(HbmChildWidget* self, s32 event) {
    if (self->state == 5 && event == 4) {
        ((HbmChildView*)self)->v_0x014(0);
    }
}

/* Stores the child the widget dispatches to and re-sorts its list. */
extern "C" void fn_80556A90(HbmWidget* self, HbmWidget* current) {
    self->mode.current = current;
    fn_8055C638(self);
}

/* 0x80556A98 (0x170): steps the five-state selector on an input event (0 also forces state 4). */
extern "C" void hbmKbChildStepState(HbmChildWidget* self, s32 event) {
    if (event == 0) {
        ((HbmChildView*)self)->v_0x014(4);
    }
    switch (self->state) {
    case 0:
        if (event == 1) {
            ((HbmChildView*)self)->v_0x014(1);
        }
        break;
    case 1:
        if (event == 4) {
            ((HbmChildView*)self)->v_0x014(3);
        }
        if (event == 2) {
            ((HbmChildView*)self)->v_0x014(2);
        }
        break;
    case 3:
        if (event == 2) {
            ((HbmChildView*)self)->v_0x014(2);
        }
        break;
    case 2:
        if (event == 4) {
            ((HbmChildView*)self)->v_0x014(0);
        }
        break;
    case 4:
        if (event == 4) {
            ((HbmChildView*)self)->v_0x014(3);
        }
        if (event == 2) {
            ((HbmChildView*)self)->v_0x014(2);
        }
        break;
    }
}

/* 0x80556C08 (0x1B4): steps the nine-state selector on an input event; event 5 outside state 0 forces state 8 and
 * state 4 hands the pane name to the value block's slot 0xC4 on event 3. */
extern "C" void hbmKbChildStepStateEx(HbmChildWidget* self, s32 event) {
    if (event == 0) {
        ((HbmChildView*)self)->v_0x014(4);
    }
    if (event == 5 && self->state != 0) {
        ((HbmChildView*)self)->v_0x014(8);
    }
    switch ((u32)self->state) {
    case 0:
        if (event == 1) {
            ((HbmChildView*)self)->v_0x014(1);
        }
        break;
    case 1:
        if (event == 4) {
            ((HbmChildView*)self)->v_0x014(3);
        }
        if (event == 2) {
            ((HbmChildView*)self)->v_0x014(2);
        }
        break;
    case 3:
        if (event == 2) {
            ((HbmChildView*)self)->v_0x014(2);
        }
        break;
    case 2:
    case 8:
        if (event == 4) {
            ((HbmChildView*)self)->v_0x014(0);
        }
        if (event == 1) {
            ((HbmChildView*)self)->v_0x014(1);
        }
        break;
    case 4:
        if (event == 3) {
            self->binding->v_0x0C4(self->pane->name);
        }
        if (event == 4) {
            ((HbmChildView*)self)->v_0x014(7);
        }
        break;
    }
}

/* Empty virtual slot. */
extern "C" void fn_80556DBC(void) {
}

/* Adjustor thunk: base #2 -> fn_8055BBAC. */
extern "C" void fn_80556DC0(HbmWidget* self) {
    fn_8055BBAC(&self->vtable_1C);
}

/* Virtual slot at +0xD4: the widget's per-frame tick, handed the region. */
extern "C" void fn_80556DC8(HbmWidget* self, s32 region) {
    ((HbmChildView*)self)->v_0x0D4(region);
}

/* Whether the widget has been initialised. */
extern "C" u8 fn_80556DD8(HbmWidget* self) {
    return self->flag_C8;
}

/* The widget's resource word (+0x30). */
extern "C" void* fn_80556DE0(HbmWidget* self) {
    return self->resource;
}

/* Clears the widget's state selector. */
extern "C" void fn_80556DE8(HbmWidget* self) {
    self->mode.state = 0;
}

/* 0x80556DF4 (0x5C): deleting destructor of a child part (base destructor, then the storage). */
extern "C" HbmWidget* hbmKbChildPartDeleteA(HbmWidget* self, s32 flag) {
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

/* 0x80556E50 (0x5C): deleting destructor of a child part (base destructor, then the storage). */
extern "C" HbmWidget* hbmKbChildPartDeleteB(HbmWidget* self, s32 flag) {
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

/* 0x80556EAC (0x5C): deleting destructor of a child part (base destructor, then the storage). */
extern "C" HbmWidget* hbmKbChildPartDeleteC(HbmWidget* self, s32 flag) {
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

/* Adjustor thunk: base #3 -> fn_80555B0C. */
extern "C" void fn_80556FA4(void* base, int flag) {
    fn_80555B0C((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_1C)), flag);
}

/* Adjustor thunk: base #3 -> fn_80556DC0. */
extern "C" void fn_80556FAC(void* base) {
    fn_80556DC0((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_1C)));
}

/* Adjustor thunk: base #3 -> hbmKbChildUpdate. */
extern "C" void fn_80556FB4(void* base) {
    hbmKbChildUpdate((HbmChildWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_1C)));
}

/* Adjustor thunk: base #4 -> fn_805563D0. */
extern "C" void fn_80556FBC(void* base, int flag) {
    fn_805563D0((HbmWidget*)((u8*)base - HBM_OFFSET_OF(HbmWidget, vtable_C4)), flag);
}
