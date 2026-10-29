/*
 * homebutton/tiHKBManager.cpp - the tiHKBManager software keyboard: the gui component/pane managers and their debug
 * outline, the USB keyboard manager singleton (its KBD/HID start-up, channels, LED requests and observer list), the
 * per-frame channel state machine and the key iterator the input widgets read.
 *
 * RANGE.     .text 0x8056A478..0x8056D814 (79 functions), .ctors 0x8056F428..0x8056F42C, .data 0x80658290..0x80658458,
 *            .bss 0x80790D58..0x80790DF8, .sdata2 0x8079D7A0..0x8079D7B0; the functions not defined below keep the
 *            target object's bytes.
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (see src/homebutton/hbm_kb_event.cpp); no extab/extabindex in the target, so
 *            `#pragma exceptions off`; `#pragma auto_inline on` around hkbGuiPaneManagerCreateLayoutScene/WalkInChildren
 *            (retail inlines the recursive walk three levels deep).
 * NAMES.     File name from the `__FILE__` string "tiHKBManager.cpp" (0x80658408) that hkbNodeRemove's OSPanic cites; the
 *            source spells it as a literal (the same bytes; our command line's `__FILE__` carries the directory).
 * NAMES.     GUESS classes HkbGuiComponent/HkbGuiPaneComponent (the HBM gui Component/PaneComponent: fields from the
 *            constructor hkbGuiPaneManagerAddPane inlines, slots init/setTriggerTarget/setPane from their callers;
 *            map rows of their tables renamed), the views HkbGuiManagerView and HkbGuiEventTarget (slots named by
 *            offset where no body names them), the classes HkbNode (observer base) and HkbManagerNode (the manager's sentinel; map
 *            rows renamed to their manglings) and the types HkbManager, HkbChannel, HkbRecord, HkbArg, HkbSlotMsg,
 *            HkbKeyIter, HkbStageEventArgs: named from the bodies that use them.
 * NAMES.     GUESS function names (map rows renamed): hkbGuiDrawLine, hkbGuiManagerUpdate, hkbGuiUpdateStageFromArgs,
 *            hkbGuiPaneManagerCreateLayoutScene/WalkInChildren/AddPane, hkbGuiSetBoundingTriggerTargets,
 *            hkbGuiPaneComponentContains/Draw (the HBM gui manager's drawLine_, Manager::update, the PaneManager scene
 *            walk and the PaneComponent members, by their bodies); hkbPaneListGetBeginIter / hkbPaneIterNotEqual (the
 *            out-of-line copies of nw4r::ut::LinkList<Pane,4>::GetBeginIter and Iterator operator!=);
 *            hkbLedRequestDone, hkbExtraLedRequestDone, hkbNotifyConnect/Disconnect/Key (the KBD callbacks),
 *            hkbHidDone, hkbManagerStartDriver/Init/Shutdown/SetMap/Update, hkbChannelKeyEvent/Update/UpdateModifiers,
 *            hkbKeyIterSeek/Valid/GetUsage/GetCode/GetChar/GetCharForUsage, hkbKeyIterBeginWord0/1/3,
 *            hkbSetModifierOverride (channel 0's override value and mask).
 * SHAPES.    The gui classes are polymorphic so MWCC emits the canonical `lwz r12,0(r3); lwz r12,N(r12)` dispatch;
 *            hkbGuiPaneManagerAddPane constructs a PaneComponent through the inline constructors (table stores, the
 *            virtual init call), the non-allocator node through a temporary assigned after its stores (a
 *            new-expression's order).
 * SHAPES.    hkbManager is a global with a constructor and an inline destructor, so MWCC emits the static initializer
 *            (`__sinit_	iHKBManager_cpp`), its .ctors word and the destructor-chain record ahead of hkbManager in
 *            .bss; the destructor is emitted after the initializer, as retail.
 * SHAPES.    The sentinel's class derives from the observer base HkbNode: retail stores prev/next before the table
 *            pointer, the order a base constructor gives (the base's own table store is dead and dropped).
 * SHAPES.    hkbNotifyConnect/Disconnect/Key compare against `&hkbManager.sentinel` itself: a named sentinel local is
 *            coloured r4 where retail keeps it in r0.
 * SHAPES.    Local declaration order decides the callee-saved colouring (the first declared takes the highest
 *            register): fn_8056A638 declares `idx` before `result`, HkbManagerNode::onConnect declares `mgr` first,
 *            hkbManagerUpdate declares `saved`, `msg`, `bit`, `index` in that order.
 * SHAPES.    The one-channel loops: a `u8` counter keeps retail's `mulli` by the zero index (the HkbManager constructor,
 *            hkbManagerInit/SetMap/Update, hkbSetModifierState); an `int` counter folds it (hkbManagerShutdown).
 * SHAPES.    The key iterator's validity, usage and lookup are inline members (HkbKeyIter::isValid/getUsage/lookupCode)
 *            inlined into the C entry points; hkbKeyIterGetCode is written out, not wrapped (a wrapper adds a copy).
 *            hkbKeyIterGetUsage's switch keeps four separate cases; hkbKeyIterSeek takes a non-const cursor (retail
 *            reloads `start->manager` after the stores).
 * SHAPES.    hkbChannelUpdateModifiers tests the HID modifier usages in an if/else chain (a switch becomes a jump
 *            table, which also loses the CTR loop); the LED bits come from the inline hkbLedBits.
 * SHAPES.    hkbGuiManagerUpdate: cases 1 and 3 reach the shared contained block through case labels inside `if (0)`
 *            blocks (the block sits after case 5, as retail lays it out); `c` is declared before the other locals.
 * SHAPES.    hkbSetModifierState materialises the lock change as `((old - locks) | (locks - old)) >> 31` (retail's
 *            subf/subf/or/srwi.; both words are masked to 0x700, so it is `old != locks`).
 * SHAPES.    HkbManager::filterAllowed tests the allow-list pointer through the inline getAllowedChars: that keeps the
 *            pointer out of a register, so the loop reloads it and indexes with `lhzx` as retail.
 * SHAPES.    hkbGuiPaneComponentDraw halves the size with `/ 2.0f` (retail's fmuls operand order).
 * RESIDUALS. fn_8056A638: retail schedules `lfs f31,4(r4)` before `mr r28,r4`; every declaration order tried keeps the
 *            move first.
 * RESIDUALS. HkbManagerNode::onDisconnect: retail holds the manager in r0 and the scaled index in r6 (ours r6/r0).
 * RESIDUALS. hkbSetModifierState: the first statement's and/andc/or and the channel pointer take other registers
 *            (retail r0/r4/r7, ours r4/r5/r0).
 * RESIDUALS. hkbManagerSetMap: the channel pointer is r27 in retail (r29 ours); an inline setMap member scores lower.
 * RESIDUALS. hkbChannelUpdate: register numbering of the bit/index/usage-pointer triple in the edge loops.
 * RESIDUALS. hkbGuiDrawLine: retail reloads the colour word before each FIFO write (also hbm_gui_draw's residual).
 * RESIDUALS. hkbGuiSetBoundingTriggerTargets: ours guards the loop entry (`lhz; cmpwi; beq`), retail does not (an int
 *            or u16 index, a named pane or cast local measured no better).
 * RESIDUALS. hkbGuiPaneComponentContains: stack slots of the point, the rectangle and the local point.
 * RESIDUALS. hkbGuiPaneManagerWalkInChildren: retail stops inlining the iterator at the third level and calls
 *            hkbPaneListGetBeginIter / hkbPaneIterNotEqual; ours inlines all three levels (inline_depth,
 *            inline_max_size and inline_max_total_size pragmas change nothing; spelling the levels out loses
 *            hkbGuiPaneManagerCreateLayoutScene's inlining).
 * RESIDUALS. The gui vtables __vt__19HkbGuiPaneComponent, lbl_806582FC, lbl_80658358 and __vt__15HkbGuiComponent
 *            are not emitted (their slots name other units' folded inline functions, so the classes keep their key
 *            functions undefined); our .data is the two strings and __vt__14HkbManagerNode.
 * RESIDUALS. HkbNode's table is not emitted here (an unrelocated `__vt__7HkbNode` symbol entry remains) and its
 *            inline destructor is emitted as a weak 0x40-byte copy retail's object does not carry.
 * RESIDUALS. datagap: the claimed `.data` 0x80658290..0x80658458 spans at least 3 TUs (emission-order seams at
 *            0x806583A0 and in [0x80658408, 0x80658440)) - a seam question for the registration.
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/gui_manager.h"
#include "NAND/nand.h"
#include "MEM/mem_allocator.h"
#include "MEM/mem_expheap.h"
#include "KBD/kbd.h"
#include "KBD/kbd_region_us.h"
#include "homebutton/hbm_hermite.h"
#include "homebutton/tiHKBManager.h"
#include "nw4r/fn_805012C4.h"
#include "HID/hid.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_bounding.h"
#include "homebutton/hkb_window.h"
#include "MTX/mtx.h"
#include "MTX/mtxvec.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTransform.h"
#include "gx.h"

/* The vtables this unit's managers store (the unit's own `.data`; their classes are not reconstructed). */
extern "C" {

extern char lbl_806582FC[];

extern char lbl_80658358[];

}

