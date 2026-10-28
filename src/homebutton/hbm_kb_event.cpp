/*
 * homebutton/hbm_kb_event.cpp - software-keyboard code of the tiHKBManager band: the event forwarders, state setters
 * and state dispatchers of the keyboard objects, the keyboard object's destructor, part setup and open/close states,
 * the keyboard's mode states and saved keys, the panel animations, and the four state objects.
 *
 * RANGE.     .text 0x8056083C..0x80566440 (97 functions; the ones not defined below keep the target object's bytes).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (cflags_nw4r with -inline noauto) for the whole band; the retail object has no
 *            extab/extabindex, so `#pragma exceptions off`. Band census against the main lib's -O3 flags: 302 -> 381 of 932
 *            rows at 100, this unit 62 -> 67 with scheduling held equal; -inline auto inlines the callee into 14
 *            adjustor thunks (`subi r3,r3,N; b callee`) the retail objects keep.
 * FLAGS.     `#pragma scheduling on` for the whole unit: retail restores r0 before r31 after a trailing call and hoists the
 *            `cmpwi r3,0` of the deleting destructors; with it nine more rows match and none drops.
 * FLAGS.     Scoped `global_optimizer off` (playbook 120) on the `switch` dispatchers and animation steps: retail keeps the
 *            compare tree `cmpwi 3; bge; cmpwi 1; bge; b` that the global optimizer folds into `subi; cmplwi; bgt`.
 * NAMES.     GUESS file name: the band's `hbm_` prefix + the event forwarders; no `__FILE__` string and no runtime-dump name.
 * NAMES.     GUESS view classes HkbVuController, HkbVuChild, HkbVuContent, HkbVuWidget, HkbVuPart, HkbKbSubPart,
 *            HkbKbLinkSub, HkbKbWrapped, HkbKbPart38..HkbKbPart54 (named for the owner slot that returns them) and
 *            HkbKbOwner (homebutton/hbm_kb_event.h), HkbVuKeyboard and HkbKbAnimView (this file): named from the members
 *            the bodies call them through; their slots are named by offset (v_0xNNN), the retail names are not recoverable.
 * NAMES.     GUESS function names (the map rows carry them): hkbVuEnterState0..14 / hkbVuEnterKanaState0 /
 *            hkbVuReenterState10 (the state number each stores at +0x44), hkbVuSaveKeys / hkbVuRestoreKeys /
 *            hkbVuResetKeys (the +0x48..+0x4F saved key state), hkbKbAnimRewindPanes / hkbKbAnimApplyShow /
 *            hkbKbAnimFinishPanes / hkbKbAnimApplyHide / hkbKbAnimRewindCursor0/3 (the panel animations over 30 frames),
 *            hkbKbSetSlideEnd, hkbKbPreDrawZAlways / hkbKbPreDrawZLequal, hkbKbForward148WhileActiveB/C,
 *            hkbKbRefreshParts, hkbKbDispatchInput and hkbKbDispatchPointer.
 * NAMES.     GUESS hkbVuCreateKeyboard / hkbVuInitState / hkbVuSetOpenImmediate (slots 0x0C, 0x10 and 0xEC of the keyboard
 *            object's table 0x80657A20: they build the parts, enter the first state and switch open/closed with sound
 *            28/29), the part views SysKbdPart..SysKbdGuiPart (named for the factory slot and the pane manager offset
 *            the bodies use) and the state classes HkbKbState / HkbKbStateClosed / HkbKbStateOpening / HkbKbStateOpen /
 *            HkbKbStateClosing (slot 0x0C returns 0..3; the opening/closing ones carry the animation frame at +0x08)
 *            with their objects hkbKbStateClosed (.sbss 0x80795A90), hkbKbStateOpen (0x80795A98), hkbKbStateOpening
 *            (.bss 0x80790D34) and hkbKbStateClosing (0x80790D4C).
 * NAMES.     GUESS hkbVuCreateGuiPart (slot 0x14C: the HbmGuiPaneWidget on fs_VK_bg_a.brlyt at +0x78).
 * SHAPES.    The gui part is built with homebutton/gui.h's inline constructor, so MWCC emits HbmGuiAttachable's table
 *            (0x80657858) and its empty inline create/init (0x805661D0 / 0x805661CC) here, as retail does.
 * NAMES.     GUESS data names hkbKbSlideY / hkbKbPanelY / hkbKbAlpha (.sdata 0x807946D8 / 0x807946E8 / 0x807946E0: the
 *            animations' start and end values).
 * NAMES.     GUESS HkbInputWidget::setEventSink and HbmGuiPaneWidget's destructor and calcComponents (their
 *            classes are homebutton/hkb_keyboard.h's and homebutton/gui.h's).
 * SHAPES.    The view classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,<slot>(r12)` dispatch;
 *            their methods are not defined here, so no table of theirs is emitted (rule 10).
 * SHAPES.    SysKbdView::~SysKbdView (menu/menu_sysmsg.h's class) is declared after the class's other virtuals, so it is
 *            not the first non-inline virtual and the class table (0x80657A20) is referenced, not emitted: emitted, it
 *            would name the placeholder slots, which no unit defines (an undefined reference per slot). The part views
 *            declare their destructors so no weak copy of an implicit one is emitted.
 * SHAPES.    The four state objects are MWCC's: their inline constructors build the static initialiser and their
 *            implicit destructors the weak deleting destructors 0x805661EC..0x80566340.
 * SHAPES.    fn_80563740 and hkbKbAnimApplyShow declare their case-block locals at function scope: inside the case block
 *            they cost a frame pointer (`mr r31,r1`) under `global_optimizer off`.
 * SHAPES.    The animations build each pane translation from an HkbPoint constructed at the call (a VEC2 returned by value
 *            adds a copy, a by-value float keeps it in f31); hkbLayoutLabels is not `const` (retail loads the labels
 *            before the call that follows).
 * SHAPES.    hkbVuSaveKeys / hkbVuRestoreKeys spell the modifier test `!(state & 0x100)` and `!off ? 0x100 : 0` (the
 *            `cntlzw` forms).
 * RESIDUALS. fn_80562244, fn_80562BFC, fn_80562FE4, hkbVuEnterState4: retail loads the next receiver
 *            (`lwz r3,0x1C/0x18(r31)`) before the `stw r0,0x44(r31)` state store; every source order, `volatile` and
 *            scheduler model tried keeps the store first.
 * RESIDUALS. hkbKbDispatchPointer: retail sets `addi r11,r1,0x50` after the f30/f31 `psq_st` saves (ours before them),
 *            and its +infinity operand is the unit's pool word lbl_8079D74C (ours an anonymous pool entry).
 * RESIDUALS. hkbKbAnimRewindPanes: retail's frame is 0x10 smaller and swaps r30/r31 (self/alpha); hkbVuEnterKanaState0 and
 *            hkbVuEnterState14: the pane-name string offsets differ by the five unwritten factories' strings that precede
 *            them in retail's pool.
 * RESIDUALS. The functions are free `extern "C"` functions taking the band's views (VuObject, KbObject) as `self`; the
 *            members they are need their map rows renamed to the mangled names before they can become methods.
 * NAMES.     GUESS table names __vt__10HkbKbState (0x806579C8) and __vt__16HkbKbStateClosed / __vt__14HkbKbStateOpen /
 *            __vt__17HkbKbStateOpening / __vt__17HkbKbStateClosing (0x80657970 / 0x806578C0 / 0x80657918 / 0x80657868),
 *            the tables the static initialiser stores; HkbKbStateClosing's shares its parent's slots, so retail's
 *            closing state derives from the opening one (the initialiser drops the middle table store).
 * RESIDUALS. __sinit_\hbm_kb_event_cpp: retail emits the state classes' tables here (0x80657868..0x80657A20) and
 *            addresses them from one `.data` base (`addi r0,r29,0x2D8`); ours references them by name (`lis/addi
 *            __vt__...`), because their key functions are this unit's free `extern "C"` functions (0x805636A4,
 *            0x80563610, 0x805647F4, hkbKbAnimApplyShow/Hide), which would have to become the classes' members first.
 * RESIDUALS. 4 functions (1200 bytes) are unwritten: 0x80561148-0x805615F8, the text part factories (slots 0x140,
 *            0x144, 0xB8, 0x148). They build homebutton/hbm_text_panel.cpp's HbmKbTextWidget (and a derived panel with
 *            table 0x80657F10) through its inline constructor, and that class is defined in that unit's .cpp.
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
#include "homebutton/hbm_kb_cursor.h"
#include "homebutton/hbm_text_panel.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/gui.h"
#include "nw4r/fn_805012C4.h"
#include "RVLGX/GXPixel.h"
#include "homebutton/hkb_layout.h"
#include "menu/menu_sysmsg.h"

/* The parts the keyboard object (menu/menu_sysmsg.h's SysKbdView) builds through its factory slots and destroys in
 * its destructor: slot 0x08 is the destructor and slot 0x0C creates the part from the band allocator.
 * size: 0x4 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class SysKbdPart {
public:
    /* slot +0x008 */ virtual ~SysKbdPart();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
}; /* size: 0x4 */

