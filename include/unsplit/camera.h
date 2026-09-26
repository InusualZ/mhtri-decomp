/*
 * Symbols of the camera band (`.text` from 0x802B5C58 on) that no registered unit owns yet
 * (docs/plan.md 6.5 rule 2).  The camera unit `camera/fn_802B5C58.cpp` consumes them; when the
 * neighbouring camera TUs register, each declaration moves to its owner's header.
 *
 * `fn_802BECD0` (0x802BECD0) is the camera-work accessor: it returns `lbl_806BB7E0` or
 * `lbl_806BB7E0 + 1272`, two 0x4F8-byte camera work slots.  Its own band is unclaimed (the registered
 * units bracketing it are `stage` below and `ai` above, i.e. different modules), so it has no owner
 * header to move to yet.
 */

#ifndef MHTRI_UNSPLIT_CAMERA_H
#define MHTRI_UNSPLIT_CAMERA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CamWork;

/* 0x802BECD0 - the camera work the whole band reads and writes. */
struct CamWork* fn_802BECD0(void);

#ifdef __cplusplus
}
#endif

/* `.sdata2` pool constants of the camera band (all inside 0x8079A4D8..0x8079A5FC).  Declared, never
 * defined: the pool belongs to the split, and defining the constants here would rebuild the section
 * (playbook 29). */
#ifdef __cplusplus
extern "C" {
#endif

extern const f32 lbl_8079A4D8; /* 0x8079A4D8 - the zero vector fn_802BE4FC/fn_802BE77C build a quake origin from */
extern const f32 lbl_8079A4EC; /* 0x8079A4EC - the follow interval fn_802BAB0C scales and fn_802BB0D4 blends with */
extern const f32 lbl_8079A56C; /* 0x8079A56C - fn_802B7980's timer threshold */

/* The eight 16-bit quake durations `fn_802BE44C` indexes by `kind & 7`.  In the unit's own `.data`
 * run (0x805CFBE8..0x805D39A0), which is not claimed yet, so it is declared rather than defined. */
extern const s16 lbl_805D1E7C[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_CAMERA_H */
