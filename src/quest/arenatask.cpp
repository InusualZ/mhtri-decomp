/*
 * quest/arenatask.cpp - the arena task band: the arena mode's own `.brres` texture load, its runtime
 * acdata -> `_arena_eq_data` conversion, the task `ArenaSelExec` (0x80041A34) installs, and the
 * per-player arena setup the task drives.
 *
 * `.text` 0x804459E4..0x80448404 (19 functions, 10784 B), extab 0x8001DE9C..0x8001DF24,
 * extabindex 0x8003EB20..0x8003EBEC.  Module `quest` and file name `arenatask.cpp` are **class-1
 * evidence**: `.data` 0x80607390 is the bare `__FILE__` string "arenatask.cpp", it has exactly one
 * copy in the DOL and its only referrer is 0x80445B3C inside this range's head `arena_resource_load`
 * (0x804459E4), so that function and this band are one TU.
 *
 * SEAM - left edge 0x804459E4, right edge 0x80448404; the internal extent is NOT proven:
 *   - left: 0x804459E4 starts the band's own `.data` (0x80607210) and `.sdata` (0x80793B28) runs -
 *     the object before it ends its `.data` exactly at 0x80607210 (a run of 16-bit tables, no label
 *     of this band's in between) - and the 8-byte accessor `fn_804459DC` before it reads
 *     `arena_lsp_data_adrs`, whose neighbour `que_info` this band's `arena_quest_info_build`
 *     (0x80445B80) fills, so 0x804459DC is a candidate *first* function and 0x804459E4 the candidate
 *     second one.  Nothing before 0x804459DC is this band's.
 *   - right: 0x80448404 begins the save-file module - it and its 8 neighbours (0x80448404..0x804490F0)
 *     share the private `.bss` path buffer `lbl_806E40C0` (0x806E40C0, 0x1B0 B) and call `strcpy`/
 *     `OSReport`/`NAND*`, and their own file-scope `.data` static sits at 0x806073F0.  The function
 *     before it, `arena_camera_light_vec_init` (0x80448358), writes this band's two `.bss` vector
 *     arrays, so the cut is exactly there.
 *   - UNPROVEN internal cut: 0x80446990.  Under that cut the band's `.data` per-function run
 *     (0x806073B8 `arena_player_init`'s float array, 0x806073E0 `arena_light_init`'s GXColor quad) and
 *     its `.bss` run (0x806E4078 camera pair, 0x806E4090 light quad) tile a *second* object exactly;
 *     under the registered (wider) extent they are the tail of this one.  The data alone cannot
 *     separate the two readings - see the outbox - and this pass registers the maximal run, which is
 *     also what the flanking registrations do.
 *
 * SEAM EVIDENCE THAT IS NOT USABLE HERE (re-measured this pass, worth recording for the playbook):
 * `python tools/splits/tudiscover.py at 0x804459E4 --window 48` has NO strong cut at either registered
 * edge - the left edge (cut 15567 = 0x804459E4) scores share 0.005 and the right edge (cut 15586 =
 * 0x80448404) share 0.003, both with no pin at all.  Its strong left candidate is a cut 23 functions
 * earlier (15544 = 0x804437FC, inside the charmake band), and its only strong candidates inside the
 * band rest on one `.sdata2` pin (`lbl_8079C96C -> lbl_8079C970`): the higher of that pin's two
 * admissible cuts (15579 = 0x80446B8C) is VETOED by this unit's own must-link `link(15578,15585)`
 * (`arena_zero_f` and `arena_50f` are used by both `arena_camera_init` (15578) and
 * `arena_camera_light_vec_init` (15585), so no cut inside that span is legal), and the pin's other end
 * (15578) sits inside the band as well.  So the `.sdata2` run-jump test is unusable FOR THIS BAND
 * because of *within-unit adjacency* - two adjacent private pool words, each used by exactly one
 * function, in function order: both cited labels are referenced by `quest/arenatask` ALONE
 * (`lbl_8079C96C` by `arena_camera_init`, `lbl_8079C970` by `arena_light_init`), so the pair is not an
 * instance of a cross-object merge either.  Both pins are only producible on a graph that still holds
 * this band: after a rename the dump is stale, every renamed function is dropped (`outside_map: 1709`)
 * and the tool still prints boundaries, so `python tools/splits/dump_asm.py` comes first (outbox).
 *
 * The phenomenon that IS measured, and does make a shared `.sdata2` label no evidence of a common TU:
 * of the 7245 `.sdata2` labels, 640 are cited by more than one registered unit's target object (a
 * relocation scan of all 282 `build/RMHE08/obj/<unit>.o`) - e.g. `menu/arena_result` at VA 0x803B1044
 * and `quest/quest_entry` at 0x803AF684 both execute `C822EA88` = `lfd f1, -5496(r2)`, the same
 * address 0x8079DAA0 - 0x1578 = 0x8079C528.  What is NOT measured is a mechanism: "MWLD merges
 * identical `.sdata2` constants across objects" is contradicted by the section itself (the 8-byte
 * int->double magic 0x4330000080000000 has 170 copies in `.sdata2`, only 13 cited by more than one
 * unit), so a single-copy private pool word cannot be *assumed* shared.  The extabindex staircase is
 * monotone across the whole region (every entry 8 B, function addresses rising), so it yields the
 * exact per-function inventory but no cut.
 *
 * DATA (CLAIMED this pass, one run per section, in `config/RMHE08/splits.txt` - rule 12: the unit that
 * uses the bytes claims the range and matches them as its own object).  All five runs are byte-identical
 * to the target's, this unit is the only registered *declarer* of every symbol in them, and the
 * declarations moved with the claim into this unit's own header `include/quest/arenatask.h` (rule 2),
 * so the band header `include/unsplit/arena.h` now holds only the band's record types.  This unit's
 * object does not emit the bytes yet, so `datagap.py --unit quest/arenatask --mode both` reports
 * `ours-extra` 0 with the `target-extra` rows as the residual the body pass closes:
 *   .data   0x80607210..0x806073F0 - the `04/arena.brres` name, the 20 arena texture names that fit
 *           in `.data`, the 32-entry pointer table (`lbl_80607310`, slot 31 NULL), the `__FILE__`
 *           string (0x80607390) and the assert message (0x806073A0, 0x18, "NW4R:Failed assertion 0")
 *           - the hole at 0x80607390 is this unit's own, all three are named by `arena_resource_load`'s
 *           own `.rela.text` - then `arena_player_init`'s ten-float array (0x806073B8) and
 *           `arena_light_init`'s GXColor quad (0x806073E0).
 *   .sdata  0x80793B28..0x80793B88 - the resource record (`lbl_80793B28`, {0x54E800, 0x80607210}) plus
 *           the eleven <=8-byte names the pointer table's slots 17..24 and 28..30 point at (the name
 *           at 0x80793B88 is not in the table).
 *   .sdata2 0x8079C960..0x8079C998 - this unit's own pool fragment: the eleven renamed `arena_*f` plus
 *           `lbl_8079C968` (1.0f), `lbl_8079C96C` (10000.0f) and `lbl_8079C970` (0x1a1a1aff).
 *           Claimable by playbook 23's own condition - declare-never-define: our object emits none of
 *           it.  It ends at 0x8079C998 because the 8-byte magic there is the NEXT band's: four
 *           functions of the following band (0x804522E0, 0x804529E0, 0x804535A4, 0x80454978) cite it
 *           and nothing in this band does.
 *   .bss    0x806E4010..0x806E40C0 - `arena_work` (0x58), `arena_draw_func` (0x10),
 *           `arena_camera_vec` (0x18) and `arena_light_vec` (0x30).  The run starts at 0x806E4010 and
 *           NOT at the 0x200 eq-data buffer below it (0x806E3E10..0x806E4010, `lbl_806E3E10`, cited by
 *           `arena_eqdata_from_userdata`/`arena_eqdata_from_vsuser`/`arena_task`/`arena_result_next`):
 *           that buffer is also read by `Pl/fn_80288CEC.cpp`'s `fn_8028F1E8` (`lis/addi` of
 *           `lbl_806E3E10` + `(self[8] << 8)`, a 256-byte stride), and that file *declares* it
 *           (`extern u8 lbl_806E3E10[]`), so claiming it here would turn another unit's declaration
 *           into a rule-2 finding - the claim waits for the body that names it and for Pl's
 *           declaration to move to this header.  Two of the four claimed words are *read* by other
 *           registered units (`arena_work`+0x04 by `Pl/fn_8028EA84`, `arena_draw_func[2]` by
 *           `menu/menu_result.cpp`'s `fn_8039806C`), but neither file declares either symbol, so the
 *           owner stays this unit - measured with `tools/units/callers.py <addr>` and the target
 *           objects' `.rela.text`.
 *   .sbss   0x80794D3C..0x80794D50 - `que_info` (0x80794D3C) and `arena_lsp_data_adrs` (0x80794D40,
 *           the map's 0x10-byte row).  The start is 4-mod-8: dtk only WARNs (`Alignment for
 *           quest/arenatask.cpp .sbss expected 8, but starts at 10:0x80794D3C`), the same class as the
 *           eleven `.data` starts the tree already carries, and widening it to 0x80794D38 would take
 *           `arena_multi_str`, another band's word.  dtk *does* refuse a range that ends inside a
 *           symbol (`... ends within symbol 'arena_lsp_data_adrs'`), which is why the end is
 *           0x80794D50 and not 0x80794D44.  The remaining `lbl_` rows' renames and the bodies ride the
 *           later pass.
 *
 * NAMES: the band's `.text` names come from the code and the dump; the two `.bss` vector names
 * (`arena_camera_vec`, `arena_light_vec`) are GUESSES from the bodies that write them - the dump names
 * the block (`arena_work`, `arena_draw_func`) but not the vectors.
 *
 * RESIDUAL: 6 of the 19 bodies are written (740 B of 10784, see the outbox for the measured
 * percentages); the other 13 are unwritten, 12 of them *blocked by naming* - each calls an unrenamed
 * neighbour (`fn_803B1EAC`, `fn_8027EFB4`/`fn_8027E7BC`/`fn_8027E72C`/`fn_8004C4F0`,
 * `fn_8004A240`, `fn_8030B790`, `fn_8042C844`/`fn_8042C850`/`fn_802D9EA4`, twelve nw4r `ResFile`
 * helpers, `_savegpr_23`'s siblings) whose rename is a cross-unit sweep this lane must not perform.
 * `menu/arena_result.cpp`'s own header records the same wall for this band's neighbours.
 *
 * DATA RESIDUAL: the 0x200 eq-data buffer at 0x806E3E10 is the one band word left unclaimed, because
 * `src/Pl/fn_80288CEC.cpp` declares it (`extern u8 lbl_806E3E10[]`) and claiming it would add a rule-2
 * finding in that file (the gate's `--diff` cannot see it: `src/` findings are compared only for the
 * files the batch changes) - the lane that writes the bodies naming it claims the run and moves that
 * declaration into this header.  `arena_lsp_data_adrs`'s map row is 0x10 bytes while the declaration is
 * a 4-byte pointer; the flip has to settle that width.
 *
 * FLAGS: `cflags_menu` (the `main` module's menu band: `-O3 -inline noauto -opt nopeephole
 * -Cpp_exceptions on`, Wii/1.3) - the same lib and group as the band's link neighbours
 * `quest/quest_entry.cpp` and `menu/arena_result.cpp`.  Every function of the range with a frame
 * carries an extab record (17 of the 19: the target's `.relaextabindex` names every band function
 * except the two leaf setters `arena_eqdata_head_set` and `arena_sub_mode_set`), which
 * `-Cpp_exceptions on` explains in a C TU as well, so the presence of extab is not language evidence
 * here; the file is C++ because the range's own `dl_acdata_to_ar_eqdata` is defined by a *mangled* map
 * name (`dl_acdata_to_ar_eqdata__FP14_arena_eq_dataUc`), i.e. the original was C++.
 *
 * FLIP: the target object owns a 4-byte `.ctors` (0x8056F3D0, the claimed range) whose relocation
 * names `arena_camera_light_vec_init` (`.rela.ctors` off=0 type=1 sym=arena_camera_light_vec_init;
 * the DOL word there is 0x80448358) - a body already at 100 %.  The claim is right; the obligation is
 * a source shape, not a missing body: the flip needs the source to declare the file-scope object
 * whose constructor body that function is, so MWCC emits the word and its relocation.
 */

