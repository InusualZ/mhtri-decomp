/*
 * homebutton/hbm_text_panel.cpp - software-keyboard code of the tiHKBManager band: the text-entry panel (its mode
 * selector, key handler, text-position records, button auto-repeat and text-box string), its trivial virtual slots and
 * the trailing accessors of the keyboard widget.
 *
 * RANGE.     .text 0x805425E4..0x8054F7BC (236 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around fn_80549CC0: retail inlines it into fn_80548950 (flag 0) and
 *            fn_80548B10 (flag 1) and keeps its out-of-line copy; -inline noauto inlines nothing on its own.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the text panel; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS view classes HbmPanelView, HbmPanelItemView, HbmPanelValueView, HbmPanelSubView, HbmPanelScrollView,
 *            HbmPaneView, HbmPanelAnimView and the struct HbmLayoutView: the dispatch views of the `*Vtbl` structs of
 *            fn_805482CC.h and of the layout objects hbmPanelSetTextBoxString reaches; slots are named by offset.
 * NAMES.     GUESS function names (hbmPanel*, hbmKb*): read off the bodies and, for the virtual slots, the table and slot
 *            that reference them; map rows renamed with them (requests hbm-value#42..#65, #75..#82).
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch;
 *            no method is defined and nothing is constructed, so no table is emitted here (rule 10).
 * SHAPES.    fn_80549CC0 is a one-case `switch` on the mode (`cmpwi 1; beq; b`), fn_80548394/fn_805482CC load their pooled
 *            float into a local first (retail schedules the `lfs` ahead of the stores), and the `before`/`after` pairs
 *            of fn_805486B0/fn_80548B10 are declared `after` first (retail's stack slots).
 * RESIDUALS. fn_805482CC: retail loads lbl_8079D698 before the 0x195C store and rotates f1/f2 against ours.
 * RESIDUALS. fn_80548950: retail tests the key as `addis r3,r31,1; subi r0,r3,0x309B` (adds 0xCF65); only the literal
 *            `(u16)(key + 0xCF65)` reproduces it, and `(u16)(key - 0x309B)` is kept.
 * RESIDUALS. fn_80548E40: retail keeps a `cmplw r10,r10` self-compare and reloads +0x19F0 around the cell swap; the
 *            source cannot express the compare without a `goto` (rule 8).
 * RESIDUALS. fn_8054F178: retail loads both arguments before either store (`lbz r4; lwz r0; stb; stw`); every spelling
 *            tried (locals in either order) keeps one load/store pair after the other.
 * RESIDUALS. fn_8054CDC4 calls the band declaration fn_80526D00 (src/unsplit/unknown.h) of KPRClearQueue (request
 *            hbm-value#73); relocdiff reports the name.
 * RESIDUALS. HbmTextPanel (fn_805482CC.h) and HkbWidget (homebutton/hkb_widget.h) are views of one object (the
 *            +0x18B8 record, the +0x193C helper); the merge and the header's own generated file name are changes
 *            outside this unit.
 * RESIDUALS. 98 functions (48312 bytes) are unwritten in 28 runs, the largest fn_8054358C (8732), fn_80549D9C (2880),
 *            fn_8054734C (2024) and fn_8054E894 (1576); the list is `python tools/units/sweepcomments.py --unit
 *            homebutton/hbm_text_panel`.
 * RESIDUALS. .data 0x8064E760..0x8064F360 spans at least 3 TUs (datagap.py emission-order seams at 0x8064F0BC and
 *            0x8064F338), so the data cannot match until the range is re-cut; hbmPanelSetTextBoxString's two text-box names are
 *            string literals here (retail lbl_8064EC08/lbl_8064EC18 in the claimed .data).
 * NAMES.     GUESS classes HbmPanelPart and its kind HbmPanelPartA (band keys with a state word; members reset,
 *            setState, getResource, getState).
 */

#include "types.h"

#include "fn_805482CC.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "homebutton/hkb_widget.h"
#include "unsplit/unknown.h"
#include "KPR/kpr.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"
#include "MSL_C/wstring.h"
#include "nw4r/fn_805012C4.h"

/* A panel part: a band key with its state word; its class table (0x8064EC70) and the out-of-line copy of its inline destructor (0x8054D5DC) are
 * this unit's.  size: 0x34 */
class HbmPanelPart : public HbmKey {
public:
    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmPanelPart() {}
    /* untyped: opaque handle passed through */
    virtual void* getResource();
    virtual u32 getState();

    /* +0x30 */ void* mpResource;
}; /* size: 0x34 */

