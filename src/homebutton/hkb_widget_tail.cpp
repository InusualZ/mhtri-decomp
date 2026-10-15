/* homebutton/hkb_widget_tail.cpp - the last `.text` unit of the software keyboard: the USB keyboard's input widget
 *   (constructor 0x8056D814, table .data 0x80658458: it turns the manager's keys into the widget's events) and the
 *   keyboard state HkbKbdState it keeps, the keyboard layout HkbLayout (a nw4r::lyt::Layout whose CreateAnimTransform
 *   builds HkbAnimTransform objects, table .data 0x80658480) and the animation transform HkbAnimTransform (an
 *   AnimTransformBasic that allocates its animation links when it is bound, table .data 0x806584C0).
 * RANGE. .text 0x8056D814-0x8056F2B4 (17 functions); .rodata 0x8057C760-0x8057C808; .data 0x80658458-0x806584E4;
 *   .bss 0x80790DF8-0x80790E04; .sdata 0x80794750-0x80794754; .sdata2 0x8079D7B0-0x8079D7C8.
 *   Edges: the left edge follows homebutton/tiHKBManager.cpp, whose static initialiser 0x8056D78C (.ctors word
 *   0x8056F428) and destructor 0x8056D7D4 end it.  This unit has no `.ctors` word.
 *   The class tables 0x80658458 -> 0x80658480 -> 0x806584C0 rise, MWCC's reverse class order for the three classes
 *   defined here (input widget, HkbLayout, HkbAnimTransform), so the range stays one unit; the `.bss` record
 *   0x80790DF8 is the destructor-chain entry of hkbGetKbdState's static (our object emits the same 0xC bytes).
 *   `.rodata` is hkbKbdInputUpdate's two character-event initialisers and its usage table; `.sdata2` is its
 *   function-key event initialiser and hkbKbdInputSetKeyMap's two key-map tables (both sections byte-identical).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS); no extab, so
 *   `#pragma exceptions off`.  `#pragma auto_inline on` around the animation-count helpers: retail inlines the
 *   recursive pane count two levels into itself and into HkbAnimTransform::Bind(Pane*).
 * NAMES. GUESS: the file name, from its place after tiHKBManager.cpp; GUESS class names HkbLayout, HkbAnimTransform
 *   (from their bases and tables), HkbKbdInput (with its owner, mode, config and lock views) and helper names
 *   hkbAnimCountContents, hkbAnimCountContentsTree, hkbAnimBindNamedPane, hkbKbdInputActivate, hkbKbdInputSyncShift,
 *   hkbKbdInputUpdate (slot 0x20), hkbKbdInputQueueKey (slot 0x24), hkbKbdInputSetKeyMap (read off the bodies);
 *   GUESS view slot names HkbKbdInputConfig::isNumeric/allowsDecimalPoint and HkbKbdInputLock::isLocked (from the
 *   keys the widget lets through), HkbKbdCharEvent/HkbKbdKeyEvent (the event 0 and event 38 records).
 *   HkbKbdState and its members follow homebutton/hkb_keyboard.cpp's names (setCapsLock, the three bits).
 * SHAPES. hkbAnimCountContents counts in an `int` and hoists the content count; HkbAnimTransform::Bind(Pane*) counts
 *   the tree with a literal `true` (retail passes `li r5,1` and drops the inner tests) into a `u32`; the links are
 *   pointed through a two-store helper (retail computes the link address once for both stores).
 * SHAPES. The three `.sdata` bytes 0x80794750..0x80794752 are one 3-byte object (hkbKbdShiftDefaults): separate
 *   zero-initialised globals would land in `.sbss`.
 * SHAPES. hkbKbdInputUpdate is one body whose early exits leave a `for (;;)` (the single hkbKbdInputSyncShift call and
 *   retail's stack layout: an inline helper with returns allocates its locals upwards).  The function keys share one
 *   send after the switch (the others `continue`), the space key and the special keys build their event directly or
 *   through hkbKbdInputSendKey (its locals sit below the iterator temporaries), the decimal point falls through to
 *   the refusing default, the kana punctuation goes through hkbKbdInputToKana, ' ' is compared as `L' '` (cmplwi),
 *   the special-key number is an `s32` and the caps-lock usage test is a one-case switch.  The usage table is a
 *   local static of the special-key branch, so `.rodata` keeps retail's order (event initialisers first).
 * RESIDUALS. hkbKbdInputSyncShift: retail loads hkbKbdShiftDefaults' second and third bytes as `sym+1@sda21` /
 *   `sym+2@sda21`; ours materialises the base (`li r3,sym@sda21`) and loads at +1/+2.
 * RESIDUALS. hkbGetKbdState's guard and object are retail's `.sbss` 0x80794D60/0x80794D64, inside
 *   menu/menu_plsearch.cpp's and menu/menu_sysmsg.cpp's claims (a seam request); ours are this object's own.
 * RESIDUALS. HkbLayout::CreateAnimTransform, hkbAnimCountContents, HkbAnimTransform::Bind(Pane*): register
 *   allocation only (the file-version test's two registers, the counter and the content count).
 * RESIDUALS. hkbKbdInputUpdate: ours addresses `.rodata` through one hoisted section base (`...rodata.0`, r27 for
 *   the whole function), retail loads each object's address before its loop, which shifts every callee-saved
 *   register by one; retail places the shared function-key send between the key cases and the space case, ours
 *   after the space case.
 * RESIDUALS. Unwritten: the input widget's constructor 0x8056D814 (0x24) stores the table 0x80658458, whose slots name
 *   other units' inline functions (0x8055CD20, 0x8054F53C, 0x80556DBC) - writing it needs the widget's class with an
 *   MWCC-emitted table (rule 10).
 */

