/*
 * PAD/PADClamp.c - the PAD stick clamp: the stick clamp helper and `PADClamp`.
 * RANGE. .text 0x804D80B0-0x804D82D0 (2 functions).  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence:
 *    `PADClamp` is the map's name for the 0xF0-byte function at 0x804D81E0 and `ClampStick` (0x804D80B0) is its only callee;
 *    neither reads PAD state (.bss 0x8074E3B0, first read by the next unit).
 * FLAGS. `cflags_os` (configure.py), the flags of the unit it was cut from.
 * NAMES. GUESS: the file name (from `PADClamp`); medium evidence for the right edge.  `ClampStick` is the SDK's name for
 *    the helper (map row renamed from its generated name).
 * RESIDUALS. none recorded.
 * SHAPES. the stick limits are the controller constants 72/40 (main stick), 59/31 (C stick) and 15 (dead zone); the
 *    triggers lose 30 and cap at 180.
 */

#include "types.h"
#include "PAD/PADClamp.h"

#define PAD_CHANNELS 4
#define PAD_STICK_MAX 72
#define PAD_STICK_MID 40
#define PAD_STICK_MIN 15
#define PAD_SUBSTICK_MAX 59
#define PAD_SUBSTICK_MID 31
#define PAD_TRIGGER_MIN 30
#define PAD_TRIGGER_MAX 180

/* 0x804D80B0 (0x130): removes the dead zone from a stick pair and pulls it inside the limit of the controller's octagon. */
static void ClampStick(s8* px, s8* py, s8 max, s8 xy, s8 min)
{
    s32 x = *px;
    s32 y = *py;
    s32 signX;
    s32 signY;
    s32 xyMax;
    s32 d;

    if (x >= 0) {
        signX = 1;
    } else {
        signX = -1;
        x = -x;
    }
    if (y >= 0) {
        signY = 1;
    } else {
        signY = -1;
        y = -y;
    }
    if (x <= min) {
        x = 0;
    } else {
        x -= min;
    }
    if (y <= min) {
        y = 0;
    } else {
        y -= min;
    }
    if (x == 0 && y == 0) {
        *px = *py = 0;
        return;
    }
    if (xy * y <= xy * x) {
        xyMax = xy * max;
        d = xy * x + y * (max - xy);
        if (xyMax < d) {
            x = (s8)(x * xyMax / d);
            y = (s8)(y * xyMax / d);
        }
    } else {
        xyMax = xy * max;
        d = xy * y + x * (max - xy);
        if (xyMax < d) {
            x = (s8)(x * xyMax / d);
            y = (s8)(y * xyMax / d);
        }
    }
    *px = signX * x;
    *py = signY * y;
}

/* 0x804D81E0 (0xF0): clamps the sticks and triggers of the four controllers that answered. */
void PADClamp(PADStatus* status)
{
    s32 i;

    for (i = 0; i < PAD_CHANNELS; i++, status++) {
        if (status->err != 0) {
            continue;
        }
        ClampStick(&status->stickX, &status->stickY, PAD_STICK_MAX, PAD_STICK_MID, PAD_STICK_MIN);
        ClampStick(&status->substickX, &status->substickY, PAD_SUBSTICK_MAX, PAD_SUBSTICK_MID, PAD_STICK_MIN);
        if (status->triggerL <= PAD_TRIGGER_MIN) {
            status->triggerL = 0;
        } else {
            if (status->triggerL > PAD_TRIGGER_MAX) {
                status->triggerL = PAD_TRIGGER_MAX;
            }
            status->triggerL -= PAD_TRIGGER_MIN;
        }
        if (status->triggerR <= PAD_TRIGGER_MIN) {
            status->triggerR = 0;
        } else {
            if (status->triggerR > PAD_TRIGGER_MAX) {
                status->triggerR = PAD_TRIGGER_MAX;
            }
            status->triggerR -= PAD_TRIGGER_MIN;
        }
    }
}
