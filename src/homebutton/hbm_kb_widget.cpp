/*
 * homebutton/hbm_kb_widget.cpp - software-keyboard code of the tiHKBManager band: the keyboard widget's prediction
 * window (its handlers, its on/off switch, its scroll buttons), the prediction-candidate list it embeds at +0x18B8
 * (layout, drawing, scrolling, input), the input handlers of the window's buttons, flag accessors, deleting
 * destructors and adjustor thunks.
 *
 * RANGE.     .text 0x8054F7BC..0x805546B4 (102 functions), .ctors 0x8056F400..0x8056F404, .rodata 0x8057B2F0..0x8057B668,
 *            .data 0x8064F360..0x80650160; the functions not defined below keep the target object's bytes.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (see src/homebutton/hbm_kb_event.cpp); no extab/extabindex in the target, so
 *            `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around hbmKbCandSelect: retail inlines it into hbmKbCandHandleInput (the
 *            two slot lookups and the -1 store) and keeps it out of line; file-wide auto_inline inlines the adjustor
 *            thunks' callees and drops nine rows to 0.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the keyboard widget class; no `__FILE__` string and no
 *            runtime-dump name.
 * NAMES.     GUESS function names (map rows renamed with them): hkbKbWidget*, hbmKbWidget* (the widget's prediction
 *            handlers), hbmKbCand* (the candidate list), hbmKbButton* and hbmKbSwitch* (the window's button handlers),
 *            read off the bodies: the panes they bind by name, the states and sounds they set.
 * NAMES.     GUESS view classes HbmKbWidget, HbmKbCandidateList, HbmKbHandlerBase, HbmKbButtonHandler,
 *            HbmKbSwitchHandler, HbmKbPartView, HbmKbWordPartView, HbmKbButtonView, HbmKbHighlightView and the slot
 *            names of their views; HbmKbCandidateList's two bases follow its constructor's stores at +0x00/+0x14.
 * NAMES.     The widget's base at +0x17F4 is the band's pane manager (homebutton/hkb_keyboard.cpp's HbmPaneManager:
 *            DrawInfo at +0x18, component list at +0x6C, input flag at +0x88); its +0x04/+0x10/+0x14 are the layout
 *            view, the root layout and the sound player, and the slot names follow that lane's view.
 * SHAPES.    The views are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no
 *            method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * SHAPES.    The handlers, the candidate list and the text buffer are constructed here, but their virtual members are
 *            the C functions below (or other units'), so their tables are referenced by name and not emitted.
 * SHAPES.    hbmKbManagerOf converts through HbmKbWidgetBases (the widget's primary base and its pane manager), so the
 *            null-preserving base conversion is the compiler's; the attach helpers are inline so their name arguments
 *            stay in saved registers as retail's inlined members do; the candidate pane-name tables are globals
 *            defined at the end of the file (retail emits them after every function's strings) and copied whole.
 * SHAPES.    Signed compares (`cmpwi`, `ble`) read the widget's words as `s32`: fn_80551E5C switches on `(s32)state`,
 *            fn_80553F84 takes its selector as `s32`; the handlers test `event == 1` unsigned and `(s32)event == 0`.
 * SHAPES.    A call result stored through (`owner->getPanel()->flag = 0`) is split into a local first: written in one
 *            expression MWCC loads the constant into a saved register before the call.
 * SHAPES.    The text-width helper is inline (retail repeats it in every caller); its result is named in a local
 *            before `+ margin` (retail's operand order), and the candidate pane sizes and positions go through
 *            temporaries (Size, a VEC2 with a constructor, a VEC3) as retail's stack stores show.
 * SHAPES.    An inline helper is a CSE boundary: hbmKbWidgetClearCandidateTexts loops through hbmKbCandClearTexts (the
 *            counter then takes r31 and the induction pointer r30, as retail), and hbmKbWidgetScrollPrevPage tests the
 *            first-word index through hbmKbCandHasPrev, so the index is reloaded for the calls as retail does.
 * RESIDUALS. hbmKbCandHandleInput, hbmKbCandLayout, hbmKbCandScrollBy: the loop counter and the induction pointer take
 *            each other's saved registers (hbmKbCandLayout: inline loop helpers and 200 declaration orders measured
 *            no better).
 * RESIDUALS. hbmKbWidgetShowPredictInput: the two TexMap temporaries take each other's stack slots (0x08/0x24);
 *            renaming them or negating the test measured no better.  hbmKbCandSetWords: the second loop's offset and
 *            word pointer swap r4/r5.
 * RESIDUALS. hbmKbWidgetUpdateScrollButtons: retail reloads the button pane after the visibility test; ours reuses
 *            the loaded value (an inline visibility test, an inline show block and a direct flag test measured no
 *            better).
 * RESIDUALS. hbmKbCandDraw: retail's frame is 0x80 with the scissor rectangle above the transformed corners; ours 0x70.
 * RESIDUALS. The `.sdata2` constants (0.0f, 640.0f, 1.0f, 20.0f, 0.01f, 0.5f, 15.0f, the int->float magics) are this
 *            unit's own pool (0x8079D6A0..0x8079D6D0) but load from MWCC's anonymous entries (`@NN`): an ARG row each.
 * RESIDUALS. The pane-name strings and the `.sdata` names load from MWCC's anonymous pool (`...data.0`, `@NN`) at
 *            retail's offsets, where retail names the map's `lbl_` rows.
 * SHAPES.    hbmKbWidgetDestroy (0x8054F90C) is HbmKbPredictWidget's destructor; it stays a C-named wrapper
 *            `if (self) { widget->HbmKbPredictWidget::~HbmKbPredictWidget(); if (flag > 0) delete }` around the inline
 *            destructor (as homebutton/hbm_text_panel.cpp's fn_8054CF38): the adjustor thunk 0x8055467C and
 *            homebutton/hbm_value.cpp's derived destructor pass the flag through, which a qualified destructor call
 *            cannot (it passes -1).  Testing `self` rather than the typed local gives retail's r29/r30.
 * RESIDUALS. hbmKbWidgetDestroy: the inline destructor keeps its own `beq` after the wrapper's (one extra
 *            instruction).
 * SHAPES.    hbmKbWidgetCreate: the first pane manager is a named local converted in place (`(HbmKbWidgetBases*)self`),
 *            the switch panes bind through the inline hbmKbAttachSwitch, the candidate texts reset through
 *            hbmKbCandResetTexts, and the text buffer's storage is its own local before the placement new: each
 *            gives retail's colouring.
 * RESIDUALS. The constructors make MWCC emit a weak copy of HbmEventHandler's inline destructor
 *            (homebutton/hkb_keyboard.h), which retail keeps once (0x80538738).
 * RESIDUALS. __vt__15HbmKbTextBuffer (0x806515D0, homebutton/hbm_value.cpp's row) and __vt__17HbmKbScrollerView
 *            (0x8064F338, homebutton/hbm_text_panel.cpp's row) are other units' map rows still named `lbl_`.
 * SHAPES.    hbmKbWidgetCreateParts appends through the inline hbmKbAddComponent (the list address is recomputed per
 *            key and the kind-1 constant takes r14, as retail) and reads the animation count into a function-scope
 *            local declared before `group`.
 * RESIDUALS. hbmKbWidgetCreateParts: retail colours the animation count first (r31) and the class-table constants
 *            one register lower (r24..r30), ours the count at r23; HbmKey's (pane, listener) constructor in
 *            homebutton/hbm_value.h stores mState itself, so ours stores it before the part's class pointer, not after.
 * RESIDUALS. datagap reads emission-order seams in the claimed `.data` (0x8064FFC4 .. 0x806500A0); they are not TU
 *            edges: every class table there holds only this unit's functions and the shared bases' members, and this
 *            unit's constructors store 0x8064FFC4/0x8064FFF0/0x8065001C/0x80650038.
 * NAMES.     GUESS classes HbmKbWidgetPart and its kinds HbmKbWidgetPartA/B/C (band keys with a state word; members
 *            reset, setAnim, setState, getState, getType).
 * NAMES.     GUESS data names hbmKbWidgetAnimDefs, hbmKbWidgetKeyDefs, hbmKbWidgetTextGroup, hbmKbWidgetScrollGroup,
 *            hbmKbWidgetTextNames, hbmKbWidgetFrameNames; GUESS function names hbmKbWidgetCreate,
 *            hbmKbWidgetCreateParts, hbmKbWidgetUnfocusParts.
 * NAMES.     GUESS classes HbmKbWindowHandler, HbmKbTextBuffer and HbmKbTextBufferBase, HbmKbWidgetPrimary,
 *            HbmKbWidgetBases, HbmKbPaneNames; the class-table rows of HbmKbWindowHandler, HbmKbSwitchHandler,
 *            HbmKbButtonHandler, HbmKbCandidateList and HbmKbHandlerBase are named from the constructors' stores and
 *            the shared slots (onEvent 0x805544CC in every handler table).
 * SHAPES.    The key table's group words are pointer variables, so the unit's static initialiser (0x80554600) patches
 *            them, as homebutton/hbm_anim_record.cpp's table does.
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "homebutton/hkb_widget.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_kb_widget.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hbm_kb_child.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_kb_scroller.h"
#include "homebutton/tiHKBManager.h"
#include "homebutton/gui_manager.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hkb_texmap.h"
#include "MTX/mtxvec.h"
#include "RVLGX/GXTransform.h"

/* A widget part: a band key with its state word and its kind; its class table (0x8064FE74) and the out-of-line copy of
 * its inline destructor (0x80550494) are this unit's.  size: 0x34 */
