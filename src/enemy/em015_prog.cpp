/* enemy/em015_prog.cpp - enemy 015 program
 *
 * `.text` 0x80176C30..0x80182C40, 81 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): fold of 7 registered units, built from `enemy/fn_80176C58.cpp`, `enemy/fn_80177608.cpp`, `enemy/fn_80177890.cpp`, `enemy/fn_80178128.cpp`, `enemy/fn_80178378.cpp`, `enemy/fn_80181C88.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 *
 * Kept views: the retired sources declared 11 callee(s) with different signatures (`em_approach_start`, `em_die_ck`, `em_hit_window_set`, `em_mot_set_blend`, `em_parts_damage_level_get`, `em_state_set`, `em_target_pos_set`, `em_turn_seq_start`, `em_turn_to_target`, `fn_80128A8C`, `fn_801823A0`); each function keeps its own source's view through a function-pointer cast macro (`<name>_viewN`, `<name>_cN`), which compiles to the same direct call, so the fold does not move any body.
 * Demoted: it absorbed the Matching unit `enemy/fn_80177608` (it matched; it is NonMatching here for the same reason).
 */

/* Retired header of `enemy/fn_80176C58.cpp` (kept for its notes and residuals): */
/* enemy/fn_80176C58.cpp - the enemy death/state sub-action unit, 11 functions, 0x80176C58..0x80177608.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every symbol this file defines is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * This is the small enemy-side state block discovery proposed between the enemy action unit
 * (`enemy/fn_8012BDF4.cpp`) and the enemy motion unit that follows it.  It is reached with a
 * `_ENEMY_WORK*` (the `em_*__FP11_ENEMY_WORK...` callees pin that type) and drives the enemy's
 * "sub state" `_ENEMY_WORK::state_sub` (+0x1E6): `fn_801775C0` is the dispatcher that calls one of
 * `fn_80177314`/`fn_801773B0`/`fn_80177430`/`fn_801774B0`/`fn_80177540` for sub states 0/1/2/4/5, and
 * each of those is the same three-step shape - bump `state_0x05`, hand the enemy a motion
 * (`em_move_mode_set` + `fn_8012F5C4`), then wait for it (`em_mot_end_ck`) and finish (`em_action_finish` /
 * `fn_80128030`).
 *
 * The rest is the enemy's damage/death bookkeeping:
 *   - `fn_80176C58(self, arg)` is the per-tick entry: it attaches a 12-byte helper (the vtable object
 *     `lbl_805AA900`, constructed by `fn_80176E50`) when `em_res_user_data_ck` says the enemy has none, then
 *     resets `field_0x1E4`, picks a motion set from `stage_map_kind_get(self->field_0x1E0)`, and - when the
 *     enemy's `state_0x009` is clear - spawns the effect (`setVector3` + `fn_801057A4` + `fn_8010A7D4`).
 *   - `fn_80176E8C` / `fn_8017708C` are the two state transition tables, keyed on the current
 *     `(state, sub-state)` pair read out of two `u8` records the caller passes in.
 *   - `fn_8017708C` also advances the damage meter `field_0x1E4` (its `sub == 6` arm raises it by a
 *     step clamped to `em_parts_damage_level_get(self, 3)`), and `fn_80177258` is the death check
 *     (`em_die_ck`) that arms the death timers `field_0x32F`/`field_0x330`/`timer_0x332`.
 *
 * Language.  The object's mapped callees with an argument list are C++ manglings
 * (`em_die_ck__FP11_ENEMY_WORK`, `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`) and the helper is allocated with `operator new`
 * (`__nw__FUl`), so the file is C++: the mangled callees are declared through their real signatures
 * (the front-end mangles them back to the map spellings) and the definitions keep their unmangled map
 * names with `extern "C"`.  No `__FILE__` string names a source file, so the map's `fn_XXXXXXXX`
 * placeholder is kept (Naming note above).
 *
 * Types.  `_ENEMY_WORK` is the shared record in `include/enemy.h`; this unit's additions to it
 * (the death meter `field_0x1E4`, the death timers `field_0x32F`/`field_0x330`/`timer_0x332`,
 * `field_0x38A` and the effect scale `field_0x7B0`) are declared there.  The 12-byte helper
 * `fn_80176C58` allocates is defined here (only this unit touches it).
 *
 * Source pragmas, evidenced: `#pragma peephole off` - retail keeps the unfused `clrlwi`+`cmpwi` /
 * `clrlwi`+`slwi` pairs the `-O3` peephole folds into `clrlwi.`/`clrlslwi` (playbook 39, the same
 * finding as `enemy/fn_8012BA00.c` and `enemy/fn_8012BDF4.cpp`; `fn_80176E50` and `fn_80176C58` are
 * the two functions whose score it changes).
 *
 * Status (official report metric, per symbol, measured in the worktree with
 * `recompile.py --main <worktree>`): 9 of the 11 functions are exactly 100 % (`fn_80176E50`,
 * `fn_80176E8C`, `fn_80177258`, `fn_80177314`, `fn_801773B0`, `fn_80177430`, `fn_801774B0`,
 * `fn_80177540`, `fn_801775C0`), `fn_80176C58` is 99.52 % and `fn_8017708C` is 86.52 %.  `extab` is
 * byte-identical to the target; `extabindex` differs only in `fn_8017708C`'s size word (our 0x1EC vs
 * the target's 0x1CC), which closes when that function does.  The object stays `NonMatching`: the
 * helper's vtable (`lbl_805AA900`) and the `.sdata2` pool are not claimed by this unit (the splitter
 * leaves them in the auto band), so the sections are not byte-identical even where the code is.
 *
 * Residual - `fn_8017708C` 86.52 % (target 0x1CC = 460 B, ours 0x1EC = 492 B).  The 32-byte gap is
 * the two nested-switch dispatches (case 5's `{0x19,0x1A}` and case 7's `{0x2A..0x2C}`/`{0x24,0x25}`
 * runs): retail lowers an adjacent case pair to a register range test
 * (`addi r0,r4,-0x19; cmplwi r0,1; ble`) and keeps the masked `sub` in one register across the whole
 * dispatch, while every source shape tried here re-masks and uses the two-compare range form
 * (`cmpwi r0,0x19; blt; cmpwi r0,0x1A; ble`).  Tried: `switch ((u8)sub)`, `u8 s = (u8)sub` and
 * `int s = (u8)sub` locals with `switch (s)`, and an if-chain with an explicit
 * `(u32)(s - lo) <= hi - lo` range test (that one gets the range form but inlines the handlers and
 * scores 82.8 % instead of 86.5 %).  The function is above the 80 % bar; the residual is codegen
 * shape, not comprehension.
 *
 * Inventory: `python tools/units/ledger.py unit enemy/fn_80176C58.cpp`.
 */

/* Retired header of `enemy/fn_80177608.cpp` (kept for its notes and residuals): */
/* enemy/fn_80177608.cpp - two enemy action handlers, 0x80177608..0x80177890 (2 functions, 648 bytes).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap: both
 * addresses resolve to bare `.text` entries `fn_80177608`/`fn_80177774` in config/RMHE08/symbols.txt,
 * and `dumpmap lookup` gives only a `zz_XXXXXXXX_` dump name, which is not evidence).
 *
 * What it is: a two-function slice of one enemy action - `fn_80177608` seeds a VEC3 from the engine's
 * vector helper, walks `state_0x05` through two steps and drives the effect/frame helpers
 * (`em_frame_check`, the `setVector3`/`eft_em_spawn` effect spawns); `fn_80177774` is the four-step
 * sibling that opens the action, waits on `em_mot_end_ck`, counts `field_0x20` down and closes it.
 *
 * Object: `_ENEMY_WORK` (name evidence: the mangled callee
 * `em_frame_check__FP11_ENEMY_WORKUsff` carries the 11-character type name).  Field offsets and widths
 * are read from the target's load/store instructions; only the two fields this unit touches are named
 * (`state_0x05`, `field_0x20`), the rest come from the shared record in `include/enemy.h`.
 *
 * Language: C++ (the region defines no symbol but its undefined set is mangled:
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`).
 *
 * Registration: moved to its final home `enemy/fn_80177608.cpp` from the discovery proposal
 * `proposal/80177608_fn_80177608.cpp`; the module is the band's (`enemy` - the bracketing registered
 * units are `enemy/fn_8014A1BC.c` below and `lobby/lobby_scene.c` above, and the region's callees are
 * the `em_*` enemy helpers), the name is the map's own `fn_XXXXXXXX` stem (Naming note above).
 *
 * Data runs in this range are NOT claimed: a stub object emits no `.sdata2`, and a range our object
 * does not emit must not be claimed (docs/plan.md 8.4).  The `extab`/`extabindex` fragments travel
 * with the code unit and ARE claimed in `splits.txt`.
 *
 * Declarations: `em_move_mode_set`, `em_mot_set`, `em_mot_end_ck` and `VEC3_ctor` come from the shared
 * headers (`include/unsplit/enemy.h`, `include/ef.h`); their signatures are the shared ones.  The
 * symbols whose owning unit is not registered and whose band has no sound header
 * (`eft_em_spawn`, `draw_shape_arm`, `em_hit_window_set`, `em_action_finish`) are declared here, as the landed
 * `enemy/fn_8014A1BC.c` does.  `fn_8013221C`, `fn_80132224`, `fn_80132264` belong to the `enemy` band
 * too, but `include/unsplit/enemy.h` does not carry them yet - see the outbox `shared-file` request.
 */

