/*
 * enemy/em020_prog.cpp - phase 4 unit, `.text` 0x8036CF64..0x80378F9C (138 functions, 49208 bytes).
 *
 * PHASE 4 (docs/splits/phase4, window d).  Fold of 3 registered units: em020_prog.cpp, em020_handlers.cpp,
 * em020_ai.cpp.  The functions below are the ones those sources define, in address order; every other function of the
 * range keeps its original bytes.  30 of 138 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  `enemy/em020_handlers.cpp` (was `Object(Matching)`) is folded in and demoted with the unit.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .data, .rodata, .sbss, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/enemy/em020_prog.cpp (written against its pre-phase-4 range) ---- */
/* enemy/em020_prog.cpp - the em020 enemy program, `.text` 0x8036CF64..0x80375084 (57 functions /
 * 0x8120 B), extab 0x800178E4..0x80017A5C (47 records) and extabindex 0x800373BC..0x800375F0
 * (47 x 12 B).  Each of the three runs is exactly the gap the bracketing split objects leave:
 * `auto_fn_8036CE78_text`'s record ends at 0x800178E4/0x800373BC and `auto_fn_80375084_text`'s
 * begins at 0x80017A5C/0x800375F0.
 *
 * WHAT IT IS.  The `.data` program table `em020_prog_tbl` (0x805EE098, 0x70 B, `scope:global`)
 * lists seven of this range's entry points - 0x8036E2BC, 0x8036E320, 0x8036E6B8, 0x80372D58,
 * 0x8036E570, 0x8036E574 and 0x803733BC - exactly the way `em035_prog_tbl` lists the registered
 * `enemy/em035_prog.cpp`'s, and every body drives the shared `_ENEMY_WORK` record through
 * `em_frame_check`/`em_after_frame_check`/`em_get_mot_no`/`em_act_ck`/`em_area_ck`, the motion
 * arming pair `em_move_mode_set`/`em_mot_set_blend`/`fn_8012F5C4` and the nw4r math helpers
 * (`setVector3`, `mulVecMat`, `rotVecY`, `calcVecAng2`/`calcVecAngX`), with the joint/effect
 * queries `get_joint_wmat_em`/`get_em_scale`/`get_em_chg_scale`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string: a relocation sweep of
 * all 47 split objects finds no `.c`/`.cpp` literal - the range's whole data reference set is the
 * `.sdata2` pool run 0x8079B820..0x8079BC64, the `.data` run 0x805EE108..0x805EE428 and the
 * `.rodata` labels 0x80570A20/60/AA0.  2. `dumpmap.py lookup` answers only `zz_XXXXXXXX_`
 * placeholders for every function in the range.  3. The module is `enemy` and the file is
 * `em020_prog`: the map's own global `em020_prog_tbl` (a sibling of `em001/em003/em008/em033/
 * em035/em019_prog_tbl`) lists this range's handlers by address, and the range's callee profile is
 * the enemy band's (`em_*`, `fn_8012*`/`fn_8013*`); the file name follows the registered
 * `enemy/em035_prog.cpp`'s scheme for the same kind of table.
 *
 * SEAM: UNPROVEN, and it is the one thing this registration cannot settle.  Neither edge has any
 * decisive class-1 evidence (no `__FILE__` string exists anywhere in the band), and the
 * `.sdata2` ordered partition only supports the cut without proving it:
 *   * 0x8079B81C (the neighbour below, proposal/80366618) | 0x8079B820..0x8079BC60 (this range,
 *     86 single-referrer labels, no inversion) | 0x8079BC64 (the neighbour above, Q118).  The
 *     entries either side are disjoint and ordered, which is the reliable class - but a single
 *     object's pool is *also* ordered by first use, so an ordered partition is consistent with
 *     both one TU and two adjacent ones (playbook 54's "candidate, never proof").
 *   * `em020_prog_tbl`'s non-null entries straddle the right edge (0x80375084, 0x80375290,
 *     0x803753A0, 0x80375424, 0x80375494 all lie in the next two proposals).  That is NOT a
 *     must-link: `em035_prog_tbl` (0x805ED838) also lists entry points from two separate registered
 *     TUs (`enemy/fn_8035E034.cpp` and `enemy/em035_prog.cpp`), so an enemy program spanning
 *     several TUs is the module's normal shape.
 *   * The range holds two visibly different code groups.  0x8036CF64..0x8036E26C is the head: ten
 *     bodies that draw through the 2D library (`get_lsp_data`/`get_menu_lsp_tbl`/`get_str_tbl`/
 *     `draw_sprite_*`/`draw_font*`), read the quest text ids (`lb_quest_name_get`/`lb_quest_msg_get`)
 *     and own no `.sdata2` pool entry and no `.data` item at all; five of them are called from
 *     `fn_8036CC44` in the proposal below and three from the 0x802A2DB8/0x802A37F4 menu band, and
 *     their `this` record is NOT `_ENEMY_WORK` (`fn_8036DCD0`/`fn_8036DD34` touch +0x04/+0x08/+0x14
 *     as bytes and a halfword pair, where `_ENEMY_WORK` has single bytes and an `f32` at +0x1AC/+0x1B0).
 *     0x8036E2BC.. is the program proper: it owns all 86 pool labels and the whole `.data` run, and
 *     `em020_prog_tbl`'s first entry is exactly 0x8036E2BC.  A re-cut at 0x8036E2BC is requested in
 *     this batch's `config_requests`; until it is measured the whole brief range is registered here
 *     as one unit, and only the program half is written (see RESIDUALS).
 *
 * DATA (measured, not claimed).  This unit's own `.data` is 0x805EE108..0x805EE428 (0x320 B): sixteen
 * items that tile exactly and are referenced only from this range (`lbl_805EE108` 0xD8 by 0x80373010,
 * `jumptable_805EE1E0` 0x70 by 0x8036F8E4, `jumptable_805EE250` 0x20 by 0x80370368,
 * `jumptable_805EE270` 0x70 by 0x803714B0, `lbl_805EE2E0`..`lbl_805EE3A8` by 0x803714B0,
 * `lbl_805EE3A8`/`_805EE3D0` by 0x80372DAC and `lbl_805EE3F8` by 0x8036E534).  Neither section is
 * claimed, for two measured reasons:
 *   * `.data` - the run is an island in the unclaimed 0x805EDAE0.. chunk whose neighbours are other
 *     units' items (`em020_prog_tbl` at 0x805EE098 before it, `lbl_805EE428` from 0x803759C4 and
 *     `jumptable_805EE490`/`_805EE4B8` after it), and playbook 53's refinement forbids claiming
 *     several runs of one section while the bytes between them belong to another unit.
 *   * `.sdata2` 0x8079B820..0x8079BC64 - 86 entries, every one solely referenced here, so it is the
 *     private case playbook 58 allows; but the claim needs the object to emit the run and this unit
 *     is far from byte-identical, so it would promise a section our object does not produce
 *     (playbook 23's `ELF_gen.c` 2802 failure).  The bodies name the map's own pool symbols
 *     (`extern`, playbook 29 - never defined), so the rows pair by name meanwhile.
 *
 * LANGUAGE AND FLAGS.  C++: the range reaches genuinely mangled callees
 * (`em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`, ...) through their
 * real signatures (rule 9), and every plain `fn_XXXXXXXX` definition is `extern "C"` so it keeps the
 * map's name (playbook 42).  Lib `enemy` (`cflags_main`); the target objects carry 47 extab records
 * and 47 extabindex entries, and `cflags_main` is already the lib's exceptions setting.
 *
 * Naming note: references only to other units' unrenamed `fn_XXXXXXXX` symbols (checked with a
 * relocation sweep over `build/RMHE08/obj/`: every `fn_` this file calls - the 0x8012xxxx/0x8013xxxx
 * enemy band, the 0x802Axxxx menu band and the 0x8036xxxx siblings - is defined outside this range).
 * The symbols this file defines are named.
 *
 * STATUS AND RESIDUALS (official report metric; `datagap.py --unit main/enemy/em020_prog
 * --all-sections` for the data gap).  12 of the 57 functions are written and every one of them is at
 * or above the 80 % bar - nine byte-identical:
 *   em020_nop 100.0, em020_angle_step_to_zero 100.0, em020_timers_tick 100.0,
 *   em020_arm_mot1_wait20 100.0, em020_substate_dispatch 100.0, em020_arm_mot3_wait50 100.0,
 *   em020_arm_mot1_wait58 100.0, em020_arm_mot1_facing_wait77 100.0,
 *   em020_arm_mot1_effects_wait78 100.0;
 *   em020_arm_mot1_turn_wait79 96.72, em020_arm_mot1_side_wait80 92.46,
 *   em020_arm_approach_mot1_wait55 92.41.
 * Unit: 5.67 % fuzzy / 1136 of 33056 code bytes (matched_code counts the 100 % rows only).
 *   * The three near-misses are instruction-identical and differ only in *argument materialisation
 *     order* (first divergence recorded, not a percentage): turn_wait79 and approach_wait55 both want
 *     `lfs`/`lis` issued before the integer argument where this compiler issues the integer `li`/`lis`
 *     first; side_wait80 wants `cntlzw`+`extrwi`+`neg`+`addi 0x51` for `0x51 - (right == 0)` where we
 *     emit `subfic r0, r0, 0x51`.  All three were probed with the local hoisted, with the ternary
 *     polarity swapped and with the pool labels `const`/non-`const`; none of the six shapes moved the
 *     divergence, so the residual is this compiler's scheduling, not the source.
 *   * 45 functions are still unwritten, so they pair at 0 %: the two giants `fn_803733BC` (0x1CC8 B)
 *     and `fn_803716CC` (0xD24 B), the head group's ten 2D-library bodies (0x8036CF64..0x8036E26C -
 *     they need the head record's own class, which is not `_ENEMY_WORK` and which belongs to the other
 *     side of the 0x8036E2BC seam), and the remaining em020 handlers.  Their addresses and sizes are
 *     in `symbols.txt`; this header is not a body inventory.
 *   * `.data` and `.sdata2` are unclaimed for the reasons above, so every row that loads a pool
 *     constant is judged on the constant's name (`extern f32 lbl_8079Bxxxx`), never on a value this
 *     file defines.  Our object emits no `.data`/`.sdata`/`.sdata2`/`.rodata` at all (the `--all-sections`
 *     row lists only `.shstrtab` on the `ours-extra` side), so no `ours-extra` data defect exists;
 *     `datagap.py --flip-blockers` does not list this unit.
 */
