/* Pl/pl_act_step.cpp - the player action step set
 *
 * `.text` 0x802430E8..0x802673A4, 104 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): fold of 6 registered units, built from `Pl/fn_802430E8.cpp`, `Pl/fn_802489D4.cpp`, `Pl/pl_act_step.cpp`, `Pl/fn_80258FCC.cpp`, `Pl/fn_8025F088.cpp`, `Pl/fn_80262940.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 *
 * Kept views: the retired sources declared 2 callee(s) with different signatures (`fn_8025E298`, `fn_802DE578`); each function keeps its own source's view through a function-pointer cast macro (`<name>_viewN`, `<name>_cN`), which compiles to the same direct call, so the fold does not move any body.
 * Hidden declarations: 3 header declaration(s) that disagree with the kept view are renamed away around their `#include` (`#define <name> <name>_hidden_<header>`): `fn_8025E298`, `get_move_work_adrs`, `get_move_work_max`.
 */

/* Retired header of `Pl/fn_802430E8.cpp` (kept for its notes and residuals): */
/*
 * Player motion -> SE (sound-effect) frame dispatch for `_PLW`, part two: the motions the sibling
 * dispatcher `Pl/fn_80241558.cpp` does not arm.
 *
 * The whole `.text` range 0x802430E8-0x802489D4 is 30 functions.  The first one, `fn_802430E8`
 * (0x1DA0 B), is the sibling of `Pl/fn_80241558.cpp`'s single function: it reads the current motion
 * number (`Get_motion_no(_PLW*)`, 0x8026A308), switches on it, and arms the fixed set of frame-timed
 * sound requests on the SE work objects hanging off the player work (`_PLW`+0xAF4 / +0xAF8 / +0xAFC).
 * MWCC turns its 261-case switch into the 261-entry `.data` jump table `jumptable_805C4134`
 * (0x805C4134-0x805C4548), which this unit owns.  52 of its 59 arms are instruction-for-instruction
 * identical to `fn_80241558`'s; the 7 that differ are motions 0x3F4/0x3F5 (different bodies here)
 * plus 0x3F6/0x410/0x43D/0x46C/0x49E, which the sibling sends to its default.
 *
 * Final home: module `Pl`, file stem kept as the map's `fn_802430E8` - the map has only
 * `fn_XXXXXXXX` for the range and no `__FILE__` string covers it (the `.data` pool between
 * `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at 0x805CDFC8 carries no source name for the
 * band; the one `enemy_control` string the proposal's seam note named is at 0x805A1BB8, 0x2A000
 * bytes below this range and referenced by nothing in it).  The subsystem is the player: the first
 * argument is passed straight to `Get_motion_no`, whose map spelling is `Get_motion_no__FP4_PLW`,
 * and the three SE works are the `_se_w` pointers at `_PLW`+0xAF4/+0xAF8/+0xAFC the sibling unit
 * uses.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `grep -n "^fn_8024" config/RMHE08/symbols.txt` - every entry in the range is a bare `fn_<addr>`
 * stem, and `dumpmap.py lookup <addr>` answers only `zz_<addr>_` for all 30).
 *
 * Language: C++. The range's calls carry the mangled `Get_motion_no__FP4_PLW`,
 * `se_req_frame_set__FP5_se_wllll` and `SE_Code_Make__Flsls`, and the retail object has an
 * extab/extabindex pair (58 unwind-only records) - which is why the Pl lib sets -Cpp_exceptions on.
 *
 * Flags: none beyond the lib's `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * mw_version Wii/1.0).
 *
 * Residual: 9 of the range's 30 functions are written and all nine measure 100.0 %
 * (`fn_802430E8`, `pl_act_clear_wait`, `fn_80245E20`, `fn_80246158`, `fn_802466C4`, `fn_802478A4`,
 * `fn_80247C2C`, `fn_80247EF0`, `fn_802488D4`).  The other 21 - `fn_80244E88` (0xE68 B, the
 * per-motion effect dispatcher `Pl/fn_80229ECC.cpp` calls with `&_PLW::field_0xAF4`) and
 * `fn_80245CF0`/`fn_80245E6C`/`fn_80245F40`/`fn_80245FD8`/`fn_802461F0`/`pl_hit_effect_spawn`/`fn_80246654`/
 * `fn_8024676C`/`fn_80246AB4`/`fn_80246B7C`/`fn_80246D7C`/`fn_802470E4`/`fn_80247534`/`fn_802476A0`/
 * `fn_8024794C`/`fn_80247CC4`/`fn_80247D74`/`fn_80248018`/`fn_802482C4`/`fn_802485C8` - are declared
 * here only as far as a written caller needs them, and build as undefined relocations; the inventory
 * is `config/RMHE08/symbols.txt`.  Two of them are half-blocked on a header clash rather than on
 * codegen: `fn_80246654` needs `get_move_work_adrs` from `include/enemy/fn_80165FC8.h`, which cannot
 * be included beside `sound/se.h` (both declare `fn_800532DC`/`setVec3` with different types), and
 * `fn_80247CC4` needs `VEC3_ctor`/`fn_8012A624` and a full `VEC3` local through `include/ef.h`.
 */

/* Retired header of `Pl/fn_802489D4.cpp` (kept for its notes and residuals): */
/*
 * The player "action/handler" cluster of the `Pl` module: `.text` 0x802489D4-0x8024F200 (42
 * functions, 0x6D2C B), with its own exception tables - extab 0x80011DB4-0x80011EFC (41 records)
 * and extabindex 0x8002EB78-0x8002ED64 (41 records).  Both runs are bracketed exactly by the
 * neighbouring functions: `fn_802488D4`'s extab record ends at 0x80011DB4 and `pl_act_step_offhand_gesture`'s
 * starts at 0x80011EFC, so the extent is pinned by the section runs, not by a byte cap.
 *
 * WHAT IT IS.  The band drives a `_PLW` (player work) actor through its act/step state machine and
 * its per-act handlers: every function takes `_PLW*` in r3 and the unit's outbound calls are the Pl
 * gates (`Pl_master_ck`, `Pl_Skill_ck`, `Pl_act_ck`, `Pl_cat_skill_ck`, `Pl_frame_check`,
 * `Pl_chr_setX`, `PlayMode_ck`) plus the 0x8026xxxx/0x8027xxxx Pl helpers.  Four of the 42 are
 * small `switch`-on-an-id classifiers shared by the rest (`fn_8024AA04`, `fn_8024CD8C`,
 * `pl_act_net_hook_c`, `fn_8024D3C8`), and `pl_act_net_hook_main` (0x15E0 B) is the band's main dispatcher.
 *
 * MODULE AND NAME (brief section 2, evidence order).
 *   - Class 1 (`__FILE__` string): out.  The `.data` source-name pool has no name for this band
 *     between `enemy_control.cpp` @0x805A1BB8 and `menu_item.cpp` @0x805CDFC8 (the sibling Pl units
 *     `Pl/fn_80241558.cpp` and `Pl/fn_80262940.cpp` checked the same pool), and no string in the
 *     range's own data references is a bare source-file name.
 *   - Class 2 (runtime-dump name): out.  `dumpmap.py lookup` gives the `zz_XXXXXXXX_` placeholder
 *     for every address in the range, and the map has only `fn_XXXXXXXX`.
 *   - Class 3 (module): `Pl`.  Every sibling unit is `Pl/*.cpp`, the actor is `_PLW`, and the gates
 *     this band calls are the Pl ones listed above.
 *   - Class 4 (file stem): the map's `fn_802489D4`, the sibling class-4 pattern of
 *     `Pl/fn_80229ECC.cpp` / `Pl/fn_80241558.cpp` / `Pl/fn_80262940.cpp` / `Pl/fn_80288CEC.cpp`.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802489D4 0x8024F200` - all 42 entries are the map's
 * `fn_XXXXXXXX` placeholders, and `python tools/symbols/dumpmap.py lookup` resolves every one of
 * them to a `zz_XXXXXXXX_` dump placeholder).
 *
 * Seam: unproven.  The range is one maximal unclaimed run cut at `attribute.py`'s `--max-bytes`
 * cap; discovery's "one source file (enemy_control.cpp)" warning is an `owner_merge` artefact (the
 * name it names is 0x10xxxx away and only "owns" the range because `source_owner` takes the latest
 * name at or before the index).  The section runs above are what actually pin the extent.
 *
 * Language: C++.  The map's undefined set carries eleven mangled names
 * (`Pl_Skill_ck__FP4_PLWUs`, `Pl_act_ck__FP4_PLWUcUs`, `Pl_chr_setX__FP4_PLWUsll`,
 * `Pl_frame_check__FP4_PLWUlff`, `rotVecXYZ__FPQ34nw4r4math4VEC3P10_CP_VECTOR`, ...), and every Pl
 * object carries an extab/extabindex pair, which is why the Pl lib sets `-Cpp_exceptions on`.
 *
 * Flags: the lib's `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * mw_version Wii/1.0), the Pl lib's measured flag set.
 *
 * Type work: the `_PLW` offsets this band touches that `include/pl.h` still spelled `unkNN` or hid
 * inside a `pad_` run were given names there in this batch (+0x018, +0x0AC, +0x37A, +0x5B8,
 * +0x5BA, +0x656, +0x657).  Where the context supports a meaning the field got one; where it only
 * fixes the offset the field follows the header's existing `field_0xNN` convention.
 *
 * Residual: work in progress - see `.pi/notes/802489d4-fn-802489d4-7ce7.md`.
 */

/* Retired header of `Pl/pl_act_step.cpp` (kept for its notes and residuals): */
/*
 * Pl/pl_act_step.cpp - the player's per-act state machine cluster.
 *
 * Proposal `8024F200_fn_8024F200`, registered once at its final home (`docs/plan.md` 12).  `.text`
 * 0x8024F200-0x80258FCC (40396 B, 92 functions): every function takes the player work
 * (`_PLW*`, passed straight to `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/`Get_motion_no` and
 * to the `Pl/pl_act.cpp` motion helpers) plus the per-part index, reads the act step byte
 * `_PLW+0x005`, and drives one step of that act.  The unit's own dispatcher is its 0x80251B88
 * (1696 B, unread): it indexes `jumptable_805C4AE0` with `_PLW+0x00C` and tail-calls one handler per
 * act id, which is where this range's functions are reached from.  The whole range is worked as one
 * unit: the discovery pass cut it on its `--max-bytes` cap, and nothing inside the range names a
 * file (the `.data` pool between `enemy_control.cpp` at 0x805A1BB8 and `menu_item.cpp` at
 * 0x805CDFC8 carries no `__FILE__` string for the whole Pl band), so no seam can be shown from the
 * range itself.
 *
 * Home (evidence order): 1. no `__FILE__` string anywhere in the band (see above); 2. no
 * runtime-dump name - `dumpmap.py lookup 0x8024F200` answers only the `zz_024f200_` placeholder;
 * 3. class 3 - the first argument is the `_PLW` every `Pl_*` predicate in this lib takes, and the
 * callees are `Pl_Skill_ck`/`Pl_cat_skill_ck`/`Pl_frame_check`/`Get_motion_no`, so the module is
 * `Pl`; 4. class 4 for the stem - the map carries only `fn_XXXXXXXX` for the range and the dump
 * answers `zz_`, so the name is the one derived below and is a **GUESS**.
 *
 * Name (GUESS, class 4).  `pl_act_step` says what the range is: the per-act step handlers the act
 * dispatcher tail-calls, each advancing one step of the act `_PLW+0x00C` names through
 * `_PLW+0x005`.  The naming scheme is the module's own (`pl_act.cpp`, `pl_skill.cpp`) with the
 * `_step_` segment naming the per-act entry each body switches on.
 *
 * Names this unit defines (all GUESSes, class 4, derived from each body - the map has no name for
 * any of them and `dumpmap.py` answers `zz_024f200_` for the whole range):
 *   * `pl_act_step_offhand_gesture` (0x8024F200) - drives the act entered from an off-hand gesture
 *     (its own body picks the skill tier out of `+0x0B6` and the `Pl_cat_skill_ck(self, 50)` gate);
 *   * `pl_act_step_pitfall_arm` (0x8024F8A8) - arms the pitfall act's three motion words from the
 *     work record's frame data, then hands the act to `fn_802F39DC` when its step 2 arrives;
 *   * `pl_act_step_pitfall_hold` (0x8024F9C4) - the pitfall act's follow-up: it hands the act back
 *     to the action the three armed words select and stops on the frame window they name;
 *   * `pl_act_armed_motion_count` (0x8024FB20) - counts how many of the three motion words agree
 *     (1, 2, or 3);
 *   * `pl_act_guard_timer_reset` (0x80257E70) - zeroes the two running guard timers and re-arms the
 *     third at 30 frames.
 *   * round 2, all class-4 GUESSes: `pl_act_step_84` (0x802504BC) - act 84, the only act id the
 *     dispatcher table gives that address; `pl_act_guard_gauge_adjust` (0x80252AEC) - steps
 *     `_PLW+0x0AC` (the guard gauge `pl_act_step_89` also clears) by the caller's two part flags and
 *     clamps it; `pl_act_arm_motion_and_flag` (0x802552F0) - arms a motion plus one of two flags and
 *     the actor mode, shared by the eleven weapon-act handlers above it; `pl_act_charge_repeat_step`
 *     (0x802574E4) - runs one repeat of the caller's six-part charge motion; `pl_act_countdown_step`
 *     (0x80257DA4) - arms attribute 26 with a 40/80-frame countdown; `pl_act_step_attr_1007` /
 *     `_1056` / `_1018` (0x8025553C / 0x80255E08 / 0x802562EC) and `pl_act_step_attr_112`
 *     (0x80253FF8) - the three-step weapon-act handlers, named from the attribute their arming step
 *     passes `Pl_chr_set_attr_default` (they are not in the act dispatch table; `fn_802564B0`'s
 *     `jumptable_805C4EB0` is what reaches the first three).
 *
 * Names this unit renamed in other units (the band's core API; every one is a **GUESS** derived from
 * the callee's own body, and the owner's header carries the declaration - rule 2):
 *   * `Pl_chr_set_attr_default` (0x8026A224, `Pl/fn_802693C4.cpp`) - `clrlwi r4,r4,16; li r7,0; b
 *     Pl_chr_set_attr`, i.e. the 5-argument attribute setter with its last argument 0;
 *   * `Pl_motion_end_ck` (0x8026A33C, `Pl/fn_802693C4.cpp`) - `fn_800E2198(&models_0x004[0], 0)`, the
 *     model layer's "the current motion finished" predicate its callers test `== 1` before arming
 *     the next one;
 *   * `Pl_act_set_motion` (0x80275B04, `Pl/fn_80273B14.cpp`) - arms the act's three status bits from
 *     a packed mask (1/2/3 through `fn_80275AFC`, then the two bit tests);
 *   * `Pl_act_set_motion_slot` (0x802761B8, `Pl/fn_80273B14.cpp`) - `li r7,0; li r8,0; b fn_80275C34`,
 *     the same hand-off with the two trailing arguments 0;
 *   * `Pl_act_set_step_table` (0x802770E8, `Pl/pl_act.cpp`) - installs the per-act `.data` record at
 *     `+0x318` and clears the `+0x313`/`+0x322` state it drives.
 * The four declarations those names had in `include/hud/net_char_sync.h` moved to their owner's header
 * in the same change (the hud unit includes it), and `include/unsplit/Pl.h`'s copies are gone.
 *
 * Language: C++ - the map's undefined set carries real manglings (`Pl_Skill_ck__FP4_PLWUs`,
 * `Pl_cat_skill_ck__FP4_PLWUs`, `Pl_frame_check__FP4_PLWUlff`, `GetGroundHit2__FPQ34nw4r4math4VEC3UlUcPUc`)
 * and the band's objects carry extab/extabindex (the Pl lib's `-Cpp_exceptions on`).
 *
 * Flags: `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`, mw_version Wii/1.0),
 * the set the sibling Pl units measure with.  The per-part argument is `s32`, not `u8`: retail
 * compares it with `cmpwi r4,0` and never masks it, which a `u8` declaration cannot produce.
 *
 * Residual (20 of the range's 92 functions are written; the other 72 have no body yet).  Measured
 * with `unitscore.py Pl/pl_act_step`: unchanged by the naming pass (a rename moves an objdiff score
 * only through a relocation name).
 *   * `pl_act_step_offhand_gesture` 99.91 - `.text` (260 insns), `extab` and `extabindex` are byte-identical; the
 *     only gap is a *name*: the target's one `R_PPC_ADDR32` in extabindex points at dtk's
 *     `@etb_80011EFC` where MWCC writes its own anonymous `@283` local (the same class of residual
 *     `Pl/fn_80230FBC.cpp` records for its jump tables).  Nothing in the source reaches it.
 *   * `pl_act_step_pitfall_arm` 98.59 - one extra `clrlwi r0,r0,24` on the case-0 store.  The switch needs a
 *     `u8` local for retail's 3-instruction range test (`addi r0,r3,-1; cmplwi r0,1; ble`); the
 *     `u8` local's own conversion is what the `clrlwi` is, and both alternatives lose the range test
 *     instead (retail's 2-instruction store with the field written `++` in place of the local, or a
 *     `(u8)` cast on the switch operand, both measure 95.35).
 *   * `pl_act_step_pitfall_hold` 97.70 - 8 B short: MWCC removes the loop's counter (`li r4,0` / `addi r4,r4,1`
 *     are absent, and the address becomes `lhz r0,1436(r3)` off retail's own base) where retail
 *     keeps it.  The shape that keeps the counter (an explicit `u16*` plus `i`, `i++, word++`)
 *     changes the base to `addi r3,r31,1436` and measures 97.00, so the first shape is kept.
 *   * `pl_act_armed_motion_count` 100.00 and `pl_act_guard_timer_reset` 100.00.
 *   * the seven bodies added with the naming pass are all at 100.00 (`pl_act_step_86` 184 B,
 *     `pl_act_step_89` 280 B, `pl_act_step_94` 212 B, `pl_act_step_175` 200 B, `pl_act_step_151`
 *     168 B, `pl_act_step_timer_wait` 172 B, `pl_act_gauge_gate_by_skill` 84 B).
 *   * the eleven further bodies added in round 2 are all at 100.00: `pl_act_step_84` (272 B, act 84),
 *     `pl_act_step_attr_112` (308 B), `pl_act_guard_gauge_adjust` (296 B), `pl_act_arm_motion_and_flag`
 *     (152 B), `pl_act_charge_repeat_step` (596 B), `pl_act_countdown_step` (204 B), `pl_act_step_attr_1007`
 *     (144 B), `pl_act_step_attr_1056` (144 B), `pl_act_step_attr_1018` (144 B), plus the two the
 *     previous pass wrote and held back for want of a pool word - `pl_act_step_121` (348 B, act 121)
 *     and `pl_act_step_135` (396 B, acts 135/137).
 *   * **the Pl band's shared `.sdata2` pool now has a named owner**, which is what unblocked every
 *     body in round 2.  Rule 12 counts findings per (rule, file) and all eight of this file's pool
 *     `extern`s resolved to `unsplit`, so its count was 8 and **any** new pool declaration was
 *     refused by construction.  Route taken: the run `0x80799E00..0x80799F98` (408 B, the exact
 *     extent of the map's `pl_*` `.sdata2` rows) is claimed by the **data-only unit
 *     `Pl/pl_frame_data.cpp`**, and this unit reads the words through its header
 *     (`Pl/pl_frame_data.h`, included above) instead of declaring them locally.  That is the right
 *     shape rather than this unit holding the claim itself: the run is the MWLD *merge* of several
 *     Pl objects' own pools (`Pl/fn_802430E8.cpp`, `Pl/fn_802489D4.cpp`, `Pl/fn_80258FCC.cpp`,
 *     `Pl/fn_8025F088.cpp` and this unit all read the same addresses - `callers.py 0x80799E40`
 *     answers 7 sites in 5 functions across 3 objects), so no single consumer can own or emit it,
 *     and the owner unit gives every one of those ~20 consumers somewhere to declare into.
 *     Measured, both when this unit held the claim and when it moved to the owner unit: **every row
 *     of the whole-project report is unchanged** (the 408 B of `total_data` simply changes unit; the
 *     only new report rows are the owner unit and the tail auto unit `auto_11_80799F98_sdata2` the
 *     re-split creates) and `ninja build/RMHE08/ok` stayed green.  Residual: the pool is *declared*,
 *     never defined (playbook 29 - and the owner unit's source intentionally defines nothing), so
 *     `datagap.py --unit Pl/pl_frame_data` reports `.sdata2 target-extra 408 B`: the bytes are the
 *     original's and a `NonMatching` unit contributes exactly those to the link.
 *   * `include/pl.h` gained **named union members only** (`field_0x406/408/40A/40C/410/422`,
 *     `field_0x42E/430/432/434/436`) for the timer run `pl_act_step_84` clears; headers carry rules
 *     2/12 only, so no rule 5 finding is created, the `_PLW` layout is unchanged and no consumer's
 *     codegen moves (the whole-project report was diffed row by row: 0 changes).
 *   * **one further body is written and measured but still held back**: `pl_act_step_arm_table`
 *     (176 B, 100.00 % measured) needs the `.data` step record `pl_act_step_table_247`
 *     (`.data:0x805BEBEC`), which is unowned, unclaimed and *outside* the Pl pool run the owner unit
 *     holds - declaring it here is a new rule-12 finding, and this batch has already spent the
 *     headroom a claim gives.  Its source is in `.pi/notes/pl-step2-ed81.md`; claiming the `.data`
 *     run it lives in is a separate measured change (filed as a `range` request in this unit's
 *     outbox).
 *   * the remaining `.data` labels the range reads (`lbl_805BDC48`, `lbl_805BDF78`, `lbl_805BEE14`,
 *     `lbl_805C4AD0`, the three `jumptable_805C4Axx`/`805C4DC4`/`805C4EB0` and
 *     `pl_act_step_table_247`) are unowned and unclaimed, which is what blocks the handlers that use
 *     them (74 rows still open).
 *   * `pl_act_charge_repeat_step`'s `part <= 2` test is `(u32)part <= 2`: the parameter stays `s32`
 *     (retail's inner 6-arm `switch` uses `cmpwi`, which a `u32` parameter turns into `cmplwi` and
 *     costs 2 points), but the range test itself is unsigned in retail (`cmplwi r30,2`).
 *   * a **new `fn_` name is still a rule-7 finding**, so every remaining body needs its callee named
 *     first (`Pl_motion_input_ck` alone blocks 9 of the 74 open rows).
 *
 * Load-bearing shapes worth copying into the next functions of this range:
 *   * the per-part argument is `s32`, never `u8` - a `u8` declaration makes MWCC mask it
 *     (`clrlwi r...,r4,24`) before every compare, which retail does not do anywhere in the range;
 *   * `++` on a `u8` field stores raw (`addi`/`stb`); `+= 1` and `= x + 1` both add a `clrlwi`;
 *   * a `switch` whose case bodies are shared (`case 1: case 2:`) has its bodies emitted in
 *     *source* order, and a 3-value switch whose 0-arm is written first emits the bodies as
 *     2, 3, 0 - write the cases in the order the target's bodies sit in `.text`.
 */

