/* quest/quest_entry.cpp - the body of the quest entry band: pair-table lot picks and rolls, the quest work block's
 * state code, the area lists and the entry/finish steps.
 *
 * `.text` 0x803AB3BC..0x803B465C: the entry band (46 functions, 0x803AB3BC..0x803B0F98) followed by the absorbed
 * arena result band (56 functions, the second header below).  The band's head (0x803AA4A4..0x803AB3BC) is
 * `quest/quest_item_slot.cpp`: the cut is the `.data` emission-order seam that unit's static initialiser and its
 * five zone tables sit on.  Phase 4 fold: `menu/arena_result.cpp` (also `cflags_menu`, so no flag difference) is
 * one TU with the entry band (the candidate unit `quest/quest_entry` is the pair); both bodies are kept in
 * their own text order, entry band first.
 *
 * NAME - class 3 (behaviour).  No `__FILE__` string, and the runtime dump answers `zz_` for 69 of the 70
 * addresses of the band, but it names the band's globals (`q_result_msg_adrs`, `quest_ex_condition_tbl`,
 * `em_bui_tbl`, `em_bui_rem_l/h`, `em_hokaku_rem_l/h`, `q_npc_snd_func`) and the one real function,
 * `quest_init(unsigned char)` (0x803AD47C), so the module is `quest`.  `quest_entry.cpp` is a GUESS.
 *
 * NAMES.  Every symbol without a dump name is a GUESS derived from its body and callers (rule 7).  Guessed here:
 * the function names of this unit and of `quest_item_slot`, `assignVec3` (0x80051490, an `out = in` 12-byte
 * copy), `quest_list_file`/`quest_list_pool` (the `work_mem_alloc` list block and its `load_file` window) and
 * the unit's `.sdata` rows.  Callee names swept from their generated stems by this unit's passes are in the
 * map (owner in brackets where a header moved): userdata_*, quest_rand_*, quest_area_spawn_*,
 * quest_time_limit_set, em_kind_release, em_area_entry_make, em_spawn_request, stage_*_get, pl_item_add,
 * hud_msg_push, snd_*, quest_objective_get, quest_result_stat_fill, quest_warp_hub, ai_slots_clear.
 * `quest_sub_state_end_ck` takes one argument (the body reads one); passing a stale float moved
 * `menu/fn_802E4978.cpp`'s `fn_802E4B8C` from 97.34 to 99.97 once it was dropped.
 *
 * DATA (rule 12).  Claimed and emitted here: `.bss` 0x806C5558..0x806C5858 (the four pair tables), `.data`
 * 0x805F7AB0..0x805F7AF8 (chance/reward tables), `.sdata` 0x807935D8..0x8079367C (163 of 164 B: the last byte is
 * the section's trailing alignment pad) and `.sbss` 0x80794C1C..0x80794C3C (the list block's three words and
 * `em_bui_tbl`, `em_bui_rem_l/h`, `em_hokaku_rem_l/h`).  The `.sdata` layout is MWCC's size-keyed alignment at
 * work: an object of 4-7 bytes is 4-aligned, of 8 or more 8-aligned, so the twelve time rows are `u16[3]`
 * (6 B, stride 8) and the rank weights `u8[7]` - a `u16[4]`/`u32[2]` row lands 4 B late.
 * Not claimable, with the reason:
 *   * `.data` 0x805F2A98 (read by `quest_init`) and 0x805F4F80 (`quest_monsters_spawn`): a second run of this
 *     unit's `.data`, with other units' data between it and 0x805F7AB0 - one spanning claim is impossible and
 *     two runs make dtk's `.data` order cyclic through the unclaimed unit between them (playbook 53).
 *   * `lbl_8058AFC8`/`lbl_8058AFE8` (.data pool shared with `enemy/em_pop`, `lobby/fn_801F9CD4`), `lbl_805F78C4`,
 *     `lbl_805F7B50/64` (second `.data` runs), `lbl_805F76A0`/`nora_set_proc` (shared with
 *     `enemy/enemy_control`, `menu/arena_result`), the `.sdata2` floats (a partial pool does not link): the
 *     bodies that read them are unwritten.
 *
 * UNWRITTEN (size, blocker): `fn_803ABE44` 2156 / `fn_803AC6B0` 1632 (reward rolls: data ready, the inlined fill
 * loops are not reproduced), `quest_monster_setup` 384 (`nora_set_proc`), `quest_list_load_hunt` 636 /
 * `quest_list_load_arena` 532 / `quest_init` 612 (`lbl_8058AFC8`, `lbl_805F2A98`), `quest_pair_apply` 376,
 * `quest_monsters_spawn` 1296 / `quest_monster_spawn_area` 1012 (`lbl_805F76A0`, `lbl_805F4F80`),
 * `quest_entry_setup` 264 (not attempted: it stores through `Q_ItemWork` offsets +0x58/+0x59/+0x8B/+0x8E/+0x8F/
 * +0x5E0/+0x5E2/+0x684/+0x688 that the record does not name yet), `fn_803AEED0` 1508 / `fn_803AF4B4` 1240 /
 * `fn_803B01C4` 2832 (the entry state machine: `lbl_805F7B50/64`, `lbl_805F78C4`, `lbl_8058AFE8`, `nora_set_proc`
 * and ~20 unnamed callees).  The last three are the orchestration of everything above.
 *
 * RESIDUAL.
 *   * `quest_lot_pick_first` 97.3 / `_last` 97.2 / `quest_lot_pick` 97.0 %: callee-saved colouring, the weight-sum
 *     compare's operand order and two narrow loads (`cmplwi`/`extsh` against our `cmpwi`/`extsb`).
 *   * `quest_pl_skill_slot_set` 97.38 %: the target keeps a dead `addi r4,r4,7` counter in the two-pass fix-up
 *     loop; a `for`, the unrolled form and a live extra variable were tried and the optimizer removes it.
 *   * `quest_work_start_reset` 97.50 %, `quest_monsters_release` 99.62 %: the colours of two callee-saved
 *     registers are swapped (declaration order moves the loop counters, not these).
 *   * `quest_grade_set` 96.52 %: the target hoists `li r4,0` across the three grade-pair stores and keeps the
 *     element pointer in r5; a hoisted `u8` zero did not move it.
 *   * `quest_element_copy` 85.52 %: instruction-identical, the target pairs its last six word copies load/store.
 *
 * SHAPES that decided a row (playbook 63 unless noted): `quest_move_state_valid_ck` masks 0x80 (`rlwinm` MB=ME=24);
 * `quest_work_busy_ck` ends `if (record != 0) return ...; return 1;`; `quest_element_find` declares `i` before
 * the element pointer; the rolls declare `item; u16 total; Q_LotEntry* entry;`; `quest_element_clear`'s tables
 * are defined after the bodies (before them MWCC folds the four `.bss` bases); `quest_area_list_init` keeps
 * `count` a `u8` compared as `(s32)count > 32`; `quest_area_list_refill` takes the unused item work first; u8
 * counters use `+= 1` (`x = x + 1` adds a `clrlwi`); `quest_work_pouch_load` and `quest_monsters_release` depend
 * on declaration order.  The `.sdata` definitions sit above every function so the `"%s"` object is last.
 *
 * TYPES.  `Q_ItemWork` (0x6AB8, `quest/quest_types.h`) is the same block as `quest_work`/`get_move_work_adrs(0)->0xDC`:
 * its `0x94..0x490` run is a union of the quest view (`elements_0x94` ...) and the arena lot-table view, told
 * apart by `Q_MoveWork::kind_0xFC == 4` (evidence in the type's comment).  `Q_ResultRow` is a prefix of
 * `QuestRecord`, `Q_MoveWork` of the slot's move work.  `Q_ResultWork` (`quest/quest_result_work.h`) is the one
 * view of `get_qResult_work`'s 0x438-byte block, shared with `menu/menu_result.cpp` and `lobby/fn_801F9CD4.cpp`.
 * `Q_ItemWork`, the lobby's `LbCompanionWork` and `unsplit/menu.h`'s `QuestWork` are still three views of one block
 * (the merge is a follow-up).  The player work is `_PLW` (`pl.h`), not a view of ours.
 */

