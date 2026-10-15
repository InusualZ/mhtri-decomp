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

/* A cursor over the set key bits of the manager's channels: the word it scans (`kind` 0..3 selects channel
 * word +0x04..+0x10), the bit and channel it stands on and the number of bits it has passed.
 * size: 0x0C */
struct HkbKeyIter {
    HkbKeyIter() {}

    /* +0x00 */ HkbManager* manager;
    /* +0x04 */ u8 kind;
    /* +0x05 */ s8 bit;
    /* +0x06 */ u8 channel;
    /* +0x08 */ u16 count;
};

extern "C" {
HkbKeyIter hkbKeyIterSeek(const HkbKeyIter* start);
HkbKeyIter hkbKeyIterBeginWord1(HkbManager* manager);
HkbKeyIter hkbKeyIterBeginWord3(HkbManager* manager);
/* Returns whether the cursor stands on a set key bit. */
s32 hkbKeyIterValid(HkbKeyIter* it);
/* Returns the HID usage code of the key the cursor stands on. */
u8 hkbKeyIterGetUsage(HkbKeyIter* it);
/* Returns the character of the key the cursor stands on. */
u16 hkbKeyIterGetChar(HkbKeyIter* it);
/* Returns channel 0's modifier word with the masked bits replaced. */
u32 hkbGetModifierState(HkbManager* manager);
/* Sets the modifier bits `mask` of the manager's state to `value` and updates the keyboard LEDs. */
void hkbSetModifierState(HkbManager* manager, u32 value, u32 mask);
/* Sets the manager's key repeat words. */
void hkbSetKeyRepeat(HkbManager* manager, u32 delay, u32 interval);
/* Returns the component of `self` whose pane is named `name`. */
Component* hkbGetPaneComponentByName(PaneManager* self, const char* name);

/* The keyboard manager singleton (.bss 0x80790D64). */
extern HkbManager hkbManager;
}
#endif

#endif
