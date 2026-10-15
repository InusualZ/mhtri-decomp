/*
 * homebutton/hbm_kb_cursor.cpp - software-keyboard code of the tiHKBManager band: the keyboard's cursor and input block
 * (arrow parts, key-slot focus, the character popup, the anchored cursor position), the guarded part forwarders, the
 * parts' deleting destructors and the adjustor thunks.
 *
 * RANGE.     .text 0x80566440..0x80569DAC (85 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the keyboard cursor; no `__FILE__` string, no runtime-dump name.
 * NAMES.     GUESS function names hbmKbCursor* (read off the bodies: the parts and slots they drive) and view classes
 *            HbmCursorTailView / HbmCursorKeyView / HbmCursorPartView / HbmCursorPartArgView / HbmCursorPaneView /
 *            HbmCursorSelfView / HbmCursorSlotView / HbmCursorPopupView / HbmCursorScrollView (slots named by offset) and
 *            records HbmCursorInput / HbmCursorAnchor; map rows renamed with them (request hbm-misc).
 * SHAPES.    The views are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch; no
 *            method is defined and nothing is constructed here, so no table is emitted (rule 10).
 * SHAPES.    fn_80568DD0 returns the anchor's Vec2f member by value (retail loads the two words straight into r3/r4).
 * RESIDUALS. fn_80568004: retail rounds the parameter before negating it (`frsp f0,f1; fneg f0,f0`, 40 vs 36 bytes);
 *            f64 parameters, casts and inline negation helpers all keep the plain `fneg`.
 * RESIDUALS. fn_80568C60: retail ends with a dead `blr` after its tail call (76 vs 72 bytes); a returned value, an
 *            inline helper and a guarded call do not produce it.
 * RESIDUALS. Unwritten, need nw4r::lyt::TexMap's `TexMap(const GXTexObj&)` constructor and its palette reset in
 *            homebutton/hkb_texmap.h (another unit's header, request hbm-misc#94): fn_805696D0.
 * SHAPES.    hbmKbCursorScrollToCaret halves the row height as `/ 2.0f` (retail's fmuls operand order) and tests the request
 *            as `(u32)(request - 1) > 1` (retail's subi + cmplwi); its 0.5 and the int-to-float bias are compiler pool
 *            entries, so they carry our pool's names, not 0x8079D76C/0x8079D758.
 * SHAPES.    The text block at +0x10 has its vptr at its own +0x4C (HbmCursorTextView derives from a 0x4C-byte head).
 * RESIDUALS. fn_805690F0 (the scroll reset): retail keeps a five-float local (sp+8..0x18) beside the pane translation
 *            and re-derives the +0x1A04 base per call; three local shapes tried stay 35 to 61 words off - unwritten.
 * RESIDUALS. Unwritten, call unnamed functions of other units (hbm_text_panel.cpp, hbm_value.cpp, hbm_hermite.cpp's
 *            hbmNameMatches, fn_8052A040.cpp) or read this unit's .data name tables: fn_80566440, fn_80566618, fn_80566740,
 *            fn_80566910, fn_80566970, fn_80566A7C, fn_80566C94, fn_80567014, fn_8056725C, fn_80567A14, fn_80567D40,
 *            fn_80567E70, fn_8056802C, fn_80568CAC, fn_80568E08, fn_80568EBC, fn_80568FF0, fn_805690F0, hbmKbCursorSetMode,
 *            fn_805698BC, fn_8056999C, and the thunks fn_80569D4C, fn_80569D74, fn_80569D7C, fn_80569D8C..fn_80569DA4
 *            whose targets are among them.
 * NAMES.     GUESS classes HbmKbCursorPart, its kinds HbmKbCursorPartA/B and HbmKbCursorKey (band keys with a state
 *            word; members reset, setState, getResource, getState).
 */

#include "types.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_kb_object.h"
#include "homebutton/hbm_kb_cursor.h"
#include "homebutton/hbm_widget.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hbm_value.h"

/* A cursor part: a band key with its state word; its class table (0x80657EC0) and the out-of-line copy of its inline destructor (0x80567204) are
 * this unit's.  size: 0x34 */
class HbmKbCursorPart : public HbmKey {
public:
    virtual void reset();
    virtual void setState(u32 state);
    virtual ~HbmKbCursorPart() {}
    /* untyped: opaque handle passed through */
    virtual void* getResource();
    virtual u32 getState();

    /* +0x30 */ void* mpResource;
}; /* size: 0x34 */

/* A HbmKbCursorPart kind (class table 0x80657E68): its own slot 0x10 are defined above as C functions.  size: 0x34 */
class HbmKbCursorPartA : public HbmKbCursorPart {
public:
    virtual void setAnim(s32 anim);
    virtual ~HbmKbCursorPartA();
};

