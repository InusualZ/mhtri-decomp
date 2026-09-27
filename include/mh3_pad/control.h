/* The pad/control entry points of `src/mh3_pad.cpp`.
 *
 * Bracket note (docs/plan.md 6.5 rule 2): this file exists so a Pl unit can reach the pad query
 * without dragging the whole of `include/mh3_pad.h` in.  The clash it was filed for - `include/ef.h`
 * spelling `copyVec3`/`setVec3`/`VEC3_ctor` `VEC3*`/`Vec*` while `mh3_pad.h` spells them `void*`,
 * MWCC `(10197)` - is CLOSED: both headers now spell them identically.
 */
#ifndef MHTRI_MH3_PAD_CONTROL_H
#define MHTRI_MH3_PAD_CONTROL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80043B48 - the game's control-type query (`mh3_pad.cpp` defines it `extern "C"`); `0` selects
 * the first player. */
u8 get_ControlType(s32 which);

/* 0x80044B14 - the per-player mode/act setter the player act band calls. */
void fn_80044B14(u8 kind, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_CONTROL_H */
