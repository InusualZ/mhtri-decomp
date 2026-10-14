/* WPAD/WPADClamp.cpp - the stick and accelerometer clamps of the Wii remote driver (the SDK's WPADClamp.c).
 * RANGE. .text 0x804F8380-0x804F8EC0 (5 functions); .sdata2 0x8079D430-0x8079D478.  Seam with WPAD/WPADHIDParser.cpp: the two
 *   .sdata2 pools hold the same constants twice (the double 4330000080000000 at 0x8079D418 and 0x8079D460, 0.0f at 0x8079D3E0
 *   and 0x8079D46C, -1.0f at 0x8079D408 and 0x8079D470).  Seams with WPAD/WPADEncrypt.cpp at 0x804F8380 and with
 *   WPAD/WPADMem.cpp at 0x804F8EC0: the SDK file list, codegen-neutral (no call crosses either seam; this unit calls the WPAD.c
 *   functions `WPADGetDataFormat` and `WPADGetAccGravityUnit` out of line).
 * FLAGS. `cflags_base` plus `-fp_contract off` (configure.py): the target has no fused multiply-add; with
 *   `-fp_contract on` the `WPADClampAcc` length sums fuse into fmadds.
 * NAMES. GUESS: the unit name (SDK spelling), `WPADClampStick`, `WPADClampAcc` (SDK names: game code calls them after
 *   `WPADRead`), `wpadStickDeadzoneSquare`, `wpadStickDeadzoneCircle` (the dead zone of a stick pair on 8-bit or 16-bit axes),
 *   `wpadClampAxes` (the accelerometer axes against a full-scale limit), `WPADStickClampParam` and the four `.sdata2` tables
 *   `wpadFsStickClampPlay`/`wpadFsStickClamp`/`wpadClStickClampPlay`/`wpadClStickClamp` (0x8079D430..0x8079D450: the
 *   arguments `WPADClampStick` passes for the Nunchuk and Classic sticks, with and without the dead zone).
 * RESIDUALS. `wpadStickDeadzoneSquare` keeps the two magnitudes in r12/r31 where the
 *   target keeps them in r0/r9 (no declaration order moves it), and `wpadClampAxes` numbers the y/z quotients f5/f6 where the
 *   target reuses f4/f2.  `wpadClampAxes` takes its scale by value in the target (both callers copy the 6-byte gravity unit
 *   before the call): `wpadClampAccBox` makes that copy.  The map still names the `.sdata2` rows `lbl_8079D430..lbl_8079D470`.
 *   Flip blockers: .text (`wpadStickDeadzoneSquare`, `wpadClampAxes`).
 */

#include "types.h"
#include "WPAD/WPADClamp.h"
#include "MSL/sqrt.h"


/* size: 0x08 - the limits of one stick clamp: the dead zone, the octagon's outer and scale terms and the circle radius. */
typedef struct WPADStickClampParam {
    /* +0x00 */ s16 dead;
    /* +0x02 */ s16 outer;
    /* +0x04 */ s16 scale;
    /* +0x06 */ s16 radius;
} WPADStickClampParam; /* size: 0x08 */

/* The Nunchuk stick limits with and without the dead zone. */
static const WPADStickClampParam wpadFsStickClampPlay = { 15, 67, 43, 56 };
static const WPADStickClampParam wpadFsStickClamp = { 0, 82, 58, 75 };

/* The Classic Controller stick limits with and without the dead zone. */
static const WPADStickClampParam wpadClStickClampPlay = { 60, 288, 188, 248 };
static const WPADStickClampParam wpadClStickClamp = { 0, 348, 248, 322 };

