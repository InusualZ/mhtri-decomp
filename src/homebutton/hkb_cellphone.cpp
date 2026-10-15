/* homebutton/hkb_cellphone.cpp - the software keyboard's cellphone-keypad screen: the multi-tap input
 *   HkbCellphoneInput, the keypad screen HkbCellphoneKeypad that draws it through the band's text field, its key and
 *   tag buttons and its pane listener, with the pane-name, window and animation tables they walk; the static
 *   initialiser writes the radio-group names into the window tables.
 * RANGE. .text 0x8053E808-0x805425E4 (79 functions); .ctors 0x8056F3F8-0x8056F3FC; .rodata 0x8057AB98-0x8057B098;
 *   .data 0x8064DE88-0x8064E760; .sdata 0x80794568-0x807945C8; .sdata2 0x8079D620-0x8079D628.
 *   Edges: the left edge follows the seven 8-byte slots 0x8053E7D0..0x8053E800 of homebutton/hkb_keyboard.cpp's class
 *   table .data 0x8064DC30; the right edge is the static initialiser 0x8054254C (.ctors word 0x8056F3F8) and its
 *   thunks to 0x805425E4.
 *   One TU: the zigzag `.data` seam 0x8064E650 (tables 0x8064E51C -> 0x8064E650, .text interval
 *   0x8053F65C..0x8053F7BC) is contradicted by three private objects read on both sides of it - .rodata 0x8057AFA8
 *   (0x8053E808, 0x8053E8B8, 0x8053FCA8), .rodata 0x8057AB98 (0x8053E968, 0x8053F7BC, 0x805404D8) and the .sdata2
 *   byte 0x8079D620 (0x8053F22C, 0x80540684).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS), plus
 *   `#pragma auto_inline on`: retail inlines HkbCellphoneInput's init, reset, showMode, receive, setAltMode and
 *   setCaps into HkbCellphoneKeypad's overrides (the shared .sdata2 initialiser 0x8079D620 proves it) and keeps the
 *   out-of-line copies in place.
 * NAMES. GUESS: the file name (from the B_CPkey_* tables) and every class, method, field and table name (from the
 *   pane names, the event ids and the slot each method fills); the bases are the band's HkbInputWidget
 *   (homebutton/hkb_keyboard.h), HbmPaneManager and HbmKey (homebutton/hbm_value.h) and HbmPaneHandler
 *   (homebutton/hkb_keyboard.h); the input's own slots 0x38/0x3C are named drawComponents/calcComponents because the
 *   keypad's one override fills both its bases' tables; HkbCellphoneCharEvent is this unit's view of the
 *   character record (the keyboard's HkbCharEvent reads it differently).
 * SHAPES. HkbCellphoneKeypad is HkbCellphoneInput (+0x00) and HbmPaneManager (+0x34): its overrides of methods both
 *   bases declare (init, drawComponents, calcComponents, updateComponents, onKeyRelease) fill both tables, the
 *   second through the `subi 0x34` thunks.
 * SHAPES. The root's onCommand/onPaneEvent take (u32, u32) and (u32, const char*): the overrides cast to the
 *   event records, and keep a signed copy of the id for retail's cmpwi.
 * SHAPES. The pane-name loops count in u16 (`clrlwi` before the stride multiply); the numeric-mode tests compare the
 *   mode with the Japanese and the European numeric index, both 3 (the doubled `beq`); a test of the language
 *   alone is a one-case switch (`beq` then `b`).
 * SHAPES. The class declaration order (input, pane listener, key button, tag button, keypad) gives retail's vtable
 *   order in .data (keypad 0x8064E51C, tag button, key button, pane listener, input 0x8064E6C0), so the
 *   emission-order seam datagap reports at 0x8064E650 is the keypad/tag-button vtable pair inside this one TU.
 * RESIDUALS. onKeyRelease (0x805417D8, 0x350) unwritten: it walks homebutton/tiHKBManager.cpp's HkbKeyIter cursor
 *   (hkbKeyIterBeginWord1/3, hkbKeyIterSeek, hkbGetModifierState, hkbKeyIterGetUsage, hkbKeyIterGetChar,
 *   hkbKeyIterValid); its .data literal "W_CPkey_DELETE" (0x8064E4D0) is missing with it, so the
 *   .data from 0x8064E4D0 sits 0x10 B early and the thunk order differs (retail: dtor, sync, onKeys, open, close,
 *   reset).
 * RESIDUALS. Unwritten: 0x80541B28 (0x8, returns the byte +0xC0 of a class whose tables are
 *   homebutton/hbm_kb_list.cpp's and homebutton/hbm_value.cpp's) and 0x8054243C (0x10, stores a u16 at +0x80 +
 *   2*index of a class whose tables are homebutton/tiHKBManager.cpp's); neither class is reconstructed.
 * RESIDUALS. HkbCellphoneInput::onPaneEvent (0x8053E968): the register allocator swaps name/backward (r27/r28 vs ours
 *   r28/r30) and the counter/base pairs of the inlined findKey loops; no declaration-order or helper-signature
 *   variant moved it.
 * RESIDUALS. HkbCellphoneKeypad::create (0x8053F7BC): the key buttons' inline constructor stores the band key's
 *   state before the vptr (HbmKey's constructor body), retail after it; the loop counters and button pointers
 *   take other callee-saved registers (retail i r30/r29, ours r17/r31).
 * RESIDUALS. String literals and the section-relative bases are compiler-local (@NNN, ...data.0) where retail's
 *   relocations name dtk's lbl_ rows; the bytes are the same.
 */
#include "homebutton/hkb_cellphone.h"
#include "homebutton/hbm_hermite.h"
#include "homebutton/tiHKBManager.h"
#include "MSL/strlen.h"
#include "MSL_C/string.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma auto_inline on

/* The predictive-input dictionary names, indexed by HkbPredictState::language. */
static const wchar_t* sDictionaryNames[11] = {
    L"", L"", L"Eng", L"Fra", L"Esp", L"EN", L"DE", L"FR", L"ES", L"IT", L"NL",
};