#include "types.h"
#include "homebutton/hkb_widget_tail.h"
#include "homebutton/tiHKBManager.h"
#include "homebutton/hkb_layout.h"
#include "MSL_C/strstr.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

namespace {

/* Returns whether a binary resource has the layout format's major version 0 and a minor version 8 to 10. */
inline bool TestFileVersion(const nw4r::ut::BinaryFileHeader& fileHeader) {
    u8 majorVer = (u8)((fileHeader.version >> 8) & 0xFF);
    u8 minorVer = (u8)(fileHeader.version & 0xFF);
    bool bValid = false;

    if (majorVer == 0 && minorVer >= 8 && minorVer <= 10) {
        bValid = true;
    }
    return bValid;
}

}  // namespace

/* The text-entry mode object the input widget asks (its owner's slot 0x98): slot 0x10 whether it is busy, 0xA8
 * whether it takes keys.  size: 0x4 (approximation: only the vptr is touched) */
class HkbKbdInputMode {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual s32 isBusy();
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
    /* slot +0x0A8 */ virtual s32 takesKeys();
}; /* size: 0x04 */

/* The object the owner's slot 0x80 returns: slot 0x48 whether the entry takes digits only, 0x4C whether it then
 * also takes a decimal point (GUESS names, from the keys the input widget lets through).
 * size: 0x4 (approximation: only the vptr is touched) */
class HkbKbdInputConfig {
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
    /* slot +0x048 */ virtual s32 isNumeric();
    /* slot +0x04C */ virtual s32 allowsDecimalPoint();
}; /* size: 0x04 */

/* The object the owner's slot 0x88 returns: slot 0xD8 whether the USB keyboard is ignored (GUESS name).
 * size: 0x4 (approximation: only the vptr is touched) */
class HkbKbdInputLock {
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
    /* slot +0x0D8 */ virtual s32 isLocked();
}; /* size: 0x04 */

/* The owner of the input widget (its +0x24): slot 0x78 returns the software keyboard, 0x80 the entry settings, 0x88
 * the lock, 0x98 the text-entry mode; +0x30 is its sound player.
 * size: 0x34 (approximation: the vptr and the sound player) */
class HkbKbdInputOwner {
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
    /* slot +0x078 */ virtual HkbKeyboard* getKeyboard();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual HkbKbdInputConfig* getConfig();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual HkbKbdInputLock* getLock();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual HkbKbdInputMode* getInputMode();

    /* +0x04 */ u8 unused_0x04[0x2C];
    /* +0x30 */ HbmSoundPlayer* mpSound;
}; /* size: 0x34 */

/* The USB keyboard's input widget (table .data 0x80658458, constructor 0x8056D814): its event sink at +0x0C, the
 * characters queued by its owner at +0x10, their count at +0x20, the first-activation flag at +0x21 and the owner
 * it reports to at +0x24.  Its slots are declared to dispatch through them; nothing here defines one, so no table
 * is emitted.
 * size: 0x28 */
