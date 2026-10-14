/*
 * homebutton/hbm_anim_record.cpp - software-keyboard code of the tiHKBManager band: the animation record that steps an
 * index through its table, and the animated sign-key widget around it (open/close, show/hide, page stepping, its
 * state selectors, the key listener and the parts' deleting destructors).
 *
 * RANGE.     .text 0x80556FC4..0x80558EB4 (61 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around fn_805571D0/fn_805571FC: retail inlines them into hbmAnimStepBack,
 *            hbmAnimStepBackAlt and hbmAnimStepForward and also keeps their out-of-line copies.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the animation record; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS function names hbmAnim* (read off the bodies: the panes they bind, the states they set), view classes
 *            HbmAnimWidgetView / HbmAnimBaseView / HbmAnimNodeView / HbmAnimOwnerView / HbmAnimOwnerBaseView /
 *            HbmAnimKeyboardView (slots named by offset) and records HbmAnimWidget, HbmAnimPane, HbmAnimListener,
 *            HbmAnimTrigger, HbmAnimInput; map rows renamed with them (request hbm-misc).
 * SHAPES.    The views are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no
 *            method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * SHAPES.    The pane visibility is `(flags & 0xFE) | bit` through hbmAnimSetPaneVisible: retail's `rlwinm 24,30` (+ `ori`)
 *            pair; a bitfield store compiles to `rlwimi` instead.
 * SHAPES.    hbmAnimOpen hands slot 0x104 `self->index` twice (retail's two `mr` from the parameter register) and
 *            declares `source` before `keytop` (register colouring).
 * RESIDUALS. The literals ("N_SGNkeytop_all", "N_SGNkeyall") are this unit's .data strings in retail; ours are emitted
 *            in source order, so the relocation names differ until the unit's .data is emitted in order.
 * RESIDUALS. Unwritten, store vtables whose classes this unit would have to emit (rule 10): fn_80557228 (destructor),
 *            fn_80557390 (constructor), fn_80557744.
 * RESIDUALS. Unwritten, read this unit's own .data/.bss/.rodata tables the source does not emit yet: fn_80558064,
 *            fn_805580F0, fn_805581C0, fn_805583B4, fn_80558670, fn_805586D4, fn_80558E18 (the static initialiser).
 * RESIDUALS. Unwritten, call hbm_hermite.cpp's hbmNameMatches or
 *            hbm_text_panel.cpp's fn_80549D74: fn_80557004, fn_80557910, fn_80557E8C.
 */

#include "types.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_value.h"
#include "nw4r/fn_805012C4.h"
#include "MSL_C/string.h"
#include "MSL/strlen.h"
#include "Runtime.PPCEABI.H/memset.h"

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

/* The dispatch view of the widget: slot 0x10 updates it, 0x14 sets the state, 0x104/0x108 take the record index.  Declared as virtuals so MWCC emits the canonical
 * `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no method is defined and nothing is constructed here, so no table
 * is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmAnimWidgetView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 state);
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
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual void v_0x0C8();
    /* slot +0x0CC */ virtual void v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual void v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104(u8 previous, u8 current);
    /* slot +0x108 */ virtual void v_0x108(u8 current);
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
};

/* A node of the widget's part list (+0x90) and the part at +0x38: slot 0x10 sets a mode, 0x14 a state, 0x28 reads the
 * kind, 0x2C plays an animation; the +0x04 word is the entry a selection hands on.  No method is defined and nothing
 * is constructed here, so no table is emitted.  size: 0x08 */
class HbmAnimNodeView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010(s32 mode);
    /* slot +0x014 */ virtual void v_0x014(s32 state);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual s32 v_0x028();
    /* slot +0x02C */ virtual void v_0x02C(s32 anim);

    /* +0x04 */ u32 entry;
}; /* size: 0x08 */

/* A pane translation.  size: 0xC */
struct HbmAnimVec3 {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
}; /* size: 0xC */

/* The pane record a holder's +0x10 word points at: its translation at +0x2C and, in bit 0 of the +0xBB flag byte,
 * its visibility.  size: 0xBC */
struct HbmAnimPane {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ HbmAnimVec3 translate;
    /* +0x38 */ u8 unused_0x38[0x83];
    /* +0xBB */ u8 flags;
}; /* size: 0xBC */

/* Sets or clears the pane's visibility bit (bit 0 of +0xBB), keeping the other flags. */
static inline void hbmAnimSetPaneVisible(HbmAnimPane* pane, BOOL visible)
{
    pane->flags = (u8)((pane->flags & 0xFE) | (visible ? 1 : 0));
}

