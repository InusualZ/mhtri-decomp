/*
 * The player work's health-ratio gate (`fn_8026FFBC`, .text 0x8026FFBC-0x80270018, ONE function,
 * 92 B) with its own exception tables - extab 0x800126CC-0x800126D4, extabindex
 * 0x8002F8B0-0x8002F8BC.  It sits on the seam between `Pl/pl_master.cpp` and `Pl/pl_skill.cpp` and is
 * registered on its own: both neighbours are registered units whose ranges end/start exactly here.
 *
 * Home is `Pl`: the argument is the player work `_PLW*` - the two callers, both in `Pl/pl_skill.cpp`
 * (0x802701E8, 0x8027060C), pass the same record they hand `Pl_Skill_ck(_PLW*, u16)` and gate the
 * result on `Pl_cat_skill_ck`/`Pl_Skill_ck`.
 *
 * rule 7 deferred: the symbol map has only `fn_8026FFBC` for this range and no evidence names it -
 * `dumpmap.py lookup 0x8026FFBC` answers the placeholder `zz_026ffbc_`, the function reads no string
 * (its only data operands are the two `.sdata2` pool words below) and the surrounding pools carry no
 * `__FILE__` name for it.  The behaviour is a ratio test, not a named API of its neighbours' scheme.
 *
 * The `.sdata2` operands are the pooled constants the target object references.  `lbl_8079A02C` is the
 * 0.4f threshold, declared here as an external load operand so the relocation pairs by name: it is the
 * unit's own (private) constant and no other registered unit reads that address.
 *
 * `lbl_8079A008` (0x4330000080000000, the signed int->float magic MWCC synthesises for the two
 * `(f32)(s16)` casts) is a **shared** pool entry and stays unclaimed:
 *   * the address is loaded by 28 `lfd`s in 9 functions of four registered units (`Pl/fn_802693C4.cpp`,
 *     this unit, `Pl/pl_skill.cpp`, `Pl/fn_80273B14.cpp`), and a pool entry is local to its TU - so those
 *     four are fragments of **one** original TU whose pool 0x8079A008 is part of.
 *   * claiming `.sdata2 0x8079A008-0x8079A010` here keeps `main.dol` green while the unit is
 *     `NonMatching`, but the flip cannot link: `undefined: 'lbl_8079A008'`, referenced from
 *     `fn_802751B4` in `fn_80273B14.o` - the linked objects keep the map's global name, our pool entry is
 *     the local `@519`.
 *   * naming the symbol from source is not reachable either: the constant is compiler-synthesised, and
 *     `-pool off`, `-sdata2 0` and `-str reuse` all leave the local 8-byte entry in the object
 *     (measured on this unit's command line).
 *
 * Residual: none in the code - `fn_8026FFBC` is 100.0 % over all 23 rows with the unit's `cflags_pl`.  The
 * unit cannot be flipped until the original TU is one registered unit again (the four fragments above) or
 * the build post-processes the pool entry into the map's global name; the compiler-emitted extab record's
 * name (`@etb_800126CC` in the map) is a relocation-name difference the report metric counts equal.
 */

#include "types.h"
#include "pl.h"

extern "C" {
extern f32 lbl_8079A02C;
}

/* Whether the actor's health is at or below the 0.4 threshold (the low-health gate the skill code
 * arms its attack modifiers with). */
extern "C" BOOL fn_8026FFBC(_PLW* self)
{
    return (f32)self->health / (f32)self->health_max <= lbl_8079A02C;
}