/* Retired header of `Pl/fn_80258FCC.cpp` (kept for its notes and residuals): */
/* The player act state-machine band `Pl/fn_80258FCC.cpp`.
 *
 * `.text` 0x80258FCC-0x8025F088 (74 functions, 0x60BC B), extab 0x8001219C-0x8001239C and
 * extabindex 0x8002F154-0x8002F454; this lib's `cflags_pl` (Wii/1.0).
 *
 * Home is `Pl` (brief section 2 class 3): every function's first argument is the player work
 * `_PLW*`, the gates are the Pl siblings (`Pl_master_ck`, `Pl_act_ck`, `Pl_Skill_ck`,
 * `Pl_frame_check`), and the bracketing registered units are `Pl/fn_80241558.cpp` (ends
 * 0x802430E8) and `Pl/fn_80262940.cpp` (starts 0x80262940).  No `__FILE__` string is reachable
 * from the range and `dumpmap.py lookup 0x80258FCC` answers only `zz_0258fcc_`, so the file keeps
 * the map's `fn_XXXXXXXX` stem (class 4).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x80258FCC 0x8025F088` - every row is `fn_` - and
 * `python tools/splits/tudiscover.py at 0x80258FCC`, which reports 0 must-link anchors, no
 * `__FILE__` anchor and no owned data).
 *
 * The seam is unproven.  The extab/extabindex runs do break on the proposal boundaries - the
 * extabindex run 0x8002F154-0x8002F454 is exactly this run's 64 framed functions and the
 * bracketing proposals' runs abut it - but the extab table is ordered by function address, so
 * that is consistency, not proof.
 *
 * RESIDUAL (this pass).  13 of the 74 functions are written, all above the brief's bar: 11
 * byte-identical, `fn_80259684` 98.32 % (476 B) and `fn_8025A7FC` 94.68 % (312 B).
 *  - `fn_80259684`: the target materialises the `0` argument of `fn_80277DAC` AFTER the two float
 *    arguments (`lfs f1, pl_frame_window_85; lfs f2, pl_float_neg60; li r4,0`), ours before them.  Same
 *    instruction count and size; only the materialisation order differs.
 *  - `fn_8025A7FC`: the target schedules the `threshold = 15` materialisation between the
 *    `cmplwi` and the `bne` of the `field_0x128 == 2` test, ours before the load.  Same size.
 *  - The remaining 61 functions are not written, address order: fn_80259860 744 B, fn_80259B48 240 B,
 *    fn_80259C38 304 B, fn_80259D68 112 B, fn_80259DD8 288 B, fn_80259EF8 252 B, fn_80259FF4 312 B,
 *    fn_8025A12C 428 B, fn_8025A2D8 1276 B, fn_8025A934 260 B, fn_8025AA38 576 B, fn_8025AC78 276 B,
 *    fn_8025AD8C 288 B, fn_8025AEAC 316 B, fn_8025AFE8 236 B, fn_8025B0F8 608 B, fn_8025B358 320 B,
 *    fn_8025B498 172 B, fn_8025B544 160 B, fn_8025B5E4 180 B, fn_8025B698 48 B, fn_8025B6C8 168 B,
 *    fn_8025B770 184 B, fn_8025B828 152 B, fn_8025B8C0 500 B, fn_8025BAB4 148 B, fn_8025BB48 180 B,
 *    fn_8025BBFC 332 B, fn_8025BD48 220 B, fn_8025BE24 328 B, fn_8025BF6C 304 B, fn_8025C09C 480 B,
 *    fn_8025C27C 452 B, fn_8025C440 292 B, fn_8025C564 620 B, fn_8025C7D0 204 B, fn_8025C89C 216 B,
 *    fn_8025C974 340 B, fn_8025CAC8 496 B, fn_8025CCB8 432 B, fn_8025CE68 772 B, fn_8025D16C 228 B,
 *    fn_8025D250 372 B, fn_8025D3C4 308 B, fn_8025D4F8 156 B, fn_8025D594 296 B, fn_8025D6BC 404 B,
 *    fn_8025D850 260 B, fn_8025D954 228 B, fn_8025DA38 420 B, fn_8025DBDC 352 B, fn_8025DD3C 252 B,
 *    fn_8025DE38 180 B, fn_8025DEEC 140 B, fn_8025DF78 336 B, fn_8025E0C8 380 B, fn_8025E448 1960 B,
 *    fn_8025EBF0 104 B, fn_8025EC58 152 B, fn_8025ED00 756 B, fn_8025EFF4 148 B
 *    They are the next pass's work.
 *
 * Load-bearing source shapes found here (they cost several variants each):
 *  - a byte field's `x = x - 1` emits a redundant narrowing (`extsb r0,r0`); `x--` does not.
 *  - a `*timer -= 10` on an `s16*` emits `extsh r0,r0`; `*timer -= (s16)10` does not.
 *  - an `s16` parameter that is only compared emits `extsh`+`cmpwi` where retail keeps `cmpwi`;
 *    the parameter is `s32` in that case and the `extsh` belongs to the callee's narrower one.
 *  - a redundant `(u8)` cast on the *second* of two byte stores is what makes MWCC narrow it.
 *  - the `if (cond >= 1) { big }` shape (rather than `if (cond < 1) { small }`) is what puts the
 *    small block at the end of the function where retail has it.
 */

/* Retired header of `Pl/fn_8025F088.cpp` (kept for its notes and residuals): */
/*
 * Player per-frame control cluster (proposal 8025F088): `.text` 0x8025F088-0x80262940 (17 functions,
 * 0x38B8 B) with its own exception tables - extab 0x8001239C-0x8001241C (16 records) and extabindex
 * 0x8002F454-0x8002F514 (16 records).  The extab run starts exactly at fn_8025F088 and ends exactly
 * after fn_80262688, so both seams are real TU boundaries, not caps.
 *
 * Registration (docs/plan.md 12, evidence class 3 + 4):
 *   - no `__FILE__` string covers the range: the only strings its data touches are the `.sdata2`
 *     float pool 0x80799E00-0x80799FDC and `.bss` `pl_move_work`, and no source-name string is
 *     referenced from the range at all (class 1 out);
 *   - `dumpmap.py lookup` answers only `zz_` placeholders for all 17 symbols (class 2 out);
 *   - the code is the `Pl` module: every actor parameter is a `_PLW` (`Pl_master_ck`, `Pl_act_ck`,
 *     `Pl_Skill_ck`, `Pl_cat_skill_ck`, `Get_motion_no`), it reads the move work
 *     `get_move_work_adrs`/`get_move_work_max` and the `pl_move_work` chunk table, and its siblings
 *     are `Pl/fn_80241558.cpp` (before) and `Pl/fn_80262940.cpp` (after);
 *   - all 17 map entries carry only the `fn_XXXXXXXX` stem, so the file keeps the stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x8025F088 0x80262940`; all 17 entries are the map's
 * placeholders and `dumpmap.py lookup` answers `zz_` for each).
 *
 * Flags: the unit uses `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * `mw_version Wii/1.0`), the Pl lib's measured flag set - the extab/extabindex pair above is what
 * `-Cpp_exceptions on` reproduces.
 *
 * Rule 2 debt (recorded, not hidden): `include/Pl/fn_8025F088.h` declares the callees this unit needs
 * whose owners' headers do not declare them (the `enemy/fn_80165FC8.h` precedent).  Moving each to
 * its owner's header is a later pass; `include/unsplit/Pl.h`'s five declarations that this
 * registration makes owned (`fn_8025FA00`, `fn_80260A18`, `fn_80261770`, `fn_802621B0`,
 * `fn_80262688`) were removed here.
 *
 * Residual: see `.pi/notes/8025f088-fn-8025f088-eedb.md` for the per-function measurements and the
 * functions still unwritten.
 */

/* Retired header of `Pl/fn_80262940.cpp` (kept for its notes and residuals): */
/*
 * Player main/control cluster (proposal 80262940): .text 0x80262940-0x802693C4 (59 functions, 0x6A84 B)
 * with its own exception tables - extab 0x8001241C-0x80012554, extabindex 0x8002F514-0x8002F6E8.
 *
 * Registration (docs/plan.md 12, evidence class 3 + 4):
 *   - no `__FILE__` string covers the band (the .data source-name pool has none between
 *     `enemy_control.cpp` @0x805A1BB8 and `menu_item.cpp` @0x805CDFC8), so class 1 is out;
 *   - `dumpmap.py lookup` gives real names for 8 of the 59 symbols (`player_control_move`,
 *     `init_player_work`, `player_move_start`, `player_control_release`, `set_max_player`,
 *     `player_init_data_load`, `player_move_start_ck`, `parts_mdl_release_all`): the game's own
 *     player-control entry points;
 *   - the code is the `Pl` module (`_PLW` actors, `Pl_master_ck`/`Pl_Skill_ck`/`Pl_act_ck`/
 *     `Pl_cat_skill_ck`/`Pl_condition_ck` gates, `get_move_work_adrs`), and every sibling unit is
 *     `Pl/*.cpp`;
 *   - 51 of the 59 map entries carry only the `fn_XXXXXXXX` stem, so the file keeps the stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x80262940 0x802693C4`; 8 of 59 entries are named by the
 * runtime dump, the rest are the map's placeholders).
 *
 * Seam: the range edge is a `--max-bytes` cap, not a TU boundary (nothing in the region names a file).
 * The extabindex run for this file is 0x8002F514-0x8002F6E8 (39 records, first `fn_80262940`, last
 * `fn_8026910C`); the next record is `fn_802695A4` at 0x8002F6E8, i.e. the four functions
 * 0x802693C4-0x802695A4 may belong to this file - they are left to their own proposal
 * (`proposal/802693C4_fn_802693C4`) and recorded here as the open seam question.
 *
 * Flags: the unit uses `cflags_pl` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * `mw_version Wii/1.0`), the Pl lib's measured flag set.
 *
 * Residual: work in progress - functions below the 80 % bar are recorded in
 * `.pi/notes/80262940-fn-80262940-8973.md`.
 */

#include "types.h"
#include "ef/get_move_work_adrs.h"
#include "pl.h"
#include "sound/se.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Network/network_pat_control.h"   /* isServerSelectState (owner header, rule 2) */
#include "ef/fn_800CDB2C.h"
#define get_move_work_adrs get_move_work_adrs_hidden_fn_8025F088_h
#define get_move_work_max get_move_work_max_hidden_fn_8025F088_h
#define fn_8025E298 fn_8025E298_hidden_fn_8025F088_h
#define fn_8025E298 fn_8025E298_hidden_fn_8025F088_h
#include "Pl/fn_8025F088.h"
#undef fn_8025E298
#undef fn_8025E298
#undef get_move_work_max
#undef get_move_work_adrs
#include "Pl/fn_802693C4.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 frame-window pool (rule 2) */
#include "nw4r/math.h"
#include "Pl/fn_802693C4.h"   /* 0x802693C4-0x8026BA1C - the owner of the Pl_chr_set_attr_default/33C/644 group */
#define get_move_work_adrs get_move_work_adrs_hidden_ef_h
#define get_move_work_max get_move_work_max_hidden_ef_h
#include "unsplit/ef.h"
#undef get_move_work_max
#undef get_move_work_adrs
#include "ef/eft004.h"
#include "ef.h"
#include "g3d/g3d_calcworld.h"
#include "Pl/pl_act_step.h"
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 float pool (rule 2) */
#include "Pl/fn_80273B14.h" /* the owner header of the act-motion setters (rule 2) */
#include "Pl/fn_80262940.h" /* the owner header of the model-state setter */
#include "ef/fn_800CDB2C.h"   /* the ef play-mode dispatcher the act-175 arm drives */
#include "Pl/pl_act_step.h"   /* 0x80257E70 - the owner of the three-timer reset this unit calls */
#include "Pl/fn_802693C4.h"   /* 0x802693C4-0x8026BA1C - the owner of the Pl_chr_set_attr_default/33C/644/3A8 group */
#include "fn_8004CAD8.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "Pl/fn_802693C4.h"   /* `Get_motion_no`, owned by Pl/fn_802693C4.cpp - the band header's
                          * copy is gone now that the owner's registration declares it (rule 2) */