/* What the owner's +0x4C base hands back (slot 0x28): the pane record at +0x10.  size: 0x14 */
struct HbmAnimPaneHolder {
    /* +0x00 */ u8 unused_0x00[0x10];
    /* +0x10 */ HbmAnimPane* pane;
}; /* size: 0x14 */

/* The owner's +0x4C base: slot 0x28 returns the pane holder.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmAnimOwnerBaseView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual HbmAnimPaneHolder* v_0x028();
};

/* The owner at the widget's +0xD8: slot 0x24 says whether it is busy, slot 0x30 closes it; its +0x4C base holds the
 * pane.
 * size: 0x50 (approximation: only the vptr and the +0x4C base are touched) */
class HbmAnimOwnerView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual s32 v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();

    /* +0x04 */ u8 unused_0x04[0x48];
    /* +0x4C */ u32 base_4C;
}; /* size: 0x50 */

/* The keyboard the listener drives: slot 0x20 reports a press with the pane name, slot 0x38 says whether it is busy;
 * its pane-lookup base sits at +0x24.  size: 0x28 (approximation: only the vptr and the +0x24 base are touched) */
class HbmAnimKeyboardView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020(s32 event, const char* name);
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual s32 v_0x038();

    /* +0x04 */ u8 unused_0x04[0x20];
    /* +0x24 */ u32 base_24;
}; /* size: 0x28 */

/* The widget's +0x24 base: slot 0x28 returns its pane holder, 0x48 selects an entry, 0x4C refreshes, 0x50 binds a named
 * animation, 0x5C finds a pane by name and 0x68 returns the active part.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmAnimBaseView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual HbmAnimPaneHolder* v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048(u32 entry);
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050(const char* name, s32 mode);
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual HbmAnimNodeView* v_0x05C(const char* name);
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual HbmAnimWidgetView* v_0x068();
};

/* The listener hbmAnimHandleKeyEvent runs as: the controller at +0x08 and the keyboard at +0x0C (its pane-lookup base
 * at +0x24).  size: 0x10 */
struct HbmAnimListener {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ HbmAnimNodeView* controller;
    /* +0x0C */ HbmAnimKeyboardView* keyboard;
}; /* size: 0x10 */

/* A pane record read for its name.  size: 0xCD (approximation: the name is at most 17 bytes) */
struct HbmAnimNamedPane {
    /* +0x00 */ u8 unused_0x00[0xBC];
    /* +0xBC */ char name[0x11];
}; /* size: 0xCD */

/* The trigger component handed to the listener: its pane record at +0x9C, whose name sits at +0xBC.  size: 0xA0 */
struct HbmAnimTrigger {
    /* +0x00 */ u8 unused_0x00[0x9C];
    /* +0x9C */ HbmAnimNamedPane* pane;
}; /* size: 0xA0 */

/* The input record handed to the listener: bit 0x10 of the +0x0C word is the press.  size: 0x10 */
struct HbmAnimInput {
    /* +0x00 */ u8 unused_0x00[0x0C];
    /* +0x0C */ u32 buttons;
}; /* size: 0x10 */

/* The animated widget as the bodies below read it: the pane record (+0x04), its animation record (+0x15..+0x18, the
 * HbmAnimRecord view, with the mode byte at +0x16), the pane-lookup base (+0x24), the state selector (+0x2C), the part
 * (+0x38), the part list (+0x90), the open and shown flags (+0xD0/+0xD1) and the owner (+0xD8).  size: 0xDC (approximation: bounded by the owner word) */
struct HbmAnimWidget {
    /* +0x00 */ u8 unused_0x00[4];      /* the vptr of HbmAnimWidgetView */
    /* +0x04 */ HbmAnimPane* pane;
    /* +0x08 */ u8 unused_0x08[0x0E];
    /* +0x16 */ u8 mode;
    /* +0x17 */ u8 index;
    /* +0x18 */ const u8* table;
    /* +0x1C */ u8 unused_0x1C[0x08];
    /* +0x24 */ u32 base_24;
    /* +0x28 */ u8 unused_0x28[0x04];
    /* +0x2C */ s32 state;
    /* +0x30 */ u8 unused_0x30[0x08];
    /* +0x38 */ HbmAnimNodeView* part;
    /* +0x3C */ u8 unused_0x3C[0x54];
    /* +0x90 */ nw4r::ut::List parts;
    /* +0x9C */ u8 unused_0x9C[0x34];
    /* +0xD0 */ u8 open;
    /* +0xD1 */ u8 shown;
    /* +0xD2 */ u8 unused_0xD2[0x06];
    /* +0xD8 */ HbmAnimOwnerView* owner;
}; /* size: 0xDC */