/* ---- types -------------------------------------------------------------------------------------- */

struct HkbManager;

struct HkbArg;

/* An observer on the manager's circular list: prev and next, and the handlers the KBD callbacks run for every
 * node (connect, disconnect, key).  Its table is not emitted here (onConnect is the key function and is
 * not defined here); the constructor's base table store folds into the derived one.
 * size: 0x0C */
class HkbNode {
public:
    HkbNode() : prev(0), next(0) {}
    /* slot +0x008 */ virtual ~HkbNode() {}
    /* slot +0x00C */ virtual void onConnect(HkbArg* arg);
    /* slot +0x010 */ virtual void onDisconnect(HkbArg* arg);
    /* slot +0x014 */ virtual void onKey(HkbArg* arg);

    /* +0x04 */ HkbNode* prev;
    /* +0x08 */ HkbNode* next;
};

/* The manager's own observer, the sentinel of the list: it points back at the manager and its handlers keep the
 * channels' connected state.  Its first handler is defined here, so MWCC emits its table (0x80658440).
 * size: 0x10 */
class HkbManagerNode : public HkbNode {
public:
    HkbManagerNode(HkbManager* owner) : manager(owner) {}
    /* slot +0x00C */ virtual void onConnect(HkbArg* arg);
    /* slot +0x010 */ virtual void onDisconnect(HkbArg* arg);
    /* slot +0x014 */ virtual void onKey(HkbArg* arg);

    /* +0x0C */ HkbManager* manager;
};

/* The message KBDSetLedsAsync carries to its callback: the channel index, three bytes of padding and a value word.
 * size: 0x08 */
struct HkbSlotMsg {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 pad_01[3];
    /* +0x04 */ u32 value;
};

extern "C" void hkbLedRequestDone(struct HkbRecord* rec);

/* Returns the keyboard LED bits (num, caps, scroll lock) of a modifier word's lock bits. */
static inline u8 hkbLedBits(u32 mods)
{
    u8 leds = 0;
    if (mods & 0x100) {
        leds |= 1;
    }
    if (mods & 0x200) {
        leds |= 2;
    }
    if (mods & 0x400) {
        leds |= 4;
    }
    return leds;
}

/* One USB keyboard channel of the manager: the eight key slots (a bit mask and a HID usage per slot) for the
 * previous frame, this frame and the next one the key callback fills, the edge words the key iterator scans
 * (held/pressed/released/repeated), the modifier word and its override, the key map and the repeat timers. */
/* size: 0x6C */
struct HkbChannel {
    /* Clears the key and modifier state, keeping the lock bits (0x700) of the modifier word. */
    inline void reset() {
        held = 0;
        pressed = 0;
        released = 0;
        repeated = 0;
        curSlots = 0;
        nextSlots = 0;
        modifiers &= 0x00000700;
        overrideMask = 0;
        overrideValue = 0;
        ledRetry = 0;
    }

    /* Hands the lock bits to the driver and requests the LEDs; a busy request is retried later. */
    inline void sendLeds() {
        u32 saved;
        u32 result;
        HkbSlotMsg msg;

        KBDSetChannelLeds(chan, modifiers);
        u32 value = hkbLedBits(modifiers);
        msg.value = value;
        msg.index = chan;
        saved = OSDisableInterrupts();
        result = KBDSetLedsAsync(chan, value, (void*)hkbLedRequestDone, *(u32*)&msg);
        OSRestoreInterrupts(saved);
        if ((s32)result == 7) {
            ledRetry = 1;
        }
    }

    /* +0x00 */ u8 chan;
    /* +0x01 */ u8 pad_01[3];
    /* +0x04 */ u32 held;
    /* +0x08 */ u32 pressed;
    /* +0x0C */ u32 released;
    /* +0x10 */ u32 repeated;
    /* +0x14 */ u32 prevSlots;
    /* +0x18 */ u32 curSlots;
    /* +0x1C */ u32 nextSlots;
    /* +0x20 */ u32 modifiers;
    /* +0x24 */ u32 overrideValue;
    /* +0x28 */ u32 overrideMask;
    /* +0x2C */ s32 mapIndex;
    /* +0x30 */ u8 ledRetry;
    /* +0x31 */ u8 prevUsage[8];
    /* +0x39 */ u8 curUsage[8];
    /* +0x41 */ u8 nextUsage[8];
    /* +0x49 */ u8 pad_49[3];
    /* +0x4C */ s32 repeatTimer[8];
};

/* The status record KBDSetLedsAsync hands its callback (hkbExtraLedRequestDone) and hkbLedRequestDone reads: the channel index and
 * the result code (7 = busy).
 * size: 0x0C (approximation: only +0x00 and +0x08 are touched) */
struct HkbRecord {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 pad_01[3];
    /* +0x04 */ u32 pad_04;
    /* +0x08 */ s32 result; /* signed: retail compares with cmpwi */
};

/* The keyboard manager singleton: the initialised flag, per-channel connected flags, the lock modifiers, the
 * pending-LED bit mask of the extra channels, the optional character allow-list, one channel, the sentinel of the
 * observer list and the driver work area (KBD's 0xCC0-byte state followed by HID's). */
/* size: 0x94 */
struct HkbManager {
    HkbManager();
    ~HkbManager() {}

    /* Returns channel 0's modifier word with the override bits replaced. */
    inline u32 getModifierState() {
        u32 v = channels[0].modifiers;
        u32 b = channels[0].overrideMask;
        v &= ~b;
        v |= channels[0].overrideValue & b;
        return v;
    }

    /* Returns the character allow-list (NULL when every character is allowed). */
    inline const u16* getAllowedChars() const { return allowedChars; }

    /* Returns `c` when no allow-list is set or the list holds it, 0 otherwise. */
    inline u16 filterAllowed(u16 c) {
        if (getAllowedChars() == 0) {
            return c;
        }
        if (allowedCount == 0) {
            return c;
        }
        for (u32 i = 0; i < allowedCount; i++) {
            if (c == allowedChars[i]) {
                return c;
            }
        }
        return 0;
    }

    /* +0x00 */ u8 initialized;
    /* +0x01 */ u8 active[1];
    /* +0x02 */ u8 pad_02[2];
    /* +0x04 */ u32 lockModifiers;
    /* +0x08 */ u32 pendingMask;
    /* +0x0C */ const u16* allowedChars;
    /* +0x10 */ u32 allowedCount;
    /* +0x14 */ HkbChannel channels[1];
    /* +0x80 */ HkbManagerNode sentinel; /* circular observer list header */
    /* +0x90 */ u8* workArea;
};

/* The record the KBD callbacks hand on to the observers: the channel, the key's HID usage and the event flags
 * (bit 0 key down, bit 1 auto-repeat); connect and disconnect pass the channel byte alone.
 * size: 0x08 */
