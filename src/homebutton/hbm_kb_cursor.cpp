/*
 * homebutton/hbm_kb_cursor.cpp - the memo keyboard of the tiHKBManager band: its overrides of the text panel's slots
 * (message, touch and scroll handlers, cursor up/down, input, reset, calc, update, hit test, the memo pane drawing), its
 * pane handler, the key-slot block and the memo keys, the parts' destructors and the adjustor thunks.
 *
 * RANGE.     .text 0x80566440..0x80569DAC (86 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the keyboard cursor; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS function names hbmKbCursor* (read off the bodies and the slot each fills in the class table
 *            0x80657F10, named after the text panel's function in the same slot: CursorUp/Down, HandleMessage, Input,
 *            Reset, Calc, Update, HitTest, ...) and view classes HbmCursor*View (slots named by offset), records
 *            HbmCursorInput / HbmCursorAnchor / HbmCursorPaneHandler / HbmCursorPaneComponent / HbmCursorPaneTrigger /
 *            HbmCursorPointMsg / HbmCursorLineMsg / HbmCursorPoint(3); GUESS data hbmKbCursorPaneNames (the memo's pane
 *            names), hbmKbCursorResetFlag / hbmKbCursorResetLimit (the pair slot 0x10C stores), hbmKbCursorRestOffset.
 * NAMES.     GUESS registry name hbmNodeReset (0x8054F788, hbm_text_panel.cpp).
 * SHAPES.    The views are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no
 *            method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * SHAPES.    fn_80568DD0 returns the anchor's Vec2f member by value (retail loads the two words straight into r3/r4).
 * SHAPES.    A pane is moved through hbmKbCursorPlacePane(pane, HbmCursorPoint(x, y)): retail builds the 2-float point
 *            and a 3-float translation on the stack and rounds the forwarded y (playbook 181); fn_80568004 negates the
 *            stored position, not the parameter, for the same reason.
 * SHAPES.    hbmKbCursorScrollToCaret halves the row height as `/ 2.0f` (retail's fmuls operand order) and tests the request
 *            as `(u32)(request - 1) > 1` (retail's subi + cmplwi); its 0.5 and the int-to-float bias are compiler pool
 *            entries, so they carry our pool's names, not 0x8079D76C/0x8079D758.
 * SHAPES.    The text block at +0x10 has its vptr at its own +0x4C (HbmCursorTextView derives from a 0x4C-byte head).
 * SHAPES.    The empty override at 0x80568CA8 (slot 0x16C) is its own row, hbmKbCursorIgnorePositionChange.
 * SHAPES.    hbmKbCursorRestOffset is zero yet lives in .sdata: `#pragma explicit_zero_data` around its definition.
 * RESIDUALS. Unwritten, store class-table pointers of classes no unit models yet (rule 10): hbmKbCursorDestroy
 *            0x80566440 (0xB0, the keyboard's four tables), hbmKbCursorCreate 0x80566C94 (0x370, builds the text buffer of
 *            table 0x806515D0, the pane handler 0x80657EF0 and the key-slot block 0x80657E50), hbmKbCursorSlotsCreate
 *            0x8056725C (0x268, builds group holders of table 0x80651830 and HbmKbCursorKey objects of 0x2C bytes).
 * RESIDUALS. hbmKbCursorCreateMemoKeys hoists the component list's address into a saved register; retail recomputes it
 *            at the append (and keeps `this`/the allocator in r25/r26).
 * RESIDUALS. hbmKbCursorUpdate: the text box button's half-height difference multiplies as `0.5 * d` against retail's
 *            `d * 0.5`, and the button origin's two stack copies sit at other offsets (29 of 631 words).
 * RESIDUALS. hbmKbCursorHandleTouchMessage: stack offsets of the returned origin and the point/line messages differ
 *            (every declaration order measured 99.90 % at best); hbmKbCursorDrawFooter: f0/f1 swapped around the
 *            `-1.0f * height` product.
 * RESIDUALS. Data: the target's .data range 0x80657BA8..0x80658200 spans three TUs (datagap seams at 0x80657E68 and
 *            0x80657EF0); the unit still declares lbl_80794728 / lbl_80794738 / lbl_80657E14 / lbl_8079D750..D784 extern.
 * NAMES.     GUESS classes HbmKbCursorPart, its kinds HbmKbCursorPartA/B and HbmKbCursorKey (band keys with a state
 *            word and a kind; members reset, setState, getType, getState).
 * NAMES.     GUESS hbmKbCursorCreateMemoKeys / hbmKbCursorNewLine and data hbmKbCursorMemoAnimDefs /
 *            hbmKbCursorMemoKeyDefs (the `my_Memo_a_*.brlan` animations of the N_Memo and Nigaoe keys).
 */

#include "types.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_kb_object.h"
#include "homebutton/hbm_kb_cursor.h"
#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_textbox.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "homebutton/hbmKbWidgetHandleMessage.h"
#include "homebutton/hbmPanelAdvanceWord1A00.h"
#include "MTX/mtxvec.h"
#include "nw4r/ut_Rect.h"
#include "homebutton/hbmNodeReset.h"
#include "homebutton/hbmPanelHitTest.h"
#include "homebutton/hbmTextPaneListenerOnEvent.h"
#include "homebutton/hbm_hermite.h"
#include "homebutton/hbmPanelSetItemCaret.h"

/* A cursor part: a band key with its state word; its class table (0x80657EC0) and the out-of-line copy of its inline destructor (0x80567204) are
 * this unit's.  size: 0x34 */
class HbmKbCursorPart : public HbmKey {
public:
    HbmKbCursorPart(nw4r::lyt::Pane* pane) : HbmKey(pane) {}

    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmKbCursorPart() {}
    virtual u32 getType();
    virtual u32 getState();

    /* +0x30 */ u32 mType;
}; /* size: 0x34 */

/* The memo's portrait key (kind 1; class table 0x80657E68).  size: 0x34 */
class HbmKbCursorPartA : public HbmKbCursorPart {
public:
    HbmKbCursorPartA(nw4r::lyt::Pane* pane) : HbmKbCursorPart(pane) { mType = 1; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbCursorPartA();
}; /* size: 0x34 */

/* The memo's body key (kind 0; class table 0x80657E94).  size: 0x34 */
class HbmKbCursorPartB : public HbmKbCursorPart {
public:
    HbmKbCursorPartB(nw4r::lyt::Pane* pane) : HbmKbCursorPart(pane) { mType = 0; }

    virtual void setAnim(s32 anim);
    virtual ~HbmKbCursorPartB();
}; /* size: 0x34 */

/* A memo key of the keyboard: its kind, its pane name and its animations.  size: 0x3C */
struct HbmKbCursorKeyDef {
    /* +0x00 */ u32 type;
    /* +0x04 */ const char* name;
    /* +0x08 */ u32 numAnims;
    /* +0x0C */ const HkbAnimDef* anims[12];
}; /* size: 0x3C */

/* The memo keys' animations (.rodata 0x8057C258). */
const HkbAnimDef hbmKbCursorMemoAnimDefs[6] = {
    {0, "my_Memo_a_normal.brlan"},
    {1, "my_Memo_a_MailIn.brlan"},
    {2, "my_Memo_a_MailOut.brlan"},
    {3, "my_Memo_a_SendOut.brlan"},
    {4, "my_Memo_a_NigaoeFoucusIn.brlan"},
    {5, "my_Memo_a_NigaoeFoucusOut.brlan"},
};

/* The memo's body and portrait keys (.rodata 0x8057C3F0). */
const HbmKbCursorKeyDef hbmKbCursorMemoKeyDefs[2] = {
    {0, "N_Memo", 4,
     {&hbmKbCursorMemoAnimDefs[0], &hbmKbCursorMemoAnimDefs[1], &hbmKbCursorMemoAnimDefs[2],
      &hbmKbCursorMemoAnimDefs[3]}},
    {1, "Nigaoe", 2, {&hbmKbCursorMemoAnimDefs[4], &hbmKbCursorMemoAnimDefs[5]}},
};

/* A cursor key: a band key whose reset and state setter are defined above as C functions (class table
 * 0x80657E2C).  size: 0x30 */
class HbmKbCursorKey : public HbmKey {
public:
    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmKbCursorKey();
}; /* size: 0x30 */



/* The compiler's own operator delete: the retail object jumps to `__dl__FPv`. */
void operator delete(void* ptr) throw();

/* Character/keystroke-name strings the keyboard searches its layout with (the two `.sdata` tables at
 * 0x80794728/0x80794738) and the pane name `T_Header` the memo button is looked up by. */
extern "C" {

extern char lbl_80794728[8];

extern char lbl_80794738[8];

extern char lbl_80657E14[];

}

/* The unit's own `.sdata2` pool: another split owns it, so it is declared, never defined
 * (playbook 29). */
extern "C" {

extern f32 lbl_8079D750;

extern f32 lbl_8079D754;

extern f32 lbl_8079D774;

extern f32 lbl_8079D784;

}


/* The dispatch views of the band objects this unit calls through their vtables: the slots are named by
 * offset (v_0xNNN), declared as virtuals so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)`
 * dispatch.  No method is defined and nothing is constructed here, so no table is emitted (rule 10). */

/* The base sub-object at +0x1A04: slots 0x2C and 0x5C look a name up and return the match, 0x40 selects it.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorKeyView;

class HbmCursorTailView {
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
    /* slot +0x02C */ virtual HbmCursorTailView* v_0x02C(char* name);
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040(char* name, s32 flag);
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual HbmCursorKeyView* v_0x05C(char* name);
};