/* Retired header of `enemy/fn_80177890.cpp` (kept for its notes and residuals): */
/* enemy/fn_80177890.cpp - the enemy motion-state update set, `.text` 0x80177890..0x80178128.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py grep '80177890'` - every symbol in the range is a bare
 * `fn_XXXXXXXX = .text:0x...` entry with no real name).
 *
 * What it is.  The twelve functions are one enemy's per-motion update table plus the dispatcher that
 * selects them:
 *
 *   * `fn_80177BA4` switches on `_ENEMY_WORK::state_sub` (at +0x1E6) and tail-branches into the state
 *     updates; entries 0 and 1 are `fn_80177608`/`fn_80177774`, which belong to the neighbouring TU
 *     (`proposal/80177608_fn_80177608`), so the dispatcher is the seam's only evidence that the twelve
 *     are one unit.
 *   * `fn_80177890`, `fn_8017791C`, `fn_8017799C`, `fn_80177A2C`, `fn_80177AA8`, `fn_80177B24`,
 *     `fn_80177CC8` are single-step state machines: `case 0` bumps `state` (+0x5) and arms a motion
 *     through `em_mot_set_blend`/`em_mot_set`; `case 1` polls a condition (`em_frame_check`, `em_mot_end_ck`,
 *     `em_turn_seq_step`) and calls `em_action_finish` (the motion-change commit) when it returns 1.
 *   * `fn_80177BEC`, `fn_80177F30`, `fn_8017801C` take a second `u8` argument that selects which float
 *     pool constant `em_approach_start` should ease the enemy toward.
 *   * `fn_80177D54` is the compound one: three interleaved sub-steps (`state_0x006`, `state_0x007`)
 *     gated on the per-frame flags at +0xA0D / +0xA69, then the same `em_frame_check`/`em_mot_end_ck` pair.
 *
 * Language.  The range's only call into another module is `em_frame_check(ENEMY_WORK*, u16, f32, f32)`,
 * which the map carries mangled (`em_frame_check__FP11_ENEMY_WORKUsff`); every other callee is a plain
 * `fn_XXXXXXXX` C symbol.  The unit is therefore C++ (the mangled callee is declared with its real
 * signature and mangled by this front-end, docs/plan.md 6.5 rule 9) and the file is `.cpp`.
 *
 * Types.  `_ENEMY_WORK` is the shared 0xB18 record; it lives in `include/enemy/ENEMY_WORK.h` so it is
 * defined once (rule 1).  Fields the twelve functions touch are named from their call sites; the rest is
 * `unused_0xNN` padding that keeps every measured offset in place.
 *
 * Data.  The pool floats (`lbl_80797B18`..) and the string block `lbl_8056FDD0` are owned elsewhere and
 * are only declared, never defined (playbook 29), so the load operands pair with the target's pool
 * references.
 */

/* Retired header of `enemy/fn_80178128.cpp` (kept for its notes and residuals): */
/* enemy/fn_80178128.cpp - enemy action helpers: two `state_0x05` steppers and one action dispatcher.
 * .text 0x80178128..0x80178378 (0x250), 3 functions.
 *
 * The range sits in the `enemy` band between `proposal/80177890_fn_80177890` (0x80177890..0x80178128)
 * and `proposal/80178378_fn_80178378` (0x80178378..).  Every callee this unit names is an enemy-module
 * function and `fn_8017827C`'s dispatch table tail-calls five of the previous range's handlers, so the
 * module is `enemy/`.
 *
 * The three functions are the classic enemy action pattern:
 *   - `fn_80178128` - two-phase action: phase 0 arms the action (`em_move_mode_set`, `em_turn_seq_start`,
 *     `em_move_vec2_clr`) and, for motion ids 0x1F/0x20, aims `v_0x310` at the target (`calcVecAng2` ->
 *     `rotVecY`, re-scaled by `get_em_base_scale`/`get_em_chg_scale`); phase 1 runs the frame check
 *     (`em_frame_check` -> `CancelFade`) and `em_action_finish` once `em_turn_seq_step` reports done.
 *   - `fn_8017827C` - dispatcher on `state_sub` (0..0xB) tail-calling the previous range's handler
 *     functions with a sub-index argument (0..3).
 *   - `fn_801782F8` - two-phase action: phase 0 arms `em_mot_set_blend(self, 0x32, 0x28, 0, 3)`; phase 1
 *     runs `em_mot_end_ck` and `fn_80128030` on completion.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * `fn_` name defined here is a bare `.text` entry in config/RMHE08/symbols.txt, and neither the map nor
 * the runtime dump offers a real name - `dumpmap.py lookup` returns `zz_0178128_`/`zz_017827c_`/
 * `zz_01782f8_`, which is not evidence).
 *
 * Status.  All three functions are written and measure above the 80 % bar (official `report generate`
 * `fuzzy_match_percent`, this worktree, against the re-split target object):
 *   `fn_80178128` 98.12 %, `fn_8017827C` 100.00 % (byte-identical), `fn_801782F8` 100.00 %
 *   (byte-identical).  Unit measure 98.92 %, 592 B, 2/3 functions byte-identical.
 *
 * Residual.  `fn_80178128` differs by exactly one instruction in the epilogue: retail restores the f31
 * paired-single half with the indexed `li r0, 0x18; psq_lx f31, r1, r0, 0, qr0`, this build with the
 * folded `psq_l f31, 0x18(r1), 0, qr0`.  The whole body is instruction-identical.  Tried:
 * `#pragma peephole off` reproduces the indexed restore but adds a `clrlwi r0, r0, 24` to the
 * `state_0x05` byte store that retail does not have, and drops `fn_801782F8` to 96.875 %, so peephole
 * stays on and the one-instruction epilogue difference is recorded here.
 *
 * Types.  `_ENEMY_WORK` and `_CP_VECTOR` come from the shared headers (`enemy.h`, via `ef.h`); the
 * u32 at +0x1C0 the aim code reads is `_CP_VECTOR::y` of `pos_0x1BC`.  Pool literals
 * (`lbl_8056FE10`, `lbl_80797B50/60/64`) and the not-yet-owned callees are declared, never defined
 * (playbook 29), and are filed as config_requests so they can move to the shared headers.
 */

/* Retired header of `enemy/fn_80178378.cpp` (kept for its notes and residuals): */
/* enemy/fn_80178378.cpp - the enemy action band: the 64-function action arming/stepping run
 * 0x80178378..0x80181C88 (39,184 B).  Registration is this worker's; the extent is the probe's.
 *
 * MODULE AND NAME.  `enemy`: every registered neighbour below is an `enemy/fn_*.{c,cpp}` unit
 * (0x80176C58, 0x80177608, 0x80177890, 0x80178128 - the last one's `.text` ends exactly at this
 * range's start) and every callee is in the enemy band, reached through `_ENEMY_WORK`.  The range's own
 * dispatchers confirm the subsystem: `fn_80179E98` tail-calls the low action writers through
 * `jumptable_805A9574`, `fn_8017F0D8` the later ones through `jumptable_805AA530`, and `fn_8017F138`
 * (the action selector at `_ENEMY_WORK::action_0x1E5`, `+0x1E5`) picks between the neighbouring units'
 * dispatchers (`fn_801775C0`, `fn_80177BA4`, `fn_8017827C`, `fn_80179E98`, `fn_8017D5A0`, `fn_8017D778`,
 * `fn_8017DB8C`, `fn_8017DC94`, `fn_8017F0D8`).
 *
 * SEAM: unproven (docs/plan.md 8.3).  `python tools/splits/tudiscover.py at 0x80178378` returns only
 * weak cuts on both sides, no `__FILE__` string, no class string and no data the range would own, so the
 * range is registered as one unit at the probe's own extent (which is function-aligned on both ends:
 * 0x80178128 ends the unit below, 0x80181C88 starts the next function).  The merge candidate is
 * `enemy/fn_80178128.cpp`, whose 3 functions all match and whose end is the probe's boundary rather
 * than evidence of a TU seam - if the original object is one TU from 0x80178128, the two splits blocks
 * are one file and should be merged.  Checked before registering, per the campaign's continuation rule.
 *
 * SECTIONS (evidence: the range's own per-function split objects, `build/RMHE08/asm/auto_*_text.s`).
 * 58 of the 64 functions carry a C++ exception table entry.  The extab blocks 0x8000E79C..0x8000E96C
 * are contiguous and belong to these functions only (the previous unit's extab ends at 0x8000E79C, the
 * next block starts at 0x8000E96C), and the 58 extabindex entries 0x80029A54..0x80029D0C (58 * 0xC) are
 * contiguous and point at functions of this range only - both ranges are the run's own entry
 * boundaries, not a range end:
 *   extab       start:0x8000E79C end:0x8000E96C
 *   extabindex  start:0x80029A54 end:0x80029D0C
 *   .text       start:0x80178378 end:0x80181C88
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80178378 0x801784D0 0x80178754`, which answers only
 * `zz_XXXXXXXX_` placeholders, and by reading the probe's data pool: no `__FILE__` string).
 *
 * STATUS (official report metric, `build/RMHE08/report.json` in this worktree after a full `ninja`,
 * whose link reproduces the pinned main.dol SHA-1 BF485073...): 34 of 64 functions written, all at or
 * above the 80 % bar, **27 byte-identical**; unit measure 15.72 % fuzzy / 27 matched functions /
 * 4424 matched bytes (11.29 % of the range).
 *   100.00 fn_80178754 fn_801787E4 fn_801798EC fn_8017996C fn_80179A2C fn_80179C38 fn_80179CB4
 *   100.00 fn_80179E18 fn_8017A004 fn_8017B60C fn_8017C504 fn_8017C5DC fn_8017DB8C fn_8017DC50
 *   100.00 fn_8017DC94 fn_8017DCA8 fn_8017DE8C fn_8017E178 fn_8017E248 fn_8017EDF0 fn_8017EE94
 *   100.00 fn_8017EF38 fn_8017EFE0 fn_80179D34 fn_8017F05C fn_8017F0D8 fn_8017F138
 *    99.93 fn_80178378   97.26 fn_8017DD68   94.44 fn_8017C918   93.10 fn_8017B990
 *    91.49 fn_8017BA78   90.74 fn_801797F8   90.48 fn_8017CA38
 *
 * FOLLOW-UP QUEUE (30 functions, 32,760 B - the rest of the probe's range, named per the campaign
 * rule).  The small ones first (all <= 600 B, the same templates as the written ones): fn_801794CC (304),
 * fn_80179E98 (364, a jump-table dispatcher - the `m2cinput` output
 * carries its `jumptable_805A9574` entries, which give the case order), fn_80179AC4 (372), fn_8017D5A0
 * (472), fn_8017F1F0 (472), fn_801792F0 (476), fn_8017DF9C (476), fn_801795FC (508), fn_80178E58 (536),
 * fn_8017E2D4 (564), fn_8017B748 (584).  Then fn_8017D05C fn_80179070 fn_801784D0 fn_8017C690 fn_8017BB34
 * fn_8017A9F8 fn_8017C238 fn_8017D2BC fn_8017A0BC, and the big ones: fn_8017D778 (1,044) fn_8017E508
 * (1,088) fn_8017BDC4 (1,140) fn_8017B190 (1,148) fn_8017E948 (1,192) fn_8017ACA8 (1,256) fn_8017CB34
 * (1,320) fn_8017889C (1,468) fn_8017A3BC (1,596) fn_8017F3C8 (10,432, the band's own jump-table body).
 * fn_801784D0, fn_80179E98, fn_8017D5A0, fn_8017D778 and fn_8017DF9C are already called by written
 * bodies and are declared at the top of this file, so their object symbols are unnamed externals until
 * they are written (a relocaudit note, not a link problem: a NonMatching unit links the original object).
 *
 * RESIDUALS (measured, not guessed; every one is a same-size instruction-ordering difference, which is
 * MWCC's scheduler, not a source shape - the stopping rule in docs/matching.md applies):
 *  - fn_80178378 99.93 % (344 B, 86 ins, same size): retail restores the counter with
 *    `subi r0,r3,1; stw r0,0x20; cmpwi r0,0; bgt`, this build fuses the test into
 *    `subic. r0,r3,1` unless the source writes the comparison as `< 1`; with `< 1` the bodies are the
 *    same length and only the compare constant + branch opcode differ (`cmpwi r0,1; bge`).  Tried:
 *    `<= 0`, `== 0`, `> 0` (all fused to `subic.` + 340 B), `--x` in the condition, a `u32` field, a
 *    reload of the field in the condition, `(s32)`/unsigned spellings - the best is 99.93 %.
 *  - fn_8017DD68 97.26 % (292 B): the two argument setups of `em_approach_start(self, 0x12, lbl_80797B4C)`
 *    are emitted in the other order (`li r4` before `lfs f1`; retail loads the pool float first).
 *  - fn_8017C918 94.44 % (288 B) and fn_8017BA78 91.49 % (188 B): same class - for
 *    `em_turn_in_window(self, 0x8000, f1, f2)` retail loads both pooled floats before the `lis/addi` of the
 *    integer argument, this build the other way round.
 *  - fn_8017B990 93.10 % (232 B): both `em_turn_in_window` calls, same class (`li r4, +/-0x4000` placement).
 *  - fn_8017CA38 90.48 % (252 B, same size): the three `em_approach_start(self, 0, f1)` sites, same class.
 *  - fn_801797F8 90.74 % (target 244 B, ours 240 B): the turn-rate store from the angle's high byte -
 *    retail `srawi r0,r0,8; clrlwi r0,r0,24; stb`, this build folds it to one `extrwi r0,r0,8,16`.
 *    Measured: `(u8)((s16)angle >> 8)` reaches 244 B but trades the fold for an `extsh` (90.16 %);
 *    `(u8)((s32)angle >> 8)` (kept) is the best of them at 90.74 %.
 *
 * SHAPES FOUND BY MEASUREMENT (the templates the rest of the band uses):
 *  - The armed-state machine.  Two-case three-way dispatch on `_ENEMY_WORK::state_0x05` (`+0x05`) is
 *    `switch (self->state)` with `case 0: { u8 state = self->state; self->state =
 *    state + 1; ... break; }`; the `u8 state` copy is needed - writing `self->state += 1` gives a
 *    different (also correct) load/add/store and a lower score.  This is what makes 24 of the 30
 *    bodies byte-identical.
 *  - The countdown idiom `self->timer_0x020 -= 1; if ((s32)self->timer_0x020 < 1)` is the only spelling
 *    found that does not fuse into `subic.` (see the fn_80178378 residual).
 *  - A trailing argument-setup pair (integer immediate + pooled float) is scheduled integer-first by
 *    this build, float-first by retail - the five residuals above are all instances.
 *  - `field_0x482` (+0x482) is read only here: it selects between the two `fn_80136D4C` fade floats.
 *
 * TYPES.  `_ENEMY_WORK` lives in `include/enemy.h` (the 0xB1C union copy this unit was measured
 * against; the canonical `include/enemy/ENEMY_WORK.h` record is 0xB18 - the 4-byte difference is the
 * union copy's own `pad_0xB18[0x4]` tail and predates this unit).  This unit added ONE named field
 * inside what was `pad_0x474`: `+0x482 field_0x482`, offsets unchanged (0x474 pad_0x474[0xE], 0x482
 * the field, 0x483 pad_0x483[0x141], 0x5C4 flags_0x5C4, 0xB14 se_handle_0xB14 - compile-proved, see
 * MERGE below).  `VEC3`/`Vec3` and the nw4r globals come from `include/nw4r/math.h`, `VEC3_ctor`
 * from `ef.h`.  The declarations this unit consumes live in the owner's header (rule 2):
 * `enemy/fn_801251D0.h`, `enemy/fn_8012BDF4.h`, `enemy/fn_80176C58.h`, `enemy/fn_80177890.h`,
 * `enemy/fn_80178128.h`; the ones whose owner is still unsplit are in `include/unsplit/enemy.h` -
 * except `fn_803B9BA0`, which is declared at the top of THIS file: its address is unsplit with no
 * sound band (bracketing units `hud/fn_80324F7C.c` and `Network/NetworkWiiMediator.c`, rule 2's named
 * gap) and the two landed consumers spell it differently (`enemy/fn_80147CE0.cpp` as
 * `(_ENEMY_WORK*, void*, s32)`, `enemy/fn_801550FC.cpp` as `(_ENEMY_WORK*, u32, u32)`), so a single
 * shared-header declaration makes one of them fail to compile (MWCC: illegal function overloading).
 * This unit matches `fn_801550FC.cpp`'s spelling, the one its body was measured against.
 *
 * MERGE (this branch merged main after the sibling `enemy/fn_8015E854.cpp` landing).  `include/enemy.h`
 * conflicted on exactly one line: both sides had split `pad_0x474` at the same offset and added the
 * same `+0x482 field_0x482`, differing only in the comment, so the resolution is ONE field whose comment
 * names both consumers (`enemy/fn_80178378.cpp`'s `fn_80178378`, `enemy/fn_8015E854.cpp`'s `fn_8015EA24`)
 * - no union, no duplicate field.  Compile-proved with the real toolchain against HEAD, main and the
 * merge (build/tmp/sizeof_probe.cpp, a scratch probe): sizeof(_ENEMY_WORK)=0xB1C and the offsets
 * 0x474/0x482/0x5C4/0xB14 are IDENTICAL in all three, i.e. the merge moved no offset.  The unit's own
 * 34 bodies re-measured after the merge to exactly the pre-merge numbers (27 byte-identical, unit 15.72 %).
 */