/* A HbmPanelPart kind (class table 0x8064EC44): its own slot 0x10 are defined above as C functions.  size: 0x34 */
class HbmPanelPartA : public HbmPanelPart {
public:
    virtual void setAnim(s32 anim);
    virtual ~HbmPanelPartA();
};


/* This unit's target object carries no `extab`/`extabindex` at all, so the original translation unit was
 * built with C++ exceptions off; cflags_hkb already passes -Cpp_exceptions off, and the pragma states it. */
#pragma exceptions off

/* This unit's own later entry point, called by fn_805486B0 before its definition. */
extern "C" void fn_805492A8(HbmTextPanel* self, u32* before, u32* after);
extern "C" void fn_80549570(HbmTextPanel* self, Panel805482CC_Cell* cell, u32 count);
extern "C" void hbmPanelUpdateButtonRepeat(HbmTextPanel* self, s32 b, s32 c);

/* Dispatch views of the panel's polymorphic parts: the slots are the ones the `*Vtbl` views of fn_805482CC.h name,
 * declared as virtuals so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch.  No method is
 * defined and nothing is constructed here, so no table is emitted (rule 10).  Slots are named by offset; the retail
 * names are not recoverable. */

/* The panel itself (Panel805482CC_Vtbl).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPanelView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018(s32 id, u32* arg);
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
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual void v_0x124();
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138();
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual void v_0x140();
    /* slot +0x144 */ virtual void v_0x144();
    /* slot +0x148 */ virtual void v_0x148();
    /* slot +0x14C */ virtual void v_0x14C();
    /* slot +0x150 */ virtual void v_0x150();
    /* slot +0x154 */ virtual void v_0x154();
    /* slot +0x158 */ virtual void v_0x158();
    /* slot +0x15C */ virtual void v_0x15C();
    /* slot +0x160 */ virtual void v_0x160();
    /* slot +0x164 */ virtual void v_0x164();
    /* slot +0x168 */ virtual void v_0x168();
    /* slot +0x16C */ virtual void v_0x16C();
    /* slot +0x170 */ virtual void v_0x170();
    /* slot +0x174 */ virtual void v_0x174();
    /* slot +0x178 */ virtual void v_0x178();
    /* slot +0x17C */ virtual void v_0x17C(s32 id);
};

/* The helper objects at +0x193C / +0x1940 / +0x1944 / +0x1A18 (Panel805482CC_ItemVtbl).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPanelItemView {
public:
    /* slot +0x008 */ virtual void v_0x008(s32 arg);
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual u16 v_0x01C();
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
    /* slot +0x04C */ virtual void v_0x04C(u32 key);
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual s32 v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C(u32 key);
    /* slot +0x060 */ virtual void v_0x060(s32 arg);
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void v_0x068();
    /* slot +0x06C */ virtual void v_0x06C(s32 arg);
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
    /* slot +0x078 */ virtual void v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual s32 v_0x088();
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
    /* slot +0x0CC */ virtual s32 v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual s32 v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* slot +0x0E4 */ virtual s32 v_0x0E4();
    /* slot +0x0E8 */ virtual s32 v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC(s32 arg);
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual u16 v_0x0F8();
    /* slot +0x0FC */ virtual s32 v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100(s32 arg);
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108(u32 key);
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C(s32 arg);
};

/* The +0x1940 helper, whose slots 0xF8 and 0x100 take other arguments than the same slots of the +0x193C/+0x1944
 * helpers (the panel's helpers are objects of more than one class).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPanelValueView {
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
    /* slot +0x0F8 */ virtual void v_0x0F8(s32 arg);
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual u32 v_0x100();
};

/* The panel's sub-object at +0x10, whose vptr sits at its own +0x4C (Panel805482CC_SubVtbl).
 * size: 0x50 (approximation: the head is not touched through this view) */
class HbmPanelSubView {
    /* +0x00 */ u8 unused_0x00[0x4C];
    /* +0x4C is the compiler-emitted vptr */
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
};

/* The scroll record embedded at +0x1964 (Panel805482CC_ScrollVtbl).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPanelScrollView {
public:
    /* slot +0x008 */ virtual void v_0x008(s32 a, s32 b, f32 c, f32 d, f32 e);
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual s32 v_0x014();
};

/* A layout pane the panel looks text boxes up in (slot 0x3C finds a pane by name, slot 0x7C sets a text box's string).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPaneView {
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
    /* slot +0x03C */ virtual HbmPaneView* v_0x03C(const char* name, int recursive);
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
    /* slot +0x07C */ virtual void v_0x07C(const u16* text, int length);
};

