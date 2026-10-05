/*
 * Symbols of the camera band (`.text` from 0x802B5C58 on) that no registered unit owns yet
 * (docs/plan.md 6.5 rule 2).  The camera unit `camera/fn_802B5C58.cpp` consumes them; when the
 * neighbouring camera TUs register, each declaration moves to its owner's header.
 *
 * `fn_802BECD0` (0x802BECD0) used to be declared here as `struct CamWork*`; `light/light.cpp` owns
 * that address and defines the record's accessor over its own view of the 0x4F8 bytes
 * (`LightWork`), so the declaration lives in `light/light.h` - included below - and this
 * header re-exports it (the `(10505) illegal overloading` the two spellings would have been).
 */

#ifndef MHTRI_UNSPLIT_CAMERA_H
#define MHTRI_UNSPLIT_CAMERA_H

#include "types.h"
/* The owner's header for the 0x802BECD0 accessor (docs/plan.md 6.5 rule 2). */
#include "light/light.h"

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