class HbmKbWidgetPart : public HbmKey {
public:
    HbmKbWidgetPart(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKey(pane, listener) { reset(); }
    HbmKbWidgetPart(nw4r::lyt::Pane* pane, HbmKeyListener* listener, u32 type) : HbmKey(pane, listener) {
        reset();
        mType = type;
    }

    virtual void reset();
    virtual void setAnim(s32 anim);
    virtual void setState(u32 state);
    virtual ~HbmKbWidgetPart() {}
    virtual u32 getState();
    virtual u32 getType();

    /* +0x30 */ u32 mType;
}; /* size: 0x34 */

/* A prediction scroll button (kind 0; class table 0x8064FE48): steps a seven-state selector.  size: 0x34 */
class HbmKbWidgetPartA : public HbmKbWidgetPart {
public:
    HbmKbWidgetPartA(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKbWidgetPart(pane, listener) { mType = 0; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbWidgetPartA();
}; /* size: 0x34 */

/* A prediction on/off button (kind 2; class table 0x8064FE1C): steps a four-state selector.  size: 0x34 */
class HbmKbWidgetPartB : public HbmKbWidgetPart {
public:
    HbmKbWidgetPartB(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKbWidgetPart(pane, listener) { mType = 2; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbWidgetPartB();
}; /* size: 0x34 */

/* The prediction window (kind 3; class table 0x8064FDF0): plays its open and close animations.  size: 0x34 */
class HbmKbWidgetPartC : public HbmKbWidgetPart {
public:
    HbmKbWidgetPartC(nw4r::lyt::Pane* pane, HbmKeyListener* listener) : HbmKbWidgetPart(pane, listener) { mType = 3; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbWidgetPartC();
}; /* size: 0x34 */

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

class HbmKbHandlerBase;

/* A text pane of the candidate list: the box the candidate words and their buttons are drawn in. */
typedef nw4r::lyt::TextBox HbmKbTextPane;

/* A pane animation of the widget's pane manager (a gui component): slot 0x10 sets its frame, 0x14 plays an
 * animation, 0x18 whether it plays, 0x24 and 0x28 read its state and its kind, 0x54 enables it, 0x58 whether its
 * words are paged, 0x5C the key at a position; its pane is at +0x9C.  No method is defined and nothing is constructed
 * here, so no table is emitted (rule 10).
 * size: 0xA0 (approximation: the gui component record; only the vptr and +0x9C are touched) */
class HbmKbPartView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void setFrame(s32 frame);
    /* slot +0x014 */ virtual void play(s32 anim);
    /* slot +0x018 */ virtual bool v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual s32 v_0x024();
    /* slot +0x028 */ virtual s32 v_0x028();
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
    /* slot +0x054 */ virtual void v_0x054(s32 enable);
    /* slot +0x058 */ virtual bool isPaged();
    /* slot +0x05C */ virtual u16 getKeyAt(s32 position);

    /* +0x04 */ u8 unused_0x04[0x94];
    /* +0x98 */ HbmKbHandlerBase* handler;
    /* +0x9C */ HbmKbTextPane* pane;
}; /* size: 0xA0 */

/* A word part of the candidate list (its text or its frame): slot 0x10 replays its press animation, 0x54 enables it;
 * its pane is at +0x9C.  size: 0xA0 (approximation: the gui component record; only the vptr and +0x9C are touched) */
class HbmKbWordPartView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void replay();
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
    /* slot +0x054 */ virtual void setEnabled(s32 enable);

    /* +0x04 */ u8 unused_0x04[0x94];
    /* +0x98 */ HbmKbHandlerBase* handler;
    /* +0x9C */ HbmKbTextPane* pane;
}; /* size: 0xA0 */

/* A button of the prediction window: slot 0x10 replays its animation, 0x14 plays one; its pane is at +0x9C.
 * size: 0xA0 (approximation: the gui component record; only the vptr and +0x9C are touched) */
class HbmKbButtonView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void replay();
    /* slot +0x014 */ virtual void play(s32 anim);
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
    /* slot +0x054 */ virtual void setEnabled(s32 enable);

    /* +0x04 */ u8 unused_0x04[0x94];
    /* +0x98 */ HbmKbHandlerBase* handler;
    /* +0x9C */ nw4r::lyt::Pane* pane;
}; /* size: 0xA0 */

/* The pane manager the widget derives from (+0x17F4), as its virtual slots: slot 0x2C finds a text box by name,
 * 0x5C a pane animation by name.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbLayoutView {
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
    /* slot +0x02C */ virtual nw4r::lyt::Pane* findTextBox(const char* name);
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
    /* slot +0x05C */ virtual HbmKbPartView* getPaneAnim(const char* name);
}; /* size: 0x04 */

/* The text panel the widget's owner hands back through slot 0x68: its +0x19E1 byte marks a pending candidate.
 * size: 0x19E2 (approximation: only +0x19E1 is touched) */
struct HbmKbPanelState {
    /* +0x0000 */ u8 unused_0x0000[0x19E1];
    /* +0x19E1 */ u8 candidatePending;
}; /* size: 0x19E2 */

/* The owner the widget reads its text panel from (+0x17F0): slot 0x68 returns the panel.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbOwnerView {
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
    /* slot +0x068 */ virtual HbmKbPanelState* getPanel();
}; /* size: 0x04 */

/* The layout view of the widget's pane manager (+0x17F8): slot 0x30 takes the DrawInfo, 0x38 shows or hides it.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbSceneView {
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
    /* slot +0x030 */ virtual void setDrawInfo(const nw4r::lyt::DrawInfo* drawInfo);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void setVisible(s32 visible);
}; /* size: 0x04 */

/* The sound player of the widget's pane manager (+0x1808): slot 0x14 plays a sound.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbSoundView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void play(s32 sound);
}; /* size: 0x04 */

/* The root layout of the widget's pane manager (+0x1804), its buttons' registry: slot 0x10 refreshes it.
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbPaneManagerView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void refresh();
}; /* size: 0x04 */

/* The button component the pane manager hands back by name: its +0x04 byte holds it pressed.
 * size: 0x08 (approximation: only +0x04 is touched) */
struct HbmKbButtonState {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u8 held;
}; /* size: 0x08 */

/* A candidate highlight: slot 0x08 clears it (homebutton/hbm_value.cpp's node object).
 * size: 0x04 (approximation: only the vptr is touched) */
class HbmKbHighlightView {
public:
    /* slot +0x008 */ virtual void clear();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void setMode(s32 mode);
    /* slot +0x014 */ virtual void setState(s32 state);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual s32 getState();
}; /* size: 0x04 */

/* The prediction dictionary the candidate list shows: the word count, then the words (61 wide characters each).
 * size: 0x4 + count * 0x7A */
struct HbmKbCandidateSet {
    /* +0x00 */ s32 count;
    /* +0x04 */ wchar_t words[1][0x3D];
}; /* size: 0x7E (approximation: the word array runs to `count`) */

struct HbmKbInputEvent;

/* The primary base of the prediction window's input handlers and of the candidate list (class table 0x80650064): an
 * event handler with the handler's id at +0x08, the keyboard widget it belongs to at +0x0C and the listener its
 * events go to at +0x10.  Its members are defined below as C functions (0x8054FA34, 0x805544CC, 0x805544DC), so
 * its table is not emitted here.
 * size: 0x14 */
class HbmKbHandlerBase : public HbmEventHandler {
public:
    HbmKbHandlerBase(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener)
        : id(handlerId), widget(pWidget), listener(pListener) {}

    /* slot +0x008 */ virtual ~HbmKbHandlerBase();
    /* untyped: caller-owned payload - the argument record of the event */
    /* slot +0x00C */ virtual void onEvent(u32 eventId, u32 event, void* data);
    /* slot +0x018 */ virtual void handleInput(HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input);

    /* +0x08 */ u32 id;
    /* +0x0C */ HbmKbWidget* widget;
    /* +0x10 */ HkbWidget* listener;
}; /* size: 0x14 */

/* The input record the handlers read: the pointer position at +0x00, the trigger kind at +0x04, the trigger flags at
 * +0x0C and the button state at +0x10.  size: 0x14 */
struct HbmKbInputEvent {
    /* +0x00 */ s32 position;
    /* +0x04 */ u32 trigger;
    /* +0x08 */ u32 unused_0x08;
    /* +0x0C */ u32 flags;
    /* +0x10 */ u32 state;
}; /* size: 0x14 */

/* The handler of one prediction-window scroll button (class table 0x8065001C): the button's key and base
 * components and its part.  Its members are defined below as C functions, so its table is not emitted here.
 * size: 0x20 */
class HbmKbButtonHandler : public HbmKbHandlerBase {
public:
    HbmKbButtonHandler(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener);

    /* slot +0x008 */ virtual ~HbmKbButtonHandler();
    /* slot +0x018 */ virtual void handleInput(HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input);

    /* +0x14 */ HbmKbButtonView* keyComponent;
    /* +0x18 */ HbmKbButtonView* baseComponent;
    /* +0x1C */ HbmKbPartView* part;
}; /* size: 0x20 */

/* The handler of the prediction on/off switch (class table 0x8064FFF0): the on and off buttons' key and base
 * components and their parts, and the listener of the parts' animations at +0x14.  Its members are defined below
 * as C functions, so its table is not emitted here.  size: 0x30 */
class HbmKbSwitchHandler : public HbmKbHandlerBase, public HbmKeyListener {
public:
    HbmKbSwitchHandler(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener);

    /* slot +0x008 */ virtual ~HbmKbSwitchHandler();
    /* slot +0x018 */ virtual void handleInput(HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input);
    /* slot +0x008 */ virtual void onAnimEvent(u32 event, HbmKey* key, HbmKeyAnim* anim);

    /* +0x18 */ HbmKbButtonView* onKey;
    /* +0x1C */ HbmKbPartView* source;
    /* +0x20 */ HbmKbButtonView* offKey;
    /* +0x24 */ HbmKbButtonView* offBase;
    /* +0x28 */ HbmKbPartView* onButton;
    /* +0x2C */ HbmKbPartView* offButton;
}; /* size: 0x30 */

/* The handler of the prediction window (class table 0x8064FFC4): its key component and its part, and the listener
 * of the part's animations at +0x14.  Its members are defined below as C functions, so its table is not emitted
 * here.  size: 0x20 */
class HbmKbWindowHandler : public HbmKbHandlerBase, public HbmKeyListener {
public:
    HbmKbWindowHandler(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener);

    /* slot +0x008 */ virtual ~HbmKbWindowHandler();
    /* slot +0x008 */ virtual void onAnimEvent(u32 event, HbmKey* key, HbmKeyAnim* anim);

    /* +0x18 */ HbmKbButtonView* keyComponent;
    /* +0x1C */ HbmKbPartView* part;
}; /* size: 0x20 */

/* The scroll-listener base of the candidate list (+0x14, its table 0x80650054): the scroller reports the end of a
 * scroll through it.  size: 0x04 */
class HbmKbScrollListener {
public:
    /* slot +0x008 */ virtual void onScrolled(s32 kind, s32 delta);
}; /* size: 0x04 */

/* The prediction-candidate list embedded in the keyboard widget (+0x18B8): twenty word slots (a text pane, its
 * frame and its highlight each), the window it scrolls through the dictionary and the scroller that moves it.
 * Its members are defined below as C functions, so its table (0x80650038) is not emitted here.
 * size: 0x200 */
class HbmKbCandidateList : public HbmKbHandlerBase, public HbmKbScrollListener {
public:
    HbmKbCandidateList(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener);

    /* slot +0x008 */ virtual ~HbmKbCandidateList();
    /* slot +0x018 */ virtual void handleInput(HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input);
    /* slot +0x008 */ virtual void onScrolled(s32 kind, s32 delta);

    /* +0x018 */ HbmKbScrollerView scroller;
    /* +0x028 */ f32 value_028;
    /* +0x02C */ u8 flag_02C;
    /* +0x02D */ u8 flag_02D;
    /* +0x02E */ u8 unused_0x02E[2];
    /* +0x030 */ u32 value_030;
    /* +0x034 */ u8 unused_0x034[4];
    /* +0x038 */ HbmKbWordPartView* texts[20];
    /* +0x088 */ HbmKbWordPartView* frames[20];
    /* +0x0D8 */ HbmKbPartView* area;
    /* +0x0DC */ HbmKbPartView* window;
    /* +0x0E0 */ HbmKbHighlightView* highlights[20];
    /* +0x130 */ HbmIdValue highlightValues[20];
    /* +0x1D0 */ s32 selected;
    /* +0x1D4 */ s32 pressed;
    /* +0x1D8 */ s32 first;
    /* +0x1DC */ f32 scroll;
    /* +0x1E0 */ s32 count;
    /* +0x1E4 */ s32 shownFirst;
    /* +0x1E8 */ s32 shownLast;
    /* +0x1EC */ s32 lastWhole;
    /* +0x1F0 */ f32 tailOffset;
    /* +0x1F4 */ f32 areaWidth;
    /* +0x1F8 */ u8 touched;
    /* +0x1F9 */ u8 unused_0x1F9[3];
    /* +0x1FC */ HbmKbCandidateSet* words;
}; /* size: 0x200 */