/* A key pane found by name (HbmCursorTailView slot 0x5C): slot 0x14 sets its state, slot 0x18 activates it.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorKeyView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 state);
    /* slot +0x018 */ virtual void v_0x018();
};

/* The parts at +0x1BD4 and +0x1BE0 (KbTail +0x1D0/+0x1DC): slot 0x10 deactivates, 0x14 sets a pair, 0x18 activates.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorPartView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 index, s32 mode);
    /* slot +0x018 */ virtual void v_0x018();
};

/* The same part when slot 0x10 is handed an argument (fn_80569970).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorPartArgView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010(s32 flag);
};

/* The layout behind KbTail +0x004 -> +0x10: slot 0x3C finds a pane by name, the pane's slot 0x7C sets its text.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorPaneView {
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
    /* slot +0x03C */ virtual HbmCursorPaneView* v_0x03C(const char* name, s32 recursive);
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
    /* slot +0x07C */ virtual void v_0x07C(const u16* text, u16 index);
};

/* The keyboard object itself: slot 0x2A4 returns its current frame.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorSelfView {
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
    /* slot +0x10C */ virtual void v_0x10C(u8* flag, u32* value);
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
    /* slot +0x17C */ virtual void v_0x17C(s32 event);
    /* slot +0x180 */ virtual void v_0x180();
    /* slot +0x184 */ virtual u32 v_0x184(f32 x, f32 y);
    /* slot +0x188 */ virtual Vec2f v_0x188();
    /* slot +0x18C */ virtual Vec2f v_0x18C();
    /* slot +0x190 */ virtual void v_0x190();
    /* slot +0x194 */ virtual void v_0x194();
    /* slot +0x198 */ virtual void v_0x198();
    /* slot +0x19C */ virtual void v_0x19C();
    /* slot +0x1A0 */ virtual void v_0x1A0();
    /* slot +0x1A4 */ virtual void v_0x1A4();
    /* slot +0x1A8 */ virtual void v_0x1A8();
    /* slot +0x1AC */ virtual void v_0x1AC();
    /* slot +0x1B0 */ virtual void v_0x1B0();
    /* slot +0x1B4 */ virtual void v_0x1B4();
    /* slot +0x1B8 */ virtual void v_0x1B8();
    /* slot +0x1BC */ virtual void v_0x1BC();
    /* slot +0x1C0 */ virtual void v_0x1C0();
    /* slot +0x1C4 */ virtual void v_0x1C4();
    /* slot +0x1C8 */ virtual void v_0x1C8();
    /* slot +0x1CC */ virtual void v_0x1CC();
    /* slot +0x1D0 */ virtual void v_0x1D0();
    /* slot +0x1D4 */ virtual void v_0x1D4();
    /* slot +0x1D8 */ virtual void v_0x1D8();
    /* slot +0x1DC */ virtual void v_0x1DC();
    /* slot +0x1E0 */ virtual void v_0x1E0();
    /* slot +0x1E4 */ virtual void v_0x1E4();
    /* slot +0x1E8 */ virtual void v_0x1E8();
    /* slot +0x1EC */ virtual void v_0x1EC();
    /* slot +0x1F0 */ virtual void v_0x1F0();
    /* slot +0x1F4 */ virtual void v_0x1F4();
    /* slot +0x1F8 */ virtual void v_0x1F8();
    /* slot +0x1FC */ virtual void v_0x1FC();
    /* slot +0x200 */ virtual void v_0x200();
    /* slot +0x204 */ virtual void v_0x204();
    /* slot +0x208 */ virtual void v_0x208();
    /* slot +0x20C */ virtual void v_0x20C();
    /* slot +0x210 */ virtual void v_0x210();
    /* slot +0x214 */ virtual void v_0x214();
    /* slot +0x218 */ virtual void v_0x218();
    /* slot +0x21C */ virtual void v_0x21C();
    /* slot +0x220 */ virtual void v_0x220();
    /* slot +0x224 */ virtual void v_0x224();
    /* slot +0x228 */ virtual void v_0x228();
    /* slot +0x22C */ virtual void v_0x22C();
    /* slot +0x230 */ virtual void v_0x230();
    /* slot +0x234 */ virtual void v_0x234();
    /* slot +0x238 */ virtual void v_0x238();
    /* slot +0x23C */ virtual void v_0x23C();
    /* slot +0x240 */ virtual void v_0x240();
    /* slot +0x244 */ virtual void v_0x244(f32 frame);
    /* slot +0x248 */ virtual void v_0x248();
    /* slot +0x24C */ virtual void v_0x24C();
    /* slot +0x250 */ virtual void v_0x250();
    /* slot +0x254 */ virtual void v_0x254();
    /* slot +0x258 */ virtual void v_0x258();
    /* slot +0x25C */ virtual void v_0x25C();
    /* slot +0x260 */ virtual void v_0x260();
    /* slot +0x264 */ virtual void v_0x264();
    /* slot +0x268 */ virtual void v_0x268();
    /* slot +0x26C */ virtual void v_0x26C();
    /* slot +0x270 */ virtual void v_0x270();
    /* slot +0x274 */ virtual void v_0x274();
    /* slot +0x278 */ virtual void v_0x278();
    /* slot +0x27C */ virtual void v_0x27C();
    /* slot +0x280 */ virtual void v_0x280();
    /* slot +0x284 */ virtual void v_0x284();
    /* slot +0x288 */ virtual void v_0x288();
    /* slot +0x28C */ virtual void v_0x28C();
    /* slot +0x290 */ virtual void v_0x290();
    /* slot +0x294 */ virtual void v_0x294();
    /* slot +0x298 */ virtual void v_0x298();
    /* slot +0x29C */ virtual void v_0x29C();
    /* slot +0x2A0 */ virtual nw4r::lyt::Material* v_0x2A0();
    /* slot +0x2A4 */ virtual f32 v_0x2A4();
    /* slot +0x2A8 */ virtual f32 v_0x2A8();
    /* slot +0x2AC */ virtual f32 v_0x2AC();
    /* slot +0x2B0 */ virtual void v_0x2B0();
    /* slot +0x2B4 */ virtual void v_0x2B4();
    /* slot +0x2B8 */ virtual void v_0x2B8();
    /* slot +0x2BC */ virtual void v_0x2BC();
    /* slot +0x2C0 */ virtual void v_0x2C0();
    /* slot +0x2C4 */ virtual void v_0x2C4();
    /* slot +0x2C8 */ virtual void v_0x2C8(s32 id, u32* arg);
    /* slot +0x2CC */ virtual void v_0x2CC(s32 id, u32* arg);
    /* slot +0x2D0 */ virtual void v_0x2D0();
};

/* The popup at +0x1BCC (KbTail +0x1C8): slot 0x08 says whether it is open, 0x0C opens and 0x10 closes it.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorPopupView {
public:
    /* slot +0x008 */ virtual s32 v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
};

/* The text block at +0x10 has its vptr at its own +0x4C: these are the bytes in front of it.  size: 0x4C */
struct HbmCursorTextHead {
    /* +0x00 */ u8 unused_0x00[0x4C];
}; /* size: 0x4C */

/* The text block at +0x10: slot 0x28 returns the row height.  size: 0x50 (approximation: only the vptr is touched) */
class HbmCursorTextView : public HbmCursorTextHead {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual f32 v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual s32 v_0x034();
};

/* The scroller at +0x1BAC as the scroll handlers use it: slot 0x08 starts a scroll from one position to another over a
 * number of frames, slot 0x14 says whether one is running.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorScrollerView {
public:
    /* slot +0x008 */ virtual void v_0x008(s32 a, s32 b, f32 from, f32 to, f32 frames);
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual f32 v_0x010();
    /* slot +0x014 */ virtual s32 v_0x014();
};

/* The part at +0x1BAC (KbTail +0x1A8): slot 0x14 takes the caller's arguments unchanged.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorScrollView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 index, s32 mode);
};

/* A key slot of the cursor's nine-entry table at +0x10: slot 0x10 updates it, slot 0x18 selects it with a key.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorSlotView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018(u32 key);
};

/* The text field at +0x193C: slot 0x1C returns its length, slot 0x8C its selection.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorFieldView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual u32 v_0x01C();
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
    /* slot +0x08C */ virtual void v_0x08C(u32* start, u32* end);
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
};

/* The node at +0x118 that hbmNodeReset drives: slot 0x14 refreshes it.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorNodeView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
};

/* A point message (ids 14 and 24): the touched point.  size: 0x0C (approximation: +0x08 is read by id 24) */
struct HbmCursorPointMsg {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 delta;
}; /* size: 0x0C */

/* Message 37: the line the caret moves to.  size: 0x08 */
struct HbmCursorLineMsg {
    /* +0x00 */ s32 kind;
    /* +0x04 */ s32* line;
}; /* size: 0x08 */

/* The cursor's input block at the end of the keyboard object (KbTail +0x1A8..+0x1E8), as the arrow and key handlers
 * below read it, with the key-slot block at its start (+0x04 focus, +0x08 key, +0x10 slots, +0x34 shown flags, +0x3D/+0x3E
 * latches), the caret top (+0x1958), the scroll request (+0x1988), the scroll position (+0x1B04) and the row count
 * (+0x1B10).  size: 0x1BEC (approximation: the gaps are the rest of the keyboard) */