/* ---- header inherited from src/enemy/em020_handlers.cpp (written against its pre-phase-4 range) ---- */
/* enemy/em020_handlers.cpp - three handler entries of the em020 enemy program, `.text`
 * 0x80375084..0x80375424 (928 B): `em020_model_refresh` 0x20C (0x80375084), `em020_condition_ck`
 * 0x110 (0x80375290) and `em020_area_model_set` 0x84 (0x803753A0).  Registered once, at its final
 * home (docs/plan.md 12), from proposal/80375084_fn_80375084.cpp.
 *
 * WHAT IT IS.  All three take the shared `_ENEMY_WORK` record in r3 and each is one entry of the
 * `.data` program table `em020_prog_tbl` (0x805EE098, 0x70 B, the map's own global name): the table
 * lists `em020_model_refresh` at its +0x20, `em020_condition_ck` at +0x24 and `em020_area_model_set`
 * at +0x34, next to the em020 band's other entry points (0x8036E2BC, 0x8036E320, 0x8036E6B8,
 * 0x80372D58, 0x8036E570, 0x8036E574, 0x803733BC and, below this range, 0x80375424 at +0x3C and
 * 0x80375494 at +0x58).  `em020_model_refresh` refreshes the model's two material effect matrices and
 * pushes the record's K-colours into the `MHchar` material; `em020_condition_ck` answers the program's
 * per-mode condition query; `em020_area_model_set` switches the stage/model state when the area
 * changes.  Module `enemy` (brief section 2, class 3): the record is `_ENEMY_WORK` (every callee is
 * the band's `em_*` API and `include/enemy/ENEMY_WORK.h` is the record's home), the bracketing
 * registered units are `enemy/*`, and the program table is an enemy-program table like
 * `em035_prog_tbl`, whose handlers the registered `enemy/em035_prog.cpp` reconstructs.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string reaches the range: its
 * only data references are the `.sdata2` float pool below and `em020_prog_tbl`, which no `s_`-prefixed
 * dump symbol covers (the .data scan of 0x80500000..0x80600000 finds no file-name literal between
 * menu_note.cpp at 0x805E91F8 and the end of the section).  2. `dumpmap.py lookup` answers only
 * `zz_0375084_`/`zz_0375290_`/`zz_03753a0_`.  3. `em020_prog_tbl` names the subsystem, so the file is
 * `em020_handlers` - the em020 program's handlers - and each symbol is derived from its own body:
 *   * `em020_model_refresh` - refreshes the model's effect matrices and K-colours (GUESS, from the
 *     body: `GetEffectMtx`/`SetEffectMtx` on the slots 8/9 material access and
 *     `MHchar::setTevKColor` on the 6/3 and 0..3 slots);
 *   * `em020_condition_ck` - the per-mode condition query (GUESS; the module's own `em*_ck` scheme,
 *     `em030_condition_ck`'s sibling precedent: the argument selects one of four conditions and the
 *     result is a 0..4 level);
 *   * `em020_area_model_set` - sets the stage/model state for the current area (GUESS, from the body:
 *     the `fn_802B0A98`/`fn_802D94C4` pair under an area switch).
 *
 * SEAM - UNPROVEN, AND A RE-DRAW CANDIDATE.  This is the run `attribute.py` left between the
 * neighbouring proposals; the extab/extabindex runs around it are contiguous and carry no seam
 * (`em020_prog_tbl` lists entry points on both sides of the range: 0x803733BC at +0x18 above,
 * 0x80375424 at +0x3C below).  If `em020_prog_tbl` is one translation unit's table, the real TU
 * spans 0x8036E2BC..0x80375494 and this range is its middle third - a seam re-draw for the round that
 * registers those neighbouring proposals, not something this registration can settle.
 *
 * FLAGS.  The lib's `cflags_main` plus a file-wide `#pragma peephole off` (the sibling
 * `enemy/em035_prog.cpp` precedent in this band): the target keeps the *unfused* form of three folds
 * the pass makes - `clrlwi r0,r4,24` + `cmpwi r0,0` for the mode switch (the pass emits the
 * record-form `clrlwi.` and drops the compare), `slwi r0,r0,7` + `clrlwi r3,r0,16` for the shake
 * angle (`clrlslwi`), and `srwi r0,r0,5` + `clrlwi r3,r0,24` for mode 1's bool (`extrwi`).  Measured
 * with the real command line, one function at a time: pass on 97.57327 (98.75572 / 94.117645 /
 * 100.0), pass off 100.0 on all three.  The range's three framed functions also carry the three
 * 8-byte extab records `-Cpp_exceptions on` emits (0x80017A5C `08 0A`, 0x80017A64 `00 0A`, 0x80017A6C
 * `08 08` - the frame descriptions of the 0x20C/0x110/0x84 bodies, r31-only / LR-only / r31-only).
 * No `.data`/`.sdata` run is claimed: the target objects for this range own `.text`, `extab` and
 * `extabindex` only, and `em020_prog_tbl` is a separate data unit's.
 *
 * RESULT (2026-09-27).  All three functions and both data sections are byte-identical to the target:
 * `.text` 0x3A0, extab 0x18 and extabindex 0x24 at 100 %, unit 100.0 fuzzy / 100 % matched code and
 * data, and `datagap.py --unit` reports no gap row.  Two shared-file fixes rode this unit:
 *   * `include/enemy/fn_8012BDF4.h` - the `s32 em_act_ck(...)` declaration sat inside the
 *     `extern "C"` block, so a caller including that header emitted the unmangled `em_act_ck` where
 *     the map (and the retail objects) reference `em_act_ck__FP11_ENEMY_WORKUcUc`; the first
 *     declaration of a name fixes its language linkage.  Removed, so the C++-scope declaration at the
 *     bottom of that header is the first (three other units' objects had the same wrong reloc name -
 *     no score moved, but their objects are now linkable);
 *   * `include/Pl/fn_8027D684.h` - `fn_8027DC64` declared `u32` here, not the owner's `s32`: the
 *     target's caller compares it unsigned (`cmplwi r3,0x1`), which is what mode 2 needs.
 *
 * Naming note: references only to other units' unrenamed `fn_XXXXXXXX` symbols (`MTX34_ctor`,
 * `fn_8005024C`, `fn_800E2994`, `fn_8006F304`, `fn_8013A9F4`, `stage_map_kind_get`, `fn_802B0A98`,
 * `fn_802D94C4`, `fn_8027DC64`), each declared by its
 * owner's header below; checked with `grep -n "fn_" include/fn_8004CAD8.h include/unsplit/{g3d,sound,unknown}.h
 * include/enemy/fn_80138074.h include/stage/stg_w.h include/ai/fn_802D44F4.h include/Pl/fn_8027D684.h`.
 */
