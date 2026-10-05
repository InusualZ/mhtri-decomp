/* enemy/em016_prog.cpp - enemy 016 program
 *
 * `.text` 0x80182C40..0x80192348, 89 functions written (the rest of the range is not decompiled yet).
 * Phase 4: fold of 4 registered units, built from `enemy/fn_80181C88.cpp`, `enemy/fn_80182D5C.cpp`, `enemy/fn_8018B3B8.cpp`, `enemy/fn_80191598.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 *
 * Kept views: the retired sources declared 20 callee(s) with different signatures (`assignVec3`, `eft_spawn_type11`, `em_alt_mode_ck`, `em_fall_start`, `em_mot_set`, `em_move_mode_set`, `em_water_check`, `fn_80126324`, `fn_80129A70`, `fn_80129DB8`, `fn_8012A014`, `fn_8012A204`, ...); each function keeps its own source's view through a function-pointer cast macro (`<name>_viewN`, `<name>_cN`), which compiles to the same direct call, so the fold does not move any body.
 * Hidden declarations: 1 header declaration(s) that disagree with the kept view are renamed away around their `#include` (`#define <name> <name>_hidden_<header>`): `stage_map_kind_get`.
 */

/* Retired header of `enemy/fn_80182D5C.cpp` (kept for its notes and residuals): */
/* enemy/fn_80182D5C.cpp - the enemy band's per-map/area step tables and their dispatchers.
 * .text 0x80182D5C..0x8018B3B8 (0x868C), 115 functions; extab 0x8000E9EC..0x8000ECC4;
 * extabindex 0x80029DCC..0x8002A210.
 *
 * Registration (proposal/80182D5C_fn_80182D5C.cpp).  The range is registered once, here, at its
 * final home.  Which class decided the name and module:
 *   * class 1 (a `__FILE__` string) fails: nothing in the region names a file.  The region's own
 *     `.data`/`.sdata2` pool entries are the shared constants 0.0/1.0/10.0/... (0x80797E88+, read
 *     out of `orig/RMHE08/sys/main.dol`), not a source-file name, and `dumpmap.py lookup` answers
 *     only `zz_` placeholders for this band.
 *   * class 3 (what the code does plus the neighbours' scheme) decides: every bracketing registered
 *     unit is `enemy/` (the unit below ends at 0x80178378, the next occupied band above is
 *     `auto_03_8018B3B8`), every callee the range names is an enemy-band function
 *     (`_ENEMY_WORK`, `em_frame_check__FP11_ENEMY_WORKUsff`, `get_enemy_data`), and the file keeps
 *     the map's own `fn_XXXXXXXX` stem because no better name is evidence-backed.
 *   * the seam is **unproven** (the brief's own warning): `tudiscover at 0x80182D5C` answers a
 *     1-function match set with weak cuts on both sides, no owned data and no anchors, and the
 *     brief records that the left edge is a `--max-bytes` cut.  The left neighbour is NOT this
 *     unit's predecessor: `enemy/fn_80178128.cpp` ends at 0x80178378 and the 0x80178378..0x80182D5C
 *     gap belongs to another proposal, so this unit starts where the run starts rather than
 *     extending that one.  The right edge (0x8018B3B8) is where the next function's symbol begins.
 *
 * C++ (`-lang=c++` through the lib's `cflags_main`) because the range reaches mangled callees -
 * `em_frame_check__FP11_ENEMY_WORKUsff`, `em_die_ck__FP11_ENEMY_WORK` - through their real
 * signatures (rule 9), and `_ENEMY_WORK` carries `nw4r::math::VEC3` members.
 *
 * Types.  `_ENEMY_WORK` comes from its single home `enemy/ENEMY_WORK.h` (rule 1), not from
 * the older `enemy.h` copy.  Two bytes that header did not name yet were added there with
 * their offsets preserved (both are pure padding splits, so no other field moved):
 *   * `+0x1EC  u16 bits_0x1EC` - `fn_8018493C` does `lhz r0,0x1ec` then `clrlwi r3,r0,27`
 *     (`& 0x1F`) to derive its 0x96/0x5A frame countdown.
 *   * `+0x482  u8 field_0x482` - `fn_80184CE0`/`fn_80184C28` pick the approach float with it
 *     (`lbz r0,0x482; cmpwi r0,0; beq`); `enemy.h` already carried the same byte.
 * The pool constants this unit loads are declared `extern`, never defined (playbook 29): redefining
 * them would rebuild the pool instead of addressing the target's.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `symedit` over
 * config/RMHE08/symbols.txt - every symbol defined here is a bare `.text` entry with no owner name -
 * and `python tools/symbols/dumpmap.py lookup 0x80182D5C` answers the `zz_` placeholder form, which
 * is not evidence).
 *
 * Status (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`).
 * 49 of the 115 functions are written and every one of them is above the 80 % bar; 36 are
 * byte-identical.  Unit: 20.966972 % fuzzy, 4272 / 34396 `.text` bytes matched, 36 / 115 functions
 * matched.  The 66 unwritten functions (27100 B) are named as the follow-up queue at the end of this
 * header.
 *
 * Residuals, by measurement (all the near-misses are codegen shapes, not comprehension):
 *   * ARGUMENT EVALUATION ORDER - `fn_80182D5C` 98.45, `fn_801844B8` 94.59, `fn_8018479C` 95.45,
 *     `fn_8018484C` 98.33, `fn_8018493C` 97.78, `fn_80185D60` 97.44.  The target evaluates the
 *     FLOAT argument of `em_motion_param_set`/`em_approach_start` before the integer one (`lfs f1,pool` then
 *     `li r4,imm`); MWCC evaluates in declaration order, so that needs a `(self, f32, s32)` view of
 *     those two band symbols, while `unsplit/enemy.h` (and the landed consumers
 *     `enemy/fn_801550FC.cpp`, `enemy/fn_80147CE0.cpp`) carry `(self, s32, f32)`.  Both spellings
 *     are ABI-equivalent (one FPR slot and one GPR slot), which is why the body still links and
 *     measures: this is the declaration-order residual `enemy/fn_80147CE0.cpp` already recorded.
 *     Locally, naming the constant in a `f32` local reproduces the float-first order for the first
 *     call (`fn_80182D5C` 96.90 -> 98.45); the remaining rows are the second call's `mr r3` slot.
 *   * FUSED `subic.` - `fn_80183E9C` 97.75, `fn_8018493C` 97.78 (each 4 B short).  Retail keeps
 *     `subi r0,rX,1; stw r0,...; cmpwi r0,0; bgt`; this compiler fuses the decrement-with-compare
 *     into `subic. r0,rX,1; stw r0,...; bgt` and drops the `cmpwi`.  Written three ways (`--x <= 0`,
 *     `x--; if (x <= 0)`, `left = x - 1; x = left; if (left <= 0)`) and all three fuse; the mwcc
 *     scheduling shape the stopping rule names, recorded rather than chased.
 *   * `clrlwi` ON A u16 ARGUMENT - `fn_801846BC` 94.64, `fn_80185D60` 97.44.  Retail truncates the
 *     selected motion id at the call (`clrlwi r4,r4,16`); `(u16)` around a conditional whose arms
 *     are both small constants is folded away.  A `u32` local (`u32 motion = ... ; em_mot_set(self,
 *     (u16)motion, ...)`) keeps the range unknown and is what `fn_801846BC` now uses - it moved the
 *     row rather than restoring it, so the remaining loss is that one instruction.  (The C view of
 *     `em_mot_set` in `unsplit/enemy.h` is `(self, s32, s32, s32)`; a `u16` parameter would
 *     emit the truncation for free, but changing it would re-measure every landed consumer.)
 *   * REGISTER COLOURING, vtable store - `fn_80183440` 99.33.  Retail materialises `lbl_805AD340`
 *     into **r0** (`lis r3,@ha; addi r0,r3,@l; stw r0,0(r31)`), this build into r3.  Three spellings
 *     measured (`*(u32*)self = (u32)lbl;`, `*(void**)self = lbl;` and a struct field) and all colour
 *     r3; recorded.
 *   * `fn_801841B4` 99.90 / `fn_801842FC` 99.91 (216 B target / ours same size): every opcode and
 *     operand row pairs and my instruction-text comparison finds no difference beyond the format's
 *     absolute branch addresses; the report's remaining fraction is the branch-target row.  Recorded
 *     as measured; nothing to change in the source.
 *   * PAIRED-SINGLE EPILOGUE - `fn_801850F8` 97.78 (288 B both sides): retail restores the f31
 *     paired-single half with the indexed `li r0,0x18; psq_lx f31,r1,r0,0,qr0` (and saves it with
 *     `stfd` + `psq_st`), this build with the folded `psq_l f31,0x18(r1),0,qr0`; the whole body is
 *     instruction-identical.  Same one-instruction shape `enemy/fn_80178128.cpp` recorded, and the
 *     stopping rule's named unreachable class - recorded, not chased.
 *   * FOLDED u8 STORE MASK + FLOAT-FIRST ARGUMENT ORDER - `fn_80185B0C` 90.98.  Retail writes
 *     `srawi r0,r0,8; clrlwi r0,r0,24; stb r0,0x7(...)` where this build drops the mask (MWCC proves
 *     the shifted u16 is already a byte), and it evaluates `em_turn_in_window`'s two float arguments before
 *     its integer one (the same declaration-order residual as above; the band header's view is
 *     `(self, s32, f32, f32)`).  Both rows measured; the body is otherwise identical.
 *
 * Follow-up queue (the 66 unwritten functions, biggest first; sizes in bytes):
 *   fn_80188A30 (2500), fn_80186D08 (2008), fn_801884E4 (1356), fn_80189CAC (1316), fn_80183040
 *   (1024), fn_8018A1D0 (1016), fn_8018A5C8 (940), fn_801879E4 (904), fn_80184D98 (864),
 *   fn_8018AE7C (840), fn_8018814C (740), fn_80185218 (712), fn_801837B0 (676), fn_8018347C (668),
 *   fn_801897B0 (516), fn_801854E0 (472), fn_80187D6C (472), fn_80185938 (468), fn_80186B34 (468),
 *   fn_801894BC (432), fn_80185DFC (420), fn_80189A68 (376), fn_80187F44 (356), fn_8018B258 (352),
 *   fn_8018966C (324), fn_801878A4 (320), fn_80186020 (296), fn_8018777C (296), fn_80185C6C (244),
 *   fn_801875A4 (240), fn_801866F4 (236), fn_801867E0 (232), fn_80187694 (232), fn_801862CC (216),
 *   fn_801861F8 (212), fn_8018AB94 (208), fn_80186438 (200), fn_80186A6C (200), fn_801893F4 (200),
 *   fn_801874E0 (196), fn_8018A9A4 (196), fn_8018663C (184), fn_80188430 (180), fn_801899B4 (180),
 *   fn_80186594 (168), fn_801869C8 (164), fn_801880A8 (164), fn_80186148 (156), fn_80183718 (152),
 *   fn_801868C8 (152), fn_801863A4 (148), fn_80186500 (148), fn_8018ADE8 (148), fn_8018B1C4 (148),
 *   fn_8018AAD8 (140), fn_8018AC64 (136), fn_8018AD68 (128), fn_8018ACEC (124), fn_8018AA68 (112),
 *   fn_80186960 (104), fn_80189BE0 (80), fn_80189C30 (76), fn_80189C7C (48), fn_8018A974 (48),
 *   fn_8018AB64 (48), fn_801861E4 (20).
 *   `fn_80183040` is the one that is understood but not finished: it builds a 0xC-byte helper through
 *   `__nw__FUl` + `fn_80183440`, writes the 0x328..0x35F block (0xEAAC/0xEE3A/0x11C7 and their
 *   u16/u8 tails) and then dispatches on `team` 0x10/0x11/0x15; `fn_80183040`'s store block needs
 *   the `lis r3,1; subi r0,r3,imm` constant-materialisation shape before it can be written down.
 *   `fn_80183718` and `fn_801837B0` are this unit's own tail targets of `fn_80183AA0`/`fn_80184488`
 *   and are declared (not defined) above, so those two dispatchers already measure 100 %.
 *
 * Callees.  Everything the written bodies call is declared where it belongs (rule 2): the unsplit
 * enemy band in `unsplit/enemy.h`, the owner units in `enemy/fn_801251D0.h`,
 * `enemy/fn_8012BDF4.h`, `enemy/fn_80138074.h`, `enemy/fn_80147CE0.h`,
 * `ef/fn_80105314.h`, `mh3_pad.h`, `sys_mem.h`.  Four declarations were moved
 * into those owner headers by this unit and are filed as `shared-file` config requests:
 * `fn_80126324` + `fn_80128030` (owner `enemy/fn_801251D0.cpp`), `fn_8012E664` + `fn_8012E694`
 * (owner `enemy/fn_8012BDF4.cpp`), plus `fn_801337FC`, `f32 fn_8013026C(_ENEMY_WORK*)` and
 * `void fn_8012FE3C(_ENEMY_WORK*, f32)` in the band header.  The two callees with no
 * registered owner and no resolvable module band (`stage_map_kind_get`, `fn_80191598` - the lint's counted
 * "address band interleaves modules" gap) are declared locally, as are the pool literals.
 */

