/*
 * menu/menu_sysmsg.h - the types of `menu/menu_sysmsg.cpp` (the game's software keyboard driver over the homebutton
 *   band's keyboard object).
 *   The view classes name the slots the driver calls on the band's objects; the slots whose bodies are inline in
 *   the band's headers are defined here, so the driver's calls emit their out-of-line copies as retail does.
 */
#ifndef MHTRI_MENU_MENU_SYSMSG_H
#define MHTRI_MENU_MENU_SYSMSG_H

#include "types.h"
#include "menu/sysmsg_text.h" /* the message tables this header's readers use (`menu/sysmsg_text.cpp` owns them) */

#ifdef __cplusplus

struct HkbManager;
struct MEMAllocator;
class HkbKeyboardBase;
class HbmAnimPanel;
class SysKbdHandlerBase;
class SysKbdPart;
class SysKbdLayoutPart;
class SysKbdSlidePart;
class SysKbdTextPart;
class SysKbdPredictPart;
class SysKbdCursorPart;
class HbmGuiPaneWidget;
namespace nw4r {
namespace lyt {
class ResourceAccessor;
}
}

/* The keyboard event the HKB manager hands its listeners: the HID usage code of the key, whether it went down (bit 0)
 * and the modifier word at the time.  size: 0xC */
struct SysKbdKeyEvent {
    /* +0x0 */ u8 channel;
    /* +0x1 */ u8 usage;
    /* +0x2 */ u8 pad_0x2[0x2];
    /* +0x4 */ s32 type;        /* 1: pressed; bit 0 set: down */
    /* +0x8 */ s32 modifiers;
};

/* The request record the driver fills from the USB keyboard and the pad (`.bss` 0x806F4AE0): the key and modifiers
 * of the last key event, the command the keyboard reads, the input mode and the kind of text asked for.
 * size: 0x14 */
struct SysKbdRequest {
    /* +0x00 */ u8 key_held;
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 key;         /* the HID usage code of the last key pressed */
    /* +0x03 */ u8 pad_0x03[0x2];
    /* +0x05 */ u8 modifiers;   /* the low byte of the modifier word */
    /* +0x06 */ u8 pad_0x06;
    /* +0x07 */ u8 input_mode;  /* the keyboard panel's input mode */
    /* +0x08 */ u8 gesture_key; /* 61: the gesture key was asked for while the panel was closed */
    /* +0x09 */ u8 flags;       /* bit 0: a key event is wanted, bit 1: one arrived */
    /* +0x0A */ u8 command;     /* the usage code the step acts on */
    /* +0x0B */ u8 command_kind; /* 1: a function key (58..69) with only the shift modifier */
    /* +0x0C */ u8 mode;        /* 0: chat, 2: the gesture-name entry, 3: no candidate list, 4: name entry */
    /* +0x0D */ u8 kind;        /* the text kind the caller asked for */
    /* +0x0E */ u8 gesture_open; /* 1: the gesture list is open */
    /* +0x0F */ u8 pad_0x0F[0x5];
};

/* The settings word: the id, two zero bytes and the panel flags.  size: 0x4 */
struct SysKbdSettingFlags {
    /* +0x0 */ u8 id;           /* 0xFA */
    /* +0x1 */ u8 field_0x1;
    /* +0x2 */ u8 field_0x2;
    /* +0x3 */ u8 enabled : 1;
    /* +0x3 */ u8 layout : 4;
    /* +0x3 */ u8 kana : 1;
    /* +0x3 */ u8 caps : 1;
    /* +0x3 */ u8 shift : 1;
};

/* The settings half-word: the panel and style nibbles and a zero byte.  size: 0x2 */
struct SysKbdSettingStyle {
    /* +0x0 */ u8 panel : 4;
    /* +0x0 */ u8 style : 4;
    /* +0x1 */ u8 field_0x1;
};

/* The keyboard object's settings (handed to slot +0xD8 by address and returned from slot +0xE0 in r3:r4): a byte, the
 * flags word and the style half-word unaligned behind it, and a byte.  size: 0x8 */
struct SysKbdSetting {
    /* +0x0 */ u8 version;      /* 0x11 */
    /* +0x1 */ SysKbdSettingFlags flags;
    /* +0x5 */ SysKbdSettingStyle style;
    /* +0x7 */ u8 field_0x7;
};

