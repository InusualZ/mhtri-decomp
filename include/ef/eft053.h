/*
 * `ef/eft053.cpp`'s own header.
 *
 * It carries ONE declaration now, and it is the last one this unit cannot take from an owner's
 * header: `fn_80041E40` belongs to `src/mh3_pad.cpp`, and `include/mh3_pad.h` collides with
 * `include/ef.h` on `fn_80043EA8`/`fn_80041E8C` (MWCC `(10197)`, probe-measured), so an `ef.h`
 * consumer cannot include it.
 *
 * The four helpers this header used to carry - `fn_80050F80`, `fn_80051378`, `fn_80050850` and
 * `vec_to_mh_vec3` - are declared by their owner's header `include/fn_8004CAD8.h`, which this unit
 * includes.  The `80366618` lane's measured 16 `FAILED` targets came from declaring
 * `fn_80041E40` in `fn_8004CAD8.h`, which is not its owner (`symbols.txt` + `splits.txt` place
 * 0x80041E40 in `mh3_pad.cpp`'s range, and stylelint rule 2 agrees); with it in
 * `include/mh3_pad.h` the whole tree builds 0 FAILED.
 */
#ifndef MHTRI_EF_EFT053_H
#define MHTRI_EF_EFT053_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"

/* 0x80041E40 is owned by `src/mh3_pad.cpp`, and its header cannot be included from this unit
 * (`include/ef.h` declares `fn_80043EA8`/`fn_80041E8C` with different parameter types than
 * `include/mh3_pad.h`, MWCC (10197) - measured, and carried as a shared-file request), so this
 * one declaration stays here, normalised to the owner body (`void*` return).  Everything else
 * this header used to carry now comes from its owner's header `include/fn_8004CAD8.h`, which this
 * unit includes: `fn_80050F80`, `fn_80051378`, `fn_80050850`, `vec_to_mh_vec3`. */
void* fn_80041E40(void* dst, const void* src);

#endif /* MHTRI_EF_EFT053_H */