/* The widget's primary base: 0x17F4 bytes and the seventeen slots of the widget's primary table (0x08..0x48; its
 * destructor runs no code of its own).  size: 0x17F4 (approximation: only the size and the slots are used) */
class HbmKbWidgetPrimary {
public:
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
    /* slot +0x008 */ virtual ~HbmKbWidgetPrimary() {}
    /* slot +0x010 */ virtual void init();
    /* slot +0x014 */ virtual void setState(s32 state);
    /* untyped: caller-owned payload - the record the property id selects */
    /* slot +0x018 */ virtual void setProperty(u32 id, void* value);
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual bool isPredicting();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void setActive(s32 active);
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual bool isLocked();
    /* slot +0x048 */ virtual void v_0x048();

    /* +0x0004 */ u8 unused_0x0004[0x17F0];
}; /* size: 0x17F4 */

/* The widget's first two bases, its primary base and its pane manager at +0x17F4: the conversion between them is
 * the compiler's.  size: 0x189C (approximation: only the bases' layout is used) */
class HbmKbWidgetBases : public HbmKbWidgetPrimary, public HbmPaneManager {};

/* The keyboard widget the band's handlers run on, as the handlers of this unit see it: its own slots (named by
 * offset), the layout base at +0x17F4, the DrawInfo its layout is drawn with, the prediction-candidate list and
 * the gui components of the prediction window.  Declared as virtuals so MWCC emits the canonical
 * `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no method is defined and nothing is constructed here, so no
 * table is emitted (rule 10).
 * size: 0x1B8C (approximation: bounded by the last field the handlers touch) */
class HbmKbWidget {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void setState(s32 state);
    /* untyped: caller-owned payload - the record the property id selects */
    /* slot +0x018 */ virtual void setProperty(u32 id, void* value);
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual bool isPredicting();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void setActive(s32 active);
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual bool isLocked();
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
    /* slot +0x0D8 */ virtual void pagePrevious();
    /* slot +0x0DC */ virtual void pageNext();
    /* slot +0x0E0 */ virtual void setSwitch(s32 on);
    /* slot +0x0E4 */ virtual void refreshKeys();
    /* slot +0x0E8 */ virtual void clearCandidate();
    /* slot +0x0EC */ virtual void selectCandidate(s32 index);
    /* slot +0x0F0 */ virtual void commitCandidate();
    /* slot +0x0F4 */ virtual void v_0x0F4();
    /* slot +0x0F8 */ virtual bool isBusy();
    /* slot +0x0FC */ virtual void setPredictMode(s32 mode);
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void setFocus(s32 index);
    /* slot +0x108 */ virtual void setCursor(s32 index);
    /* slot +0x10C */ virtual s32 getCandidateCode();

    /* +0x0004 */ u8 unused_0x0004[0xC];
    /* +0x0010 */ u8 layoutBlock[0x17D8];
    /* +0x17E8 */ u8 flag_17E8;
    /* +0x17E9 */ u8 predictEnabled;
    /* +0x17EA */ u8 unused_0x17EA[2];
    /* +0x17EC */ u32 textureMode;
    /* +0x17F0 */ HbmKbOwnerView* owner;
    /* +0x17F4 */ void* vtable_17F4;     /* the layout base (HbmKbLayoutView) */
    /* +0x17F8 */ HbmKbSceneView* layoutView;
    /* +0x17FC */ u8 unused_0x17FC[8];
    /* +0x1804 */ HbmKbPaneManagerView* root;
    /* +0x1808 */ HbmKbSoundView* sound;
    /* +0x180C */ nw4r::lyt::DrawInfo drawInfo;
    /* +0x1860 */ u8 unused_0x1860[0xC];
    /* +0x186C */ f32 areaLeft;
    /* +0x1870 */ u8 unused_0x1870[4];
    /* +0x1874 */ f32 areaRight;
    /* +0x1878 */ u8 unused_0x1878[4];
    /* +0x187C */ u8 layoutShown;
    /* +0x187D */ u8 unused_0x187D[0x23];
    /* +0x18A0 */ HbmPaneHandler paneHandler;
    /* +0x18AC */ u32 unused_0x18AC;
    /* +0x18B0 */ HbmKbTextBuffer* textBuffer;
    /* +0x18B4 */ MEMAllocator* allocator;
    /* +0x18B8 */ HbmKbCandidateList candidates;
    /* +0x1AB8 */ HbmKbHandlerBase leftHandler;
    /* +0x1ACC */ HbmKbButtonView* leftArrow;
    /* +0x1AD0 */ HbmKbButtonView* leftButton;
    /* +0x1AD4 */ HbmKbButtonView* leftAnim;
    /* +0x1AD8 */ HbmKbHandlerBase rightHandler;
    /* +0x1AEC */ HbmKbButtonView* rightArrow;
    /* +0x1AF0 */ HbmKbButtonView* rightButton;
    /* +0x1AF4 */ HbmKbButtonView* rightAnim;
    /* +0x1AF8 */ HbmKbHandlerBase switchHandler;
    /* +0x1B0C */ HbmKeyListener switchListener;
    /* +0x1B10 */ HbmKbButtonView* onButton;
    /* +0x1B14 */ HbmKbButtonView* onButtonFrame;
    /* +0x1B18 */ HbmKbButtonView* offButton;
    /* +0x1B1C */ HbmKbButtonView* offButtonFrame;
    /* +0x1B20 */ HbmKbButtonView* onLabel;
    /* +0x1B24 */ HbmKbButtonView* offLabel;
    /* +0x1B28 */ GXTexObj textureOn;
    /* +0x1B48 */ GXTexObj textureOff;
    /* +0x1B68 */ HbmKbHandlerBase windowHandler;
    /* +0x1B7C */ HbmKeyListener windowListener;
    /* +0x1B80 */ HbmKbButtonView* switchButton;
    /* +0x1B84 */ HbmKbPartView* switchPart;
    /* +0x1B88 */ u8 flag_1B88;
}; /* size: 0x1B8C */

/* The widget's third base (+0x189C): the receiver of its pane events (slot 0x08 is hbmKbWidgetHandleEvent's adjustor
 * thunk 0x805546AC).  size: 0x4 */
class HbmKbWidgetEventSink {
public:
    /* slot +0x008 */ virtual void handleEvent(u32 sender, s32 event, s32 arg);
}; /* size: 0x4 */

/* The keyboard widget as its destructor builds it: the primary base, the pane manager at +0x17F4 and the event sink
 * at +0x189C (class table 0x8064FEA0 with its secondary tables at +0x4C and +0xC0).  A slot with no body here is
 * declared first, so it stays the key function and the table is not emitted here; the destructor is inline and
 * reached through hbmKbWidgetDestroy.  size: 0x1B8C (approximation: the fields the destructor touches) */
class HbmKbPredictWidget : public HbmKbWidgetPrimary, public HbmPaneManager, public HbmKbWidgetEventSink {
public:
    /* slot +0x010 */ virtual void init();
    /* slot +0x008 */ virtual ~HbmKbPredictWidget() {
        textBuffer->~HbmKbTextBuffer();
        MEMFreeToAllocator(allocator, textBuffer);
        for (HbmKey* key = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL); key != NULL;
             key = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL)) {
            nw4r::ut::List_Remove(&mComponents, key);
            key->destroy(allocator);
        }
    }

    /* +0x18A0 */ u8 unused_0x18A0[0x10];
    /* +0x18B0 */ HbmKbTextBuffer* textBuffer;
    /* +0x18B4 */ MEMAllocator* allocator;
    /* +0x18B8 */ u8 unused_0x18B8[0x2D4];
}; /* size: 0x1B8C */

/* The layout base of the widget, through which its panes and components are found by name. */
static inline HbmKbLayoutView* hbmKbLayout(HbmKbWidget* self) {
    return (HbmKbLayoutView*)&self->vtable_17F4;
}

/* The pane manager the widget derives from (its base at +0x17F4). */
static inline HbmPaneManager* hbmKbPaneManager(HbmKbWidget* self) {
    return (HbmPaneManager*)&self->vtable_17F4;
}

/* The widget converted to its pane manager base: a null widget stays null. */
static inline HbmPaneManager* hbmKbManagerOf(HbmKbWidget* self) {
    return (HbmKbWidgetBases*)self;
}

extern "C" {
void hbmKbWidgetCreateParts(HbmKbWidget* self, MEMAllocator* allocator);
void hbmKbCandLayout(HbmKbCandidateList* self);
void hbmKbCandSetWords(HbmKbCandidateList* self);
void hbmKbCandDraw(HbmKbCandidateList* self, const nw4r::lyt::DrawInfo* drawInfo);
s32 hbmKbCandNextWindow(HbmKbCandidateList* self, s32 index);
s32 hbmKbCandPrevWindow(HbmKbCandidateList* self, s32 index);
void hbmKbCandScrollTo(HbmKbCandidateList* self, s32 index);
}

/* Sets a pane's size. */
static inline void hbmKbSetPaneSize(nw4r::lyt::Pane* pane, const nw4r::lyt::Size& size) {
    pane->mSize = size;
}

/* A two-component vector built from its components.  size: 0x8 */
struct HbmKbVec2 : public nw4r::math::VEC2 {
    HbmKbVec2(f32 fx, f32 fy) {
        x = fx;
        y = fy;
    }
}; /* size: 0x8 */

/* Places a pane at `pos` in its parent's plane. */
static inline void hbmKbSetPaneTranslate(nw4r::lyt::Pane* pane, const nw4r::math::VEC2& pos) {
    nw4r::math::VEC3 translate;
    translate.x = pos.x;
    translate.y = pos.y;
    translate.z = 0.0f;
    pane->mTranslate = translate;
}

/* The drawn width of a wide string in a text box's font, character spacing and font size; 0 for no string. */
static inline f32 hbmKbTextWidth(const nw4r::lyt::TextBox* text, const wchar_t* str) {
    if (str == NULL) {
        return 0.0f;
    }
    f32 charSpace;
    f32 fontWidth;
    const nw4r::ut::Font* font = text->GetFont();
    fontWidth = text->mFontSize.width;
    charSpace = text->mCharSpace;
    f32 width = 0.0f;
    for (; *str != 0; str++) {
        width += charSpace;
        width += font->GetCharWidth(*str);
    }
    return 0.01f + width * (fontWidth / font->GetWidth());
}