/* ---- header inherited from src/enemy/em020_ai.cpp (written against its pre-phase-4 range) ---- */
/*
 * enemy/em020_ai.cpp - the tail of the em020 monster-AI file, `.text` 0x80375424..0x80378F9C
 * (78 functions / 0x3B78 B) with extab 0x80017A74..0x80017C44 (58 records) and extabindex
 * 0x80037614..0x800378CC (58 x 12 B).
 *
 * WHAT IT IS.  Monster-AI code of the em020 program.  Every body takes the shared `_ENEMY_WORK`
 * record (`include/enemy/ENEMY_WORK.h`) and drives it through the enemy core API -
 * `em_frame_check` (124 calls), `em_parts_damage_level_get`, `em_magma_check`, `get_em_chg_scale`,
 * `get_joint_wpos_em`, `em_mot_set`/`em_mot_set_ck`/`em_mot_end_ck` - and through the game's work
 * blocks `system_w` (38 calls), `lobby_w` (45), `get_move_work_adrs`, `my_player_no`,
 * `work_mem_alloc`/`work_mem_free`.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range: every `lis`/`addi` pair and every `lbl_` reference in its 78 split objects resolves to the
 * `.data` program tables, the `.bss`/`.sbss` work blocks, the shared `.sdata2` float pool or a call -
 * never to a source-file-name literal (checked by reading every relocation target of the range's
 * objects out of the DOL: the only string in its own `.data` run is a Japanese network message at
 * 0x805EE428).  2. `dumpmap.py lookup` answers `zz_<addr>_` for every address in the range except the
 * two `em0XX_prog_tbl` rows below.  3. The module is `enemy` from the `.data` program table
 * `em020_prog_tbl` (0x805EE098, `scope:global`, size 0x70) whose entry-point list is this band's own
 * functions - `fn_8036E2BC`/`fn_8036E320`/`fn_8036E6B8`/`fn_8036E570`/`fn_8036E574`/`fn_8036E...`,
 * `fn_80375084` (+0x20), `fn_80375290` (+0x24), `fn_803753A0` (+0x34), `fn_80375424` (+0x3C),
 * `fn_80375494` (+0x58) - and from the code (`_ENEMY_WORK` field for field, `em_*` callees only).
 * The file is therefore named for the program the table names (`em020`), on the module's `em*`
 * scheme (`em024_ai.cpp`, `em035_prog.cpp`).
 *
 * SEAM (unproven - the range is an `attribute.py` `--max-bytes` run, not a TU boundary).
 * `tudiscover.py at 0x80375424` reports the 7-function match set 0x80375424..0x803757E0 with a strong
 * `.sdata2` seam at 0x80375290 (outside the brief's range, and explained by the mergeable-constant
 * pool: 0x8079BC68..0x8079BC88 is referenced from both sides, so it is a shared constant run, not an
 * object boundary), and a weak right boundary.  The evidence that does pin this file's right edge is
 * the `.data` block boundary plus the call closure: `fn_80378464`'s two jump tables
 * (0x805EE4B8..0x805EE514) are the last `.data` of the em020 block, `em019_prog_tbl` starts the next
 * block at 0x805EE518, and `fn_80378F7C` (the last function here) is called only from 0x8036C284 /
 * 0x8036C6E8 - both em020-side - while `fn_80378F9C` (the first function of `enemy/em019_ai.cpp`) is
 * called only from 0x8037939C upward.  This file's left edge (0x80375424) is FALSE: `em020_prog_tbl`
 * references functions at 0x8036E2BC..0x80375290, so the original em020 file starts well before the
 * brief's range; the head is left to its own lane and filed as a `config_requests` `range` entry.
 *
 * Naming note: the names this file *references* in other units are still the map's generated
 * `fn_XXXXXXXX` stems (the enemy core band 0x8012xxxx/0x8013xxxx and the game-root 0x8042xxxx band,
 * checked with `tools/symbols/symedit.py range`); every symbol this file DEFINES is named from its
 * own body and renamed in the map with `symedit.py rename`.
 *
 * Sections this unit claims: `.text` 0x80375424..0x80378F9C, extab 0x80017A74..0x80017C44,
 * extabindex 0x80037614..0x800378CC, and the `.data` run its own jump tables occupy
 * (0x805EE490..0x805EE518).
 *
 * Residuals: the range is registered as `NonMatching`; the bodies still unwritten keep the map's
 * `fn_XXXXXXXX` names, and the ones written but not yet byte-identical are listed in the outbox with
 * their first divergence.  Re-measure with `ninja build/RMHE08/report.json` +
 * `python tools/objdiff/symdiff.py -u enemy/em020_ai.cpp <symbol>`.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "nw4r/math.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"
#include "stage/stg_w.h"

/* The `.sdata2` pool half this range reads (0x8079B820..0x8079BC60, 86 single-referrer entries that
 * only this unit's functions load).  Declared, never defined - playbook 29/58: a definition would
 * make MWCC emit the named constant *and* a pool copy, so the section would grow instead of
 * pairing.  The section itself is left to its auto data unit (see the header's DATA block). */