/* Retired header of `enemy/fn_8018B3B8.cpp` (kept for its notes and residuals): */
/* enemy/fn_8018B3B8.cpp - the enemy multi-motion action band between `fn_80182D5C` and
 * `fn_80191598`.
 *
 * `.text` 0x8018B3B8..0x80191598 (24 functions, 0x61E0 B), extab 0x8000ECC4..0x8000ED64,
 * extabindex 0x8002A210..0x8002A300.
 *
 * Module `enemy`, decided by class 3 (what the code does plus the neighbours' scheme): both
 * bracketing registered units are `enemy/*` (the unit below ends exactly at 0x8018B3B8 and
 * `enemy/fn_80191598.cpp` starts exactly at 0x80191598), every callee out of the range is an
 * enemy-band function, and the four dispatchers ([`fn_8018B3C8`] on the work's +0x1E6, `fn_8018D250`
 * on +0x1E6, `fn_8018D2B0` on +0x1E5, and the whole +0x5 state-machine family) key on the same
 * `_ENEMY_WORK` the neighbours read.  Class 1 fails: nothing in the range names a source file (the
 * `.data`/`.sdata2` pool holds only numeric constants and the two jump tables), and class 2 fails:
 * `python tools/symbols/dumpmap.py lookup 0x8018B3B8` answers the `zz_018b3b8_` placeholder form,
 * which is not evidence.  The file keeps the map's own stem (class 4).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with `symedit` over
 * config/RMHE08/symbols.txt - every symbol defined here is a bare `.text` entry with no owner name -
 * and `python tools/symbols/dumpmap.py lookup` answers the `zz_XXXXXXXX_` placeholder form for the
 * whole inventory, which is not evidence).
 *
 * Language C++ (`-lang=c++` through the lib's `cflags_main`): the range reaches mangled callees
 * (`em_frame_check__FP11_ENEMY_WORKUsff`, `em_after_frame_check__FP11_ENEMY_WORKUsff`,
 * `get_em_chg_scale__FP11_ENEMY_WORK`, `get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor`) through
 * their real signatures (rule 9), and `_ENEMY_WORK::char_0x024` is the `MHchar` base `pl.h` owns.
 *
 * Types.  `_ENEMY_WORK` comes from its single home `enemy/ENEMY_WORK.h` (rule 1), not from
 * the older `enemy.h` copy.  This band reads the +0x350..+0x35E bytes as a run of signed
 * 16-bit TEV colour words (`fn_80191038` does `lha`/`sth` at +0x350/+0x352/+0x354/+0x356/+0x358 and
 * `fn_801913FC` `lha` at +0x35A/+0x35C), where the header had byte views; the three union members
 * the range needs (`tev_0x350`, `tev_0x354`, the +0x358 union member) were added there with the
 * offsets preserved, so no other unit's layout moved.
 *
 * The +0x5 state byte is a step counter (`fn_8018B418` steps 0..10, the `fn_8018BBD8` family 0..1);
 * +0x1E6 is the multi-motion class (`fn_8018B3C8`/`fn_8018D250` dispatch 0..13), +0x1E5 the action
 * (`fn_8018D2B0` dispatches 0..13) and +0x3 the team (0x10/0x11/0x15).  `fn_8018D8C8` is the
 * per-(team,motion) frame-window driver: for each motion it runs the `em_after_frame_check` windows
 * and calls `fn_8018D558` (the effect-spawn router) with the window's id/kind.
 *
 * Status (official `build/RMHE08/report.json`, full `ninja` in this worktree, `main.dol: OK`):
 * all 24 bodies are written and every one is above the 80 % bar; 13 are byte-identical.
 *   fn_8018B3B8 100, fn_8018B3BC 100, fn_8018B3C8 100, fn_8018BBD8 100, fn_8018BC7C 100,
 *   fn_8018BF4C 100, fn_8018C2CC 100, fn_8018C528 100, fn_8018C998 100, fn_8018CDE0 100,
 *   fn_8018D1AC 100, fn_8018D250 100, fn_8018D2B0 100;
 *   fn_8018B418 98.31 (1984 B), fn_8018BFF0 98.91 (732 B), fn_8018C370 97.27 (440 B),
 *   fn_8018C5CC 97.94 (972 B), fn_8018CA3C 97.42 (932 B), fn_8018CE84 97.52 (808 B),
 *   fn_8018D370 91.76 (488 B), fn_8018D558 93.16 (880 B), fn_8018D8C8 96.98 (14192 B),
 *   fn_80191038 83.84 (964 B), fn_801913FC 92.22 (412 B).  Unit: 96.6689 % fuzzy, 24 functions,
 *   13 matched.  Sections: extab 0xA0 and extabindex 0xF0 match the target's exactly; `.text` is
 *   0x6028 against the target's 0x61E0 (the shortfall is inside the non-identical functions).
 *
 * Residuals, by measurement (all are codegen shapes, not comprehension):
 *   * `fnmsubs` FUSION - `fn_801913FC` 92.22 (412 B; ours 392).  The target keeps the
 *     `(lbl_80797ED0 + fn_8013026C(self)) * scale` as `fmuls` + `fsubs`; this build's -O3 fuses it
 *     into `fnmsubs f1,f31,f1,f0`.  A `#pragma peephole off` was not applied (it moves the other
 *     twelve functions); the residual is the fused pair only.
 *   * ARGUMENT EVALUATION ORDER - `fn_8018D370` 91.76 (488 B; ours 464) and `fn_80191038` 83.84
 *     (964 B; ours 912).  The target evaluates the float argument of `em_water_check`/
 *     `eft009_spawn_at_joint` before the integer ones, and materialises the `_GXColor` byte record in the
 *     order b,g,r,a; MWCC orders by declaration, so a few rows still differ.  Both spellings are
 *     ABI-equivalent.
 *   * `_GXColor` record - `fn_80191038` stores the four colour bytes through `_GXColor`; the
 *     target's store order (b/g/r/a, then a) is the compiler's, and the 52-byte frame difference is
 *     this unit's view of the same record.
 *   * The near-identical multi-window machines (`fn_8018B418` 98.31, `fn_8018C5CC` 97.94,
 *     `fn_8018CA3C` 97.42, `fn_8018CE84` 97.52, `fn_8018D558` 93.16, `fn_8018D8C8` 96.98) each miss
 *     a handful of rows on the shared `fn_8018D558` call tail and on a `b` that has become a
 *     fall-through (or the reverse); the control flow and every callee are correct.
 *
 * Type and declaration follow-ups (the outbox carries each as a `shared-file`/`config_requests`
 * entry):
 *   * the plain band callees declared in this file (`fn_8018479C`, `fn_80184D98`, `fn_80183DCC`,
 *     `fn_80184488`, `fn_80184BF8`, `fn_801861E4`, `fn_80186960`, `fn_80189C7C`, `fn_8018A974`,
 *     `fn_8018AB64`, `fn_8018AB94`..`fn_8018B258`) belong in `unsplit/enemy.h` or their
 *     owner's header (rule 2); they are elected `_ENEMY_WORK*`-typed here.
 *   * the shell callback table is the shared `ShellSetFuncs` of `stage/shell_set_func_ptr.h`.
 *   * the +0x350..+0x35E TEV union members added to `enemy/ENEMY_WORK.h` (`tev_0x350`,
 *     `tev_0x354`, the +0x358 union member) are this unit's signed-short view; they should be named
 *     once for the band.
 */

/* Retired header of `enemy/fn_80191598.cpp` (kept for its notes and residuals): */
/* enemy/fn_80191598.cpp - the enemy aim/action group of the `ResUserDataAc` class set.
 *
 * `.text` 0x80191598..0x801926EC (26 functions, 4436 B), extab 0x8000ED64..0x8000EDF4,
 * extabindex 0x8002A300..0x8002A3D8.
 *
 * Module `enemy`.  The range's callees are the enemy work API
 * (`em_act_ck__FP11_ENEMY_WORKUcUc`, `em_parts_damage_level_get__FP11_ENEMY_WORKUc`,
 * `get_em_scale__FP11_ENEMY_WORK`, `get_joint_wmat_em__FP11_ENEMY_WORKUlPQ34nw4r4math5MTX34`,
 * `get_move_work_adrs__FUc`), both bracketing registered units are `enemy/*`, and the three `.data`
 * class tables that hold this range's entry points (0x805AA960, 0x805AA9CC, 0x805AAA38, slots 4..13)
 * sit beside the `em017_prog_tbl`/`em021_prog_tbl` labels, i.e. the range is part of the per-enemy
 * class definition block.  Language C++: every call out of the range is a mangled symbol.
 *
 * Seam.  The edges are the proposal's own: below is `proposal/8018B3B8` (0x8018B3B8..0x80191598,
 * unclaimed) and above `proposal/801926EC`.  Neither is a byte-budget artefact - the extabindex run
 * this unit claims (0x8002A300..0x8002A3D8) holds exactly the 18 records of this range's functions,
 * with the neighbouring functions' records (`fn_801913FC` below, `fn_801926EC` above) on either side,
 * and the class tables straddle the seam (`fn_80191038`/`fn_801913FC` are their first two slots).
 *
 * Name.  The map has only `fn_XXXXXXXX` for this range and the runtime dump answers only `zz_`
 * placeholders (`dumpmap.py lookup 0x80191598` -> `zz_0191598_`), so the file keeps the map's stem.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with dumpmap.py
 * lookup on the range's inventory: all 26 names are bare `.text` entries in
 * config/RMHE08/symbols.txt and the runtime dump has only `zz_XXXXXXXX_` placeholders for them)
 *
 * Types.  This range reads `_ENEMY_WORK` bytes that `enemy.h` views differently: it steps
 * +0x328/+0x32C/+0x330/+0x338/+0x33C as 4-byte fixed-point angle words where that header has a
 * `VEC3 v_0x320` and 16-bit timers, and it reads +0x344/+0x348/+0x34C as floats where the header has
 * padding.  The unit therefore carries its own view (`EmActWork`), exactly as `enemy/fn_8013ACC4.cpp`
 * (`EmWork`), `enemy/fn_80138074.c` (`_ENEMY_WORK`) and `enemy/fn_8013F764.cpp` (`EmcWork`) do.  The
 * view's size (0xB18) is measured from this range itself: `fn_80192448` walks the
 * `get_move_work_adrs(3)` records with `addi r29,r29,0xB18`.  The outbox asks for these fields to be
 * folded into the shared header.
 *
 * The record at +0x328 (0x3C bytes) is the work's aim/animation target: a fixed-point angle word,
 * two 3-word angle groups (`fn_80191EF8` steps them, `fn_80191CE4` copies them into the `_CP_VECTOR`
 * `cpSetRotMatrix` takes), a float vector at +0x1C, two floats and a byte of bits (0x01 `fn_80192618`
 * arms, 0x02 `fn_801923E0` and 0x08 `fn_80192410` read).  `fn_80192630` initialises it; the flag
 * accessors `fn_80192348`/`58`/`70`/`CC` use the work's own displacement (0x35D/0x35B), which is what
 * places the record at the work base.
 *
 * Measured result.  All 26 bodies are written - nothing is stubbed - and 22 of them are byte-identical
 * (official `report generate` fuzzy_match_percent, this worktree, against the split target object):
 *   fn_80191990 100.00, fn_80191A6C 100.00, fn_80191AD8 100.00, fn_80191AE8 100.00,
 *   fn_80191CDC 100.00, fn_80191CE4 100.00, fn_80191EF8 100.00, fn_80192080 100.00,
 *   fn_80192108 100.00, fn_801921A8 100.00, fn_80192204 100.00, fn_80192348 100.00,
 *   fn_80192358 100.00, fn_80192370 100.00, fn_8019238C 100.00, fn_801923CC 100.00,
 *   fn_801923E0 100.00, fn_80192410 100.00, fn_80192440 100.00, fn_8019255C 100.00,
 *   fn_80192618 100.00, fn_80192630 100.00;
 *   fn_80191B4C 99.80 (400 B), fn_80192448 99.13 (276 B), fn_80191598 98.82 (1016 B),
 *   fn_80191E30 93.10 (200 B).  The unit's sections match the target's exactly: `.text` 0x1144,
 *   extab 0x90, extabindex 0xD8 (18 records, one per function with a frame).
 *
 * Source shapes worth keeping (each measured, each worth its score):
 *   * `#pragma peephole off` (one scoped pragma, docs/plan.md 8.2): the build's -O3 peephole fuses
 *     the `extsh` + `cmpwi` pair a 16-bit value test needs into the record form `extsh.`.  With the
 *     peephole on: `fn_80191EF8` 94.64, `fn_80192108` 97.38, `fn_801921A8` 95.43; with it off all
 *     three are 100.00 and every already-identical function is unchanged.
 *   * A one-case `switch` is the idiom several of these guards are written in: `switch (x) { case 3: }`
 *     makes MWCC emit the *signed* compare (`cmpwi r0,3`) where `if (x == 3)` emits `cmplwi`
 *     (`fn_80191990`'s map-key gate and state byte, `fn_80191AE8`'s kind gate, `fn_80191B4C`'s
 *     kind gate, `fn_80191CE4`/`fn_80191E30`'s item-type gate - the last two were 97/92 % as `if`s).
 *   * `u8 >= 2` (`fn_8019238C`) is not a compare at all: MWCC emits the borrow trick
 *     `((v | ~2) - ((v - 2) >> 1)) >> 31` (`li r3,2; orc; addi; srwi; subf; srwi 31`).  The source is
 *     the comparison, not the arithmetic - writing the arithmetic by hand gives the same bytes but
 *     the *idiom* is what a later reader needs.
 *   * `fn_80191CE4`'s frame is sized for a 0x24-byte 3x3 matrix (`EmMtx33`) at +0x14, not for a
 *     second 0x30-byte matrix: with `MTX34` there the frame is 0x90 where retail keeps 0x80, and the
 *     score drops from 100.00 to 98.96.
 *   * Local *declaration order* drives MWCC's callee-saved assignment (`fn_80192448`: `max`, `i`,
 *     `work` gives retail's r31/r30/r29; `max`, `work`, `i` gives r31/r29/r30 and 98.04 vs 99.13).
 *
 * Residuals (every remaining objdiff row of the four functions that are not byte-identical):
 *   * `fn_80191E30` 93.10 - the whole body matches except (a) the two callee-saved registers of the
 *     item argument and the work pointer are swapped (retail: item in r30, work in r31; this build:
 *     the reverse, i.e. MWCC ordered by first use here) and (b) two extra `b` instructions (12 B):
 *     retail's three inner cases each branch to the shared continuation at +0x70, ours fall into the
 *     next case's `li`.  Tried: `if (item->type_0x04 == 0xFF)` instead of the one-case `switch` (drops
 *     to 91.9), the `work` pointer declared before/after the other locals, and `default: return;` vs a
 *     guarded `if` (the latter grows the function further).
 *   * `fn_80191598` 98.82 - one instruction (4 B) long.  `fn_80191B4C` 99.80 and `fn_80192448` 99.13 -
 *     a single row each; both are the residual register colouring of a value MWCC keeps in a different
 *     callee-saved register than retail (the `hit`/`probe` and `work`/`i` pairs).
 *
 * Type and declaration follow-ups (the outbox carries each as a `shared-file`/`config_requests` entry):
 *   * `_ENEMY_WORK`'s +0x328..+0x3A4 aim record, the +0x590 cluster array's live byte and position,
 *     +0x9F6 and +0x1E0/0x1E1/0x1E2 belong in `enemy.h` (this range's view of 0x328..0x33E
 *     contradicts that header's `VEC3 v_0x320`/16-bit timers).
 *   * `ResUserDataAc` (this range's `EmUserData` view) belongs in `enemy/fn_80138074.h`, its
 *     owner's header - `enemy/fn_80138074.h` declares `fn_8013A654` with `_ENEMY_WORK*` where
 *     the callee's own body reads +0x04/+0x08 (settled from the callee, rule 6 of the playbook).
 *   * the plain prototypes at the top of this file (`em_move_mode_set`, `fn_80126324`, `fn_80129xxx`,
 *     `fn_8013918C`, `joint_mtx_store`, `joint_mtx_load`, `fn_8013A654`, `fn_8008E8D0`, `fn_8008EE68`,
 *     `fn_800FBB90`, `eft_rot_vec_copy`, `fn_805012E8`, `mtx34_trans_add`, `mtx34_trans_get`, `MTX34_ctor`,
 *     `fn_800516F0`, `setVec3`, `copyVec3`, `VEC3_ctor`, `assignVec3`, `stage_map_kind_get`,
 *     `fn_80182D5C`) belong in their owners' headers / `unsplit/enemy.h`; `setVec3`'s
 *     `void` return in `mh3_pad.h` is wrong for this range's call sites, which read its r3.
 *   * the unit registers a `.ctors` word (0x8056F33C..0x8056F340, added by the split itself): the
 *     original translation unit has a static constructor, most plausibly the one that fills the five
 *     static vectors `fn_80192204` builds (`vec_pair_80191598_0`..`vec_default_80191598`).
 */

