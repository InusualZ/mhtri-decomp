/*
 * homebutton/hbm_kb_event.cpp - software-keyboard code of the tiHKBManager band: the event forwarders, state setters
 * and state dispatchers of the keyboard objects, plus the deleting destructors of the animation-widget classes.
 *
 * RANGE.     .text 0x8056083C..0x80566440 (97 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (cflags_nw4r with -inline noauto) for the whole band; the retail object has no
 *            extab/extabindex, so `#pragma exceptions off`. Band census against the main lib's -O3 flags: 302 -> 381 of 932
 *            rows at 100, this unit 62 -> 67 with scheduling held equal; -inline auto inlines the callee into 14
 *            adjustor thunks (`subi r3,r3,N; b callee`) the retail objects keep.
 * FLAGS.     `#pragma scheduling on` for the whole unit: retail restores r0 before r31 after a trailing call and hoists the
 *            `cmpwi r3,0` of the deleting destructors; with it nine more rows match and none drops.
 * FLAGS.     Scoped `global_optimizer off` (playbook 120) on the four `switch` dispatchers: retail keeps the compare tree
 *            `cmpwi 3; bge; cmpwi 1; bge; b` that the global optimizer folds into `subi; cmplwi; bgt`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the event forwarders; no `__FILE__` string and no runtime-dump name.
 * NAMES.     GUESS view classes HkbVuController, HkbVuChild, HkbVuContent, HkbVuWidget, HkbVuPart, HkbKbSubPart,
 *            HkbKbWrapped, HkbKbPart38..HkbKbPart54 (named for the owner slot that returns them) and HkbKbOwner: named
 *            from the members the bodies call them through; their slots are named by offset (v_0xNNN), the retail names are
 *            not recoverable.
 * NAMES.     GUESS function names (the map rows carry them): hkbVuEnterState4
 *            (sets state 4 and passes 0 to every part), hkbKbPreDrawZAlways / hkbKbPreDrawZLequal (GXSetZMode(1, 7|3, 1)
 *            then the per-mode part calls), hkbKbForward148WhileActiveB/C (copies of fn_805636BC), hkbKbRefreshParts,
 *            hkbKbDispatchInput (input record to the mode's part) and hkbKbDispatchPointer (key, x, y event to the parts).
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch;
 *            no method is defined and nothing is constructed, so no table is emitted here (rule 10).
 * SHAPES.    fn_80563740 declares its argument copy at function scope: inside the case block it costs a frame pointer
 *            (`mr r31,r1`) under `global_optimizer off`.
 * RESIDUALS. fn_80562244, fn_80562BFC, fn_80562FE4, hkbVuEnterState4: retail loads the next receiver
 *            (`lwz r3,0x1C/0x18(r31)`) before the `stw r0,0x44(r31)` state store; every source order, `volatile` and
 *            scheduler model tried keeps the store first.
 * RESIDUALS. hkbKbDispatchPointer: retail sets `addi r11,r1,0x50` after the f30/f31 `psq_st` saves (ours before them),
 *            and its +infinity operand is the unit's pool word lbl_8079D74C (ours an anonymous pool entry).
 * RESIDUALS. The functions are free `extern "C"` functions taking the band's views (VuObject, KbObject) as `self`; the
 *            members they are need their map rows renamed to the mangled names before they can become methods.
 * RESIDUALS. 30 functions are unwritten: 0x805608F8-0x80560B68, 0x80560BC4-0x80560FB4, 0x80561074-0x8056167C,
 *            0x80561688-0x80562244, 0x8056229C-0x805625C4, 0x805626DC-0x80562BFC, 0x80562C94-0x80562FE4,
 *            0x805630AC-0x8056329C, 0x805632BC-0x80563610, 0x805638D4-0x80563DEC, 0x80563E50-0x805644F8,
 *            0x80564780-0x805647F4, 0x805655C4-0x805661C8, 0x80566340-0x80566440. They call unnamed functions of
 *            hbm_text_panel, hbm_value, hbm_hermite, hbm_kb_cursor and tiHKBManager, or read the unit's own `.sbss`
 *            global objects / `.sdata` records, which need their classes (vtables) and names first.
 * NAMES.     GUESS HkbInputWidget::setEventSink and HbmGuiPaneWidget's destructor and calcComponents (their
 *            classes are homebutton/hkb_keyboard.h's and homebutton/gui.h's).
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

/* Retail schedules this unit (see FLAGS in the header). */
#pragma scheduling on