/* The layout part at +0x5C: a pane manager embedded at +0x4C.  size: 0xF4 (approximation: the vptr and the
 * embedded pane manager) */
class SysKbdLayoutPart : public SysKbdPart {
public:
    /* slot +0x008 */ virtual ~SysKbdLayoutPart();

    /* +0x04 */ u8 unused_0x04[0x48];
    /* +0x4C */ HbmPaneManager mPaneManager;
}; /* size: 0xF4 */

/* The sliding part at +0x60: a pane manager embedded at +0x34.  size: 0xDC (approximation) */
class SysKbdSlidePart : public SysKbdPart {
public:
    /* slot +0x008 */ virtual ~SysKbdSlidePart();

    /* +0x04 */ u8 unused_0x04[0x30];
    /* +0x34 */ HbmPaneManager mPaneManager;
}; /* size: 0xDC */

/* The text parts at +0x58 and +0x64 (the keyboard widgets of homebutton/hbm_text_panel.cpp): slot 0xD4 creates
 * one with the keyboard object's text buffers; the pane manager is embedded at +0x1A04.
 * size: 0x1AAC (approximation: the vptr and the embedded pane manager) */
class SysKbdTextPart : public SysKbdPart {
public:
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
    /* slot +0x008 */ virtual ~SysKbdTextPart();
    /* slot +0x0D4 */ virtual void createWithBuffers(MEMAllocator* allocator, HbmTextBufferSet* buffers);

    /* +0x0004 */ u8 unused_0x0004[0x1A00];
    /* +0x1A04 */ HbmPaneManager mPaneManager;
}; /* size: 0x1AAC */

/* The prediction part at +0x68 (homebutton/hbm_kb_widget.cpp's widget): the pane manager is embedded at +0x17F4.
 * size: 0x189C (approximation) */
class SysKbdPredictPart : public SysKbdPart {
public:
    /* slot +0x008 */ virtual ~SysKbdPredictPart();

    /* +0x0004 */ u8 unused_0x0004[0x17F0];
    /* +0x17F4 */ HbmPaneManager mPaneManager;
}; /* size: 0x189C */

/* The cursor part at +0x6C: slot 0x08 creates it, its destructor is slot 0x94; the pane manager is embedded at +0x14.
 * size: 0xBC (approximation) */
class SysKbdCursorPart {
public:
    /* slot +0x008 */ virtual void create(MEMAllocator* allocator);
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
    /* slot +0x094 */ virtual ~SysKbdCursorPart();

    /* +0x04 */ u8 unused_0x04[0x10];
    /* +0x14 */ HbmPaneManager mPaneManager;
}; /* size: 0xBC */

struct HkbVuKeyboard;

/* The keyboard object's states (its +0x40): the closed, opening, open and closing panel.  Slot 0x0C returns the state
 * number, slot 0x2C enters the state and slot 0x30 leaves it; the base's own slots 0x34..0x54 are shared by all four.
 * size: 0x8 */
class HkbKbState {
public:
    HkbKbState() : owner(NULL) {}
    /* slot +0x008 */ virtual ~HkbKbState() {}
    /* slot +0x00C */ virtual s32 getState() = 0;
    /* slot +0x010 */ virtual void reset() = 0;
    /* slot +0x014 */ virtual void calc() = 0;
    /* slot +0x018 */ virtual void v_0x018() = 0;
    /* slot +0x01C */ virtual void v_0x01C() = 0;
    /* slot +0x020 */ virtual void v_0x020() = 0;
    /* slot +0x024 */ virtual void v_0x024() = 0;
    /* slot +0x028 */ virtual void v_0x028() = 0;
    /* slot +0x02C */ virtual void enter() = 0;
    /* slot +0x030 */ virtual void leave() = 0;
    /* slot +0x034 */ virtual void v_0x034();

    /* +0x04 */ HkbVuKeyboard* owner;
}; /* size: 0x8 */