#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "ef/fn_80117E58.h" /* fn_80117E58 (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "enemy/em_busy_set.h" /* em_busy_set (rule 2: the owner's header) */
#include "enemy/em_alt_mode_ck.h" /* em_alt_mode_ck (rule 2: the owner's header) */
#include "enemy/fn_8012EC3C.h" /* fn_8012EC3C (rule 2: the owner's header) */
#include "enemy/em_mot_set_blend.h" /* em_mot_set_blend (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/em_mot_speed_set.h" /* em_mot_speed_set (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/fn_8013026C.h" /* fn_8013026C (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/em_frame_flag_set.h" /* em_frame_flag_set (rule 2: the owner's header) */
#include "enemy/em_busy_timer_reset.h" /* em_busy_timer_reset (rule 2: the owner's header) */
#include "enemy/fn_801321DC.h" /* fn_801321DC (rule 2: the owner's header) */
#include "enemy/fn_80133C3C.h" /* fn_80133C3C (rule 2: the owner's header) */
#include "enemy/fn_80133DB0.h" /* fn_80133DB0 (rule 2: the owner's header) */
#include "enemy/em_turn_in_window.h" /* em_turn_in_window (rule 2: the owner's header) */
#include "enemy/em_flags836_ck.h" /* em_flags836_ck (rule 2: the owner's header) */
#include "enemy/em_camera_req.h" /* em_camera_req (rule 2: the owner's header) */
#include "enemy/fn_80136D14.h" /* fn_80136D14 (rule 2: the owner's header) */
#include "enemy/fn_80129D3C.h" /* fn_80129D3C (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "enemy/ENEMY_WORK.h"
#define fn_8012EC3C fn_8012EC3C_hidden_fn_8012E968_h
#define em_alt_mode_ck em_alt_mode_ck_hidden_fn_8012E968_h
#include "unsplit/enemy.h"
#undef em_alt_mode_ck
#undef fn_8012EC3C
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* fn_80126278/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* fn_8013026C */
#include "enemy/fn_8012BDF4.h" /* fn_8012E5A8 */
#include "ef.h"
#include "enemy/fn_80138074.h" /* fn_8013A654/fn_8013918C + the EmUserData record */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "fn_8004CAD8.h"       /* fn_8005024C/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */
#define stage_map_kind_get stage_map_kind_get_hidden_stg_w_h
#include "stage/stg_w.h"
#undef stage_map_kind_get
#include "enemy/fn_801251D0.h"
#include "ef.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80147CE0.h"
#include "fn_8004CAD8.h"
#include "ef/fn_80105314.h"
#include "mh3_pad.h"
#include "sys_mem.h"
#include "enemy/fn_8012EC74.h" /* fn_8013026C/fn_8012FE3C/fn_801337FC/fn_80136D4C */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#include "enemy/em016_prog_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80117E58_c1 ((void (*)(_ENEMY_WORK*, u32, nw4r::math::VEC3*, u32, f32))fn_80117E58)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_alt_mode_ck_c1 ((u32 (*)(_ENEMY_WORK*))em_alt_mode_ck)
#define fn_8012EC3C_c1 ((u32 (*)(_ENEMY_WORK*))fn_8012EC3C)
#define fn_80129D3C_c1 ((u32 (*)(EmActWork*))fn_80129D3C)
/* fn_80182D5C_noarg: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80182D5C_noarg ((void (*)(void))fn_80182D5C)
/* stage_map_kind_get_view3: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define stage_map_kind_get_view3 ((u8 (*)(u8))stage_map_kind_get)
/* stage_map_kind_get_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define stage_map_kind_get_view1 ((u8 (*)(u8))stage_map_kind_get)
/* fn_80192080_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80192080_view1 ((void (*)(_ENEMY_WORK*))fn_80192080)
/* fn_80191EF8_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80191EF8_view1 ((void (*)(_ENEMY_WORK*))fn_80191EF8)
/* fn_80182D5C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80182D5C_view1 ((void (*)(void))fn_80182D5C)
/* fn_8013A654_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013A654_view1 ((void (*)(EmUserData*, u32))fn_8013A654)
/* fn_8013918C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8013918C_view1 ((void (*)(void*, s16))fn_8013918C)
/* fn_801321DC_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_801321DC_view1 ((u32 (*)(EmActWork*))fn_801321DC)
/* fn_8012EC3C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8012EC3C_view1 ((u32 (*)(EmActWork*))fn_8012EC3C)
/* fn_8012A204_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8012A204_view1 ((s32 (*)(EmActWork*))fn_8012A204)
/* fn_8012A014_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_8012A014_view1 ((u32 (*)(EmActWork*, u32, u32, u16, u32, u8*))fn_8012A014)
/* fn_80129DB8_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80129DB8_view1 ((u8 (*)(EmActWork*))fn_80129DB8)
/* fn_80129A70_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80129A70_view1 ((u32 (*)(EmActWork*, u16))fn_80129A70)
/* fn_80126324_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_80126324_view1 ((void (*)(EmActWork*, u8, u8, f32))fn_80126324)
/* em_water_check_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_water_check_view1 ((u32 (*)(struct _ENEMY_WORK*))em_water_check)
/* em_move_mode_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_move_mode_set_view1 ((void (*)(EmActWork*, u32))em_move_mode_set)
/* em_mot_set_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_mot_set_view1 ((void (*)(EmActWork*, s32, s32, s32))em_mot_set)
/* em_fall_start_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_fall_start_view1 ((void (*)(_ENEMY_WORK*, f32))em_fall_start)
/* em_alt_mode_ck_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define em_alt_mode_ck_view1 ((u32 (*)(EmActWork*))em_alt_mode_ck)
/* eft_spawn_type11_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define eft_spawn_type11_view1 ((void (*)(_ENEMY_WORK*, nw4r::math::VEC3*, u32, f32))eft_spawn_type11)
/* assignVec3_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define assignVec3_view1 ((void (*)(void*, const void*))assignVec3)

extern "C" {
void fn_80182C40(_ENEMY_WORK* self, u32 kind, void* out);
void fn_80182D44(_ENEMY_WORK* self, u32 arg);

extern f32 lbl_80797E88;
extern f32 lbl_80797E8C;
extern f32 lbl_80797E90;
extern f32 lbl_80797E94;
extern f32 lbl_80797E98;

/* ------------------------------------------------------------------------------------------------
 * Callees with no registered owner (the lint's counted "band interleaves modules" gap).
 * ------------------------------------------------------------------------------------------------ */

/* 0x802B0668 - a byte table lookup (`lbzx` into `lbl_805CED40`, `0xFF` meaning "no entry", in which
 * case the argument is returned unchanged).  The three landed consumers declare three different
 * return types for it; the call here masks the result to a byte itself, so the wider view is the
 * one this range's target was built with (it carries the `clrlwi r0,r3,24`). */
u32 stage_map_kind_get(u32 kind);

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions: declared up front so the dispatchers can tail-call them.
 * ------------------------------------------------------------------------------------------------ */
void fn_80183718(_ENEMY_WORK* self);
void fn_801837B0(_ENEMY_WORK* self);
void fn_801841B4(_ENEMY_WORK* self);
void fn_80184280(_ENEMY_WORK* self);
void fn_801842FC(_ENEMY_WORK* self);
void fn_801843D4(_ENEMY_WORK* self);
void fn_8018454C(_ENEMY_WORK* self, u32 arg);
void fn_8018460C(_ENEMY_WORK* self, u32 arg);
void fn_801846BC(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void fn_8018479C(_ENEMY_WORK* self);
void fn_8018484C(_ENEMY_WORK* self, u32 arg1, u32 arg2);
void fn_8018493C(_ENEMY_WORK* self, u32 arg);
void fn_801844B8(_ENEMY_WORK* self);
void fn_80184A5C(_ENEMY_WORK* self);
void fn_80184B0C(_ENEMY_WORK* self);
void fn_80184B78(_ENEMY_WORK* self);
void fn_80184434(_ENEMY_WORK* self);
void fn_80184458(_ENEMY_WORK* self);

extern f32 lbl_80797E9C;
extern f32 lbl_80797EA0;
extern f32 lbl_80797EA4;
extern f32 lbl_80797EA8;
extern f32 lbl_80797EAC;

extern f32 lbl_80797EB4;

extern f32 lbl_80797EC0;
extern f32 lbl_80797EC4;
extern f32 lbl_80797EC8;
extern f32 lbl_80797ECC;
/* The two action tables `fn_8018454C` picks between (`arg` 1 selects the second). */
extern u32 lbl_8056FF50[];
extern u32 lbl_8056FF90[];
/* The action table `fn_80184CE0` arms. */
extern u32 lbl_80570010[];
extern u32 lbl_8056FFD0[];
extern f32 lbl_80797ED0;
extern f32 lbl_80797ED4;
extern f32 lbl_80797ED8;
extern f32 lbl_80797EDC;
extern f32 lbl_80797EE0;
extern f32 lbl_80797EE4;
extern f32 lbl_80797EE8;
extern f32 lbl_80797EEC;
extern f32 lbl_80797F00;
extern f32 lbl_80797F04;
/* The vtable word the 0xC-byte helper's constructors install at +0 (map: `.data`, no module). */
extern u32 lbl_805AD340[];
}

/* ------------------------------------------------------------------------------------------------ *
 * The mangled callees, outside `extern "C"` so the front-end mangles them the way the map spells
 * them (rule 9).  Each parameter width is the one the map's mangling encodes.
 * ------------------------------------------------------------------------------------------------ */
u16 calcVecAngX(nw4r::math::VEC3* v);
void eft009_set_pos(u8 id, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 arg);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
f32 get_em_scale(struct _ENEMY_WORK* self);
void get_joint_wmat_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

