/*
 * homebutton/hbm_kb_list.cpp - software-keyboard code of the tiHKBManager band: the keyboard list widget's button and
 * key-change pane animations, its key-table refresh, its state selectors and the parts' deleting destructors.
 *
 * RANGE.     .text 0x805546B4..0x80555B0C (37 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the list widget; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS function names hbmKbList* (read off the bodies: the pane names they bind, the states they set) and view
 *            classes HbmListPaneView / HbmListKeyPaneView (slots named by offset) and records HbmListKeyEntry,
 *            HbmListModeView, HbmListFlagView; map rows renamed with them (request hbm-misc).
 * SHAPES.    The views are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no
 *            method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * SHAPES.    The pane names are string literals: retail emits them into this unit's .data in first-use order, between
 *            the switch tables (0x806502A0.. "P_BT_confirm", 0x806502D8 "P_kyChng_CP", 0x80650328 "P_keyChange").
 * SHAPES.    hbmKbListStepState switches on `(s32)state` (retail's signed compare tree); hbmKbListStepStateEx's guard
 *            compares `(s32)state` too.
 * RESIDUALS. hbmKbListSetKeyChangeAnims: its three literals are addressed off retail's string base 0x80650160 at
 *            +0x188/+0x178/+0x1C8; ours sit at other offsets of our pool until the unwritten fn_80554BA8 (which uses
 *            "P_kyChng_CP" first) is written.
 * RESIDUALS. Unwritten, store vtables whose classes this unit would have to emit (rule 10): fn_80554714 (destructor),
 *            fn_80554834 (constructor).
 * RESIDUALS. Unwritten, call unnamed functions of other units: fn_80554BA8 (hbm_value.cpp's fn_8055BB88, the table
 *            0x806574D8), fn_80555798 (hbm_hermite.cpp's hbmNameMatches/hbmNameCopyWithChar).
 * RESIDUALS. .data 0x80650160..0x80650500 starts with four strings ("P_BT_cancel", "P_kyChng_QWERTY", ...) that
 *            duplicate this unit's own literals, so the range holds more than one TU's data.
 * NAMES.     GUESS classes HbmKbListPart and its kinds HbmKbListPartA/B (band keys with a state word; members reset,
 *            setState, getResource, getState).
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "homebutton/hkb_widget.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"

/* A list part: a band key with its state word; its class table (0x806503A8) and the out-of-line copy of its inline destructor (0x80554AE0) are
 * this unit's.  size: 0x34 */
class HbmKbListPart : public HbmKey {
public:
    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmKbListPart() {}
    /* untyped: opaque handle passed through */
    virtual void* getResource();
    virtual u32 getState();

    /* +0x30 */ void* mpResource;
}; /* size: 0x34 */

/* A HbmKbListPart kind (class table 0x80650350): its own slot 0x10 are defined above as C functions.  size: 0x34 */
class HbmKbListPartA : public HbmKbListPart {
public:
    virtual void setAnim(s32 anim);
    virtual ~HbmKbListPartA();
};

/* A HbmKbListPart kind (class table 0x8065037C): its own slot 0x10 are defined above as C functions.  size: 0x34 */
class HbmKbListPartB : public HbmKbListPart {
public:
    virtual void setAnim(s32 anim);
    virtual ~HbmKbListPartB();
};


extern "C" void fn_80554714(void* self);

extern "C" void fn_80554BA8(void* self);

extern "C" void wmemset(void* dst, int value, u16 size);

/* The layout strings the band passes to its widgets (`.sdata` words; their band is unregistered). */
extern char lbl_80794610[5];     /* "N_UP" */

extern char lbl_80794618[7];     /* "N_DOWN" */

extern void* lbl_80794608;       /* widget-string slot the static initialiser copies */

extern void* lbl_8079460C;       /* widget-string slot the static initialiser copies */

extern HbmWidget lbl_806501A0;   /* the band's static widget instance */