struct HkbArg {
    /* +0x00 */ u8 index;
    /* +0x01 */ u8 value;
    /* +0x02 */ u16 pad_02;
    /* +0x04 */ u32 flags;
};

/* Returns whether the cursor stands on a set key bit of its kind. */
inline bool HkbKeyIter::isValid() const
{
    if (manager == 0) {
        return false;
    }
    if (bit < 0 || bit >= 8) {
        return false;
    }
    if (channel >= 1) {
        return false;
    }
    switch (kind) {
    case 0:
        return (manager->channels[channel].held & (1 << bit)) != 0;
    case 1:
        return (manager->channels[channel].pressed & (1 << bit)) != 0;
    case 2:
        return (manager->channels[channel].released & (1 << bit)) != 0;
    case 3:
        return (manager->channels[channel].repeated & (1 << bit)) != 0;
    }
    return false;
}

/* Returns the HID usage of the slot the cursor stands on: the previous frame's for a released key. */
inline u8 HkbKeyIter::getUsage() const
{
    if (!isValid()) {
        return 0;
    }
    if (channel >= 1) {
        return 0;
    }
    switch (kind) {
    case 0:
        return manager->channels[channel].curUsage[bit];
    case 1:
        return manager->channels[channel].curUsage[bit];
    case 2:
        return manager->channels[channel].prevUsage[bit];
    case 3:
        return manager->channels[channel].curUsage[bit];
    }
    return 0;
}

/* Looks up the key code `usage` produces under the current modifiers and caches it. */
inline u16 HkbKeyIter::lookupCode(u8 usage)
{
    if (channel >= 1) {
        return 0;
    }
    u32 mods = manager->getModifierState();
    if ((mods & 0xD) && (mods & 5) != 5) {
        return 0;
    }
    switch (kind) {
    case 0:
    case 1:
    case 3:
        code = kbd_lookup_key(usage, mods, manager->channels[channel].mapIndex);
        break;
    case 2:
        code = kbd_lookup_key(usage, mods, manager->channels[channel].mapIndex);
        break;
    }
    return code;
}


void operator delete(void* ptr) throw();

/* The keyboard manager singleton, constructed and registered for destruction by the unit's static initializer. */
HkbManager hkbManager;

class HkbGuiManagerView;
class HkbGuiEventTarget;

/* A gui component (the HBM gui manager's Component): the slots are named by their offset in the table where no
 * body names them (+0x08 is the deleting destructor), so MWCC emits the canonical `lwz r12,0(r3); lwz r12,N(r12)`
 * dispatch.  Its first slot is the key function and is not defined here, so the table (0x806583A0) is stored by
 * the constructor and not emitted.  The fields are the ones the constructor sets; init clears the per-pointer state.
 * size: 0x9C */
class HkbGuiComponent {
public:
    HkbGuiComponent(u32 id)
        : mbDragging(false), mDragButtonMask(0xFFFF), mID(id), mbTriggerTarget(false), mpManager(NULL),
          mpEventTarget(NULL)
    {
        init();
    }

    /* slot +0x008 */ virtual void v_0x008(int flag);
    /* untyped: opaque handles passed through */
    /* slot +0x00C */ virtual void v_0x00C(void* a, void* b, void* c);
    /* slot +0x010 */ virtual void init();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual Pane* v_0x020(); /* the pane the manager keys the component by */
    /* slot +0x024 */ virtual u32 v_0x024(u32 idx);
    /* slot +0x028 */ virtual u32 v_0x028(u32 idx); /* whether pointer `idx` is over the component */
    /* slot +0x02C */ virtual void v_0x02C(u32 idx, u32 on);
    /* slot +0x030 */ virtual void v_0x030(u32 idx);
    /* slot +0x034 */ virtual void v_0x034(u32 idx);
    /* slot +0x038 */ virtual void v_0x038(f32 dx, f32 dy);
    /* slot +0x03C */ virtual void v_0x03C(u32 idx, f32 x, f32 y);
    /* slot +0x040 */ virtual void v_0x040(u32 idx, u32 trigger, StageRec* pos);
    /* untyped: opaque handle passed through */
    /* slot +0x044 */ virtual void v_0x044(void* arg);
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual u32 v_0x04C(u32 idx, f32 x, f32 y, u32 trigger, u32 hold, u32 release);
    /* slot +0x050 */ virtual u32 v_0x050(); /* whether the component takes the trigger */
    /* slot +0x054 */ virtual void setTriggerTarget(u32 bTarget);
    /* slot +0x058 */ virtual u32 v_0x058(StageEvent* event);
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual u32 v_0x064(f32 x, f32 y);

    /* +0x04 */ bool mbDragging;
    /* +0x05 */ u8 pad_0x05[0x78 - 0x05];
    /* +0x78 */ u32 mDragButtonMask;
    /* +0x7C */ u32 mID;
    /* +0x80 */ u8 pad_0x80[0x90 - 0x80];
    /* +0x90 */ bool mbTriggerTarget;
    /* +0x94 */ HkbGuiManagerView* mpManager;
    /* +0x98 */ HkbGuiEventTarget* mpEventTarget;
};

/* A component bound to a layout pane (the HBM gui manager's PaneComponent).  setPane is its key function and is
 * not defined here, so the table (0x80658290) is stored by the constructor and not emitted.
 * size: 0xA0 */
class HkbGuiPaneComponent : public HkbGuiComponent {
public:
    HkbGuiPaneComponent(u32 id) : HkbGuiComponent(id), mpPane(NULL) {}

    /* slot +0x068 */ virtual void setPane(nw4r::lyt::Pane* pane);

    /* +0x9C */ nw4r::lyt::Pane* mpPane;
};

/* The event target a component forwards to (its +0x04 and +0x98 members): slot +0x10 takes the event's third
 * word, slot +0x0C the whole event.
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HkbGuiEventTarget {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* untyped: opaque handles passed through */
    /* slot +0x00C */ virtual void v_0x00C(Component* source, void* b, void* d);
    /* untyped: opaque handle passed through */
    /* slot +0x010 */ virtual void v_0x010(void* c);
};

/* The polymorphic view of a gui manager the component bodies call through: slot +0x20 adds a component, +0x34
 * reports a component event, +0x50 returns the draw info (NULL while it does not draw).  Nothing is defined here,
 * so no table is emitted (rule 10).
 * size: 0x04 (approximation: only the compiler-emitted vptr at +0x00 is touched) */
class HkbGuiManagerView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void addComponent(Component* component);
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void v_0x02C();
    /* slot +0x030 */ virtual void v_0x030();
    /* untyped: caller-owned payload passed through */
    /* slot +0x034 */ virtual void onEvent(Component* component, u32 event, u32 idx, void* data);
    /* slot +0x038 */ virtual void v_0x038();
    /* slot +0x03C */ virtual void v_0x03C();
    /* slot +0x040 */ virtual void v_0x040();
    /* slot +0x044 */ virtual void v_0x044();
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void v_0x04C();
    /* slot +0x050 */ virtual const nw4r::lyt::DrawInfo* getDrawInfo();
};

/* 0x8056A478 (0x1C0): Draws a `width`-wide line of one colour from (x0, y0) to (x1, y1) at depth z. */
extern "C" void hkbGuiDrawLine(u8 width, const GXColor& color, f32 x0, f32 y0, f32 x1, f32 y1, f32 z) {
    Mtx mtx;
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetCullMode(0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 1, 1, 0, 0, 2);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 4);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetBlendMode(0, 0, 0, 5);
    PSMTXTrans(mtx, 0.0f, 0.0f, 0.0f);
    GXLoadPosMtxImm(mtx, 0);
    GXSetLineWidth(width, 0);
    GXBegin(0xA8, 0, 2);
    GXWGFifo.f32 = x0;
    GXWGFifo.f32 = y0;
    GXWGFifo.f32 = z;
    GXWGFifo.u32 = *(const u32*)&color;
    GXWGFifo.f32 = x1;
    GXWGFifo.f32 = y1;
    GXWGFifo.f32 = z;
    GXWGFifo.u32 = *(const u32*)&color;
}