/* The keypad's button pane names, in the order the pane listener tests them. */
static const char* sButtonPaneNames[26] = {
    "B_CPkey_00", "B_CPkey_01", "B_CPkey_02", "B_CPkey_03", "B_CPkey_04", "B_CPkey_05",
    "B_CPkey_06", "B_CPkey_07", "B_CPkey_08", "B_CPkey_09", "B_CPkey_10", "B_CPkey_11",
    "B_CPkey_LF", "B_CPkey_DELETE", "B_othersBT_JP", "B_othersBT_EU", "B_spaceBT_JP",
    "B_ChngTag_00", "B_ChngTag_01", "B_ChngTag_02", "B_ChngTag_03",
    "B_SGNkey_close", "B_SGNkey_prev", "B_SGNkey_next", "B_CPkey_Prdc_JP", "B_prdcModeBT_EU",
};

/* The radio groups the window panes belong to, named after their first window. */
static const char* sKeyGroup = "W_CPkey_00";
static const char* sTagGroup = "W_ChngTag_00";
static const char* sOthersGroup = "W_othersBT_EU";

/* The key and mode-button windows the keypad builds key buttons for. */
static HkbCellphoneWindow sKeyWindows[19] = {
    {"W_CPkey_00", NULL},
    {"W_CPkey_01", sKeyGroup},
    {"W_CPkey_02", sKeyGroup},
    {"W_CPkey_03", sKeyGroup},
    {"W_CPkey_04", sKeyGroup},
    {"W_CPkey_05", sKeyGroup},
    {"W_CPkey_06", sKeyGroup},
    {"W_CPkey_07", sKeyGroup},
    {"W_CPkey_08", sKeyGroup},
    {"W_CPkey_09", sKeyGroup},
    {"W_CPkey_10", sKeyGroup},
    {"W_CPkey_11", sKeyGroup},
    {"W_CPkey_LF", sKeyGroup},
    {"W_CPkey_DELETE", sKeyGroup},
    {"W_spaceBT_JP", sKeyGroup},
    {"W_othersBT_JP", sOthersGroup},
    {"W_othersBT_EU", NULL},
    {"W_prdcModeBT_EU", NULL},
    {"W_smlCptChngeBT", NULL},
};

/* The mode-tag windows the keypad builds tag buttons for. */
static HkbCellphoneWindow sTagWindows[4] = {
    {"W_ChngTag_00", NULL},
    {"W_ChngTag_01", sTagGroup},
    {"W_ChngTag_02", sTagGroup},
    {"W_ChngTag_03", sTagGroup},
};

/* The buttons whose pane press triggers an action rather than a key. */
static const HkbCellphoneButton sButtons[12] = {
    {"B_CPkey_LF", 0},
    {"B_CPkey_DELETE", 1},
    {"B_othersBT_JP", 9},
    {"B_ChngTag_00", 18},
    {"B_ChngTag_01", 19},
    {"B_ChngTag_02", 11},
    {"B_ChngTag_03", 13},
    {"B_othersBT_EU", 9},
    {"B_CPkey_Prdc_JP", 10},
    {"B_prdcModeBT_EU", 10},
    {"B_smlCptChngeBT", 23},
    {"B_spaceBT_JP", 2},
};

/* The key-button animations, by button state. */
static const HkbCellphoneAnim sKeyAnims[5] = {
    {0, "fs_VK_cellPhone_a_normal.brlan"},
    {1, "fs_VK_cellPhone_a_Focus-IN.brlan"},
    {2, "fs_VK_cellPhone_a_Focus-OUT.brlan"},
    {4, "fs_VK_cellPhone_a_Pushed.brlan"},
    {3, "fs_VK_cellPhone_a_Roll_over.brlan"},
};

/* The tag-button animations, by button state. */
static const HkbCellphoneAnim sTagAnims[6] = {
    {0, "fs_VK_cellPhone_a_normal.brlan"},
    {1, "fs_VK_cellPhone_a_Focus-IN.brlan"},
    {2, "fs_VK_cellPhone_a_Focus-OUT.brlan"},
    {4, "fs_VK_cellPhone_a_Pushed.brlan"},
    {5, "fs_VK_cellPhone_a_toggle-ON.brlan"},
    {6, "fs_VK_cellPhone_a_toggle-OFF.brlan"},
};

/* The languages' mode rows, indexed by getLayoutType(): Japanese, then the European languages. */
static const HkbCellphoneLanguage sLanguages[8] = {
    {5, 0, {&hkb_cellphone_mode_hiragana, &hkb_cellphone_mode_katakana, &hkb_cellphone_mode_address_upper,
            &hkb_cellphone_mode_numeric, &hkb_cellphone_mode_address_lower}},
    {5, 0, {&hkb_cellphone_mode_us_title, &hkb_cellphone_mode_us_lower, &hkb_cellphone_mode_us_upper,
            &hkb_cellphone_mode_numeric, NULL}},
    {5, 0, {&hkb_cellphone_mode_uk_title, &hkb_cellphone_mode_uk_lower, &hkb_cellphone_mode_uk_upper,
            &hkb_cellphone_mode_numeric, NULL}},
    {5, 0, {&hkb_cellphone_mode_dutch_title, &hkb_cellphone_mode_dutch_lower, &hkb_cellphone_mode_dutch_upper,
            &hkb_cellphone_mode_numeric, NULL}},
    {5, 0, {&hkb_cellphone_mode_german_title, &hkb_cellphone_mode_german_lower, &hkb_cellphone_mode_german_upper,
            &hkb_cellphone_mode_numeric, NULL}},
    {5, 0, {&hkb_cellphone_mode_french_title, &hkb_cellphone_mode_french_lower, &hkb_cellphone_mode_french_upper,
            &hkb_cellphone_mode_numeric, NULL}},
    {5, 0, {&hkb_cellphone_mode_spanish_title, &hkb_cellphone_mode_spanish_lower,
            &hkb_cellphone_mode_spanish_upper, &hkb_cellphone_mode_numeric, NULL}},
    {5, 0, {&hkb_cellphone_mode_italian_title, &hkb_cellphone_mode_italian_lower,
            &hkb_cellphone_mode_italian_upper, &hkb_cellphone_mode_numeric, NULL}},
};

/* Returns the action of the button pane `name`, or 33 when it names no action button. */
static s32 findAction(const char* name) {
    u16 i;

    for (i = 0; i < 12; i++) {
        if (hbmNameMatches(sButtons[i].name, name)) {
            return sButtons[i].action;
        }
    }
    return 33;
}

/* Returns the key of `keys` whose pane is `name`, or NULL. */
static const HkbCellphoneKey* findKey(const char* name, const HkbCellphoneKey* keys) {
    u16 i;

    for (i = 0; i < 12; i++) {
        if (hbmNameMatches(keys[i].name, name)) {
            return &keys[i];
        }
    }
    return NULL;
}

