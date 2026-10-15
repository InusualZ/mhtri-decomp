/*
 * homebutton/hkb_cellphone.h - the cellphone-keypad classes of `homebutton/hkb_cellphone.cpp`: the multi-tap input
 * HkbCellphoneInput, the keypad screen HkbCellphoneKeypad, its key and tag buttons and its pane listener, with the
 * views of the band objects they call (named by slot offset where the slot is not understood); the bases are the
 * band's HkbInputWidget, HbmPaneManager, HbmKey and HbmPaneHandler (homebutton/hkb_keyboard.h, homebutton/hbm_value.h).
 */
#ifndef HOMEBUTTON_HKB_CELLPHONE_H
#define HOMEBUTTON_HKB_CELLPHONE_H

#include "types.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "homebutton/hkb_cellphone_keys.h"

#ifdef __cplusplus

#include "nw4r/fn_805012C4.h"
#include "homebutton/hkb_layout.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hkb_drawinfo.h"
#include "homebutton/hkb_resource_accessor.h"
#include "homebutton/tiHKBManager.h"

/* --------------------------------------------------------------------------------------------- */
/* Records                                                                                         */
/* --------------------------------------------------------------------------------------------- */

/* One language's modes in cycling order (the letter modes, then the numeric one). size: 0x18 */
struct HkbCellphoneLanguage {
    u16 mode_count;                       /* +0x00 */
    u16 pad_0x02;                         /* +0x02 */
    const HkbCellphoneMode* modes[5];     /* +0x04 */
};

/* A button pane and the action it triggers. size: 0x18 */
struct HkbCellphoneButton {
    char name[20];                        /* +0x00 */
    s32 action;                           /* +0x14 */
};

/* A pane animation and the button state it plays for. size: 0x44 */
struct HkbCellphoneAnim {
    u32 state;                            /* +0x00 */
    char name[64];                        /* +0x04 */
};

/* A window pane and the radio group it belongs to. size: 0x18 */
struct HkbCellphoneWindow {
    char name[20];                        /* +0x00 */
    const char* group;                    /* +0x14 */
};

/* The pane event the keypad receives: the pane's name, whether the press cycles backwards and whether it is a
 * long press. size: 0x8 */
struct HkbPaneEvent {
    const char* pane_name;                /* +0x00 */
    u8 backward;                          /* +0x04 */
    u8 long_press;                        /* +0x05 */
    u8 pad_0x06[2];                       /* +0x06 */
};

/* The pad state a pane event carries: the channel and the trigger and hold bits. size: 0x14 */
struct HkbPadState {
    u32 channel;                          /* +0x00 */
    u8 pad_0x04[8];                       /* +0x04 */
    u32 trigger;                          /* +0x0C */
    u32 hold;                             /* +0x10 */
};

/* The character record the keypad posts (ids 3 and 5): the character, the mode it was typed in, the commit
 * flags and the key it came from. size: 0x10 */
struct HkbCellphoneCharEvent {
    u16 ch;                               /* +0x00 */
    s16 mode;                             /* +0x02 */
    u8 replace;                           /* +0x04 */
    u8 confirmed;                         /* +0x05 */
    u8 commit;                            /* +0x06 */
    u8 pad_0x07;                          /* +0x07 */
    const HkbCellphoneKey* key;           /* +0x08 */
    u32 pad_0x0C;                         /* +0x0C */
};

/* The predictive-input state (queried with id 0x1F, set with id 0x1D): the dictionary language and whether
 * prediction is on. size: 0x8 */
struct HkbPredictState {
    s32 language;                         /* +0x00 */
    u8 on;                                /* +0x04 */
    u8 pad_0x05[3];                       /* +0x05 */
};

/* The key-code record the keypad posts (id 0): the code and the repeat count. size: 0x10 */
struct HkbKeyEvent {
    u16 code;                             /* +0x00 */
    u16 pad_0x02;                         /* +0x02 */
    u32 pad_0x04;                         /* +0x04 */
    u16 count;                            /* +0x08 */
    u16 pad_0x0A;                         /* +0x0A */
    u32 pad_0x0C;                         /* +0x0C */
};

/* A one-byte switch payload (ids 0x13, 0x14, 0x20, 0x22). size: 0x1 */
struct HkbSwitchArg {
    u8 on;                                /* +0x00 */
};