struct HbmCursorInput {
    /* +0x0000 */ u8 unused_0x0000[0x04];
    /* +0x0004 */ s32 focus;
    /* +0x0008 */ u32 key;
    /* +0x000C */ u8 unused_0x000C[0x04];
    /* +0x0010 */ HbmCursorSlotView* parts[9];
    /* +0x0034 */ u8 shown[9];
    /* +0x003D */ u8 leftLatch;
    /* +0x003E */ u8 rightLatch;
    /* +0x003F */ u8 unused_0x003F[0xC1];
    /* +0x0100 */ f32 textOffset;           /* the text writer's vertical offset (its +0xF0) */
    /* +0x0104 */ u8 unused_0x0104[0x14];
    /* +0x0118 */ u32 node_118;             /* the vptr of the HbmCursorNodeView at +0x118 */
    /* +0x011C */ u8 unused_0x011C[0x04];
    /* +0x0120 */ s32 nodeValue;            /* the node's word, cleared when a touch places the caret */
    /* +0x0124 */ u8 unused_0x0124[0x17D4];
    /* +0x18F8 */ nw4r::ut::Rect textRect;  /* the text box on the screen */
    /* +0x1908 */ nw4r::math::MTX34 textMtx;
    /* +0x1938 */ u8 unused_0x1938[0x04];
    /* +0x193C */ HbmCursorFieldView* textField;
    /* +0x1940 */ HbmCursorFieldView* candidateList;
    /* +0x1944 */ u8 unused_0x1944[0x10];
    /* +0x1954 */ f32 caretX;
    /* +0x1958 */ f32 caretTop;
    /* +0x195C */ u8 unused_0x195C[0x2C];
    /* +0x1988 */ s32 scrollRequest;
    /* +0x198C */ u8 unused_0x198C[0x0C];
    /* +0x1998 */ u32 maxLines;             /* the line limit */
    /* +0x199C */ u8 unused_0x199C[0x08];
    /* +0x19A4 */ u8 textAlpha;
    /* +0x19A5 */ u8 pad_0x19A5[0x03];
    /* +0x19A8 */ u8 textColorR;
    /* +0x19A9 */ u8 textColorG;
    /* +0x19AA */ u8 textColorB;
    /* +0x19AB */ u8 textColorA;
    /* +0x19AC */ u8 unused_0x19AC[0x58];
    /* +0x1A04 */ u32 paneManager_1A04;     /* the vptr of the keyboard's HbmPaneManager base */
    /* +0x1A08 */ u8 unused_0x1A08[0x14];
    /* +0x1A1C */ nw4r::lyt::DrawInfo drawInfo;     /* the pane manager's draw info */
    /* +0x1A70 */ u8 unused_0x1A70[0x44];
    /* +0x1AB4 */ u8 upArrowShown;
    /* +0x1AB5 */ u8 downArrowShown;
    /* +0x1AB6 */ u8 unused_0x1AB6[0x3E];
    /* +0x1AF4 */ nw4r::lyt::Pane* memoPane;
    /* +0x1AF8 */ nw4r::lyt::Pane* memoRootPane;
    /* +0x1AFC */ nw4r::lyt::Pane* textBoxPane;
    /* +0x1B00 */ nw4r::lyt::Pane* textBoxButtonPane;
    /* +0x1B04 */ f32 position;
    /* +0x1B08 */ f32 touchPosition;        /* the scroll position when the text box was touched */
    /* +0x1B0C */ f32 touchLineTop;         /* the touched line's top */
    /* +0x1B10 */ s32 rows;
    /* +0x1B14 */ nw4r::ut::Rect buttonRect;           /* the text box button on the screen */
    /* +0x1B24 */ nw4r::math::VEC3 buttonTranslate;    /* the button pane's laid-out translation */
    /* +0x1B30 */ f32 buttonWidth;
    /* +0x1B34 */ f32 buttonHeight;
    /* +0x1B38 */ nw4r::math::VEC3 textBoxTranslate;   /* the text box pane's laid-out translation */
    /* +0x1B44 */ f32 textBoxWidth;
    /* +0x1B48 */ f32 textBoxHeight;
    /* +0x1B4C */ nw4r::math::MTX34 buttonMtx;
    /* +0x1B7C */ nw4r::math::MTX34 textBoxMtx;
    /* +0x1BAC */ u32 scroll_1BAC;
    /* +0x1BB0 */ u8 unused_0x1BB0[0x1C];
    /* +0x1BCC */ HbmCursorPopupView* popup;
    /* +0x1BD0 */ GXTexObj* texObj;         /* the texture object the material's first map takes */
    /* +0x1BD4 */ u8 unused_0x1BD4[0x04];
    /* +0x1BD8 */ u8 touchLetter;
    /* +0x1BD9 */ u8 dirty;
    /* +0x1BDA */ u8 leftPressed;
    /* +0x1BDB */ u8 rightPressed;
    /* +0x1BDC */ u8 shifted;
    /* +0x1BDD */ u8 unused_0x1BDD[0x03];
    /* +0x1BE0 */ HbmCursorPartView* arrows;
    /* +0x1BE4 */ u8 leftHeld;
    /* +0x1BE5 */ u8 rightHeld;
    /* +0x1BE6 */ u8 unused_0x1BE6[0x02];
    /* +0x1BE8 */ s32 mode;

    /* The keyboard's pane manager base (+0x1A04). */
    HbmPaneManager* manager() { return (HbmPaneManager*)&paneManager_1A04; }
}; /* size: 0x1BEC */

/* A pane position as the pane placement takes it.  size: 0x08 */
struct HbmCursorPoint {
    HbmCursorPoint(f32 ax, f32 ay) : x(ax), y(ay) {}

    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
}; /* size: 0x08 */

/* A pane translation built from a position.  size: 0x0C */
struct HbmCursorPoint3 {
    HbmCursorPoint3(f32 ax, f32 ay, f32 az) : x(ax), y(ay), z(az) {}

    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
}; /* size: 0x0C */

/* Moves the pane to the position (z 0). */
static inline void hbmKbCursorPlacePane(nw4r::lyt::Pane* pane, const HbmCursorPoint& pos)
{
    HbmCursorPoint3 translate(pos.x, pos.y, 0.0f);

    pane->mTranslate.x = translate.x;
    pane->mTranslate.y = translate.y;
    pane->mTranslate.z = translate.z;
}

/* The keyboard's pane handler (class table 0x80657EF0): the text panel's listener with the keyboard at +0x0C and
 * +0x10.  size: 0x14 */
struct HbmCursorPaneHandler {
    /* +0x00 */ u8 unused_0x00[0x0C];       /* the vptr and the listener's manager and sound */
    /* +0x0C */ HbmCursorInput* widget;
    /* +0x10 */ HbmCursorInput* keyboard;
}; /* size: 0x14 */

/* A pane component the handler hears about: its pane at +0x9C.  size: 0xA0 (approximation: only +0x9C is read) */
struct HbmCursorPaneComponent {
    /* +0x00 */ u8 unused_0x00[0x9C];
    /* +0x9C */ nw4r::lyt::Pane* pane;
}; /* size: 0xA0 */

/* The pointer state a pane event carries.  size: 0x10 (approximation: +0x10.. is not read) */
struct HbmCursorPaneTrigger {
    /* +0x00 */ u32 channel;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
    /* +0x0C */ u32 trigger;
}; /* size: 0x10 */

/* The anchor object at KbTail +0x0F4: its position pair at +0x44 is returned by value.
 * size: 0x4C (approximation: only +0x44..+0x4C is touched) */
struct HbmCursorAnchor {
    /* +0x00 */ u8    unused_0x00[0x44];
    /* +0x44 */ Vec2f pos;
}; /* size: 0x4C */

/* The keyboard widget's input as hbm_text_panel.cpp's body takes it (its header declares one parameter). */
#define hbmKbWidgetInput_c1 ((void (*)(HbmCursorInput*, u32, u32, u32, u32, u32, f32, f32))hbmKbWidgetInput)