#include "Pl/pl_frame_data.h" /* the owner of the Pl band's shared .sdata2 pool 0x80799E00-0x80799F98 (rule 2) */
#include "Pl/pl_act_data.h" /* the owner of the pool's second run, 0x80799F98-0x80799FDC (rule 2) */
#include "Pl/bss_pool.h" /* the owner of the `.bss` move-work table `pl_move_work` (rule 2) */
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "ef/eft052.h" /* hud_item_msg_push (the owner's header, rule 2) */
#include "lobby/fn_8021E1EC.h"
/* fn_802DE578_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_802DE578_view1 ((void (*)(_PLW*, u16*))fn_802DE578)
/* fn_8025E298_view3: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8025E298_view3 ((u32 (*)(struct _PLW*, s32, s32))fn_8025E298)
/* fn_8025E298_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8025E298_view1 ((u32 (*)(struct _PLW*, s32, s32))fn_8025E298)

extern "C" void fn_80247CC4(_PLW* self);

extern "C" {
/* This unit's own helpers, defined further down but called from the handlers above them. */
void pl_act_net_hook_a(_PLW* self);
void pl_act_net_hook_c(_PLW* self);
void fn_8024D3C8(_PLW* self);

s32 fn_8024D5AC(_PLW* self, _PLW* other);

s16 fn_8024CD8C(_PLW* self, s16 value);

void fn_8024CA50(_PLW* self, s32 a);

void fn_802F39DC(_PLW* self, s32 arg);
}

/* One 0x16-byte entry of the `_PLW::field_0x318` scan table `fn_8025E298` walks; `id_0x00` == 0xFF
 * terminates the run. size: 0x16 */
typedef struct PlScanEntry {
    /* +0x00 */ u8 id_0x00;
    /* +0x01 */ u8 pad_0x01[0x1];
    /* +0x02 */ u16 key_0x02;
    /* +0x04 */ u8 pad_0x04[0x2];
    /* +0x06 */ u16 flags_0x06;
    /* +0x08 */ u8 pad_0x08[0x6];
    /* +0x0E */ u16 flags_0x0E;
    /* +0x10 */ u8 pad_0x10[0x6];
} PlScanEntry; /* size: 0x16 */

/* One entry of `pl_move_work` is `PlMoveEntry`, declared by its owner's header (rule 1/2). */

/* One 2840-byte (0xB18) move-work record `get_move_work_adrs` hands back; the motion slot `fn_802607C4`
 * scores sits at +0x188 and the entry's own kind byte at +0x1E1. size: 0xB18 */
typedef struct PlMoveWork {
    /* +0x000 */ u8 field_0x000;
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 pad_0x002[0x186];
    /* +0x188 */ VEC3 pos_0x188;
    /* +0x194 */ u8 pad_0x194[0x4D];
    /* +0x1E1 */ u8 field_0x1E1;
    /* +0x1E2 */ u8 pad_0x1E2[0x936];
} PlMoveWork; /* size: 0xB18 */

/* The player body sub-object `_PLW::physics_0x13C` points at: its `MHchar` block sits at +0x04 (the
 * layout `ef/fn_80114E34.cpp` owns as `_PLW_PHYSICS`).  A local view, because that owner keeps its
 * definition in its own `.cpp`; moving it to a shared header is the rule-1 debt this unit records.
 * size: 0x144 (lower bound: the `MHchar` it holds is 0x140) */
typedef struct PlBodyWork {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr_0x04;
} PlBodyWork; /* size: 0x144 */

/* One item-data record `GetItemData` hands back; only the kind byte at +0x01 is read here.
 * size: 0x02 (lower bound: +0x01 is the highest byte this unit names) */
typedef struct PlItemData {
    /* +0x00 */ u8 pad_0x00[0x1];
    /* +0x01 */ u8 field_0x01;
} PlItemData; /* size: 0x02 */

extern "C" {
/* this unit's own entry points (unmangled `fn_*` stems, so C linkage - playbook 42/48) */
void fn_80264274(struct _PLW* self, s16 value);
void fn_80264940(struct _PLW* self, u32 value);
u8 fn_8026495C(struct _PLW* self);
void fn_80264A28(struct _PLW* self);
s32 fn_80264A84(struct _PLW* self);
s32 fn_80264EA4(struct _PLW* self);
s32 fn_80264EC0(struct _PLW* self);
s32 fn_80264ED4(struct _PLW* self);
s32 fn_80264EE8(struct _PLW* self);
s32 fn_80264F78(struct _PLW* self);
s32 fn_80264FF0(struct _PLW* self);
s32 fn_8026505C(struct _PLW* self);
s32 fn_802650EC(struct _PLW* self, u8 kind);
s32 fn_80265348(struct _PLW* self);

void fn_802656EC(struct _PLW* self);
s32 fn_802656FC(struct _PLW* self);
u32 fn_80265748(struct _PLW* self);
void fn_80265780(struct _PLW* self);
u32 fn_802657AC(struct _PLW* self);

void pl_model_state_set(struct _PLW* self, u32 action, s32 a, u16 b);
s32 fn_80264B4C(struct _PLW* self);
void fn_80266EB8(struct _PLW* self);
}

s32 fn_80260198(_PLW* self);
void fn_8025F088(_PLW* self);

extern "C" void fn_802430E8(_PLW* work, u8 part) {
    u32 partHi;
    /* The three per-part `_se_w` work objects; the declaration order and the assignment order are
     * deliberately different - it is the shape that reproduces retail's colouring
     * (r31 = +0xAF4, r29 = +0xAF8, r30 = +0xAFC) together with its load order, exactly as in
     * `Pl/fn_80241558.cpp`. */
    _se_w* seWorkPart;
    _se_w* seWorkMain;
    _se_w* seWorkFrame;
    seWorkPart = work->field_0xAF4;
    seWorkFrame = work->field_0xAF8;
    seWorkMain = work->field_0xAFC;

    switch (Get_motion_no(work)) {
    case 0x3EA:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4A, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x60, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 4, (part << 0x18) | 4, 0x14, 2);
        return;
    case 0x3EB:
        fn_80229E10(seWorkPart, 4, 3);
        se_req_frame_set(seWorkMain, 0xE, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x24, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x4E, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x66, part << 0x18, 0x11, 2);
        fn_80229E10(seWorkPart, 0x68, 3);
        return;
    case 0x3EC:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 8, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        return;
    case 0x3F1:
        se_req_frame_set(seWorkMain, 0xE, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x20, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3E, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x16, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x30, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x4A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x66, partHi | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 6, 3);
        fn_80229E10(seWorkPart, 0x20, 3);
        fn_80229E10(seWorkPart, 0x40, 3);
        fn_80229E10(seWorkPart, 0x5A, 3);
        return;
    case 0x3F3:
        se_req_frame_set(seWorkPart, 6, SE_Code_Make(0, 3, 1, 3), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0x29, 0xC, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xB, 0xE, 0x03000002);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 4, partHi | 3, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x2E, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x34, 3);
        return;
    case 0x408:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1E, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 8, 3);
        fn_80229E10(seWorkPart, 0x24, 3);
        return;
    case 0x409:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x14, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x16, 3);
        fn_80229E10(seWorkPart, 0x28, 3);
        return;
    case 0x40A:
        se_req_frame_set(seWorkPart, 0x2A, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x16, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2C, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        fn_80229E10(seWorkPart, 0x2C, 3);
        return;
    case 0x40B:
        se_req_frame_set(seWorkMain, 2, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x14, (part << 0x18) | 4, 0x14, 2);
        fn_80229E10(seWorkPart, 0x1C, 3);
        return;
    case 0x40C:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        fn_800DA428(seWorkFrame, 0x38, part << 0x18, 0x14, 2);
        fn_80229E10(seWorkPart, 0x3A, 3);
        return;
    case 0x40D:
        se_req_frame_set(seWorkMain, 2, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x10, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x24, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi | 4, 0x11, 2);
        fn_80229E10(seWorkPart, 0x14, 3);
        fn_80229E10(seWorkPart, 0x34, 3);
        return;
    case 0x40E:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x3A, partHi | 4, 0x11, 2);
        fn_80229EA8(seWorkPart, 0x26, 0xB, 3);
        fn_80229E10(seWorkPart, 0x3C, 3);
        return;
    case 0x40F:
        fn_80229E10(seWorkPart, 4, 3);
        se_req_frame_set(seWorkMain, 4, 2, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x14, 4, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x36, 1, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xE, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2A, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x5A, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x10, 3);
        fn_80229E10(seWorkPart, 0x2C, 3);
        return;
    case 0x411:
        se_req_frame_set(seWorkPart, 0x18, SE_Code_Make(7, 1, 8, 1), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x16, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x2E, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        fn_80229E10(seWorkPart, 0x30, 3);
        return;
    case 0x412:
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 2, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 4, 3);
        fn_80229E10(seWorkPart, 0x1C, 3);
        return;
    case 0x41B:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x3A, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x5C, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x03000003);
        return;
    case 0x41C:
        se_req_frame_set(seWorkMain, 0xA, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x20, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4A, 0xEA, 3, 0x03000003);
        return;
    case 0x41D:
        se_req_frame_set(seWorkPart, 0x12, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x30, 0xED, 0x11, 0x03000003);
        return;
    case 0x421:
        se_req_frame_set(seWorkMain, 0xC, 0xC, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x1E, 0xE, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkMain, 0x48, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 8, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4C, 0xEA, 3, 0x03000003);
        return;
    case 0x426:
        se_req_frame_set(seWorkPart, 2, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x40, 0xED, 0x11, 0x03000003);
        return;
    case 0x428:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3E, 0xED, 0x14, 0x03000003);
        return;
    case 0x429:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x42A:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1C, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3E, 0xED, 0x11, 0x03000003);
        return;
    case 0x42B:
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x43A:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        return;
    case 0x43B:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x43C:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 3) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 6, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x22, 0xEE, 0x14, 0x03000003);
        return;
    case 0x424:
    case 0x425:
    case 0x42C:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x42, 0xED, 0x11, 0x03000003);
        return;
    case 0x42D:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x14, 2, 0x15, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 4, 0xF5, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x44, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x54, 0xED, 0x11, 0x03000003);
        return;
    case 0x43D:
        se_req_frame_set(seWorkMain, 4, 0xB, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x44D:
    case 0x44E:
    case 0x44F:
        se_req_frame_set(seWorkPart, 8, 8, 0x11, 0x03000003);
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x5A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x6C, partHi | 1, 0x11, 2);
        fn_80229E10(seWorkPart, 0x60, 3);
        return;
    case 0x450:
        se_req_frame_set(seWorkMain, 0x20, 2, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 9, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 3, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = (part << 0x18) | 4;
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x78, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x1A, 3);
        fn_80229E10(seWorkPart, 0x7A, 3);
        return;
    case 0x451:
    case 0x452:
    case 0x453:
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        return;
    case 0x454:
        se_req_frame_set(seWorkMain, 0x22, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x2C, partHi | 2, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x3E, partHi | 4, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x60, partHi | 1, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x7C, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 0x46, 3);
        fn_80229E10(seWorkPart, 0x7E, 3);
        return;
    case 0x455:
    case 0x456:
        se_req_frame_set(seWorkMain, 0x24, 3, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x68, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x84, partHi | 1, 0x11, 2);
        se_req_frame_set(seWorkPart, 6, 0x10, 0x11, 0x03000003);
        fn_80229E10(seWorkPart, 0x6C, 3);
        return;
    case 0x458:
        se_req_frame_set(seWorkMain, 4, 1, 7, 2);
        return;
    case 0x459:
        se_req_frame_set(seWorkPart, 0x16, SE_Code_Make(2, 3, 1, 3), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x42, partHi | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 0x27, 0x14, 0x03000003);
        fn_80229EA8(seWorkPart, 0x36, 7, 3);
        return;
    case 0x45A:
    case 0x45B:
        se_req_frame_set(seWorkMain, 0x24, 2, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2E, 9, 7, 2);
        se_req_frame_set(seWorkMain, 0x3C, 3, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 6, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 5, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = (part << 0x18) | 4;
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x78, partHi, 0x14, 2);
        fn_80229E10(seWorkPart, 4, 3);
        fn_80229E10(seWorkPart, 0x7A, 3);
        return;
    case 0x45C:
    case 0x45D:
        se_req_frame_set(seWorkMain, 4, 1, 7, 2);
        return;
    case 0x46A:
        se_req_frame_set(seWorkMain, 2, 1, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x40, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x5C, 4, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0x12, partHi | 1, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x58, partHi | 4, 0x14, 2);
        return;
    case 0x46B:
        se_req_frame_set(seWorkMain, 2, 2, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        fn_800DA428(seWorkFrame, 0x24, part << 0x18, 0x14, 2);
        fn_80229E10(seWorkPart, 0x26, 3);
        return;
    case 0x47E:
    case 0x47F:
    case 0x480:
        se_req_frame_set(seWorkMain, 0x22, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x30, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x46, 0xED, 0x14, 0x03000003);
        return;
    case 0x481:
        se_req_frame_set(seWorkMain, 0x20, 0xC, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 0xD, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x74, 0xED, 3, 0x03000003);
        return;
    case 0x482:
    case 0x483:
    case 0x484:
        se_req_frame_set(seWorkMain, 0x22, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 0x12, 0xED, 0x11, 0x03000003);
        return;
    case 0x485:
    case 0x486:
    case 0x487:
        se_req_frame_set(seWorkMain, 0x24, 0xD, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x36, 0xEE, 0x11, 0x03000003);
        se_req_frame_set(seWorkPart, 0x44, 0xEE, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x64, 0xED, 0x11, 0x03000003);
        return;
    case 0x489:
        se_req_frame_set(seWorkMain, 4, 2, 7, 2);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x48A:
        se_req_frame_set(seWorkPart, 0x14, SE_Code_Make(0x14, 2, 0x14, 2), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xE, 0x8B, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x4C, 0xED, 0x14, 0x03000003);
        return;
    case 0x48B:
    case 0x48C:
        se_req_frame_set(seWorkMain, 0x20, 0xC, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x2C, 0x1D, 7, 2);
        se_req_frame_set(seWorkMain, 0x3E, 0xD, 7, ((seWorkMain->field_0x0C + 3) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0xA, 0x10, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x68, 0xF, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 0x68, 0xED, 0x14, 0x03000003);
        return;
    case 0x48D:
    case 0x48E:
        se_req_frame_set(seWorkMain, 4, 2, 7, 2);
        se_req_frame_set(seWorkPart, 0x26, 0xED, 3, 0x03000003);
        return;
    case 0x49C:
        se_req_frame_set(seWorkMain, 4, 0xB, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x40, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkMain, 0x5A, 0xE, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x49D:
        se_req_frame_set(seWorkMain, 6, 0xC, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        return;
    case 0x49E:
        se_req_frame_set(seWorkMain, 6, 0xB, 7, ((seWorkMain->field_0x0C + 1) << 0x18) | 2);
        se_req_frame_set(seWorkPart, 4, 0xEA, 3, 0x03000003);
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        return;
    case 0x410:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xA, partHi | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1A, partHi | 4, 0x11, 2);
        return;
    case 0x3F4:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 6, partHi, 0x14, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x11, 0x03000003);
        partHi = partHi | 1;
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        return;
    case 0x3F5:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 6, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x14, 0x03000003);
        partHi = partHi | 1;
        fn_800DA428(seWorkFrame, 0x1A, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        return;
    case 0x3F6:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0, 2, 1, 2), 0xC, 3);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 4, partHi, 0x11, 2);
        se_req_frame_set(seWorkPart, 0x14, 8, 0x14, 0x03000003);
        partHi = partHi | 1;
        fn_800DA428(seWorkFrame, 0x18, partHi, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x11, 2);
        fn_80229E10(seWorkPart, 0x18, 3);
        return;
    case 0x46C:
        se_req_frame_set(seWorkMain, 4, 1, 3, ((seWorkMain->field_0x0C + 1) << 0x18) | 4);
        partHi = part << 0x18;
        fn_800DA428(seWorkFrame, 0xC, partHi | 2, 0x14, 2);
        fn_800DA428(seWorkFrame, 0x1C, partHi | 3, 0x11, 2);
        fn_800DA428(seWorkFrame, 0x32, partHi, 0x14, 2);
        return;
    case 0x4B1:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(4, 2, 4, 2), 0xC, 3);
        fn_800DA428(seWorkFrame, 2, part << 0x18, 0x14, 2);
        return;
    case 0x4BB:
        fn_800DA428(seWorkFrame, 0x3A, (part << 0x18) | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 2, 0x10, 0x11, 0x03000003);
        fn_80229E10(seWorkPart, 0x3C, 3);
        return;
    case 0x4BC:
        fn_800DA428(seWorkFrame, 0x72, (part << 0x18) | 1, 0x14, 2);
        se_req_frame_set(seWorkPart, 8, 0x10, 0x11, 0x03000003);
        fn_80229E10(seWorkPart, 0x74, 3);
        return;
    case 0x4E3:
        se_req_frame_set(seWorkPart, 4, SE_Code_Make(0x18, 1, 0x19, 1), 0xC, 3);
        se_req_frame_set(seWorkPart, 0xC, 0xED, 0x11, 0x03000003);
        return;
    case 0x4ED:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x1E, 0xED, 0x11, 0x03000003);
        return;
    case 0x4EE:
        se_req_frame_set(seWorkPart, 4, 0xED, 0x14, 0x03000003);
        se_req_frame_set(seWorkPart, 0x3A, 0xED, 0x11, 0x03000003);
        /* fallthrough */
    default:
        return;
    }
}