#pragma push
#pragma auto_inline on
/* Steps the animation record's index down, wrapping to the last entry. */
extern "C" void fn_805571D0(HbmAnimRecord* self) {
    if (self->flag_17 == 0) {
        self->flag_17 = self->table[0] - 1;
    } else {
        self->flag_17 = self->flag_17 - 1;
    }
}

/* Steps the animation record's index up, wrapping to the first entry. */
extern "C" void fn_805571FC(HbmAnimRecord* self) {
    u8 next = self->flag_17 + 1;
    self->flag_17 = next;
    if (next >= self->table[0]) {
        self->flag_17 = 0;
    }
}
#pragma pop

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

/* 0x80557AA8 (0x1AC): opens the widget for an owner at a mode and entry: resets the parts, selects the entry, moves
 * the key-top pane onto the owner's pane and plays the part's opening animation. */
extern "C" void hbmAnimOpen(HbmAnimWidget* self, HbmAnimOwnerView* owner, u8 mode, u8 index) {
    HbmAnimPane* source;
    HbmAnimNodeView* keytop;
    HbmAnimPane* target;

    ((HbmAnimWidgetView*)self)->v_0x0FC();
    ((HbmAnimWidgetView*)self)->v_0x100();
    ((HbmAnimBaseView*)&self->base_24)->v_0x068()->v_0x010();
    self->open = 1;
    self->shown = 0;
    self->owner = owner;
    ((HbmAnimBaseView*)&self->base_24)->v_0x04C();
    self->mode = mode;
    self->index = index;
    ((HbmAnimWidgetView*)self)->v_0x104(self->index, self->index);
    ((HbmAnimWidgetView*)self)->v_0x108(self->index);
    keytop = ((HbmAnimBaseView*)&self->base_24)->v_0x05C("N_SGNkeytop_all");
    source = ((HbmAnimOwnerBaseView*)&self->owner->base_4C)->v_0x028()->pane;
    target = ((HbmAnimBaseView*)&self->base_24)->v_0x028()->pane;
    target->translate = source->translate;
    keytop->v_0x010(6);
    self->part->v_0x014(14);
    self->part->v_0x02C(0);
}

/* 0x80557C54 (0xC8): closes the widget over its owner: shows the owner's pane again while the owner is idle, plays
 * the key-top close animation and clears the shown flag. */
extern "C" void hbmAnimHide(HbmAnimWidget* self) {
    if (self->owner != 0) {
        if (self->owner->v_0x024() == 0) {
            hbmAnimSetPaneVisible(((HbmAnimOwnerBaseView*)&self->owner->base_4C)->v_0x028()->pane, TRUE);
        }
        ((HbmAnimBaseView*)&self->base_24)->v_0x05C("N_SGNkeytop_all")->v_0x010(7);
        self->shown = 0;
        self->part->v_0x02C(0);
    }
}

/* 0x80557D1C (0x60): runs the widget's slots 0xF8, 0x4C and 0x10C. */
extern "C" void hbmAnimRefresh(HbmAnimWidget* self) {
    ((HbmAnimWidgetView*)self)->v_0x0F8();
    ((HbmAnimWidgetView*)self)->v_0x04C();
    ((HbmAnimWidgetView*)self)->v_0x10C();
}

/* 0x80557D7C (0x88): closes the owner, clears the open flag and stops the key-top animation. */
extern "C" void hbmAnimClose(HbmAnimWidget* self) {
    if (self->owner != 0) {
        self->owner->v_0x030();
    }
    self->open = 0;
    ((HbmAnimBaseView*)&self->base_24)->v_0x050("N_SGNkeytop_all", 0);
    self->part->v_0x02C(1);
}

/* 0x80557E04 (0x88): marks the widget shown, hides the owner's pane while the owner is idle and plays the part's
 * animation 2. */
extern "C" void hbmAnimShow(HbmAnimWidget* self) {
    self->shown = 1;
    if (self->owner->v_0x024() == 0) {
        hbmAnimSetPaneVisible(((HbmAnimOwnerBaseView*)&self->owner->base_4C)->v_0x028()->pane, FALSE);
    }
    self->part->v_0x02C(2);
}