/* Absorbed unit `menu/arena_result.cpp` (phase 4 fold):

 * menu/arena_result.cpp - the multiplayer/arena quest-result band: the run's point score, its
 * clear-time and rank text, the per-player item counts and the lobby sync that goes with them.
 *
 * `.text` 0x803B0F98..0x803B465C (56 functions, 0x36C4 B), registered whole as the proposal
 * `803B0F98` asked.  The seam is UNPROVEN, and the range is one maximal unclaimed run rather than a
 * proven translation unit:
 *   - the left boundary is the proposal cap and the right one is confirmed: the three private jump
 *     tables `jumptable_805F7B78`/`BB0`/`BE8` (`.data` 0x805F7B78..0x805F7C68) are each cited by
 *     exactly one function of this range, in code order, and `jumptable_805F7C68` belongs to
 *     `quest_result_field_text_cur_get` - the next band (`tudiscover at 0x803B0F98` calls the `.data` run jump at both
 *     edges "strong" evidence).
 *   - no `__FILE__` string covers the range: the DOL's only single-copy menu source name is
 *     `menu_note.cpp` (0x805E91F8), cited from `menu/menu_note.cpp`'s own range, and every other
 *     menu source name (`menu_item.cpp`, `menu_placeinfo.cpp`, `menu_friendlist.cpp`,
 *     `menu_plsearch.cpp`, `arenatask.cpp`, `movie.cpp`) is cited from a band at 0x8043xxxx/0x8031xxxx.
 *   - the runtime dump answers `zz_` for every address of the range, so the NAME is evidence class 4:
 *     module `menu` from the neighbours (`menu/multi_result.cpp` below, the registered `menu/*` band
 *     above) and from the callees (`get_str_tbl`, `msg_str_gen`, `GetItemData`, `my_player_no`,
 *     `get_qResult_work`), the file name a GUESS from what the band does.  Every function name and
 *     every field name here is a GUESS derived from its body.
 *
 * The band is C++ (its objects carry mangled undefined symbols only), and every function it defines is
 * spelled `extern "C"` so the emitted name is the map's (playbook row 42) - the target objects' own
 * symbols are unmangled stems.
 *
 * RESIDUAL: 43 of the 56 functions are written - 6528 B of the range's 14020 B, 27 of them at
 * 100 %, 41 at 80 % or better (unit fuzzy 44.41 %).  The two rows below the 80 % bar are
 * `quest_work_word_get` (79.23 %): the target's loop is `mtctr`/`bdnz` over the list count while
 * ours re-reads the count, and the register pair (`items`/`count` in r3/r4 retail, r4/r5 ours) is
 * the allocator's (playbook 22); and `quest_arena_data_step` (75.40 %, 352 B ours vs 364 target):
 * retail keeps `&quest_work + player*12` as the address base and folds `player_items_0x6A6A` into
 * every displacement, where ours folds that addend into the row pointer instead; direct
 * `[player][i]` indexing measures 73.11 % / 424 B, worse.  `quest_item_count_sum` (85.70 %),
 * `quest_element_value_set` (86.47 %), `quest_players_state_get` (85.47 %), `quest_slot_items_get`
 * (84.48 %), `quest_element_item_count_get` (83.81 %) and `quest_slot_count_get` (82.50 %) keep
 * their measured best shape; the first four are 4-16 B long and all six are the allocator's.
 *  - `quest_arena_time_text_get` (97.15 %, 316 B both) is the only written row with a
 *    *scheduling* residual: the target hoists `rec->field_0x02C - 9000` above the
 *    `system_w.field_0x8af` load (retail `subi r4,r3,-9000` at the branch head, ours after the
 *    load), and its `seconds` numerator stays in r6 (`subf r6,r0,r30`) where ours stages it through
 *    r3.  Both are the same instruction multiset in the same size; the expression forms tried are
 *    recorded in the outbox.
 *  - `quest_clear_time_text_get` (98.25 %) / `quest_elapsed_time_text_get` (98.33 %) keep the
 *    `sprintf(..., minutes, seconds, Screen_w.frame_scale)` form their call sites' float argument
 *    makes; `quest_arena_time_text_get`'s own call site passes two integers only (retail `crclr`
 *    cr1eq, no r7), which is why its text omits the third argument.
 *  - `quest_item_list_prune` (108 B, the row this pass added) is byte-identical; its shape needed
 *    a named pointer local declared before the index (`s32 i; QuestItemSlot* slot = slots;` -
 *    with `i` or `slots[i]` alone it is 91.41 %, the allocator's callee-saved pair mirrors,
 *    playbook 63).  Its only callee was `fn_802752C8`, renamed `Pl_item_id_usable_ck`: retail
 *    passes a second argument (`li r4,0` here, 1 in `em_pop`, 2 in `ai`) that the callee's body
 *    never reads, so the **declaration** carries it (`include/Pl/fn_80273B14.h`); the Pl unit was
 *    re-measured after the change and every row of it is unchanged (`Pl_item_id_usable_ck` keeps
 *    its own 96.47 % residual - three `cmpwi`/`cmplwi` choices).
 *  - `quest_result_field_text_get` (684 B, this pass's second row) is byte-identical too, and it
 *    is the band's biggest single win: 44.41 % from 39.53 %.  Two shapes had to be read off the
 *    target rather than guessed: the NULL-record guard returns straight to the epilogue while the
 *    `switch` has NO explicit bounds check - its `default` is the `return quest_text_buffer` after
 *    the switch, which is where MWCC puts the `cmplwi kind,31` + `bgt` - and MWCC emits the case
 *    BODIES in source order, so the arms are written in the jump table's own order (0, 22, 1, 16,
 *    17, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 12, 30, 31, 26, 27, 28).  Its five localized
 *    strings and their pointer table are DEFINED here (the target's pool labels are map globals the
 *    unit does not claim); the three accented characters are `\x` byte escapes so the file stays
 *    ASCII and `sjiswrap` cannot re-encode them.  `QuestRecord`'s text runs (+0x000, +0x02E,
 *    +0x08C, +0x0B5, +0x0DE, +0x13C, +0x19A, +0x1C9) are named in `include/unsplit/menu.h` now,
 *    which is also where the field-kind numbering comes from.
 *  - the `QuestElement` size correction to the 0x60 its own target objects use (`addi r5,r5,96` in
 *    `quest_element_value_get`) moved **eight** rows up and none down: `quest_field2E8_text_get`
 *    99.96 -> 100 %, `quest_element_item_apply` 97.68 -> 97.86, `quest_element_value_set`
 *    86.43 -> 86.47, `quest_item_count_sum` 85.64 -> 85.70, `quest_players_state_get`
 *    85.38 -> 85.47, `quest_slot_items_get` 84.38 -> 84.48, `quest_element_item_count_get`
 *    83.79 -> 83.81, `quest_slot_count_get` 82.42 -> 82.50 - the header's union had pushed the
 *    struct to 0x64, shifting every field past +0x94 by 0xC.
 *  - ORDERING, unrecorded until now: the 43 written bodies are in DEFINITION order, not the target's
 *    address order - our `.text` opens with `quest_record_get` (target offset 0x23FC) where the
 *    target opens with `fn_803B0F98` (0).  `flipcheck.py` cannot see it here (it names a permutation
 *    only when the sizes agree; ours is 0x1980 B against the target's 0x36C4), so the source order
 *    has to become the address order before a flip can be attempted (playbook 52's permutation
 *    class, as `src/Pl/pl_act.cpp` and `src/g3d/g3d_resnode.cpp` record it).  Not reordered here:
 *    with 13 rows unpaired nothing measures the move, so a reorder would have to prove itself
 *    score-neutral first.
 *
 * The 13 functions still unwritten are blocked by *naming*, not by evidence: every one of them but
 * `quest_arena_summary_step` calls another band's starter name (`pl_item_add`,
 * `Pl_motion_input_ck`, `countOccupiedServerSlots`/`isServerSelectState`/`isReadyCountOne`,
 * `fn_802D8ABC`..`fn_802D8EA8`, `hud_msg_push`, `fn_8033A920`, `fn_802AFC94`/`AFE08`, ..., and the
 * `fn_800CF280` this list used to carry is the already-named `move_work_state_ck`), and a call to
 * one in this new file is a rule-7 finding; `quest_arena_summary_step` (344 B) is blocked
 * by *ownership* instead - its only foreign callee `quest_element_build` (0x803AD008) is already
 * named, but it is `quest/quest_entry.cpp`'s row, so its declaration belongs in that owner's
 * header.  Naming them means sweeping every reference site in the units that own them
 * (`stage_map_kind_get` alone is cited from 30 files of `enemy`, `Pl`, `sound` and `stage`), so the sweep
 * belongs to the campaign's naming wall - registered in `.pi/notes/naming-backlog.md` (`| 14 | 30 |
 * stage_map_kind_get |`) and carried in this batch's outbox `blockers` (never in `config_requests`, which
 * is all `tools/units/backlog.py` reads) - and not to this lane's diff.  The blocked rows are
 * listed with their sizes in the outbox; the two biggest are `fn_803B0F98` (2020 B) and
 * `fn_803B177C` (1596 B).
 *
 * `quest_element_set` (0x803A9DEC) is declared where rule 2 wants it - in its owner's header
 * `include/lobby/lb_quest_screen.h`, whose unit's registered `.text` 0x803A3A50-0x803AA4A4 covers
 * the address.  The owner's own source does not cite it yet (its row is unwritten); this file's two
 * call sites reach it through that header.
 *
 * Data (measured, `datagap.py --unit menu/arena_result`).  Claimed and byte-identical: `.sdata`
 * 0x8079367C..0x80793684 (8 B) - the " " and "..." string-pool entries
 * `quest_result_field_text_get` is the sole referencer of.  UNCLAIMED, and the unit's open data
 * debt: `.data` 288 B, which is TWO runs in retail, 27 KB apart - the three switch jump tables
 * 0x805F7B78..0x805F7C68 (0xF0 B, cited by `fn_803B0F98`, `fn_803B177C` and this file's
 * `quest_result_field_text_get` respectively) and the five localized "(Quest Name Unavailable)"
 * strings plus their 6-pointer table 0x8060E800..0x8060E8A0 (0xA0 B).  Measured: claiming BOTH
 * `.data` runs dies in `dtk dol split` with `Cyclic dependency encountered while resolving link
 * order: menu/arena_result.cpp -> ... -> quest/arenatask.cpp -> auto_07_806073F0_data`, and the
 * 0x8060E800 run *alone* dies the same way (it sits strictly inside `auto_07_806073F0_data`, so the
 * auto unit's remainder would have to be both before and after this unit).  The 0x805F7B78 run
 * *alone* splits fine but is the wrong claim for this object: our `.data` is one 288 B section, so
 * the linker would place it at 0x805F7B78 and the strings would land in the jump tables' range.
 * Both runs are therefore left to their auto units and our object carries them as `ours-extra
 * .data 288 B` (+ `.rela.data`): a claim-shape gap, not a source defect - the five strings and
 * their padding are byte-identical to the DOL's 0x8060E800..0x8060E888 (checked against the auto
 * data object).  It needs dtk to express several runs of one section, or a data-only unit for the
 * strings run, before this unit can flip; filed as a tooling item.  `quest_name_unavailable_text`
 * is therefore defined TWICE in the link inputs: here (`src/menu/arena_result.o`, `.data` +0x88,
 * 0x18 B, global) and in `obj/auto_07_806073F0_data.o` (`.data` +0x7498, 0x18 B, global) - a
 * duplicate strong symbol that is invisible only because a `NonMatching` object is never linked,
 * and fatal the moment this unit can flip.  The faithful shape is what the target object carries -
 * a DECLARATION (its own `quest_name_unavailable_text` is UND) - and it is measured
 * codegen-neutral: the extern variant's `.text` is byte-identical to this one and its `.text`
 * relocation set identical bar the local pool-label numbering, with `ours-extra .data` shrinking
 * 288 B -> 128 B and `.rela.data` 456 B -> 384 B.  It is nonetheless not landable today:
 * `stylelint.py --diff` counts the declaration as an ADDED finding whichever way it is spelled - in
 * this file as rule 2 + rule 12 (there is no owner's header for it, and no registered range covers
 * 0x8060E888) and in the band header `include/unsplit/menu.h` as two rule-12 findings - because the
 * claim rule 12 asks for is exactly the cycle above.  So the definition stays and the clash is
 * handed to the same tooling item: it disappears with the claim (multi-run support or a data-only
 * unit for the strings run).  The pool constants the band
 * addresses (`frames_per_second_60f`, `percent_scale_100f`, `quest_grade_ratio_*`) are declared
 * `extern` in `include/unsplit/menu.h` and never defined here, so no `.sdata2` is emitted for them.
 * The other data row is `ours-extra .sdata2 16 B`: MWCC's implicit int->float magic
 * (`0x4330000080000000` unsigned / `0x4330000000000000` signed), which the compiler pools per TU and
 * `quest_grade_get`/`quest_grade_rank_get`/`quest_grade_text_cur_get` are the band's first users of.
 * The target's copies are `lbl_8079C528`/`lbl_8079C550`, inside the band's unclaimed `.sdata2` run
 * (0x8079C500.., which also holds `quest_grade_ratio_*`/`percent_scale_100f`), so dtk put them in an
 * `auto_*_sdata2` unit and the target object carries none.  They are shared pool entries - neither
 * claimable nor nameable from source (playbook 58) - so the row is a claim gap, not a source defect.
 *
 * Cross-unit declarations this pass needed: the `quest_flag_*_ck`/`quest_arena_*_get`/
 * `quest_element_value_get`/`quest_element_value_at`/`quest_element_remaining_get`/
 * `quest_all_player_item_count_sum` accessors at 0x803B4BEC..0x803B68F0 are the quest band's own but
 * sit inside `enemy/em_pop.cpp`'s registered range (a `--max-bytes` cap over several bands), so their
 * declarations went into that owner's header (`include/enemy/em_pop.h`), which already carries the
 * same shape for its own unwritten rows.  `str_tbl_33_get` (0x802DFACC) is unowned and stays in its
 * band header `include/unsplit/lobby.h`.  `system_w.field_0x8af` (0x8AF) was split out of
 * `SystemWork`'s `pad_0x898`, and `QuestRecord` gained `field_0x36C` while its +0x30C word became
 * the byte pair `quest_monster_text_get` reads (it was an unused `s32`).
 *
 * Rule-7 renames this pass: `fn_803B421C` -> `quest_name_index_get`, `fn_803B4308` ->
 * `quest_monster_text_get` (both this unit's own), and the six map-only rows the band's callees
 * needed - `fn_803B5FF0` -> `quest_element_value_at`, `fn_803B5F84` -> `quest_element_value_index_get`,
 * `fn_803B6150` -> `quest_element_remaining_get` (also swept in `src/Pl/pl_act.cpp`, its only
 * referrer), `fn_803B61DC` -> `quest_all_player_item_count_sum`, `fn_803B4E50` ->
 * `quest_flag_10000000_ck`, and `fn_802DFACC` -> `str_tbl_33_get` (swept in
 * `include/unsplit/lobby.h` and `src/lobby/fn_801E7530.cpp`).
 *
 * FLAGS: `menu`'s `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, Wii/1.3).
 */