/* Clears the act's timer/latch block on entry and, when the alternate path is taken on a live master,
 * drops the 0x300 pair from the act bitfield. */
extern "C" void pl_act_clear_wait(_PLW* self, u8 a) {
    if (Pl_master_ck(self) == 1 && a == 0) {
        self->field_0x3D8 &= ~0x300;
    }
    if (a == 0) {
        self->field_0x418 = 0;
        self->field_0x41E = 0;
        self->field_0x444 = 0;
    }
    self->field_0x416 = 0;
    self->field_0x41C = 0;
}

/* Reports the input-hold latch, defaulting to "held" while the pad layer has nothing to say. */
extern "C" s32 fn_80245E20(_PLW* self) {
    if (isServerSelectState() == 0) {
        return 1;
    }
    return self->field_0x645 != 0;
}

/* Steps the actor's chase one frame along the chunk's own rotation-offset pair. */
extern "C" void fn_80246158(_PLW* self) {
    VEC3 vec;
    VEC3 scratch;
    VEC3_ctor(&vec);
    VEC3_ctor(&scratch);
    vec.x = lbl_805C4898[self->chunk_ofs * 2];
    vec.y = pl_float_zero;
    vec.z = lbl_805C4898[self->chunk_ofs * 2 + 1];
    rotVecY(&vec, self->field_0x058);
    self->motion_pos_0x3C = self->motion_pos_0x3C + vec.x;
    self->motion_pos_0x44 = self->motion_pos_0x44 + vec.z;
}

/* Scans the equipment slots for the first crafted item and records its index. */
extern "C" void fn_802466C4(_PLW* self) {
    s32 i;
    self->field_0x304 = 0;
    for (i = 0; i < 0x1A; i++) {
        u16 item_id = self->slot_id[i].item_id;
        if (self->slot_id[i].value > 0) {
            u8* item = GetItemData(item_id);
            if (item_id != 0 && (item[2] & 8) != 0 && item[0] != 1) {
                self->field_0x304 = (u8)i;
                break;
            }
        }
    }
}

/* Drives the act's first two steps: arm the motion and hold, then wait for it. */
extern "C" void fn_802478A4(_PLW* self) {
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x12, 4, 0);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* Halves (or thirds) the act's damage figure from the equip skills and hands it to the health change
 * helper. */
extern "C" void fn_80247C2C(_PLW* self) {
    s16 amount;
    if (Pl_Skill_ck(self, 0xA8) == 1) {
        amount = -0x10;
    } else if (Pl_Skill_ck(self, 0xA9) == 1) {
        amount = -0x24;
    } else {
        amount = -0x1E;
    }
    if (Pl_cat_skill_ck(self, 0x21) == 1) {
        amount = (s16)amount / 2;
    }
    fn_80276868(self, amount);
}

/* Runs the first two act steps of the alternate chase and then the shared chase update. */
extern "C" void fn_80247EF0(_PLW* self, u32 a) {
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (a == 0) {
            Pl_act_set_motion(self, 2, 0, 0);
            Pl_chr_set_attr_default(self, 0x22, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x8B, 4, 0);
        }
        self->field_0x585 = 0;
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_90, pl_float_zero) == 1) {
            if (a == 0) {
                pl_act_enter(self, 0, 0xB, 0);
            } else {
                pl_act_enter(self, 0, 0x4B, 0);
            }
            return;
        }
        break;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

/* The close-quarters variant of the same two-step chase. */
extern "C" void fn_802488D4(_PLW* self, u32 a) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (a == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x24, 6, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x8D, 6, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (a == 0) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
            } else {
                Pl_act_set_motion_slot(self, 3, 6, 0);
            }
            return;
        }
        break;
    }
    fn_80260198(self);
    fn_80247CC4(self);
}

extern "C" {
/* 0x80249424 - the "down/knock-down" act handler: arms the down motion, then ends the act once the
 * master gate and the hit check agree. */
void fn_80249424(_PLW* self, s32 arg1) {
    fn_8027A17C(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x3C, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x87, 4, 0);
        }
        self->act_end_request = 0;
        fn_80101594(self);
        return;
    case 1:
        if (Pl_master_ck(self) != 0 && (pl_part_flag_ck(self, 4) == 1 || self->act_end_request != 0)) {
            if (arg1 == 0) {
                pl_act_enter(self, 0, 0x15, 0);
                return;
            }
            pl_act_enter(self, 0, 0x9F, 0);
        }
        return;
    }
}

/* 0x80249558 - the get-up handler: arms the get-up motion, then hands the actor's kind to the motion
 * table once the pose check passes. */
void fn_80249558(_PLW* self, s32 arg1) {
    fn_8027A17C(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x3D, 4, 0);
            return;
        }
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x88, 4, 0);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        return;
    }
}

/* 0x8024963C - the get-up handler with an item-recovery arm: on `arg2 == 1` it reads the pending
 * recovery item and feeds it to `pl_item_add`. */
void fn_8024963C(_PLW* self, s32 arg1, s32 arg2) {
    u16 item;
    s16 value;

    fn_8027A17C(self);
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 0x200, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x214, 4, 0);
        }
        if (arg2 == 1) {
            fn_802D884C(&item, &value);
            if (Pl_motion_input_ck(1) == 0 && value > 0) {
                pl_item_add(self, item, value);
            }
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        /* the last case has no branch: retail falls straight into the epilogue */
    }
}

/* 0x80249768 - the "hold/struggle" handler: arms the struggle motion and gives up after 600 frames
 * or when the act-end signal arrives. */
void fn_80249768(_PLW* self, s32 arg1) {
    s32 timer;

    self->act_handler_entered = 1;
    fn_802DE578(self, &self->field_0x598);
    fn_8027A17C(self);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_setX(self, 1, 6, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_setX(self, 0x64, 6, 0);
        }
        pl_act_set_frame_timer(self);
        if (isServerSelectState() == 1) {
            self->act_end_request = 0;
        }
        self->field_0x28 = 0;
        return;
    case 1:
        if (Pl_master_ck(self) == 0) {
            return;
        }
        timer = self->field_0x28 + 1;
        self->field_0x28 = timer;
        if (timer > 0x258 || self->act_end_request != 0) {
            if (self->act_end_request == 1) {
                if (arg1 == 0) {
                    pl_act_enter(self, 0, 0x49, 0);
                    return;
                }
                pl_act_enter(self, 0, 0xA0, 0);
                return;
            }
            Pl_act_set_motion_slot(self, self->kind_0x09, 2, 0);
            return;
        }
        return;
    }
}

/* 0x802498E0 - the "stagger/recover" handler: it either arms a fixed motion table or hands the choice
 * to `fn_802771A0`, then picks the follow-up motion from the actor's state. */
void fn_802498E0(_PLW* self) {
    s32 timer;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8003, 0, 0);
        self->field_0x28 = 0x1E;
        if (self->field_0x018 == 0) {
            Pl_act_set_step_table(self, (u32)lbl_805BE824, 0);
            return;
        }
        fn_802771A0(self, 0);
        return;
    case 1:
        if (self->field_0x28 > 0 || fn_8027A340(self) == 0) {
            timer = self->field_0x28 - 1;
            self->field_0x28 = timer;
            if (timer <= 0) {
                self->field_0x0AC = 0;
            }
        }
        if (Pl_master_ck(self) == 1 && fn_8026FE44(self) == 1 && self->field_0x018 == 1) {
            fn_8027A57C(self, 0x41A, 0);
        }
        if (self->field_0x018 != 1) {
            if (fn_80276800(self, 0) == 1) {
                Pl_chr_setX(self, 0x79, 4, 0);
                return;
            }
            if (self->field_0x37A <= 0x96) {
                Pl_chr_setX(self, 0x168, 4, 0);
                return;
            }
            if (Pl_suimen_ck(self) == 1) {
                Pl_chr_setX(self, 0x76, 4, 0);
                return;
            }
            Pl_chr_setX(self, 0x64, 4, 0);
            return;
        }
        return;
    }
}

/* 0x8024A51C - the "sit/rest" handler: one arming step, then a two-way exit through `pl_act_enter`. */
void fn_8024A51C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 7, 2, 0);
        pl_act_clear_mode5c4(self);
        pl_act_set_frame_timer(self);
        if (arg1 == 0) {
            pl_act_clear_flag5bb(self);
            return;
        }
        fn_8027AC00(self);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 1) {
                if (arg1 == 0) {
                    pl_act_enter(self, 0, 0x1F, 0);
                    return;
                }
                pl_act_enter(self, 0xA, 0xE, 0);
                return;
            }
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 8, 6, 0);
            return;
        }
        return;
    }
}

/* 0x8024A640 - the one-step "equip/ready" handler. */
void fn_8024A640(_PLW* self) {
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0x8001, 0, 0);
        self->field_0x018 = 0;
        Pl_chr_setX(self, 8, 4, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_step_table(self, (u32)lbl_805BE5B8, 0);
    }
}

/* 0x8024A6CC - the two-step "sheathe" handler. */
void fn_8024A6CC(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 1, 0, 0);
        Pl_chr_set_attr_default(self, 9, 0, 0);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8024A75C - the "stagger" handler: arms a 150-frame motion, then hands the actor's `+0x37A` value
 * to the motion table. */
void fn_8024A75C(_PLW* self, s32 arg1) {
    s32 timer;

    fn_80276868(self, 4);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        if (arg1 == 0 || arg1 == 2) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 5, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x7A, 6, 0);
        }
        self->field_0x28 = 0x96;
        return;
    case 1:
        timer = self->field_0x28 - 1;
        self->field_0x28 = timer;
        if (timer > 0) {
            return;
        }
        self->act_step_0x05++;
        if (arg1 == 0 || arg1 == 2) {
            Pl_chr_set_attr_default(self, 6, 4, 0);
            return;
        }
        Pl_chr_set_attr_default(self, 0x7C, 4, 0);
        return;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            fn_80276868(self, self->field_0x37A);
            if (arg1 == 0 || arg1 == 2) {
                Pl_act_set_motion_slot(self, 0, 6, 0);
                return;
            }
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8024A8EC - the "item use" handler: arms one of three motion/SE rows selected by `arg1`, then
 * ends the act through the matching `pl_act_enter` code. */
void fn_8024A8EC(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A54[arg1 * 2], lbl_805C4A54[arg1 * 2 + 1], 0);
        self->field_0x018 = 0;
        if (arg1 == 0) {
            self->field_0x5B8 = 0;
            self->field_0x5BA = 0;
            fn_802B8DF8(self);
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            switch (arg1) {
            case 0:
                pl_act_enter(self, 0, 0x23, 2);
                return;
            case 1:
                pl_act_enter(self, 0, 0x78, 2);
                return;
            case 2:
                pl_act_enter(self, 0, 0x77, 2);
                break;
            }
        }
        break;
    }
}

/* 0x8024AA04 - maps a motion id to the small class the act tables use: 0 for the idle pair, 1 for
 * 0x28, 2 for the two heavy ids. */
s32 fn_8024AA04(u16 motion) {
    s32 result = 0;

    switch (motion) {
    case 0x2F:
        result = 0;
        break;
    case 0x28:
        result = 1;
        break;
    case 0x109:
        result = 2;
        break;
    case 0x1B6:
        result = 2;
        break;
    }
    return result;
}

/* 0x8024A248 - the "ride/mount" handler: arms one of three motion sets from the actor's `+0x2`
 * kind, then either ends the act through `pl_act_enter` or pushes the rider along the mount's
 * rotation. */
void fn_8024A248(_PLW* self, s32 arg1) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        if ((u32)arg1 <= 1U) {
            fn_80277BC4(self, 0);
        } else {
            fn_80277BC4(self, 1);
        }
        fn_80277C50(self, 0);
        fn_80276B58(self, -0x96);
        fn_8027A17C(self);
        pl_act_set_flag(self, 0x1000);
        pl_act_arm_flags(self, 1);
        if (self->field_0x018 == 0) {
            Pl_chr_set_attr_default(self, 0x7B, 4, 0);
            return;
        }
        if (arg1 == 0 || arg1 == 2) {
            Pl_chr_set_attr_default(self, 0x426, 4, 0);
        } else {
            Pl_chr_set_attr_default(self, 0x427, 4, 0);
        }
        switch (self->field_0x002) {
        case 1:
            Pl_act_set_step_table(self, (u32)lbl_805C9118, 0);
            return;
        case 2:
            Pl_act_set_step_table(self, (u32)lbl_805CAD74, 0);
            return;
        case 8:
            if (fn_80331104() == 0) {
                Pl_act_set_step_table(self, (u32)lbl_805E2048, 0);
                return;
            }
            return;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 0 || self->field_0x018 == 1) {
                Pl_act_set_motion_slot(self, 3, 8, 0);
                return;
            }
            if (pl_act_param_tier_ck(self, 0) >= 2U) {
                if (pl_part_flag_ck(self, 0x25) == 1) {
                    pl_act_enter(self, 1, 9, 0x80);
                    return;
                }
                pl_act_enter(self, 1, 7, 0x80);
                return;
            }
            pl_act_enter(self, 1, 8, 0x80);
            return;
        }
        if (Pl_Skill_ck(self, 0xB6) == 1 && Pl_frame_check(self, 2, pl_frame_window_20, pl_float_zero) == 1) {
            vec.x = pl_float_zero;
            vec.y = pl_float_zero;
            if (arg1 == 0 || arg1 == 2) {
                vec.z = pl_frame_window_8;
            } else {
                vec.z = pl_float_neg8;
            }
            /* +0x054 is the actor's rotation; other units read its first word as a scalar, so the
             * `_CP_VECTOR` view is taken through the named field rather than a byte offset. */
            rotVecXYZ(&vec, (_CP_VECTOR*)&self->param_0x54);
            addVec3To(&self->field_0x03C, &vec);
        }
        break;
    }
}

/* 0x8024B35C - the two-step "fall" handler: arms the motion row for `arg1`, then ends the act
 * through the `Pl_act_set_motion_slot` or `pl_act_enter` code that `arg1` selects. */
void fn_8024B35C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A60[arg1], 0, 0);
        return;
    case 1:
        if ((u32)(arg1 - 3) > 1U) {
            if ((u32)(arg1 - 1) > 1U) {
                if (arg1 != 0) {
                    return;
                }
            } else {
                if (Pl_motion_end_ck(self) == 1) {
                    if (arg1 == 1) {
                        pl_act_enter(self, 0, 0x71, 2);
                        return;
                    }
                    pl_act_enter(self, 0, 0x72, 2);
                }
                return;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8024B46C - the sibling "fall" handler whose 0x72 exit is gated on a frame check. */
void fn_8024B46C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, lbl_805C4A6C[arg1], 0, 0);
        return;
    case 1:
        if ((u32)(arg1 - 3) > 1U && arg1 != 0) {
            switch (arg1) {
            case 1:
                if (Pl_motion_end_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x71, 2);
                    return;
                }
                break;
            case 2:
                if (Pl_frame_check(self, 1, pl_frame_window_80, pl_float_zero) == 1) {
                    pl_act_enter(self, 0, 0x72, 2);
                }
                break;
            }
        } else {
            if (Pl_motion_end_ck(self) == 1) {
                Pl_act_set_motion_slot(self, 0, 4, 0);
                return;
            }
            return;
        }
        return;
    }
}

/* 0x8024B594 - the "swim/dive" handler: arms one of two motion sets, then hands the actor's kind to
 * the motion table. */
void fn_8024B594(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (arg1 == 0) {
            Pl_act_set_motion(self, 0x8000, 0, 0);
            Pl_chr_set_attr_default(self, 0x29, 2, 0);
        } else {
            Pl_act_set_motion(self, 0x8003, 0, 0);
            Pl_chr_set_attr_default(self, 0x7E, 2, 0);
        }
        pl_act_set_frame_timer(self);
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 8, 0);
        }
        return;
    }
}

