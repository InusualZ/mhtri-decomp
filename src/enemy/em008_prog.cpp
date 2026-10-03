/* enemy/em008_prog.cpp - enemy 008 program
 *
 * `.text` 0x8015D860..0x801663E4, 62 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): fold of 3 registered units, built from `enemy/fn_8015D860.cpp`, `enemy/fn_8015E854.cpp`, `enemy/fn_80165FC8.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 *
 * Kept views: the retired sources declared 13 callee(s) with different signatures (`assignVec3`, `em_act_ck`, `em_fall_height_get`, `em_frame_flag_set`, `em_mot_set`, `em_parts_damage_level_get`, `em_mot_finished_ck`, `em_motion_param_set`, `fn_8013918C`, `fn_8013A654`, `fn_8015D934`, `rotMatrixX`, ...); each function keeps its own source's view through a function-pointer cast macro (`<name>_viewN`, `<name>_cN`), which compiles to the same direct call, so the fold does not move any body.
 * Hidden declarations: 12 header declaration(s) that disagree with the kept view are renamed away around their `#include` (`#define <name> <name>_hidden_<header>`): `assignVec3`, `em_act_ck`, `em_fall_height_get`, `em_frame_flag_set`, `em_mot_set`, `em_mot_finished_ck`, `em_motion_param_set`, `fn_8013918C`, `fn_8013A654`, `fn_8015D934`, `rotMatrixX`, `rotMatrixZ`.
 */

/* Retired header of `enemy/fn_8015D860.cpp` (kept for its notes and residuals): */
/* enemy/fn_8015D860.cpp - the em008 enemy's per-action state-step band,
 * `.text` 0x8015D860..0x8015E854 (28 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup 0x8015D860`: the shared runtime dump answers only
 * `zz_015d860_` - no `__FILE__`/class string names a source file - and
 * `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX` entries for the range;
 * every data reference the range makes is a vtable word, a jump table or a `.sdata2` float).
 *
 * What it is.  One `enemy`-module actor's per-action state steps, addressed through the shared
 * `_ENEMY_WORK` record: each step is the same three-part shape - a `state` (+0x05) switch, the
 * motion hand-off (`em_move_mode_set`/`em_mot_set`/`em_mot_set_ck`/`fn_80128AAC`), then the wait
 * (`em_mot_end_ck`) and the finish (`em_action_finish`/`fn_801280F4`).  `fn_8015E05C` and `fn_8015E804`
 * are the two dispatchers over `state_sub` (+0x1E6): the first is the compare chain for the
 * sparse step set {0,1,2,3,4,6,7}, the second the jump table for the contiguous 0..9 set (the
 * `.data` table at `jumptable_805A5DC0`, which the split leaves in the auto band - this unit does
 * not claim it, as the two bracketing registered units do not claim their own tables either).
 * `fn_8015D860` seeds the `field_0x1E4` flag byte from `em_parts_damage_level_get(self, 2)` and
 * two `em_flags836_ck` mask probes; `fn_8015D8F0`/`fn_8015D908`/`fn_8015D934` are its predicates;
 * `fn_8015D9B8`/`fn_8015DD6C` own the two +0x328/+0x32A countdowns; `fn_8015D9C8` is the per-tick
 * entry that attaches the 0xC-byte vtable helper `fn_8015DAA8` constructs; `fn_8015DAE4` and
 * `fn_8015DB68` are the `(state, sub-state)` transition tables.
 *
 * The seam is unproven (docs/plan.md 8.3).  The bracketing registered units are
 * `enemy/fn_801550FC.cpp` (below, `.text` ends exactly at 0x8015D860) and `enemy/fn_8015E854.cpp`
 * (above, starts exactly at 0x8015E854), and both headers record their own boundary as
 * provisional: `enemy/fn_8015E854.cpp`'s dispatchers call this range's `fn_8015E05C`/`fn_8015E804`
 * and `fn_80162500` calls `fn_8015D860`, so the real translation unit plausibly spans all three.
 * The registration follows the proposal's own range.
 *
 * Language: C++ (`medium` in the brief, re-derived here from the range's own evidence: the callees
 * with an argument list are C++ manglings - `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`,
 * `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3` - and the 0xC-byte helper is allocated
 * with `operator new` (`__nw__FUl`), which is C++ only).  The flat `fn_*` symbols are `extern "C"`
 * so objdiff pairs them by name; the mangled callees are declared through their real signatures
 * (rule 9) and the front-end produces the map spelling.
 *
 * Registration.  Class 4 of the brief's evidence order: no `__FILE__` string in the region's data
 * (class 1), `dumpmap.py lookup` answers only `zz_015d860_` (class 2), and the siblings' naming
 * scheme is the map's own stem with the rule-7 deferral (`enemy/fn_801550FC.cpp`,
 * `enemy/fn_8015E854.cpp`), so the file keeps the `fn_8015D860` stem.  Module `enemy` from the
 * link band (both bracketing registered units are `enemy`) and from the code (`_ENEMY_WORK` and
 * the enemy-band helpers).  Sections claimed with the block: `.text`, the `extab`
 * (0x8000DFA4..0x8000E04C) and `extabindex` (0x80028E60..0x80028F5C) runs the range's 21 framed
 * functions carry - the auto units of the retired scaffolding bucket split them exactly there - and
 * the `.data` jump table `fn_8015E804` lowers to (0x805A5DC0..0x805A5DE8, the map's 0x28-byte
 * `jumptable_805A5DC0`): the object emits it and its ten relocations are the target's own, so the
 * range is claimed rather than left to the auto band (the two bracketing units have no jump-table
 * claim of their own; this one is measured byte-identical).
 *
 * Types.  `_ENEMY_WORK` is the shared record in `include/enemy/ENEMY_WORK.h` (the one home,
 * rule 1).  This unit's additions there: the byte at +0x491 (`fn_8015DB68` sets it) and the s16
 * view of +0x32A (`fn_8015D9B8`/`fn_8015DB68`/`fn_8015DD6C` count it down as a halfword, where
 * `enemy/fn_80170600.cpp` stores a `stb` over the same byte - a union member, not a re-typing).
 * The 0xC-byte helper `fn_8015DAA8` constructs is private to this unit (`Helper_8015DAA8`), the
 * same record `enemy/fn_80147CE0.cpp`/`enemy/fn_80176C58.cpp` carry privately until rule 1 folds
 * the three into one header.
 *
 * Flags.  `#pragma peephole off` is load-bearing for the whole unit: retail keeps the unfused
 * `clrlwi`/`rlwinm` + `cmpwi` pairs (playbook 39) that `-O3`'s peephole folds into `clrlwi.`.  Probed
 * by turning the pass back on and re-measuring every symbol (peephole on -> off): fn_8015D8F0
 * 80.83 -> 100.0, fn_8015D908 79.82 -> 98.91, fn_8015D934 80.91 -> 100.0, fn_8015DAA8 99.33 ->
 * 100.0, fn_8015DAE4 96.82 -> 100.0, fn_8015DB68 97.36 -> 100.0, fn_8015E1C4 98.87 -> 100.0,
 * fn_8015E338 97.75 -> 100.0, fn_8015E6E8 96.00 -> 100.0; the other nineteen symbols are identical
 * under both.  No lib flag is involved: the `enemy` lib's `cflags_main` (see the block's comment in
 * `configure.py`) measures every body below.
 *
 * Status (official report metric, per symbol, measured in the worktree with
 * `python tools/units/recompile.py enemy/fn_8015D860.cpp --measure <symbol>`): 27 of the 28
 * functions are exactly 100 %; `fn_8015D908` is 98.91 % (44 B, the target's own size).  The object's
 * `.text` is 0xFF4 - the target's size to the byte - and its `extab` (0xA8), `extabindex` (0xFC) and
 * `.data` (0x28, the `fn_8015E804` jump table: ten `fn_8015E804+0x24..+0x48` words, the same ten
 * relocations the split's table at 0x805A5DC0 carries) are byte-identical to the target object
 * (`objdump -s -j <section>` on the two objects diffs clean).
 *
 * Residual - `fn_8015D908` 98.91 %.  Same 11 instructions, same size, same branch polarities
 * (`beq`/`bne`); only the two return blocks' ORDER differs: the target lays out
 * `[test bit0][test bit1][li r3,1 / blr][li r3,0 / blr]` and this source's shape lays out
 * `[test bit0][test bit1][li r3,0 / blr][li r3,1 / blr]`, so the two branch displacements are 4 B
 * apart.  Shapes tried and measured: the nested `if (bit0) { if (bit1) return 0; } return 1;`
 * (98.91 %, this one), the same with an explicit `return 1` inside the outer arm (79.45 % - MWCC
 * if-converts the if/else-return pair to `cntlzw`/`srwi`), `if ((bit0) == 0) return 1;` first
 * (48.18 %), an explicit `else` on either arm (79.45 %), a `result` variable with `return result`
 * (74.45 %), the if/else-if chain (48.18 %) and the `cond`/`compound`/`stmt_order`/`ternary`/`switch`
 * shape search (`tools/flags/shapesearch.py`, depth 3, beam 4 - best semantics-preserving candidate
 * 98.91 %; its 99.36 % row is a condition-flipped variant, i.e. not this function).  The residual is
 * the compiler's block ordering, not comprehension: the predicate is "0 only when flag bits 0 and 1
 * are both set".
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_8015D860.cpp`.
 */

