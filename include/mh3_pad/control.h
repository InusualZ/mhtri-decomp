/* The pad/control entry points of `src/mh3_pad.cpp`.
 *
 * Bracket note (docs/plan.md 6.5 rule 2): `include/mh3_pad.h` is the same unit's header, but its
 * three `fn_80041E40`/`fn_80041E8C`/`fn_80043EA8` prototypes spell those helpers `void*` while
 * `include/ef.h` (pulled in by `include/pl.h`) spells them `VEC3*`; a unit that needs both headers
 * cannot take two parameter types for one function.  This file carries the two declarations that
 * have no such overlap, so a Pl unit can reach the pad query without dragging the whole header in.
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