/* 0x8053E808 (0xB0): Takes the allocator and resets the input to the language's first mode. */
void HkbCellphoneInput::create(MEMAllocator* alloc) {
    allocator = alloc;
    char_index = 0;
    mode = 0;
    pending_key = NULL;
    locked = 0;
    predicting = 0;
    alt_mode = 0;
    caps = 1;
    caps_pending = 1;
    predict_on = 0;
    modes = &sLanguages[getLayoutType()];
    HkbCellphoneCharEvent event = {0};
    sendEvent(3, &event);
}

/* 0x8053E8B8 (0xAC): Resets the input to the language's first mode. */
void HkbCellphoneInput::init() {
    char_index = 0;
    mode = 0;
    pending_key = NULL;
    locked = 0;
    predicting = 0;
    alt_mode = 0;
    caps = 1;
    caps_pending = 1;
    predict_on = 0;
    modes = &sLanguages[getLayoutType()];
    HkbCellphoneCharEvent event = {0};
    sendEvent(3, &event);
}

/* 0x8053E964 (0x4): Does nothing; the keypad shows its panes here. */
void HkbCellphoneInput::drawComponents() {}

/* 0x8053E968 (0x808): Handles a pane press: runs an action button, cycles the pressed key's characters (or types
 * the numeric key's digit) and commits the pending character when its key is released. */
void HkbCellphoneInput::onPaneEvent(u32 kind, const char* arg) {
    HkbPaneEvent* event = (HkbPaneEvent*)arg;
    if (locked) {
        return;
    }
    const char* name = event->pane_name;
    if (kind == 4) {
        u8 backward = event->backward;
        s32 action = findAction(name);
        if (action != 33) {
            switch (action) {
            case 0:
                sendEvent(7, NULL);
                break;
            case 1:
                sendEvent(1, NULL);
                break;
            case 18:
                setMode(0);
                break;
            case 19:
                setMode(1);
                break;
            case 11:
                setMode(2);
                break;
            case 13:
                setMode(3);
                break;
            case 9:
                openSymbols();
                break;
            case 10:
                commitText();
                break;
            case 23:
                setCaps(caps == 0);
                break;
            }
        } else if (!predictIsNotEnglish() || mode == 3 || mode == 3) {
            if (pending_key == NULL) {
                s32 keyMode = mode;
                if (getLayoutType() == 0 && keyMode == 2) {
                    if (!caps) {
                        keyMode = 4;
                    }
                } else if (modes->modes[keyMode]->case_mode == 1 && !caps) {
                    keyMode = 1;
                }
                pending_key = findKey(name, modes->modes[keyMode]->keys);
                if (backward) {
                    char_index = 15;
                    while (pending_key->chars[char_index] == 0) {
                        char_index--;
                    }
                } else {
                    char_index = 0;
                }
            } else {
                if (backward) {
                    char_index--;
                } else {
                    char_index++;
                }
                if (char_index == 16) {
                    char_index = 0;
                } else if (char_index < 0) {
                    char_index = 15;
                    while (pending_key->chars[char_index] == 0) {
                        char_index--;
                    }
                }
                if (pending_key->chars[char_index] == 0) {
                    char_index = 0;
                }
            }
            if (pending_key != NULL) {
                HkbCellphoneCharEvent ch = {0};
                ch.ch = pending_key->chars[char_index];
                ch.key = pending_key;
                if (mode == 3 || mode == 3) {
                    if (pending_key->chars[1] != L'0' && pending_key->chars[0] != L' ') {
                        ch.replace = 1;
                    }
                }
                if (pending_key->chars[1] == 0) {
                    if (predicting && ch.ch == L'*') {
                        ch.ch = L'.';
                    }
                    sendEvent(5, &ch);
                    char_index = 0;
                    pending_key = NULL;
                } else {
                    if (getLayoutType() == 0 && ch.ch == L' ' && getMode() != 4 && getMode() != 2) {
                        ch.ch = 0x3000;
                    }
                    sendEvent(3, &ch);
                }
            }
        } else {
            const HkbCellphoneKey* keys = hkb_cellphone_mode_numeric.keys;
            u16 i;
            for (i = 0; i < 12; i++) {
                if (hbmNameMatches(keys[i].name, name)) {
                    u16 digit = keys[i].chars[0];
                    HkbSwitchArg delete_on;
                    HkbCellphoneCharEvent ch = {0};
                    ch.ch = digit;
                    ch.mode = mode;
                    ch.key = pending_key;
                    sendEvent(32, &delete_on);
                    if (digit == L'0') {
                        if (delete_on.on) {
                            sendEvent(6, NULL);
                        }
                        HkbCellphoneCharEvent space = {L' ', 0, 1, 0, 1};
                        space.mode = mode;
                        space.key = pending_key;
                        sendEvent(5, &space);
                        char_index = 0;
                        pending_key = NULL;
                    } else {
                        ch.key = findKey(name, modes->modes[mode]->keys);
                        if (modes->modes[mode]->case_mode == 1 && !caps_pending) {
                            ch.mode = 1;
                            ch.key = findKey(name, modes->modes[1]->keys);
                        }
                        sendEvent(5, &ch);
                    }
                }
            }
        }
    }
    if (kind == 1) {
        if (pending_key != NULL && hbmNameMatches(name, pending_key->name)) {
            u16 c = pending_key->chars[char_index];
            if (c != 0x309B && c != 0x309C) {
                if (getLayoutType() == 0 && c == L' ' && getMode() != 4 && getMode() != 2) {
                    c = 0x3000;
                }
                HkbCellphoneCharEvent done = {0, 0, 0, 0, 1};
                done.ch = c;
                done.key = pending_key;
                if (mode == 3 || mode == 3) {
                    done.replace = 1;
                }
                sendEvent(5, &done);
            }
            char_index = 0;
            pending_key = NULL;
        }
    }
}

/* 0x8053F170 (0x4): Does nothing; the keypad opens the symbol list here. */
void HkbCellphoneInput::openSymbols() {}

/* 0x8053F174 (0x4): Does nothing; the keypad hands the text on here. */
void HkbCellphoneInput::commitText() {}

/* 0x8053F178 (0x8): Returns the current mode. */
s32 HkbCellphoneInput::getMode() {
    return mode;
}