/* The layout the panel's +0x1A04 record holds at its +0x04: its root pane sits at +0x10.
 * size: 0x14 (approximation: only +0x10 is touched) */
struct HbmLayoutView {
    /* +0x00 */ u8 unused_0x00[0x10];
    /* +0x10 */ HbmPaneView* rootPane;
};

/* The panel's +0x1A04 record: an animation driver (slot 0x50 starts the named animation) holding the layout.
 * size: 0x08 (approximation: only +0x00 and +0x04 are touched) */
class HbmPanelAnimView {
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
    /* slot +0x050 */ virtual void v_0x050(const char* name, int on);

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ HbmLayoutView* layout;
};

#ifdef __cplusplus
extern "C" {
#endif

/* --- this unit's own entry points, in address order (so the bodies read in the object's order) --- */
void fn_805482CC(HbmTextPanel* self, Panel805482CC_Change* change);
void fn_80548394(Panel805482CC_Set* set, Panel805482CC_Item* item, u32 id, f32 a, f32 b, f32 c);
void fn_805483E4(HbmTextPanel* self);
void fn_80548680(HbmTextPanel* self);
void fn_80548694(void);
void fn_80548698(HbmTextPanel* self);
void fn_805486AC(void);
void fn_805486B0(HbmTextPanel* self);
s32 fn_8054893C(void);
s32 fn_80548944(void);
void fn_8054894C(void);
void fn_80548950(HbmTextPanel* self, u32 key);
void fn_80548B04(void);
void fn_80548B08(HbmTextPanel* self, u16 value);
Panel805482CC_Item* fn_80549CC0(HbmTextPanel* self, s32 flag);

/* ---------------------------------------------------------------------------------------------------
 * virtual slots of the band's keyboard classes that do nothing or return a constant
 * ------------------------------------------------------------------------------------------------ */

/* 0x80542FA0 (0x40): Deleting destructor of a keyboard object with no base destructor to run. */
u8* hbmKbSlot150DeletingDtor(u8* self, s32 flag)
{
    if (self != NULL) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x80542FE0 (0x40): Deleting destructor of a keyboard object with no base destructor to run. */
u8* hbmKbPlainDeletingDtor(u8* self, s32 flag)
{
    if (self != NULL) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x805431AC (0x4): Empty virtual slot. */
void hbmKbSlot158Nop(void)
{
}

/* 0x805431B0 (0x8): Stores the panel's +0x1952 byte. */
void hbmPanelSetInputLatch(HbmTextPanel* self, u8 value)
{
    self->field_0x1952 = value;
}

/* 0x805431B8 (0x4): Empty virtual slot. */
void hbmKbSlot130Nop(void)
{
}

/* 0x8054322C (0x8): Virtual slot that returns 1. */
s32 hbmKbSlotE4True(void)
{
    return 1;
}

/* 0x80543554 (0x38): Asks the panel's slot 0x18 for value 31 and returns it. */
u32 hbmPanelQuery31(HbmTextPanel* self)
{
    u32 value;
    ((HbmPanelView*)self)->v_0x018(31, &value);
    return value;
}

/* 0x805457A8 (0x4): Empty virtual slot. */
void hbmPanelSlot17CNop(void)
{
}

/* 0x805457AC (0x4): Empty virtual slot. */
void hbmKbSlot68Nop(void)
{
}

/* 0x805457B8 (0x38): Whether the widget's KPR queue at +0x2C holds no character. */
u32 hbmKbQueueIsEmpty(HkbWidget* self)
{
    return (u8)KPRLookAhead(&self->state, 0, 0) == 0;
}

/* 0x80545804 (0x8): Virtual slot that returns 0. */
s32 hbmKbSlot140False(void)
{
    return 0;
}

/* 0x8054580C (0x4): Empty virtual slot. */
void hbmKbSlotF4Nop(void)
{
}

/* 0x80545818 (0x4): Empty virtual slot. */
void hbmKbSlot6CNop(void)
{
}

/* 0x8054581C (0x18): Clears the widget's +0x08 word, then runs its slot 0x14 with the argument passed through. */
void hbmPanelResetAndSlot14(HkbWidget* self, u32 value)
{
    self->value_08 = 0;
    self->v_0x014(value);
}

/* 0x80545834 (0x4): Empty virtual slot. */
void hbmKbSlotF0Nop(void)
{
}

/* 0x80545838 (0x4): Empty virtual slot. */
void hbmKbSlotE0Nop(void)
{
}

/* 0x8054583C (0x4): Empty virtual slot. */
void hbmKbSlot128Nop(void)
{
}

/* 0x80545840 (0x4): Empty virtual slot. */
void hbmKbSlot134Nop(void)
{
}

/* 0x80545844 (0x4): Empty virtual slot. */
void hbmKbSlotF8Nop(void)
{
}

/* 0x80545848 (0x8): Virtual slot that returns 0. */
s32 hbmKbSlot124False(void)
{
    return 0;
}

/* 0x80545850 (0x8): Virtual slot that returns 0. */
s32 hbmKbSlotE8False(void)
{
    return 0;
}

/* 0x80545858 (0x4): Empty virtual slot. */
void hbmKbSlot11CNop(void)
{
}

/* 0x8054585C (0x4): Empty virtual slot. */
void hbmKbSlotF0NopB(void)
{
}

/* 0x80545AC0 (0x8): Virtual slot that returns 0. */
s32 hbmKbSlot118False(void)
{
    return 0;
}

/* 0x80545AC8 (0x8): Virtual slot that returns 0. */
s32 hbmKbSlotECFalse(void)
{
    return 0;
}

/* 0x80545B1C (0x8): Virtual slot that returns 0. */
s32 hbmKbSlotDCFalse(void)
{
    return 0;
}

/* Applies an absolute or relative text-position change to the panel, then reports it through slot 0x17C. */
void fn_805482CC(HbmTextPanel* self, Panel805482CC_Change* change)
{
    if (((HbmPanelScrollView*)&self->scroll)->v_0x014() != 0) {
        return;
    }
    if (change->field_0x00 != 0) {
        self->field_0x195C = change->field_0x04;
        self->sub.field_0xF0 = self->field_0x1960 = change->field_0x08;
    } else {
        f32 bound = lbl_8079D698;
        self->field_0x195C += change->field_0x04;
        ((HbmPanelScrollView*)&self->scroll)->v_0x008(0, 0, self->field_0x1960,
                                                     self->field_0x1960 + change->field_0x08, bound);
    }
    ((HbmPanelView*)self)->v_0x17C(11);
}

/*
 * Initialise a text-position record: the three positions, the 640.0f row bound, the "active" byte and
 * the object/id it belongs to, then hand the record to its owning object's dispatch slot 0x08.
 */
void fn_80548394(Panel805482CC_Set* set, Panel805482CC_Item* item, u32 id, f32 a, f32 b, f32 c)
{
    f32 bound = lbl_8079D628;

    set->field_0x04 = a;
    set->field_0x08 = b;
    set->field_0x10 = c;
    set->field_0x0C = bound;
    set->field_0x14 = 1;
    set->field_0x18 = item;
    set->field_0x1C = id;
    set->field_0x15 = 0;
    if (item != NULL) {
        ((HbmPanelItemView*)item)->v_0x008(0);
    }
}

/* Dispatch the panel's object at +0x1940 through its slot 0x110 (a tail call in retail). */
void fn_80548680(HbmTextPanel* self)
{
    Panel805482CC_Item* item = self->field_0x1940;

    ((HbmPanelItemView*)item)->v_0x110();
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548694(void)
{
}

/* Dispatch the panel's object at +0x1940 through its slot 0x114 (a tail call in retail). */
void fn_80548698(HbmTextPanel* self)
{
    Panel805482CC_Item* item = self->field_0x1940;

    ((HbmPanelItemView*)item)->v_0x114();
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_805486AC(void)
{
}

/*
 * Advance the panel one frame: with no input latched, pull the current position from the object at
 * +0x193C and push the delta into the text record and the panel's own cursor; with input latched,
 * either accept the object at +0x1940's pending selection or the one at +0x1944.
 */
void fn_805486B0(HbmTextPanel* self)
{
    Panel805482CC_Item* item;

    if (self->field_0x1950 == 0) {
        s32 before;
        s32 after;

        item = self->field_0x193C;
        if (((HbmPanelItemView*)item)->v_0x0CC() != 0) {
            return;
        }
        before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
        item = self->field_0x193C;
        ((HbmPanelItemView*)item)->v_0x0D4();
        item = self->field_0x193C;
        after = ((HbmPanelItemView*)item)->v_0x088();
        self->field_0x19F0->field_0x04 += after - before;
        self->field_0x19F8 += after - before;
        ((HbmPanelSubView*)&self->sub)->v_0x090();
        ((HbmPanelSubView*)&self->sub)->v_0x094();
    } else if (self->field_0x194C == 1) {
        u32 value;

        item = self->field_0x1940;
        if (((HbmPanelItemView*)item)->v_0x0FC() != 0) {
            item = self->field_0x1940;
            value = ((HbmPanelValueView*)item)->v_0x100();
            ((HbmPanelView*)self)->v_0x018(21, &value);
            return;
        }
        item = self->field_0x1940;
        ((HbmPanelItemView*)item)->v_0x06C(0);
        {
            s32 selected;
            u32 after;
            u32 before;

            selected = ((HbmPanelItemView*)self->field_0x1940)->v_0x0E8();
            before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

            item = self->field_0x193C;
            ((HbmPanelItemView*)item)->v_0x06C(selected);
            after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
            if (before != after) {
                fn_805492A8(self, &before, &after);
            }
        }
        item = self->field_0x1940;
        ((HbmPanelItemView*)item)->v_0x11C(0);
    } else {
        s32 selected;
        u32 after;
        u32 before;

        selected = ((HbmPanelItemView*)self->field_0x1944)->v_0x0E8();
        before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

        item = self->field_0x193C;
        ((HbmPanelItemView*)item)->v_0x060(selected);
        after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
        if (before != after) {
            fn_805492A8(self, &before, &after);
        }
        item = self->field_0x1944;
        ((HbmPanelItemView*)item)->v_0x0D8();
        self->field_0x19E0 = 0;
    }
}

/* Constant-0 accessor (retail has no body beyond `return 0`). */
s32 fn_8054893C(void)
{
    return 0;
}

/* Constant-0 accessor (retail has no body beyond `return 0`). */
s32 fn_80548944(void)
{
    return 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_8054894C(void)
{
}

/*
 * Route a key to the object the panel currently has selected, then run its own key handler.
 *
 * The three helper objects are tried in turn (the panel's input byte at +0x1950 and its mode word at
 * +0x194C pick which), and the key is special-cased for the two ids at 0x309B/0x309C.
 */
void fn_80548950(HbmTextPanel* self, u32 key)
{
    Panel805482CC_Item* item;

    item = fn_80549CC0(self, 0);

    if (self->field_0x1950 != 0 && self->field_0x194C == 1) {
        if ((u16)(key - 0x309B) <= 1) {
            ((HbmPanelView*)self)->v_0x018(27, NULL);
        } else {
            Panel805482CC_Item* selected = self->field_0x1940;

            if (((HbmPanelItemView*)selected)->v_0x0FC() != 0) {
                ((HbmPanelView*)self)->v_0x018(6, NULL);
            }
            selected = self->field_0x1940;
            ((HbmPanelItemView*)selected)->v_0x108(key);
        }
    } else if ((u16)(key - 0x309B) <= 1) {
        ((HbmPanelView*)self)->v_0x018(27, NULL);
    } else {
        ((HbmPanelItemView*)item)->v_0x04C(key);
    }

    if (key != 0) {
        ((HbmPanelView*)self)->v_0x17C(10);
    }
    self->field_0x1988 = 1;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548B04(void)
{
}

/* Store the incoming u16 into the sub-object's +0x04 field. */
void fn_80548B08(HbmTextPanel* self, u16 value)
{
    self->sub.field_0x04 = value;
}

/*
 * The panel's key handler: pick the object the current mode selects, run the mode-specific key
 * handling, then push the key into the text cursor and report the move.
 *
 * `a` is the key, `b` a flag that selects the "cursor moved" report only, `c`/`d`/`e` the mode
 * parameters the selected object's slots 0xEC / 0x54 / 0x100 take.
 */
void fn_80548B10(HbmTextPanel* self, u32 a, s32 b, s32 c, s32 d, s32 e)
{
    Panel805482CC_Item* item;

    item = fn_80549CC0(self, 1);

    if (a == 0) {
        return;
    }
    if (b != 0) {
        u32 after;
        u32 before;

        before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

        ((HbmPanelItemView*)self->field_0x193C)->v_0x05C(a);
        after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
        if (before != after) {
            fn_805492A8(self, &before, &after);
        }
        return;
    }

    if (item == self->field_0x1944) {
        if ((u16)((HbmPanelItemView*)self->field_0x1944)->v_0x0F8() == 0) {
            ((HbmPanelItemView*)self->field_0x1944)->v_0x0EC(c);
            ((HbmPanelItemView*)self->field_0x1944)->v_0x100(e);
        }
        if ((u16)((HbmPanelItemView*)self->field_0x1944)->v_0x0F8() >= 32) {
            ((HbmPanelView*)self)->v_0x018(6, NULL);
            ((HbmPanelItemView*)self->field_0x1944)->v_0x0EC(c);
            ((HbmPanelItemView*)self->field_0x1944)->v_0x05C(a);
            ((HbmPanelView*)self)->v_0x17C(9);
            ((HbmPanelView*)self)->v_0x0D8();
            return;
        }
    } else if (item == self->field_0x1940) {
        if (d != 0 && ((HbmPanelItemView*)self->field_0x1940)->v_0x054() == 0) {
            return;
        }
    } else if (self->field_0x1950 != 0 && self->field_0x194C == 1
               && item != self->field_0x1940 && a == 32) {
        a = 12288;
        ((HbmPanelItemView*)self->field_0x1940)->v_0x04C(0);
    }

    if (item == self->field_0x193C) {
        u32 after;
        u32 before;

        before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

        ((HbmPanelItemView*)self->field_0x193C)->v_0x05C(a);
        after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
        if (before != after) {
            fn_805492A8(self, &before, &after);
        }
    } else {
        ((HbmPanelItemView*)item)->v_0x05C(a);
    }
    ((HbmPanelView*)self)->v_0x17C(10);
}

/* Whether the sub-object's +0x04 field is non-zero (`(x | -x) >> 31`, retail's zero test). */
s32 fn_80548E2C(HbmTextPanel* self)
{
    return self->sub.field_0x04 != 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548E20(void)
{
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548E24(void)
{
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_80548E28(void)
{
}

/*
 * Re-sync the text box's node list after the object at +0x193C moved its cursor: walk the 8-byte
 * node list at +0x19E4, unlink the node the panel currently points at, then hand the new node and
 * the remaining count to fn_80549570 and refresh the sub-object's two layout slots.
 */
void fn_80548E40(HbmTextPanel* self)
{
    u32 before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
    u32 after;

    ((HbmPanelItemView*)self->field_0x193C)->v_0x068();
    after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
    if (before == after || before == 0) {
        return;
    }

    if (self->field_0x19F8 == 0) {
        Panel805482CC_Cell* cell = self->field_0x19F0;
        Panel805482CC_Cell* node = &self->field_0x19E4[cell->field_0x00];
        u16 spare;
        Panel805482CC_Cell* tail;
        u16 tail_next;

        node->field_0x04 = node->field_0x04 - 1;
        self->field_0x19F8 = node->field_0x04;
        node->field_0x04 = node->field_0x04 + cell->field_0x04;

        spare = self->field_0x19E4[cell->field_0x02].field_0x00;
        self->field_0x19E4[cell->field_0x02].field_0x00 = cell->field_0x00;
        self->field_0x19E4[cell->field_0x00].field_0x02 = cell->field_0x02;
        cell->field_0x00 = spare;
        cell->field_0x02 = spare;

        tail = &self->field_0x19E4[self->field_0x19E4[self->field_0x19E8].field_0x00];
        tail_next = tail->field_0x02;
        cell->field_0x00 = self->field_0x19E4[tail_next].field_0x00;
        cell->field_0x02 = tail_next;
        tail->field_0x02 = spare;
        self->field_0x19E4[tail_next].field_0x00 = spare;

        self->field_0x19F0 = node;
    } else {
        Panel805482CC_Cell* cell = self->field_0x19F0;

        cell->field_0x04 = cell->field_0x04 - 1;
        self->field_0x19F8 = self->field_0x19F8 - 1;
    }

    {
        u32 length = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

        fn_80549570(self, self->field_0x19F0, length - self->field_0x19F8);
    }
    ((HbmPanelSubView*)&self->sub)->v_0x090();
    ((HbmPanelSubView*)&self->sub)->v_0x094();
}

/* Hand the sub-object's layout pass at +0x118 to 0x8054F788, then refresh the sub-object. */
void fn_80549C50(HbmTextPanel* self)
{
    ((HbmPanelView*)self)->v_0x018(6, NULL);
    fn_8054F788(&self->field_0x118);
    ((HbmPanelSubView*)&self->sub)->v_0x090();
    ((HbmPanelSubView*)&self->sub)->v_0x094();
}

/* The helper the panel's current mode selects; with `flag` 0 it falls back to +0x193C while the selected helper's
 * slot 0xD8 / 0xE4 reports non-zero. */
#pragma push
#pragma auto_inline on
Panel805482CC_Item* fn_80549CC0(HbmTextPanel* self, s32 flag)
{
    if (self->field_0x1950 == 0) {
        return self->field_0x193C;
    }
    switch (self->field_0x194C) {
    case 1:
        if (((HbmPanelItemView*)self->field_0x1940)->v_0x0D8() == 0 || flag != 0) {
            return self->field_0x1940;
        }
        break;
    default:
        if (((HbmPanelItemView*)self->field_0x1944)->v_0x0E4() == 0 || flag != 0) {
            return self->field_0x1944;
        }
        break;
    }
    return self->field_0x193C;
}
#pragma pop

/* Whether the panel is in mode 1 (`field_0x194C == 1`) while its input byte is set. */
s32 fn_80549D74(HbmTextPanel* self)
{
    if (self->field_0x1950 == 0) {
        return 0;
    }
    return self->field_0x194C == 1;
}

/* Forward the +0x18B8 record to 0x80553670 (a tail call in retail). */
void fn_8054A8DC(HbmTextPanel* self)
{
    fn_80553670(&self->field_0x18B8);
}

/* Constant-0 accessor. */
s32 fn_8054CA28(void)
{
    return 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_8054CA30(void)
{
}

/* Constant-0 accessor. */
s32 fn_8054CA34(void)
{
    return 0;
}

/* Empty in retail (no body was emitted for this map entry). */
void fn_8054CA3C(void)
{
}

/* 0x8054CA40 (0x108): Whether the +0x193C helper is empty and ready and the helper the mode selects is idle. */
s32 hbmPanelIsReadyToCommit(HbmTextPanel* self)
{
    if (((HbmPanelItemView*)self->field_0x193C)->v_0x01C() != 0
        || ((HbmPanelItemView*)self->field_0x193C)->v_0x0CC() == 0) {
        return 0;
    }
    if (self->field_0x194C == 1) {
        if (((HbmPanelItemView*)self->field_0x1940)->v_0x0D8() == 0) {
            return 0;
        }
        if (((HbmPanelItemView*)self->field_0x193C)->v_0x01C() != 0) {
            return 0;
        }
        if (((HbmPanelItemView*)self->field_0x1940)->v_0x0FC() != 0) {
            return 0;
        }
    } else if (((HbmPanelItemView*)self->field_0x1944)->v_0x01C() != 0) {
        return 0;
    }
    return 1;
}

/* Store the incoming word into the panel's +0x1988 field. */
void fn_8054CDBC(HbmTextPanel* self, s32 value)
{
    self->field_0x1988 = value;
}

/* Forward the +0x19C4 record to 0x80526D00 (a tail call in retail). */
void fn_8054CDC4(HbmTextPanel* self)
{
    fn_80526D00(&self->field_0x19C4);
}

/* 0x8054CF04 (0x34): In mode 1 with input latched, passes -1 to slot 0xF8 of the +0x1940 helper. */
void hbmPanelResetCandidate(HbmTextPanel* self)
{
    if (self->field_0x1950 != 0 && self->field_0x194C == 1) {
        ((HbmPanelValueView*)self->field_0x1940)->v_0x0F8(-1);
    }
}

/* 0x8054D06C (0x40): Deleting destructor of a helper with no base destructor to run. */
u8* hbmPanelHelperDeletingDtor(u8* self, s32 flag)
{
    if (self != NULL) {
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Store the incoming byte into the panel's +0x1AB6 field. */
void fn_8054D810(HbmTextPanel* self, u8 value)
{
    self->field_0x1AB6 = value;
}

/* 0x8054DCC8 (0x104): Sets the text box's string and switches its two highlight animations on or off with it. */
void hbmPanelSetTextBoxString(HbmTextPanel* self, const u16* text)
{
    HbmPaneView* textBox =
        ((HbmPanelAnimView*)&self->field_0x1A04)->layout->rootPane->v_0x03C("T_As_TextBox_00", 1);

    textBox->v_0x07C(text, 0);
    if (wcslen(text) != 0) {
        ((HbmPanelAnimView*)&self->field_0x1A04)->v_0x050("W_TextBox_02", 1);
        ((HbmPanelAnimView*)&self->field_0x1A04)->v_0x050("T_As_TextBox_00", 1);
    } else {
        ((HbmPanelAnimView*)&self->field_0x1A04)->v_0x050("W_TextBox_02", 0);
        ((HbmPanelAnimView*)&self->field_0x1A04)->v_0x050("T_As_TextBox_00", 0);
    }
}

/* Drop the second argument and forward the rest to hbmPanelUpdateButtonRepeat (a tail call in retail). */
void fn_8054DDCC(HbmTextPanel* self, s32 a, s32 b, s32 c)
{
    hbmPanelUpdateButtonRepeat(self, b, c);
}

/* 0x8054E064 (0x11C): Records the pressed buttons and adds an auto-repeat press for each held direction button whose
 * countdown runs out (30 frames to the first repeat, 9 between repeats). */
void hbmPanelUpdateButtonRepeat(HbmTextPanel* self, s32 pressed, s32 held)
{
    self->field_0x1AF0 = pressed;
    if (held & 0x0002) {
        if (--self->field_0x1AE0 == 0) {
            self->field_0x1AF0 |= 0x0002;
            self->field_0x1AE0 = 9;
        }
    } else {
        self->field_0x1AE0 = 30;
    }
    if (held & 0x8000) {
        if (--self->field_0x1AE4 == 0) {
            self->field_0x1AF0 |= 0x8000;
            self->field_0x1AE4 = 9;
        }
    } else {
        self->field_0x1AE4 = 30;
    }
    if (held & 0x0001) {
        if (--self->field_0x1AD8 == 0) {
            self->field_0x1AF0 |= 0x0001;
            self->field_0x1AD8 = 9;
        }
    } else {
        self->field_0x1AD8 = 30;
    }
    if (held & 0x4000) {
        if (--self->field_0x1ADC == 0) {
            self->field_0x1AF0 |= 0x4000;
            self->field_0x1ADC = 9;
        }
    } else {
        self->field_0x1ADC = 30;
    }
    if (held & 0x1000) {
        if (--self->field_0x1AE8 == 0) {
            self->field_0x1AF0 |= 0x1000;
            self->field_0x1AE8 = 9;
        }
    } else {
        self->field_0x1AE8 = 30;
    }
}

/* Forwards the key release to the +0x1A04 pane manager (a tail call in retail). */
void fn_8054E05C(void* panel, HkbManager* manager)
{
    HbmTextPanel* self = (HbmTextPanel*)panel;
    ((HbmPaneManager*)&self->field_0x1A04)->HbmPaneManager::onKeyRelease(manager);
}

/* The u16 at +0x008. */
u16 fn_8054E524(HbmTextPanel* self)
{
    return self->field_0x008;
}

/* Dispatch the object at +0x1A18 through its slot 0x14 (a tail call in retail). */
void fn_8054E808(HbmTextPanel* self)
{
    Panel805482CC_Item* item = self->field_0x1A18;

    ((HbmPanelItemView*)item)->v_0x014();
}

/* Reset the +0x1A04 record and then the sub-object's layout at mode 0. */
void fn_8054E81C(void* panel)
{
    HbmTextPanel* self = (HbmTextPanel*)panel;
    ((HbmPaneManager*)&self->field_0x1A04)->HbmPaneManager::v_0x03C();
    fn_8055A3E0(&self->sub, 0);
}

/* Close the +0x1A04 record and then the sub-object's layout at mode 1. */
void fn_8054E858(void* panel)
{
    HbmTextPanel* self = (HbmTextPanel*)panel;
    ((HbmPaneManager*)&self->field_0x1A04)->HbmPaneManager::v_0x040();
    fn_8055A3E0(&self->sub, 1);
}

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

/* Returns the part's state. */
u32 HbmPanelPart::getState() {
    return mState;
}


/* Stores the state, then hands it to the band key. */
void HbmPanelPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
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

/* Appends a node to the widget's list at +0x04. */
extern "C" void fn_8054F134(HkbWidget* self, HkbNode* node) {
    nw4r::ut::List_Append((nw4r::ut::List*)&self->vtable_04, node);
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

/* Returns the part's resource word. */
/* untyped: opaque handle passed through */
void* HbmPanelPart::getResource() {
    return mpResource;
}


/* Clears the part's state. */
void HbmPanelPart::reset() {
    mState = 0;
}


/* Destroys the part (the inline base destructors run here). */
HbmPanelPartA::~HbmPanelPartA() {
}


/* Whether the widget's node list is empty. */
extern "C" u32 fn_8054F4A8(HkbWidget* self) {
    return (u8)KPRLookAhead(&self->state, 0, 0) == 0;
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
extern "C" void fn_8054F6BC(HkbWidget* self, s32 flag) {
    fn_8054CF38((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)), flag);
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
extern "C" void fn_8054F6EC(HkbWidget* self, HkbWidget* arg) {
    fn_80546BCC((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)), arg);
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
extern "C" void fn_8054F73C(HkbWidget* self, HkbManager* manager) {
    fn_8054E05C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)), manager);
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
/* untyped: opaque node record, typed by the callers' views */
extern "C" void fn_8054F788(void* record) {
    HkbNode* self = (HkbNode*)record;
    HkbWidget* target = self->target;
    self->argument = 0;
    if (target == 0) {
        return;
    }
    target->v_0x030(&self->argument);
}

#ifdef __cplusplus
}
#endif
