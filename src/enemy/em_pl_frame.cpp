/*
 * enemy/em_pl_frame.cpp - the second half of proposal/8032C920_fn_8032C920.cpp's range, `.text`
 * 0x8033041C..0x80334568 (48 functions, 16716 B).  Registered once, at its final home (docs/plan.md
 * 12): that proposal's range 0x8032C920..0x80334568 was one `--max-bytes` cut over **two** translation
 * units, and the seam re-draw of 2026-09-26 split it into `enemy/em_act_step.cpp` (first half, 74
 * functions / 15100 B) and this file.  No function is reconstructed yet - the file exists as the
 * registration the register-once rule requires, and its header says what the unit is, its range, why
 * it sits there, what is unknown and where the evidence lives.
 *
 * THE SEAM, MEASURED - three independent instruments agree on 0x8033041C:
 *
 *  - the extabindex run 0x800357A8..0x800359A0: an extabindex entry names its own function, and entry
 *    57 (at 0x800357A8) names `fn_8033041C`.  The run's 42 entries cover this unit's 42 framed
 *    functions; the first half takes the other 57 of the 99.
 *  - the `.sdata2` pool run 0x8079B108..0x8079B2AC is **two objects' pools**, cut at 0x8079B20C |
 *    0x8079B210: the first half's labels are referenced only by functions 0x8032C920..0x80330128, this
 *    unit's only by functions 0x803305F8..0x80334398, and no label is shared.  The duplicated
 *    constants are the proof: MWCC's u32->f32 magic `0x4330000080000000` is emitted at 0x8079B140
 *    (first half - `em_act_arm_mot5`, `em_act_arm_mot7`) and at 0x8079B228 (this unit - `fn_803305F8`), and
 *    0.0f (0x8079B108 / 0x8079B240), 0.5f (0x8079B1E4 / 0x8079B214), 1.0f (0x8079B130 / 0x8079B284),
 *    10.0f (0x8079B110 / 0x8079B238), 20.0f (0x8079B160 / 0x8079B21C), 30.0f (0x8079B12C /
 *    0x8079B218 + 0x8079B258), 60.0f (0x8079B168 / 0x8079B220), 0.8f (0x8079B178 / 0x8079B2A0) and
 *    -30.0f (0x8079B17C / 0x8079B294) each occur once per half.  A pool emits one copy of a constant
 *    per object - the first half's own object uses the magic in two functions and emits a single
 *    8-byte entry - and the linker merges nothing (that magic occurs >= 40x in the DOL's `.sdata2`),
 *    so two copies mean two emitters, i.e. two TUs.
 *  - the `.data` run: the first half's table fragment ends with the class vtable 0x805E04E0 (0x30 B,
 *    installed by `em_work_ctor`) and this unit's begins at 0x805E0510 with six objects that
 *    `fn_803305F8` alone references; the referrer addresses jump 0x8032FA88 -> 0x803305F8 exactly at
 *    that fragment edge.
 *
 * SECTIONS.  `.text` 0x8033041C..0x80334568; extab 0x8001662C..0x8001677C (42 x 8 B); extabindex
 * 0x800357A8..0x800359A0 (42 x 12 B).  Its `.sdata2` pool half is 0x8079B210..0x8079B2AC - 0x8079B210
 * (0.5), 0x8079B214 (0.5), 0x8079B218 (30.0), 0x8079B21C (20.0), 0x8079B220 (60.0), 0x8079B228 (the
 * u32->f32 magic, 8 B), 0x8079B230 (8 B, `0x03AA2425`), 0x8079B238 (10.0), 0x8079B23C (0.7),
 * 0x8079B240 (0.0), 0x8079B244 (-1.0), 0x8079B248.., 0x8079B258 (30.0), 0x8079B294 (-30.0),
 * 0x8079B2A0 (0.8) - and it stays **unclaimed**: a `.sdata2` claim links only while the object emits
 * no pool of its own (playbook 23/58), so the labels are declared and used as load operands only
 * (playbook 29).  Its `.data` run 0x805E0510..0x805E201C is deliberately **not claimed** either: the
 * next band's `fn_803346B4` owns an object at 0x805E1ED0 inside that address range, so the run is not
 * claimable whole, and a run with an unclaimed hole in it becomes an `auto_*_data` unit in the middle
 * of the unit's range (playbook 53).  The measured object list, for the data pass that claims it:
 * 0x805E0510 / 0x805E0540 / 0x805E0570 (0x30 each), 0x805E05A0 (0x38), 0x805E05D8 (0xC),
 * jumptable_805E05E4 (0x50) - all `fn_803305F8`; 0x805E0638 / 0x805E0688 / 0x805E06D8 (0x50 each, no
 * referrer); 0x805E0728 (0xC) + 0x805E0734 (0x32C) - `fn_80331238`; then one object per function from
 * 0x805E0A60 (`fn_80331868`) to 0x805E1E20 (`fn_80333FB8`), plus 0x805E1F6C (`fn_803341B0`) and
 * 0x805E1F98 (`fn_80334398`).
 *
 * MODULE AND NAME - the pass that writes the bodies decides, and this is the hint it starts from.  No
 * `__FILE__` string is reachable and `tools/symbols/dumpmap.py lookup` answers `zz_XXXXXXXX_` for every
 * address of the range, so the map has no name for the file either.  Module `enemy` is the link
 * band's: 0x803250B0..0x80334568 is the enemy band (`enemy/em_action.cpp` and
 * `enemy/em_act_step.cpp` bracket it) and the neighbouring files are `enemy/`.  The honest caveat, of
 * the same kind `hud/fn_80334568.cpp` records for its band: the content is the *player* work.  This
 * half drives the player work record (`_PLW` reached with `get_move_work_adrs(2)`, then
 * `Pl_frame_check` / `Get_motion_no` / `Pl_chr_setX` / `Pl_master_ck`, and the documented `+4` ->
 * `get_joint_wpos` idiom) and it owns a second class's table family (vtable 0x805E0510 plus
 * `fn_803305F8`'s six objects at 0x805E0510..0x805E0634).
 *
 * The name `em_pl_frame` is **provisional** - the merger lane's 2026-09-26 naming pass had to leave a
 * `fn_XXXXXXXX` file stem behind (the gate's rule 7 row exempts a bodyless unit from the rule itself,
 * but never a generated *path*), and the only evidence a name can come from before the bodies exist is
 * the target object's own call surface, read with `tools/units/m2cinput.py`: of its 48 functions, 31
 * call `Pl_frame_check__FP4_PLWUlff` (15 `Pl_master_ck__FP4_PLW`), and the range's heaviest callees are
 * all in the `Pl`/`player` band - `fn_8026A33C` x44, `fn_803313D0` x40, `fn_80275B04` x33,
 * `fn_802761B8` x24, `fn_80277C50` x24, `fn_80275AC4` x16, `fn_802770E8` x10 - plus the effect spawns
 * `res_eft_create__FUsUsUl`, `res_eft_model_create__FP6MHcharUsUl` and `eft007_set__FP4_PLWUcUcUlPQ34nw4r4math4VEC3f`.
 * So the half runs the *player records'* frame checks and spawns the hit effects; the pass that writes
 * the bodies renames this file from them, and if a `Pl` home turns out to be right, this unit moves.
 * Its own 48 symbols keep the map's `fn_` stems until then (writing a body is what names it).
 *
 * FLAGS.  `cflags_main` (Wii/1.3, `-O3 -inline noauto -Cpp_exceptions on`), the first half's set: 42 of
 * its 48 functions are framed and carry an extab/extabindex record, which is what
 * `-Cpp_exceptions on` produces, and the band's callees are the same mangled C++ symbols.
 *
 * THE RECORD.  Nothing is written yet.  The first half's file keeps the `_EM_CHARA_WORK` view and
 * `include/unsplit/enemy.h` carries the first half's pool declarations only; this unit's pool labels
 * (listed under SECTIONS) are NOT declared there and have to be declared, as externs, in this unit's
 * own header when its bodies arrive.
 *
 * NEXT.  Write the 48 bodies in address order from `fn_8033041C` (0x1DC B, the per-attacker effect spawn
 * that continues `em_act_arm_mot201_hit7_2`'s scan shape): `python tools/units/m2cinput.py` for a draft,
 * then `python tools/units/recompile.py enemy/em_pl_frame.cpp --measure <symbol> --main .`, one function
 * per pass, best-scoring variant, never a regression.
 */