class HkbKbdInput {
public:
    /* slot +0x008 */ virtual ~HkbKbdInput();
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x014 */ virtual void setEventSink(HkbEventSink* sink);
    /* untyped: caller-owned payload - the argument record of event `id` */
    /* slot +0x018 */ virtual void sendEvent(u32 id, void* data);
    /* slot +0x01C */ virtual void onCommand(u32 command, u32 arg);
    /* slot +0x020 */ virtual s32 update(HkbManager* manager);
    /* slot +0x024 */ virtual void queueKey(HkbManager* manager, u8 usage, s8 bReset);

    /* +0x04 */ u8 unused_0x04[8];
    /* +0x0C */ HkbEventSink* mpEventSink;
    /* +0x10 */ u16 queue[8];
    /* +0x20 */ s8 queued;
    /* +0x21 */ s8 activated;
    /* +0x22 */ u8 unused_0x22[2];
    /* +0x24 */ HkbKbdInputOwner* owner;
}; /* size: 0x28 */

/* The character record of event 0 (a typed character): the character, the case bits (1 shift, 2 caps lock) and
 * the typed flag.  size: 0x10 */
struct HkbKbdCharEvent {
    /* +0x00 */ u16 c;
    /* +0x02 */ u16 unused_0x02;
    /* +0x04 */ u32 caseBits;
    /* +0x08 */ u8 bTyped;
    /* +0x09 */ u8 unused_0x09[3];
    /* +0x0C */ u32 unused_0x0C;
}; /* size: 0x10 */

/* The record of event 38 (a function key): the key number and the modifier word.  size: 0x8 */
struct HkbKbdKeyEvent {
    /* +0x00 */ u32 key;
    /* +0x04 */ u32 modifiers;
}; /* size: 0x8 */


/* The shift states the keyboard state takes: when the widget starts, when shift is reported down and up.
 * size: 0x3 */
struct HkbKbdShiftDefaults {
    /* +0x0 */ u8 onStart;
    /* +0x1 */ u8 onDown;
    /* +0x2 */ u8 onUp;
}; /* size: 0x3 */

HkbKbdShiftDefaults hkbKbdShiftDefaults = {0, 1, 0};

/* 0x8056D838 (0xA4): Takes the start shift state and sets the manager's modifiers, reporting the first
 * activation (0x100) once. */
extern "C" void hkbKbdInputActivate(HkbKbdInput* self) {
    HkbKbdState& kbd = hkbGetKbdState();
    kbd.mbLeftShift = hkbKbdShiftDefaults.onStart;
    u32 state = hkbGetModifierState(&hkbManager);
    if (!self->activated) {
        state |= 0x100;
        self->activated = 1;
    }
    hkbSetModifierState(&hkbManager, state, 0x700);
}

/* 0x8056D8DC (0x184): Follows the manager's shift modifier (bit 2) into the keyboard state and the software
 * keyboard's shift, while the text-entry mode is busy or takes keys. */
extern "C" void hkbKbdInputSyncShift(HkbKbdInput* self, HkbManager* manager) {
    HkbKbdState& kbd = hkbGetKbdState();
    u32 state = hkbGetModifierState(manager);
    if (self->owner->getInputMode()->isBusy() || self->owner->getInputMode()->takesKeys()) {
        bool shift = kbd.isShiftHeld();
        if (state & 2) {
            kbd.mbLeftShift = hkbKbdShiftDefaults.onDown;
            if (!shift) {
                self->owner->getKeyboard()->setShiftOn(0);
            }
        } else {
            kbd.mbLeftShift = hkbKbdShiftDefaults.onUp;
            if (shift && !kbd.isShiftHeld()) {
                self->owner->getKeyboard()->setShiftOff();
            }
        }
    }
}

/* Turns the half-width punctuation `c` into its full-width kana form and lights the software keyboard's key for it;
 * a keypad minus (`usage` 0x56) keeps its form. */
static inline void hkbKbdInputToKana(HkbKbdInput* self, u16& c, u8 usage) {
    switch (c) {
    case ',':
        c = 0x3001;
        break;
    case '.':
        c = 0x3002;
        break;
    case '-':
        if (usage != 0x56) {
            c = 0x30FC;
        }
        break;
    case '[':
        c = 0x300C;
        break;
    case ']':
        c = 0x300D;
        break;
    case '/':
        c = 0x30FB;
        break;
    case '\\':
        return;
    default:
        return;
    }
    HbmKey* key = self->owner->getKeyboard()->getKeyAnim(c);
    if (key != NULL) {
        key->setAnim(0);
    }
}

