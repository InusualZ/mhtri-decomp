/*
 * homebutton/tiHKBManager.h - declarations of the symbols owned by `homebutton/tiHKBManager.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_TIHKBMANAGER_H
#define HOMEBUTTON_TIHKBMANAGER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char lbl_806582FC[];

extern char lbl_80658358[];

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
struct HkbManager;
struct PaneManager;
struct Component;

/* A cursor over the set key bits of the manager's channels: the word it scans (`kind` 0..3 selects the held,
 * pressed, released or repeated word), the bit (key slot) and channel it stands on and the key code it has looked up.
 * size: 0x0C */
struct HkbKeyIter {
    HkbKeyIter() {}
    HkbKeyIter(HkbManager* m, u8 k) : manager(m), kind(k), bit(-1), channel(0), code(0) {}

    inline bool isValid() const;
    inline u8 getUsage() const;
    inline u16 lookupCode(u8 usage);

    /* +0x00 */ HkbManager* manager;
    /* +0x04 */ u8 kind;
    /* +0x05 */ s8 bit;
    /* +0x06 */ u8 channel;
    /* +0x08 */ u16 code;
};

extern "C" {
HkbKeyIter hkbKeyIterSeek(HkbKeyIter* start);
HkbKeyIter hkbKeyIterBeginWord0(HkbManager* manager);
HkbKeyIter hkbKeyIterBeginWord1(HkbManager* manager);
HkbKeyIter hkbKeyIterBeginWord3(HkbManager* manager);
/* Returns whether the cursor stands on a set key bit. */
s32 hkbKeyIterValid(HkbKeyIter* it);
/* Returns the HID usage code of the key the cursor stands on. */
u8 hkbKeyIterGetUsage(HkbKeyIter* it);
/* Returns the character of the key the cursor stands on. */
u16 hkbKeyIterGetChar(HkbKeyIter* it);
/* Returns the character a key produces under the current modifiers. */
u16 hkbKeyIterGetCharForUsage(HkbKeyIter* it, u8 usage);
/* Returns the key code of the key the cursor stands on. */
u16 hkbKeyIterGetCode(HkbKeyIter* it);
/* Selects the key map of the manager's channel and re-sends its LEDs. */
void hkbManagerSetMap(HkbManager* self, s32 mapIndex);
/* Returns channel 0's modifier word with the masked bits replaced. */
u32 hkbGetModifierState(HkbManager* manager);
/* Sets the modifier bits `mask` of the manager's state to `value` and updates the keyboard LEDs. */
void hkbSetModifierState(HkbManager* manager, u32 value, u32 mask);
/* Sets channel 0's modifier override: the bits `mask` of the modifier word read as `value`. */
void hkbSetModifierOverride(HkbManager* manager, u32 value, u32 mask);
/* Returns the component of `self` whose pane is named `name`. */
Component* hkbGetPaneComponentByName(PaneManager* self, const char* name);

/* The keyboard manager singleton (.bss 0x80790D64). */
extern HkbManager hkbManager;
}
#endif

#endif