/* Retired header of `enemy/fn_8015E854.cpp` (kept for its notes and residuals): */
/* enemy/fn_8015E854.cpp - the enemy action/state unit that follows the em003 block,
 * 0x8015E854..0x80165FC8 (55 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup 0x8015E854`: the shared runtime dump answers only
 * `zz_015e854_`, and `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX`
 * entries for the range - no `__FILE__`/class string names a file here).
 *
 * What it is.  A continuation of the same enemy-band action family as `enemy/fn_801550FC.cpp`:
 * three jump-table dispatchers keyed on the byte at +0x1E6 (`state_sub`)
 * - fn_8015F23C, fn_801620A4 and fn_80163298 - fan out to the range's per-action steps, and the
 * steps are the usual frame/timer gate + `em_frame_check`/`em_mot_end_ck` shape over the shared
 * `_ENEMY_WORK` record.  The range drives the same enemy-band helpers
 * (`em_move_mode_set`, `em_mot_set`, `em_approach_start`, `em_approach_step`, `em_turn_seq_start`, `em_turn_seq_step`,
 * `get_em_chg_scale`, `em_frame_check`, `em_after_frame_check`, `setVector3`, ...).
 *
 * The seam is unproven (docs/plan.md 8.3).  The range's own `tudiscover` run has no strong cut:
 * the set it returns is a single function (fn_8015E854) and both edges are weak.  It is NOT a
 * continuation of `enemy/fn_801550FC.cpp` (that unit's `.text` ends at 0x8015D860, 0xFF4 bytes
 * below this range's first function, and its own header records em008 taking over after it) and
 * it is not contiguous with it, so this registers a new unit at the proposal's range.  The
 * dispatcher fn_80163298 calls fn_8015E05C/fn_8015E804 below the range and fn_80162500 calls
 * fn_8015D860 (both in the still-unclaimed band 0x8015D860..0x8015E854), so the real translation
 * unit plausibly extends below this range; the boundary is recorded as provisional and those
 * neighbours are the follow-up queue.
 *
 * Language: C++ (every callee the range reaches is a C++ mangling - `em_frame_check__FP11_ENEMY_WORKUsff`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `get_em_chg_scale__FP11_ENEMY_WORK` - and the range
 * calls them through their real signatures, rule 9).  The flat `fn_*` symbols are `extern "C"`
 * so objdiff pairs them by name.
 *
 * Object: `_ENEMY_WORK`, included from `include/enemy.h`.
 *
 * Flags: this unit needs no deviation - the `enemy` lib's `cflags_main` (see the block's comment
 * in `configure.py`) measured every body below.  No `#pragma` is used.
 *
 * State of the reconstruction.  The 29 bodies below cover the range's state steps, its three
 * jump-table dispatchers (fn_8015F23C, fn_801620A4, fn_8015FCB0) and the small actor
 * predicates/wrappers, in address order; 28 of the 29 measure >= 80 % (the official report
 * metric), the exception is fn_80165C64 (65.7 %).  The remaining 26 functions are the follow-up
 * queue, below.
 *
 * Residuals, per shape (the measurement lives in the outbox, not here; playbook rows in
 * `docs/matching.md`):
 *   * fn_80165C64 (65.7 %, 96 B vs target 108 B): the target tests
 *     `(em_parts_damage_level_get(self, arg) & 1)` with an explicit
 *     `clrlwi r0,r3,24 / clrlwi r0,r0,31 / cmpwi / bne` pair, while every spelling tried
 *     (`== 0 return 1`, `!= 0 return 0`, a `u32`/`u8` temp, an if/else-if on the byte-narrowed
 *     argument) makes MWCC fold it to `clrlwi. r0,r3,31 / xori r3,r0,1` and reorder the two
 *     condition blocks; same source shape, 12 bytes shorter.  Recorded, not a flag problem.
 *   * fn_8015F23C (99.97 %) is byte-identical in size; only the table's unused 15..21 entries
 *     differ by one slot.  Residual only.
 *   * fn_8015F510 (92.6 %) saves f31 through the paired-single idiom the target uses
 *     (`stfd f31,.. ; psq_st f31,.. ; psq_lx f31`) where our command line emits `stfd`/`lfd`
 *     only - the stopping rule's shape (`docs/matching.md`: 785 such instructions, none
 *     matching).  Recorded; fn_8015ED94 has the same prologue and is unwritten.
 *   * the big per-motion effect/keyframe drivers fn_8015FD84 (0x18DC), fn_80163A48 (0x1F18),
 *     fn_80162500 (0xCC4) and fn_80162154 (0x2E8) call `em_frame_check` + `setVector3` +
 *     `eft_em_spawn`/`fn_801635A4` hundreds of times in straight-line keyframe streams; they need
 *     the keyframe tables (the `.data` records at 0x805A5E78/0x805A5EB4 and friends) recovered
 *     first.  The rest of the follow-up queue: fn_8015EB44, fn_8015ED94, fn_8015F0E8, fn_8015F820,
 *     fn_8015F8E4, fn_8015F9FC, fn_8015FAC0, fn_8015FB78, fn_8015FD1C, fn_80161660, fn_80162154,
 *     fn_8016243C, fn_801631C4, fn_80163298, fn_80163344, fn_801634F8, fn_801635A4, fn_801639C0,
 *     fn_80165964, fn_80165C14, fn_80165CD0.
 *   * fn_80161660 and fn_8015ED94 also need a VEC3 at +0x31C that `include/enemy.h` currently
 *     spells `pad_0x31C[4]` before its `v_0x320` VEC3 - a genuine two-view clash (this unit's
 *     actors read a vector at +0x31C; `enemy/fn_8014A1BC.c` reads `v_0x320` at +0x320), so the
 *     type needs a union before those bodies can be written (rule 1/3).  Left for the follow-up.
 */