/* --------------------------------------------------------------------------------------------- */
/* Views of layout objects this unit only calls                                                    */
/* --------------------------------------------------------------------------------------------- */

/* The keyboard's input switch (HkbKeyboardView::getInput). size: 0x4 (view) */
class HkbKeyboardInput {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual BOOL isActive();              /* +0x10 */
};

/* The keyboard's predictive-input state (HkbKeyboardView::getPredictor). size: 0x4 (view) */
class HkbKeyboardPredictor {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual void v_0x10();
    virtual void v_0x14();
    virtual void v_0x18();
    virtual void v_0x1C();
    virtual void v_0x20();
    virtual void v_0x24();
    virtual void v_0x28();
    virtual void v_0x2C();
    virtual void v_0x30();
    virtual void v_0x34();
    virtual void v_0x38();
    virtual void v_0x3C();
    virtual void v_0x40();
    virtual void v_0x44();
    virtual BOOL isOpen();                /* +0x48 */
    virtual void setOpen(u8 on);          /* +0x4C */
};

/* The keyboard's conversion state (HkbKeyboardView::getConverter). size: 0x4 (view) */
class HkbKeyboardConverter {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual void v_0x10();
    virtual void v_0x14();
    virtual void v_0x18();
    virtual void v_0x1C();
    virtual void v_0x20();
    virtual void v_0x24();
    virtual void v_0x28();
    virtual void v_0x2C();
    virtual void v_0x30();
    virtual void v_0x34();
    virtual void v_0x38();
    virtual void v_0x3C();
    virtual void v_0x40();
    virtual void v_0x44();
    virtual void v_0x48();
    virtual void v_0x4C();
    virtual void v_0x50();
    virtual void v_0x54();
    virtual void v_0x58();
    virtual void v_0x5C();
    virtual void v_0x60();
    virtual void v_0x64();
    virtual void v_0x68();
    virtual void v_0x6C();
    virtual void v_0x70();
    virtual void v_0x74();
    virtual void v_0x78();
    virtual void v_0x7C();
    virtual void v_0x80();
    virtual void v_0x84();
    virtual void v_0x88();
    virtual void v_0x8C();
    virtual void v_0x90();
    virtual void v_0x94();
    virtual void v_0x98();
    virtual void v_0x9C();
    virtual void v_0xA0();
    virtual void setBusy(u8 on);          /* +0xA4 */
    virtual BOOL isBusy();                /* +0xA8 */
};

/* The keyboard's composition state (HkbKeyboardView::getComposer). size: 0x4 (view) */
class HkbKeyboardComposer {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual void v_0x10();
    virtual void v_0x14();
    virtual void v_0x18();
    virtual void v_0x1C();
    virtual void v_0x20();
    virtual void v_0x24();
    virtual void v_0x28();
    virtual void v_0x2C();
    virtual void v_0x30();
    virtual void v_0x34();
    virtual void v_0x38();
    virtual void v_0x3C();
    virtual void v_0x40();
    virtual void v_0x44();
    virtual void v_0x48();
    virtual void v_0x4C();
    virtual void v_0x50();
    virtual void v_0x54();
    virtual void v_0x58();
    virtual void v_0x5C();
    virtual void v_0x60();
    virtual void v_0x64();
    virtual void v_0x68();
    virtual void v_0x6C();
    virtual void v_0x70();
    virtual void v_0x74();
    virtual void v_0x78();
    virtual void v_0x7C();
    virtual void v_0x80();
    virtual void v_0x84();
    virtual void v_0x88();
    virtual void v_0x8C();
    virtual void v_0x90();
    virtual void v_0x94();
    virtual void v_0x98();
    virtual void v_0x9C();
    virtual void v_0xA0();
    virtual void v_0xA4();
    virtual void v_0xA8();
    virtual void v_0xAC();
    virtual void v_0xB0();
    virtual void v_0xB4();
    virtual void v_0xB8();
    virtual void v_0xBC();
    virtual void v_0xC0();
    virtual void v_0xC4();
    virtual void v_0xC8();
    virtual void v_0xCC();
    virtual void v_0xD0();
    virtual void v_0xD4();
    virtual void v_0xD8();
    virtual void v_0xDC();
    virtual void v_0xE0();
    virtual void v_0xE4();
    virtual void v_0xE8();
    virtual void v_0xEC();
    virtual void v_0xF0();
    virtual void v_0xF4();
    virtual void v_0xF8();
    virtual void v_0xFC();
    virtual void v_0x100();
    virtual void v_0x104();
    virtual void v_0x108();
    virtual void v_0x10C();
    virtual void v_0x110();
    virtual void v_0x114();
    virtual void v_0x118();
    virtual void v_0x11C();
    virtual void v_0x120();
    virtual void v_0x124();
    virtual void v_0x128();
    virtual void v_0x12C();
    virtual void v_0x130();
    virtual void v_0x134();
    virtual void v_0x138();
    virtual void v_0x13C();
    virtual BOOL isConverting();          /* +0x140 */
};