/* 0x8024B66C - the "evade" handler: one of three arming motions, then the frame-checked roll-out. */
void fn_8024B66C(_PLW* self, s32 arg1) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (arg1) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 7, 2, 0);
            return;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16C, 4, 0);
            return;
        case 2:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x135, 4, 0);
            return;
        }
        break;
    case 1:
        switch (arg1) {
        case 0:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x29, 0xC);
                    return;
                }
                Pl_chr_set_attr_default(self, 8, 6, 0);
                return;
            }
            return;
        case 1:
            if (Pl_frame_check(self, 0, pl_frame_window_30, pl_float_zero) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x99, 0xC);
                    return;
                }
                self->field_0x354 = pl_float_zero;
                return;
            }
            break;
        case 2:
            if (Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
                self->act_step_0x05++;
                if (Pl_master_ck(self) == 1) {
                    pl_act_enter(self, 0, 0x9C, 0xC);
                    return;
                }
                self->field_0x354 = pl_float_zero;
            }
            break;
        }
        break;
    }
}

/* 0x8024B868 - the "get-up from evade" handler: arms the motion, feeds the pending item back, then
 * waits 14 frames before handing over to `Pl_act_set_motion_slot`. */
void fn_8024B868(_PLW* self, s32 arg1) {
    s32 timer;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        switch (arg1) {
        case 0:
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 8, 6, 0);
            break;
        case 1:
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 0x16C, 0, 0x1E);
            break;
        case 2:
            Pl_act_set_motion(self, 1, 0, 0);
            Pl_chr_set_attr_default(self, 0x135, 0, 0x14);
            break;
        }
        self->field_0x28 = 0;
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_a(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
                return;
            }
        }
        return;
    case 1:
        if ((u32)(arg1 - 1) > 1U) {
            if (arg1 == 0) {
                timer = self->field_0x28 + 1;
                self->field_0x28 = timer;
                if (timer >= 0xE) {
                    self->act_step_0x05++;
                    Pl_chr_set_attr_default(self, 9, 0, 0);
                    return;
                }
            }
            return;
        }
        self->act_step_0x05++;
        return;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 2, 0);
        }
        break;
    }
}

/* 0x8024C75C - the "drink/item" handler: arms the item motion, refunds the pending item, and arms
 * the recovery scale when the skill is set. */
void fn_8024C75C(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 0x136, 0, 0x3C);
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_c(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x131, -4, 0xCE);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 0xA, 0);
        }
    }
}

/* 0x8024C878 - the sibling "drink" handler whose 0x7E exit is gated on a frame check. */
void fn_8024C878(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x16B, -8, 0);
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_208, pl_float_zero) == 1) {
            if (Pl_master_ck(self) == 1) {
                pl_act_enter(self, 0, 0x7E, 0xC);
                return;
            }
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        Pl_chr_set_attr_default(self, 0x16B, 0, 0xD0);
    }
}

/* 0x8024C96C - the "drink, standing" variant of 0x8024C75C. */
void fn_8024C96C(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 0x16B, 0, 0xD0);
        if (Pl_master_ck(self) == 1) {
            pl_act_net_hook_c(self);
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
            }
        }
        if (Pl_Skill_ck(self, 0xB8) == 1) {
            self->field_0x354 = pl_frame_window_2;
            return;
        }
        return;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 0xA, 0);
        }
    }
}

/* 0x8024CA50 - arms the recovery scale for the two "eat/drink" skills. */
void fn_8024CA50(_PLW* self, s32 arg1) {
    if (arg1 == 0) {
        if (Pl_Skill_ck(self, 0xB0) == 1) {
            self->field_0x354 = pl_float_1_7;
        }
    } else if (Pl_Skill_ck(self, 0xAF) == 1 || Pl_Skill_ck(self, 0xB0) == 1) {
        self->field_0x354 = pl_float_1_8;
    }
}

/* 0x8024CD8C - scales a `s16` amount by the skill's multiplier (0xA6 first, then 0xA7). */
s16 fn_8024CD8C(_PLW* self, s16 value) {
    s16 scaled = value;

    if (Pl_Skill_ck(self, 0xA6) == 1) {
        scaled = (s16)(pl_float_1_5 * (f32)scaled);
    } else if (Pl_Skill_ck(self, 0xA7) == 1) {
        scaled = (s16)(pl_float_0_66 * (f32)scaled);
    }
    return scaled;
}

/* 0x8024D3C8 - one-in-three chance the actor is staggered by `fn_80276CE8` when the skill is set. */
void fn_8024D3C8(_PLW* self) {
    if (Pl_master_ck(self) == 1 && Pl_Skill_ck(self, 0x49) == 1 && ran_suu(1) % 3 == 0) {
        fn_80276CE8(self, 0x96);
    }
}

/* 0x8024D5AC - the lobby-mode actor-match test: only in play mode, and only when the two actors
 * share their `+0x016` area byte. */
s32 fn_8024D5AC(_PLW* self, _PLW* other) {
    if (PlayMode_ck() != 2) {
        return 0;
    }
    if (other == NULL) {
        return 0;
    }
    return self->area_0x16 == other->area_0x16;
}
}

/* The frame windows this act cluster gates its frame checks on are this unit's *use* of the Pl band's
 * shared `.sdata2` pool; the declarations live in their owner's header (`Pl/pl_frame_data.h`, included
 * above) and the range is claimed by the data-only unit `Pl/pl_frame_data.cpp` (rule 2 + rule 12). */

/* Advances one step of the act the player entered from an off-hand gesture.  The act step byte
 * picks between the arming step and the per-skill follow-up steps; the two constants the follow-up
 * steps hand `Pl_frame_check` only differ per gesture, so each arm is written out in full. */
extern "C" void pl_act_step_offhand_gesture(_PLW* self, s32 part)
{
    s32 motion;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        if (part == 0) {
            Pl_act_set_motion(self, 0, 0, 0);
            Pl_chr_set_attr_default(self, 313, 4, 0);
        } else {
            Pl_act_set_motion(self, 3, 0, 0);
            Pl_chr_set_attr_default(self, 356, 4, 0);
        }
        self->field_0x018 = 0;
        pl_act_set_frame_timer(self);
        self->field_0x007 = 0;
        if (Pl_cat_skill_ck(self, 50) == 1) {
            s32 level = self->field_0x0B6 & 7;

            if (level == 0) {
                self->field_0x007 = 1;
            } else if (level <= 2) {
                self->field_0x007 = 2;
            } else {
                self->field_0x007 = 3;
            }
        }
        if (Pl_Skill_ck(self, 32) == 1) {
            self->field_0x007++;
        }
        break;
    case 1:
        if (Pl_Skill_ck(self, 31) == 1 || self->field_0x007 == 1) {
            if (Pl_frame_check(self, 0, pl_frame_window_96, pl_float_zero) == 1) {
                if (part == 0) {
                    Pl_chr_set_attr_default(self, 313, 0, 238);
                } else {
                    Pl_chr_set_attr_default(self, 356, 0, 238);
                }
            }
        } else if (self->field_0x006 == 0) {
            switch (self->field_0x007) {
            case 2:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, pl_frame_window_142, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, pl_frame_window_142, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 356, 0, 238);
                    }
                }
                break;
            case 3:
                if (part == 0) {
                    if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 313, 0, 238);
                    }
                } else {
                    if (Pl_frame_check(self, 0, pl_frame_window_190, pl_float_zero) == 1) {
                        self->field_0x006++;
                        Pl_chr_set_attr_default(self, 356, 0, 238);
                    }
                }
                break;
            case 0:
                if (Pl_Skill_ck(self, 32) == 1) {
                    if (part == 0) {
                        if (Pl_frame_check(self, 0, pl_frame_window_246, pl_float_zero) == 1) {
                            self->field_0x006++;
                            Pl_chr_set_attr_default(self, 313, 0, 200);
                        }
                    } else {
                        if (Pl_frame_check(self, 0, pl_frame_window_238, pl_float_zero) == 1) {
                            self->field_0x006++;
                            Pl_chr_set_attr_default(self, 356, 0, 198);
                        }
                    }
                }
                break;
            }
        }
        if (Pl_frame_check(self, 0, pl_frame_window_260, pl_float_zero) == 1) {
            if (Pl_motion_input_ck(1) == 0) {
                pl_item_add(self, self->field_0x306, -1);
                switch (self->field_0x306) {
                case 98:
                case 207:
                    motion = 150;
                    break;
                case 48:
                    motion = 100;
                    break;
                }
                fn_80278674(self, motion, 0);
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, self->kind_0x09, 4, 0);
        }
        break;
    }
}

/* Advances the act the player enters when it is caught by the pitfall state: arms the three motion
 * words from the work record's own frame data, then waits for the step's frame window (or for the
 * master action to finish) before handing the act to `fn_802F39DC`. */
extern "C" void pl_act_step_pitfall_arm(_PLW* self)
{
    u8 step;

    fn_802DE578_view1(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    step = self->act_step_0x05;
    switch (step) {
    case 0:
        self->act_step_0x05 = step + 1;
        Pl_chr_set_attr_default(self, 338, 4, 0);
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 2048);
        self->field_0x018 = 0;
        self->field_0x596 = 1;
        self->field_0x59A = 0;
        self->field_0x597 = 0;
        self->field_0x59C[0] = 0;
        self->field_0x59C[1] = 0;
        self->field_0x59C[2] = 0;
        break;
    case 1:
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0, 64, 0);
        } else if (Pl_frame_check(self, 0, pl_frame_window_104, pl_float_zero) == 1) {
            if (self->act_step_0x05 == 1) {
                self->act_step_0x05++;
                fn_802F39DC(self, 0);
            }
        }
        break;
    default:
        break;
    }
}

/* Advances the same pitfall act's follow-up: it hands the act back to the action the three armed
 * motion words select, and stops for the frame window that the work record's own data names. */
extern "C" void pl_act_step_pitfall_hold(_PLW* self)
{
    s32 i;

    fn_802DE578_view1(self, &self->field_0x598);
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        pl_act_set_flag(self, 2048);
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 339, 4, 0);
        break;
    case 1:
        if (self->field_0x59A != 0) {
            for (i = 0; i < 3; i++) {
                if ((self->field_0x59C[i] & 0x8000) != 0) {
                    pl_act_enter(self, 0, 154, 0);
                    return;
                }
            }
        }
        if (pl_part_flag_ck(self, 9) == 1) {
            if (self->field_0x59A != 0) {
                pl_act_enter(self, 0, 65, 0);
            } else if (self->field_0x596 == 0) {
                pl_act_enter(self, 0, 66, 0);
            } else {
                pl_act_enter(self, 0, 67, 0);
            }
        }
        break;
    }
}

/* Counts how many of the three motion words the work record still has armed: 1, or 2 when the first
 * two agree, plus one more when the third matches the first. */
extern "C" s32 pl_act_armed_motion_count(_PLW* self)
{
    s16 count = 1;

    if (self->field_0x59C[0] == self->field_0x59C[1]) {
        count = 2;
    }
    if (self->field_0x59C[0] == self->field_0x59C[2]) {
        count = count + 1;
    }
    return count;
}

/* Advances act 151's step: the arming step clears the actor mode, arms motion 113 with attribute
 * 2 and drops the act's two latch flags; the follow-up step hands the motion on once the model
 * reports it finished. */
extern "C" void pl_act_step_151(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 113, 2, 0);
        pl_act_clear_flag5bb(self);
        pl_act_clear_mode5c4(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* Arms the act's 15-frame `+0x028` timer together with motion 1 and attribute 8/10, then counts the
 * timer down while the master gate holds and hands the motion on when it reaches zero.  Tail-called
 * by `pl_act_step_arm_hold`. */
extern "C" void pl_act_step_timer_wait(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 15;
        Pl_act_set_motion(self, 1, 0, 0);
        Pl_chr_set_attr_default(self, 8, 10, 0);
        break;
    case 1:
        if (Pl_master_ck(self) != 0) {
            if (--self->field_0x28 <= 0) {
                Pl_act_set_motion_slot(self, 1, 4, 0);
            }
        }
        break;
    }
}

/* Picks the guard gauge band the cat-skill arm selects: skill 20 narrows it by one step, its absence
 * by two. */
extern "C" void pl_act_gauge_gate_by_skill(_PLW* self)
{
    if (Pl_cat_skill_ck(self, 20) == 1) {
        pl_act_gauge_gate(self, -1);
    } else {
        pl_act_gauge_gate(self, -2);
    }
}

/* Advances act 86's step: the arming step rotates the actor's +0x058/+0x0A8 angle pair by half a
 * turn (a `u16` wrap) and arms motion 329; the follow-up hands the act's motion on once the model
 * reports it finished. */
extern "C" void pl_act_step_86(_PLW* self)
{
    pl_act_set_step_time(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        self->field_0x058 = (u16)(self->field_0x058 + 0x8000);
        self->field_0x0A8 = self->field_0x058;
        Pl_chr_set_attr_default(self, 329, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        }
        break;
    }
}

/* Advances act 94's step: the arming step arms motion 360 with attribute 4, and the second step
 * waits out the master gate and the act's own 20-frame window before bumping the act's follow-up
 * stage and handing the motion on. */
extern "C" void pl_act_step_94(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        Pl_chr_set_attr_default(self, 360, 4, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1 && self->field_0x006 == 0) {
            if (Pl_frame_check(self, 0, pl_frame_window_20, pl_float_zero) == 1) {
                self->field_0x006++;
            }
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* Advances act 175's step: the arming step clears the actor mode, restarts the move work and arms
 * motion 307 with attribute 4, and the follow-up step hands the act its motion 1 once the model
 * reports the current one finished. */
extern "C" void pl_act_step_175(_PLW* self)
{
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 307, 4, 0);
        ef_move_state_dispatch(0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 1, 4, 0);
        }
        break;
    }
}

/* Advances acts 89 and 90's step: the arming step sets the actor's flag bit, arms motion 3, clears
 * the two rotation words and picks the motion attribute from the actor mode (102/117 clean, 1065/1067
 * when the mode byte is set), both keyed on the caller's part index; the follow-up hands the motion
 * on once the model reports it finished. */
extern "C" void pl_act_step_89(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        pl_act_arm_flags(self, 1);
        Pl_act_set_motion(self, 3, 0, 0);
        self->param_0x54 = 0;
        self->field_0x0AC = 0;
        if (self->field_0x018 == 0) {
            if (part == 0) {
                Pl_chr_set_attr_default(self, 102, 0, 0);
            } else {
                Pl_chr_set_attr_default(self, 117, 0, 0);
            }
        } else {
            if (part == 0) {
                Pl_chr_set_attr_default(self, 1065, 8, 0);
            } else {
                Pl_chr_set_attr_default(self, 1067, 8, 0);
            }
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 8, 0);
        }
        break;
    }
}

/* Advances act 121's step: the arming step restarts the actor's move work, arms motion 318 with
 * attribute -4 and starts the act's 60-frame `+0x028` countdown, which the function decrements on
 * every call; the second step waits out the act's 46-frame window and arms motion 345; the third
 * waits for the master gate with the countdown expired before handing the model state on, and hands
 * the motion on once the model reports it finished. */
extern "C" void pl_act_step_121(_PLW* self)
{
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    if (self->field_0x28 != 0) {
        self->field_0x28--;
    }
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 318, -4, 0);
        self->field_0x28 = 60;
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_frame_window_46, pl_float_zero) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 345, 0, 0);
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1 && self->field_0x28 == 0 && self->field_0x006 == 0) {
            self->field_0x006++;
            pl_model_state_set(self, 1, 25, 0);
        }
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    default:
        break;
    }
}

/* Advances acts 135 and 137's step: the arming step restarts the motion and arms attribute 320 with
 * the actor's flag word (or 324 alone for the second part); the follow-up waits out the act's
 * 44-frame window and its part flag before re-entering the act, and the second part counts the same
 * window up until the act's own two frames have passed. */