/* 0x80557FCC (0x90): sets mode 2 on every part of kind 2. */
extern "C" void hbmAnimArmParts(HbmAnimWidget* self) {
    HbmAnimNodeView* node;

    for (node = (HbmAnimNodeView*)nw4r::ut::List_GetNext(&self->parts, 0); node != 0;
         node = (HbmAnimNodeView*)nw4r::ut::List_GetNext(&self->parts, node)) {
        if (node->v_0x028() == 2) {
            node->v_0x010(2);
        }
    }
}

/* The widget's resource word (+0x30). */
extern "C" void* fn_8055805C(HbmWidget* self) {
    return self->resource;
}

/* Clears the widget's state selector. */
extern "C" void fn_805580E4(HbmWidget* self) {
    self->mode.state = 0;
}

/* Sets the widget's state selector to its "showing" state. */
extern "C" void fn_805581B4(HbmWidget* self) {
    self->mode.state = 6;
}

/* 0x80558434 (0xBC): steps the record back (stopping at the first entry) and shows the new entry. */
extern "C" void hbmAnimStepBack(HbmAnimWidget* self) {
    u8 previous = self->index;
    u8 current;

    if (previous == 0) {
        return;
    }
    fn_805571D0((HbmAnimRecord*)self);
    current = self->index;
    ((HbmAnimWidgetView*)self)->v_0x104(previous, current);
    ((HbmAnimWidgetView*)self)->v_0x108(current);
    ((HbmAnimWidgetView*)self)->v_0x114();
    self->part->v_0x014(15);
}

/* 0x805584F0 (0xB8): steps the record forward (wrapping to the first entry) and shows the new entry. */
extern "C" void hbmAnimStepForward(HbmAnimWidget* self) {
    u8 previous = self->index;
    u8 current;

    fn_805571FC((HbmAnimRecord*)self);
    current = self->index;
    ((HbmAnimWidgetView*)self)->v_0x104(previous, current);
    ((HbmAnimWidgetView*)self)->v_0x108(current);
    ((HbmAnimWidgetView*)self)->v_0x118();
    self->part->v_0x014(15);
}

/* 0x805585A8 (0x48): sets mode 8 on the "N_SGNkeyall" pane. */
extern "C" void hbmAnimKeysMode8(HbmAnimWidget* self) {
    ((HbmAnimBaseView*)&self->base_24)->v_0x05C("N_SGNkeyall")->v_0x010(8);
}