/* The keyboard object the input reports to (+0x30); the slots it never calls are named by offset. size: 0x4 */
class HkbKeyboardView {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual void v_0x10();
    virtual void v_0x14();
    virtual void v_0x18();
    virtual void v_0x1C();
    virtual void v_0x20();
    virtual void v_0x24();
    virtual void v_0x28();
    virtual void v_0x2C();
    virtual void v_0x30();
    virtual void v_0x34();
    virtual void v_0x38();
    virtual void v_0x3C();
    virtual void v_0x40();
    virtual void v_0x44();
    virtual void v_0x48();
    virtual void v_0x4C();
    virtual void v_0x50();
    virtual void v_0x54();
    virtual void v_0x58();
    virtual void v_0x5C();
    virtual void v_0x60();
    virtual void v_0x64();
    virtual HkbKeyboardComposer* getComposer();       /* +0x68 */
    virtual void v_0x6C();
    virtual void v_0x70();
    virtual void v_0x74();
    virtual HkbKeyboardPredictor* getPredictor();     /* +0x78 */
    virtual void v_0x7C();
    virtual void v_0x80();
    virtual void v_0x84();
    virtual void v_0x88();
    virtual void v_0x8C();
    virtual void v_0x90();
    virtual void v_0x94();
    virtual HkbKeyboardInput* getInput();             /* +0x98 */
};

/* --------------------------------------------------------------------------------------------- */
/* HkbCellphoneInput                                                                               */
/* --------------------------------------------------------------------------------------------- */

/* The cellphone keypad's multi-tap input: the current mode, the pending key and the character it cycles
 * through; it reports characters and state changes through post(). size: 0x34 */
class HkbCellphoneInput : public HkbInputWidget {
public:
    virtual ~HkbCellphoneInput();
    virtual void create(MEMAllocator* allocator);
    virtual void init();
    virtual void onCommand(u32 command, u32 arg);
    virtual void onPaneEvent(u32 kind, const char* arg);
    virtual s32 isBusy();
    virtual void setLayoutType(s32 language);
    virtual void showMode();                              /* +0x34 */
    virtual void drawComponents();                        /* +0x38 */
    virtual void calcComponents();                        /* +0x3C */
    virtual s32 getMode();                                /* +0x40 */
    virtual u8 isLocked();                                /* +0x44 */
    virtual u8 isAltMode();                               /* +0x48 */
    virtual u8 isPredicting();                            /* +0x4C */
    virtual BOOL hasPendingKey();                         /* +0x50 */
    virtual void setAltMode(BOOL on);                     /* +0x54 */
    virtual void setMode(s32 mode);                       /* +0x58 */
    virtual u8 isCaps();                                  /* +0x5C */
    virtual void selectMode(s32 newMode);                 /* +0x60 */
    virtual void latchCaps(u8 on);                        /* +0x64 */
    virtual void redraw();                                /* +0x68 */
    virtual void openSymbols();                           /* +0x6C */
    virtual void commitText();                            /* +0x70 */
    virtual void setCaps(bool on);                        /* +0x74 */
    virtual BOOL predictIsNotEnglish();                   /* +0x78 */
    virtual BOOL predictIsEnglish();                      /* +0x7C */
    virtual void onIdle();                                /* +0x80 */
    virtual void updateSpaceKey();                        /* +0x84 */