#include "quest/quest_entry.h"
#include "quest/quest_item_slot.h"
#include "ef/fn_800CDB2C.h"    /* `move_work_state_ck` - owned by ef/fn_800CDB2C.cpp (rule 2) */
#include "unsplit/menu.h"       /* `quest_work_ptr` - the band data no registered unit claims */
#include "unsplit/lobby.h"      /* `lb_param_w` - the option block no registered unit claims */
#include "hud/cockpit.h"
#include "unsplit/unknown.h"    /* `system_w` - the system block no registered unit claims */
#include "Network/network_pat_control.h"   /* isServerSelectState (owner header, rule 2) */
#include "enemy/em_pop.h"       /* `quest_flag_*_ck` - owned by enemy/em_pop.cpp (rule 2) */
#include "enemy/fn_801251D0.h"  /* `enemy_kind_same_ck` - owned by enemy/fn_801251D0.cpp (rule 2) */
#include "fn_8004CAD8.h"    /* get_qResult_work, userdata_record_*_add, userdata_quest_stat_set - owned by fn_8004CAD8.cpp (rule 2) */
#include "lobby/lb_quest_screen.h"  /* quest_rand_seed_set, quest_rand_next - owned by lobby/lb_quest_screen.cpp (rule 2) */
#include "pl.h"                 /* _PLW - the player work (rule 1) */
#include "lobby/lb_entry_notify_send.h" /* lb_entry_notify_send - owned by lobby/lb_companion_ui.cpp (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h" /* sprintf, strcat (the MSL band has no registered owner) */
#include "ai/fn_802D44F4.h"    /* ai_slots_clear - owned by ai/fn_802D44F4.cpp (rule 2) */
#include "fn_80056F24.h"        /* system_copy_filter_request - owned by fn_80056F24.cpp (rule 2) */
#include "sound/fn_800F2A94.h" /* snd_quest_scene_set - owned by sound/fn_800F2A94.cpp (rule 2) */
#include "lobby/lb_sub18_send.h" /* lb_sub18_send - owned by lobby/lb_companion_ui.cpp (rule 2) */
#include "ef/eft052.h"          /* hud_item_msg_push - owned by ef/eft052.cpp (rule 2) */
#include "enemy/enemy_control.h" /* em_area_entry_make, em_spawn_request - owned by enemy/enemy_control.cpp (rule 2) */
#include "enemy/fn_8013F764.h"   /* em_kind_release - owned by enemy/fn_8013F764.cpp (rule 2) */
#include "Pl/pl_act_stage_latch_set.h"  /* pl_act_stage_latch_set, my_player_work_get - owned by Pl/fn_80273B14.cpp (rule 2) */
#include "Pl/fn_80288CEC.h"   /* pl_warp_start - owned by Pl/fn_80288CEC.cpp (rule 2) */
#include "mh3_pad.h"          /* setVec3 (rule 2) */
#include "stage/stg_w.h"        /* stage_map_area_count_get - owned by stage/stg_w.cpp (rule 2) */
#include "sound/fn_800D7F54.h"  /* snd_item_fail_play - owned by sound/fn_800D7F54.cpp (rule 2) */
#include "Pl/Pl_master_ck.h"    /* Pl_master_ck - owned by Pl/pl_master.cpp (rule 2) */
#include "Pl/pl_skill.h"    /* Pl_Skill_ck, Pl_cat_skill_ck - owned by Pl/pl_skill.cpp (rule 2) */
#include "Runtime.PPCEABI.H/memset.h"  /* memset (owner: the Runtime.PPCEABI.H lib) */
#include "Runtime.PPCEABI.H/memcpy.h"  /* memcpy (owner: the Runtime.PPCEABI.H lib) */
#include "types.h"
#include "quest/quest_list_values.h"     /* `quest_list_values` - owned by quest/quest_entry.cpp (rule 2) */
#include "g3d/g3d_anmchr.h"               /* msg_str_gen / flfntStrLen / flKnjMsgNumPtr (rule 2) */
#include "Pl/fn_80273B14.h"               /* `Pl_item_id_usable_ck`, the id-usable predicate (rule 2) */

/* The band's quest-work pointer in this unit's own view of the record.  `quest_work_ptr` itself is
 * `.sbss` band data `include/unsplit/menu.h` declares, and that header cannot take this unit's
 * offsets, so the two views are cast rather than merged. */
#define QUEST_WORK ((Q_ItemWork*)quest_work_ptr)

/* This unit's own `.sdata` (`splits.txt` `.sdata 0x807935D0..0x8079367C`, the rows tile it in address order;
 * the twelve `quest_grade_time_*` rows and the two `quest_rank_weight_*` rows are separate objects because
 * `.sdata` only takes objects of at most 8 bytes).
 * Every row is referenced by this unit's code only, except the three bytes tables at 0x807935F0..0x80793600
 * and the 0x80793604 table, which no symbol reads (they sit inside the claim as unreferenced data). */

/* The zero-terminated resource id lists the entry state loads for quest kinds 7 (arena; the list starts one
 * entry in when the stat block is live), 0xB and 6, and the per-kind id table (indexed 1..5) it loads for a
 * quick quest. */
u8 quest_res_ids_arena[5] = { 0x2B, 0x2C, 0x2D, 0x2E, 0 };
u8 quest_res_ids_kind_b[4] = { 0x2F, 0x30, 0x31, 0 };
u8 quest_res_ids_kind_6[4] = { 0x28, 0x29, 0x2A, 0 };
u8 quest_res_id_by_kind[8] = { 0, 2, 0x0F, 0x10, 0x11, 0x12, 0, 0 };

/* Unreferenced rows (GUESS names: pairs of small thresholds, sole readers unknown). */
u8 quest_sdata_tbl_a[8] = { 0x08, 0x20, 0x06, 0x30, 0x04, 0x10, 0, 0 };
u8 quest_sdata_tbl_b[4] = { 0x64, 0, 0xFF, 0 };
u8 quest_sdata_tbl_c[4] = { 0x64, 1, 0xFF, 0 };

/* The four-byte chance table `quest_pair_roll_all` copies over the head of its 16-byte roll buffer before
 * the last two picks.  The row's sole referrer is that function. */
u8 quest_pair_chance_tbl_d[0x4] = { 32, 22, 22, 22 };

/* Twelve separate 8-byte rows (clear time, par time, 3000, 0) - one object each, which is what keeps them in
 * `.sdata`; no symbol reads them (GUESS names). */
u16 quest_grade_time_00[3] = { 0x00B4, 0x00F0, 0x0BB8 };
u16 quest_grade_time_01[3] = { 0x00D2, 0x012C, 0x0BB8 };
u16 quest_grade_time_02[3] = { 0x010E, 0x0168, 0x0BB8 };
u16 quest_grade_time_03[3] = { 0x010E, 0x0168, 0x0BB8 };
u16 quest_grade_time_04[3] = { 0x01A4, 0x021C, 0x0BB8 };
u16 quest_grade_time_05[3] = { 0x02D0, 0x0384, 0x0BB8 };
u16 quest_grade_time_06[3] = { 0x0258, 0x02D0, 0x0BB8 };
u16 quest_grade_time_07[3] = { 0x021C, 0x0294, 0x0BB8 };
u16 quest_grade_time_08[3] = { 0x0258, 0x02D0, 0x0BB8 };
u16 quest_grade_time_09[3] = { 0x02D0, 0x0348, 0x0BB8 };
u16 quest_grade_time_0A[3] = { 0x0258, 0x02D0, 0x0BB8 };
u16 quest_grade_time_0B[3] = { 0x03C0, 0x0474, 0x0BB8 };

/* The rank weights `fn_803AEED0` indexes by the quest's rank byte (0..6), and the four zero bytes after
 * them. */
u8 quest_rank_weight_tbl[7] = { 0, 100, 80, 60, 40, 20, 10 };

/* The quest grade each bonus-element mask maps to (`quest_grade_set`; sole referrer). */
u8 quest_grade_table[8] = { 0, 1, 1, 3, 1, 6, 5, 3 };

/* The `"%s"` format `quest_reward_faint_penalty` prints a player name through. */
char quest_entry_fmt_str[3] = "%s";

/* Copies both of the band's first two picked-pair tables into a result record's own two adjacent
 * 0xC0 buffers, one four-byte pair at a time. */
void quest_item_pair_tbl_copy(Q_ItemPair* dst_a, Q_ItemPair* dst_b) {
    const Q_ItemPair* a;
    const Q_ItemPair* b;
    s32 i = 0;

    a = quest_item_pair_tbl_a;
    b = quest_item_pair_tbl_b;
    for (; i < 0x30; i++) {
        item_pair_copy(dst_a++, a);
        item_pair_copy(dst_b++, b);
        a++;
        b++;
    }
}

/* Replaces the placeholder skill slot (0x16) of the 16-entry chance buffer with the skill slot the
 * player's equipped skills select, then, for the two quest-reward skills, forces slot 2 (and its
 * partner) to the top-rank marker 0x20. */
void quest_pl_skill_slot_set(_PLW* owner, u8* chance, u8 mode) {
    u8 fill = 0x16;
    u8* p;
    s32 n;
    u32 top;

    if (Pl_Skill_ck(owner, 0x91) == 1) {
        fill = 0x1A;
    }
    if (Pl_Skill_ck(owner, 0x92) == 1) {
        fill = 0x1D;
    }
    if (Pl_Skill_ck(owner, 0x93) == 1) {
        fill = 0x10;
    }
    if (Pl_Skill_ck(owner, 0x94) == 1) {
        fill = 8;
    }
    p = chance;
    n = 2;
    do {
        if (p[0] == 0x16) {
            p[0] = fill;
        }
        if (p[1] == 0x16) {
            p[1] = fill;
        }
        if (p[2] == 0x16) {
            p[2] = fill;
        }
        if (p[3] == 0x16) {
            p[3] = fill;
        }
        if (p[4] == 0x16) {
            p[4] = fill;
        }
        if (p[5] == 0x16) {
            p[5] = fill;
        }
        if (p[6] == 0x16) {
            p[6] = fill;
        }
        if (p[7] == 0x16) {
            p[7] = fill;
        }
        p += 8;
    } while (--n != 0);
    top = 0;
    if (Pl_cat_skill_ck(owner, 0x10) == 1) {
        top = 1;
    } else if (Pl_cat_skill_ck(owner, 0x11) == 1 && (s32)(ran_suu(0) % 100) < 0x32) {
        top = 1;
    }
    if (top == 1) {
        chance[2] = 0x20;
        if (mode == 0) {
            chance[10] = 0x20;
        } else {
            chance[9] = 0x20;
            chance[13] = 0x20;
        }
    }
}