/* --- 0x8056A638: per-stage update dispatcher - returns 0/1/2/3 by which transition fired. --- */
extern "C" u32 fn_8056A638(Component* self, StageEvent* p) {
    HkbGuiComponent* view = (HkbGuiComponent*)self;
    u32 idx;
    f32 x = p->x;
    u32 result = 0;
    f32 y = p->y;
    idx = p->idx;
    if (view->v_0x058(p) != 0) {
        if (view->v_0x064(x, y) != 0) {
            if (view->v_0x024(idx) != 0) {
                view->v_0x03C(idx, x, y);
                result = 3;
            } else {
                view->v_0x02C(idx, 1);
                view->v_0x030(idx);
                result = 1;
            }
        } else if (view->v_0x024(idx) != 0) {
            view->v_0x02C(idx, 0);
            view->v_0x034(idx);
            result = 2;
        }
        if (self->flags_0x0D[idx] != 0) {
            StageRec* r = &self->recs_0x18[idx];
            view->v_0x038(x - r->x, y - r->y);
            r->x = x;
            r->y = y;
        }
    }
    if (self->flags_0x0D[idx] != 0 && (p->field_0x10 & self->field_0x78) == 0) {
        self->flags_0x0D[idx] = 0;
    }
    return result;
}

/* --- 0x8056A824: constant virtual (always-true predicate). --- */
extern "C" u32 fn_8056A824(void) { return 1; }

/* --- 0x8056A82C: per-index colour-selector byte. --- */
extern "C" u8 fn_8056A82C(ColorBlock* self, u32 idx) { return self->colors_0x05[idx]; }

/* --- 0x8056A838: return the old per-index counter, then increment it. --- */
extern "C" u16 fn_8056A838(ColorBlock* self, u32 idx) {
    u16 v = self->counts_0x80[idx];
    self->counts_0x80[idx] = v + 1;
    return v;
}

/* --- 0x8056A850: store a per-index colour-selector byte. --- */
extern "C" void fn_8056A850(ColorBlock* self, u32 idx, u8 v) { self->colors_0x05[idx] = v; }

/* --- 0x8056A85C / 0x8056A870: clear a per-index counter. --- */
extern "C" void fn_8056A85C(ColorBlock* self, u32 idx) {
    u16* p = &self->counts_0x80[idx];
    *p = 0;
}

extern "C" void fn_8056A870(ColorBlock* self, u32 idx) {
    u16* p = &self->counts_0x80[idx];
    *p = 0;
}

/* --- 0x8056A884: empty virtual. --- */
extern "C" void fn_8056A884(void) {}

/* --- 0x8056A888: Manager deleting destructor - free the component list, then the manager. --- */
extern "C" Manager* fn_8056A888(Manager* self, int flag) {
    if (self != 0) {
        self->vtable = (VTable*)lbl_80658358;
        Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
        while (n != 0) {
            nw4r::ut::List_Remove((nw4r::ut::List*)&self->list_0x0C, n);
            if (self->allocator != 0) {
                MEMFreeToAllocator((MEMAllocator*)self->allocator, n);
            } else {
                operator delete(n);
            }
            n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* --- 0x8056A944: run virtual slot 4 over every component. --- */
extern "C" void fn_8056A944(Manager* self) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->init();
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056A9B4: append `comp` to the manager's list, keyed by its virtual slot 8 result. --- */
extern "C" void fn_8056A9B4(Manager* self, Component* comp) {
    Pane* key = ((HkbGuiComponent*)comp)->v_0x020();
    comp->field_0x94 = (Component*)self;
    if (self->allocator != 0) {
        Node* node = (Node*)MEMAllocFromAllocator((MEMAllocator*)self->allocator, sizeof(Node));
        if (node != 0) {
            node->mKey = key;
            node->mValue = comp;
        }
        nw4r::ut::List_Append((nw4r::ut::List*)&self->list_0x0C, node);
    } else {
        Node* node = (Node*)operator new(sizeof(Node));
        if (node != 0) {
            node->mKey = key;
            node->mValue = comp;
        }
        nw4r::ut::List_Append((nw4r::ut::List*)&self->list_0x0C, node);
    }
}

/* --- 0x8056AA64: component id. --- */
extern "C" u32 fn_8056AA64(Component* self) { return self->field_0x7C; }

/* --- 0x8056AA6C / 0x8056AA98: indexed component lookup (Manager::getComponent). --- */
extern "C" Component* fn_8056AA6C(Manager* self, u32 index) {
    return (Component*)((Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x0C, (u16)index))->mValue;
}

extern "C" Component* fn_8056AA98(Manager* self, u32 index) {
    return (Component*)((Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x0C, (u16)index))->mValue;
}

/* 0x8056AAC4 (0x2B8): Runs pointer `idx` over every component and reports what each did; the last component that
 * took the trigger gets the trigger and the release.  Returns whether any component was touched. */
/* untyped: caller-owned payload passed through */
extern "C" u32 hkbGuiManagerUpdate(Manager* self, u32 idx, f32 x, f32 y, u32 trigger, u32 hold, u32 release,
                                   void* data) {
    HkbGuiManagerView* view = (HkbGuiManagerView*)self;
    HkbGuiComponent* c;
    u32 bTouched = 0;
    HkbGuiComponent* pLast = NULL;
    for (Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0); n != NULL;
         n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n)) {
        c = (HkbGuiComponent*)n->mValue;
        switch (c->v_0x04C(idx, x, y, trigger, hold, release)) {
        case 1:
            view->onEvent((Component*)c, 0, idx, data);
            if (0) {
            case 2:
                view->onEvent((Component*)c, 1, idx, data);
                break;
            case 3:
                view->onEvent((Component*)c, 2, idx, data);
                if (0) {
                case 4:
                    view->onEvent((Component*)c, 4, idx, data);
                    break;
                case 5:
                    view->onEvent((Component*)c, 5, idx, data);
                    break;
                }
            }
            bTouched = 1;
            if (c->v_0x050()) {
                pLast = c;
            }
            break;
        }
        if (c->v_0x028(idx)) {
            view->onEvent((Component*)c, 3, idx, data);
        }
    }
    if (pLast != NULL) {
        if (trigger) {
            StageRec pos = {0.0f, 0.0f, 0.0f};
            pos.x = x;
            pos.y = y;
            pLast->v_0x040(idx, trigger, &pos);
            view->onEvent((Component*)pLast, 4, idx, data);
        }
        if (release) {
            view->onEvent((Component*)pLast, 5, idx, data);
        }
    }
    return bTouched;
}

/* The argument block hkbGuiUpdateStageFromArgs builds for the stage update: the event and one trailing word.
 * size: 0x18 */
struct HkbStageEventArgs {
    /* +0x00 */ StageEvent event;
    /* +0x14 */ u32 extra_14;
};

/* --- 0x8056AD7C: build a stage event from the arguments and run the stage update on it. --- */
extern "C" u32 hkbGuiUpdateStageFromArgs(Component* self, u32 idx, f32 x, f32 y, u32 word_0C, u32 mask, u32 extra) {
    HkbStageEventArgs args;
    args.event.idx = idx;
    args.event.x = x;
    args.event.y = y;
    args.event.field_0x0C = word_0C;
    args.event.field_0x10 = mask;
    args.extra_14 = extra;
    return fn_8056A638(self, &args.event);
}

/* --- 0x8056ADB8: forward an event from `source` to the +4 handler and to the source's +0x98 handler. --- */
/* untyped: opaque handles passed through */
extern "C" void fn_8056ADB8(Component* self, Component* source, void* b, void* c, void* d) {
    HkbGuiEventTarget* h = (HkbGuiEventTarget*)self->field_0x04;
    if (h != 0) {
        h->v_0x010(c);
        h = (HkbGuiEventTarget*)self->field_0x04;
        h->v_0x00C(source, b, d);
    }
    h = (HkbGuiEventTarget*)source->field_0x98;
    if (h != 0) {
        h->v_0x00C(source, b, d);
    }
}

/* --- 0x8056AE70: per-component flag byte. --- */
extern "C" u8 fn_8056AE70(Component* self) { return self->field_0x90; }

/* --- 0x8056AE78: latch a stage position and arm its flag when the mask matches. --- */
extern "C" void fn_8056AE78(Component* self, u32 idx, u32 mask, StageRec* v) {
    if ((mask & self->field_0x78) != 0) {
        self->recs_0x18[idx] = *v;
        self->flags_0x0D[idx] = 1;
        self->counts_0x80[idx] = 0;
    }
}

/* --- 0x8056AEC4 / 0x8056AF34: run virtual slot 5 / slot 7 over every component. --- */
extern "C" void fn_8056AEC4(Manager* self) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x014();
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

extern "C" void fn_8056AF34(Manager* self) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x01C();
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056AFA4: forward an argument to virtual slot 21 of every component. --- */
extern "C" void fn_8056AFA4(Manager* self, u32 bTarget) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->setTriggerTarget(bTarget);
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056B024: forward an argument to virtual slot 17 of every component. --- */
extern "C" void fn_8056B024(Manager* self, void* arg) {
    Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, 0);
    while (n != 0) {
        HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
        c->v_0x044(arg);
        n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x0C, n);
    }
}

