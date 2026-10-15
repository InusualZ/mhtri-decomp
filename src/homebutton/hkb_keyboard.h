/*
 * homebutton/hkb_keyboard.h - declarations of the symbols owned by `homebutton/hkb_keyboard.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HKB_KEYBOARD_H
#define HOMEBUTTON_HKB_KEYBOARD_H

#include "types.h"

#ifdef __cplusplus
#include "nw4r/fn_805012C4.h"
#include "homebutton/hkb_material.h"

namespace nw4r {
namespace lyt {

class Group;
class AnimTransform;

namespace detail {

/* The texture coordinates of one quad: four corners. size: 0x20 */
struct TexCoordQuad {
    /* +0x00 */ math::VEC2 coord[4];
};

/* A pane's texture-coordinate quads, allocated from the layout allocator. size: 0x8 */
class TexCoordAry {
public:
    TexCoordAry();
    void Free();
    void Reserve(u8 num);
    void SetSize(u8 num);
    /* untyped: byte range - the resource's packed TexCoordQuad array */
    void Copy(const void* pResTexCoord, u8 texCoordNum);

    u8 GetSize() const { return mNum; }
    const TexCoordQuad* GetArray() const { return mpData; }
    bool IsEmpty() const { return mCap == 0; }

    /* +0x0 */ u8 mCap;
    /* +0x1 */ u8 mNum;
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ TexCoordQuad* mpData;
};

/* Returns whether two pane or group names agree in their first 16 characters. */
bool EqualsResName(const char* name1, const char* name2);

/* Returns whether two material names agree in their first 20 characters. */
bool EqualsMaterialName(const char* name1, const char* name2);
/* Returns whether a binary resource header carries `signature` and the big-endian byte-order mark. */
bool TestFileHeader(const ut::BinaryFileHeader& fileHeader, u32 signature);
/* Returns whether the vertex colours or the alpha are not plain white. */
bool IsModulateVertexColor(ut::Color* vtxColors, u8 glbAlpha);
/* Returns a colour with its alpha scaled by `alpha`. */
const ut::Color MultipleAlpha(const ut::Color col, u8 alpha);
/* Sets the GX vertex format: position, optional colour and `texCoordNum` texture coordinates. */
void SetVertexFormat(bool bModulate, u8 texCoordNum);
/* Draws one quad of `size` from `basePt` with its texture coordinates and corner colours. */
void DrawQuad(const math::VEC2& basePt, const Size& size, u8 num, const TexCoordQuad* texCoords,
              const ut::Color* vtxColors);
/* Draws one quad with its corner colours' alpha scaled by `alpha`. */
void DrawQuad(const math::VEC2& basePt, const Size& size, u8 num, const TexCoordQuad* texCoords,
              const ut::Color* vtxColors, u8 alpha);
/* Binds an animation to every pane of a group. */
void BindAnimation(Group* pGroup, AnimTransform* pAnimTrans, bool bRecursive, bool bDisable);

}  // namespace detail
}  // namespace lyt
}  // namespace nw4r
#endif

#ifdef __cplusplus
struct MEMAllocator;
class HbmPaneManager;
class HkbInputWidget;

/* The band's event handler: the pane manager it reports to (class table 0x8064DE70 in this unit's .data).
 * size: 0x8 */
class HbmEventHandler {
public:
    HbmEventHandler() : mpManager(NULL) {}

    /* slot +0x008 */ virtual ~HbmEventHandler() {}
    /* untyped: caller-owned payload - the argument record of the event */
    /* slot +0x00C */ virtual void onEvent(u32 id, u32 event, void* data);
    /* slot +0x010 */ virtual void setManager(HbmPaneManager* manager);
    /* slot +0x014 */ virtual HbmPaneManager* getManager();

    /* +0x04 */ HbmPaneManager* mpManager;
}; /* size: 0x8 */

/* The receiver of an input widget's notifications (the widget's +0x0C): the slots the band calls.
 * size: 0x4 (approximation: only the vptr is touched) */
class HkbEventSink {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* untyped: caller-owned payload - the argument record of event `id` */
    /* slot +0x018 */ virtual void onEvent(u32 id, void* data);
    /* slot +0x01C */ virtual void setOwner(HkbInputWidget* owner);
}; /* size: 0x4 */