#include "homebutton/hbm_vu_object.h"
#include "homebutton/hbm_kb_object.h"
#include "homebutton/hbm_kb_event.h"
#include "homebutton/hbm_value.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/gui.h"
#include "nw4r/fn_805012C4.h"
#include "RVLGX/GXPixel.h"

/* The controller handed to HkbInputWidget::setEventSink: it is stored at +0x0C and told its owner through slot 0x1C.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuController {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C(VuObject* owner);
};

/* An element of the owner's child list at +0x04: slot 0x1C receives the two words fn_8056085C broadcasts.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuChild {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* untyped: opaque handles passed through */
    /* slot +0x01C */ virtual void v_0x01C(void* first, void* second);
};

/* The content object at +0x40: slot 0x0C returns the value fn_80561018 hands on, 0x18..0x28 are forwarded.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuContent {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual u32 v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
};

/* The widget the state setters run on: its own slots 0xEC, 0x100, 0x114 and 0x128.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuWidget {
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
    /* slot +0x0EC */ virtual void v_0x0EC(u32 value);
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
};

/* A part the state setters switch (the widget's +0x14/+0x18/+0x1C/+0x20 members): the slots take an on/off flag.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbVuPart {
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
    /* slot +0x0FC */ virtual void v_0x0FC(s32 enable);
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void v_0x104(s32 enable);
    /* slot +0x108 */ virtual void v_0x108(s32 enable);
    /* slot +0x10C */ virtual void v_0x10C(s32 enable);
    /* slot +0x110 */ virtual void v_0x110(s32 enable);
    /* slot +0x114 */ virtual void v_0x114();
    /* slot +0x118 */ virtual void v_0x118();
    /* slot +0x11C */ virtual void v_0x11C();
    /* slot +0x120 */ virtual void v_0x120(s32 enable);
    /* slot +0x124 */ virtual void v_0x124(s32 enable);
    /* slot +0x128 */ virtual void v_0x128(s32 enable);
    /* slot +0x12C */ virtual void v_0x12C();
    /* slot +0x130 */ virtual void v_0x130();
    /* slot +0x134 */ virtual void v_0x134();
    /* slot +0x138 */ virtual void v_0x138(s32 enable);
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
    /* slot +0x17C */ virtual void v_0x17C();
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
    /* slot +0x23C */ virtual void v_0x23C(s32 enable);
    /* slot +0x240 */ virtual void v_0x240();
    /* slot +0x244 */ virtual void v_0x244();
    /* slot +0x248 */ virtual void v_0x248();
    /* slot +0x24C */ virtual void v_0x24C();
    /* slot +0x250 */ virtual void v_0x250();
    /* slot +0x254 */ virtual void v_0x254();
    /* slot +0x258 */ virtual void v_0x258();
    /* slot +0x25C */ virtual void v_0x25C();
};

/* A sub-object embedded in the keyboard's parts; slots 0x1C (refresh) and 0x20 (pointer event) are the ones this unit calls.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbSubPart {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual s32 v_0x020(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
};

/* The wrapped keyboard object at +0x04 (and what the owner's slot 0x34 returns): slot 0xF0 is its state.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbWrapped {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018(s32 mode, s32 arg);
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
    /* slot +0x0F0 */ virtual s32 v_0x0F0();
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
    /* slot +0x17C */ virtual void v_0x17C();
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
    /* slot +0x220 */ virtual s32 v_0x220(s32 key, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* slot +0x224 */ virtual s32 v_0x224(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x228 */ virtual s32 v_0x228(void* input);
};

/* The part the owner's slot 0x38 returns: slot 0x20 receives the input record.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbPart38 {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* untyped: the input record is passed through untouched */
    /* slot +0x020 */ virtual void v_0x020(void* input);
};

/* The part the owner's slot 0x3C returns (used while the mode flag of slot 0x4C is set).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbPart3C {
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
    /* slot +0x12C */ virtual s32 v_0x12C(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x130 */ virtual s32 v_0x130(void* input);
};