/* Retired header of `enemy/fn_80181C88.cpp` (kept for its notes and residuals): */
/* enemy/fn_80181C88.cpp - the enemy MHchar material/step band 0x80181C88..0x80182D5C (4,308 B,
 * 21 functions).  Registration is this worker's; the extent is the probe's.
 *
 * MODULE AND NAME.  `enemy`: the unit below (`enemy/fn_80178378.cpp`) ends exactly at this range's
 * start, the unit above (`enemy/fn_80182D5C.cpp`) begins exactly at its end, and every callee out of
 * the range is an enemy-band symbol (`_ENEMY_WORK`, `em_parts_damage_level_get`, `get_em_chg_scale`,
 * `fn_80129xxx`, `fn_8013A654`).  There is no `__FILE__` string for the range: the only in-image
 * `enemy_control.cpp` literal (`0x805A1B98`) is referenced by one function inside the already-registered
 * `enemy/enemy_control.cpp` band (0x801411B8) and by none of this range's functions, so the
 * discovery queue's "one source file (enemy_control.cpp)" note is its `source_owner` walking back to
 * the latest accepted name - not evidence this range is that file (checked with
 * `python tools/splits/tudiscover.py at 0x80181C88`, which reports no source anchor, no must-link
 * anchor and only weak `.sdata2` cuts on both sides; and by reading the range's own `.sdata2` pool:
 * every `lbl_80797xxx` it loads is a bare float).  `python tools/symbols/dumpmap.py lookup
 * 0x80181C88` answers only `zz_0181c88_`, so the file keeps the map's own stem (brief section 2,
 * class 4).
 *
 * SEAM: unproven (docs/plan.md 8.3).  The probe's left cut is a `.sdata2` pool-run jump
 * (lbl_80797E68 -> lbl_80797E70) and its right cut another (lbl_80797E98 -> lbl_80797EA8); neither
 * is a TU boundary, so the range is registered at the probe's own extent, which is function-aligned
 * on both ends.  If the original object is one TU with `enemy/fn_80178378.cpp` the two splits blocks
 * are one file; the merge candidate is recorded rather than taken.
 *
 * SECTIONS (evidence: each function's own per-symbol split object, `build/RMHE08/obj/auto_*_text.o`).
 * 16 of the 21 functions carry a C++ exception frame (an 8-byte `extab` + a 12-byte `extabindex`
 * entry); the extab block 0x8000E96C..0x8000E9EC (16 entries) and the extabindex block
 * 0x80029D0C..0x80029DCC (16 entries) are contiguous and belong to these functions only (the unit
 * below's extab ends at 0x8000E96C / extabindex at 0x80029D0C, the next unit's block starts at
 * 0x8000E9EC / 0x80029DCC):
 *   extab       start:0x8000E96C end:0x8000E9EC
 *   extabindex  start:0x80029D0C end:0x80029DCC
 *   .text       start:0x80181C88 end:0x80182D5C
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x80181C88 0x80181CC0 0x80182D44`, which answers only
 * `zz_XXXXXXXX_` placeholders, and by reading the probe's own data pool: no `__FILE__` string).
 *
 * The unit's functions are the `_ENEMY_WORK` colour/material steppers and their helpers: fn_80181C88
 * is the scalar clamp they all share, fn_80181CC0/fn_80181E24 drive the MHchar TEV key colours,
 * fn_801820DC/fn_80182768/fn_80182978 are the area/action dispatchers, and fn_80182430/fn_80182C40
 * build the rotated target offsets the movement code measures.  Every `fn_XXXXXXXX` definition is
 * `extern "C"`; only the mangles reached through their real owners (MHchar members) are C++.
 *
 * STATUS (official report metric, `recompile.py --measure`, against MAIN's retired `auto_*_text.o`
 * split objects - the same original bytes the registered object will carry): all 21 functions
 * written; 7 byte-identical, 20 at or above the 80 % bar.  The one below it is fn_80182978.
 *   100.00 fn_80181C88 fn_801823A0 fn_801823C0 fn_8018257C fn_80182914 fn_80182AB8 fn_80182D44
 *    97.39 fn_80182080   95.43 fn_80182B38   94.63 fn_80181E24   93.75 fn_80182040
 *    93.13 fn_80182918   91.24 fn_801820DC   89.90 fn_801825A4   88.79 fn_80182768
 *    85.45 fn_80181CC0   84.64 fn_80182430   84.06 fn_80182320   82.88 fn_80182C40
 *    81.40 fn_80182B94   77.04 fn_80182978
 *
 * RESIDUALS (measured, not guessed):
 *  - fn_80182978 77.04 % (320 B target, 296 B ours).  The four `fn_80126278` argument builds are
 *    `clrlwi r0,r0,28; slwi r0,r0,8; clrlwi r4,r0,16` in retail (`(u16)((area_no & 0xF) << 8)`),
 *    but this compiler folds the mask into one `rlwinm r4,r0,8,20,23`.  Six source spellings were
 *    measured - `& 0xF` with `<< 8`, `* 256`, `% 16 * 256`, a `u16`/`u32` local, a `(u8)` cast and
 *    a 4-bit bitfield view of +0x1E1 - and all fold identically (best 77.04 %); the retail shape
 *    needs the un-folded intermediate the stopping rule names.  The case body's own `(s32)`
 *    comparisons (target `cmpwi`, not `cmplwi`) are kept because they measured +3.7 points.
 *  - fn_801820DC 91.24 % (580/560): the action-start chain is a shared-tail `goto` in retail
 *    (rule 8 forbids it).  The conformant shape is the nested `if` with the two `switch` selects;
 *    the remaining rows are the `cmpwi`/`cmplwi` and re-mask colouring.  Measured: nested
 *    `if`/`else if` for the two selects 75.5 %, `switch` selects (kept) 91.24 %.
 *  - The 80-90 % group is same-size argument-evaluation-order colouring (float pool load vs `li`
 *    placement) and register colouring; each is the MWCC scheduler, not a source shape.
 *
 * SHARED FILES this landing edits in its own worktree (each filed as a `shared-file` config request):
 *  - `include/enemy/ENEMY_WORK.h`: the EmColorBlock view of +0x328 (the colour scalar + K-colour
 *    bytes), `field_0x48F`, the EmPartState block at +0x740 and `field_0x81A`; every other offset is
 *    unchanged.
 *  - `include/enemy/fn_8012E968.h` (new): the owner declarations of `fn_8012EC3C`/`em_alt_mode_ck`
 *    (rule 2).  `em_alt_mode_ck` keeps the `(void)` spelling the landed bands need - a variadic
 *    or `(self)` spelling here costs `enemy/fn_801CA004.cpp` 1.1 points and `fn_8019DB9C` 6.25
 *    (measured), because those call sites leave the record in r3.
 *  - `include/unsplit/enemy.h`: re-exports that owner header instead of carrying its own copy
 *    of the two declarations (rule 2; the band is a fallback, not the owner).
 *  - `include/enemy/fn_801251D0.h`: `fn_80128AEC`/`fn_80128B80`/`fn_80129A70`/`fn_80129DB8`/
 *    `fn_8012A014`/`fn_8012A204` (owner header, rule 2).
 *  - `include/enemy/fn_80138074.h`: the `EmUserData` record `fn_8013A654` runs on.
 *  - `include/enemy/fn_8011D448.h` (new): `em_parts_damage_level_get`, declared at C++ scope
 *    (rule 9, the map name is the mangling).
 *  - `include/fn_8004CAD8.h`: `fn_8005024C`.
 *  - `include/sound/mhchar.h`: the pointer-taking `setTevKColor` overload the enemy action band's
 *    target relocations encode (`...P8_GXColor`); the by-value spelling is kept for the existing
 *    `enemy/fn_801CCBC4.cpp`/`enemy/fn_801D80EC.cpp` call sites.
 *
 * fn_80182B94 seals its four table entries with the `copyVec3` 0xC-byte copy (mh3_pad.h) rather
 * than the target's `assignVec3`: the target's reloc pairs identically in the report metric (both
 * measured 81.40 %) and declaring `assignVec3` in its owner header collides with the
 * `Vec*`-spelling locals in `ef/ef_cylinder.cpp` (a pre-existing rule-2 conflict, not this unit's).
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/fn_8012A354.h" /* fn_8012A354 (rule 2: the owner's header) */
#include "enemy/fn_8012999C.h" /* fn_8012999C (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/fn_80128030.h" /* fn_80128030 (rule 2: the owner's header) */
#include "enemy/fn_8013221C.h" /* fn_8013221C (rule 2: the owner's header) */
#include "enemy/fn_80132224.h" /* fn_80132224 (rule 2: the owner's header) */
#include "enemy/fn_80132264.h" /* fn_80132264 (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/fn_8012F810.h" /* fn_8012F810 (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/em_approach_step.h" /* em_approach_step (rule 2: the owner's header) */
#include "enemy/em_turn_seq_start.h" /* em_turn_seq_start (rule 2: the owner's header) */
#include "enemy/em_turn_seq_step.h" /* em_turn_seq_step (rule 2: the owner's header) */
#include "enemy/em_move_vec2_clr.h" /* em_move_vec2_clr (rule 2: the owner's header) */
#include "enemy/get_em_base_scale.h" /* get_em_base_scale (rule 2: the owner's header) */
#include "enemy/CancelFade.h" /* CancelFade (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "ef.h"
#include "enemy/EnemyData.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_801251D0.h" /* em_se_tbl_play_alt/fn_80128030/fn_8012933C/fn_80129668/fn_80129724 */
#include "enemy/fn_8012BDF4.h" /* em_busy_set/em_busy_ck */
#include "enemy/fn_80176C58.h" /* fn_801775C0 */
#include "enemy/fn_80177890.h" /* fn_80177BA4 */
#include "enemy/fn_80178128.h" /* fn_8017827C (the master dispatcher's case 2) */
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h" /* em_demo_pos_set/fn_8014610C */
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* fn_80126278/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* fn_8013026C */
#include "enemy/fn_8012BDF4.h" /* fn_8012E5A8 */
#include "enemy/fn_80138074.h" /* fn_8013A654/fn_8013918C + the EmUserData record */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "fn_8004CAD8.h"       /* fn_8005024C/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */
#include "stage/stg_w.h"
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)
/* fn_801823A0_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_801823A0_view1 ((void (*)(_ENEMY_WORK*, s32))fn_801823A0)
/* fn_80128A8C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80128A8C_view1 ((void (*)(_ENEMY_WORK*, u32, u32))fn_80128A8C)
/* em_turn_to_target_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_turn_to_target_view1 ((void (*)(_ENEMY_WORK*, u32))em_turn_to_target)
/* em_turn_seq_start_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_turn_seq_start_view1 ((void (*)(_ENEMY_WORK*, void*, u32, u32, u32))em_turn_seq_start)
/* em_target_pos_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_target_pos_set_view1 ((void (*)(_ENEMY_WORK*, u32))em_target_pos_set)
/* em_state_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_state_set_view1 ((void (*)(_ENEMY_WORK*, s32, s32))em_state_set)
/* em_parts_damage_level_get_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_parts_damage_level_get_view1 ((u32 (*)(_ENEMY_WORK*, u8))em_parts_damage_level_get)
/* em_mot_set_blend_view3: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_mot_set_blend_view3 ((void (*)(_ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
/* em_mot_set_blend_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_mot_set_blend_view1 ((void (*)(_ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
/* em_hit_window_set_view3: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_hit_window_set_view3 ((void (*)(_ENEMY_WORK*, s32, s32, s32))em_hit_window_set)
/* em_hit_window_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_hit_window_set_view1 ((void (*)(_ENEMY_WORK*, u32, u32, u32))em_hit_window_set)
/* em_die_ck_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_die_ck_view1 ((u32 (*)(_ENEMY_WORK*))em_die_ck)
/* em_approach_start_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_approach_start_view1 ((void (*)(_ENEMY_WORK*, f32, s32))em_approach_start)

/* The 12-byte helper `fn_80176C58` allocates and `fn_80176E50` constructs (storing the vtable
 * `lbl_805AA900` at +0).  Only the vtable slot is touched by this unit; the rest is padding.
 * size: 0xC (traced from the `operator new(0xC)` call in `fn_80176C58`). */