/* Retired header of `enemy/fn_80165FC8.cpp` (kept for its notes and residuals): */
/* enemy/fn_80165FC8.cpp - the enemy per-area seat/action unit, 0x80165FC8..0x801679B0 (21 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/dumpmap.py lookup` on every one of the 21 addresses: the shared runtime dump answers
 * only `zz_<addr>_`, and `config/RMHE08/symbols.txt` carries nothing but the bare `fn_XXXXXXXX`
 * entries - no `__FILE__`/class string names a file here).
 *
 * What it is.  The range sits between the two registered enemy action/state units
 * (`enemy/fn_8015E854.cpp` ends exactly at 0x80165FC8, `enemy/fn_801679B0.cpp` starts exactly at
 * 0x801679B0) and continues the same family: it drives the shared `_ENEMY_WORK` record.
 *   * `fn_80165FC8` and `fn_80166DF8` are the per-area seat/entry selectors: they switch on
 *     `stage_map_kind_get(self->field_0x1E0)` (the map lookup) and on `self->area_no` (+0x1E1) and the
 *     entry state `self->field_0x9F6`, then call the motion setter `fn_80126324` with the per-area
 *     motion ids.
 *   * `fn_801663E4` is the big nested dispatch that maps (map, area, entry-state) to the same
 *     `fn_80126324` ids and reports whether the entry is still running.
 *   * `fn_801671AC` is the per-tick seat hook; the `fn_8016730C`..`fn_801678A0` set is the
 *     per-motion state step (`switch (self->state)` -> set a motion through `em_mot_set`/
 *     `em_mot_set_ck` and wait on `em_mot_end_ck`/`em_frame_check`), dispatched by `fn_80167404` /
 *     `fn_80167968` on `self->state_sub` (+0x1E6).
 *   * `fn_80166330` is the `.ctors` initializer that seeds the two global float vectors
 *     (`vec_pair_80165FC8_0`, `vec_pair_80165FC8_1`); the range owns `.ctors 0x8056F328..0x8056F32C`.
 *   * `fn_801661BC`, `fn_801661FC` and `fn_801662D4` are three methods of the `ResUserDataAc`
 *     user-data accessor (their addresses fall in this range, so they are emitted here): the
 *     `lbl_805A6D28` vtable slots +0x18, +0x2C and +0x08.  The table itself is another TU's data
 *     (docs/plan.md 6.5 rule 10: a table outside our ranges is referenced, not emitted), so these
 *     stay flat `extern "C"` functions and no class with virtuals is declared here.
 *
 * The seam is unproven (docs/plan.md 8.3).  It is exactly the unclaimed gap between the two
 * registered units, so both edges are their `.text` edges; the range is not contiguous with
 * `enemy/fn_8015E854.cpp`'s own unwritten follow-up queue and does not reuse its jump tables.
 *
 * Language: C++.  Every callee the range reaches through a mangling is declared at its real
 * signature (`em_frame_check`, `em_die_ck`, `em_act_ck`, `get_move_work_adrs`, `get_move_work_max`,
 * `ran_suu`, `rotMatrixX`, `rotMatrixZ`, `copyMat33`) - rule 9 never spells the mangling.  The
 * range's own flat symbols stay C-linkage through the `extern "C"` block in
 * `include/enemy/fn_80165FC8.h`.
 *
 * Object: `_ENEMY_WORK`, included from `include/enemy/ENEMY_WORK.h` (the one shared home; the fields
 * this range names were added there - +0x00C, +0x320, +0x608/+0x610, +0x834/+0x835).  The
 * `enemy_data_find` entry and the `ResUserDataAc` accessor are in this unit's own header.
 *
 * Flags: no deviation - the `enemy` lib's `cflags_main` measured every body below.  No `#pragma`.
 *
 * State of the reconstruction.  All 21 bodies are written; the per-symbol measurements are in the
 * outbox (`flags_probed` empty - no flag was needed).  The residual differences are recorded with
 * the function that carries them.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8015D860.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "ef.h"              /* VEC3_ctor */
#include "ef/fn_80105314.h"  /* fn_801057A4 */
#include "draw_shape.h"      /* draw_shape_arm */
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "ef.h"
#include "enemy/EnemyData.h"
#include "enemy/fn_8015D860.h" /* the band below: fn_8015DDB8/... (rule 2, moved out of unsplit/enemy.h) */
#include "fn_8004CAD8.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
/* `enemy/fn_80165FC8.h` spells these callees with signatures that clash with this file's own views, so each is hidden
 * for the include. */
#define assignVec3 assignVec3_hidden_fn_80165FC8_h
#define em_act_ck em_act_ck_hidden_fn_80165FC8_h
#define em_fall_height_get em_fall_height_get_hidden_fn_80165FC8_h
#define em_frame_flag_set em_frame_flag_set_hidden_fn_80165FC8_h
#define em_mot_set em_mot_set_hidden_fn_80165FC8_h
#define fn_8013918C fn_8013918C_hidden_fn_80165FC8_h
#define fn_8013A654 fn_8013A654_hidden_fn_80165FC8_h
#define fn_8015D934 fn_8015D934_hidden_fn_80165FC8_h
#define rotMatrixX rotMatrixX_hidden_fn_80165FC8_h
#define rotMatrixZ rotMatrixZ_hidden_fn_80165FC8_h
#include "enemy/fn_80165FC8.h"
#undef rotMatrixZ
#undef rotMatrixX
#undef fn_8015D934
#undef fn_8013A654
#undef fn_8013918C
#undef em_mot_set
#undef em_frame_flag_set
#undef em_fall_height_get
#undef em_act_ck
#undef assignVec3
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */
#include "stage/stg_w.h"
/* rotMatrixZ_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define rotMatrixZ_view1 ((void (*)(u32, nw4r::math::MTX34*))rotMatrixZ)
/* rotMatrixX_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define rotMatrixX_view1 ((void (*)(u32, nw4r::math::MTX34*))rotMatrixX)
/* fn_8015D934_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8015D934_view1 ((u8 (*)(struct _ENEMY_WORK*))fn_8015D934)
/* fn_8013A654_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013A654_view1 ((void (*)(ResUserDataAc*, u32))fn_8013A654)
/* fn_8013918C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013918C_view1 ((void (*)(void*, s16))fn_8013918C)
/* em_motion_param_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_motion_param_set_view1 ((void (*)(struct _ENEMY_WORK*, u32, f32))em_motion_param_set)
/* em_mot_finished_ck_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_mot_finished_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_mot_finished_ck)
/* em_parts_damage_level_get_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_parts_damage_level_get_view1 ((u8 (*)(_ENEMY_WORK*, u8))em_parts_damage_level_get)
/* em_mot_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_mot_set_view1 ((void (*)(struct _ENEMY_WORK*, u32, u32, u32))em_mot_set)
/* em_frame_flag_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_frame_flag_set_view1 ((void (*)(void))em_frame_flag_set)
/* em_fall_height_get_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_fall_height_get_view1 ((void (*)(struct _ENEMY_WORK*))em_fall_height_get)
/* em_act_ck_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_act_ck_view1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
/* assignVec3_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define assignVec3_view1 ((void (*)(nw4r::math::VEC3*, const nw4r::math::VEC3*))assignVec3)

/* ---------------------------------------------------------------------------------------------------
 * the 0xC-byte vtable helper this unit allocates
 * ------------------------------------------------------------------------------------------------- */