/* Walks `count` rows of `chance`, and for each row whose chance byte admits it rolls a `total`-wide
 * number into the weighted table and appends the entry the roll lands on.  The first row always
 * rolls 0, so it always takes the table's first non-empty entry. */
s32 quest_lot_pick_first(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;

    for (i = 0; i < count; i++, chance++) {
        const Q_LotEntry* entry = table;
        u16 roll;
        u16 acc;
        s32 hit;

        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        if (i == 0) {
            roll = 0;
        }
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (acc > roll) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The same walk without the first-roll override: every row rolls its own number. */
s32 quest_lot_pick(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;
    const Q_LotEntry* entry;

    for (i = 0; i < count; i++) {
        u16 roll;
        u16 acc;
        s32 hit;

        entry = table;
        if (entry->id == 0) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (roll < acc) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The same walk for the band that gates on the row's own chance byte only. */
s32 quest_lot_pick_last(u8* chance, const Q_LotEntry* table, Q_ItemPair* out, s32 count, u16 total) {
    s32 picked = 0;
    s32 i;

    for (i = 0; i < count; i++, chance++) {
        const Q_LotEntry* entry = table;
        u16 roll;
        u16 acc;
        s32 hit;

        if (entry->id == 0) {
            break;
        }
        if (*chance <= ((u16)ran_suu(0) & 0x1F)) {
            break;
        }
        roll = (u16)ran_suu(0) % total;
        hit = 0;
        acc = 0;
        while (entry->id != 0) {
            acc += entry->weight;
            if (acc > roll) {
                hit = 1;
                break;
            }
            entry++;
        }
        if (hit == 1 && entry->id != 0) {
            out->id = entry->id;
            out->num = (s8)entry->value;
            out++;
            picked++;
        }
    }
    return picked;
}

/* The four-group roll: once the result row's live bit, the element state probe or the first pick gate
 * lets it through, it fills the first two 8-pair groups of `quest_item_pair_tbl_a` from the item
 * work's first two element tables, then - with the four-byte chance table copied over the buffer's
 * head - the two remaining four-pair groups from its third and fourth tables. */
void quest_pair_roll_all(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_a, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if ((item->record_0x3C->flags_0x310 & 0x00800000) != 0 || quest_element_state_ck() == 1 ||
        quest_element_pick_ck((QuestWork*)item, 0, 1) != 0) {
        entry = item->lot_e0a_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
        }
        entry = item->lot_e0b_0xC8;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
        }
    }
    memcpy(chance, quest_pair_chance_tbl_d, sizeof(quest_pair_chance_tbl_d));
    if (quest_element_pick_ck((QuestWork*)item, 1, 1) != 0) {
        entry = item->lot_e1_0xFC;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance, item->lot_e1_0xFC, quest_item_pair_tbl_a + 0x20, 4, total);
        }
    }
    if (quest_element_pick_ck((QuestWork*)item, 2, 1) != 0) {
        entry = item->lot_e2_0x15C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total != 0) {
            quest_lot_pick_last(chance, item->lot_e2_0x15C, quest_item_pair_tbl_a + 0x24, 4, total);
        }
    }
}

/* The two-group roll that picks both groups with `quest_lot_pick_first` and sets the pl skill slots;
 * the clause `quest_element_pick_ck` gates walks the item work's first two element tables. */
void quest_pair_roll_first(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_b, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 1);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 0) {
        return;
    }
    entry = item->lot_e0a_0x9C;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
    }
    entry = item->lot_e0b_0xC8;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
    }
}

/* The same two-group roll with the pl skill slots cleared and the second group picked with
 * `quest_lot_pick_last`. */
void quest_pair_roll_last(_PLW* owner, u8 state) {
    u8 chance[16];
    Q_ItemWork* item;
    u16 total;
    Q_LotEntry* entry;

    memcpy(chance, quest_pair_chance_tbl_b, sizeof(chance));
    quest_pl_skill_slot_set(owner, chance, 0);
    item = move_work_item_work_get();
    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if (state != 4) {
        return;
    }
    if (quest_element_pick_ck((QuestWork*)item, 0, 1) == 0) {
        return;
    }
    entry = item->lot_e0a_0x9C;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_first(chance, item->lot_e0a_0x9C, quest_item_pair_tbl_a, 8, total);
    }
    entry = item->lot_e0b_0xC8;
    total = 0;
    while (entry->id != 0) {
        total += entry->weight;
        entry++;
    }
    if (total != 0) {
        quest_lot_pick_last(chance + 8, item->lot_e0b_0xC8, quest_item_pair_tbl_a + 8, 8, total);
    }
}

/* The same build for the band's persisted pair tables: zeroes all four, then refills the first from
 * the item work's own weighted lot table - 3, 5 or 8 rows, chosen by the element's sub-flag. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_clear(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[8];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(quest_item_pair_tbl_a, 0, sizeof(quest_item_pair_tbl_a));
    memset(quest_item_pair_tbl_b, 0, sizeof(quest_item_pair_tbl_b));
    memset(quest_item_pair_tbl_c, 0, sizeof(quest_item_pair_tbl_c));
    memset(quest_item_pair_tbl_d, 0, sizeof(quest_item_pair_tbl_d));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else if (element->flag_0x434 == 1) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        } else {
            chance[0] = 32;
            chance[1] = 32;
            chance[2] = 22;
            chance[3] = 22;
            chance[4] = 22;
            chance[5] = 12;
            chance[6] = 12;
            chance[7] = 12;
            rows = 8;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, quest_item_pair_tbl_a, rows, total);
    }
}

/* Clears the element's payload, then fills it from the item work's own weighted lot table: a table of
 * 3 or 5 rows, chosen by the element's own sub-flag, and only when the caller passed the arena kind. */
/* untyped: caller-owned context payload the target never reads */
void quest_element_build(void* owner, u32 kind, Q_ArenaElement* element) {
    u8 chance[6];
    u16 total;
    Q_LotEntry* entry;
    s32 rows;

    memset(element->payload_0x3F4, 0, sizeof(element->payload_0x3F4));
    if ((u8)kind != 4) {
        return;
    }
    {
        Q_ItemWork* item = move_work_item_work_get();

        memset(chance, 0, sizeof(chance));
        if (element->flag_0x434 == 0) {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 12;
            rows = 3;
        } else {
            chance[0] = 32;
            chance[1] = 22;
            chance[2] = 22;
            chance[3] = 12;
            chance[4] = 12;
            rows = 5;
        }
        if ((u8)kind != 4) {
            return;
        }
        entry = item->lot_0x9C;
        total = 0;
        while (entry->id != 0) {
            total += entry->weight;
            entry++;
        }
        if (total == 0) {
            return;
        }
        quest_lot_pick_first(chance, item->lot_0x9C, element->payload_0x3F4, rows, total);
    }
}

/* Credits a finished quest's two count runs (and, for a finished row, its stat) to the save block, then
 * clears every block of the quest result work. */
void quest_result_work_flush(void) {
    Q_ResultWork* result = get_qResult_work();
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return;
    }
    item = move_work_item_work_get();
    if (work->sub_0xFA != 7) {
        userdata_record_a_count_add(result->count_a);
        userdata_record_b_count_add(result->count_b);
        if (item->record_0x3C->state_0x8B == 7) {
            userdata_quest_stat_set(&result->stat_0x3E0);
        }
    }
    memset(result->count_a, 0, sizeof(result->count_a));
    memset(result->count_b, 0, sizeof(result->count_b));
    memset(result->block_0x0A4, 0, sizeof(result->block_0x0A4));
    result->field_0x1E4 = 0;
    result->field_0x1E8 = 0;
    memset(result->block_0x304, 0, sizeof(result->block_0x304));
    result->present_0x3A4 = 0;
    memset(&result->stat_0x3E0, 0, sizeof(result->stat_0x3E0));
}

/* The local slot's quest phase byte (+0xE9 of the move work). */
u8 quest_phase_get(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);

    if (work == NULL) {
        return 0;
    }
    return work->phase_0xE9;
}

/* Loads the carried-item pouch into the quest work: the local player's own gear in a network session, else
 * the two runs the player's record of the third move-work array carries. */
void quest_work_pouch_load(void) {
    _PLW* me;
    Q_ItemPair* src;
    Q_UserData* user = get_userdata();
    s32 i;

    if (quest_work_ptr != NULL) {
        me = (_PLW*)get_move_work_adrs(2);

        if (me != NULL) {
            me += (s8)my_player_no();

            memset(QUEST_WORK->pouch_0x6778, 0, 0x8C);
            if (system_w.net_session_0x90f == 1) {
                src = (Q_ItemPair*)userdata_equip_item_slots_get(user);

                for (i = 0; i < 0x18; i++, src++) {
                    item_pair_copy(&QUEST_WORK->pouch_0x6778[i], src);
                }
                if ((u32)userdata_gunner_ck(user) == 1) {
                    for (i = 0; i < 8; i++, src++) {
                        item_pair_copy(&QUEST_WORK->pouch_0x6778[i + 0x18], src);
                    }
                }
            } else {
                memcpy(QUEST_WORK->pouch_0x6778, me->slot_id, 0x60);
                memcpy(&QUEST_WORK->pouch_0x6778[0x18], me->spare_slot_id, 0x20);
            }
        }
    }
}

/* Releases the monsters of every armed element (stopping early when the quest kind says the hunt is over),
 * then loads the lobby area for the hand-off and leaves the quest play mode. */
void quest_monsters_release(void) {
    u8 mode;
    Q_MoveWork* work;
    u32 active;
    s32 i;
    s32 released;

    if (quest_work_ptr != NULL) {
        work = (Q_MoveWork*)get_move_work_adrs(0);
        if (work != NULL) {
            active = 0;
            mode = PlayMode_ck();
            if (quest_work_ptr != NULL && QUEST_WORK->hunt_active_0x675C != 0) {
                active = 1;
            }
            if (active == 1) {
                released = 0;
                for (i = 0; i < 3; i++) {
                    if (QUEST_WORK->armed_0x2D4[i] != 0) {
                        em_kind_release(QUEST_WORK->armed_0x2D4[i]);
                        released++;
                        if (quest_flag_4000000_ck(NULL) == 1) {
                            QUEST_WORK->hunt_end_0x6977 = 1;
                            QUEST_WORK->result_kind_0x308 = 1;
                            break;
                        }
                        if (quest_flag_80000000_ck(NULL) == 1) {
                            QUEST_WORK->hunt_end_0x6977 = 1;
                            QUEST_WORK->result_kind_0x308 = 1;
                            break;
                        }
                        if (quest_flag_100_ck(NULL) == 1 && released == 2) {
                            QUEST_WORK->hunt_end_0x6977 = 1;
                            QUEST_WORK->result_kind_0x308 = 2;
                            break;
                        }
                    }
                }
                if (quest_flag_4000000_ck(NULL) == 0) {
                    if (quest_flag_100_ck(NULL) == 1) {
                        em_kind_release(QUEST_WORK->release_kind_0x6760);
                    } else if (QUEST_WORK->release_kind_0x6760 != 0 && released <= 1) {
                        em_kind_release(QUEST_WORK->release_kind_0x6760);
                    }
                }
                quest_area_spawn_setup(work, QUEST_WORK->area_0x69A8, QUEST_WORK->area_count_0x69A5, 0);
                quest_area_spawn_apply(work, quest_byte_table[work->phase_0xE9], work->sub_0xEA);
                if (mode == 6) {
                    PlayMode_set(3);
                }
            }
        }
    }
}