extern "C" void pl_act_step_135(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 0, 0);
        if (part == 0) {
            Pl_chr_set_attr_default(self, 320, 4, 0);
            pl_act_set_flag(self, 512);
        } else {
            Pl_chr_set_attr_default(self, 324, 4, 0);
        }
        break;
    case 1:
        switch (part) {
        case 0:
            if (self->field_0x006 >= 3 ||
                (Pl_frame_check(self, 1, pl_frame_window_44, pl_float_zero) == 1 &&
                 pl_part_flag_ck(self, 4) == 1)) {
                pl_act_enter(self, 0, 136, 0);
            } else if (Pl_frame_check(self, 1, pl_rig_get_float_a4(self), pl_float_zero) == 1) {
                self->field_0x006++;
            }
            break;
        case 1:
            if (Pl_frame_check(self, 1, pl_rig_get_float_a4(self), pl_float_zero) == 1) {
                if (++self->field_0x006 >= 2) {
                    pl_act_enter(self, 0, 138, 0);
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/* Advances the weapon act whose arming step carries attribute 112: the arming step arms motion 3
 * with flag 64 and copies the actor's second counter into its first; the follow-up step enters act
 * 8 out of its 56-frame window while the act's tier still reads 0, and the unmatched path falls
 * back to entering act 7 once the model reports the motion finished. */
extern "C" void pl_act_step_attr_112(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 0, 0);
        pl_act_set_flag(self, 64);
        self->field_0x058 = self->field_0x0A8;
        Pl_chr_set_attr_default(self, 112, 0, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (Pl_frame_check(self, 1, pl_frame_window_56, pl_float_zero) == 1 &&
                pl_act_param_tier_ck(self, 0) == 0) {
                pl_act_enter(self, 1, 8, 0);
            } else if (Pl_motion_end_ck(self) == 1) {
                pl_act_enter(self, 1, 7, 0);
            }
        } else if (Pl_frame_check(self, 1, pl_frame_window_56, pl_float_zero) == 1) {
            pl_act_enter(self, 1, 8, 0);
        }
        break;
    default:
        break;
    }
}

/* Resets the three guard timers the act cluster counts down: the two running timers to zero and the
 * third to its 30-frame window. */
extern "C" void pl_act_guard_timer_reset(_PLW* self)
{
    self->field_0x400 = 0;
    self->field_0x3FC = 0;
    self->field_0x402 = 30;
}

/* Advances act 84's step: restarts the actor's move work and re-arms the act's whole timer block -
 * the stagger timer, the stamina/guard pair, the two four-word runs and the act bitfield - then
 * hands the act to 85 once the model reports the motion finished. */
extern "C" void pl_act_step_84(_PLW* self)
{
    pl_model_set_state(self, 2);
    pl_act_set_step_time(self, 2);
    pl_act_set_gauge_arm(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 0, 0);
        Pl_chr_set_attr_default(self, 326, -6, 0);
        self->field_0x3EA = 0;
        pl_act_clear_wait(self, 0);
        self->field_0x3FC = 0;
        self->field_0x42E = 0;
        self->field_0x430 = 0;
        self->field_0x432 = 0;
        self->field_0x434 = 0;
        self->field_0x436 = 0;
        self->field_0x422 = 0;
        self->field_0x404 = 0;
        self->field_0x406 = 0;
        self->field_0x408 = 0;
        self->field_0x40A = 0;
        self->field_0x40C = 0;
        self->field_0x3DC = 0;
        self->field_0x410 = 0;
        self->field_0x3F4 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 0, 85, 0);
        }
        break;
    default:
        break;
    }
}

/* Steps this act's guard gauge by the caller's two part flags and clamps it into the act's window.
 * The first flag takes 1024 off the gauge unless the menu owns the screen, the second puts 1024
 * back, and neither leaves the gauge to walk back toward zero in 768 steps; the result is then
 * clamped to the caller's two limits, held in the same 16-bit wrap the field itself uses. */
extern "C" void pl_act_guard_gauge_adjust(_PLW* self, u16 neg_limit, u16 pos_limit, s32 flag_a,
                                          s32 flag_b)
{
    u32 gauge;

    if (pl_part_flag_ck(self, flag_a) == 1 && Pl_suimen_ck(self) == 0) {
        self->field_0x0AC -= 1024;
    } else if (pl_part_flag_ck(self, flag_b) == 1) {
        self->field_0x0AC += 1024;
    } else {
        gauge = self->field_0x0AC;
        if ((s16)gauge >= 0) {
            if (gauge >= 768) {
                self->field_0x0AC = gauge - 768;
            } else {
                self->field_0x0AC = 0;
            }
        } else {
            if (gauge >= 64768) {
                self->field_0x0AC = 0;
            } else {
                self->field_0x0AC = gauge + 768;
            }
        }
    }
    gauge = self->field_0x0AC;
    if ((s16)gauge < 0) {
        if ((u16)gauge <= neg_limit) {
            self->field_0x0AC = neg_limit;
        }
    } else if ((u16)gauge >= pos_limit) {
        self->field_0x0AC = pos_limit;
    }
}

/* Arms one motion of the caller's part, sets the act's low or high flag from the second argument,
 * marks the actor mode, and re-arms the act's flag set unless the caller says otherwise.  Shared by
 * the whole weapon-act band above 0x80255388, which is why its arguments stay generic. */
extern "C" void pl_act_arm_motion_and_flag(_PLW* self, u16 motion, u8 high_flag, u8 skip_arm)
{
    Pl_act_set_motion(self, motion, 0, 0);
    if (high_flag == 0) {
        pl_act_set_flag(self, 16);
    } else {
        pl_act_set_flag(self, 80);
    }
    self->field_0x018 = 1;
    if (skip_arm == 0) {
        pl_act_arm_flags(self, 1);
    }
}

/* Runs one repeat of the charge act the caller's part selects: the arming step picks the motion
 * (0 for the low parts, 3 for the high ones) and the attribute (211 or 263) together with the
 * repeat count the part sets at `+0x007`; the follow-up step waits out that many motion-end reports
 * before re-entering the act, and the last step hands the act's own motion slot on. */
extern "C" void pl_act_charge_repeat_step(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        switch (part) {
        case 0:
            self->field_0x007 = 1;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 1:
            self->field_0x007 = 2;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 2:
            self->field_0x007 = 4;
            Pl_act_set_motion(self, 0, 0, 1);
            Pl_chr_set_attr_default(self, 211, 4, 0);
            break;
        case 3:
            self->field_0x007 = 1;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        case 4:
            self->field_0x007 = 2;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        case 5:
            self->field_0x007 = 4;
            Pl_act_set_motion(self, 3, 0, 1);
            Pl_chr_set_attr_default(self, 263, 4, 0);
            break;
        default:
            break;
        }
        pl_act_set_frame_timer(self);
        self->field_0x28 = 0;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (++self->field_0x006 >= self->field_0x007) {
                self->act_step_0x05++;
                if ((u32)part <= 2) {
                    Pl_chr_set_attr_default(self, 212, 2, 0);
                } else {
                    Pl_chr_set_attr_default(self, 264, 2, 0);
                }
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, self->kind_0x09, 12, 0);
        }
        break;
    default:
        break;
    }
}

/* Arms one of the act's countdown motions: the arming step clears the actor mode, arms attribute 26
 * with its frame timer and the motion, and starts a 40-frame countdown for the first part (80 for
 * the others); the follow-up hands motion slot 12 on once the countdown runs out. */
extern "C" void pl_act_countdown_step(_PLW* self, s32 part)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x018 = 0;
        Pl_chr_set_attr_default(self, 26, 6, 0);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        if (part == 0) {
            self->field_0x28 = 40;
        } else {
            self->field_0x28 = 80;
        }
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            Pl_act_set_motion_slot(self, 0, 12, 0);
        }
        break;
    default:
        break;
    }
}

/* Advances the weapon act whose arming step carries attribute 1007: it arms that attribute, hands
 * the act's motion on through the shared part armer, and hands motion slot 2 on once the model
 * reports the motion finished. */
extern "C" void pl_act_step_attr_1007(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1007, 2, 0);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* The same three steps for the weapon act whose arming step carries attribute 1056, which arms
 * motion 3 instead of motion 0. */
extern "C" void pl_act_step_attr_1056(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1056, 2, 0);
        pl_act_arm_motion_and_flag(self, 3, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* And for the weapon act whose arming step carries attribute 1018: the follow-up does not arm the
 * act's own motion but enters the child act 5 instead. */
extern "C" void pl_act_step_attr_1018(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 1018, 2, 0);
        pl_act_arm_motion_and_flag(self, 0, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 5, 1, 0);
        }
        break;
    default:
        break;
    }
}

/* Drives one tick of the player's act state: it ages the shell timer the act belongs to, and
 * picks the next action code from the motion/act predicates when the timer runs out. */
extern "C" void fn_80258FCC(_PLW* self, s16 a2, s16 a3, u8 a4)
{
    s16* timer;

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (a3 == 0) {
        timer = &self->field_0x418;
    } else {
        timer = &self->field_0x41E;
    }
    if (a4 == 0) {
        if ((s8)self->field_0x444 == 0) {
            if (fn_8026F888(self) == 1) {
                if (*timer > 0) {
                    *timer -= (s16)10;
                    if (*timer < 0) {
                        *timer = 0;
                    }
                    self->field_0x444 = 1;
                }
            }
        } else {
            self->field_0x444--;
        }
    }
    if (*timer <= 0) {
        pl_act_clear_wait(self, 0);
        if (a3 == 0) {
            self->field_0x3D8 &= ~0x300;
            pl_act_enter(self, 6, 45, 0);
        } else {
            self->field_0x3D8 &= ~0x300;
            pl_act_enter(self, 6, 65, 0);
        }
        return;
    }
    if (a4 != 0) {
        return;
    }
    if (pl_part_flag_ck(self, 4) == 1) {
        if (self->field_0x378 >= 150) {
            self->field_0x0A8 = self->field_0x058;
            if (a3 == 0) {
                pl_act_enter(self, 6, 73, 4);
            } else {
                pl_act_enter(self, 6, 72, 4);
            }
            return;
        }
    }
    if (pl_act_param_tier_ck(self, 0) >= 1) {
        pl_act_set_cam_ang(self);
        if (pl_part_flag_ck(self, 0) == 1 && (s8)self->field_0x266 == 0) {
            if (a3 == 0) {
                if (Pl_act_ck(self, 6, 47) != 0) {
                    return;
                }
                pl_act_enter(self, 6, 47, 0);
            } else {
                if (Pl_act_ck(self, 6, 67) != 0) {
                    return;
                }
                pl_act_enter(self, 6, 67, 0);
            }
            return;
        }
        if (a3 == 0) {
            if (Pl_act_ck(self, 6, 46) != 0) {
                return;
            }
            if ((s8)self->field_0x266 != 0) {
                return;
            }
            pl_act_enter(self, 6, 46, 0);
        } else {
            if (Pl_act_ck(self, 6, 66) != 0) {
                return;
            }
            if ((s8)self->field_0x266 != 0) {
                return;
            }
            pl_act_enter(self, 6, 66, 0);
        }
        return;
    }
    if (fn_8026FE98((_ENEMY_WORK*)self, 0x1C0) == 0) {
        return;
    }
    if (a2 == 1 && self->field_0x28 != 0) {
        return;
    }
    if (a3 == 0) {
        pl_act_enter(self, 6, 44, 0);
    } else {
        pl_act_enter(self, 6, 64, 0);
    }
}

/* Advances the act's own three-step state and hands the tick to `fn_80258FCC` once the timer the
 * caller passed has been picked. */
extern "C" void fn_80259310(_PLW* self, s32 a2, s32 a3)
{
    u8 flag;

    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        flag = 0;
        self->field_0x18 = 0;
        pl_act_clear_flag5bb(self);
        Pl_chr_setX(self, 51, 4, 0);
        if (a2 == 0) {
            self->field_0x3F6 = 0;
            self->field_0x3F0 = 0;
            pl_act_guard_timer_reset(self);
            pl_act_clear_wait(self, 0);
            if (a3 == 0) {
                self->field_0x418 = 900;
            } else {
                self->field_0x41E = 450;
            }
            self->field_0x45A = 0;
            self->field_0x45C = 0;
            self->field_0x28 = 4;
        } else {
            pl_act_clear_wait(self, 1);
            self->field_0x28 = 4;
        }
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        Pl_act_set_step_table(self, (u32)lbl_805C4F5C, 0);
    case 1:
        self->field_0x28 = self->field_0x28 - 1;
        if (self->field_0x28 <= 0) {
            self->act_step_0x05++;
        }
        flag = 1;
        break;
    case 2:
        flag = 0;
        break;
    }
    if (Pl_master_ck(self) == 1) {
        fn_80258FCC(self, 0, a3, flag);
    }
}

/* Two-step act: arms the weapon pose and clears the act flag, then runs the tail predicate. */
extern "C" void fn_8025948C(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x18 = 0;
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        Pl_chr_set_attr_default(self, 307, 6, 0);
        pl_act_clear_flag5bb(self);
        self->field_0x354 = pl_float_1_7;
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_reenter(self, 0, 4, 0);
        }
        break;
    }
}

/* Two-step act: arms the timed pose, then per tick nudges the actor along the heading and counts the
 * timer down while `fn_80258FCC` runs. */
extern "C" void fn_8025953C(_PLW* self, s32 a2)
{
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 20;
        self->field_0x266 = 30;
        Pl_chr_set_attr_default(self, 52, 4, 0);
        pl_act_clear_flag5bb(self);
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        pl_act_set_flag(self, 0x40);
        Pl_act_set_step_table(self, (u32)lbl_805C4F5C, 0);
        return;
    case 1:
        v.x = pl_float_zero;
        v.y = pl_float_zero;
        v.z = pl_frame_window_2;
        rotVecY(&v, self->field_0x058);
        self->field_0x03C = self->field_0x03C + v.x;
        self->field_0x040 = self->field_0x040 + v.y;
        self->field_0x044 = self->field_0x044 + v.z;
        if (self->field_0x28 > 0) {
            self->field_0x28 = self->field_0x28 - 1;
        }
        fn_80258FCC(self, 1, a2, 0);
        return;
    }
}

/* Two-step act: arms the pose picked by the actor's SE name set, then per tick either steps to the
 * next action or nudges the actor along the heading while `fn_80258FCC` runs. */
extern "C" void fn_80259684(_PLW* self, s32 a2)
{
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 53, 4, 0);
        } else {
            Pl_chr_setX(self, 59, 4, 0);
        }
        pl_act_clear_flag5bb(self);
        self->field_0x28 = 20;
        self->field_0x266 = 30;
        pl_act_set_frame_timer(self);
        Pl_act_set_motion(self, 0, 0, 1);
        fn_8027D4F0(self);
        pl_act_set_flag(self, 0x180);
        Pl_act_set_step_table(self, (u32)lbl_805C4F5C, 0);
        return;
    case 1:
        if (Pl_master_ck(self) == 0) {
            return;
        }
        if (fn_80277DAC(self, 0, pl_frame_window_85, pl_float_neg60) != 0) {
            self->field_0x0A8 = (u16)self->field_0x058;
            if (a2 == 0) {
                pl_act_enter(self, 6, 48, 0);
                return;
            }
            pl_act_enter(self, 6, 68, 0);
            return;
        }
        v.x = pl_float_zero;
        v.y = pl_float_zero;
        v.z = pl_frame_window_5;
        rotVecY(&v, self->field_0x058);
        self->field_0x03C = self->field_0x03C + v.x;
        self->field_0x040 = self->field_0x040 + v.y;
        self->field_0x044 = self->field_0x044 + v.z;
        if (self->field_0x28 > 0) {
            self->field_0x28 = self->field_0x28 - 1;
        }
        fn_80258FCC(self, 1, a2, 0);
        return;
    }
}

/* Re-arms the act's 300-frame cooldown. */
extern "C" void fn_8025A7D4(_PLW* self)
{
    self->field_0x3A8 = 300;
}

/* Loads the act's strike budget: `a2` steps remain before the cooldown re-arms. */
extern "C" void fn_8025A7E0(_PLW* self, u8 a2)
{
    self->field_0x3A5 = a2;
    self->field_0x3A4 = (u8)a2;
    self->field_0x3A6 = 0;
    self->field_0x3A7 = 0;
    fn_8025A7D4(self);
}

/* Counts the act's strike window down and re-arms the cooldown once the per-skill strike budget is
 * spent; `field_0x309`'s 0x40 bit is the "struck" status the shell layer reads. */
extern "C" void fn_8025A7FC(_PLW* self)
{
    u8 threshold;

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s8)self->field_0x3A7 > 0) {
        self->field_0x3A7--;
    }
    if (Pl_cat_skill_ck(self, 48) == 1) {
        if (self->field_0x128 == 2) {
            threshold = 6;
        } else {
            threshold = 8;
        }
    } else {
        threshold = 15;
        if (self->field_0x128 == 2) {
            threshold = 12;
        }
    }
    if (self->field_0x3A8 > 0) {
        if (self->field_0x378 <= 0) {
            return;
        }
        if (fn_8026F888(self) != 1) {
            return;
        }
        if ((s8)self->field_0x3A7 != 0) {
            return;
        }
        self->field_0x3A6++;
        self->field_0x3A7 = 1;
        if (self->field_0x3A6 < threshold) {
            return;
        }
        self->field_0x3A6 = 0;
        self->field_0x309 |= 0x40;
        if (self->field_0x3A4 != 0) {
            self->field_0x3A4 = self->field_0x3A4 - 1;
        }
        fn_8025A7D4(self);
    } else {
        self->field_0x3A6 = 0;
        fn_8025A7D4(self);
    }
}

/* Ages the strike counter and saturates it at ten. */
extern "C" void fn_8025B0D4(_PLW* self)
{
    self->field_0x446++;
    if (self->field_0x446 >= 10) {
        self->field_0x446 = 10;
    }
}

/* Tests the actor's action number against the handful of actions the held-item window accepts. */
extern "C" u32 fn_8025E244(_PLW* self)
{
    u16 action;

    if (self->field_0x00A != 0) {
        return 0;
    }
    action = self->act_no;
    if ((u32)(action - 0x31) <= 3 || (u32)(action - 0x61) <= 2 || (u32)(action - 0x2C) <= 1 ||
        (s32)action == 0x7F) {
        return 1;
    }
    return 0;
}