/* The helper `fn_8015D9C8` allocates and `fn_8015DAA8` constructs: a 12-byte record whose +0x00
 * word is the address of a `.data` table (`lbl_805A6D28`, the same shape as
 * `enemy/fn_80147CE0.cpp`'s `Helper_80147CE0` and `enemy/fn_80176C58.cpp`'s `Helper_80176E50` -
 * all three constructors call this band's base `em_res_user_data_ctor` first and then store their own
 * table).  Only the table slot is touched by this unit; the rest is padding.
 * size: 0xC (traced from the `operator new(0xC)` call in `fn_8015D9C8`). */
typedef struct Helper_8015DAA8 {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_8015DAA8;

/* The helper's `.data` table (a label the split has not assigned to a unit; rule 10: reference it,
 * never define it - declaring the class here would make MWCC emit a table into this object). */
extern "C" u8 lbl_805A6D28[];

/* ---------------------------------------------------------------------------------------------------
 * callees
 * ------------------------------------------------------------------------------------------------- */

/* Declared in a shared header: `em_move_mode_set`, `em_mot_set`, `em_mot_set_ck`, `em_mot_end_ck`,
 * `em_action_finish`, `em_state_set`, `fn_8013032C`, `fn_801303EC`, `fn_80130CDC`, `em_busy_timer_reset`,
 * `fn_80133BB4`, `em_state_refresh`, `em_part_hit_set`, `fn_80136D14`, `fn_801376B4`, `fn_8013221C`,
 * `fn_80132224`, `fn_80132264`, `em_spawn_request`, `fn_8012EC3C` (`unsplit/enemy.h`);
 * `fn_80128A8C`, `fn_80128AAC`, `em_hit_window_set`, `fn_801280F4` (`enemy/fn_801251D0.h`);
 * `em_busy_ck` (`enemy/fn_8012BDF4.h`); `em_res_user_data_ck`, `em_res_user_data_set` (`enemy/fn_80138074.h`);
 * `em_res_user_data_ctor` (`enemy/fn_80147CE0.h`); `VEC3_ctor` (`ef.h`); `fn_801057A4`
 * (`ef/fn_80105314.h`); `draw_shape_arm` (`draw_shape.h`); `setVector3` (`nw4r/math.h`);
 * `system_w` (`unsplit/unknown.h`). */

/* The three C++ free functions, declared by their real signatures so the front-end mangles them to
 * the map spellings (rule 9: the mangled spelling is never written). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wpos_em(_ENEMY_WORK* self, u32 joint, Vec3* out);

/* This unit's own forward declarations (each is called above its definition). */
extern "C" Helper_8015DAA8* fn_8015DAA8(Helper_8015DAA8* self);
extern "C" void fn_8015DE48(_ENEMY_WORK* self);
extern "C" void fn_8015DEC4(_ENEMY_WORK* self);
extern "C" void fn_8015DF40(_ENEMY_WORK* self);
extern "C" void fn_8015DFBC(_ENEMY_WORK* self);
extern "C" void fn_8015E0BC(_ENEMY_WORK* self);
extern "C" void fn_8015E148(_ENEMY_WORK* self);
extern "C" void fn_8015E1C4(_ENEMY_WORK* self);
extern "C" void fn_8015E338(_ENEMY_WORK* self);
extern "C" void fn_8015E454(_ENEMY_WORK* self);
extern "C" void fn_8015E4F8(_ENEMY_WORK* self);
extern "C" void fn_8015E568(_ENEMY_WORK* self);
extern "C" void fn_8015E62C(_ENEMY_WORK* self);
extern "C" void fn_8015E6E8(_ENEMY_WORK* self);
extern "C" void fn_8015E788(_ENEMY_WORK* self);

/* Declarations for addresses whose bracketing registered units name different modules, so rule 2's
 * band header has no sound home for them (the same gap and the same spellings as the landed
 * `enemy/fn_80147CE0.cpp`, whose band these addresses sit in). */
/* 0x80305924 - between `ai/fn_802D0DCC.c` and `ef/fn_803066F0.c`: the effect spawn
 * `(self, u32, u32, VEC3*, f32, s32)`. */
extern "C" void fn_80305924(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s, s32 id);
/* 0x80304508 - the same band: `(self, u32, u32, VEC3*, f32)`. */
extern "C" void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);
/* 0x803B9BA0 - between `hud/fn_80324F7C.c` and `Network/NetworkWiiMediator.c`:
 * `(self, VEC3*, s32)`. */
extern "C" void fn_803B9BA0(_ENEMY_WORK* self, void* pos, s32 value);

/* The `.sdata2` pool this band's float work reads (values measured from the DOL's `.sdata2`;
 * pooled data owned by another unit - declared, never defined, playbook 29). */
extern f32 lbl_80797330; /* 0.0 */
extern f32 lbl_80797334; /* -20.0 */
extern f32 lbl_80797338; /* 100.0 */
extern f32 lbl_8079733C; /* 1.0 */
extern f32 lbl_80797340; /* 50.0 */
extern f32 lbl_80797344; /* 150.0 */
extern f32 lbl_80797348; /* 160.0 */
extern f32 lbl_8079734C; /* -50.0 */
extern f32 lbl_80797350; /* 164.0 */
extern f32 lbl_80797354; /* 264.0 */
extern f32 lbl_80797358; /* 0.6 */
extern f32 lbl_8079735C; /* 120.0 */

extern u32 em_frame_check(_ENEMY_WORK*, u16, f32, f32);
extern u32 em_parts_damage_level_get(_ENEMY_WORK*, u8);