/* --- 0x8056B0A4: store the trigger mask. --- */
extern "C" void fn_8056B0A4(Component* self, u32 v) { self->field_0x78 = v; }

/* --- 0x8056B0AC: PaneManager deleting destructor - free the pane list, then the base Manager. --- */
extern "C" PaneManager* fn_8056B0AC(PaneManager* self, int flag) {
    if (self != 0) {
        self->vtable = (VTable*)lbl_806582FC;
        Node* n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x24, 0);
        while (n != 0) {
            nw4r::ut::List_Remove((nw4r::ut::List*)&self->list_0x24, n);
            if (self->allocator != 0) {
                HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
                c->v_0x008(-1);
                MEMFreeToAllocator((MEMAllocator*)self->allocator, n->mValue);
                MEMFreeToAllocator((MEMAllocator*)self->allocator, n);
            } else {
                HkbGuiComponent* c = (HkbGuiComponent*)n->mValue;
                if (c != 0) {
                    c->v_0x008(1);
                }
                operator delete(n);
            }
            n = (Node*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list_0x24, 0);
        }
        fn_8056A888(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* --- 0x8056B1BC / 0x8056B1FC: conditional-delete helpers. --- */
extern "C" void* fn_8056B1BC(void* self, int flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}

extern "C" void* fn_8056B1FC(void* self, int flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}

/* A pane's child list. */
typedef nw4r::ut::LinkList<nw4r::lyt::Pane, 4> HkbPaneList;

extern "C" void hkbGuiPaneManagerAddPane(PaneManager* self, nw4r::lyt::Pane* pane);

#pragma push
#pragma auto_inline on

extern "C" void hkbGuiPaneManagerWalkInChildren(PaneManager* self, HkbPaneList& children);

/* 0x8056B23C (0xE4): Restarts the component ids and adds a component for every pane of the layout. */
extern "C" void hkbGuiPaneManagerCreateLayoutScene(PaneManager* self, const nw4r::lyt::Layout& layout) {
    self->field_0x20 = 0;
    hkbGuiPaneManagerWalkInChildren(self, layout.mpRootPane->mChildList);
}

/* 0x8056B320 (0xFC): Adds a component for every pane below `children`, depth first. */
extern "C" void hkbGuiPaneManagerWalkInChildren(PaneManager* self, HkbPaneList& children) {
    for (HkbPaneList::Iterator it = children.GetBeginIter(); it != children.GetEndIter();
         ++it) {
        hkbGuiPaneManagerAddPane(self, &*it);
        hkbGuiPaneManagerWalkInChildren(self, it->mChildList);
    }
}

#pragma pop

/* 0x8056B41C (0x8): Returns the first pane of a child list (the out-of-line LinkList<Pane,4>::GetBeginIter). */
extern "C" HkbPaneList::Iterator hkbPaneListGetBeginIter(HkbPaneList* list) {
    return list->GetBeginIter();
}

/* 0x8056B424 (0x1C): Returns whether two pane iterators differ (the out-of-line Iterator operator!=). */
extern "C" bool hkbPaneIterNotEqual(HkbPaneList::Iterator it1, HkbPaneList::Iterator it2) {
    return it1 != it2;
}

/* 0x8056B440 (0x288): Adds a component for `pane`, numbered by the running pane count, to the pane list and the
 * manager; a picture or window pane's component takes the trigger. */
extern "C" void hkbGuiPaneManagerAddPane(PaneManager* self, nw4r::lyt::Pane* pane) {
    HkbGuiPaneComponent* component;
    Node* node;
    if (self->allocator != NULL) {
        void* componentBlock = MEMAllocFromAllocator((MEMAllocator*)self->allocator, sizeof(HkbGuiPaneComponent));
        node = (Node*)MEMAllocFromAllocator((MEMAllocator*)self->allocator, sizeof(Node));
        component = new (componentBlock) HkbGuiPaneComponent(self->field_0x20);
        if (node != NULL) {
            node->mKey = pane;
            node->mValue = component;
        }
    } else {
        component = new HkbGuiPaneComponent(self->field_0x20);
        Node* block = (Node*)operator new(sizeof(Node));
        if (block != NULL) {
            block->mKey = pane;
            block->mValue = component;
        }
        node = block;
    }
    nw4r::ut::List_Append((nw4r::ut::List*)&self->list_0x24, node);
    self->field_0x20++;
    component->setPane(pane);
    if (nw4r::ut::DynamicCast<nw4r::lyt::Picture>(pane) != NULL) {
        component->setTriggerTarget(true);
    }
    if (nw4r::ut::DynamicCast<nw4r::lyt::Window>(pane) != NULL) {
        component->setTriggerTarget(true);
    }
    ((HkbGuiManagerView*)self)->addComponent((Component*)component);
}

/* --- 0x8056B6C8: PaneComponent::setPane. --- */
extern "C" void fn_8056B6C8(Component* self, void* pane) { self->field_0x9C = pane; }

/* --- 0x8056B6D0: getPaneComponentByName - find the component whose pane's name matches `name`. --- */
extern "C" Component* hkbGetPaneComponentByName(PaneManager* self, const char* name) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x24, (u16)i);
        Pane* pane = (Pane*)n->mKey;
        if (hbmNameMatches(pane->name_0xBC, name)) {
            return (Component*)n->mValue;
        }
    }
    return 0;
}

/* --- 0x8056B760: PaneManager::getPaneComponentByPane - find the component keyed by `pane`. --- */
extern "C" Component* fn_8056B760(PaneManager* self, void* pane) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x24, (u16)i);
        if (n->mKey == pane) {
            return (Component*)n->mValue;
        }
    }
    return 0;
}

/* 0x8056B7D8 (0xD4): Makes every bounding-pane component take the trigger, or none of them. */
extern "C" void hkbGuiSetBoundingTriggerTargets(PaneManager* self, u32 bTarget) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)nw4r::ut::List_GetNth((nw4r::ut::List*)&self->list_0x24, (u16)i);
        if (nw4r::ut::DynamicCast<nw4r::lyt::Bounding>((nw4r::lyt::Pane*)n->mKey) != NULL) {
            ((HkbGuiComponent*)n->mValue)->setTriggerTarget(bTarget);
        }
    }
}

/* 0x8056B8AC (0x128): Returns whether the point (x, y) lies on the component's pane. */
extern "C" u32 hkbGuiPaneComponentContains(Component* self, f32 x, f32 y) {
    HkbGuiManagerView* manager = (HkbGuiManagerView*)self->field_0x94;
    if (manager == NULL) {
        return 0;
    }
    const nw4r::lyt::DrawInfo* drawInfo = manager->getDrawInfo();
    if (drawInfo == NULL) {
        return 0;
    }
    Mtx inv;
    Vec local;
    PSMTXInverse(((nw4r::lyt::Pane*)self->field_0x9C)->mGlbMtx.m, inv);
    Vec pos;
    pos.x = x;
    pos.y = y;
    pos.z = 0.0f;
    PSMTXMultVec(inv, &pos, &local);
    nw4r::ut::Rect rect = ((nw4r::lyt::Pane*)self->field_0x9C)->GetPaneRect(*drawInfo);
    if (rect.left <= local.x && local.x <= rect.right && rect.bottom <= local.y && local.y <= rect.top) {
        return 1;
    }
    return 0;
}