/* The text panel's mode object (slot +0xE0 of the text panel): slot +0xD8 is its input mode, +0xE4 whether it shows
 * the half-width set.  size: 0x4 (approximation: only the vptr is touched) */
class SysKbdModeView {
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
    /* Returns the panel's input mode (the base object has the one mode). */
    /* slot +0x0D8 */ virtual u32 getInputMode() { return 1; }
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* slot +0x0E0 */ virtual void v_0x0E0();
    /* Returns whether the panel shows the half-width set (the base object never does). */
    /* slot +0x0E4 */ virtual u32 isHalfWidth() { return 0; }
};

/* The text panel's animation (its +0x1964): whether it plays (+0x14) and its frame (+0xC).  size: 0x18 */
class SysKbdPanelAnim {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* Returns whether the animation plays. */
    /* slot +0x014 */ virtual s32 isPlaying() { return mPlaying; }
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* Stops the animation and rewinds it. */
    /* slot +0x020 */ virtual void stop() {
        mPlaying = 0;
        mFrame = 0.0f;
    }

    /* +0x04 */ u8 unused_0x04[0x8];
    /* +0x0C */ f32 mFrame;
    /* +0x10 */ u8 unused_0x10[0x4];
    /* +0x14 */ u8 mPlaying;
    /* +0x15 */ u8 pad_0x15[0x3];
};

/* The text box sub-object at the text panel's +0x1A04 (the band's `subi r3,r3,0x1A04` thunks): slot +0x2C plays the
 * named animation.  size: 0x4 (approximation: only the vptr is touched) */
class SysKbdPanelTitle {
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
    /* slot +0x02C */ virtual void playAnim(const char* name);
};

/* The text writer's character-writer part (the 0x4C bytes in front of its vptr).  size: 0x4C */
struct SysKbdCharWriter {
    /* +0x00 */ u8 unused_0x00[0x4C];
};

/* The text panel's writer (its +0x10, vptr at +0x4C): slot +0x3C sets its candidate mode byte (+0xC8).
 * size: 0xCC (approximation: the members reached) */
class SysKbdWriter : public SysKbdCharWriter {
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
    /* Sets the writer's candidate mode. */
    /* slot +0x03C */ virtual void setCandidateMode(bool mode) { mCandidateMode = mode; }

    /* +0x50 */ u8 unused_0x50[0x78];
    /* +0xC8 */ bool mCandidateMode;
    /* +0xC9 */ u8 pad_0xC9[0x3];
};

/* The keyboard's text panel (slot +0x68 of the keyboard object; the band's `HbmTextPanel`): its writer, its mode
 * object (+0x1940), its animation (+0x1964), the modifier word it reads (+0x19B8) and its title box (+0x1A04).
 * size: 0x1A08 (approximation: the members reached) */
class SysKbdPanelView {
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
    /* Returns the panel's mode object. */
    /* slot +0x0E0 */ virtual SysKbdModeView* getModeView() { return mModeView; }
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
    /* Returns whether the panel's animation plays. */
    /* slot +0x124 */ virtual s32 isAnimating() { return mAnim.isPlaying(); }
    /* Stops the panel's animation when it plays. */
    /* slot +0x128 */ virtual void close() {
        if (isAnimating()) {
            mAnim.stop();
        }
    }

    /* Hands the panel the modifier bits of the USB keyboard. */
    void setModifiers(u32 modifiers) { mModifiers = modifiers; }

    /* +0x0004 */ u8 unused_0x0004[0xC];
    /* +0x0010 */ SysKbdWriter mWriter;
    /* +0x00DC */ u8 unused_0x00DC[0x1864];
    /* +0x1940 */ SysKbdModeView* mModeView;
    /* +0x1944 */ u8 unused_0x1944[0x20];
    /* +0x1964 */ SysKbdPanelAnim mAnim;
    /* +0x197C */ u8 unused_0x197C[0x3C];
    /* +0x19B8 */ u32 mModifiers;
    /* +0x19BC */ u8 unused_0x19BC[0x48];
    /* +0x1A04 */ SysKbdPanelTitle mTitle;
};