extern "C" {
extern f32 lbl_8079B83C; /* 0.0 */
extern f32 lbl_8079B854; /* the approach scale `fn_80136D4C` takes */
extern f32 lbl_8079B858;
extern f32 lbl_8079B85C;
extern f32 lbl_8079B874;
extern f32 lbl_8079B87C;
extern f32 lbl_8079B8A4;
extern f32 lbl_8079B8C4;
extern f32 lbl_8079B8EC;
extern f32 lbl_8079B8F0;
}

#ifdef __cplusplus

extern "C" {
#endif
}
#include "gx.h"
#include "enemy/fn_8012BDF4.h" /* em_act_ck (the C++ declaration, which mangles to the map's name) */
#include "enemy/fn_80138074.h" /* fn_8013A9F4, the owner's own declaration */
#include "sound/mhchar.h"
#include "nw4r/g3d/scnmdl.h"     /* ScnMdl::CopiedMatAccess */
#include "nw4r/g3d/g3d_resmat.h" /* ResTexSrt */
#include "fn_8004CAD8.h"         /* MTX34_ctor, fn_8005024C (their owner's header) */
#include "unsplit/g3d.h"         /* fn_8006F304 */
#include "unsplit/sound.h"       /* fn_800E2994 */
#include "unsplit/unknown.h"     /* SystemWork / system_w, stage_map_kind_get */
#include "enemy/em020_ai.h"      /* em020_aim_target_ck (its owner is enemy/em020_ai.cpp) */
#include "stage/stg_w.h"         /* fn_802B0A98 (the owner is stage/stg_w.cpp) */
#include "ai/fn_802D44F4.h"      /* fn_802D94C4 (the owner is ai/fn_802D44F4.cpp) */
#include "Pl/fn_8027D684.h"      /* fn_8027DC64 (the owner is Pl/fn_8027D684.cpp) */