#include "types.h"
#include "nw4r/math.h"
#include "quest/arenatask.h"    /* the owner's own header: this band's data + records (rule 2) */
#include "ef/fn_800CDB2C.h"     /* `my_player_no` (the owner's header, rule 2) */
#include "mh3_pad/vec3.h"       /* `setVec3` (the owner header, rule 2) */
#include "menu/get_pop_dat_ptr.h"  /* `get_arena_cfg` (the owner header, rule 2) */

/* The arena eq-data record pair this band's three writers walk (0x80446C34/0x80446D40/0x80446DDC).
 * Declared here: the owner of `arena_eqdata_apply` is this TU, so its prototype lives with it. */
extern "C" void arena_eqdata_apply(ArenaEqParams* params, ArenaEqSlot* slot, u8 kind, u8 index);

/* ---- the band's own two allocator-free helpers (0x80445D40, 0x80446D40) ---- */

/* Copies the id byte and the u16 that follows it out of one source record into a head. */
extern "C" void arena_eqdata_head_set(ArenaEqDataHead* dst, const u16* src) {
    dst->id_0x00 = (u8)src[0];
    dst->value_0x02 = src[1];
}

/* ---- the arena work block's own setters (0x804463B0, 0x80445D58) ---- */
/* Stores the sub-mode the task compares against 3 before it hands over to `GameModeExec`. */
extern "C" void arena_sub_mode_set(u8 mode) {
    arena_work.sub_mode_0x4E = mode;
}