/* A pane found by name (HbmListPaneView slot 0x5C): slot 0x10 shows it with a flag, 0x14 sets its state, 0x24 and 0x28
 * read its mode and kind.  No method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmListKeyPaneView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010(s32 flag);
    /* slot +0x014 */ virtual void v_0x014(s32 state);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual s32 v_0x024();
    /* slot +0x028 */ virtual s32 v_0x028();
};

/* The dispatch view of the list widget, its panes and its base sub-objects: slot 0x14 sets a state, 0x24/0x28 read
 * the pane's mode and kind, 0x3C sets a layout name, 0x50/0x58 bind a named animation, 0x5C finds a pane by name.
 * Declared as virtuals so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no method is
 * defined and nothing is constructed here, so no table is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmListPaneView {
public:
    /* slot +0x008 */ virtual HbmListPaneView* v_0x008();
    /* slot +0x00C */ virtual HbmListPaneView* v_0x00C();
    /* slot +0x010 */ virtual HbmListPaneView* v_0x010();
    /* slot +0x014 */ virtual HbmListPaneView* v_0x014(s32 state);
    /* slot +0x018 */ virtual HbmListPaneView* v_0x018();
    /* slot +0x01C */ virtual HbmListPaneView* v_0x01C();
    /* slot +0x020 */ virtual HbmListPaneView* v_0x020();
    /* slot +0x024 */ virtual s32 v_0x024();
    /* slot +0x028 */ virtual s32 v_0x028();
    /* slot +0x02C */ virtual HbmListPaneView* v_0x02C();
    /* slot +0x030 */ virtual HbmListPaneView* v_0x030();
    /* slot +0x034 */ virtual HbmListPaneView* v_0x034();
    /* slot +0x038 */ virtual HbmListPaneView* v_0x038();
    /* slot +0x03C */ virtual HbmListPaneView* v_0x03C(const char* name, s32 on);
    /* slot +0x040 */ virtual HbmListPaneView* v_0x040();
    /* slot +0x044 */ virtual HbmListPaneView* v_0x044();
    /* slot +0x048 */ virtual HbmListPaneView* v_0x048();
    /* slot +0x04C */ virtual HbmListPaneView* v_0x04C();
    /* slot +0x050 */ virtual HbmListPaneView* v_0x050(const char* name, s32 mode);
    /* slot +0x054 */ virtual HbmListPaneView* v_0x054();
    /* slot +0x058 */ virtual HbmListPaneView* v_0x058(const char* name, s32 mode);
    /* slot +0x05C */ virtual HbmListKeyPaneView* v_0x05C(const char* name);
    /* slot +0x060 */ virtual HbmListPaneView* v_0x060();
    /* slot +0x064 */ virtual HbmListPaneView* v_0x064();
    /* slot +0x068 */ virtual HbmListPaneView* v_0x068();
    /* slot +0x06C */ virtual HbmListPaneView* v_0x06C();
    /* slot +0x070 */ virtual HbmListPaneView* v_0x070();
    /* slot +0x074 */ virtual HbmListPaneView* v_0x074();
    /* slot +0x078 */ virtual HbmListPaneView* v_0x078();
    /* slot +0x07C */ virtual HbmListPaneView* v_0x07C();
    /* slot +0x080 */ virtual HbmListPaneView* v_0x080();
    /* slot +0x084 */ virtual HbmListPaneView* v_0x084();
    /* slot +0x088 */ virtual HbmListPaneView* v_0x088();
    /* slot +0x08C */ virtual HbmListPaneView* v_0x08C();
    /* slot +0x090 */ virtual HbmListPaneView* v_0x090();
    /* slot +0x094 */ virtual HbmListPaneView* v_0x094();
    /* slot +0x098 */ virtual HbmListPaneView* v_0x098();
    /* slot +0x09C */ virtual HbmListPaneView* v_0x09C();
    /* slot +0x0A0 */ virtual HbmListPaneView* v_0x0A0();
    /* slot +0x0A4 */ virtual HbmListPaneView* v_0x0A4();
    /* slot +0x0A8 */ virtual HbmListPaneView* v_0x0A8();
    /* slot +0x0AC */ virtual HbmListPaneView* v_0x0AC();
    /* slot +0x0B0 */ virtual HbmListPaneView* v_0x0B0();
    /* slot +0x0B4 */ virtual HbmListPaneView* v_0x0B4();
    /* slot +0x0B8 */ virtual HbmListPaneView* v_0x0B8();
    /* slot +0x0BC */ virtual HbmListPaneView* v_0x0BC();
    /* slot +0x0C0 */ virtual HbmListPaneView* v_0x0C0();
    /* slot +0x0C4 */ virtual HbmListPaneView* v_0x0C4();
    /* slot +0x0C8 */ virtual HbmListPaneView* v_0x0C8();
};


