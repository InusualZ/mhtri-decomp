/*
 * homebutton/hbm_text_panel.cpp - software-keyboard code of the tiHKBManager band: the text-entry panel (its message
 * dispatcher, key handlers, cursor and selection movement, text layout, caret and composition drawing, line-cell list,
 * button auto-repeat), the keyboard widget built on it (reset, calc, update, input, message limits, pane listener) and
 * the adjustor thunks of the widget's secondary tables.
 *
 * RANGE.     .text 0x805425E4..0x8054F7BC (236 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * FLAGS.     Scoped `#pragma auto_inline on` around the helpers retail inlines into a later caller and also keeps out of
 *            line (hbmPanelGetInputTarget, the line-cell list, hbmPanelNewLine, hbmPanelWriterRefresh, ...); -inline
 *            noauto inlines nothing on its own. hbmPanelLayoutText is `inline` and reached out of line by fn_8054F6B4
 *            under `dont_inline` (retail inlines it into hbmKbWidgetCalc and keeps the copy).
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the text panel; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS every hbmPanel/hbmKb/hbmText function name and every Hbm* view class: read off the bodies, the pane
 *            names they pass ("P_txtScrll_UP", "T_2l_TextBox", "N_separateBarAll") and the table slots that reference
 *            them. The map half is the rename-batch of request hbm-text-panel#12; view slots are named by offset.
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch;
 *            nothing here constructs one, so no table is emitted (rule 10). HbmPanelView carries the whole widget
 *            layout (0x1AF4 bytes) with the CharWriter-derived writer at +0x10 and the pane manager at +0x1A04.
 * SHAPES.    The line-cell list (HbmTextCellList) is inline methods; its unlink/insert pair reproduces retail's
 *            `cmplw rX,rX` self-compare. Colour globals copy as words through `ToU32()`; `/ 2.0f` keeps retail's fmuls
 *            operand order; `nw4r::ut::Rect::GetWidth()` keeps its evaluation order.
 * RESIDUALS. hbmPanelHandleMessage (0x8054358C): frame 0x250 against retail 0x270, and the saved-register numbering of
 *            the case bodies differs.
 * RESIDUALS. hbmTextCellListInit (0x80542BE0), hbmPanelGetTextOrigin (0x8054826C), hbmPanelHitTest (0x80547D00),
 *            hbmPanelLayoutText (0x80545C44): register allocation and frame size; the instruction streams agree.
 * RESIDUALS. fn_805482CC: retail loads its pooled float before the 0x195C store and rotates f1/f2 against ours.
 * RESIDUALS. hbmPanelHandleKeyDown (0x80548950): retail tests the key as `addis r3,r31,1; subi r0,r3,0x309B`; only the
 *            literal `(u16)(key + 0xCF65)` reproduces it, and `(u16)(key - 0x309B)` is kept.
 * RESIDUALS. fn_8054F178: retail loads both arguments before either store (`lbz r4; lwz r0; stb; stw`); every spelling
 *            tried keeps one load/store pair after the other.
 * RESIDUALS. Key-handler branch shapes (hbmPanelArrowLeft/Right, hbmPanelCaretForward, hbmPanelHandleSpecialKey): retail
 *            lays some `if` tails out as `bne; b` where ours falls through.
 * RESIDUALS. Unwritten (9 functions, 0xC1C bytes): the destructors fn_805425E4 (0xF4), fn_805426D8 (0x58),
 *            fn_80542730 (0x5C), fn_8054278C (0x5C), fn_80543020 (0x74) and fn_8054CF38 (0x134), the constructors
 *            hbmTextBuffersInit (0x1CC) and fn_80542D8C (0x214), and the widget create fn_8054D0AC (0x530); they store the
 *            tables at 0x8064ECA0..0x8064F1C0 and 0x806515D0..0x80651850, which wait on the classes behind them (rule 10)
 *            and on the .data re-cut below.
 * RESIDUALS. The static initializer (0x8054F550, paired with __sinit_\hbm_text_panel_cpp) fills each colour with
 *            four `stb` (the colours use nw4r::ut::Color's (r, g, b, a) constructor); its trailing copy of an .sdata
 *            word into 0x8064E7CC is not written.
 * NAMES.     Other units' symbols come from their owners' headers (hbm_value, hbm_hermite, hbm_kb_event, hbm_kb_widget,
 *            hkb_keyboard, hkb_widget_tail, tiHKBManager, hbm_gui_draw, KPR, MSL_C extras).
 * RESIDUALS. .data 0x8064E760..0x8064F360 spans at least 3 TUs (datagap.py emission-order seams at 0x8064F0BC and
 *            0x8064F338), so the data cannot match until the range is re-cut; string literals and the colour globals
 *            are emitted here meanwhile.
 * RESIDUALS. .sbss: ours adds s_emptyLength and the guarded local static s_kbdState (41 B against the target's 32 B,
 *            which holds the seven colours and one word).
 * RESIDUALS. homebutton/hbm_text_panel.h keeps the consumers' opaque prototypes under the map's names and is not
 *            included here, so the rename sweep cannot make it clash with the typed definitions.
 * NAMES.     GUESS classes HbmPanelPart and its kind HbmPanelPartA (band keys with a state word; members reset,
 *            setState, getResource, getState).
 */

#include "types.h"

#include "fn_805482CC.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "MTX/mtxvec.h"
#include "nw4r/math_triangular.h"
#include "homebutton/hkb_widget.h"
#include "unsplit/unknown.h"
#include "KPR/kpr.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hbm_kb_object.h"
#include "MSL_C/wstring.h"
#include "nw4r/fn_805012C4.h"
#include "nw4r/fn_80502828.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hkb_widget_tail.h"
#include "homebutton/hbm_hermite.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_kb_widget.h"
#include "homebutton/hbm_gui_draw.h"
#include "homebutton/tiHKBManager.h"
#include "MSL_C/extras.h"

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
extern "C" void hbmPanelUpdateButtonRepeat(HbmTextPanel* self, s32 b, s32 c);

/* Dispatch views of the panel's polymorphic parts: the slots are the ones the `*Vtbl` views of fn_805482CC.h name,
 * declared as virtuals so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch.  No method is
 * defined and nothing is constructed here, so no table is emitted (rule 10).  Slots are named by offset; the retail
 * names are not recoverable. */

/* The helper objects at +0x193C / +0x1940 / +0x1944 / +0x1A18 (Panel805482CC_ItemVtbl): the text buffers of 0x50 bytes
 * hbmTextBuffersInit allocates and fills +0x04..+0x28 (homebutton/hbm_value.cpp's HbmTextBuffer view of the same class).
 * size: 0x50 */
class HbmPanelItemView {
public:
    /* slot +0x008 */ virtual void v_0x008(s32 event, u32 id);
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
    /* slot +0x034 */ virtual u16 v_0x034();
    /* slot +0x038 */ virtual u32 v_0x038(u32 limit);
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044(u16 length);
    /* slot +0x048 */ virtual u16* v_0x048();
    /* slot +0x04C */ virtual void v_0x04C(u32 key);
    /* slot +0x050 */ virtual u16 v_0x050();
    /* slot +0x054 */ virtual s32 v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C(u32 key);
    /* slot +0x060 */ virtual void v_0x060(const u16* text);
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void v_0x068();
    /* slot +0x06C */ virtual void v_0x06C(const u16* text);
    /* slot +0x070 */ virtual s32 v_0x070();
    /* slot +0x074 */ virtual s32 v_0x074();
    /* slot +0x078 */ virtual void v_0x078(s32 position);
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual s32 v_0x084();
    /* slot +0x088 */ virtual s32 v_0x088();
    /* slot +0x08C */ virtual void v_0x08C(u32* start, u32* end);
    /* slot +0x090 */ virtual s32 v_0x090();
    /* slot +0x094 */ virtual s32 v_0x094();
    /* slot +0x098 */ virtual void v_0x098(u32* start, u32* end);
    /* slot +0x09C */ virtual u16 v_0x09C();
    /* slot +0x0A0 */ virtual void v_0x0A0();
    /* slot +0x0A4 */ virtual void v_0x0A4();
    /* slot +0x0A8 */ virtual void v_0x0A8();
    /* slot +0x0AC */ virtual void v_0x0AC();
    /* slot +0x0B0 */ virtual void v_0x0B0();
    /* slot +0x0B4 */ virtual void v_0x0B4();
    /* slot +0x0B8 */ virtual void v_0x0B8();
    /* slot +0x0BC */ virtual void v_0x0BC();
    /* slot +0x0C0 */ virtual s32 v_0x0C0();
    /* slot +0x0C4 */ virtual void v_0x0C4();
    /* slot +0x0C8 */ virtual const u16* v_0x0C8();
    /* slot +0x0CC */ virtual s32 v_0x0CC();
    /* slot +0x0D0 */ virtual void v_0x0D0();
    /* slot +0x0D4 */ virtual void v_0x0D4();
    /* slot +0x0D8 */ virtual s32 v_0x0D8();
    /* slot +0x0DC */ virtual s32 v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0(u32 index, u16* text);
    /* slot +0x0E4 */ virtual s32 v_0x0E4();
    /* slot +0x0E8 */ virtual const u16* v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC(s32 arg);
    /* slot +0x0F0 */ virtual void v_0x0F0(s32 page);
    /* slot +0x0F4 */ virtual void v_0x0F4(s32 kind);
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

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u16 capacity;              /* 512 when hbmTextBuffersInit builds it */
    /* +0x06 */ u16 length;
    /* +0x08 */ u16 byteLength;
    /* +0x0A */ u16 pad_0x0A;
    /* +0x0C */ u16* text;
    /* +0x10 */ u16* workText;
    /* +0x14 */ u16 deadKey;               /* the pending dead key slot 0x50 returns */
    /* +0x16 */ u8 pad_0x16[0x2];
    /* +0x18 */ u32 allocator;
    /* +0x1C */ u32 cursor;                /* slot 0x88 */
    /* +0x20 */ u32 anchor;                /* the selection's other end */
    /* +0x24 */ u8 selecting;
    /* +0x25 */ u8 pad_0x25[0x3];
    /* +0x28 */ s32 inputMode;
    /* +0x2C */ KPRQueue queue;
    /* +0x44 */ u16 lookahead[6];
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
    /* slot +0x0EC */ virtual s32 v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0(u32 index, u16* text);
    /* slot +0x0F4 */ virtual void v_0x0F4(s32 index);
    /* slot +0x0F8 */ virtual void v_0x0F8(s32 arg);
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual s32 v_0x100();
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual s32 v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120();
    /* slot +0x124 */ virtual s32 v_0x124();
    /* slot +0x128 */ virtual void v_0x128(u8 on);
    /* slot +0x12C */ virtual u16 v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134(u8 on);
    /* slot +0x138 */ virtual void v_0x138();
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual s16 v_0x140();
    /* slot +0x144 */ virtual s32 v_0x144();
};

/* The mark record at +0xC0 of the text writer (the constructor stores 0xFFFF, 0 and a cleared byte).
 * size: 0x0C */
struct HbmTextWriterMark {
    /* +0x00 */ u32 index;
    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ u8 active;
    /* +0x09 */ u8 pad_0x09[0x3];
};

/* A homebutton/hbm_value.cpp object whose virtual slot returns its +0x18 word.
 * size: 0x1C (approximation: only +0x18 is touched) */
struct HbmWordView {
    /* +0x00 */ u8 unused_0x00[0x18];
    /* +0x18 */ u32 value_0x18;
};

/* The panel's text writer at +0x10: an nw4r::ut::CharWriter (0x4C bytes) whose class adds the vptr at its own +0x4C
 * (Panel805482CC_SubVtbl) and the layout state below.
 * size: 0x108 (the panel's constructor fills it through +0x105) */
struct HbmCharBox;

class HbmPanelSubView : public nw4r::ut::CharWriter {
    /* +0x00 is the CharWriter, +0x4C the compiler-emitted vptr */
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020(const nw4r::ut::Rect* clip);
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual f32 v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* slot +0x034 */ virtual s32 v_0x034();
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
    /* slot +0x068 */ virtual void v_0x068(HbmCharBox* box);
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074(s32 firstLine, s32 lastLine);
    /* slot +0x078 */ virtual void v_0x078(s32 first, s32 last);
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual s32 v_0x088();
    /* slot +0x08C */ virtual u32 v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();

    /* +0x050 */ u32 unused_0x050;
    /* +0x054 */ u8 unused_0x054[0x34];
    /* +0x088 */ nw4r::ut::Rect clip;      /* the clip rectangle slot 0x20 sets */
    /* +0x098 */ f32 unused_0x098;
    /* +0x09C */ u8 unused_0x09C;
    /* +0x09D */ u8 unused_0x09D;
    /* +0x09E */ u8 pad_0x09E[0x2];
    /* +0x0A0 */ s32 lastLine;             /* the index of the last laid-out line */
    /* +0x0A4 */ f32 lineSpace;            /* the text box's line spacing */
    /* +0x0A8 */ f32 charSpacing;          /* added to the cursor after every printed character */
    /* +0x0AC */ f32 fontWidth;            /* the text box's font size */
    /* +0x0B0 */ f32 fontHeight;
    /* +0x0B4 */ f32 unused_0x0B4;
    /* +0x0B8 */ f32 textHeight;           /* the laid-out text's height; the caret is on the last line below it */
    /* +0x0BC */ u32 unused_0x0BC;
    /* +0x0C0 */ HbmTextWriterMark mark;
    /* +0x0CC */ u32 unused_0x0CC;
    /* +0x0D0 */ u32 unused_0x0D0;
    /* +0x0D4 */ u32 unused_0x0D4;
    /* +0x0D8 */ u32 unused_0x0D8;
    /* +0x0DC */ s32 caretIndex;           /* the character the caret stands before */
    /* +0x0E0 */ u32 unused_0x0E0;
    /* +0x0E4 */ u8 caretVisible;          /* cleared by hbmTextWriterHideCaret */
    /* +0x0E5 */ u8 pad_0x0E5[0x3];
    /* +0x0E8 */ f32 unused_0x0E8[2];
    /* +0x0F0 */ f32 scrollY;              /* the text's vertical scroll, copied from the panel's +0x1960 */
    /* +0x0F4 */ u8 unused_0x0F4[0x10];
    /* +0x104 */ u8 selectionVisible;      /* cleared by hbmTextWriterHideSelection */
    /* +0x105 */ u8 unused_0x105;
    /* +0x106 */ u8 pad_0x106[0x2];
};

/* The scroll record embedded at +0x1964 (Panel805482CC_ScrollVtbl).
 * size: 0x24 (the panel's next member sits at +0x1988) */
class HbmPanelScrollView {
public:
    /* slot +0x008 */ virtual void v_0x008(s32 a, s32 b, f32 c, f32 d, f32 e);
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual f32 v_0x010();
    /* slot +0x014 */ virtual s32 v_0x014();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0xC];
    /* +0x10 */ f32 unused_0x10;
    /* +0x14 */ u8 unused_0x14;
    /* +0x15 */ u8 unused_0x15;
    /* +0x16 */ u8 pad_0x16[0x2];
    /* +0x18 */ u32 unused_0x18;
    /* +0x1C */ u32 unused_0x1C;
    /* +0x20 */ f32 idleTime;              /* frames the panel's text field has been idle */
};

/* A layout pane the panel looks text boxes up in (slot 0x3C finds a pane by name, slot 0x7C sets a text box's string).
 * size: 0x4C (approximation: only the compiler-emitted vptr and the scale at +0x44 are touched) */
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

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0x40];
    /* +0x44 */ Vec2f scale;               /* nw4r::lyt::Pane::mScale */
};

/* The layout the panel's +0x1A04 record holds at its +0x04: slot 0x30 calculates it, slot 0x38 animates it, and its
 * root pane sits at +0x10.
 * size: 0x14 (approximation: only the compiler-emitted vptr and +0x10 are touched) */
class HbmLayoutView {
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
    /* slot +0x030 */ virtual void v_0x030(const nw4r::lyt::DrawInfo* drawInfo);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038(s32 option);

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0xC];
    /* +0x10 */ HbmPaneView* rootPane;
};

/* A button of the keyboard widget's pane manager (slot 0x14 sets its state).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPaneButtonView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010(s32 state);
    /* slot +0x014 */ virtual void v_0x014(s32 state);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual s32 v_0x028();
};

/* A pane component of the widget's component list (tiHKBManager's; slot 0x10 activates it, slot 0x28 says whether a
 * pointer channel is on it, +0x9C is its pane).
 * size: 0xA0 (approximation: only the compiler-emitted vptr and +0x9C are touched) */
class HbmPaneComponentView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual s32 v_0x028(u32 channel);

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0x98];
    /* +0x9C */ nw4r::lyt::Pane* pane;
};

/* The widget's pane component list (+0x1A14), searched by pane name.
 * size: 0x04 (approximation: no member is touched) */
struct HbmPaneComponentList {
    /* +0x00 */ u32 unused_0x00;
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
    /* slot +0x028 */ virtual HbmLayoutView* v_0x028();
    /* slot +0x02C */ virtual s32 v_0x02C(const char* name);
    /* slot +0x030 */ virtual u32 v_0x030(u32 channel, const char* name);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050(const char* name, int on);
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual HbmPaneButtonView* v_0x05C(const char* name);
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* slot +0x068 */ virtual void v_0x068();
    /* slot +0x06C */ virtual void v_0x06C(Mtx dst, const nw4r::lyt::DrawInfo* drawInfo, const nw4r::math::MTX34* glbMtx);

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ HbmLayoutView* layout;
};

/* An eased value: fn_80548394 starts it, hbmFadeUpdate steps it and reports its end to the owner's slot 0x08.
 * size: 0x20 */
struct HbmFade {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ f32 from;
    /* +0x08 */ f32 to;
    /* +0x0C */ f32 time;
    /* +0x10 */ f32 duration;
    /* +0x14 */ u8 active;
    /* +0x15 */ u8 unused_0x15;
    /* +0x16 */ u8 pad_0x16[0x2];
    /* +0x18 */ HbmPanelItemView* owner;
    /* +0x1C */ u32 id;
};

/* The set of the panel's three text buffers (hbmTextBuffersInit builds them, fn_805425E4 is its deleting destructor).
 * size: 0x14 */
class HbmTextBufferSet {
public:
    /* slot +0x008 */ virtual void v_0x008();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ HbmPanelItemView* textField;
    /* +0x08 */ HbmPanelItemView* candidateList;
    /* +0x0C */ HbmPanelItemView* converter;
    /* +0x10 */ MEMAllocator* allocator;
};

/* The glyph box the text writer's slot 0x68 measures: the bounds it fills in for the character `code`.
 * size: 0x14 */
struct HbmCharBox {
    /* +0x00 */ nw4r::ut::Rect bounds;
    /* +0x10 */ u16 code;
    /* +0x12 */ u8 pad_0x12[0x2];
};

/* Where hbmPanelDrawComposition reports the composition's caret.
 * size: 0x0C */
struct HbmCaretSpot {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
};

/* The keyboard settings the panel's owner hands out (its slot 0x48 and the flag words at +0x28 / +0x30).
 * size: 0x34 (approximation: only the compiler-emitted vptr, +0x28 and +0x30 are touched) */
