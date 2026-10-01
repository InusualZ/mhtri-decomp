/*
 * Pl/pl_hit_sphere.cpp - the player actor's box and sphere hit helpers (`hit_point_sphr` and the box tests before it).
 *
 * `.text` 0x8028F44C..0x8028F66C (4 functions), `.sdata2` 4 B, extab 0x18 B and extabindex 0x24 B.  Phase 4 recut of
 * `Pl/fn_80288CEC` (docs/splits/phase4): see `Pl/pl_motion.cpp` for the two-way split.
 *
 * Name: GUESS, from `hit_point_sphr`, the one named function of the range.
 * Flags: `cflags_pl` as the former unit.
 */

/* ==== recut from Pl/fn_80288CEC.cpp (0x8028F44C..0x8028F66C) ==== */
/*
 * Naming note: the symbol map spells 75 of this range's 78 functions as bare `fn_XXXXXXXX`
 * (checked with tools/symbols/dumpmap.py - every address of the range answers `zz_XXXXXXXX_` or
 * `FUN_XXXXXXXX`, never a real runtime name - and the range's `config/RMHE08/symbols.txt` entries
 * carry no signature).  The three the map does name keep the map's spelling
 * (`pl_motion_set__Fv`, `get_gm_daynight__Fv`,
 * `hit_point_sphr__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3f`).
 *
 * Rule 5/7 on the `_PLW` record: the two fields this unit reads arrived as `pl.h`'s `unk009`/`unk00C`
 * and are named there now - `+0x009` is `kind_0x09` (compared against 3 here; the sibling
 * `ef/eft019.cpp` names the same `_PLW` byte `kind_0x09`) and `+0x0C` is `act_no` (the action number
 * `Pl_act_ck` compares as its `u16` argument and the value this unit's act dispatchers switch on).
 *
 * Pl/fn_80288CEC.cpp - the player-mode / move-work TU of the `Pl` module.  `.text`
 * 0x80288CEC-0x8028F66C (78 functions, 0x697C B), extab 0x80012FFC-0x8001323C (72 records) and
 * extabindex 0x80030678-0x800309D8 (72 records); all three ranges are registered in splits.txt.
 * Nothing is claimed out of `.data`/`.sdata`/`.sdata2`: the jump tables and the two resource tables
 * this unit's dispatchers use are still emitted by the `auto_*` objects (invariant 8.4 - do not claim
 * a range our object does not emit).
 *
 * Extent - one TU, not a tiler guess.  The unit's own `.sdata2` run is 0x8079A270-0x8079A314 and it
 * *starts* at the left edge's first function (`fn_80288CEC` loads `lbl_8079A270` = 0.0f) while the
 * preceding proposal's run ends exactly there - a `.sdata2` run boundary is a TU boundary.  Reading
 * the run's first-use offsets in source order gives one strictly monotone authoring sequence (0x270,
 * 0x274 ... 0x2BC, 0x2C0, 0x2C8, 0x2D4 for the act half; then 0x2D8, 0x2DC, 0x2E8, 0x2F0, 0x2F8 ...
 * 0x310 for the rest), and `tools/units/attribute.py`'s `tu.open_seams` for this proposal records no
 * interior cut - only the right-edge jump `lbl_8079A310 -> lbl_8079A314`.
 *
 * Module (`Pl`) - class 3 of the brief's evidence order.  Class 1 fails: there is no `__FILE__`
 * string for this band at all - scanning `orig/RMHE08/sys/main.dol` for `[\\x20-\\x7e]{3,}\\.cpp`
 * yields 99 source names (g3d_*, ef_*, menu_*, enemy_control, cockpit, ...) and not one `pl_*`, so the
 * Pl files carry no assert strings.  Class 2 fails: `dumpmap.py lookup` answers a `zz_XXXXXXXX_`
 * placeholder for every address of the range except 0x8028F2C0, whose real name
 * (`get_gm_daynight`) the symbol map already carries.  Class 3 is conclusive: the unit operates on
 * the player work - `Pl_frame_check__FP4_PLWUlff`, `Pl_zanzo_set__FP4_PLWlUc`,
 * `Pl_act_ck__FP4_PLWUcUs` and `eft029_set_scale__FP4_PLWUcf` all take `_PLW*`, it *defines*
 * `pl_motion_set` (the player motion resource loader), and every accessor at its tail reads the
 * record `get_move_work_adrs(0)` hands back, which `src/enemy/fn_8012BDF4.cpp` documents as the
 * player-side root (`_PLAYER_ROOT`).  The nearest registered unit brackets the band on the `Pl` side:
 * `Pl/pl_act.cpp` ends at 0x8027D684, and the whole unclaimed band 0x8027D684-0x802B2978 is Pl
 * act/equip/skill/camera work.
 *
 * File name - class 4.  Nothing supports a *file* name: no `__FILE__` string (above), no
 * runtime-dump name, and the siblings' scheme (`pl_act`/`pl_master`/`pl_skill`, each from its
 * dominant `Pl_*_ck` symbol) has no dominant prefix here.  The unit is a mixture - act-style `_PLW`
 * state handlers (0x80288CEC-0x8028B330), the player scene/quest/move-work driver
 * (0x8028B330-0x8028EF30) and the root-work accessors plus the `hit_point_sphr` math helper
 * (0x8028EF30-0x8028F66C) - so naming it `pl_motion.cpp` from the single `pl_`-prefixed definition
 * would be inventing a name for a 78-function file.  The stem therefore stays the map's
 * `fn_80288CEC`, matching the two sibling class-4 registrations `Pl/fn_80229ECC.cpp` and
 * `Pl/fn_80241558.cpp`.
 *
 * Language: C++ - the map carries the mangled definitions `pl_motion_set__Fv`,
 * `get_gm_daynight__Fv` and `hit_point_sphr__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3f`, and the retail
 * object carries 72 extab/extabindex records (the Pl lib's `cflags_pl` sets -Cpp_exceptions on).
 *
 * Flags: `cflags_pl` (the lib's) - the act half reads `_PLW+0x0C`/`+0x565`/`+0x566` exactly like
 * `Pl/pl_act.cpp`, which settled `-O3 -inline noauto -opt nopeephole -Cpp_exceptions on` for the lib.
 * The scene half's `PlayMode_ck()` dispatchers were not re-probed.
 *
 * RESIDUAL (this pass).  Function-by-function scores are in the report; what is deliberately NOT
 * written is the set of dispatchers whose bodies are 100-450 instructions of jump-table state
 * machine: fn_80288E68, fn_8028912C, fn_8028921C, fn_802893D4, fn_80289518, fn_80289654,
 * fn_802896F8, fn_8028992C, fn_80289A60, fn_80289BD8, fn_80289CD4, fn_80289FF0, fn_8028A118,
 * fn_8028A1E8, fn_8028A2F8, fn_8028A400, fn_8028A62C, fn_8028A760, fn_8028A8D8, fn_8028A9D4,
 * fn_8028AC14, fn_8028ADB8, fn_8028AEB8, fn_8028AF50, fn_8028B0F4, fn_8028B198, fn_8028B298,
 * fn_8028B330, fn_8028B524, fn_8028BAA8, fn_8028BD98, fn_8028BF1C, fn_8028C168, fn_8028C468,
 * fn_8028C6CC, fn_8028CC7C, fn_8028CD3C, fn_8028CDB0, pl_warp_start, fn_8028D2DC, fn_8028D5F4,
 * fn_8028D778, fn_8028DDCC, fn_8028DF58, fn_8028E0B8, fn_8028E24C, fn_8028E3EC, fn_8028E528,
 * fn_8028E694, fn_8028E718, fn_8028EA84, fn_8028EC28, fn_8028EF7C's tail-call partner
 * `pl_motion_set` (needs the local string-object shape) and the remaining box builders
 * fn_8028F4B4/`558`'s exact float forms.  They are the next pass's work rather than guesses.
 * fn_8028F44C is byte-identical (100.0 %, 104 B): its target allocation is the typed-parameter
 * spelling (`PlBox* a`/`b`, `&b->vec_0x0C`), which spells the three field reaches through the
 * `PlBox` members and lets MWCC emit them as immediate offsets; the earlier `void*`-plus-cast
 * form (the one this pass originally landed) CSE'd the reaches into a saved register and left the
 * body at 116 B / 77.96154 %.
 * Two rule-1 follow-ups are named, not fixed: `_PL_ROOT` here and `_PLAYER_ROOT` in
 * `src/enemy/fn_8012BDF4.cpp` are the same record (the second user should move one definition into a
 * header), and `Pl_frame_check` has no registered owner, so its declaration belongs in
 * `include/unsplit/Pl.h` (not edited here to keep this batch to its own files).
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_80288CEC.h" /* `PlBox`, shared with `Pl/fn_8028F66C.cpp` (rule 1) */
#include "ef/fn_800CDB2C.h"
#include "enemy/em_pop.h" /* quest_flag_8_ck, quest_flag_80_ck (the owner's header, rule 2) */
#include "fn_80047398.h" /* `arena_userdata_apply` (rule 2) */
#include "quest/arenatask.h" /* `arena_user_data_buf` (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "quest/quest_entry.h" /* quest_monsters_release (the owner's header, rule 2) */