/* 0x805585F0 (0x48): sets mode 9 on the "N_SGNkeyall" pane. */
extern "C" void hbmAnimKeysMode9(HbmAnimWidget* self) {
    ((HbmAnimBaseView*)&self->base_24)->v_0x05C("N_SGNkeyall")->v_0x010(9);
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

/* 0x80558738 (0xBC): the second class's step back (stopping at the first entry), showing the new entry. */
extern "C" void hbmAnimStepBackAlt(HbmAnimWidget* self) {
    u8 previous = self->index;
    u8 current;

    if (previous == 0) {
        return;
    }
    fn_805571D0((HbmAnimRecord*)self);
    current = self->index;
    ((HbmAnimWidgetView*)self)->v_0x104(previous, current);
    ((HbmAnimWidgetView*)self)->v_0x108(current);
    ((HbmAnimWidgetView*)self)->v_0x114();
    self->part->v_0x014(15);
}

/* 0x805587F4 (0x1AC): handles a key trigger: reports presses to the keyboard and, for a "B..." button, drives its
 * "P..." pane (press, release, select). */
extern "C" void hbmAnimHandleKeyEvent(HbmAnimListener* self, HbmAnimTrigger* trigger, u32 event, HbmAnimInput* input) {
    char* name = trigger->pane->name;
    char paneName[17];
    HbmAnimNodeView* pane;

    if (self->keyboard->v_0x038() != 0 && event == 4) {
        return;
    }
    if (event == 4 && (input->buttons & 0x10)) {
        self->keyboard->v_0x020(4, name);
    }
    if (name[0] != 'B') {
        return;
    }
    memset(paneName, 0, sizeof(paneName));
    strncpy(paneName, name, strlen(name));
    paneName[0] = 'P';
    pane = ((HbmAnimBaseView*)&self->keyboard->base_24)->v_0x05C(paneName);
    if (pane == 0) {
        return;
    }
    switch ((s32)event) {
    case 4:
        if (input->buttons & 0x10) {
            pane->v_0x010(0);
        }
        break;
    case 1:
        pane->v_0x010(2);
        break;
    case 0:
        self->controller->v_0x014(4);
        ((HbmAnimBaseView*)&self->keyboard->base_24)->v_0x048(pane->entry);
        pane->v_0x010(1);
        break;
    }
}

/* Whether the widget is flagged. */
extern "C" u8 fn_805589A0(HbmWidget* self) {
    return ((HbmAnimRecord*)self)->flag_15;
}

/* 0x805589A8 (0x9C): event 6 shows the pane and selects state 1, event 7 selects state 8; event 4 leaves state 1 for
 * 0 and hides the pane in state 8. */
extern "C" void hbmAnimHandleEvent(HbmAnimWidget* self, s32 event) {
    if (event == 6) {
        hbmAnimSetPaneVisible(self->pane, TRUE);
        ((HbmAnimWidgetView*)self)->v_0x014(1);
        return;
    }
    if (event == 7) {
        ((HbmAnimWidgetView*)self)->v_0x014(8);
        return;
    }
    switch (self->state) {
    case 1:
        if (event == 4) {
            ((HbmAnimWidgetView*)self)->v_0x014(0);
        }
        break;
    case 8:
        if (event == 4) {
            hbmAnimSetPaneVisible(self->pane, FALSE);
        }
        break;
    }
}

/* Stores the child the widget dispatches to and re-sorts its list. */
extern "C" void fn_80558A44(HbmWidget* self, HbmWidget* current) {
    self->mode.current = current;
    fn_8055C638(self);
}

/* 0x80558A4C (0xBC): steps the idle (0) and closing (5, 7) states on an input event. */
extern "C" void hbmAnimStepIdle(HbmAnimWidget* self, s32 event) {
    switch (self->state) {
    case 0:
        if (event == 9) {
            ((HbmAnimWidgetView*)self)->v_0x014(5);
        }
        if (event == 8) {
            ((HbmAnimWidgetView*)self)->v_0x014(7);
        }
        break;
    case 5:
    case 7:
        if (event == 4) {
            ((HbmAnimWidgetView*)self)->v_0x014(0);
        }
        break;
    }
}

/* 0x80558B08 (0x184): steps the seven-state selector on an input event (0 also forces state 4). */
extern "C" void hbmAnimStepState(HbmAnimWidget* self, s32 event) {
    if (event == 0) {
        ((HbmAnimWidgetView*)self)->v_0x014(4);
    }
    switch ((u32)self->state) {
    case 0:
        if (event == 1) {
            ((HbmAnimWidgetView*)self)->v_0x014(2);
        }
        break;
    case 2:
        if (event == 4) {
            ((HbmAnimWidgetView*)self)->v_0x014(3);
        }
        if (event == 2) {
            ((HbmAnimWidgetView*)self)->v_0x014(6);
        }
        break;
    case 3:
        if (event == 2) {
            ((HbmAnimWidgetView*)self)->v_0x014(6);
        }
        break;
    case 6:
        if (event == 4) {
            ((HbmAnimWidgetView*)self)->v_0x014(0);
        }
        if (event == 1) {
            ((HbmAnimWidgetView*)self)->v_0x014(2);
        }
        break;
    case 4:
        if (event == 4) {
            ((HbmAnimWidgetView*)self)->v_0x014(3);
        }
        if (event == 2) {
            ((HbmAnimWidgetView*)self)->v_0x014(6);
        }
        break;
    }
}

/* Walks the widget's child pane. */
extern "C" void fn_80558C8C(HbmWidget* self) {
    fn_8055BBAC(&self->vtable_24);
}

/* Empty virtual slot. */
extern "C" void fn_80558C94(void) {
}

/* Sets the animation record's index, wrapping to the first entry. */
extern "C" void fn_80558C98(HbmAnimRecord* self, int index) {
    self->flag_17 = index;
    if ((u8)index < self->table[0]) {
        return;
    }
    self->flag_17 = 0;
}

/* Stores the widget's active child, then updates it. */
extern "C" void fn_80558CBC(HbmWidget* self, HbmWidget* child) {
    self->child = child;
    ((HbmAnimWidgetView*)self)->v_0x010();
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

/* 0x80558CE0 (0x5C): deleting destructor of an animated part (base destructor, then the storage). */
extern "C" HbmWidget* hbmAnimPartDeleteA(HbmWidget* self, s32 flag) {
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

/* 0x80558D54 (0x5C): deleting destructor of an animated part (base destructor, then the storage). */
extern "C" HbmWidget* hbmAnimPartDeleteB(HbmWidget* self, s32 flag) {
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

/* 0x80558DBC (0x5C): deleting destructor of an animated part (base destructor, then the storage). */
extern "C" HbmWidget* hbmAnimPartDeleteC(HbmWidget* self, s32 flag) {
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