/* The part the owner's slot 0x40 returns (used while the mode flag of slot 0x4C is clear); slot 0x40 is its kind.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbPart40 {
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
    /* slot +0x040 */ virtual s32 v_0x040();
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
    /* slot +0x114 */ virtual s32 v_0x114(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x118 */ virtual s32 v_0x118(void* input);
};

/* The part the owner's slot 0x44 returns; slot 0xD8 says whether it is shown.
 * size: 0x28 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart44 {
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
    /* slot +0x0D8 */ virtual s32 v_0x0D8();
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual s32 v_0x0E0(s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d, KbArgs* args);
    /* untyped: the input record is passed through untouched */
    /* slot +0x0E4 */ virtual s32 v_0x0E4(void* input);
    /* +0x04 */ u8 pad_0x04[0x20];
    /* +0x24 */ HkbKbSubPart child_0x24; /* an embedded sub-object the dispatchers call */
};

/* The part the owner's slot 0x48 returns; slot 0x100 says whether it is shown, slot 0x40 takes a flag.
 * size: 0x17F8 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart48 {
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
    /* slot +0x040 */ virtual void v_0x040(s32 enable);
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
    /* slot +0x100 */ virtual s32 v_0x100();
    /* +0x04 */ u8 pad_0x04[0x17F0];
    /* +0x17F4 */ HkbKbSubPart child_0x17F4; /* an embedded sub-object the dispatchers call */
};

/* The part the owner's slot 0x4C returns; slot 0x10 is the mode flag the dispatchers branch on.
 * size: 0x18 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart4C {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual u32 v_0x010();
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
    /* +0x04 */ u8 pad_0x04[0x10];
    /* +0x14 */ HkbKbSubPart child_0x14; /* an embedded sub-object the dispatchers call */
};

/* The part the owner's slot 0x50 returns; slot 0xB8 says whether it is shown.
 * size: 0x20 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart50 {
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
    /* slot +0x0B8 */ virtual s32 v_0x0B8();
    /* +0x04 */ u8 pad_0x04[0x18];
    /* +0x1C */ HkbKbSubPart child_0x1C; /* an embedded sub-object the dispatchers call */
};

/* The part the owner's slot 0x54 returns.
 * size: 0x08 (approximation: only the compiler-emitted vptr at +0x00 and the embedded sub-object are touched) */
class HkbKbPart54 {
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
    /* +0x04 */ HkbKbSubPart child_0x04; /* an embedded sub-object the dispatchers call */
};

/* The keyboard object the dispatchers run on: slots 0x34..0x54 return the wrapped object and its parts.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 are touched) */
class HkbKbOwner {
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
    /* slot +0x034 */ virtual HkbKbWrapped* v_0x034();
    /* slot +0x038 */ virtual HkbKbPart38* v_0x038();
    /* slot +0x03C */ virtual HkbKbPart3C* v_0x03C();
    /* slot +0x040 */ virtual HkbKbPart40* v_0x040();
    /* slot +0x044 */ virtual HkbKbPart44* v_0x044();
    /* slot +0x048 */ virtual HkbKbPart48* v_0x048();
    /* slot +0x04C */ virtual HkbKbPart4C* v_0x04C();
    /* slot +0x050 */ virtual HkbKbPart50* v_0x050();
    /* slot +0x054 */ virtual HkbKbPart54* v_0x054();
};
extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * chained-sub-object calls
 * ------------------------------------------------------------------------------------------------ */
/* Stores the notification sink and tells it its owner. */
void HkbInputWidget::setEventSink(HkbEventSink* sink)
{
    mpEventSink = sink;
    sink->setOwner(this);
}

void fn_8056085C(VuObject* self, void* a, void* b)
{
    HkbVuChild* node = (HkbVuChild*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x04, 0);

    while (node != 0) {
        node->v_0x01C(a, b);
        node = (HkbVuChild*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x04, node);
    }
}

void fn_805608EC(VuObject* self) { nw4r::ut::List_Init((nw4r::ut::List*)&self->list_0x04, 4); }

/* Destroys the pane widget's pane manager (at +4). */
HbmGuiPaneWidget::~HbmGuiPaneWidget()
{
}