typedef struct Helper_80176E50 {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_80176E50;

/* The 12-byte helper's vtable (a `.data` label the split has not assigned to a unit). */
extern "C" u8 lbl_805AA900[];

/* --------------------------------------------------------------------------------------------- */
/* callees                                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* Declared in a shared header: `VEC3_ctor`, `setVector3` (`nw4r/math.h`); `em_mot_finished_ck`,
 * `em_mot_end_ck`, `em_move_mode_set` (`unsplit/enemy.h`).  The C++ free functions are declared by their
 * real signatures so the front-end mangles them to the map spellings. */
extern "C" u8 stage_map_kind_get(u8 id);
extern "C" void fn_80182978(_ENEMY_WORK* self);
extern "C" u32 quest_id_get(void);
extern "C" void fn_801823A0(_ENEMY_WORK* self, u32 a);

/* This unit's own forward declaration (fn_80176C58 calls it before its definition). */
extern "C" Helper_80176E50* fn_80176E50(Helper_80176E50* self);

/* The two mangled free functions, declared by their real signatures (rule 9: never the mangled
 * spelling - the front-end produces `em_die_ck__FP11_ENEMY_WORK` from this declaration). */

/* `em_frame_check` is a C++ mangling (`em_frame_check__FP11_ENEMY_WORKUsff`): declaring its real name
 * at global scope makes the front-end emit the map symbol, which is what docs/plan.md 6.5 rule 9 asks
 * for (never write the mangled spelling as the identifier). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

extern "C" {
/* Not registered yet, and the two bracketing registered units of their address bands name different
 * modules, so there is no sound `include/unsplit/<module>.h` to move them to (rule 2's named gap). */
void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, VEC3* v, f32 s);
void draw_shape_arm(_ENEMY_WORK* self, u32 a, u32 b);

/* `enemy`-band, not yet in include/unsplit/enemy.h (see the outbox `shared-file` request). */
}

/* `.sdata2` constants this unit reads (shared pool; unsplit band, no sound module header). */
extern f32 lbl_80797B18;
extern f32 lbl_80797B28;
extern f32 lbl_80797B2C;
extern f32 lbl_80797B30;
extern f32 lbl_80797B34;
extern f32 lbl_80797B38;
extern f32 lbl_80797B3C;
extern f32 lbl_80797B40;
extern f32 lbl_80797B44;

extern f32 lbl_80797B48;
extern f32 lbl_80797B4C;
extern f32 lbl_80797B50;
extern f32 lbl_80797B54;
extern f32 lbl_80797B58;
extern f32 lbl_80797B5C;
extern u8 lbl_8056FDD0[];

extern "C" {


/* the two neighbouring-TU entries the dispatcher tail-branches into */
void fn_80177608(_ENEMY_WORK* self);
void fn_80177774(_ENEMY_WORK* self);
}

/* ------------------------------------------------------------------------------------------------
 * Callees and pool literals owned by other units (declared by their map spelling; playbook 29).
 * ------------------------------------------------------------------------------------------------ */

/* nw4r math free functions the map carries at global scope (`...__FPQ34nw4r4math4VEC3...`). */
s32 calcVecAng2(VEC3* a, VEC3* b);
void rotVecY(VEC3* v, u32 angle);

/* Mangled enemy-service entry points (declared as C++ prototypes so the compiler mangles them). */
u16 em_get_mot_no(_ENEMY_WORK* self);
f32 get_em_chg_scale(_ENEMY_WORK* self);