extern "C" {
/* ------------------------------------------------------------------------------------------------ *
 * This unit's own functions, declared up front so the dispatchers can call them.
 * ------------------------------------------------------------------------------------------------ */
void fn_8018B3B8(_ENEMY_WORK* self);
void fn_8018B3BC(_ENEMY_WORK* self);
void fn_8018B3C8(_ENEMY_WORK* self);
void fn_8018B418(_ENEMY_WORK* self);
void fn_8018BBD8(_ENEMY_WORK* self);
void fn_8018BC7C(_ENEMY_WORK* self);
void fn_8018BF4C(_ENEMY_WORK* self);
void fn_8018BFF0(_ENEMY_WORK* self);
void fn_8018C2CC(_ENEMY_WORK* self);
void fn_8018C370(_ENEMY_WORK* self);
void fn_8018C528(_ENEMY_WORK* self);
void fn_8018C5CC(_ENEMY_WORK* self);
void fn_8018C998(_ENEMY_WORK* self);
void fn_8018CA3C(_ENEMY_WORK* self);
void fn_8018CDE0(_ENEMY_WORK* self);
void fn_8018CE84(_ENEMY_WORK* self);
void fn_8018D1AC(_ENEMY_WORK* self);
void fn_8018D250(_ENEMY_WORK* self);
void fn_8018D2B0(_ENEMY_WORK* self);
void fn_8018D370(_ENEMY_WORK* self);
void fn_8018D558(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5);
void fn_8018D8C8(_ENEMY_WORK* self);
void fn_80191038(_ENEMY_WORK* self);
s32 fn_801913FC(_ENEMY_WORK* self, u8 arg1);

void fn_80184D98(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80183DCC(_ENEMY_WORK* self);
void fn_80184488(_ENEMY_WORK* self);
void fn_80184BF8(_ENEMY_WORK* self);
void fn_801861E4(_ENEMY_WORK* self);
void fn_80186960(_ENEMY_WORK* self);
void fn_80189C7C(_ENEMY_WORK* self);
void fn_8018A974(_ENEMY_WORK* self);
void fn_8018AB64(_ENEMY_WORK* self);
void fn_8018AB94(_ENEMY_WORK* self);
void fn_8018AC64(_ENEMY_WORK* self);
void fn_8018ACEC(_ENEMY_WORK* self);
void fn_8018AD68(_ENEMY_WORK* self);
void fn_8018ADE8(_ENEMY_WORK* self);
void fn_8018AE7C(_ENEMY_WORK* self);
void fn_8018B1C4(_ENEMY_WORK* self);
void fn_8018B258(_ENEMY_WORK* self);
s16 em_demo_frame_get(void);
u32 em_demo_time_ck(u32 a);
void em_demo_pos_set(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(_ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_reset(_ENEMY_WORK* self, u32 a);
void em_demo_enable(_ENEMY_WORK* self);
void em_demo_key3_apply(_ENEMY_WORK* self, s16 a, const void* tbl, u32 b);
void em_demo_key_apply(_ENEMY_WORK* self, s16 a, const void* tbl, const void* tbl2, u32 b, u32 c);
void eft009_spawn_at_joint(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_801049D0(_ENEMY_WORK* self, u32 a, u32 b, u32 c, nw4r::math::VEC3* p, f32 d);
void eft_spawn_type10(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 d);
void eft_spawn_pos_in_area(nw4r::math::VEC3* p, u8 a, u32 b, u32 c, f32 d);
void fn_8011D448(_ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_8011D4FC(_ENEMY_WORK* self, u32 a, u32 b, u32 c, u32 d, f32 e);
void fn_8011D690(_ENEMY_WORK* self, u32 a, u32 b, f32 c);
void eft_em_spawn_joint(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, u32 c, f32 d);
void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* p, f32 c);
void addVec3(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 fn_8005024C(u16 a);

extern f32 lbl_80797EB8;

extern f32 lbl_80797EF8;

extern f32 lbl_80797F08;
extern f32 lbl_80797F0C;
extern f32 lbl_80797F10;
extern f32 lbl_80797F14;
extern f32 lbl_80797F34;
extern f32 lbl_80797F38;
extern f32 lbl_80797F40;
extern f32 lbl_80797F44;
extern f32 lbl_80797F4C;
extern f32 lbl_80797F50;
extern f32 lbl_80797F58;
extern f32 lbl_80797F5C;
extern f32 lbl_80797F64;
extern f32 lbl_80797F68;
extern f32 lbl_80797F6C;
extern f32 lbl_80797F70;
extern f32 lbl_80797F78;
extern f32 lbl_80797F7C;
extern f32 lbl_80797F80;
extern f32 lbl_80797F84;
extern f32 lbl_80797F90;
extern f32 lbl_80797F98;
extern f32 lbl_80797F9C;
extern f32 lbl_80797FA4;
extern f32 lbl_80797FA8;
extern f32 lbl_80797FB0;
extern f32 lbl_80797FC8;
extern f32 lbl_80797FCC;
extern f32 lbl_80797FE0;
extern f32 lbl_80797FF4;
extern f32 lbl_80797FF8;

extern f32 lbl_80798004;
extern f32 lbl_8079800C;
extern f32 lbl_80798010;
extern f32 lbl_80798014;
extern f32 lbl_80798018;
extern f32 lbl_8079801C;
extern f32 lbl_80798020;
extern f32 lbl_80798024;
extern f32 lbl_80798028;
extern f32 lbl_8079802C;
extern f32 lbl_80798030;
extern f32 lbl_80798034;
extern f32 lbl_80798038;
extern f32 lbl_8079803C;
extern f32 lbl_80798040;
extern f32 lbl_80798044;
extern f32 lbl_80798048;
extern f32 lbl_8079804C;
extern f32 lbl_80798050;
extern f32 lbl_80798054;
extern f32 lbl_80798058;
extern f32 lbl_8079805C;
extern f32 lbl_80798060;
extern f32 lbl_80798064;
extern f32 lbl_80798068;
extern f32 lbl_8079806C;
extern f32 lbl_80798070;
extern f32 lbl_80798074;
extern f32 lbl_80798078;
extern f32 lbl_8079807C;
extern f32 lbl_80798080;
extern f32 lbl_80798084;
extern f32 lbl_80798088;
extern f32 lbl_8079808C;
extern f32 lbl_80798090;
extern f32 lbl_80798094;
extern f32 lbl_80798098;
extern f32 lbl_8079809C;
extern f32 lbl_807980A0;
extern f32 lbl_807980A4;
extern f32 lbl_807980A8;
extern f32 lbl_807980AC;
extern f32 lbl_807980B0;
extern f32 lbl_807980B4;
extern f32 lbl_807980B8;
extern f32 lbl_807980BC;
extern f32 lbl_807980C0;
extern f32 lbl_807980C4;
extern f32 lbl_807980C8;
extern f32 lbl_807980CC;
extern f32 lbl_807980D0;
extern f32 lbl_807980D4;
extern f32 lbl_807980D8;
extern f32 lbl_807980DC;
extern f32 lbl_807980E0;
extern f32 lbl_807980E4;
extern f32 lbl_807980E8;
extern f32 lbl_807980EC;
extern f32 lbl_807980F0;
extern f32 lbl_807980F4;
extern f32 lbl_807980F8;
extern f32 lbl_807980FC;
extern f32 lbl_80798100;
extern f32 lbl_80798104;
extern f32 lbl_80798108;
extern f32 lbl_8079810C;
extern f32 lbl_80798110;
extern f32 lbl_80798114;
extern f32 lbl_80798118;
extern f32 lbl_8079811C;
extern f32 lbl_80798120;
extern f32 lbl_80798124;
extern f32 lbl_80798128;
extern f32 lbl_8079812C;
extern f32 lbl_80798130;
extern f32 lbl_80798134;
extern f32 lbl_80798138;
extern f32 lbl_8079813C;
extern f32 lbl_80798140;
extern f32 lbl_80798144;
extern f32 lbl_80798148;
extern f32 lbl_8079814C;
extern f32 lbl_80798150;
extern f32 lbl_80798154;
extern f32 lbl_80798158;
extern f32 lbl_8079815C;
extern f32 lbl_80798160;
extern f32 lbl_80798164;
extern f32 lbl_80798168;
extern f32 lbl_8079816C;
extern f32 lbl_80798170;
extern f32 lbl_80798174;
extern f32 lbl_80798178;
extern f32 lbl_8079817C;
extern f32 lbl_80798180;
extern f32 lbl_80798184;
extern f32 lbl_80798188;
extern f32 lbl_8079818C;
extern f32 lbl_80798190;
extern f32 lbl_80798194;
extern f32 lbl_80798198;
extern f32 lbl_8079819C;
extern f32 lbl_807981A0;
extern f32 lbl_807981A4;
extern f32 lbl_807981A8;
extern f32 lbl_807981AC;
extern f32 lbl_807981B0;
extern f32 lbl_807981B4;
extern f32 lbl_807981B8;
extern f32 lbl_807981BC;
extern f32 lbl_807981C0;
extern f32 lbl_807981C4;
extern f32 lbl_807981C8;
extern f32 lbl_807981CC;
extern f32 lbl_807981D0;
extern f32 lbl_807981D4;
extern f32 lbl_807981D8;
extern f32 lbl_807981DC;
extern f32 lbl_807981E0;
extern f32 lbl_807981E4;
extern f32 lbl_807981E8;
extern f32 lbl_807981EC;
extern f32 lbl_807981F0;
extern f32 lbl_807981F4;
extern f32 lbl_807981F8;
extern f32 lbl_807981FC;
extern f32 lbl_80798200;
extern f32 lbl_80798204;
extern f32 lbl_80798208;
extern f32 lbl_8079820C;
extern f32 lbl_80798210;
extern f32 lbl_80798214;
extern f32 lbl_80798218;
extern f32 lbl_8079821C;
extern f32 lbl_80798220;
extern f32 lbl_80798224;
extern f32 lbl_80798228;
extern u32 lbl_805ABDB8[];
extern u32 lbl_805ABF30[];
extern u32 lbl_805AC1F0[];
extern u32 lbl_805AC3C8[];
extern u32 lbl_805AC698[];
extern u32 lbl_805AC9A8[];
extern u32 lbl_805ACC00[];
extern u32 lbl_805ACD80[];
}

/* `_CP_VECTOR` is `ef.h`'s type (three words).  That header is not includable here: its
 * `setVec3` declaration returns void, where this range's call sites read the callee's r3 (see
 * `fn_80192108`/`fn_80192204`), so the outbox carries the header fix instead. */
struct _CP_VECTOR;

/* The 3x3 matrix `fn_805012E8` fills (the SDK's `Mtx33` layout - nine floats; the callee's own body
 * stores only at +0x20, the 3x3's last element, and the frame this unit's `fn_80191CE4` measures is
 * sized for this object, not for a second 0x30-byte matrix).
 * size: 0x24 */
struct EmMtx33 {
    /* +0x00 */ f32 m[3][3];
};



/* The user-data item the six-argument class methods take (`src/enemy/fn_80138074.c` owns the full
 * 0x18-byte record `UserDataItem`; this range reads the +0x04 type byte).
 * size: 0x18 */
struct EmUserItem {
    /* +0x00 */ u32 key_0x00;
    /* +0x04 */ u8 type_0x04;   /* 0xFF admits the call */
    /* +0x05 */ u8 unused_0x05[0x13];
};

/* A holder of one matrix pointer (`joint_mtx_store`/`joint_mtx_load`'s argument; the owner spells it
 * `MtxHolder`).
 * size: 0x04 */
struct EmMtxHolder {
    /* +0x00 */ MTX34* mtx_0x00;
};

/* The static vector pair `fn_80192204` builds (`VEC3[2]` per 0x18-byte object).
 * size: 0x18 */
struct EmVecPair {
    /* +0x00 */ VEC3 vec_0x00;
    /* +0x0C */ VEC3 vec_0x0C;
};

/* The request record `fn_80192108` fills in.
 * size: 0x18 */
struct EmEffRequest {
    /* +0x00 */ u32 id_0x00;      /* always 0x17 */
    /* +0x04 */ VEC3 pos_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 unused_0x11;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ s16 field_0x14;
    /* +0x16 */ u8 unused_0x16[0x02];
};

/* ------------------------------------------------------------------------------------------------ */
/* the shared pool this range reads                                                                  */
/* ------------------------------------------------------------------------------------------------ */
extern const f32 lbl_80797F24;
extern const f32 lbl_8079822C;
extern const f32 lbl_80798230;
extern const f32 lbl_80798234;

/* The five static vectors `fn_80192204` builds (the last one is the default `fn_80192108` copies
 * from). */
extern EmVecPair vec_pair_80191598_0;
extern EmVecPair vec_pair_80191598_1;
extern EmVecPair vec_pair_80191598_2;
extern EmVecPair vec_pair_80191598_3;
extern VEC3 vec_default_80191598;

/* The one-shot latch `fn_80192108` sets. */
extern s8 lbl_80794AA0;

/* The `.data` word `fn_80191B4C` hands to `fn_8012A014` (the map's own label; it sits in the class
 * table block this range's entry points live in). */
extern u8 lbl_805AAA74[];

u8 em_parts_damage_level_get(struct _ENEMY_WORK* work, u8 part);

void cpSetRotMatrix(struct _CP_VECTOR* rot, MTX34* mtx);

extern "C" {
/* The plain callees.  Their signatures are this range's own call sites (the arity and the return
 * width the target's registers show); they belong in their owner's header or in
 * `unsplit/enemy.h`, and the outbox carries that list - the same interim spelling
 * `enemy/fn_8013ACC4.cpp` uses for its neighbours. */
void fn_803B9BA0(EmActWork* self, VEC3* pos, s32 value);

void joint_mtx_store(EmMtxHolder* holder, void* src);
void joint_mtx_load(EmMtxHolder* holder, void* mtx);
void fn_8008E8D0(void* holder, void* vec);
void fn_8008EE68(void* holder, void* mtx);
void fn_800FBB90(MTX34* mtx, VEC3* vec);
void eft_rot_vec_copy(void* dst, void* src);
void fn_805012E8(EmMtx33* dst, const MTX34* src);

void fn_800516F0(void* mtx);

/* This range's own entry points (rule 2: declared where they are defined, i.e. here). */
void fn_80191598(EmActWork* self, u8* out_class, u8* out_state);
void fn_80191990(EmActWork* self);
u32 fn_80191A6C(EmActWork* self);
u32 fn_80191AD8(EmActWork* self);
void fn_80191AE8(EmActWork* self, u32 kind);
s32 fn_80191B4C(EmActWork* self, u32 arg);
void fn_80191CDC(EmUserData* self);
void fn_80191CE4(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item);
void fn_80191E30(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item);
void fn_80191EF8(EmActWork* self);
void fn_80192080(EmActWork* self);
void fn_80192108(EmEffRequest* out, u8 a, s16 b, s16 c);
void* fn_801921A8(void* p, u32 flag);
void fn_80192204(void);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182C40 - rotate a per-kind offset into the work's position and write the resulting relative
 * vector the movement code steers by.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182C40(_ENEMY_WORK* self, u32 kind, void* out) {
    VEC3 v;
    VEC3 rel;
    VEC3_ctor(&v);
    v.x = lbl_80797E88;
    v.y = lbl_80797E88;
    v.z = lbl_80797E8C;
    s32 offset;
    switch (kind & 0xFF) {
    case 1:
        offset = 0x9555;
        v.z = lbl_80797E8C;
        break;
    case 2:
        offset = 0xAAAB;
        v.z = lbl_80797E94;
        break;
    case 10:
        offset = 0xA000;
        v.z = lbl_80797E98;
        break;
    case 11:
        offset = 0x4E39;
        v.z = lbl_80797E90;
        break;
    default:
        offset = 0x11C7;
        v.z = lbl_80797E90;
        break;
    }
    rotVecY(&v, (u16)(self->field_0x1C0 + offset));
    addVec3(&rel, &self->pos, &v);
    copyVec3((nw4r::math::VEC3*)out, &rel);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182D44 - forward the value to the motion setter only for team 0x10.
 * ------------------------------------------------------------------------------------------------ */
void fn_80182D44(_ENEMY_WORK* self, u32 arg) {
    if (self->team == 0x10) {
        fn_80130CDC(self, (s16)arg);
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182D5C - per-(map,area) motion start: two timed motion sets, then the area's pair.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80182D5C(_ENEMY_WORK* self) {
    {
        f32 t = lbl_80797E88;
        em_motion_param_set(self, 0, t);
    }
    {
        f32 t = lbl_80797E9C;
        em_motion_param_set(self, 0x1E, t);
    }

    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 5:
            fn_80126324(self, 6, 7, lbl_80797EA0);
            break;
        case 6:
            fn_80126324(self, 9, 10, lbl_80797EA4);
            break;
        case 7:
            fn_80126324(self, 0xD, 0xE, lbl_80797EA0);
            break;
        case 8:
            fn_80126324(self, 8, 9, lbl_80797EA0);
            break;
        case 0xC:
            fn_80126324(self, 0x19, 0x1A, lbl_80797EA0);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 1:
            fn_80126324(self, 6, 7, lbl_80797EA8);
            break;
        case 2:
            fn_80126324(self, 7, 8, lbl_80797EA0);
            break;
        case 3:
            fn_80126324(self, 9, 10, lbl_80797EAC);
            break;
        case 4:
            fn_80126324(self, 5, 6, lbl_80797EA0);
            break;
        case 5:
            fn_80126324(self, 7, 8, lbl_80797EA0);
            break;
        case 6:
            fn_80126324(self, 0xD, 0xE, lbl_80797EAC);
            break;
        case 7:
            fn_80126324(self, 0, 2, lbl_80797EA0);
            break;
        case 8:
            fn_80126324(self, 6, 7, lbl_80797EA0);
            break;
        case 10:
            fn_80126324(self, 0, 1, lbl_80797EAC);
            break;
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182F60 - whether this map/area pair has a motion set at all.
 * ------------------------------------------------------------------------------------------------ */
extern "C" u32 fn_80182F60(_ENEMY_WORK* self) {
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((u32)(self->area_no - 5) <= 1U || (s32)self->area_no == 8) {
            return 1;
        }
        break;
    case 3:
        if ((s32)self->area_no == 1 || (s32)self->area_no == 5 || (s32)self->area_no == 7
            || (s32)self->area_no == 10) {
            return 1;
        }
        break;
    }
    return 0;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80182FF8 - arm the motion that this map/area pair starts with.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80182FF8(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 5);
    fn_80133BB4(self);
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183440 - the 0xC-byte helper's second constructor: base first, then this class's vtable.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void* fn_80183440(void* self) {
    em_res_user_data_ctor(self);
    *(void**)self = lbl_805AD340;
    return self;
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183A54 - the map-0x15 (teardown) step: run it once `em_die_ck` and the state check agree.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183A54(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (fn_801337FC(self) == 1) {
            fn_8012E664(self);
        }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183AA0 - per-team step dispatch (`team` 0x10/0x11/0x15).
 * ------------------------------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------------------------------
 * fn_80183AD0 / fn_80183B4C / fn_80183BC8 / fn_80183C44 / fn_80183CC4 - the armed-motion steps.
 * Each is the same two-phase action: phase 0 latches the state byte, arms the action and hands the
 * motion pair to the action setter; phase 1 waits for `em_mot_end_ck` and runs the matching finish
 * call.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183AD0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80183B4C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x14, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80183BC8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x1D, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80183C44(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80183CC4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x36, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80183D44(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0xCA, 6, 0);
        fn_801303EC(self, lbl_80797E88);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183AA0 - per-`team` step dispatch into the five armed-motion steps above (tail calls).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183AA0(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_80183718(self);
        return;
    case 0x11:
        fn_801837B0(self);
        return;
    case 0x15:
        fn_80183A54(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183DCC - per-`state_sub` step dispatch into the same five steps (tail calls).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183DCC(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80183AD0(self);
        return;
    case 1:
        fn_80183B4C(self);
        return;
    case 2:
        fn_80183BC8(self);
        return;
    case 4:
        fn_80183C44(self);
        return;
    case 5:
        fn_80183CC4(self);
        return;
    case 7:
        fn_80183D44(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183E20 - the armed-motion step with the (0x1A, 0xA) pair.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183E20(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
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
 * fn_80183E9C - the four-phase armed-motion step: arm, run the sub-action and its 0x708-frame timer,
 * then count it down and finish.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183E9C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            u8 state = self->state;
            self->state = state + 1;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2: {
        fn_8013221C(self, lbl_80797EC0, 1, 0xF);
        s32 left = self->timer_0x020 - 1;
        self->timer_0x020 = left;
        if (left <= 0) {
            u8 state = self->state;
            self->state = state + 1;
            em_mot_set(self, 5, 4, 0);
            fn_80132264(self);
        }
        break;
    }
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80183FB8 / fn_80184038 / fn_801840B4 / fn_80184138 - more armed-motion steps, same two phases.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80183FB8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x14, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80184038(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801840B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 2, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 7);
        }
        break;
    }
}

extern "C" void fn_80184138(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
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
 * fn_801841B4 - the armed-motion step with two frame checks and its finish step.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801841B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 4, 0);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797EC4, lbl_80797E88) == 1) {
            fn_80136D14(self);
            if (em_frame_check(self, 1, lbl_80797EA4, lbl_80797E88) == 1) {
                fn_80131DB4(self);
                fn_80131DF4(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            fn_8012E694(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184280 / fn_801842FC - the last two armed-motion steps of the second family.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80184280(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xB, 6, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801842FC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        fn_801303EC(self, lbl_80797E88);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797EC8, lbl_80797E88) == 1) {
            fn_80136D14(self);
            if (em_frame_check(self, 1, lbl_80797ECC, lbl_80797E88) == 1) {
                fn_80131DB4(self);
                fn_80131DF4(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            fn_8012E694(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801843D4 / fn_80184434 / fn_80184458 - the three per-`state_sub` dispatchers of the second
 * step family (tail calls).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801843D4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80183E20(self);
        return;
    case 1:
        fn_80183E9C(self);
        return;
    case 2:
        fn_80183FB8(self);
        return;
    case 3:
        fn_80184038(self);
        return;
    case 4:
        fn_801840B4(self);
        return;
    case 5:
        fn_80184138(self);
        return;
    case 7:
        fn_80184280(self);
        return;
    }
}

extern "C" void fn_80184434(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_80183FB8(self);
        return;
    case 6:
        fn_801841B4(self);
        return;
    }
}

extern "C" void fn_80184458(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_80183FB8(self);
        return;
    case 3:
        fn_80184038(self);
        return;
    case 8:
        fn_801842FC(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184488 - per-`team` dispatch into the second step family (tail calls).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80184488(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_801843D4(self);
        return;
    case 0x11:
        fn_80184434(self);
        return;
    case 0x15:
        fn_80184458(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801844B8 - the armed-motion step that hands a float to `em_approach_start` and waits on
 * `em_approach_step`.  The call's argument order is the band header's ABI-equivalent `(u32, f32)` view
 * (see the header's residual note).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801844B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_approach_start(self, lbl_80797E88, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018454C - the same step, but the argument picks one of two action tables.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8018454C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (u8)arg == 1 ? lbl_8056FF90 : lbl_8056FF50, 0, 0, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, (u8)arg == 1 ? lbl_8056FF90 : lbl_8056FF50) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018460C - the armed-motion step whose phase 1 gates `em_turn_to_target` on an argument and a frame
 * check.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8018460C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 6, 0);
        break;
    }
    case 1:
        if ((u8)arg == 1 && em_frame_check(self, 3, lbl_80797ED0, lbl_80797ED4) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801846BC - the armed-motion step whose argument picks the motion pair and whether the target
 * distance is clamped.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801846BC(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        u32 motion = (u8)arg1 == 1 ? 9 : 2;
        em_mot_set(self, (u16)motion, 0xA, 0);
        em_approach_start(self, lbl_80797E88, 0);
        if ((u8)arg2 == 1 && self->value_0x378 > lbl_80797ED8) {
            self->value_0x378 = lbl_80797ED8;
        }
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018479C - the armed-motion step shared by two sub-states (`em_busy_set`/`em_busy_timer_reset` first).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8018479C(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);

    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xA, 0);
        em_approach_start(self, lbl_80797E88, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_state_set(self, 5, 5);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018484C - the armed-motion step whose argument picks the motion pair, the blend float and the
 * `em_approach_step` mask.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8018484C(_ENEMY_WORK* self, u32 arg1, u32 arg2) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 0xA, 0);
        em_hit_window_set(self, 0, 0xD, 2);

        f32 blend;
        switch ((u8)arg2) {
        default:
            blend = lbl_80797E88;
            break;
        case 1:
            blend = lbl_80797EDC;
            break;
        case 2:
            blend = lbl_80797EE0;
            break;
        }
        em_approach_start(self, blend, 0);
        break;
    }
    case 1:
        if (em_approach_step(self, 0, (u16)((u8)arg1 == 1 ? 0xC0 : 0x40)) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_8018493C - the armed-motion step with the 0x96/0x5A countdown derived from `bits_0x1EC`.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_8018493C(_ENEMY_WORK* self, u32 arg) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 4, 0);

        u32 ticks = self->bits_0x1EC & 0x1F;
        self->timer_0x020 = ticks + 0x96;

        f32 blend;
        switch ((u8)arg) {
        default:
            blend = lbl_80797E88;
            break;
        case 1:
            blend = lbl_80797EE4;
            break;
        case 2:
            blend = lbl_80797E88;
            break;
        case 3:
            blend = lbl_80797EE4;
            self->timer_0x020 = ticks + 0x5A;
            break;
        }
        em_approach_start(self, blend, 0);
        break;
    }
    case 1: {
        u32 done = 0;
        if ((u8)arg != 2) {
            s32 left = self->timer_0x020 - 1;
            self->timer_0x020 = left;
            if (left <= 0) {
                done = 1;
            }
        }
        if (em_approach_step(self, 0, 0x40) == 1 || done == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184A5C / fn_80184B0C / fn_80184B78 - the three per-`state_sub` dispatchers of the third step
 * family (tail calls; the dense ones are jump tables).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80184A5C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801844B8(self);
        return;
    case 1:
        fn_8018454C(self, 0);
        return;
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 4:
        fn_801846BC(self, 1, 0);
        return;
    case 5:
        fn_8018479C(self);
        return;
    case 6:
        fn_8018454C(self, 0);
        return;
    case 7:
        fn_8018484C(self, 0, 1);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 9:
        fn_8018484C(self, 1, 0);
        return;
    case 10:
        fn_801846BC(self, 0, 1);
        return;
    case 11:
        fn_801846BC(self, 1, 1);
        return;
    case 13:
        fn_8018484C(self, 0, 0);
        return;
    case 16:
        fn_8018484C(self, 0, 2);
        return;
    }
}

extern "C" void fn_80184B0C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 12:
        fn_8018493C(self, 1);
        return;
    case 14:
        fn_8018493C(self, 0);
        return;
    case 15:
        fn_8018493C(self, 2);
        return;
    case 17:
        fn_8018454C(self, 1);
        return;
    case 18:
        fn_8018493C(self, 3);
        return;
    }
}

extern "C" void fn_80184B78(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 2:
        fn_8018460C(self, 0);
        return;
    case 3:
        fn_801846BC(self, 0, 0);
        return;
    case 8:
        fn_8018460C(self, 1);
        return;
    case 12:
        fn_8018493C(self, 1);
        return;
    case 14:
        fn_8018493C(self, 0);
        return;
    case 15:
        fn_8018493C(self, 2);
        return;
    case 17:
        fn_8018454C(self, 1);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184CE0 - the armed-motion step with the `field_0x482`-selected approach float.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80184CE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570010, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_80570010) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797EE8);
        } else {
            fn_80136D4C(self, lbl_80797EEC);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184BF8 - per-`team` dispatch into the third step family (tail calls).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80184BF8(_ENEMY_WORK* self) {
    switch (self->team) {
    case 0x10:
        fn_80184A5C(self);
        return;
    case 0x11:
        fn_80184B0C(self);
        return;
    case 0x15:
        fn_80184B78(self);
        return;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_80184C28 - fn_80184CE0's sibling with the other action table.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80184C28(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FFD0, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_8056FFD0) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797EE8);
        } else {
            fn_80136D4C(self, lbl_80797EEC);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801856B8 / fn_80185738 / fn_801857B8 / fn_80185838 / fn_801858B8 - the fifth family's armed
 * motion steps: arm the motion with the action setter, then finish on `em_mot_end_ck`.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801856B8(_ENEMY_WORK* self) {
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

extern "C" void fn_80185738(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x35, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801857B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x14, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185838(_ENEMY_WORK* self) {
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
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801858B8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x14, 0, 3);
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
 * fn_80185BEC / fn_80185FA0 - two more of the same family.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80185BEC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80185FA0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x14, 0, 3);
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
 * fn_80185D60 - the team-selected motion step (`em_busy_set` runs first).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80185D60(_ENEMY_WORK* self) {
    em_busy_set(self);

    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)(self->team == 0x10 ? 0x3D : 0x3B), 0, 0);
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
 * fn_80185B0C - the motion step that stores the target angle byte: the difference between the body
 * angle and `field_0x1C0`, quantised into `state_0x007` (0 / 0x80 / the >>8 byte).
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_80185B0C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2D, 0xA, 0);

        u32 diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff > 0x8000) {
            if (diff > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)diff >> 8);
        }
        break;
    }
    case 1:
        em_turn_in_window(self, lbl_80797EB4, lbl_80797ECC, (s32)(self->state_0x007 << 8));
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------
 * fn_801850F8 - the motion step that runs the angle-driven approach: two `em_frame_check` gates and
 * a clamped blend of `fn_8012F8EC`'s value.
 * ------------------------------------------------------------------------------------------------ */
extern "C" void fn_801850F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x39, 0xA, 0, 1);
        break;
    }
    case 1:
        fn_80136D4C(self, lbl_80797EEC);

        if (em_frame_check(self, 1, lbl_80797F00, lbl_80797E88) == 1) {
            f32 blend = lbl_80797EEC * (fn_8012F8EC(self) - lbl_80797EB4);
            if (blend > lbl_80797EA4) {
                blend = lbl_80797EA4;
            }
            fn_8012FE3C(self, blend + fn_8013026C(self));
        }

        if (em_frame_check(self, 1, lbl_80797F04, lbl_80797E88) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ *
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------ */

/* 0x8018B3B8 - `state_sub` 8 of the multi-motion dispatch: a tail call into the armed-motion step. */
void fn_8018B3B8(_ENEMY_WORK* self) {
    fn_8018479C(self);
}

/* 0x8018B3BC - `state_sub` 9: the motion-table step with id 8, no sub. */
void fn_8018B3BC(_ENEMY_WORK* self) {
    fn_80184D98(self, 8, 0);
}

/* 0x8018B3C8 - the multi-motion dispatcher keyed on the work's +0x1E6 class (0..9). */
void fn_8018B3C8(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8018AB94(self);
        return;
    case 1:
        fn_8018AC64(self);
        return;
    case 2:
        fn_8018ACEC(self);
        return;
    case 3:
        fn_8018AD68(self);
        return;
    case 4:
        fn_8018ADE8(self);
        return;
    case 5:
        fn_8018AE7C(self);
        return;
    case 6:
        fn_8018B1C4(self);
        return;
    case 7:
        fn_8018B258(self);
        return;
    case 8:
        fn_8018B3B8(self);
        return;
    case 9:
        fn_8018B3BC(self);
        return;
    default:
        return;
    }
}

/* 0x8018B418 - the run's opening step machine (states 0..10) on the work's +0x5 byte. */
void fn_8018B418(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 0, 0);
        em_demo_reset(self, 0);
        em_demo_pos_set(self, lbl_80798010, lbl_80798014, lbl_80798018);
        em_demo_rot_set(self, lbl_80797E88, lbl_8079801C, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x37, 0, 0);
            em_demo_enable(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F78);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        if (em_demo_time_ck(0x1DE) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x2C, 0x18, 0, 1);
            em_demo_pos_set(self, lbl_80798020, lbl_80798014, lbl_80798024);
        }
        break;
    case 3:
        if (em_frame_check(self, 3, lbl_80797F6C, lbl_80797F08) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797E9C);
        }
        if (em_demo_time_ck(0x1FA) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x2C, 0, 0x1E);
            em_demo_pos_set(self, lbl_80798028, lbl_8079802C, lbl_80798030);
            em_demo_rot_set(self, lbl_80797E88, lbl_8079801C, lbl_80797E88);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805ABDB8, NULL, 5, 0);
        }
        break;
    case 4:
        if (em_frame_check(self, 3, lbl_80797F80, lbl_80798034) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797EB8);
        }
        if (em_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        self->pos.y = lbl_8079802C;
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805ABDB8, NULL, 5, 0);
        if (em_demo_time_ck(0x2B2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x29, 0xE, 0, 1);
        }
        break;
    case 5:
        if (em_frame_check(self, 3, lbl_80797F6C, lbl_80797F08) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797E9C);
        }
        if (em_frame_check(self, 3, lbl_80797F80, lbl_80798034) == 1U && (em_demo_frame_get() & 3) == 0) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F50);
            eft_em_spawn(self, 0xCD, 0x26, &sp8, lbl_80797EB8);
        }
        if (em_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_8079800C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ABF30, 0);
        if (em_demo_time_ck(0x382) == 1U) {
            self->state = (u8)(self->state + 1);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set_blend(self, 0x3C, 0xE, 0x6C, 1);
            em_demo_pos_set(self, lbl_8079803C, lbl_80798040, lbl_80798044);
        }
        break;
    case 6:
        if (em_frame_check(self, 0, lbl_80798048, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797EA4);
            eft_em_spawn(self, 0xCC, 0x16, &sp8, lbl_80797E9C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ABF30, 0);
        if (em_demo_time_ck(0x3B0) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x40, 0, 0);
            em_mot_speed_set(self, lbl_8079804C);
            em_demo_pos_set(self, lbl_80798050, lbl_80798054, lbl_80798058);
            em_demo_rot_set(self, lbl_80797E88, lbl_8079805C, lbl_80797E88);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC1F0, NULL, 3, 0);
            eft009_spawn_at_joint(self, 3U, 0x11U, 0, lbl_80797EB8);
        }
        break;
    case 7:
        if (em_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_80797F10, lbl_80797E88);
            eft_em_spawn(self, 0x32, 3, &sp8, lbl_8079804C);
        }
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC1F0, NULL, 3, 0);
        if (em_demo_time_ck(0x420) == 1U) {
            self->state = (u8)(self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3D, 0, 0);
            em_demo_pos_set(self, lbl_80798060, lbl_80798064, lbl_80798068);
            em_demo_rot_set(self, lbl_80797E88, lbl_8079806C, lbl_80797E88);
        }
        break;
    case 8:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0x15U, 0x24U, 0, lbl_80797EC0);
        }
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, lbl_80798070, lbl_80798074, lbl_80798078);
        }
        break;
    case 9:
        if (em_demo_time_ck(0x4A6) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x1A, 0xA, 0, 1);
        }
        break;
    case 10:
        if (em_demo_time_ck(0x582) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0x1E, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BBD8 - the first arm/step pair: a two-state opener for the motion-table step. */
void fn_8018BBD8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_80798070, lbl_80798074, lbl_80798078);
        em_demo_rot_set(self, lbl_80797E88, lbl_8079806C, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BC7C - the run's second step machine (states 0..7). */
void fn_8018BC7C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_8079807C, lbl_80798080, lbl_80798084);
        em_demo_rot_set(self, lbl_80797E88, lbl_80797F44, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 4, 0, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 5, 0, 0x2A);
            em_demo_pos_set(self, lbl_80798088, lbl_80798080, lbl_8079808C);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F9C, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 8U, 0x92U, 0, lbl_80798090);
        }
        if (em_demo_time_ck(0x134) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_pos_set(self, lbl_80798094, lbl_80798098, lbl_8079809C);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980A0, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_807980A4, lbl_807980A8, lbl_807980AC);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980B0, lbl_80797E88);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x4AE) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 2, 0x1E, 0, 1);
            return;
        }
        break;
    case 7:
        if (em_frame_check(self, 0, lbl_80797FA8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 8U, 0x92U, 0, lbl_80798090);
        }
        if (em_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0xDU, 0x92U, 0, lbl_80798090);
        }
        break;
    default:
        break;
    }
}