/* Re-rolls the quest work's clock words, reseeds the quest random source from them and clears the
 * per-quest state. */
void quest_work_start_reset(void) {
    s32 i;
    Q_ItemWork* work = QUEST_WORK;

    if (work == NULL) {
        return;
    }
    for (i = 0; i < 15; i++) {
        work->rand_words_0x6C[i] = (u8)ran_suu(0);
    }
    quest_rand_seed_set();
    quest_rand_next();
    for (i = 0; i < 4; i++) {
        work->rand_words_0x6C[i] = (u8)quest_rand_next();
    }
    work->field_0x60 = 0;
    work->field_0x5C = 0;
    work->field_0x16 = 0;
    work->field_0x30C = 0;
    work->rot_0x54 = 0;
    work->flag_0x55 = 0;
    work->field_0x00 = 0;
    work->field_0x01 = 0;
    work->time_limit_0x24 = 0;
    work->field_0x14 = 0;
    work->armed_0x2D4[0] = 0;
    work->slot_key_0x46C[0] = 0xFFFF;
    work->armed_0x2D4[1] = 0;
    work->slot_key_0x46C[1] = 0xFFFF;
    work->armed_0x2D4[2] = 0;
    work->slot_key_0x46C[2] = 0xFFFF;
}

/* Marks every live element as graded and derives the quest grade from which of them the bonus flag
 * covers, then copies each finished element's value into its grade pair. */
void quest_grade_set(void) {
    Q_Element* e = QUEST_WORK->elements_0x94;
    u32 bonus = 0;
    s32 i;
    u8 mask;

    if (quest_flag_2000000_ck(NULL) == 1) {
        bonus = 1;
    }
    mask = 0;
    for (i = 0; i < 3; i++, e++) {
        if ((e->flags & 1) || (e->flags & 2) || (e->flags & 0x1000) || (e->flags & 0x2000) || (e->flags & 4)) {
            e->flags = e->flags | 8;
            if (bonus == 1) {
                mask = (u8)(mask | (u8)(1 << i));
            }
        }
    }
    if (bonus == 1) {
        QUEST_WORK->grade_0x93 = quest_grade_table[mask];
    }
    if (quest_flag_80000000_ck(NULL) == 1) {
        QUEST_WORK->grade_0x93 = 2;
    }
    {
        Q_ItemWork* w = QUEST_WORK;

        if ((w->elements_0x94[0].flags & 2) && w->elements_0x94[0].id == 0) {
            w->grade_pair_0x68F[0].flag = 0;
            QUEST_WORK->grade_pair_0x68F[0].value = (s8)w->elements_0x94[0].value;
        }
        if ((w->elements_0x94[1].flags & 2) && w->elements_0x94[1].id == 0) {
            QUEST_WORK->grade_pair_0x68F[1].flag = 0;
            QUEST_WORK->grade_pair_0x68F[1].value = (s8)w->elements_0x94[1].value;
        }
        if ((w->elements_0x94[2].flags & 2) && w->elements_0x94[2].id == 0) {
            QUEST_WORK->grade_pair_0x68F[2].flag = 0;
            QUEST_WORK->grade_pair_0x68F[2].value = (s8)w->elements_0x94[2].value;
        }
    }
    {
        Q_ItemWork* w = QUEST_WORK;

        if ((w->elements_0x94[0].flags & 0x2000) && w->elements_0x94[0].id == 4) {
            w->score_0x5D8 = w->elements_0x94[0].value;
        }
        if ((w->elements_0x94[1].flags & 0x2000) && w->elements_0x94[1].id == 4) {
            QUEST_WORK->score_0x5D8 = w->elements_0x94[1].value;
        }
        if ((w->elements_0x94[2].flags & 0x2000) && w->elements_0x94[2].id == 4) {
            QUEST_WORK->score_0x5D8 = w->elements_0x94[2].value;
        }
    }
}

/* Spawns the current quest work's monsters. */
void quest_monsters_spawn_now(void) {
    quest_monsters_spawn(QUEST_WORK);
}

/* The state byte of a result row (the current one when `rec` is NULL), 0 when there is none. */
u8 quest_record_state_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x08B;
}

/* The index of the quest-work element `kind` names: the first of the three whose +0x00 gate bit is set
 * and whose +0x04 id either equals `kind` or maps to the same enemy kind, or -1 when none does. */
s32 quest_element_find(u8 kind) {
    s32 i;
    QuestElement* e = quest_work.elements_0x0094;

    if (e == NULL) {
        return -1;
    }
    for (i = 0; i < 3; i++, e++) {
        if (e->flags & 1) {
            if ((u32)e->id == (u32)kind) {
                return i;
            }
            if (enemy_kind_same_ck((u8)e->id, kind) == 1) {
                return i;
            }
        }
    }
    return -1;
}

/* Whether the local slot's item work has its arena item count set. */
u32 quest_item_work_flag_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    Q_ItemWork* item;

    if (work == NULL) {
        return 0;
    }
    item = work->item_work;
    if (item == NULL) {
        return 0;
    }
    return (s8)item->count_0x6A2A != 0;
}

/* Copies the 0x20-byte key block one of those rows carries. */
void quest_element_copy(Q_ElementBlock* dst, const Q_ElementBlock* src) {
    *dst = *src;
}

/* Copies the 8-byte slot pair a result row is built from. */
void quest_pair_copy(Q_SlotPair* dst, const Q_SlotPair* src) {
    *dst = *src;
}

/* Clears the area handle list, loads the result row's two arena items and builds one handle per entry of
 * its area block (at most 32). */
void quest_area_list_init(void) {
    Q_ResultRow* rec;
    u8* area;
    u8 count;
    s32 i;

    for (i = 0; i < 0x20; i++) {
        QUEST_WORK->area_0x69A8[i] = 0;
    }
    rec = QUEST_WORK->record_0x3C;
    if (rec->items_0x384[0].id == 0) {
        QUEST_WORK->arena_items_0x6A2C[0].id = 0;
        QUEST_WORK->arena_items_0x6A2C[0].count = 0;
        QUEST_WORK->arena_items_0x6A2C[0].remaining = 0;
    } else {
        QUEST_WORK->arena_items_0x6A2C[0].id = rec->items_0x384[0].id;
        QUEST_WORK->arena_items_0x6A2C[0].count = rec->items_0x384[0].count;
        QUEST_WORK->arena_items_0x6A2C[0].remaining = rec->items_0x384[0].remaining;
    }
    if (rec->items_0x384[1].id == 0) {
        QUEST_WORK->arena_items_0x6A2C[1].id = 0;
        QUEST_WORK->arena_items_0x6A2C[1].count = 0;
        QUEST_WORK->arena_items_0x6A2C[1].remaining = 0;
    } else {
        QUEST_WORK->arena_items_0x6A2C[1].id = rec->items_0x384[1].id;
        QUEST_WORK->arena_items_0x6A2C[1].count = rec->items_0x384[1].count;
        QUEST_WORK->arena_items_0x6A2C[1].remaining = rec->items_0x384[1].remaining;
    }
    QUEST_WORK->area_state_0x6A28 = 0;
    QUEST_WORK->flag_0x6A29 = 0;
    QUEST_WORK->count_0x6A2A = 0;
    if (rec->area_ofs_0x37C != 0) {
        area = (u8*)(rec->area_ofs_0x37C + (u32)QUEST_WORK->record_0x3C);
    } else {
        area = NULL;
    }
    count = (u8)stage_map_area_count_get(QUEST_WORK->record_0x3C->state_0x8B);
    if ((s32)count > 32) {
        count = 32;
    }
    QUEST_WORK->area_count_0x69A5 = count;
    if (area != NULL) {
        for (i = 0; i < count; i++) {
            QUEST_WORK->area_0x69A8[i] = em_area_entry_make(area, 0, (u8)i);
        }
    }
    QUEST_WORK->hunt_active_0x675C = 1;
}

/* Rebuilds the area handle list for entry kind `kind`, once: only while the list still waits in state 1. */
void quest_area_list_refill(Q_ItemWork* item, u8 kind) {
    Q_ResultRow* rec;
    s32 i;
    u8 count;
    u8* area;

    if ((s8)QUEST_WORK->area_state_0x6A28 == 1) {
        rec = QUEST_WORK->record_0x3C;
        count = (u8)stage_map_area_count_get(rec->state_0x8B);
        if (rec->area_ofs_0x37C != 0) {
            area = (u8*)(rec->area_ofs_0x37C + (u32)QUEST_WORK->record_0x3C);
            for (i = 0; i < count; i++) {
                QUEST_WORK->area_0x69A8[i] = em_area_entry_make(area, kind, (u8)i);
            }
            QUEST_WORK->area_state_0x6A28 = 2;
        }
    }
}

/* Charges one faint of `player` against the quest reward: announces it, and ends the quest when the reward
 * is gone. */
void quest_reward_faint_penalty(u8 player) {
    char text[128];
    s32 amount;

    if (system_w.field_0x7d2 == 1) {
        return;
    }
    if (system_w.field_0x7d1 == 1) {
        return;
    }
    if (QUEST_WORK->record_0x3C == NULL) {
        return;
    }
    if (QUEST_WORK->reward_0x2E8 <= 0) {
        return;
    }
    if (quest_sub_state_end_ck(1) != 0) {
        return;
    }
    amount = QUEST_WORK->penalty_0x2F4;
    if (QUEST_WORK->reward_0x2E8 < amount) {
        amount = QUEST_WORK->reward_0x2E8;
    }
    QUEST_WORK->reward_0x2E8 = QUEST_WORK->reward_0x2E8 - amount;
    if (player == (s8)my_player_no()) {
        QUEST_WORK->my_faint_count_0x92 += 1;
    }
    QUEST_WORK->faint_count_0x91 += 1;
    if (isServerSelectState() == 1) {
        if (player == (s8)my_player_no()) {
            hud_item_msg_push(1, 2, 0);
        } else {
            sprintf(text, quest_entry_fmt_str, ((_PLW*)get_move_work_adrs(2))[player].user_profile_0x5CA);
            strcat(text, quest_str_tbl_4_get(0x25));
            hud_msg_push(0, text);
        }
    } else {
        hud_item_msg_push(1, 2, 0);
    }
    sprintf(text, quest_str_tbl_35_get(9), amount);
    hud_msg_push(0, text);
    if (QUEST_WORK->reward_0x2E8 <= 0) {
        QUEST_WORK->reward_0x2E8 = 0;
        hud_msg_push(0, quest_str_tbl_35_get(10));
        hud_msg_push(0, quest_str_tbl_35_get(0x13));
        return;
    }
    if (player == (s8)my_player_no()) {
        snd_quest_scene_set();
        snd_player_mask_set(player);
        hud_msg_push(0, quest_str_tbl_35_get(0x12));
    }
}

/* Whether the quest work is busy: either of the system block's two pre-quest flags, or a live work
 * whose record is missing or whose +0x2E8 counter is set. */