    s32 char_index;                                       /* +0x14 */
    s32 mode;                                             /* +0x18 */
    u8 locked;                                            /* +0x1C */
    u8 alt_mode;                                          /* +0x1D */
    u8 predicting;                                        /* +0x1E */
    u8 caps;                                              /* +0x1F */
    u8 caps_pending;                                      /* +0x20 */
    u8 predict_on;                                        /* +0x21 */
    u8 pad_0x22[2];                                       /* +0x22 */
    const HkbCellphoneKey* pending_key;                   /* +0x24 */
    const HkbCellphoneLanguage* modes;                    /* +0x28 */
    MEMAllocator* allocator;                              /* +0x2C */
    HkbKeyboardView* keyboard;                            /* +0x30 */
};

/* --------------------------------------------------------------------------------------------- */
/* Pane buttons and the pane listener                                                              */
/* --------------------------------------------------------------------------------------------- */

class HkbCellphoneKeypad;

/* The pane controller whose events the pane listener receives; the slots it never calls are named by offset.
 * size: 0xA0 (view: only the pane at +0x9C is read) */
class HkbPaneController {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual void v_0x10();
    virtual void v_0x14();
    virtual void v_0x18();
    virtual void v_0x1C();
    virtual void v_0x20();
    virtual void v_0x24();
    virtual BOOL isHeld(u32 channel);                     /* +0x28 */
    virtual void v_0x2C();
    virtual void v_0x30();
    virtual void v_0x34();
    virtual void v_0x38();
    virtual void v_0x3C();
    virtual void v_0x40();
    virtual void v_0x44();
    virtual void v_0x48();
    virtual void v_0x4C();
    virtual void v_0x50();
    virtual void v_0x54();
    virtual void v_0x58();
    virtual void v_0x5C();
    virtual void release(u32 channel, s32 flag);          /* +0x60 */

    u8 pad_0x04[0x98];                                    /* +0x04 */
    nw4r::lyt::Pane* mPane;                               /* +0x9C */
};

/* The keypad's pane listener: forwards the pane events of the keypad's buttons to the keypad. size: 0x10 */
class HkbCellphonePaneListener : public HbmPaneHandler {
public:
    HkbCellphonePaneListener(HkbCellphoneKeypad* keypad) : mKeypad(keypad) {}

    /* untyped: byte range - the object's storage from the band allocator */
    static void* operator new(unsigned long size, MEMAllocator* allocator) {
        return MEMAllocFromAllocator(allocator, size);
    }

    virtual ~HkbCellphonePaneListener();
    virtual void onPaneEvent(HbmComponent* component, u32 kind, HbmPaneTrigger* trigger);

    HkbCellphoneKeypad* mKeypad;                          /* +0x0C */
};

/* A keypad key or mode button: the state machine over the button animations. size: 0x34 */
class HkbCellphoneKeyButton : public HbmKey {
public:
    HkbCellphoneKeyButton(nw4r::lyt::Pane* pane) : HbmKey(pane), mRadio(0) {
        reset();
        setState(mState);
    }

    /* untyped: byte range - the object's storage from the band allocator */
    static void* operator new(unsigned long size, MEMAllocator* allocator) {
        return MEMAllocFromAllocator(allocator, size);
    }

    virtual ~HkbCellphoneKeyButton();
    virtual void reset();
    virtual void setAnim(s32 input);
    virtual void setState(u32 state);
    virtual BOOL isRadio();                               /* +0x24 */

    BOOL mRadio;                                          /* +0x30 */
};

/* A tag (mode) button: a radio key button. size: 0x34 */
class HkbCellphoneTagButton : public HkbCellphoneKeyButton {
public:
    HkbCellphoneTagButton(nw4r::lyt::Pane* pane) : HkbCellphoneKeyButton(pane) {
        mRadio = 1;
    }

    virtual void setAnim(s32 input);
};

class HkbCellphoneKeypad;