/* Scans the `field_0x318` table for the entry whose id/key pair the caller asks for, honouring the
 * entry's two suppression flags. */
extern "C" u32 fn_8025E298(_PLW* self, u16 a2, u16 a3)
{
    PlScanEntry* entry;

    entry = (PlScanEntry*)self->field_0x318;
    if (entry == NULL || (s8)self->field_0x313 > 0) {
        return 0;
    }
    while (entry->id_0x00 != 0xFF) {
        if ((((entry->flags_0x06 & 0x8000) == 0) || ((entry->flags_0x0E & 0x4000) == 0) ||
             (s8)self->field_0x314 <= 0) &&
            entry->id_0x00 == a2 && entry->key_0x02 == a3) {
            return 1;
        }
        entry++;
    }
    return 0;
}

/* Arms the held-item stance when the actor stands still and the item pair is in the scan table. */
extern "C" u32 fn_8025E32C(_PLW* self)
{
    if (self->field_0x002 == 1) {
        if (self->field_0x308 != 0) {
            return 0;
        }
    } else if (self->field_0x18 == 0 && self->field_0x308 != 0) {
        return 0;
    }
    if ((fn_8025E298(self, 0, 0xB1) == 1 || fn_8025E298(self, 0xC, 0xB) == 1) &&
        fn_8027D8A0(self, 1) == 1) {
        self->field_0x3E5 = 0x10;
        self->field_0x3E4 = 0x10;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return 1;
    }
    if (fn_8025E298(self, 0, 0xA6) == 1 && fn_8027D8A0(self, 0) == 1) {
        self->field_0x3E5 = 0x10;
        self->field_0x3E4 = 0x10;
        self->field_0x3E7 = 0;
        self->field_0x3E8 = 0;
        return 1;
    }
    return 0;
}

/* Toggles the act's boolean latch. */
extern "C" void fn_8025ECF0(_PLW* self)
{
    self->field_0x5E6 = self->field_0x5E6 ^ 1;
}

/* 0x8025F478 - enters the water-surface (swim) state: reads three motion slots and latches the
 * resulting depth, state byte and the two water flags. */
void fn_8025F478(_PLW* self) {
    VEC3 work;
    f32 value;

    fn_8012A624(&work);
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        self->kind_0x09 = 0;
        return;
    }
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        self->kind_0x09 = 3;
        if (fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
            self->field_0x064 = value;
            self->field_0x074 = 1;
            if (self->vec_0x03C.y >= value - pl_frame_window_30) {
                self->vec_0x03C.y = value - pl_frame_window_30;
            }
            if (Pl_suimen_ck(self) == 1) {
                self->field_0x075 = 1;
            } else {
                self->field_0x075 = 0;
            }
        } else {
            self->field_0x074 = 0;
        }
    }
}

/* 0x80260198 - looks for an armed entry in the actor's own chunk of the move-work table and adopts
 * its position pair as the two damage counters. */
s32 fn_80260198(_PLW* self) {
    s32 i;

    if ((self->field_0x364 & 0xE0000007) != 0) {
        for (i = 0; i < 10; i++) {
            PlMoveEntry* entry = &pl_move_work[self->chunk_ofs][i];
            u16 kind = entry->kind;
            u32 value;

            if (kind == 0) {
                break;
            }
            value = fn_80050A40(entry->x_0x08, entry->z_0x10, pl_float_zero, pl_float_zero);
            if ((kind & 2) != 0) {
                self->field_0x058 = value;
                self->field_0x0A8 = value;
                return 1;
            }
        }
    }
    return 0;
}

/* 0x80260248 - advances the actor's signed 16-bit lean value by `delta`, clamps it to +/-100 and
 * zeroes it when `mode` 1 asks for the sign to be cancelled. */
void fn_80260248(_PLW* self, s32 mode, s16 delta) {
    s16 old = self->field_0x648;

    self->field_0x648 += delta;
    if (self->field_0x648 <= -100) {
        self->field_0x648 = -100;
    } else if (self->field_0x648 >= 100) {
        self->field_0x648 = 100;
    }
    if (mode != 1) {
        return;
    }
    if ((s32)old * (s32)self->field_0x648 < 0) {
        self->field_0x648 = 0;
    }
}

/* 0x8026077C - whether the two state bytes disagree while the game is outside play mode 3. */
s32 fn_8026077C(u8 a, u8 b) {
    if (a == b || (u8)PlayMode_ck() != 3) {
        return 1;
    }
    return 0;
}

/* 0x8026099C - runs the actor's per-frame body for the current mode byte, when the slot is live. */
void fn_8026099C(_PLW* self) {
    if (self->slot_active == 0) {
        return;
    }
    switch (self->field_0x004) {
    default:
        break;
    case 0:
        fn_8024676C();
        break;
    case 1:
        fn_8026F7B4();
        if ((u8)fn_803C482C() == 0 && event_demo_ck() == 0) {
            fn_8025F088(self);
        }
        break;
    }
}

/* 0x80260A18 - whether the actor's skill 382 is active. */
s16 fn_80260A18(_PLW* self) {
    s16 active = 0;

    if (Pl_item_timer_get(self, 382) > 0) {
        active = 1;
    }
    return active;
}

/* 0x80260A58 - the gate that keeps the actor out of the skill-382 action. */
s32 fn_80260A58(_PLW* self) {
    if (self->field_0x3EA > 0) {
        return 0;
    }
    if (fn_80278578(self, 2) == 0) {
        return 0;
    }
    if (fn_8027D40C(self) > 0) {
        return 0;
    }
    if ((self->field_0x5A4 & 0x200) != 0 && Pl_cat_skill_ck(self, 47) == 0) {
        return 0;
    }
    if (self->field_0x404 > 0) {
        return 0;
    }
    if (fn_80276800(self, 0) == 1) {
        return 0;
    }
    return fn_80260A18(self) <= 0;
}

/* 0x8025FF0C - computes the action flag word for the current state and hands it to the action layer. */
void fn_8025FF0C(_PLW* self, u8 mode) {
    u16 flags = 0x102;

    switch (self->kind_0x09) {
    default:
        if (fn_8026FE98((_ENEMY_WORK*)self, 4544) == 0) {
            flags |= 0x40;
        } else if (self->field_0x00A != 0) {
            flags |= 0xC0;
        } else if ((u32)(self->act_no - 128) <= 6 || (u32)(self->act_no - 15) <= 1 ||
                   self->act_no == 123 || self->act_no == 139 || self->act_no == 182) {
            flags |= 0x40;
        } else {
            flags |= 0xC0;
        }
        break;
    case 2:
        if (self->field_0x00A == 6) {
            flags |= 0x40;
        } else if (Pl_act_ck(self, 0, 62) == 1) {
            flags |= 0x40;
        } else {
            flags |= 0xC0;
        }
        break;
    case 3:
        if (fn_8026FE98((_ENEMY_WORK*)self, 4544) == 0) {
            flags |= 0x80;
        } else if (self->field_0x00A != 0) {
            flags |= 0xC0;
        } else if ((u32)(self->act_no - 140) <= 9 || (u32)(self->act_no - 25) <= 1 ||
                   self->act_no == 124) {
            flags |= 0x80;
        } else if (self->act_no == 27) {
            if (self->field_0x002 == 3 && self->field_0x18 == 1) {
                flags |= 0x80;
            } else {
                flags |= 0xC0;
            }
        } else {
            flags |= 0xC0;
        }
        break;
    }
    if (self->field_0x00A == 0 &&
        ((u32)(self->act_no - 10) <= 4 || (u32)(self->act_no - 74) <= 4 ||
         (u32)(self->act_no - 102) <= 3)) {
        flags &= ~0x100;
    }
    fn_802950D8(self, mode, flags);

    self->field_0x369 = 0;
    if (self->field_0x364 != 0) {
        s32 i;

        for (i = 0; i < 10; i++) {
            u16 kind = pl_move_work[self->chunk_ofs][i].kind;

            if (kind == 0) {
                break;
            }
            if ((kind & 2) != 0 && (self->field_0x364 & 0xE0000007) != 0) {
                self->field_0x369 = 1;
            }
        }
    }
}

/* 0x802607C4 - steers the actor's stored angle toward the nearest armed move-work slot's bearing. */
void fn_802607C4(_PLW* self) {
    PlMoveWork* best_work;
    s32 found = 0;

    if ((u16)Get_motion_no(self) == 38) {
        f32 best = pl_float_2250000;
        PlMoveWork* work = (PlMoveWork*)get_move_work_adrs(3);
        u16 count = (u16)get_move_work_max(3);
        s32 i;

        for (i = 0; i < count; i++) {
            if (work->field_0x000 != 0 && work->field_0x001 != 0 &&
                fn_8026077C(self->area_0x16, work->field_0x1E1) != 0) {
                f32 distance = fn_80050EAC(&self->vec_0x03C, &work->pos_0x188);

                if (best >= distance) {
                    best = distance;
                    best_work = work;
                    found = 1;
                }
            }
            work++;
        }
    }
    if (found == 0) {
        u16 value = self->field_0x0B2;

        if (value == 0) {
            return;
        }
        if ((u32)(value + 1024) <= 2048) {
            self->field_0x0B2 = 0;
        } else if ((s16)value >= 0) {
            self->field_0x0B2 = value - 1024;
        } else {
            self->field_0x0B2 = value + 1024;
        }
        return;
    }
    {
        s32 diff = ((s32)(u16)calcVecAng2(&self->vec_0x03C, &best_work->pos_0x188) + 0x10000) -
                   (s32)self->field_0x058;
        u16 current = self->field_0x0B2;

        if ((u16)diff >= 32768) {
            s32 target;

            if ((u16)diff < 54614) {
                diff = 54614;
            }
            target = current - 2048;
            if (current >= 32768) {
                if (target > (s32)(u16)diff) {
                    self->field_0x0B2 = target;
                } else {
                    self->field_0x0B2 = diff;
                }
            } else {
                self->field_0x0B2 = target;
            }
        } else {
            s32 target;

            if ((u16)diff > 10923) {
                diff = 10923;
            }
            target = current + 2048;
            if (current < 32768) {
                if (target < (s32)(u16)diff) {
                    self->field_0x0B2 = target;
                } else {
                    self->field_0x0B2 = diff;
                }
            } else {
                self->field_0x0B2 = target;
            }
        }
    }
}

/* 0x8025FA00 - the second half of the player's water-surface state machine (the entry/exit side). */
void fn_8025FA00(_PLW* self) {
    VEC3 work;
    f32 value;

    fn_8012A624(&work);
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            break;
        case 3:
            if (Pl_act_ck(self, 1, 21) == 1) {
                if (self->vec_0x03C.y < self->ground_y_0x060) {
                    self->vec_0x03C.y = self->ground_y_0x060;
                }
            } else if (Pl_master_ck(self) == 1 && self->field_0x00A != 8) {
                pl_act_enter(self, 1, 21, 12);
            }
            break;
        default:
            if (self->field_0x585 != 0) {
                break;
            }
            if (self->vec_0x03C.y - value <= pl_frame_window_30) {
                self->vec_0x03C.y = value;
                self->field_0x585 = 0;
                if (fn_8027DCA8(self) == 1 || (self->field_0x5A4 & 0x1000) == 0) {
                    break;
                }
                if (self->field_0x00A == 8) {
                    self->field_0x06C -= pl_frame_window_2;
                    if (self->field_0x06C < pl_float_neg250) {
                        self->field_0x06C = pl_float_neg250;
                    } else {
                        self->vec_0x03C.z += pl_frame_window_10;
                    }
                } else if (Pl_master_ck(self) == 1 && self->field_0x370 <= 0) {
                    fn_80278BE4(self);
                    return;
                } else {
                    pl_act_enter(self, 6, 52, 0);
                }
                break;
            }
            if (self->field_0x370 > 0) {
                self->kind_0x09 = 2;
                if (self->field_0x00A != 12) {
                    pl_act_enter(self, 2, 1, 0);
                } else {
                    pl_act_enter(self, 12, 6, 0);
                }
            } else if (self->field_0x00A == 8) {
                self->vec_0x03C.y = value;
            }
            break;
        }
    } else if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if (Pl_master_ck(self) != 0 &&
                fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                self->field_0x064 = value;
                self->field_0x074 = 1;
                if (self->vec_0x03C.y <= value && fn_8027D7EC(self, 0) == 0) {
                    self->vec_0x03C.y = self->field_0x064;
                    self->kind_0x09 = 3;
                    if (Pl_master_ck(self) == 1 && self->field_0x370 <= 0) {
                        fn_80278BE4(self);
                    } else {
                        pl_act_enter(self, 0, 151, 12);
                    }
                }
            }
            break;
        case 3:
            if (Pl_act_ck(self, 1, 21) == 1) {
                if (self->vec_0x03C.y < self->ground_y_0x060) {
                    self->vec_0x03C.y = self->ground_y_0x060;
                }
            } else if (Pl_act_ck(self, 0, 151) == 0 &&
                       self->vec_0x03C.y < pl_frame_window_100 + self->ground_y_0x060) {
                self->vec_0x03C.y = pl_frame_window_100 + self->ground_y_0x060;
            }
            break;
        default:
            if (Pl_master_ck(self) != 0 &&
                fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                self->field_0x064 = value;
                self->field_0x074 = 1;
                if (self->vec_0x03C.y <= value) {
                    if (self->field_0x00A != 8) {
                        self->kind_0x09 = 3;
                        pl_act_enter(self, 1, 20, 12);
                    }
                } else if (self->field_0x370 > 0) {
                    self->kind_0x09 = 2;
                    if (self->field_0x00A != 12) {
                        pl_act_enter(self, 2, 1, 0);
                    } else {
                        pl_act_enter(self, 12, 6, 0);
                    }
                }
            }
            break;
        }
    }
    if (fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
        self->field_0x064 = value;
        if (self->kind_0x09 == 3) {
            self->field_0x074 = 1;
            if (self->field_0x00A == 0) {
                if ((u32)(self->act_no - 74) > 3 && (self->act_no < 104 || self->act_no > 105) &&
                    self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                    self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
                }
            } else if (self->field_0x00A == 1) {
                if (self->act_no != 21 &&
                    self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                    self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
                }
            } else if (self->vec_0x03C.y >= self->field_0x064 - pl_frame_window_30) {
                self->vec_0x03C.y = self->field_0x064 - pl_frame_window_30;
            }
            if (Pl_suimen_ck(self) == 1) {
                self->field_0x075 = 1;
            } else {
                self->field_0x075 = 0;
            }
        }
    } else if (self->kind_0x09 == 3) {
        self->field_0x074 = 0;
    }
}

/* 0x8025F588 - the player's water-surface state machine: reads the swim motion slots and drives the
 * actor between the standing / swimming / diving kinds. */
s32 fn_8025F588(_PLW* self) {
    VEC3 work;
    f32 value;

    fn_8012A624(&work);
    if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, -6) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if ((self->field_0x5C4 & 0xF) != 0) {
                self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                pl_act_enter_raw(self, 12, 6, 0);
                return 1;
            }
            if (fn_80278C7C(self) == 1) {
                pl_act_enter_raw(self, 6, 49, 0);
                return 1;
            }
            if (fn_80278CD0(self) == 1) {
                pl_act_enter_raw(self, 6, 69, 0);
                return 1;
            }
            self->kind_0x09 = 0;
            break;
        case 3:
            self->kind_0x09 = 0;
            break;
        default:
            if (self->vec_0x03C.y - value > pl_frame_window_30) {
                if (self->field_0x370 > 0) {
                    self->kind_0x09 = 2;
                    if ((self->field_0x5C4 & 0xF) != 0) {
                        self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                        pl_act_enter_raw(self, 12, 6, 0);
                        return 1;
                    }
                    if (fn_80278C7C(self) == 1) {
                        pl_act_enter_raw(self, 6, 49, 0);
                        return 1;
                    }
                    if (fn_80278CD0(self) == 1) {
                        pl_act_enter_raw(self, 6, 69, 0);
                        return 1;
                    }
                    pl_act_enter(self, 2, 1, 0);
                    return 1;
                }
                if (self->field_0x00A == 8) {
                    self->vec_0x03C.y = value;
                }
                pl_act_enter(self, 2, 1, 0);
            }
            break;
        }
    } else if (fn_802919FC(self, &self->vec_0x03C, &self->field_0x5A4, &value, 1) == 1) {
        self->ground_y_0x060 = value;
        switch (self->kind_0x09) {
        case 2:
            if (fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
                if (self->vec_0x03C.y <= value) {
                    self->kind_0x09 = 3;
                } else {
                    if ((self->field_0x5C4 & 0xF) != 0) {
                        self->field_0x5C4 = (self->field_0x5C4 & 0xF0) | 5;
                        pl_act_enter_raw(self, 12, 6, 0);
                        return 1;
                    }
                    if (fn_80278C7C(self) == 1) {
                        pl_act_enter_raw(self, 6, 49, 0);
                        return 1;
                    }
                    if (fn_80278CD0(self) == 1) {
                        pl_act_enter_raw(self, 6, 69, 0);
                        return 1;
                    }
                }
            }
            break;
        default:
            self->kind_0x09 = 3;
            break;
        case 3:
            break;
        }
    }
    if ((u8)(self->kind_0x09 - 2) <= 1 &&
        fn_802919FC(self, &self->vec_0x03C, &work, &value, 4) == 1) {
        self->field_0x064 = value;
        self->field_0x074 = 1;
        if (self->vec_0x03C.y >= pl_frame_window_200 + value) {
            self->field_0x075 = 0;
            self->kind_0x09 = 2;
            if ((self->field_0x5C4 & 0xF) != 0) {
                pl_act_enter(self, 12, 6, 0);
            } else if (fn_80278C7C(self) == 1) {
                pl_act_enter_raw(self, 6, 49, 0);
            } else if (fn_80278CD0(self) == 1) {
                pl_act_enter_raw(self, 6, 69, 0);
            } else {
                pl_act_enter(self, 2, 1, 0);
            }
            return 1;
        }
        if (self->vec_0x03C.y >= value - pl_frame_window_30) {
            self->vec_0x03C.y = value - pl_frame_window_30;
        }
        if (Pl_suimen_ck(self) == 1) {
            self->field_0x075 = 1;
        } else {
            self->field_0x075 = 0;
        }
    } else if ((u8)(self->kind_0x09 - 2) <= 1) {
        self->field_0x074 = 0;
    }
    return 0;
}