/* 0x8053F180 (0xAC): Follows the keyboard's events: id 0x21 turns caps on, a typed character (id 5) that is not
 * confirmed turns it off. */
void HkbCellphoneInput::onCommand(u32 command, u32 arg) {
    s32 id = (s32)command;
    void* data = (void*)arg;
    if (getLayoutType() != 0) {
        if (id == 0x21) {
            setCaps(1);
        }
        if (id == 5 && ((HkbCellphoneCharEvent*)data)->confirmed == 0) {
            setCaps(0);
            caps_pending = 0;
        }
    }
}

/* 0x8053F22C (0xDC): Posts the mode change and the switch state of the current mode. */
void HkbCellphoneInput::showMode() {
    s32 none = 0;
    sendEvent(0x12, &none);
    if (mode == 0) {
        HkbSwitchArg on = {1};
        sendEvent(0x13, &on);
    }
    if (mode == 1) {
        HkbSwitchArg off = {0};
        sendEvent(0x13, &off);
    }
    if (mode == 0) {
        setMode(mode);
    }
    updateSpaceKey();
}

/* 0x8053F308 (0x4): Does nothing; the keypad redraws its panes here. */
void HkbCellphoneInput::redraw() {}

/* 0x8053F30C (0x134): Selects a mode and posts the mode change. */
void HkbCellphoneInput::setMode(s32 newMode) {
    mode = newMode;
    if (getLayoutType() != 0) {
        caps = 1;
        caps_pending = 1;
    }
    HkbSwitchArg numeric = {0};
    if (newMode == 3) {
        numeric.on = 1;
    }
    sendEvent(0x22, &numeric);
    sendEvent(6, NULL);
    if (newMode == 0) {
        HkbSwitchArg on = {1};
        sendEvent(0x13, &on);
    }
    if (newMode == 1) {
        HkbSwitchArg off = {0};
        sendEvent(0x13, &off);
    }
    updateSpaceKey();
    sendEvent(0x29, NULL);
}

/* 0x8053F440 (0x68): Returns whether prediction is on with a dictionary other than English. */
BOOL HkbCellphoneInput::predictIsNotEnglish() {
    HkbPredictState state = {0};
    sendEvent(0x1F, &state);
    s32 language = state.language;
    if (state.on) {
        return language != 1;
    }
    return FALSE;
}

/* 0x8053F4A8 (0x64): Returns whether prediction is on with the English dictionary. */
BOOL HkbCellphoneInput::predictIsEnglish() {
    HkbPredictState state = {0};
    sendEvent(0x1F, &state);
    s32 language = state.language;
    if (state.on && language == 1) {
        return TRUE;
    }
    return FALSE;
}

/* 0x8053F50C (0x18): Sets caps; turning it on also arms it for the next character. */
void HkbCellphoneInput::setCaps(bool on) {
    caps = on;
    if (on) {
        caps_pending = 1;
    }
}

/* 0x8053F524 (0x60): Sets the alternate mode; turning it on lights the keyboard's caps lock and selects the
 * numeric mode. */
void HkbCellphoneInput::setAltMode(BOOL on) {
    alt_mode = on;
    if (on) {
        hkbSetModifierState(&hkbManager, 0x100, 0x100);
        setMode(3);
    }
}

/* 0x8053F584 (0xD8): Posts whether the space key types a full-width space (the kana modes of the Japanese
 * layout). */
void HkbCellphoneInput::updateSpaceKey() {
    if (keyboard->getInput() == NULL || !keyboard->getInput()->isActive()) {
        switch (getLayoutType()) {
        case 0:
            HkbSwitchArg wide = {0};
            if (mode == 4 || mode == 2) {
                wide.on = 1;
            } else {
                wide.on = 0;
            }
            sendEvent(0x14, &wide);
            break;
        }
    }
}

/* 0x8053F65C (0xE0): Destroys the pane listener and the buttons, then the text field. */
HkbCellphoneKeypad::~HkbCellphoneKeypad() {
    mPaneListener->~HkbCellphonePaneListener();
    MEMFreeToAllocator(allocator, mPaneListener);
    HbmKey* button = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL);
    while (button != NULL) {
        nw4r::ut::List_Remove(&mComponents, button);
        button->destroy(allocator);
        button = (HbmKey*)nw4r::ut::List_GetNext(&mComponents, NULL);
    }
}

/* 0x8053F73C (0x40): Destroys the input. */
HkbCellphoneInput::~HkbCellphoneInput() {}

/* 0x8053F77C (0x40): Destroys the pane listener. */
HkbCellphonePaneListener::~HkbCellphonePaneListener() {}

/* 0x8053F7BC (0x488): Sets the keypad up: the input, the pane listener and the text field, then one key button
 * per key window and one tag button per mode tag, each with its animations. */
void HkbCellphoneKeypad::create(MEMAllocator* alloc) {
    u16 i;
    u16 j;
    HkbCellphoneKeyButton* button;
    const char* group;
    HkbCellphoneWindow* window;

    HkbCellphoneInput::create(alloc);
    mPaneListener = new (alloc) HkbCellphonePaneListener(this);
    HbmPaneManager::create(alloc, mPaneListener);
    mpRoot->setVisible(0);
    mpRoot->setEnable(1);
    for (i = 0; i < 19; i++) {
        button = new (alloc) HkbCellphoneKeyButton(findTextBox(sKeyWindows[i].name));
        nw4r::ut::List_Append(&mComponents, button);
        group = sKeyWindows[i].group;
        window = &sKeyWindows[i];
        for (j = 0; j < 5; j++) {
            void* res = mpResAccessor->GetResource(0, sKeyAnims[j].name, NULL);
            nw4r::lyt::AnimTransform* anim = getLayout()->CreateAnimTransform(res, mpResAccessor);
            if (group != NULL) {
                button->addGroupAnim(alloc, sKeyAnims[j].state, anim, window->group, 0, 1);
            } else {
                button->addAnim(alloc, sKeyAnims[j].state, anim, 0, 1);
            }
        }
    }
    for (i = 0; i < 4; i++) {
        button = new (alloc) HkbCellphoneTagButton(findTextBox(sTagWindows[i].name));
        nw4r::ut::List_Append(&mComponents, button);
        group = sTagWindows[i].group;
        window = &sTagWindows[i];
        for (j = 0; j < 6; j++) {
            void* res = mpResAccessor->GetResource(0, sTagAnims[j].name, NULL);
            nw4r::lyt::AnimTransform* anim = getLayout()->CreateAnimTransform(res, mpResAccessor);
            if (group != NULL) {
                button->addGroupAnim(alloc, sTagAnims[j].state, anim, window->group, 0, 1);
            } else {
                button->addAnim(alloc, sTagAnims[j].state, anim, 0, 1);
            }
        }
    }
    init();
    setLayoutType(getLayoutType());
}