/* 0x8018BF4C - the second arm/step pair opener. */
void fn_8018BF4C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, lbl_807980B4, lbl_807980A8, lbl_807980B8);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980B0, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018BFF0 - the run's third step machine (states 0..8). */
void fn_8018BFF0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_807980BC, lbl_807980C0, lbl_807980C4);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980C8, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            return;
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x134) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_807980BC, lbl_807980C0, lbl_807980C4);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980CC, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x190) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x420) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980D8, lbl_80797E88);
            return;
        }
        break;
    case 6:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x18, 0, 0);
            em_demo_pos_set(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980D8, lbl_80797E88);
            return;
        }
        break;
    case 7:
        em_turn_in_window(self, lbl_807980DC, lbl_807980E0, 0x4000);
        if (em_demo_time_ck(0x49A) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0x1E, 0, 1);
            em_demo_rot_set(self, lbl_80797E88, lbl_807980E4, lbl_80797E88);
            return;
        }
        break;
    case 8:
        if (em_demo_time_ck(0x51E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x14, 0x28, 0, 1);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C2CC - the third arm/step pair opener. */
void fn_8018C2CC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 0, 0);
        em_demo_pos_set(self, lbl_807980D0, lbl_807980C0, lbl_807980D4);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980E8, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C370 - the run's fourth step machine (states 0..3). */
