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
 * SEAM EVIDENCE THAT IS NOT USABLE HERE (re-measured this pass): `python tools/splits/tudiscover.py at
 * 0x804459E4 --window 48` has NO strong cut at either registered edge - the left edge (cut 15567 =
 * 0x804459E4) scores share 0.005, the right edge (cut 15586 = 0x80448404) share 0.003, neither with a
 * pin at all.  Its strong left candidate is a cut 23 functions earlier (15544 = 0x804437FC, inside the
 * charmake band); its only strong in-band candidates rest on one `.sdata2` pin, now named
 * (`arena_camera_far_z -> arena_ambient_color`, 0x8079C96C -> 0x8079C970), and the higher of that
 * pin's two admissible cuts (15579 = 0x80446B8C) is VETOED by this unit's own must-link
 * `link(15578,15585)` (`arena_zero_f` and `arena_50f` are used by both `arena_camera_init` (15578) and
 * `arena_camera_light_vec_init` (15585), so no cut inside that span is legal), while the pin's other
 * end (15578) is in-band as well.  So the run-jump test is unusable FOR THIS BAND - *within-unit
 * adjacency*: two adjacent private pool words, each used by exactly one neighbouring function, in
 * function order, and both cited by `quest/arenatask` ALONE.  Either pin needs a graph that still
 * holds this band: after a rename the dump is stale, every renamed function is dropped
 * (`outside_map: 1709`) and the tool still prints boundaries, so `python tools/splits/dump_asm.py`
 * comes first (outbox).
 *
 * The measurement behind that caveat, kept here without the argument it used to carry (the request to
 * amend `docs/matching.md` section 23 is filed in the outbox): of the 7245 `.sdata2` labels, 640 are
 * cited by more than one registered unit's target object (a relocation scan of all 282
 * `build/RMHE08/obj/<unit>.o`) - e.g. `menu/arena_result` at VA 0x803B1044 and `quest/quest_entry` at
 * 0x803AF684 both execute `C822EA88` = `lfd f1, -5496(r2)`, the same address 0x8079DAA0 - 0x1578 =
 * 0x8079C528.  The extabindex staircase is monotone across the whole region (every entry 8 B,
 * function addresses rising), so it yields the exact per-function inventory but no cut.
 *
 * DATA (CLAIMED, in `config/RMHE08/splits.txt` - rule 12: the unit that uses the bytes claims the
 * range and matches them as its own object).  All seven runs are byte-identical to the target's, this
 * unit is the only registered *declarer* of every symbol in them, and the declarations moved with the
 * claim into this unit's own header `include/quest/arenatask.h` (rule 2), so the band header
 * `include/unsplit/arena.h` now holds only the band's record types.  Two more runs were added by the
 * second pass (`.data` 0x80604D30, `.bss` 0x806E3E10), each an *exact* extension of a run already
 * claimed here:
 *   .data   0x80604D30..0x80607210 - `arena_stage_config`, the ten 0x3B0-byte arena stage records the
 *           band's three task bodies index with `arena_work.stage_0x0C * 0x3B0`.  Sole referencer
 *           (all three `.rela.text` sites are in this unit: `arena_task` 0x804466C0,
 *           `arena_result_next` 0x804472C8, `arena_game_task` 0x804479E4), extent equal to the map
 *           row, and the run carries NO relocations (the target's whole `.rela.data` is 31
 *           `R_PPC_ADDR32` entries at offsets 0x25E0..0x2658, all in the `.data` half below).
 *           EMITTED, as a transcription - see the array's own comment.
 *   .data   0x80607210..0x806073F0 - the `04/arena.brres` name, the 20 arena texture names that fit
 *           in `.data`, the 32-entry pointer table (`lbl_80607310`, slot 31 NULL), the `__FILE__`
 *           string (0x80607390) and the assert message (0x806073A0, 0x18, "NW4R:Failed assertion 0")
 *           - the hole at 0x80607390 is this unit's own, all three are named by `arena_resource_load`'s
 *           own `.rela.text` - then `arena_player_init`'s ten-float array (0x806073B8, renamed
 *           `arena_player_offset_table`) and `arena_light_init`'s GXColor quad (0x806073E0).
 *   .sdata  0x80793B28..0x80793B88 - the resource record (`lbl_80793B28`, {0x54E800, 0x80607210}) plus
 *           the eleven <=8-byte names the pointer table's slots 17..24 and 28..30 point at (the name
 *           at 0x80793B88 is not in the table).
 *   .sdata2 0x8079C960..0x8079C998 - this unit's own pool fragment: the eleven renamed `arena_*f`, the
 *           camera's near/far pair (0x8079C968 = 1.0f, 0x8079C96C = 10000.0f, renamed
 *           `arena_camera_near_z`/`arena_camera_far_z`) and `arena_ambient_color` (0x8079C970).
 *           Claimable by playbook 23's own condition - declare-never-define: our object emits none of
 *           it.  It ends at 0x8079C998 because the 8-byte magic there is the NEXT band's: four
 *           functions of the following band (0x804522E0, 0x804529E0, 0x804535A4, 0x80454978) cite it
 *           and nothing in this band does.
 *   .bss    0x806E3E10..0x806E40C0 - `arena_user_data_buf`, the 0x200-byte arena user-data record
 *           (0x806E3E10, two 0x100-byte records selected by the player work's `chunk_ofs << 8`),
 *           followed by `arena_work` (0x58), `arena_draw_func` (0x10), `arena_camera_vec` (0x18) and
 *           `arena_light_vec` (0x30).  `arena_user_data_buf` was claimed in the second pass: 7 of its
 *           8 address-taken sites are this unit's (`arena_eqdata_from_userdata` x2,
 *           `arena_eqdata_from_vsuser`, `arena_task` x2, `arena_result_next` x2) and the eighth is a
 *           *reader* in `Pl/fn_80288CEC.cpp`'s `fn_8028F1E8`, so taking the range is what makes that
 *           reader legitimate - its local `extern` moved into this unit's header and the file includes
 *           it now.  Two of the four `arena_work` words are *read* by other registered units
 *           (`arena_work`+0x04 by `Pl/fn_8028EA84`, `arena_draw_func[2]` by `menu/menu_result.cpp`'s
 *           `fn_8039806C`), but neither file declares either symbol, so the owner stays this unit -
 *           measured with `tools/units/callers.py <addr>` and the target objects' `.rela.text`.
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
 * (`arena_camera_vec`, `arena_light_vec`), `arena_user_data_buf`, `arena_player_offset_table` and
 * `arena_stage_config` are GUESSES from the bodies that address them - the dump names the block
 * (`arena_work`, `arena_draw_func`) but not the vectors, and the two data runs are named after the
 * index each one's readers use (`chunk_ofs << 8` selecting two 0x100-byte records; `stage_0x0C`
 * indexing ten 0x3B0-byte ones).  Two more GUESSES of the same kind, added when
 * `arena_eqdata_from_vsuser` was written: `_PLW`'s `user_profile_0x1C` and `user_profile_0xB1C[4]`
 * (`include/pl.h`) are the two runs that body serialises into the arena user-data record's bytes
 * 8..13, named from that store and from the game user-data block's `src[158]`/`src[14752..14753]`
 * mirror of the same record; the run at `+0xB1B` itself stays `pad_0xB1B`.
 *
 * SECOND PASS (2026-09-28): `arena_player_init` (344 B), `arena_eqdata_from_userdata` (764 B) and
 * `arena_light_init`'s residual are settled (unit 18.69 % fuzzy, code 1848/10784 B).  The three shapes
 * that were load-bearing and are worth not re-deriving:
 *   - `arena_player_init`: `0xF778` must be a `u16` local (`motion = 0xF778;`) - a plain int literal
 *     materialises as `li r4,-2184` (sign-extended) where the target's zero-extension is `lis r31,1;
 *     addi r4,r31,-2184`, with the peephole off.  The pair `(u16)Get_motion_no(plw) < 700` needs the
 *     `u32` cast for the target's `cmplwi`; `se_name_set != 0`/`else` (NOT `== 0` with the 720 arm
 *     first) is what puts the 700 call on the fall-through; the count/i locals have to be declared
 *     `i` before `count` for r29/r28; `i++, plw++` (both in the for-increment) gives the target's
 *     `addi r29` before `addi r30`.
 *   - `arena_eqdata_from_userdata`: the index is `plw->chunk_ofs % 4`, and MWCC's signed-modulo
 *     sequence (`slwi/srwi/subf/rotlwi/add`) is exactly what `%` emits for an `int`-promoted `u8` -
 *     `& 3` compiles to a bare `clrlwi` and does not match.  The else branch needs `plw->chunk_ofs <=
 *     3` with the two `get_vsUser_work` calls written out (`0` first) and a redundant
 *     `buf = arena_user_data_buf;` before its `memcpy` - that reassignment is what the target's second
 *     `lis/addi` pair is.  `&arena_work` has to be taken into a local (`ArenaWork* work`) for it to
 *     live in a callee-saved register across the calls.  The two packed words are a `u32` copy on each
 *     side; `*(u32*)&buf[4]` (index form, not `*(u32*)(buf + 4)`) is what keeps rule 6 clean.
 *
 * THIRD PASS (2026-09-28): `arena_eqdata_from_vsuser` (508 B) is written and byte- and
 * relocation-identical to the target; 10 of the 19 bodies are written, unit 23.40 % fuzzy, code
 * 2356/10784 B, 9 rows at 100 %.  Two things that pass settled and that a later reader should not have
 * to re-derive: the parameter type of `dl_acdata_to_ar_eqdata` must be the map row's own tag,
 * `_arena_eq_data` - a typedef named `ArenaEqData` emits
 * `dl_acdata_to_ar_eqdata__FP11ArenaEqDataUc` and objdiff still scores that row 100.00, so a byte
 * score says nothing about a relocation *name* (only `flipcheck`/`relocaudit` see it); and the missing
 * `get_move_work_adrs`/`get_move_work_max`/`get_userdata` declarations below are this file's only
 * remaining rule-2 shape, filed as a shared-file sweep in the outbox because they cannot move into
 * their owners' headers until the tree's ten conflicting views are folded (the compiler's own
 * `(10505) illegal overloading` names the clash).
 *
 * RESIDUAL: 10 of the 19 bodies are written; the other 9 are unwritten and every one of them is blocked
 * by a *naming* sweep across other units' files, not by missing knowledge - the full list with each
 * function's callee set is in the previous pass's outbox and in
 * `python tools/units/callees.py quest/arenatask` (48 generated symbols, 72 reference sites in the
 * target object).  The tenth body, `arena_eqdata_from_vsuser` (508 B), landed this pass once the two
 * `_PLW` runs its fourteen serialised bytes live in were named as GUESSES (see NAMES).  Its wire
 * format, recorded here so the next pass does not re-read the disassembly for a body that is already
 * written - arena user-data record byte <- `_PLW` byte:
 *   0 <- +0x14 (se_name_set)          1 <- +0x261          2 <- +0x1D (se_name_idx)   3 <- +0x260
 *   4..7 <- +0x258..+0x25B as a big-endian word          8 <- +0x1C
 *   9 <- +0xB1C   10 <- +0xB1D   11 <- +0x262   12 <- +0xB1E   13 <- +0xB1F
 *   16..19 <- +0x25C..+0x25F as a big-endian word
 * Both packed words must be assembled byte-wise (`(b0 << 24) | (b1 << 16) | (b2 << 8) | b3`): the
 * target emits four `lbz` and an `or` tree, not a `lwz`/`stw` pair.  The two runs of zeroed item slots
 * are `slot_id`'s first 24 entries (NOT all 26 - the target stops before the union view's entries 24
 * and 25) and all 8 of `spare_slot_id`, one `sth` per u16, unrolled; the record is then re-applied
 * (`arena_userdata_apply` + `arena_equip_color_set`), which is the last thing the body does.
 *
 * DATA RESIDUAL: `datagap.py --unit quest/arenatask --mode both` now reports our `.data` at 9440 B
 * against the target's 9920 B - the emitted `arena_stage_config` is byte-identical (`cmp` clean over
 * all 9440 bytes), and the 480 B still missing is the `.data 0x80607210..0x806073F0` half (the texture
 * names, the 32-entry pointer table with its 31 relocations, the `__FILE__` string and the assert
 * message) plus the whole `.sdata`/`.sbss`/`.sdata2` runs, which are claimed but not yet emitted: none
 * of them is referenced by a written body except `arena_player_offset_table` (declared extern by the
 * declare-never-define rule).  `arena_lsp_data_adrs`'s map row is 0x10 bytes while the declaration is
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
#include "pl.h"                 /* the `_PLW` move-work record the arena task drives */
#include "quest/arenatask.h"    /* the owner's own header: this band's data + records (rule 2) */
#include "ef/fn_800CDB2C.h"     /* `my_player_no` + the move-work accessors (the owner's header) */
#include "fn_80047398.h"        /* `arena_userdata_apply` (the owner's header, rule 2) */
#include "fn_8004CAD8.h"        /* `get_vsUser_work` (the owner's header, rule 2) */
#include "Pl/fn_802693C4.h"     /* `Get_motion_no` / `Pl_chr_setX` (the owner's header, rule 2) */
#include "mh3_pad/vec3.h"       /* `setVec3` (the owner header, rule 2) */
#include "Runtime.PPCEABI.H/memcpy.h" /* `memcpy` (the owner's header, rule 2) */
#include "menu/get_pop_dat_ptr.h"  /* `get_arena_cfg` (the owner header, rule 2) */
#include "light/light.h"        /* `set_amblight`/`make_dir_light2` (the owner header, rule 2) */

/* The arena eq-data record pair this band's three writers walk (0x80446C34/0x80446D40/0x80446DDC).
 * Declared here: the owner of `arena_eqdata_apply` is this TU, so its prototype lives with it. */
extern "C" void arena_eqdata_apply(ArenaEqParams* params, ArenaEqSlot* slot, u8 kind, u8 index);

/* `ef/fn_800CDB2C.cpp` owns the two move-work accessors (`get_move_work_adrs__FUc` /
 * `get_move_work_max__FUc`) and they are declared at C++ scope so the front-end reproduces those
 * manglings (rule 9).  Its own header cannot carry them: TEN headers in the tree declare a view of the
 * pair and they disagree - `include/ai/fn_802D0F34.h` (`u8*`), `include/enemy/fn_80165FC8.h`,
 * `include/enemy/fn_80387844.h`, `include/hud/fn_80334568.h`, `include/menu/fn_802E4978.h`,
 * `include/Pl/fn_8025F088.h` and `include/unsplit/ef.h` (`void*`), `include/hud/cockpit_quest.h`
 * (`u8*` / `u16` count), `include/lobby/lb_companion_ui.h` (`LbMoveWork*`),
 * `include/quest/quest_entry.h` (`void*`), plus the owner's own bodies (`void*` / `u16`), and six of
 * them also differ on linkage (`extern "C"` vs C++ scope).  MWCC refuses any TU that sees two of the
 * C++-scope views: `(10505) illegal overloading 'get_move_work_adrs(unsigned char)' was declared as
 * 'LbMoveWork * (...)' now declared as '_PLW * (...)'` - so a declaration added to
 * `include/ef/fn_800CDB2C.h`, or to this unit's own header (which `lobby/lb_companion_ui.h` includes),
 * stops other units compiling (measured: 10 and 3 FAILED).  The declarations below are the view this
 * unit's call sites need; `u16` is the count's own width, which is what `arena_player_init`'s target
 * narrows with `clrlwi`.  One canonical view plus the sweep of the ten is filed in the outbox. */
_PLW* get_move_work_adrs(u8 index);
u16 get_move_work_max(u8 index);

/* 0x8004D120 - the game's user-data block (`get_userdata__Fv`, also C++ scope for the same reason).
 * Its return is only ever read as a byte range here, so the type is the block's own forward-declared
 * owner rather than `void*`.  Its own owner (`src/fn_8004CAD8.cpp`, whose range this address is in)
 * does not publish it - `include/fn_8004CAD8.h` carries `get_vsUser_work` only - and the one published
 * view, `include/quest/quest_entry.h`'s `struct Q_UserData* get_userdata(void);`, cannot be included
 * here because that header declares `void* get_move_work_adrs(u8)` against this file's `_PLW*` view.
 * Both halves are the shared-file sweep filed in the outbox. */
struct Q_UserData;
struct Q_UserData* get_userdata(void);

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

/* ---- the arena scene's per-player setup (0x80446990) ---- */

/* Places every player's move work at its arena spawn offset, gives it the motion its equip slot's
 * kind selects, and marks the slot whose own player byte agrees with the index. */
extern "C" void arena_player_init(ArenaEqParams* params) {
    _PLW* plw = get_move_work_adrs(2);
    s32 i;
    s32 count = (u16)get_move_work_max(2);

    for (i = 0; i < count; i++, plw++) {
        ArenaEqSlot* slot;
        u16 motion;
        s32 kind;

        if (i < 4) {
            slot = &params->slot_0x14;
            if (params->vs_mode_0x04 == 1) {
                kind = 1;
                motion = 2185;
            } else {
                kind = 0;
                motion = 0;
            }
        } else {
            slot = &params->slot_0x2C;
            kind = 2;
            motion = 0xF778;
        }

        plw->motion_pos_0x3C = arena_player_offset_table[kind * 3];
        plw->motion_pos_0x40 = arena_player_offset_table[kind * 3 + 1];
        plw->motion_pos_0x44 = arena_player_offset_table[kind * 3 + 2];
        plw->field_0x058 = (params->field_0x03 << 8) + motion;

        if ((u32)Get_motion_no(plw) < 700) {
            if (plw->se_name_set != 0) {
                Pl_chr_setX(plw, 700, 0, 0);
            } else {
                Pl_chr_setX(plw, 720, 0, 0);
            }
        }

        if (slot->player_0x00 == (i & 3)) {
            plw->field_0x001 = 1;
        } else {
            plw->field_0x001 = 0;
        }
    }

    params->field_0x03++;
}

/* ---- the arena user-data fillers (0x80445EB8) ---- */

/* Builds the arena user-data record the two-player mode reads: the local player's own user block
 * supplies the header bytes, the two packed words and the equip slot index, the arena's acdata table
 * the 0xEC-byte equip record, and the record is then applied to the player's move work. */
extern "C" void arena_eqdata_from_userdata(_PLW* plw) {
    ArenaWork* work = &arena_work;
    _arena_eq_data eq;
    u8* buf = arena_user_data_buf;
    u8* src;
    s32 i;

    if (work->mode_0x04 == 2) {
        src = (u8*)get_userdata();
        buf[0] = src[0];
        buf[1] = src[1];
        buf[2] = src[159];
        buf[3] = src[2];
        *(u32*)&buf[4] = *(u32*)&src[20];
        buf[8] = src[158];
        buf[9] = src[14752];
        buf[10] = src[14753];
        buf[11] = src[14778];
        *(u32*)&buf[16] = *(u32*)&src[14780];
        buf[12] = src[15846];
        buf[13] = src[15847];

        dl_acdata_to_ar_eqdata(&eq, (u8)(plw->chunk_ofs % 4));
        memcpy(buf + 20, &eq, 236);
        arena_userdata_apply(buf, (u8*)plw);
        arena_equip_color_set(plw);

        /* A byte-wise copy of the seventeen profile bytes the user block carries; it names no field
         * of either record (`_PLW` has no labelled run there), so the offsets are written raw. */
        for (i = 0; i < 17; i++) {
            plw->user_profile_0x5CA[i] = src[3 + i];
        }
    } else {
        if (plw->chunk_ofs <= 3) {
            src = (u8*)get_vsUser_work(0);
        } else {
            src = (u8*)get_vsUser_work(1);
        }

        if (src == NULL) {
            return;
        }

        buf[0] = src[0];
        buf[1] = src[1];
        buf[2] = src[33];
        buf[3] = src[2];
        *(u32*)&buf[4] = *(u32*)&src[20];
        buf[8] = src[32];
        buf[9] = src[34];
        buf[10] = src[35];
        buf[11] = src[36];
        *(u32*)&buf[16] = *(u32*)&src[40];
        buf[12] = src[37];
        buf[13] = src[38];

        buf = arena_user_data_buf;
        memcpy(buf + 20, (const void*)(work->eq_data_0x10 + (plw->chunk_ofs % 4) * 236), 236);
        arena_userdata_apply(buf, (u8*)plw);
        arena_equip_color_set(plw);

        for (i = 0; i < 17; i++) {
            plw->user_profile_0x5CA[i] = src[3 + i];
        }
    }
}

/* Builds the same record from the *local* player's own move work instead of the game's user-data
 * block: the fourteen profile bytes below are the `_PLW` mirror of the ones `arena_eqdata_from_
 * userdata` takes at src[158]/[14752..]/[14778..]/[15846..], the five runs it clears are the item
 * slots - both packed words are assembled byte-wise (a `lwz`/`stw` pair does not match, the target
 * serialises big-endian explicitly) - and the record is then re-applied to the move work. */
extern "C" void arena_eqdata_from_vsuser(_PLW* plw) {
    u8* buf = arena_user_data_buf + ((u32)plw->chunk_ofs << 8);
    s32 i;

    buf[0] = plw->se_name_set;          /* _PLW +0x14  */
    buf[1] = plw->field_0x25C[5];       /* _PLW +0x261 */
    buf[2] = plw->se_name_idx;          /* _PLW +0x1D  */
    buf[3] = plw->field_0x25C[4];       /* _PLW +0x260 */
    *(u32*)&buf[4] = ((u32)plw->field_0x258[0] << 24) | ((u32)plw->field_0x258[1] << 16) |
                     ((u32)plw->field_0x258[2] << 8) | plw->field_0x258[3];   /* _PLW +0x258..0x25B */
    buf[8] = plw->user_profile_0x1C;    /* _PLW +0x1C  */
    buf[9] = plw->user_profile_0xB1C[0];   /* _PLW +0xB1C */
    buf[10] = plw->user_profile_0xB1C[1];  /* _PLW +0xB1D */
    buf[11] = plw->field_0x25C[6];      /* _PLW +0x262 */
    *(u32*)&buf[16] = ((u32)plw->field_0x25C[0] << 24) | ((u32)plw->field_0x25C[1] << 16) |
                      ((u32)plw->field_0x25C[2] << 8) | plw->field_0x25C[3];  /* _PLW +0x25C..0x25F */
    buf[12] = plw->user_profile_0xB1C[2];  /* _PLW +0xB1E */
    buf[13] = plw->user_profile_0xB1C[3];  /* _PLW +0xB1F */

    /* The item slots the record's consumer resets: the 24 live entries of `slot_id` (the target
     * stops before entries 24/25, which only the union's item-scan view reaches) and all eight of
     * `spare_slot_id`.  MWCC unrolls both loops into one `sth` per u16. */
    for (i = 0; i < 24; i++) {
        plw->slot_id[i].item_id = 0;
        plw->slot_id[i].value = 0;
    }
    for (i = 0; i < 8; i++) {
        plw->spare_slot_id[i].item_id = 0;
        plw->spare_slot_id[i].value = 0;
    }

    arena_userdata_apply(buf, (u8*)plw);
    arena_equip_color_set(plw);
}

/* ---- the arena scene's light setup (0x80446B8C) ---- */

/* Installs the arena scene's ambient colour, then one direct light per entry of the arena's own
 * four-colour table (the first one twice: index 0 before the loop and again as the loop's first
 * entry), each with the matching `arena_light_vec` position. */
extern "C" void arena_light_init(void) {
    u32* colors;
    u32 color;
    s32 i;

    color = arena_ambient_color;
    set_amblight(0, *(_GXColor*)&color);

    colors = arena_light_colors;
    color = colors[0];
    make_dir_light2(0, &arena_light_vec[0], *(_GXColor*)&color, 0);
    for (i = 0; i < 4; i++) {
        color = colors[i];
        make_dir_light2(i + 5, &arena_light_vec[i], *(_GXColor*)&color, 0);
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
/* The arena stage configuration table (`arena_stage_config`, `.data` 0x80604D30, 0x24E0 B):
 * ten 0x3B0-byte records, one per arena stage, indexed by `arena_work.stage_0x0C`.
 *
 * TRANSCRIPTION, not a reconstruction: the record internals are not settled (the field names,
 * their widths and every table inside them are still unknown - only its two indexes are measured,
 * `stage_0x0C * 0x3B0` and the two `(u16 id, u16 value)` runs the records end with), so the bytes
 * are copied verbatim out of the DOL rather than guessed at.  It is emitted because this unit
 * claimed the run (rule 12) and a claim means the object carries the bytes; a later pass that
 * settles the structure replaces this array with it and must keep the byte total (0x24E0). */
u32 arena_stage_config[2360] = {
    0x08000020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x05000003, 0x00000000, 0x00000000, 0x01000002, 0x00000000, 0x00000000, 0x03000015,
    0x00000000, 0x00000000, 0x0200001E, 0x00000000, 0x00000000, 0x04000003, 0x00000000, 0x00000000,
    0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000D000A, 0x002D000A, 0x001F0001, 0x000B0005,
    0x00020002, 0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x09000028, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000000, 0x00000000, 0x00000000, 0x0100000E,
    0x00000000, 0x00000000, 0x0300000F, 0x00000000, 0x00000000, 0x0200000B, 0x00000000, 0x00000000,
    0x04000009, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000D000A,
    0x001A000A, 0x00320001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0A00002A, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000079,
    0x00000000, 0x00000000, 0x01000006, 0x00000000, 0x00000000, 0x03000007, 0x00000000, 0x00000000,
    0x02000005, 0x00000000, 0x00000000, 0x04000007, 0x00000000, 0x00000000, 0x06000000, 0x00000000,
    0x00000000, 0x00620014, 0x000D000A, 0x002D000A, 0x00860002, 0x00030001, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0B000002, 0x00000000, 0x00000000, 0x0C000002, 0x00000000, 0x00000000, 0x0D000002,
    0x00000000, 0x00000000, 0x05000004, 0x00000000, 0x00000000, 0x01000003, 0x00000000, 0x00000000,
    0x03000004, 0x00000000, 0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x04000004, 0x00000000,
    0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x000D000A, 0x002D000A, 0x00320001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00360063, 0x0038003C, 0x003C0032, 0x0059000A,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x08000011, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000018, 0x00000000, 0x00000000,
    0x0100001B, 0x00000000, 0x00000000, 0x0300001C, 0x00000000, 0x00000000, 0x02000018, 0x00000000,
    0x00000000, 0x0400001B, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014,
    0x000D000A, 0x002D000A, 0x00860005, 0x01870002, 0x00030001, 0x00070002, 0x02460001, 0x00010002,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000004,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05000008, 0x00000000, 0x00000000, 0x01000014, 0x00000000, 0x00000000, 0x03000015, 0x00000000,
    0x00000000, 0x02000007, 0x00000000, 0x00000000, 0x04000015, 0x00000000, 0x00000000, 0x06000000,
    0x00000000, 0x00000000, 0x00620014, 0x000D000A, 0x002D000A, 0x00040001, 0x00320001, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x09000004, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0500000A, 0x00000000, 0x00000000, 0x01000004, 0x00000000,
    0x00000000, 0x03000018, 0x00000000, 0x00000000, 0x0200007B, 0x00000000, 0x00000000, 0x04000007,
    0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000D000A, 0x002D000A,
    0x000F0001, 0x00320001, 0x02460001, 0x00010001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0B000001, 0x00000000, 0x00000000,
    0x0C000005, 0x00000000, 0x00000000, 0x0D000005, 0x00000000, 0x00000000, 0x05000015, 0x00000000,
    0x00000000, 0x01000018, 0x00000000, 0x00000000, 0x03000006, 0x00000000, 0x00000000, 0x02000000,
    0x00000000, 0x00000000, 0x0400000A, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000,
    0x000D000A, 0x002D000A, 0x00320001, 0x00020002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00360063, 0x00370063, 0x00410005, 0x003B0032, 0x00550003, 0x004D000C, 0x00000000, 0x00000000,
    0x0F000004, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0500000C, 0x00000000, 0x00000000, 0x0100000E, 0x00000000, 0x00000000, 0x0300000F,
    0x00000000, 0x00000000, 0x0200000B, 0x00000000, 0x00000000, 0x0400000F, 0x00000000, 0x00000000,
    0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x001A000A, 0x000B0005, 0x00030001,
    0x00320001, 0x00020002, 0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0E00000A, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000008, 0x00000000, 0x00000000, 0x01000008,
    0x00000000, 0x00000000, 0x03000015, 0x00000000, 0x00000000, 0x02000011, 0x00000000, 0x00000000,
    0x04000029, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A,
    0x002D000A, 0x01770005, 0x00320001, 0x02460002, 0x00010002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000022, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000016,
    0x00000000, 0x00000000, 0x01000002, 0x00000000, 0x00000000, 0x03000022, 0x00000000, 0x00000000,
    0x02000002, 0x00000000, 0x00000000, 0x04000021, 0x00000000, 0x00000000, 0x06000000, 0x00000000,
    0x00000000, 0x00620014, 0x000E000A, 0x002D000A, 0x01770005, 0x01680002, 0x000A0005, 0x00320001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0B00000B, 0x00000000, 0x00000000, 0x0C000005, 0x00000000, 0x00000000, 0x0D00000C,
    0x00000000, 0x00000000, 0x05000000, 0x00000000, 0x00000000, 0x01000032, 0x00000000, 0x00000000,
    0x0300000E, 0x00000000, 0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x0400000E, 0x00000000,
    0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x000E000A, 0x002D000A, 0x01770005, 0x01680002,
    0x00040001, 0x00020002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00360063, 0x0038003C, 0x005C000A, 0x00410005,
    0x004F000C, 0x0044001E, 0x0045001E, 0x00000000, 0x0800000D, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0500000E, 0x00000000, 0x00000000,
    0x01000010, 0x00000000, 0x00000000, 0x03000015, 0x00000000, 0x00000000, 0x0200007B, 0x00000000,
    0x00000000, 0x04000025, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014,
    0x000D000A, 0x002D000A, 0x0006000A, 0x01680002, 0x00020003, 0x0001000A, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0700000D,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05000083, 0x00000000, 0x00000000, 0x01000014, 0x00000000, 0x00000000, 0x0300001A, 0x00000000,
    0x00000000, 0x02000016, 0x00000000, 0x00000000, 0x04000005, 0x00000000, 0x00000000, 0x06000000,
    0x00000000, 0x00000000, 0x00620014, 0x000D000A, 0x002D000A, 0x0006000A, 0x00140002, 0x00040001,
    0x00320001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x09000035, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x05000014, 0x00000000, 0x00000000, 0x01000017, 0x00000000,
    0x00000000, 0x03000018, 0x00000000, 0x00000000, 0x02000007, 0x00000000, 0x00000000, 0x04000017,
    0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000D000A, 0x002D000A,
    0x0006000A, 0x000B0005, 0x00320001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0B000004, 0x00000000, 0x00000000,
    0x0C000006, 0x00000000, 0x00000000, 0x0D000001, 0x00000000, 0x00000000, 0x05000009, 0x00000000,
    0x00000000, 0x01000009, 0x00000000, 0x00000000, 0x0300000A, 0x00000000, 0x00000000, 0x02000025,
    0x00000000, 0x00000000, 0x04000002, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000,
    0x000D000A, 0x002D000A, 0x0006000A, 0x01680002, 0x00040001, 0x00340002, 0x00020002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00360063, 0x0038003C, 0x00390032, 0x00410005, 0x004B000C, 0x004D000C, 0x00000000, 0x00000000,
    0x0F00002E, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x05000084, 0x00000000, 0x00000000, 0x01000006, 0x00000000, 0x00000000, 0x0300002C,
    0x00000000, 0x00000000, 0x02000028, 0x00000000, 0x00000000, 0x0400002B, 0x00000000, 0x00000000,
    0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x002D000A, 0x01680002, 0x00110002,
    0x015B0001, 0x00320002, 0x02460001, 0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0E000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000086, 0x00000000, 0x00000000, 0x01000021,
    0x00000000, 0x00000000, 0x03000026, 0x00000000, 0x00000000, 0x0200001E, 0x00000000, 0x00000000,
    0x04000011, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A,
    0x002D000A, 0x01680002, 0x00110002, 0x015B0001, 0x00310001, 0x00320001, 0x00020002, 0x00010002,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0A00002C, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000020,
    0x00000000, 0x00000000, 0x01000023, 0x00000000, 0x00000000, 0x0300000D, 0x00000000, 0x00000000,
    0x0200002E, 0x00000000, 0x00000000, 0x0400000D, 0x00000000, 0x00000000, 0x06000000, 0x00000000,
    0x00000000, 0x00620014, 0x000E000A, 0x002D000A, 0x00110002, 0x01680002, 0x015B0001, 0x01870002,
    0x00320001, 0x00020001, 0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0B000003, 0x00000000, 0x00000000, 0x0C000006, 0x00000000, 0x00000000, 0x0D000003,
    0x00000000, 0x00000000, 0x05000007, 0x00000000, 0x00000000, 0x01000026, 0x00000000, 0x00000000,
    0x03000008, 0x00000000, 0x00000000, 0x02000023, 0x00000000, 0x00000000, 0x0400000A, 0x00000000,
    0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x000E000A, 0x002D000A, 0x00110002, 0x01680002,
    0x015B0001, 0x00020001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00360063, 0x0038003C, 0x00410005, 0x005C0009,
    0x004D000C, 0x004F000C, 0x0059000A, 0x00000000, 0x0F000007, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000084, 0x00000000, 0x00000000,
    0x01000027, 0x00000000, 0x00000000, 0x03000028, 0x00000000, 0x00000000, 0x0200007A, 0x00000000,
    0x00000000, 0x04000027, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014,
    0x000E000A, 0x002D000A, 0x002E000A, 0x018A0002, 0x01680002, 0x00090005, 0x00320002, 0x00020003,
    0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0E000013,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05000028, 0x00000000, 0x00000000, 0x0100002B, 0x00000000, 0x00000000, 0x03000003, 0x00000000,
    0x00000000, 0x02000028, 0x00000000, 0x00000000, 0x04000003, 0x00000000, 0x00000000, 0x06000000,
    0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x002D000A, 0x002E000A, 0x01680002, 0x000A0005,
    0x00040001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x0A000049, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x05000088, 0x00000000, 0x00000000, 0x01000017, 0x00000000,
    0x00000000, 0x0300002A, 0x00000000, 0x00000000, 0x02000026, 0x00000000, 0x00000000, 0x04000029,
    0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x002D000A,
    0x002E000A, 0x01680002, 0x00860003, 0x00040001, 0x00320001, 0x02460002, 0x00010002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0B00000B, 0x00000000, 0x00000000,
    0x0C00000D, 0x00000000, 0x00000000, 0x0D00000C, 0x00000000, 0x00000000, 0x05000085, 0x00000000,
    0x00000000, 0x01000013, 0x00000000, 0x00000000, 0x03000025, 0x00000000, 0x00000000, 0x02000010,
    0x00000000, 0x00000000, 0x04000014, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000,
    0x000E000A, 0x002D000A, 0x002E000A, 0x01680002, 0x02460002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00360063, 0x00370063, 0x003E0009, 0x0049000C, 0x004B000C, 0x004D000C, 0x0044003C, 0x00000000,
    0x08000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0500001E, 0x00000000, 0x00000000, 0x0100000C, 0x00000000, 0x00000000, 0x0300000D,
    0x00000000, 0x00000000, 0x02000020, 0x00000000, 0x00000000, 0x0400000D, 0x00000000, 0x00000000,
    0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x000D000A, 0x002D000A, 0x01870002,
    0x01680002, 0x00040001, 0x00320002, 0x00020003, 0x00010002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0E00000A, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000081, 0x00000000, 0x00000000, 0x01000031,
    0x00000000, 0x00000000, 0x0300002A, 0x00000000, 0x00000000, 0x02000078, 0x00000000, 0x00000000,
    0x0400002F, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A,
    0x000D000A, 0x002D000A, 0x01680002, 0x000B0005, 0x00030001, 0x00320001, 0x02460002, 0x00010002,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x09000030, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0500002A,
    0x00000000, 0x00000000, 0x0100002D, 0x00000000, 0x00000000, 0x03000005, 0x00000000, 0x00000000,
    0x0200007C, 0x00000000, 0x00000000, 0x0400002D, 0x00000000, 0x00000000, 0x06000000, 0x00000000,
    0x00000000, 0x00620014, 0x000E000A, 0x000D000A, 0x002D000A, 0x000F0001, 0x00320001, 0x02460001,
    0x00010001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0B000005, 0x00000000, 0x00000000, 0x0C000005, 0x00000000, 0x00000000, 0x0D000006,
    0x00000000, 0x00000000, 0x05000082, 0x00000000, 0x00000000, 0x01000022, 0x00000000, 0x00000000,
    0x03000023, 0x00000000, 0x00000000, 0x0200001F, 0x00000000, 0x00000000, 0x04000022, 0x00000000,
    0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x000E000A, 0x000D000A, 0x002D000A, 0x01680002,
    0x00020002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00360063, 0x0038003C, 0x00390032, 0x003F0009,
    0x0045001E, 0x00000000, 0x00000000, 0x00000000, 0x0700003A, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000004, 0x00000000, 0x00000000,
    0x01000014, 0x00000000, 0x00000000, 0x03000030, 0x00000000, 0x00000000, 0x02000011, 0x00000000,
    0x00000000, 0x0400002F, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014,
    0x000E000A, 0x002D000A, 0x00110002, 0x01680002, 0x015B0001, 0x000B0005, 0x00030001, 0x00320002,
    0x02460002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0F000017,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x05000022, 0x00000000, 0x00000000, 0x01000025, 0x00000000, 0x00000000, 0x03000026, 0x00000000,
    0x00000000, 0x02000022, 0x00000000, 0x00000000, 0x0400000F, 0x00000000, 0x00000000, 0x06000000,
    0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x002D000A, 0x00110002, 0x01680002, 0x015B0001,
    0x00040001, 0x00320002, 0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x09000033, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x05000084, 0x00000000, 0x00000000, 0x01000021, 0x00000000,
    0x00000000, 0x0300002E, 0x00000000, 0x00000000, 0x0200001E, 0x00000000, 0x00000000, 0x04000011,
    0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x002D000A,
    0x000F0002, 0x00110002, 0x01870003, 0x01680002, 0x015B0001, 0x000A0005, 0x00030001, 0x00040001,
    0x00320002, 0x00020003, 0x00010002, 0x00330001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0B00000D, 0x00000000, 0x00000000,
    0x0C00000B, 0x00000000, 0x00000000, 0x0D000006, 0x00000000, 0x00000000, 0x05000082, 0x00000000,
    0x00000000, 0x0100002A, 0x00000000, 0x00000000, 0x0300002B, 0x00000000, 0x00000000, 0x02000027,
    0x00000000, 0x00000000, 0x0400002A, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000,
    0x000E000A, 0x002D000A, 0x00110002, 0x01870002, 0x015B0001, 0x01680002, 0x00030001, 0x00320001,
    0x02460002, 0x00330001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00360063, 0x00390032, 0x00420005, 0x00430005, 0x0045003C, 0x0046003C, 0x00000000, 0x00000000,
    0x0800000F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x05000026, 0x00000000, 0x00000000, 0x01000029, 0x00000000, 0x00000000, 0x03000015,
    0x00000000, 0x00000000, 0x0200007A, 0x00000000, 0x00000000, 0x04000029, 0x00000000, 0x00000000,
    0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x000D000A, 0x002D000A, 0x002E000A,
    0x018A0002, 0x01680003, 0x00090005, 0x00040001, 0x00310001, 0x02460002, 0x00010002, 0x00330001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0A000033, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000079, 0x00000000, 0x00000000, 0x01000027,
    0x00000000, 0x00000000, 0x03000024, 0x00000000, 0x00000000, 0x02000024, 0x00000000, 0x00000000,
    0x04000009, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A,
    0x000D000A, 0x002D000A, 0x002E000A, 0x01680002, 0x000A0005, 0x00040001, 0x00320002, 0x00020003,
    0x00010002, 0x00330001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0900002F, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000086,
    0x00000000, 0x00000000, 0x0100002B, 0x00000000, 0x00000000, 0x03000026, 0x00000000, 0x00000000,
    0x02000028, 0x00000000, 0x00000000, 0x0400002B, 0x00000000, 0x00000000, 0x06000000, 0x00000000,
    0x00000000, 0x00620014, 0x000E000A, 0x000D000A, 0x002D000A, 0x002E000A, 0x01680002, 0x000F0002,
    0x00040001, 0x00320003, 0x02460001, 0x00330002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x0B000009, 0x00000000, 0x00000000, 0x0C000006, 0x00000000, 0x00000000, 0x0D00000E,
    0x00000000, 0x00000000, 0x05000021, 0x00000000, 0x00000000, 0x01000024, 0x00000000, 0x00000000,
    0x03000025, 0x00000000, 0x00000000, 0x02000021, 0x00000000, 0x00000000, 0x04000024, 0x00000000,
    0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x000E000A, 0x000D000A, 0x002D000A, 0x002E000A,
    0x01680003, 0x00040001, 0x00320003, 0x02460001, 0x00020002, 0x00330002, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00360063, 0x0038003C, 0x00410005, 0x004C0008,
    0x004D000C, 0x004F000C, 0x0044003C, 0x00000000, 0x0F000013, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x05000086, 0x00000000, 0x00000000,
    0x01000029, 0x00000000, 0x00000000, 0x0300002A, 0x00000000, 0x00000000, 0x02000026, 0x00000000,
    0x00000000, 0x04000029, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014,
    0x000E000A, 0x000D000A, 0x002D000A, 0x01770005, 0x00110002, 0x01680003, 0x015B0001, 0x00030001,
    0x00040001, 0x00320002, 0x02460002, 0x00010002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x07000030,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0500002C, 0x00000000, 0x00000000, 0x0100002F, 0x00000000, 0x00000000, 0x0300002E, 0x00000000,
    0x00000000, 0x0200002C, 0x00000000, 0x00000000, 0x0400002D, 0x00000000, 0x00000000, 0x06000000,
    0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x000D000A, 0x002D000A, 0x01770005, 0x00110002,
    0x01680003, 0x015B0001, 0x000B0005, 0x00040001, 0x00320002, 0x02460002, 0x00010002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x0A000026, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x0500002E, 0x00000000, 0x00000000, 0x01000031, 0x00000000,
    0x00000000, 0x03000030, 0x00000000, 0x00000000, 0x02000026, 0x00000000, 0x00000000, 0x0400002F,
    0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000, 0x00620014, 0x000E000A, 0x000D000A,
    0x002D000A, 0x01770005, 0x00110002, 0x01680003, 0x015B0001, 0x00040001, 0x00310001, 0x00320001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0B00000D, 0x00000000, 0x00000000,
    0x0C000007, 0x00000000, 0x00000000, 0x0D000008, 0x00000000, 0x00000000, 0x0500008A, 0x00000000,
    0x00000000, 0x0100002C, 0x00000000, 0x00000000, 0x0300002D, 0x00000000, 0x00000000, 0x02000029,
    0x00000000, 0x00000000, 0x0400002C, 0x00000000, 0x00000000, 0x06000000, 0x00000000, 0x00000000,
    0x000E000A, 0x000D000A, 0x002D000A, 0x01770005, 0x00110002, 0x01680003, 0x015B0001, 0x00030001,
    0x02460002, 0x00320002, 0x0044003C, 0x0046003C, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00360063, 0x00390032, 0x003B003C, 0x003C003C, 0x00420005, 0x00430005, 0x005D000A, 0x004D000C,
};