/* 0x8053FC44 (0xC): Returns the button to its first state. */
void HkbCellphoneKeyButton::reset() {
    mState = 0;
}

/* 0x8053FC50 (0x58): Destroys the button. */
HkbCellphoneKeyButton::~HkbCellphoneKeyButton() {}

/* 0x8053FCA8 (0x684): Resets the input and redraws every pane of the keypad for the language: the tag buttons
 * and their labels, the language's mode and option buttons, the prediction state and the space key. */
void HkbCellphoneKeypad::init() {
    HkbCellphoneInput::init();
    getTagButton(0)->setState(0);
    getTagButton(1)->setState(0);
    getTagButton(2)->setState(0);
    getTagButton(3)->setState(0);
    getTagButton(mode)->setState(5);
    showLineFeed(1);
    setAltMode(0);
    showPredictButton(1);
    showOthersButton(1);
    setPaneVisible("W_ChngTag_00", 1);
    setPaneVisible("W_ChngTag_01", 1);
    setPaneVisible("W_ChngTag_02", 1);
    setPaneVisible("W_ChngTag_03", 1);
    for (s32 i = 0; i < 4; i++) {
        char name[17];
        memset(name, 0, 17);
        strncpy(name, sTagWindows[i].name, strlen(sTagWindows[i].name));
        name[0] = 'T';
        ((nw4r::lyt::TextBox*)findTextBox(name))->SetString(modes->modes[i]->label, 0);
    }
    if (getLayoutType() == 0) {
        setPaneVisible("W_prdcModeBT_EU", 0);
        setPaneVisible("W_othersBT_EU", 0);
        setPaneVisible("W_smlCptChngeBT", 1);
        setPaneVisible("W_othersBT_JP", 1);
        setPaneVisible("W_CPkey_Prdc_JP", 1);
        setPaneVisible("W_spaceBT_JP", 1);
        setPaneVisible("W_CPkey_09", 1);
        setPaneVisible("W_CPkey_11", 1);
    } else {
        setPaneVisible("W_prdcModeBT_EU", 1);
        setPaneVisible("W_othersBT_EU", 1);
        setPaneVisible("W_smlCptChngeBT", 0);
        setPaneVisible("W_othersBT_JP", 0);
        setPaneVisible("W_CPkey_Prdc_JP", 0);
        setPaneVisible("W_spaceBT_JP", 0);
        setPaneVisible("W_CPkey_09", 0);
        setPaneVisible("W_CPkey_11", 0);
        showPredictButton(0);
    }
    setMode(mode);
    HkbPredictState state = {0};
    sendEvent(0x1F, &state);
    setPredict(&state);
    resetCursor();
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
    nw4r::lyt::Picture* space = nw4r::ut::DynamicCast<nw4r::lyt::Picture, nw4r::lyt::Pane>(
        mpLyt->mpRootPane->FindPaneByName("P_spaceBT_JP", true));
    nw4r::lyt::Picture* convert = nw4r::ut::DynamicCast<nw4r::lyt::Picture, nw4r::lyt::Pane>(
        mpLyt->mpRootPane->FindPaneByName("P_HENKAN_JP", true));
    space->SetVisible(true);
    convert->SetVisible(false);
}

/* 0x8054032C (0x8): Closes the text field. */
void HkbCellphoneKeypad::calcComponents() {
    HbmPaneManager::calcComponents();
}

/* 0x80540334 (0x1A0): Opens the text field and shows the conversion key while the keyboard converts, the space
 * key otherwise. */
void HkbCellphoneKeypad::drawComponents() {
    HbmPaneManager::drawComponents();
    nw4r::lyt::Picture* space = nw4r::ut::DynamicCast<nw4r::lyt::Picture, nw4r::lyt::Pane>(
        mpLyt->mpRootPane->FindPaneByName("P_spaceBT_JP", true));
    nw4r::lyt::Picture* convert = nw4r::ut::DynamicCast<nw4r::lyt::Picture, nw4r::lyt::Pane>(
        mpLyt->mpRootPane->FindPaneByName("P_HENKAN_JP", true));
    if (keyboard->getComposer()->isConverting()) {
        space->SetVisible(false);
        convert->SetVisible(true);
    } else {
        space->SetVisible(true);
        convert->SetVisible(false);
    }
}

/* 0x805404D4 (0x4): Does nothing. */
void HkbCellphoneKeypad::calc() {}

/* 0x805404D8 (0x1AC): Handles a pane press like the input, then plays the action button's sound or hands the space
 * key to the keyboard. */
void HkbCellphoneKeypad::onPaneEvent(u32 kind, const char* arg) {
    HkbPaneEvent* event = (HkbPaneEvent*)arg;
    HkbCellphoneInput::onPaneEvent(kind, arg);
    if (locked) {
        return;
    }
    const char* name = event->pane_name;
    if (kind == 4) {
        s32 action = findAction(name);
        if (action != 33) {
            switch (action) {
            case 11:
            case 13:
            case 18:
            case 19:
                mpSound->play(13);
                break;
            case 23:
                if (!caps) {
                    mpSound->play(21);
                } else {
                    mpSound->play(20);
                }
                break;
            case 2:
                if (keyboard->getComposer()->isConverting()) {
                    sendEvent(0x28, NULL);
                } else {
                    HkbKeyEvent space = {L' ', 0, 0, 1};
                    sendEvent(0, &space);
                }
                break;
            }
        }
    }
}

/* 0x80540684 (0xE4): Posts the mode change like the input, then lets the text field show the mode. */
void HkbCellphoneKeypad::showMode() {
    HkbCellphoneInput::showMode();
    HbmPaneManager::init();
}

/* 0x80540768 (0x50): Redraws the text field and clears the buttons. */
void HkbCellphoneKeypad::redraw() {
    resetCursor();
    clearButtons();
}