/* What the keyboard object's slot +0x70 returns: slot +0x24 feeds it a key of the HKB manager.  size: 0x4
 * (approximation: only the vptr is touched) */
class SysKbdInputView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual void v_0x00C();
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void v_0x014();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void v_0x020();
    /* slot +0x024 */ virtual void inputKey(HkbManager* manager, u8 usage, s32 repeat);
};

/* What the keyboard object's slot +0x98 returns: slot +0xA0 cancels the input.  size: 0x4 (approximation) */
class SysKbdCancelView {
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
    /* slot +0x0A0 */ virtual void cancel();
};

/* What the keyboard object's slot +0xE8 returns: slot +0x0C is its state (2: the input finished).
 * size: 0x4 (approximation) */
class SysKbdStateView {
public:
    /* slot +0x008 */ virtual void v_0x008();
    /* slot +0x00C */ virtual s32 getState();
};

/* The homebutton band's Vu object the keyboard object derives from (its constructor is homebutton/hbm_value.cpp's
 * 0x8055CAD0, table 0x80651950): the parts the keyboard object hands out.  size: 0x40 */
class SysKbdVuObject {
public:
    SysKbdVuObject(MEMAllocator* allocator, nw4r::lyt::ResourceAccessor* resources, SysKbdHandlerBase* handler);
    /* slot +0x008 */ virtual ~SysKbdVuObject();

    /* +0x04 */ MEMAllocator* mAllocator;
    /* +0x08 */ u8 unused_0x08[0x8];
    /* +0x10 */ SysKbdInputView* mInput;
    /* +0x14 */ HkbKeyboardBase* mSoftKeyboard;
    /* +0x18 */ SysKbdSlidePart* mSlidePart;
    /* +0x1C */ SysKbdPanelView* mPanel;
    /* +0x20 */ SysKbdPredictPart* mPredictPart;
    /* +0x24 */ SysKbdCancelView* mCanceller;
    /* +0x28 */ SysKbdPart* mAuxPart;
    /* +0x2C */ HbmAnimPanel* mList;
    /* +0x30 */ u8 unused_0x30[0x10];
};

/* The homebutton band's keyboard object the driver creates (class table 0x80657A20 in homebutton/hbm_kb_event.cpp's
 * `.data`): the slots the driver calls, named from what it passes and does with the result; the rest by offset.
 * size: 0x7C (the driver's `new`) */
class SysKbdView : public SysKbdVuObject {
public:
    SysKbdView(MEMAllocator* allocator, nw4r::lyt::ResourceAccessor* resources, SysKbdHandlerBase* handler)
        : SysKbdVuObject(allocator, resources, handler)
    {
        mStateView = 0;
        mStateFlag = 1;
        field_0x50 = 0;
        field_0x54 = 0;
        field_0x58 = 0;
        field_0x5C = 0;
        field_0x60 = 0;
        field_0x64 = 0;
        field_0x68 = 0;
        field_0x6C = 0;
        field_0x70 = 0;
        field_0x74 = 0;
        field_0x78 = 0;
    }