/* State 0: the panel is closed. size: 0x8 */
class HkbKbStateClosed : public HkbKbState {
public:
    /* slot +0x00C */ virtual s32 getState() { return 0; }
    /* slot +0x010 */ virtual void reset();
    /* slot +0x014 */ virtual void calc();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void enter();
    /* slot +0x030 */ virtual void leave() {}
}; /* size: 0x8 */

/* State 2: the panel is open and takes the input. size: 0x8 */
class HkbKbStateOpen : public HkbKbState {
public:
    /* slot +0x00C */ virtual s32 getState() { return 2; }
    /* slot +0x010 */ virtual void reset();
    /* slot +0x014 */ virtual void calc();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void enter();
    /* slot +0x030 */ virtual void leave() {}
}; /* size: 0x8 */

/* State 1: the panel slides in over 30 frames. size: 0xC */
class HkbKbStateOpening : public HkbKbState {
public:
    HkbKbStateOpening() : frame(0.0f) {}
    /* slot +0x00C */ virtual s32 getState() { return 1; }
    /* slot +0x010 */ virtual void reset();
    /* slot +0x014 */ virtual void calc();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void enter();
    /* slot +0x030 */ virtual void leave() {}

    /* +0x08 */ f32 frame;
}; /* size: 0xC */

/* State 3: the panel slides out over 30 frames: the opening state with the hide animation and the closing cursor
 * (its table shares the opening state's other slots). size: 0xC */
class HkbKbStateClosing : public HkbKbStateOpening {
public:
    /* slot +0x00C */ virtual s32 getState() { return 3; }
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x02C */ virtual void enter();
}; /* size: 0xC */

/* The saved keyboard state at the keyboard Vu object's +0x4C: the shift flag, the layout, the caps flag and whether
 * the modifier was off.  size: 0x1 */
struct HkbVuSavedKeys {
    u8 shift : 1;       /* +0x0 */
    u8 layout : 4;      /* +0x0 */
    u8 caps : 1;        /* +0x0 */
    u8 modifierOff : 1; /* +0x0 */
    u8 dirty : 1;       /* +0x0 */
}; /* size: 0x1 */

/* The saved panel position at the keyboard Vu object's +0x4D.  size: 0x1 */
struct HkbVuSavedPanel {
    u8 column : 4; /* +0x0 */
    u8 row : 4;    /* +0x0 */
}; /* size: 0x1 */

/* The keyboard's Vu object: the Vu object's fields (homebutton/hbm_vu_object.h's VuObject), the saved keyboard state,
 * then the three alternative wrapped keyboards the states put at +0x1C and the default parts the states copy to
 * +0x14..+0x2C.  size: 0x7C */
struct HkbVuKeyboard {
    /* +0x00 */ VuVTable* vt;
    /* +0x04 */ MEMAllocator* allocator;
    /* +0x08 */ nw4r::lyt::ResourceAccessor* resources;
    /* +0x0C */ HbmTextBufferSet* textBuffers;
    /* +0x10 */ SysKbdInputView* input;
    /* +0x14 */ void* field_0x14;
    /* +0x18 */ void* field_0x18;
    /* +0x1C */ VuSub* sub_0x1C;
    /* +0x20 */ void* field_0x20;
    /* +0x24 */ void* field_0x24;
    /* +0x28 */ void* field_0x28;
    /* +0x2C */ void* field_0x2C;
    /* +0x30 */ HbmSoundPlayer* sound;
    /* +0x34 */ s32 layoutType;
    /* +0x38 */ s32 language;
    /* +0x3C */ u8 field_0x3C;
    /* +0x3D */ u8 unused_0x3D[3];
    /* +0x40 */ HkbKbState* state;
    /* +0x44 */ s32 field_0x44;
    /* +0x48 */ u8 savedMode;
    /* +0x49 */ u8 savedCursor;
    /* +0x4A */ u8 savedPanelShown;
    /* +0x4B */ u8 savedListState;
    union {
        /* +0x4C */ HkbVuSavedKeys savedKeys;
        /* +0x4C */ u8 savedKeysByte;
    };
    union {
        /* +0x4D */ HkbVuSavedPanel savedPanel;
        /* +0x4D */ u8 savedPanelByte;
    };
    /* +0x4E */ u8 savedPad[2];
    /* +0x50 */ void* altWrapped[3];
    /* +0x5C */ void* defaultParts[7];
    /* +0x78 */ HbmGuiPaneWidget* guiPart;
}; /* size: 0x7C */
extern "C" void hkbVuRestoreKeys(HkbVuKeyboard* self);
extern "C" void hkbVuResetKeys(HkbVuKeyboard* self);

/* An animation object of the keyboard: its wrapped keyboard and its frame.
 * size: 0xC (approximation: the members the animation steps read) */
struct HkbKbAnimView {
    /* +0x00 */ KbVTable* vt;
    /* +0x04 */ HkbKbWrapped* wrapped;
    /* +0x08 */ f32 frame;
}; /* size: 0xC */

/* The four states of the keyboard object (.sbss 0x80795A90 / 0x80795A98, .bss 0x80790D34 / 0x80790D4C). */
HkbKbStateClosed hkbKbStateClosed;
HkbKbStateOpen hkbKbStateOpen;
HkbKbStateOpening hkbKbStateOpening;
HkbKbStateClosing hkbKbStateClosing;

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

void hbmKbEventBroadcast(VuObject* self, u32 id, u32* arg)
{
    HkbVuChild* node = (HkbVuChild*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x04, 0);

    while (node != 0) {
        node->v_0x01C(id, arg);
        node = (HkbVuChild*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x04, node);
    }
}

void fn_805608EC(VuObject* self) { nw4r::ut::List_Init((nw4r::ut::List*)&self->list_0x04, 4); }

/* 0x805608F8 (0x270): Destroys the keyboard object's parts into its allocator, clears the parts it handed out and
 * destroys the Vu object. */