/* The root of the band's input widgets (the software keyboard, the cellphone keypad input and the band's widgets
 * derive from it): its notification sink and its layout type; the slots 0x08..0x30 are the ones every derived
 * class table shares.  size: 0x14 (approximation: the fields the shared members read) */
class HkbInputWidget {
public:
    /* slot +0x008 */ virtual ~HkbInputWidget() {}
    /* slot +0x00C */ virtual void create(MEMAllocator* allocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x014 */ virtual void setEventSink(HkbEventSink* sink);
    /* untyped: caller-owned payload - the argument record of event `id` */
    /* slot +0x018 */ virtual void sendEvent(u32 id, void* data);
    /* slot +0x01C */ virtual void onCommand(u32 command, u32 arg);
    /* slot +0x020 */ virtual void onPaneEvent(u32 event, const char* paneName);
    /* slot +0x024 */ virtual s32 isBusy();
    /* slot +0x028 */ virtual void setLayoutType(s32 type);
    /* slot +0x02C */ virtual s32 getLayoutType();
    /* slot +0x030 */ virtual void calc();

    /* +0x04 */ u32 unused_0x04;
    /* +0x08 */ u32 unused_0x08;
    /* +0x0C */ HkbEventSink* mpEventSink;
    /* +0x10 */ s32 mLayoutType;
}; /* size: 0x14 */

#include "homebutton/hbm_value.h"
#include "homebutton/hbm_hermite.h"
#include "homebutton/hkb_qwerty_keys.h"
#include "homebutton/tiHKBManager.h"
#include "MEM/MEMAllocFromAllocator.h"

/* ---------------------------------------------------------------------------------------------------
 * The software keyboard.
 * ------------------------------------------------------------------------------------------------ */

class HkbKeyboardBase;
struct HkbManager;

/* The object the keyboard's +0x48 points at (the input context of the manager), as the slots this file calls
 * see it: no method is defined here, so no table is emitted.  size: 0x4 (approximation: only the vptr) */
class HkbContextView {
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
    /* slot +0x068 */ virtual HkbContextView* v_0x068();
    /* slot +0x06C */ virtual void v_0x06C();
    /* slot +0x070 */ virtual void v_0x070();
    /* slot +0x074 */ virtual void v_0x074();
    /* slot +0x078 */ virtual HkbContextView* v_0x078();
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* slot +0x088 */ virtual void v_0x088();
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual HkbContextView* v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* slot +0x098 */ virtual HkbContextView* v_0x098();
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
    /* slot +0x0E0 */ virtual HkbContextView* v_0x0E0();
    /* slot +0x0E4 */ virtual s32 v_0x0E4();
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
    /* slot +0x13C */ virtual void v_0x13C();
    /* slot +0x140 */ virtual s32 v_0x140();
};

/* One kana/symbol key of a layout: the key pane's name and its character per input column. size: 0x20 */
struct HkbKanaKeyDef {
    /* +0x00 */ char paneName[18];
    /* +0x12 */ u16 chars[7];
};

/* One command key of a layout: the key pane's name and the command it issues. size: 0x18 */
struct HkbCommandKeyDef {
    /* +0x00 */ char paneName[20];
    /* +0x14 */ u32 command;
};

/* The panes a layout shows and hides when it is selected. size: 0xC4 */
struct HkbPaneSwitch {
    /* +0x00 */ u16 showNum;
    /* +0x02 */ u16 hideNum;
    /* +0x04 */ const char* show[24];
    /* +0x64 */ const char* hide[24];
};

/* One keyboard layout (.rodata 0x8057A690, eight languages, then the symbol layout 0x8057A730). size: 0x14 */
struct HkbKeyLayout {
    /* +0x00 */ const HkbQwertyKey* charKeys;
    /* +0x04 */ const HkbKanaKeyDef* kanaKeys;
    /* +0x08 */ const HkbCommandKeyDef* commandKeys;
    /* +0x0C */ const HkbPaneSwitch* paneSwitch;
    /* +0x10 */ const u16 (*accents)[8];
};

/* The record notification 0x1F fills: the current text (or the prediction language) and the prediction flag.
 * size: 0x8 */
struct HkbTextQuery {
    /* +0x0 */ u32 value;
    /* +0x4 */ u8 bFlag;
    /* +0x5 */ u8 pad_0x5[3];
};

/* The argument record of notification 0 (one typed character). size: 0x10 */
struct HkbCharEvent {
    /* +0x00 */ u16 c;
    /* +0x02 */ u16 unused_0x02;
    /* +0x04 */ u32 caseFlags;
    /* +0x08 */ u16 count;
    /* +0x0A */ u16 unused_0x0A;
    /* +0x0C */ u32 unused_0x0C;
};

/* The case argument of notification 0x12 (the input mode). size: 0x4 */
struct HkbModeEvent {
    /* +0x0 */ s32 mode;
};

/* The keyboard's case flags as bit fields: the one-shot shift, the caps lock and the input mode. size: 0x4 */
struct HkbCaseBits {
    /* +0x0 */ u32 unused_bits : 24;
    /* +0x0 */ u32 bShift : 1;
    /* +0x0 */ u32 bCaps : 1;
    /* +0x0 */ u32 unused_bits2 : 2;
    /* +0x0 */ u32 mode : 4;
};

/* The keyboard's case and kana state: the case flags, the half-width flag, the kana flags, the layout index and
 * the table it selects; every change is pushed to the keyboard that owns it.  size: 0x18 */
class HkbKeyState {
public:
    void update();
    void setupPanes(nw4r::lyt::Pane* pRoot);
    void setCaseFlags(u32 flags);
    s32 getMode();
    u16 getChar(u16 index);
    u16 findChar(const char* paneName);