extern "C" {
extern void fn_8015EA24(_ENEMY_WORK*);
extern void fn_8015EAC8(_ENEMY_WORK*);
extern void fn_8015EB44(_ENEMY_WORK*);
extern void fn_8015ED00(_ENEMY_WORK*);
extern void fn_8015ED94(_ENEMY_WORK*);
extern void fn_8015EED4(_ENEMY_WORK*);
extern void fn_8015EFAC(_ENEMY_WORK*);
extern void fn_8015F0E8(_ENEMY_WORK*);
extern void fn_8015F6C4(_ENEMY_WORK*, u8);
extern void fn_8015FD84(_ENEMY_WORK*, u8);
extern void fn_80161660(_ENEMY_WORK*);
extern void fn_80161784(_ENEMY_WORK*, u8);
extern void fn_801618A4(_ENEMY_WORK*);
extern void fn_8016198C(_ENEMY_WORK*, u8);
extern void fn_80161A34(_ENEMY_WORK*, u8);
extern void fn_80161D24(_ENEMY_WORK*);
extern void fn_80161DCC(_ENEMY_WORK*, u8);
extern void fn_80161E94(_ENEMY_WORK*, u8);
extern void fn_8015F820(_ENEMY_WORK*);
extern void fn_8015F8E4(_ENEMY_WORK*);
extern void fn_8015F9FC(_ENEMY_WORK*);
extern void fn_8015FAC0(_ENEMY_WORK*);
extern void fn_8015FB78(_ENEMY_WORK*);
extern void fn_80162500(_ENEMY_WORK*);
extern void fn_801631C4(_ENEMY_WORK*);
extern void fn_8015E854(_ENEMY_WORK* self, u8 arg1, u8 arg2);
extern void fn_801624E4(_ENEMY_WORK* self);
extern void fn_801624EC(_ENEMY_WORK* self);
extern void fn_80163274(_ENEMY_WORK* self);
extern void fn_80165960(void);
extern s32 fn_80165BC0(void);
extern void fn_80165BC8(_ENEMY_WORK*, u8*, u8*);
extern s32 fn_80165C20(_ENEMY_WORK*, u8);
extern s32 fn_80165C64(_ENEMY_WORK*, u32);
}

/* rodata pools this unit references (owned by the split, not defined here) */
extern u8 lbl_8056FB50[];

extern f32 lbl_80797330;
extern f32 lbl_8079733C;
extern f32 lbl_80797360;
extern f32 lbl_80797364;
extern f32 lbl_80797368;
extern f32 lbl_8079736C;
extern f32 lbl_80797380;
extern f32 lbl_80797384;
extern f32 lbl_80797390;
extern f32 lbl_80797394;
extern f32 lbl_80797398;
extern f32 lbl_8079739C;
extern f32 lbl_807973A0;
extern f32 lbl_807973A4;

extern f32 lbl_807973B0;
extern f32 lbl_807973B4;
extern f32 lbl_807973B8;
extern f32 lbl_80797340;
extern f32 lbl_807973BC;
extern f32 lbl_807973C0;

extern f32 lbl_807973E4;
extern f32 lbl_807973E8;

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x8015D860 - seed the action's flag byte: clear it, then set bit 2 when the enemy has at least
 * two damage levels on part 2, bit 0 when the mask-2 probe answers and bit 5 when the mask-1 probe
 * does. */
extern "C" void fn_8015D860(_ENEMY_WORK* self) {
    self->field_0x1E4 = 0;
    if (em_parts_damage_level_get_view1(self, 2) >= 2) {
        self->field_0x1E4 |= 4;
    }
    if (em_flags836_ck(self, 2) == 1) {
        self->field_0x1E4 |= 1;
    }
    if (em_flags836_ck(self, 1) == 1) {
        self->field_0x1E4 |= 0x20;
    }
}

/* 0x8015D8F0 - is none of the mask's bits set in the flag byte? */
extern "C" u32 fn_8015D8F0(_ENEMY_WORK* self, u8 mask) {
    return (self->field_0x1E4 & mask) == 0;
}

/* 0x8015D908 - the two-bit gate: 0 only when bit 0 and bit 1 are both set. */
extern "C" u32 fn_8015D908(_ENEMY_WORK* self) {
    if (self->field_0x1E4 & 1) {
        if (self->field_0x1E4 & 2) {
            return 0;
        }
    }
    return 1;
}

/* 0x8015D934 - count the clear bits of the low six (bit 0 .. bit 5). */
extern "C" u32 fn_8015D934(_ENEMY_WORK* self) {
    u8 count = 0;

    if ((self->field_0x1E4 & 1) == 0) {
        count = 1;
    }
    if ((self->field_0x1E4 & 2) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 4) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 8) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 0x10) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 0x20) == 0) {
        count = count + 1;
    }
    return count;
}

/* 0x8015D9B8 - clear the two +0x328/+0x32A countdowns. */
extern "C" void fn_8015D9B8(_ENEMY_WORK* self) {
    self->field_0x328 = 0;
    self->timer_0x32A = 0;
}

/* 0x8015D9C8 - the per-tick entry: on the `arg == 2` arm hand the enemy its motion, re-seed the
 * flag byte, attach the vtable helper when the enemy has none, and - when the enemy is not in the
 * +0x009 gate - spawn the position effect. */
extern "C" void fn_8015D9C8(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Helper_8015DAA8* helper;

    VEC3_ctor(&v);
    switch ((u8)arg) {
      case 2:
        em_move_mode_set(self, 4);
        fn_80128A8C(self, 6, 5);
        em_state_refresh(self);
        break;
    }
    fn_8015D860(self);
    if (em_res_user_data_ck(self) == 0) {
        helper = (Helper_8015DAA8*)operator new(0xC);
        if (helper != 0) {
            fn_8015DAA8(helper);
        }
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, lbl_80797330, lbl_80797334, lbl_80797338);
        fn_801057A4(self, 0x14, &v, lbl_8079733C, 0);
    }
}

/* 0x8015DAA8 - the helper's constructor: base first, then this class's table. */
extern "C" Helper_8015DAA8* fn_8015DAA8(Helper_8015DAA8* self) {
    em_res_user_data_ctor(self);
    self->vtbl = lbl_805A6D28;
    return self;
}

/* 0x8015DAE4 - the (state, sub-state) transition table of one action pair: state 7 advances the
 * sub-state 7/0xD to 0x10 when `fn_8012EC3C` answers, and 0xB to 0x11 while the flag byte's bit 0
 * is clear. */
extern "C" void fn_8015DAE4(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (*state) {
      case 7:
        switch (*sub) {
          case 7:
          case 0xD:
            if (fn_8012EC3C(self) == 1) {
                *sub = 0x10;
            }
            break;
          case 0xB:
            if ((self->field_0x1E4 & 1) == 0) {
                *sub = 0x11;
            }
            break;
        }
        break;
    }
}

/* 0x8015DB68 - the second transition table: action 1 arms the +0x32A countdown (sub 6), spawns the
 * joint effect when `em_busy_ck` answers (sub 8) and hands over to `fn_801376B4` (sub 9);
 * action 0xA's sub 0xC3/0xC8 arms the flag byte's bit 0/bit 5, stores the +0x491 byte, and both
 * arms spawn an effect at the enemy's position. */
extern "C" void fn_8015DB68(_ENEMY_WORK* self, u8 arg, u8 sub) {
    VEC3 v1;
    VEC3 v2;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    switch (arg) {
      case 1:
        switch (sub) {
          case 6:
            self->timer_0x32A = 0x384;
            break;
          case 8:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x14, &v2);
                em_spawn_request(self->field_0x01A, 0x21, 0, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF,
                            &v2, 0);
            }
            break;
          case 9:
            fn_801376B4(self);
            break;
        }
        break;
      case 0xA:
        switch (sub) {
          case 0xC3:
            em_part_hit_set(self, 1, 0);
            if (fn_8015D8F0(self, 1) == 1) {
                self->field_0x1E4 |= 1;
                setVector3(&v1, lbl_80797330, lbl_80797340, lbl_80797344);
                fn_80305924(self, 1, 0x14, &v1, lbl_8079733C, -1);
                fn_803B9BA0(self, &self->pos, 0x1E);
            }
            break;
          case 0xC8:
            em_part_hit_set(self, 0, 0);
            self->field_0x491 = 1;
            if (fn_8015D8F0(self, 0x20) == 1) {
                self->field_0x1E4 |= 0x20;
                setVector3(&v1, lbl_80797330, lbl_80797330, lbl_80797330);
                fn_80305924(self, 1, 0x2D, &v1, lbl_8079733C, -1);
                fn_803B9BA0(self, &self->pos, 0x1E);
            }
            break;
        }
        break;
    }
}