SysKbdView::~SysKbdView()
{
    if (field_0x5C != NULL) {
        field_0x5C->~SysKbdLayoutPart();
        MEMFreeToAllocator(mAllocator, field_0x5C);
    }
    if (field_0x60 != NULL) {
        field_0x60->~SysKbdSlidePart();
        MEMFreeToAllocator(mAllocator, field_0x60);
    }
    if (field_0x64 != NULL) {
        field_0x64->~SysKbdTextPart();
        MEMFreeToAllocator(mAllocator, field_0x64);
    }
    if (field_0x68 != NULL) {
        field_0x68->~SysKbdPredictPart();
        MEMFreeToAllocator(mAllocator, field_0x68);
    }
    if (field_0x6C != NULL) {
        field_0x6C->~SysKbdCursorPart();
        MEMFreeToAllocator(mAllocator, field_0x6C);
    }
    if (field_0x70 != NULL) {
        field_0x70->~SysKbdPart();
        MEMFreeToAllocator(mAllocator, field_0x70);
    }
    if (field_0x74 != NULL) {
        field_0x74->~SysKbdPart();
        MEMFreeToAllocator(mAllocator, field_0x74);
    }
    if (field_0x50 != NULL) {
        field_0x50->~SysKbdPart();
        MEMFreeToAllocator(mAllocator, field_0x50);
    }
    if (field_0x54 != NULL) {
        field_0x54->~SysKbdPart();
        MEMFreeToAllocator(mAllocator, field_0x54);
    }
    if (field_0x58 != NULL) {
        field_0x58->~SysKbdTextPart();
        MEMFreeToAllocator(mAllocator, field_0x58);
    }
    if (field_0x78 != NULL) {
        field_0x78->~HbmGuiPaneWidget();
        MEMFreeToAllocator(mAllocator, field_0x78);
    }
    mSoftKeyboard = NULL;
    mSlidePart = NULL;
    mPanel = NULL;
    mPredictPart = NULL;
    mCanceller = NULL;
    mAuxPart = NULL;
    mList = NULL;
}

/* Destroys the pane widget's pane manager (at +4). */
HbmGuiPaneWidget::~HbmGuiPaneWidget()
{
}

/* 0x80560BC4 (0x280): Builds the keyboard object's text buffers, its input object and its parts, gives the four states
 * their owner, enters state 0 and resets the saved keys before the first state. */
void hkbVuCreateKeyboard(HkbVuKeyboard* self)
{
    HkbVuWidget* widget = (HkbVuWidget*)self;

    self->textBuffers = widget->createTextBuffers();
    hbmTextBuffersInit(self->textBuffers, self->allocator);
    self->input = widget->createInput();

    SysKbdLayoutPart* layout = widget->createLayoutPart();
    self->defaultParts[0] = layout;
    layout->create(self->allocator);

    SysKbdSlidePart* slide = widget->createSlidePart();
    self->defaultParts[1] = slide;
    slide->create(self->allocator);

    SysKbdTextPart* text = widget->createTextPart();
    self->defaultParts[2] = text;
    text->createWithBuffers(self->allocator, self->textBuffers);

    SysKbdPredictPart* predict = widget->createPredictPart();
    self->defaultParts[3] = predict;
    predict->create(self->allocator);

    SysKbdCursorPart* cursor = widget->createCursorPart();
    self->defaultParts[4] = cursor;
    cursor->create(self->allocator);

    SysKbdPart* part70 = widget->createPart70();
    self->defaultParts[5] = part70;
    part70->create(self->allocator);

    SysKbdPart* part74 = widget->createPart74();
    self->defaultParts[6] = part74;
    part74->create(self->allocator);

    SysKbdTextPart* altText = widget->createAltTextPart();
    self->altWrapped[2] = altText;
    altText->createWithBuffers(self->allocator, self->textBuffers);

    HbmGuiPaneWidget* gui = widget->createGuiPart();
    self->guiPart = gui;
    gui->create(self->allocator);

    widget->v_0x100();
    hkbKbStateClosed.owner = self;
    hkbKbStateOpen.owner = self;
    hkbKbStateOpening.owner = self;
    hkbKbStateClosing.owner = self;
    HkbKbState* closing = &hkbKbStateClosing;
    closing->reset();
    hbmVuApplyAspectRatio((VuObject*)self);
    self->savedMode = 0xF0;
    self->savedCursor = 0;
    self->savedPanelShown = 0;
    self->savedListState = 0;
    self->savedKeysByte = 0;
    self->savedPanelByte = 0;
    self->savedPad[0] = 0;
    self->savedPad[1] = 0;
    widget->v_0x010();
}

/* 0x80560E44 (0x170): Enters the closed state and lets every part's root pane take its parent's alpha. */
void hkbVuInitState(HkbVuKeyboard* self)
{
    self->state = &hkbKbStateClosed;
    self->state->enter();
    self->state->calc();
    ((SysKbdLayoutPart*)self->defaultParts[0])->mPaneManager.getLayout()->GetRootPane()->SetInfluencedAlpha(true);
    ((SysKbdSlidePart*)self->defaultParts[1])->mPaneManager.getLayout()->GetRootPane()->SetInfluencedAlpha(true);
    ((SysKbdTextPart*)self->defaultParts[2])->mPaneManager.getLayout()->GetRootPane()->SetInfluencedAlpha(true);
    ((SysKbdTextPart*)self->altWrapped[2])->mPaneManager.getLayout()->GetRootPane()->SetInfluencedAlpha(true);
    ((SysKbdPredictPart*)self->defaultParts[3])->mPaneManager.getLayout()->GetRootPane()->SetInfluencedAlpha(true);
    ((SysKbdCursorPart*)self->defaultParts[4])->mPaneManager.getLayout()->GetRootPane()->SetInfluencedAlpha(true);
    self->guiPart->getLayout()->GetRootPane()->SetInfluencedAlpha(true);
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

#pragma push
#pragma global_optimizer off
/* 0x80561074 (0xD4): Opens (modes 0 and 1) or closes (modes 2 and 3) the panel at once: plays the sound, leaves the
 * current state and enters the open or the closed one. */
void hkbVuSetOpenImmediate(HkbVuKeyboard* self, s32 mode)
{
    switch (mode) {
    case 0:
    case 1:
        self->sound->play(28);
        self->state->leave();
        self->state = &hkbKbStateOpen;
        self->state->enter();
        break;
    case 2:
    case 3:
        self->sound->play(29);
        self->state->leave();
        self->state = &hkbKbStateClosed;
        self->state->enter();
        break;
    }
}
#pragma pop

/* ---------------------------------------------------------------------------------------------------
 * stores into the unit's small-data globals and the float pools
 * ------------------------------------------------------------------------------------------------ */
/* 0x805615F8 (0x84): Builds the keyboard object's gui part, a pane widget on the background layout. */
HbmGuiPaneWidget* hkbVuCreateGuiPart(HkbVuKeyboard* self)
{
    return new (MEMAllocFromAllocator(self->allocator, sizeof(HbmGuiPaneWidget)))
        HbmGuiPaneWidget(self->resources, "fs_VK_bg_a.brlyt", self->sound);
}

/* 0x8056167C (0xC): Sets where the sliding parts end. */
void hkbKbSetSlideEnd(f32 value) { hkbKbSlideY[1] = value; }

/* 0x80561688 (0x130): Enters state 0 with the default parts and the main panel. */
void hkbVuEnterState0(HkbVuKeyboard* self)
{
    self->field_0x44 = 0;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->defaultParts[2];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(1);
    if (self->layoutType != 1 && self->layoutType != 2) {
        ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    }
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
    ((HkbVuWidget*)self)->v_0x03C(0x100);
    ((HkbVuWidget*)self)->v_0x044(2);
    ((HkbVuWidget*)self)->v_0x040(0x100);
}

/* 0x805617B8 (0x520): Enters state 0 on the kana panel in kana mode `mode`: hides the other modes' panes and shows
 * the ones `mode` uses. */
void hkbVuEnterKanaState0(HkbVuKeyboard* self, s32 mode)
{
    self->field_0x44 = 0;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->defaultParts[2];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbKbPart3C*)self->field_0x14)->v_0x124(0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_hiragana", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_hiragana", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_katakana", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_katakana", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_abc", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_sign", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_sign", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("P_dakuten", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Gkey_dakuten", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("P_handakuten", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Gkey_handaku", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_hankaku", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_zenkaku", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_hanzen", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_action", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_action", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_sign", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_sign", 1);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_Gkey_55", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Gkey_55", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_Mode_roma_hira", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Mode_roma_hira", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_Mode_roma_kata", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Mode_roma_kata", 0);
    ((HkbKbPart3C*)self->field_0x14)->kanaFlags = 0;
    ((HkbKbPart3C*)self->field_0x14)->kanaMode = mode;
    if (((HkbKbPart3C*)self->field_0x14)->kanaMode == 1) {
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_action", 0);
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_action", 0);
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_sign", 0);
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_sign", 0);
    } else if (((HkbKbPart3C*)self->field_0x14)->kanaMode == 2) {
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_action", 0);
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_action", 0);
    } else if (((HkbKbPart3C*)self->field_0x14)->kanaMode == 3) {
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_sign", 0);
        ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_sign", 0);
    }
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
    ((HkbKbPart3C*)self->field_0x14)->v_0x134((u8)self->field_0x44);
    ((HkbVuWidget*)self)->v_0x03C(0x100);
    ((HkbVuWidget*)self)->v_0x044(2);
    ((HkbVuWidget*)self)->v_0x040(0x100);
}