/* The drawn width of a text box's own string; 0 for no box. */
static inline f32 hbmKbPaneTextWidth(const nw4r::lyt::TextBox* text) {
    if (text == NULL) {
        return 0.0f;
    }
    return hbmKbTextWidth(text, text->mTextBuf);
}

/* The word slot that shows candidate `index`, or -1 when the word is outside the shown range. */
static inline s32 hbmKbCandidateSlot(const HbmKbCandidateList* list, s32 index) {
    s32 slot;
    if (index < list->shownFirst) {
        return -1;
    }
    if (index > list->shownLast) {
        return -1;
    }
    if (index < list->first) {
        slot = 20 - (list->first - index);
    } else {
        slot = index - list->first;
    }
    while (slot < 0) {
        slot += 20;
    }
    while (slot >= 20) {
        slot -= 20;
    }
    return slot;
}

/* The number of words the list shows. */
static inline s32 hbmKbShownCount(const HbmKbCandidateList* list) {
    return list->count > 0 ? list->shownLast - list->shownFirst + 1 : 0;
}

/* Saves every highlight's state and clears it. */
static inline void hbmKbResetHighlights(HbmKbCandidateList* list) {
    u32 i;
    for (i = 0; i < 20; i++) {
        hbmAnimSaveState((HbmWidgetNodeView*)list->highlights[i], &list->highlightValues[i]);
        HbmIdValue empty = {0, 0.0f};
        list->highlights[i]->clear();
        hbmAnimRestoreState((HbmWidgetNodeView*)list->highlights[i], &empty);
    }
}

/* The prediction window's key animations (.rodata 0x8057B2F0). */
const HkbAnimDef hbmKbWidgetAnimDefs[13] = {
    {0, "fs_VK_predictInput_a_normal.brlan"},
    {1, "fs_VK_predictInput_a_Foucus_IN.brlan"},
    {2, "fs_VK_predictInput_a_Focus_OUT.brlan"},
    {4, "fs_VK_predictInput_a_Pushed.brlan"},
    {5, "fs_VK_predictInput_a_Fade_IN.brlan"},
    {6, "fs_VK_predictInput_a_Fade_OUT.brlan"},
    {3, "fs_VK_predictInput_a_Roll_over.brlan"},
    {7, "fs_VK_predictInput_a_OnOffButton_Pushed.brlan"},
    {8, "fs_VK_predictInput_a_predict_ON.brlan"},
    {9, "fs_VK_predictInput_a_predict_OFF.brlan"},
    {11, "fs_VK_predictInput_a_invalidMode_ON.brlan"},
    {10, "fs_VK_predictInput_a_invalidMode_normal.brlan"},
    {12, "fs_VK_predictInput_a_invalidMode_OFF.brlan"},
};

/* The keys whose animations the candidate texts and the right scroll button share. */
const char* hbmKbWidgetTextGroup = "T_prdc_Text_00";
const char* hbmKbWidgetScrollGroup = "P_prdc_scrl_Left";

#define HBM_KB_WIDGET_TEXT_ANIMS                                                                                       \
    {&hbmKbWidgetAnimDefs[0], &hbmKbWidgetAnimDefs[1], &hbmKbWidgetAnimDefs[2], &hbmKbWidgetAnimDefs[3],               \
     &hbmKbWidgetAnimDefs[6]}
#define HBM_KB_WIDGET_SCROLL_ANIMS                                                                                     \
    {&hbmKbWidgetAnimDefs[0], &hbmKbWidgetAnimDefs[1], &hbmKbWidgetAnimDefs[2], &hbmKbWidgetAnimDefs[3],               \
     &hbmKbWidgetAnimDefs[4], &hbmKbWidgetAnimDefs[5], &hbmKbWidgetAnimDefs[6]}
#define HBM_KB_WIDGET_SWITCH_ANIMS                                                                                     \
    {&hbmKbWidgetAnimDefs[0], &hbmKbWidgetAnimDefs[1], &hbmKbWidgetAnimDefs[2], &hbmKbWidgetAnimDefs[3],               \
     &hbmKbWidgetAnimDefs[4], &hbmKbWidgetAnimDefs[5], &hbmKbWidgetAnimDefs[6], &hbmKbWidgetAnimDefs[7]}
#define HBM_KB_WIDGET_TEXT(name) {1, name, 5, hbmKbWidgetTextGroup, HBM_KB_WIDGET_TEXT_ANIMS}

/* The prediction input, its scroll buttons, the twenty candidate texts, the on/off buttons and the window
 * (.data 0x8064F388). */
HbmKbChildKeyDef hbmKbWidgetKeyDefs[26] = {
    {1, "N_predictInput", 1, NULL, {&hbmKbWidgetAnimDefs[0]}},
    {0, "P_prdc_scrl_Left", 7, NULL, HBM_KB_WIDGET_SCROLL_ANIMS},
    {0, "P_prdc_scrl_Rght", 7, hbmKbWidgetScrollGroup, HBM_KB_WIDGET_SCROLL_ANIMS},
    {1, "T_prdc_Text_00", 5, NULL, HBM_KB_WIDGET_TEXT_ANIMS},
    HBM_KB_WIDGET_TEXT("T_prdc_Text_01"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_02"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_03"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_04"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_05"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_06"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_07"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_08"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_09"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_10"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_11"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_12"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_13"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_14"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_15"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_16"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_17"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_18"),
    HBM_KB_WIDGET_TEXT("T_prdc_Text_19"),
    {2, "P_OnBtn", 8, NULL, HBM_KB_WIDGET_SWITCH_ANIMS},
    {2, "P_OffBtn", 8, NULL, HBM_KB_WIDGET_SWITCH_ANIMS},
    {3, "W_predictWindow", 6, NULL,
     {&hbmKbWidgetAnimDefs[0], &hbmKbWidgetAnimDefs[8], &hbmKbWidgetAnimDefs[9], &hbmKbWidgetAnimDefs[10],
      &hbmKbWidgetAnimDefs[11], &hbmKbWidgetAnimDefs[12]}},
};

/* Empty virtual slot. */
extern "C" void fn_8054F7BC(void) {
}

/* The widget's own update, then the +0x10 virtual slot. */
extern "C" void fn_8054F7C0(HkbWidget* self, HkbEventSink* sink) {
    ((HkbInputWidget*)self)->HkbInputWidget::setEventSink(sink);
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

/* 0x8054F90C (0xE8): Destroys the keyboard widget (its text buffer, its pane parts and the pane manager) and frees it
 * when `flag` is positive. */
/* untyped: opaque band object - the widget or a class derived from it (homebutton/hbm_value.cpp) */
extern "C" void* hbmKbWidgetDestroy(void* self, s32 flag) {
    HbmKbPredictWidget* widget = (HbmKbPredictWidget*)self;
    if (self != NULL) {
        widget->HbmKbPredictWidget::~HbmKbPredictWidget();
        if (flag > 0) {
            operator delete(widget);
        }
    }
    return widget;
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

/* The pane names of the twenty candidate words.  size: 0x50 */
struct HbmKbPaneNames {
    /* +0x00 */ const char* names[20];
}; /* size: 0x50 */

extern HbmKbPaneNames hbmKbWidgetTextNames;
extern HbmKbPaneNames hbmKbWidgetFrameNames;

/* Binds the candidate list's word texts, frames and highlights, its text area and its window to their panes, and
 * the frames' and the area's presses to the list. */
static inline void hbmKbAttachCandidates(HbmPaneManager* pManager, HbmKbCandidateList* pList) {
    HbmKbPaneNames textNames = hbmKbWidgetTextNames;
    HbmKbPaneNames frameNames = hbmKbWidgetFrameNames;
    PaneManager* pRoot = (PaneManager*)pManager->getRoot();
    for (u32 i = 0; i < 20; i++) {
        pList->texts[i] = (HbmKbWordPartView*)hkbGetPaneComponentByName(pRoot, textNames.names[i]);
        pList->frames[i] = (HbmKbWordPartView*)hkbGetPaneComponentByName(pRoot, frameNames.names[i]);
        pList->highlights[i] = (HbmKbHighlightView*)pManager->getPaneAnim(textNames.names[i]);
        pList->frames[i]->handler = pList;
    }
    pList->area = (HbmKbPartView*)hkbGetPaneComponentByName(pRoot, "N_prdcTextArea");
    pList->window = (HbmKbPartView*)hkbGetPaneComponentByName(pRoot, "N_prdc_Texts");
    pList->area->handler = pList;
}

/* Binds the prediction window's button and animation to their pane. */
static inline void hbmKbAttachWindow(HbmPaneManager* pManager, HbmKbButtonView** pKey, HbmKbPartView** pAnim,
                                     const char* name) {
    PaneManager* pRoot = (PaneManager*)pManager->getRoot();
    *pKey = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, name);
    *pAnim = (HbmKbPartView*)pManager->getPaneAnim(name);
}

/* Binds a scroll button's key, base and animation to their panes and the button's presses to its handler. */
static inline void hbmKbAttachButton(HbmPaneManager* pManager, HbmKbButtonView** pKey, HbmKbButtonView** pBase,
                                     HbmKbButtonView** pAnim, HbmKbHandlerBase* pHandler, const char* keyName,
                                     const char* baseName) {
    PaneManager* pRoot = (PaneManager*)pManager->getRoot();
    *pKey = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, keyName);
    *pBase = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, baseName);
    *pAnim = (HbmKbButtonView*)pManager->getPaneAnim(keyName);
    (*pBase)->handler = pHandler;
}

/* Empties every candidate text and enables its frame. */
static inline void hbmKbCandResetTexts(HbmKbCandidateList* list) {
    for (u32 i = 0; i < 20; i++) {
        list->texts[i]->pane->SetString(L"", 0);
        list->frames[i]->setEnabled(1);
    }
}

/* Binds the on/off switch's buttons, frames and labels to their panes. */
static inline void hbmKbAttachSwitch(HbmPaneManager* pManager, HbmKbWidget* self) {
    PaneManager* pRoot = (PaneManager*)pManager->getRoot();
    self->onButton = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, "P_OnBtn");
    self->onButtonFrame = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, "B_OnBtn");
    self->onLabel = (HbmKbButtonView*)pManager->getPaneAnim("P_OnBtn");
    self->offButton = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, "P_OffBtn");
    self->offButtonFrame = (HbmKbButtonView*)hkbGetPaneComponentByName(pRoot, "B_OffBtn");
    self->offLabel = (HbmKbButtonView*)pManager->getPaneAnim("P_OffBtn");
}

/* 0x8054FBB4 (0x570): Creates the pane manager, the text buffer and the parts, binds the candidate words, the on/off
 * switch, the scroll buttons and the window to their panes, then initialises the widget. */