    /* Returns the column of the kana key table the kana sub-mode selects. */
    inline s32 getKanaColumn();

    /* Returns the animation frame of the kana keys for the kana sub-mode. */
    s32 getKanaFrame() {
        switch (mKanaFlags & 0xF) {
        case 0:
            return 6;
        case 1:
            return 7;
        case 2:
            return 5;
        case 3:
            return 5;
        case 4:
            return 5;
        }
        return 6;
    }

    /* Clears the kana flags above the sub-mode, refreshing on a change. */
    void clearKanaFlags() {
        if (mKanaFlags & ~0xF) {
            mKanaFlags &= 0xF;
            update();
        }
    }

    /* Toggles the small-kana flag unless the kana sub-mode is 0 or 3, refreshing on a change. */
    void toggleSmall() {
        u32 mode = mKanaFlags & 0xF;
        if (mode != 3 && mode != 0) {
            mKanaFlags ^= 4;
            update();
        }
    }

    /* Returns the input mode bits (the low four case bits). */
    u32 getModeBits() const { return mFlags & 0xF; }

    /* Sets the input mode (the low four case bits), refreshing on a change. */
    void setMode(u32 mode) {
        if ((mFlags & 0xF) != mode) {
            mFlags = (mFlags & ~0xF) | (mode & 0xF);
            update();
        }
    }

    /* Sets the kana sub-mode (the low four kana bits), refreshing on a change. */
    void setKanaMode(u32 mode) {
        if ((mKanaFlags & 0xF) != mode) {
            mKanaFlags = (mKanaFlags & ~0xF) | (mode & 0xF);
            update();
        }
    }

    /* Returns the case flags above the input mode. */
    u32 getCaseFlags() const { return mFlags & ~0xF; }

    /* Sets or clears the one-shot shift, telling the keyboard on a change. */
    void setShiftFlag(u32 bOn) {
        union {
            u32 raw;
            HkbCaseBits bits;
        } flags;
        flags.raw = getCaseFlags();
        flags.bits.bShift = bOn;
        setCaseFlags(flags.raw);
    }

    /* Drops the one-shot shift when any case flag is set, refreshing. */
    void clearShift() {
        if (mFlags & ~0xF) {
            mBits.bShift = 0;
            update();
        }
    }

    /* Drops the caps lock when any case flag is set, refreshing. */
    void clearCaps() {
        if (mFlags & ~0xF) {
            mBits.bCaps = 0;
            update();
        }
    }

    /* Sets the full-width flag, refreshing on a change. */
    void setFullWidthInline(u32 bFullWidth) {
        if (bFullWidth != mbFullWidth) {
            mbFullWidth = bFullWidth;
            update();
        }
    }

    /* Toggles the voiced-mark flag unless the kana sub-mode is 3, refreshing on a change. */
    void toggleVoiced() {
        if ((mKanaFlags & 0xF) != 3) {
            mKanaFlags ^= 8;
            update();
        }
    }