/* Clamps the sticks of a status record: `type` 0 and 2 keep a dead zone, 0 and 1 clamp to an octagon, 2 and 3 to a circle. */
void WPADClampStick(s32 chan, WPADStatus* status, u32 type)
{
    const WPADStickClampParam* param;

    if (status->err != 0) {
        return;
    }
    switch (WPADGetDataFormat(chan)) {
    case 3:
    case 4:
    case 5: {
        WPADFSStatus* fs = (WPADFSStatus*)status;

        if (type == 0 || type == 2) {
            param = &wpadFsStickClampPlay;
        } else {
            param = &wpadFsStickClamp;
        }
        if (type <= 1) {
            wpadStickDeadzoneSquare((WPADStickAxis*)&fs->fsStickX, (WPADStickAxis*)&fs->fsStickY, param->outer, param->scale,
                                    param->dead, 0);
        } else {
            wpadStickDeadzoneCircle((WPADStickAxis*)&fs->fsStickX, (WPADStickAxis*)&fs->fsStickY, param->radius, param->dead, 0);
        }
        break;
    }
    case 6:
    case 7:
    case 8: {
        WPADCLStatus* cl = (WPADCLStatus*)status;

        if (type == 0 || type == 2) {
            param = &wpadClStickClampPlay;
        } else {
            param = &wpadClStickClamp;
        }
        if (type <= 1) {
            wpadStickDeadzoneSquare((WPADStickAxis*)&cl->clLStickX, (WPADStickAxis*)&cl->clLStickY, param->outer, param->scale,
                                    param->dead, 1);
            wpadStickDeadzoneSquare((WPADStickAxis*)&cl->clRStickX, (WPADStickAxis*)&cl->clRStickY, param->outer, param->scale,
                                    param->dead, 1);
        } else {
            wpadStickDeadzoneCircle((WPADStickAxis*)&cl->clLStickX, (WPADStickAxis*)&cl->clLStickY, param->radius, param->dead, 1);
            wpadStickDeadzoneCircle((WPADStickAxis*)&cl->clRStickX, (WPADStickAxis*)&cl->clRStickY, param->radius, param->dead, 1);
        }
        break;
    }
    }
}

/* Clamps three accelerometer axes to `limit` times their one-G reading per axis (a box). */
static inline void wpadClampAccBox(s16* x, s16* y, s16* z, WPADAccGravityUnit unit, f32 limit)
{
    wpadClampAxes(x, y, z, (WPADAxisScale*)&unit, limit);
}

/* Scales three accelerometer axes back onto the sphere of radius `limit` (in one-G units) when they leave it. */
static inline void wpadClampAccSphere(s16* x, s16* y, s16* z, WPADAccGravityUnit unit, f32 limit)
{
    f32 fx = (f32)*x / (f32)unit.x;
    f32 fy = (f32)*y / (f32)unit.y;
    f32 fz = (f32)*z / (f32)unit.z;

    f32 lengthSq = fx * fx + fy * fy + fz * fz;

    if (limit * limit < lengthSq) {
        f32 length = sqrt(lengthSq);
        fx = fx * limit / length;
        fy = fy * limit / length;
        fz = fz * limit / length;
    }
    *x = (s16)(fx * (f32)unit.x);
    *y = (s16)(fy * (f32)unit.y);
    *z = (s16)(fz * (f32)unit.z);
}

/* Clamps the accelerometers of a status record: `type` 0 clamps each axis, otherwise the vector length. */
void WPADClampAcc(s32 chan, WPADStatus* status, u32 type)
{
    WPADAccGravityUnit unit;
    WPADAccGravityUnit fsUnit;

    if (status->err != 0) {
        return;
    }
    switch (WPADGetDataFormat(chan)) {
    case 1:
    case 2:
        WPADGetAccGravityUnit(chan, 0, &unit);
        if (type == 0) {
            wpadClampAccBox(&status->accX, &status->accY, &status->accZ, unit, 3.4f);
        } else {
            wpadClampAccSphere(&status->accX, &status->accY, &status->accZ, unit, 3.4f);
        }
        break;
    case 4:
    case 5: {
        WPADFSStatus* fs = (WPADFSStatus*)status;

        WPADGetAccGravityUnit(chan, 0, &unit);
        WPADGetAccGravityUnit(chan, 1, &fsUnit);
        if (type == 0) {
            wpadClampAccBox(&status->accX, &status->accY, &status->accZ, unit, 3.4f);
            wpadClampAccBox(&fs->fsAccX, &fs->fsAccY, &fs->fsAccZ, fsUnit, 2.1f);
        } else {
            wpadClampAccSphere(&status->accX, &status->accY, &status->accZ, unit, 3.4f);
            wpadClampAccSphere(&fs->fsAccX, &fs->fsAccY, &fs->fsAccZ, fsUnit, 2.1f);
        }
        break;
    }
    }
}

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