extern "C" void hbmKbWidgetCreate(HbmKbWidget* self, MEMAllocator* allocator) {
    self->allocator = allocator;
    hbmKbPaneManager(self)->HbmPaneManager::create(allocator, &self->paneHandler);
    void* pMemory = MEMAllocFromAllocator(allocator, sizeof(HbmKbTextBuffer));
    HbmKbTextBuffer* pBuffer = new (pMemory) HbmKbTextBuffer();
    self->textBuffer = pBuffer;
    pBuffer->create(allocator);
    hbmKbPaneManager(self)->mpRoot->setVisible(0);
    hbmKbPaneManager(self)->mpRoot->v_0x00C();
    hbmKbWidgetCreateParts(self, allocator);

    HbmPaneManager* pFirst = (HbmKbWidgetBases*)self;
    hbmKbAttachCandidates(pFirst, &self->candidates);
    self->candidates.words = (HbmKbCandidateSet*)self->layoutBlock;
    hbmKbCandResetTexts(&self->candidates);
    self->candidates.tailOffset = 0.0f;

    HbmPaneManager* pManager = hbmKbManagerOf(self);
    hbmKbAttachSwitch(pManager, self);
    const nw4r::lyt::Material* pMaterial =
        pManager->getLayout()->mpRootPane->FindPaneByName("P_OffBtn", true)->GetMaterial();
    pMaterial->GetTexMapAry()->Get(&self->textureOn);
    pMaterial = pManager->getLayout()->mpRootPane->FindPaneByName("P_JPOffBtn", true)->GetMaterial();
    pMaterial->GetTexMapAry()->Get(&self->textureOff);
    self->onButtonFrame->handler = self->offButtonFrame->handler = &self->switchHandler;
    self->onButtonFrame->setEnabled(1);
    self->offButtonFrame->setEnabled(1);

    hbmKbAttachButton(hbmKbManagerOf(self), &self->leftArrow, &self->leftButton, &self->leftAnim, &self->leftHandler,
                      "P_prdc_scrl_Left", "B_prdc_scrl_Left");
    hbmKbAttachButton(hbmKbManagerOf(self), &self->rightArrow, &self->rightButton, &self->rightAnim,
                      &self->rightHandler, "P_prdc_scrl_Rght", "B_prdc_scrl_Rght");
    self->leftButton->setEnabled(1);
    self->rightButton->setEnabled(1);

    hbmKbAttachWindow(hbmKbManagerOf(self), &self->switchButton, &self->switchPart, "W_predictWindow");
    self->v_0x010();
}

/* Appends a part to the pane manager's component list. */
static inline void hbmKbAddComponent(HbmPaneManager* manager, HbmKey* part) {
    nw4r::ut::List_Append(&manager->mComponents, part);
}

/* 0x80550128 (0x360): Creates one part per key of the prediction window and gives it its animations. */
extern "C" void hbmKbWidgetCreateParts(HbmKbWidget* self, MEMAllocator* allocator) {
    u32 numAnims;
    const char* group;

    for (u16 i = 0; i < 26; i++) {
        const HbmKbChildKeyDef* def = &hbmKbWidgetKeyDefs[i];
        HbmKbWidgetPart* pPart = NULL;
        switch (def->type) {
        case 0:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbWidgetPartA)))
                HbmKbWidgetPartA(hbmKbPaneManager(self)->findTextBox(def->name), NULL);
            break;
        case 1:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbWidgetPart)))
                HbmKbWidgetPart(hbmKbPaneManager(self)->findTextBox(def->name), NULL, 1);
            break;
        case 2:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbWidgetPartB)))
                HbmKbWidgetPartB(hbmKbPaneManager(self)->findTextBox(def->name), &self->switchListener);
            break;
        case 3:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbWidgetPartC)))
                HbmKbWidgetPartC(hbmKbPaneManager(self)->findTextBox(def->name), &self->windowListener);
            break;
        }
        hbmKbAddComponent(hbmKbPaneManager(self), pPart);
        group = def->group;
        numAnims = def->numAnims;
        for (u16 j = 0; j < numAnims; j++) {
            const void* res = hbmKbPaneManager(self)->mpResAccessor->GetResource(0, def->anims[j]->name, NULL);
            nw4r::lyt::AnimTransform* anim =
                hbmKbPaneManager(self)->getLayout()->CreateAnimTransform(res, hbmKbPaneManager(self)->mpResAccessor);
            if (group == NULL) {
                pPart->addAnim(allocator, def->anims[j]->kind, anim, 0, 1);
            } else {
                pPart->addGroupAnim(allocator, def->anims[j]->kind, anim, group, 0, 1);
            }
        }
    }
}

/* Clears the part's state. */
void HbmKbWidgetPart::reset() {
    mState = 0;
}


/* Shows the prediction input, clears its candidate texts and frames, and binds the on/off texture to the switch. */
extern "C" void hbmKbWidgetShowPredictInput(HbmKbWidget* self) {
    hbmKbLayout(self)->getPaneAnim("N_predictInput")->play(0);
    self->predictEnabled = 0;
    self->setPredictMode(1);
    self->setActive(0);
    self->drawInfo.mFlag.invisiblePaneCalculateMtx = 1;
    hbmKbCandResetTexts(&self->candidates);
    f32 areaWidth = self->areaRight - self->areaLeft;
    self->candidates.tailOffset = 0.0f;
    self->candidates.areaWidth = areaWidth;
    if (self->textureMode == 0) {
        nw4r::lyt::TexMap texMap(self->textureOff);
        self->offButton->pane->GetMaterial()->GetTexMapAry()[0] = texMap;
    } else {
        nw4r::lyt::TexMap texMap(self->textureOn);
        self->offButton->pane->GetMaterial()->GetTexMapAry()[0] = texMap;
    }
    self->switchPart->play(9);
    self->layoutView->setVisible(0);
    self->layoutView->setDrawInfo(&self->drawInfo);
}

/* Stores the keyboard widget's +0x1B88 byte. */
extern "C" void fn_80550768(HkbWidget* self, u8 flag) {
    self->flag_1B88 = flag;
}

/* Hides the candidate text area, steps the layout base, shows the area again and draws the candidate list. */
extern "C" void hbmKbWidgetDrawCandidates(HbmKbWidget* self) {
    hbmKbLayout(self)->findTextBox("N_prdc_Texts")->SetVisible(false);
    hbmKbPaneManager(self)->HbmPaneManager::calcComponents();
    hbmKbLayout(self)->findTextBox("N_prdc_Texts")->SetVisible(true);
    hbmKbCandDraw(&self->candidates, &self->drawInfo);
}

/* Steps the layout base and the candidate scroller, re-lays the candidate words out and, while the layout is shown,
 * animates and recalculates the candidate text area. */