/* The `.sdata2` floats this range reads (bare marker symbols in the map; the values are below).
 * Declared, never defined here: the pool belongs to the data pass (playbook 29/58). */
extern "C" const f32 lbl_8079B8CC; /* 0.5f   - the shake amplitude */
extern "C" const f32 lbl_8079BC64; /* 0.0025f - the per-frame step of the wrap test */
extern "C" const f32 lbl_8079B848; /* 1.0f   - the wrap bound */
extern "C" const f32 lbl_8079B870; /* 400.0f */
extern "C" const f32 lbl_8079B8D0; /* -1400.0f */
extern "C" const f32 lbl_8079BC68; /* -400.0f */
#include "enemy/em020_ai.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_80138074.h"
#include "Network/network_pat_control.h"
#include "sys_mem.h"


/* The shared `.bss` lobby state block `lbl_806BF530` (0x806BF530, 0x2EB8 B): this unit reads its
 * `+0x03` quest-active byte, and the lobby's `lb_npc.cpp` reads the same byte.  Its own home is the
 * unclaimed `.bss` blob, so the declaration stays here until a unit owns the block (rule 2's unsplit
 * case, like `lbl_806BE340`). */
extern u8 lbl_806BF530[];
/* The shared `.bss` quest-page block (0x806BE340, ten 0x130-byte records) whose +0x03 byte
 * `em020_quest_page_ptr` returns the address of; declared by its owner's leaf header. */
#include "lobby/lbl_806BE340.h"
/* The `.sbss` one-byte flag `em020_unknown_flag_set` writes. */
extern u8 lbl_80794BF4;
/* The pooled `.sdata2` constants this unit loads through `r2`.  Declared, never defined (playbook
 * 29/58): a definition would make MWCC emit a second copy and grow `.sdata2` instead of pairing. */
