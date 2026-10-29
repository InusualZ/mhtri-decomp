/*
 * mh3_pad/rc_record.c - the SDK demo library's GameCube pad poller (`DEMOPad.c`) that fills the DemoPad table.
 *
 * RANGE. `.text` 0x804A4BC0-0x804A4F20; `.data` 0x806184F8-0x80618508; `.bss` 0x80741998-0x80741A40; `.sbss`
 *   0x80795048-0x80795050 (3 functions / 0x344 B).
 *   - `DEMOPadCopy`, `DEMOPadRead`, `DEMOPadInit` in address order; the pad reader, `PADClamp` and `PADReset` for
 *     four channels fill the 0x1E-byte `DemoPad` records that `mh3_pad.cpp`'s `rc_get_*` accessors read
 *   - `.data` 0x806184F8 (`PadChanMask`), `.bss` 0x80741998 (`Pad`, the PADStatus copy) and `.sbss` 0x80795048
 *     (`DemoNumValidPads`) are read only here
 *   - the left edge (0x804A4BC0) follows `DBPrintf`; the right edge is the DSP library's first function
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`, `cflags_base`: -O4,p, functions 16-aligned).
 * NAMES. the SDK demo library's names (`DEMOPadCopy`/`DEMOPadRead`/`DEMOPadInit`, `DemoPad`, `DemoNumValidPads`,
 *   `Pad`, `PadChanMask`, the `DEMOPadStatus` fields): the code is that file statement for statement (the +-48
 *   stick threshold, the 0x4000..0x100 direction bits, the error-code tests). The unit's file name is a GUESS kept
 *   from registration (`DEMO/DEMOPad.c` is the real one).
 * RESIDUALS. none in `.text` (3/3 functions byte-identical, relocations equal); `.sbss` emits 4 of the claimed 8 B
 *   (the tail is alignment fill before the next 8-aligned section), `.text` 0x354 of 0x360 B (the tail is link padding).
 */

#include "types.h"
#include "PAD/pad.h"
#include "mh3_pad/rc_record.h"

#define PAD_ERR_NONE 0
#define PAD_ERR_NO_CONTROLLER (-1)
#define PAD_ERR_TRANSFER (-3)

#define PAD_CHAN0_BIT 0x80000000
#define PAD_CHAN1_BIT 0x40000000
#define PAD_CHAN2_BIT 0x20000000
#define PAD_CHAN3_BIT 0x10000000

#define DEMO_STICK_THRESHOLD 48

#define DEMO_STICK_UP 0x1000
#define DEMO_STICK_DOWN 0x2000
#define DEMO_STICK_LEFT 0x4000
#define DEMO_STICK_RIGHT 0x8000
#define DEMO_SUBSTICK_UP 0x0100
#define DEMO_SUBSTICK_DOWN 0x0200
#define DEMO_SUBSTICK_LEFT 0x0400
#define DEMO_SUBSTICK_RIGHT 0x0800

#define PADButtonDown(buttonLast, button) ((u16)(((buttonLast) ^ (button)) & (button)))
#define PADButtonUp(buttonLast, button) ((u16)(((buttonLast) ^ (button)) & (buttonLast)))

static PADStatus Pad[4];
DEMOPadStatus DemoPad[4];
u32 DemoNumValidPads;

static u32 PadChanMask[4] = { PAD_CHAN0_BIT, PAD_CHAN1_BIT, PAD_CHAN2_BIT, PAD_CHAN3_BIT };

/* 0x804A4BC0 (0x1A4): copies one channel's status into its record with the button/direction edges and stick deltas. */
static void DEMOPadCopy(PADStatus* pad, DEMOPadStatus* dmpad)
{
    u16 dirs;

    if (pad->err != PAD_ERR_TRANSFER) {
        dirs = 0;
        if (pad->stickX < -DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_STICK_LEFT;
        }
        if (pad->stickX > DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_STICK_RIGHT;
        }
        if (pad->stickY < -DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_STICK_DOWN;
        }
        if (pad->stickY > DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_STICK_UP;
        }
        if (pad->substickX < -DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_SUBSTICK_LEFT;
        }
        if (pad->substickX > DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_SUBSTICK_RIGHT;
        }
        if (pad->substickY < -DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_SUBSTICK_DOWN;
        }
        if (pad->substickY > DEMO_STICK_THRESHOLD) {
            dirs |= DEMO_SUBSTICK_UP;
        }

        dmpad->dirsNew = PADButtonDown(dmpad->dirs, dirs);
        dmpad->dirsReleased = PADButtonUp(dmpad->dirs, dirs);
        dmpad->dirs = dirs;

        dmpad->buttonDown = PADButtonDown(dmpad->pst.button, pad->button);
        dmpad->buttonUp = PADButtonUp(dmpad->pst.button, pad->button);

        dmpad->stickDeltaX = (s16)(pad->stickX - dmpad->pst.stickX);
        dmpad->stickDeltaY = (s16)(pad->stickY - dmpad->pst.stickY);
        dmpad->substickDeltaX = (s16)(pad->substickX - dmpad->pst.substickX);
        dmpad->substickDeltaY = (s16)(pad->substickY - dmpad->pst.substickY);

        dmpad->pst = *pad;
    } else {
        dmpad->dirsNew = dmpad->dirsReleased = 0;
        dmpad->buttonDown = dmpad->buttonUp = 0;
        dmpad->stickDeltaX = dmpad->stickDeltaY = 0;
        dmpad->substickDeltaX = dmpad->substickDeltaY = 0;
    }
}

/* 0x804A4D70 (0xCC): reads and clamps the four pads, counts the answering ones and resets the missing ones. */
void DEMOPadRead(void)
{
    s32 i;
    u32 ResetReq = 0;

    PADRead(Pad);
    PADClamp(Pad);
    DemoNumValidPads = 0;
    for (i = 0; i < 4; i++) {
        if (Pad[i].err == PAD_ERR_NONE || Pad[i].err == PAD_ERR_TRANSFER) {
            ++DemoNumValidPads;
        } else if (Pad[i].err == PAD_ERR_NO_CONTROLLER) {
            ResetReq |= PadChanMask[i];
        }
        DEMOPadCopy(&Pad[i], &DemoPad[i]);
    }
    if (ResetReq) {
        PADReset(ResetReq);
    }
}

/* 0x804A4E40 (0xD4): initialises the pad library and clears every field of the four records. */
void DEMOPadInit(void)
{
    s32 i;

    PADInit();
    for (i = 0; i < 4; i++) {
        DemoPad[i].pst.button = 0;
        DemoPad[i].pst.stickX = 0;
        DemoPad[i].pst.stickY = 0;
        DemoPad[i].pst.substickX = 0;
        DemoPad[i].pst.substickY = 0;
        DemoPad[i].pst.triggerL = 0;
        DemoPad[i].pst.triggerR = 0;
        DemoPad[i].pst.analogA = 0;
        DemoPad[i].pst.analogB = 0;
        DemoPad[i].pst.err = 0;
        DemoPad[i].buttonDown = 0;
        DemoPad[i].buttonUp = 0;
        DemoPad[i].dirs = 0;
        DemoPad[i].dirsNew = 0;
        DemoPad[i].dirsReleased = 0;
        DemoPad[i].stickDeltaX = 0;
        DemoPad[i].stickDeltaY = 0;
        DemoPad[i].substickDeltaX = 0;
        DemoPad[i].substickDeltaY = 0;
    }
}