void fn_8018C370(_ENEMY_WORK* self) {
    VEC3 sp8;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_807980EC, lbl_807980F0, lbl_807980F4);
        em_demo_rot_set(self, lbl_80797E88, lbl_807980F8, lbl_80797E88);
        return;
    case 1:
        if (em_demo_time_ck(0x1E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 2, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797ED0, lbl_80797E88) == 1U) {
            setVector3(&sp8, lbl_80797E88, lbl_807980FC, lbl_80797F34);
            fn_801049D0(self, 8, 0x16, 0, &sp8, lbl_80798100);
        }
        if (em_demo_time_ck(0xD2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_reset(self, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0x456) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 0, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, lbl_80798104, lbl_80798108, lbl_8079810C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798110, lbl_80797E88);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C528 - the fourth arm/step pair opener. */
void fn_8018C528(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_80798104, lbl_80798108, lbl_8079810C);
        em_demo_rot_set(self, lbl_80797E88, lbl_80798110, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018C5CC - the run's fifth step machine (states 0..6). */
void fn_8018C5CC(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x168) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_enable(self);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797FF8, lbl_80797E88);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0x13U, 0xDU, 0x8000, lbl_80798090);
            eft009_spawn_at_joint(self, 0x13U, 0xFU, 0, lbl_80798090);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
        if (em_demo_time_ck(0x19E) == 1U) {
            self->state = (u8)(self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set_blend(self, 0x3B, 6, 6, 1);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
        if (em_demo_time_ck(0x242) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 2, 0xE, 0x6A);
            return;
        }
        break;
    case 4:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805AC3C8, 0);
        if (em_demo_time_ck(0x26C) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_pos_set(self, lbl_80798114, lbl_80798118, lbl_8079811C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797FF8, lbl_80797E88);
        }
        /* fallthrough */
    case 5:
        if (em_demo_time_ck(0x57A) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x18, 0, 0);
            em_mot_speed_set(self, lbl_80798120);
            em_demo_pos_set(self, lbl_80798124, lbl_80798128, lbl_8079812C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798130, lbl_80797E88);
            return;
        }
        break;
    case 6:
        self->field_0x1C0 = fn_80133DB0(0x49F5, (u16)self->field_0x1C0, 0x160);
        if (em_demo_time_ck(0x652) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x14, 0xA, 0);
            em_demo_pos_set(self, lbl_80798124, lbl_80798128, lbl_8079812C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798134, lbl_80797E88);
        }
        break;
    default:
        break;
    }
}

/* 0x8018C998 - the fifth arm/step pair opener. */
void fn_8018C998(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, lbl_80798124, lbl_80798128, lbl_8079812C);
        em_demo_rot_set(self, lbl_80797E88, lbl_80798134, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018CA3C - the run's sixth step machine (states 0..7). */
void fn_8018CA3C(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x1C2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_enable(self);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_rot_set(self, lbl_80797F14, lbl_80798138, lbl_80797E88);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC698, lbl_805AC9A8, 7, 3);
            fn_80133C3C(self);
            return;
        }
        break;
    case 2:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC698, lbl_805AC9A8, 7, 3);
        fn_80133C3C(self);
        if (em_demo_time_ck(0x1F8) == 1U) {
            self->state = (u8)(self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0xC, 6);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805AC698, lbl_805AC9A8, 7, 3);
        fn_80133C3C(self);
        if (em_demo_time_ck(0x26C) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_pos_set(self, lbl_8079813C, lbl_80797ED0, lbl_80798140);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797F70, lbl_80797E88);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x29C) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 2, 0x14, 0, 1);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x566) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x19, 0, 0x14);
            em_demo_pos_set(self, lbl_80798144, lbl_80798148, lbl_8079814C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798150, lbl_80797E88);
            return;
        }
        break;
    case 6:
        em_turn_in_window(self, lbl_80797ED0, lbl_80798154, -0x4000);
        if (em_demo_time_ck(0x5B8) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0xA, 0, 1);
            em_demo_pos_set(self, lbl_80798144, lbl_80798148, lbl_8079814C);
            em_demo_rot_set(self, lbl_80797E88, lbl_80798158, lbl_80797E88);
            return;
        }
        break;
    case 7:
        if (em_demo_time_ck(0x6F2) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 0x14, 0xE, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018CDE0 - the sixth arm/step pair opener. */
void fn_8018CDE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, lbl_80798144, lbl_80798148, lbl_8079814C);
        em_demo_rot_set(self, lbl_80797E88, lbl_80798158, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018CE84 - the run's seventh step machine (states 0..5). */
void fn_8018CE84(_ENEMY_WORK* self) {
    VEC3 sp8;
    f32 spC;

    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x4A8) == 1U) {
            self->state = (u8)(self->state + 1);
            em_demo_enable(self);
            em_fall_start_view1(self, lbl_80797E88);
            em_mot_set(self, 0x40, 0, 0);
            em_demo_rot_set(self, lbl_80797E88, lbl_80797E88, lbl_80797E88);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ACC00, 0);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
            eft009_spawn_at_joint(self, 0x13U, 0x10U, 0, lbl_8079804C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ACC00, 0);
        if (em_demo_time_ck(0x4DE) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 0x3B, 8, 6, 1);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
            get_joint_wpos_em(self, 0x10U, &sp8);
            spC = lbl_80797EE8 + self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 3U, self->field_0x1C0,
                        lbl_80797EC0 * get_em_chg_scale(self));
        }
        if (em_demo_time_ck(0x4E6) == 0) {
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805ACC00, 0);
        } else {
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805ACD80, NULL, 5, 0);
        }
        if (em_demo_time_ck(0x584) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 2, 0, 0);
            em_demo_pos_set(self, lbl_8079815C, lbl_80798160, lbl_80798164);
            return;
        }
        break;
    case 4:
        if (em_demo_time_ck(0x662) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set_blend(self, 1, 0xA, 0, 1);
            em_demo_pos_set(self, lbl_80798168, lbl_8079816C, lbl_80798170);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x768) == 1U) {
            self->state = (u8)(self->state + 1);
            em_mot_set(self, 1, 2, 0);
        }
        break;
    default:
        break;
    }
}

/* 0x8018D1AC - the seventh arm/step pair opener. */
void fn_8018D1AC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, lbl_80798168, lbl_8079816C, lbl_80798170);
        em_demo_rot_set(self, lbl_80797E88, lbl_80797E88, lbl_80797E88);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

/* 0x8018D250 - the `state_sub` class dispatcher (0..13) into this unit's twelve step machines. */
void fn_8018D250(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8018B418(self);
        return;
    case 1:
        fn_8018BBD8(self);
        return;
    case 2:
        fn_8018BC7C(self);
        return;
    case 3:
        fn_8018BF4C(self);
        return;
    case 4:
        fn_8018BFF0(self);
        return;
    case 5:
        fn_8018C2CC(self);
        return;
    case 6:
        fn_8018C370(self);
        return;
    case 7:
        fn_8018C528(self);
        return;
    case 8:
        fn_8018C5CC(self);
        return;
    case 9:
        fn_8018C998(self);
        return;
    case 10:
        fn_8018CA3C(self);
        return;
    case 11:
        fn_8018CDE0(self);
        return;
    case 12:
        fn_8018CE84(self);
        return;
    case 13:
        fn_8018D1AC(self);
        return;
    default:
        return;
    }
}

/* 0x8018D2B0 - the action dispatcher (0..13) plus the two shared post-checks. */
void fn_8018D2B0(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80183DCC(self);
        break;
    case 1:
        fn_80184488(self);
        break;
    case 2:
        fn_80184BF8(self);
        break;
    case 5:
        fn_801861E4(self);
        break;
    case 6:
        fn_80186960(self);
        break;
    case 7:
        fn_80189C7C(self);
        break;
    case 10:
        fn_8018A974(self);
        break;
    case 11:
        fn_8018AB64(self);
        break;
    case 12:
        fn_8018B3C8(self);
        break;
    case 13:
        fn_8018D250(self);
        break;
    }
    if (self->team != 0x15 && self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* 0x8018D370 - the per-frame effect spawner: measures the joint point against the water/height
 * gates and picks one of the four ground/air effect ids. */
void fn_8018D370(_ENEMY_WORK* self) {
    VEC3 sp8;
    MTX34 sp18;
    u16 ang;
    u16 mot;

    VEC3_ctor(&sp8);
    MTX34_ctor(&sp18);
    if (em_alt_mode_ck_c1(self) != 0) {
        ang = calcVecAngX(&self->vec_0x76C);
        if ((u16)(ang + 0x8000) > 0x671B) {
            mot = em_get_mot_no(self);
            if (mot != 0x4A && mot != 0x57 && mot != 0x5B && mot != 0x63 && mot != 0xC8) {
                setVector3(&sp8, lbl_80797E88, lbl_80797E88, lbl_80797F78);
                get_joint_wmat_em(self, 0x16, &sp18);
                mulVecMat(&sp8, &sp18);
                sp8.x += sp18.m[0][3];
                sp8.y += sp18.m[1][3];
                sp8.z += sp18.m[2][3];
                if (em_water_check_view1(self) == 1U && sp8.y < self->field_0x210) {
                    if ((system_w.field_0x0c & 0x1F) == 0) {
                        if ((u16)(ang + 0x8000) > 0x6E38U) {
                            eft_spawn_type10(self, 3, 0x16, &sp8, lbl_8079800C);
                            return;
                        }
                        eft_spawn_type10(self, 5, 0x16, &sp8, lbl_8079800C);
                    }
                } else if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(ang + 0x8000) > 0x6E38U) {
                        eft_spawn_type10(self, 0x1D, 0x16, &sp8, lbl_80797E9C);
                        return;
                    }
                    eft_spawn_type10(self, 0x1C, 0x16, &sp8, lbl_80797E9C);
                }
            }
        }
    }
}

/* 0x8018D558 - the effect-spawn router: builds the spawn position from the work (or a joint) and
 * spawns through `eft009_set_pos` (a fixed spawn point) or `eft009_spawn_at_joint` (a joint spawn).  `arg1`
 * selects the position/height source (0 = the +0x210 height, 1 = a joint, 2 = the +0x20C height),
 * `arg2` the effect kind, `arg3` the joint (0xFF = the work's own position), `arg4` the joint flag
 * and `arg5` the scale. */
void fn_8018D558(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 arg5) {
    VEC3 vec;
    u8 var;

    var = arg2;
    VEC3_ctor(&vec);
    switch (arg1) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (arg2) {
            case 2:
                var = 0xC;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, 0xE, arg4, arg5);
                }
                break;
            case 4:
            case 6:
                var = 0x13;
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xF, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, arg3, 0xF, arg4, arg5);
                }
                break;
            default:
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                }
                break;
            }
        } else {
            if ((u32)(arg2 - 12) <= 13) {
                return;
            }
            if (arg2 == 0x26) {
                return;
            }
            if (arg3 == 0xFF) {
                vec.x = self->pos.x;
                vec.y = lbl_80797EE8 + self->field_0x20C;
                vec.z = self->pos.z;
            }
        }
        if (arg3 == 0xFF) {
            eft009_set_pos(var, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
            return;
        }
        eft009_spawn_at_joint(self, arg3, var, arg4, arg5);
        return;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            if (arg2 == 0 || arg2 == 3) {
                if (arg3 == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = lbl_80797EE8 + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x11, &vec, (_CP_VECTOR*)&self->field_0x1BC, arg5, self->area_no);
                    return;
                }
                eft009_spawn_at_joint(self, arg3, 0x11, arg4, arg5);
                return;
            }
            return;
        }
        if (arg3 == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        eft_spawn_pos_in_area(&vec, self->area_no, arg2, arg4, arg5 * get_em_chg_scale(self));
        return;
    case 2:
        if (arg3 == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, arg3, &vec);
        }
        vec.y = self->field_0x20C;
        {
            f32 scale = arg5 * get_em_chg_scale(self);
            if (self->pos.y <= lbl_80798174 + self->field_0x20C) {
                eft_spawn_type11_view1(self, &vec, arg2, scale);
            }
        }
        return;
    default:
        return;
    }
}

/* 0x80191038 - the team-0x10 TEV tint stepper: fades the two +0x350/+0x352 highlight words and the
 * +0x354..+0x358 fade triple toward the team's key colour. */