/* --- 0x8056B9D4: the manager's +0x18 word. --- */
extern "C" void* fn_8056B9D4(Manager* self) { return self->field_0x18; }

/* 0x8056B9DC (0x1A0): Outlines the component's pane, red, or blue while pointer 0 is over it. */
extern "C" void hkbGuiPaneComponentDraw(Component* self) {
    if (((HkbGuiManagerView*)self->field_0x94)->getDrawInfo()) {
        nw4r::lyt::Pane* pane = (nw4r::lyt::Pane*)self->field_0x9C;
        nw4r::lyt::Size size = pane->GetSize();
        f32 x = pane->mGlbMtx.m[0][3];
        f32 y = pane->mGlbMtx.m[1][3];
        GXColor color = {0xFF, 0x00, 0x00, 0xFF};
        if (((HkbGuiComponent*)self)->v_0x024(0)) {
            color.r = 0;
            color.b = 0xFF;
        }
        hkbGuiDrawLine(8, color, x - size.width / 2.0f, y - size.height / 2.0f, x + size.width / 2.0f,
                       y - size.height / 2.0f, 0.0f);
        hkbGuiDrawLine(8, color, x + size.width / 2.0f, y - size.height / 2.0f, x + size.width / 2.0f,
                       y + size.height / 2.0f, 0.0f);
        hkbGuiDrawLine(8, color, x + size.width / 2.0f, y + size.height / 2.0f, x - size.width / 2.0f,
                       y + size.height / 2.0f, 0.0f);
        hkbGuiDrawLine(8, color, x - size.width / 2.0f, y + size.height / 2.0f, x - size.width / 2.0f,
                       y - size.height / 2.0f, 0.0f);
    }
}

/* --- 0x8056BB7C: every node on the +0xC chain must carry the low visibility bit. --- */
extern "C" u32 fn_8056BB7C(Component* self) {
    VisNode* p = (VisNode*)self->field_0x9C;
    if (p == 0) {
        return 0;
    }
    while (p != 0) {
        if ((p->flags_0xBB & 1) == 0) {
            return 0;
        }
        p = p->next_0x0C;
    }
    return 1;
}

/* --- 0x8056BBBC: tail-call forward to fn_8056A638 (stage update with the point record). --- */
extern "C" u32 fn_8056BBBC(Component* self, StageEvent* p) { return fn_8056A638(self, p); }

/* --- 0x8056BBC0: store the +4 word. --- */
extern "C" void fn_8056BBC0(Manager* self, u32 v) { self->field_0x04 = v; }

/* --- 0x8056BBC8: constant virtual (always-false predicate). --- */
extern "C" u32 fn_8056BBC8(void) { return 0; }

/* --- 0x8056BBD0: append `node` at the tail of the +8/+4 chain rooted at `head`. --- */
extern "C" void hkbNodeAppend(CNode* head, CNode* node) {
    CNode* p = head;
    while (p->next_0x08 != 0) {
        p = p->next_0x08;
    }
    p->next_0x08 = node;
    node->prev_0x04 = p;
}

/* ---- functions ---------------------------------------------------------------------------------- */