    /* +0x00 */ union {
        u32 mFlags;
        HkbCaseBits mBits;
    };
    /* +0x04 */ u32 mbFullWidth;
    /* +0x08 */ u32 mKanaFlags;
    /* +0x0C */ u32 mLayoutIndex;
    /* +0x10 */ const HkbKeyLayout* mpLayout;
    /* +0x14 */ HkbKeyboardBase* mpOwner;
};

/* The keyboard's input status, handed out by address (getStatus). size: 0x8 */
struct HkbInputStatus {
    /* +0x0 */ u8 bInputDone;
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ u32 value;
};

class HbmPaneManager;

/* The software keyboard's common part: its event sink, its layout and case state and the input context it
 * reads; the slot order is the one the class table 0x8064DDE0 lists.  size: 0x4C */
class HkbKeyboardBase : public HkbInputWidget {
public:
    /* slot +0x008 */ virtual ~HkbKeyboardBase();
    /* slot +0x00C */ virtual void create(MEMAllocator* pAllocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x01C */ virtual void onCommand(u32 command, u32 arg);
    /* slot +0x020 */ virtual void onPaneEvent(u32 event, const char* paneName);
    /* slot +0x024 */ virtual s32 isBusy();
    /* slot +0x028 */ virtual void setLayoutType(s32 type);
    /* slot +0x034 */ virtual void updateView();
    /* slot +0x038 */ virtual void inputChar(u16 c);
    /* slot +0x03C */ virtual HkbInputStatus* getStatus();
    /* slot +0x040 */ virtual s32 isShift();
    /* slot +0x044 */ virtual s32 isCapsLock();
    /* slot +0x048 */ virtual s32 isHalfWidth();
    /* slot +0x04C */ virtual void setHalfWidth(u32 bHalfWidth);
    /* slot +0x050 */ virtual s32 getFocusKey();
    /* slot +0x054 */ virtual s32 getKanaAnimFrame();
    /* slot +0x058 */ virtual s32 getMode();
    /* slot +0x05C */ virtual void setMode(s32 mode);
    /* slot +0x060 */ virtual void setActive(bool bActive);
    /* slot +0x064 */ virtual void setModes(u32 bHalfWidth, u32 mode, u32 kanaMode);
    /* slot +0x068 */ virtual void onStateChanged();
    /* slot +0x06C */ virtual void setCaseLock(s32 lock);
    /* slot +0x070 */ virtual void setKanaLock(s32 lock);
    /* slot +0x074 */ virtual void close();
    /* slot +0x078 */ virtual u16 findCharByName(const char* paneName);
    /* slot +0x07C */ virtual u32 findCommandByName(const char* paneName);
    /* slot +0x080 */ virtual void inputChar(u16 c, u32 flags);
    /* slot +0x084 */ virtual void setCursor(u32 position);
    /* slot +0x088 */ virtual void submit();
    /* slot +0x08C */ virtual void updateEnterKey();

    void selectCursor(u32 position);

    /* +0x14 */ s32 mInputStyle;
    /* +0x18 */ s32 mPanelStyle;
    /* +0x1C */ HkbInputStatus mStatus;
    /* +0x24 */ const HkbKeyLayout* mpLayout;
    /* +0x28 */ HkbKeyState mKeyState;
    /* +0x40 */ MEMAllocator* mpAllocator;
    /* +0x44 */ u8 mbActive;
    /* +0x45 */ u8 pad_0x45[3];
    /* +0x48 */ HkbContextView* mpContext;
};

/* Returns the column of the kana key table the kana sub-mode selects. */
inline s32 HkbKeyState::getKanaColumn() {
    s32 column = 0;
    switch (mKanaFlags & 0xF) {
    case 0:
        column = 0;
        break;
    case 1:
        column = 0;
        break;
    case 2:
        column = 0;
        break;
    case 3:
        column = 0;
        break;
    case 5:
        column = 1;
        break;
    case 6:
        if (mpOwner->mpContext->v_0x078()->v_0x138() != 14) {
            column = 3;
        } else {
            column = 5;
        }
        break;
    case 8:
        column = 0;
        break;
    case 9:
        column = 0;
        break;
    case 13:
        column = 2;
        break;
    case 10:
        column = 0;
        break;
    case 14: {
        u8 panelKind = mpOwner->mpContext->v_0x078()->v_0x138();
        column = 6;
        if (panelKind != 14) {
            column = 4;
        }
        break;
    }
    }
    return column;
}

/* ---------------------------------------------------------------------------------------------------
 * The keyboard panel: the keyboard with its pane manager, its key components and its shift/caps buttons.
 * ------------------------------------------------------------------------------------------------ */

class HbmPaneManager;
class HkbKeyboard;

/* The last five keys pressed, oldest first.  size: 0x18 */
class HkbRecentKeys {
public:
    void clear();
    u32 getLast();
    void push(u32 key);

    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 mKeys[5];
};


class HbmPaneLayout;
class HbmComponent;


/* The sound player of the pane manager (its +0x14). size: 0x4 (approximation: only the vptr) */
class HbmSoundPlayer {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void play(s32 sound);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual void v_0x028();
    /* slot +0x02C */ virtual void setEnable(s32 bEnable);
};

class HbmEventHandler;

/* A component of the pane manager (a key or a button), as this file sees it. size: 0xA0 */
class HbmComponent {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void v_0x024();
    /* slot +0x028 */ virtual bool isPointed(u32 channel);
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
    /* slot +0x054 */ virtual void setTriggerTarget(s32 bTarget);