/* One entry of the list's static key table (0x806501A0, four 0x40-byte entries): the pane name at +0x04.
 * size: 0x40 */
struct HbmListKeyEntry {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ const char* name;
    /* +0x08 */ u8 unused_0x08[0x38];
}; /* size: 0x40 */

/* The mode view of the list widget hbmKbListSelectKeyLayout runs on: the mode byte at +0x04, the child whose current
 * part is committed at +0x10 and the pane-lookup base at +0x14.  size: 0x18 */
struct HbmListModeView {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ u8 mode;
    /* +0x05 */ u8 unused_0x05[0xB];
    /* +0x10 */ HkbWidget* child;
    /* +0x14 */ u32 base_14;
}; /* size: 0x18 */

/* The list widget's own state byte at +0xC0 (the byte hbmKbListSetKeyChangeAnims stores).  size: 0xC4 */
struct HbmListFlagView {
    /* +0x00 */ u8 unused_0x00[0xC0];
    /* +0xC0 */ u8 keyChangeMode;
}; /* size: 0xC4 */

/* Stores the object's +0x04 flag, then re-runs the child's +0x68 slot and commits it. */
extern "C" void fn_805546B4(HkbFlagChild* self, u8 flag) {
    void* value;
    self->flag_04 = flag;
    value = self->child_10->v_0x068();
    if (value != 0) {
        hbmPanelCloseCandidates(self->child_10->v_0x068());
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

/* 0x80554B38 (0x1C): binds the confirm button's pane animation. */
extern "C" void hbmKbListAnimConfirmButton(HkbWidget* self, s32 mode) {
    ((HbmListPaneView*)&self->vtable_14)->v_0x050("P_BT_confirm", mode);
}

/* 0x80554B54 (0x1C): binds the cancel button's pane animation. */
extern "C" void hbmKbListAnimCancelButton(HkbWidget* self, s32 mode) {
    ((HbmListPaneView*)&self->vtable_14)->v_0x050("P_BT_cancel", mode);
}

/* 0x80554B70 (0x1C): binds the confirm button's text animation. */
extern "C" void hbmKbListAnimConfirmText(HkbWidget* self, s32 mode) {
    ((HbmListPaneView*)&self->vtable_14)->v_0x058("T_BT_confirm", mode);
}

/* 0x80554B8C (0x1C): binds the cancel button's text animation. */
extern "C" void hbmKbListAnimCancelText(HkbWidget* self, s32 mode) {
    ((HbmListPaneView*)&self->vtable_14)->v_0x058("T_BT_cancel", mode);
}

/* Empty virtual slot. */
extern "C" void fn_80554DBC(void) {
}

/* The keyboard widget's +0x08 word. */
extern "C" u32 fn_80554DC0(HkbWidget* self) {
    return self->value_08;
}

/* 0x80554DC8 (0x138): looks each key of the static table up by name and sets its state from its kind and mode. */
extern "C" void hbmKbListRefreshKeys(HkbWidget* self) {
    HbmListKeyEntry* entry = (HbmListKeyEntry*)&lbl_806501A0;
    u32 i;

    for (i = 0; i < 4; i++, entry++) {
        HbmListKeyPaneView* pane = ((HbmListPaneView*)&self->vtable_14)->v_0x05C(entry->name);

        if (pane == 0) {
            continue;
        }
        switch (pane->v_0x028()) {
        case 1:
        case 3:
            pane->v_0x014(2);
            break;
        case 4:
            switch (pane->v_0x024()) {
            case 0:
                pane->v_0x014(2);
                break;
            case 1:
                pane->v_0x014(5);
                break;
            }
            break;
        case 8:
        case 11:
            pane->v_0x014(9);
            break;
        }
    }
}

/* Returns the part's state. */
u32 HbmKbListPart::getState() {
    return mState;
}


/* Stores the state, then hands it to the band key. */
void HbmKbListPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
}


/* Returns the part's resource word. */
/* untyped: opaque handle passed through */
void* HbmKbListPart::getResource() {
    return mpResource;
}


/* 0x80554F18 (0xFC): shows the second pane, then resets the widget's panes and parts. */
extern "C" void hbmKbListResetShown(HkbWidget* self) {
    ((HbmListPaneView*)self->pane_28)->v_0x018();
    ((HbmListPaneView*)self->pane_28)->v_0x014(9);
    ((HbmListPaneView*)self)->v_0x0C8();
    ((HbmListPaneView*)self->pane_24)->v_0x010();
    ((HbmListPaneView*)&self->vtable_14)->v_0x04C();
    ((HbmListPaneView*)self->vtable_10)->v_0x078()->v_0x074();
    ((HbmListPaneView*)self->vtable_10)->v_0x080()->v_0x068();
    ((HbmListPaneView*)self->vtable_10)->v_0x090()->v_0x038();
}

/* 0x80555014 (0xFC): hides the second pane, then resets the widget's panes and parts. */
extern "C" void hbmKbListResetHidden(HkbWidget* self) {
    ((HbmListPaneView*)self->pane_28)->v_0x01C();
    ((HbmListPaneView*)self->pane_28)->v_0x014(9);
    ((HbmListPaneView*)self)->v_0x0C8();
    ((HbmListPaneView*)self->pane_24)->v_0x010();
    ((HbmListPaneView*)&self->vtable_14)->v_0x04C();
    ((HbmListPaneView*)self->vtable_10)->v_0x078()->v_0x074();
    ((HbmListPaneView*)self->vtable_10)->v_0x080()->v_0x068();
    ((HbmListPaneView*)self->vtable_10)->v_0x090()->v_0x038();
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

/* Re-targets the widget's pane when the value changes, then the widget itself. */
extern "C" void fn_8055512C(HkbWidget* self, void* value) {
    if (value != self->v_0x010()) {
        if (value != 0) {
            self->pane_28->v_0x014(22);
        } else {
            self->pane_28->v_0x014(23);
        }
    }
    self->v_0x014((u32)value);
}

/* --------------------------------------------------------------------------------------------- */
/* Bodies (address order)                                                                         */
/* --------------------------------------------------------------------------------------------- */

/* 0x805551CC (0x1A8): switches the key-change panes to the cellphone (non-zero) or QWERTY layout, stores the mode
 * and commits the child's current part. */
extern "C" void hbmKbListSelectKeyLayout(HbmListModeView* self, u8 cellphone) {
    if (cellphone != 0) {
        s32 kind = ((HbmListPaneView*)&self->base_14)->v_0x05C("P_kyChng_CP")->v_0x028();

        if (kind != 0 && kind != 6) {
            ((HbmListPaneView*)&self->base_14)->v_0x05C("P_kyChng_CP")->v_0x014(6);
        }
        ((HbmListPaneView*)&self->base_14)->v_0x05C("P_kyChng_QWERTY")->v_0x010(0);
    } else {
        s32 kind = ((HbmListPaneView*)&self->base_14)->v_0x05C("P_kyChng_QWERTY")->v_0x028();

        if (kind != 0 && kind != 6) {
            ((HbmListPaneView*)&self->base_14)->v_0x05C("P_kyChng_QWERTY")->v_0x014(6);
        }
        ((HbmListPaneView*)&self->base_14)->v_0x05C("P_kyChng_CP")->v_0x010(0);
    }
    self->mode = cellphone;
    if (self->child->v_0x068() != 0) {
        hbmPanelCloseCandidates(self->child->v_0x068());
    }
}

/* 0x80555374 (0x98): stores the key-change mode and binds the three key-change animations with it. */
extern "C" void hbmKbListSetKeyChangeAnims(HkbWidget* self, u8 mode) {
    ((HbmListFlagView*)self)->keyChangeMode = mode;
    ((HbmListPaneView*)&self->vtable_14)->v_0x050("P_kyChng_QWERTY", mode);
    ((HbmListPaneView*)&self->vtable_14)->v_0x050("P_kyChng_CP", mode);
    ((HbmListPaneView*)&self->vtable_14)->v_0x050("P_keyChange", mode);
}

/* Re-targets the widget's own child pane with a layout name. */
extern "C" void fn_8055540C(HbmWidget* self) {
    ((HbmListPaneView*)self->owner->child)->v_0x03C(lbl_80794610, 1);
}

/* Re-targets the widget's own child pane with a layout name. */
extern "C" void fn_8055542C(HbmWidget* self) {
    ((HbmListPaneView*)self->owner->child)->v_0x03C(lbl_80794618, 1);
}

/* 0x8055544C (0x190): steps the four-state selector on an input event (0 also forces state 4). */
extern "C" void hbmKbListStepState(HkbWidget* self, s32 event) {
    if (event == 0) {
        self->v_0x014(4);
    }
    switch ((s32)self->state) {
    case 0:
        if (event == 1) {
            self->v_0x014(1);
        }
        break;
    case 1:
        if (event == 4) {
            self->v_0x014(3);
        }
        if (event == 2) {
            self->v_0x014(2);
        }
        break;
    case 3:
        if (event == 2) {
            self->v_0x014(2);
        }
        break;
    case 2:
        if (event == 4) {
            self->v_0x014(0);
        }
        if (event == 1) {
            self->v_0x014(1);
        }
        break;
    case 4:
        if (event == 4) {
            self->v_0x014(3);
        }
        if (event == 2) {
            self->v_0x014(2);
        }
        break;
    }
}

/* 0x805555DC (0x1BC): steps the seven-state selector on an input event (0 forces state 4 outside states 4 and 5). */
extern "C" void hbmKbListStepStateEx(HkbWidget* self, s32 event) {
    if (event == 0 && (s32)self->state != 5 && (s32)self->state != 4) {
        self->v_0x014(4);
    }
    switch (self->state) {
    case 0:
        if (event == 1) {
            self->v_0x014(1);
        }
        break;
    case 1:
        if (event == 4) {
            self->v_0x014(3);
        }
        if (event == 2) {
            self->v_0x014(2);
        }
        break;
    case 3:
        if (event == 2) {
            self->v_0x014(2);
        }
        break;
    case 2:
        if (event == 4) {
            self->v_0x014(0);
        }
        if (event == 1) {
            self->v_0x014(1);
        }
        break;
    case 4:
        if (event == 4) {
            self->v_0x014(5);
        }
        break;
    case 6:
        if (event == 4) {
            self->v_0x014(0);
        }
        if (event == 1) {
            self->v_0x014(1);
        }
        break;
    }
}

/* Adjustor thunk: base #1 -> HbmPaneManager::calcComponents. */
extern "C" void fn_80555A0C(HbmWidget* self) {
    ((HbmPaneManager*)&self->vtable_14)->HbmPaneManager::calcComponents();
}

/* Clears the part's state. */
void HbmKbListPart::reset() {
    mState = 0;
}


/* Destroys the part (the inline base destructors run here). */
HbmKbListPartA::~HbmKbListPartA() {
}


/* Destroys the part (the inline base destructors run here). */
HbmKbListPartB::~HbmKbListPartB() {
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