/* Records the equip index a *remote* player picked, and marks it dirty; the local player's own
 * choice never comes through here.  The work block's address is taken before the query so it lives
 * in a callee-saved register across the call, as the target's r31 does. */
extern "C" void arena_other_player_eq_set(u8 player, u8 equip) {
    ArenaWork* work = &arena_work;
    if (player != (s8)my_player_no()) {
        work->other_eq_dirty_0x37 = 1;
        work->other_eq_0x2C = equip;
    }
}

/* Clears one equip slot and refills its `kind`/`state` halves: the state byte comes from the owner's
 * +0x04 mode, then `arena_eqdata_apply` walks the slot to the end of its own bookkeeping. */
extern "C" void arena_eqdata_reset(ArenaEqParams* params, ArenaEqSlot* slot, u8 index) {
    slot->player_0x00 = 0;
    slot->mode_0x01 = 0;
    slot->clear_0x02 = 0;
    slot->state_0x03 = 0;
    slot->unused_0x04 = 0;
    slot->unused_0x05 = 0;
    slot->pair_a_0x06 = 0;
    slot->pair_b_0x07 = 0;
    slot->value_0x12 = 0;
    slot->flag_0x0B = 0;
    if (params->vs_mode_0x04 == 2) {
        slot->state_0x09 = 3;
    } else {
        slot->state_0x09 = 4;
    }
    arena_eqdata_apply(params, slot, 0, index);
    slot->unused_0x0A = 0;
    slot->value_0x0E = 0;
    slot->value_0x10 = 0;
    slot->pair_0x0C = 0;
}