extern "C" {

/* The memo's panes, by role (.data 0x80657BF4): header, body, body button, footer, portrait, portrait caption, portrait
 * button, letter caption and touch-letter caption. */
const char* hbmKbCursorPaneNames[9] = {
    "N_Header", "N_Body", "B_Body", "N_Footer", "Nigaoe", "T_Nigaoe", "B_Nigaoe", "T_Letter", "T_TouchLetter",
};

/* 0x80566440 (0xB0): the keyboard's deleting destructor (slot 0x08); its body stores the class's four table
 * pointers, so it is left to the keyboard's class (RESIDUALS). */
HbmCursorInput* hbmKbCursorDestroy(HbmCursorInput* self, s32 flag);

/* The flag and limit the keyboard's reset hands to slot 0x10C (.sdata 0x8079472F / 0x80794730). */
u8 hbmKbCursorResetFlag = 1;
u32 hbmKbCursorResetLimit = 10;

/* The writer offset outside mode 2 (.sdata 0x80794734). */
#pragma explicit_zero_data on
f32 hbmKbCursorRestOffset = 0.0f;
#pragma explicit_zero_data off

/* 0x805664F0 (0x128): advances the frame by `delta` and plays the arrow parts' press (4) and release (3) animations
 * as the held state of each arrow changes. */
void hbmKbCursorUpdateArrows(HbmCursorInput* self, u8 left, u8 right, f32 delta)
{
    ((HbmCursorSelfView*)self)->v_0x244(((KbObject*)self)->tail_0x1A04.float_0x100 + delta);
    if (left != 0 && self->leftHeld == 0) {
        self->arrows->v_0x014(0, 4);
    } else if (right != 0 && self->rightHeld == 0) {
        self->arrows->v_0x014(1, 4);
    }
    if (left == 0 && self->leftHeld != 0) {
        self->arrows->v_0x014(0, 3);
    } else if (right == 0 && self->rightHeld != 0) {
        self->arrows->v_0x014(1, 3);
    }
    self->leftHeld = left;
    self->rightHeld = right;
}

/* 0x80566618 (0x128): moves the text field's caret one line up (to the start on the top line) and asks for a scroll to
 * the caret. */
void hbmKbCursorCursorUp(HbmCursorInput* self)
{
    f32 y = 1.0f + (self->caretTop - ((HbmCursorTextView*)&self->parts)->v_0x028());

    if (y <= 0.0f) {
        u32 start;
        u32 end;

        self->textField->v_0x08C(&start, &end);
        if (start == 0 && end == 0) {
            ((HbmCursorSelfView*)self)->v_0x17C(6);
        } else {
            ((HbmCursorSelfView*)self)->v_0x17C(5);
        }
        hbmPanelSetItemCaret((HbmPanelView*)self, (HbmPanelItemView*)self->textField, 0);
    } else {
        hbmPanelSetItemCaret((HbmPanelView*)self, (HbmPanelItemView*)self->textField,
                             ((HbmCursorSelfView*)self)->v_0x184(self->caretX, y - self->position));
        ((HbmCursorSelfView*)self)->v_0x17C(5);
    }
    self->scrollRequest = 1;
}

/* 0x80566740 (0x1D0): moves the text field's caret one line down (to the end on the last line) and asks for a scroll
 * to the caret. */
void hbmKbCursorCursorDown(HbmCursorInput* self)
{
    f32 lines;
    f32 y;

    y = 1.0f + (self->caretTop + ((HbmCursorTextView*)&self->parts)->v_0x028());
    lines = (f32)((HbmCursorTextView*)&self->parts)->v_0x034();

    if (y / ((HbmCursorTextView*)&self->parts)->v_0x028() >= lines) {
        u32 start;
        u32 end;

        self->textField->v_0x08C(&start, &end);
        if (start == (u16)self->textField->v_0x01C() && end == (u16)self->textField->v_0x01C()) {
            ((HbmCursorSelfView*)self)->v_0x17C(6);
        } else {
            ((HbmCursorSelfView*)self)->v_0x17C(5);
        }
        hbmPanelSetItemCaret((HbmPanelView*)self, (HbmPanelItemView*)self->textField, (u16)self->textField->v_0x01C());
    } else {
        hbmPanelSetItemCaret((HbmPanelView*)self, (HbmPanelItemView*)self->textField,
                             ((HbmCursorSelfView*)self)->v_0x184(self->caretX, y - self->position));
        ((HbmCursorSelfView*)self)->v_0x17C(5);
    }
    self->scrollRequest = 1;
}

/* 0x80566910 (0x60): the keyboard's message handler (slot 0x18): mode 2 and mode 1 have their own handlers; otherwise
 * only messages 6, 21, 29 and 31 reach the keyboard widget's handler. */
void hbmKbCursorHandleMessage(HbmCursorInput* self, s32 id, u32* arg)
{
    if (self->mode == 2) {
        ((HbmCursorSelfView*)self)->v_0x2CC(id, arg);
        return;
    }
    if (self->mode == 1) {
        ((HbmCursorSelfView*)self)->v_0x2C8(id, arg);
        return;
    }
    switch (id) {
    case 6:
    case 21:
    case 29:
    case 31:
        hbmKbWidgetHandleMessage((HbmPanelView*)self, id, arg);
        break;
    }
}

/* 0x80566970 (0x10C): the mode-2 message handler (slot 0x2CC): a drag (24) scrolls the text a row up or down unless a
 * scroll runs; every other message goes to the keyboard widget's handler. */
void hbmKbCursorHandleScrollMessage(HbmCursorInput* self, s32 id, u32* arg)
{
    switch (id) {
    case 24:
        if (((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x014() != 0)
            break;
        if (((HbmCursorPointMsg*)arg)->delta < 0.0f) {
            ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x008(
                0, 0, self->position, self->position + ((HbmCursorTextView*)&self->parts)->v_0x028(),
                hbmKbCursorScrollFrames);
        } else {
            ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x008(
                0, 0, self->position, self->position - ((HbmCursorTextView*)&self->parts)->v_0x028(),
                hbmKbCursorScrollFrames);
        }
        ((HbmCursorSelfView*)self)->v_0x17C(11);
        break;
    default:
        hbmKbWidgetHandleMessage((HbmPanelView*)self, id, arg);
        break;
    }
}

/* 0x80566A7C (0x218): the mode-1 message handler (slot 0x2C8): a touch (14) puts the caret on the touched line (not
 * the last one), passes the point on and moves the caret there; messages 29, 31 and 41 go to the keyboard widget's
 * handler. */
void hbmKbCursorHandleTouchMessage(HbmCursorInput* self, s32 id, u32* arg)
{
    switch (id) {
    case 14: {
        Vec2f result = ((HbmCursorSelfView*)self)->v_0x188();
        Vec2f origin = result;
        HbmCursorPointMsg point;
        HbmCursorLineMsg lineMsg;
        s32 caretLine;
        s32 line;

        line = (s32)((((HbmCursorPointMsg*)arg)->y - origin.y) / ((HbmCursorTextView*)&self->parts)->v_0x028());

        if (line > ((HbmCursorTextView*)&self->parts)->v_0x034() - 1)
            line = ((HbmCursorTextView*)&self->parts)->v_0x034() - 1;
        if (((HbmCursorTextView*)&self->parts)->v_0x034() > 1
            && line >= ((HbmCursorTextView*)&self->parts)->v_0x034() - 1)
            line = ((HbmCursorTextView*)&self->parts)->v_0x034() - 2;
        self->touchPosition = self->position;
        self->touchLineTop = (f32)line * ((HbmCursorTextView*)&self->parts)->v_0x028();
        point.x = ((HbmCursorPointMsg*)arg)->x;
        point.y = ((HbmCursorPointMsg*)arg)->y - self->position;
        hbmKbWidgetHandleMessage((HbmPanelView*)self, 14, (u32*)&point);
        caretLine = line;
        lineMsg.kind = 0;
        lineMsg.line = &caretLine;
        hbmKbWidgetHandleMessage((HbmPanelView*)self, 37, (u32*)&lineMsg);
        self->nodeValue = 0;
        self->touchLetter = 1;
        ((HbmCursorNodeView*)&self->node_118)->v_0x014();
        hbmNodeReset(&self->node_118);
        break;
    }
    case 30:
        break;
    case 29:
    case 31:
    case 41:
        hbmKbWidgetHandleMessage((HbmPanelView*)self, id, arg);
        break;
    }
}

/* 0x80567014 (0x1F0): Creates the memo's body and portrait keys with their animations and lists them in the pane
 * manager. */
void hbmKbCursorCreateMemoKeys(KbObject* self, MEMAllocator* allocator)
{
    for (u16 i = 0; i < 2; i++) {
        const HbmKbCursorKeyDef* def = &hbmKbCursorMemoKeyDefs[i];
        HbmKbCursorPart* pPart = NULL;
        switch (def->type) {
        case 0:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbCursorPartB)))
                HbmKbCursorPartB(((HbmPaneManager*)&self->tail_0x1A04)->findTextBox(def->name));
            break;
        case 1:
            pPart = new (MEMAllocFromAllocator(allocator, sizeof(HbmKbCursorPartA)))
                HbmKbCursorPartA(((HbmPaneManager*)&self->tail_0x1A04)->findTextBox(def->name));
            break;
        }
        nw4r::ut::List_Append(&((HbmPaneManager*)&self->tail_0x1A04)->mComponents, pPart);
        u32 numAnims = def->numAnims;
        for (u16 j = 0; j < numAnims; j++) {
            const void* res = ((HbmPaneManager*)&self->tail_0x1A04)->mpResAccessor->GetResource(0, def->anims[j]->name, NULL);
            nw4r::lyt::AnimTransform* anim = ((HbmPaneManager*)&self->tail_0x1A04)->getLayout()->CreateAnimTransform(res, ((HbmPaneManager*)&self->tail_0x1A04)->mpResAccessor);
            pPart->addAnim(allocator, def->anims[j]->kind, anim, 0, 1);
        }
    }
}

void* fn_80567004(KbObject* self, void* value)
{
    void* old = self->sub_0x04;
    self->sub_0x04 = (Wrapped*)value;
    return old;
}

/* 0x805674C4 (0x58): updates the nine parts at +0x10. */
void hbmKbCursorUpdateParts(HbmCursorInput* self)
{
    s32 i;

    for (i = 0; i < 9; i++)
        self->parts[i]->v_0x010();
}

/* 0x8056751C (0x1A8): records the key and moves the focus to the slot the key selects (the shifted and unshifted
 * layouts differ; keys 3/4 toggle a latch), then marks that slot shown and selects it with the key. */