u32 quest_work_busy_ck(void) {
    if (system_w.field_0x7d2 == 1) {
        return 1;
    }
    if (system_w.field_0x7d1 == 1) {
        return 1;
    }
    if (quest_work_ptr->record_0x03C != 0) {
        return quest_work_ptr->field_0x2E8 > 0;
    }
    return 1;
}

/* Warps the player to the quest's start position: the local one for a network quest, a fixed spot for the
 * two arena phases, else the stage's own start. */
void quest_start_warp(_PLW* plw) {
    nw4r::math::VEC3 pos;
    s32 angle;
    Q_MoveWork* work;
    u8 saved;
    u8 phase;

    setVec3(&pos, 0.0f, 0.0f, 0.0f);
    work = (Q_MoveWork*)get_move_work_adrs(0);
    if (quest_work_busy_ck() != 0) {
        if (PlayMode_ck() == 2) {
            saved = my_player_no();
            my_player_no_set((s8)plw->chunk_ofs);
            stage_area_start_get(work->sub_0xEA, &pos, &angle);
            pl_act_stage_latch_set(my_player_work_get(), 2);
            pl_warp_start(0xFF, &pos, (u16)angle);
            my_player_no_set((s8)saved);
            return;
        }
        phase = work->phase_0xE9;
        if ((phase == 6 || phase == 0x11) && plw->area_0x16 == 2) {
            pos.x = 9960.0f;
            pos.y = 230.0f;
            pos.z = 300.0f;
            angle = 0xC000;
            pl_act_stage_latch_set(my_player_work_get(), 2);
            pl_warp_start(2, &pos, (u16)angle);
            return;
        }
        stage_start_get(phase, &pos, &angle);
        pl_act_stage_latch_set(my_player_work_get(), 2);
        pl_warp_start(0, &pos, (u16)angle);
    }
}

/* Enters the quest result state (sub-state 5): latches the time limit, clears the AI slots, fills the
 * result stat and posts the kind's message. */
void quest_result_enter(Q_ItemWork* item, Q_MoveWork* work, u8 kind) {
    work->sub_0xFA = 5;
    item->time_base_0x20 = item->time_limit_0x24;
    item->step_0x2C = 2;
    item->field_0x2D = 0;
    quest_time_limit_set((s32)(quest_grade_ratio_10f * Screen_w.frame_scale));
    ai_slots_clear();
    quest_result_stat_fill((QuestWork*)item);
    if (kind == 1) {
        hud_msg_push(0, quest_str_tbl_35_get(8));
    }
    if (kind == 2) {
        hud_msg_push(0, quest_str_tbl_35_get(0x1E));
    }
    if (kind == 3) {
        hud_msg_push(0, quest_str_tbl_35_get(0x21));
    }
    snd_quest_result_bgm_set();
}

/* Enters the quest start state (sub-state 3): latches the time limit and posts the entry message the
 * quest's objective and the entry-send header select. */
void quest_start_enter(Q_ItemWork* item, Q_MoveWork* work) {
    u8 objective;
    Q_ResultRow* rec;

    work->sub_0xFA = 3;
    system_copy_filter_request();
    item->time_base_0x20 = item->time_limit_0x24;
    item->step_0x2C = 2;
    item->field_0x2D = 0;
    if (work->kind_0xFC != 4) {
        if (item->entry_send_0x6A3A[0] == 0) {
            hud_msg_push(0, quest_str_tbl_35_get(0x17));
        } else {
            switch (item->entry_send_0x6A3A[1]) {
            case 0:
                hud_msg_push(0, quest_str_tbl_35_get(0x17));
                break;
            case 1:
                hud_msg_push(0, quest_str_tbl_35_get(0x1F));
                break;
            case 2:
                hud_msg_push(0, quest_str_tbl_35_get(0x20));
                break;
            }
        }
    }
    if (work->kind_0xFC == 4) {
        quest_time_limit_set((s32)(quest_grade_ratio_10f * Screen_w.frame_scale));
        if (system_w.field_0x7cf == 0x15) {
            hud_msg_push(0, quest_str_tbl_35_get(0));
        } else {
            hud_msg_push(0, quest_str_tbl_35_get(1));
        }
    } else {
        rec = (Q_ResultRow*)quest_record_get();
        if ((rec != NULL && (rec->flags_0x310 & 0x20000000) != 0) || (objective = quest_objective_get(NULL), objective == 1) ||
            objective == 4) {
            quest_time_limit_set((s32)(20.0f * Screen_w.frame_scale));
            if (PlayMode_ck() == 2) {
                hud_msg_push(0, quest_str_tbl_35_get(0x27));
            } else if (system_w.field_0x7cf == 0x15) {
                hud_msg_push(0, quest_str_tbl_35_get(2));
            } else {
                hud_msg_push(0, quest_str_tbl_35_get(3));
            }
        } else {
            quest_time_limit_set((s32)(frames_per_second_60f * Screen_w.frame_scale));
            if (system_w.field_0x7cf == 0x15) {
                hud_msg_push(0, quest_str_tbl_35_get(4));
            } else {
                hud_msg_push(0, quest_str_tbl_35_get(5));
            }
        }
    }
    snd_quest_start_bgm_set();
}

/* Whether the quest work's entry-send header has been started. */
u32 quest_entry_active_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    return QUEST_WORK->entry_send_0x6A3A[0] != 0;
}

/* Whether the entry may start: the header must be live and the quest's 0x800000 condition satisfied. */
u32 quest_entry_ready_ck(void) {
    if (quest_work_ptr == NULL) {
        return 0;
    }
    if (QUEST_WORK->entry_send_0x6A3A[0] == 0) {
        return 0;
    }
    if (quest_flag_800000_ck(NULL) == 0) {
        return 0;
    }
    return quest_entry_active_ck();
}

/* Whether the local slot's sub-state byte is the entry pair (6 or 7), or 4. */
u32 quest_move_sub_state_ck(void) {
    Q_MoveWork* work = (Q_MoveWork*)get_move_work_adrs(0);
    u8 state;

    if (work == NULL) {
        return 0;
    }
    state = work->sub_0xFA;
    if (state == 4) {
        return 1;
    }
    return (u8)(state + 250) <= 1;
}

/* Steps the quest finish sequence: waits (the hunt-end wait is five seconds of frames), then respawns the
 * quest's monster once for the session and hands over to the lobby. */
void quest_finish_step(void) {
    Q_ItemWork* work;

    if (quest_flag_80000000_ck(NULL) == 0) {
        return;
    }
    if (get_move_work_adrs(0) == NULL) {
        return;
    }
    work = QUEST_WORK;
    if (work == NULL) {
        return;
    }
    switch (work->phase_0x6978) {
    case 0:
        work->counter_0x697C = work->counter_0x697C + 1;
        if (work->hunt_end_0x6977 != 0) {
            if ((f32)work->counter_0x697C < 5.0f * (frames_per_second_60f * Screen_w.frame_scale)) {
                return;
            }
        }
        if (isServerSelectState() == 1) {
            work->phase_0x6978 = 2;
        } else {
            work->phase_0x6978 = 1;
        }
        break;
    case 1:
        if (isServerSelectState() == 1) {
            if (isReadyCountOne() != 0) {
                if (work->respawn_kind_0x321 != 0 && work->respawn_gate_0x323 == 3) {
                    em_spawn_request(0xFFFF, work->respawn_kind_0x321, 0, 0xFF, 0xFF, 1, 1, 0, 0xFF, NULL, 0);
                    lb_sub18_send();
                }
                work->phase_0x6978 = 3;
            }
        } else {
            work->phase_0x6978 = 2;
        }
        break;
    case 2:
        if (work->respawn_kind_0x321 != 0 && work->respawn_gate_0x323 == 3) {
            em_spawn_request(0xFFFF, work->respawn_kind_0x321, 0, 0xFF, 0xFF, 1, 1, 0, 0xFF, NULL, 0);
        }
        work->phase_0x6978 = 3;
        break;
    }
}

/* This unit's own `.bss` (`config/RMHE08/splits.txt`, `.bss 0x806C5558..0x806C5858`): the band's four
 * picked-pair tables, 48 four-byte pairs each.  The lot picks write them and the result rows are built
 * from them - `quest_item_pair_tbl_copy` moves `_a`/`_b` into two adjacent 0xC0 buffers of a result
 * record, `quest_element_clear` zeroes all four and refills `_a` from the item work's own lot table. */
Q_ItemPair quest_item_pair_tbl_a[0x30];
Q_ItemPair quest_item_pair_tbl_b[0x30];
Q_ItemPair quest_item_pair_tbl_c[0x30];
Q_ItemPair quest_item_pair_tbl_d[0x30];

/* This unit's own `.data` (`config/RMHE08/splits.txt`, `.data 0x805F7AB0..0x805F7AF8`, the four rows
 * the readers of each table are the only referrers of): the band's reward roll tables.  Each chance
 * table is one byte per pick slot - 32 or 22 - in the shape `quest_element_clear` builds in-line, and
 * the four runs tile the claim exactly (0x10 + 0x18 + 0x10 + 0x10). */
u8 quest_pair_chance_tbl_a[0x10] = {
    32, 32, 32, 22, 22, 22, 22, 22, 32, 22, 22, 22, 22, 22, 22, 22,
};
Q_RewardGroup quest_reward_group_tbl[6] = {
    { 0, 0, 0 }, { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 1 }, { 4, 2, 1 }, { 5, 3, 0 },
};
u8 quest_pair_chance_tbl_b[0x10] = {
    32, 32, 22, 22, 22, 22, 22, 22, 32, 32, 22, 22, 22, 22, 22, 22,
};
u8 quest_pair_chance_tbl_c[0x10] = {
    32, 32, 22, 22, 22, 22, 22, 22, 32, 32, 22, 22, 22, 22, 22, 22,
};

/* This unit's own `.sbss` (`splits.txt` `.sbss 0x80794C1C..0x80794C3C`, in address order): the quest list
 * block `quest_list_load_hunt`/`_arena` allocate (GUESS names: `quest_list_pool` is the 0x4400-byte
 * `work_mem_alloc` block, `quest_list_values` is pool + 0x1A0 - read by `menu/arena_result.cpp`, see
 * `quest/quest_list_values.h` - and `quest_list_file` is that + 0xE0, the `load_file` destination), then the
 * five table pointers the roll functions read - the per-kind entry lists and the two pairs of remaining-lot
 * tables by count (GUESS: the dump names them `em_bui_tbl`, `em_bui_rem_l/h` and `em_hokaku_rem_l/h`).  Set
 * outside this unit. */
u8* quest_list_file;
u8* quest_list_pool;
u16* quest_list_values;
u8** em_bui_tbl;
Q_LotEntry** em_bui_rem_l;
Q_LotEntry** em_bui_rem_h;
Q_LotEntry** em_hokaku_rem_l;
Q_LotEntry** em_hokaku_rem_h;


