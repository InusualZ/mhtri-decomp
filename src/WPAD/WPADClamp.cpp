/* WPAD/WPADClamp.cpp - the stick and accelerometer clamps of the Wii remote driver (the SDK's WPADClamp.c).
 * RANGE. .text 0x804F8380-0x804F8EC0 (5 functions); .sdata2 0x8079D430-0x8079D478.  Seam with WPAD/WPADHIDParser.cpp: the two
 *   .sdata2 pools hold the same constants twice (the double 4330000080000000 at 0x8079D418 and 0x8079D460, 0.0f at 0x8079D3E0
 *   and 0x8079D46C, -1.0f at 0x8079D408 and 0x8079D470).  Seams with WPAD/WPADEncrypt.cpp at 0x804F8380 and with
 *   WPAD/WPADMem.cpp at 0x804F8EC0: the SDK file list, codegen-neutral (no call crosses either seam; this unit calls the WPAD.c
 *   functions `WPADGetDataFormat` and `WPADGetAccGravityUnit` out of line).
 * FLAGS. `cflags_base` plus `-fp_contract off` (configure.py): the target has no fused multiply-add and keeps unfused
 *   fmuls->fadds pairs in `wpadClampAxes` (2) and `WPADClampAcc` (12 candidates); the written bodies emit no fused op under
 *   either setting.
 * NAMES. GUESS: the unit name (SDK spelling), `WPADClampStick`, `WPADClampAcc` (SDK names: game code calls them after
 *   `WPADRead`), `wpadStickDeadzoneSquare`, `wpadStickDeadzoneCircle` (the dead zone of a stick pair on 8-bit or 16-bit axes),
 *   `wpadClampAxes` (the accelerometer axes against a full-scale limit).
 * RESIDUALS. Written: 3 of 5 rows, 1 of them matches; 2 rows are not attempted (0x804F8380 (0x158), 0x804F84E0 (0x558)).
 *   `wpadStickDeadzoneSquare` keeps r31 for the sign copy where the target does not, and `wpadClampAxes` differs in the float
 *   register numbering and in the .sdata2 constant names.  Flip blockers: .text (2 unwritten rows) and .sdata2 (the pool the
 *   unwritten bodies load).
 */

#include "types.h"
#include "WPAD/WPADClamp.h"
#include "MSL/sqrt.h"

/* Applies a square dead zone to the stick (x, y) and scales the pair back inside the octagon of radius `outer`; `wide` selects 16-bit axes. */
void wpadStickDeadzoneSquare(WPADStickAxis* x, WPADStickAxis* y, s32 outer, s32 scale, s32 dead, s32 wide)
{
    s32 vx;
    s32 vy;
    s32 signX;
    s32 signY;
    s32 scaledX;
    s32 scaledY;
    s32 denominator;
    s32 numerator;

    if (wide == 0) {
        vx = x->narrow;
        vy = y->narrow;
    } else {
        vx = x->wide;
        vy = y->wide;
    }
    if (vx >= 0) {
        signX = 1;
    } else {
        signX = -1;
        vx = -vx;
    }
    if (vy >= 0) {
        signY = 1;
    } else {
        signY = -1;
        vy = -vy;
    }
    vx = vx <= dead ? 0 : vx - dead;
    vy = vy <= dead ? 0 : vy - dead;
    if (vx == 0 && vy == 0) {
        if (wide == 0) {
            x->narrow = 0;
            y->narrow = 0;
        } else {
            x->wide = 0;
            y->wide = 0;
        }
        return;
    }
    scaledX = scale * vx;
    scaledY = scale * vy;
    if (scaledY <= scaledX) {
        numerator = scale * outer;
        denominator = scaledX + vy * (outer - scale);
        if (numerator < denominator) {
            if (wide == 0) {
                vx = (s8)((vx * numerator) / denominator);
                vy = (s8)((vy * numerator) / denominator);
            } else {
                vx = (s16)((vx * numerator) / denominator);
                vy = (s16)((vy * numerator) / denominator);
            }
        }
    } else {
        numerator = scale * outer;
        denominator = scaledY + vx * (outer - scale);
        if (numerator < denominator) {
            if (wide == 0) {
                vx = (s8)((vx * numerator) / denominator);
                vy = (s8)((vy * numerator) / denominator);
            } else {
                vx = (s16)((vx * numerator) / denominator);
                vy = (s16)((vy * numerator) / denominator);
            }
        }
    }
    if (wide == 0) {
        x->narrow = signX * vx;
        y->narrow = signY * vy;
    } else {
        x->wide = signX * vx;
        y->wide = signY * vy;
    }
}

/* Applies a circular dead zone to the stick (x, y), shrinking the pair to the radius `limit` when it is longer; `wide` selects 16-bit axes. */
void wpadStickDeadzoneCircle(WPADStickAxis* x, WPADStickAxis* y, s32 limit, s32 dead, s32 wide)
{
    s32 vx;
    s32 vy;
    s32 lengthSq;

    if (wide == 0) {
        vx = x->narrow;
        vy = y->narrow;
    } else {
        vx = x->wide;
        vy = y->wide;
    }
    if (-dead < vx && vx < dead) {
        vx = 0;
    } else if (vx > 0) {
        vx = vx - dead;
    } else {
        vx = vx + dead;
    }
    if (-dead < vy && vy < dead) {
        vy = 0;
    } else if (vy > 0) {
        vy = vy - dead;
    } else {
        vy = vy + dead;
    }
    lengthSq = vx * vx + vy * vy;
    if (limit * limit < lengthSq) {
        s32 length = (s32)(f32)sqrt((f32)lengthSq);
        vx = (vx * limit) / length;
        vy = (vy * limit) / length;
    }
    if (wide == 0) {
        x->narrow = vx;
        y->narrow = vy;
    } else {
        x->wide = vx;
        y->wide = vy;
    }
}

/* Clamps the three accelerometer axes to `limit` times their full-scale value, keeping each sign. */
void wpadClampAxes(s16* x, s16* y, s16* z, WPADAxisScale* scale, f32 limit)
{
    s16 fullScaleX = scale->x;
    f32 nx = (f32)*x / (f32)fullScaleX;
    f32 signX = 1.0f;
    f32 signY = signX;
    f32 signZ = signX;
    f32 ny = (f32)*y / (f32)scale->y;
    f32 nz = (f32)*z / (f32)scale->z;

    if (nx < 0.0f) {
        signX = -1.0f;
        nx = -nx;
    }
    if (ny < 0.0f) {
        signY = -1.0f;
        ny = -ny;
    }
    if (nz < 0.0f) {
        signZ = -1.0f;
        nz = -nz;
    }
    if (nx > limit) {
        nx = limit;
    }
    if (ny > limit) {
        ny = limit;
    }
    if (nz > limit) {
        nz = limit;
    }
    *x = (s16)(nx * signX * (f32)fullScaleX);
    *y = (s16)(ny * signY * (f32)scale->y);
    *z = (s16)(nz * signZ * (f32)scale->z);
}