extern f32 lbl_8079BC6C; /* 0.65f */

#pragma peephole off

extern "C" {

/* Steps the record's +0x1BC rotation angle half a word toward zero and hands the record on.  The
 * angle is a `s16` in the low halfword of a 32-bit field: values inside +/-0x40 collapse to 0 and
 * everything else moves by 0x40 in the direction of the sign. */
void em020_angle_step_to_zero(_ENEMY_WORK* self)
{
    u32 ang = self->field_0x1BC;
    u16 half = (u16)ang;

    if (half < 0x8000) {
        if (half < 0x40)
            self->field_0x1BC = 0;
        else
            self->field_0x1BC = ang - 0x40;
    } else if (half > 0xFFC0) {
        self->field_0x1BC = 0;
    } else {
        self->field_0x1BC = ang + 0x40;
    }

    fn_80133C3C(self);
}

/* Ticks the two em020 timers: the +0x334 countdown always, and the +0x33E one only while the map
 * lookup reports the record's `area_no` 2 and the interpreter's byte stack is at 6.  The +0x338
 * byte is a per-frame latch of "the map is 7 and the area is 3". */
void em020_timers_tick(_ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3)
        self->em020_0x328.latch_0x338 = 1;
    else
        self->em020_0x328.latch_0x338 = 0;

    if (self->em020_0x328.timer_0x334 > 0)
        self->em020_0x328.timer_0x334 -= 1;

    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 2 && self->stack_0x961[0] == 6) {
        if (self->em020_0x328.timer_0x33E > 0)
            self->em020_0x328.timer_0x33E -= 1;
    }
}

/* The record's two-step wake-up: it hands the record to the shared per-frame tick, steps the
 * +0x1BC angle, and then either arms the 0x14-frame pose (step 0) or waits for the motion to report
 * done and releases the record into the next action (step 1). */
void em020_arm_mot1_wait20(_ENEMY_WORK* self)
{
    fn_80133C3C(self);
    em020_angle_step_to_zero(self);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The per-frame byte the +0x1E6 sub-state makes `em020_arm_mot1_wait20` run for each of its five steps, with
 * every other sub-state falling straight through. */
void em020_substate_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em020_arm_mot1_wait20(self);
        break;
    case 1:
        em020_arm_mot1_wait20(self);
        break;
    case 2:
        em020_arm_mot1_wait20(self);
        break;
    case 4:
        em020_arm_mot1_wait20(self);
        break;
    case 5:
        em020_arm_mot1_wait20(self);
        break;
    default:
        break;
    }
}

/* The same two-step wake-up for the second half of the pose: step 0 arms a 0x32/0x28-frame motion
 * (kind 3), step 1 waits for it and releases the record. */
void em020_arm_mot3_wait50(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x28, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The three-step wake-up of the pose whose closing approach depends on the argument: step 0 arms a
 * 0x37/0x14-frame motion and re-aims the model (`em_approach_start`), storing one of the two approach
 * floats at +0x378 for mode 1 and mode 2; step 1 waits for the 0x80-frame window and the motion
 * helper to agree, then arms the closing 0x2f/0x28 pose; step 2 waits and releases the record. */
void em020_arm_approach_mot1_wait55(_ENEMY_WORK* self, u8 mode)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);

        switch (mode) {
        default:
            em_approach_start(self, lbl_8079B83C, 0x12);
            break;
        case 1:
            em_approach_start(self, lbl_8079B83C, 0x12);
            self->value_0x378 = lbl_8079B858;
            break;
        case 2:
            em_approach_start(self, lbl_8079B83C, 0x12);
            self->value_0x378 = lbl_8079B85C;
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            self->state += 1;
            em_mot_set_blend(self, 0x2f, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x3a/0x0a-frame pose's two-step wake-up. */
void em020_arm_mot1_wait58(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3a, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x4d/0x14-frame pose's wake-up: step 1 keeps the record facing the target
 * (`em_frame_check(self, 1, 0.0.., 0.0)` re-aims it with `em_turn_to_target`) and then waits for the
 * motion to report done. */
void em020_arm_mot1_facing_wait77(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4d, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0xa);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079B874, lbl_8079B83C) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The 0x4f/0x0a-frame pose's wake-up: step 1 re-aims the record with `fn_80136D4C` and closes the
 * pose with `em_turn_to_target` as soon as either of the two frame windows reports done, then re-arms the
 * 0x10000-step turn. */
void em020_arm_mot1_turn_wait79(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4f, 0xa, 0, 1);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1: {
        if (em_frame_check(self, 2, lbl_8079B8EC, lbl_8079B83C) == 1 ||
            em_frame_check(self, 3, lbl_8079B8F0, lbl_8079B8C4) == 1) {
            fn_80136D4C(self, lbl_8079B854);
            em_turn_to_target(self, 0x50);
        }
        f32 lo = lbl_8079B8EC;
        f32 hi = lbl_8079B8F0;
        em_turn_in_window(self, lo, hi, 0x10000);
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    }
    default:
        break;
    }
}