/* The player-mode root work `_PL_ROOT` (`get_move_work_adrs(0)`) is declared in `Pl/fn_80288CEC.h`, shared with
 * `hud/cockpit_quest.cpp` (rule 1). */

/* The object `fn_8028BCF4` releases: three `fn_800D8E44` handles, 4 bytes apart.
 * size: 0x144 (lower bound) */


/* ------------------------------------------------------------------------------------------------ *
 * Callees.
 *
 * The map spelled these with an argument list, so they are manglings and belong at C++ scope with
 * the real signature, so the front-end reproduces the map's name (rule 9).  The bare `fn_XXXXXXXX`
 * names are the map's own placeholders and stay `extern "C"`.
 * ------------------------------------------------------------------------------------------------ */

/* ef/fn_800CDB2C.cpp owns the pool accessors (`PlayMode_ck` is declared in that unit's header). */


/* Pl/pl_act.cpp owns `Pl_zanzo_set`; Pl/pl_master.cpp owns `Pl_act_ck` (`include/Pl/pl_master.h`). */


/* No registered unit covers these addresses; declared with the view this unit's call sites take. */



extern "C" {
/* `arena_userdata_apply` (0x8004A240) is `fn_80047398.cpp`'s and now comes from its owner header. */
f32 subVec3(void* dst, void* a, void* b);














}