extern "C" void hbmKbWidgetUpdateCandidates(HbmKbWidget* self) {
    hbmKbPaneManager(self)->HbmPaneManager::drawComponents();
    self->candidates.scroller.step();
    if (self->candidates.scroller.isMoving()) {
        self->candidates.scroll = self->candidates.scroller.getOffset();
    }
    hbmKbCandLayout(&self->candidates);
    if (self->layoutShown) {
        hbmKbLayout(self)->findTextBox("N_prdc_Texts")->Animate(0);
        hbmKbLayout(self)->findTextBox("N_prdc_Texts")->CalculateMtx(self->drawInfo);
    }
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

/* Switches the prediction buttons between the on and the off look, optionally replaying the window's animation. */
extern "C" void hbmKbWidgetSetPredictLook(HbmKbWidget* self, s32 animate) {
    bool off = !self->isPredicting();
    if (off) {
        self->switchButton->pane->SetVisible(true);
        self->switchPart->play(8);
    } else {
        self->switchPart->play(9);
    }
    if (self->isPredicting()) {
        hbmKbLayout(self)->getPaneAnim("P_OnBtn")->setFrame(0);
        if (animate) {
            self->sound->play(17);
        }
    } else {
        hbmKbLayout(self)->getPaneAnim("P_OffBtn")->setFrame(0);
        hbmKbLayout(self)->getPaneAnim("P_prdc_scrl_Left")->play(0);
        hbmKbLayout(self)->getPaneAnim("P_prdc_scrl_Rght")->play(0);
        if (animate) {
            self->sound->play(16);
        }
    }
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

/* Clears every candidate text of the list. */
static inline void hbmKbCandClearTexts(HbmKbCandidateList* list) {
    u32 i;
    for (i = 0; i < 20; i++) {
        list->texts[i]->pane->SetString(L"", 0);
    }
}

/* Clears every candidate text and the panel's pending-candidate mark. */
extern "C" void hbmKbWidgetClearCandidateTexts(HbmKbWidget* self) {
    hbmKbCandClearTexts(&self->candidates);
    HbmKbPanelState* panel = self->owner->getPanel();
    panel->candidatePending = 0;
}

/* Drops the panel's pending candidate and resets the focus and the cursor. */
extern "C" void hbmKbWidgetResetCandidate(HbmKbWidget* self) {
    HbmKbPanelState* panel = self->owner->getPanel();
    panel->candidatePending = 0;
    hbmPanelResetCandidate((struct HbmTextPanel*)self->owner->getPanel());
    self->setFocus(-1);
    self->setCursor(-1);
}

/* Stores the keyboard widget's +0x1A88 word. */
extern "C" void fn_80550CBC(HkbWidget* self, u32 value) {
    self->value_1A88 = value;
}

/* Stores the keyboard widget's +0x1A8C word. */
extern "C" void fn_80550CC4(HkbWidget* self, u32 value) {
    self->value_1A8C = value;
}

/* Drops the panel's pending candidate, applies property 0x17 and resets the focus and the cursor. */
extern "C" void hbmKbWidgetCancelCandidate(HbmKbWidget* self) {
    HbmKbPanelState* panel = self->owner->getPanel();
    panel->candidatePending = 0;
    self->setProperty(0x17, 0);
    self->setFocus(-1);
    self->setCursor(-1);
}

/* Marks the panel's candidate pending, applies the candidate code as property 0x16 and moves focus and cursor to it. */
extern "C" void hbmKbWidgetFocusCandidate(HbmKbWidget* self, s32 code) {
    u32 value;
    HbmKbPanelState* panel = self->owner->getPanel();
    panel->candidatePending = 1;
    value = (u16)code;
    self->setProperty(0x16, &value);
    self->setFocus(code);
    self->setCursor(code);
}

/* Stores the +0x17E8 flag, applies it as property 0x1D, resets the switch labels and shows the on or the off buttons;
 * switched off, it also hides the switch, resets its part and clears every candidate text. */
extern "C" void hbmKbWidgetSetPredictFlag(HbmKbWidget* self, u8 flag) {
    HkbWidgetProperty prop;
    u32 i;
    self->flag_17E8 = flag;
    self->setProperty(0x1F, &prop);
    prop.flag_04 = flag;
    self->setProperty(0x1D, &prop);
    self->onLabel->play(0);
    self->offLabel->play(0);
    if (flag) {
        self->onButton->pane->SetVisible(true);
        self->onButtonFrame->pane->SetVisible(true);
        self->offButton->pane->SetVisible(false);
        self->offButtonFrame->pane->SetVisible(false);
    } else {
        self->onButton->pane->SetVisible(false);
        self->onButtonFrame->pane->SetVisible(false);
        self->offButton->pane->SetVisible(true);
        self->offButtonFrame->pane->SetVisible(true);
    }
    if (!flag) {
        self->switchButton->pane->SetVisible(false);
        self->switchPart->play(0);
        for (i = 0; i < 20; i++) {
            self->candidates.texts[i]->pane->SetString(L"", 0);
        }
    }
}

/* Takes a new keyboard layout block: resets the scroller, holds the prediction button while the layout base
 * re-binds and the keys refresh, then empties the candidate list and commits. */
extern "C" void hbmKbWidgetSetLayout(HbmKbWidget* self, const u8* layout) {
    HbmKbButtonState* button;
    self->candidates.scroller.reset();
    memcpy(self->layoutBlock, layout, 0x17D8);
    button = (HbmKbButtonState*)hkbGetPaneComponentByName((PaneManager*)self->root,
                                                         self->isPredicting() ? "B_OnBtn" : "B_OffBtn");
    if (button) {
        button->held = 1;
    }
    hbmKbPaneManager(self)->HbmPaneManager::init();
    self->refreshKeys();
    if (button) {
        button->held = 0;
    }
    self->candidates.pressed = -1;
    self->candidates.selected = -1;
    self->candidates.scroll = 0.0f;
    self->candidates.shownFirst = 0;
    self->candidates.shownLast = 0;
    hbmKbResetHighlights(&self->candidates);
    self->candidates.first = 0;
    self->candidates.scroll = 0.0f;
    self->candidates.pressed = -1;
    hbmKbCandSetWords(&self->candidates);
    self->commitCandidate();
}

/* Shows the left and right scroll buttons while there are candidates beyond the window, hides them otherwise. */
extern "C" void hbmKbWidgetUpdateScrollButtons(HbmKbWidget* self) {
    if (self->candidates.first <= 0) {
        self->leftArrow->pane->SetVisible(false);
        self->leftButton->pane->SetVisible(false);
        self->leftAnim->play(0);
    } else if (!self->leftArrow->pane->IsVisible()) {
        self->leftArrow->pane->SetVisible(true);
        self->leftButton->pane->SetVisible(true);
        self->leftButton->replay();
    }
    if (self->candidates.lastWhole >= self->candidates.count - 1) {
        self->rightArrow->pane->SetVisible(false);
        self->rightButton->pane->SetVisible(false);
        self->rightAnim->play(0);
    } else if (!self->rightArrow->pane->IsVisible()) {
        self->rightArrow->pane->SetVisible(true);
        self->rightButton->pane->SetVisible(true);
        self->rightButton->replay();
    }
}

/* Scrolls the candidate list one word on while its last word is not wholly shown, and replays the window. */
extern "C" void hbmKbWidgetScrollNextWord(HbmKbWidget* self) {
    if (self->candidates.lastWhole < self->candidates.count - 1) {
        hbmKbCandScrollBy(&self->candidates, 1);
        self->sound->play(11);
    }
}

/* Scrolls the candidate list one word back while its first word is not shown, and replays the window. */
extern "C" void hbmKbWidgetScrollPrevWord(HbmKbWidget* self) {
    if (self->candidates.first > 0) {
        hbmKbCandScrollBy(&self->candidates, -1);
        self->sound->play(11);
    }
}

/* Scrolls the candidate list one word on, unless its last word is already shown. */
extern "C" void hbmKbWidgetScrollNextPage(HbmKbWidget* self) {
    if (self->candidates.lastWhole < self->candidates.count - 1) {
        if (self->candidates.scroll > 0.0f) {
            hbmKbCandScrollTo(&self->candidates, self->candidates.first);
        } else if (!(self->candidates.scroll < 0.0f)) {
            s32 index = hbmKbCandNextWindow(&self->candidates, self->candidates.first);
            if (index >= 0) {
                hbmKbCandScrollTo(&self->candidates, index);
            }
        }
        self->sound->play(11);
    }
}

/* Whether the candidate list has words before its first shown one. */
static inline bool hbmKbCandHasPrev(const HbmKbCandidateList* list) {
    return list->first > 0;
}

/* Scrolls the candidate list one word back, unless its first word is already shown. */
extern "C" void hbmKbWidgetScrollPrevPage(HbmKbWidget* self) {
    if (hbmKbCandHasPrev(&self->candidates)) {
        if (self->candidates.scroll < 0.0f) {
            hbmKbCandScrollTo(&self->candidates, self->candidates.first);
        } else if (!(self->candidates.scroll > 0.0f)) {
            hbmKbCandScrollTo(&self->candidates, hbmKbCandPrevWindow(&self->candidates, self->candidates.first));
        }
        self->sound->play(11);
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

/* 0x805514E0 (0x1A0): Takes the focus off every focused or pressed part of the prediction window. */
extern "C" void hbmKbWidgetUnfocusParts(HbmKbWidget* self) {
    for (u32 i = 0; i < 26; i++) {
        HbmKey* pPart = hbmKbPaneManager(self)->getPaneAnim(hbmKbWidgetKeyDefs[i].name);
        if (pPart != NULL) {
            switch (((HbmKbWidgetPart*)pPart)->getType()) {
            case 1:
                switch (((HbmKbWidgetPart*)pPart)->getState()) {
                case 1:
                case 3:
                case 4:
                    pPart->setState(2);
                    break;
                }
                break;
            case 0:
                switch (((HbmKbWidgetPart*)pPart)->getState()) {
                case 1:
                case 3:
                    pPart->setState(2);
                    break;
                }
                break;
            case 2:
                switch (((HbmKbWidgetPart*)pPart)->getState()) {
                case 1:
                case 3:
                    pPart->setState(2);
                    break;
                }
                break;
            }
        }
    }
}

/* Returns the part's kind. */
u32 HbmKbWidgetPart::getType() {
    return mType;
}


/* Returns the part's state. */
u32 HbmKbWidgetPart::getState() {
    return mState;
}


/* Stores the state, then hands it to the band key. */
void HbmKbWidgetPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
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

/* The widget's event handler (base #6): window states, candidate commit, selection and the prediction switch. */
extern "C" void hbmKbWidgetHandleEvent(HbmKbWidget* self, u32 sender, s32 event, s32 arg) {
    (void)sender;
    switch (event) {
    case 0:
        self->sound->play(arg);
        break;
    case 0x100:
        self->commitCandidate();
        break;
    case 0x101:
        if (self->getCandidateCode() != 0xFFFF) {
            u32 code;
            HbmKbPanelState* panel = self->owner->getPanel();
            panel->candidatePending = 0;
            code = (u16)self->getCandidateCode();
            self->sound->play(9);
            self->setProperty(0x15, &code);
        }
        break;
    case 0x102:
        if (arg >= 0) {
            self->selectCandidate(arg);
        } else {
            self->clearCandidate();
        }
        break;
    case 0x300:
        self->setActive(1);
        self->root->refresh();
        hbmKbLayout(self)->getPaneAnim("P_prdc_scrl_Left")->play(0);
        hbmKbLayout(self)->getPaneAnim("P_prdc_scrl_Rght")->play(0);
        if (self->predictEnabled) {
            hbmKbLayout(self)->getPaneAnim("W_predictWindow")->play(11);
        }
        break;
    case 0x301:
        self->clearCandidate();
        self->setActive(0);
        self->root->refresh();
        break;
    case 0x400:
        self->root->refresh();
        break;
    }
}

/* The keyboard widget's +0x1A8C word. */
extern "C" u32 fn_80551964(HkbWidget* self) {
    return self->value_1A8C;
}

/* Empty virtual slot. */
extern "C" void fn_8055196C(void) {
}

/* Steps a five-state button through its states for the caller's event. */
void HbmKbWidgetPart::setAnim(s32 event) {
    if (event == 0) {
        setState(4);
        return;
    }
    if (event == 5) {
        setState(0);
        return;
    }
    switch ((s32)mState) {
    case 0:
        if (event == 1) {
            setState(1);
        }
        break;
    case 1:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(2);
        }
        break;
    case 3:
        if (event == 2) {
            setState(2);
        }
        break;
    case 2:
        if (event == 4) {
            setState(0);
        }
        if (event == 1) {
            setState(1);
        }
        break;
    case 4:
        if (event == 4) {
            setState(3);
        }
        if (event == 2) {
            setState(2);
        }
        break;
    }
}

/* Steps a seven-state button (with a held state 6) through its states for the caller's event. */
extern "C" void hbmKbButtonStep7(HkbWidget* self, s32 event) {
    if (event == 0 && (s32)self->state != 6) {
        self->v_0x014(4);
    }
    if (event == 6 && (s32)self->state == 6) {
        self->v_0x014(5);
    }
    if (event == 7 && (s32)self->state != 6) {
        self->v_0x014(6);
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
            self->v_0x014(3);
        }
        if (event == 2) {
            self->v_0x014(2);
        }
        break;
    case 5:
        if (event == 4) {
            self->v_0x014(0);
        }
        break;
    case 6:
        break;
    }
}

/* Steps a four-state button (with a locked state 4) through its states for the caller's event. */
extern "C" void hbmKbButtonStep4(HkbWidget* self, s32 event) {
    if (event == 0) {
        self->v_0x014(7);
        return;
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
        break;
    }
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
    ((HbmPaneManager*)&self->vtable_17F4)->HbmPaneManager::v_0x03C();
    self->v_0x110();
}

/* The width the two +0x186C/+0x1874 timers span. */
extern "C" void fn_80551EF4(HkbWidget* self) {
    self->value_1AAC = self->value_1874 - self->value_186C;
}

/* Virtual slot +0x10 on base #5, then the widget's own +0x110 slot. */
extern "C" void fn_80551F08(HkbWidget* self) {
    ((HbmPaneManager*)&self->vtable_17F4)->HbmPaneManager::v_0x040();
    self->v_0x110();
}

/* 0x80551F4C (0x17C): Builds an empty candidate list: no words, no selection, the full area width. */
HbmKbCandidateList::HbmKbCandidateList(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener)
    : HbmKbHandlerBase(handlerId, pWidget, pListener), value_028(0.0f), flag_02C(0), flag_02D(0), value_030(0),
      selected(-1), pressed(-1), first(0), scroll(0.0f), count(0), shownFirst(0), shownLast(0), lastWhole(0),
      tailOffset(0.0f), areaWidth(640.0f), touched(0), words(NULL) {
    for (int i = 0; i < 20; i++) {
        texts[i] = NULL;
        frames[i] = NULL;
        highlights[i] = NULL;
    }
    area = NULL;
    window = NULL;
}

/* Lays the candidate words out around the scroll offset: the words before the window's first word fill leftwards,
 * the rest rightwards until the area is full; records the shown range and the last whole word. */
extern "C" void hbmKbCandLayout(HbmKbCandidateList* self) {
    f32 margin = 20.0f;
    f32 x = self->scroll;
    f32 left = self->area->pane->mTranslate.x;
    f32 areaWidth = self->area->pane->mSize.width;
    s32 selected = self->selected;
    s32 i;
    s32 slot;
    s32 index;
    bool whole;

    for (i = 0; i < 20; i++) {
        HbmKbTextPane* text = self->texts[i]->pane;
        nw4r::lyt::Pane* frame = self->frames[i]->pane;
        const nw4r::lyt::Size size = text->GetSize();
        text->SetString(L"", 0);
        text->SetVisible(false);
        hbmKbSetPaneSize(frame, nw4r::lyt::Size(0.0f, size.height));
    }

    slot = 20;
    index = self->window->isPaged() ? self->first : 0;
    while (x > 0.0f) {
        HbmKbTextPane* text;
        nw4r::lyt::Pane* frame;
        f32 width;
        f32 pos;
        if (index == 0) {
            break;
        }
        slot--;
        index--;
        if (slot < 0) {
            slot = 19;
        }
        text = self->texts[slot]->pane;
        frame = self->frames[slot]->pane;
        text->SetString(self->words->words[index], 0);
        text->SetVisible(true);
        const nw4r::lyt::Size size = text->GetSize();
        width = hbmKbPaneTextWidth(text);
        x -= margin;
        x -= width;
        hbmKbSetPaneSize(text, nw4r::lyt::Size(width, size.height));
        hbmKbSetPaneSize(frame, nw4r::lyt::Size(width, size.height));
        pos = width * 0.5f + (left + x);
        hbmKbSetPaneTranslate(text, HbmKbVec2(pos, 0.0f));
        hbmKbSetPaneTranslate(frame, HbmKbVec2(pos, 0.0f));
        if (index == selected) {
            self->highlights[slot]->setMode(1);
        } else {
            self->highlights[slot]->setMode(2);
        }
    }
    self->shownFirst = index;

    slot = 0;
    index = self->window->isPaged() ? self->first : self->count;
    x = self->scroll;
    while (x < areaWidth) {
        HbmKbTextPane* text;
        nw4r::lyt::Pane* frame;
        f32 width;
        f32 pos;
        if (index == self->count) {
            break;
        }
        text = self->texts[slot]->pane;
        frame = self->frames[slot]->pane;
        text->SetString(self->words->words[index], 0);
        text->SetVisible(true);
        const nw4r::lyt::Size size = text->GetSize();
        width = hbmKbPaneTextWidth(text);
        hbmKbSetPaneSize(text, nw4r::lyt::Size(width, size.height));
        hbmKbSetPaneSize(frame, nw4r::lyt::Size(width, size.height));
        pos = width * 0.5f + (left + x);
        hbmKbSetPaneTranslate(text, HbmKbVec2(pos, 0.0f));
        hbmKbSetPaneTranslate(frame, HbmKbVec2(pos, 0.0f));
        if (index == selected) {
            self->highlights[slot]->setMode(1);
        }
        x += margin;
        x += width;
        index++;
        if (x <= 0.0f) {
            self->highlights[slot]->setMode(5);
        }
        if (++slot >= 20) {
            slot = 0;
        }
    }

    for (i = 0; i < 20; i++) {
        if (!self->texts[i]->pane->IsVisible()) {
            self->highlights[i]->setMode(5);
        }
    }
    self->shownLast = index - 1;
    whole = true;
    if (!(x - margin <= areaWidth) && hbmKbShownCount(self) != 1) {
        whole = false;
    }
    if (whole) {
        self->lastWhole = self->shownLast;
    } else {
        self->lastWhole = self->shownLast - 1;
    }
    self->tailOffset = x;
}

#pragma push
#pragma auto_inline on
/* Moves the selection highlight from the selected word to `index`. */
extern "C" void hbmKbCandSelect(HbmKbCandidateList* self, s32 index) {
    s32 slot = hbmKbCandidateSlot(self, self->selected);
    if (slot >= 0) {
        self->highlights[slot]->setMode(2);
    }
    slot = hbmKbCandidateSlot(self, index);
    if (slot >= 0) {
        self->highlights[slot]->setMode(1);
    }
    self->selected = index;
}
#pragma pop

/* Takes a new dictionary: resets the highlights and the selection, replaces the space characters that open a word
 * with the blank glyph, and lays the words out. */
extern "C" void hbmKbCandSetWords(HbmKbCandidateList* self) {
    u32 i;
    self->count = self->words->count;
    for (i = 0; i < 20; i++) {
        self->highlights[i]->setState(0);
    }
    self->selected = -1;
    self->pressed = -1;
    self->scroll = 0.0f;
    for (i = 0; i < self->count; i++) {
        wchar_t* word = self->words->words[i];
        if (word[0] == 0x20 || word[0] == 0x3000) {
            word[0] = 0xE057;
        }
    }
    hbmKbCandLayout(self);
}

/* Draws the candidate words clipped to the candidate area, keeping the selected and the pressed words in view while
 * the list scrolls. */
extern "C" void hbmKbCandDraw(HbmKbCandidateList* self, const nw4r::lyt::DrawInfo* drawInfo) {
    s32 selected = hbmKbCandidateSlot(self, self->selected);
    u32 i;
    u32 scissorLeft;
    u32 scissorTop;
    u32 scissorWidth;
    u32 scissorHeight;
    f32 scale;
    f32 center;
    nw4r::math::VEC2 pos;
    nw4r::math::VEC2 size;

    if (!self->scroller.isMoving()) {
        for (i = 0; i < 20; i++) {
            if (i == selected || self->highlights[i]->getState() == 2) {
                self->texts[i]->pane->SetVisible(false);
            }
        }
    }
    const nw4r::ut::Rect rect = self->area->pane->GetPaneRect(*drawInfo);
    Vec leftTop = {0.0f, 0.0f, 0.0f};
    Vec rightBottom = {0.0f, 0.0f, 0.0f};
    leftTop.x = rect.left;
    leftTop.y = rect.top;
    rightBottom.x = rect.right;
    rightBottom.y = rect.bottom;
    PSMTXMultVec(self->area->pane->mGlbMtx.m, &leftTop, &leftTop);
    PSMTXMultVec(self->area->pane->mGlbMtx.m, &rightBottom, &rightBottom);
    GXGetScissor(&scissorLeft, &scissorTop, &scissorWidth, &scissorHeight);
    scale = 640.0f / self->areaWidth;
    size.x = scale * (rightBottom.x - leftTop.x);
    center = leftTop.x + self->areaWidth * 0.5f;
    pos.x = center + center * (scale - 1.0f);
    GXSetScissor(pos.x, scissorTop, size.x, scissorHeight);
    self->window->pane->Draw(*drawInfo);
    if (!self->scroller.isMoving()) {
        GXSetScissor(scissorLeft, scissorTop, size.x + pos.x - scissorLeft, scissorHeight);
        for (i = 0; i < 20; i++) {
            if (i == selected || self->highlights[i]->getState() == 2) {
                self->texts[i]->pane->SetVisible(true);
                self->texts[i]->pane->Draw(*drawInfo);
            }
        }
    }
    GXSetScissor(scissorLeft, scissorTop, scissorWidth, scissorHeight);
}

/* The first word after `index` that does not fit when the window starts at `index`, minus one; -1 at the end. */
extern "C" s32 hbmKbCandNextWindow(HbmKbCandidateList* self, s32 index) {
    f32 areaWidth;
    f32 margin = 20.0f;
    f32 x = -margin;
    areaWidth = self->area->pane->mSize.width;
    HbmKbTextPane* text = self->texts[0]->pane;
    s32 i = index;
    while (x <= areaWidth) {
        if (i == self->count) {
            return -1;
        }
        f32 width = hbmKbTextWidth(text, self->words->words[i]);
        x += width + margin;
        i++;
    }
    if (i - 1 == index) {
        i++;
    }
    return i - 1;
}

/* The first word of the window that ends just before `index`. */
extern "C" s32 hbmKbCandPrevWindow(HbmKbCandidateList* self, s32 index) {
    f32 areaWidth;
    f32 margin = 20.0f;
    f32 x = -margin;
    areaWidth = self->area->pane->mSize.width;
    HbmKbTextPane* text = self->texts[0]->pane;
    s32 i = index - 1;
    while (x <= areaWidth) {
        if (i < 0) {
            return 0;
        }
        f32 width = hbmKbTextWidth(text, self->words->words[i]);
        x += width + margin;
        i--;
    }
    if (i + 2 == index) {
        i--;
    }
    return i + 2;
}

/* Scrolls the list by `delta` words from its first word. */
extern "C" void hbmKbCandScrollBy(HbmKbCandidateList* self, s32 delta) {
    f32 distance = 0.0f;
    f32 margin = 20.0f;
    if (delta < 0) {
        HbmKbTextPane* text = self->texts[0]->pane;
        for (s32 i = 0; i > delta; i--) {
            f32 width = hbmKbTextWidth(text, self->words->words[i + self->first - 1]);
            distance -= width + margin;
        }
    } else {
        HbmKbTextPane* text = self->texts[0]->pane;
        for (s32 i = 0; i < delta; i++) {
            f32 width = hbmKbTextWidth(text, self->words->words[self->first + i]);
            distance += width + margin;
        }
    }
    self->scroller.start(self, 0.0f, -distance, delta, 15.0f);
}

/* Scrolls the list so that its window starts at word `target`. */
extern "C" void hbmKbCandScrollTo(HbmKbCandidateList* self, s32 target) {
    f32 distance = 0.0f;
    f32 margin = 20.0f;
    s32 first = self->first;
    if (target > first) {
        HbmKbTextPane* text = self->texts[0]->pane;
        for (s32 i = first; i < target; i++) {
            f32 width = hbmKbTextWidth(text, self->words->words[i]);
            distance -= width + margin;
        }
    } else if (target < first) {
        HbmKbTextPane* text = self->texts[0]->pane;
        for (s32 i = first - 1; i >= target; i--) {
            f32 width = hbmKbTextWidth(text, self->words->words[i]);
            distance += width + margin;
        }
    }
    s32 words = target - self->first;
    self->scroller.start(self, self->scroll, distance, words, 15.0f);
}

/* Scrolls the list just far enough that word `target` is in its window. */
extern "C" void hbmKbCandShowWord(HbmKbCandidateList* self, s32 target) {
    s32 first = self->first;
    if (target > first) {
        for (;;) {
            s32 next = hbmKbCandNextWindow(self, first);
            if (next < 0 || next > target) {
                break;
            }
            first = next;
        }
        hbmKbCandScrollTo(self, first);
    } else if (target < first) {
        while (target < first) {
            first = hbmKbCandPrevWindow(self, first);
        }
        hbmKbCandScrollTo(self, first);
    }
}

/* Dispatches the base subobject at +0x18 with the caller's argument. */
extern "C" void fn_80553670(HkbWidget* self, u32 arg) {
    ((HkbWidget*)&self->table_18)->v_0x014(arg);
}

/* Brings the selected word into the window when the list is at rest. */
extern "C" void hbmKbCandShowSelected(HbmKbCandidateList* self) {
    if (self->selected >= 0 && !self->scroller.isMoving()) {
        if (self->first > self->selected) {
            hbmKbCandShowWord(self, self->selected);
        } else if (self->lastWhole < self->selected) {
            hbmKbCandShowWord(self, self->selected);
        }
    }
}

/* The end of a scroll (scroll-listener slot): resets the highlights, moves the window by `delta` words (clamped at
 * the first word) and tells the listener. */
extern "C" void hbmKbCandOnScrolled(HbmKbCandidateList* self, s32 kind, s32 delta) {
    if (kind == 1) {
        s32 first = self->first + delta;
        if (first < 0) {
            hbmKbResetHighlights(self);
            self->first = 0;
            self->scroll = 0.0f;
            self->pressed = -1;
        } else {
            hbmKbResetHighlights(self);
            self->first = first;
            self->scroll = 0.0f;
            self->pressed = -1;
        }
        if (self->listener) {
            self->listener->v_0x008(self, 0x100, 0);
        }
    }
}

/* Input handler of the candidate list: a key press commits the selected word, touching the area or a word frame
 * selects and reports it, releasing the area or the pressed word lets it go - while the widget takes input. */
extern "C" void hbmKbCandHandleInput(HbmKbCandidateList* self, HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input) {
    s32 i;
    if ((!self->widget->isBusy() || event == 1 || (s32)event == 0) &&
        (!hbmKbLayout(self->widget)->getPaneAnim("W_predictWindow")->v_0x018() || event == 1) &&
        !self->widget->isLocked()) {
        switch ((s32)event) {
        case 4:
            if (input->flags == 0x10) {
                s32 slot = hbmKbCandidateSlot(self, self->selected);
                if (self->pressed >= 0 && slot >= 0) {
                    self->texts[slot]->replay();
                    self->frames[slot]->replay();
                    self->highlights[slot]->setMode(0);
                    if (self->listener) {
                        self->listener->v_0x008(self, 0x101, 0);
                    }
                }
            }
            break;
        case 0:
            if (sender == self->area) {
                self->touched = 1;
                for (i = 0; i < 20; i++) {
                    self->texts[i]->replay();
                    self->frames[i]->replay();
                }
                hbmKbCandSelect(self, -1);
            } else if (self->pressed < 0 && self->touched) {
                for (i = 0; i < 20; i++) {
                    if (self->frames[i] == (HbmKbWordPartView*)sender) {
                        s32 index = self->first + i;
                        while (index < self->shownFirst) {
                            index += 20;
                        }
                        while (index > self->shownLast) {
                            index -= 20;
                        }
                        if (self->listener) {
                            self->listener->v_0x008(self, 0x102, index);
                        }
                        self->highlights[i]->setMode(1);
                        if (self->listener) {
                            self->listener->v_0x008(self, 0, 4);
                        }
                        break;
                    }
                }
            }
            break;
        case 1:
            if (sender == self->area) {
                self->touched = 0;
                for (i = 0; i < 20; i++) {
                    self->highlights[i]->setMode(2);
                }
            }
            if (self->pressed >= 0) {
                self->pressed = -1;
                if (self->listener) {
                    self->listener->v_0x008(self, 0x102, -1);
                }
                for (i = 0; i < 20; i++) {
                    self->texts[i]->replay();
                    self->frames[i]->replay();
                    self->highlights[i]->setMode(2);
                }
            }
            break;
        }
    }
}

/* 0x80553F50 (0x34): Builds the prediction window's handler with no pane bound. */
HbmKbWindowHandler::HbmKbWindowHandler(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener)
    : HbmKbHandlerBase(handlerId, pWidget, pListener), keyComponent(NULL), part(NULL) {}

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

/* 0x80553FC8 (0x30): Builds a scroll button's handler with no pane bound. */
HbmKbButtonHandler::HbmKbButtonHandler(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener)
    : HbmKbHandlerBase(handlerId, pWidget, pListener), keyComponent(NULL), baseComponent(NULL), part(NULL) {}

/* Input handler of a prediction-window button: a release or a key press on it pages the candidates, a touch and a
 * release replay its part, while the widget takes input. */
extern "C" void hbmKbButtonHandleInput(HbmKbButtonHandler* self, HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input) {
    if ((!self->widget->isBusy() || event == 1 || (s32)event == 0) &&
        (!hbmKbLayout(self->widget)->getPaneAnim("W_predictWindow")->v_0x018() || event == 1) &&
        !self->widget->isLocked()) {
        switch ((s32)event) {
        case 4:
            if (input->flags == 0x10) {
                self->part->setFrame(0);
                if (self->id == 1) {
                    self->widget->pageNext();
                } else if (self->id == 2) {
                    self->widget->pagePrevious();
                }
            }
            break;
        case 2:
            if (input->state == 0x10 && !(input->flags & 0x10) && sender->getKeyAt(input->position) % 20 != 0) {
                self->part->setFrame(0);
                if (self->id == 1) {
                    self->widget->pageNext();
                } else if (self->id == 2) {
                    self->widget->pagePrevious();
                }
            }
            break;
        case 1:
            self->part->setFrame(2);
            break;
        case 0:
            self->part->setFrame(1);
            if (self->listener) {
                self->listener->v_0x008(self, 0, 4);
            }
            break;
        }
    }
}

/* The u16 element at index `index` of the array at +0x80. */
extern "C" u16 fn_80554268(HkbU16Array* self, u32 index) {
    return self->values_80[index];
}

/* 0x80554278 (0x44): Builds the on/off switch's handler with no pane bound. */
HbmKbSwitchHandler::HbmKbSwitchHandler(u32 handlerId, HbmKbWidget* pWidget, HkbWidget* pListener)
    : HbmKbHandlerBase(handlerId, pWidget, pListener), onKey(NULL), source(NULL), offKey(NULL), offBase(NULL),
      onButton(NULL), offButton(NULL) {}

/* Event slot of the prediction switch (listener base): a trigger-7 release on the on or the off button reports
 * 0x301 or 0x300 to the listener. */
extern "C" void hbmKbSwitchOnPartEvent(HbmKbSwitchHandler* self, s32 kind, HbmKbPartView* sender, const HbmKbInputEvent* input) {
    if (sender == self->onButton) {
        if (input->trigger == 7 && kind == 1 && self->listener) {
            self->listener->v_0x008(self, 0x301, 0);
        }
    } else if (sender == self->offButton) {
        if (input->trigger == 7 && kind == 1 && self->listener) {
            self->listener->v_0x008(self, 0x300, 0);
        }
    }
}

/* Input handler of the prediction switch: a key press toggles the switch, a touch and a release replay the button
 * the event came from, while the widget takes input. */
extern "C" void hbmKbSwitchHandleInput(HbmKbSwitchHandler* self, HbmKbPartView* sender, u32 event, const HbmKbInputEvent* input) {
    if ((!self->widget->isBusy() || event == 1 || (s32)event == 0) &&
        (!hbmKbLayout(self->widget)->getPaneAnim("W_predictWindow")->v_0x018() || event == 1)) {
        HbmKbPartView* button = sender == self->source ? self->onButton : self->offButton;
        switch ((s32)event) {
        case 4:
            if (input->flags & 0x10) {
                self->widget->setSwitch(1);
            }
            break;
        case 1:
            button->setFrame(2);
            break;
        case 0:
            button->setFrame(1);
            if (self->listener) {
                self->listener->v_0x008(self, 0, 4);
            }
            break;
        }
    }
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

/* Destroys the part (the inline base destructors run here). */
HbmKbWidgetPartA::~HbmKbWidgetPartA() {
}


/* Destroys the part (the inline base destructors run here). */
HbmKbWidgetPartB::~HbmKbWidgetPartB() {
}


/* Destroys the part (the inline base destructors run here). */
HbmKbWidgetPartC::~HbmKbWidgetPartC() {
}

/* Adjustor thunk: base #3 -> hbmKbCandOnScrolled. */
extern "C" void fn_80554664(HkbWidget* self, s32 kind, s32 delta) {
    hbmKbCandOnScrolled((HbmKbCandidateList*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)), kind, delta);
}

/* Adjustor thunk: base #3 -> hbmKbSwitchOnPartEvent. */
extern "C" void fn_8055466C(HkbWidget* self, s32 kind, HbmKbPartView* sender, const HbmKbInputEvent* input) {
    hbmKbSwitchOnPartEvent((HbmKbSwitchHandler*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)), kind, sender, input);
}

/* Adjustor thunk: base #3 -> fn_80553F84. */
extern "C" void fn_80554674(HkbWidget* self, s32 arg1, u32 arg2, HkbFlagRecord* rec) {
    fn_80553F84((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_14)), arg1, arg2, rec);
}