extern "C" {

/* 0x8056BBF0: unlink `node` from `list`, panicking if it is not on the list. */
void hkbNodeRemove(HkbNode* list, HkbNode* node)
{
    HkbNode* p = list->next;
    while (p != 0) {
        if (p == node) {
            HkbNode* prev = p->prev;
            HkbNode* next = p->next;
            if (prev != 0) {
                prev->next = next;
            }
            if (next != 0) {
                next->prev = prev;
            }
            node->prev = 0;
            node->next = 0;
            return;
        }
        p = p->next;
    }
    OSPanic("tiHKBManager.cpp", 0xB3, "specified listener not found!");
}

/* 0x8056BC5C: mark the channel of a busy LED request as pending. */
void hkbLedRequestDone(HkbRecord* rec)
{
    if (rec->result != 7) {
        return;
    }
    hkbManager.channels[rec->index].ledRetry = 1;
}

/* 0x8056BC88: clear the channel's pending bit, re-issue the LED request, and re-set the bit while it stays busy. */
void hkbExtraLedRequestDone(HkbRecord* rec)
{
    if (rec->result != 7) {
        return;
    }
    {
        u8 index = rec->index;
        u32 saved;
        u32 bit;
        u32 result;
        HkbSlotMsg msg;

        msg.value = 0;
        msg.index = index;

        saved = OSDisableInterrupts();
        bit = 1u << index;
        hkbManager.pendingMask &= ~bit;
        result = KBDSetLedsAsync(index, 0, (void*)hkbExtraLedRequestDone, *(u32*)&msg);
        if ((s32)result == 7) {
            hkbManager.pendingMask |= bit;
        }
        OSRestoreInterrupts(saved);
    }
}

/* 0x8056BD38: (de)activate a channel; channels past the first issue an LED request instead. */
void HkbManagerNode::onConnect(HkbArg* arg)
{
    if (arg->index >= 1) {
        HkbManager* mgr;
        u8 index;
        u32 saved;
        u32 bit;
        u32 result;
        HkbSlotMsg msg;

        KBDSetChannelValue(arg->index, 0);
        index = arg->index;
        msg.value = 0;
        mgr = manager;
        msg.index = index;
        saved = OSDisableInterrupts();
        bit = 1u << index;
        mgr->pendingMask &= ~bit;
        result = KBDSetLedsAsync(index, 0, (void*)hkbExtraLedRequestDone, *(u32*)&msg);
        if ((s32)result == 7) {
            mgr->pendingMask |= bit;
        }
        OSRestoreInterrupts(saved);
    } else {
        manager->active[arg->index] = 1;
        KBDSetChannelValue(arg->index, 0);
        manager->channels[arg->index].ledRetry = 1;
    }
}

/* 0x8056BE2C: reset one channel's state. */
void HkbManagerNode::onDisconnect(HkbArg* arg)
{
    u32 index = arg->index;
    HkbManager* mgr;
    HkbChannel* ch;
    if (index >= 1) {
        return;
    }
    mgr = manager;
    mgr->active[index] = 0;
    ch = &mgr->channels[index];
    ch->held = 0;
    ch->pressed = 0;
    ch->released = 0;
    ch->repeated = 0;
    ch->curSlots = 0;
    ch->nextSlots = 0;
    ch->modifiers &= 0x00000700;
    ch->overrideMask = 0;
    ch->overrideValue = 0;
    ch->ledRetry = 0;
}

/* 0x8056BE84: forward a channel update. */
void hkbChannelKeyEvent(HkbChannel* ch, u32 down, u8 usage);

void HkbManagerNode::onKey(HkbArg* arg)
{
    u32 index = arg->index;
    u32 flags;
    if (index >= 1) {
        return;
    }
    flags = arg->flags;
    if (flags & 2) {
        return;
    }
    hkbChannelKeyEvent(&manager->channels[index], flags & 1, arg->value);
}

/* 0x8056BEBC / 0x8056BF58 / 0x8056BFF4: notify every observer on the global manager's list by calling
 * the vtable slot +0x0C / +0x10 / +0x14.  Each is a separate body (the retail object has three copies). */
#define HKB_NOTIFY(method)                                                     \
    HkbNode* first;                                                            \
    HkbNode* n = &hkbManager.sentinel;                                         \
    while (n->prev != 0) {                                                     \
        n = n->prev;                                                           \
        if (n->prev == &hkbManager.sentinel) {                                 \
            break;                                                             \
        }                                                                      \
    }                                                                          \
    first = n;                                                                 \
    while (n != 0) {                                                           \
        n->method(arg);                                                        \
        n = n->next;                                                           \
        if (n == first) {                                                      \
            break;                                                             \
        }                                                                      \
    }

void hkbNotifyConnect(HkbArg* arg)
{
    HKB_NOTIFY(onConnect)
}

void hkbNotifyDisconnect(HkbArg* arg)
{
    HKB_NOTIFY(onDisconnect)
}

void hkbNotifyKey(HkbArg* arg)
{
    HKB_NOTIFY(onKey)
}

/* 0x8056C090 (0x78): Clears the manager and its channel and makes the sentinel point back at it. */
HkbManager::HkbManager()
    : initialized(0), lockModifiers(0), pendingMask(0), allowedChars(0), allowedCount(0), sentinel(this), workArea(0)
{
    u8 i;
    for (i = 0; i < 1; i++) {
        HkbChannel* ch;
        active[i] = 0;
        ch = &channels[i];
        ch->held = 0;
        ch->pressed = 0;
        ch->released = 0;
        ch->repeated = 0;
        ch->curSlots = 0;
        ch->nextSlots = 0;
        ch->overrideMask = 0;
        ch->overrideValue = 0;
        ch->ledRetry = 0;
        ch->chan = 0;
        ch->mapIndex = 9;
        ch->modifiers = 0;
    }
}

/* 0x8056C148 (0x1C): Stores a HID completion result, or 1 for a zero result, into the caller's flag. */
void hkbHidDone(u32 value, u32* out)
{
    if (value == 0) {
        *out = 1;
        return;
    }
    *out = value;
}

/* 0x8056C164 (0x140): Brings the KBD and HID drivers up in the work area, or only installs the callbacks
 * when there is none; returns KBD's result, 1 when HID fails. */
s32 hkbManagerStartDriver(HkbManager* self)
{
    s32 result;
    if (self->workArea != 0) {
        u8* hidWork = self->workArea + 0xCC0;
        volatile s32 done;
        for (;;) {
            done = 0;
            HIDInit(hidWork, (HIDDoneCallback)hkbHidDone, (u32)&done);
            while (done == 0) {
            }
            if (done > 0) {
                break;
            }
            if (done != 0xFFFFFFFB) {
                return 1;
            }
        }
        result = KBDInit((struct KBDState*)self->workArea, (KBDChannelCallback)hkbNotifyConnect,
                         (KBDDisconnectCallback)hkbNotifyDisconnect, (KBDKeyEventCallback)hkbNotifyKey);
    } else {
        KBDChannelCallback onConnect;
        KBDDisconnectCallback onDisconnect;
        KBDKeyEventCallback onKey;
        u32 saved = OSDisableInterrupts();
        onConnect = KBDSetConnectCallback((KBDChannelCallback)hkbNotifyConnect);
        onDisconnect = KBDSetDisconnectCallback((KBDDisconnectCallback)hkbNotifyDisconnect);
        onKey = KBDSetKeyCallback((KBDKeyEventCallback)hkbNotifyKey);
        if (onConnect != 0 || onDisconnect != 0 || onKey != 0) {
            KBDSetConnectCallback(onConnect);
            KBDSetDisconnectCallback(onDisconnect);
            KBDSetKeyCallback(onKey);
        }
        OSRestoreInterrupts(saved);
        result = 0;
    }
    KBDInitRegionUS();
    return result;
}

/* 0x8056C2A4 (0x70): Starts the manager on `workArea` once; 3 when it already runs. */
s32 hkbManagerInit(HkbManager* self, u8* workArea)
{
    s32 result;
    if (self->initialized) {
        return 3;
    }
    self->workArea = workArea;
    self->initialized = 1;
    result = hkbManagerStartDriver(self);
    for (u8 i = 0; i < 1; i++) {
        KBDSetChannelMap(i, self->channels[i].mapIndex);
    }
    return result;
}

/* 0x8056C314 (0xC4): Stops the drivers and clears the channel state. */
void hkbManagerShutdown(HkbManager* self)
{
    if (self->initialized) {
        self->initialized = 0;
        KBDSetConnectCallback(0);
        KBDSetDisconnectCallback(0);
        KBDSetKeyCallback(0);
        if (self->workArea != 0) {
            volatile s32 done;
            KBDEnd();
            done = 0;
            HIDEnd((HIDDoneCallback)hkbHidDone, (u32)&done);
            while (done == 0) {
            }
        }
        for (int i = 0; i < 1; i++) {
            self->channels[i].reset();
            self->active[i] = 0;
        }
    }
}

/* 0x8056C3D8 (0x1C): Returns channel 0's modifier word with the override bits replaced. */
u32 hkbGetModifierState(HkbManager* self)
{
    u32 v = self->channels[0].modifiers;
    u32 b = self->channels[0].overrideMask;
    v &= ~b;
    v |= self->channels[0].overrideValue & b;
    return v;
}

/* 0x8056C3F4 (0xDC): Selects the key map of the channel and re-sends its LEDs. */
void hkbManagerSetMap(HkbManager* self, s32 mapIndex)
{
    for (u8 i = 0; i < 1; i++) {
        HkbChannel* ch = &self->channels[i];
        if (mapIndex != ch->mapIndex) {
            ch->mapIndex = mapIndex;
            KBDSetChannelMap(i, mapIndex);
        }
        ch->sendLeds();
    }
}

/* 0x8056C4D0 (0x118): Sets the lock bits `mask` to `value` and re-sends the LEDs when they changed. */
void hkbSetModifierState(HkbManager* self, u32 value, u32 mask)
{
    self->lockModifiers = (self->lockModifiers & ~mask) | (value & mask);
    for (u8 i = 0; i < 1; i++) {
        HkbChannel* ch = &self->channels[i];
        u32 locks = self->lockModifiers & 0x700;
        u32 old = ch->modifiers & 0x700;
        ch->modifiers &= ~0x700;
        u32 changed = ((old - locks) | (locks - old)) >> 31;
        ch->modifiers |= self->lockModifiers;
        if (changed && self->active[i]) {
            ch->sendLeds();
        }
    }
}

/* 0x8056C5E8 (0xC): Sets channel 0's modifier override value and mask. */
void hkbSetModifierOverride(HkbManager* self, u32 a, u32 b)
{
    self->channels[0].overrideValue = a;
    self->channels[0].overrideMask = b;
}

void hkbChannelUpdate(HkbChannel* ch);

/* 0x8056C5F4 (0xCC): Retries the pending LED requests of the extra channels and advances the channel a frame. */
void hkbManagerUpdate(HkbManager* self)
{
    u32 saved;
    HkbSlotMsg msg;
    u32 bit;
    u8 index;
    if (self->initialized) {
        for (index = 1; index < 4; index++) {
            bit = 1 << index;
            if (self->pendingMask & bit) {
                msg.index = index;
                msg.value = 0;
                saved = OSDisableInterrupts();
                self->pendingMask &= ~bit;
                if ((s32)KBDSetLedsAsync(index, 0, (void*)hkbExtraLedRequestDone, *(u32*)&msg) == 7) {
                    self->pendingMask |= bit;
                }
                OSRestoreInterrupts(saved);
            }
        }
        for (u8 i = 0; i < 1; i++) {
            hkbChannelUpdate(&self->channels[i]);
        }
    }
}

/* 0x8056C6C0: the first set bit of the manager's key word 0. */
HkbKeyIter hkbKeyIterBeginWord0(HkbManager* manager)
{
    HkbKeyIter it;
    it.manager = manager;
    it.kind = 0;
    it.bit = -1;
    it.channel = 0;
    it.code = 0;
    return hkbKeyIterSeek(&it);
}

/* 0x8056C700: the first set bit of the manager's key word 1. */
HkbKeyIter hkbKeyIterBeginWord1(HkbManager* manager)
{
    HkbKeyIter it;
    it.manager = manager;
    it.kind = 1;
    it.bit = -1;
    it.channel = 0;
    it.code = 0;
    return hkbKeyIterSeek(&it);
}

/* 0x8056C744: the first set bit of the manager's key word 3. */
HkbKeyIter hkbKeyIterBeginWord3(HkbManager* manager)
{
    HkbKeyIter it;
    it.manager = manager;
    it.kind = 3;
    it.bit = -1;
    it.channel = 0;
    it.code = 0;
    return hkbKeyIterSeek(&it);
}

/* 0x8056C788 (0x1F0): Puts a pressed key into a free (or its own) slot of the next frame, or takes a released
 * key out of it. */
void hkbChannelKeyEvent(HkbChannel* ch, u32 down, u8 usage)
{
    s32 i;
    s32 slot = -1;
    if (down == 1) {
        for (i = 0; i < 8; i++) {
            if (ch->nextSlots & (1 << i)) {
                if (usage == ch->nextUsage[i]) {
                    slot = i;
                    break;
                }
            } else {
                slot = i;
                break;
            }
        }
        if (slot >= 0) {
            ch->nextUsage[slot] = usage;
            ch->nextSlots |= 1 << slot;
        }
    } else {
        for (i = 0; i < 8; i++) {
            if ((ch->nextSlots & (1 << i)) && usage == ch->nextUsage[i]) {
                slot = i;
                break;
            }
        }
        if (slot >= 0) {
            ch->nextSlots &= ~(1 << slot);
        }
    }
}

void hkbChannelUpdateModifiers(HkbChannel* ch);

/* 0x8056C978 (0x2DC): Advances the channel a frame: takes the keys the callback queued, then derives the
 * pressed, released and auto-repeated slots. */
void hkbChannelUpdate(HkbChannel* ch)
{
    u32 bit;
    s32 i;
    u32 saved = OSDisableInterrupts();
    ch->prevSlots = ch->curSlots;
    ch->curSlots = ch->nextSlots;
    for (i = 0; i < 8; i++) {
        ch->prevUsage[i] = ch->curUsage[i];
        ch->curUsage[i] = ch->nextUsage[i];
    }
    OSRestoreInterrupts(saved);
    hkbChannelUpdateModifiers(ch);

    ch->held = ch->curSlots;
    ch->pressed = 0;
    for (i = 0; i < 8; i++) {
        bit = 1 << i;
        if ((ch->curSlots & bit) && (!(ch->prevSlots & bit) || ch->prevUsage[i] != ch->curUsage[i])) {
            ch->pressed |= bit;
        }
    }
    ch->released = 0;
    for (i = 0; i < 8; i++) {
        bit = 1 << i;
        if ((ch->prevSlots & bit) && (!(ch->curSlots & bit) || ch->prevUsage[i] != ch->curUsage[i])) {
            ch->released |= bit;
        }
    }
    ch->repeated = 0;
    for (i = 0; i < 8; i++) {
        bit = 1 << i;
        if (ch->curSlots & bit) {
            if (ch->pressed & bit) {
                ch->repeatTimer[i] = 30;
                ch->repeated |= bit;
            }
            if (--ch->repeatTimer[i] == 0) {
                ch->repeatTimer[i] = 8;
                ch->repeated |= bit;
            }
        }
    }
}

/* 0x8056CC54 (0x1E0): Rebuilds the modifier bits from the held HID modifier keys and re-sends the LEDs when the
 * lock bits changed or a request is to be retried. */
void hkbChannelUpdateModifiers(HkbChannel* ch)
{
    u32 old = ch->modifiers;
    ch->modifiers = old & ~0x2F;
    for (u8 i = 0; i < 8; i++) {
        if (ch->curSlots & (1 << i)) {
            u8 usage = ch->curUsage[i];
            if (usage == 0xE5 || usage == 0xE1) {
                ch->modifiers |= 2;
            } else if (usage == 0xE4 || usage == 0xE0) {
                ch->modifiers |= 1;
            } else if (usage == 0xE7 || usage == 0xE3) {
                ch->modifiers |= 8;
            } else if (usage == 0xE2) {
                ch->modifiers |= 4;
            } else if (usage == 0xE6) {
                if (ch->mapIndex == 9 || ch->mapIndex == 17) {
                    ch->modifiers |= 4;
                } else {
                    ch->modifiers |= 0x20;
                }
            }
        }
    }
    if ((old & 0x700) != (ch->modifiers & 0x700) || ch->ledRetry) {
        ch->ledRetry = 0;
        ch->sendLeds();
    }
}

/* Folds the driver's private-use key codes into characters and drops the rest of them and the controls. */
static inline u16 hkbNormalizeChar(u16 c)
{
    if (c >= 0xF130 && c <= 0xF139) {
        c -= 0xF100;
    } else if (c == 0xF10D) {
        c = 0xF1CD;
    } else if (c >= 0xF100 && c <= 0xF13F) {
        c -= 0xF100;
    } else if (c >= 0xF140 && c <= 0xF17F) {
        c += 0x40;
    }
    if ((c & 0xF000) == 0xF000) {
        c = 0;
    } else if (c == 0xEEEE) {
        c = 0;
    } else if (c < 0x20) {
        c = 0;
    }
    return c;
}

/* 0x8056CE34 (0x1DC): Returns the HID usage code of the key the cursor stands on. */
u8 hkbKeyIterGetUsage(HkbKeyIter* it)
{
    return it->getUsage();
}

/* 0x8056D010 (0x134): Returns the character of the key the cursor stands on. */
u16 hkbKeyIterGetChar(HkbKeyIter* it)
{
    u16 c = hkbNormalizeChar(hkbKeyIterGetCode(it));
    return it->manager->filterAllowed(c);
}

/* 0x8056D144 (0x1D4): Returns the character `usage` produces under the current modifiers, cached in the cursor. */
u16 hkbKeyIterGetCharForUsage(HkbKeyIter* it, u8 usage)
{
    u16 c = hkbNormalizeChar(it->lookupCode(usage));
    return it->manager->filterAllowed(c);
}

/* 0x8056D318 (0x244): Returns the key code of the key the cursor stands on, cached in the cursor. */
u16 hkbKeyIterGetCode(HkbKeyIter* it)
{
    if (!it->isValid()) {
        return 0;
    }
    if (it->code != 0) {
        return it->code;
    }
    if (it->channel >= 1) {
        return 0;
    }
    u32 mods = it->manager->getModifierState();
    if ((mods & 0xD) && (mods & 5) != 5) {
        return 0;
    }
    switch (it->kind) {
    case 0:
    case 1:
    case 3:
        if (it->manager->channels[it->channel].curSlots & (1 << it->bit)) {
            it->code = kbd_lookup_key(it->manager->channels[it->channel].curUsage[it->bit], mods, it->manager->channels[it->channel].mapIndex);
        }
        break;
    case 2:
        if (it->manager->channels[it->channel].prevSlots & (1 << it->bit)) {
            it->code = kbd_lookup_key(it->manager->channels[it->channel].prevUsage[it->bit], mods, it->manager->channels[it->channel].mapIndex);
        }
        break;
    }
    return it->code;
}

/* 0x8056D55C (0x118): Returns whether the cursor stands on a set key bit. */
s32 hkbKeyIterValid(HkbKeyIter* it)
{
    return it->isValid();
}

/* 0x8056D674 (0x118): Returns the cursor moved to the next set key bit after `start`. */
HkbKeyIter hkbKeyIterSeek(HkbKeyIter* start)
{
    HkbKeyIter it(start->manager, start->kind);
    if (start->manager == 0) {
        return it;
    }
    for (it.channel = start->channel; it.channel < 1; it.bit = -1, it.channel++) {
        u32 word = 0;
        switch (start->kind) {
        case 0:
            word = start->manager->channels[it.channel].held;
            break;
        case 1:
            word = start->manager->channels[it.channel].pressed;
            break;
        case 2:
            word = start->manager->channels[it.channel].released;
            break;
        case 3:
            word = start->manager->channels[it.channel].repeated;
            break;
        }
        for (it.bit = start->bit + 1; it.bit < 8; it.bit++) {
            if (word & (1 << it.bit)) {
                return it;
            }
        }
    }
    return it;
}

}