    void setTriggerFlag(u8 flag);

    /* +0x04 */ u8 unused_0x04[9];
    /* +0x0D */ bool mabPointed[4];
    /* +0x11 */ u8 unused_0x11[0x7F];
    /* +0x90 */ u8 mTriggerFlag;
    /* +0x91 */ u8 pad_0x91[7];
    /* +0x98 */ HbmEventHandler* mpEventHandler;
    /* +0x9C */ nw4r::lyt::Pane* mpPane;
};

/* A key component of the keyboard's component list, as the keyboard sees it (the key classes' table order).
 * size: 0x34 */
class HbmKeyView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void setAnim(s32 anim);
    /* slot +0x014 */ virtual void setState(u32 state);
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual s32 getState();
    /* slot +0x028 */ virtual s32 getType();
};


/* One animation definition of a key: its kind and the animation resource name. size: 0x44 */
struct HkbAnimDef {
    /* +0x00 */ u32 kind;
    /* +0x04 */ char name[64];
};

/* One key definition (.data 0x8064B180, 142 keys): the key kind, its pane, its animations and the group they
 * share. size: 0x40 */
struct HkbKeyDef {
    /* +0x00 */ u32 type;
    /* +0x04 */ const char* name;
    /* +0x08 */ u32 numAnims;
    /* +0x0C */ const char* group;
    /* +0x10 */ const HkbAnimDef* anims[12];
};

/* The receiver of submitted text (the keyboard's +0xFC). size: 0x4 (approximation: only the vptr) */
class HkbTextSink {
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
    /* slot +0x0A8 */ virtual void submitText(u32 text, HkbKeyboard* pKeyboard);
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
    /* slot +0x0D4 */ virtual void moveCursor(HkbKeyboard* pKeyboard, u32 flags, u32 position);
};

/* The keyboard's third base (at +0xF4): the receiver of the pane sounds. size: 0x4 */
class HbmSoundListener {
public:
    /* slot +0x008 */ virtual void onSoundEvent(u32 id, u32 event, u32 sound);
};

/* The trigger record a pane event carries. size: 0x14 */
struct HbmPaneTrigger {
    /* +0x00 */ u32 channel;
    /* +0x04 */ u32 unused_0x04[2];
    /* +0x0C */ u32 trigger;
    /* +0x10 */ u32 hold;
};

/* The pane manager's event handler (homebutton/hbm_value.cpp): its sound player.  size: 0xC */
class HbmPaneHandler : public HbmEventHandler {
public:
    /* slot +0x008 */ virtual ~HbmPaneHandler() {}
    /* untyped: caller-owned payload - the argument record of the event */
    /* slot +0x00C */ virtual void onEvent(u32 id, u32 event, void* data);
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);
    /* slot +0x01C */ virtual void setSound(HbmSoundPlayer* pSound);

    /* +0x08 */ HbmSoundPlayer* mpSound;
};

/* The keyboard's pane handler: presses on the key panes reach the keyboard; the class table is 0x8064DDC0.
 * size: 0x10 */
class HkbKeyboardPaneHandler : public HbmPaneHandler {
public:
    HkbKeyboardPaneHandler(HkbKeyboard* pKeyboard) : mpKeyboard(pKeyboard) {}
    /* slot +0x008 */ virtual ~HkbKeyboardPaneHandler();
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);

    /* +0x0C */ HkbKeyboard* mpKeyboard;
};