/* The 0x4e/0x0a-frame pose's wake-up: step 0 arms the motion and the two 8/0x10-part effect slots,
 * step 1 re-aims and closes the pose once the frame window reports done. */
void em020_arm_mot1_effects_wait78(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4e, 0xa, 0);
        em_hit_window_set(self, 0, 0xb, 8);
        em_hit_window_set(self, 1, 0xc, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079B8A4, lbl_8079B83C) == 0) {
            fn_80136D4C(self, lbl_8079B854);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    default:
        break;
    }
}

/* The two-directional 0x50/0x51-frame pose's wake-up: the argument picks the motion (0x51 when set,
 * 0x50 otherwise) and the sign of the 0x4000-step turn `em_turn_in_window` re-arms every frame. */
void em020_arm_mot1_side_wait80(_ENEMY_WORK* self, u8 right)
{
    Vec3 keys;

    VEC3_ctor(&keys);

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, (u16)(0x51 - (right == 0)), 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 0x11);
        break;
    case 1: {
        f32 lo = lbl_8079B87C;
        f32 hi = lbl_8079B8C4;
        em_turn_in_window(self, lo, hi, right == 0 ? 0x4000 : -0x4000);
        if (em_mot_end_ck(self) == 1)
            fn_80128030(self);
        break;
    }
    default:
        break;
    }
}

/* One handler slot of `em020_prog_tbl` that does nothing - the table's index 4. */
void em020_nop(_ENEMY_WORK* self)
{
    (void)self;
}

#ifdef __cplusplus
}
#endif


/* Pushes the record's model state into the scene model: refreshes the effect matrix of the two
 * material slots (8 and 9) the record drives, then rewrites the material's K-colours - the 6/3 slot
 * from the record's own triple and the 0..3 slots from the area/action test. */