/* The object the keypad hands its committed text and its symbol requests to. size: 0x4 (view) */
class HkbKeypadHost {
public:
    virtual void v_0x08();
    virtual void v_0x0C();
    virtual void v_0x10();
    virtual void v_0x14();
    virtual void v_0x18();
    virtual void v_0x1C();
    virtual void v_0x20();
    virtual void v_0x24();
    virtual void v_0x28();
    virtual void v_0x2C();
    virtual void v_0x30();
    virtual void v_0x34();
    virtual void v_0x38();
    virtual void v_0x3C();
    virtual void v_0x40();
    virtual void v_0x44();
    virtual void v_0x48();
    virtual void v_0x4C();
    virtual void v_0x50();
    virtual void v_0x54();
    virtual void v_0x58();
    virtual void v_0x5C();
    virtual void v_0x60();
    virtual void v_0x64();
    virtual void v_0x68();
    virtual void v_0x6C();
    virtual void v_0x70();
    virtual void v_0x74();
    virtual void v_0x78();
    virtual void v_0x7C();
    virtual void v_0x80();
    virtual void v_0x84();
    virtual void v_0x88();
    virtual void v_0x8C();
    virtual void v_0x90();
    virtual void v_0x94();
    virtual void v_0x98();
    virtual void v_0x9C();
    virtual void v_0xA0();
    virtual void v_0xA4();
    virtual void openText(s32 language, HkbCellphoneKeypad* keypad);                /* +0xA8 */
    virtual void v_0xAC();
    virtual void v_0xB0();
    virtual void v_0xB4();
    virtual void v_0xB8();
    virtual void v_0xBC();
    virtual void v_0xC0();
    virtual void v_0xC4();
    virtual void v_0xC8();
    virtual void v_0xCC();
    virtual void v_0xD0();
    virtual void openSymbols(HkbCellphoneKeypad* keypad, BOOL numeric, s32 page);   /* +0xD4 */
};

/* --------------------------------------------------------------------------------------------- */
/* HkbCellphoneKeypad                                                                              */
/* --------------------------------------------------------------------------------------------- */

/* The cellphone keypad screen: the multi-tap input drawn through a text field, its key and tag buttons and the
 * targets that receive the committed text and the symbol requests. size: 0xEC */
class HkbCellphoneKeypad : public HkbCellphoneInput, public HbmPaneManager {
public:
    virtual ~HkbCellphoneKeypad();
    virtual void create(MEMAllocator* allocator);
    virtual void init();
    virtual void setEventSink(HkbEventSink* sink);
    virtual void onCommand(u32 command, u32 arg);
    virtual void onPaneEvent(u32 kind, const char* arg);
    virtual void setLayoutType(s32 language);
    virtual void calc();
    virtual void showMode();
    virtual void drawComponents();
    virtual void calcComponents();
    virtual void setAltMode(BOOL on);
    virtual void setMode(s32 mode);
    virtual void selectMode(s32 newMode);
    virtual void latchCaps(u8 on);
    virtual void redraw();
    virtual void openSymbols();
    virtual void commitText();
    virtual void setCaps(bool on);
    virtual void setTextTarget(HkbKeypadHost* target);            /* +0xFC */
    virtual void setSymbolTarget(HkbKeypadHost* target);          /* +0x100 */
    virtual void enablePredict(BOOL on);                          /* +0x104 */
    virtual void showLineFeed(BOOL visible);                      /* +0x108 */
    virtual void showPredictButton(BOOL visible);                 /* +0x10C */
    virtual void showOthersButton(BOOL visible);                  /* +0x110 */
    virtual u32 updateComponents();                               /* +0x114 */
    virtual s32 onKeyRelease(HkbManager* manager);                /* +0x118 */
    virtual HkbCellphoneKeyButton* getTagButton(s32 index);       /* +0x11C */
    virtual void releaseButtons();                                /* +0x120 */
    virtual void clearButtons();                                  /* +0x124 */
    virtual void setPredict(HkbPredictState* event);              /* +0x128 */
    virtual void setKeyLabels(const HkbCellphoneKey* keys);       /* +0x12C */
    virtual void updateZeroKey(const HkbCellphoneKey* keys);      /* +0x130 */

    u8 mPredictVisible;                                   /* +0xDC */
    u8 pad_0xDD[3];                                       /* +0xDD */
    HkbCellphonePaneListener* mPaneListener;              /* +0xE0 */
    HkbKeypadHost* mTextTarget;                           /* +0xE4 */
    HkbKeypadHost* mSymbolTarget;                         /* +0xE8 */
};

#endif

#endif