/* 0x8015DD6C - the per-tick bookkeeping: mirror "state 4" into the +0x38B flag and run the two
 * +0x328/+0x32A countdowns down while they are positive. */
extern "C" void fn_8015DD6C(_ENEMY_WORK* self) {
    switch (self->field_0x1E2) {
      case 4:
        self->field_0x38B = 1;
        break;
      default:
        self->field_0x38B = 0;
        break;
    }
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (self->timer_0x32A > 0) {
        self->timer_0x32A--;
    }
}

/* 0x8015DDB8 - the state-0 step: motion 0, then motion set (7, 0x12). */
extern "C" void fn_8015DDB8(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 7, 0x12);
    fn_80133BB4(self);
}

/* 0x8015DE00 - the state-0 step: motion 4, then motion set (6, 0xC). */
extern "C" void fn_8015DE00(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 0xC);
    fn_80133BB4(self);
}

/* 0x8015DE48 - the two-step state: start motion set (1, 0xA) on entry, finish on the wait. */
extern "C" void fn_8015DE48(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 0xA, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015DEC4 - the same two-step shape with motion set (2, 6). */
extern "C" void fn_8015DEC4(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015DF40 - the same two-step shape with motion set (0xE, 6). */
extern "C" void fn_8015DF40(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015DFBC - the two-step state that also refreshes the scene: motion set (1, 0, 0) and the
 * height pair on entry, `fn_801280F4` on the wait. */
extern "C" void fn_8015DFBC(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    fn_80136D14(self);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8015E05C - the sparse sub-state dispatcher: the compare chain for {0,1,2,3,4,6,7}. */
extern "C" void fn_8015E05C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_8015DE48(self);
        break;
      case 1:
        fn_8015DEC4(self);
        break;
      case 2:
        fn_8015DF40(self);
        break;
      case 3:
        fn_8015DE48(self);
        break;
      case 4:
        fn_8015DE48(self);
        break;
      case 6:
        fn_8015DE48(self);
        break;
      case 7:
        fn_8015DFBC(self);
        break;
    }
}

/* 0x8015E0BC - the two-step state with motion set (0xF, 6); the local vector is zeroed but not
 * used by either arm (retail does the same call). */
extern "C" void fn_8015E0BC(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E148 - the same two-step shape with motion set (9, 6). */
extern "C" void fn_8015E148(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E1C4 - the long step: motion set (0xF, 6) on entry, then three frame windows
 * (`em_frame_check`) that fire the part effect, the ground effect at (0, -50, 100) and the second
 * one at the same point while `system_w`'s +0x0C low three bits are clear. */
extern "C" void fn_8015E1C4(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
      case 1:
        if (em_frame_check(self, 0, lbl_80797348, lbl_80797330) == 1) {
            em_hit_window_set(self, 1, 0x12, 5);
            draw_shape_arm((u32)self, 0x14, 0xA);
        }
        if (em_frame_check(self, 0, lbl_80797348, lbl_80797330) == 1) {
            setVector3(&v, lbl_80797330, lbl_8079734C, lbl_80797338);
            eft_em_spawn(self, 0, 0x13, &v, lbl_8079733C);
        }
        if (em_frame_check(self, 3, lbl_80797350, lbl_80797354) == 1) {
            if ((system_w.field_0x0c & 7) == 0) {
                setVector3(&v, lbl_80797330, lbl_8079734C, lbl_80797338);
                eft_em_spawn(self, 1, 0x13, &v, lbl_8079733C);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E338 - the four-step state: motion set (0x28, 2), then (0x6E, 4) with the +0x20 counter
 * seeded to 0x708, then the 0.6-ratio step (0x70, 4) once the counter runs out, then the wait. */
extern "C" void fn_8015E338(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x28, 2, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
      case 2:
        fn_8013221C(self, lbl_80797358, 1, 0xA);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            fn_80132264(self);
        }
        break;
      case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E454 - the two-step state: motion set (0xC8, 8) and the +0x20 counter at 3 plus the
 * `fn_80130CDC` arm on entry; the 120-frame window hands over to `em_state_set`. */
extern "C" void fn_8015E454(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 8, 0);
        self->timer_0x020 = 3;
        fn_80130CDC(self, 0x3E8);
        break;
      case 1:
        if (em_frame_check(self, 1, lbl_8079735C, lbl_80797330) == 1) {
            em_state_set(self, 1, 8);
        }
        break;
    }
}

/* 0x8015E4F8 - the two-step state with motion set (0xCA, 2). */
extern "C" void fn_8015E4F8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_mot_set(self, 0xCA, 2, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E568 - the two-step state that counts two motion waits: motion set (0x24, 6) plus the
 * `em_hit_window_set` hand-off and the +0x20 counter cleared, then the second motion set (1, 7) once the
 * counter reaches 2. */
extern "C" void fn_8015E568(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x24, 6, 0);
        em_hit_window_set(self, 1, 0xB, 2);
        self->timer_0x020 = 0;
        /* falls through to the wait arm */
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (self->timer_0x020 >= 2) {
            self->state++;
            em_state_set(self, 1, 7);
        }
        break;
    }
}

/* 0x8015E62C - the three-step state: motion set (0x29, 8) plus a flag re-seed on entry, then
 * (0x70, 2, 0x42), then the wait. */
extern "C" void fn_8015E62C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 8, 0);
        fn_8015D860(self);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x70, 2, 0x42);
        }
        break;
      case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E6E8 - the two-step state: motion set (0xC8, 4) and the +0x20 counter at 3 on entry, then
 * the counter runs down and hands over to `em_state_set`. */
extern "C" void fn_8015E6E8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC8, 4, 0);
        self->timer_0x020 = 3;
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            if (--self->timer_0x020 <= 0) {
                em_state_set(self, 1, 5);
            }
        }
        break;
    }
}

/* 0x8015E788 - the two-step state with motion set (0xE, 6). */
extern "C" void fn_8015E788(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E804 - the contiguous sub-state dispatcher (the jump table at `jumptable_805A5DC0`). */
extern "C" void fn_8015E804(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_8015E0BC(self);
        break;
      case 1:
        fn_8015E148(self);
        break;
      case 2:
        fn_8015E1C4(self);
        break;
      case 3:
        fn_8015E338(self);
        break;
      case 4:
        fn_8015E454(self);
        break;
      case 5:
        fn_8015E4F8(self);
        break;
      case 6:
        fn_8015E568(self);
        break;
      case 7:
        fn_8015E62C(self);
        break;
      case 8:
        fn_8015E6E8(self);
        break;
      case 9:
        fn_8015E788(self);
        break;
    }
}

