/*
 * `ef/eft053.cpp`'s own header.
 *
 * It carries NO declarations any more: every callee comes from its owner's header.
 * `calcVecDistXZ`, `addVec3`, `fn_80050850` and `vec_to_mh_vec3` are `fn_8004CAD8.h`'s,
 * and `copyVec3` (0x80041E40, `src/mh3_pad.cpp`) now comes from `mh3_pad.h` - the
 * `(10197)` clash this file's comment used to record (`ef.h` spelling `VEC3_ctor`/`setVec3` as
 * `VEC3*`/`Vec*` against `mh3_pad.h`'s `void*`) is closed: both headers spell them identically.
 * The `80366618` lane's measured 16 `FAILED` targets came from declaring `copyVec3` in
 * `fn_8004CAD8.h`, which is not its owner (`symbols.txt` + `splits.txt` place 0x80041E40 in
 * `mh3_pad.cpp`'s range, and stylelint rule 2 agrees).
 */
#ifndef MHTRI_EF_EFT053_H
#define MHTRI_EF_EFT053_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"

/* `copyVec3`, `calcVecDistXZ`, `addVec3`, `fn_80050850` and `vec_to_mh_vec3` all come from their
 * owners' headers now (`mh3_pad.h` and `fn_8004CAD8.h`), which this unit includes. */

#endif /* MHTRI_EF_EFT053_H */