void fn_80560FB4(VuObject* self) { ((HkbVuContent*)self->sub_0x40)->v_0x020(); }

void fn_80560FC8(VuObject* self) { ((HkbVuContent*)self->sub_0x40)->v_0x018(); }

void fn_80560FDC(VuObject* self) { ((HkbVuContent*)self->sub_0x40)->v_0x01C(); }

void fn_80560FF0(VuObject* self) { ((HkbVuContent*)self->sub_0x40)->v_0x024(); }

void fn_80561004(VuObject* self) { ((HkbVuContent*)self->sub_0x40)->v_0x028(); }

/* Event forwarders: a state change pushes the wrapped object's value through a virtual slot. */
void fn_80561018(VuObject* self, s32 state)
{
    if (state != 1)
        return;
    u32 value = ((HkbVuContent*)self->sub_0x40)->v_0x00C();
    ((HkbVuWidget*)self)->v_0x0EC(value);
}

/* ---------------------------------------------------------------------------------------------------
 * stores into the unit's small-data globals and the float pools
 * ------------------------------------------------------------------------------------------------ */
void fn_8056167C(f32 value) { lbl_807946D8[1] = value; }

void fn_80562244(VuObject* self)
{
    ((HkbVuWidget*)self)->v_0x114();
    self->field_0x44 = 0xB;
    ((HkbVuPart*)self->sub_0x1C)->v_0x23C(1);
}

/* 0x805625C4 (0x118): Enters state 4 and switches every part off. */
void hkbVuEnterState4(VuObject* self)
{
    ((HkbVuWidget*)self)->v_0x100();
    self->field_0x44 = 4;
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->field_0x18)->v_0x110(0);
    ((HkbVuPart*)self->field_0x14)->v_0x128(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
}

void fn_80562BFC(VuObject* self)
{
    ((HkbVuWidget*)self)->v_0x114();
    self->field_0x44 = 8;
    ((HkbVuPart*)self->field_0x18)->v_0x104(1);
}

void fn_80562C54(VuObject* self)
{
    ((HkbVuWidget*)self)->v_0x128();
    self->field_0x44 = 9;
}

void fn_80562FE4(VuObject* self)
{
    ((HkbVuWidget*)self)->v_0x100();
    self->field_0x44 = 0xD;
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
}

/* ---------------------------------------------------------------------------------------------------
 * guarded forwarders: `if (state < 1 || state >= 3) return; <wrapped>->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563054(VuObject* self)
{
    if (self->field_0x44 >= 3)
        return;
    if (self->field_0x44 < 1)
        return;
    ((HkbVuPart*)self->sub_0x1C)->v_0x258();
}

void fn_80563080(VuObject* self)
{
    if (self->field_0x44 >= 3)
        return;
    if (self->field_0x44 < 1)
        return;
    ((HkbVuPart*)self->sub_0x1C)->v_0x25C();
}

/* The nibble dispatcher: tail-calls the registered neighbour according to bits 4..7 of +0x48. */
#pragma push
#pragma global_optimizer off
void fn_8056329C(VuObject* self)
{
    switch ((self->field_0x48 >> 4) & 0xF) {
    case 1:
        fn_805632BC(self);
        break;
    case 0:
    default:
        fn_805634A4(self);
        break;
    }
}
#pragma pop
}

extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * no-op virtuals: the retail body is a bare `blr`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563610(KbObject* self) { (void)self; }

/* ---------------------------------------------------------------------------------------------------
 * small virtual forwarders: `return self->sub->vt->slot(...)`
 * ------------------------------------------------------------------------------------------------ */
void fn_80563614(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0(); }

/* ---------------------------------------------------------------------------------------------------
 * accessors
 * ------------------------------------------------------------------------------------------------ */
void* fn_80563628(KbObject* self) { return self->field_0x44; }

/* ---------------------------------------------------------------------------------------------------
 * state dispatchers: forward to the object the owner's slot 0x34 returns while the state is 1 or 2
 * ------------------------------------------------------------------------------------------------ */
#pragma push
#pragma global_optimizer off
void fn_80563630(KbObject* self)
{
    switch (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0()) {
    case 1:
    case 2:
        ((HkbKbOwner*)self)->v_0x034()->v_0x14C();
        break;
    }
}
#pragma pop