/* 0x80561CD8 (0x420): Enters state 14 on the kana panel with every mode pane hidden and `flags` set. */
void hkbVuEnterState14(HkbVuKeyboard* self, u32 flags)
{
    self->field_0x44 = 14;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->defaultParts[2];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbKbPart3C*)self->field_0x14)->v_0x124(0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_hiragana", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_hiragana", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_katakana", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_katakana", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_abc", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_sign", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_sign", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("P_dakuten", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Gkey_dakuten", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("P_handakuten", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Gkey_handaku", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_hankaku", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_zenkaku", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_hanzen", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_action", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_action", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_JP_Chng_sign", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_JP_Chng_sign", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_Gkey_55", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Gkey_55", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_Mode_roma_hira", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Mode_roma_hira", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("T_Mode_roma_kata", 0);
    ((HkbKbPart3C*)self->field_0x14)->child_0x4C.setPaneVisible("B_Mode_roma_kata", 0);
    ((HkbKbPart3C*)self->field_0x14)->kanaFlags = flags;
    ((HkbKbPart3C*)self->field_0x14)->kanaMode = 0;
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
    ((HkbKbPart3C*)self->field_0x14)->v_0x134((u8)self->field_0x44);
    ((HkbVuWidget*)self)->v_0x03C(0x100);
    ((HkbVuWidget*)self)->v_0x044(1);
    ((HkbVuWidget*)self)->v_0x040(0x100);
}

/* 0x805620F8 (0x14C): Enters state 3 with the second wrapped keyboard and the side parts switched off. */
void hkbVuEnterState3(HkbVuKeyboard* self)
{
    self->field_0x44 = 3;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->altWrapped[2];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(0);
    ((HkbVuPart*)self->field_0x24)->v_0x0AC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x054(1);
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
}

void fn_80562244(VuObject* self)
{
    ((HkbVuWidget*)self)->v_0x114();
    self->field_0x44 = 0xB;
    ((HkbVuPart*)self->sub_0x1C)->v_0x23C(1);
}

/* 0x8056229C (0x190): Enters state 1 with the first alternative wrapped keyboard, the layout's labels on the list
 * and the panel at y 145. */
void hkbVuEnterState1(HkbVuKeyboard* self)
{
    self->field_0x44 = 1;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->altWrapped[0];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    s32 type = ((HkbVuPart*)self->field_0x24)->v_0x01C();
    const wchar_t* label = hkbLayoutLabels[0][type];
    const wchar_t* label2 = hkbLayoutLabels[9][type];
    ((HkbVuPart*)self->field_0x24)->v_0x0B8(1);
    ((HkbVuPart*)self->field_0x24)->v_0x0C0(label);
    ((HkbVuPart*)self->field_0x24)->v_0x0C4(label2);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(1);
    ((HkbVuWidget*)self)->v_0x0F4();
    ((HkbVuWidget*)self)->v_0x03C(0x100);
    ((HkbVuWidget*)self)->v_0x044(0x10);
    ((HkbVuWidget*)self)->v_0x040(0x200);
    hkbKbPanelY[1] = 145.0f;
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
}

/* 0x8056242C (0x198): Enters state 2 with the second alternative wrapped keyboard, the layout's labels on the list
 * and the panel at y 151. */
void hkbVuEnterState2(HkbVuKeyboard* self)
{
    self->field_0x44 = 2;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->altWrapped[1];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x0B8(1);
    ((HkbVuPart*)self->field_0x24)->v_0x0C0(hkbLayoutLabels[0][((HkbVuPart*)self->field_0x24)->v_0x01C()]);
    ((HkbVuPart*)self->field_0x24)->v_0x0C4(hkbLayoutLabels[9][((HkbVuPart*)self->field_0x24)->v_0x01C()]);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(1);
    ((HkbVuWidget*)self)->v_0x0F4();
    ((HkbVuWidget*)self)->v_0x03C(0x100);
    ((HkbVuWidget*)self)->v_0x044(0x10);
    ((HkbVuWidget*)self)->v_0x040(0x100);
    hkbKbPanelY[1] = 151.0f;
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
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

/* 0x805626DC (0x1F8): Enters state 5: the main panel on the third alternative wrapped keyboard, then every side part
 * switched off but the list. */
void hkbVuEnterState5(HkbVuKeyboard* self)
{
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    self->sub_0x1C = (VuSub*)self->altWrapped[2];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(1);
    if (self->layoutType != 1 && self->layoutType != 2) {
        ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    }
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
    self->field_0x44 = 5;
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->field_0x24)->v_0x0AC(1);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x18)->v_0x110(0);
    ((HkbVuPart*)self->field_0x14)->v_0x128(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
}

/* 0x805628D4 (0x194): Enters state 6: the default parts with the list off and every side part switched off. */
void hkbVuEnterState6(HkbVuKeyboard* self)
{
    self->field_0x44 = 6;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->defaultParts[2];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x14)->v_0x060(1);
    ((HkbVuPart*)self->field_0x24)->v_0x0AC(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->field_0x18)->v_0x110(0);
    ((HkbVuPart*)self->field_0x14)->v_0x128(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
}

/* 0x80562A68 (0x194): Enters state 7: state 6 on the third alternative wrapped keyboard. */
void hkbVuEnterState7(HkbVuKeyboard* self)
{
    self->field_0x44 = 7;
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->sub_0x1C = (VuSub*)self->altWrapped[2];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x14)->v_0x060(1);
    ((HkbVuPart*)self->field_0x24)->v_0x0AC(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->field_0x18)->v_0x110(0);
    ((HkbVuPart*)self->field_0x14)->v_0x128(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
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

/* 0x80562C94 (0x1F8): Enters state 10: the main panel on the third alternative wrapped keyboard, then the side parts
 * switched off and the list and the cursor parts on. */
void hkbVuEnterState10(HkbVuKeyboard* self)
{
    self->field_0x14 = self->defaultParts[0];
    self->field_0x18 = self->defaultParts[1];
    self->field_0x20 = self->defaultParts[3];
    self->field_0x24 = self->defaultParts[4];
    self->field_0x28 = self->defaultParts[5];
    self->field_0x2C = self->defaultParts[6];
    self->sub_0x1C = (VuSub*)self->altWrapped[2];
    hbmVuInitParts((VuObject*)self);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(1);
    if (self->layoutType != 1 && self->layoutType != 2) {
        ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    }
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
    self->field_0x44 = 10;
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->field_0x24)->v_0x0AC(1);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x18)->v_0x110(1);
    ((HkbVuPart*)self->field_0x14)->v_0x128(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
}

/* 0x80562E8C (0x158): Re-enters state 10 on the current parts after slot 0x100 picks them. */
void hkbVuReenterState10(HkbVuKeyboard* self)
{
    ((HkbVuWidget*)self)->v_0x100();
    hbmVuInitParts((VuObject*)self);
    self->field_0x44 = 10;
    ((HkbVuPart*)self->field_0x18)->v_0x108(0);
    ((HkbVuPart*)self->field_0x14)->v_0x120(0);
    ((HkbVuPart*)self->field_0x20)->v_0x0FC(0);
    ((HkbVuPart*)self->field_0x18)->v_0x10C(0);
    ((HkbVuPart*)self->field_0x14)->v_0x124(0);
    ((HkbVuPart*)self->field_0x24)->v_0x0AC(1);
    ((HkbVuPart*)self->field_0x24)->v_0x014(1);
    ((HkbVuPart*)self->field_0x18)->v_0x110(1);
    ((HkbVuPart*)self->field_0x14)->v_0x128(1);
    ((HkbVuPart*)self->sub_0x1C)->v_0x108(0);
    ((HkbVuPart*)self->sub_0x1C)->v_0x138(0);
    hbmPanelRelayout((HbmPanelView*)(HkbVuPart*)self->sub_0x1C);
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

/* 0x805630AC (0x1F0): Saves the keyboard state: the mode, the cursor (250 when it rests on the mode's own key), the
 * panel and list states, the shift, layout and caps keys, the panel position and whether the modifier is off. */
void hkbVuSaveKeys(HkbVuKeyboard* self)
{
    s32 modeKeys[11] = {0, 0, 1, 3, 6, 2, 4, 3, 6, 5, 7};

    self->savedMode = ((HkbKbPart4C*)self->field_0x24)->v_0x010() | 0x10;
    self->savedCursor = ((HkbKbWrapped*)self->sub_0x1C)->v_0x118();
    if (self->savedCursor < 11 && modeKeys[self->savedCursor] == ((HkbVuWidget*)self)->v_0x034()) {
        self->savedCursor = 250;
    }
    self->savedPanelShown = ((HkbKbPart48*)self->field_0x20)->v_0x034();
    self->savedListState = ((HkbKbPart44*)self->field_0x2C)->v_0x040();
    self->savedKeys.shift = ((HkbKbPart40*)self->field_0x18)->v_0x05C();
    self->savedKeys.layout = ((HkbKbPart40*)self->field_0x18)->v_0x040();
    self->savedKeys.caps = ((HkbKbPart3C*)self->field_0x14)->v_0x048();
    self->savedKeys.dirty = 0;
    self->savedPanel.column = ((HkbKbPart3C*)self->field_0x14)->cursorColumn;
    self->savedPanel.row = ((HkbKbPart3C*)self->field_0x14)->cursorRow;
    self->savedKeys.modifierOff = !(hkbGetModifierState(&hkbManager) & 0x100);
    self->savedPad[0] = 0;
    self->savedPad[1] = 0;
}

/* The nibble dispatcher: restores the saved keys (mode 1) or resets them, by bits 4..7 of +0x48. */
#pragma push
#pragma global_optimizer off
void fn_8056329C(VuObject* self)
{
    switch ((self->field_0x48 >> 4) & 0xF) {
    case 1:
        hkbVuRestoreKeys((HkbVuKeyboard*)self);
        break;
    case 0:
    default:
        hkbVuResetKeys((HkbVuKeyboard*)self);
        break;
    }
}
#pragma pop
/* 0x805632BC (0x1E8): Restores the keyboard state hkbVuSaveKeys saved. */
void hkbVuRestoreKeys(HkbVuKeyboard* self)
{
    if ((self->savedMode & 1) == 1) {
        ((HkbKbPart4C*)self->field_0x24)->v_0x014(1);
    } else {
        ((HkbKbPart4C*)self->field_0x24)->v_0x014(0);
    }
    ((HkbKbPart44*)self->field_0x2C)->v_0x03C(self->savedListState);
    if (self->savedPanelShown == 0) {
        ((HkbKbPart48*)self->field_0x20)->v_0x02C();
    } else {
        ((HkbKbPart48*)self->field_0x20)->v_0x028();
    }
    if (self->savedCursor != 250) {
        ((HkbKbWrapped*)self->sub_0x1C)->v_0x114(self->savedCursor);
    }
    ((HkbKbPart3C*)self->field_0x14)->v_0x064(self->savedKeys.caps, self->savedPanel.column, self->savedPanel.row);
    ((HkbKbPart40*)self->field_0x18)->v_0x064(self->savedKeys.shift);
    ((HkbKbPart40*)self->field_0x18)->v_0x060(self->savedKeys.layout);
    if (!((HkbKbPart4C*)self->field_0x24)->v_0x010() && ((HkbKbPart40*)self->field_0x18)->v_0x040() == 3) {
        ((HkbKbPart48*)self->field_0x20)->v_0x040(1);
    } else {
        ((HkbKbPart48*)self->field_0x20)->v_0x040(0);
    }
    hkbSetModifierState(&hkbManager, !self->savedKeys.modifierOff ? 0x100 : 0, 0x100);
}

/* 0x805634A4 (0x16C): Resets the keyboard state: list and panel off, the English keys and the modifier on. */
void hkbVuResetKeys(HkbVuKeyboard* self)
{
    ((HkbKbPart4C*)self->field_0x24)->v_0x014(1);
    ((HkbKbPart44*)self->field_0x2C)->v_0x03C(0);
    if (self->language == 0) {
        ((HkbKbPart48*)self->field_0x20)->v_0x028();
    } else {
        ((HkbKbPart48*)self->field_0x20)->v_0x02C();
    }
    if (self->language == 0) {
        ((HkbKbPart3C*)self->field_0x14)->v_0x064(false, 1, 0);
    }
    ((HkbKbPart40*)self->field_0x18)->v_0x064(true);
    ((HkbKbPart40*)self->field_0x18)->v_0x060(2);
    if (!((HkbKbPart4C*)self->field_0x24)->v_0x010() && ((HkbKbPart40*)self->field_0x18)->v_0x040() == 3) {
        ((HkbKbPart48*)self->field_0x20)->v_0x040(1);
    } else {
        ((HkbKbPart48*)self->field_0x20)->v_0x040(0);
    }
    hkbSetModifierState(&hkbManager, 0x100, 0x100);
}

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

/* A point built in place, as the panes' translations take it.  size: 0x8 */
struct HkbPoint : public nw4r::math::VEC2 {
    HkbPoint(f32 px, f32 py) {
        x = px;
        y = py;
    }
}; /* size: 0x8 */

/* Places a pane at `pos` in its plane. */
static inline void hkbPaneSetTranslate(nw4r::lyt::Pane* pane, const nw4r::math::VEC2& pos)
{
    nw4r::math::VEC3 translate;
    translate.x = pos.x;
    translate.y = pos.y;
    translate.z = 0.0f;
    pane->mTranslate = translate;
}

/* Sets a pane's alpha. */
static inline void hkbPaneSetAlpha(nw4r::lyt::Pane* pane, u8 alpha)
{
    pane->mAlpha = alpha;
}

#pragma push
#pragma global_optimizer off
/* 0x805638D4 (0x518): Puts every animated pane at its start: the wrapped keyboard's panel while it is active,
 * otherwise the sliding parts at their start y and alpha. */
void hkbKbAnimRewindPanes(HkbKbAnimView* self)
{
    switch (self->wrapped->v_0x0F0()) {
    case 1:
    case 2:
        hbmKbCursorSetMode(((HkbKbOwner*)self)->v_0x034(), 1);
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, HkbPoint(0.0f, hkbKbPanelY[0]));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, 255);
        break;
    default:
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, HkbPoint(0.0f, hkbKbSlideY[0]));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, hkbKbAlpha[0]);
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, HkbPoint(0.0f, hkbKbSlideY[0]));
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, HkbPoint(0.0f, hkbKbSlideY[0]));
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, HkbPoint(0.0f, hkbKbSlideY[0]));
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), HkbPoint(0.0f, hkbKbSlideY[0] / 3.0f));
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), HkbPoint(0.0f, -hkbKbSlideY[0] / 3.0f));
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, HkbPoint(0.0f, 0.0f));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, hkbKbAlpha[0]);
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, hkbKbAlpha[0]);
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, hkbKbAlpha[0]);
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), hkbKbAlpha[0]);
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), hkbKbAlpha[0]);
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, hkbKbAlpha[0]);
        break;
    }
}
#pragma pop