extern "C" {



/* Owner: `proposal/80177890_fn_80177890` (0x80177890..0x80178128) - this unit's dispatcher targets. */
void fn_80177BEC(_ENEMY_WORK* self, s32 index);
void fn_80177CC8(_ENEMY_WORK* self);
void fn_80177D54(_ENEMY_WORK* self);
void fn_80177F30(_ENEMY_WORK* self, s32 index);
void fn_8017801C(_ENEMY_WORK* self, s32 index);

/* Pool literals owned by the data pass: the enemy action table and the aim scale constants. */
extern u32 lbl_8056FE10[];

extern f32 lbl_80797B60;
extern f32 lbl_80797B64;

/* 0x803B9BA0 - the stage-side pose request `fn_8017E178` posts: r3 `self`, r4 the pose vector's address,
 * r5 the request id.  Declared HERE rather than in a shared header on purpose: the address is unsplit with
 * no sound band (its bracketing registered units are `hud/fn_80324F7C.c` and
 * `Network/NetworkWiiMediator.c` - rule 2's named gap), and the two landed consumers spell it
 * differently (`enemy/fn_80147CE0.cpp` as `(_ENEMY_WORK*, void*, s32)`, `enemy/fn_801550FC.cpp` as
 * `(_ENEMY_WORK*, u32, u32)`), so a single header declaration makes one of them fail to compile
 * (MWCC `illegal function overloading`).  This matches fn_801550FC.cpp's spelling, the one this unit's
 * body was measured against. */
void fn_803B9BA0(_ENEMY_WORK* self, u32 v, u32 a);
/* This unit's own not-yet-written functions (same TU, so the declaration is legal here): the action
 * band's table writers call each other.  Each one is a follow-up-queue entry named in the header. */
void fn_801784D0(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80179E98(_ENEMY_WORK* self);
void fn_8017D5A0(_ENEMY_WORK* self);
void fn_8017D778(_ENEMY_WORK* self);
void fn_8017DF9C(_ENEMY_WORK* self);
void fn_8017E2D4(_ENEMY_WORK* self);
void fn_8017E508(_ENEMY_WORK* self);
void fn_8017E948(_ENEMY_WORK* self);

/* Pool literals owned by the data pass: the enemy action table and the aim scale constants. */
extern u32 lbl_8056FE50[];
extern u32 lbl_8056FE90[];
extern u8 lbl_805AA260[];
extern u8 lbl_805AA2A0[];
extern u8 lbl_805AA2D8[];
extern u8 lbl_805AA320[];
extern u8 lbl_805AA350[];
extern u8 lbl_805AA378[];

extern f32 lbl_80797BF4;

extern f32 lbl_80797BB0;
extern f32 lbl_80797BE0;
extern f32 lbl_80797BF0;
extern f32 lbl_80797CC0;
extern f32 lbl_80797BF8;
extern f32 lbl_80797BFC;
extern f32 lbl_80797CD0;
extern f32 lbl_80797C94;
extern f32 lbl_80797C34;

extern f32 lbl_80797B20;
extern f32 lbl_80797B80;
extern f32 lbl_80797B9C;
extern f32 lbl_80797C1C;
extern f32 lbl_80797C68;
extern f32 lbl_80797B88;
extern f32 lbl_80797CBC;

extern f32 lbl_80797D28;
extern f32 lbl_80797D2C;
extern f32 lbl_80797DC8;
extern f32 lbl_80797D34;
extern f32 lbl_80797D38;
extern f32 lbl_80797D50;
extern f32 lbl_80797DB8;
extern f32 lbl_80797D88;
extern f32 lbl_80797DBC;
extern f32 lbl_80797DC0;
extern f32 lbl_80797DC4;

extern f32 lbl_80797B68;
extern f32 lbl_80797B6C;
extern f32 lbl_80797B70;
}

/* One 0x16-byte action record `fn_80182AB8` builds (the +0x00 type word, a VEC3 and three scalars).
 * size: 0x16 */
struct EmWorkItem {
    /* +0x00 */ u32 type_0x00;
    /* +0x04 */ VEC3 vec_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ s16 field_0x14;
};

extern "C" {
/* `stage_map_kind_get` (the byte-table map lookup) comes from `include/unsplit/unknown.h`, which already
 * carries the band-interleaves-modules declaration. */

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions, declared up front so the later ones can call forward.
 * ------------------------------------------------------------------------------------------------ */
f32 fn_80181C88(f32 value, f32 center, f32 step);
void fn_80181CC0(_ENEMY_WORK* self);
void fn_80181E24(_ENEMY_WORK* self);
u32 fn_80182040(_ENEMY_WORK* self);
void fn_80182080(_ENEMY_WORK* self, u32 part);
u32 fn_801820DC(_ENEMY_WORK* self, u32 arg);
void fn_80182320(_ENEMY_WORK* self);
void fn_801823A0(_ENEMY_WORK* self, u32 value);
void fn_801823C0(EmUserData* self);
u32 fn_80182430(_ENEMY_WORK* self, u32 arg);
u32 fn_8018257C(_ENEMY_WORK* self);
u32 fn_801825A4(_ENEMY_WORK* self, u32 kind);
void fn_80182768(_ENEMY_WORK* self, u8* out_a, u8* out_b);
void fn_80182914(_ENEMY_WORK* self);
u32 fn_80182918(_ENEMY_WORK* self);
void fn_80182978(_ENEMY_WORK* self);
void fn_80182AB8(struct EmWorkItem* out, u32 a, s16 b, s16 c);
void* fn_80182B38(void* p, s16 arg);
void fn_80182B94(void);

/* ------------------------------------------------------------------------------------------------
 * Pool literals owned by the data pass (unresolved module -> declared, never defined; playbook 29).
 * ------------------------------------------------------------------------------------------------ */
extern f32 lbl_80797B10;
extern f32 lbl_80797B14;

extern f32 lbl_80797BB4;
extern f32 lbl_80797BE4;
extern f32 lbl_80797C14;
extern f32 lbl_80797C24;

extern f32 lbl_80797C90;
extern f32 lbl_80797D08;
extern f32 lbl_80797E6C;

extern f32 lbl_80797E78;
extern f32 lbl_80797E7C;
extern f32 lbl_80797E80;

/* The two argument labels `fn_8012A014` takes (`.data`, no module). */
extern u32 lbl_805A950C[];
extern u32 lbl_805A9518[];
/* The four 0xC-byte vectors `fn_80182B94` seeds (`.data`, no module). */
extern nw4r::math::VEC3 vec_tbl_80181C88[4];
}

#pragma peephole off

/* --------------------------------------------------------------------------------------------- */
/* functions, in address order                                                                    */
/* --------------------------------------------------------------------------------------------- */
extern "C" void fn_80176C58(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Helper_80176E50* helper;
    u8 kind;
    u8 mode;

    VEC3_ctor(&v);
    if (em_res_user_data_ck(self) == 0) {
        helper = (Helper_80176E50*)operator new(0xC);
        if (helper != 0) {
            fn_80176E50(helper);
        }
        em_res_user_data_set(self, helper);
    }
    fn_80182978(self);
    self->field_0x1E4 = 0;
    mode = arg;
    if ((u32)(mode - 1) <= 1 || mode == 4) {
        /* these modes do not run the motion hand-off */
    } else {
        kind = stage_map_kind_get(self->field_0x1E0);
        switch (kind) {
          case 1:
            if (self->area_no == 5) {
                em_move_mode_set(self, 0);
                fn_80128A8C_view1(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                fn_80128A8C_view1(self, 0, 4);
            }
            break;
          case 8:
            em_move_mode_set(self, 0);
            fn_80128A8C_view1(self, 0, 0);
            break;
          case 9:
            if (self->area_no == 0) {
                em_move_mode_set(self, 0);
                fn_80128A8C_view1(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                fn_80128A8C_view1(self, 0, 4);
            }
            break;
          default:
            em_move_mode_set(self, 2);
            fn_80128A8C_view1(self, 0, 4);
            break;
        }
    }
    if ((u16)quest_id_get() == 0x3EC) {
        self->field_0x7B0 = 0.5f;
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, -80.0f, 90.0f);
        fn_801057A4(self, 0x1A, &v, 1.5f, 0x18E4);
        fn_8010A7D4(self, 1);
    }
}

extern "C" Helper_80176E50* fn_80176E50(Helper_80176E50* self) {
    em_res_user_data_ctor(self);
    self->vtbl = lbl_805AA900;
    return self;
}

extern "C" void fn_80176E8C(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (state[0]) {
      case 2:
        switch (sub[0]) {
          case 0:
          case 3:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 4;
            }
            break;
          case 5:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 7;
            }
            break;
          case 6:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 8;
            }
            break;
          case 9:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xA;
            }
            break;
        }
        break;
      case 5:
        switch (sub[0]) {
          case 2:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xD;
            }
            break;
          case 0xA:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xE;
            }
            break;
          case 0xB:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xF;
            }
            break;
          case 0x1F:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0x20;
            }
            break;
          case 0x24:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0x25;
            }
            break;
        }
        break;
      case 7:
        switch (sub[0]) {
          case 6:
            if (fn_8012EC3C(self) == 1) {
                sub[0] = 0x17;
            }
            break;
          case 7:
          case 0xF:
            if (fn_8012EC3C(self) == 1) {
                sub[0] = 0x18;
            }
            break;
          case 0x22:
            if (em_mot_finished_ck_c1() == 1) {
                state[0] = 2;
                sub[0] = 4;
            }
            break;
        }
        break;
    }
}