class HbmKbConfigView {
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
    /* slot +0x048 */ virtual s32 v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual void v_0x050();
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual s32 v_0x058();
    /* slot +0x05C */ virtual void v_0x05C(s32 mode);
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
    /* slot +0x138 */ virtual u8 v_0x138();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0x10];
    /* +0x14 */ u32 keyboardType;         /* 1: letters a..z are not typed */
    /* +0x18 */ u32 cursorMode;           /* 0 or 2: the cursor key may be selected */
    /* +0x1C */ u8 unused_0x1C[0xC];
    /* +0x28 */ u32 spaceFlags;           /* low four bits: when a space may start a candidate */
    /* +0x2C */ u32 unused_0x2C;
    /* +0x30 */ u32 keyFlags;             /* bit 2: a space may start a candidate */
    /* +0x34 */ u8 unused_0x34[0x10];
    /* +0x44 */ u8 kanaLocked;            /* the kana key does not toggle */
    /* +0x45 */ u8 unused_0x45[0xBF];
    /* +0x104 */ u8 multiLine;            /* line breaks may be typed */
};

/* The alternative keyboard settings (the owner's slot 0x80): slot 0x40 and the line-break flag at +0xDC.
 * size: 0xE0 (approximation: only the compiler-emitted vptr and +0xDC are touched) */
class HbmKbAltConfigView {
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

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0xD8];
    /* +0xDC */ u8 multiLine;
    /* +0xDD */ u8 pad_0xDD[0x3];
};

/* The keyboard widget the owner's slot 0x90 returns: slots 0xF8 and 0x100 and its input-kind record at +0x18B8.
 * size: 0x18BC (approximation: only the compiler-emitted vptr and +0x18B8 are touched) */
class HbmKbWidgetView {
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
    /* slot +0x0F8 */ virtual s32 v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual s32 v_0x100();

    /* +0x0000 is the compiler-emitted vptr */
    /* +0x0004 */ u8 unused_0x0004[0x18B4];
    /* +0x18B8 */ u32 inputKindRecord;       /* the record hbmKbWidgetSetInputKind updates */
};

/* The owner's slot 0x98 object: slot 0x10 says whether conversions commit directly, slot 0x9C acknowledges Enter.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmKbModeView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual s32 v_0x010();
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
};

/* The panel's owner (+0x19C0, the constructor's second argument): slot 0x78 returns the keyboard settings.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmPanelOwnerView {
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
    /* slot +0x078 */ virtual HbmKbConfigView* v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual HbmKbAltConfigView* v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual HbmKbWidgetView* v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual HbmKbModeView* v_0x098();
};

/* The message records hbmPanelHandleMessage reads for its ids. */

/* Message 0 (a key from the keyboard).
 * size: 0x10 */
struct HbmKeyMessage {
    /* +0x00 */ u16 code;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u32 flags;                /* handed to hbmPanelTypeChar */
    /* +0x08 */ u8 setInputMode;          /* type the key under the keyboard's input mode */
    /* +0x09 */ u8 deadKey;               /* the code is a spacing accent to compose */
    /* +0x0A */ u8 pad_0x0A[0x6];
};

/* Message 5 (a key or a string typed by the panel's owner).
 * size: 0x10 */
struct HbmTypeMessage {
    /* +0x00 */ u16 code;
    /* +0x02 */ u16 value;
    /* +0x04 */ u8 direct;
    /* +0x05 */ u8 pad_0x05;
    /* +0x06 */ u8 silent;
    /* +0x07 */ u8 quiet;                 /* no click sound */
    /* +0x08 */ u32 extra;
    /* +0x0C */ const u16* text;          /* typed instead of the key when set */
};

/* An input mode and the candidate list's two switches, as message 0 sets and restores them.
 * size: 0x08 */
struct HbmInputModeState {
    /* +0x00 */ s32 mode;
    /* +0x04 */ u8 wide;
    /* +0x05 */ u8 small;
    /* +0x06 */ u8 pad_0x06[0x2];
};

/* Messages 14..16 (a pointer at a panel position).
 * size: 0x08 */
struct HbmPointMessage {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
};

/* Messages 29 and 31 (the input mode).
 * size: 0x08 */
struct HbmModeMessage {
    /* +0x00 */ s32 mode;
    /* +0x04 */ u8 latched;
    /* +0x05 */ u8 pad_0x05[0x3];
};

/* The panel's candidate strings at +0x118: a node (target +0x04, count +0x08) and 49 strings of 61 characters.
 * size: 0x1768 */
class HbmCandidateStrings {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u32 target;
    /* +0x08 */ s32 count;
    /* +0x0C */ u16 strings[49][61];
    /* +0x1766 */ u8 pad_0x1766[0x2];
};

/* The listener the keyboard widget tells about every message (its +0x1A18): slots 0x08, 0x0C and 0x20.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmMessageListener {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C(u32 id, u32* arg);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
};

/* The eight-key press record hbmKeyStatesReset clears (the held and pressed flags, the press positions, the codes).
 * size: 0x90 */
struct HbmKeyStates {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u8 locked;
    /* +0x05 */ u8 held[8];
    /* +0x0D */ u8 pressed[8];
    /* +0x15 */ u8 pad_0x15[0x3];
    /* +0x18 */ Vec positions[8];
    /* +0x78 */ u8 unused_0x78[0x8];
    /* +0x80 */ u16 codes[8];
};

/* A key button of the widget's pane manager: slot 0x14 sets its state, +0x2C holds it.
 * size: 0x30 (approximation: only the compiler-emitted vptr and +0x2C are touched) */
class HbmKeyButtonView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010(s32 event);
    /* slot +0x014 */ virtual void v_0x014(s32 state);

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u8 unused_0x04[0x28];
    /* +0x2C */ s32 state;
};

/* The sound player the text panel's pane listener clicks through (slot 0x14 plays a sound).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmSoundView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014(s32 sound);
};

/* The pointer record a pane listener receives (the channel, its position and its button words).
 * size: 0x18 */
struct HbmPointerInfo {
    /* +0x00 */ u32 channel;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
    /* +0x0C */ u32 trigger;
    /* +0x10 */ u32 held;
    /* +0x14 */ u32 released;
};

/* Message 24 (scroll the text by `distance`).
 * size: 0x0C */
struct HbmScrollMessage {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ f32 distance;
};

class HbmPanelView;

/* The text panel's pane listener (vtable 0x8064ECA0: hbmPanelHelperDeletingDtor, hbmTextPaneListenerOnEvent).
 * size: 0x10 (approximation: only the compiler-emitted vptr, +0x08 and +0x0C are touched) */
class HbmTextPaneListener {
public:
    /* slot +0x008 */ virtual void v_0x008();

    /* +0x00 is the compiler-emitted vptr */
    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ HbmSoundView* sound;
    /* +0x0C */ HbmPanelView* widget;
};

/* One line cell of the panel's text: the run of characters one laid-out paragraph holds, linked by index.
 * size: 0x08 */
struct HbmTextCell {
    /* +0x00 */ u16 prev;
    /* +0x02 */ u16 next;
    /* +0x04 */ u16 length;                /* the characters the cell holds */
    /* +0x06 */ u16 lines;                 /* the lines they lay out to */
};

/* The panel's line-cell list at +0x19E4: `count` free cells, then the free-list sentinel at [count] and the used-list
 * sentinel at [count + 1], linked by u16 index.
 * size: 0x08 */
struct HbmTextCellList {
    HbmTextCell* at(u16 index) {
        return &cells[index];
    }
    HbmTextCell* freeList() {
        return &cells[count];
    }
    HbmTextCell* usedList() {
        return &cells[(u16)(count + 1)];
    }
    /* Unlinks `cell` and leaves it linked to itself. */
    void unlink(HbmTextCell* cell) {
        HbmTextCell* list = cells;
        u16 next = cell->next;
        u16 prev = cell->prev;
        u16 index = list[next].prev;

        list[next].prev = prev;
        list[prev].next = next;
        cell->prev = index;
        cell->next = index;
    }
    /* Links the detached `cell` in after `position`. */
    void insertAfter(HbmTextCell* position, HbmTextCell* cell) {
        if (cell->prev == cell->next) {
            HbmTextCell* list = cells;
            u16 index = cell->prev;
            u16 next = position->next;

            cell->prev = list[next].prev;
            cell->next = next;
            position->next = index;
            list[next].prev = index;
        }
    }
    /* Moves the first free cell to the end of the used list. */
    HbmTextCell* allocate() {
        HbmTextCell* cell = at(freeList()->next);

        unlink(cell);
        insertAfter(at(usedList()->prev), cell);
        return cell;
    }
    /* Returns `cell` to the end of the free list. */
    void release(HbmTextCell* cell) {
        unlink(cell);
        insertAfter(at(freeList()->prev), cell);
    }
    /* Moves every used cell back to the free list. */
    void freeAll() {
        HbmTextCell* cell = at(usedList()->prev);

        while (usedList() != cell) {
            HbmTextCell* done = cell;

            cell = at(cell->prev);
            release(done);
        }
    }

    /* +0x00 */ HbmTextCell* cells;
    /* +0x04 */ u16 count;
    /* +0x06 */ u8 pad_0x06[0x2];
};

class HbmPanelView;
extern "C" void hbmPanelOnTextChanged(HbmPanelView* self, u32* from, u32* to);

/* The panel itself (Panel805482CC_Vtbl): the object fn_80542D8C constructs (its last store is +0x1A00), as the keyboard
 * widget extends it from +0x1A04.
 * size: 0x1AF4 (approximation: the keyboard widget's members are known through +0x1AF0) */
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
    /* slot +0x0E0 */ virtual HbmPanelItemView* v_0x0E0();
    /* slot +0x0E4 */ virtual void v_0x0E4();
    /* slot +0x0E8 */ virtual void v_0x0E8();
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual s32 v_0x0F4(s32 atBreak);
    /* slot +0x0F8 */ virtual void v_0x0F8();
    /* slot +0x0FC */ virtual void v_0x0FC();
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104();
    /* slot +0x108 */ virtual void v_0x108();
    /* slot +0x10C */ virtual void v_0x10C();
    /* slot +0x110 */ virtual void v_0x110();
    /* slot +0x114 */ virtual void v_0x114(s32 page);
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual u32 v_0x11C(u32 maxLines, const u16* text);
    /* slot +0x120 */ virtual s32 v_0x120(u32* linkStart, u32* linkEnd, const u16* text, u32 position, u32 end);
    /* slot +0x124 */ virtual s32 v_0x124();
    /* slot +0x128 */ virtual void v_0x128();
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138(u8 on);
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
    /* slot +0x168 */ virtual void v_0x168(f32 x, f32 y);
    /* slot +0x16C */ virtual void v_0x16C(u32* arg);
    /* slot +0x170 */ virtual void v_0x170();
    /* slot +0x174 */ virtual s32 v_0x174(const u16* text, u32 position, f32 x);
    /* slot +0x178 */ virtual void v_0x178(u32 cursor);
    /* slot +0x17C */ virtual void v_0x17C(s32 id);
    /* slot +0x180 */ virtual void v_0x180();
    /* slot +0x184 */ virtual u32 v_0x184(f32 x, f32 y);
    /* slot +0x188 */ virtual Vec2f v_0x188();
    /* slot +0x18C */ virtual Vec2f v_0x18C();
    /* slot +0x190 */ virtual void v_0x190();
    /* slot +0x194 */ virtual void v_0x194();
    /* slot +0x198 */ virtual void v_0x198(u32 id);
    /* slot +0x19C */ virtual void v_0x19C(u32 id);
    /* slot +0x1A0 */ virtual void v_0x1A0();
    /* slot +0x1A4 */ virtual void v_0x1A4();
    /* slot +0x1A8 */ virtual void v_0x1A8(u32* cursor, u32 position, s32 line, s32 lineEnd);
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
    /* slot +0x22C */ virtual s32 v_0x22C();
    /* slot +0x230 */ virtual s32 v_0x230();
    /* slot +0x234 */ virtual void v_0x234();
    /* slot +0x238 */ virtual void v_0x238();
    /* slot +0x23C */ virtual void v_0x23C(s32 a, s32 b);

    /* Types `c` into the text field and books the text change. */
    void typeIntoTextField(u16 c) {
        u32 before = textField->v_0x088();
        u32 after;

        textField->v_0x05C(c);
        after = textField->v_0x088();
        if (before != after) {
            hbmPanelOnTextChanged(this, &before, &after);
        }
    }

    /* Finds the cell holding the text field's cursor and the cursor's offset inside it, then hides the writer's caret
     * and selection. */
    void syncCursorCell() {
        HbmTextCell* cell;
        u32 length;
        u32 total;
        u32 cursor = textField->v_0x088();

        cell = cells.at(cells.usedList()->next);
        cursorCell = cell;
        length = cell->length;
        total = length;
        while (total <= cursor) {
            if (cells.at(cells.usedList()->prev) == cell) {
                break;
            }
            cell = cells.at(cell->next);
            cursorCell = cell;
            length = cell->length;
            total += length;
        }
        cursorOffset = cursor - (total - length);
        writer.v_0x090();
        writer.v_0x094();
    }

    /* +0x0000 is the compiler-emitted vptr */
    /* +0x0004 */ u8 unused_0x0004[0xC];
    /* +0x0010 */ HbmPanelSubView writer;
    /* +0x0118 */ HbmCandidateStrings candidates;            /* the strings the candidate list shows */
    /* +0x1880 */ u8 unused_0x1880[0x78];
    /* +0x18F8 */ nw4r::ut::Rect textRect;                   /* the text box in layout units */
    /* +0x1908 */ Mtx textMtx;                               /* places the text box on the screen */
    /* +0x1938 */ s32 inputKind;                             /* 0, 1 or 2: which keyboard switches hbmPanelRequestKeyboard allows */
    /* +0x193C */ HbmPanelItemView* textField;
    /* +0x1940 */ HbmPanelItemView* candidateList;
    /* +0x1944 */ HbmPanelItemView* converter;
    /* +0x1948 */ u32 unused_0x1948;
    /* +0x194C */ s32 mode;
    /* +0x1950 */ u8 inputLatched;
    /* +0x1951 */ u8 wordWrap;                               /* words that do not fit move to the next line */
    /* +0x1952 */ u8 spaceAtEnd;                             /* moving right past the end types a space */
    /* +0x1953 */ u8 pad_0x1953;
    /* +0x1954 */ f32 caretX;                                /* where hbmPanelDrawCaret last drew the caret */
    /* +0x1958 */ f32 caretY;
    /* +0x195C */ f32 scrollTarget;                          /* where hbmPanelScrollText scrolls to */
    /* +0x1960 */ f32 scrollY;                               /* the text's vertical scroll */
    /* +0x1964 */ HbmPanelScrollView scroll;
    /* +0x1988 */ s32 scrollRequest;                         /* 1 or 2 asks hbmPanelScrollToCaret to bring the caret into view */
    /* +0x198C */ u32 drawPosition;                          /* the character position hbmPanelDrawLineAt draws at */
    /* +0x1990 */ u32 maxLength;                             /* the character limit (512 when constructed) */
    /* +0x1994 */ u32 byteLimit;                             /* the byte-length limit */
    /* +0x1998 */ u32 maxLines;                              /* 9999 when constructed */
    /* +0x199C */ nw4r::ut::Color caretColor;
    /* +0x19A0 */ f32 unused_0x19A0;
    /* +0x19A4 */ u8 textAlpha;
    /* +0x19A5 */ u8 pad_0x19A5[0x3];
    /* +0x19A8 */ nw4r::ut::Color textColor;
    /* +0x19AC */ u8 unused_0x19AC[0x8];
    /* +0x19B4 */ f32 candidateCaretY;
    /* +0x19B8 */ s32 composing;                             /* a composition is in progress (the kana key is ignored) */
    /* +0x19BC */ MEMAllocator* allocator;
    /* +0x19C0 */ HbmPanelOwnerView* owner;                  /* the constructor's second argument */
    /* +0x19C4 */ KPRQueue keyQueue;                         /* the keyboard's character queue */
    /* +0x19DC */ s32 keyboardMode;                          /* the keyboard page hbmPanelSetKeyboardMode selects */
    /* +0x19E0 */ u8 converting;
    /* +0x19E1 */ u8 convertingKana;
    /* +0x19E2 */ u8 pad_0x19E2[0x2];
    /* +0x19E4 */ HbmTextCellList cells;                     /* the text's line cells */
    /* +0x19EC */ MEMAllocator* cellAllocator;               /* the allocator of cells.cells */
    /* +0x19F0 */ HbmTextCell* cursorCell;
    /* +0x19F4 */ HbmTextCell* lastVisibleCell;              /* the last cell hbmPanelLayoutLines hands to the writer */
    /* +0x19F8 */ u16 cursorOffset;                          /* the caret's offset inside cursorCell */
    /* +0x19FA */ u8 wrapLines;                              /* the text wraps through the line cells */
    /* +0x19FB */ u8 pad_0x19FB;
    /* +0x19FC */ u32 minVisibleLines;
    /* +0x1A00 */ u32 unused_0x1A00;
    /* +0x1A04 */ HbmPanelAnimView anim;                     /* the keyboard widget's base at +0x1A04 */
    /* +0x1A0C */ u8 unused_0x1A0C[0x8];
    /* +0x1A14 */ HbmPaneComponentList* paneComponents;
    /* +0x1A18 */ HbmMessageListener* listener;
    /* +0x1A1C */ nw4r::lyt::DrawInfo drawInfo;              /* the state the widget's layout is drawn with */
    /* +0x1A70 */ u8 unused_0x1A70[0x3C];
    /* +0x1AAC */ const char* textBoxName;                   /* the pane hbmPanelGetTextBoxScale measures */
    /* +0x1AB0 */ u8 unused_0x1AB0[0x4];
    /* +0x1AB4 */ u8 scrollUpShown;                          /* the scroll-up button is switched on */
    /* +0x1AB5 */ u8 scrollDownShown;
    /* +0x1AB6 */ u8 separatorShown;                         /* the separator bar pane is shown */
    /* +0x1AB7 */ u8 pad_0x1AB7[0x21];
    /* +0x1AD8 */ s32 repeatBit0001;                         /* auto-repeat countdowns of the direction buttons */
    /* +0x1ADC */ s32 repeatBit4000;
    /* +0x1AE0 */ s32 repeatBit0002;
    /* +0x1AE4 */ s32 repeatBit8000;
    /* +0x1AE8 */ s32 repeatBit1000;
    /* +0x1AEC */ u8 unused_0x1AEC[0x4];
    /* +0x1AF0 */ u32 pressedButtons;                        /* the pressed buttons plus the auto-repeated ones */
};