/* The two small-data seeds `fn_8028F400` fills, and the 256-byte-stride table `fn_8028F1E8` indexes.
 * `setVec3` itself is declared by `include/ef.h` (pulled in by `pl.h`) as `(Vec*, f32, f32,
 * f32)`; re-declaring it here with `void*` is an illegal overload, so the call site casts. */
/* Two 0xC-byte records, still referenced by their own `lbl_` names (a `VEC3[]` view would fold
 * `lbl_806AB83C` into `lbl_806AB830 + 0xC` and lose the second relocation, measured 100 -> 77.84
 * on fn_8028F400). */


/* `arena_user_data_buf` (0x806E3E10) is `quest/arenatask.cpp`'s range and comes from its header. */

/* ------------------------------------------------------------------------------------------------ *
 * Bodies, in address order.
 * ------------------------------------------------------------------------------------------------ */

/* Every bare `fn_XXXXXXXX` is the map's own (unmangled) name, so the definitions carry C linkage;
 * `get_gm_daynight`/`hit_point_sphr` are mangled in the map and stay at C++ scope below. */
extern "C" {

/* 0x8028F44C - build the first 0x24 bytes of a box from two vectors and their cross product. */
void fn_8028F44C(PlBox* a, PlBox* b) {
    f32 cross[3];

    copyVec3(&b->vec_0x00, &a->vec_0x00);
    copyVec3(&b->vec_0x0C, &a->vec_0x0C);
    subVec3(cross, &a->vec_0x0C, &a->vec_0x00);
    copyVec3(&b->vec_0x18, (const nw4r::math::VEC3*)cross);
}

}


/* 0x8028F61C - `hit_point_sphr(point, center, radius)`: 1 when the point is inside the sphere. */
u32 hit_point_sphr(nw4r::math::VEC3* point, nw4r::math::VEC3* center, f32 radius) {
    f32 dx = center->x - point->x;
    f32 dy = center->y - point->y;
    f32 dz = center->z - point->z;
    return dx * dx + dy * dy + dz * dz <= radius * radius;
}