/* An event handler that hands the manager's events on as pane events. The keyboard file emits its table
 * (0x8064DDA4).  size: 0x8 */
class HbmPaneEventHandler : public HbmEventHandler {
public:
    /* slot +0x008 */ virtual ~HbmPaneEventHandler();
    /* untyped: caller-owned payload - the argument record of the event */
    /* slot +0x00C */ virtual void onEvent(u32 id, u32 event, void* data);
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);
};

/* The keyboard's shift or caps button: its key, base and animation components and whether it is lit; the
 * keyboard file emits its table (0x8064DD88).  size: 0x24 */
class HkbKeyButton : public HbmPaneEventHandler {
public:
    HkbKeyButton(u32 type, HkbKeyboard* pKeyboard, HbmSoundListener* pSound);
    /* slot +0x008 */ virtual ~HkbKeyButton();
    /* slot +0x018 */ virtual void onPaneEvent(HbmComponent* pComponent, u32 event, HbmPaneTrigger* pTrigger);

    /* Lights the button or puts it out, playing the matching frame. */
    void setLit(u8 bOn) {
        mbOn = bOn;
        if (mbOn) {
            mpAnim->setAnim(10);
        } else {
            mpAnim->setAnim(11);
        }
    }

    /* Finds the button's key and base components and its animation in `pManager`, and takes the base's events. */
    void attach(HbmPaneManager* pManager, const char* keyName, const char* baseName);

    /* +0x08 */ u32 mType;
    /* +0x0C */ HkbKeyboard* mpKeyboard;
    /* +0x10 */ HbmSoundListener* mpSound;
    /* +0x14 */ HbmComponent* mpKey;
    /* +0x18 */ HbmComponent* mpBase;
    /* +0x1C */ HbmKey* mpAnim;
    /* +0x20 */ u8 mbOn;
    /* +0x21 */ u8 pad_0x21[3];
};

/* The software keyboard panel: the keyboard, its pane manager and its sound listener; the slot order is the one
 * the class table 0x8064DC30 lists.  size: 0x150 */
class HkbKeyboard : public HkbKeyboardBase, public HbmPaneManager, public HbmSoundListener {
public:
    /* slot +0x008 */ virtual ~HkbKeyboard();
    /* slot +0x00C */ virtual void create(MEMAllocator* pAllocator);
    /* slot +0x010 */ virtual void init();
    /* slot +0x01C */ virtual void onCommand(u32 command, u32 arg);
    /* slot +0x020 */ virtual void onPaneEvent(u32 event, const char* paneName);
    /* slot +0x028 */ virtual void setLayoutType(s32 type);
    /* slot +0x034 */ virtual void updateView();
    /* slot +0x038 */ virtual void inputChar(u16 c);
    /* slot +0x04C */ virtual void setHalfWidth(u32 bHalfWidth);
    /* slot +0x05C */ virtual void setMode(s32 mode);
    /* slot +0x060 */ virtual void setActive(bool bActive);
    /* slot +0x064 */ virtual void setModes(u32 bHalfWidth, u32 mode, u32 kanaMode);
    /* slot +0x068 */ virtual void onStateChanged();
    /* slot +0x074 */ virtual void close();
    /* slot +0x080 */ virtual void inputChar(u16 c, u32 flags);
    /* slot +0x084 */ virtual void setCursor(u32 position);
    /* slot +0x088 */ virtual void submit();
    /* slot +0x110 */ virtual void calcComponents();
    /* slot +0x114 */ virtual void drawComponents();
    /* slot +0x118 */ virtual void setTextSink(HkbTextSink* pSink);
    /* slot +0x11C */ virtual void setCursorSink(HkbTextSink* pSink);
    /* slot +0x120 */ virtual void setEnterKeyVisible(u32 bVisible);
    /* slot +0x124 */ virtual void hideLineFeed();
    /* slot +0x128 */ virtual void setLanguageKeys(s32 bVisible);
    /* slot +0x12C */ virtual u32 updateComponents();
    /* slot +0x130 */ virtual s32 onKeyRelease(HkbManager* pManager);
    /* slot +0x134 */ virtual void setPanelKind(u8 kind);
    /* slot +0x138 */ virtual u8 getPanelKind();
    /* slot +0x13C */ virtual void resetKeys();
    /* slot +0x140 */ virtual void releaseKeys();
    /* slot +0x144 */ virtual void refreshKeys();
    /* slot +0x148 */ virtual void updatePrediction(HkbTextQuery* pQuery);
    /* slot +0x14C */ virtual void updateKanaKeys();
    /* slot +0x150 */ virtual void updateModePanes();
    /* slot +0x154 */ virtual void onSoundEvent(u32 id, u32 event, u32 sound);