extern "C" void em020_model_refresh(_ENEMY_WORK* self) {
    nw4r::math::MTX34 mtx;
    _GXColor color;

    MTX34_ctor(&mtx);
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access_a((nw4r::g3d::ScnMdl*) self->field_0x13C, 8);
        nw4r::g3d::ScnMdl::CopiedMatAccess access_b((nw4r::g3d::ScnMdl*) self->field_0x13C, 9);

        if (fn_800E2994(&access_a) != 0 && fn_800E2994(&access_b) != 0) {
            nw4r::g3d::ResTexSrt srt_a;
            nw4r::g3d::ResTexSrt srt_b;

            fn_8006F304(&srt_a, access_a.GetResTexSrt(false));
            fn_8006F304(&srt_b, access_b.GetResTexSrt(false));
            srt_a.GetEffectMtx(1, &mtx);
            mtx.m[0][3] = lbl_8079B8CC * fn_8005024C((u16) (system_w.field_0x0c << 7));
            mtx.m[1][3] += lbl_8079BC64;
            if (mtx.m[1][3] > lbl_8079B848) {
                mtx.m[1][3] -= lbl_8079B848;
            }
            srt_a.SetEffectMtx(1, &mtx);
            srt_b.SetEffectMtx(1, &mtx);
        }
    }

    ((MHchar*) &self->char_0x024)->getTevKColor(6, GX_KCOLOR3, &color);
    color.r = self->em020_kcolor_0x328.kcolor_r_0x339;
    color.g = self->em020_kcolor_0x328.kcolor_g_0x33A;
    color.b = self->em020_kcolor_0x328.kcolor_b_0x33B;
    ((MHchar*) &self->char_0x024)->setTevKColor(6, GX_KCOLOR3, &color);

    ((MHchar*) &self->char_0x024)->getTevKColor(0, GX_KCOLOR0, &color);
    if (self->area_no == 0 || em_act_ck(self, 13, 2) != 0) {
        color.r = 150;
        color.g = 150;
        color.b = 150;
    } else {
        color.r = 255;
        color.g = 255;
        color.b = 255;
    }
    ((MHchar*) &self->char_0x024)->setTevKColor(0, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(1, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(2, GX_KCOLOR0, &color);
    ((MHchar*) &self->char_0x024)->setTevKColor(3, GX_KCOLOR0, &color);
}

/* The program's per-mode condition query: mode 0 is the target/self height difference as a 0..4
 * level, mode 1 the aim-target flag, mode 2 the slot-free test and mode 3 a timer's sign. */
extern "C" u8 em020_condition_ck(_ENEMY_WORK* self, u32 mode) {
    f32 diff;

    switch ((u8) mode) {
    case 0:
        diff = self->vec_0x36C.y - self->pos.y;
        if (diff >= lbl_8079B858) {
            return 2;
        }
        if (diff >= lbl_8079B870) {
            return 1;
        }
        if (diff <= lbl_8079B8D0) {
            return 4;
        }
        if (diff <= lbl_8079BC68) {
            return 3;
        }
        return 0;
    case 1:
        return em020_aim_target_ck(self) == 1;
    case 2:
        if (self->em020_0x328.timer_0x334 <= 0 && fn_8027DC64() == 1) {
            return 1;
        }
        return 0;
    case 3:
        return self->em020_0x328.timer_0x33E > 0;
    default:
        return 0;
    }
}

/* Switches the stage/model state when the program's map is the one that owns the two area models:
 * the record's interpreter stack is reset first, then the area selects the stage table entry and the
 * model's area mode. */
extern "C" void em020_area_model_set(_ENEMY_WORK* self) {
    fn_8013A9F4(self);

    if (stage_map_kind_get(self->field_0x1E0) == 7) {
        switch (self->area_no) {
        case 2:
            fn_802B0A98(9, 1);
            fn_802D94C4(0);
            break;
        case 3:
            fn_802B0A98(10, 1);
            fn_802D94C4(1);
            break;
        }
    }
}


extern "C" {

/* The em020 area hit's damage-level gate: every part's damage level is folded into `out->levels_0x01`
 * and the enemy's current facing angle and damage numerator are copied out.
 * 0x80375540 */
void em020_hit_info_get(struct _ENEMY_WORK* self, struct Em020HitInfo* out)
{
    if (self->area_no == 3) {
        out->hit_0x00 = 1;
        out->levels_0x01 = 0;
        if ((self->flags_0x836 & 2) != 0) {
            out->levels_0x01 |= 1;
        }
        if ((self->flags_0x836 & 0x8000) != 0) {
            out->levels_0x01 |= 2;
        }
        if (em_parts_damage_level_get(self, 3) >= 2) {
            out->levels_0x01 |= 4;
        }
        if (em_parts_damage_level_get(self, 5) >= 1) {
            out->levels_0x01 |= 8;
        }
        out->angle_0x02 = self->parts_0x838[0].value_0x04;
        out->damage_0x04 = self->field_0x7A0;
    } else {
        out->hit_0x00 = 0;
    }
}

/* The em020 "res user data" apply step: hands the area's third resource record to the shared
 * `fn_8013A654` installer.
 * 0x803754EC */
void em020_res_user_data_apply(struct _ENEMY_WORK* self)
{
    fn_8013A654(self, 3);
}

/* The em020 aim-target predicate: whether the `+0x836` bit 15 "aim target found" flag is set.
 * 0x803754F4 */
u32 em020_aim_target_ck(struct _ENEMY_WORK* self)
{
    return (self->flags_0x836 & 0x8000) != 0;
}

/* The em020 low-HP predicate: the damage numerator's share of the denominator is at or below 0.65
 * and the work sits in area 3.  This one body keeps the peephole pass (its `xoris` is part of the
 * unsigned int-to-float conversion the target keeps).
 * 0x80375424 */
u32 em020_hp_ratio_ck(struct _ENEMY_WORK* self)
{
    f32 ratio;

    if (self->area_no != 3) {
        return 0;
    }
    ratio = (f32)self->field_0x7A0 / (f32)self->field_0x7A4;
    if (ratio <= lbl_8079BC6C) {
        return 1;
    }
    return 0;
}

/* The em020 map-7 area-3 action request: when the work stands on map 7 in area 3, asks the shared
 * action setter for action 13 sub-state 3.
 * 0x80375494 */
void em020_map_area_action_set(struct _ENEMY_WORK* self)
{
    if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) {
        fn_80128AEC(self, 13, 3);
    }
}

/* The em020 area-2 action-20 sub-state-30 predicate.
 * 0x8037550C */
u32 em020_area2_action20_ck(struct _ENEMY_WORK* self)
{
    if (self->team == 20 && self->area_no == 2 && self->stack_0x961[0] == 30) {
        return 1;
    }
    return 0;
}

/* Releases the work's sub-record through the shared teardown, then frees the block when the caller's
 * size argument is positive.
 * 0x803757E0 */
void* em020_work_free(struct _ENEMY_WORK* self, s16 size)
{
    if (self != NULL) {
        fn_8013918C(self, 0);
        if (size > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The empty stub the program table reserves for em020.
 * 0x80376964 */
void em020_noop(void)
{
}

/* The em020 quest-page pointer: the address of the shared quest-page block's fourth record.
 * 0x80376968 */
u8* em020_quest_page_ptr(void)
{
    return lbl_806BE340 + 3;
}

/* The em020 "false" program-table stub.
 * 0x803788A0 */
u32 em020_false_ck(void)
{
    return 0;
}

/* Writes the em020 one-byte `.sbss` flag.
 * 0x803759BC */
void em020_unknown_flag_set(u8 value)
{
    lbl_80794BF4 = value;
}

/* The em020 quest-active predicate: whether the shared lobby block's `+0x03` byte is 1.
 * 0x8037583C */
u32 em020_quest_active_ck(void)
{
    return lbl_806BF530[3] == 1;
}

/* Clears the shared lobby block's `+0x03` quest-active byte.
 * 0x80375858 */
void em020_quest_active_clear(void)
{
    lbl_806BF530[3] = 0;
}

/* Clears the nine quest-page records after the first in the shared quest-page block.
 * 0x80376978 */
void em020_quest_pages_clear(void)
{
    s32 i;

    for (i = 1; i < 10; i++) {
        memset(lbl_806BE340 + i * 304, 0, 304);
    }
}

/* The address of the em020 twelve-entry 31-byte row array at the shared lobby block's `+0x13E6`.
 * 0x80378F7C */
u8* em020_row_ptr(u8 index)
{
    u8* rows = lbl_806BF530 + 0x13E6;
    return rows + index * 31;
}

#ifdef __cplusplus
}
#endif