/* ---- the arena parameter block's own mode setup (0x80446DDC) ---- */

/* Loads one of the arena's three parameter presets: the mode selects how many equip slots exist and
 * which of the two Vs configurations each of them takes. */
extern "C" void arena_eqdata_setup(ArenaEqParams* params, u8 mode) {
    params->mode_0x00 = mode;
    params->field_0x01 = 0;
    params->field_0x02 = 0;
    params->field_0x03 = 0;
    switch (mode) {
    case 0:
        arena_eqdata_reset(params, &params->slot_0x14, 0);
        params->slot_0x14.mode_0x01 = 3;
        params->slot_0x14.mirror_0x0D = get_arena_cfg(0, 7);
        params->slot_0x2C.mirror_0x0D = get_arena_cfg(1, 7);
        params->value_0x44 = 0;
        params->value_0x46 = 0;
        break;
    case 1:
        arena_eqdata_reset(params, &params->slot_0x14, 0);
        params->slot_0x14.pair_a_0x06 = 5;
        params->slot_0x14.pair_b_0x07 = 2;
        params->value_0x44 = 0;
        params->value_0x46 = 0;
        break;
    case 2:
        arena_eqdata_reset(params, &params->slot_0x14, 0);
        params->slot_0x14.mode_0x01 = 4;
        arena_eqdata_reset(params, &params->slot_0x2C, 1);
        params->slot_0x2C.mode_0x01 = 4;
        params->value_0x44 = 0;
        params->value_0x46 = 0;
        params->value_0x48 = 0x1518;
        break;
    default:
        break;
    }
}

/* ---- the arena scene's default camera/light placement (0x80448358) ---- */

/* Writes the six default vectors the arena camera and the four arena lights start from. */
extern "C" void arena_camera_light_vec_init(void) {
    setVec3(&arena_camera_vec[0], arena_zero_f, arena_85f, arena_440f);
    setVec3(&arena_camera_vec[1], arena_zero_f, arena_75f, arena_zero_f);
    setVec3(&arena_light_vec[0], arena_zero_f, arena_zero_f, arena_50f);
    setVec3(&arena_light_vec[1], arena_n50f, arena_70f, arena_20f);
    setVec3(&arena_light_vec[2], arena_70f, arena_n30f, arena_n20f);
    setVec3(&arena_light_vec[3], arena_zero_f, arena_100f, arena_zero_f);
}
