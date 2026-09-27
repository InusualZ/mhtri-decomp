/* enemy/fn_80171194.cpp
 *
 * Enemy-band translation unit at .text 0x80171194-0x80176C58 (46 functions, 23 236 B).
 *
 * Registered from `proposal/80171194_fn_80171194.cpp` (attribute.py queue): one maximal unclaimed run
 * whose left/right boundaries `tools/splits/tudiscover.py at 0x80171194` calls strong (2/1 anchors),
 * and whose extabindex run (0x80029790-0x80029928, 34 records) places every entry inside this .text
 * range. The unit owns `extab`/`extabindex`/`.text` (dtk's split added the `.ctors` word at
 * 0x8056F334-0x8056F338 on its own, so the TU has a static constructor); the .rodata/.data/.bss/
 * .sdata/.sbss/.sdata2 runs tudiscover lists are left unclaimed (they stay in the auto data objects) -
 * the neighbours (`enemy/fn_8014A1BC.c`, `enemy/fn_8013BE60.c`, `enemy/fn_8012BDF4.cpp`) claim exactly
 * those three sections.
 *
 * Module `enemy`: the nearest registered units are `enemy/fn_8014A1BC.c` (0x8014A1BC-0x801502C8) and the
 * rest of the `enemy` directory, and 11 of the run's mangled-undefined references are the enemy work
 * API (`em_frame_check__FP11_ENEMY_WORKUsff`, `em_get_mot_no__FP11_ENEMY_WORK`,
 * `get_em_chg_scale__FP11_ENEMY_WORK`, ...) plus nw4r::math helpers. Language C++: every call out of the
 * range is a mangled symbol (a C TU would reference them unmangled).
 *
 * Name: the map has only `fn_XXXXXXXX` for this range and the runtime dump has only `zz_` placeholders
 * (`.pi/notes/dumpmap-join.json`), so the map stem is kept as the file name.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked symbols.txt via the
 *   brief's section 3 inventory - all 46 symbols are fn_* - and the runtime dump's zz_ placeholders in
 *   .pi/notes/dumpmap-join.json; the .rodata/.data runs carry no source-file string)
 *
 * Object: `_ENEMY_WORK` (include/enemy.h, the union of every consumer's copy, size 0xB1C; the run's
 * mangled-undefined `em_*__FP11_ENEMY_WORK*` arguments name it, and the split's extabindex records
 * place all 34 exception entries inside this range). Every field this file names already carries an
 * offset and a name in that header, so nothing is redefined here (rule 1).
 *
 * Status.  7 of the 46 symbols are byte-identical and one more is above the bar (8/46 >= 80 %):
 *   `fn_80173264`, `fn_8017326C`, `fn_80173278`, `fn_80173280`, `fn_801740A0`, `fn_80176328`,
 *   `fn_80176374` 100.000 %; `fn_801740F4` 99.944 % (above the bar).  The other 38 are unwritten (0 %).
 *   This first batch is the `action_0x1E5`/`state_sub` dispatch group, which needs no header change.
 *
 * Residuals and what the next round needs (all measured against
 * build/RMHE08/obj/enemy/fn_80171194.o, `report generate` fuzzy_match_percent):
 *   - `fn_801740F4` 99.944 %.  Every case body, the jump table and its order match; the one difference is
 *     the range test - retail tests `cmplwi r0,13` where this source's 8 dense cases give `cmplwi r0,7`
 *     (cases 8..13 are the default block).  A `switch` whose expression has a 14-value enum/type range is
 *     the shape that produces the 13 bound (MWCC bounds by the type's range, not the written cases);
 *     naming that enum needs 14 state ids the code does not name, so it is recorded rather than guessed.
 *   - **Header gap (the big one).**  The 38 unwritten functions touch `_ENEMY_WORK` bytes the shared
 *     header still calls `pad_*`, so they cannot be written under rule 5 without naming them (and
 *     `include/enemy.h` is read-only here): `+0x1E4` (read as a byte by `fn_80176AA8`, next to
 *     `action_0x1E5`), `+0x32C..+0x333` (`fn_80176C30` writes `+0x328` as a float, then five bytes and a
 *     u16; the header's `+0x320 VEC3`/`+0x32C u16` do not fit the byte stores), `+0x328` again in
 *     `fn_801762A0` (a `sth`, so the header's `VEC3 v_0x320` is the wrong type there), and the `+0x9AC`
 *     effect queue the header already describes as `_ENEMY_QUEUE_ENTRY`.  The outbox carries the list as a
 *     `shared-file` config_request.
 *   - **Unsplit callees.**  The range calls 10 functions outside it that no shared header declares
 *     (`fn_80170A00`, `fn_80171130`, `fn_80131D84`, `fn_8012CF20`, `fn_8010562C`, `VEC3_ctor`,
 *     `fn_8011E6EC`, `fn_803B9BA0`, `em_parts_damage_level_get`, `fn_8012EC74`); all but the library ones
 *     are enemy-band and belong in `include/unsplit/enemy.h` (its own docstring says so).  That header is
 *     read-only here, so the declarations sit at the top of this file and the outbox carries them as a
 *     `shared-file` config_request.
 *   - **`.ctors`.**  dtk's own split added `.ctors 0x8056F334-0x8056F338` to the registration, so the TU
 *     has a static constructor; the static object it initialises is not reconstructed yet (it is part of
 *     the same pass as the `.rodata`/`.data` runs tudiscover lists, which are deliberately left to the
 *     auto data objects for now).
 */