void hbmKbCursorSelectKey(HbmCursorInput* self, s32 shifted, u32 key)
{
    self->key = key;
    if (shifted != 0) {
        switch (key) {
        case 0:
            self->focus = 0;
            break;
        case 1:
            self->focus = 5;
            break;
        case 2:
            self->focus = 5;
            break;
        case 3:
            if (self->leftLatch == 0)
                return;
            self->focus = 8;
            self->leftLatch = 0;
            break;
        case 4:
            if (self->leftLatch != 0)
                return;
            self->focus = 8;
            self->leftLatch = 1;
            break;
        case 5:
            self->focus = 0;
            break;
        case 6:
            self->focus = 4;
            break;
        case 7:
            self->focus = 2;
            break;
        case 8:
            self->focus = 2;
            break;
        }
    } else {
        switch (key) {
        case 0:
            self->focus = 0;
            break;
        case 1:
            self->focus = 6;
            break;
        case 2:
            self->focus = 6;
            break;
        case 3:
            if (self->rightLatch == 0)
                return;
            self->focus = 7;
            self->rightLatch = 0;
            break;
        case 4:
            if (self->rightLatch != 0)
                return;
            self->focus = 7;
            self->rightLatch = 1;
            break;
        case 5:
            self->focus = 0;
            break;
        case 6:
            self->focus = 3;
            break;
        case 7:
            self->focus = 1;
            break;
        case 8:
            self->focus = 1;
            break;
        }
    }
    self->shown[self->focus] = 1;
    self->parts[self->focus]->v_0x018(key);
}

/* 0x805676C4 (0x68): in mode 1, holds the left arrow (state 8) and posts event 24. */
void hbmKbCursorHoldLeftArrow(HbmCursorInput* self)
{
    if (self->mode != 1)
        return;
    self->arrows->v_0x014(0, 8);
    ((HbmCursorSelfView*)self)->v_0x17C(24);
}

/* ---------------------------------------------------------------------------------------------------
 * guarded forwarders: `if (self->tail_0x1A04.field_0x1E4 != 1) return; <wrapped>->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_8056772C(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((HbmCursorPartView*)self->tail_0x1A04.field_0x1DC)->v_0x014(0, 7);
}

/* 0x80567758 (0x68): in mode 1, holds the right arrow (state 8) and posts event 24. */
void hbmKbCursorHoldRightArrow(HbmCursorInput* self)
{
    if (self->mode != 1)
        return;
    self->arrows->v_0x014(1, 8);
    ((HbmCursorSelfView*)self)->v_0x17C(24);
}

/* 0x805677EC (0x114): in mode 1 and while no scroll runs, presses the up arrow and scrolls three rows up, not past the
 * top limit (own slot 0x2A8), then posts event 11. */