/* 0x805407B8 (0xD0): Follows the keyboard's events like the input; id 0x1D sets the prediction state. */
/* untyped: caller-owned payload - the record type depends on the event id */
void HkbCellphoneKeypad::onCommand(u32 command, u32 arg) {
    s32 id = (s32)command;
    void* data = (void*)arg;
    HkbCellphoneInput::onCommand(command, arg);
    switch (id) {
    case 0x1D:
        setPredict((HkbPredictState*)data);
        break;
    }
}

/* 0x80540888 (0x23C): Selects a mode, shows the panes of its key layout and relabels the keys. */
void HkbCellphoneKeypad::setMode(s32 newMode) {
    HkbCellphoneInput::setMode(newMode);
    if (getLayoutType() == 0) {
        if (!caps && newMode == 2) {
            newMode = 4;
        }
        switch (newMode) {
        case 0:
        case 1:
            setPaneVisible("W_smlCptChngeBT", 0);
            setPaneVisible("W_CPkey_09", 1);
            setPaneVisible("P_CPkey_dakuten", 1);
            setPaneVisible("W_smlCptChngeBT", 0);
            break;
        case 2:
        case 4:
            setPaneVisible("W_smlCptChngeBT", 1);
            setPaneVisible("W_CPkey_09", 0);
            setPaneVisible("P_CPkey_dakuten", 0);
            setPaneVisible("W_smlCptChngeBT", 1);
            break;
        case 3:
            setPaneVisible("W_smlCptChngeBT", 0);
            setPaneVisible("W_CPkey_09", 1);
            setPaneVisible("P_CPkey_dakuten", 0);
            setPaneVisible("W_smlCptChngeBT", 0);
            break;
        }
    }
    setKeyLabels(modes->modes[newMode]->keys);
    resetCursor();
}

/* 0x80540AC4 (0x118): Writes the labels of the twelve keys into their T_ text panes. */
void HkbCellphoneKeypad::setKeyLabels(const HkbCellphoneKey* keys) {
    for (u16 i = 0; i < 12; i++) {
        char name[17];
        hbmNameCopyWithChar(name, 17, keys[i].name, 0, 'T');
        nw4r::lyt::TextBox* text = nw4r::ut::DynamicCast<nw4r::lyt::TextBox, nw4r::lyt::Pane>(
            mpLyt->mpRootPane->FindPaneByName(name, true));
        if (text != NULL) {
            text->SetString(keys[i].label, 0);
        }
    }
    updateZeroKey(keys);
}

/* 0x80540BDC (0xD0): Labels the zero key with the prediction marker when it does not type a zero. */
void HkbCellphoneKeypad::updateZeroKey(const HkbCellphoneKey* keys) {
    if (getLayoutType() != 0) {
        char name[17];
        hbmNameCopyWithChar(name, 17, keys[10].name, 0, 'T');
        nw4r::lyt::TextBox* text = (nw4r::lyt::TextBox*)mpLyt->mpRootPane->FindPaneByName(name, true);
        if (keys[10].chars[0] != L'0') {
            if (!predict_on) {
                text->SetString(hkb_cellphone_zero_label, 0);
            } else {
                text->SetString(hkb_cellphone_predict_label, 0);
            }
        }
    }
}

/* 0x80540CAC (0x9C): Returns the tag button of mode `index` (0..3), or NULL. */
HkbCellphoneKeyButton* HkbCellphoneKeypad::getTagButton(s32 index) {
    switch (index) {
    case 2:
        return (HkbCellphoneKeyButton*)getPaneAnim(sTagWindows[2].name);
    case 3:
        return (HkbCellphoneKeyButton*)getPaneAnim(sTagWindows[3].name);
    case 0:
        return (HkbCellphoneKeyButton*)getPaneAnim(sTagWindows[0].name);
    case 1:
        return (HkbCellphoneKeyButton*)getPaneAnim(sTagWindows[1].name);
    }
    return NULL;
}

/* 0x80540D48 (0x90): Releases every key button that is not a radio button. */
void HkbCellphoneKeypad::releaseButtons() {
    HkbCellphoneKeyButton* button = (HkbCellphoneKeyButton*)nw4r::ut::List_GetNext(&mComponents, NULL);
    while (button != NULL) {
        if (!button->isRadio()) {
            button->setAnim(2);
        }
        button = (HkbCellphoneKeyButton*)nw4r::ut::List_GetNext(&mComponents, button);
    }
}

/* 0x80540DD8 (0x8): Returns whether the button is a radio (tag) button. */
BOOL HkbCellphoneKeyButton::isRadio() {
    return mRadio;
}

/* 0x80540DE0 (0x90): Returns every key button that is not a radio button to its first state. */
void HkbCellphoneKeypad::clearButtons() {
    HkbCellphoneKeyButton* button = (HkbCellphoneKeyButton*)nw4r::ut::List_GetNext(&mComponents, NULL);
    while (button != NULL) {
        if (!button->isRadio()) {
            button->setState(0);
        }
        button = (HkbCellphoneKeyButton*)nw4r::ut::List_GetNext(&mComponents, button);
    }
}

/* 0x80540E70 (0x5C): Selects the language; once the layout exists, resets and redraws the keypad for it. */
void HkbCellphoneKeypad::setLayoutType(s32 newLanguage) {
    mLayoutType = newLanguage;
    if (mpLyt != NULL) {
        init();
        resetCursor();
    }
}

/* 0x80540ECC (0x8): Selects the language. */
void HkbCellphoneInput::setLayoutType(s32 newLanguage) {
    mLayoutType = newLanguage;
}

/* 0x80540ED4 (0x7C): Releases the buttons and hands the text to the text target with the prediction dictionary. */
void HkbCellphoneKeypad::commitText() {
    releaseButtons();
    HkbPredictState state = {0};
    sendEvent(0x1F, &state);
    mTextTarget->openText(state.language, this);
}

/* 0x80540F50 (0x154): Shows the prediction state: the on/off panes, the dictionary name, and relabels the keys. */
void HkbCellphoneKeypad::setPredict(HkbPredictState* state) {
    if (getLayoutType() != 0) {
        if (state->on == 0) {
            setPaneVisible("N_prdc_EU_ON", 0);
            setPaneVisible("N_prdc_EU_OFF", 1);
            predict_on = 0;
        } else {
            setPaneVisible("N_prdc_EU_ON", 1);
            setPaneVisible("N_prdc_EU_OFF", 0);
            predict_on = 1;
        }
        ((nw4r::lyt::TextBox*)findTextBox("N_prdc_EU_lang"))->SetString(sDictionaryNames[state->language], 0);
        updateZeroKey(modes->modes[mode]->keys);
    }
}