#include "types.h"

#include "nw4r/math.h"
#include "enemy.h"

/* Retail keeps several folds this build's -O3 peephole fuses: it keeps `clrlwi` + `cmpwi` separate
 * where the peephole emits the record form `clrlwi.`.  Probed on `fn_80176328`/`fn_80176374`: with the
 * peephole on they measure 88.9/95.8 %, with it off 100/100 %, and the seven functions that were already
 * byte-identical stay at 100 % (the pragma is a per-unit deviation, docs/plan.md 8.2; the numbers are in
 * the outbox's `flags_probed`). */
#pragma peephole off

/* `em_parts_damage_level_get` is the one callee the map spells *mangled*, so it is declared as a C++
 * free function - outside the `extern "C"` block below - and the front-end then emits exactly the map's
 * `em_parts_damage_level_get__FP11_ENEMY_WORKUc` (rule 9).  It is the enemy-band damage-part query
 * `src/ef/eft009.cpp` also declares. */
u8 em_parts_damage_level_get(_ENEMY_WORK* enemy, u8 part);

/* ------------------------------------------------------------------------------------------------ *
 * This unit's own symbols (rule 2: they are declared where they are defined, i.e. here).
 *
 * The map's `fn_XXXXXXXX` stems are non-mangled symbols, so the definitions need C linkage - the same
 * spelling `include/enemy/fn_8012BDF4.h` uses for its unit's stems (`extern "C"`).
 *
 * Parameter widths: the map's mangled names encode the *map's* guess, not the original source's widths.
 * `fn_80176328`/`fn_80176374` take a `u8` in the mangling (`Uc`) but retail masks the incoming register
 * (`clrlwi r0,r4,24`) at the top - a `u8` parameter is never masked by this compiler - so their second
 * parameter is declared `u32` here and masked with an explicit `(u8)` cast at the compare, which is what
 * reproduces the `clrlwi` (the same trap `src/enemy/fn_8012BDF4.cpp` records for its `em_*` entries).
 * ------------------------------------------------------------------------------------------------ */