void hbmKbCursorScrollUp(HbmCursorInput* self)
{
    f32 target;

    if (self->mode != 1)
        return;
    if (((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x014() != 0)
        return;
    self->arrows->v_0x014(0, 6);
    target = self->position - hbmKbCursorScrollRows * ((HbmCursorTextView*)&self->parts)->v_0x028();
    if (target <= ((HbmCursorSelfView*)self)->v_0x2A8())
        target = ((HbmCursorSelfView*)self)->v_0x2A8();
    ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x008(0, 0, self->position, target, hbmKbCursorScrollFrames);
    ((HbmCursorSelfView*)self)->v_0x17C(11);
}

/* 0x80567900 (0x114): in mode 1 and while no scroll runs, presses the down arrow and scrolls three rows down, not
 * past the bottom limit (own slot 0x2AC), then posts event 11. */
void hbmKbCursorScrollDown(HbmCursorInput* self)
{
    f32 target;

    if (self->mode != 1)
        return;
    if (((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x014() != 0)
        return;
    self->arrows->v_0x014(1, 6);
    target = self->position + hbmKbCursorScrollRows * ((HbmCursorTextView*)&self->parts)->v_0x028();
    if (target >= ((HbmCursorSelfView*)self)->v_0x2AC())
        target = ((HbmCursorSelfView*)self)->v_0x2AC();
    ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x008(0, 0, self->position, target, hbmKbCursorScrollFrames);
    ((HbmCursorSelfView*)self)->v_0x17C(11);
}

void fn_805677C0(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((HbmCursorPartView*)self->tail_0x1A04.field_0x1DC)->v_0x014(1, 7);
}

/* 0x80567A14 (0x328): the keyboard's input (slot 0x224): in mode 1, a drag (held bit 0x40) goes to the widget alone;
 * the arrow keys press and release the arrows and, while held, scroll the text 3.5 units toward the limits (own slots
 * 0x2A8/0x2AC); then the keyboard widget's input runs. */
void hbmKbCursorInput(HbmCursorInput* self, u32 channel, u32 trigger, u32 held, u32 release, u32 repeat, f32 x, f32 y)
{
    if (self->mode == 1) {
        if (held & 0x40) {
            hbmKbWidgetInput_c1(self, channel, 0, 0, 0, repeat, 0.0f, 0.0f);
            return;
        }
        if (trigger & 1)
            self->arrows->v_0x014(0, 4);
        if (release & 1)
            self->arrows->v_0x014(0, 3);
        if (trigger & 0x4000)
            self->arrows->v_0x014(1, 4);
        if (release & 0x4000)
            self->arrows->v_0x014(1, 3);
        if (held & 1) {
            if (((HbmCursorSelfView*)self)->v_0x2A8() <= self->position) {
                ((HbmCursorSelfView*)self)->v_0x244(self->position - 3.5f);
                if (self->position <= ((HbmCursorSelfView*)self)->v_0x2A8()) {
                    ((HbmCursorSelfView*)self)->v_0x244(((HbmCursorSelfView*)self)->v_0x2A8());
                } else {
                    ((HbmCursorSelfView*)self)->v_0x17C(26);
                }
            }
            self->arrows->v_0x014(0, 4);
        } else {
            self->arrows->v_0x014(0, 3);
        }
        if (held & 0x4000) {
            if (((HbmCursorSelfView*)self)->v_0x2AC() >= self->position) {
                ((HbmCursorSelfView*)self)->v_0x244(3.5f + self->position);
                if (self->position >= ((HbmCursorSelfView*)self)->v_0x2AC()) {
                    ((HbmCursorSelfView*)self)->v_0x244(((HbmCursorSelfView*)self)->v_0x2AC());
                } else {
                    ((HbmCursorSelfView*)self)->v_0x17C(26);
                }
            }
            self->arrows->v_0x014(1, 4);
        } else {
            self->arrows->v_0x014(1, 3);
        }
    }
    hbmKbWidgetInput_c1(self, channel, trigger, held, release, repeat, x, y);
}

void fn_80567D3C(void* p, HkbManager* manager) { fn_8054E05C(p, manager); }

/* 0x80567D40 (0x130): the keyboard's reset (slot 0x10): the text colour back to dark grey, the scroll arrows hidden and
 * released, the scroll position and the touch record cleared, then the candidate list, the panel and the wrap setting
 * reset. */
void hbmKbCursorReset(HbmCursorInput* self)
{
    if (self->textBoxPane != 0) {
        self->textColorR = 20;
        self->textColorG = 20;
        self->textColorB = 20;
        self->textColorA = 255;
    }
    self->upArrowShown = 0;
    self->downArrowShown = 0;
    self->manager()->getPaneAnim("P_txtScrll_UP")->setState(7);
    self->manager()->getPaneAnim("P_txtScrll_DOWN")->setState(7);
    self->dirty = 0;
    self->leftPressed = 0;
    self->rightPressed = 0;
    ((HbmCursorSelfView*)self)->v_0x244(0.0f);
    self->touchPosition = 0.0f;
    self->touchLineTop = 0.0f;
    self->candidateList->v_0x130();
    ((HbmCursorSelfView*)self)->v_0x0D8();
    ((HbmCursorSelfView*)self)->v_0x10C(&hbmKbCursorResetFlag, &hbmKbCursorResetLimit);
}

/* 0x80567E70 (0x194): the keyboard's calc (slot 0x148): updates the pane manager and the keyboard's own parts, draws
 * the scroll bar (mode 2) or the top button (mode 1) and lays the text out; the row count is at least 4. */
void hbmKbCursorCalc(HbmCursorInput* self)
{
    self->manager()->v_0x044();
    ((HbmCursorSelfView*)self)->v_0x2BC();
    ((HbmCursorSelfView*)self)->v_0x2C0();
    ((HbmCursorSelfView*)self)->v_0x2C4();
    if (self->mode == 2)
        self->manager()->findTextBox("N_txt_scrl")->Draw(self->drawInfo);
    hbmPanelLayoutText(self);
    if (self->mode == 1) {
        self->manager()->v_0x044();
        self->manager()->findTextBox("N_TopBtn_00")->Animate(0);
        self->manager()->findTextBox("N_TopBtn_00")->CalculateMtx(self->drawInfo);
        self->manager()->findTextBox("N_TopBtn_00")->Draw(self->drawInfo);
    }
    self->rows = ((HbmCursorTextView*)&self->parts)->v_0x034();
    if (self->rows <= 4)
        self->rows = 4;
}

/* ---------------------------------------------------------------------------------------------------
 * the 0x1B04.. key-frame block
 * ------------------------------------------------------------------------------------------------ */
void fn_80568004(KbObject* self, f32 value)
{
    f32 out;

    self->tail_0x1A04.float_0x100 = value;
    if (self->tail_0x1A04.field_0x1E4 == 2)
        out = -self->tail_0x1A04.float_0x100;
    else
        out = lbl_8079D754;
    self->float_0x100 = out;
}

/* Returns the screen point of the rectangle's bottom-left corner under the matrix (y up). */
static inline Vec2f hbmKbCursorRectOrigin(const nw4r::math::MTX34& mtx, const nw4r::ut::Rect& rect)
{
    Vec corner;
    Vec2f origin;

    corner.x = rect.left;
    corner.y = rect.bottom;
    corner.z = 0.0f;
    PSMTXMultVec(mtx.m, &corner, &corner);
    origin.x = corner.x;
    origin.y = -corner.y;
    return origin;
}

/* 0x8056802C (0x9D4): the keyboard's update (slot 0x14C): draws the pane manager, scrolls the memo pane with the text,
 * sizes the text box and its button to the rows, runs the scroller (mode 2: shows the scroll arrows while there is text
 * above or below), places the scroll bar and, in mode 1, latches the arrow parts at the scroll limits. */
void hbmKbCursorUpdate(HbmCursorInput* self)
{
    f32 textHeight;
    f32 textTop;

    self->manager()->HbmPaneManager::drawComponents();
    self->textAlpha = self->manager()->getLayout()->mpRootPane->mAlpha;
    self->manager()->mpLyt->Animate(0);
    self->manager()->mpLyt->CalculateMtx(self->drawInfo);
    hbmKbCursorPlacePane(self->manager()->mpLyt->mpRootPane->FindPaneByName(lbl_80794728, true),
                         HbmCursorPoint(0.0f, self->position));

    {
        f32 height = (f32)self->rows * ((HbmCursorTextView*)&self->parts)->v_0x028();
        f32 y = self->buttonTranslate.y - (height - self->buttonHeight) * 0.5f;

        self->textBoxButtonPane->mSize = nw4r::lyt::Size(self->textBoxButtonPane->mSize.width, height);
        hbmKbCursorPlacePane(self->textBoxButtonPane, HbmCursorPoint(self->buttonTranslate.x, y));
    }
    self->textBoxButtonPane->CalculateMtx(self->drawInfo);
    self->manager()->calcDrawMtx(&self->buttonMtx, self->drawInfo, &self->textBoxButtonPane->mGlbMtx);
    {
        nw4r::ut::Rect rect;
        Vec2f scale;
        Vec2f origin;

        rect = self->textBoxButtonPane->GetPaneRect(self->drawInfo);
        scale = ((HbmCursorSelfView*)self)->v_0x18C();
        scale.y = 1.0f;
        origin = hbmKbCursorRectOrigin(self->buttonMtx, rect);
        self->buttonRect.left = origin.x;
        self->buttonRect.bottom = origin.y;
        self->buttonRect.right = origin.x + scale.x * (rect.right - rect.left);
        self->buttonRect.top = origin.y + scale.y * (rect.top - rect.bottom);
    }

    textHeight = (f32)self->rows * ((HbmCursorTextView*)&self->parts)->v_0x028();
    textTop = self->textBoxTranslate.y - (textHeight - self->textBoxHeight) * 0.5f;
    if (self->mode == 2) {
        textHeight = hbmKbCursorTwoRows * ((HbmCursorTextView*)&self->parts)->v_0x028();
        textTop = self->textBoxTranslate.y - (textHeight - self->textBoxHeight) * 0.5f - self->position;
    } else {
        self->textOffset = hbmKbCursorRestOffset;
    }
    self->textBoxPane->mSize = nw4r::lyt::Size(self->textBoxPane->mSize.width, textHeight);
    hbmKbCursorPlacePane(self->textBoxPane, HbmCursorPoint(self->textBoxTranslate.x, textTop));
    self->textBoxPane->CalculateMtx(self->drawInfo);
    self->manager()->calcDrawMtx(&self->textBoxMtx, self->drawInfo, &self->textBoxPane->mGlbMtx);
    self->manager()->calcDrawMtx(&self->textMtx, self->drawInfo, &self->textBoxPane->mGlbMtx);
    self->textRect = self->textBoxPane->GetPaneRect(self->drawInfo);
    self->textAlpha = self->textBoxPane->mAlpha;

    ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x00C();
    if (((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x014() != 0) {
        ((HbmCursorSelfView*)self)->v_0x244(((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x010());
    } else if (self->mode == 2) {
        ((HbmCursorSelfView*)self)->v_0x2D0();
        if (((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x014() == 0) {
            f32 textEnd;

            if (self->position - ((HbmCursorTextView*)&self->parts)->v_0x028() * 0.5f > 0.0f) {
                if (self->upArrowShown == 0) {
                    self->manager()->getPaneAnim("P_txtScrll_UP")->setState(5);
                    self->manager()->mpRoot->findComponent(self->manager()->findTextBox("B_txtScrll_UP"))->v_0x010();
                }
                self->upArrowShown = 1;
            } else {
                if (self->upArrowShown != 0)
                    self->manager()->getPaneAnim("P_txtScrll_UP")->setState(6);
                self->upArrowShown = 0;
            }
            textEnd = ((HbmCursorTextView*)&self->parts)->v_0x028();
            textEnd = (f32)((HbmCursorTextView*)&self->parts)->v_0x034() * textEnd;
            if (self->position + textHeight + ((HbmCursorTextView*)&self->parts)->v_0x028() * 0.5f < textEnd) {
                if (self->downArrowShown == 0) {
                    self->manager()->getPaneAnim("P_txtScrll_DOWN")->setState(5);
                    self->manager()->mpRoot->findComponent(self->manager()->findTextBox("B_txtScrll_DOWN"))->v_0x010();
                }
                self->downArrowShown = 1;
            } else {
                if (self->downArrowShown != 0)
                    self->manager()->getPaneAnim("P_txtScrll_DOWN")->setState(6);
                self->downArrowShown = 0;
            }
        }
    } else {
        self->upArrowShown = 0;
        self->downArrowShown = 0;
        self->manager()->getPaneAnim("P_txtScrll_UP")->setState(7);
        self->manager()->getPaneAnim("P_txtScrll_DOWN")->setState(7);
        self->manager()->findTextBox("N_txt_scrl")->Animate(0);
        self->manager()->findTextBox("N_txt_scrl")->CalculateMtx(self->drawInfo);
    }

    {
        nw4r::lyt::Pane* bar = self->manager()->findTextBox("N_txt_scrl");

        hbmKbCursorPlacePane(bar, HbmCursorPoint(bar->mTranslate.x, 0.0f));
    }
    if (self->mode == 1) {
        self->arrows->v_0x010();
        if (self->dirty == 0) {
            if (self->leftPressed != 0 && ((HbmCursorSelfView*)self)->v_0x2A8() >= self->position) {
                self->leftPressed = 0;
                self->arrows->v_0x014(0, 2);
            } else if (self->leftPressed == 0 && ((HbmCursorSelfView*)self)->v_0x2A8() < self->position) {
                self->leftPressed = 1;
                self->arrows->v_0x014(0, 1);
            }
            if (self->rightPressed != 0 && ((HbmCursorSelfView*)self)->v_0x2AC() <= self->position) {
                self->rightPressed = 0;
                self->arrows->v_0x014(1, 2);
            }
            if (self->rightPressed == 0 && ((HbmCursorSelfView*)self)->v_0x2AC() > self->position) {
                self->rightPressed = 1;
                self->arrows->v_0x014(1, 1);
            }
            if (self->popup != 0)
                self->popup->v_0x014();
        }
    }
    hbmPanelAdvanceWord1A00((HbmPanelView*)self);
}

/* 0x80568A00 (0x260): on a pending scroll request (1 or 2) in mode 2 and while no scroll runs, scrolls by whole rows
 * so the caret row (+0x1958) is visible, then clears the request. */
void hbmKbCursorScrollToCaret(HbmCursorInput* self)
{
    f32 bottom;

    if (((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x014() != 0)
        return;
    if ((u32)(self->scrollRequest - 1) > 1)
        return;
    if (self->mode == 2) {
        bottom = self->caretTop + ((HbmCursorTextView*)&self->parts)->v_0x028() / 2.0f;
        if (self->position > bottom) {
            s32 lines = (s32)((self->position - self->caretTop) / ((HbmCursorTextView*)&self->parts)->v_0x028());

            ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x008(0, 0, self->position,
                self->position - (f32)lines * ((HbmCursorTextView*)&self->parts)->v_0x028(), hbmKbCursorScrollFrames);
            ((HbmCursorSelfView*)self)->v_0x17C(11);
        }
        if (self->position + hbmKbCursorTwoRows * ((HbmCursorTextView*)&self->parts)->v_0x028() < bottom) {
            s32 lines = (s32)((bottom - (self->position + hbmKbCursorTwoRows * ((HbmCursorTextView*)&self->parts)->v_0x028()))
                              / ((HbmCursorTextView*)&self->parts)->v_0x028()) + 1;

            ((HbmCursorScrollerView*)&self->scroll_1BAC)->v_0x008(0, 0, self->position,
                self->position + (f32)lines * ((HbmCursorTextView*)&self->parts)->v_0x028(), hbmKbCursorScrollFrames);
            ((HbmCursorSelfView*)self)->v_0x17C(11);
        }
    }
    self->scrollRequest = 0;
}

void fn_80568C60(KbObject* self)
{
    if (self->tail_0x1A04.float_0x110 == self->tail_0x1A04.float_0x118)
        self->tail_0x1A04.float_0x118 = self->tail_0x1A04.float_0x118 + lbl_8079D774;
    if (self->tail_0x1A04.float_0x114 == self->tail_0x1A04.float_0x11C)
        self->tail_0x1A04.float_0x114 = self->tail_0x1A04.float_0x114 + lbl_8079D774;
    hbmTextWriterBeginDraw((HbmTextWriter*)&self->field_0x010, (const nw4r::ut::Rect*)&self->tail_0x1A04.float_0x110);
}

/* 0x80568CA8 (0x4): ignores a text-position change (slot 0x16C): the keyboard scrolls its text itself. */
void hbmKbCursorIgnorePositionChange(KbObject* self) { (void)self; }

/* 0x80568CAC (0x110): the keyboard's hit test (slot 0x184): the point is taken in scrolled text and clamped to the
 * last laid-out row (at most the line limit). */
u32 hbmKbCursorHitTest(HbmCursorInput* self, f32 x, f32 y)
{
    u32 lines = ((HbmCursorTextView*)&self->parts)->v_0x034();

    if (lines > self->maxLines)
        lines = self->maxLines;
    if (y + self->position > (f32)lines * ((HbmCursorTextView*)&self->parts)->v_0x028())
        return hbmPanelHitTest((HbmPanelView*)self, x, (f32)lines * ((HbmCursorTextView*)&self->parts)->v_0x028());
    return hbmPanelHitTest((HbmPanelView*)self, x, y + self->position);
}

void fn_80568DBC(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        hbmPanelDrawCaret(self);
}

/* The 2D anchored position of the keyboard's cursor: the object's own pair when one is attached, the
 * shared constant pair otherwise. */
Vec2f fn_80568DD0(KbObject* self)
{
    Vec2f out;

    HbmCursorAnchor* anchor = (HbmCursorAnchor*)self->tail_0x1A04.field_0x0F4;

    if (anchor == 0) {
        out.x = lbl_8079D750;
        out.y = lbl_8079D750;
        return out;
    }
    return anchor->pos;
}

/* 0x80568E08 (0xB4): draws the memo's header, the portrait and the portrait's caption (slot 0x2BC). */
void hbmKbCursorDrawHeader(HbmCursorInput* self)
{
    self->manager()->findTextBox(hbmKbCursorPaneNames[0])->Draw(self->drawInfo);
    self->manager()->findTextBox(lbl_80794738)->Draw(self->drawInfo);
    self->manager()->findTextBox("T_Nigaoe")->Draw(self->drawInfo);
}

/* 0x80568EBC (0x134): draws the memo's body pane once per row, each a row lower, then the touch-letter caption unless
 * a touch placed the caret (slot 0x2C0). */
void hbmKbCursorDrawBody(HbmCursorInput* self)
{
    nw4r::lyt::Pane* body = self->manager()->findTextBox(hbmKbCursorPaneNames[1]);
    f32 y = 0.0f;
    s32 i;

    for (i = 0; i < self->rows; i++) {
        hbmKbCursorPlacePane(body, HbmCursorPoint(0.0f, y));
        body->CalculateMtx(self->drawInfo);
        body->Draw(self->drawInfo);
        y -= body->mSize.height;
    }
    if (self->touchLetter == 0)
        self->manager()->findTextBox("T_TouchLetter")->Draw(self->drawInfo);
}

/* 0x80568FF0 (0x100): places the memo's footer below the last row and draws it (slot 0x2C4). */
void hbmKbCursorDrawFooter(HbmCursorInput* self)
{
    f32 height = self->manager()->findTextBox(hbmKbCursorPaneNames[1])->mSize.height;
    f32 y = (f32)(self->rows - 1) * (-1.0f * height);
    nw4r::lyt::Pane* footer = self->manager()->findTextBox(hbmKbCursorPaneNames[3]);

    hbmKbCursorPlacePane(footer, HbmCursorPoint(0.0f, y));
    footer->CalculateMtx(self->drawInfo);
    footer->Draw(self->drawInfo);
}

/* 0x805690F0 (0xFC): resets the scroll (slot 0x258): the position and the touch record cleared, the layout's root pane
 * back at the origin, the memo key shown and the scroll arrows hidden. */
void hbmKbCursorResetScroll(HbmCursorInput* self)
{
    ((HbmCursorSelfView*)self)->v_0x244(0.0f);
    self->touchPosition = 0.0f;
    self->touchLineTop = 0.0f;
    hbmKbCursorPlacePane(self->manager()->mpLyt->mpRootPane, HbmCursorPoint(0.0f, 0.0f));
    self->manager()->getPaneAnim(lbl_80794728)->setState(1);
    self->manager()->getPaneAnim("P_txtScrll_UP")->setState(7);
    self->manager()->getPaneAnim("P_txtScrll_DOWN")->setState(7);
}

/* ---------------------------------------------------------------------------------------------------
 * the keyboard's pane/text lookups (the layout vocabulary of the `.data` pool)
 * ------------------------------------------------------------------------------------------------ */
/* 0x805691EC (0xF0): shows the shift key's state (3 shifted, 2 not), releases pressed arrows and marks the input
 * dirty. */
void hbmKbCursorRefreshShift(HbmCursorInput* self)
{
    if (self->shifted == 0)
        ((HbmCursorTailView*)&((KbObject*)self)->tail_0x1A04)->v_0x05C(lbl_80794728)->v_0x014(2);
    else
        ((HbmCursorTailView*)&((KbObject*)self)->tail_0x1A04)->v_0x05C(lbl_80794728)->v_0x014(3);
    if (self->leftPressed != 0) {
        self->arrows->v_0x014(0, 2);
        self->leftPressed = 0;
    }
    if (self->rightPressed != 0) {
        self->arrows->v_0x014(1, 2);
        self->rightPressed = 0;
    }
    self->dirty = 1;
}

void fn_805692DC(KbObject* self)
{
    ((HbmCursorTailView*)&self->tail_0x1A04)->v_0x05C(lbl_80794728)->v_0x018();
}

void fn_8056931C(KbObject* self, void* arg)
{
    ((HbmCursorPaneView*)self->tail_0x1A04.field_0x004->field_0x10)->v_0x03C(lbl_80657E14, 1)->v_0x07C((const u16*)arg, 0);
}

/* 0x80569380 (0x64): sets the text of the layout's "T_Nigaoe" pane. */
void hbmKbCursorSetNigaoeText(KbObject* self, const u16* text)
{
    ((HbmCursorPaneView*)self->tail_0x1A04.field_0x004->field_0x10)->v_0x03C("T_Nigaoe", 1)->v_0x07C(text, 0);
}

/* 0x805693E4 (0x78): sets the text of the layout's "T_TouchLetter" pane and clears the touch-letter flag (+0x1BD8). */
void hbmKbCursorSetTouchLetterText(HbmCursorInput* self, const u16* text)
{
    ((HbmCursorPaneView*)((KbObject*)self)->tail_0x1A04.field_0x004->field_0x10)->v_0x03C("T_TouchLetter", 1)->v_0x07C(text, 0);
    self->touchLetter = 0;
}

/* ---------------------------------------------------------------------------------------------------
 * text-search wrappers: the character name (lbl_80794738) is looked up in the keyboard's own list
 * ------------------------------------------------------------------------------------------------ */
void fn_8056945C(KbObject* self)
{
    ((HbmCursorTailView*)&self->tail_0x1A04)->v_0x02C(lbl_80794738);
}

void fn_80569470(KbObject* self)
{
    ((HbmCursorTailView*)&self->tail_0x1A04)->v_0x02C(lbl_80794738)->v_0x040(lbl_80794738, 1);
}

f32 fn_805694B8(KbObject* self) { (void)self; return lbl_8079D754; }

f32 fn_805694C0(KbObject* self)
{
    f32 frame = ((HbmCursorSelfView*)self)->v_0x2A4();
    return frame - lbl_8079D784;
}

/* 0x805694F4 (0x60): the text block's height: the row height times the row count. */
f32 hbmKbCursorTextHeight(HbmCursorInput* self)
{
    return ((HbmCursorTextView*)&self->parts)->v_0x028() * (f32)self->rows;
}

/* 0x80569554 (0x70): in mode 1, posts event 25 while the popup is open. */
void hbmKbCursorPollPopup(HbmCursorInput* self)
{
    if (self->mode == 1 && self->popup != 0 && self->popup->v_0x008() != 0)
        ((HbmCursorSelfView*)self)->v_0x17C(25);
}

/* 0x805695C4 (0x90): in mode 1, presses the character key (state 4), opens the popup and posts event 24. */
void hbmKbCursorOpenPopup(HbmCursorInput* self)
{
    if (self->mode != 1)
        return;
    ((HbmCursorTailView*)&((KbObject*)self)->tail_0x1A04)->v_0x05C(lbl_80794738)->v_0x014(4);
    if (self->popup != 0)
        self->popup->v_0x00C();
    ((HbmCursorSelfView*)self)->v_0x17C(24);
}

/* 0x80569654 (0x7C): in mode 1, closes the popup and releases the character key (state 5). */
void hbmKbCursorClosePopup(HbmCursorInput* self)
{
    if (self->mode != 1)
        return;
    if (self->popup != 0)
        self->popup->v_0x010();
    ((HbmCursorTailView*)&((KbObject*)self)->tail_0x1A04)->v_0x05C(lbl_80794738)->v_0x014(5);
}

/* 0x805696D0 (0xAC): puts the keyboard's texture object into the first texture map of its material (slot 0x27C). */
void hbmKbCursorApplyTexture(HbmCursorInput* self)
{
    nw4r::lyt::TexMap texMap(*self->texObj);

    ((HbmCursorSelfView*)self)->v_0x2A0()->GetTexMapAry()[0] = texMap;
}

/* 0x8056977C (0xE0): puts the keyboard in `mode`: 1 hides the text scroll bar, 2 shows it, 3 resets the panel (message
 * 6) and its node. */
void hbmKbCursorSetMode(HkbKbWrapped* wrapped, s32 mode)
{
    HbmCursorInput* self = (HbmCursorInput*)wrapped;

    self->mode = mode;
    switch (mode) {
    case 1:
        self->manager()->findTextBox("N_txt_scrl")->SetVisible(false);
        break;
    case 2:
        self->manager()->findTextBox("N_txt_scrl")->SetVisible(true);
        break;
    case 3:
        ((HbmCursorSelfView*)self)->v_0x018(6, NULL);
        self->nodeValue = 0;
        ((HbmCursorNodeView*)&self->node_118)->v_0x014();
        hbmNodeReset(&self->node_118);
        break;
    }
}

void fn_8056985C(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((HbmCursorPartView*)self->tail_0x1A04.field_0x1D0)->v_0x018();
    hbmPanelSetDrawPosition(self, arg);
}

void fn_80569910(KbObject* self, u16 code)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((HbmCursorPartView*)self->tail_0x1A04.field_0x1D0)->v_0x010();
    hbmTextWriterPrintChar((HbmTextWriter*)&self->field_0x010, code);
}

/* 0x805698BC (0x54): In mode 1, plays the part's new-line animation (10), then breaks the line. */
void hbmKbCursorNewLine(HbmCursorInput* self)
{
    if (self->mode == 1)
        ((HbmCursorPartArgView*)((KbObject*)self)->tail_0x1A04.field_0x1D0)->v_0x010(10);
    hbmPanelNewLine(self);
}

void fn_80569970(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((HbmCursorPartArgView*)self->tail_0x1A04.field_0x1D0)->v_0x010(0);
}

void fn_80569998(KbObject* self) { (void)self; }

/* 0x8056999C (0x1FC): the keyboard's pane handler (slot 0x18): runs the text panel's listener, then on a press (4 with
 * trigger bit 0x10), a pointer entering (0) or leaving (1) the portrait button or an arrow button, calls the keyboard's
 * matching slot. */
void hbmKbCursorOnPaneEvent(HbmCursorPaneHandler* self, HbmCursorPaneComponent* component, u32 event,
                            HbmCursorPaneTrigger* trigger)
{
    const char* name;

    hbmTextPaneListenerOnEvent((HbmTextPaneListener*)self, (HbmPaneComponentView*)component, event,
                               (HbmPointerInfo*)trigger);
    name = component->pane->mName;
    if (name[0] != 'B')
        return;
    if (event == 4 && (trigger->trigger & 0x10)) {
        if (hbmNameMatches("B_Nigaoe", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x270();
        if (hbmNameMatches("B_ArwR", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x290();
        if (hbmNameMatches("B_ArwL", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x294();
    }
    if (event == 0) {
        if (hbmNameMatches("B_Nigaoe", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x274();
        if (hbmNameMatches("B_ArwR", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x280();
        if (hbmNameMatches("B_ArwL", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x288();
    }
    if (event == 1) {
        if (hbmNameMatches("B_Nigaoe", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x278();
        if (hbmNameMatches("B_ArwR", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x284();
        if (hbmNameMatches("B_ArwL", name))
            ((HbmCursorSelfView*)self->keyboard)->v_0x28C();
    }
}

void fn_80569B98(HbmKey* key, u32 state) { key->HbmKey::setState(state); }

void fn_80569B9C(KbObject* self) { (void)self; }

void fn_80569BA0(KbObject* self) { (void)self; }

/* Returns the part's state. */
u32 HbmKbCursorPart::getState() {
    return mState;
}


/* Stores the state, then hands it to the band key. */
void HbmKbCursorPart::setState(u32 state) {
    mState = state;
    HbmKey::setState(state);
}


/* Returns the part's kind. */
u32 HbmKbCursorPart::getType() {
    return mType;
}


/* Clears the part's state. */
void HbmKbCursorPart::reset() {
    mState = 0;
}


/* Destroys the part (the inline base destructors run here). */
HbmKbCursorPartA::~HbmKbCursorPartA() {
}


void fn_80569C24(KbObject* self) { (void)self; }

/* Destroys the part (the inline base destructors run here). */
HbmKbCursorPartB::~HbmKbCursorPartB() {
}


/* 0x80569C84 (0x10): forwards to the scroll part's slot 0x14. */
void hbmKbCursorScroll(HbmCursorInput* self, s32 index, s32 mode)
{
    ((HbmCursorScrollView*)&self->scroll_1BAC)->v_0x014(index, mode);
}

void  fn_80569C94(KbObject* self, void* value) { self->tail_0x1A04.field_0x1C8 = value; }

f32 fn_80569C9C(KbObject* self) { return self->tail_0x1A04.float_0x100; }

/* 0x80569CA4 (0x40): deleting destructor of a part with an empty destructor. */
HbmWidget* hbmKbCursorRecordDelete(HbmWidget* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

/* Destroys the cursor key. */
HbmKbCursorKey::~HbmKbCursorKey() {
}


/* ---------------------------------------------------------------------------------------------------
 * adjustor thunks: the retail body is `addi r3,r3,-N; b <method>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
/* 0x80569D3C (0x8): +0x10 base -> fn_80568DBC. */
void hbmKbCursorAdj10Refresh(u8* base) { fn_80568DBC((KbObject*)(base - 0x10)); }

/* 0x80569D44 (0x8): +0x10 base -> fn_80569910. */
void hbmKbCursorAdj10Deactivate(u8* base, u16 code) { fn_80569910((KbObject*)(base - 0x10), code); }

/* 0x80569D4C (0x8): +0x10 base -> hbmKbCursorNewLine. */
void hbmKbCursorAdj10NewLine(u8* base) { hbmKbCursorNewLine((HbmCursorInput*)(base - 0x10)); }

/* 0x80569D54 (0x8): +0x10 base -> fn_80569970. */
void hbmKbCursorAdj10Release(u8* base) { fn_80569970((KbObject*)(base - 0x10)); }

/* 0x80569D5C (0x8): +0x10 base -> fn_8056985C. */
void hbmKbCursorAdj10Activate(u8* base, u32 arg) { fn_8056985C((KbObject*)(base - 0x10), (void*)arg); }

/* 0x80569D64 (0x8): +0x10 base -> fn_80568DD0. */
Vec2f hbmKbCursorAdj10Anchor(u8* base) { return fn_80568DD0((KbObject*)(base - 0x10)); }

/* 0x80569D6C (0x8): +0x10 base -> fn_80568C60. */
void hbmKbCursorAdj10Clamp(u8* base) { fn_80568C60((KbObject*)(base - 0x10)); }

/* 0x80569D74 (0x8): +0x10 base -> hbmKbCursorCalc. */
void hbmKbCursorAdj10Calc(u8* base) { hbmKbCursorCalc((HbmCursorInput*)(base - 0x10)); }

/* 0x80569D7C (0x8): +0x1A04 base -> hbmKbCursorInput. */
void hbmKbCursorAdj1A04Input(u8* base, u32 channel, u32 trigger, u32 held, u32 release, u32 repeat, f32 x, f32 y)
{
    hbmKbCursorInput((HbmCursorInput*)(base - 0x1A04), channel, trigger, held, release, repeat, x, y);
}

/* 0x80569D84 (0x8): +0x1A04 base -> fn_80567D3C. */
void hbmKbCursorAdj1A04Close(u8* base, HkbManager* manager) { fn_80567D3C(base - 0x1A04, manager); }

/* 0x80569D8C (0x8): +0x1A04 base -> hbmKbCursorUpdate. */
void hbmKbCursorAdj1A04Update(u8* base) { hbmKbCursorUpdate((HbmCursorInput*)(base - 0x1A04)); }

/* 0x80569D94 (0x8): +0x1A04 base -> hbmKbCursorCalc. */
void hbmKbCursorAdj1A04Calc(u8* base) { hbmKbCursorCalc((HbmCursorInput*)(base - 0x1A04)); }

/* 0x80569D9C (0x8): +0x1A04 base -> hbmKbCursorReset. */
void hbmKbCursorAdj1A04Reset(u8* base) { hbmKbCursorReset((HbmCursorInput*)(base - 0x1A04)); }

/* 0x80569DA4 (0x8): +0x1A04 base -> hbmKbCursorDestroy. */
HbmCursorInput* hbmKbCursorAdj1A04Destroy(u8* base, s32 flag)
{
    return hbmKbCursorDestroy((HbmCursorInput*)(base - 0x1A04), flag);
}
}
