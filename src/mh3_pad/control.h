/* The pad/control entry points of `src/mh3_pad.cpp`.
 *
 * Bracket note (docs/plan.md 6.5 rule 2): this file exists so a Pl unit can reach the pad query
 * without dragging the whole of `mh3_pad.h` in.  The clash it was filed for - `ef.h`
 * spelling `copyVec3`/`setVec3`/`VEC3_ctor` with a different record type than the owner's header,
 * MWCC `(10197)` - is CLOSED: every header now spells them `nw4r::math::VEC3*` (rule 11).
 */
#ifndef MHTRI_MH3_PAD_CONTROL_H
#define MHTRI_MH3_PAD_CONTROL_H

#include "types.h"

#ifdef __cplusplus
/* 0x80043B48 - the game's control-type query; `0` selects the first player.  C++ linkage: the map row is
 * `get_ControlType__Fl`, the mangling of `mh3_pad.cpp`'s definition. */
s32 get_ControlType(s32 which);
/* 0x800438C8 - the Classic Controller buttons of pad `chan` that went down (C++ linkage: `get_rcSwitch_on__Fl`). */
u16 get_rcSwitch_on(s32 chan);

extern "C" {
#endif

/* 0x80044B14 - the per-player mode/act setter the player act band calls. */
void pad_motor_start_level(u8 kind, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_CONTROL_H */