extern "C" {

/* The result screen's localized "(Quest Name Unavailable)" rows, indexed by the message-language
 * index `system_w.field_0x09`; the first two entries are the same English row, so `-str reuse`
 * pools one literal for both.  These five strings and the table are this band's `.data`
 * (0x8060E800..0x8060E8A0), UNCLAIMED and left to its auto unit - which is also the second
 * definition of this name, the clash the file header's data paragraph records - emitted here
 * because `quest_result_field_text_get` is their only referencer.  The three accented characters
 * are written as byte escapes: the source has to stay pure ASCII or `sjiswrap` re-encodes it. */
const char* quest_name_unavailable_text[6] = {
    "(Quest Name Unavailable)",
    "(Quest Name Unavailable)",
    "(Nom de qu\xC3\xAAte indispo.)",
    "(Questname nicht verf\xC3\xBCgb.)",
    "(Nome missione non disp.)",
    "(Misi\xC3\xB3n no disponible)",
};

/* Prototypes for the definitions below that an earlier function calls (`quest_item_count_sum` is
 * defined at the address its own body lives at, after its two callers). */
s16 quest_item_count_sum(u16 id, s32 who);
QuestRecord* quest_record_get(void);

/* The current result row, or NULL when the screen has none. */
QuestRecord* quest_record_get(void) {
    QuestRecord* rec = quest_work.record_0x03C;

    if (rec == NULL) {
        return NULL;
    }
    return rec;
}

/* String table 35's `index`-th entry (the item-summary formats). */
char* quest_str_tbl_35_get(u32 index) {
    return (char*)get_str_tbl(35)[index];
}

/* String table 4's `index`-th entry. */
char* quest_str_tbl_4_get(u32 index) {
    return (char*)get_str_tbl(4)[index];
}

/* The pair table's `[row][col]` byte at +0x805F7898. */
u8 quest_pair_table_get(u8 row, u8 col) {
    return *(col + (&quest_pair_table[0] + (row * 2)));
}

/* The byte table's `index`-th entry at +0x805F78B4. */
u8 quest_byte_table_get(u8 index) {
    return *(&quest_byte_table[0] + index);
}

/* The record's name out of string table 5 (an empty string when there is no record). */
char* quest_name_text_get(void) {
    QuestRecord* rec = quest_record_get();
    u8** tbl = get_str_tbl(5);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        strcpy(quest_text_buffer, (char*)tbl[rec->field_0x08B]);
    }
    return quest_text_buffer;
}

/* The same name for an explicit record. */
char* quest_name_text_get_of(QuestRecord* rec) {
    u8** tbl = get_str_tbl(5);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        strcpy(quest_text_buffer, (char*)tbl[rec->field_0x08B]);
    }
    return quest_text_buffer;
}

/* The record's +0x198 entry out of string table 41. */
char* quest_field198_text_get(void) {
    QuestRecord* rec = quest_record_get();
    u8** tbl = get_str_tbl(41);

    quest_text_buffer[0] = 0;
    if (rec == NULL) {
        return quest_text_buffer;
    }
    strcpy(quest_text_buffer, (char*)tbl[rec->field_0x198]);
    return quest_text_buffer;
}

/* The same +0x198 entry for an explicit record. */
char* quest_field198_text_get_of(QuestRecord* rec) {
    u8** tbl = get_str_tbl(41);

    quest_text_buffer[0] = 0;
    strcpy(quest_text_buffer, (char*)tbl[rec->field_0x198]);
    return quest_text_buffer;
}

/* The record's +0x13A value formatted with string table 6's first format. */
char* quest_field13A_text_get(void) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[0], rec->field_0x13A);
    }
    return quest_text_buffer;
}

/* The same +0x13A value for an explicit record. */
char* quest_field13A_text_get_of(QuestRecord* rec) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[0], rec->field_0x13A);
    }
    return quest_text_buffer;
}

/* The record's +0x34C / +0x350 / +0x354 timers as text: `which` picks which of the three. */
char* quest_time_text_get(u8 which) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        switch (which) {
        case 0:
            sprintf(quest_text_buffer, tbl[1], rec->field_0x34C);
            break;
        case 1:
            if (rec->field_0x350 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        case 2:
            if (rec->field_0x354 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        }
    }
    return quest_text_buffer;
}

/* The same three timers for an explicit record. */
char* quest_time_text_get_of(QuestRecord* rec, u8 which) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        switch (which) {
        case 0:
            sprintf(quest_text_buffer, tbl[1], rec->field_0x34C);
            break;
        case 1:
            if (rec->field_0x350 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        case 2:
            if (rec->field_0x354 == 0) {
                sprintf(quest_text_buffer, tbl[4]);
            } else {
                sprintf(quest_text_buffer, tbl[1]);
            }
            break;
        }
    }
    return quest_text_buffer;
}

/* The record's +0x348 value as text. */
char* quest_field348_text_get(void) {
    QuestRecord* rec = quest_record_get();
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[1], rec->field_0x348);
    }
    return quest_text_buffer;
}

/* The same +0x348 value for an explicit record. */
char* quest_field348_text_get_of(QuestRecord* rec) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    if (rec != NULL) {
        sprintf(quest_text_buffer, tbl[1], rec->field_0x348);
    }
    return quest_text_buffer;
}

/* The work block's +0x2E8 value (clamped up to 0) as text. */
char* quest_field2E8_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 value;

    quest_text_buffer[0] = 0;
    value = quest_work.field_0x2E8;
    if (value < 0) {
        value = 0;
    }
    sprintf(quest_text_buffer, tbl[1], value);
    return quest_text_buffer;
}

/* The run's clear time (`quest_work` +0x24 frames) as `mm'ss"ff` text. */
char* quest_clear_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x024;
    f32 frames = frames_per_second_60f * Screen_w.frame_scale;
    s32 minutes = time / (s32)frames;
    s32 seconds = (time - minutes * (s32)frames) / (s32)Screen_w.frame_scale;

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds, Screen_w.frame_scale);
    return quest_text_buffer;
}

/* The elapsed time (`quest_work` +0x1C minus +0x20 frames) as the same text. */
char* quest_elapsed_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = quest_work.field_0x01C - quest_work.field_0x020;
    f32 frames = frames_per_second_60f * Screen_w.frame_scale;
    s32 minutes = time / (s32)frames;
    s32 seconds = (time - minutes * (s32)frames) / (s32)Screen_w.frame_scale;

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds, Screen_w.frame_scale);
    return quest_text_buffer;
}

/* The elapsed time in frames. */
s32 quest_elapsed_time_get(void) {
    return quest_work.field_0x01C - quest_work.field_0x020;
}

/* The run's score pair (+0x90/+0x91) as text. */
char* quest_score_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[3], quest_work.field_0x091, quest_work.field_0x090);
    return quest_text_buffer;
}

/* The arena item tally as text: the "N of M" string table 35's format 26 carries when the player
 * still holds items and the run is short of them, else the "none" format 27.  `index` 0 sums every
 * arena element, otherwise it reads element `index` alone; the `quest_flag_800000_ck` case reports
 * the whole set at once. */
char* quest_arena_items_text_get(u8 index) {
    s32 have;
    s32 missing;
    s32 count;
    s32 i;
    s32 need;
    s32 value;

    quest_text_buffer[0] = 0;
    have = 0;
    if (index != 0 && quest_flag_800000_ck(NULL) == 1) {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(27));
        return quest_text_buffer;
    }
    if (quest_flag_80000_ck(NULL) == 1 || quest_flag_2000000_ck(NULL) == 1 ||
        quest_flag_80000000_ck(NULL) == 1 || quest_flag_4000000_ck(NULL) == 1) {
        count = (quest_flag_100_ck(NULL) == 1) ? 2 : 3;
        need = 0;
        for (i = 0; i < count; i++) {
            value = quest_arena_count_get(i);
            if (value >= 0) {
                have += value;
            }
            value = quest_arena_need_get(i);
            if (value >= 0) {
                need += value;
            }
        }
        missing = have - need;
    } else {
        have = quest_arena_count_get(index);
        missing = have - quest_arena_need_get(index);
    }
    if (missing < 0 || have <= 0) {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(27));
    } else {
        sprintf(quest_text_buffer, quest_str_tbl_35_get(26), missing, have);
    }
    return quest_text_buffer;
}

/* The arena run's time as the same `mm'ss"ff` text `quest_clear_time_text_get` builds, from the
 * record's clear time instead of the work block's.  The clear time is first mapped through the
 * arena's own tables: the fixed `multi_arena_clr_time` pair when the 0x100000 flag is set and the
 * time is below 10000, else `arena_time_table`'s per-rank table scaled by 30 when 0x10 is set. */
char* quest_arena_time_text_get(QuestRecord* rec, s32 which) {
    char** tbl = (char**)get_str_tbl(6);
    s32 time = which;
    f32 frames;
    s32 minutes;
    s32 seconds;

    if (quest_flag_100000_ck(rec) == 1 && rec->field_0x02C < 10000) {
        time = multi_arena_clr_time[(rec->field_0x02C - 9000) * 2 + (system_w.field_0x8af != 0)];
    } else if (quest_flag_10_ck(rec) == 1) {
        time = arena_time_table[rec->field_0x02C - 60000][(u8)which] * 30;
    }
    frames = frames_per_second_60f * Screen_w.frame_scale;
    minutes = time / (s32)frames;
    seconds = (time - minutes * (s32)frames) / (s32)Screen_w.frame_scale;
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[2], minutes, seconds);
    return quest_text_buffer;
}

/* The grade (0 = no record) as text: the grade's own format for 1..4, else the language's
 * "no record" string. */
char* quest_grade_text_get(u8 grade) {
    char** tbl = (char**)get_str_tbl(6);
    u32 slot;

    switch (grade) {
    case 0:
        quest_text_buffer[0] = 0;
        sprintf(quest_text_buffer, quest_grade_none_text_table[system_w.field_0x09]);
        return quest_text_buffer;
    case 1:
    case 2:
    case 3:
    case 4:
        slot = grade + 5;
        break;
    }
    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[slot]);
    return quest_text_buffer;
}

/* The current run's grade as text: the same ratio `quest_grade_get` scores, rendered by
 * `quest_grade_text_get` (1 when no element matches, so the "no record" string comes out). */