/* Sends event 38 for function key `key` with the modifier word. */
static inline void hkbKbdInputSendKey(HkbKbdInput* self, u32 key, u32 mods) {
    HkbKbdKeyEvent event = {key, mods};
    self->sendEvent(38, &event);
}

/* 0x8056DA60 (0xEDC): Turns this frame's USB keyboard keys into the widget's events - the pressed control keys, the
 * repeated function keys, the characters the owner queued and the pressed character keys - then follows the
 * shift key. */
extern "C" s32 hkbKbdInputUpdate(HkbKbdInput* self, HkbManager* manager) {
    HkbKeyIter it(NULL, 0);
    u32 mods;

    for (;;) {
        if (self->owner->getLock()->isLocked()) {
            break;
        }

        mods = hkbGetModifierState(manager);
        for (it = hkbKeyIterBeginWord1(manager); hkbKeyIterValid(&it); it = hkbKeyIterSeek(&it)) {
            u8 usage = hkbKeyIterGetUsage(&it);
            switch (usage) {
            case 0x28:
            case 0x58:
                hkbGetModifierState(manager);
                break;
            case 0x29:
            case 0x2A:
                if ((hkbGetModifierState(manager) & 4) && usage == 0x2A) {
                    self->sendEvent(1, NULL);
                }
                if ((hkbGetModifierState(manager) & 1) && usage == 0x2A) {
                    self->sendEvent(1, NULL);
                }
                break;
            }
        }

        it = hkbKeyIterBeginWord3(manager);
        if (hkbKeyIterValid(&it)) {
            if (!self->owner->getInputMode()->isBusy() && self->owner->getInputMode()->takesKeys()) {
                break;
            }
            for (; hkbKeyIterValid(&it); it = hkbKeyIterSeek(&it)) {
                u16 code = hkbKeyIterGetCode(&it);
                hkbKeyIterGetUsage(&it);
                u32 key;
                switch (code) {
                case 0xF16E:
                case 0xF1AE:
                    self->sendEvent(2, NULL);
                    continue;
                case 0xF1C8:
                    self->sendEvent(1, NULL);
                    continue;
                case 0xF176:
                case 0xF1B6:
                    key = 32;
                    break;
                case 0xF174:
                case 0xF1B4:
                    key = 31;
                    break;
                case 0xF172:
                case 0xF1B2:
                    key = 30;
                    break;
                case 0xF178:
                case 0xF1B8:
                    key = 29;
                    break;
                case ' ':
                    if (self->owner->getInputMode()->isBusy() || !self->owner->getConfig()->isNumeric()) {
                        HkbKbdKeyEvent event = {8, mods};
                        self->sendEvent(38, &event);
                    }
                    continue;
                case 0xF173:
                case 0xF175:
                case 0xF177:
                case 0xF1B3:
                case 0xF1B5:
                default:
                    continue;
                }
                hkbKbdInputSendKey(self, key, mods);
            }
        }

        {
            int i = 0;
            while (self->queued > 0) {
                bool bAccept = true;
                self->queued--;
                u16 c = self->queue[i++];
                if (self->owner->getInputMode()->isBusy() || !self->owner->getConfig()->isNumeric()) {
                    if (self->owner->getKeyboard()->isCompactPanel() && (c < '0' || c > '9') && (c < 'A' || c > 'Z') &&
                        (c < 'a' || c > 'z')) {
                        bAccept = false;
                    }
                } else {
                    switch (c) {
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
                        break;
                    case '.':
                        if (self->owner->getConfig()->allowsDecimalPoint()) {
                            break;
                        }
                        /* a decimal point the entry does not take is refused like any other character */
                    default:
                        bAccept = false;
                        break;
                    }
                }
                if (bAccept) {
                    bool bKana = false;
                    if (self->owner->getInputMode()->isBusy()) {
                        bKana = self->owner->getKeyboard()->getMode() != 0;
                    }
                    if (bKana) {
                        hkbKbdInputToKana(self, c, 0);
                    }
                    mods = 0;
                    if (hkbGetModifierState(manager) & 2) {
                        mods |= 1;
                    }
                    if (hkbGetModifierState(manager) & 0x200) {
                        mods |= 2;
                    }
                    if (!(mods & 1)) {
                        HkbKeyboard* keyboard = self->owner->getKeyboard();
                        keyboard->mKeyState.setCaseFlags(keyboard->mKeyState.mFlags & ~0x8F);
                    }
                    HkbKbdCharEvent event = {c, 0, mods, 1};
                    self->sendEvent(0, &event);
                }
            }
        }

        it = hkbKeyIterBeginWord1(manager);
        if (!hkbKeyIterValid(&it)) {
            break;
        }
        if (!self->owner->getInputMode()->isBusy() && self->owner->getInputMode()->takesKeys()) {
            break;
        }
        if (!self->owner->getKeyboard()->isHalfWidth()) {
            if (self->owner->getInputMode()->isBusy() && self->owner->mpSound != NULL) {
                self->owner->mpSound->play(13);
            }
            self->owner->getKeyboard()->setHalfWidth(1);
        }
        for (; hkbKeyIterValid(&it); it = hkbKeyIterSeek(&it)) {
            u16 c = hkbKeyIterGetChar(&it);
            u8 usage = hkbKeyIterGetUsage(&it);
            if (c == L' ') {
            } else if (c != 0) {
                bool bAccept = true;
                if (self->owner->getInputMode()->isBusy() || !self->owner->getConfig()->isNumeric()) {
                    if (self->owner->getKeyboard()->isCompactPanel()) {
                        if ((c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
                            bAccept = false;
                        }
                        if (self->owner->getKeyboard()->mInputStyle == 1 && c >= 'a' && c <= 'z') {
                            c -= 0x20;
                        }
                    }
                } else {
                    switch (c) {
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
                        break;
                    case '.':
                        if (self->owner->getConfig()->allowsDecimalPoint()) {
                            break;
                        }
                        /* a decimal point the entry does not take is refused like any other character */
                    default:
                        bAccept = false;
                        break;
                    }
                }
                if (bAccept) {
                    bool bKana = false;
                    if (self->owner->getInputMode()->isBusy()) {
                        bKana = self->owner->getKeyboard()->getMode() != 0;
                    }
                    if (bKana) {
                        hkbKbdInputToKana(self, c, usage);
                    }
                    mods = 0;
                    if (hkbGetModifierState(manager) & 2) {
                        mods |= 1;
                    }
                    if (hkbGetModifierState(manager) & 0x200) {
                        mods |= 2;
                    }
                    if (!(mods & 1)) {
                        HkbKeyboard* keyboard = self->owner->getKeyboard();
                        keyboard->mKeyState.setCaseFlags(keyboard->mKeyState.mFlags & ~0x8F);
                    }
                    HkbKbdCharEvent event = {c, 0, mods, 1};
                    self->sendEvent(0, &event);
                }
            } else {
                bool bAccept = true;
                static const u8 hkbKbdUsageKeys[0x88] = {
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x01, 0x0A, 0x03, 0x08, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
                0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x19, 0x1A, 0x1B, 0x13, 0x15, 0x17, 0x14, 0x16, 0x18, 0x00,
                0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0F, 0x0E, 0x00, 0x00, 0x00, 0x00,
                0x11, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x05, 0x07, 0x0B, 0x06, 0x05, 0x07, 0x0B,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            };
            s32 key;
            if (usage < 0x20) {
                key = 0;
            } else if (usage < 0x98) {
                key = hkbKbdUsageKeys[usage - 0x20];
            } else if (usage < 0xE0) {
                key = 0;
            } else if (usage < 0xF0) {
                key = hkbKbdUsageKeys[usage - 0x68];
            } else {
                key = 0;
            }
                if (key == 28) {
                    bool bOn = !(hkbGetModifierState(manager) & 0x100);
                    if (!self->owner->getInputMode()->isBusy() && self->owner->getConfig()->isNumeric()) {
                        bOn = true;
                    }
                    hkbSetModifierState(manager, bOn ? 0x100 : 0, 0x100);
                    if (self->owner->mpSound != NULL) {
                        self->owner->mpSound->play(13);
                    }
                }
                if (!self->owner->getInputMode()->isBusy() && self->owner->getConfig()->isNumeric()) {
                    switch (key) {
                    case 21:
                    case 22:
                    case 29:
                    case 30:
                    case 31:
                    case 32:
                        break;
                    default:
                        bAccept = false;
                        break;
                    }
                }
                if (bAccept) {
                    hkbKbdInputSendKey(self, key, mods);
                }
            }
            switch (hkbKeyIterGetUsage(&it)) {
            case 0x39:
                if (self->owner->getInputMode()->isBusy() || self->owner->getInputMode()->takesKeys()) {
                    self->owner->getKeyboard()->toggleCapsLock();
                }
                break;
            }
        }
        break;
    }
    hkbKbdInputSyncShift(self, manager);
    return 0;
}


/* 0x8056E93C (0xDC): Queues the character a key produces under the current modifiers; `bReset` empties the queue
 * and drops the caps lock first. */
extern "C" void hkbKbdInputQueueKey(HkbKbdInput* self, HkbManager* manager, u8 usage, s8 bReset) {
    HkbKeyIter it(NULL, 0);
    if (bReset) {
        self->queued = 0;
        hkbSetModifierState(manager, 0, 0x200);
    }
    it = hkbKeyIterBeginWord0(manager);
    u16 c = hkbKeyIterGetCharForUsage(&it, usage);
    if (self->queued < 8) {
        self->queue[self->queued++] = c;
    }
}

/* 0x8056EA18 (0x30): Selects the USB keyboard's key map for a layout type and language. */
extern "C" void hkbKbdInputSetKeyMap(HkbKbdInput* self, s32 layoutType, s32 language) {
    static const u8 hkbKbdMapsKana[8] = {9, 17, 16, 5, 6, 8, 14, 0};
    static const u8 hkbKbdMapsLatin[8] = {9, 17, 16, 2, 6, 8, 10, 0};
    (void)self;
    if (layoutType == 2) {
        hkbManagerSetMap(&hkbManager, hkbKbdMapsKana[language]);
    } else {
        hkbManagerSetMap(&hkbManager, hkbKbdMapsLatin[language]);
    }
}

#pragma push
#pragma auto_inline on

/* 0x8056EB10 (0xBC): The number of the animation's contents (pane or material) named `name`. */
extern "C" int hkbAnimCountContents(const nw4r::lyt::AnimTransform* anim, const char* name) {
    const nw4r::lyt::res::AnimationBlock* pRes = anim->mpRes;
    if (pRes == NULL) {
        return 0;
    }
    int num = 0;
    const u32* animContOffsets = nw4r::lyt::detail::ConvertOffsToPtr<u32>(pRes, pRes->animContOffsetsOffset);
    const u16 animContNum = pRes->animContNum;
    for (u16 i = 0; i < animContNum; ++i) {
        const nw4r::lyt::res::AnimationContent& animCont =
            *nw4r::lyt::detail::ConvertOffsToPtr<nw4r::lyt::res::AnimationContent>(pRes, animContOffsets[i]);
        if (animCont.type == 0) {
            if (strncmp(animCont.name, name, 17) == 0) {
                ++num;
            }
        } else {
            if (strncmp(animCont.name, name, 17) == 0) {
                ++num;
            }
        }
    }
    return num;
}

/* 0x8056EBCC (0xE4): The number of the animation's contents named by `pane` and, when `bRecursive`, by its
 * descendants. */
extern "C" int hkbAnimCountContentsTree(const nw4r::lyt::AnimTransform* anim, nw4r::lyt::Pane* pane, bool bRecursive) {
    int num = hkbAnimCountContents(anim, pane->mName);
    if (bRecursive) {
        for (nw4r::ut::LinkList<nw4r::lyt::Pane, 4>::Iterator it = pane->mChildList.GetBeginIter();
             it != pane->mChildList.GetEndIter(); ++it) {
            num += hkbAnimCountContentsTree(anim, &*it, bRecursive);
        }
    }
    return num;
}

#pragma pop

/* Points an animation link at a transform's content `idx`. */
static inline void hkbAnimLinkSet(nw4r::lyt::AnimationLink* link, nw4r::lyt::AnimTransform* anim, u16 idx) {
    link->mAnimTrans = anim;
    link->mIdx = idx;
}

/* An animation transform that allocates its animation links when it is bound, one per content it finds there.
 * size: 0x20 */
class HkbAnimTransform : public nw4r::lyt::AnimTransformBasic {
public:
    virtual void SetResource(const nw4r::lyt::res::AnimationBlock* pRes, nw4r::lyt::ResourceAccessor* pResAccessor);
    virtual void Bind(nw4r::lyt::Pane* pPane, bool bRecursive, bool bDisable);
    virtual void Bind(nw4r::lyt::Material* pMaterial, bool bDisable);
}; /* size: 0x20 */

/* 0x8056EA48 (0xC8): Sets the resource and loads its texture files through `pResAccessor`; the links wait for Bind. */
void HkbAnimTransform::SetResource(const nw4r::lyt::res::AnimationBlock* pRes,
                                   nw4r::lyt::ResourceAccessor* pResAccessor) {
    mpRes = pRes;
    mpFileResAry = NULL;

    if (pRes->fileNum > 0) {
        mpFileResAry = static_cast<void**>(nw4r::lyt::Layout::AllocMemory(sizeof(void*) * pRes->fileNum));
        if (mpFileResAry) {
            const u32* fileNameOffsets = nw4r::lyt::detail::ConvertOffsToPtr<u32>(mpRes, sizeof(*mpRes));
            for (int i = 0; i < mpRes->fileNum; ++i) {
                const char* fileName = nw4r::lyt::detail::ConvertOffsToPtr<char>(fileNameOffsets, fileNameOffsets[i]);
                mpFileResAry[i] = pResAccessor->GetResource('timg', fileName, NULL);
            }
        }
    }
}

/* 0x8056ECB0 (0x210): Allocates one link per content naming `pPane`'s panes and materials (below it when
 * `bRecursive`) and binds each of them. */
void HkbAnimTransform::Bind(nw4r::lyt::Pane* pPane, bool bRecursive, bool bDisable) {
    u32 animNum;
    (void)bDisable;
    if (!bRecursive) {
        animNum = hkbAnimCountContents(this, pPane->mName);
    } else {
        animNum = hkbAnimCountContentsTree(this, pPane, true);
    }

    mAnimLinkAry = static_cast<nw4r::lyt::AnimationLink*>(
        nw4r::lyt::Layout::AllocMemory(sizeof(nw4r::lyt::AnimationLink) * animNum));
    mAnimLinkNum = animNum;
    memset(mAnimLinkAry, 0, sizeof(nw4r::lyt::AnimationLink) * animNum);
    for (u16 i = 0; i < animNum; ++i) {
        new (&mAnimLinkAry[i]) nw4r::lyt::AnimationLink();
    }

    u32 linkIdx = 0;
    const u32* animContOffsets = nw4r::lyt::detail::ConvertOffsToPtr<u32>(mpRes, mpRes->animContOffsetsOffset);
    for (u16 i = 0; i < mpRes->animContNum; ++i) {
        const nw4r::lyt::res::AnimationContent& animCont =
            *nw4r::lyt::detail::ConvertOffsToPtr<nw4r::lyt::res::AnimationContent>(mpRes, animContOffsets[i]);
        if (animCont.type == 0) {
            nw4r::lyt::Pane* pFindPane = pPane->FindPaneByName(animCont.name, bRecursive);
            if (pFindPane) {
                hkbAnimLinkSet(&mAnimLinkAry[linkIdx], this, i);
                pFindPane->AddAnimationLink(&mAnimLinkAry[linkIdx]);
                ++linkIdx;
            }
        } else {
            nw4r::lyt::Material* pFindMat = pPane->FindMaterialByName(animCont.name, bRecursive);
            if (pFindMat) {
                hkbAnimLinkSet(&mAnimLinkAry[linkIdx], this, i);
                pFindMat->AddAnimationLink(&mAnimLinkAry[linkIdx]);
                ++linkIdx;
            }
        }
    }
}

/* 0x8056EEC0 (0x4): Materials are bound through their panes. */
void HkbAnimTransform::Bind(nw4r::lyt::Material* pMaterial, bool bDisable) {
    (void)pMaterial;
    (void)bDisable;
}

/* 0x8056EEC4 (0x168): Allocates one link per content named `name` and binds each to `pane` (pane contents) or to
 * its material (material contents). */
extern "C" void hkbAnimBindNamedPane(HkbAnimTransform* anim, nw4r::lyt::Pane* pane, const char* name) {
    u32 animNum = hkbAnimCountContents(anim, name);

    anim->mAnimLinkAry = static_cast<nw4r::lyt::AnimationLink*>(
        nw4r::lyt::Layout::AllocMemory(sizeof(nw4r::lyt::AnimationLink) * animNum));
    anim->mAnimLinkNum = animNum;
    memset(anim->mAnimLinkAry, 0, sizeof(nw4r::lyt::AnimationLink) * animNum);
    for (u16 j = 0; j < animNum; ++j) {
        for (u16 i = 0; i < animNum; ++i) {
            new (&anim->mAnimLinkAry[i]) nw4r::lyt::AnimationLink();
        }
    }

    u32 linkIdx = 0;
    const u32* animContOffsets =
        nw4r::lyt::detail::ConvertOffsToPtr<u32>(anim->mpRes, anim->mpRes->animContOffsetsOffset);
    for (u16 i = 0; i < anim->mpRes->animContNum; ++i) {
        const nw4r::lyt::res::AnimationContent& animCont =
            *nw4r::lyt::detail::ConvertOffsToPtr<nw4r::lyt::res::AnimationContent>(anim->mpRes, animContOffsets[i]);
        if (strncmp(animCont.name, name, 17) == 0) {
            hkbAnimLinkSet(&anim->mAnimLinkAry[linkIdx], anim, i);
            if (animCont.type == 0) {
                pane->AddAnimationLink(&anim->mAnimLinkAry[linkIdx]);
            } else {
                pane->GetMaterial()->AddAnimationLink(&anim->mAnimLinkAry[linkIdx]);
            }
            ++linkIdx;
        }
    }
}

/* 0x8056F02C (0x1A0): Creates one HkbAnimTransform per `pai1` block of an RLAN image of a known animation kind and
 * returns the last; NULL for an unknown file version. */
/* untyped: byte range - the RLAN resource image */
nw4r::lyt::AnimTransform* HkbLayout::CreateAnimTransform(const void* animResBuf, nw4r::lyt::ResourceAccessor* pResAcsr) {
    const nw4r::ut::BinaryFileHeader* pFileHead = static_cast<const nw4r::ut::BinaryFileHeader*>(animResBuf);

    if (!TestFileVersion(*pFileHead)) {
        return NULL;
    }

    const nw4r::ut::BinaryBlockHeader* pDataBlockHead =
        nw4r::lyt::detail::ConvertOffsToPtr<nw4r::ut::BinaryBlockHeader>(pFileHead, pFileHead->headerSize);
    nw4r::lyt::AnimTransform* pAnimTrans = NULL;

    for (int i = 0; i < pFileHead->dataBlocks; ++i) {
        switch (pDataBlockHead->kind) {
        case 'pai1':
            switch (pFileHead->signature) {
            case 'RLPA':
            case 'RLTS':
            case 'RLVC':
            case 'RLMC':
            case 'RLVI':
            case 'RLTP':
            case 'RLAN': {
                HkbAnimTransform* pAnimTransBasic =
                    new (nw4r::lyt::Layout::AllocMemory(sizeof(HkbAnimTransform))) HkbAnimTransform;
                if (pAnimTransBasic) {
                    pAnimTransBasic->SetResource(
                        reinterpret_cast<const nw4r::lyt::res::AnimationBlock*>(pDataBlockHead), pResAcsr);
                    pAnimTrans = pAnimTransBasic;
                }
            } break;
            }
            if (pAnimTrans) {
                mAnimTransList.PushBack(pAnimTrans);
            }
            break;
        }
        pDataBlockHead = nw4r::lyt::detail::ConvertOffsToPtr<nw4r::ut::BinaryBlockHeader>(pDataBlockHead,
                                                                                          pDataBlockHead->size);
    }
    return pAnimTrans;
}


/* 0x8056F224 (0x10): Starts with the caps lock off and no shift key held. */
HkbKbdState::HkbKbdState() {
    mbCapsLock = 0;
    mbLeftShift = 0;
    mbRightShift = 0;
}

/* 0x8056F234 (0x40): Nothing to release. */
HkbKbdState::~HkbKbdState() {
}

/* 0x8056F274 (0x40): Stores the caps lock and sets the manager's caps-lock modifier (0x200) to match. */
void HkbKbdState::setCapsLock(const u8& bCapsLock) {
    mbCapsLock = bCapsLock;
    if (mbCapsLock) {
        hkbSetModifierState(&hkbManager, 0x200, 0x200);
    } else {
        hkbSetModifierState(&hkbManager, 0, 0x200);
    }
}
