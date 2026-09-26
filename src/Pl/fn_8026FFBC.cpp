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
 * The `.sdata2` operands are the pooled constants the target object references: `lbl_8079A008` is the
 * signed int->float magic MWCC synthesises for the two `(f32)(s16)` casts (it stays implicit, so the
 * object carries its own copy and the relocation is unnamed - see the residual below), `lbl_8079A02C`
 * is the 0.4f threshold and is declared here as the load operand so the relocation pairs by name.
 *
 * Residual: none in the code.  The two relocation *names* that no source spelling can produce - the
 * compiler-synthesised `.sdata2` magic (`@117` here, `lbl_8079A008` in the map) and the compiler-emitted
 * extab record (`@119` vs `@etb_800126CC`) - differ from the map's names, and the official report metric
 * counts those rows equal (`functionRelocDiffs: none`), so `fn_8026FFBC` measures 100.0 % against the
 * target object with the unit's `cflags_pl`.
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