char* quest_grade_text_cur_get(void) {
    s16 total;
    f32 ratio;
    u8 grade = 1;

    quest_text_buffer[0] = 0;
    if (quest_element_value_get(4, &total) == 0) {
        return quest_text_buffer;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    if (ratio <= quest_grade_ratio_10f) {
        grade = 4;
    } else if (ratio <= quest_grade_ratio_30f) {
        grade = 3;
    } else if (ratio <= quest_grade_ratio_50f) {
        grade = 2;
    }
    return quest_grade_text_get(grade);
}

/* The run's grade: the score as a percentage of the arena element 4 target, banded at the three
 * `quest_grade_ratio_*f` thresholds (0 when the element is not live). */
s32 quest_grade_get(void) {
    s16 total;
    f32 ratio;

    if (quest_element_value_get(4, &total) == 0) {
        return 0;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    if (ratio <= quest_grade_ratio_10f) {
        return 4;
    }
    if (ratio <= quest_grade_ratio_30f) {
        return 3;
    }
    if (ratio <= quest_grade_ratio_50f) {
        return 2;
    }
    return 1;
}

/* Whether the same percentage reaches `grade` - the rank the caller's threshold asks for. */
s32 quest_grade_rank_get(u8 grade) {
    s16 total;
    f32 ratio;

    if (quest_element_value_get(4, &total) == 0) {
        return 0;
    }
    ratio = percent_scale_100f * ((f32)quest_work.field_0x5D8 / (f32)total);
    return ratio <= (f32)grade;
}

/* The record's +0x36C slot progress byte, or 0 while the work state is "moving" and for a missing
 * record. */
u8 quest_slot_progress_get(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 0;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x36C;
}

/* The record's name index (string table 5's key), or 0 for a missing record; 1 while the work
 * state is "moving". */
u8 quest_name_index_get(QuestRecord* rec) {
    if (move_work_state_ck() == 1) {
        return 1;
    }
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x08B;
}

/* The record's +0x372 word, or the current record's. */
u16 quest_field372_get(QuestRecord* rec) {
    if (rec == NULL) {
        rec = quest_record_get();
        if (rec == NULL) {
            return 0;
        }
    }
    return rec->field_0x372;
}

/* The "no time yet" text (string table 6's fifth format). */
char* quest_no_time_text_get(void) {
    char** tbl = (char**)get_str_tbl(6);

    quest_text_buffer[0] = 0;
    sprintf(quest_text_buffer, tbl[5]);
    return quest_text_buffer;
}

/* The record's +0x780 `which`-th monster id rendered through string table 33, the "no time yet"
 * text when the id is 0 or the record is missing, and a lone space for the second slot while both
 * ids are still unset. */
char* quest_monster_text_get(QuestRecord* rec, u8 which) {
    if (rec == NULL) {
        return quest_no_time_text_get();
    }
    if (which == 1 && rec->field_0x30C[0] == 0 && rec->field_0x30C[1] == 0) {
        quest_text_buffer[0] = ' ';
        quest_text_buffer[1] = 0;
        return quest_text_buffer;
    }
    if (rec->field_0x30C[which] == 0) {
        return quest_no_time_text_get();
    }
    strcpy(quest_text_buffer, (char*)str_tbl_33_get(rec->field_0x30C[which]));
    return quest_text_buffer;
}

/* Applies `used` of the arena item `id` to the work block's item table (the entry the table's own
 * count has room for). */
void quest_element_item_apply(u8 id, u16 count, u16 used) {
    QuestWork* work = quest_work_ptr;
    QuestArenaItem* item;
    s8 index;
    s32 rest;

    if (work == NULL) {
        return;
    }
    index = work->count_0x6A2A;
    if (index >= 2) {
        return;
    }
    item = &work->arena_items_0x6A2C[index];
    if (item->id != id) {
        return;
    }
    if (item->count != count) {
        return;
    }
    if (item->remaining == 0) {
        return;
    }
    rest = item->remaining - used;
    if (rest < 0) {
        rest = 0;
    }
    item->remaining = rest;
}

/* The number of u16 entries the work block's item stack `index` holds (0 when out of range). */
s32 quest_slot_count_get(u8 index) {
    QuestWork* work = quest_work_ptr;

    if (work == NULL) {
        return 0;
    }
    if (index >= 8) {
        return 0;
    }
    return work->slot_counts_0x66B8[index];
}

/* Copies the work block's item stack `index` into `out`. */
void quest_slot_items_get(u8 index, u16* out) {
    QuestWork* work = quest_work_ptr;
    u16* src;
    s32 i;

    if (work == NULL) {
        return;
    }
    if (index >= 8) {
        return;
    }
    src = work->slot_values_0x6698[index];
    for (i = 0; i < work->slot_counts_0x66B8[index]; i++) {
        out[i] = src[i];
    }
}

/* Clears both halves of every entry of the run's 35-slot item list whose id is not a usable one. */
void quest_item_list_prune(QuestItemSlot* slots) {
    s32 i;
    QuestItemSlot* slot = slots;

    for (i = 0; i < 35; i++, slot++) {
        if (Pl_item_id_usable_ck(slot->id, 0) == 0) {
            slot->id = 0;
            slot->count = 0;
        }
    }
}

/* The u16 the `quest_list_items` entry whose +0x2C key is `key` pairs with (0 when there is none). */
u16 quest_work_word_get(u16 key) {
    QuestListItem** items = quest_list_items;
    s32 count = quest_list_count;
    s32 i;

    for (i = 0; i < count; i++) {
        if (items[i]->field_0x02C == key) {
            break;
        }
    }
    if (i >= count) {
        return 0;
    }
    return quest_list_values[i];
}

/* Whether exactly one of the three key bytes of entry 6 is set and all of them are finished (3). */
s32 quest_players_state_get(void) {
    QuestWork* work = quest_work_ptr;
    s32 done = 0;
    s32 active = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        u8 state = work->player_state_0x2D4[i];

        if (state == 3) {
            done = 1;
        }
        if (state != 0) {
            active++;
        }
    }
    if (done == 1 && active <= 1) {
        return 1;
    }
    return 0;
}

/* Clears element `index` of the work block's element array. */
void quest_element_reset(u8 index) {
    if (quest_work_ptr != NULL) {
        quest_element_set(quest_work_ptr, index, 0);
    }
}

/* Sets element `index`'s value to 0 once `id`'s own count has caught up with its target. */
void quest_element_value_set(s32 index, u16 id) {
    QuestWork* work = quest_work_ptr;
    s32 i;

    if (work == NULL) {
        return;
    }
    for (i = index; i < 3; i++) {
        QuestElement* el = &work->elements_0x0094[i];

        if ((el->flags & 0x8) != 0 && (el->flags & 0x2) != 0) {
            if (el->id == id) {
                if (quest_item_count_sum(id, 0) >= el->value) {
                    quest_element_set(work, (u16)i, 0);
                }
            }
        }
    }
}

/* How many of the item `id`'s count `element`'s value still asks for, returning the element's id
 * (`out` gets the remainder, or 0 when the element is not live). */
u16 quest_element_item_count_get(s32 index, u16* out) {
    QuestElement* el;
    s16 rest;

    if (quest_work_ptr == NULL) {
        return 0;
    }
    el = &quest_work_ptr->elements_0x0094[index];
    *out = 0;
    if ((el->flags & 0x8) == 0) {
        return 0;
    }
    if ((el->flags & 0x2) == 0) {
        return 0;
    }
    rest = el->value - quest_item_count_sum(el->id, 0);
    if (rest < 0) {
        rest = 0;
    }
    *out = rest;
    return el->id;
}

/* The item `id`'s count: `who` 0 sums every player's slot, 1 the local player's, 2 and up the
 * player whose slot follows. */
s16 quest_item_count_sum(u16 id, s32 who) {
    QuestWork* work;
    QuestItemSlot* slots;
    s16 total = 0;
    s32 player;
    s32 i;
    s32 j;

    if (get_move_work_adrs(0) == NULL) {
        return 0;
    }
    switch (who) {
    case 0:
        work = quest_work_ptr;
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                if (work->player_items_0x6A6A[i][j].id == id) {
                    total += work->player_items_0x6A6A[i][j].count;
                }
            }
        }
        break;
    case 1:
        player = (s8)my_player_no();
        slots = &quest_work_ptr->player_items_0x6A6A[player][0];
        for (i = 0; i < 3; i++) {
            if (slots[i].id == id) {
                return slots[i].count;
            }
        }
        break;
    default:
        player = (s8)(who - 2);
        slots = &quest_work_ptr->player_items_0x6A6A[player][0];
        for (i = 0; i < 3; i++) {
            if (slots[i].id == id) {
                return slots[i].count;
            }
        }
        break;
    }
    return total;
}

/* Stores `value` into player `player`'s slot holding arena item `id` (taking a free slot when none
 * does), then clears every element whose remaining count has run out. */
void quest_arena_data_step(u8 player, u16 id, s16 value) {
    QuestItemSlot* slots = quest_work.player_items_0x6A6A[player];
    s16 count = 0;
    s16 rest;
    s32 i;

    if (slots[0].id == id) {
        count = slots[0].count;
    }
    if (slots[1].id == id) {
        count = slots[1].count;
    }
    if (slots[2].id == id) {
        count = slots[2].count;
    }
    if (count == 0) {
        for (i = 0; i < 3; i++) {
            if (slots[i].id == 0) {
                slots[i].id = id;
                slots[i].count = value;
                break;
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (slots[i].id == id) {
                slots[i].count = value;
                break;
            }
        }
    }
    for (i = 0; i < 3; i++) {
        rest = (s16)quest_element_value_at(id, (u16)i);
        if (rest < 0) {
            continue;
        }
        rest = rest - (s16)quest_all_player_item_count_sum(id);
        if (rest <= 0) {
            quest_element_reset((u8)i);
        }
    }
}

/* The text a result-screen field kind shows: kinds 0..31 pick one of the record's own text runs or
 * one of this band's text getters, and a null record falls back to the "name unavailable" table. */
char* quest_result_field_text_get(QuestRecord* rec, u8 kind) {
    char buf[256];
    char* brk;

    buf[0] = 0;
    quest_text_buffer[0] = 0;
    if (rec == NULL) {
        if (kind == 0) {
            strcpy(quest_text_buffer, quest_name_unavailable_text[system_w.field_0x09]);
        } else {
            sprintf(quest_text_buffer, " ");
        }
        return quest_text_buffer;
    }
    switch (kind) {
    case 0:
        msg_str_gen(rec->field_0x000, quest_text_buffer);
        break;
    case 22:
        msg_str_gen(rec->field_0x000, buf);
        if ((u32)flfntStrLen(buf) > 22) {
            brk = flKnjMsgNumPtr(buf, 19);
            if (brk != NULL) {
                *brk = 0;
                strcat(buf, "...");
            }
        }
        strcpy(quest_text_buffer, buf);
        break;
    case 1:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        break;
    case 16:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        brk = flfntStrChr(quest_text_buffer, 10);
        if (brk != NULL) {
            *brk = 0;
        }
        break;
    case 17:
        msg_str_gen(rec->field_0x02E, quest_text_buffer);
        brk = flfntStrChr(quest_text_buffer, 10);
        if (brk != NULL) {
            strcpy(buf, brk + 1);
            strcpy(quest_text_buffer, buf);
        } else {
            quest_text_buffer[0] = 0;
        }
        break;
    case 2:
        if (quest_flag_10000000_ck(rec) == 1) {
            return quest_grade_text_get(0);
        }
        msg_str_gen(rec->field_0x08C, quest_text_buffer);
        break;
    case 3:
        msg_str_gen(rec->field_0x0B5, quest_text_buffer);
        break;
    case 4:
        msg_str_gen(rec->field_0x0DE, quest_text_buffer);
        break;
    case 5:
        msg_str_gen(rec->field_0x13C, quest_text_buffer);
        break;
    case 6:
        return quest_field198_text_get_of(rec);
    case 7:
        msg_str_gen(rec->field_0x19A, quest_text_buffer);
        break;
    case 8:
        msg_str_gen(rec->field_0x1C9, quest_text_buffer);
        break;
    case 9:
        return quest_name_text_get_of(rec);
    case 10:
        return quest_field13A_text_get_of(rec);
    case 11:
        return quest_time_text_get_of(rec, 0);
    case 13:
        return quest_time_text_get_of(rec, 1);
    case 14:
        return quest_time_text_get_of(rec, 2);
    case 15:
        return quest_time_text_get_of(rec, 3);
    case 12:
        return quest_field348_text_get_of(rec);
    case 30:
        return quest_monster_text_get(rec, 0);
    case 31:
        return quest_monster_text_get(rec, 1);
    case 26:
        return quest_arena_time_text_get(rec, 0);
    case 27:
        return quest_arena_time_text_get(rec, 1);
    case 28:
        return quest_arena_time_text_get(rec, 2);
    default:
        break;
    }
    return quest_text_buffer;
}

}  /* extern "C" */
