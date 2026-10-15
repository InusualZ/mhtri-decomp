/*
 * homebutton/hkb_widget_tail.h - declarations of the symbols owned by `homebutton/hkb_widget_tail.cpp` that other units call or read.
 */
#ifndef HOMEBUTTON_HKB_WIDGET_TAIL_H
#define HOMEBUTTON_HKB_WIDGET_TAIL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
void  fn_8056D814(void* obj, void* owner);

u8 lbl_80794750[];

#ifdef __cplusplus
}

/* The USB keyboard's lock and shift state: the caps lock and the two shift keys, one bit each.  size: 0x1 */
class HkbKbdState {
public:
    HkbKbdState();
    ~HkbKbdState();
    void setCapsLock(const u8& bCapsLock);

    /* Returns whether a shift key is held. */
    bool isShiftHeld() const { return mbLeftShift || mbRightShift; }

    /* +0x0 */ u8 mbCapsLock : 1;
    /* +0x0 */ u8 mbLeftShift : 1;
    /* +0x0 */ u8 mbRightShift : 1;
}; /* size: 0x1 */

/* Returns the USB keyboard's state, built on first use. */
inline HkbKbdState& hkbGetKbdState() {
    static HkbKbdState sState;
    return sState;
}
#endif

#endif