#ifdef __cplusplus
extern "C" {
#endif

/* The text colours of the composition and the pending key (the TU's static initializer builds them). */
nw4r::ut::Color s_colorPendingKey(200, 50, 50, 255);
nw4r::ut::Color s_colorShadow(20, 20, 20, 255);
nw4r::ut::Color s_colorComposing(255, 50, 50, 255);
nw4r::ut::Color s_colorUnconverted(192, 192, 192, 255);
nw4r::ut::Color s_colorConverted(50, 100, 50, 255);
nw4r::ut::Color s_colorAccent(100, 200, 200, 255);
nw4r::ut::Color s_colorPendingSpace(255, 20, 20, 255);

/* The highlights behind the look-ahead characters and the dead key; their alpha follows the panel's. */
GXColor s_highlightLookahead = {0x80, 0xFF, 0x80, 0xFF};
GXColor s_highlightDeadKey = {0xFF, 0xD2, 0x0C, 0xFF};

/* The caret's blink phase, in degrees (two per frame). */
static f32 s_caretPhase;

/* Whether the word being measured breaks at a hyphen (hbmPanelCheckWordWrap); hbmPanelSetDrawPosition clears it. */
static u8 s_hyphenBreak = 1;

/* Whether hbmPanelNextKey drops the Latin-1 characters the font cannot show. */
static u8 s_filterKeys = 1;

/* The link prefixes hbmPanelFindLink looks for at the start of a word. */
static const u16 s_linkPrefixes[2][10] = {
    {'h', 't', 't', 'p', ':', '/', '/', 0},
    {'h', 't', 't', 'p', 's', ':', '/', '/', 0},
};

/* The space key hbmPanelCaretForward types at the end of the text. */
static const HbmKeyMessage s_spaceKey = {32};

/* The zero text length hbmPanelClearText and hbmPanelReset report the old text with. */
static u32 s_emptyLength = 0;

void hbmPanelOnTextChanged(HbmPanelView* self, u32* from, u32* to);
void hbmPanelCountCellLines(HbmPanelView* self, HbmTextCell* cell, u32 start);
nw4r::ut::Color hbmTextWriterGetTextColor(const HbmPanelSubView* writer);
void hbmPanelHandleSpecialKey(HbmPanelView* self, u32 key, u32 mods);
void hbmPanelArrowLeft(HbmPanelView* self);
void hbmPanelArrowRight(HbmPanelView* self);
void hbmPanelKeyNext(HbmPanelView* self);
void hbmPanelSetConversionMode(HbmPanelView* self, s32 kind);
void hbmNodeNotify(HkbNode* self);
void hbmPanelHandleKey(HbmTextPanel* self, u32 a, s32 b, s32 c, s32 d, s32 e);
void hbmPanelTypeChar(HbmPanelView* self, u16 c, u32 flags);
void hbmPanelTypeIntoTarget(HbmPanelView* self, u16 c);
void hbmPanelDeleteBackward(HbmPanelView* self);
void hbmPanelTruncateText(HbmPanelView* self, u32 length);

/* --- this unit's own entry points, in address order (so the bodies read in the object's order) --- */
void fn_805482CC(HbmTextPanel* self, Panel805482CC_Change* change);
void fn_80548394(Panel805482CC_Set* set, Panel805482CC_Item* item, u32 id, f32 a, f32 b, f32 c);
void fn_805483E4(HbmTextPanel* self);
void fn_80548680(HbmTextPanel* self);
void fn_80548694(void);
void fn_80548698(HbmTextPanel* self);
void fn_805486AC(void);
void hbmPanelCommitInput(HbmTextPanel* self);
s32 fn_8054893C(void);
s32 fn_80548944(void);
void fn_8054894C(void);
void hbmPanelHandleKeyDown(HbmTextPanel* self, u32 key);
void fn_80548B04(void);
void fn_80548B08(HbmTextPanel* self, u16 value);
Panel805482CC_Item* hbmPanelGetInputTarget(HbmTextPanel* self, s32 flag);

/* ---------------------------------------------------------------------------------------------------
 * virtual slots of the band's keyboard classes that do nothing or return a constant
 * ------------------------------------------------------------------------------------------------ */

#pragma push
#pragma auto_inline on
/* 0x80542B68 (0x78): Maps a combining diacritic (U+0300..U+0327) to its spacing form; any other character is kept. */
u16 hbmKbDeadKeyToSpacing(u16 c)
{
    if (c < 0x300 || c > 0x330) {
        return c;
    }
    switch (c) {
    case 0x300:
            return 0x60;
        case 0x301:
            return 0xB4;
        case 0x302:
            return 0x5E;
        case 0x303:
            return 0x7E;
        case 0x308:
            return 0xA8;
        case 0x30D:
            return 0x27;
        case 0x30E:
            return 0x22;
        case 0x327:
            return 0xB8;
    }
    return c;
}

/* 0x805429B4 (0xA0): Peeks the first queued character and returns it in its spacing form. */
u16 hbmKbPeekSpacingChar(KPRQueue* queue)
{
    u16 c;

    KPRLookAhead(queue, (u32)&c, 1);
    return hbmKbDeadKeyToSpacing(c);
}
#pragma pop

/* 0x80542A54 (0x114): Maps a spacing accent to the combining diacritic the keyboard layout composes with it. */
u16 hbmKbSpacingToDeadKey(s32 layout, u16 c)
{
    switch (layout) {
    case 4:
        if (c == 0xB4) {
            return 0x301;
        }
        if (c == 0x60) {
            return 0x300;
        }
        break;
    case 3:
        if (c == 0x5E) {
            return 0x302;
        }
        if (c == 0xA8) {
            return 0x308;
        }
        break;
    case 5:
        break;
    case 6:
        if (c == 0x60) {
            return 0x300;
        }
        if (c == 0xB4) {
            return 0x301;
        }
        if (c == 0x5E) {
            return 0x302;
        }
        if (c == 0x7E) {
            return 0x303;
        }
        if (c == 0xA8) {
            return 0x308;
        }
        break;
    case 7:
        if (c == 0x60) {
            return 0x300;
        }
        if (c == 0x5E) {
            return 0x302;
        }
        if (c == 0x7E) {
            return 0x303;
        }
        if (c == 0x27) {
            return 0x30D;
        }
        if (c == 0x22) {
            return 0x30E;
        }
        break;
    }
    return c;
}

/* 0x80542BE0 (0x9C): Links every cell into the free list and leaves the used list empty. */
void hbmTextCellListInit(HbmTextCellList* list)
{
    u16 i;

    for (i = 0; i < list->count; i++) {
        HbmTextCell* cell = &list->cells[i];

        cell->prev = i - 1;
        cell->next = i + 1;
        cell->length = 0;
        cell->lines = 0;
    }
    {
        HbmTextCell* free = &list->cells[list->count];
        u16 last = list->count - 1;

        free->prev = last;
        free->next = 0;
        list->cells[last].next = list->count;
        list->cells[0].prev = list->count;
    }
    {
        HbmTextCell* used = &list->cells[(u16)(list->count + 1)];

        used->prev = list->count + 1;
        used->next = list->count + 1;
    }
}

#pragma push
#pragma auto_inline on
/* 0x80542C7C (0x110): Moves an item's caret; for the text field, finds the cell holding the new caret and re-lays out. */
void hbmPanelSetItemCaret(HbmPanelView* self, HbmPanelItemView* item, s32 position)
{
    item->v_0x078(position);
    if (item == self->textField) {
        self->syncCursorCell();
    }
}
#pragma pop

/* 0x805431BC (0xC): Hides the text writer's caret. */
void hbmTextWriterHideCaret(HbmPanelSubView* writer)
{
    writer->caretVisible = 0;
}

/* 0x805431C8 (0xC): Hides the text writer's selection. */
void hbmTextWriterHideSelection(HbmPanelSubView* writer)
{
    writer->selectionVisible = 0;
}

/* 0x805431D4 (0x58): Whether the converter is idle: always in modes 0 and 1, otherwise as its slot 0xE4 reports. */
s32 hbmPanelIsConverterIdle(HbmPanelView* self)
{
    if (self->mode >= 2 || self->mode < 0) {
        if (self->converter->v_0x0E4() == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x80543234 (0x11C): Whether the text field may be committed; with `atBreak` set the text must end in a space or a
 * line break. */
s32 hbmPanelCanCommitText(HbmPanelView* self, s32 atBreak)
{
    u8 locked;
    s32 trimmed;
    s32 result;

    self->v_0x018(32, (u32*)&locked);
    if (locked != 0) {
        return 0;
    }
    trimmed = 0;
    if (self->textField->v_0x088() < 1) {
        return 1;
    }
    if (atBreak != 0) {
        switch ((u16)self->textField->v_0x09C()) {
        case 32:
        case 10:
            break;
        default:
            return 0;
        }
        trimmed = self->textField->v_0x074();
    }
    result = self->textField->v_0x0C0();
    if (trimmed != 0) {
        self->textField->v_0x070();
    }
    return result;
}

/* 0x80543350 (0x8): The text buffer's cursor. */
u32 hbmTextGetCursorIndex(HbmPanelItemView* self)
{
    return self->cursor;
}

/* 0x80543358 (0x8): Stores which keyboard switches the panel allows. */
void hbmPanelSetInputKind(HbmPanelView* self, s32 kind)
{
    self->inputKind = kind;
}

/* 0x80543360 (0x108): Stores the keyboard page and shows the matching key layout through slot 0x114. */
void hbmPanelSetKeyboardMode(HbmPanelView* self, u32 mode)
{
    self->keyboardMode = mode;
    switch (mode) {
    case 0:
        self->v_0x114(1);
        break;
    case 1:
        self->v_0x114(2);
        break;
    case 2:
        self->v_0x114(5);
        break;
    case 4:
        self->v_0x114(6);
        break;
    case 5:
        self->v_0x114(9);
        break;
    case 7:
        self->v_0x114(10);
        break;
    case 6:
        if (self->inputKind == 2) {
            self->v_0x114(8);
        } else {
            self->v_0x114(4);
        }
        break;
    case 3:
        if (self->inputKind == 2) {
            self->v_0x114(7);
        } else {
            self->v_0x114(3);
        }
        break;
    }
}

/* 0x80543468 (0xEC): Requests a keyboard switch when the panel's input kind allows the target page. */
void hbmPanelRequestKeyboard(HbmPanelView* self, s32 page)
{
    u32 value;

    switch (self->inputKind) {
    case 2:
        if (page != 5 && page != 6 && page != 7 && page != 8 && page != 9 && page != 10) {
            return;
        }
        break;
    case 0:
        if (page != 1) {
            return;
        }
        break;
    case 1:
        if (page != 2 && page != 3 && page != 4) {
            return;
        }
        break;
    }
    self->v_0x018(31, &value);
    value = page;
    self->v_0x018(29, &value);
}

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

/* 0x80543094 (0x118): Resets the panel's caret, scroll and colour state, its key queue and the writer's fixed width,
 * then refreshes the candidates and the writer. */
void hbmPanelResetState(HbmPanelView* self)
{
    f32 zero = 0.0f;

    self->unused_0x1948 = 0;
    self->caretX = zero;
    self->caretY = zero;
    self->scrollTarget = zero;
    self->scrollY = zero;
    self->scroll.idleTime = zero;
    self->unused_0x19A0 = zero;
    self->textAlpha = 0xFF;
    KPRInitQueue(&self->keyQueue);
    KPRSetQueueMode(&self->keyQueue, 2);
    {
        f32 width = self->writer.mFont->GetWidth();

        self->writer.mIsWidthFixed = false;
        self->writer.mFixedWidth = width;
    }
    self->v_0x138(1);
    ((HbmPanelValueView*)self->candidateList)->v_0x130();
    self->v_0x0D8();
    self->writer.v_0x090();
    self->writer.v_0x094();
    self->writer.scrollY = self->scrollY;
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

#pragma push
#pragma auto_inline on
/* Whether `c` may be typed: the Latin-1 characters the font cannot show are dropped. */
static BOOL hbmIsTypable(u16 c)
{
    u16 dropped[12] = {0xAF, 0xB6, 0xB8, 0xB9, 0xBC, 0xBD, 0xBE, 0xD0, 0xDE, 0xF0, 0xFE, 0};
    s32 i;

    if (c < dropped[0] || c > dropped[10]) {
        return 1;
    }
    for (i = 0; i < 11; i++) {
        if (c == dropped[i]) {
            return 0;
        }
    }
    return 1;
}

/* Takes the next character from the panel's key queue, dropping the characters that may not be typed. */
static u16 hbmPanelNextKey(HbmPanelView* self)
{
    u16 c = KPRGetChar(&self->keyQueue);

    if (s_filterKeys) {
        while (!hbmIsTypable(c)) {
            c = KPRGetChar(&self->keyQueue);
        }
    }
    return c;
}
#pragma pop

/* 0x8054358C (0x221C): The panel's message handler (slot 0x18): runs the base handler, then acts on message `id`
 * with its argument record; most messages end by re-laying the panel out through slot 0xD8. */
void hbmPanelHandleMessage(HbmPanelView* self, u32 id, u32* arg)
{
    HbmPanelItemView* target;
    BOOL targetIsText;
    u16 conversion[64];
    u16 converted[62];
    u16 candidate[64];

    hbmKbEventBroadcast((VuObject*)self, id, arg);
    target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);
    targetIsText = target == self->textField;
    switch (id) {
    case 40:
        if (target == self->candidateList) {
            static HkbKbdState s_kbdState;

            if (s_kbdState.isShiftHeld()) {
                self->v_0x198(id);
            } else {
                self->v_0x19C(id);
            }
            self->scrollRequest = 1;
            return;
        }
        if ((u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
            KPRPutChar(&self->keyQueue, 0xFFFF);
        } else {
            KPRPutChar(&self->keyQueue, 32);
        }
        if (self->keyQueue.ready == 0) {
            self->v_0x17C(10);
        }
        if ((HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0) == self->candidateList) {
            self->candidateList->v_0x0E4();
        }
        for (;;) {
            u16 c = hbmPanelNextKey(self);

            if (c == 0) {
                break;
            }
            if (self->inputLatched && self->mode != 1) {
                hbmPanelTypeChar(self, c, 0);
            } else {
                hbmPanelTypeIntoTarget(self, c);
            }
        }
        break;
    case 0: {
        HbmKeyMessage* key = (HbmKeyMessage*)arg;
        u16 c = key->code;
        u32 flags = key->flags;
        HbmInputModeState current;
        HbmInputModeState saved;

        if (key->setInputMode) {
            current.mode = self->owner->v_0x078()->v_0x058();
            current.small = self->owner->v_0x078()->v_0x058() == 0;
            current.wide = self->owner->v_0x078()->v_0x058() == 1;
            {
                HbmPanelItemView* modeTarget = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);

                saved.mode = modeTarget->inputMode;
                if (modeTarget == self->textField) {
                    u32 before = self->textField->v_0x088();
                    u32 after;

                    hbmTextSetInputMode((HbmTextBuffer*)self->textField, current.mode);
                    after = self->textField->v_0x088();
                    if (before != after) {
                        hbmPanelOnTextChanged(self, &before, &after);
                    }
                } else {
                    hbmTextSetInputMode((HbmTextBuffer*)modeTarget, current.mode);
                }
            }
            if (self->inputLatched ? self->mode == 1 : 0) {
                saved.small = 0;
                saved.wide = 0;
                ((HbmPanelValueView*)self->candidateList)->v_0x134(current.small);
                ((HbmPanelValueView*)self->candidateList)->v_0x128(current.wide);
            }
        }
        if (key->deadKey) {
            c = hbmKbSpacingToDeadKey(self->keyboardMode, c);
        }
        if (self->owner->v_0x078()->keyboardType == 1 && c >= 'a' && c <= 'z') {
            break;
        }
        if (c == 32 && (u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
            KPRPutChar(&self->keyQueue, 0xFFFF);
        } else {
            KPRPutChar(&self->keyQueue, c);
        }
        if (self->keyQueue.ready == 0) {
            self->v_0x17C(10);
        }
        if ((HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0) == self->candidateList) {
            self->candidateList->v_0x0E4();
        }
        for (;;) {
            u16 next = hbmPanelNextKey(self);

            if (next == 0) {
                break;
            }
            if (self->inputLatched && self->mode != 1) {
                hbmPanelTypeChar(self, next, flags);
            } else {
                hbmPanelTypeIntoTarget(self, next);
            }
        }
        if (target == self->converter) {
            self->converting = 0;
        }
        if (key->setInputMode) {
            HbmPanelItemView* modeTarget = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);

            if (modeTarget == self->textField) {
                u32 before = self->textField->v_0x088();
                u32 after;

                hbmTextSetInputMode((HbmTextBuffer*)self->textField, saved.mode);
                after = self->textField->v_0x088();
                if (before != after) {
                    hbmPanelOnTextChanged(self, &before, &after);
                }
            } else {
                hbmTextSetInputMode((HbmTextBuffer*)modeTarget, saved.mode);
            }
            if (self->inputLatched ? self->mode == 1 : 0) {
                ((HbmPanelValueView*)self->candidateList)->v_0x134(saved.small);
                ((HbmPanelValueView*)self->candidateList)->v_0x128(saved.wide);
            }
        }
        if (self->v_0x0F4(1)) {
            self->v_0x018(33, NULL);
        }
        break;
    }
    case 1:
        if (self->inputKind != 0 && (u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
            self->v_0x17C(7);
            KPRClearQueue(&self->keyQueue);
        } else {
            if (target->v_0x090()) {
                self->v_0x17C(7);
            } else {
                self->v_0x17C(8);
            }
            if (target == self->textField) {
                ((HbmPanelValueView*)self->candidateList)->v_0x130();
                self->v_0x0D8();
                hbmPanelDeleteBackward(self);
            } else if (target == self->converter) {
                self->converting = 0;
                target->v_0x068();
            } else {
                if (self->candidateList->v_0x0E4() == 0) {
                    self->candidateList->v_0x068();
                }
                self->v_0x0D8();
            }
        }
        self->scrollRequest = 2;
        if (self->v_0x0F4(1)) {
            self->v_0x018(33, NULL);
        }
        break;
    case 2: {
        s32 deleted;

        if (self->inputKind != 0 && (u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
            self->v_0x17C(8);
            return;
        }
        if (target != self->textField) {
            self->v_0x17C(8);
            return;
        }
        if (self->textField->v_0x094() == 0) {
            deleted = 0;
        } else {
            u16 length = --self->cursorCell->length;

            if (self->cursorOffset == length && self->cells.at(self->cells.usedList()->prev) != self->cursorCell) {
                HbmTextCell* next = self->cells.at(self->cursorCell->next);

                self->cursorCell->length = length + next->length;
                self->cells.release(next);
            }
            hbmPanelCountCellLines(self, self->cursorCell, self->textField->v_0x088() - self->cursorOffset);
            self->writer.v_0x090();
            self->writer.v_0x094();
            deleted = 1;
        }
        if (!deleted) {
            self->v_0x17C(8);
        } else {
            self->v_0x17C(7);
        }
        self->scrollRequest = 1;
        break;
    }
    case 7:
        if (target == self->textField) {
            u8 multiLine;
            s32 newLine;
            s32 refused;

            ((HbmPanelValueView*)self->candidateList)->v_0x130();
            self->v_0x0D8();
            if (self->owner->v_0x098()->v_0x010()) {
                multiLine = self->owner->v_0x078()->multiLine;
            } else {
                multiLine = self->owner->v_0x080()->multiLine;
            }
            newLine = 1;
            if (self->textField->v_0x0CC()) {
                s32 fits = 0;

                if (multiLine && self->maxLines >= (u32)(self->writer.v_0x034() + 1)) {
                    fits = 1;
                }
                if (!fits) {
                    newLine = 0;
                }
            }
            refused = 1;
            if (self->maxLines >= (u32)(self->writer.v_0x034() + 1) && self->owner->v_0x090()->v_0x100() == 0) {
                refused = 0;
            }
            self->owner->v_0x098()->v_0x09C();
            if (!newLine) {
                if (refused) {
                    self->v_0x17C(8);
                }
                return;
            }
            self->v_0x17C(9);
        } else if (target == self->converter) {
            self->v_0x17C(9);
            if (!(self->convertingKana | self->converting) && self->owner->v_0x098()->v_0x010()) {
                if (self->inputLatched && self->mode != 1) {
                    u32 before = self->textField->v_0x088();
                    u32 after;

                    self->textField->v_0x060(conversion);
                    after = self->textField->v_0x088();
                    if (before != after) {
                        hbmPanelOnTextChanged(self, &before, &after);
                    }
                    self->converter->v_0x0D8();
                    self->converting = 0;
                }
            } else {
                hbmPanelCommitInput((HbmTextPanel*)self);
            }
        } else {
            self->v_0x17C(9);
            if (self->candidateList->v_0x0E4()) {
                ((HbmPanelValueView*)self->candidateList)
                    ->v_0x0F4(((HbmPanelValueView*)self->candidateList)->v_0x140());
                self->v_0x0D8();
            } else {
                hbmPanelCommitInput((HbmTextPanel*)self);
            }
        }
        self->scrollRequest = 1;
        if (self->v_0x0F4(1)) {
            self->v_0x018(33, NULL);
        }
        break;
    case 6:
        hbmPanelCommitInput((HbmTextPanel*)self);
        self->scrollRequest = 1;
        break;
    case 3:
        hbmPanelHandleKeyDown((HbmTextPanel*)self, ((HbmKeyMessage*)arg)->code);
        break;
    case 5: {
        HbmTypeMessage* type = (HbmTypeMessage*)arg;
        const u16* text;

        if (type->code == 0) {
            return;
        }
        text = type->text;
        if (text == NULL) {
            hbmPanelHandleKey((HbmTextPanel*)self, type->code, type->direct, type->value, type->silent, type->extra);
        } else {
            u32 before = self->textField->v_0x088();
            u32 after;

            hbmTextPutInMode((HbmTextBuffer*)self->textField, text, 0);
            after = self->textField->v_0x088();
            if (before != after) {
                hbmPanelOnTextChanged(self, &before, &after);
            }
        }
        if (!type->quiet) {
            if (type->silent) {
                self->v_0x17C(9);
            } else {
                self->v_0x17C(10);
            }
        }
        self->scrollRequest = 1;
        if (self->v_0x0F4(1)) {
            self->v_0x018(33, NULL);
        }
        break;
    }
    case 8:
        if (target == self->textField) {
            if (self->inputKind != 0 && (u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
                self->v_0x17C(6);
                return;
            }
            self->v_0x1A0();
            return;
        }
        hbmPanelHandleSpecialKey(self, 31, 0);
        return;
    case 9:
        if (target == self->textField) {
            if (self->inputKind != 0 && (u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
                self->v_0x17C(6);
                return;
            }
            self->v_0x1A4();
            return;
        }
        hbmPanelHandleSpecialKey(self, 32, 0);
        return;
    case 10:
        self->v_0x198(id);
        return;
    case 11:
        self->v_0x19C(id);
        return;
    case 24:
        self->v_0x16C(arg);
        return;
    case 14:
        if (targetIsText && target->v_0x0CC()) {
            HbmPointMessage* point = (HbmPointMessage*)arg;
            Vec2f origin = self->v_0x188();

            hbmPanelSetItemCaret(self, target, self->v_0x184(point->x - origin.x, point->y - origin.y));
            self->v_0x17C(5);
            self->v_0x018(12, NULL);
            ((HbmPanelValueView*)self->candidateList)->v_0x130();
            self->v_0x0D8();
            return;
        }
        self->v_0x17C(9);
        self->v_0x018(6, NULL);
        return;
    case 15:
        if (target->v_0x084()) {
            HbmPointMessage* point = (HbmPointMessage*)arg;
            Vec2f origin = self->v_0x188();

            hbmPanelSetItemCaret(self, target, self->v_0x184(point->x - origin.x, point->y - origin.y));
            self->v_0x018(13, NULL);
        }
        return;
    case 16:
        if (target->v_0x084()) {
            HbmPointMessage* point = (HbmPointMessage*)arg;
            Vec2f origin = self->v_0x188();
            u32 start;
            u32 end;
            u32 position;

            target->v_0x08C(&start, &end);
            position = self->v_0x184(point->x - origin.x, point->y - origin.y);
            hbmPanelSetItemCaret(self, target, position);
            if (start != position) {
                self->v_0x17C(5);
            }
            self->scrollRequest = 1;
        }
        return;
    case 17:
        if (arg != NULL && *(u8*)arg) {
            self->candidateList->v_0x06C(0);
            self->v_0x018(6, NULL);
            self->candidates.count = 0;
            self->candidates.v_0x014();
            hbmNodeNotify((HkbNode*)&self->candidates);
        }
        break;
    case 21:
        if (self->inputLatched) {
            if (self->mode == 1) {
                if (self->candidateList->v_0x0E4()) {
                    ((HbmPanelValueView*)self->candidateList)->v_0x0F4(*arg);
                } else {
                    ((HbmPanelValueView*)self->candidateList)->v_0x0F0(*arg, candidate);
                    {
                        u32 before = self->textField->v_0x088();
                        u32 after;

                        hbmTextPutInMode((HbmTextBuffer*)self->textField, candidate, 0);
                        after = self->textField->v_0x088();
                        if (before != after) {
                            hbmPanelOnTextChanged(self, &before, &after);
                        }
                    }
                    ((HbmPanelValueView*)self->candidateList)->v_0x0F4(*arg);
                }
            } else {
                self->converter->v_0x0E0(*arg, converted);
                {
                    u32 before = self->textField->v_0x088();
                    u32 after;

                    self->textField->v_0x060(converted);
                    after = self->textField->v_0x088();
                    if (before != after) {
                        hbmPanelOnTextChanged(self, &before, &after);
                    }
                }
                self->converter->v_0x0D8();
                self->converting = 0;
            }
        }
        self->scrollRequest = 1;
        break;
    case 18: {
        s32 inputMode = *arg;
        u32 before = self->textField->v_0x088();
        u32 after;

        hbmTextSetInputMode((HbmTextBuffer*)self->textField, inputMode);
        after = self->textField->v_0x088();
        if (before != after) {
            hbmPanelOnTextChanged(self, &before, &after);
        }
        hbmTextSetInputMode((HbmTextBuffer*)self->candidateList, inputMode);
        break;
    }
    case 19:
        ((HbmPanelValueView*)self->candidateList)->v_0x128(*(u8*)arg);
        break;
    case 20:
        ((HbmPanelValueView*)self->candidateList)->v_0x134(*(u8*)arg);
        break;
    case 22:
        hbmPanelSetConversionMode(self, *arg);
        return;
    case 23:
        if (!self->inputLatched) {
            break;
        }
        if (self->mode != 1) {
            return;
        }
        ((HbmPanelValueView*)self->candidateList)->v_0x0F8(-1);
        return;
    case 25:
        target->v_0x0A8();
        self->v_0x17C(10);
        break;
    case 26:
        target->v_0x0B0();
        self->v_0x17C(10);
        break;
    case 27:
        target->v_0x0B4();
        break;
    case 28:
        target->v_0x0BC();
        self->v_0x17C(10);
        break;
    case 29: {
        HbmModeMessage* mode = (HbmModeMessage*)arg;
        s32 textMode;
        s32 candidateMode;

        self->mode = mode->mode;
        self->inputLatched = mode->latched != 0;
        textMode = self->textField->inputMode;
        candidateMode = self->candidateList->inputMode;
        self->textField->v_0x0C4();
        self->candidateList->v_0x0C4();
        {
            u32 before = self->textField->v_0x088();
            u32 after;

            hbmTextSetInputMode((HbmTextBuffer*)self->textField, textMode);
            after = self->textField->v_0x088();
            if (before != after) {
                hbmPanelOnTextChanged(self, &before, &after);
            }
        }
        hbmTextSetInputMode((HbmTextBuffer*)self->candidateList, candidateMode);
        switch (self->mode) {
        case 1:
            if (!self->inputLatched) {
                if (((HbmPanelValueView*)self->candidateList)->v_0x124() == 0) {
                    return;
                }
                if (target != self->candidateList) {
                    return;
                }
                self->candidateList->v_0x06C(0);
                {
                    const u16* selected = self->candidateList->v_0x0E8();
                    u32 before = self->textField->v_0x088();
                    u32 after;

                    self->textField->v_0x06C(selected);
                    after = self->textField->v_0x088();
                    if (before != after) {
                        hbmPanelOnTextChanged(self, &before, &after);
                    }
                }
                self->candidateList->v_0x11C(0);
                self->scrollRequest = 1;
                return;
            }
            ((HbmPanelValueView*)self->candidateList)->v_0x130();
            self->v_0x0D8();
            return;
        case 2:
            self->converter->v_0x0F0(0);
            break;
        case 3:
            self->converter->v_0x0F0(1);
            break;
        case 4:
            self->converter->v_0x0F0(2);
            break;
        case 5:
            self->converter->v_0x0F0(3);
            break;
        case 6:
            self->converter->v_0x0F0(4);
            break;
        case 7:
            self->converter->v_0x0F0(5);
            break;
        case 8:
            self->converter->v_0x0F0(6);
            break;
        case 9:
            self->converter->v_0x0F0(7);
            break;
        case 10:
            self->converter->v_0x0F0(8);
            break;
        }
        self->v_0x018(30, NULL);
        return;
    }
    case 30:
        self->candidateList->v_0x06C(0);
        {
            const u16* selected = self->candidateList->v_0x0E8();
            u32 before = self->textField->v_0x088();
            u32 after;

            self->textField->v_0x06C(selected);
            after = self->textField->v_0x088();
            if (before != after) {
                hbmPanelOnTextChanged(self, &before, &after);
            }
        }
        self->candidateList->v_0x11C(0);
        self->converter->v_0x0D8();
        self->converting = 0;
        self->candidates.count = 0;
        self->candidates.v_0x014();
        return;
    case 31:
        ((HbmModeMessage*)arg)->mode = self->mode;
        ((HbmModeMessage*)arg)->latched = self->inputLatched;
        return;
    case 32: {
        u8 converting;

        if (!self->inputLatched) {
            converting = 0;
        } else if (self->mode != 1 && self->converter->v_0x0DC() > 0) {
            converting = 1;
        } else {
            converting = 0;
        }
        *(u8*)arg = converting;
        return;
    }
    case 38:
        hbmPanelHandleSpecialKey(self, arg[0], arg[1]);
        return;
    case 41:
        if (self->mode != 1) {
            return;
        }
        if (!self->owner->v_0x098()->v_0x010() || !self->owner->v_0x078()->v_0x048()
            || self->owner->v_0x078()->v_0x058() != 0) {
            if (!self->owner->v_0x098()->v_0x010()) {
                self->owner->v_0x080()->v_0x040();
            }
        }
        if ((u16)((HbmPanelValueView*)self->candidateList)->v_0x12C() == 0) {
            self->v_0x0D8();
        }
        return;
    case 42:
    case 43:
    case 44:
    case 45:
        if (self->inputKind != 0 && (u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
            self->v_0x17C(6);
            return;
        }
        switch (id) {
        case 42:
            hbmPanelArrowLeft(self);
            break;
        case 43:
            hbmPanelArrowRight(self);
            break;
        case 44:
            self->v_0x198(40);
            break;
        case 45:
            hbmPanelKeyNext(self);
            break;
        }
        return;
    case 39:
        KPRClearQueue(&self->keyQueue);
        return;
    case 49:
        return;
    case 36:
        hbmPanelTruncateText(self, *arg);
        return;
    case 46:
        self->v_0x17C(10);
        return;
    }
    self->v_0x0D8();
}

/* 0x805457A8 (0x4): Empty virtual slot. */
void hbmPanelSlot17CNop(void)
{
}

/* 0x805457AC (0x4): Empty virtual slot. */
void hbmKbSlot68Nop(void)
{
}

/* 0x805457B0 (0x8): Returns the object's +0x18 word (a virtual slot of a homebutton/hbm_value.cpp class). */
u32 hbmKbGetWord18(HbmWordView* self)
{
    return self->value_0x18;
}

/* 0x805457B8 (0x38): Whether the widget's KPR queue at +0x2C holds no character. */
u32 hbmKbQueueIsEmpty(HkbWidget* self)
{
    return (u8)KPRLookAhead(&self->state, 0, 0) == 0;
}

/* 0x805457F0 (0xC): Returns the text writer's line count (its last line index + 1). */
s32 hbmTextWriterGetLineCount(HbmPanelSubView* writer)
{
    return writer->lastLine + 1;
}

/* 0x805457FC (0x8): Returns the keyboard widget's +0x1B88 byte. */
u8 hbmKbWidgetGetFlag1B88(HkbWidget* self)
{
    return self->flag_1B88;
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

/* 0x80545810 (0x8): Returns whether the text buffer is selecting. */
u8 hbmTextIsSelecting(HbmPanelItemView* self)
{
    return self->selecting;
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

#pragma push
#pragma auto_inline on
/* 0x80545AD0 (0x4C): Appends a candidate string (at most 61 characters). */
void hbmCandidatesAdd(HbmCandidateStrings* self, const u16* text)
{
    wcsncpy(self->strings[self->count], text, 61);
    self->count++;
}
#pragma pop

/* 0x80545860 (0x260): Refills the candidate strings from the candidate list or the converter (unless a listener of
 * message 35 handled them) and notifies the strings' target. */
void hbmPanelRefreshCandidates(HbmPanelView* self)
{
    if (!self->inputLatched) {
        return;
    }
    if (self->mode == 1) {
        if (((HbmPanelValueView*)self->candidateList)->v_0x118()) {
            const u16* selected = self->candidateList->v_0x0E8();
            u32 before = self->textField->v_0x088();
            u32 after;

            self->textField->v_0x06C(selected);
            after = self->textField->v_0x088();
            if (before != after) {
                hbmPanelOnTextChanged(self, &before, &after);
            }
            self->candidateList->v_0x11C(0);
            self->v_0x17C(9);
        }
        {
            u8 handled = 0;

            hbmKbEventBroadcast((VuObject*)self, 35, (u32*)&handled);
            self->candidates.count = 0;
            self->candidates.v_0x014();
            if (!handled) {
                s32 count = ((HbmPanelValueView*)self->candidateList)->v_0x0EC();
                s32 i;

                for (i = 0; i < count; i++) {
                    u16 text[62];

                    ((HbmPanelValueView*)self->candidateList)->v_0x0F0(i, text);
                    hbmCandidatesAdd(&self->candidates, text);
                }
                hbmNodeNotify((HkbNode*)&self->candidates);
            }
        }
    } else {
        s32 count;
        s32 i;

        self->candidates.count = 0;
        self->candidates.v_0x014();
        count = self->converter->v_0x0DC();
        for (i = 0; i < count; i++) {
            u16 text[62];

            self->converter->v_0x0E0(i, text);
            hbmCandidatesAdd(&self->candidates, text);
        }
        hbmNodeNotify((HkbNode*)&self->candidates);
    }
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

/* 0x80545B24 (0x120): Binds the panel to its allocator and text buffers, allocates the line cells and takes the first
 * cell into use. */
void hbmPanelCreate(HbmPanelView* self, MEMAllocator* allocator, HbmTextBufferSet* buffers)
{
    HbmTextCell* cell;

    self->allocator = allocator;
    hbmTextWriterInit((HbmTextWriter*)&self->writer);
    self->textField = buffers->textField;
    self->candidateList = buffers->candidateList;
    self->converter = buffers->converter;
    self->cellAllocator = allocator;
    self->cells.cells = (HbmTextCell*)MEMAllocFromAllocator(allocator, (self->cells.count + 2) * sizeof(HbmTextCell));
    hbmTextCellListInit(&self->cells);
    cell = self->cells.allocate();
    cell->length = 0;
    cell->lines = 1;
    self->cursorCell = cell;
}

/* 0x80545C44 (0x168): Places the text writer at the text origin, clips it to the scaled text rectangle and redraws
 * the text up to the cursor. */
inline void hbmPanelLayoutText(HbmPanelView* self)
{
    f32 y = self->scrollY;
    Vec2f position = self->v_0x18C();
    s32 cursor;

    self->writer.mCursorPos.y = y;
    self->writer.mCursorPos.x = position.x;
    {
        Vec2f scale = self->v_0x18C();
        nw4r::ut::Rect clip;
        Vec2f origin = self->v_0x188();

        clip.left = origin.x;
        clip.bottom = origin.y;
        clip.right = origin.x + scale.x * (self->textRect.right - self->textRect.left);
        clip.top = origin.y + scale.y * (self->textRect.top - self->textRect.bottom);
        cursor = self->textField->v_0x088();
        self->writer.v_0x020(&clip);
    }
    self->v_0x178(cursor);
    self->writer.v_0x024();
}

/* 0x80545DAC (0x20): Returns the text scale (1, 1). */
Vec2f hbmPanelGetTextScale(HbmPanelView* self)
{
    Vec2f scale;

    scale.x = 1.0f;
    scale.y = 1.0f;
    return scale;
}

/* 0x80545DCC (0x44): Draws the line at `position` through the text writer, with the panel's draw position set to it. */
void hbmPanelDrawLineAt(HbmPanelView* self, u32* cursor, u32 position, s32 line, s32 lineEnd)
{
    u32 saved = self->drawPosition;

    self->drawPosition = position;
    hbmTextWriterDrawLine((HbmTextWriter*)&self->writer);
    self->drawPosition = saved;
}

/* 0x80545E10 (0x290): Finds the cells the scrolled text box shows, hands their lines and characters to the text
 * writer, and draws the cells above the window line by line. */
void hbmPanelLayoutLines(HbmPanelView* self, u32* cursor)
{
    s32 first;
    HbmTextCell* cell;
    u32 position;
    s32 start;
    s32 line;
    u32 visible;
    u32 firstPosition;
    s32 firstLine;
    f32 height = __fabsf(self->textRect.bottom - self->textRect.top);
    Vec2f scale = self->v_0x18C();

    visible = (s32)(0.5f + height / (self->writer.v_0x028() * scale.y));

    if (visible < self->minVisibleLines) {
        visible = self->minVisibleLines;
    }
    start = (s32)(0.5f + -self->writer.scrollY / self->writer.v_0x028()) + 1;
    first = start - 1;
    position = 0;
    line = 0;
    cell = &self->cells.cells[self->cells.cells[(u16)(self->cells.count + 1)].next];
    for (;;) {
        if (first <= line + cell->lines) {
            break;
        }
        if (&self->cells.cells[self->cells.cells[(u16)(self->cells.count + 1)].prev] == cell) {
            break;
        }
        if (position <= *cursor && *cursor < position + cell->length) {
            self->v_0x1A8(cursor, position, line, line + cell->lines);
        } else {
            u32 i;

            for (i = 0; i < cell->lines; i++) {
                self->v_0x160();
            }
        }
        line += cell->lines;
        position += cell->length;
        cell = &self->cells.cells[cell->next];
    }
    firstPosition = position;
    firstLine = line;
    for (;;) {
        line += cell->lines;
        position += cell->length;
        if ((s32)(start + visible + 1) <= line) {
            break;
        }
        if (&self->cells.cells[self->cells.cells[(u16)(self->cells.count + 1)].prev] == cell) {
            break;
        }
        cell = &self->cells.cells[cell->next];
    }
    self->lastVisibleCell = cell;
    self->writer.v_0x074(firstLine, line);
    self->writer.v_0x078(start - 2, start + visible - 1);
    hbmTextWriterSetText((HbmTextWriter*)&self->writer, self->textField->v_0x048(), firstPosition, position);
}

/* 0x805460A0 (0x8): Returns the text buffer's characters. */
u16* hbmTextGetText(HbmPanelItemView* self)
{
    return self->text;
}

/* 0x805460A8 (0x140): Draws the cells after the last visible one, one line at a time, or through slot 0x1A8 for the
 * cell that holds the cursor. */
void hbmPanelDrawTrailingLines(HbmPanelView* self, u32* cursor)
{
    HbmTextCell* cell = self->lastVisibleCell;
    s32 line;
    u32 position;
    u8 last;

    if (&self->cells.cells[self->cells.cells[(u16)(self->cells.count + 1)].prev] == cell) {
        return;
    }
    line = self->writer.v_0x088();
    position = self->writer.mark.index;
    last = 0;
    while (!last) {
        cell = &self->cells.cells[cell->next];
        if (&self->cells.cells[self->cells.cells[(u16)(self->cells.count + 1)].prev] == cell) {
            last = 1;
        }
        if (position <= *cursor && *cursor < position + cell->length) {
            self->v_0x1A8(cursor, position, line, line + cell->lines);
        } else {
            u32 i;

            for (i = last; i < cell->lines; i++) {
                self->v_0x160();
            }
        }
        position += cell->length;
        line += cell->lines;
    }
}

/* 0x805461E8 (0x8): Returns the text writer's +0xD0 word. */
u32 hbmTextWriterGetWordD0(HbmPanelSubView* writer)
{
    return writer->unused_0x0D0;
}

/* 0x805461F0 (0x8): Returns the text writer's mark record at +0xC0. */
HbmTextWriterMark* hbmTextWriterGetMark(HbmPanelSubView* writer)
{
    return &writer->mark;
}

/* 0x805461F8 (0x110): Draws the text in the panel's colour: wrapped through the line cells, or as one run. */
void hbmPanelDrawText(HbmPanelView* self, u32 cursor)
{
    Vec2f offset;
    u32 position = cursor;

    offset.x = 0.0f;
    offset.y = 0.0f;
    self->textColor.a = self->textAlpha;
    self->writer.SetTextColor(self->textColor);
    if (self->wrapLines) {
        hbmPanelLayoutLines(self, &position);
        hbmTextWriterDraw((HbmTextWriter*)&self->writer, &position);
        hbmPanelDrawTrailingLines(self, &position);
    } else {
        u32 length = self->textField->v_0x01C();

        hbmTextWriterSetText((HbmTextWriter*)&self->writer, self->textField->v_0x048(), 0, length);
        hbmTextWriterDraw((HbmTextWriter*)&self->writer, &position);
    }
    self->v_0x168(offset.x, offset.y);
}

/* 0x80546308 (0x8): Returns the text buffer's length. */
u16 hbmTextGetLength(HbmPanelItemView* self)
{
    return self->length;
}

/* 0x80546310 (0x294): Advances the scroll animation and the caret's blink colour. */
void hbmPanelUpdate(HbmPanelView* self)
{
    f32 wave;

    if (self->scroll.v_0x014() != 0) {
        self->writer.scrollY = self->scrollY = self->scroll.v_0x010();
    }
    if (self->scroll.v_0x014() == 0) {
        self->v_0x170();
    }
    if (self->textField->v_0x084() == 0) {
        self->scroll.idleTime += 1.0f;
    }
    self->scroll.v_0x00C();
    wave = nw4r::math::SinFIdx(0.7111111f * s_caretPhase);
    self->caretColor.r = 2;
    self->caretColor.g = 90;
    self->caretColor.b = 253;
    self->caretColor.a = 150.0f + 30.0f * wave;
    if (self->textField->v_0x084() != 0) {
        self->caretColor.g = 140;
        self->caretColor.r = 52;
        self->caretColor.b = 253;
        self->caretColor.a = 50.0f + 10.0f * wave;
    } else if (self->scroll.idleTime <= 50.0f) {
        self->caretColor.r = hbmHermite(self->scroll.idleTime, 0.0f, 52.0f, 0.0f, 50.0f, 2.0f, 0.0f);
        self->caretColor.g = hbmHermite(self->scroll.idleTime, 0.0f, 140.0f, 0.0f, 50.0f, 90.0f, 0.0f);
        self->caretColor.b = hbmHermite(self->scroll.idleTime, 0.0f, 253.0f, 0.0f, 50.0f, 253.0f, 0.0f);
        wave = nw4r::math::SinFIdx(0.7111111f * s_caretPhase);
        self->caretColor.a = hbmHermite(self->scroll.idleTime, 0.0f, 50.0f, 0.0f, 50.0f, 150.0f, 0.0f) + 20.0f * wave;
    }
    self->unused_0x1A00 += 8;
    s_caretPhase += 2.0f;
}

/* 0x805465A4 (0x20): Returns the record's current value, eased from `from` to `to` over its duration. */
f32 hbmFadeGetValue(HbmFade* fade)
{
    return hbmHermite(fade->time, 0.0f, fade->from, 0.0f, fade->duration, fade->to, 0.0f);
}

/* 0x805465C4 (0x84): Steps the record one frame; when it ends, reports the end to its owner and stops. */
void hbmFadeUpdate(HbmFade* fade)
{
    if (fade->active) {
        if (fade->time < fade->duration) {
            fade->time += 1.0f;
        } else {
            if (fade->active && fade->owner != NULL) {
                fade->owner->v_0x008(1, fade->id);
            }
            fade->active = 0;
        }
    }
}

/* 0x80546648 (0x10): Advances the panel's +0x1A00 word by 8. */
void hbmPanelAdvanceWord1A00(HbmPanelView* self)
{
    self->unused_0x1A00 += 8;
}

/* 0x80546658 (0x8): Forwards the panel's slot 0xEC to its text writer. */
void hbmPanelWriterRefresh(HbmPanelView* self)
{
    hbmTextWriterRefresh((HbmTextWriter*)&self->writer);
}

#pragma push
#pragma auto_inline on
/* Returns every line cell to the free list, starts one empty cell and books the whole text as typed into it. */
static void hbmPanelRebuildCells(HbmPanelView* self)
{
    HbmTextCell* cell;
    u32 length;

    self->cursorOffset = 0;
    self->cells.freeAll();
    cell = self->cells.allocate();
    cell->length = 0;
    cell->lines = 1;
    self->cursorCell = cell;
    length = self->textField->v_0x01C();
    hbmPanelOnTextChanged(self, &s_emptyLength, &length);
    self->syncCursorCell();
}
#pragma pop

/* 0x80546660 (0x260): Clears the text field, returns every line cell to the free list and starts one empty cell. */
void hbmPanelClearText(HbmPanelView* self)
{
    self->textField->v_0x03C();
    hbmPanelRebuildCells(self);
}

/* 0x805468C0 (0x4C): Sets the text buffer's characters and moves the cursor and the anchor to its end. */
void hbmTextSetAll(HbmPanelItemView* self, const u16* text)
{
    hbmTextSet((HbmTextBuffer*)self, text);
    self->cursor = self->anchor = self->v_0x01C();
}

/* 0x8054690C (0x270): Resets the panel and its text field, then clears the line cells like hbmPanelClearText. */
void hbmPanelReset(HbmPanelView* self)
{
    self->v_0x010();
    self->textField->v_0x018();
    hbmPanelRebuildCells(self);
}

/* 0x80546B7C (0x50): Clears the text buffer, its cursor, anchor and selection, then runs its slot 0xC4. */
void hbmTextReset(HbmPanelItemView* self)
{
    hbmTextClear((HbmTextBuffer*)self);
    self->cursor = 0;
    self->anchor = 0;
    self->selecting = 0;
    self->v_0x0C4();
}

/* 0x80546BCC (0x10): Stores the panel's draw position and stops the caret's blink. */
void hbmPanelSetDrawPosition(HbmPanelView* self, u32 position)
{
    self->drawPosition = position;
    s_hyphenBreak = 0;
}

/* 0x80546BDC (0x1BC): Scans text[position, end) for an "http://" or "https://" link that starts a word; returns
 * whether one was found and reports its first character and the character that ends it. */
s32 hbmPanelFindLink(HbmPanelView* self, u32* linkStart, u32* linkEnd, const u16* text, u32 position, u32 end)
{
    const u16* table = s_linkPrefixes[0];
    s32 found = 0;
    s32 wordStart = 1;

    for (; position < end; position++) {
        switch (text[position]) {
        case 0:
            if (found && linkEnd != NULL) {
                *linkEnd = position;
            }
            return found;
        case 10:
        case 13:
        case 32:
        case 0x3000:
            if (!found) {
                wordStart = 1;
                break;
            }
            if (linkEnd != NULL) {
                *linkEnd = position;
            }
            return 1;
        case 'H':
        case 'h':
            if (wordStart && !found) {
                const u16* prefix = table;
                u32 i;

                wordStart = 0;
                for (i = 0; i < 2; i++) {
                    u32 length = wcslen(prefix);

                    if (wcsnicmp(&text[position], prefix, length) == 0) {
                        if (linkStart != NULL) {
                            *linkStart = position;
                        }
                        found = 1;
                        position = position + length - 1;
                        break;
                    }
                    prefix += 10;
                }
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
            wordStart = 0;
            break;
        default:
            wordStart = 1;
            break;
        }
    }
    return found;
}

/* 0x80546D98 (0x35C): Decides whether the word that starts at the panel's draw position must wrap before `x`: measures
 * the word, notes a hyphen inside it, and moves the draw position past it. */
s32 hbmPanelCheckWordWrap(HbmPanelView* self, const u16* text, u32 position, f32 x)
{
    s32 over;
    const u16* p;
    u32 start;
    s32 prevKind;
    u32 i;
    u32 hyphenAt;
    s32 wrap;
    f32 width;

    if (!self->wordWrap) {
        return 0;
    }
    if (position < self->drawPosition) {
        return 0;
    }
    if (!hbmCanBreakBefore(text[self->drawPosition])) {
        self->drawPosition = position;
        return 0;
    }
    start = self->drawPosition;
    prevKind = 0;
    if (start != 0 && start != self->writer.v_0x08C()) {
        u16 c = text[start - 1];

        if (c == 0x2D) {
            prevKind = 1;
        } else if (c != 0x20) {
            prevKind = 2;
        }
    }
    width = 0.0f;
    hyphenAt = 0;
    start = self->drawPosition;
    wrap = 0;
    i = start;
    p = &text[start];
    do {
        if (*p == 32 || *p == 10) {
            break;
        }
        {
            HbmCharBox box;

            box.code = *p;
            self->writer.v_0x068(&box);
            over = 0;
            width += box.bounds.right - box.bounds.left;
            if (x != self->v_0x18C().x) {
                if (x + width * self->v_0x18C().x >= self->textRect.GetWidth() * self->v_0x18C().x) {
                    over = 1;
                }
            }
            if (over) {
                wrap = 1;
            }
            if (i != start && *p == 0x2D) {
                hyphenAt = i;
                s_hyphenBreak = 1;
            }
        }
        i++;
    } while (*++p != 0);
    if (i == start) {
        s_hyphenBreak = 0;
    }
    {
        if (width * self->v_0x18C().x >= self->textRect.GetWidth() * self->v_0x18C().x) {
            wrap = 0;
            if (s_hyphenBreak) {
                wrap = 1;
            }
            if (prevKind == 1) {
                wrap = 1;
            }
        }
    }
    if (wrap) {
        if (self->v_0x120(NULL, NULL, text, start, i) == 0) {
            if (hyphenAt == 0) {
                s_hyphenBreak = 0;
                self->drawPosition = i + 1;
                return 1;
            }
            i = hyphenAt;
        }
    } else {
        s_hyphenBreak = 0;
    }
    self->drawPosition = i + 1;
    return 0;
}

/* 0x805470F4 (0x20): Returns the caret's character index while the caret is shown, otherwise 0x7FFFFFFF. */
s32 hbmTextWriterGetCaretIndex(HbmPanelSubView* writer)
{
    if (writer->caretVisible) {
        return writer->caretIndex;
    }
    return 0x7FFFFFFF;
}

#pragma push
#pragma auto_inline on
/* 0x805472C4 (0x88): Starts a new line: the next line index, the cursor moved down one line height and back to the
 * left edge. */
void hbmPanelNewLine(HbmPanelView* self)
{
    f32 y = self->writer.mCursorPos.y;

    self->writer.lastLine++;
    self->writer.mCursorPos.y = y + self->writer.v_0x028();
    self->writer.mCursorPos.x = self->v_0x18C().x;
}
#pragma pop

/* 0x80547114 (0x1B0): Before a glyph is drawn: wraps the line when the word or the glyph does not fit, and highlights
 * the glyph when it lies inside the text field's selection. */
void hbmPanelPrepareGlyph(HbmPanelView* self, const u16* text, u32 position, const nw4r::ut::Rect* glyph)
{
    u32 selectionStart;
    u32 selectionEnd;

    if (self->v_0x174(text, position, self->writer.mCursorPos.x)) {
        self->v_0x160();
    }
    if (self->writer.mCursorPos.x + (glyph->right - glyph->left) * self->v_0x18C().x
        >= self->textRect.GetWidth() * self->v_0x18C().x) {
        hbmPanelNewLine(self);
    }
    self->textField->v_0x098(&selectionStart, &selectionEnd);
    if (position >= selectionStart && position < selectionEnd) {
        f32 x = self->writer.mCursorPos.x;
        f32 y = self->writer.mCursorPos.y;

        hbmGuiDrawQuad(self->caretColor, glyph->left + x, glyph->top + y, glyph->right + x, glyph->bottom + y, 0.0f,
                       1.0f);
    }
}

/* 0x8054734C (0x7E8): Draws what follows the caret: the converter's composition (reporting where its caret is), the
 * pending dead key, the queued keyboard character and the text field's look-ahead characters, each on its highlight.
 * Returns whether the composition's caret was placed. */
s32 hbmPanelDrawComposition(HbmPanelView* self, HbmCaretSpot* caret)
{
    s32 caretPlaced = 0;
    nw4r::ut::Color saved = hbmTextWriterGetTextColor(&self->writer);
    const u16* lookahead;
    u16 c;

    self->writer.SetupGX();
    lookahead = self->textField->v_0x0C8();
    c = self->textField->v_0x050();
    if (self->inputLatched && self->mode != 1) {
        const u16* text = self->converter->v_0x0E8();

        if (text != NULL) {
            u16 i = 0;

            self->writer.SetTextColor(s_colorComposing.ToU32());
            while (*text != 0) {
                if (!(self->convertingKana | self->converting)) {
                    if (i == self->converter->v_0x0F8()) {
                        caretPlaced = 1;
                        caret->x = self->writer.mCursorPos.x;
                        caret->y = self->writer.mCursorPos.y;
                    }
                }
                if (self->converter->v_0x0F8() <= i) {
                    if (self->convertingKana | self->converting) {
                        self->writer.SetTextColor(s_colorConverted.ToU32());
                    } else {
                        self->writer.SetTextColor(s_colorUnconverted.ToU32());
                    }
                }
                if (*text == 0xFFFE) {
                    self->writer.SetTextColor(s_colorComposing.ToU32());
                }
                {
                    HbmCharBox box;

                    box.code = *text;
                    self->writer.v_0x068(&box);
                    if (self->writer.mCursorPos.x + (box.bounds.right - box.bounds.left) >= self->textRect.GetWidth()) {
                        self->v_0x160();
                    }
                }
                self->writer.Print(*text);
                text++;
                i++;
                self->writer.MoveCursorX(self->writer.charSpacing);
            }
        }
    }
    if (self->inputKind == 0 && c != 0) {
        HbmCharBox box;
        f32 x;
        f32 y;
        f32 left;
        f32 top;
        f32 right;
        f32 bottom;

        box.code = c;
        self->writer.v_0x068(&box);
        x = self->writer.mCursorPos.x;
        left = box.bounds.left + x;
        y = self->writer.mCursorPos.y;
        right = box.bounds.right + x;
        top = box.bounds.top + y;
        bottom = y + self->writer.GetFontHeight();
        s_highlightDeadKey.a = self->textAlpha;
        hbmGuiDrawQuad(s_highlightDeadKey, left, top, right, bottom, 0.0f, 1.0f);
        self->writer.SetupGX();
        self->writer.SetTextColor(self->textColor);
        switch (c) {
        case 0x3000:
        case 32: {
            HbmCharBox space;
            HbmCharBox mark;
            f32 markWidth;
            f32 spaceWidth;
            f32 pad;

            space.code = c;
            mark.code = 0xE057;
            self->writer.v_0x068(&space);
            self->writer.v_0x068(&mark);
            markWidth = mark.bounds.right - mark.bounds.left;
            spaceWidth = space.bounds.right - space.bounds.left;
            pad = self->v_0x18C().x * ((spaceWidth - markWidth) / 2.0f);
            self->writer.mCursorPos.x += pad;
            self->writer.Print(0xE057);
            self->writer.mCursorPos.x += pad;
            break;
        }
        default:
            self->writer.Print(c);
            break;
        }
        self->writer.MoveCursorX(self->writer.charSpacing);
    }
    if (self->inputKind != 0) {
        u16 key = self->textField->v_0x050();

        if (key != 0) {
            self->writer.SetTextColor(s_colorPendingKey.ToU32());
            if (key == 32) {
                self->writer.SetTextColor(s_colorPendingSpace.ToU32());
                key = 0xE057;
            }
            self->writer.Print(key);
            self->writer.MoveCursorX(self->writer.charSpacing);
        }
        if ((u8)KPRLookAhead(&self->keyQueue, 0, 0) != 0) {
            f32 x;
            f32 y;
            f32 left;
            f32 top;
            f32 right;
            f32 bottom;

            c = hbmKbPeekSpacingChar(&self->keyQueue);
            {
            HbmCharBox box;

            box.code = c;
            self->writer.v_0x068(&box);
            x = self->writer.mCursorPos.x;
            left = box.bounds.left + x;
            right = box.bounds.right + x;
            y = self->writer.mCursorPos.y;
            top = box.bounds.top + y;
            bottom = y + self->writer.GetFontHeight();
            }
            {
                GXColor fill = {0xFF, 0xFA, 0x64, 0xFF};

                hbmGuiDrawQuad(fill, left, top, right, bottom, 0.0f, 1.0f);
            }
            {
                GXColor underline = {0xFF, 0x00, 0x00, 0xFF};

                hbmGuiDrawLine(18, underline, left, bottom, right, bottom, 0.0f);
            }
            self->writer.SetupGX();
            self->writer.SetTextColor(nw4r::ut::Color(0xC83232FF));
            self->writer.SetTextColor(s_colorPendingKey.ToU32());
            self->writer.Print(c);
            self->writer.MoveCursorX(self->writer.charSpacing);
        }
    }
    while ((c = *lookahead) != 0) {
        HbmCharBox box;
        f32 x;
        f32 y;
        f32 left;
        f32 top;
        f32 right;
        f32 bottom;

        if (self->inputLatched) {
            c = hbmToFullWidth(c);
        }
        box.code = c;
        self->writer.v_0x068(&box);
        x = self->writer.mCursorPos.x;
        left = box.bounds.left + x;
        right = box.bounds.right + x;
        y = self->writer.mCursorPos.y;
        top = box.bounds.top + y;
        bottom = y + self->writer.GetFontHeight();
        s_highlightLookahead.a = self->textAlpha;
        hbmGuiDrawQuad(s_highlightLookahead, left, top, right, bottom, 0.0f, 1.0f);
        self->writer.SetupGX();
        self->textColor.a = self->textAlpha;
        self->writer.SetTextColor(self->textColor);
        self->writer.Print(c);
        lookahead++;
        self->writer.MoveCursorX(self->writer.charSpacing);
    }
    self->writer.SetTextColor(saved);
    return caretPlaced;
}

/* 0x80547B34 (0x24): Returns the text writer's text colour (CharWriter::mTextColor.start). */
nw4r::ut::Color hbmTextWriterGetTextColor(const HbmPanelSubView* writer)
{
    return writer->mTextColor.start;
}

/* 0x80547B58 (0x8): Returns the text buffer's look-ahead characters. */
u16* hbmTextGetLookahead(HbmPanelItemView* self)
{
    return self->lookahead;
}

/* 0x80547B60 (0x8): Returns the text buffer's pending dead key. */
u16 hbmTextGetDeadKey(HbmPanelItemView* self)
{
    return self->deadKey;
}

/* 0x80547B68 (0x8): Virtual slot that returns 0. */
s32 hbmTextSlotE8False(void)
{
    return 0;
}

/* 0x80547B70 (0x8): Virtual slot that returns 0. */
s32 hbmTextSlotF8False(void)
{
    return 0;
}

/* 0x80547B78 (0x6C): Returns the sum of the text field's and the candidate list's slot 0x8C counts. */
s32 hbmPanelCountBoth(HbmPanelView* self)
{
    u32 first;
    u32 second;
    u32 unused;

    self->textField->v_0x08C(&first, &unused);
    self->candidateList->v_0x08C(&second, &unused);
    return first + second;
}

/* 0x80547BE4 (0x4): Empty virtual slot. */
void hbmTextSlot8CNop(void)
{
}

/* 0x80547BE8 (0x118): Draws the blinking caret at (x, y) and remembers where it is. */
void hbmPanelDrawCaret(HbmPanelView* self, f32 x, f32 y)
{
    f32 alpha = self->textAlpha / 255.0f;
    f32 width;

    self->caretX = x;
    self->caretY = y;
    {
        GXColor color = {0xFF, 0x32, 0x32, 0x00};

        color.a = alpha * (127.0f + 127.0f * nw4r::math::SinFIdx(0.7111111f * self->unused_0x1A00));
        width = self->writer.clip.GetWidth();
        hbmGuiDrawLine(14592.0f / width, color, self->caretX, 2.0f + self->caretY, self->caretX,
                       self->caretY + self->writer.GetFontHeight() - 2.0f, 0.0f);
    }
}

/* 0x80547D00 (0x56C): Returns the index of the character nearest to the point (x, y), laying the text out line by line
 * the way the panel draws it; a point outside every line retries on the nearest line. */
u32 hbmPanelHitTest(HbmPanelView* self, f32 x, f32 y)
{
    f32 penX;
    f32 lineTop;
    f32 lineBottom;
    f32 py = y - self->scrollY;
    const u16* text = self->textField->v_0x048();
    f32 charRight;
    u32 i;

    if (*text == 0) {
        return 0;
    }
    penX = self->v_0x18C().x;
    lineTop = 0.0f;
    lineBottom = lineTop;
    {
        Vec2f scale = self->v_0x18C();

        i = 0;
        self->drawPosition = 0;
        s_hyphenBreak = 0;
        while (*text != 0) {
            HbmCharBox box;

            box.code = *text;
            self->writer.v_0x068(&box);
            if (box.bounds.right > 10.0f) {
                box.bounds.right = 16.4545f;
            } else {
                box.bounds.right = 9.4727f;
            }
            lineBottom = lineTop + scale.y * (box.bounds.bottom - box.bounds.top);
            charRight = penX + scale.x * (box.bounds.right - box.bounds.left);
            if (self->v_0x174(self->textField->v_0x048(), i, penX)) {
                if (py >= lineTop && py <= lineBottom) {
                    return i != 0 ? i - 1 : i;
                }
                lineTop += scale.y * self->writer.v_0x028();
                penX = self->v_0x18C().x;
                lineBottom = lineTop + scale.y * (box.bounds.bottom - box.bounds.top);
                charRight = penX + scale.x * (box.bounds.right - box.bounds.left);
            }
            if (charRight >= scale.x * self->textRect.GetWidth()) {
                if (py >= lineTop && py <= lineBottom) {
                    return i;
                }
                lineTop += scale.y * self->writer.v_0x028();
                penX = self->v_0x18C().x;
                lineBottom = lineTop + scale.y * (box.bounds.bottom - box.bounds.top);
                charRight = penX + scale.x * (box.bounds.right - box.bounds.left);
            }
            if (x <= box.bounds.left && py >= lineTop && py <= lineBottom) {
                return i;
            }
            if (*text == 10) {
                if (py >= lineTop && py <= lineBottom) {
                    return i;
                }
                lineTop += scale.y * self->writer.v_0x028();
                penX = self->v_0x18C().x;
                lineBottom = lineTop + scale.y * (box.bounds.bottom - box.bounds.top);
                if (text[1] == 0 && py >= lineTop && py <= lineBottom) {
                    return i + 1;
                }
            } else {
                if (x >= penX && x < charRight && py >= lineTop && py < lineBottom) {
                    if (penX + (charRight - penX) * 0.5f > x) {
                        return i;
                    }
                    return i + 1;
                }
                penX += scale.x * (box.bounds.right - box.bounds.left);
            }
            text++;
            i++;
            if (*text == 0 && py >= lineTop && py <= lineBottom) {
                return i;
            }
        }
    }
    if (lineTop + (lineBottom - lineTop) * 0.5f <= py) {
        return hbmPanelHitTest(self, x, 1.0f + lineTop + self->scrollY);
    }
    return hbmPanelHitTest(self, x, 1.0f);
}

/* 0x8054826C (0x60): Returns the text box's bottom-left corner transformed by the panel's matrix, y flipped. */
Vec2f hbmPanelGetTextOrigin(HbmPanelView* self)
{
    Vec corner;
    Vec2f origin;
    f32 left = self->textRect.left;
    f32 bottom = self->textRect.bottom;

    corner.x = left;
    corner.y = bottom;
    corner.z = 0.0f;
    PSMTXMultVec(self->textMtx, &corner, &corner);
    origin.x = corner.x;
    origin.y = -corner.y;
    return origin;
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
        f32 bound = 15.0f;
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
    f32 bound = 0.0f;

    set->field_0x04 = a;
    set->field_0x08 = b;
    set->field_0x10 = c;
    set->field_0x0C = bound;
    set->field_0x14 = 1;
    set->field_0x18 = item;
    set->field_0x1C = id;
    set->field_0x15 = 0;
    if (item != NULL) {
        ((HbmPanelItemView*)item)->v_0x008(0, id);
    }
}

/* 0x805483E4 (0x29C): On a scroll request, scrolls the text so that the caret (or the candidate caret) is inside the
 * text box. */
void hbmPanelScrollToCaret(HbmPanelView* self)
{
    f32 caretY;

    if (self->scroll.v_0x014() != 0) {
        return;
    }
    if ((u32)(self->scrollRequest - 1) > 1) {
        return;
    }
    if (self->candidateList->v_0x0E4() != 0) {
        caretY = self->candidateCaretY + self->writer.v_0x028() * 0.5f;
    } else {
        caretY = self->caretY + self->writer.v_0x028() * 0.5f;
    }
    if (caretY < 0.0f) {
        s32 lines = caretY / self->writer.v_0x028() - 1.0f;

        self->scroll.v_0x008(0, 0, self->scrollY, self->scrollY - lines * self->writer.v_0x028(), 15.0f);
        self->v_0x17C(11);
    }
    if (caretY >= (self->textRect.top - self->textRect.bottom) * self->v_0x18C().y) {
        Vec2f scale = self->v_0x18C();
        s32 lines = (caretY - (self->textRect.top - self->textRect.bottom) * scale.y) / self->writer.v_0x028() + 1;

        self->scroll.v_0x008(0, 0, self->scrollY, self->scrollY - lines * self->writer.v_0x028(), 15.0f);
        self->v_0x17C(11);
    }
    self->scrollRequest = 0;
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
void hbmPanelCommitInput(HbmTextPanel* self)
{
    Panel805482CC_Item* item;

    if (self->field_0x1950 == 0) {
        u32 before;
        u32 after;

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
            const u16* selected;
            u32 after;
            u32 before;

            selected = ((HbmPanelItemView*)self->field_0x1940)->v_0x0E8();
            before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

            item = self->field_0x193C;
            ((HbmPanelItemView*)item)->v_0x06C(selected);
            after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
            if (before != after) {
                hbmPanelOnTextChanged((HbmPanelView*)self, &before, &after);
            }
        }
        item = self->field_0x1940;
        ((HbmPanelItemView*)item)->v_0x11C(0);
    } else {
        const u16* selected;
        u32 after;
        u32 before;

        selected = ((HbmPanelItemView*)self->field_0x1944)->v_0x0E8();
        before = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();

        item = self->field_0x193C;
        ((HbmPanelItemView*)item)->v_0x060(selected);
        after = ((HbmPanelItemView*)self->field_0x193C)->v_0x088();
        if (before != after) {
            hbmPanelOnTextChanged((HbmPanelView*)self, &before, &after);
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
void hbmPanelHandleKeyDown(HbmTextPanel* self, u32 key)
{
    Panel805482CC_Item* item;

    item = hbmPanelGetInputTarget(self, 0);

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
void hbmPanelHandleKey(HbmTextPanel* self, u32 a, s32 b, s32 c, s32 d, s32 e)
{
    Panel805482CC_Item* item;

    item = hbmPanelGetInputTarget(self, 1);

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
            hbmPanelOnTextChanged((HbmPanelView*)self, &before, &after);
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
            hbmPanelOnTextChanged((HbmPanelView*)self, &before, &after);
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

/* 0x80548E40 (0x1A4): Deletes the character before the cursor; at the start of a line, joins the line to the one
 * above. Re-lays out the cursor's cell. */
void hbmPanelDeleteBackward(HbmPanelView* self)
{
    u32 before = self->textField->v_0x088();
    u32 after;

    self->textField->v_0x068();
    after = self->textField->v_0x088();
    if (before != after && before != 0) {
        if (self->cursorOffset == 0) {
            HbmTextCell* prev = self->cells.at(self->cursorCell->prev);

            self->cursorOffset = --prev->length;
            prev->length += self->cursorCell->length;
            self->cells.release(self->cursorCell);
            self->cursorCell = prev;
        } else {
            self->cursorCell->length--;
            self->cursorOffset--;
        }
        hbmPanelCountCellLines(self, self->cursorCell, self->textField->v_0x088() - self->cursorOffset);
        self->writer.v_0x090();
        self->writer.v_0x094();
    }
}

/* 0x80548FE4 (0x2C4): Truncates the text field to `length` characters, shrinking or freeing the trailing line cells,
 * then re-lays the last cell out and finds the cursor's cell again. */
void hbmPanelTruncateText(HbmPanelView* self, u32 length)
{
    u32 removed;
    HbmTextCell* cell;

    if ((u16)self->textField->v_0x01C() > length) {
        removed = (u16)self->textField->v_0x01C() - length;
    } else {
        removed = 0;
    }
    self->textField->v_0x044(length);
    cell = self->cells.at(self->cells.usedList()->prev);
    for (; removed > 0; removed--) {
        s32 empty;

        if (self->cells.at(self->cells.usedList()->prev) == cell) {
            empty = cell->length == 0;
        } else {
            empty = cell->length == 1;
        }
        if (empty) {
            HbmTextCell* prev = self->cells.at(cell->prev);

            prev->length--;
            prev->length += cell->length;
            self->cells.release(cell);
            cell = prev;
        } else {
            cell->length--;
        }
    }
    hbmPanelCountCellLines(self, cell, (u16)self->textField->v_0x01C() - cell->length);
    self->writer.v_0x090();
    self->writer.v_0x094();
    self->syncCursorCell();
}

/* 0x805492A8 (0x2C8): Splits the line cells at every line break typed into [*from, *to), keeps the cursor cell and
 * offset on the cursor, and re-lays out the cells that changed. */
void hbmPanelOnTextChanged(HbmPanelView* self, u32* from, u32* to)
{
    const u16* text = self->textField->v_0x048();
    u32 position = *from;
    s32 changed = 1;

    while (position < *to) {
        if (text[position] == 10) {
            HbmTextCell* cell = self->cells.allocate();
            u32 lineStart;
            u32 end;
            u32 breakEnd;

            cell->length = 0;
            cell->lines = 1;
            self->cells.unlink(cell);
            self->cells.insertAfter(self->cursorCell, cell);
            lineStart = position - self->cursorOffset;
            breakEnd = 0;
            for (end = lineStart; end < (u16)self->textField->v_0x01C(); end++) {
                if (text[end] == 10) {
                    if (breakEnd == 0) {
                        breakEnd = end + 1;
                    } else {
                        end++;
                        break;
                    }
                }
            }
            self->cursorCell->length = breakEnd - lineStart;
            self->cursorCell = cell;
            cell->length = end - breakEnd;
            if (text[end - 1] == 10 && end != breakEnd) {
                position = end - 1;
                if (*to >= end) {
                    self->cursorOffset = self->cursorCell->length - 1;
                } else {
                    self->cursorOffset = *to - breakEnd;
                }
            } else {
                position = end;
                self->cursorOffset = *to - breakEnd;
            }
            changed++;
        } else {
            position++;
            self->cursorCell->length++;
            self->cursorOffset++;
        }
    }
    {
        HbmTextCell* cell = self->cursorCell;
        u32 start = *to - self->cursorOffset;

        do {
            hbmPanelCountCellLines(self, cell, start);
            cell = self->cells.at(cell->prev);
            start -= cell->length;
        } while (--changed != 0);
    }
    self->writer.v_0x090();
    self->writer.v_0x094();
}

/* 0x80549570 (0x20C): Counts the lines the cell's characters (starting at `start` in the text) lay out to. */
void hbmPanelCountCellLines(HbmPanelView* self, HbmTextCell* cell, u32 start)
{
    const u16* text = self->textField->v_0x048() + start;
    const u16* p;
    f32 penX;
    u32 i;

    self->drawPosition = 0;
    s_hyphenBreak = 0;
    penX = self->v_0x18C().x;
    p = text;
    i = 0;
    cell->lines = 1;
    for (; i < cell->length; i++) {
        HbmCharBox box;

        box.code = *p;
        self->writer.v_0x068(&box);
        if (self->v_0x174(text, i, penX)) {
            cell->lines++;
            penX = self->v_0x18C().x;
        }
        if (penX + (box.bounds.right - box.bounds.left) * self->v_0x18C().x
            >= self->textRect.GetWidth() * self->v_0x18C().x) {
            cell->lines++;
            penX = self->v_0x18C().x;
        }
        p++;
        penX += box.bounds.right - box.bounds.left;
    }
}

/* 0x8054977C (0x1E8): Types `c` (converted first unless `flags` & 3 is 0): a space goes straight into the text field,
 * committing a pending conversion first; anything else goes through the key handler. */
void hbmPanelTypeChar(HbmPanelView* self, u16 c, u32 flags)
{
    u32 kind = flags & 3;

    if (kind != 0) {
        c = hbmToWideChar(c);
    }
    if (c == 32) {
        s32 converting;

        if (!self->inputLatched) {
            converting = 0;
        } else if (self->mode != 1 && self->converter->v_0x0DC() > 0) {
            converting = 1;
        } else {
            converting = 0;
        }
        if (converting) {
            hbmPanelCommitInput((HbmTextPanel*)self);
            self->typeIntoTextField(c);
        } else {
            self->typeIntoTextField(c);
        }
        self->v_0x17C(9);
    } else {
        s32 mode;

        switch (kind) {
        case 1:
            mode = 0;
            break;
        case 2:
            mode = 2;
            break;
        default:
            mode = 1;
            break;
        }
        hbmPanelHandleKey((HbmTextPanel*)self, c, 0, mode, 0, 0);
    }
    self->scrollRequest = 1;
}

/* 0x80549964 (0x2EC): Types `c` into the helper the input mode selects: the text field directly, or the candidate
 * list or converter (a space into an empty candidate list only when the keyboard settings allow it). */
void hbmPanelTypeIntoTarget(HbmPanelView* self, u16 c)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 1);

    if ((HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0) == self->candidateList
        && self->candidateList->v_0x0E4() == 0 && ((HbmPanelValueView*)self->candidateList)->v_0x100() >= 0) {
        hbmPanelCommitInput((HbmTextPanel*)self);
    }
    if (self->textField == target) {
        self->typeIntoTextField(c);
    } else if (wcslen(target->v_0x048()) == 0 && c == 32 && self->candidateList == target) {
        if ((self->owner->v_0x078()->v_0x048() != 0 && (self->owner->v_0x078()->spaceFlags & 0xF) == 0)
            || (self->owner->v_0x078()->v_0x048() == 0 && (self->owner->v_0x078()->keyFlags & 4) != 0)) {
            target->v_0x05C(c);
        }
    }
    if (c == 32) {
        self->v_0x17C(9);
    } else if (c != 10) {
        self->v_0x17C(10);
    }
    self->scrollRequest = 1;
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
Panel805482CC_Item* hbmPanelGetInputTarget(HbmTextPanel* self, s32 flag)
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
s32 hbmPanelIsSingleLine(HbmTextPanel* self)
{
    if (self->field_0x1950 == 0) {
        return 0;
    }
    return self->field_0x194C == 1;
}

#pragma push
#pragma auto_inline on
/* Moves the candidate list's conversion `delta` places on (wrapping) and shows it on the keyboard widget. */
static void hbmPanelCycleConversion(HbmPanelView* self, s32 delta)
{
    if ((HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0) == self->candidateList) {
        s16 index = ((HbmPanelValueView*)self->candidateList)->v_0x140() + delta;

        if (index >= ((HbmPanelValueView*)self->candidateList)->v_0x0EC()) {
            index = 0;
        } else if (index < 0) {
            index = ((HbmPanelValueView*)self->candidateList)->v_0x0EC() - 1;
        }
        hbmPanelSetConversionMode(self, index);
        hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
    }
}
#pragma pop

/* 0x80549D9C (0xB40): Handles a special key of the keyboard (`key` 0..36, `mods` the modifier bits): Enter and space
 * commit or cycle the conversion, the paging keys post messages, the kana key switches the keyboard. */
void hbmPanelHandleSpecialKey(HbmPanelView* self, u32 key, u32 mods)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);

    switch (key) {
    case 36:
        if (mods & 0x2F) {
            return;
        }
        if (self->composing != 0) {
            return;
        }
        if ((u8)self->owner->v_0x078()->v_0x138() == 14) {
            return;
        }
        if (self->owner->v_0x078()->cursorMode != 0 && self->owner->v_0x078()->cursorMode != 2) {
            return;
        }
        if (self->v_0x0E0()->v_0x0E4() != 0) {
            return;
        }
        ((HkbKeyboardBase*)self->owner->v_0x078())->selectCursor(0);
        return;
    case 1:
        if (target == self->candidateList) {
            self->scrollRequest = 1;
            self->v_0x0D8();
        }
        return;
    case 9:
        self->v_0x018(7, NULL);
        self->v_0x018(39, NULL);
        return;
    case 8:
        if (self->owner->v_0x078()->keyboardType == 1) {
            return;
        }
        if (self->inputLatched && self->mode != 1) {
            s32 converting;

            if (!self->inputLatched) {
                converting = 0;
            } else if (self->mode != 1 && self->converter->v_0x0DC() > 0) {
                converting = 1;
            } else {
                converting = 0;
            }
            if (converting) {
                if (!(self->convertingKana | self->converting) && self->owner->v_0x098()->v_0x010()) {
                    if (self->inputLatched && self->mode != 1) {
                        u16 text[64];
                        u32 before = self->textField->v_0x088();
                        u32 after;

                        self->textField->v_0x060(text);
                        after = self->textField->v_0x088();
                        if (before != after) {
                            hbmPanelOnTextChanged(self, &before, &after);
                        }
                        self->converter->v_0x0D8();
                        self->converting = 0;
                    }
                } else {
                    hbmPanelCommitInput((HbmTextPanel*)self);
                }
                self->v_0x0D8();
            }
        } else if ((self->inputLatched ? self->mode == 1 : 0) && target == self->candidateList) {
            if (self->candidateList->v_0x0CC() == 0) {
                self->candidateList->v_0x0D4();
            }
            if (self->candidateList->v_0x0E4()) {
                if (self->owner->v_0x090()->v_0x0F8()) {
                    return;
                }
                if (mods & 2) {
                    hbmPanelCycleConversion(self, -1);
                } else {
                    hbmPanelCycleConversion(self, 1);
                }
                self->v_0x17C(6);
                hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
                return;
            }
            if ((u16)((HbmPanelValueView*)self->candidateList)->v_0x12C()) {
                ((HbmPanelValueView*)self->candidateList)->v_0x120();
                self->v_0x0D8();
                hbmPanelSetConversionMode(self, 0);
                hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
                self->v_0x018(49, NULL);
            }
            self->v_0x17C(10);
            return;
        }
        if ((u8)KPRLookAhead(&self->keyQueue, 0, 0)) {
            KPRPutChar(&self->keyQueue, 0xFFFF);
        } else {
            KPRPutChar(&self->keyQueue, 32);
        }
        if (self->inputLatched && self->mode != 1) {
            for (;;) {
                u16 c = hbmPanelNextKey(self);

                if (c == 0) {
                    break;
                }
                hbmPanelTypeChar(self, c, mods);
            }
        } else {
            for (;;) {
                u16 c = hbmPanelNextKey(self);

                if (c == 0) {
                    break;
                }
                hbmPanelTypeIntoTarget(self, c);
            }
        }
        self->v_0x0D8();
        return;
    case 29:
        self->v_0x018(44, NULL);
        return;
    case 30:
        self->v_0x018(45, NULL);
        return;
    case 31:
        self->v_0x018(42, NULL);
        return;
    case 32:
        self->v_0x018(43, NULL);
        return;
    case 16:
    case 17:
        if ((u8)self->owner->v_0x078()->v_0x138() == 14) {
            return;
        }
        if (self->mode != 1) {
            return;
        }
        if (self->owner->v_0x078()->v_0x058() == 1) {
            self->owner->v_0x078()->v_0x05C(2);
        } else if (self->owner->v_0x078()->v_0x058() == 2) {
            self->owner->v_0x078()->v_0x05C(1);
        } else if (self->owner->v_0x078()->kanaLocked == 0) {
            self->owner->v_0x078()->v_0x05C(1);
        }
        return;
    }
}

/* Forward the +0x18B8 record to 0x80553670 (a tail call in retail). */
void fn_8054A8DC(HbmTextPanel* self)
{
    fn_80553670(&self->field_0x18B8);
}

/* 0x8054A8E4 (0x2F8): Moves the text field's cursor one line up (to the start on the top line). */
void hbmPanelCursorUp(HbmPanelView* self)
{
    f32 y = self->caretY;

    if (-(y - self->scrollY) >= 0.0f) {
        u32 start;
        u32 end;

        self->textField->v_0x08C(&start, &end);
        if (start == 0 && end == 0) {
            self->v_0x17C(6);
        } else {
            self->v_0x17C(5);
        }
        hbmPanelSetItemCaret(self, self->textField, 0);
    } else {
        if (!(y >= 0.0f)) {
            y = 0.0f;
        }
        hbmPanelSetItemCaret(self, self->textField, self->v_0x184(self->caretX, 1.0f + (y - self->writer.v_0x028())));
        self->v_0x17C(5);
    }
    self->scrollRequest = 1;
}

/* 0x8054ABDC (0x318): Moves the text field's cursor one line down (to the end on the last line). */
void hbmPanelCursorDown(HbmPanelView* self)
{
    if (-(self->caretY - self->scrollY) < self->writer.textHeight) {
        u32 start;
        u32 end;

        self->textField->v_0x08C(&start, &end);
        if (start == (u16)self->textField->v_0x01C() && end == (u16)self->textField->v_0x01C()) {
            self->v_0x17C(6);
        } else {
            self->v_0x17C(5);
        }
        hbmPanelSetItemCaret(self, self->textField, (u16)self->textField->v_0x01C());
    } else {
        hbmPanelSetItemCaret(self, self->textField,
                             self->v_0x184(self->caretX, 1.0f + (self->caretY + self->writer.v_0x028())));
        self->v_0x17C(5);
    }
    self->scrollRequest = 1;
}

/* 0x8054AEF4 (0x3CC): The left key: cycles the candidate list's conversion back, or moves the text cursor up. */
void hbmPanelKeyLeft(HbmPanelView* self, s32 key)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);

    if (self->converter == target) {
        return;
    }
    if (self->candidateList == target) {
        if (key == 40 && self->candidateList->v_0x0CC() == 0) {
            self->candidateList->v_0x0D4();
        }
        if (self->candidateList->v_0x0E4()) {
            if (self->owner->v_0x090()->v_0x0F8()) {
                return;
            }
            hbmPanelCycleConversion(self, -1);
            self->v_0x17C(6);
            return;
        }
        if (key == 40) {
            if ((u16)((HbmPanelValueView*)self->candidateList)->v_0x12C()) {
                ((HbmPanelValueView*)self->candidateList)->v_0x120();
                self->v_0x0D8();
                hbmPanelSetConversionMode(self, 0);
                hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
                self->v_0x018(49, NULL);
            }
            self->v_0x17C(10);
        }
        return;
    }
    if (target == self->textField) {
        if (target->v_0x054()) {
            return;
        }
        if (target->v_0x0CC()) {
            ((HbmPanelValueView*)self->candidateList)->v_0x130();
            self->v_0x0D8();
            if ((u16)self->candidateList->v_0x050() != 32) {
                self->v_0x190();
                self->v_0x018(49, NULL);
            }
        }
    }
}

/* 0x8054B2C0 (0x3CC): The right key: cycles the candidate list's conversion on, or moves the text cursor down. */
void hbmPanelKeyRight(HbmPanelView* self, s32 key)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);

    if (self->converter == target) {
        return;
    }
    if (self->candidateList == target) {
        if (key == 40 && self->candidateList->v_0x0CC() == 0) {
            self->candidateList->v_0x0D4();
        }
        if (self->candidateList->v_0x0E4()) {
            if (self->owner->v_0x090()->v_0x0F8()) {
                return;
            }
            hbmPanelCycleConversion(self, 1);
            self->v_0x17C(6);
            return;
        }
        if (key == 40) {
            if ((u16)((HbmPanelValueView*)self->candidateList)->v_0x12C()) {
                ((HbmPanelValueView*)self->candidateList)->v_0x120();
                self->v_0x0D8();
                hbmPanelSetConversionMode(self, 0);
                hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
                self->v_0x018(49, NULL);
            }
            self->v_0x17C(10);
        }
        return;
    }
    if (target == self->textField) {
        if (target->v_0x054()) {
            return;
        }
        if (target->v_0x0CC()) {
            ((HbmPanelValueView*)self->candidateList)->v_0x130();
            self->v_0x0D8();
            if ((u16)self->candidateList->v_0x050() != 32) {
                self->v_0x194();
                self->v_0x018(49, NULL);
            }
        }
    }
}

/* 0x8054B68C (0x24C): Moves the input target's caret one character back (slot 0x1A0), keeping the text field's
 * cursor cell and offset. */
void hbmPanelCaretBack(HbmPanelView* self)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);
    u32 before = target->v_0x088();

    if (target == self->textField) {
        if (target->v_0x054()) {
            return;
        }
        if (!target->v_0x0CC()) {
            return;
        }
        ((HbmPanelValueView*)self->candidateList)->v_0x130();
        self->v_0x0D8();
        if ((u16)self->candidateList->v_0x050() == 32) {
            return;
        }
    }
    if (target->v_0x074()) {
        self->v_0x17C(5);
    } else {
        self->v_0x17C(6);
    }
    if (target == self->candidateList) {
        self->v_0x0D8();
    }
    self->scrollRequest = 1;
    if (target == self->textField && before != target->v_0x088()) {
        if (self->cursorOffset == 0 && self->cursorCell != self->cells.at(self->cells.usedList()->next)) {
            self->cursorCell = self->cells.at(self->cursorCell->prev);
            self->cursorOffset = self->cursorCell->length;
        }
        self->cursorOffset--;
    }
}

/* 0x8054B8D8 (0x2F8): Moves the input target's caret one character on (slot 0x1A4); at the end of the text it types a
 * space instead when the panel asks for that. */
void hbmPanelCaretForward(HbmPanelView* self)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);
    u32 before = target->v_0x088();
    s32 track = 1;
    s32 moved;

    if (target == self->textField) {
        if (target->v_0x054()) {
            return;
        }
        if (!target->v_0x0CC()) {
            return;
        }
        ((HbmPanelValueView*)self->candidateList)->v_0x130();
        self->v_0x0D8();
        if ((u16)self->candidateList->v_0x050() == 32) {
            return;
        }
    }
    moved = target->v_0x070();
    if (moved && self->spaceAtEnd) {
        HbmKeyMessage key = s_spaceKey;

        self->v_0x018(0, (u32*)&key);
        self->v_0x17C(5);
        track = 0;
    }
    if (target == self->converter || target == self->candidateList) {
        self->v_0x17C(6);
    } else if (moved) {
        self->v_0x17C(6);
    } else {
        self->v_0x17C(5);
    }
    if (target == self->candidateList) {
        self->v_0x0D8();
    }
    self->scrollRequest = 1;
    if (track && target == self->textField && before != target->v_0x088()) {
        if (++self->cursorOffset == self->cursorCell->length
            && self->cells.at(self->cells.usedList()->prev) != self->cursorCell) {
            self->cursorOffset = 0;
            self->cursorCell = self->cells.at(self->cursorCell->next);
        }
    }
}

/* 0x8054BBD0 (0x414): The next key: moves the text cursor down, or commits/cycles the candidate list's conversion. */
void hbmPanelKeyNext(HbmPanelView* self)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);
    u32 modifiers = hkbGetModifierState(&hkbManager);

    if (target == self->textField) {
        ((HbmPanelValueView*)self->candidateList)->v_0x130();
        self->v_0x0D8();
        if (target->v_0x054()) {
            return;
        }
        if (target->v_0x0CC()) {
            if ((u16)self->candidateList->v_0x050() != 32) {
                self->v_0x194();
                self->v_0x018(49, NULL);
            }
        }
    } else if (self->candidateList == target) {
        if (self->candidateList->v_0x0CC() == 0) {
            self->candidateList->v_0x0D4();
        }
        if (self->candidateList->v_0x0E4()) {
            if (modifiers & 2) {
                ((HbmPanelValueView*)self->candidateList)
                    ->v_0x0F4(((HbmPanelValueView*)self->candidateList)->v_0x140());
                self->v_0x0D8();
                return;
            }
            if (self->owner->v_0x090()->v_0x0F8()) {
                return;
            }
            hbmPanelCycleConversion(self, 1);
            self->v_0x17C(6);
            return;
        }
        if ((u16)((HbmPanelValueView*)self->candidateList)->v_0x12C()) {
            ((HbmPanelValueView*)self->candidateList)->v_0x120();
            self->v_0x0D8();
            hbmPanelSetConversionMode(self, 0);
            hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
            self->v_0x018(49, NULL);
        }
        self->v_0x17C(10);
    } else if (self->converter == target) {
    }
}

/* 0x8054BFE4 (0x430): The left arrow: moves the text cursor one character back (keeping its cell and offset), or
 * selects the previous candidate or conversion. */
void hbmPanelArrowLeft(HbmPanelView* self)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);
    u32 before;

    hkbGetModifierState(&hkbManager);
    before = target->v_0x088();
    if (target == self->textField) {
        if (target->v_0x054()) {
            return;
        }
        if (!target->v_0x0CC()) {
            return;
        }
        ((HbmPanelValueView*)self->candidateList)->v_0x130();
        self->v_0x0D8();
        if (target->v_0x074()) {
            self->v_0x17C(5);
        } else {
            self->v_0x17C(6);
        }
    } else if (target == self->candidateList) {
        if (self->owner->v_0x090()->v_0x0F8()) {
            return;
        }
        if (((HbmPanelValueView*)self->candidateList)->v_0x0EC() <= 0) {
            self->v_0x17C(5);
            return;
        }
        if (self->candidateList->v_0x0E4()) {
            self->v_0x1A0();
            hbmPanelSetConversionMode(self, 0);
            return;
        }
        {
            s16 index = ((HbmPanelValueView*)self->candidateList)->v_0x100() - 1;

            if (index < -1) {
                index = 0;
            } else if (index < 0) {
                index = ((HbmPanelValueView*)self->candidateList)->v_0x0EC() - 1;
            }
            hbmPanelSetConversionMode(self, index);
        }
        hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
        self->v_0x17C(6);
    } else if (target == self->converter) {
        u8 converting;
        s32 index;

        if (self->owner->v_0x090()->v_0x0F8()) {
            return;
        }
        converting = self->converting;
        self->converting = 1;
        if (self->converter->v_0x0DC() < 0) {
            return;
        }
        index = converting ? -2 : -1;
        if (index < 0) {
            index = self->converter->v_0x0DC() - 1;
        }
        hbmPanelSetConversionMode(self, index);
        hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
        self->v_0x17C(6);
    } else {
        return;
    }
    self->scrollRequest = 1;
    if (target == self->textField && before != target->v_0x088()) {
        if (self->cursorOffset == 0 && self->cursorCell != self->cells.at(self->cells.usedList()->next)) {
            self->cursorCell = self->cells.at(self->cursorCell->prev);
            self->cursorOffset = self->cursorCell->length;
        }
        self->cursorOffset--;
    }
}

/* 0x8054C414 (0x450): The right arrow: moves the text cursor one character on (keeping its cell and offset), or
 * selects the next candidate or conversion. */
void hbmPanelArrowRight(HbmPanelView* self)
{
    HbmPanelItemView* target = (HbmPanelItemView*)hbmPanelGetInputTarget((HbmTextPanel*)self, 0);
    u32 before;

    hkbGetModifierState(&hkbManager);
    before = target->v_0x088();
    if (target == self->textField) {
        if (target->v_0x054()) {
            return;
        }
        if (!target->v_0x0CC()) {
            return;
        }
        ((HbmPanelValueView*)self->candidateList)->v_0x130();
        self->v_0x0D8();
        if (target->v_0x070()) {
            self->v_0x17C(6);
        } else {
            self->v_0x17C(5);
        }
    } else if (target == self->candidateList) {
        if (self->owner->v_0x090()->v_0x0F8()) {
            return;
        }
        if (((HbmPanelValueView*)self->candidateList)->v_0x0EC() <= 0) {
            self->v_0x17C(5);
            return;
        }
        if (self->candidateList->v_0x0E4()) {
            target->v_0x070();
            self->v_0x17C(6);
            self->v_0x0D8();
            hbmPanelSetConversionMode(self, 0);
            return;
        }
        {
            s16 index = ((HbmPanelValueView*)self->candidateList)->v_0x100() + 1;

            if (index >= ((HbmPanelValueView*)self->candidateList)->v_0x0EC()) {
                index = 0;
            }
            hbmPanelSetConversionMode(self, index);
        }
        hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
        self->v_0x17C(6);
    } else if (target == self->converter) {
        u8 converting;
        s32 index;

        if (self->owner->v_0x090()->v_0x0F8()) {
            return;
        }
        converting = self->converting;
        self->converting = 1;
        if (self->converter->v_0x0DC() < 0) {
            return;
        }
        index = converting ? 0 : -1;
        if (index >= self->converter->v_0x0DC()) {
            index = 0;
        }
        hbmPanelSetConversionMode(self, index);
        hbmKbCandShowSelected((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord);
        self->v_0x17C(6);
    } else {
        return;
    }
    self->scrollRequest = 1;
    if (target == self->textField && before != target->v_0x088()) {
        if (++self->cursorOffset == self->cursorCell->length
            && self->cells.at(self->cells.usedList()->prev) != self->cursorCell) {
            self->cursorOffset = 0;
            self->cursorCell = self->cells.at(self->cursorCell->next);
        }
    }
}

/* 0x8054C864 (0x1C4): Switches the conversion to candidate `kind`: commits the candidate list's selection first when it
 * is shown, then hands the kind to the list or the converter and to the keyboard widget. */
void hbmPanelSetConversionMode(HbmPanelView* self, s32 kind)
{
    if (self->inputLatched) {
        if (self->owner->v_0x090()->v_0x0F8()) {
            return;
        }
        if (self->mode == 1) {
            if (((HbmPanelValueView*)self->candidateList)->v_0x144() && self->candidateList->v_0x0E4() == 0) {
                self->candidateList->v_0x06C(0);
                {
                    const u16* selected = self->candidateList->v_0x0E8();
                    u32 before = self->textField->v_0x088();
                    u32 after;

                    self->textField->v_0x06C(selected);
                    after = self->textField->v_0x088();
                    if (before != after) {
                        hbmPanelOnTextChanged(self, &before, &after);
                    }
                }
                self->candidateList->v_0x11C(0);
            }
            ((HbmPanelValueView*)self->candidateList)->v_0x0F8(kind);
        } else {
            self->converter->v_0x0F4(kind);
        }
        hbmKbCandSelect((HbmKbCandidateList*)&self->owner->v_0x090()->inputKindRecord, kind);
    } else {
        self->scrollRequest = 1;
    }
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

/* 0x8054CB48 (0x274): Hides the writer's selection and caret and re-lays the whole text out. */
void hbmPanelRelayout(HbmPanelView* self)
{
    self->writer.v_0x094();
    self->writer.v_0x090();
    hbmPanelRebuildCells(self);
}

/* Store the incoming word into the panel's +0x1988 field. */
void fn_8054CDBC(HbmTextPanel* self, s32 value)
{
    self->field_0x1988 = value;
}

/* Forward the +0x19C4 record to 0x80526D00 (a tail call in retail). */
void fn_8054CDC4(HbmTextPanel* self)
{
    KPRClearQueue((KPRQueue*)&self->field_0x19C4);
}

/* 0x8054CDCC (0x138): In the candidate mode, closes the candidate window the keyboard settings ask for and refreshes
 * the candidates when the list holds none. */
void hbmPanelCloseCandidates(HbmPanelView* self)
{
    if (self->mode == 1) {
        if (!self->owner->v_0x098()->v_0x010() || !self->owner->v_0x078()->v_0x048()
            || self->owner->v_0x078()->v_0x058() != 0) {
            if (!self->owner->v_0x098()->v_0x010()) {
                self->owner->v_0x080()->v_0x040();
            }
        }
        if ((u16)((HbmPanelValueView*)self->candidateList)->v_0x12C() == 0) {
            self->v_0x0D8();
        }
    }
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

/* 0x8054D634 (0x1DC): Resets the keyboard widget: the panel state, the two scroll buttons, the key repeat, the text
 * colour and the text box, whose spacing, rectangle and matrix the writer takes over. */
void hbmKbWidgetReset(HbmPanelView* self)
{
    nw4r::lyt::TextBox* box;

    hbmPanelResetState(self);
    self->anim.v_0x05C("P_txtScrll_UP")->v_0x014(7);
    self->anim.v_0x05C("P_txtScrll_DOWN")->v_0x014(7);
    self->scrollUpShown = 0;
    self->scrollDownShown = 0;
    self->repeatBit0001 = 0;
    self->repeatBit4000 = 0;
    self->repeatBit0002 = 0;
    self->repeatBit8000 = 0;
    self->pressedButtons = 0;
    self->textColor = s_colorShadow;
    self->v_0x23C(0, 0);
    box = (nw4r::lyt::TextBox*)self->anim.layout->rootPane->v_0x03C(self->textBoxName, 1);
    box->SetString(L"", 0);
    self->writer.charSpacing = box->mCharSpace;
    self->writer.lineSpace = box->mLineSpace;
    self->writer.fontWidth = box->mFontSize.width;
    self->writer.fontHeight = box->mFontSize.height;
    self->textRect = box->GetPaneRect(self->drawInfo);
    self->anim.v_0x06C(self->textMtx, &self->drawInfo, &box->mGlbMtx);
    self->anim.layout->v_0x038(0);
    self->anim.layout->v_0x030(&self->drawInfo);
    self->writer.textHeight = 1.0f + self->scrollY;
}

/* Store the incoming byte into the panel's +0x1AB6 field. */
void fn_8054D810(HbmTextPanel* self, u8 value)
{
    self->field_0x1AB6 = value;
}

/* 0x8054D818 (0x1D8): Updates the keyboard widget's panes, takes the root pane's alpha and the text box's matrix, and
 * lays the text out. */
void hbmKbWidgetCalc(HbmPanelView* self)
{
    nw4r::lyt::Pane* box;

    ((HbmPaneManager*)&self->anim)->HbmPaneManager::calcComponents();
    self->textAlpha = ((nw4r::lyt::Pane*)self->anim.v_0x028()->rootPane)->mAlpha;
    box = (nw4r::lyt::Pane*)self->anim.layout->rootPane->v_0x03C(self->textBoxName, 1);
    self->anim.v_0x06C(self->textMtx, &self->drawInfo, &box->mGlbMtx);
    hbmPanelLayoutText(self);
}

/* 0x8054D9F0 (0x200): Updates the panel and draws the widget's panes; shows the separator bar and switches the two
 * scroll buttons on while the text can scroll that way. */
void hbmKbWidgetUpdate(HbmPanelView* self)
{
    s32 canScrollUp = 0;
    s32 canScrollDown = 0;

    hbmPanelUpdate(self);
    ((HbmPaneManager*)&self->anim)->HbmPaneManager::drawComponents();
    if (self->anim.v_0x02C("N_separateBarAll")) {
        self->anim.v_0x050("N_separateBarAll", self->separatorShown);
    }
    if (self->scroll.v_0x014() != 0) {
        return;
    }
    if (0.0f > self->scrollY) {
        canScrollUp = 1;
    }
    if (self->writer.textHeight <= self->scrollY) {
        canScrollDown = 1;
    }
    if (self->scrollUpShown != canScrollUp) {
        self->scrollUpShown = canScrollUp;
        if (canScrollUp) {
            self->anim.v_0x05C("P_txtScrll_UP")->v_0x010(6);
            ((HbmPaneComponentView*)hkbGetPaneComponentByName((PaneManager*)self->paneComponents, "P_txtScrll_UP"))->v_0x010();
        } else {
            self->anim.v_0x05C("P_txtScrll_UP")->v_0x010(7);
        }
    }
    if (self->scrollDownShown != canScrollDown) {
        self->scrollDownShown = canScrollDown;
        if (canScrollDown) {
            self->anim.v_0x05C("P_txtScrll_DOWN")->v_0x010(6);
            ((HbmPaneComponentView*)hkbGetPaneComponentByName((PaneManager*)self->paneComponents, "P_txtScrll_DOWN"))->v_0x010();
        } else {
            self->anim.v_0x05C("P_txtScrll_DOWN")->v_0x010(7);
        }
    }
}

/* 0x8054DBF0 (0xD8): Releases every key of the record unless it is locked. */
void hbmKeyStatesReset(HbmKeyStates* self)
{
    s32 i;

    if (self->locked) {
        return;
    }
    for (i = 0; i < 8; i++) {
        self->held[i] = 0;
        self->positions[i].x = 0.0f;
        self->positions[i].y = 0.0f;
        self->positions[i].z = 0.0f;
        self->pressed[i] = 0;
        self->codes[i] = 0;
    }
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

/* 0x8054DDD8 (0x284): Handles the pointer and the buttons for the widget's panes: the direction buttons page or move
 * through the candidates, a held trigger drags the selection, a press in the text box moves the cursor. */
s32 hbmKbWidgetInput(HbmPanelView* self, u32 unused1, u32 unused2, u32 trigger, u32 held, f32 x, f32 y)
{
    s32 hit = ((HbmPaneManager*)&self->anim)->HbmPaneManager::updateComponents();

    if (self->candidateList->v_0x0E4() != 0 || hit != 0) {
        u32 pressed = self->pressedButtons;

        if (pressed & 0x0002) {
            self->v_0x018(8, NULL);
        }
        if (pressed & 0x8000) {
            self->v_0x018(9, NULL);
        }
        if (self->candidateList->v_0x0E4() != 0) {
            if (pressed & 0x0001) {
                self->v_0x018(10, NULL);
            }
            if (pressed & 0x4000) {
                self->v_0x018(11, NULL);
            }
        }
    }
    if ((held & 0x10) && self->textField->v_0x084()) {
        self->v_0x018(13, NULL);
    }
    if (hit == 0 && (trigger & 0x10) && self->textField->v_0x084()) {
        HbmPointMessage point;

        point.x = x;
        point.y = y;
        if (-y < (self->textRect.top - self->textRect.bottom) * 0.5f + self->v_0x188().y) {
            point.y = self->v_0x188().y - 1.0f;
        } else {
            point.y = 1.0f + ((self->textRect.top - self->textRect.bottom) + self->v_0x188().y);
        }
        self->v_0x018(16, (u32*)&point);
    }
    return hit;
}

/* Forwards the key release to the +0x1A04 pane manager (a tail call in retail). */
void fn_8054E05C(void* panel, HkbManager* manager)
{
    HbmTextPanel* self = (HbmTextPanel*)panel;
    ((HbmPaneManager*)&self->field_0x1A04)->HbmPaneManager::onKeyRelease(manager);
}

/* 0x8054E180 (0x48): Returns the scale of the text box pane the keyboard widget names. */
Vec2f hbmPanelGetTextBoxScale(HbmPanelView* self)
{
    return self->anim.layout->rootPane->v_0x03C(self->textBoxName, 1)->scale;
}

/* 0x8054E1C8 (0x35C): The keyboard widget's message handler (slot 0x18): tells the listener, runs the panel's
 * handler, then keeps the text inside the byte, character and line limits. */
void hbmKbWidgetHandleMessage(HbmPanelView* self, s32 id, u32* arg)
{
    self->listener->v_0x008();
    self->listener->v_0x00C(id, arg);
    hbmPanelHandleMessage(self, id, arg);
    if (id == 31) {
        return;
    }
    {
        u32 limit = self->byteLimit;

        if (limit < (u16)self->textField->v_0x034()) {
            u32 end = self->textField->v_0x038(limit);
            u32 start;
            u32 stop;

            self->textField->v_0x08C(&start, &stop);
            if (start > end || stop > end) {
                hbmPanelHandleMessage(self, 36, &end);
            }
        } else if (self->maxLength < (u16)self->textField->v_0x01C()) {
            u32 start;
            u32 stop;

            self->textField->v_0x08C(&start, &stop);
            if (start > self->maxLength || stop > self->maxLength) {
                hbmPanelHandleMessage(self, 36, &self->maxLength);
            }
        }
    }
    switch (id) {
    case 32:
    case 33:
    case 35:
        break;
    default: {
        u32 limit = self->byteLimit;

        if (limit < (u16)self->textField->v_0x034()) {
            hbmPanelTruncateText(self, self->textField->v_0x038(limit));
            self->listener->v_0x020();
            if (id == 6 || id == 5 || id == 0 || id == 38 || id == 21 || id == 7) {
                self->v_0x17C(8);
            }
        } else {
            limit = self->maxLength;
            if (limit < (u16)self->textField->v_0x01C()) {
                hbmPanelTruncateText(self, limit);
                self->listener->v_0x020();
                if (id == 6 || id == 5 || id == 0 || id == 38 || id == 21 || id == 7) {
                    self->v_0x17C(8);
                }
            }
        }
        break;
    }
    }
    {
        u32 overflow = self->v_0x11C(self->maxLines, self->textField->v_0x048());

        if (overflow != 0) {
            hbmPanelTruncateText(self, overflow);
            self->listener->v_0x020();
            self->v_0x17C(30);
            hbmPanelHandleMessage(self, 36, &overflow);
        }
    }
}

/* The u16 at +0x008. */
u16 fn_8054E524(HbmTextPanel* self)
{
    return self->field_0x008;
}

/* 0x8054E52C (0x2DC): Returns the index of the first character of `text` that lays out past `maxLines` lines, or 0
 * when the whole text fits. */
u32 hbmPanelFindLineOverflow(HbmPanelView* self, u32 maxLines, const u16* text)
{
    const u16* p = text;
    f32 x;
    u32 i;
    u32 lines;

    if (*text == 0) {
        return 0;
    }
    x = self->v_0x18C().x;
    Vec2f scale = self->v_0x18C();
    i = 0;
    lines = 0;
    self->drawPosition = 0;
    s_hyphenBreak = 0;
    while (*p != 0) {
        HbmCharBox box;
        f32 right;

        box.code = *p;
        self->writer.v_0x068(&box);
        right = x + scale.x * (box.bounds.right - box.bounds.left);
        if (self->v_0x174(text, i, x)) {
            if (++lines >= maxLines) {
                for (; i < self->drawPosition; i++) {
                    f32 width = scale.x * (box.bounds.right - box.bounds.left);

                    if (x + width >= scale.x * self->textRect.GetWidth()) {
                        return i;
                    }
                    box.code = *++p;
                    x += width;
                    self->writer.v_0x068(&box);
                }
                return self->drawPosition - 1;
            }
            x = self->v_0x18C().x;
        } else if (right >= scale.x * self->textRect.GetWidth()) {
            x = self->v_0x18C().x;
            if (++lines >= maxLines) {
                return i;
            }
        }
        if (*p == 10) {
            x = self->v_0x18C().x;
            if (++lines >= maxLines) {
                return i;
            }
            if (p[1] == 0) {
                return 0;
            }
        } else {
            x += scale.x * (box.bounds.right - box.bounds.left);
        }
        i++;
        p++;
    }
    return 0;
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
    hbmTextWriterSetStandardScreen((HbmTextWriter*)&self->sub, 0);
}

/* Close the +0x1A04 record and then the sub-object's layout at mode 1. */
void fn_8054E858(void* panel)
{
    HbmTextPanel* self = (HbmTextPanel*)panel;
    ((HbmPaneManager*)&self->field_0x1A04)->HbmPaneManager::v_0x040();
    hbmTextWriterSetStandardScreen((HbmTextWriter*)&self->sub, 1);
}

/* --------------------------------------------------------------------------------------------- */
/* Bodies (address order)                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* 0x8054E894 (0x628): The text panel's pane listener (slot 0x18): the scroll buttons scroll the text a line per press
 * and repeat while held, the other buttons switch their state, and the text box takes the pointer. */
void hbmTextPaneListenerOnEvent(HbmTextPaneListener* self, HbmPaneComponentView* component, u32 event,
                                HbmPointerInfo* info)
{
    const char* name = component->pane->mName;
    HbmPointMessage point;

    point.x = info->x;
    point.y = -info->y;
    if (name[0] == 'B') {
        char paneName[17];
        HbmPaneButtonView* button;

        hbmNameCopyWithChar(paneName, 17, name, 0, 'P');
        if (event == 4 && (info->trigger & 0x10)) {
            if (self->widget->v_0x124() == 0) {
                if (self->widget->v_0x22C()) {
                    if (hbmNameMatches("P_txtScrll_UP", paneName)) {
                        HbmScrollMessage scroll = {0, 0, 0.0f};

                        scroll.distance = self->widget->writer.v_0x028();
                        self->widget->v_0x018(24, (u32*)&scroll);
                        self->widget->anim.v_0x05C(paneName)->v_0x010(0);
                    }
                } else if (self->widget->v_0x230()) {
                    if (hbmNameMatches("P_txtScrll_DOWN", paneName)) {
                        HbmScrollMessage scroll = {0, 0, 0.0f};

                        scroll.distance = -self->widget->writer.v_0x028();
                        self->widget->v_0x018(24, (u32*)&scroll);
                        self->widget->anim.v_0x05C(paneName)->v_0x010(0);
                    }
                }
            }
        }
        if (event == 2 && (info->held & 0x10) && !(info->trigger & 0x10)) {
            if (self->widget->v_0x22C()) {
                if (hbmNameMatches("P_txtScrll_UP", paneName)) {
                    if (component->v_0x028(info->channel)) {
                        u32 heldFrames = self->widget->anim.v_0x030(info->channel, name);

                        if (heldFrames >= 60 && heldFrames % 20 == 0) {
                            HbmScrollMessage scroll = {0, 0, 0.0f};

                            scroll.distance = self->widget->writer.v_0x028();
                            self->widget->v_0x018(24, (u32*)&scroll);
                            self->widget->anim.v_0x05C(paneName)->v_0x010(0);
                        }
                    }
                }
            } else if (self->widget->v_0x230()) {
                if (hbmNameMatches("P_txtScrll_DOWN", paneName)) {
                    if (component->v_0x028(info->channel)) {
                        u32 heldFrames = self->widget->anim.v_0x030(info->channel, name);

                        if (heldFrames >= 60 && heldFrames % 20 == 0) {
                            HbmScrollMessage scroll = {0, 0, 0.0f};

                            scroll.distance = -self->widget->writer.v_0x028();
                            self->widget->v_0x018(24, (u32*)&scroll);
                            self->widget->anim.v_0x05C(paneName)->v_0x010(0);
                        }
                    }
                }
            }
        }
        button = self->widget->anim.v_0x05C(paneName);
        if (button != NULL) {
            switch (event) {
            case 1:
                button->v_0x010(2);
                break;
            case 0:
                if (button->v_0x028() != 7) {
                    self->sound->v_0x014(4);
                    button->v_0x010(1);
                }
                break;
            }
        }
    } else if (name[0] == 'P') {
        HbmPaneButtonView* button = self->widget->anim.v_0x05C(name);

        if (button != NULL) {
            switch (event) {
            case 0:
                if (button->v_0x028() != 7) {
                    self->sound->v_0x014(4);
                    button->v_0x010(1);
                }
                break;
            case 1:
                if (button->v_0x028() != 7) {
                    self->sound->v_0x014(4);
                    button->v_0x010(2);
                }
                break;
            }
        }
    } else if (hbmNameMatches(name, "T_2l_TextBox")) {
        if (event == 5 && (info->released & 0x10)) {
            self->widget->v_0x018(15, (u32*)&point);
        }
        if (event == 2 && (info->held & 0x10)) {
            self->widget->v_0x018(16, (u32*)&point);
        }
    }
}

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


/* 0x8054EED4 (0x24C): Steps a key button's state machine with `event` (slot 0x10 of the key class). */
void hbmKeyOnEvent(HbmKeyButtonView* self, s32 event)
{
    if (event == 6 && self->state == 7) {
        self->v_0x014(5);
    } else if (event == 7 && self->state != 7) {
        self->v_0x014(6);
    } else if (event == 0) {
        self->v_0x014(4);
    }
    switch (self->state) {
    case 5:
        if (event == 4) {
            self->v_0x014(0);
        }
        if (event == 1) {
            self->v_0x014(1);
        }
        break;
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
    case 6:
        if (event == 4) {
            self->v_0x014(7);
        }
        break;
    }
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

/* 0x8054F18C (0x250): Switches word wrapping on or off and re-lays the whole text out. */
void hbmPanelSetWordWrap(HbmPanelView* self, u8 on)
{
    self->wordWrap = on;
    hbmPanelRebuildCells(self);
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

/* Adjustor thunk: base #2 -> hbmPanelGetTextScale. */
extern "C" Vec2f fn_8054F6AC(HkbWidget* self) {
    return hbmPanelGetTextScale((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> hbmPanelLayoutText. */
#pragma push
#pragma dont_inline on
extern "C" void fn_8054F6B4(HkbWidget* self) {
    hbmPanelLayoutText((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}
#pragma pop

/* Adjustor thunk: base #7 -> fn_8054CF38. */
/* The keyboard widget's destructor (0x8054CF38); its body is not written yet. */
extern "C" void fn_8054CF38(HkbWidget* self, s32 flag);

extern "C" void fn_8054F6BC(HkbWidget* self, s32 flag) {
    fn_8054CF38((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)), flag);
}

/* Adjustor thunk: base #2 -> hbmPanelDrawCaret. */
extern "C" void fn_8054F6C4(HkbWidget* self, f32 x, f32 y) {
    hbmPanelDrawCaret((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)), x, y);
}

/* Adjustor thunk: base #2 -> hbmPanelDrawLineAt. */
extern "C" void fn_8054F6CC(HkbWidget* self, u32* cursor, u32 position, s32 line, s32 lineEnd) {
    hbmPanelDrawLineAt((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)), cursor, position, line, lineEnd);
}

/* Adjustor thunk: base #2 -> hbmPanelDrawComposition. */
extern "C" s32 fn_8054F6D4(HkbWidget* self, HbmCaretSpot* caret) {
    return hbmPanelDrawComposition((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)), caret);
}

/* Adjustor thunk: base #2 -> hbmPanelNewLine. */
#pragma push
#pragma dont_inline on
extern "C" void fn_8054F6DC(HkbWidget* self) {
    hbmPanelNewLine((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}
#pragma pop

/* Adjustor thunk: base #2 -> fn_8054F130. */
extern "C" void fn_8054F6E4(HkbWidget* self) {
    fn_8054F130((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> hbmPanelSetDrawPosition. */
extern "C" void fn_8054F6EC(HkbWidget* self, u32 position) {
    hbmPanelSetDrawPosition((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)), position);
}

/* Adjustor thunk: base #2 -> fn_8054F12C. */
extern "C" void fn_8054F6F4(HkbWidget* self) {
    fn_8054F12C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> hbmPanelPrepareGlyph. */
extern "C" void fn_8054F6FC(HkbWidget* self, const u16* text, u32 position, const nw4r::ut::Rect* glyph) {
    hbmPanelPrepareGlyph((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)), text, position, glyph);
}

/* Adjustor thunk: base #2 -> hbmPanelGetTextBoxScale. */
extern "C" Vec2f fn_8054F704(HkbWidget* self) {
    return hbmPanelGetTextBoxScale((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> hbmPanelWriterRefresh. */
extern "C" void fn_8054F70C(HkbWidget* self) {
    hbmPanelWriterRefresh((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
}

/* Adjustor thunk: base #2 -> hbmKbWidgetCalc. */
extern "C" void fn_8054F714(HkbWidget* self) {
    hbmKbWidgetCalc((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_10)));
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

/* Adjustor thunk: base #7 -> hbmKbWidgetInput. */
extern "C" s32 fn_8054F734(HkbWidget* self, u32 unused1, u32 unused2, u32 trigger, u32 held, f32 x, f32 y) {
    return hbmKbWidgetInput((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)), unused1, unused2, trigger, held, x, y);
}

/* Adjustor thunk: base #7 -> fn_8054E05C. */
extern "C" void fn_8054F73C(HkbWidget* self, HkbManager* manager) {
    fn_8054E05C((HkbWidget*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)), manager);
}

/* Adjustor thunk: base #7 -> hbmKbWidgetUpdate. */
extern "C" void fn_8054F744(HkbWidget* self) {
    hbmKbWidgetUpdate((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> hbmKbWidgetCalc. */
extern "C" void fn_8054F74C(HkbWidget* self) {
    hbmKbWidgetCalc((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Adjustor thunk: base #7 -> hbmKbWidgetReset. */
extern "C" void fn_8054F754(HkbWidget* self) {
    hbmKbWidgetReset((HbmPanelView*)((u8*)self - HKB_OFFSET_OF(HkbWidget, vtable_1A04)));
}

/* Dispatches the target's own update with the node's argument. */
extern "C" void hbmNodeNotify(HkbNode* self) {
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