#pragma peephole on

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_8015E854(_ENEMY_WORK* self, u8 arg1, u8 arg2) {
    u8 temp_r4;
    u8 temp_a1;
    u8 temp_a2;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 8, 0);
        temp_a1 = arg1;
        switch ((s32) temp_a1) {
        case 0:
            em_approach_start(self, lbl_80797360, 0);
            return;
        case 1:
            em_approach_start(self, lbl_80797360, 0);
            return;
        case 2:
            em_approach_start(self, lbl_80797364, 0);
            return;
        case 3:
            em_approach_start(self, lbl_80797330, 0);
            return;
        case 4:
            em_approach_start(self, lbl_80797368, 0);
            return;
        case 5:
            em_approach_start(self, lbl_8079736C, 0);
            return;
        case 6:
            em_approach_start(self, lbl_80797364, 0);
            return;
        }
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            if (fn_8012F948(self) == 0) {
                temp_a2 = arg2;
                switch ((s32) temp_a2) {
                case 0:
                    self->state = self->state + 1;
                    em_mot_set(self, 0xB, 4, 0);
                    return;
                case 1:
                    em_action_finish(self);
                    return;
                }
            }
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015EA24(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056FB50, 0, 1, 0);
        if (self->field_0x482 == 1U) {
            em_mot_speed_set(self, lbl_8079733C);
        }
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FB50) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015EAC8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015ED00(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 0xA, 0);
        em_approach_start(self, lbl_80797330, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016198C(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r5;
    f32 temp_f1;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        if ((u8) arg1 == 0) {
            temp_f1 = lbl_807973E8;
        } else {
            temp_f1 = lbl_807973B0;
        }
        if (em_frame_check(self, 1, temp_f1, lbl_80797330) == 1U) {
            fn_8015DDB8(self);
        }
        return;
    }
}

extern "C" void fn_80162028(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0x2C);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            fn_8015DDB8(self);
        }
        return;
    }
}

extern "C" void fn_801624E4(_ENEMY_WORK* self) {
    fn_8015F6C4(self, 0);
}

extern "C" void fn_801624EC(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801624E4(self);
    }
}

extern "C" void fn_80163274(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_80162500(self);
        return;
    case 1:
        fn_801631C4(self);
        return;
    }
}

extern "C" void fn_80165960(void) {
}

extern "C" s32 fn_80165BC0(void) {
    return 1;
}

extern "C" void fn_80165BC8(_ENEMY_WORK* self, u8* out_state, u8* out_flags) {
    em_move_mode_set(self, 4);
    *out_state = 0xC;
    *out_flags = 0;
}

extern "C" s32 fn_80165C20(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4 = arg1;

    if ((u8) (temp_r4 - 3) <= 1U) {
        if (em_alt_mode_ck(self) == 1U) {
            return 1;
        }
    }
    return 0;
}

extern "C" s32 fn_80165C64(_ENEMY_WORK* self, u32 arg1) {
    u8 temp_r4;
    u8 temp_r0;
    u32 temp_r3;

    temp_r4 = (u8) arg1;
    if (temp_r4 <= 2U) {
        temp_r3 = em_parts_damage_level_get(self, temp_r4);
        temp_r0 = (u8) temp_r3;
        if ((temp_r0 & 1) == 0) {
            return 1;
        }
    } else if ((u32) (temp_r4 - 3) <= 1U) {
        if (fn_8012EC3C(self) == 1U) {
            return 1;
        }
    }
    return 0;
}