extern "C" void fn_8017708C(_ENEMY_WORK* self, u32 kind, u32 sub) {
    s32 step;
    s32 limit;

    switch ((u8)kind) {
      case 1:
        switch ((u8)sub) {
          case 6:
            fn_80130F74(self);
            break;
          case 7:
            fn_801376B4(self);
            break;
        }
        break;
      case 5: {
        int s = (u8)sub;

        switch (s) {
          case 0x19:
          case 0x1A:
            fn_8012A354(self);
            break;
          case 0x1D:
            fn_80130F74(self);
            break;
          case 0x26:
            fn_801376B4(self);
            break;
        }
        break;
      }
      case 7: {
        int s = (u8)sub;

        switch (s) {
          case 6: {
            u8 v;

            step = 0x32;
            if (self->field_0x7C8 >= 0x29) {
                step = 0x64;
            }
            v = em_parts_damage_level_get_view1(self, 3);
            limit = 0xFF;
            if (v >= 3) {
                limit = 0x63;
            }
            if ((s32)self->field_0x1E4 < limit - step) {
                self->field_0x1E4 += (u8)step;
            } else {
                self->field_0x1E4 = (u8)limit;
            }
            break;
          }
          case 7:
          case 0xF:
            if (self->field_0x1E4 > 0x64) {
                self->field_0x1E4 -= 0x64;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
          case 0x24:
          case 0x25:
          case 0x2A:
          case 0x2B:
          case 0x2C:
            if (self->field_0x1E4 > 0x1E) {
                self->field_0x1E4 -= 0x1E;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
        }
        break;
      }
      case 0xA:
        switch ((u8)sub) {
          case 0xB1:
            fn_8012999C(self);
            break;
          case 0xC9:
          case 0xD1:
            em_part_hit_set(self, 0, 0);
            break;
        }
        break;
    }
}

extern "C" void fn_80177258(_ENEMY_WORK* self) {
    fn_80182978(self);
    if (em_die_ck_view1(self) == 0) {
        if (fn_8012EC3C(self) == 1) {
            if (self->field_0x1E2 == 0) {
                self->field_0x32F = 1;
                self->field_0x330 = 1;
            }
        } else {
            self->field_0x32F = 0;
        }
        if (self->field_0x38F != 0) {
            if (self->timer_0x332 < 0x384) {
                self->timer_0x332 += 1;
            }
        } else {
            self->timer_0x332 = 0;
        }
    }
    if (self->field_0x1E2 == 2) {
        self->field_0x38A = 1;
    } else {
        self->field_0x38A = 0;
    }
}

extern "C" void fn_80177314(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
      case 1:
        em_target_pos_set_view1(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801773B0(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x14, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80177430(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x1D, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801774B0(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x28, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80177540(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x36, 0x1E, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801775C0(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80177314(self);
        break;
      case 1:
        fn_801773B0(self);
        break;
      case 2:
        fn_80177430(self);
        break;
      case 4:
        fn_801774B0(self);
        break;
      case 5:
        fn_80177540(self);
        break;
    }
}

/* The map names these two with a bare `fn_XXXXXXXX` stem (no mangling), so they take C linkage to
 * emit that exact symbol; rule 9 keeps an `fn_` stem legal. */
extern "C" void fn_80177608(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        setVector3(&v, lbl_80797B18, lbl_80797B28, lbl_80797B2C);
        if (em_frame_check(self, 0, lbl_80797B30, lbl_80797B18) == 1U) {
            eft_em_spawn(self, 0, 0x18, &v, lbl_80797B34);
        }
        if (em_frame_check(self, 0, lbl_80797B38, lbl_80797B18) == 1U) {
            draw_shape_arm(self, 0x1A, 0xA);
            em_hit_window_set_view1(self, 0, 0x1C, 5);
        }
        if (em_frame_check(self, 3, lbl_80797B3C, lbl_80797B40) == 1U) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x18, &v, lbl_80797B34);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80177774(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_80797B44, 1, 0xF);
        self->timer_0x020 = self->timer_0x020 - 1;
        if ((s32)self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 5, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

#pragma peephole on

extern "C" {
/* ------------------------------------------------------------------------------------------------ *
 * the motion-state updates
 * ------------------------------------------------------------------------------------------------ */
void fn_80177890(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 0x14, 0x14, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80797B48, lbl_80797B18) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_8017791C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 7, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
}

#pragma peephole off

extern "C" {
void fn_8017799C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 0xa, 0);
        self->timer_0x020 = 0x12c;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_state_set_view1(self, 1, 6);
        }
        break;
    }
}
}

#pragma peephole on

extern "C" {
void fn_80177A2C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x82, 0xa, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177AA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xa, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177B24(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 0x1d, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The dispatcher: `state_sub` selects the active motion update.  Cases 0/1 are the neighbouring TU's
 * entries, so the switch is dense 0..7 and MWCC lowers it to the `.data` jump table the target carries. */
void fn_80177BA4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80177608(self);
        break;
    case 1:
        fn_80177774(self);
        break;
    case 2:
        fn_80177890(self);
        break;
    case 3:
        fn_8017791C(self);
        break;
    case 4:
        fn_8017799C(self);
        break;
    case 5:
        fn_80177A2C(self);
        break;
    case 6:
        fn_80177AA8(self);
        break;
    case 7:
        fn_80177B24(self);
        break;
    }
}

void fn_80177BEC(_ENEMY_WORK* self, s32 arg) {
    fn_801823A0_view1(self, 1);

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F810(self);
        em_mot_set_blend_view1(self, 0x15, 0x28, 0, 1);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, lbl_80797B18, 0);
            break;
        case 1:
            em_approach_start_view1(self, lbl_80797B4C, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177CC8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056FDD0, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FDD0) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177D54(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1b, 6, 0);
        em_hit_window_set_view3(self, 0, 0x2e, 8);
        em_hit_window_set_view3(self, 1, 0x32, 0x18);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x2f, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x30, 0x18);
            }
            break;
        case 2:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x31, 0x18);
            }
            break;
        }

        switch (self->state_0x007) {
        case 0:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                em_hit_window_set_view3(self, 1, 0x33, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                em_hit_window_set_view3(self, 1, 0x34, 0x18);
            }
            break;
        }

        if (em_frame_check(self, 1, lbl_80797B50, lbl_80797B54) == 1) {
            em_turn_to_target_view1(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177F30(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xa, 0);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, lbl_80797B18, 0);
            break;
        case 1:
            em_approach_start_view1(self, lbl_80797B18, 0);
            if (self->value_0x378 > lbl_80797B58) {
                self->value_0x378 = lbl_80797B58;
            }
            /* falls through to case 2's easing */
        case 2:
            em_approach_start_view1(self, lbl_80797B5C, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_8017801C(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 0xa, 0);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, lbl_80797B18, 0);
            break;
        case 1:
            em_approach_start_view1(self, lbl_80797B4C, 0);
            break;
        case 2:
            em_approach_start_view1(self, lbl_80797B18, 0);
            if (self->value_0x378 > lbl_80797B58) {
                self->value_0x378 = lbl_80797B58;
            }
            break;
        case 3:
            em_approach_start_view1(self, lbl_80797B5C, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
}

/* ------------------------------------------------------------------------------------------------
 * fn_80178128 - two-phase enemy action: arm the action, then aim at the target for motion ids 0x1F/0x20
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80178128(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start_view1(self, lbl_8056FE10, 0, 1, 0);
        em_move_vec2_clr(self);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            s32 ang = calcVecAng2(&self->pos, &self->vec_0x36C);
            u16 rel = (u16)(ang - self->field_0x1C0);

            self->offset_0x30C.vec_0x310.z = lbl_80797B60 * get_em_base_scale(self) * get_em_chg_scale(self);
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + rel);
        }
        break;
    }
    case 1: {
        u32 done = em_turn_seq_step(self, lbl_8056FE10);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            if (em_frame_check(self, 3, lbl_80797B50, lbl_80797B64) == 1) {
                CancelFade(self);
            }
        }

        if (done == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017827C - tail-call this action's per-sub-state handler
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017827C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80177BEC(self, 0);
        break;
    case 1:
        fn_80177CC8(self);
        break;
    case 2:
        fn_80177D54(self);
        break;
    case 3:
        fn_80177F30(self, 0);
        break;
    case 4:
        fn_8017801C(self, 0);
        break;
    case 5:
        fn_80177BEC(self, 1);
        break;
    case 6:
        fn_80177F30(self, 1);
        break;
    case 7:
        fn_8017801C(self, 1);
        break;
    case 8:
        fn_8017801C(self, 2);
        break;
    case 9:
        fn_80177F30(self, 2);
        break;
    case 10:
        fn_8017801C(self, 3);
        break;
    case 11:
        fn_80178128(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801782F8 - two-phase action: arm the timer action, then finish on `em_mot_end_ck`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801782F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend_view3(self, 0x32, 0x28, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80178378 - arm/step an enemy action, aiming at the target once the arming frame reports 1
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80178378(_ENEMY_WORK* self, u32 flag) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FE50, 0, 1, 0);
        em_move_vec_clr(self);
        if ((u8)flag == 1) {
            s32 ang = calcVecAng2(&self->pos, &self->vec_0x36C);
            setVector3(&self->offset_0x30C.vec_0x310, lbl_80797B18, lbl_80797B18, lbl_80797B68);
            rotVecY(&self->offset_0x30C.vec_0x310, ang);
            self->timer_0x020 = 0x28;
        }
        break;
    }
    case 1:
        if ((u8)flag == 1 && self->state_0x006 == 0) {
            em_move_offset_apply(self);
            self->timer_0x020 -= 1;
            if ((s32)self->timer_0x020 < 1) {
                self->state_0x006 = self->state_0x006 + 1;
            }
        }
        if (em_turn_seq_step(self, lbl_8056FE50) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80178754 - arm the 0x3A/0x0A motion, then finish on `em_mot_end_ck`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80178754(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801787E4 - the timed-table variant: same arm, then the table's own completion test
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801787E4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FE90, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_8056FE90) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801798EC - arm the 0x2E/0x14 motion, then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801798EC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017996C - three-phase: arm 0x31/0x14, step to 0x21/0, then `em_state_set(self, 5, 0x1D)`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017996C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x1D);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179A2C - reset the shared action state, arm 0x3D/0 and the (2, 7) motion pair
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80179A2C(_ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x3D, 0, 0);
        em_camera_req(self, 2, 7);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179C38 - arm the 0x68/4 motion (`em_mot_set` four-argument form), then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80179C38(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179CB4 - arm the 0x22/4 motion, then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80179CB4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x22, 4, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179E18 - arm the 0x36/0x1E motion, then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80179E18(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x1E, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017A004 - arm 0x4D/0x14 and play `em_hit_window_set_default(self, 0, 1)` while the frame counter runs
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017A004(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4D, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 1);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797BF4, lbl_80797B18) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017B60C - arm the 0x50/0xA motion and two joint-pair programs keyed on `phase_0x06`/`step_0x07`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017B60C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x50, 0xA, 0);
        em_hit_window_set(self, 0, 7, 8);
        em_hit_window_set(self, 1, 0x20, 0x18);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, lbl_80797B50, lbl_80797B18) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (self->state_0x006 == 0 && self->field_0xA0D == 0) {
            em_hit_window_set(self, 0, 0x18, 0x18);
        }
        if (self->state_0x007 == 0 && self->field_0xA69 == 0) {
            em_hit_window_set(self, 1, 0x21, 0x18);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017B990 - arm 0x52/4, then two mirrored `em_turn_in_window` poses around a 0x30 fade
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017B990(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x52, 4, 0);
        em_hit_window_set_default(self, 0, 9);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, lbl_80797B18, lbl_80797C1C) == 1) {
            em_turn_to_target(self, 0x30);
        }
        em_turn_in_window(self, lbl_80797C1C, lbl_80797B9C, -0x4000);
        em_turn_in_window(self, lbl_80797B2C, lbl_80797C68, 0x4000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017BA78 - arm 0x51/0xA and two joint programs, then `em_turn_in_window(self, 0x8000, ...)`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017BA78(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x51, 0xA, 0);
        em_hit_window_set(self, 0, 0xE, 8);
        em_hit_window_set(self, 1, 0xF, 0x10);
        break;
    }
    case 1:
        em_turn_in_window(self, lbl_80797B20, lbl_80797B80, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017C504 - arm 0x4E/0xA and two joint programs, with the fade on the first frame check
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017C504(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x10, 8);
        em_hit_window_set(self, 1, 0x11, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797B88, lbl_80797B18) == 0) {
            fn_80136D4C(self, lbl_80797B70);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017C5DC - arm the 0x58/4 motion and `em_hit_window_set_default(self, 0, 0x15)`, then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017C5DC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x58, 4, 0);
        em_hit_window_set_default(self, 0, 0x15);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, lbl_80797CBC, lbl_80797B64) == 1) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017C918 - arm 0x25/6, two `em_hit_window_set_default` cues and the 0x180/0x280 viewport pair
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017C918(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -0xA);
        em_mot_set(self, 0x25, 6, 0);
        em_hit_window_set_default(self, 0, 0x19);
        em_hit_window_set_default(self, 1, 0x1A);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, lbl_80797BB0, lbl_80797B18) == 1) {
            fn_80133CC8(self, 0x180, 0x280);
        }
        if (em_frame_check(self, 2, lbl_80797CC0, lbl_80797B18) == 1) {
            fn_80133C3C(self);
        }
        em_turn_in_window(self, lbl_80797BF8, lbl_80797BFC, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017CA38 - arm 0x59/6 and one of three 0x... pose constants chosen by the caller's selector
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017CA38(_ENEMY_WORK* self, u32 selector) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 6, 0);
        em_hit_window_set(self, 0, 0x1B, 2);
        switch ((u8)selector) {
        default:
            em_approach_start(self, lbl_80797CD0, 0);
            break;
        case 1:
            em_approach_start(self, lbl_80797B18, 0);
            break;
        case 2:
            em_approach_start(self, lbl_80797B9C, 0);
            break;
        }
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_hit_window_clear(self, 0);
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DB8C - pick the program table by `state_sub`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017DB8C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, lbl_805AA260, 0, 0);
        break;
    case 10:
        em_se_tbl_play_alt(self, lbl_805AA2A0, 2, 0xA);
        break;
    case 15:
        em_se_tbl_play_alt(self, lbl_805AA2D8, 0, 0xF);
        break;
    case 26:
        em_se_tbl_play_alt(self, lbl_805AA320, 0, 0x1A);
        break;
    case 27:
        em_se_tbl_play_alt(self, lbl_805AA350, 2, 0x1B);
        break;
    case 28:
        em_se_tbl_play_alt(self, lbl_805AA378, 0, 0x1C);
        break;
    default:
        em_se_tbl_play_alt(self, lbl_805AA260, 0, 0);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DC50 - reset the action, then re-arm the 0x37/0x14 motion with both selectors 0
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017DC50(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_801784D0(self, 0, 0);
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DC94 - run `fn_8017DC50` while the sub-state is 0
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017DC94(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_8017DC50(self);
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DCA8 - the table variant: arm the 0x80178378 action table, then switch to 0xD/1
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017DCA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FE50, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_8056FE50) == 1) {
            em_state_set(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DD68 - three-phase: 0x37/0x14, then 0x2F/0x28 once `em_approach_step` and `fn_8012F948` agree
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017DD68(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        em_approach_start(self, lbl_80797B4C, 0x12);
        break;
    }
    case 1: {
        u32 done = em_approach_step(self, 0, 0x80);

        em_mot_speed_set(self, lbl_80797C94);
        fn_80136D4C(self, lbl_80797C34);
        if (done == 1 && fn_8012F948(self) == 0) {
            self->state = self->state + 1;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    }
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 2);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017DE8C - arm 0x4E/0xA and two joint programs; the 0xD/3 switch needs `fn_80182430`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017DE8C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x29, 8);
        em_hit_window_set(self, 1, 0x2A, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797B88, lbl_80797B18) == 0) {
            fn_80136D4C(self, lbl_80797B70);
            em_turn_to_target(self, 0x80);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (fn_80182430(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
            } else {
                fn_80128030(self);
            }
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017E178 - three-phase: 0x31/0x14 plus the `fn_803B9BA0` pose request, then 0x21/0
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017E178(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        fn_803B9BA0(self, (u32)&self->pos, 0x32);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 5);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017E248 - arm 0x21/4 with the 0x3E8 timer, then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017E248(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x21, 4, 0, 1);
        fn_80130CDC(self, 0x3E8);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EDF0 - arm 0x28/0 with the two `em_demo_pos_set`/`em_demo_rot_set` pose pairs
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017EDF0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_demo_pos_set(self, lbl_80797DC4, lbl_80797D28, lbl_80797D2C);
        em_demo_rot_set(self, lbl_80797B18, lbl_80797DC8, lbl_80797B18);
        em_mot_set(self, 0x28, 0, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EE94 - arm 0x14/0, then the two pose pairs in the other order, ending on `em_action_finish`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017EE94(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, lbl_80797D34, lbl_80797B18, lbl_80797D38);
        em_demo_rot_set(self, lbl_80797B18, lbl_80797D50, lbl_80797B18);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EF38 - arm 0x28/0 and the stage-side `fn_802B1FEC`, then the two pose pairs
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017EF38(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 0, 0);
        fn_802B1FEC();
        em_demo_pos_set(self, lbl_80797DB8, lbl_80797D88, lbl_80797DBC);
        em_demo_rot_set(self, lbl_80797B18, lbl_80797DC0, lbl_80797B18);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017EFE0 - arm 0x79/4, then finish on `em_action_finish`
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017EFE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x79, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017F05C - arm 0x64/4, then finish
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017F05C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x64, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017F0D8 - the second action-table dispatcher: `state_sub` (0..13) selects a writer, and nothing
 * runs for a value outside the table (the target's `bgtlr`).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017F0D8(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8017DCA8(self);
        break;
    case 1:
        fn_8017DD68(self);
        break;
    case 2:
        fn_8017DE8C(self);
        break;
    case 3:
        fn_8017DF9C(self);
        break;
    case 4:
        fn_8017E178(self);
        break;
    case 5:
        fn_8017E248(self);
        break;
    case 6:
        fn_8017E2D4(self);
        break;
    case 7:
        fn_8017E508(self);
        break;
    case 8:
        fn_8017E948(self);
        break;
    case 9:
        fn_8017EDF0(self);
        break;
    case 10:
        fn_8017EE94(self);
        break;
    case 11:
        fn_8017EF38(self);
        break;
    case 12:
        fn_8017EFE0(self);
        break;
    case 13:
        fn_8017F05C(self);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8017F138 - the band's master selector on `_ENEMY_WORK::action_0x1E5` (+0x1E5): it hands the frame
 * to the neighbouring units' dispatchers (0, 1, 2, 5, 7) and to this unit's own ones (10..13), and runs
 * `em_action_finish` for the actions it does not own.  The trailing pair is the action's own frame end.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8017F138(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801775C0(self);
        break;
    case 1:
        fn_80177BA4(self);
        break;
    case 2:
        fn_8017827C(self);
        break;
    case 5:
        fn_80179E98(self);
        break;
    case 7:
        fn_8017D5A0(self);
        break;
    case 10:
        fn_8017D778(self);
        break;
    case 11:
        fn_8017DB8C(self);
        break;
    case 12:
        fn_8017DC94(self);
        break;
    case 13:
        fn_8017F0D8(self);
        break;
    default:
        em_action_finish(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801797F8 - arm 0x2D/6, then aim the turn rate at the target: the angle difference picks one of
 * three `step_0x07` turn rates (`0`, `0x80`, or the difference's own high byte).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801797F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        u16 angle;

        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2D, 6, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle > 0x8000) {
            if (angle > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)angle >> 8);
        }
        em_busy_set(self);
        break;
    }
    case 1:
        em_turn_in_window(self, lbl_80797BB0, lbl_80797BE0, self->state_0x007 << 8);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        } else {
            em_busy_set(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80179D34 - arm 0x3A/0xA, then re-arm 0x3A/0x18/0x5C when the frame counter reports 1
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80179D34(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797BF0, lbl_80797B18) == 1) {
            self->state = self->state + 1;
            em_hit_window_clear(self, 0);
            em_mot_set_blend(self, 0x3A, 0x18, 0x5C, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80181C88 - step a scalar toward `center` by `step`, never overshooting it.  One float on each
 * side, so it is a plain helper, not a member.
 * ------------------------------------------------------------------------------------------------ */
f32 fn_80181C88(f32 value, f32 center, f32 step) {
    if (value < center) {
        value += step;
        if (value < center) {
            return value;
        }
    } else if (value > center) {
        if (value > center + step) {
            return value - step;
        }
    }
    return center;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80181CC0 - drive the work's colour scalar and the three K-colour bytes toward the mode's
 * targets: the scalar uses the mode-independent pool float (0x1E2 == 2 picks the second), then
 * fn_80182918's damage level picks the two byte targets.
 * ------------------------------------------------------------------------------------------------ */
void fn_80181CC0(_ENEMY_WORK* self) {
    f32 target = (self->field_0x1E2 == 2) ? lbl_80797B10 : lbl_80797B9C;
    self->color_0x328.field_0x328 = fn_80181C88(self->color_0x328.field_0x328, target, lbl_80797E6C);
    u8 level = (u8)fn_80182918(self);
    f32 byte_a;
    f32 byte_b;
    if (level == 1) {
        target = lbl_80797BB4;
        byte_a = lbl_80797B20;
        byte_b = lbl_80797C24;
    } else if (level == 2) {
        target = lbl_80797B9C;
        byte_a = lbl_80797B40;
        byte_b = lbl_80797B10;
    } else {
        target = lbl_80797B18;
        byte_a = target;
        byte_b = target;
    }
    self->color_0x328.field_0x32C =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32C, target, lbl_80797C14);
    self->color_0x328.field_0x32D =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32D, byte_a, lbl_80797C14);
    self->color_0x328.field_0x32E =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32E, byte_b, lbl_80797C14);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80181E24 - the MHchar K-colour material update: rebuild the four channel colours from the
 * work's scalar and byte colours, then set the fifth from the aim/special state.
 * ------------------------------------------------------------------------------------------------ */