void fn_80191038(_ENEMY_WORK* self) {
    _GXColor color;
    s16 v;
    u8 flag;

    if (self->team == 0x10) {
        flag = (self->field_0x1E2 - 2) == 0;
        if ((fn_8012EC3C_c1(self) - 1) == 0) {
            v = self->tev_0x350 - 2;
            self->tev_0x350 = v;
            if (v < 0xAA) {
                self->tev_0x350 = 0xAA;
            }
            v = self->tev_0x352 - 2;
            self->tev_0x352 = v;
            if (v < 0xB4) {
                self->tev_0x352 = 0xB4;
            }
        } else {
            v = self->tev_0x350 + 2;
            self->tev_0x350 = v;
            if (v > 0xFF) {
                self->tev_0x350 = 0xFF;
            }
            v = self->tev_0x352 + 2;
            self->tev_0x352 = v;
            if (v > 0xFF) {
                self->tev_0x352 = 0xFF;
            }
        }
        v = self->tev_0x350;
        color.r = (u8)v;
        color.g = (u8)v;
        color.b = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(0, GX_KCOLOR3, &color);
        v = self->tev_0x352;
        color.r = (u8)v;
        color.g = (u8)v;
        color.b = (u8)v;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
        if (flag == 1) {
            v = self->tev_0x354 + 7;
            self->tev_0x354 = v;
            if (v > 0xFF) {
                self->tev_0x354 = 0xFF;
            }
            v = self->tev_0x356 + 6;
            self->tev_0x356 = v;
            if (v > 0xFF) {
                self->tev_0x356 = 0xFF;
            }
            v = self->tev_0x358 + 6;
            self->tev_0x358 = v;
            if (v > 0xFF) {
                self->tev_0x358 = 0xFF;
            }
        } else {
            v = self->tev_0x354 - 7;
            self->tev_0x354 = v;
            if (v < 0x28) {
                self->tev_0x354 = 0x28;
            }
            v = self->tev_0x356 - 6;
            self->tev_0x356 = v;
            if (v < 0x3C) {
                self->tev_0x356 = 0x3C;
            }
            v = self->tev_0x358 - 6;
            self->tev_0x358 = v;
            if (v < 0x3C) {
                self->tev_0x358 = 0x3C;
            }
        }
        color.r = (u8)self->tev_0x354;
        color.g = (u8)self->tev_0x356;
        color.b = (u8)self->tev_0x358;
        color.a = 0;
        ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
        if (em_flags836_ck(self, 1) == 1U) {
            if (self->tev_0x35E == 1) {
                color.r = 0xFF;
                color.g = 0xFF;
                color.b = 0xFF;
                color.a = 0;
            } else {
                color.r = 0x28;
                color.g = 0x3C;
                color.b = 0x3C;
                color.a = 0;
            }
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
        } else {
            ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR1, &color);
            self->tev_0x35E = flag;
        }
        ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
        if (fn_8012EC3C_c1(self) == 1U) {
            color.a = 0;
        } else if (em_alt_mode_ck_c1(self) == 1U) {
            color.a = (s32)(lbl_80797EB4 *
                            (lbl_807981BC * (lbl_80797E9C + fn_8005024C((u16)(system_w.field_0x0c * 0x2000))))) +
                      0xE1;
        } else {
            color.a = (s32)(lbl_80797EB4 *
                            (lbl_807981BC * (lbl_80797E9C + fn_8005024C((u16)(system_w.field_0x0c * 0x2000))))) +
                      0x87;
        }
        ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    }
}

/* 0x801913FC - per-(team,kind) "the effect may fire" predicate: 0 = no, 1 = the main window,
 * 2 = the secondary window. */
s32 fn_801913FC(_ENEMY_WORK* self, u8 arg1) {
    f32 d;
    f32 scale;

    switch (self->team) {
    case 16:
        switch (arg1) {
        case 0:
            d = self->vec_0x36C.y - self->pos.y;
            if (d >= lbl_80797EA0) {
                return 1;
            }
            if (d <= lbl_80797EE4) {
                return 2;
            }
            return 0;
        case 1:
            if (self->tev_0x35A <= 0) {
                return 1;
            }
            break;
        case 2:
            if (self->field_0x1E2 == 2) {
                scale = get_em_chg_scale(self);
                if (self->pos.y >= self->field_0x210 - (lbl_80797ED0 + fn_8013026C(self)) * scale) {
                    return 1;
                }
            }
            break;
        case 5:
            if (self->tev_0x35C > 0) {
                return 1;
            }
            break;
        }
        break;
    case 17:
        if (arg1 == 4 && self->field_0x011 != 0) {
            return 1;
        }
        break;
    case 21:
        switch (arg1) {
        case 3:
            if (fn_801321DC(self) == 1U) {
                return 1;
            }
            break;
        case 4:
            if (self->field_0x011 != 0) {
                return 1;
            }
            break;
        }
        break;
    }
    return 0;
}

/* 0x8018D8C8 - the per-(team,motion) frame-window driver.  For the work's team it reads the motion
 * number `em_get_mot_no(self)` and runs that motion's set of `em_after_frame_check` windows; each
 * window that fires calls `fn_8018D558` (the effect-spawn router) with the window's id/kind, or
 * `fn_8011D448`/`fn_8011D4FC`/`fn_8011D690`/`eft_em_spawn_joint`/`eft_em_spawn` directly.  The `field_0x228 & 6`
 * bit pair selects the "large/air" variant of each window; the team-0x10 tail runs `fn_80191EF8`
 * and `fn_80192080`. */
void fn_8018D8C8(_ENEMY_WORK* self) {
    VEC3 sp8;
    VEC3 sp14;
    VEC3 sp20;
    VEC3 sp2C;
    f32 temp_f1;
    u16 temp_r3;
    u16 temp_r3_2;
    u16 temp_r3_3;
    u8 temp_r0;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    VEC3_ctor(&sp14);
    temp_r0 = self->team;
    switch ((s32) temp_r0) {                        /* switch 1; irregular */
    case 16:                                        /* switch 1 */
        fn_8018D370(self);
        temp_r3 = em_get_mot_no(self);
        switch (temp_r3) {                          /* switch 2 */
        case 0x2:                                   /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_807980E0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            break;
        case 0x9:                                   /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079800C);
            }
            break;
        case 0x15:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797EEC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798188, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797FC8, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            break;
        case 0x16:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807980DC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x18:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F7C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x19:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F7C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_8079819C, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798178, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            break;
        case 0x1B:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F70, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            if ((em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if ((em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            break;
        case 0x2D:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981A4, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797E88);
                eft_em_spawn(self, 0x46, 0x15, &sp14, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_807981A8, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079804C);
            }
            break;
        case 0x33:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                em_hit_window_set_default(self, 0, 9);
            }
            if (em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797F38, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 8, 0xF, &sp14, 0x10, lbl_80797EC0);
                setVector3(&sp14, lbl_807981B0, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 8, 0x11, &sp14, 0x3C, lbl_80797EB8);
            }
            break;
        case 0x34:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                em_hit_window_set_default(self, 0, 0xA);
            }
            if (em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_807981B4, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 0xD, 0xF, &sp14, 0x10, lbl_80797EC0);
                setVector3(&sp14, lbl_807981B0, lbl_80797E88, lbl_80797E88);
                eft_em_spawn_joint(self, 0xD, 0x11, &sp14, 0x3C, lbl_80797EB8);
            }
            break;
        case 0x39:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807980DC, lbl_80797E88) == 1U) {
                fn_8011D448(self, 0, 0x26, 8, lbl_8079800C);
                sp14.x = lbl_80797E88;
                sp14.y = lbl_807981B8;
                sp14.z = lbl_80797F5C;
                shell_set_func_ptr->method_0x2C(self, 3, 1, &sp14, lbl_807981BC, 0xFFFF, shell_set_func_ptr);
            }
            break;
        case 0x3B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981C0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, self->field_0x1C0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0xFU, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x3D:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797E9C);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x3E:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 0x1DU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x1DU, self->field_0x1C0, lbl_8079804C);
                }
            }
            break;
        case 0x43:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x10, &sp14, 0x1E, lbl_8079800C);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x46:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x47:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x49:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8011D4FC(self, 2, 0x26, 0x15, 0x18, lbl_8079800C);
            }
            break;
        case 0x4D:                                  /* switch 2 */
            if ((s32) (self->flags_0x836 & 1) == 0) {
                if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                    fn_8011D690(self, 8, 0x28, lbl_80797E9C);
                }
                if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                    fn_8011D690(self, 9, 0x29, lbl_80797E9C);
                }
                if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                    setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_807981C8 * get_em_chg_scale(self));
                    fn_80117E58_c1(self, 1, &sp14, 3, lbl_8079800C * get_em_chg_scale(self));
                    setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797F58 * get_em_chg_scale(self));
                    rotVecY(&sp14, self->field_0x1C0);
                    addVec3(&sp8, &self->pos, &sp14);
                    copyVec3(&sp2C, copyVec3(&sp20, &sp8));
                    temp_f1 = lbl_807981CC * get_em_chg_scale(self);
                    sp2C.y -= temp_f1;
                    sp20.y += lbl_807981CC * get_em_chg_scale(self);
                    shell_set_func_ptr->method_0x30(self, 4, &sp2C, &sp20, lbl_807981BC, 0xFFFF, shell_set_func_ptr);
                }
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                em_camera_req(self, 0x28, 7);
            }
            break;
        case 0x4E:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0xF, &sp14, 0x10, lbl_8079800C);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x4F:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F08, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U)) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0xF, &sp14, 0x10, lbl_8079800C);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x50:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_8079818C);
            }
            if (em_after_frame_check(self, 0, lbl_807981D4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_8079818C);
            }
            break;
        case 0x51:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798198, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797EC4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0xE000, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0xE000, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981D8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797E9C);
            }
            break;
        case 0x52:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981DC, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            if ((em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981E0, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_8079817C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x55:                                  /* switch 2 */
        case 0x56:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981E4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981EC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x57:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981A8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x58:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F90, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xEU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0xEU, 0, lbl_8079800C);
                }
                em_camera_req(self, 0xE, 1);
            }
            break;
        case 0x59:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_807981AC, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80797F80, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F90, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 8U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 8U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xDU, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x5B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981F0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
            }
            break;
        case 0x5C:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798154, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x26U, 0, lbl_80797EF8);
                    fn_8018D558(self, 0U, 0xFU, 0x26U, 0, lbl_80797EF8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x26U, 0, lbl_80797EF8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0x11U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x11U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_80797FE0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797FCC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0x5E:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981F4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798034, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797EB8);
            }
            break;
        case 0x5F:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981F4, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80798034, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            if (em_after_frame_check(self, 0, lbl_807981C4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            break;
        case 0x60:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F98, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 6U, 0xDU, 0x2AAB, lbl_80797F0C);
            }
            break;
        case 0x61:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F98, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 6U, 8U, 0xD555, lbl_80797F0C);
            }
            break;
        case 0x62:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80797EC8, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x28U, 0, lbl_80798090);
                    fn_8018D558(self, 0U, 0xFU, 0x28U, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x28U, 0, lbl_80798090);
                }
            }
            break;
        case 0x63:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_807981E4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981EC, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x6F:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798130, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x12, &sp14, 0x3C, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_807981F8, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            if (em_after_frame_check(self, 0, lbl_807981FC, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797E88);
                eft_em_spawn(self, 0x46, 3, &sp14, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80798200, lbl_80797E88) == 1U) {
                setVector3(&sp14, lbl_80797E88, lbl_80797E88, lbl_80797ED4);
                eft_em_spawn_joint(self, 0x16, 0x11, &sp14, 0x3C, lbl_8079800C);
            }
            break;
        case 0x79:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0x7A:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FF4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 3U, self->field_0x1C0 + 0x4000, lbl_80797F0C);
            }
            break;
        case 0x7B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FA8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xFU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x7C:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797EB8);
            }
            if (em_after_frame_check(self, 0, lbl_80798204, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x7D:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798184, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 3U, self->field_0x1C0 + 0xC000, lbl_80797F0C);
            }
            break;
        case 0x7E:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80798134, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 3U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xFU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x7F:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797EB8);
            }
            if (em_after_frame_check(self, 0, lbl_80798204, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            break;
        case 0x81:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x11U, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x11U, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x82:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80798090);
            }
            break;
        case 0x83:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80798208, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x10U, 0xFU, 0, lbl_807981E8);
                } else {
                    fn_8018D558(self, 1U, 1U, 0xFU, self->field_0x1C0, lbl_80797E9C);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x84:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797F40, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80798180, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x1DU, self->field_0x1C0, lbl_80798090);
                }
            }
            break;
        case 0x88:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 2U, 3U, self->field_0x1C0, lbl_80797E9C);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0x89:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797FF4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797F0C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797F0C);
            }
            break;
        case 0x8B:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_8079820C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 1U, 0U, 0xFU, self->field_0x1C0, lbl_80797E9C);
            }
            break;
        case 0xC8:                                  /* switch 2 */
            if ((em_after_frame_check(self, 0, lbl_80798004, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_8079820C, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_807981E0, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x25U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x25U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x25U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80798188, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798210, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                em_camera_req(self, 8, 0);
            }
            if ((em_after_frame_check(self, 0, lbl_807981D4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798038, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x21U, 0, lbl_80797EB8);
                    fn_8018D558(self, 0U, 0xFU, 0x21U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x21U, 0, lbl_80797EB8);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80798214, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798218, lbl_80797E88) == 1U)) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                em_camera_req(self, 0xD, 0);
            }
            break;
        case 0xC9:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 4U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F70, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_807981E8);
            }
            if (em_after_frame_check(self, 0, lbl_807981A0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0xCA:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0xFU, self->field_0x1C0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 1U, 3U, 3U, self->field_0x1C0, lbl_80797EB8);
                }
                em_camera_req(self, -1, 7);
            }
            break;
        case 0xCB:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xEU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797FB0, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x21U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F68, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        case 0xCC:                                  /* switch 2 */
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_80798090);
                } else {
                    fn_8018D558(self, 1U, 6U, 0xEU, self->field_0x1C0, lbl_80798090);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F84, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797ED4, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 0x25U, 0, lbl_80797E9C);
            }
            if (em_after_frame_check(self, 0, lbl_80797F4C, lbl_80797E88) == 1U) {
                fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
            }
            break;
        }
        break;
    case 17:                                        /* switch 1 */
        temp_r3_2 = em_get_mot_no(self);
        switch ((s32) temp_r3_2) {                  /* switch 3; irregular */
        case 0x3B:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_807981C0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x11U, 0x10U, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 1U, 4U, 0x10U, self->field_0x1C0, lbl_80797EC0);
                }
            }
            break;
        case 0x3C:                                  /* switch 3 */
            if ((em_after_frame_check(self, 0, lbl_80798194, lbl_80797E88) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                fn_8018D558(self, 0U, 0x11U, 0x10U, 0, lbl_8079804C);
            }
            break;
        case 0x3E:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80798154, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x17U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x17U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x3F:                                  /* switch 3 */
            if ((em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) && ((s32) (self->field_0x228 & 6) != 0)) {
                fn_8018D558(self, 0U, 0x11U, 3U, 0, lbl_80797EB8);
                fn_8018D558(self, 0x27U, 0x30U, 3U, 0, lbl_80797EB8);
            }
            break;
        case 0x5C:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0x17U, 0x8000, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xFU, 0x17U, 0x8000, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x17U, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 2U, 2U, 0x10U, 0, lbl_8079800C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0x10U, 0x8000, lbl_80797E9C);
                }
            }
            break;
        case 0x81:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x10U, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x82:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x83:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, 0, lbl_80797F0C);
                }
            }
            break;
        case 0x85:                                  /* switch 3 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0xC9:                                  /* switch 3 */
            if ((em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80798190, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 0xCU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 0U, 0xCU, 0, lbl_8079804C);
                }
            }
            if ((em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) || (em_after_frame_check(self, 0, lbl_80797F5C, lbl_80797E88) == 1U)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xDU, 7U, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 0U, 7U, 0, lbl_8079804C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_8079819C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_8079804C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_80797EB8);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797EC4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_8079821C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_807981D0, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_80797F0C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797ECC, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0xEU, 0, lbl_807981BC);
                } else {
                    fn_8018D558(self, 0U, 6U, 0xEU, 0, lbl_807981E8);
                }
            }
            break;
        }
        break;
    case 21:                                        /* switch 1 */
        temp_r3_3 = em_get_mot_no(self);
        switch ((s32) temp_r3_3) {                  /* switch 4; irregular */
        case 0x70:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F40, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, lbl_80797EB8 * get_em_scale(self));
            }
            fn_80136D14(self);
            break;
        case 0x73:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797EF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_8079818C);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, self->field_0x1C0, lbl_80798220);
                }
            }
            break;
        case 0x81:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797EA4, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 1U, 6U, 0x10U, self->field_0x1C0, lbl_80797EB8);
                }
            }
            break;
        case 0x82:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F00, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797E9C);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            break;
        case 0x83:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F64, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 0x10U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 6U, 0x10U, 0, lbl_80797F0C);
                }
            }
            break;
        case 0x85:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 0xDU, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 0xDU, 0, lbl_80797E9C);
                }
            }
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0xCU, 8U, 0, lbl_80797EB8);
                } else {
                    fn_8018D558(self, 0U, 2U, 8U, 0, lbl_80797E9C);
                }
            }
            break;
        case 0xCD:                                  /* switch 4 */
        case 0xC9:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80798224, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, lbl_807981BC * get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, lbl_80797EB4, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x68U, 0, lbl_807981BC * get_em_scale(self));
            }
            if ((em_after_frame_check(self, 3, lbl_80797F08, lbl_80798194) == 1U) && ((s32) (system_w.field_0x0c & 7) == 0)) {
                eft009_spawn_at_joint(self, 0x13U, 0x69U, 0, lbl_807981BC * get_em_scale(self));
            }
            break;
        case 0xD0:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797FF8, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x66U, 0, get_em_scale(self));
            }
            if (em_after_frame_check(self, 0, lbl_80797F44, lbl_80797E88) == 1U) {
                eft009_spawn_at_joint(self, 0x13U, 0x67U, 0, lbl_80797EB8 * get_em_scale(self));
            }
            break;
        case 0xD2:                                  /* switch 4 */
            if (em_after_frame_check(self, 0, lbl_80797F6C, lbl_80797E88) == 1U) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    fn_8018D558(self, 0U, 0x14U, 3U, 0, lbl_8079800C);
                } else {
                    fn_8018D558(self, 1U, 4U, 3U, self->field_0x1C0, lbl_80798228);
                }
            }
            break;
        }
        break;
    }
    if ((u8) self->team == 0x10) {
        fn_80191EF8_view1(self);
        fn_80192080_view1(self);
    }
}