extern "C" void fn_8015EED4(_ENEMY_WORK* self) {
    u8 temp_r4;
    f32 temp_f1;
    f32 t1;
    f32 t2;
    f32 t3;
    f32 t4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80797330, lbl_80797390) == 1U) {
            temp_f1 = get_em_base_scale(self);
            t1 = lbl_8079739C * temp_f1;
            t2 = t1 * lbl_80797398;
            t3 = t2 / lbl_807973A0;
            t4 = lbl_80797394 + t3;
            em_turn_to_target(self, (u16) (s32) t4);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015EFAC(_ENEMY_WORK* self) {
    f32* temp_r31;
    u8 temp_r3;
    f32 temp_f0;
    f32 temp_f1;

    temp_r31 = fn_80126454(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 6, 0);
        temp_f1 = fn_80050EF4(&self->pos, &self->vec_0x36C);
        temp_f1 = lbl_807973A4 * temp_f1;
        temp_f0 = -temp_r31[1];
        if (temp_f1 > temp_f0) {
            temp_f1 = temp_f0;
        }
        em_approach_start(self, temp_f1, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            if (fn_8012F948(self) == 0) {
                self->state = self->state + 1;
                em_mot_set(self, 0xB, 4, 0);
                em_approach_start(self, lbl_80797330, 0);
            }
        }
        return;
    case 2:
        em_approach_step(self, 0, 0x80);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801618A4(_ENEMY_WORK* self) {
    u8 temp_r4;
    f32 temp_f1;
    f32 t1;
    f32 t2;
    f32 t3;
    f32 t4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 4, 0);
        em_hit_window_set_default(self, 0, 0xC);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80797330, lbl_80797384) == 1U) {
            temp_f1 = get_em_base_scale(self);
            t1 = lbl_807973E4 * temp_f1;
            t2 = t1 * lbl_80797398;
            t3 = t2 / lbl_807973A0;
            t4 = lbl_80797394 + t3;
            em_turn_to_target(self, (u16) (s32) t4);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80161D24(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 2, 0);
        em_hit_window_set_default(self, 0, 2);
        em_hit_window_set_default(self, 1, 0x10);
        fn_80130CDC(self, -0x1E);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80161DCC(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 2, 0);
        if ((u8) arg1 == 0) {
            em_hit_window_set_default(self, 0, 3);
        } else {
            em_hit_window_set(self, 0, 0x14, 0x40);
        }
        fn_80130CDC(self, -0x1E);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015F2D8(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_hit_window_set_default(self, 0, 0xE);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_807973B4, lbl_80797330) == 1U) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F39C(_ENEMY_WORK* self) {
    u8 temp_r3;
    s32 temp_r0;

    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 == 0) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F450(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_mot_set(self, 6, 0, 0);
        em_approach_start(self, lbl_80797380, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F510(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r3;
    f32 f31;

    f31 = lbl_80797330;
    em_busy_set(self);
    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 0, 0);
        if ((u8) arg1 == 0) {
            f31 = lbl_807973B8;
        }
        em_approach_start(self, f31, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F60C(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_8056FB50, 0, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FB50) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F6C4(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        if ((u8) arg1 == 0) {
            em_mot_set(self, 0x27, 2, 0x24);
            fn_801303EC(self, lbl_807973BC);
            em_hit_window_set_default(self, 0, 0xF);
        } else {
            em_mot_set(self, 0x27, 2, 0);
            fn_801303EC(self, lbl_80797330);
            em_hit_window_set_default(self, 0, 0x11);
        }
        fn_80136D14(self);
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_807973C0, lbl_80797330) == 1U) {
            fn_80136D14(self);
        }
        if ((u8) arg1 == 0) {
            if (self->field_0x1AC >= lbl_80797330) {
                fn_801303EC(self, lbl_80797330);
            } else {
                fn_801303FC(self, lbl_80797340);
            }
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80161784(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        em_move_mode_set(self, 0);
        if ((u8) arg1 == 0) {
            em_mot_set(self, 0x14, 4, 0);
            em_hit_window_set_default(self, 0, 5);
            em_hit_window_set_default(self, 1, 6);
        } else {
            em_mot_set(self, 0x1A, 4, 0);
            em_hit_window_set_default(self, 0, 7);
            em_hit_window_set_default(self, 1, 8);
        }
        return;
    case 1:
        if ((u8) arg1 == 0) {
            em_turn_in_window(self, lbl_80797330, lbl_80797340, 0x4000);
        } else {
            em_turn_in_window(self, lbl_80797330, lbl_80797340, -0x4000);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015FCB0(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015F2D8(self);
        return;
    case 1:
        fn_8015F39C(self);
        return;
    case 2:
        fn_8015F450(self);
        return;
    case 3:
        fn_8015F510(self, 0);
        return;
    case 4:
        fn_8015F60C(self);
        return;
    case 5:
        fn_8015F6C4(self, 0);
        return;
    case 6:
        fn_8015F510(self, 1);
        return;
    case 7:
        fn_8015F820(self);
        return;
    case 8:
        fn_8015F8E4(self);
        return;
    case 9:
        fn_8015F6C4(self, 1);
        return;
    case 10:
        fn_8015F9FC(self);
        return;
    case 11:
        fn_8015FAC0(self);
        return;
    case 12:
        fn_8015FB78(self);
        return;
    case 13:
        return;
    }
}

extern "C" void fn_8015F23C(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015E854(self, 0, 0);
        return;
    case 1:
        fn_8015E854(self, 1, 1);
        return;
    case 2:
        fn_8015E854(self, 2, 1);
        return;
    case 3:
        fn_8015E854(self, 3, 1);
        return;
    case 4:
        fn_8015E854(self, 4, 1);
        return;
    case 5:
        fn_8015E854(self, 5, 1);
        return;
    case 6:
        fn_8015EA24(self);
        return;
    case 7:
        fn_8015EAC8(self);
        return;
    case 8:
        fn_8015EB44(self);
        return;
    case 9:
        fn_8015ED00(self);
        return;
    case 10:
        fn_8015ED94(self);
        return;
    case 11:
        fn_8015EED4(self);
        return;
    case 12:
        fn_8015E854(self, 6, 1);
        return;
    case 13:
        fn_8015EFAC(self);
        return;
    case 14:
        fn_8015F0E8(self);
        return;
    }
}

extern "C" void fn_801620A4(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015FD84(self, 0);
        return;
    case 1:
        fn_80161660(self);
        return;
    case 2:
        fn_80161784(self, 0);
        return;
    case 3:
        fn_8015FD84(self, 1);
        return;
    case 4:
        fn_80161DCC(self, 1);
        return;
    case 5:
        fn_801618A4(self);
        return;
    case 6:
        fn_8016198C(self, 0);
        return;
    case 7:
        fn_80161A34(self, 0);
        return;
    case 8:
        fn_80161D24(self);
        return;
    case 9:
        fn_80161DCC(self, 0);
        return;
    case 10:
        fn_80161784(self, 1);
        return;
    case 11:
        fn_80161E94(self, 0);
        return;
    case 12:
        fn_8016198C(self, 1);
        return;
    case 13:
        fn_80161A34(self, 1);
        return;
    case 14:
        fn_8015FD84(self, 2);
        return;
    case 15:
        fn_8015FD84(self, 3);
        return;
    case 16:
        fn_80161A34(self, 2);
        return;
    case 17:
        fn_80161E94(self, 1);
        return;
    case 18:
        fn_80162028(self);
        return;
    }
}

/* -------------------------------------------------------------------------------------------------
 * the range's functions, in address order
 * ------------------------------------------------------------------------------------------------- */
u32 fn_80165FC8(_ENEMY_WORK* self, u32 arg1) {
    s16* seat = &self->field_0x328;
    u8 entry;
    u32 sel;

    get_move_work_adrs(3);
    get_move_work_max(3);
    entry = stage_map_kind_get(self->field_0x1E0);
    if ((s32)entry != 2) {
        return 0;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    sel = 0xFF;
    if ((s32)entry == 2) {
        sel = 3;
    }
    if (sel != 0xFF && (s32)fn_80129DB8(self) == 2) {
        return 1;
    }
    if (em_mot_finished_ck_view1(self) == 0 && self->value_0x452 >= 0x384) {
        u32 motion;
        if ((s32)self->area_no != 6) {
            if (fn_802B0998(3) == 1) {
                motion = 6;
            } else {
                motion = 4;
            }
        } else {
            motion = 6;
        }
        if (fn_8012A014(self, 0, motion, (u16)arg1, 0, lbl_805A5DB4) == 1) {
            return 1;
        }
    }
    if (em_mot_finished_ck_view1(self) == 0 && self->value_0x452 >= 0x384 && fn_8015D934_view1(self) <= 2 && (s32)entry == 2
        && self->area_no != 3 && fn_80129A1C(self, (u16)arg1, 0x1E, seat) == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 3;
        return 1;
    }
    if (fn_80129A70(self, (u16)arg1) == 1) {
        return 1;
    }
    return fn_8012A204(self) == 1;
}

void fn_801661BC(ResUserDataAc* self) {
    u8 pad[0x1C];

    fn_8005D1AC(pad, 0);
    fn_8013A654_view1(self, 7);
}

void fn_801661FC(ResUserDataAc* self, MTX34* mtx, void* cursor, s32 arg3) {
    u32 head[1];
    s32 idx;
    MTX34 local;
    MTX34 out;

    (void)arg3;
    fn_8005D1AC(head, 0);
    MTX34_ctor(&out);
    MTX34_ctor(&local);
    idx = (s32)(u32)fn_80097EB0(cursor, 0x18);
    fn_8005D0CC(head, &idx);
    {
        _ENEMY_WORK* work = self->work;
        fn_800532DC(&local, &mtx[fn_8006FDCC(head)]);
        mtx34_identity(&out);
        rotMatrixX_view1(work->field_0x608, &out);
        rotMatrixZ_view1(work->field_0x610, &out);
        mtx34_concat_assign(&local, &out);
        copyMat33(&mtx[fn_8006FDCC(head)], &local);
    }
}

ResUserDataAc* fn_801662D4(ResUserDataAc* self, s32 flags) {
    if (self != 0) {
        fn_8013918C_view1(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

void fn_80166330(void) {
    VEC3 v0;
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    /* `setVec3` returns its first argument (the retail call site keeps it in r4 across the `bl`), so
     * the cast back to the callee's `VEC3*` costs no instruction. */
    assignVec3_view1(&vec_pair_80165FC8_0[0], (VEC3*)setVec3(&v0, lbl_80797330, lbl_80797338, lbl_80797330));
    assignVec3_view1(&vec_pair_80165FC8_0[1], (VEC3*)setVec3(&v1, lbl_80797330, lbl_807974EC, lbl_80797330));
    assignVec3_view1(&vec_pair_80165FC8_1[0], (VEC3*)setVec3(&v2, lbl_80797330, lbl_80797330, lbl_80797330));
    assignVec3_view1(&vec_pair_80165FC8_1[1], (VEC3*)setVec3(&v3, lbl_80797330, lbl_807974EC, lbl_80797330));
}

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A7868..0x806A7898`), in address order: the 2 two-vector record(s)
 * its static constructor `fn_80166330` builds (`.data` tables point at them).  Names are GUESSes: each record is a
 * pair of model-space points. */
VEC3 vec_pair_80165FC8_0[2];  /* +0x806A7868 */
VEC3 vec_pair_80165FC8_1[2];  /* +0x806A7880 */