/* Adjustor thunk: base #5 -> hbmKbWidgetDestroy. */
extern "C" void fn_8055467C(HkbWidget* self, s32 flag) {
    hbmKbWidgetDestroy((HbmKbPredictWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)), flag);
}

/* Adjustor thunk: base #5 -> fn_80551F08. */
extern "C" void fn_80554684(HkbWidget* self) {
    fn_80551F08((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> fn_80551EB0. */
extern "C" void fn_8055468C(HkbWidget* self) {
    fn_80551EB0((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> hbmKbWidgetUpdateCandidates. */
extern "C" void fn_80554694(HkbWidget* self) {
    hbmKbWidgetUpdateCandidates((HbmKbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> hbmKbWidgetDrawCandidates. */
extern "C" void fn_8055469C(HkbWidget* self) {
    hbmKbWidgetDrawCandidates((HbmKbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #5 -> hbmKbWidgetShowPredictInput. */
extern "C" void fn_805546A4(HkbWidget* self) {
    hbmKbWidgetShowPredictInput((HbmKbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_17F4)));
}

/* Adjustor thunk: base #6 -> hbmKbWidgetHandleEvent. */
extern "C" void fn_805546AC(HkbWidget* self, u32 sender, s32 event, s32 arg) {
    hbmKbWidgetHandleEvent((HbmKbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_189C)), sender, event, arg);
}

/* The candidate words' text panes (.data 0x8064FC10) and frame buttons (.data 0x8064FDA0). */
HbmKbPaneNames hbmKbWidgetTextNames = {{
    "T_prdc_Text_00", "T_prdc_Text_01", "T_prdc_Text_02", "T_prdc_Text_03", "T_prdc_Text_04",
    "T_prdc_Text_05", "T_prdc_Text_06", "T_prdc_Text_07", "T_prdc_Text_08", "T_prdc_Text_09",
    "T_prdc_Text_10", "T_prdc_Text_11", "T_prdc_Text_12", "T_prdc_Text_13", "T_prdc_Text_14",
    "T_prdc_Text_15", "T_prdc_Text_16", "T_prdc_Text_17", "T_prdc_Text_18", "T_prdc_Text_19",
}};
HbmKbPaneNames hbmKbWidgetFrameNames = {{
    "B_prdc_Text_00", "B_prdc_Text_01", "B_prdc_Text_02", "B_prdc_Text_03", "B_prdc_Text_04",
    "B_prdc_Text_05", "B_prdc_Text_06", "B_prdc_Text_07", "B_prdc_Text_08", "B_prdc_Text_09",
    "B_prdc_Text_10", "B_prdc_Text_11", "B_prdc_Text_12", "B_prdc_Text_13", "B_prdc_Text_14",
    "B_prdc_Text_15", "B_prdc_Text_16", "B_prdc_Text_17", "B_prdc_Text_18", "B_prdc_Text_19",
}};