#pragma peephole off

extern "C" {
/* ------------------------------------------------------------------------------------------------ */
/* bodies, in address order                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* Picks the caller's class/state pair from the work's team, map key and action id. */
void fn_80191598(EmActWork* self, u8* out_class, u8* out_state) {
    switch (self->team) {
    case 16:
        em_move_mode_set_view1(self, 0);
        *out_class = 12;
        *out_state = 2;
        switch (stage_map_kind_get_view3(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 6:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 8;
                fn_80126324_view1(self, 14, 15, lbl_80797E88);
                break;
            case 7:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 13, 14, lbl_80797E88);
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 2:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 6, 4, lbl_80797E88);
                break;
            case 3:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 15, 16, lbl_80797E88);
                break;
            }
            break;
        case 9:
        case 11:
            if (self->act_id == 1) {
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 9;
                fn_80126324_view1(self, 0, 1, lbl_80797E88);
            }
            break;
        }
        break;
    case 17:
        fn_80182D5C_view1();
        switch (stage_map_kind_get_view3(self->field_0x1E0)) {
        case 1:
            switch (self->act_id) {
            case 5:
            case 6:
            case 8:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
            case 12:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    em_move_mode_set_view1(self, 2);
                    *out_state = 3;
                } else {
                    em_move_mode_set_view1(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        case 3:
            switch (self->act_id) {
            case 1:
            case 5:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 5;
                break;
            case 7:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 6;
                break;
            case 2:
            case 3:
            case 4:
            case 6:
            case 8:
                em_move_mode_set_view1(self, 2);
                *out_class = 12;
                *out_state = 0;
                break;
            case 10:
                em_move_mode_set_view1(self, 0);
                *out_class = 12;
                *out_state = 7;
                break;
            default:
                *out_class = 12;
                if (self->field_0x00A == 1) {
                    em_move_mode_set_view1(self, 2);
                    *out_state = 3;
                } else {
                    em_move_mode_set_view1(self, 0);
                    *out_state = 2;
                }
                break;
            }
            break;
        default:
            *out_class = 12;
            if (self->field_0x00A == 1) {
                em_move_mode_set_view1(self, 2);
                *out_state = 3;
            } else {
                em_move_mode_set_view1(self, 0);
                *out_state = 2;
            }
            break;
        }
        break;
    case 21:
        em_move_mode_set_view1(self, 4);
        *out_class = 12;
        *out_state = 1;
        break;
    }
}

/* Re-arms the action the work's map key and action id select. */
void fn_80191990(EmActWork* self) {
    u32 state = 0;

    switch (stage_map_kind_get_view3(self->field_0x1E0)) {
    case 3:
        switch (self->act_id) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            switch (self->state_0x9F6) {
            case 2:
                state = 1;
                break;
            }
            break;
        }
        break;
    }
    if (state == 1) {
        em_move_mode_set_view1(self, 0);
        em_mot_set_view1(self, 1, 0, 0);
    } else if (state == 2) {
        em_move_mode_set_view1(self, 2);
        em_mot_set_view1(self, 0x28, 0, 0);
    }
}

/* Reports whether the work is in one of the two team-specific ready states. */
u32 fn_80191A6C(EmActWork* self) {
    if (self->team == 21) {
        if (self->state_0x1E2 == 4 && fn_801321DC_view1(self) == 1) {
            return 1;
        }
    } else if (self->state_0x1E2 == 2 && em_alt_mode_ck_view1(self) == 0) {
        return 1;
    }
    return 0;
}

/* Reports whether the work's state byte is clear. */
u32 fn_80191AD8(EmActWork* self) {
    return self->state_0x1E2 == 0;
}

/* Re-seats the work at its own position while the damage part admits the action. */
void fn_80191AE8(EmActWork* self, u32 kind) {
    switch ((u8)kind) {
    case 1:
        if (em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1) == 2 && self->state_0x1E2 != 1) {
            fn_803B9BA0(self, &self->pos_0x188, 100);
        }
        break;
    }
}

/* Reports whether the work may run its current action: the team/state gate, the entry probe and the
 * two restart checks. */
s32 fn_80191B4C(EmActWork* self, u32 arg) {
    u8 kind = stage_map_kind_get_view3(self->field_0x1E0);
    s32 hit;
    u32 probe;

    if (self->team != 16) {
        return 0;
    }
    switch (kind) {
    case 1:
    case 3:
        break;
    default:
        return 0;
    }
    if (fn_80129D3C_c1(self) == 1) {
        return 1;
    }
    hit = 0;
    switch (kind) {
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
        switch (fn_80129DB8_view1(self)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0) {
        switch (kind) {
        case 1:
            probe = 12;
            break;
        case 3:
            probe = 4;
            break;
        default:
            probe = 0xFF;
            break;
        }
        if (fn_8012A014_view1(self, 0, probe, (u16)arg, 0, lbl_805AAA74) == 1) {
            return 1;
        }
    }
    if (fn_80129A70_view1(self, (u16)arg) == 1) {
        return 1;
    }
    return (fn_8012A204_view1(self) - 1) == 0;
}

/* Re-enters the user-data accessor's third flag state. */
void fn_80191CDC(EmUserData* self) {
    fn_8013A654_view1(self, 3);
}

/* Applies one user-data item's aim transform to the caller's matrix holder. */
void fn_80191CE4(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item) {
    MTX34 mtx;
    EmMtx33 dst;
    EmRotVec rot;
    EmActWork* work;

    work = (EmActWork*)self->work_0x04;
    MTX34_ctor(&mtx);
    fn_800516F0(&dst);
    switch (item->type_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
        case 18:
        case 20:
            fn_8008E8D0(holder, &work->aim_0x328.vec_0x1C);
            break;
        case 24:
            rot.x = work->aim_0x328.angle_0x00;
            rot.y = 0;
            rot.z = 0;
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        case 25:
            eft_rot_vec_copy(&rot, &work->aim_0x328.rot_0x04);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        case 26:
            eft_rot_vec_copy(&rot, &work->aim_0x328.rot_0x10);
            cpSetRotMatrix((struct _CP_VECTOR*)&rot, &mtx);
            fn_805012E8(&dst, &mtx);
            fn_8008EE68(holder, &dst);
            break;
        }
        break;
    }
}

/* Places the aimed cluster the item's index selects. */
void fn_80191E30(EmUserData* self, EmMtxHolder* holder, u32 a2, u32 a3, u32 kind, EmUserItem* item) {
    MTX34 mtx;
    u32 index;
    EmActWork* work;

    work = (EmActWork*)self->work_0x04;
    MTX34_ctor(&mtx);
    switch (item->type_0x04) {
    case 0xFF:
        switch (kind) {
        case 16:
            index = 0;
            break;
        case 18:
            index = 1;
            break;
        case 20:
            index = 2;
            break;
        default:
            return;
        }
        if (work->clusters_0x590[index].live_0x03 >= 1) {
            joint_mtx_load(holder, &mtx);
            fn_800FBB90(&mtx, &work->clusters_0x590[index].pos_0x24);
            joint_mtx_store(holder, &mtx);
        }
        break;
    }
}

/* Steps the five aim angles by their per-frame increments and clamps them. */
void fn_80191EF8(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    u32 v;

    if (em_alt_mode_ck_view1(self) == 1) {
        v = rec->angle_0x00 + 546;
        rec->angle_0x00 = v;
        if ((s16)v > 0) {
            rec->angle_0x00 = 0;
        }
        v = rec->rot_0x04.x + 455;
        rec->rot_0x04.x = v;
        if ((s16)v > 0) {
            rec->rot_0x04.x = 0;
        }
        v = rec->rot_0x04.y + 455;
        rec->rot_0x04.y = v;
        if ((s16)v > 0) {
            rec->rot_0x04.y = 0;
        }
        v = rec->rot_0x10.x + 455;
        rec->rot_0x10.x = v;
        if ((s16)v > 0) {
            rec->rot_0x10.x = 0;
        }
        v = rec->rot_0x10.y - 455;
        rec->rot_0x10.y = v;
        if ((s16)v < 0) {
            rec->rot_0x10.y = 0;
        }
    } else {
        v = rec->angle_0x00 - 546;
        rec->angle_0x00 = v;
        if ((s16)v < -5460) {
            rec->angle_0x00 = (u16)-5460;
        }
        v = rec->rot_0x04.x - 455;
        rec->rot_0x04.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.x = (u16)-4550;
        }
        v = rec->rot_0x04.y - 455;
        rec->rot_0x04.y = v;
        if ((s16)v < -4550) {
            rec->rot_0x04.y = (u16)-4550;
        }
        v = rec->rot_0x10.x - 455;
        rec->rot_0x10.x = v;
        if ((s16)v < -4550) {
            rec->rot_0x10.x = (u16)-4550;
        }
        v = rec->rot_0x10.y + 455;
        rec->rot_0x10.y = v;
        if ((s16)v > 4551) {
            rec->rot_0x10.y = 4551;
        }
    }
}

/* Runs the aim vector's fade in or out and mirrors it into its follower. */
void fn_80192080(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    f32 v;

    if (fn_8012EC3C_view1(self) == 0) {
        v = rec->vec_0x1C.x - lbl_8079822C;
        rec->vec_0x1C.x = v;
        if (v > lbl_80797E9C) {
            rec->vec_0x1C.x = lbl_80797E9C;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    } else {
        v = rec->vec_0x1C.x + lbl_8079822C;
        rec->vec_0x1C.x = v;
        if (v < lbl_8079800C) {
            rec->vec_0x1C.x = lbl_8079800C;
        }
        rec->vec_0x1C.y = rec->vec_0x1C.x;
    }
}

/* Fills in one effect request (id 0x17) from the default vector. */
void fn_80192108(EmEffRequest* out, u8 a, s16 b, s16 c) {
    if (lbl_80794AA0 == 0) {
        setVec3(&vec_default_80191598, lbl_80797E88, lbl_80797E88, lbl_80797EB4);
        lbl_80794AA0 = 1;
    }
    out->id_0x00 = 0x17;
    copyVec3(&out->pos_0x04, &vec_default_80191598);
    out->field_0x10 = a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* Releases a heap block when the flag asks for it, then hands the pointer back. */
void* fn_801921A8(void* p, u32 flag) {
    if (p != 0) {
        fn_8013918C_view1(p, 0);
        if ((s16)flag > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* Builds the five static vectors the effect requests start from. */
void fn_80192204(void) {
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;
    VEC3 v4;
    VEC3 v5;
    VEC3 v6;
    VEC3 v7;
    VEC3 v8;

    assignVec3_view1(&vec_pair_80191598_0.vec_0x00,
                setVec3(&v1, lbl_80797E88, lbl_807981C0, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_0.vec_0x0C,
                setVec3(&v2, lbl_80797E88, lbl_80797E88, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_1.vec_0x00,
                setVec3(&v3, lbl_80797E88, lbl_80798230, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_1.vec_0x0C,
                setVec3(&v4, lbl_80797E88, lbl_80797E88, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_2.vec_0x00,
                setVec3(&v5, lbl_80797E88, lbl_80797F80, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_2.vec_0x0C,
                setVec3(&v6, lbl_80797E88, lbl_80798234, lbl_80797E88));
    assignVec3_view1(&vec_pair_80191598_3.vec_0x00,
                setVec3(&v7, lbl_80797E88, lbl_80797E88, lbl_80797EB4));
    assignVec3_view1(&vec_pair_80191598_3.vec_0x0C,
                setVec3(&v8, lbl_80797E88, lbl_80797F24, lbl_80797E88));
}
}

/* This unit's own `.bss` (`splits.txt` `.bss 0x806A7A00..0x806A7A70`), in address order: the four vector pairs
 * and the default vector its static constructor `fn_80192204` builds (`fn_80192108` copies the default).  Names
 * are GUESSes. */
EmVecPair vec_pair_80191598_0;  /* +0x806A7A00 */
EmVecPair vec_pair_80191598_1;  /* +0x806A7A18 */
EmVecPair vec_pair_80191598_2;  /* +0x806A7A30 */
EmVecPair vec_pair_80191598_3;  /* +0x806A7A48 */
VEC3 vec_default_80191598;      /* +0x806A7A60 */