void fn_80563DEC(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x078(); }

void fn_80563E00(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x080(); }

void fn_80563E14(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x090(); }

void fn_80563E28(KbObject* self) { ((HkbKbWrapped*)self->sub_0x04)->v_0x098(); }

void* fn_80563E3C(KbObject* self) { return self->sub_0x04->field_0x78; }

void fn_80563E48(KbObject* self) { (void)self; }

void fn_80563E4C(KbObject* self) { (void)self; }

#pragma push
#pragma global_optimizer off
/* 0x80563E50 (0x6A8): Refreshes the parts and puts every animated pane at the show animation's current frame (a
 * Hermite curve over 30 frames), then marks the animation finished. */
void hkbKbAnimApplyShow(HkbKbAnimView* self)
{
    f32 y;
    u8 alpha;
    f32 panelY;
    HkbKbWrapped* wrapped;
    f32 to;

    ((HkbKbOwner*)self)->v_0x034()->v_0x14C();
    if (((HkbKbOwner*)self)->v_0x04C()->v_0x010()) {
        ((HkbKbOwner*)self)->v_0x03C()->v_0x114();
    } else {
        ((HkbKbOwner*)self)->v_0x040()->v_0x038();
    }
    if (((HkbKbOwner*)self)->v_0x048()->v_0x100()) {
        ((HkbKbOwner*)self)->v_0x048()->v_0x0CC();
    }
    ((HkbKbOwner*)self)->v_0x04C()->child_0x14.v_0x01C();
    ((HkbKbOwner*)self)->v_0x054()->child_0x04.v_0x01C();
    if (30.0f <= self->frame) {
        self->wrapped->v_0x0E4(1);
    }
    y = hbmHermite(self->frame, 0.0f, hkbKbSlideY[0], 0.0f, 30.0f, hkbKbSlideY[1], 0.0f);
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, HkbPoint(0.0f, y));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, HkbPoint(0.0f, y));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, HkbPoint(0.0f, y));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), HkbPoint(0.0f, y / 3.0f));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), HkbPoint(0.0f, -y / 3.0f));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, HkbPoint(0.0f, 0.0f));
    alpha = hbmHermite(self->frame, 0.0f, hkbKbAlpha[0], 0.0f, 30.0f, hkbKbAlpha[1], 0.0f);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, alpha);
    switch (self->wrapped->v_0x0F0()) {
    case 1:
    case 2:
        panelY = hbmHermite(self->frame, 0.0f, hkbKbPanelY[0], 0.0f, 30.0f, hkbKbPanelY[1], 0.0f);
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane,
                            HkbPoint(0.0f, panelY));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, 255);
        wrapped = ((HkbKbOwner*)self)->v_0x034();
        to = wrapped->v_0x254();
        wrapped->v_0x244(hbmHermite(self->frame, 0.0f, wrapped->v_0x250(), 0.0f, 30.0f, to, 0.0f));
        break;
    default:
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, HkbPoint(0.0f, y));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, alpha);
        break;
    }
    self->frame = 30.0f;
}
#pragma pop

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

