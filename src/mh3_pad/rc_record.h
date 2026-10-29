/* mh3_pad/rc_record.h - the record table and entry points of `mh3_pad/rc_record.c` (the SDK demo library's
 * GameCube pad poller, `DEMOPad.c`), for `mh3_pad.cpp`'s pad setup and frame read. */
#ifndef MHTRI_MH3_PAD_RC_RECORD_H
#define MHTRI_MH3_PAD_RC_RECORD_H

#include "types.h"
#include "PAD/PADClamp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One channel's polled status plus the edges and stick deltas since the previous read. */
typedef struct DEMOPadStatus {
    /* +0x00 */ PADStatus pst;
    /* +0x0C */ u16 buttonDown;     /* buttons pressed since the previous read */
    /* +0x0E */ u16 buttonUp;       /* buttons released since the previous read */
    /* +0x10 */ u16 dirs;           /* the DEMO_STICK_* / DEMO_SUBSTICK_* direction bits */
    /* +0x12 */ u16 dirsNew;        /* direction bits set since the previous read */
    /* +0x14 */ u16 dirsReleased;   /* direction bits cleared since the previous read */
    /* +0x16 */ s16 stickDeltaX;
    /* +0x18 */ s16 stickDeltaY;
    /* +0x1A */ s16 substickDeltaX;
    /* +0x1C */ s16 substickDeltaY;
} DEMOPadStatus; /* size: 0x1E */

/* 0x807419C8: the four channels' records. */
extern DEMOPadStatus DemoPad[4];

/* 0x80795048: the number of channels whose last read answered. */
extern u32 DemoNumValidPads;

/* 0x804A4D70: reads and clamps the four GameCube pads into the DemoPad table. */
void DEMOPadRead(void);
/* 0x804A4E40: initialises the GameCube pads and clears the DemoPad table. */
void DEMOPadInit(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_RC_RECORD_H */