    /* slot +0x00C */ virtual void setVisible(s32 visible);
    /* slot +0x010 */ virtual void v_0x010();
    /* slot +0x014 */ virtual void calc();
    /* slot +0x018 */ virtual void draw();
    /* slot +0x01C */ virtual void inputPointer(s32 channel, f32 x, f32 y, u16 held, u16 pressed, u16 released);
    /* slot +0x020 */ virtual void inputKeyboard(HkbManager* manager);
    /* slot +0x024 */ virtual u16* getText();
    /* slot +0x028 */ virtual void setText(const u16* text);
    /* slot +0x02C */ virtual void setTitle(const u16* title);
    /* slot +0x030 */ virtual void setLanguage(s32 language);
    /* slot +0x034 */ virtual void v_0x034();
    /* slot +0x038 */ virtual void setInputStyle(s32 style);
    /* slot +0x03C */ virtual void setMaxLength(s32 length);
    /* slot +0x040 */ virtual void setMaxBytes(s32 length);
    /* slot +0x044 */ virtual void setLineCount(s32 lines);
    /* slot +0x048 */ virtual void v_0x048();
    /* slot +0x04C */ virtual void setStandardScreen(s32 standard);
    /* Sets the text panel writer's candidate mode. */
    /* slot +0x050 */ virtual void setCandidateMode(bool mode) { mPanel->mWriter.setCandidateMode(mode); }
    /* slot +0x054 */ virtual void v_0x054();
    /* slot +0x058 */ virtual void v_0x058();
    /* slot +0x05C */ virtual void v_0x05C();
    /* slot +0x060 */ virtual void v_0x060();
    /* slot +0x064 */ virtual void v_0x064();
    /* Returns the text panel. */
    /* slot +0x068 */ virtual SysKbdPanelView* getPanel() { return mPanel; }
    /* slot +0x06C */ virtual void v_0x06C();
    /* Returns the key input object. */
    /* slot +0x070 */ virtual SysKbdInputView* getInput() { return mInput; }
    /* slot +0x074 */ virtual void v_0x074();
    /* Returns the soft keyboard. */
    /* slot +0x078 */ virtual HkbKeyboardBase* getSoftKeyboard() { return mSoftKeyboard; }
    /* slot +0x07C */ virtual void v_0x07C();
    /* slot +0x080 */ virtual void v_0x080();
    /* slot +0x084 */ virtual void v_0x084();
    /* Returns the candidate list. */
    /* slot +0x088 */ virtual HbmAnimPanel* getList() { return mList; }
    /* slot +0x08C */ virtual void v_0x08C();
    /* slot +0x090 */ virtual void v_0x090();
    /* slot +0x094 */ virtual void v_0x094();
    /* Returns the object that cancels the input. */
    /* slot +0x098 */ virtual SysKbdCancelView* getCanceller() { return mCanceller; }
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
    /* Takes the settings and re-opens the panel with them. */
    /* slot +0x0D8 */ virtual void setSetting(const SysKbdSetting* setting) {
        mSetting = *setting;
        open();
    }
    /* slot +0x0DC */ virtual void v_0x0DC();
    /* Returns the settings after the panel stored its state in them. */
    /* slot +0x0E0 */ virtual SysKbdSetting getSetting() {
        storeSetting();
        return mSetting;
    }
    /* slot +0x0E4 */ virtual void setActive(s32 active);
    /* Returns the state object. */
    /* slot +0x0E8 */ virtual SysKbdStateView* getStateView() { return mStateView; }
    /* slot +0x0EC */ virtual void v_0x0EC();
    /* slot +0x0F0 */ virtual void v_0x0F0();
    /* slot +0x0F4 */ virtual void open();
    /* slot +0x0F8 */ virtual void storeSetting();
    /* slot +0x0FC */ virtual void setOffsetY(f32 y);
    /* slot +0x100 */ virtual void v_0x100();
    /* slot +0x104 */ virtual void setNameEntry(s32 on);
    /* slot +0x108 */ virtual void setGestureEntry(s32 on);
    /* slot +0x008, SysKbdVuObject's: declared last so that it is not the class's first non-inline virtual (the
     * destructor is homebutton/hbm_kb_event.cpp's, the class table 0x80657A20 is not emitted from it) */
    virtual ~SysKbdView();

    /* +0x40 */ SysKbdStateView* mStateView;
    /* +0x44 */ s32 mStateFlag;
    /* +0x48 */ SysKbdSetting mSetting;
    /* +0x50 */ SysKbdPart* field_0x50;
    /* +0x54 */ SysKbdPart* field_0x54;
    /* +0x58 */ SysKbdTextPart* field_0x58;
    /* +0x5C */ SysKbdLayoutPart* field_0x5C;
    /* +0x60 */ SysKbdSlidePart* field_0x60;
    /* +0x64 */ SysKbdTextPart* field_0x64;
    /* +0x68 */ SysKbdPredictPart* field_0x68;
    /* +0x6C */ SysKbdCursorPart* field_0x6C;
    /* +0x70 */ SysKbdPart* field_0x70;
    /* +0x74 */ SysKbdPart* field_0x74;
    /* +0x78 */ HbmGuiPaneWidget* field_0x78;
};

#endif /* __cplusplus */

#endif /* MHTRI_MENU_MENU_SYSMSG_H */