void fn_805636A4(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x068(); }

void fn_805636B8(KbObject* self) { (void)self; }

void fn_805636BC(KbObject* self)
{
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() == 1 || ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() == 2)
        ((HkbKbOwner*)self)->v_0x034()->v_0x148();
}

#pragma push
#pragma global_optimizer off
s32 fn_80563740(KbObject* self, s32 a, f32 x, f32 y, s32 b, s32 c, s32 d)
{
    s32 ret;
    KbArgs args; /* the copy of the argument tuple the callee receives as its 8th argument */

    switch (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0()) {
    case 1:
    case 2: {
        ((HkbKbOwner*)self)->v_0x034();
        args.a = a;
        args.x = x;
        args.y = y;
        args.b = b;
        args.c = c;
        args.d = d;
        ret = ((HkbKbOwner*)self)->v_0x034()->v_0x224(a, x, y, b, c, d, &args);
        break;
    }
    default:
        ret = 0;
        break;
    }
    return ret;
}
#pragma pop

#pragma push
#pragma global_optimizer off
s32 fn_80563834(KbObject* self, void* value)
{
    switch (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0()) {
    case 1:
    case 2:
        ((HkbKbOwner*)self)->v_0x034();
        return ((HkbKbOwner*)self)->v_0x034()->v_0x228(value);
    }
    return 0;
}
#pragma pop

void fn_80563DEC(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x078(); }

void fn_80563E00(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x080(); }

void fn_80563E14(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x090(); }

void fn_80563E28(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x098(); }

void* fn_80563E3C(KbObject* self) { return self->sub_0x04->field_0x78; }

void fn_80563E48(KbObject* self) { (void)self; }

void fn_80563E4C(KbObject* self) { (void)self; }

f32 fn_805644F8(KbObject* self) { return self->tail_0x1A04.float_0x104; }

f32 fn_80564500(KbObject* self) { return self->tail_0x1A04.float_0x108; }

/* 0x80564508 (0x1DC): Sets the z mode to always-pass and redraws the parts the current mode shows. */
void hkbKbPreDrawZAlways(KbObject* self)
{
    HkbKbOwner* owner = (HkbKbOwner*)self;

    GXSetZMode(1, 7, 1);
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 1 && ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 2)
        owner->v_0x054()->v_0x088();
    owner->v_0x04C()->v_0x098();
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 1 && ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 2)
        owner->v_0x034()->v_0x148();
    if (owner->v_0x048()->v_0x100())
        owner->v_0x048()->v_0x024();
    if (owner->v_0x04C()->v_0x010())
        owner->v_0x03C()->v_0x110();
    else
        owner->v_0x040()->v_0x03C();
}

/* ---------------------------------------------------------------------------------------------------
 * tail-call thunks: the retail body is `b <victim>` (the arguments pass through untouched)
 * ------------------------------------------------------------------------------------------------ */
/* Runs the pane manager's calcComponents. */
void HbmGuiPaneWidget::calcComponents() { HbmPaneManager::calcComponents(); }

/* 0x805646EC (0x84): Forwards slot 0x148 of the wrapped object while its state is 1 or 2. */
void hkbKbForward148WhileActiveB(KbObject* self)
{
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() == 1 || ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() == 2)
        ((HkbKbOwner*)self)->v_0x034()->v_0x148();
}

/* ---------------------------------------------------------------------------------------------------
 * constant returns
 * ------------------------------------------------------------------------------------------------ */
s32 fn_80564770(KbObject* self) { (void)self; return 0; }

s32 fn_80564778(KbObject* self) { (void)self; return 0; }

void fn_805647F4(KbObject* self) { (void)self; }

void fn_805647F8(KbObject* self) { (void)self; }