#pragma push
#pragma global_optimizer off
/* 0x80564780 (0x74): Rewinds the animation and, while the wrapped keyboard is active, puts the cursor in mode 0. */
void hkbKbAnimRewindCursor0(HkbKbAnimView* self)
{
    self->frame = 0.0f;
    switch (self->wrapped->v_0x0F0()) {
    case 1:
    case 2:
        hbmKbCursorSetMode(((HkbKbOwner*)self)->v_0x034(), 0);
        break;
    }
}
#pragma pop

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

/* 0x805655C4 (0x604): Puts every animated pane at its end: the wrapped keyboard's panel while it is active,
 * otherwise at the sliding parts' end y and alpha; then hands the parts the mode's focus. */
void hkbKbAnimFinishPanes(HkbKbAnimView* self)
{
    if (self->wrapped->v_0x0F0() == 1 || self->wrapped->v_0x0F0() == 2) {
        hbmKbCursorSetMode(((HkbKbOwner*)self)->v_0x034(), 2);
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane,
                            HkbPoint(0.0f, hkbKbPanelY[1]));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, 255);
    } else {
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane,
                            HkbPoint(0.0f, hkbKbSlideY[1]));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, hkbKbAlpha[1]);
    }
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane,
                        HkbPoint(0.0f, hkbKbSlideY[1]));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane,
                        HkbPoint(0.0f, hkbKbSlideY[1]));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane,
                        HkbPoint(0.0f, hkbKbSlideY[1]));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), HkbPoint(0.0f, hkbKbSlideY[1] / 3.0f));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), HkbPoint(0.0f, -hkbKbSlideY[1] / 3.0f));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, HkbPoint(0.0f, 0.0f));
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, hkbKbAlpha[1]);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, hkbKbAlpha[1]);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, hkbKbAlpha[1]);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), hkbKbAlpha[1]);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), hkbKbAlpha[1]);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, hkbKbAlpha[1]);
    ((HkbKbOwner*)self)->v_0x03C()->v_0x034();
    ((HkbKbOwner*)self)->v_0x040()->v_0x034();
    if (((HkbKbOwner*)self)->v_0x04C()->v_0x010()) {
        ((HkbKbOwner*)self)->v_0x03C()->v_0x034();
    } else {
        ((HkbKbOwner*)self)->v_0x040()->v_0x034();
    }
}