void fn_80181E24(_ENEMY_WORK* self) {
    _GXColor color;
    fn_80181CC0(self);
    color.r = (u8)(s32)self->color_0x328.field_0x328;
    color.g = (u8)(s32)self->color_0x328.field_0x328;
    color.b = (u8)(s32)self->color_0x328.field_0x328;
    color.a = 0xFF;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR1, &color);
    if ((self->flags_0x836 & 1) != 0) {
        if (self->field_0x48F == 1) {
            color.r = 0xFF;
            color.g = 0xFF;
            color.b = 0xFF;
        } else {
            color.r = 100;
            color.g = 100;
            color.b = 100;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    color.r = self->color_0x328.field_0x32C;
    color.g = self->color_0x328.field_0x32D;
    color.b = self->color_0x328.field_0x32E;
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (fn_8012EC3C(self) == 1) {
        color.a = 0;
    } else if (em_alt_mode_ck(self) == 1) {
        color.a = (u8)((s32)(lbl_80797BB0 * (lbl_80797B14 *
                    (lbl_80797B34 + fn_8005024C((u16)(system_w.field_0x0c << 13))))) + 225);
    } else {
        color.a = (u8)((s32)(lbl_80797BB0 * (lbl_80797B14 *
                    (lbl_80797B34 + fn_8005024C((u16)(system_w.field_0x0c << 13))))) + 135);
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, &color);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182040 - true for the dead mode (0x1E2 == 2) while the record has not latched its aim state.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_80182040(_ENEMY_WORK* self) {
    if (self->field_0x1E2 == 2 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182080 - clamp the 0x1E4 counter: part 3 at full damage caps it at 99.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182080(_ENEMY_WORK* self, u32 part) {
    if ((part & 0xFF) == 3) {
        if ((u8)em_parts_damage_level_get(self, 3) == 3) {
            if (self->field_0x1E4 >= 100) {
                self->field_0x1E4 = 99;
            }
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801820DC - the area/action start predicate: true when the record's map/area state arms the
 * next action, either by setting the 0x1FC/0x1FE/0x1FF request or by running the 0x8012A014 test.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_801820DC(_ENEMY_WORK* self, u32 arg) {
    u8 mode = (u8)stage_map_kind_get(self->field_0x1E0);
    if (mode != 1 && mode != 3) {
        return 0;
    }
    u32 armed = 0;
    u32 probe;
    switch (mode) {
    case 1:
        probe = 8;
        break;
    case 3:
        probe = 6;
        break;
    default:
        probe = 0xFF;
        break;
    }
    if (probe != 0xFF) {
        u8 state = (u8)fn_80129DB8(self);
        switch (state) {
        case 1:
            armed = 1;
            break;
        case 2:
            return 1;
        default:
            break;
        }
    }
    if (armed == 0) {
        if (self->field_0x1FC == 1 && self->field_0x1FE == 8) {
            return 1;
        }
        if (self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1) {
            if (fn_8012EC3C(self) == 1 && self->color_0x328.field_0x32F == 0) {
                if (mode == 1) {
                    if ((s32)self->area_no == 7) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 12;
                        return 1;
                    }
                } else if (mode == 3) {
                    if ((s32)self->area_no == 4 || (s32)self->area_no == 8) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 3;
                        return 1;
                    }
                }
            } else if (self->color_0x328.field_0x330 != 0) {
                u32 sel;
                switch (mode) {
                case 1:
                    sel = 8;
                    break;
                case 3:
                    sel = 3;
                    break;
                default:
                    sel = 0xFF;
                    break;
                }
                if (fn_8012A014(self, 23, sel, (u16)arg, lbl_805A950C, lbl_805A9518) == 1) {
                    return 1;
                }
            }
        }
    }
    if (fn_80129A70(self, (u16)arg) == 1) {
        return 1;
    }
    return fn_8012A204(self) == 1;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182320 - the area's follow-up arming: when the action has run out (not action 10, 0x81A <= 0)
 * pick the two motion ids the 0x1E2 mode names.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182320(_ENEMY_WORK* self) {
    fn_80128B80(self);
    if (self->action != 10 && self->field_0x81A <= 0) {
        if (self->field_0x1E2 == 0) {
            fn_80128AEC(self, 13, 12);
        } else if (self->field_0x1E2 == 2) {
            fn_80128AEC(self, 13, 13);
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801823A0 - set the part flag from a one-value selector.
 * ------------------------------------------------------------------------------------------------ */
void fn_801823A0(_ENEMY_WORK* self, u32 value) {
    if (value == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801823C0 - reset the work's part state after the user-data teardown.
 * ------------------------------------------------------------------------------------------------ */
void fn_801823C0(EmUserData* self) {
    fn_8013A654((_ENEMY_WORK*)self, 5);
    self->work_0x04->part_0x740.field_0x740 = 1;
    self->work_0x04->part_0x740.field_0x742 = 0;
    self->work_0x04->part_0x740.field_0x744 = 0;
    self->work_0x04->part_0x740.field_0x746 = 0;
    self->work_0x04->part_0x740.field_0x748 = 0;
    self->work_0x04->part_0x740.field_0x74A = 0;
    self->work_0x04->part_0x740.field_0x74C = 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182430 - "is the work within the aimed target's attack range": builds the target's rotated
 * offset from the work position, adds it to the candidate record's position and compares the squared
 * XZ distance against the scaled range.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_80182430(_ENEMY_WORK* self, u32 arg) {
    VEC3 a;
    VEC3 b;
    VEC3 rel;
    VEC3 probe;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    _ENEMY_WORK* target = fn_80131034(self, 23, 0);
    if (target == 0) {
        return 0;
    }
    if (fn_8012E5A8(target) != 1) {
        return 0;
    }
    if ((arg & 0xFF) == 0) {
        return 1;
    }
    setVec3(&rel, lbl_80797B18, lbl_80797B18, lbl_80797D08 * get_em_chg_scale(self));
    copyVec3(&b, &rel);
    rotVecY(&b, self->field_0x1C0);
    addVec3(&probe, &self->pos, &b);
    copyVec3(&a, &probe);
    f32 dist = calcDistanceSqXZ(&a, &target->pos);
    f32 range = lbl_80797BE4 * get_em_chg_scale(self);
    if (dist >= range * (lbl_80797BE4 * get_em_chg_scale(self))) {
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018257C - the "action 13, sub-state <= 5" predicate.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_8018257C(_ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_801825A4 - the per-kind aim/approach query the action band dispatches over.
 * ------------------------------------------------------------------------------------------------ */
u32 fn_801825A4(_ENEMY_WORK* self, u32 kind) {
    switch (kind & 0xFF) {
    case 0: {
        f32 delta = self->vec_0x36C.y - self->pos.y;
        if (delta >= lbl_80797B58) {
            return 2;
        }
        if (delta >= lbl_80797B2C) {
            return 1;
        }
        if (delta <= lbl_80797B5C) {
            return 4;
        }
        if (delta <= lbl_80797C90) {
            return 3;
        }
        return 0;
    }
    case 1:
        return fn_80182918(self);
    case 2:
        return self->color_0x328.field_0x32F;
    case 3: {
        _ENEMY_WORK* target = fn_80131034(self, 23, 0);
        if (target != 0) {
            return fn_8012E5A8(target) == 1;
        }
        return 0;
    }
    case 4:
        return self->color_0x328.field_0x332 < 900;
    case 5: {
        f32 scale = get_em_chg_scale(self);
        f32 limit = self->field_0x210 -
                    (lbl_80797B9C + fn_8013026C(self)) * scale;
        return self->pos.y < limit;
    }
    case 6:
        return self->color_0x328.field_0x330 != 0;
    case 7:
        if ((u8)em_parts_damage_level_get(self, 3) >= 3) {
            return 0;
        }
        return fn_80182918(self) != 0;
    default:
        return 0;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182768 - area/action -> motion pair: fills the two out-bytes and arms the motion ids the
 * map/area state names.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182768(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if (self->area_no == 7) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 14, 15, lbl_80797B18);
        } else if (self->area_no == 12) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 23, 24, lbl_80797B18);
        }
        break;
    case 3:
        if (self->area_no == 4) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 6, 7, lbl_80797B18);
        } else if (self->area_no == 8) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 6, 0, lbl_80797B18);
        }
        break;
    case 9:
    case 11:
        if (self->area_no == 1) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 0, 1, lbl_80797B18);
        }
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182914 - thin tail-call wrapper for fn_80182978.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182914(_ENEMY_WORK* self) {
    fn_80182978(self);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182918 - the 0x1E4 damage counter's level: 0 below 1, 1 below 100, and past that the part-3
 * damage level maps to 5, 4, 3, ... (retail's branchless `subfc`/`adde` run).
 * ------------------------------------------------------------------------------------------------ */
u32 fn_80182918(_ENEMY_WORK* self) {
    u8 health = self->field_0x1E4;
    if (health < 1) {
        return 0;
    }
    if (health < 100) {
        return 1;
    }
    u8 part = (u8)em_parts_damage_level_get(self, 3);
    return 5 - part + (part >= 3 ? -1 : 0);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182978 - the map/area aim/rotation setter: clears the aim vector then arms the rotation id
 * the map/area state names.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182978(_ENEMY_WORK* self) {
    f32 zero = lbl_80797B18;
    self->aim.x = zero;
    self->aim.y = zero;
    self->aim.z = zero;
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((s32)self->area_no == 7 || (s32)self->area_no == 12) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256), &self->aim);
        }
        break;
    case 3:
        if ((s32)self->area_no == 3 || (s32)self->area_no == 6) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 8), &self->aim);
        } else if ((s32)self->area_no == 4) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 3), &self->aim);
        } else if ((s32)self->area_no == 8) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 4), &self->aim);
        }
        break;
    case 9:
    case 11:
        if ((s32)self->area_no == 1) {
            f32 v = lbl_80797B18;
            self->aim.x = v;
            self->aim.y = lbl_80797E78;
            self->aim.z = v;
        }
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182AB8 - build one 0x16-byte action record: the type, a fixed vector and the three scalars.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182AB8(struct EmWorkItem* out, u32 a, s16 b, s16 c) {
    VEC3 vec;
    setVec3(&vec, lbl_80797B18, lbl_80797E7C, lbl_80797B64);
    out->type_0x00 = 0x1A;
    copyVec3(&out->vec_0x04, &vec);
    out->field_0x10 = (u8)a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182B38 - the record's release step: unregister, then free when the caller asks (arg > 0);
 * returns the record so a caller can chain.
 * ------------------------------------------------------------------------------------------------ */
void* fn_80182B38(void* p, s16 arg) {
    if (p != 0) {
        fn_8013918C((_ENEMY_WORK*)p, 0);
        if (arg > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182B94 - install the four static vectors of the shared 0x806A79D0 table.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182B94(void) {
    VEC3 a;
    VEC3 b;
    VEC3 c;
    VEC3 d;
    setVec3(&a, lbl_80797B18, lbl_80797B18, lbl_80797E80);
    copyVec3(&vec_tbl_80181C88[0], &a);
    setVec3(&b, lbl_80797B18, lbl_80797B18, lbl_80797B18);
    copyVec3(&vec_tbl_80181C88[1], &b);
    setVec3(&c, lbl_80797C34, lbl_80797B18, lbl_80797B18);
    copyVec3(&vec_tbl_80181C88[2], &c);
    setVec3(&d, lbl_80797B18, lbl_80797B18, lbl_80797B18);
    copyVec3(&vec_tbl_80181C88[3], &d);
}

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A79D0..0x806A7A00`): the four-vector table its static
 * constructor `fn_80182B94` fills (the map's second row at +0x18 folded into it; `.data` tables point at both
 * halves).  Name is a GUESS. */
VEC3 vec_tbl_80181C88[4];  /* +0x806A79D0 */
