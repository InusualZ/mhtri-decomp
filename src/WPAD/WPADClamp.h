/* WPAD/WPADClamp.h - the stick and accelerometer clamps `WPAD/WPADClamp.cpp` owns (docs/plan.md 6.5 rule 2). */
#ifndef WPAD_WPADCLAMP_H
#define WPAD_WPADCLAMP_H

#include "types.h"
#include "WPAD/wpad.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One stick axis as the extension report stores it: a byte or a halfword, selected by the caller's `wide` flag. size: 0x02 */
typedef union WPADStickAxis {
    /* +0x00 */ s8 narrow;
    /* +0x00 */ s16 wide;
} WPADStickAxis;

/* The full-scale value of each accelerometer axis. size: 0x06 */
typedef struct WPADAxisScale {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 z;
} WPADAxisScale;

/* 0x804F8380 / 0x804F84E0 - clamp the sticks and the accelerometer of a status record read by `WPADRead`. */
void WPADClampStick(s32 chan, WPADStatus* status, u32 type);
void WPADClampAcc(s32 chan, WPADStatus* status, u32 type);
/* 0x804F8A40 / 0x804F8C00 - dead zone and range limit of a stick pair; 0x804F8D30 - clamps the accelerometer axes. */
void wpadStickDeadzoneSquare(WPADStickAxis* x, WPADStickAxis* y, s32 outer, s32 scale, s32 dead, s32 wide);
void wpadStickDeadzoneCircle(WPADStickAxis* x, WPADStickAxis* y, s32 limit, s32 dead, s32 wide);
void wpadClampAxes(s16* x, s16* y, s16* z, WPADAxisScale* scale, f32 limit);

#ifdef __cplusplus
}
#endif

#endif /* WPAD_WPADCLAMP_H */