#pragma push
#pragma global_optimizer off
/* 0x80565BC8 (0x550): Puts every animated pane at the hide animation's current frame (the show curve run back over
 * 30 frames), then marks the animation finished. */
void hkbKbAnimApplyHide(HkbKbAnimView* self)
{
    f32 y;
    u8 alpha;
    f32 panelY;

    if (30.0f <= self->frame) {
        self->wrapped->v_0x0E4(1);
    }
    ((HkbKbOwner*)self)->v_0x034()->v_0x14C();
    y = hbmHermite(self->frame, 0.0f, hkbKbSlideY[1], 0.0f, 30.0f, hkbKbSlideY[0], 0.0f);
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, HkbPoint(0.0f, y));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, HkbPoint(0.0f, y));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, HkbPoint(0.0f, y));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), HkbPoint(0.0f, y / 3.0f));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), HkbPoint(0.0f, -y / 3.0f));
    hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, HkbPoint(0.0f, 0.0f));
    alpha = hbmHermite(self->frame, 0.0f, hkbKbAlpha[1], 0.0f, 30.0f, hkbKbAlpha[0], 0.0f);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x03C()->child_0x4C.getLayout()->mpRootPane, alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x040()->child_0x34.getLayout()->mpRootPane, alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x048()->child_0x17F4.getLayout()->mpRootPane, alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B4(), alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x04C()->v_0x0B0(), alpha);
    hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x054()->child_0x04.getLayout()->mpRootPane, alpha);
    switch (self->wrapped->v_0x0F0()) {
    case 1:
    case 2:
        panelY = hbmHermite(self->frame, 0.0f, hkbKbPanelY[1], 0.0f, 30.0f, hkbKbPanelY[0], 0.0f);
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane,
                            HkbPoint(0.0f, panelY));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, 255);
        break;
    default:
        hkbPaneSetTranslate(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, HkbPoint(0.0f, y));
        hkbPaneSetAlpha(((HkbKbOwner*)self)->v_0x034()->child_0x1A04.getLayout()->mpRootPane, alpha);
        break;
    }
    self->frame = 30.0f;
}
#pragma pop

#pragma push
#pragma global_optimizer off
/* 0x80566118 (0xB0): Rewinds the animation; while the wrapped keyboard is active it puts the cursor in mode 3 and
 * runs the wrapped keyboard's slot 0xDC, otherwise the wrapped keyboard's slot 0x13C. */
void hkbKbAnimRewindCursor3(HkbKbAnimView* self)
{
    self->frame = 0.0f;
    switch (self->wrapped->v_0x0F0()) {
    case 1:
    case 2:
        hbmKbCursorSetMode(((HkbKbOwner*)self)->v_0x034(), 3);
        self->wrapped->v_0x0DC();
        break;
    default:
        ((HkbKbOwner*)self)->v_0x034()->v_0x13C();
        break;
    }
}
#pragma pop

void fn_805661C8(KbObject* self) { (void)self; }

void fn_805661D4(KbObject* self) { (void)self; }

s32 fn_805661D8(KbObject* self) { (void)self; return 3; }

void fn_805661E0(KbObject* self) { (void)self; }

s32 fn_805661E4(KbObject* self) { (void)self; return 2; }

/* The five deleting destructors of the state classes (0x805661EC, 0x8056622C, 0x80566280, 0x805662C0, 0x80566300) and
 * the static initialiser (0x80566340) are the compiler's: HkbKbState's inline destructors and the four state objects. */
s32 fn_8056626C(KbObject* self) { (void)self; return 1; }

void fn_80566274(KbObject* self) { (void)self; }

s32 fn_80566278(KbObject* self) { (void)self; return 0; }

}