/* 0x805647FC (0x230): Refreshes the parts the current mode and the wrapped state show. */
void hkbKbRefreshParts(KbObject* self)
{
    HkbKbOwner* owner = (HkbKbOwner*)self;

    if (owner->v_0x04C()->v_0x010())
        owner->v_0x03C()->v_0x114();
    else
        owner->v_0x040()->v_0x038();
    owner->v_0x04C()->child_0x14.v_0x01C();
    if (owner->v_0x048()->v_0x100())
        owner->v_0x048()->v_0x0CC();
    owner->v_0x034()->v_0x14C();
    if (owner->v_0x050()->v_0x0B8())
        owner->v_0x050()->child_0x1C.v_0x01C();
    if (owner->v_0x044()->v_0x0D8())
        owner->v_0x044()->child_0x24.v_0x01C();
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 1 && ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 2)
        owner->v_0x054()->child_0x04.v_0x01C();
}

void fn_80564A2C(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x0A0(); }

void fn_80564A40(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x088(); }

/* 0x80564A54 (0x27C): Sets the z mode to less-or-equal and redraws the parts the current mode shows. */
void hkbKbPreDrawZLequal(KbObject* self)
{
    HkbKbOwner* owner = (HkbKbOwner*)self;

    GXSetZMode(1, 3, 1);
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 1 && ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 2)
        owner->v_0x054()->v_0x088();
    owner->v_0x04C()->v_0x098();
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 1 && ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() != 2)
        owner->v_0x034()->v_0x148();
    if (owner->v_0x048()->v_0x100())
        owner->v_0x048()->v_0x024();
    if (owner->v_0x04C()->v_0x010())
        owner->v_0x03C()->v_0x110();
    else
        owner->v_0x040()->v_0x03C();
    if (owner->v_0x050()->v_0x0B8())
        owner->v_0x050()->v_0x0A4();
    if (owner->v_0x044()->v_0x0D8())
        owner->v_0x044()->v_0x0D0();
}