#ifdef __cplusplus
extern "C" {
#endif

void fn_80171194(_ENEMY_WORK* self, u8 kind, s32 arg);
void fn_80171744(_ENEMY_WORK* self, s32 arg);
void fn_80171EB8(_ENEMY_WORK* self, s32 arg);
void fn_801721B8(_ENEMY_WORK* self);
void fn_80172E10(_ENEMY_WORK* self);
void fn_80172E98(_ENEMY_WORK* self);
void fn_801731BC(_ENEMY_WORK* self);
void fn_80173264(_ENEMY_WORK* self);
void fn_8017326C(_ENEMY_WORK* self);
void fn_80173278(_ENEMY_WORK* self);
void fn_80173280(_ENEMY_WORK* self);
void fn_801732B0(_ENEMY_WORK* self);
void fn_8017394C(_ENEMY_WORK* self);
void fn_80173A04(_ENEMY_WORK* self);
void fn_80173C60(_ENEMY_WORK* self);
void fn_80173D04(_ENEMY_WORK* self);
void fn_80173FF4(_ENEMY_WORK* self);
void fn_801740A0(_ENEMY_WORK* self);

/* ------------------------------------------------------------------------------------------------ *
 * Callees outside this range.  They belong in `include/unsplit/enemy.h`; that header is read-only for
 * this round, so the declarations sit here with the parameter widths the call sites show.  The
 * functions at 0x80170A00/0x80171130 are the `action_0x1E5` handlers that live just below this range.
 * ------------------------------------------------------------------------------------------------ */

void fn_80170A00(_ENEMY_WORK* self);
void fn_80171130(_ENEMY_WORK* self);
void fn_803B9BA0(_ENEMY_WORK* self, VEC3* pos, s32 value);

/* ------------------------------------------------------------------------------------------------ *
 * The `state_sub` (+0x1E6) and `action_0x1E5` (+0x1E5) dispatchers.
 *
 * `fn_801740F4` is the outer one: it switches on `action_0x1E5`, and the entries 0 and 1 are the two
 * functions immediately below this range (0x80170A00/0x80171130), i.e. the tail of the same dispatch
 * table continues into the previous TU - which is why its left boundary is a "strong" cut.
 * ------------------------------------------------------------------------------------------------ */

void fn_80173264(_ENEMY_WORK* self) {
    fn_80171744(self, 2);
}

void fn_8017326C(_ENEMY_WORK* self) {
    fn_80171194(self, 10, 1);
}

void fn_80173278(_ENEMY_WORK* self) {
    fn_80171EB8(self, 0);
}

void fn_80173280(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80173264(self);
        break;
    case 1:
        fn_8017326C(self);
        break;
    case 2:
        fn_80173278(self);
        break;
    }
}

void fn_801740A0(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801732B0(self);
        break;
    case 1:
        fn_8017394C(self);
        break;
    case 2:
        fn_80173A04(self);
        break;
    case 3:
        fn_80173C60(self);
        break;
    case 4:
        fn_80173D04(self);
        break;
    case 5:
        fn_80173FF4(self);
        break;
    }
}

void fn_801740F4(_ENEMY_WORK* self) {
    switch (self->action_0x1E5) {
    case 0:
        fn_80170A00(self);
        break;
    case 1:
        fn_80171130(self);
        break;
    case 2:
        fn_801721B8(self);
        break;
    case 3:
        fn_80172E10(self);
        break;
    case 4:
        fn_80172E98(self);
        break;
    case 5:
        fn_801731BC(self);
        break;
    case 6:
        fn_80173280(self);
        break;
    case 7:
        fn_801740A0(self);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * The damage-part gates at the tail of the range.  Both read `team` (+0x003) and `pos` (+0x188,
 * `VEC3`) from the shared layout and ask `em_parts_damage_level_get` about a part.
 * ------------------------------------------------------------------------------------------------ */

int fn_80176328(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 1 && ((u8)em_parts_damage_level_get(self, 1) & 1) == 0) {
        return 1;
    }
    return 0;
}

void fn_80176374(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 0 && self->team == 14 && em_parts_damage_level_get(self, 0) == 1) {
        fn_803B9BA0(self, &self->pos, 100);
    }
}

#ifdef __cplusplus
}
#endif