/* A HbmKbCursorPart kind (class table 0x80657E94): its own slot 0x10 are defined above as C functions.  size: 0x34 */
class HbmKbCursorPartB : public HbmKbCursorPart {
public:
    virtual void setAnim(s32 anim);
    virtual ~HbmKbCursorPartB();
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
    /* slot +0x17C */ virtual void v_0x17C(s32 event);
    /* slot +0x180 */ virtual void v_0x180();
    /* slot +0x184 */ virtual void v_0x184();
    /* slot +0x188 */ virtual void v_0x188();
    /* slot +0x18C */ virtual void v_0x18C();
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
    /* slot +0x2A0 */ virtual void v_0x2A0();
    /* slot +0x2A4 */ virtual f32 v_0x2A4();
    /* slot +0x2A8 */ virtual f32 v_0x2A8();
    /* slot +0x2AC */ virtual f32 v_0x2AC();
};

/* The popup at +0x1BCC (KbTail +0x1C8): slot 0x08 says whether it is open, 0x0C opens and 0x10 closes it.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorPopupView {
public:
    /* slot +0x008 */ virtual s32 v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
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
};

/* The scroller at +0x1BAC as the scroll handlers use it: slot 0x08 starts a scroll from one position to another over a
 * number of frames, slot 0x14 says whether one is running.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HbmCursorScrollerView {
public:
    /* slot +0x008 */ virtual void v_0x008(s32 a, s32 b, f32 from, f32 to, f32 frames);
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
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
    /* +0x003F */ u8 unused_0x003F[0x1919];
    /* +0x1958 */ f32 caretTop;
    /* +0x195C */ u8 unused_0x195C[0x2C];
    /* +0x1988 */ s32 scrollRequest;
    /* +0x198C */ u8 unused_0x198C[0x178];
    /* +0x1B04 */ f32 position;
    /* +0x1B08 */ u8 unused_0x1B08[0x08];
    /* +0x1B10 */ s32 rows;
    /* +0x1B14 */ u8 unused_0x1B14[0x98];
    /* +0x1BAC */ u32 scroll_1BAC;
    /* +0x1BB0 */ u8 unused_0x1BB0[0x1C];
    /* +0x1BCC */ HbmCursorPopupView* popup;
    /* +0x1BD0 */ u8 unused_0x1BD0[0x08];
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
}; /* size: 0x1BEC */

/* The anchor object at KbTail +0x0F4: its position pair at +0x44 is returned by value.
 * size: 0x4C (approximation: only +0x44..+0x4C is touched) */
struct HbmCursorAnchor {
    /* +0x00 */ u8    unused_0x00[0x44];
    /* +0x44 */ Vec2f pos;
}; /* size: 0x4C */

extern "C" {

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

void fn_80567D3C(void* p, HkbManager* manager) { fn_8054E05C(p, manager); }

/* ---------------------------------------------------------------------------------------------------
 * the 0x1B04.. key-frame block
 * ------------------------------------------------------------------------------------------------ */
void fn_80568004(KbObject* self, f32 value)
{
    f32 out;

    self->tail_0x1A04.float_0x100 = value;
    if (self->tail_0x1A04.field_0x1E4 == 2)
        out = -value;
    else
        out = lbl_8079D754;
    self->float_0x100 = out;
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
    (void)fn_8055B174(&self->field_0x010, &self->tail_0x1A04.float_0x110);
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

void fn_8056985C(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((HbmCursorPartView*)self->tail_0x1A04.field_0x1D0)->v_0x018();
    hbmPanelSetDrawPosition(self, arg);
}

void fn_80569910(KbObject* self, void* arg)
{
    if (self->tail_0x1A04.field_0x1E4 == 1)
        ((HbmCursorPartView*)self->tail_0x1A04.field_0x1D0)->v_0x010();
    fn_8055A888((HbmWidget*)&self->field_0x010, (int)arg);
}

void fn_80569970(KbObject* self)
{
    if (self->tail_0x1A04.field_0x1E4 != 1)
        return;
    ((HbmCursorPartArgView*)self->tail_0x1A04.field_0x1D0)->v_0x010(0);
}

void fn_80569998(KbObject* self) { (void)self; }

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


/* Returns the part's resource word. */
/* untyped: opaque handle passed through */
void* HbmKbCursorPart::getResource() {
    return mpResource;
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
void hbmKbCursorAdj10Deactivate(u8* base, u32 arg) { fn_80569910((KbObject*)(base - 0x10), (void*)arg); }

/* 0x80569D54 (0x8): +0x10 base -> fn_80569970. */
void hbmKbCursorAdj10Release(u8* base) { fn_80569970((KbObject*)(base - 0x10)); }

/* 0x80569D5C (0x8): +0x10 base -> fn_8056985C. */
void hbmKbCursorAdj10Activate(u8* base, u32 arg) { fn_8056985C((KbObject*)(base - 0x10), (void*)arg); }

/* 0x80569D64 (0x8): +0x10 base -> fn_80568DD0. */
Vec2f hbmKbCursorAdj10Anchor(u8* base) { return fn_80568DD0((KbObject*)(base - 0x10)); }

/* 0x80569D6C (0x8): +0x10 base -> fn_80568C60. */
void hbmKbCursorAdj10Clamp(u8* base) { fn_80568C60((KbObject*)(base - 0x10)); }

/* 0x80569D84 (0x8): +0x1A04 base -> fn_80567D3C. */
void hbmKbCursorAdj1A04Close(u8* base, HkbManager* manager) { fn_80567D3C(base - 0x1A04, manager); }
}