/* 0x80564CD0 (0x84): Forwards slot 0x148 of the wrapped object while its state is 1 or 2. */
void hkbKbForward148WhileActiveC(KbObject* self)
{
    if (((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() == 1 || ((HkbKbWrapped*)self->sub_0x04)->v_0x0F0() == 2)
        ((HkbKbOwner*)self)->v_0x034()->v_0x148();
}

/* 0x80564D54 (0x534): Hands a pointer event to the parts the current mode shows and reports whether any took it;
 * when the mode flag changed underneath, re-arms the wrapped object and the parts for the new mode. */
s32 hkbKbDispatchPointer(KbObject* self, s32 key, f32 x, f32 y, s32 arg_b, s32 arg_c, s32 arg_d)
{
    KbArgs args;
    u32 mode = ((HkbKbOwner*)self)->v_0x04C()->v_0x010();
    s32 handled_list;
    s32 handled_mode;
    s32 handled_panel;
    s32 handled_wrapped;

    args.a = key;
    args.x = x;
    args.y = y;
    args.b = arg_b;
    args.c = arg_c;
    args.d = arg_d;
    if (((HkbKbOwner*)self)->v_0x050()->v_0x0B8())
        return ((HkbKbOwner*)self)->v_0x050()->child_0x1C.v_0x020(key, x, y, arg_b, arg_c, arg_d, &args);
    ((HkbKbOwner*)self)->v_0x034()->v_0x220(key, arg_b, arg_c, arg_d, &args);
    if (((HkbKbOwner*)self)->v_0x044()->v_0x0D8())
        return ((HkbKbOwner*)self)->v_0x044()->v_0x0E0(key, x, y, arg_b, arg_c, arg_d, &args);
    if (mode)
        handled_mode = ((HkbKbOwner*)self)->v_0x03C()->v_0x12C(key, x, y, arg_b, arg_c, arg_d, &args);
    else
        handled_mode = ((HkbKbOwner*)self)->v_0x040()->v_0x114(key, x, y, arg_b, arg_c, arg_d, &args);
    handled_list = ((HkbKbOwner*)self)->v_0x04C()->child_0x14.v_0x020(key, x, y, arg_b, arg_c, arg_d, &args);
    handled_panel = 0;
    if (((HkbKbOwner*)self)->v_0x048()->v_0x100())
        handled_panel = ((HkbKbOwner*)self)->v_0x048()->child_0x17F4.v_0x020(key, x, y, arg_b, arg_c, arg_d, &args);
    handled_wrapped = 0;
    if (handled_panel == 0)
        handled_wrapped = ((HkbKbOwner*)self)->v_0x034()->v_0x224(key, x, y, arg_b, arg_c, arg_d, &args);
    else
        ((HkbKbOwner*)self)->v_0x034()->v_0x224(key, 1e39f, 1e39f, arg_b, arg_c, arg_d, &args); /* +infinity: no position */
    handled_mode = (handled_list | handled_panel | handled_wrapped | handled_mode) != 0;
    if (mode != ((HkbKbOwner*)self)->v_0x04C()->v_0x010()) {
        ((HkbKbOwner*)self)->v_0x034()->v_0x018(6, 0);
        if (!((HkbKbOwner*)self)->v_0x04C()->v_0x010() && ((HkbKbOwner*)self)->v_0x040()->v_0x040() == 3)
            ((HkbKbOwner*)self)->v_0x048()->v_0x040(1);
        else
            ((HkbKbOwner*)self)->v_0x048()->v_0x040(0);
        if (((HkbKbOwner*)self)->v_0x04C()->v_0x010())
            ((HkbKbOwner*)self)->v_0x03C()->v_0x034();
        else
            ((HkbKbOwner*)self)->v_0x040()->v_0x034();
    }
    return handled_mode;
}

/* 0x80565288 (0x328): Hands an input record to the part the current mode selects and, when the mode flag
 * changed underneath, re-arms the wrapped object and the parts for the new mode. */
/* untyped: the input record is passed through untouched */
s32 hkbKbDispatchInput(KbObject* self, void* input)
{
    u32 mode = ((HkbKbOwner*)self)->v_0x04C()->v_0x010();
    s32 result;

    if (((HkbKbOwner*)self)->v_0x050()->v_0x0B8())
        return 0;
    ((HkbKbOwner*)self)->v_0x038()->v_0x020(input);
    if (((HkbKbOwner*)self)->v_0x044()->v_0x0D8())
        return ((HkbKbOwner*)self)->v_0x044()->v_0x0E4(input);
    if (mode)
        result = ((HkbKbOwner*)self)->v_0x03C()->v_0x130(input);
    else
        result = ((HkbKbOwner*)self)->v_0x040()->v_0x118(input);
    if (mode != ((HkbKbOwner*)self)->v_0x04C()->v_0x010()) {
        ((HkbKbOwner*)self)->v_0x034()->v_0x018(6, 0);
        if (!((HkbKbOwner*)self)->v_0x04C()->v_0x010() && ((HkbKbOwner*)self)->v_0x040()->v_0x040() == 3)
            ((HkbKbOwner*)self)->v_0x048()->v_0x040(1);
        else
            ((HkbKbOwner*)self)->v_0x048()->v_0x040(0);
        if (((HkbKbOwner*)self)->v_0x04C()->v_0x010())
            ((HkbKbOwner*)self)->v_0x03C()->v_0x034();
        else
            ((HkbKbOwner*)self)->v_0x040()->v_0x034();
        ((HkbKbOwner*)self)->v_0x034()->v_0x228(input);
    }
    return result;
}

void fn_805655B0(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x070(); }

void fn_805661C8(KbObject* self) { (void)self; }

void fn_805661CC(KbObject* self) { (void)self; }

void fn_805661D0(KbObject* self) { (void)self; }

void fn_805661D4(KbObject* self) { (void)self; }

s32 fn_805661D8(KbObject* self) { (void)self; return 3; }

void fn_805661E0(KbObject* self) { (void)self; }

s32 fn_805661E4(KbObject* self) { (void)self; return 2; }

/* ---------------------------------------------------------------------------------------------------
 * the five deleting destructors of the animation-widget classes: `if (self && flag > 0) delete self;`
 * returns the object (the retail body is the compiler's own deleting-destructor shape)
 * ------------------------------------------------------------------------------------------------ */
void* fn_805661EC(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_8056622C(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

s32 fn_8056626C(KbObject* self) { (void)self; return 1; }

void fn_80566274(KbObject* self) { (void)self; }

s32 fn_80566278(KbObject* self) { (void)self; return 0; }

void* fn_80566280(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_805662C0(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}

void* fn_80566300(void* self, s32 flag)
{
    if (self != 0 && flag > 0)
        operator delete(self);
    return self;
}
}