/* 0x805410A4 (0x104): Sets caps like the input and relabels the keys for the case it selects. */
void HkbCellphoneKeypad::setCaps(bool on) {
    HkbCellphoneInput::setCaps(on);
    if (getLayoutType() == 0) {
        s32 keyMode = 2;
        if (!caps) {
            keyMode = 4;
        }
        setKeyLabels(modes->modes[keyMode]->keys);
        mpSound->play(18);
    } else if (modes->modes[mode]->case_mode == 1) {
        s32 keyMode = 0;
        if (!caps) {
            keyMode = 1;
        }
        setKeyLabels(modes->modes[keyMode]->keys);
    }
}

/* 0x805411A8 (0x6C): Opens the symbol list on the symbol target and releases the buttons. */
void HkbCellphoneKeypad::openSymbols() {
    BOOL numeric = FALSE;
    if (mode == 3) {
        numeric = TRUE;
    }
    mSymbolTarget->openSymbols(this, numeric, 0);
    releaseButtons();
}

/* 0x80541214 (0x24C): Sets the alternate mode like the input; turning it on hides the mode and option panes. */
void HkbCellphoneKeypad::setAltMode(BOOL on) {
    HkbCellphoneInput::setAltMode(on);
    if (on) {
        BOOL visible = on == 0;
        setPaneVisible("W_smlCptChngeBT", visible);
        setPaneVisible("W_CPkey_09", visible);
        setPaneVisible("P_CPkey_dakuten", visible);
        setPaneVisible("W_smlCptChngeBT", visible);
        setPaneVisible("W_prdcModeBT_EU", visible);
        setPaneVisible("W_othersBT_EU", visible);
        setPaneVisible("W_spaceBT_JP", visible);
        setPaneVisible("W_smlCptChngeBT", visible);
        setPaneVisible("W_othersBT_JP", visible);
        setPaneVisible("W_CPkey_Prdc_JP", visible);
        setPaneVisible("W_CPkey_09", visible);
        setPaneVisible("W_CPkey_11", visible);
        setPaneVisible("W_ChngTag_00", visible);
        setPaneVisible("W_ChngTag_01", visible);
        setPaneVisible("W_ChngTag_02", visible);
        setPaneVisible("W_ChngTag_03", visible);
    }
}

/* 0x80541460 (0x8C): Turns the predictive input on: the alternate mode, the dot key and its window. */
void HkbCellphoneKeypad::enablePredict(BOOL on) {
    setAltMode(on);
    setPaneText("T_CPkey_11", L".");
    setPaneVisible("W_CPkey_11", on);
    predicting = 1;
}

/* 0x805414EC (0x24): Shows or hides the line-feed key. */
void HkbCellphoneKeypad::showLineFeed(BOOL visible) {
    mPredictVisible = visible;
    setPaneVisible("W_CPkey_LF", mPredictVisible);
}

/* 0x80541510 (0x1C): Shows or hides the prediction-mode button. */
void HkbCellphoneKeypad::showPredictButton(BOOL visible) {
    setPaneVisible("W_prdcModeBT_EU", visible);
}

/* 0x8054152C (0x118): Shows the language's others button, or hides both. */
void HkbCellphoneKeypad::showOthersButton(BOOL visible) {
    if (visible == 0) {
        setPaneVisible("W_othersBT_EU", visible);
        setPaneVisible("W_othersBT_JP", visible);
    } else {
        setPaneVisible("W_othersBT_EU", 0);
        setPaneVisible("W_othersBT_JP", 0);
        if (getLayoutType() == 0) {
            setPaneVisible("W_othersBT_JP", 1);
        } else {
            setPaneVisible("W_othersBT_EU", 1);
        }
    }
}

/* 0x80541644 (0x144): Selects a mode and redraws the tag buttons and the layout. */
void HkbCellphoneKeypad::selectMode(s32 newMode) {
    setMode(newMode);
    getTagButton(0)->setState(0);
    getTagButton(1)->setState(0);
    getTagButton(2)->setState(0);
    getTagButton(3)->setState(0);
    getTagButton(mode)->setState(5);
    mpLyt->Animate(0);
    mpLyt->CalculateMtx(mDrawInfo);
}

/* 0x80541788 (0x50): Sets caps directly on the Japanese keypad. */
void HkbCellphoneKeypad::latchCaps(u8 on) {
    if (getLayoutType() == 0) {
        caps = on;
    }
}

/* 0x80541B30 (0x1C): Enters a state and plays its animation (state 7 plays the focus-out one). */
void HkbCellphoneKeyButton::setState(u32 state) {
    mState = state;
    HbmKey::setState(state == 7 ? 2 : state);
}

/* 0x80541B4C (0x254): Moves the key button through its states on an input (0 press, 1 focus in, 2 focus out,
 * 4 release). */
void HkbCellphoneKeyButton::setAnim(s32 input) {
    switch (mState) {
    case 0:
        if (input == 1) {
            setState(1);
        }
        if (input == 0) {
            setState(7);
        }
        break;
    case 1:
        if (input == 4) {
            setState(3);
        }
        if (input == 2) {
            setState(2);
        }
        if (input == 0) {
            setState(4);
        }
        break;
    case 3:
        if (input == 2) {
            setState(2);
        }
        if (input == 0) {
            setState(4);
        }
        break;
    case 2:
        if (input == 4) {
            setState(0);
        }
        if (input == 1) {
            setState(1);
        }
        if (input == 0) {
            setState(7);
        }
        break;
    case 4:
        if (input == 4) {
            setState(3);
        }
        if (input == 2) {
            setState(2);
        }
        if (input == 0) {
            setState(4);
        }
        break;
    case 7:
        if (input == 4) {
            setState(0);
        }
        if (input == 1) {
            setState(1);
        }
        if (input == 0) {
            setState(7);
        }
        break;
    }
}