/* 0x80262688 - adopts the nearest live player work as the actor's held item when the item-data kind
 * allows it, and reports whether one was adopted. */
s32 fn_80262688(_PLW* self) {
    _PLW* work;
    u16 count;
    s32 i;

    if (self->field_0x658 != 0) {
        return 0;
    }
    work = (_PLW*)get_move_work_adrs(2);
    count = (u16)get_move_work_max(2);
    for (i = 0; i < count; i++, work++) {
        if (work->slot_active != 0 &&
            (isServerSelectState() == 1 ? Pl_master_ck(work) != 1 : work->chunk_ofs != self->chunk_ofs) &&
            (work->field_0x655 == self->chunk_ofs || work->field_0x655 == 0xFF) &&
            (Pl_act_ck(work, 0, 20) != 0 || Pl_act_ck(work, 0, 158) != 0) &&
            self->area_0x16 == work->area_0x16 &&
            fn_80050EF4(&self->vec_0x03C, &work->vec_0x03C) >= pl_float_300 &&
            ((PlItemData*)GetItemData(work->field_0x650))->field_0x01 < 3) {
            self->field_0x658 = 90;
            if (fn_80273228(self, work->field_0x650, work->field_0x652) < work->field_0x652) {
                if ((u16)fn_80273044(self, work->field_0x650) == 0xFFFF) {
                    pl_model_state_set(self, 1, 0, 0);
                } else {
                    pl_model_state_set(self, 2, 30, work->field_0x650);
                }
                return 0;
            }
            self->field_0x650 = work->field_0x650;
            self->field_0x652 = work->field_0x652;
            if (self->kind_0x09 == 3) {
                pl_act_enter(self, 0, 161, 0);
            } else {
                pl_act_enter(self, 0, 91, 0);
            }
            if (isServerSelectState() == 1) {
                if (Pl_master_ck(self) == 1) {
                    Pl_net_send(self, 7, (u16)work->chunk_ofs);
                }
            } else {
                pl_item_add(work, work->field_0x650, (s16)(-work->field_0x652));
                work->field_0x656 = 0xFF;
                pl_item_add(self, self->field_0x650, self->field_0x652);
                self->field_0x656 = 1;
                pl_model_state_set(self, 2, 29, self->field_0x650);
                if ((u16)item_se_ck(self->field_0x650) == 1) {
                    se_slot_req(4);
                }
            }
            return 1;
        }
    }
    return 0;
}

/* 0x8025F088 - the actor's per-frame reset/update: latches the state bytes, runs the sub-updates and
 * pushes the body transform into the model layer. */
void fn_8025F088(_PLW* self) {
    MHchar* chr = &((PlBodyWork*)self->physics_0x13C)->chr_0x04;

    get_move_work_adrs(0);
    copyVec3(&self->vec_0x048, &self->vec_0x03C);

    self->field_0x65C = self->field_0x65D;
    if (self->field_0x017 != self->area_0x16) {
        self->field_0x65C |= 1;
    }
    self->field_0x65D = 0;
    self->field_0x01B = 0;
    self->field_0x017 = self->area_0x16;
    self->field_0x076 = self->field_0x075;
    self->field_0x276 = 0;
    self->field_0x3DC &= ~0x100000;

    fn_80260B38(self);
    self->field_0x309 = 0;
    self->field_0x657 = 0;
    self->field_0x30A = 0;
    if (Pl_master_ck(self) == 1 && fn_8027E1E4(self) == 0) {
        fn_8025E0C8(self);
        fn_80279C20(self);
    }
    self->field_0x46C = self->field_0x46D;
    if (Pl_master_ck(self) == 1) {
        if (fn_80131934(self->chunk_ofs) == 1) {
            self->field_0x46C = 1;
        } else {
            self->field_0x46C = 0;
            self->field_0x46E = 0;
        }
        if (Pl_act_ck(self, 2, 1) == 0 && Pl_act_ck(self, 0, 6) == 0) {
            self->field_0x5C7 = 0;
        }
    } else {
        self->field_0x46C = 0;
        self->field_0x46D = 0;
        self->field_0x46E = 0;
        self->field_0x5C7 = 0;
    }
    fn_8025EC58(self);
    if (fn_8026FE44(self) == 1) {
        fn_802872E4(self);
    }
    self->field_0x3B4 = 0;
    self->field_0x3B5 = 0;
    self->field_0x5C9 = 0;
    if (fn_802657F8(self) == 0) {
        fn_80262940(self);
    }
    fn_8027035C(self);
    fn_80270728(self);
    fn_80270CA4(self);
    fn_802642D0(self);
    self->field_0x3B0 = self->field_0x3AC;
    self->field_0x3AC = 0;
    self->act_state_0x00E[0] = 0;
    self->field_0x654 = 0;
    self->field_0x268 = 0;
    fn_8025DE38(self);
    if (self->act_state_0x00E[0] != 0) {
        self->act_state_0x00E[0] = 0;
        fn_8025DE38(self);
    }
    fn_8025E448(self);
    if (self->field_0x30A == 0xFF) {
        self->field_0x30A = 0;
    } else if (fn_8027AC18(self) == 0 &&
               (fn_8025E298_view3(self, 128, 255) == 1 || fn_8025E298_view3(self, 137, 248) == 1 ||
                fn_8025E298_view3(self, 137, 247) == 1) &&
               self->field_0x00A != 7 && self->field_0x00A != 12) {
        self->field_0x30A = 1;
    }
    self->field_0x5C8 = 0;
    fn_80278D1C(self);
    fn_80277EC0(self);
    fn_802602A0(self);
    fn_8027AF34(self);
    fn_802607C4(self);
    fn_8026FD0C(self);
    fn_8025ED00(self);
    fn_80224AC4(self->physics_0x13C);
    copyVec3(&self->vec_0x03C, &chr->pos_0x04);
    switch (self->kind_0x09) {
    case 2:
        break;
    case 3:
        if (self->vec_0x03C.y < pl_float_110 + self->ground_y_0x060) {
            fn_800524C0(pl_float_0_6, &self->vec_0x048, &self->vec_0x03C, &self->field_0x5AC,
                        &self->vec_0x03C);
        }
        break;
    default:
        if (self->field_0x01B == 0) {
            fn_800524C0(pl_float_0_3, &self->vec_0x048, &self->vec_0x03C, &self->field_0x5AC,
                        &self->vec_0x03C);
        }
        break;
    }
    fn_8025FF0C(self, 0);
    fn_8025FA00(self);
    if (Pl_act_ck(self, 0, 20) == 1 || Pl_act_ck(self, 0, 158) == 1) {
        self->field_0x268 = 1;
    }
    fn_8026FD0C(self);
    fn_800E0914(chr);
    if (fn_8026FD94(self) == 0) {
        fn_8027D4F0(self);
    }
    hit_attack_list_push(&self->field_0x484);
    hit_attack_list_push(&self->field_0x4E0);
}

/* 0x80264274 - adds `value` to the actor's hit-stop timer unless the skill overrides it. */
void fn_80264274(_PLW* self, s16 value) {
    if (Pl_Skill_ck(self, 1) == 0) {
        self->field_0x3EA += value;
        return;
    }
    self->field_0x3EA = 0;
}

/* 0x80264940 - accumulates a u16 into the two damage counters it feeds. */
void fn_80264940(_PLW* self, u32 value) {
    u16 sum = self->field_0x398 + (u16)value;

    self->field_0x058 = sum;
    self->field_0x0A8 = sum;
}

/* 0x80264EA4 - the actor's stamina/guard timer has not run out yet. */
s32 fn_80264EA4(_PLW* self) {
    return self->field_0x3FC >= 50;
}

/* 0x80264EC0 - the actor's poison timer is running. */
s32 fn_80264EC0(_PLW* self) {
    return self->field_0x416 > 0;
}

/* 0x80264ED4 - the actor's bleed timer is running. */
s32 fn_80264ED4(_PLW* self) {
    return self->field_0x41C > 0;
}

/* 0x80264EE8 - the "konchu ball" rolling state predicate. */
s32 fn_80264EE8(_PLW* self) {
    u16 motion;

    if (Pl_Skill_ck(self, 0xB2) != 1 && Pl_Skill_ck(self, 0xB3) != 1 && Pl_cat_skill_ck(self, 0x23) != 1) {
        return 0;
    }
    if (self->field_0x00A != 0) {
        return 0;
    }
    motion = self->act_no;
    if ((u32)(motion - 0x33) <= 1 || motion == 0x61) {
        return 1;
    }
    return 0;
}

/* 0x80264F78 - the "rolled up into a ball" predicate. */
s32 fn_80264F78(_PLW* self) {
    if ((Pl_cat_skill_ck(self, 0x21) == 1 || Pl_cat_skill_ck(self, 0x22) == 1) && fn_8027D7EC(self, 0) == 1 &&
        self->field_0x370 > 0) {
        return 1;
    }
    return 0;
}

/* 0x80264FF0 - the sleeping/paralysed state predicate. */
s32 fn_80264FF0(_PLW* self) {
    if ((Pl_cat_skill_ck(self, 0x17) == 1 || Pl_cat_skill_ck(self, 0x18) == 1) && fn_8027BCE0(self) != 0xFFFF) {
        return 1;
    }
    return 0;
}

/* 0x8026505C - the actor is held/knocked down by any of the four status sources. */
s32 fn_8026505C(_PLW* self) {
    if (self->field_0x45A > 0) {
        return 1;
    }
    if (self->field_0x45C > 0) {
        return 1;
    }
    if (fn_80264EE8(self) == 1) {
        return 1;
    }
    if (fn_80264F78(self) == 1) {
        return 1;
    }
    return fn_80264FF0(self) == 1;
}

/* 0x802650EC - maps a motion id onto "the actor is locked in this motion". */
s32 fn_802650EC(_PLW* self, u8 motion) {
    switch (motion) {
    case 6:
    case 7:
    case 8:
        if (fn_8026505C(self) == 0) {
            return 0;
        }
        if (fn_80264EE8(self) == 1 && self->field_0x45A <= 0 && self->field_0x45C <= 0) {
            return 0;
        }
        return 1;
    case 1:
    case 17:
    case 18:
    case 24:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        return fn_8026505C(self) != 0;
    default:
        return 0;
    }
}

/* 0x80265348 - the actor is in the "konchu ball bounce" motion pair. */
s32 fn_80265348(_PLW* self) {
    if (self->field_0x00A == 6 && (u32)(self->act_no - 0x1F) <= 3) {
        return 1;
    }
    return 0;
}

/* 0x802656EC - clears the two shell timers. */
void fn_802656EC(_PLW* self) {
    self->field_0x418 = 0;
    self->field_0x41E = 0;
}

/* 0x802656FC - the actor is in one of the "charge up a shot" motions. */
s32 fn_802656FC(_PLW* self) {
    u16 motion;

    if (self->field_0x00A != 5) {
        return 0;
    }
    motion = self->act_no;
    if ((u32)(motion - 3) <= 2 || (u32)(motion - 9) <= 2 || (u32)(motion - 16) <= 1) {
        return 1;
    }
    return 0;
}

/* 0x80265748 - the shell/element gauge is at or past its first charge step. */
u32 fn_80265748(_PLW* self) {
    u16 gauge = self->field_0x39C;

    if ((u16)(gauge + 0x1555) > 0x2AAB) {
        return (((u32)(gauge - (u16)0x8001) >> 31) + 1);
    }
    return 0;
}

/* 0x80265780 - fires the "draw the weapon" action request. */
void fn_80265780(_PLW* self) {
    if (self->kind_0x09 == 3) {
        pl_act_enter(self, 6, 0x22, 0);
        return;
    }
    pl_act_enter(self, 6, 0x20, 0);
}

/* 0x8026495C - the actor's ball-spin charge level after the skill adjustments. */
u8 fn_8026495C(_PLW* self) {
    u8 level = self->field_0x3A1;

    if (Pl_Skill_ck(self, 0x22) == 1) {
        if (level > 0x15) {
            level -= 0x14;
        } else {
            level = 1;
        }
    } else if (Pl_Skill_ck(self, 0x21) == 1 || fn_8026FE98((_ENEMY_WORK*)self, 0x2000) != 0) {
        if (level > 0xB) {
            level -= 0xA;
        } else {
            level = 1;
        }
    } else if (Pl_Skill_ck(self, 0x23) == 1) {
        level = (level < 0xF5) ? level + 0xA : 0xFF;
    }
    return level;
}

/* 0x80264A28 - converts the ball-spin charge level into the motion's knock-back distance. */
void fn_80264A28(_PLW* self) {
    u8 level = fn_8026495C(self);
    s16 distance = -0x96;

    if (level >= 0x28) {
        distance = -0x168;
    } else if (level >= 0xF) {
        distance = -0xC8;
    }
    fn_80276B58(self, distance);
}

/* 0x80264A84 - the motion's charge tier for the actor's current state. */
s32 fn_80264A84(_PLW* self) {
    u8 level = fn_8026495C(self);
    s32 tier = 0;

    switch ((u32)(self->field_0x002 - 4) <= 2 ? 1 : self->field_0x002) {
    case 0:
        if (level >= 0x28) {
            tier = 2;
        } else if (level >= 0xF) {
            tier = 1;
        }
        break;
    case 1:
        if (level >= 0x15) {
            tier = 2;
        } else if (level >= 0xF) {
            tier = 1;
        }
        break;
    case 3:
        if (level >= 0x32) {
            tier = 2;
        } else if (level >= 0x28) {
            tier = 1;
        }
        break;
    }
    return tier;
}

/* 0x802657AC - the actor is aiming a shot. */
u32 fn_802657AC(_PLW* self) {
    if (self->kind_0x09 != 3) {
        return 0;
    }
    return Pl_act_ck(self, 1, 0x15) != 0;
}

/* 0x80267270 - forwards an action request to the actor's G3D work. */
void pl_model_state_set(_PLW* self, u32 action, s32 a, u16 b) {
    if (Pl_master_ck(self) != 0 || action == 3) {
        switch (action) {
        case 2:
            hud_item_msg_push(2, a, b);
            return;
        case 1:
            hud_item_msg_push(1, a, b);
            return;
        case 3:
            hud_item_msg_push(3, a, self->chunk_ofs);
            break;
        }
    }
}

/* 0x80264B4C - the motion belongs to the "uncontrollable" set for the actor's current state. */
s32 fn_80264B4C(_PLW* self) {
    s32 motion = self->act_no;

    switch (self->field_0x00A) {
    case 0:
        if (motion < 0x64) {
            if (motion < 0x51) {
                if (motion < 0x1B) {
                    if (motion < 0x17) {
                        return 0;
                    }
                    return 1;
                }
                return 0;
            }
            if (motion < 0x53) {
                return 1;
            }
            return 0;
        }
        if (motion < 0x8C) {
            if (motion < 0x66) {
                return 1;
            }
            return 0;
        }
        if (motion < 0x96) {
            return 1;
        }
        return 0;
    case 1:
        if (motion == 0x14) {
            return 1;
        }
        return 0;
    case 6:
        if (motion != 0x1A) {
            if (motion < 0x1A) {
                if (motion < 0xC) {
                    if (motion < 0xA) {
                        if (motion < 2) {
                            return 0;
                        }
                        return 1;
                    }
                    return 0;
                }
                if (motion < 0xE) {
                    return 1;
                }
                return 0;
            }
            if (motion < 0x37) {
                if (motion != 0x26) {
                    return 0;
                }
                return 1;
            }
            if (motion < 0x3B) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x80266EB8 - tells the SE layer the actor's action has finished (or was superseded). */
void fn_80266EB8(_PLW* self) {
    s32 settled = 0;
    u16 motion;

    if (Pl_master_ck(self) != 0) {
        if (self->field_0x00A == 0) {
            motion = self->act_no;
            if ((u32)(motion - 0x6F) <= 9 || (u32)(motion - 0x22) <= 3) {
                settled = 1;
            }
        }
        if (settled == 0) {
            fn_800DB2DC();
        }
    }
}