    void createKeys(MEMAllocator* pAllocator);
    s32 isCompactPanel();


    void toggleCapsLock();
    void switchCapsLock();
    void setShiftOn(s32 bSound);
    void setShiftOff();

    /* +0x0F8 */ HbmEventHandler* mpPaneHandler;
    /* +0x0FC */ HkbTextSink* mpPaneSink;
    /* +0x100 */ HkbTextSink* mpCursorSink;
    /* +0x104 */ u8 mbEnterKeyVisible;
    /* +0x105 */ u8 mPanelKind;
    /* +0x106 */ u8 pad_0x106[2];
    /* +0x108 */ HkbKeyButton mShiftButton;
    /* +0x12C */ HkbKeyButton mCapsButton;
};

/* An animation entry of a key's animation list. size: 0x8 (approximation: the members this file reads) */
struct HbmKeyAnim {
    /* +0x0 */ u32 unused_0x00;
    /* +0x4 */ u32 kind;
};

/* A key of the software keyboard: a band key with its kind; the class table is 0x8064DC04.  size: 0x34 */
class HkbKey : public HbmKey {
public:
    HkbKey(nw4r::lyt::Pane* pText) : HbmKey(pText) {}

    /* slot +0x008 */ virtual void reset();
    /* slot +0x014 */ virtual void setState(u32 state);
    /* slot +0x020 */ virtual ~HkbKey();
    /* slot +0x024 */ virtual s32 getState();
    /* slot +0x028 */ virtual s32 getType();

    s32 isLit();

    /* +0x30 */ u32 mType;
};

/* A character key (kind 0); the class table is 0x8064DBD8. size: 0x34 */
class HkbCharKey : public HkbKey {
public:
    HkbCharKey(nw4r::lyt::Pane* pText) : HkbKey(pText) { mType = 0; }
    /* slot +0x010 */ virtual void setAnim(s32 anim);
    /* slot +0x020 */ virtual ~HkbCharKey();
};

/* A kana key (kind 1); the class table is 0x8064DBAC. size: 0x38 */
class HkbKanaKey : public HkbKey {
public:
    HkbKanaKey(nw4r::lyt::Pane* pText) : HkbKey(pText), mbHeld(0) { mType = 1; }
    /* slot +0x010 */ virtual void setAnim(s32 anim);
    /* slot +0x020 */ virtual ~HkbKanaKey();

    /* +0x34 */ u8 mbHeld;
    /* +0x35 */ u8 pad_0x35[3];
};

/* A function key (kind 2); the class table is 0x8064DB80. size: 0x34 */
class HkbFuncKey : public HkbKey {
public:
    HkbFuncKey(nw4r::lyt::Pane* pText) : HkbKey(pText) { mType = 2; }
    /* slot +0x010 */ virtual void setAnim(s32 anim);
    /* slot +0x020 */ virtual ~HkbFuncKey();
};

/* A space key (kind 3); the class table is 0x8064DB54. size: 0x34 */
class HkbSpaceKey : public HkbKey {
public:
    HkbSpaceKey(nw4r::lyt::Pane* pText) : HkbKey(pText) { mType = 3; }
    /* slot +0x010 */ virtual void setAnim(s32 anim);
    /* slot +0x020 */ virtual ~HkbSpaceKey();
};


inline void HkbKeyButton::attach(HbmPaneManager* pManager, const char* keyName, const char* baseName) {
    HbmPaneLayout* pRoot = pManager->getRoot();
    mpKey = (HbmComponent*)hkbGetPaneComponentByName((PaneManager*)pRoot, keyName);
    mpBase = (HbmComponent*)hkbGetPaneComponentByName((PaneManager*)pRoot, baseName);
    mpAnim = pManager->getPaneAnim(keyName);
    mpBase->mpEventHandler = this;
}
#endif

#endif