/* 0x80541DA0 (0x22C): Moves the tag button through its states; input 5 toggles it on or back off. */
void HkbCellphoneTagButton::setAnim(s32 input) {
    if (input == 5 && mState != 6) {
        if (mState == 5) {
            setState(6);
            return;
        }
        if (mState != 0 && mState != 2) {
            setState(2);
            return;
        }
    }
    if (input == 0) {
        setState(4);
        return;
    }
    switch (mState) {
    case 0:
        if (input == 1) {
            setState(1);
        }
        break;
    case 1:
        if (input == 4) {
            setState(3);
        }
        if (input == 2) {
            setState(2);
        }
        if (input == 0) {
            setState(4);
        }
        break;
    case 3:
        if (input == 2) {
            setState(2);
        }
        if (input == 0) {
            setState(4);
        }
        break;
    case 2:
        if (input == 4) {
            setState(0);
        }
        break;
    case 6:
        if (input == 4) {
            setState(0);
        }
        if (input == 1) {
            setState(1);
        }
        break;
    case 4:
        if (input == 4) {
            setState(5);
        }
        break;
    }
}

/* Returns whether `name` is the pane of one of the twelve keys. */
static BOOL isKeyPane(const char* name) {
    for (s32 i = 0; i < 12; i++) {
        if (hbmNameMatches(name, sButtonPaneNames[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

/* 0x80541FCC (0x470): Turns the events of the keypad's button panes into button inputs and pane events for the
 * keypad: presses, releases, focus changes and the auto-repeat and long press of the delete and space keys. */
void HkbCellphonePaneListener::onPaneEvent(HbmComponent* component, u32 kind, HbmPaneTrigger* trigger) {
    const char* name = ((HkbPaneController*)component)->mPane->mName;
    if (name[0] == 'B') {
        char window[17];
        hbmNameCopyWithChar(window, 17, name, 0, 'W');
        HkbCellphoneKeyButton* button = ((HkbCellphoneKeyButton*)mKeypad->getPaneAnim(window));
        if (button != NULL) {
            switch (kind) {
            case 4:
                if ((trigger->trigger & 0x10) || ((trigger->trigger & 0x40) && isKeyPane(name))) {
                    if (button->isRadio() == 1) {
                        nw4r::ut::List* buttons = mKeypad->getComponentList();
                        HkbCellphoneKeyButton* other = (HkbCellphoneKeyButton*)nw4r::ut::List_GetNext(buttons, NULL);
                        while (other != NULL) {
                            if (other != button && other->isRadio() == 1) {
                                other->setAnim(5);
                            }
                            other = (HkbCellphoneKeyButton*)nw4r::ut::List_GetNext(buttons, other);
                        }
                    }
                    button->setAnim(0);
                }
                break;
            case 1:
                button->setAnim(2);
                break;
            case 0:
                mpSound->play(4);
                mKeypad->setCursorTarget((u32)button->mpPane);
                button->setAnim(1);
                break;
            }
        }
    }
    HkbPaneEvent event = {name};
    if (kind == 4) {
        if (trigger->trigger & 0x10) {
            event.backward = 0;
            mKeypad->onPaneEvent(4, (const char*)&event);
        } else if ((trigger->trigger & 0x40) && isKeyPane(name)) {
            event.backward = 1;
            mKeypad->onPaneEvent(4, (const char*)&event);
            ((HkbPaneController*)component)->release(trigger->channel, 0);
        }
    }
    if (kind == 1) {
        mKeypad->onPaneEvent(1, (const char*)&event);
    }
    if (kind == 2 && (trigger->hold & 0x10) && !(trigger->trigger & 0x10) &&
        (hbmNameMatches(sButtonPaneNames[13], name) || hbmNameMatches(sButtonPaneNames[16], name)) &&
        ((HkbPaneController*)component)->isHeld(trigger->channel)) {
        u32 frames = mKeypad->getHoldFrames(trigger->channel, name);
        if (frames >= 30 && frames % 9 == 0) {
            char window[17];
            hbmNameCopyWithChar(window, 17, name, 0, 'W');
            ((HkbCellphoneKeyButton*)mKeypad->getPaneAnim(window))->setAnim(0);
            mKeypad->onPaneEvent(4, (const char*)&event);
        }
    }
    if (kind == 2) {
        if (mKeypad->getHoldFrames(trigger->channel, name) == 90) {
            event.long_press = 1;
            mKeypad->onPaneEvent(1, (const char*)&event);
        }
    }
}

/* 0x8054244C (0x8): Synchronises the text field. */
u32 HkbCellphoneKeypad::updateComponents() {
    return HbmPaneManager::updateComponents();
}

/* 0x80542454 (0x4): Does nothing. */
void HkbCellphoneInput::onIdle() {}

/* 0x80542458 (0x8): Returns whether caps is on. */
u8 HkbCellphoneInput::isCaps() {
    return caps;
}

/* 0x80542460 (0x14): Returns whether a key is waiting for its character to be committed. */
BOOL HkbCellphoneInput::hasPendingKey() {
    return pending_key != NULL;
}

/* 0x80542474 (0x8): Returns whether the predictive input is on. */
u8 HkbCellphoneInput::isPredicting() {
    return predicting;
}

/* 0x8054247C (0x8): Returns whether the alternate mode is on. */
u8 HkbCellphoneInput::isAltMode() {
    return alt_mode;
}

/* 0x80542484 (0x8): Returns whether the input is locked. */
u8 HkbCellphoneInput::isLocked() {
    return locked;
}

/* 0x8054248C (0x8): Returns that the input is available. */
s32 HkbCellphoneInput::isBusy() {
    return TRUE;
}

/* 0x80542494 (0x8): Sets the object that opens the symbol list. */
void HkbCellphoneKeypad::setSymbolTarget(HkbKeypadHost* target) {
    mSymbolTarget = target;
}

/* 0x8054249C (0x8): Sets the object that receives the committed text. */
void HkbCellphoneKeypad::setTextTarget(HkbKeypadHost* target) {
    mTextTarget = target;
}

/* 0x805424A4 (0x40): Activates the input, then resets the keypad. */
void HkbCellphoneKeypad::setEventSink(HkbEventSink* sink) {
    HkbInputWidget::setEventSink(sink);
    init();
}

/* 0x805424E4 (0x4): Does nothing; the keypad latches caps here. */
void HkbCellphoneInput::latchCaps(u8 on) {}

/* 0x805424E8 (0x4): Does nothing; the keypad selects the mode and redraws its tag buttons here. */
void HkbCellphoneInput::selectMode(s32 newMode) {}

/* 0x805424EC (0x4): Does nothing; the keypad closes its text field here. */
void HkbCellphoneInput::calcComponents() {}
