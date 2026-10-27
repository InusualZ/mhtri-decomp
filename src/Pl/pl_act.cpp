/*
 * Pl/pl_act.cpp - the player actor: the part/motion cluster, the master, skill and item layers, the act-entry front
 *   end, the action state machines and the equipment helpers.
 * RANGE. .text 0x802693C4-0x802840DC (459 functions); .rodata 0x805706C0-0x80570700, .data 0x805C5F30-0x805C80A8,
 *   .bss 0x806AB810-0x806AB830, .sdata 0x80792140-0x80792188, .sbss 0x80794B30-0x80794B38, .sdata2
 *   0x8079A000-0x8079A1D8, extab, extabindex.  Seven sections in address order, each banner-marked: part/motion
 *   0x802693C4, master 0x8026BA1C, health gate 0x8026FFBC, skill 0x80270018, act entry 0x80273B14, action 0x80276B58,
 *   equipment 0x8027D684.  Each section keeps its own callee declarations in a namespace (some shared callees are
 *   declared with different signatures); `extern "C"` names stay unmangled, and the C++-linkage callees and the types
 *   the manglings name stay at global scope.  Uniting the declarations is open work.
 *   The left seam is unproven by extabindex: 0x8002F6DC -> 0x8026910C is `Pl/player_control.cpp`'s last record and
 *   0x8002F6E8 -> 0x802695A4 this unit's first; `pl_obj_ready_ck` and `fn_802693C4`..`pl_parts_value_get` are frameless, so
 *   extab cannot place the 0x802693C4 seam.
 *   The right part is several TUs (seam re-draw candidate): `.sdata2` holds 0.0f twice back to back (0x8079A11C, read
 *   by the equipment section; 0x8079A120, read only by 0x80280050-0x80282BDC) and 52.0f at 0x8079A19C/0x8079A1B0, the
 *   second copy read from 0x80282CCC and from `Pl/fn_802840DC.cpp`'s 0x802850E0-0x8028663C - so the weapon-0 band
 *   0x80280050-0x80282BDC is its own TU and the weapon-1 band starts at 0x80282BDC and runs past 0x802840DC; its `.data`
 *   0x805C80A8-0x805C9568 is unclaimed (`unsplit/Pl.h`).
 * FLAGS. `cflags_pl` (docs/pl.md).  The skill section is under `#pragma optimization_level 4` (docs/pl.md: level 3
 *   rematerialises `pl_parts_refresh`'s base); the file is under `#pragma peephole off` from the act-entry section on.  The
 *   master section's `#pragma peephole off` runs from `fn_8026BE94` through `fn_8026CBE4` (including `fn_8026BF98`),
 *   and `off`/`reset` pairs scope `pl_input_active_ck`, `pl_stick_window_gated_ck` and `Pl_act_ck`..`pl_cond_flags_clear`: recorded as
 *   load-bearing in six places (they turn the fused record form back into retail's `clrlwi`/`rlwinm`/`and` + `cmpwi`
 *   pair and keep the byte/short truncations), measured with a flag set without `-opt nopeephole` and not re-measured
 *   under the lib flag.  The equipment section runs under `#pragma optimization_level 4` (its switches are retail's
 *   `subi`/`cmplwi` range chains, playbook 108), and so do `pl_weapon_affinity_get`..`pl_weapon_class_refresh`,
 *   `fn_80273ED8` and `pl_weapon_guard_ck`/`fn_80274AEC` in the act-entry section; the weapon bands stay at level 3.
 * NAMES. GUESSes read from each body: `Pl_chr_set_attr_default` 0x8026A224 (`Pl_chr_set_attr` with its last argument
 *   0), `Pl_motion_end_ck` 0x8026A33C (the model layer's "motion finished" predicate), `Pl_act_set_motion` 0x80275B04
 *   (arms the act's three status bits from a packed mask), `Pl_act_set_motion_slot` 0x802761B8 (the same hand-off with
 *   two trailing zeros), `Pl_act_set_step_table` 0x802770E8 (installs the per-act `.data` record at +0x318),
 *   `Pl_item_id_usable_ck` 0x802752C8 and its parameter `mode` (dead in retail; the call sites pass 0/1/2 against the
 *   body's 1/2/0x10 masks).
 *   GUESS (from each body and the NPC swap callers): pl_carry_item_get, pl_item_room_get
 *   GUESS (from each body and its callers): pl_act_name_row_get
 *   GUESS (from each body): pl_id_bit_ck, pl_part_flag2_ck (the +0xDC word and the second 128-id set), pl_act_add_stagger,
 *   pl_act_true_ck, pl_act_no_68_71_ck, pl_chr_set_motion_default, pl_model_set_value, pl_rig_get_motion_frame,
 *   pl_act_gauge_step_unlatched, pl_act_gauge_step_signed, pl_act_held_ck, pl_ground_probe_ahead,
 *   pl_ground_band_ck, pl_stg_weapon_near_find
 *   GUESS (field, from its menu reader `weapon_slot_count_get`): `PlSkillRec::slot_count` (`Pl/fn_8027D684.h`)
 *   GUESS (from each body and its menu/lobby callers): equip_color_rgba_get and equip_color_index_get (the
 *   equipment colour table's RGBA row and the kind row's +0x05 index), item_kind_name_get, skill_name_get (the
 *   system string table's +0x94/+0x90 lists), item_species_class_get, armor_color_rgba_get and armor_color_table (the 3-byte
 *   palette the armour colour index picks), equip_color_table, equip_custom_color_rgba_get (the piece's own RGB565
 *   word at +0x04), equip_level_max_ck, pl_act_set_motion_gate (+0x45A), pl_act_add_class7_gauge (+0x384, class 7
 *   only), pl_act_timer_step (+0x445)
 *   GUESS (fields, from their readers): `PlWeaponRec::bonus_0x06` (`fn_8027E708`), `PlWeaponRec::bonus_flag_0x15`
 *   (`equip_gun_bonus_ck`), `PlSlotRec::dye_mode` (`EnableChangeColor` tests 2), `PlSlotRec::color_index`
 *   (`equip_color_index_get`); `PlRgba` (the stack word the colour getters assemble)
 *   GUESS (from each body and its callers): `pl_status_bit_ck`, `pl_stagger_level_get`,
 *   `pl_impact_pos_get`, `pl_charge_stage_get`, `pl_act_frame_get`.
 *   GUESS (from each body, its dispatcher slot and its tables): the weapon-0 band `pl_weapon0_*` (attack_set,
 *   motion_row_apply, hit_feedback, act_rows_a..e, act_charge, act_lunge, act_kick/kick2, charge_se, charge_frames_get,
 *   charge_step, act_charge_hold/hold2..4, charge_attack_set, act_charge_release/release2..4, act_slam/slam2,
 *   act_tackle) with its tables `pl_weapon0_rows_*`/`steps_*`/`charge_*`/`hit_*` and pool `pl_weapon0_float_*`/`frame_*`;
 *   the weapon-1 band `pl_weapon1_*` (attack_set, hit_latch, act_combo, shell_swing, double, turn_swing, swing2, chain,
 *   single, combo3, rise, double3, single3, finisher3, chain3, kick3, kick, turn3) with `pl_weapon1_hit_table`/`hit_ids`,
 *   pool `pl_weapon1_*` and the registry names of its step tables `pl_weapon1_steps_*`
 *   GUESS (from each body): pl_atk_act_flag_set, pl_id_bit_set/pl_id_bit_clear, pl_input_lock_set, the pad-input
 *   helpers `pl_input_*` and `PlInput` (the +0xB8 block), pl_weapon_parts_refresh, pl_alive_act_ck, equip_gun_level_bonus_get,
 *   equip_item_id_clamp, pl_armor_model_or_default_get, equip_bowgun_frame_kind_get, equip_deco_pack, talisman_skill_pick,
 *   talisman_roll, equip_drop_roll, `pl_armor_defense_steps`, `pl_armor_default_models_a/b/c`, `talisman_*`,
 *   `equip_drop_roll_tables`, `pl_input_button_masks`, `pl_act_float_zero/one/10/neg1`; the record fields split out of
 *   `PlSkillRec`/`PlWeaponRec`/`PlSlotRec` and `PlArmorLevelRec` (`Pl/fn_8027D684.h`); registry: pl_se_charge_level_req,
 *   pl_gun_clip_table, lb_bowgun_frame_kinds.
 *   GUESS (from each body and its callers): `pl_input_refresh` 0x8026AF08 (refreshes the +0xB8 input block from the pad
 *   record; `fn_8026F7B4`/`pl_input_clear` call it), `pl_note_near_ck` 0x8027D8A0 (the 300-unit reach to a note-pane
 *   record), the pool rows `pl_input_acc_swing_min`/`pl_input_fs_acc_swing_min` (200.0/300.0, the swing thresholds
 *   `vec3_len` is compared against), `pl_note_near_dist_sq` (90000.0) and `pl_stg_weapon_foot_ofs` (110.0); the
 *   `StageWeaponWork` fields (`stage/get_stg_weapon_work.h`), `SysQuestMove::hold_off_0x001`/`player_hold_0x004`,
 *   `_PLW::tail_slot_0x300` and `_PLW::bind_reset_0x00B`.
 *   The rig's implicit members are the compiler's: `__ct__7PlSeRigFv`, `__dt__7PlSeRigFv` and the callback classes'
 *   `__ct__21LbEquipModelCallbackA/BFv` (`Pl/pl_se_rig.h` holds the classes by value; `PlSeRig` is the GUESS name).
 * RESIDUALS. Every function is written.
 *  - flipcheck: the object emits no `.rodata` (0x40 claimed), `.sbss` (0x8) or `.sdata` (0x48); `.text` 0x1AD38,
 *    `.data` 0x27C, `.sdata2` 0x10, extab 0x930 and extabindex 0xD5C against the claims 0x1AD18, 0x2178, 0x1D8, 0x928 and
 *    0xD50; every compared section differs in bytes, and five symbols other units reference are not defined here
 *    (`lbl_805C7A64`, `lbl_805C7C34`, `pl_steps_guard_kind0`, `pl_steps_guard_walk_kind0`, `set_demo_equip_data_func`).
 *  - `pl_net_slot_bind`: the seven `set_valid |= bit` stores keep a `clrlwi` after every second `ori` where retail
 *    truncates once after the fourth (the expression, loop and two-statement forms measured; +8 B);
 *  - `pl_input_refresh`: the Classic Controller arm's held/alt/pressed locals take r3/r6 where retail has r6/r3 (block
 *    locals and every declaration order measured);
 *  - `pl_stg_weapon_near_find`: the second loop's index and row pointer sit in r28/r27 where retail has r27/r28;
 *  - `fn_802753E4`: the lobby parameter block base and the id swap r3/r4, and retail keeps an `extsh` after the first
 *    `lha` (`value +=`, `value = (s16)(value + x)` and a plain store measured);
 *  - `fn_8026A068`: retail divides the id before it loads the work's byte (`kind` local, no local and a pre-computed
 *    quotient all measured);
 *  - `talisman_roll`: retail tests each skill's points with `bne`/`b` around the return and re-extends the points before
 *    the `+ 10` (the nested, combined and `switch` forms measured);
 *  - `equip_deco_slots_free`: the free-slot return forms `andc` where ours has `and` (three return spellings measured);
 *  - `equip_gun_level_bonus_get`: the remainder loop bumps the index before the row pointer;
 *  - 80 functions partial (`sweepcomments.py --unit Pl/pl_act` lists them), including:
 *  - `equip_color_rgba_get`/`armor_color_rgba_get`: retail forms the row address `add r3,r4,r3` (offset first), ours
 *    `add r3,r3,r4`; a row-pointer local is worse;
 *  - `pl_area_stamina_drain_step`: retail's `self->unk37C -= v` subtracts the unextended `v` (r31) after the `extsh r0` compare;
 *    `-= (s16)v` reuses the extended copy (r3), and `-= v` moves the `extsh` ahead of the `sth`;
 *  - `pl_pos_blend_start`: retail stores the raw `arg1` (r31) to +0xB4 where ours reuses the `(s16)arg1` the
 *    condition extended (r0); the header's `s32` parameter fixes the spelling, and a `(u16)` store is worse;
 *  - `pl_sharpness_add`: a `clrlwi` before the `stb` of `m` into +0x56B (peephole is off; a `u8` local is worse);
 *  - `fn_80278D1C`: retail branches every id range test forward to its block (8, then 2, then 4) with a fall-through
 *    `b`; the nested if, the else-if chain and a `switch` all lay the blocks out differently;
 *  - `fn_802693C4`/`pl_parts_value_get`: retail adds the record offset into the base register (`add r4,r4,r0` /
 *    `add r5,r5,r0`), ours into a fresh one; `fn_8027AF88`: one `extsb` target register (r4 against r0);
 *  - `pl_status_sheet_get`: the implicit int->float conversion constant rows (compiler-synthesised, playbook 29);
 *  - `equip_skill_points_get`/`equip_skill_tree_has_ck`: MWCC's `switch` decision tree against retail's three linear `subi`/`cmplwi` range
 *    tests, and in `equip_skill_tree_has_ck` the `deco_count` bound retail re-masks from r0; the label-chain shape that rule 8
 *    removed reproduced more rows (docs/pl.md);
 *  - `Pl_cat_skill_ck`: MWCC reassociates `(plw + 4) + 0x612` into `plw + 0x616` for every walk tried;
 *  - `pl_parts_refresh`: the three call-spanning values are coloured base/`pend`/mask in r24/r25/r26 by retail, mask/base/
 *    `pend` by ours;
 *  - `pl_gun_deviation_get`: retail saves `f31` through a `psq_st`/`vmrghb` pair (the compiler's own float-save form);
 *  - `fn_80278144`/`pl_area_spot_valid_ck`: the early `(u32)((u8)m - 6) <= 1` exit inlines `li r3,0; b epilogue` (+8 B each)
 *    where retail shares one block through a label (rule 8; docs/pl.md);
 *  - `Pl_motion_input_ck`: the case-3/5 failures return inline where retail branches to a shared `return 0`;
 *  - `pl_motion_stick_move`: the conformant if/else-if dispatch interleaves the case tests retail lays out `switch`-shaped;
 *  - `pl_skill_update`: retail's `mr r0,r3; mr r3,self; extsh r4,r0` says `fn_802753E4` returns `s16`; the declaration
 *    stays wider (`pl_skill_update2`/`fn_80278D1C` depend on it) and each of the five `(s16)` call sites costs one row;
 *  - `fn_8027B0BC`: six `clrlwi r0,r0,24` before the `stb`s into `q + 0x5E1` (a peephole-level residual);
 *  - `pl_carry_item_get`: a `v`/`id` callee-saved swap; `pl_area_buff_apply`: retail gives the jump table r23 and the loop bound
 *    r30, ours the mirror (playbook 22); `pl_item_now_usable_ck`: the frame is 0x30 against retail's 0x40 (unused locals);
 *  - `pl_target_lock_ck`: an extra `li r31,0`/`b` pair where retail shares case 2's block with the guard's edge;
 *    `Pl_bari_ck`: retail lays the first arm out of line and the second inline, the natural chain is the mirror;
 *  - `pl_kind_item_equip_find`/`pl_act_held_class_get`/`pl_act6_class_ck`: one or two register or block choices around the id scans;
 *  - `pl_weapon0_hit_feedback`: the self/quake callee-saved registers are swapped (r31/r30), every spelling tried;
 *  - `pl_weapon_parts_refresh`: the weapon pointer and the variant sit in r26/r29 where retail has r27/r27 (98.5 %);
 *  - `equip_defense_get`: register and loop-shape residuals;
 *  - `pl_hit_result_best_get`: retail re-tests the second byte's range (`li r0,0xff; cmplwi; beq; cmplw; bge`) where ours
 *    folds it (20 B short); `fn_8027D40C`: the 32-bit scan's two induction variables sit in r5/r6 swapped;
 *  - `pl_stun_tick`: two extra `extsh` before its clamp stores (+8 B);
 *  - the other 54 partial rows have no recorded cause.
 *  - `fn_8026CC7C` is byte-identical; its switch table relocation goes through MWCC's local `@NNNN` where the target
 *    names `jumptable_805C5FA0` (same address).  The extab record names agree (`@etb_800126CC` is in both objects).
 *  - the action and equipment sections are not in `.text` order (`Pl_attack_set_sub` and `pl_hit_attack_build`.. sit after
 *    `fn_8027B0BC`); the object's function order is source order, so the file has to
 *    be sorted before the unit can link.
 *  - `.data`: retail has a zero word at 0x805C60A4 between the act-entry section's second switch table and its pick
 *    table that MWCC does not emit (it puts a zero-initialised variable in `.bss`/`.sdata`), so the pick table sits 4
 *    bytes early.
 *  - flipcheck: the object emits no `.bss` (0x20 claimed), `.rodata` (0x40), `.sbss` (0x8) or `.sdata` (0x48);
 *    `.text` 0x130B8, `.data` 0x174, `.sdata2` 0x10, extab 0x588 and extabindex 0x84C against the claims 0x1AD18,
 *    0x2178, 0x1D8, 0x928 and 0xD50; every compared section differs in bytes.
 *  - `equip_color_table`/`armor_color_table` (0x805C6238/0x805C6278) are this unit's `.data`, read through `extern`
 *    declarations until the section is emitted.
 * SHAPES. `switch` with `default:` written first lays the default arm first (`fn_80275C34`, `Pl_act_set_motion`); a
 *   case list whose bodies retail places shared puts the second body first (`fn_80277FF8`'s case 3: 1/8 then 5/6/9);
 *  - a pad record reached through a local `PlayerPad*` (`rec = &Psw[pad]`) forms `Psw + pad * 0x350` index first
 *    (`pl_input_pause_flag_set`, `pl_input_refresh`); a `u32` frame counter divided signed is `(s32)frame % 5`;
 *  - the rig's `new PlSeRig`/`delete` sit under `#pragma dont_inline on`, which keeps the implicit constructor and
 *    destructor out of line (retail's `__ct__7PlSeRigFv`/`__dt__7PlSeRigFv`);
 *  - an `s16` parameter updated in place (`amount += (s16)(amount / 10)`, `value *= 3`) keeps retail's raw sums
 *    (`pl_health_recover`, `pl_act_gauge_gate`, `pl_gauge_ceiling_grow`); an `s32` sum clamped through `(s8)` casts
 *    (`pl_gun_recoil_get`, `pl_gun_reload_get`);
 *  - `Pl_decide_mot_get` walks `pl_mot_pick_tbl` as a flat word table (`row += 4`, `[index * 4]`);
 *  - A `switch` on an unsigned value puts the case blocks out of line with a forward `b` to the default
 *   (`pl_equip_skill_valid_ck`, `pl_skill_valid_ck`, `pl_bonus_skill_valid_ck`); `switch (u8)` promotes to a signed `cmpwi` chain.
 *  - the pool floats are `extern` declarations used as load operands, never definitions (playbook 29), and a multiply
 *    is written `lbl * value`, the target's `fmuls` operand order;
 *  - `pl_act_param_tier_ck`: the dead copy chain `classCopy0..2` of `self->field_0x002` plus a separate `weaponClass`
 *    load fixes MWCC's web order so `value` lands in r4; the load stays `(s16)self->part_tbl_b_0xD4[idx]`, above the
 *    early-out (docs/pl.md);
 *  - `fn_8026BF98`/`fn_8026CC7C` take the action state as a local `st` (retail keeps the block base in a callee-saved
 *    register); `fn_8026BF98`'s `Vec3` loop is `do { } while (p < &vec[16])` (a `for`/`while` adds a guard), it reads
 *    `st->unk7D` through a `switch` with `default:` first (`cmpwi`, fall-through layout), and it tests the inner
 *    request bits through `self->req_bits_b_0x0BC` but the enclosing ones through `st->flagsBC` (the base register shows);
 *  - `fn_8026CC7C` is the m2c reconstruction: its `1U`/`s32` spellings and `st->flagsB8`/`self->field_0x0CC` base choices
 *    are load-bearing; `fn_8026CBE4` keeps two unused trailing parameters (its callers pass four);
 *  - `equip_skill_points_get`/`equip_skill_tree_has_ck` dispatch with `switch ((u32)kind)`: cases 1-5, case 6, then 7-15 as `default`, which
 *    lays the bodies out in retail's B, C, A order;
 *  - the record walks index the typed arrays (`rec->skill_id[i]`); `pl_item_slot_find`/`pl_item_room_get` scan `plw->slot_id`
 *    with a flat `for (i = 0; i < 24; i++)` (unrolled eight-wide under `mtctr 3`), and `pl_item_room_get`'s spare-slot scan
 *    is a `u16*` walk (`p[i * 2]`); `Pl_cat_skill_ck` is a two-iteration loop over a `u16*` walk (`p += 2`);
 *  - the valid-bit updates in `pl_parts_color_set`/`fn_8027373C`/`pl_equip_model_set` are compound (`plw->set_valid &= (u16)~mask`);
 *    the plain assignment adds a `clrlwi` before the `sth`;
 *  - narrow returns decide where MWCC re-converts: `item_take` and `pl_item_add` return `s16`, `fn_802736A0`'s
 *    `Get_pl_type__FP6_EQUIPP6_EQUIP` returns `u8`, `fn_802724E8`/`equip_columns_skills_get` take signed-byte `s8*`/`s8` levels;
 *    `fn_80272D5C` restates `(s16)v` at its second store and compare;
 *  - `equip_columns_skills_get` declares `tv, tb, ta` in that order (stack slots 0x20/0x18/0x10 and the register order), nests the
 *    value test `if (v != 0) { if (v > 0) ... } else { zeros }` and swaps tv/tb before ta; `fn_80272B10` indexes
 *    `plw->slot_id` with `?:` and calls `GetItemData` in each arm; `pl_item_add` skips its second lookup through a
 *    single-iteration `for (;;)` with `break`, and its `case 4` compares `plw->item_slot_sel` against the first lookup;
 *  - `pl_weapon_stat_get` and `pl_gun_clip_get` write their case bodies in the order their jump tables point at, not case order;
 *  - `Pl_net_send` is called through its two-parameter view `PlNetSend2`: retail's call sites pass two arguments to
 *    the owner's three-parameter function, and a third argument would emit `li r5,X`;
 *  - an `s16`/`s8` parameter with a compound assignment stores the raw sum and sign-extends only at the compare
 *    (`pl_attack_up_set`, `pl_defense_up_set`, `fn_8027C89C`, `fn_8027A044`, `fn_80276CE8`, `pl_class7_meter_add`, `pl_area_stamina_drain_step`,
 *    `pl_sharpness_add`, `fn_8027D5A4`; playbook 38); `field = field + value` and a temporary extend at the store, and
 *    `field--` stores raw where `field = field - 1` extends (`fn_8027B0BC`);
 *  - a 32-bit accumulator that retail sign-extends only at each compare is an `s32` with `b = (s16)(b + N)` on the
 *    arms that convert and `a -= b` for the difference (`pl_shell_class_get`, `pl_gun_reload_tier_get`, `pl_carry_item_get`);
 *  - equality on a `u16`/`u8` value pairs as `cmpwi` through a signed local or cast (`s32 id = self->act_no`,
 *    `(s32)(u8)x`, `(s32)arg0 == N`) and `(u32)` on an `s32` helper gives `cmplwi` (`pl_act_add_class7_gauge`, `pl_skill_update2`);
 *  - `x <= N ? 1 : 2` gives retail's `xoris/subfic/addc/subfe/addi` and `x == N ? a : b` its `addi/subfic/nor/srawi`
 *    (`Get_Shell_bure_type`, `pl_gun_reload_tier_get`, `pl_shell_class_get`); a constant before the index (`lbl + k + idx * 4`) makes
 *    MWCC form the base first;
 *  - `extern u8 lbl_80792150;` taken by address gives retail's `@sda21` form where an unsized array gives `lis/addi`;
 *  - `fn_802784B8`'s first case falls through into a `default:` written between the cases; `pl_motion_stick_move`'s shared
 *    tail is the `switch`'s `default:` after the last case; `pl_impact_pos_get` writes its `default:` first;
 *  - `pl_item_now_usable_ck` ends in `return 0` with an explicit `default: return 1`, and every refused path is
 *    `if (!c) return 1; break;` (the `if (c) return 0; return 1;` form if-converts; playbook 34);
 *  - `pl_target_lock_ck` writes its cases in body address order (0, 4, 2) with the range guard negated and its arms swapped;
 *  - `fn_8027AF34` keeps the pre-decrement value in an `s16` local and an unreachable `if (v < 0) return;` after the
 *    store, which reproduces retail's `blelr ... sth ... bltlr`;
 *  - `pl_pos_blend_start` writes `(f32)(s16)arg1` inline at each division (retail recomputes the conversion);
 *  - `pl_attack_range_tier_get` writes `* 14` as `(x * 7) << 1` (strength reduction kept); `pl_item_slot_next_get` casts its `u16` index to
 *    `s16` each iteration; `pl_impact_pos_get`'s local declaration order fixes its stack slots; `pl_ground_probe_ahead` spells its
 *    constant `-0x1006`; `pl_gun_ammo_entry_get`/`Get_Shell_rate_adj` read `((u8*)&self->equipC)[0]` before forming the pointer;
 *  - `pl_area_buff_apply`'s 15-case `switch` is a jump table and one arm assigns the loop counter (`i = 5`);
 *  - `Get_Shell_rate_adj` converts both shell values through one `s16 raw` local (one conversion slot, as retail);
 *    `pl_step_se_kind_get` declares its loop index before `v` (the r8/r9 order); `pl_item_icon_kind_get` is `while (row[0] != 0)` with
 *    `id == row[0]` (the rotated loop); `equip_gun_bonus_ck` opens with the `return 0` guard for non-gun kinds.
 */

/* ==== 0x802693C4-0x8026BA1C: the part/motion cluster ==== */

#include "types.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/pl_se_rig.h"
#include "Pl/pl_skill.h"
#include "sound/fn_800DD1F0.h"
#include "Pl/pl_act_step_data.h"
#include "lobby/lobby_w.h"
#include "lobby/lb_equip_page.h"
#include "menu/hit_attack_list_push.h"
#include "unsplit/Pl.h"
#include "sound/mhchar_motion_set.h" /* mhchar_motion_set / mhchar_motion_layer_set (rule 2) */
#include "camera/camera_main.h"   /* get_camera_rot_x / get_camera_rot_y / camera_tilt_state_get (rule 2) */
#include "menu/get_pop_dat_ptr.h" /* option_cfg_eq_ck (rule 2) */
#include "ef/get_move_work_adrs.h"
#include "fn_8004CAD8.h"          /* calcVecAngX / rotVecX / rotVecY / MTX34_ctor (rule 2) */
#include "mh3_pad.h"              /* VEC3_ctor (rule 2) */
#include "ef/fn_800CDB2C.h"       /* my_player_no (rule 2) */
#include "Pl/equip_variant_get.h"
#include "Pl/set_demo_equip_data_func.h"
#include "Pl/arena_userdata_slot_apply.h"
#include "Pl/Pl_master_ck.h"
#include "quest/arenatask.h"           /* arena_eqdata_from_userdata / arena_eqdata_from_vsuser (rule 2) */
#include "quest/quest_item_slot.h"     /* quest_move_state_valid_ck / quest_move_state_get (rule 2) */
#include "userdata_item/userdata_pl_look_set.h"
#include "vs_user_name_copy.h"
#include "sound/fn_800D7F54.h"         /* se_handle_clear (rule 2) */
#include "Network/network_pat_control.h" /* NetCtrlWk, isServerSelectState (rule 2) */
#include "mh3_pad/system_w.h"          /* system_w (rule 2) */

/* 0x8079A014: -1.0, this unit's `.sdata2` pool (a load, never a definition - playbook 29). */
extern "C" {
extern const f32 pl_act_float_neg1;
extern f32 pl_act_float_10; /* 0x8079A010: 10.0 */
}

/* The joint holder `_PLW::physics_0x13C` points at: the `MHchar` the player's joints are read from
 * sits at `+0x04`, the same layout `ef/eft001.cpp` names `joint_0x004`. size: 0x4 + sizeof(MHchar) */
typedef struct PlJointHolder {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ MHchar chr_0x04;
} PlJointHolder;

/* The six symbols the map spells mangled are defined at C++ scope so the front-end reproduces the
 * map's names (rules 9 and 50); the rest are the map's unmangled `fn_` stems and are `extern "C"`. */

void set_com_motion_type(u8 type)
{
    if (pl_global != NULL) {
        pl_global->com_motion_type = type;
    }
}

u16 Get_motion_no(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.motion_no_0x50;
}

/* The two flags are the callers' (`frame` is masked to 16 bits); the body passes a constant 0/1 to
 * the model layer in their place, which is why the two floats are unused here. */
u32 Pl_frame_check(_PLW* self, u32 frame, f32 a, f32 b)
{
    return mhchar_frame_check(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, (u16)frame, 0);
}

/* 0x8026A0D4 (0xA4): sets the actor's model motion `motion` (looked up in the 0-999 or 1000+ motion table) with
 * its two set-up values, the start frame `b` and the flags `d`. */
void Pl_chr_set_attr(_PLW* self, u16 motion, s32 a, s32 b, u32 d)
{
    s32 id;

    if (motion < 1000) {
        id = fn_8026A00C(motion);
    } else {
        id = fn_8026A068(self, motion);
    }
    mhchar_motion_set(&((PlJointHolder*)self->physics_0x13C)->chr_0x04, 0, self->chunk_ofs, motion, a, id, d, b,
                      pl_act_float_one);
}

/* 0x8026A248 (0x74): sets motion `motion` unless the model already plays it. */
void Pl_chr_setX(_PLW* self, u16 motion, s32 a, s32 b)
{
    if (motion != Get_motion_no(self)) {
        Pl_chr_set_attr(self, motion, a, b, 0);
    }
}

void pl_get_joint_wpos(_PLW* self, u32 joint, nw4r::math::VEC3* out)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.get_joint_wpos(joint, out);
}
namespace s_802693C4 {


extern "C" {

/* Arms one part of one player's object record: 0 when the part is already armed (its state is 1) or
 * was consumed (> 1), 1 after the state moves to 2 and the part's gate is cleared. */
u32 fn_802693C4(s32 player, u8 part, s32 value)
{
    if (pl_global == NULL) {
        return 0;
    }
    _PLOBJ* obj;
    _PLWORK* work;

    obj = &pl_global->table[player];
    if (obj->state[part] > 1) {
        return 0;
    }
    if (obj->group[3][part] == value) {
        obj->state[part] = 1;
        return 0;
    }
    obj->part_flag_0x0B[part] = 0;
    obj->group[0][part] = value;
    obj->state[part] = 2;
    work = &pl_global->objects[player];
    if (part < 7) {
        ((PlSeRig*)work->model_0x13C)->field_0x9C0[part] = 0;
    } else {
        ((PlSeRig*)work->model_0x13C)->field_0x9C7[part - 7] = 0;
    }
    return 1;
}

/* Consumes one part: 0 when the part already carries this value, 1 after the flag is set, the value
 * stored and the state moved to 2; a consumed part (> 1) is reported as 0. */
u32 pl_parts_set(s32 player, u8 part, s32 value)
{
    _PLGLOBAL* g = pl_global;
    _PLOBJ* obj;

    if (g == NULL) {
        return 0;
    }
    obj = &g->table[player];
    if (obj->group[3][part] == value) {
        return 0;
    }
    if (obj->state[part] > 1) {
        return 0;
    }
    obj->part_flag_0x0B[part] = 1;
    obj->group[0][part] = value;
    obj->state[part] = 2;
    ((PlSeRig*)pl_global->objects[player].model_0x13C)->field_0x9C0[part] = 0;
    return 1;
}

/* The value stored for one part of the player the work record names, or -1 with no global or no
 * work record. */
s32 pl_parts_value_get(_PLW* self, s32 part)
{
    if (self == NULL) {
        return -1;
    }
    if (pl_global == NULL) {
        return -1;
    }
    return pl_global->table[self->chunk_ofs].group[3][part];
}

/* 0 when the part already carries this value, 1 while it is still free, -1 once it is consumed. */
s32 pl_parts_set_ck(s32 player, u8 part, s32 value)
{
    if (pl_global == NULL) {
        return -1;
    }
    _PLOBJ* obj = &pl_global->table[player];
    if (obj->group[3][part] == value) {
        return 0;
    }
    if (obj->state[part] <= 1) {
        return 1;
    }
    return -1;
}

/* Returns 0 with no global, otherwise the global's common motion type. */
u8 pl_motion_bank_no_get(void)
{
    if (pl_global == NULL) {
        return 0;
    }
    return pl_global->com_motion_type;
}

/* Clears one player's object record: a part mid-load (state 4 or 5) drops its load request, then the state and
 * the six value words go to -1 and the player's slot byte to 0. */
void fn_802699C8(u8 player)
{
    _PLGLOBAL* g = pl_global;
    _PLOBJ* obj;
    s32 i;

    if (g == NULL) {
        return;
    }
    obj = &g->table[player];
    for (i = 0; i < 7; i++) {
        if ((u32)(obj->state[i] - 4) <= 1) {
            g->loaded_0x200[i] = 0;
        }
        obj->state[i] = 0;
        obj->group[0][i] = -1;
        obj->group[1][i] = -1;
        obj->group[2][i] = -1;
        obj->group[3][i] = -1;
        obj->group[4][i] = -1;
        obj->group[5][i] = -1;
    }
    for (; i < 11; i++) {
        if ((u32)(obj->state[i] - 4) <= 1) {
            g->loaded_0x200[i] = 0;
        }
        obj->state[i] = 0;
        obj->group[0][i] = -1;
        obj->group[1][i] = -1;
        obj->group[2][i] = -1;
        obj->group[3][i] = -1;
        obj->group[4][i] = -1;
        obj->group[5][i] = -1;
    }
    g->slot_state[player] = 0;
}

#pragma dont_inline on

/* This unit's own callees, defined further down. */
void fn_80273484(_PLW* plw);
void pl_parts_refresh(_PLW* plw);
void pl_push_clear(_PLW* plw);
void pl_equip_refresh(_PLW* plw);
void pl_info_text_set(_PLW* plw, const char* src);
void pl_equip_model_set(_PLW* plw, u8* rec);
void pl_parts_color_set(_PLW* plw, u8 idx, s32 val);

/* 0x80269AC4 (0x314): binds a player work to the network slot `slot`: resets its record, takes its look from the
 * mode's source (the demo hook, the arena, the server-select peer card or the user data), builds its model rig and
 * resets its motion, position and equipment models. */
void pl_net_slot_bind(_PLW* plw, s32 slot)
{
    plw->chunk_ofs = slot;
    plw->slot_active = 1;
    plw->field_0xB04 = 0;
    plw->field_0x267 = 0;
    plw->bind_reset_0x00B = 0;
    memset(&plw->field_0x0B8, 0, 0x84);
    plw->held_attack_0x5C5 = 0;
    if (PlayMode_ck() != 2) {
        if ((s32)GameMode_ck() == 0) {
            if (set_demo_equip_data_func != NULL) {
                set_demo_equip_data_func(plw);
            }
        } else if (GameMode_ck() == 3) {
            arena_eqdata_from_userdata(plw);
        } else if (isServerSelectState() == 1 || (u32)NetCtrlWk::isCityModeAlias() == 1) {
            if (slot == (s8)my_player_no()) {
                userdata_pl_look_set(plw);
                if (system_w.net_session_0x90f == 1) {
                    arena_eqdata_from_vsuser(plw);
                }
            } else if (isServerSelectState() == 1) {
                NetCtrlWk::loadPeerCard((NetPlayerCard*)plw, slot);
                if (system_w.net_session_0x90f == 1) {
                    arena_eqdata_from_vsuser(plw);
                }
            }
        } else {
            userdata_pl_look_set(plw);
        }
    } else {
        arena_userdata_slot_apply((u8*)plw);
        vs_user_name_copy(plw, slot);
    }
    plw->physics_0x13C = (_PLW_PHYSICS*)new PlSeRig;
    plw->slot_id[24].item_id = 0xDF;
    plw->slot_id[24].value = 1;
    if (Pl_master_ck(plw) == 1 && quest_move_state_valid_ck() == 1 && (u8)quest_move_state_get() == 4) {
        plw->slot_id[25].item_id = 0x1B6;
    } else {
        plw->slot_id[25].item_id = 0x1B7;
    }
    plw->slot_id[25].value = 1;
    plw->tail_slot_0x300.item_id = 0x35;
    plw->tail_slot_0x300.value = 1;
    plw->vec_0x03C.x = pl_act_float_zero;
    plw->vec_0x03C.y = pl_act_float_zero;
    plw->vec_0x03C.z = pl_act_float_zero;
    copyVec3(&plw->vec_0x048, &plw->vec_0x03C);
    plw->field_0x068 = pl_act_float_zero;
    plw->field_0x06C = pl_act_float_zero;
    plw->field_0x070 = pl_act_float_zero;
    plw->param_0x54 = 0;
    plw->field_0x058 = 0;
    plw->rot_z_0x5C = 0;
    plw->field_0x354 = pl_act_float_one;
    fn_80273484(plw);
    pl_equip_model_set(plw, (u8*)&plw->equipA[0]);
    pl_equip_model_set(plw, (u8*)&plw->equipA[3]);
    pl_equip_model_set(plw, (u8*)&plw->equipA[1]);
    pl_equip_model_set(plw, (u8*)&plw->equipA[2]);
    pl_equip_model_set(plw, (u8*)&plw->equipA[4]);
    pl_equip_model_set(plw, (u8*)&plw->equipA[5]);
    pl_parts_color_set(plw, 5, plw->field_0x25C[4]);
    pl_parts_color_set(plw, 6, plw->field_0x25C[5]);
    if (GameMode_ck() != 2) {
        plw->set_valid |= 1;
        plw->set_valid |= 2;
        plw->set_valid |= 4;
        plw->set_valid |= 8;
        plw->set_valid |= 0x10;
        plw->set_valid |= 0x20;
        plw->set_valid |= 0x40;
        pl_parts_refresh(plw);
    }
    pl_push_clear(plw);
    pl_equip_refresh(plw);
    plw->field_0xB00 = 0;
    plw->field_0xB01 = 0;
    plw->field_0xB02 = 0;
    plw->field_0xB03 = 0;
    pl_info_text_set(plw, NULL);
}

/* 0x80269F04 (0x84): releases the player's model rig: clears its object record and slot flags, deletes the rig, stops
 * its three sound handles and clears the whole work record. */
void pl_model_release(_PLW* plw)
{
    fn_802699C8(plw->chunk_ofs);
    plw->slot_active = 0;
    plw->field_0x001 = 0;
    if (plw->physics_0x13C != NULL) {
        delete (PlSeRig*)plw->physics_0x13C;
        plw->physics_0x13C = NULL;
        se_handle_clear((u8*)plw->field_0xAF4);
        se_handle_clear((u8*)plw->field_0xAF8);
        se_handle_clear((u8*)plw->field_0xAFC);
        memset(plw, 0, sizeof(_PLW));
    }
}

#pragma dont_inline reset

/* The motion-number table lookup for the 0-999 range: row `id / 100`, entry `id % 100`. */
s32 fn_8026A00C(u16 id)
{
    if (id >= 1000) {
        return 0;
    }
    return lbl_805C0C98[id / 100][id % 100];
}

/* The motion-number table lookup for the 1000+ range: two levels, the first keyed by the work
 * record's own byte, the second by `(id - 1000) / 100`. */
s32 fn_8026A068(_PLW* self, u16 id)
{
    u16 index = (u16)(id - 1000);
    u8 kind = self->field_0x002;
    s16* row = lbl_805C1584[kind][index / 100];

    if (row == NULL) {
        return 0;
    }
    return row[index % 100];
}

/* The character attribute setter's argument shuffle: the motion word is narrowed and the fifth
 * argument is left 0. */
void Pl_chr_set_attr_default(_PLW* self, u16 motion, s32 a, s32 b)
{
    Pl_chr_set_attr(self, motion, a, b, 0);
}

/* The character motion setter's argument shuffle (`fn_8026A178` takes a sixth, always-0 argument). */
void pl_chr_set_motion_default(_PLW* self, s32 a, u16 motion, s32 b, s32 c)
{
    fn_8026A178(self, a, motion, b, c, 0);
}

/* The model block's per-index value setter. */
void pl_model_set_value(_PLW* self, s32 index, f32 value)
{
    mhchar_mat_blend_set(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, index, value);
}

/* Hands the model layer the scale the work record carries. */
void pl_motion_speed_apply(_PLW* self)
{
    mhchar_motion_speed_set(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, self->field_0x354);
}

void pl_model_visible_set(_PLW* self, u8 value)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF1 = value;
}

void pl_model_hide(_PLW* self)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF1 = 0;
}

void pl_model_set_state(_PLW* self, u8 value)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF2 = value;
}

void pl_model_aux_flag_clear(_PLW* self)
{
    ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xF2 = 0;
}

/* The same frame check as `Pl_frame_check`, with the model layer's second flag set. */
u32 Pl_frame_check_alt(_PLW* self, u16 frame, f32 a, f32 b)
{
    return mhchar_frame_check(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, frame, 1);
}

/* The model layer's frame reset. */
u32 Pl_motion_end_ck(_PLW* self)
{
    return mhchar_motion_end_ck(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, 0);
}

f32 pl_rig_get_motion_frame(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0x74;
}

f32 pl_rig_get_float_a4(_PLW* self)
{
    return ((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xA4;
}

/* Whether the model's blend value is above zero. */
u32 pl_model_blend_positive_ck(_PLW* self)
{
    return !(((PlSeRig*)self->physics_0x13C)->models_0x004[0].model.field_0xBC < pl_act_float_zero);
}

/* The named joint's world position, through the model. */
void pl_joint_mtx_get(_PLW* self, s32 joint, nw4r::math::MTX34* out)
{
    mhchar_joint_mtx_get(&((PlSeRig*)self->physics_0x13C)->models_0x004[0].model, joint, out);
}

/* The work record's actor-mode byte. */
u8 fn_8026A3A0(_PLW* self)
{
    return self->field_0x18;
}

/* The three motion integrators: position += velocity, velocity += acceleration, and both zeroed. */
void pl_motion_integrate_stop(_PLW* self)
{
    self->motion_pos_0x3C += self->motion_vel_0x78;
    self->motion_pos_0x40 += self->motion_vel_0x7C;
    self->motion_pos_0x44 += self->motion_vel_0x80;
}

void pl_motion_integrate(_PLW* self)
{
    self->motion_vel_0x78 = self->motion_vel_0x78 + self->motion_acc_0x84;
    self->motion_vel_0x7C = self->motion_vel_0x7C + self->motion_acc_0x88;
    self->motion_vel_0x80 = self->motion_vel_0x80 + self->motion_acc_0x8C;
    self->motion_pos_0x3C += self->motion_vel_0x78;
    self->motion_pos_0x40 += self->motion_vel_0x7C;
    self->motion_pos_0x44 += self->motion_vel_0x80;
}

void pl_motion_clear(_PLW* self)
{
    self->motion_acc_0x8C = pl_act_float_zero;
    self->motion_acc_0x88 = pl_act_float_zero;
    self->motion_acc_0x84 = pl_act_float_zero;
    self->motion_vel_0x80 = pl_act_float_zero;
    self->motion_vel_0x7C = pl_act_float_zero;
    self->motion_vel_0x78 = pl_act_float_zero;
}

void pl_motion_acc_clear(_PLW* self)
{
    self->motion_acc_0x8C = pl_act_float_zero;
    self->motion_acc_0x88 = pl_act_float_zero;
    self->motion_acc_0x84 = pl_act_float_zero;
}

/* The three per-id flag words (128 ids each at +0xE0 and +0xF0, one word at +0xDC): set, test and
 * clear. */
void fn_8026A618(_PLW* self, s32 id)
{
    self->id_flags_0xE0[id / 32] |= 1 << (id & 31);
}

u32 pl_part_flag_ck(_PLW* self, s32 id)
{
    return (self->id_flags_0xE0[id / 32] & (1 << (id & 31))) != 0;
}

void fn_8026A678(_PLW* self, s32 id)
{
    self->id_flags_0xF0[id / 32] |= 1 << (id & 31);
}

u32 pl_part_flag2_ck(_PLW* self, s32 id)
{
    return (self->id_flags_0xF0[id / 32] & (1 << (id & 31))) != 0;
}

void pl_id_bit_set(_PLW* self, s32 id)
{
    self->id_flags_0xDC |= 1 << (id & 31);
}

s32 pl_id_bit_ck(_PLW* self, s32 id)
{
    return (self->id_flags_0xDC & (1 << (id & 31))) != 0;
}

void pl_id_bit_clear(_PLW* self, s32 id)
{
    self->id_flags_0xDC &= ~(1 << (id & 31));
}

/* 0x802695A4 (0x3F4): re-arms the player's weapon sub-model parts 7-9 from its weapon (a blade: part 7 only; kinds 1/3:
 * parts 7 and 8 with one variant; a bowgun: the frame, barrel and stock variants), tagging each part's value with the
 * weapon type in the top byte; non-zero when any part changed. */
u32 pl_weapon_parts_refresh(s32 player, _EQUIP* weapon, _EQUIP* parts)
{
    _PLOBJ* obj;
    PlSeRig* rig;
    u16 variant;
    s32 changed;
    u16 barrel;
    u16 stock;
    s32 forced;
    s32 cur;
    s32 id;
    s32 kind;
    u8 type;

    if (pl_global == NULL) {
        return 0;
    }
    obj = &pl_global->table[player];
    rig = (PlSeRig*)pl_global->objects[player].model_0x13C;
    rig->sub_kind_0x9CA[0] = 0xFF;
    rig->sub_kind_0x9CA[1] = 0xFF;
    rig->sub_kind_0x9CA[2] = 0xFF;
    changed = 0;
    type = Get_pl_type(weapon, parts);
    variant = equip_variant_get(weapon);
    if (variant == 0) {
        variant = 0xFFFF;
    }
    switch (type) {
    case 0:
    case 2:
    case 7:
    case 8:
        cur = obj->group[3][7];
        id = cur & 0xFFFFFF;
        kind = (u32)(cur & 0xFF000000) >> 24;
        if (kind != type || id != variant) {
            if (variant == 0xFFFF) {
                fn_802693C4(player, 7, -1);
            } else {
                fn_802693C4(player, 7, (type << 24) | variant);
            }
            changed = 1;
        }
        if (obj->group[3][8] != -1) {
            fn_802693C4(player, 8, -1);
            changed++;
        }
        if (obj->group[3][9] != -1) {
            fn_802693C4(player, 9, -1);
            changed++;
        }
        break;
    case 1:
    case 3:
        cur = obj->group[3][7];
        id = cur & 0xFFFFFF;
        kind = (u32)(cur & 0xFF000000) >> 24;
        if (kind != type || id != variant) {
            if (variant == 0xFFFF) {
                fn_802693C4(player, 7, -1);
            } else {
                fn_802693C4(player, 7, (type << 24) | variant);
            }
            changed = 1;
        }
        cur = obj->group[3][8];
        id = cur & 0xFFFFFF;
        kind = (u32)(cur & 0xFF000000) >> 24;
        if (kind != type || id != variant) {
            if (variant == 0xFFFF) {
                fn_802693C4(player, 8, -1);
            } else {
                fn_802693C4(player, 8, (type << 24) | variant);
            }
            changed++;
        }
        if (obj->group[3][9] != -1) {
            fn_802693C4(player, 9, -1);
            changed++;
        }
        break;
    case 4:
    case 5:
    case 6:
        barrel = equip_variant_get(&parts[0]);
        stock = equip_variant_get(&parts[1]);
        cur = obj->group[3][7];
        id = cur & 0xFFFFFF;
        kind = (u32)(cur & 0xFF000000) >> 24;
        if (kind != type || id != variant) {
            if (variant == 0xFFFF) {
                fn_802693C4(player, 7, -1);
            } else {
                fn_802693C4(player, 7, (type << 24) | variant);
            }
            changed = 1;
            forced = 1;
        } else {
            forced = 0;
        }
        cur = obj->group[3][8];
        id = cur & 0xFFFFFF;
        kind = (u32)(cur & 0xFF000000) >> 24;
        if (kind != type || id != barrel || forced > 0) {
            if (barrel == 0xFFFF) {
                fn_802693C4(player, 8, -1);
            } else {
                obj->group[3][8] = 0xFFFF;
                fn_802693C4(player, 8, (type << 24) | barrel);
                obj->group[3][8] = cur;
            }
            changed++;
        }
        cur = obj->group[3][9];
        id = cur & 0xFFFFFF;
        kind = (u32)(cur & 0xFF000000) >> 24;
        if (kind != type || id != stock || forced > 0) {
            if (stock == 0xFFFF) {
                fn_802693C4(player, 9, -1);
            } else {
                obj->group[3][9] = 0xFFFF;
                fn_802693C4(player, 9, (type << 24) | stock);
                obj->group[3][9] = cur;
            }
            changed++;
        }
        break;
    }
    return changed != 0;
}

/* 0x8026A178 (0xAC): sets motion `motion` on model layer `layer` (the layered form of `Pl_chr_set_attr`). */
void fn_8026A178(_PLW* self, s32 layer, u16 motion, s32 a, s32 b, u32 d)
{
    s32 id;

    if (motion < 1000) {
        id = fn_8026A00C(motion);
    } else {
        id = fn_8026A068(self, motion);
    }
    mhchar_motion_layer_set(&((PlJointHolder*)self->physics_0x13C)->chr_0x04, layer, 0, self->chunk_ofs, motion, a,
                            id, d, b, pl_act_float_one);
}

/* 0x8026A3A8 (0x44): turns the actor's heading (+0x0A8) to its stick angle (+0x0D0) seen from the camera. */
void pl_act_set_cam_ang(_PLW* self)
{
    u32 cam = (u16)get_camera_rot_y() - 0x4000;

    self->field_0x0A8 = (u16)(self->part_tbl_a_0xD0[0] + cam);
}

/* 0x8026A3EC (0xF8): turns the actor's heading to the stick from the camera, and unless the option or a camera
 * tilt holds it, points its pitch (+0x0AC) along the camera's. */
void pl_camera_heading_update(_PLW* self)
{
    nw4r::math::MTX34 m0;
    nw4r::math::MTX34 m1;
    VEC3 dir;
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;
    u32 cam;
    u32 yaw;

    MTX34_ctor(&m0);
    MTX34_ctor(&m1);
    VEC3_ctor(&dir);
    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    VEC3_ctor(&v3);
    cam = (u16)get_camera_rot_y() - 0x4000;
    self->field_0x0A8 = (u16)(self->part_tbl_a_0xD0[0] + cam);
    if (option_cfg_eq_ck(self->chunk_ofs, 10, 1) != 1 && camera_tilt_state_get() == 0) {
        yaw = self->field_0x0A8 - cam - 0x4000;
        self->field_0x0AC = (u16)get_camera_rot_x();
        dir.x = pl_act_float_zero;
        dir.y = pl_act_float_zero;
        dir.z = pl_act_float_10;
        rotVecY(&dir, yaw);
        rotVecX(&dir, self->field_0x0AC);
        self->field_0x0AC = calcVecAngX(&dir);
    }
}

/* 0x8026A5A4 (0x74): the per-frame jump gravity (+0x088) from the rise speed (+0x07C) over half the jump frames;
 * 0 when the jump is too short or already falling. */
u32 pl_jump_gravity_set(_PLW* self, s16 frames)
{
    s16 half = frames / 2;
    f32 gravity = pl_act_float_neg1 * self->motion_vel_0x7C;

    if (half <= 1 || self->motion_vel_0x7C < pl_act_float_zero) {
        self->motion_acc_0x88 = gravity;
        return 0;
    }
    gravity /= half;
    self->motion_acc_0x88 = gravity;
    return 1;
}

/* 0x8026AE04 (0x58): stores `frames` into the local player's +0x352 input-lock timer. */
void pl_input_lock_set(s16 frames)
{
    _PLW* work = (_PLW*)get_move_work_adrs(2);

    if (work != NULL) {
        work[(s8)my_player_no()].field_0x352 = frames;
    }
}

/* 0x8026B934 (0x68): whether option 0 is on and the work record has bit 0 but not bit 2 of +0x134. */
u32 fn_8026B934(_PLW* self)
{
    if (option_cfg_eq_ck(self->chunk_ofs, 0, 0) == 1 && (self->field_0x134 & 1) != 0 && (self->field_0x134 & 4) == 0) {
        return 1;
    }
    return 0;
}

/* 0x8026B99C (0x68): the option-1 form of `fn_8026B934`. */
u32 fn_8026B99C(_PLW* self)
{
    if (option_cfg_eq_ck(self->chunk_ofs, 1, 0) == 1 && (self->field_0x134 & 1) != 0 && (self->field_0x134 & 4) == 0) {
        return 1;
    }
    return 0;
}

/* Whether the work record's second flag bit is set. */
u32 fn_8026BA04(_PLW* self)
{
    return (self->field_0x134 & 2) != 0;
}

} /* extern "C" */

} /* namespace s_802693C4 */

/* ==== 0x8026BA1C-0x8026FFBC: the master layer (the actor's action-state block) ==== */

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the vector record these bodies work on (rule 11) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

typedef nw4r::math::VEC3 Vec3;

/* The actor's 132-byte action-state block, `self + 0xB8`: `fn_8026F7B4`/`pl_input_clear` clear it, the two
 * big dispatchers (`fn_8026BF98`, `fn_8026CC7C`) drive it, and `fn_8026BE94` reads the request word and the
 * per-part selector out of it, so one type has to carry both views. Retail materialises the base pointer
 * once and keeps it in a callee-saved register - hence the local `st` in the dispatchers below. */
typedef struct ActState {
    u16 flagsB8;         /* 0x00 - self->field_0x0B8: request bit word A (the "kind" mask fn_8026BE94 tests) */
    u16 unk02;           /* 0x02 */
    u16 flagsBC;         /* 0x04 - self->req_bits_b_0x0BC: request bit word B */
    u8 pad06[0x0A];      /* 0x06 */
    u16 unk10;           /* 0x10 - the word fn_8026BE94 tests 0x2000/0x1000/0x800/0x400 against */
    u16 unk12;           /* 0x12 */
    u16 flagsCC;         /* 0x14 - self->field_0x0CC: per-part behaviour bits */
    u8 pad16[0x3E];      /* 0x16 */
    f32 unk54;           /* 0x54 - self->unk10C: the charge/axis value fn_8026CC7C bands */
    f32 unk58;           /* 0x58 - self->field_0x110: the weapon/model axis length */
    u8 pad5C[0xC];       /* 0x5C */
    u8 part[8];          /* 0x68 - per-part selector table (fn_8026BE94 indexes it, the dispatchers read part[1]) */
    u8 mode;             /* 0x70 - self->unk128: the dispatchers' switch value */
    u8 unk71;            /* 0x71 - self->field_0x129: the "an action is running" latch */
    u8 pad72[6];         /* 0x72 */
    s32 unk78;           /* 0x78 - self->unk130 */
    u8 unk7C;            /* 0x7C - self->unk134: low two bits are a per-part flag */
    u8 unk7D;            /* 0x7D - self->unk135 */
    u8 unk7E;            /* 0x7E - self->unk136: the deferred-command bits */
    u8 unk7F;            /* 0x7F - self->unk137: the hold delay */
    u16 unk80;           /* 0x80 - self->unk138: flags latched for the hold */
    u16 unk82;           /* 0x82 - self->unk13A: flags latched for the hold */
} ActState;
namespace s_8026BA1C {


/* Unmangled map names: `extern "C"` so the compiler emits the map's spelling. */
extern "C" {
void fn_8026A618(_PLW* self, u32 id);
u32 pl_part_flag_ck(_PLW* self, u32 id);
void fn_8026A678(_PLW* self, u32 id);
u32 pl_id_bit_ck(_PLW* self, u32 id);
u32 fn_8026B99C(_PLW* self);
u32 fn_8026B934(_PLW* self);
u32 fn_8026BA04(_PLW* self);
u32 option_cfg_eq_ck(u8 value, u32 low, u32 high);
void pl_input_refresh(_PLW* self, u32 value);
void mhchar_ofs_set(void* dst, void* src);
u8 GameMode_ck(void);
s8 my_player_no(void);
u32 lobby_input_locked_ck(void);
u16 get_camera_rot_y(void);
void* memset(void* dst, int value, u32 size);
}

/* Pool literals owned by a neighbouring Pl unit: 60.0f and the two charge bands `fn_8026CC7C` compares
 * `st->unk54` against. Declared, not defined - defining them would rebuild the section (playbook 29). */
extern "C" { extern f32 lbl_8079A020; }
extern "C" { extern f32 lbl_8079A024; }
extern "C" { extern f32 lbl_8079A028; }
} /* namespace s_8026BA1C */


/* Mangled map names: written by their source name so the compiler emits the map's spelling. */
u8 PlayMode_ck(void);
u8 get_now_areano(void);
u32 Pl_Skill_ck(_PLW* self, u16 skill);
namespace s_8026BA1C {


/* This unit's own order: the file has to emit the functions in the map's address order. */
extern "C" void fn_8026BE94(_PLW* self, ActState* req, u32 mask, u32 idx);
extern "C" void fn_8026CC7C(_PLW* self, u8 pad);
extern "C" void fn_8026BA1C(_PLW* self);
} /* namespace s_8026BA1C */

u32 Pl_master_ck(_PLW* self);
namespace s_8026BA1C {

extern "C" u8 pl_act_param_tier_ck(_PLW* self, u32 idx);
extern "C" u32 pl_stick_window_gated_ck(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 pl_stick_dir_ck(_PLW* self, s32 kind);
extern "C" u32 pl_stick_window_level_ck(_PLW* self, u32 idx, u16 low, u16 high);
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which, u32 arg2, u32 arg3);

/* 0x8026BA1C: the master action dispatcher - picks the actor's action id from the angle window the
 * target lies in, per weapon class. */
extern "C" void fn_8026BA1C(_PLW* self)
{
    fn_8026A618(self, 4);
    fn_8026A618(self, 25);
    if (self->field_0x129 == 0) {
        if (fn_8026FA6C(self, 0, -9102, 9102) == 1) {
            fn_8026A618(self, 62);
            fn_8026A618(self, 68);
        } else if (fn_8026FA6C(self, 0, 23666, -23666) == 1) {
            fn_8026A618(self, 63);
            fn_8026A618(self, 69);
        } else if (fn_8026FA6C(self, 0, -23666, -9102) == 1) {
            fn_8026A618(self, 61);
            fn_8026A618(self, 67);
            fn_8026A618(self, 65);
        } else if (fn_8026FA6C(self, 0, 9102, 23666) == 1) {
            fn_8026A618(self, 60);
            fn_8026A618(self, 66);
            fn_8026A618(self, 64);
        }
    } else {
        if (self->part_tbl_b_0xD4[0] < 50 && self->unk128 == 1) {
            if (self->field_0x110 <= lbl_8079A020) {
                fn_8026A618(self, 64);
            } else {
                fn_8026A618(self, 65);
            }
        }
        switch (self->field_0x002) {
        case 6:
            if (fn_8026FA6C(self, 0, -8192, 8192) == 1) {
                fn_8026A618(self, 62);
            } else if (fn_8026FA6C(self, 0, 24576, -24576) == 1) {
                fn_8026A618(self, 63);
            } else if (fn_8026FA6C(self, 0, -24576, -8192) == 1) {
                fn_8026A618(self, 61);
            } else if (fn_8026FA6C(self, 0, 8192, 24576) == 1) {
                fn_8026A618(self, 60);
            }
            if (fn_8026FA6C(self, 0, -16384, 5461) == 1) {
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 27307, -16384) == 1) {
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, 5461, 27307) == 1) {
                fn_8026A618(self, 66);
            }
            break;
        case 3:
            if (fn_8026FA6C(self, 0, -5461, 16384) == 1) {
                fn_8026A618(self, 62);
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 16384, -27307) == 1) {
                fn_8026A618(self, 63);
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, -27307, -5461) == 1) {
                fn_8026A618(self, 61);
                fn_8026A618(self, 67);
            }
            break;
        default:
            if (fn_8026FA6C(self, 0, -16384, 5461) == 1) {
                fn_8026A618(self, 62);
                fn_8026A618(self, 68);
            } else if (fn_8026FA6C(self, 0, 27307, -16384) == 1) {
                fn_8026A618(self, 63);
                fn_8026A618(self, 69);
            } else if (fn_8026FA6C(self, 0, 5461, 27307) == 1) {
                fn_8026A618(self, 60);
                fn_8026A618(self, 66);
            }
            break;
        }
    }
}

#pragma peephole off

/* 0x8026BE94: raises the requests an action's own kind mask and state word imply. */
extern "C" void fn_8026BE94(_PLW* self, ActState* req, u32 mask, u32 idx)
{
    if ((req->flagsB8 & mask) == 0) {
        u8 part = req->part[idx];
        if ((u8)(part - 1) <= 7) {
            fn_8026A618(self, 54);
        }
    }
    if (req->unk10 & 0x2000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 56);
    } else if (req->unk10 & 0x1000) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 57);
    }
    if (req->unk10 & 0x800) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 58);
    } else if (req->unk10 & 0x400) {
        fn_8026A618(self, 55);
        fn_8026A618(self, 59);
    }
}

/* 0x8026BF98: the action dispatcher. Switches on the action state's mode byte and raises the command set
 * the actor's request words and hold state imply. */
extern "C" void fn_8026BF98(_PLW* self, u8 pad)
{
    ActState* st = (ActState*)&self->field_0x0B8;
    Vec3 vec[16];
    Vec3* p = vec;

    do {
        VEC3_ctor(p);
        p++;
    } while (p < &vec[16]);

    switch (st->mode) {
    case 2:
        switch (st->unk7D) {
        default:
                if ((st->flagsBC & 0x120) == 0x120) {
                    st->unk7E |= 8;
                } else {
                    st->unk7E = 0;
                    if (st->flagsBC & 0x320) {
                        st->unk7D++;
                        st->unk7F = 2;
                        st->unk80 = st->flagsBC & 0x320;
                        st->unk82 = st->flagsBC & 0x100;
                    } else {
                        st->unk80 = 0;
                        st->unk82 = 0;
                    }
                }
            break;
        case 1:
                if (--st->unk7F == 0 || (st->flagsBC & 0x320) != 0) {
                    if (st->flagsBC & 0x320) {
                        if (st->flagsBC & 0x200) {
                            st->unk7E |= 4;
                        }
                        if (st->flagsBC & 0x100) {
                            st->unk7E |= 1;
                        }
                        if (st->flagsBC & 0x20) {
                            st->unk7E |= 2;
                        }
                        if (((u16)(st->flagsBC | st->unk80) & 0x120) == 0x120) {
                            st->unk7E |= 8;
                        }
                    } else {
                        if (st->unk80 & 0x200) {
                            st->unk7E |= 4;
                        }
                        if (st->unk80 & 0x100) {
                            st->unk7E |= 1;
                        }
                        if (st->unk80 & 0x20) {
                            st->unk7E |= 2;
                        }
                    }
                    st->unk7D = 0;
                } else if ((st->flagsB8 & 0x320) == 0) {
                    if (st->unk80 & 0x200) {
                        st->unk7E |= 4;
                    }
                    if (st->unk80 & 0x100) {
                        st->unk7E |= 1;
                    }
                    if (st->unk80 & 0x20) {
                        st->unk7E |= 2;
                    }
                    st->unk7D = 0;
                }
            break;
        }
        fn_8026BE94(self, st, 0x80, 2);
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 0);
            fn_8026A618(self, 34);
            fn_8026A618(self, 37);
            fn_8026A618(self, 50);
        }
        if (st->flagsBC & 0x200) {
            fn_8026A618(self, 1);
            fn_8026A618(self, 20);
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
            fn_8026A618(self, 22);
        }
        if (self->held_attack_0x5C5 != 0) {
            if (st->flagsBC & 0x8000) {
                fn_8026A618(self, 8);
            }
            if (st->unk7E & 1) {
                fn_8026A618(self, 10);
                fn_8026A618(self, 11);
            }
            if (st->unk7E & 2) {
                if (self->field_0x308 == 0) {
                    if (pl_act_param_tier_ck(self, 0) >= 1) {
                        fn_8026A618(self, 27);
                    } else {
                        fn_8026A618(self, 26);
                    }
                }
                fn_8026A618(self, 76);
            }
        } else {
            if (st->unk7E & 1) {
                fn_8026A618(self, 8);
                fn_8026A618(self, 76);
            }
            if ((st->flagsCC & 0x3C) != 0 && self->field_0x308 == 0) {
                if (st->flagsCC & 0x10) {
                    fn_8026A618(self, 12);
                    fn_8026A618(self, 24);
                }
                fn_8026A618(self, 11);
                fn_8026A618(self, 10);
            }
            if (st->flagsB8 & 0x100) {
                if (pl_act_param_tier_ck(self, 0) >= 1) {
                    fn_8026A618(self, 27);
                } else {
                    fn_8026A618(self, 26);
                }
            }
            if (st->flagsBC & 0x20) {
                fn_8026A618(self, 13);
                fn_8026A618(self, 14);
            }
        }
        if (st->flagsB8 & 0x80) {
            if (st->unk7E & 8) {
                fn_8026A618(self, 18);
                fn_8026A618(self, 19);
                fn_8026A618(self, 15);
                fn_8026A618(self, 16);
            } else if (st->unk7E & 1) {
                fn_8026A618(self, 15);
                fn_8026A618(self, 16);
            }
            fn_8026A618(self, 28);
        }
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 17);
        }
        if (option_cfg_eq_ck(self->chunk_ofs, 10, 1) == 1) {
            if (st->flagsB8 & 4) {
                fn_8026A618(self, 43);
            } else if (st->flagsB8 & 0x10) {
                fn_8026A618(self, 44);
            }
        }
        if (st->flagsB8 & 0x10) {
            fn_8026A618(self, 39);
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 38);
        }
        if (st->flagsBC & 0x10) {
            fn_8026A618(self, 33);
            fn_8026A618(self, 49);
        }
        if (st->flagsBC & 4) {
            fn_8026A618(self, 32);
            fn_8026A618(self, 48);
        }
        if (st->flagsBC & 0x40) {
            fn_8026A618(self, 3);
            fn_8026A618(self, 51);
            fn_8026A618(self, 7);
            fn_8026A618(self, 47);
            fn_8026BA1C(self);
        } else if (st->unk71 != 0) {
            if (self->req_bits_b_0x0BC & 4) {
                fn_8026A618(self, 64);
            } else if (self->req_bits_b_0x0BC & 0x10) {
                fn_8026A618(self, 65);
            }
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 5);
            fn_8026A618(self, 6);
            fn_8026A618(self, 9);
            fn_8026A618(self, 53);
            if (self->field_0x308 == 0) {
                fn_8026A618(self, 2);
                fn_8026A618(self, 52);
                fn_8026A618(self, 77);
            }
        }
        if ((st->flagsB8 & 0x80) != 0 && (st->flagsBC & 0x20) != 0) {
            fn_8026A618(self, 71);
            fn_8026A618(self, 73);
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 70);
                fn_8026A618(self, 72);
            }
        }
        if (st->flagsB8 & 0x80) {
            fn_8026A618(self, 21);
        }
        break;
    case 1:
        if (option_cfg_eq_ck(self->chunk_ofs, 10, 1) == 1) {
            if (st->flagsB8 & 0x2000) {
                fn_8026A618(self, 43);
            } else if (st->flagsB8 & 0x1000) {
                fn_8026A618(self, 44);
            }
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 0);
            fn_8026A618(self, 50);
            fn_8026A618(self, 37);
        }
        if (st->flagsBC & 0x80) {
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
            fn_8026A618(self, 10);
            fn_8026A618(self, 11);
        }
        if ((st->flagsBC & 0x20) != 0 || ((st->flagsBC & 0x200) != 0 && self->field_0x308 == 1)) {
            fn_8026A618(self, 1);
            fn_8026A618(self, 20);
            fn_8026A618(self, 13);
            fn_8026A618(self, 14);
        }
        if ((st->flagsB8 & 0x100) == 0 && (u8)(st->part[1] - 1) <= 7) {
            fn_8026A618(self, 47);
        }
        if (st->flagsBC & 0x100) {
            fn_8026A618(self, 3);
            fn_8026A618(self, 51);
            fn_8026A618(self, 7);
            if (st->unk78 <= 24) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A618(self, 32);
                } else {
                    fn_8026A618(self, 33);
                }
            }
            fn_8026BA1C(self);
        }
        if (st->flagsB8 & 0x100) {
            if (st->unk58 <= lbl_8079A020) {
                fn_8026A618(self, 38);
                if ((s8)st->part[1] > 8) {
                    fn_8026A618(self, 48);
                }
            } else {
                fn_8026A618(self, 39);
                if ((s8)st->part[1] > 8) {
                    fn_8026A618(self, 49);
                }
            }
        }
        fn_8026BE94(self, st, 4, 7);
        if (st->flagsBC & 0x40) {
            fn_8026A618(self, 8);
        }
        if ((st->flagsBC & 0x200) != 0 || fn_8026BA04(self) == 1) {
            fn_8026A618(self, 6);
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 52);
                fn_8026A618(self, 2);
                fn_8026A618(self, 5);
            }
        }
        if (((st->flagsB8 & 4) != 0 && (st->flagsBC & 0x200) != 0) || fn_8026BA04(self) == 1) {
            fn_8026A618(self, 71);
            if (st->unk58 <= lbl_8079A020) {
                fn_8026A618(self, 73);
            }
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 70);
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A618(self, 72);
                }
            }
        }
        if ((st->flagsBC & 0x220) != 0 || (st->unk7C & 4) != 0) {
            fn_8026A618(self, 9);
        }
        if (st->flagsB8 & 4) {
            fn_8026A618(self, 21);
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 20);
        }
        if (st->flagsBC & 0x200) {
            fn_8026A618(self, 76);
            if (pl_part_flag_ck(self, 1) == 0) {
                fn_8026A618(self, 53);
                fn_8026A618(self, 77);
            }
        }
        if (st->flagsBC & 0x20) {
            fn_8026A618(self, 22);
        }
        if (fn_8026B934(self) == 1) {
            fn_8026A618(self, 10);
            fn_8026A618(self, 11);
        }
        if ((st->flagsBC & 0x200) != 0 && self->field_0x308 == 0) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                fn_8026A618(self, 27);
            } else {
                fn_8026A618(self, 26);
            }
        }
        break;
    }
}

#pragma peephole off

/* 0x8026CBE4 */
extern "C" u32 fn_8026CBE4(_PLW* self, u8 which, u32 arg2, u32 arg3)
{
    if (self->unk18 == 1) {
        u16 flags = which == 0 ? self->field_0x0B8 : self->req_bits_b_0x0BC;
        if ((flags & 4) != 0) {
            return 1;
        }
    } else if (which == 0) {
        if ((self->field_0x0B8 & 0x44) == 0x44) {
            return 1;
        }
    } else if ((self->req_bits_b_0x0BC & 0x44) != 0 && (self->field_0x0B8 & 0x44) == 0x44) {
        return 1;
    }
    return 0;
}

#pragma peephole reset

/* 0x8026CC70: the retail body compares a byte of its second argument's structure and returns the first. */
extern "C" u32 fn_8026CC70(u32 value, u8* data)
{
    switch ((u32)data[0x70]) {
    case 1:
        return value;
    }
    return value;
}


/* 0x8026CC7C: the per-weapon-class action dispatcher - a nine-entry jump table on the actor's weapon
 * class, each arm a further switch on the action state's mode byte. */
extern "C" void fn_8026CC7C(_PLW* self, u8 pad)
{
    ActState* st = (ActState*)&self->field_0x0B8;
    fn_8026CC70((u32)self, (u8*)st);
    switch (self->field_0x002) {
    case 0:
        switch (st->mode) {
        case 2:
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 4);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    if ((s32) (st->flagsB8 & 0x80) != 0) {
                        fn_8026A618(self, 0xC);
                    } else {
                        fn_8026A618(self, 0x18);
                    }
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 8);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    if ((s32) (st->flagsB8 & 0x80) != 0) {
                        fn_8026A618(self, 0xC);
                    } else {
                        fn_8026A618(self, 0x18);
                    }
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xC);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 0x10);
                    } else {
                        fn_8026A678(self, 0x12);
                        fn_8026A678(self, 0x13);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 9);
                }
                if ((s32) (st->flagsB8 & 0x100) != 0) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0x11);
                    return;
                }
            } else {
                if (pl_stick_window_gated_ck(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0xC);
                } else if (pl_stick_window_gated_ck(self, 1, 0xE000, 0x2000) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 8);
                } else if (pl_stick_window_gated_ck(self, 1, 0x6000, 0xA000) == 1U) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 9);
                } else if (pl_stick_window_gated_ck(self, 1, 0xA000, 0x2000) == 1U) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0x13);
                }
                if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                    fn_8026A678(self, 4);
                }
                if (pl_stick_window_level_ck(self, 1, 0xA000, 0x2000) == 1U) {
                    fn_8026A678(self, 5);
                }
                if (pl_stick_window_level_ck(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0x11);
                    return;
                }
            }
            break;
        case 1:
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0x13);
                    if (pl_part_flag_ck(self, 1) == 0) {
                        fn_8026A618(self, 0xC);
                        fn_8026A618(self, 0x18);
                    }
                }
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if ((s32) (st->flagsB8 & 0x200) != 0) {
                fn_8026A678(self, 5);
                fn_8026A678(self, 0x11);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 4);
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 0xE);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 9);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 8);
                    } else {
                        fn_8026A678(self, 7);
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 0x10);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 0xA);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 4);
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x10);
                fn_8026A678(self, 0);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0xF);
            }
            if ((s32) (st->unk7C & 1) != 0) {
                fn_8026A678(self, 0xD);
                return;
            }
            break;
        }
        break;
    case 4:
    case 5:
    case 6:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsB8 & 0x80) != 0) {
                if (((s8) st->part[2] > 8) && ((u8) self->unk18 == 1)) {
                    fn_8026A618(self, 0x1D);
                }
            } else if ((u8) (st->part[2] - 1) <= 7U) {
                fn_8026A678(self, 7);
            }
            if ((s32) (st->flagsB8 & 0x2000) != 0) {
                fn_8026A678(self, 0);
            } else if ((s32) (st->flagsB8 & 0x1000) != 0) {
                fn_8026A678(self, 1);
            }
            if ((s32) (st->flagsB8 & 0x800) != 0) {
                fn_8026A678(self, 2);
            } else if ((s32) (st->flagsB8 & 0x400) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if (((s32) (st->unk7E & 8) != 0) && ((s32) self->unk5E6 == 0)) {
                    fn_8026A678(self, 6);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 5);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 4);
                }
                if (((s32) (st->flagsB8 & 0x80) != 0) && ((s32) (st->unk7E & 8) != 0)) {
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0xA);
                    fn_8026A618(self, 0x18);
                    return;
                }
            } else {
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 4);
                }
                if (pl_stick_window_gated_ck(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 6);
                    return;
                }
                if (((s32) self->field_0x308 == 0) && ((s32) (self->field_0x0CC & 0x10) != 0)) {
                    fn_8026A678(self, 5);
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0x18);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xC);
                fn_8026A618(self, 0x18);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 4);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && ((s32) (st->flagsB8 & 0x100) != 0)) {
                fn_8026A678(self, 8);
            }
            if ((s32) (st->flagsB8 & 4) != 0) {
                if (((s8) st->part[7] > 8) && ((u8) self->unk18 == 1)) {
                    fn_8026A618(self, 0x1D);
                }
                if ((s32) (st->flagsBC & 0x40) != 0) {
                    fn_8026A618(self, 0x18);
                }
            } else if ((u8) (st->part[7] - 1) <= 7U) {
                fn_8026A678(self, 7);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 5);
                if ((s32) (st->flagsB8 & 4) != 0) {
                    fn_8026A618(self, 0xC);
                    fn_8026A618(self, 0xA);
                    fn_8026A618(self, 0xB);
                }
            }
            if ((s32) (st->flagsB8 & 0x2000) != 0) {
                fn_8026A678(self, 0);
            } else if ((s32) (st->flagsB8 & 0x1000) != 0) {
                fn_8026A678(self, 1);
            }
            if ((s32) (st->flagsB8 & 0x800) != 0) {
                fn_8026A678(self, 2);
            } else if ((s32) (st->flagsB8 & 0x400) != 0) {
                fn_8026A678(self, 3);
            }
            if ((fn_8026B99C(self) == 1U) && ((s32) self->unk5E6 == 0)) {
                fn_8026A678(self, 6);
                return;
            }
            break;
        }
        break;
    case 1:
        switch (st->mode) {
        case 2:
            if ((s32) self->held_attack_0x5C5 != 0) {
                if (((s32) (st->flagsB8 & 0x80) != 0) && ((s32) (st->flagsBC & 0x200) != 0)) {
                    fn_8026A678(self, 0x15);
                }
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0x14);
                }
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 9);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 5);
                    }
                    if (fn_8026FA6C(self, 0, 0xAAAB, 0xD555) == 1U) {
                        fn_8026A678(self, 0xF);
                    }
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xB);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 1);
                        fn_8026A618(self, 0x4B);
                        return;
                    }
                    fn_8026A678(self, 2);
                    fn_8026A618(self, 0x4A);
                    return;
                }
            } else {
                if ((s32) (st->flagsBC & 0x200) != 0) {
                    fn_8026A678(self, 0x15);
                }
                if (pl_stick_window_gated_ck(self, 1, 0x1555, 0x6AAB) == 1U) {
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 5);
                    }
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                } else if (pl_stick_window_gated_ck(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 0xC);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A618(self, 0x4B);
                    } else {
                        fn_8026A618(self, 0x4A);
                    }
                } else if (pl_stick_window_gated_ck(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xB);
                } else if (pl_stick_window_gated_ck(self, 1, 0xAAAB, 0xD555) == 1U) {
                    if (fn_8026FA6C(self, 0, 0xAAAB, 0xD555) == 1U) {
                        fn_8026A678(self, 0xF);
                    }
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 0x14);
                }
                if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                    fn_8026A678(self, 9);
                    return;
                }
            }
            break;
        case 1:
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0) && (pl_part_flag_ck(self, 1) == 0)) {
                fn_8026A618(self, 0xC);
                fn_8026A618(self, 0x18);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if ((pl_id_bit_ck(self, 0xB) == 0) && ((s32) (st->flagsBC & 0x200) != 0)) {
                if (st->unk58 <= lbl_8079A020) {
                    fn_8026A678(self, 7);
                }
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (pl_id_bit_ck(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 6);
                    }
                }
                if (pl_act_param_tier_ck(self, 0) >= 1U) {
                    fn_8026A678(self, 3);
                }
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xA);
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 6);
                fn_8026A678(self, 0xE);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 9);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (pl_id_bit_ck(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0xB);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        if (pl_act_param_tier_ck(self, 0) >= 1U) {
                            fn_8026A618(self, 0x4B);
                        } else {
                            fn_8026A618(self, 0x4A);
                        }
                    } else {
                        fn_8026A678(self, 0xD);
                        fn_8026A678(self, 0xE);
                    }
                }
                if (fn_8026FA6C(self, 0, 0xA000, 0xE000) == 1U) {
                    fn_8026A678(self, 0xF);
                }
                if (pl_act_param_tier_ck(self, 0) >= 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 5);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 0x12);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                if (pl_act_param_tier_ck(self, 0) >= 1U) {
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 5);
                }
                fn_8026A678(self, 0x14);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 8);
                fn_8026A678(self, 0x13);
                fn_8026A678(self, 3);
            }
            if (((s32) (st->flagsBC & 0x20) != 0) || (((s32) (st->flagsBC & 0x200) != 0) && ((u8) self->field_0x308 == 1))) {
                fn_8026A678(self, 0x15);
                return;
            }
            break;
        }
        break;
    case 3:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 2);
                }
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 8);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xD);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x12);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 0xE);
                    } else {
                        fn_8026A678(self, 7);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                }
                if ((s32) (st->flagsB8 & 0x80) != 0) {
                    if ((s32) (st->unk7E & 2) != 0) {
                        fn_8026A678(self, 0xA);
                        fn_8026A678(self, 0x10);
                        return;
                    }
                } else {
                    fn_8026A678(self, 0xB);
                    return;
                }
            } else {
                if (pl_stick_window_gated_ck(self, 1, 0x1555, 0x6AAB) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    if (pl_act_param_tier_ck(self, 0) >= 1U) {
                        fn_8026A678(self, 0xE);
                    } else {
                        fn_8026A678(self, 7);
                    }
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 0xF);
                } else if (pl_stick_window_gated_ck(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 0xC);
                    fn_8026A678(self, 0xD);
                } else if (pl_stick_window_gated_ck(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 9);
                    fn_8026A678(self, 0xA);
                } else if (pl_stick_window_gated_ck(self, 1, 0xAAAB, 0xD555) == 1U) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                }
                if (pl_stick_window_level_ck(self, 1, 0x6AAB, 0xAAAB) == 0) {
                    fn_8026A678(self, 0xB);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 2);
                }
                if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                    fn_8026A678(self, 8);
                    return;
                }
            }
            break;
        case 1:
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0) && (pl_part_flag_ck(self, 1) == 0)) {
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xA);
            }
            if (((s32) (st->flagsB8 & 0x200) == 0) || ((s32) (st->unk7C & 1) != 0)) {
                fn_8026A678(self, 0xB);
            }
            if ((s32) (st->flagsBC & 0x100) != 0) {
                fn_8026A678(self, 3);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 2);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            }
            if (fn_8026CBE4(self, 0, 0, 4) == 1U) {
                fn_8026A618(self, 0x11);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (pl_id_bit_ck(self, 0xB) == 0)) {
                fn_8026A678(self, 0x11);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        fn_8026A678(self, 0xD);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 1);
                    }
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 0x12);
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 1);
                fn_8026A678(self, 0x11);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xE);
                fn_8026A678(self, 0xF);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 4);
            }
            if (((s32) (st->flagsBC & 0x200) != 0) && (pl_id_bit_ck(self, 0xB) == 0)) {
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 6);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xC);
                        fn_8026A678(self, 0xD);
                    } else {
                        fn_8026A678(self, 0);
                        fn_8026A678(self, 1);
                    }
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 8);
                return;
            }
            break;
        }
        break;
    case 2:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsB8 & 0x80) != 0) {
                fn_8026A618(self, 0x11);
            } else {
                fn_8026A678(self, 5);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 5);
                }
                if ((s32) (st->unk7E & 9) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 2);
                    fn_8026A618(self, 0xB);
                    fn_8026A618(self, 0xA);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 3);
                    return;
                }
            } else {
                if (pl_stick_window_gated_ck(self, 1, 0xEAAB, 0x9555) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 2);
                    if ((s32) self->field_0x308 == 0) {
                        fn_8026A618(self, 0xB);
                        fn_8026A618(self, 0xA);
                    }
                } else if (pl_stick_window_gated_ck(self, 1, 0x9555, 0xEAAB) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 3);
                }
                if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 5);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 0, 1, 4) == 1U) {
                fn_8026A618(self, 0x11);
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
            } else {
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 6);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 2);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 3);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 3);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 5);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 1);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 1);
                    } else {
                        fn_8026A678(self, 0);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 0);
                fn_8026A678(self, 2);
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 3);
                return;
            }
            break;
        }
        break;
    case 7:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x80) != 0) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 9);
                fn_8026A678(self, 0xD);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 7);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                    if (pl_stick_dir_ck(self, 0x800) == 1U) {
                        fn_8026A678(self, 0x12);
                    } else if (pl_stick_dir_ck(self, 0x400) == 1U) {
                        fn_8026A678(self, 0x11);
                    }
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A618(self, 0xB);
                    fn_8026A618(self, 0xA);
                    fn_8026A678(self, 0xC);
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xC);
                    return;
                }
            } else {
                if (pl_stick_window_gated_ck(self, 1, 0x1555, 0x6AAB) == 1U) {
                    fn_8026A678(self, 0);
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 8);
                    fn_8026A678(self, 0xC);
                    if ((s32) self->field_0x308 == 0) {
                        fn_8026A618(self, 0xB);
                        fn_8026A618(self, 0xA);
                    }
                } else if (pl_stick_window_gated_ck(self, 1, 0xD555, 0x1555) == 1U) {
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x11);
                } else if (pl_stick_window_gated_ck(self, 1, 0x6AAB, 0xAAAB) == 1U) {
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x12);
                } else if (pl_stick_window_gated_ck(self, 1, 0xAAAB, 0xD555) == 1U) {
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 4);
                }
                if ((s32) (st->flagsBC & 0x100) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 5);
                }
                if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                    fn_8026A678(self, 7);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
                fn_8026A678(self, 6);
                fn_8026A678(self, 9);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 0xC);
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 8);
                fn_8026A678(self, 0xE);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 5);
                    fn_8026A678(self, 0xA);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 5);
                        fn_8026A678(self, 0xA);
                    } else {
                        fn_8026A678(self, 3);
                        fn_8026A678(self, 0xB);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 7);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 2);
                    } else {
                        fn_8026A678(self, 0);
                    }
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 3);
                fn_8026A678(self, 8);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0xB);
                fn_8026A678(self, 0xE);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 4);
                if (st->unk54 >= lbl_8079A028) {
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x11);
                    return;
                }
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x12);
                    return;
                }
            }
            break;
        }
        break;
    case 8:
        switch (st->mode) {
        case 2:
            if ((s32) (st->flagsBC & 0x80) != 0) {
                fn_8026A678(self, 0);
                fn_8026A678(self, 5);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 9);
            }
            if ((s32) self->held_attack_0x5C5 != 0) {
                if ((s32) (st->unk7E & 0xB) != 0) {
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0x1F);
                }
                if ((s32) (st->unk7E & 8) != 0) {
                    fn_8026A678(self, 3);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x19);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 1);
                    if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x13);
                }
                if ((s32) (st->unk7E & 1) != 0) {
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x1B);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 0x1D);
                    fn_8026A678(self, 1);
                    if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x13);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x16);
                    if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                    }
                }
                if ((s32) (st->unk7E & 2) != 0) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0x17);
                    fn_8026A678(self, 0x18);
                    fn_8026A678(self, 0x14);
                    fn_8026A678(self, 0x15);
                }
                if ((s32) (st->flagsBC & 0x8000) != 0) {
                    fn_8026A678(self, 6);
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 4);
                    fn_8026A678(self, 0x13);
                    if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                        return;
                    }
                }
            } else {
                if (pl_stick_window_gated_ck(self, 1, 0x2000, 0x6000) == 1U) {
                    fn_8026A678(self, 0xE);
                    fn_8026A678(self, 0xF);
                    fn_8026A678(self, 0x1C);
                    fn_8026A678(self, 0x1D);
                    fn_8026A678(self, 1);
                    fn_8026A678(self, 0xA);
                    fn_8026A678(self, 0x11);
                    fn_8026A678(self, 0x12);
                    fn_8026A678(self, 0x13);
                    if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                        fn_8026A678(self, 0xC);
                    }
                    fn_8026A678(self, 7);
                    fn_8026A678(self, 0x16);
                    if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                        fn_8026A678(self, 8);
                    }
                } else {
                    if ((pl_stick_window_gated_ck(self, 1, 0xE000, 0x2000) == 1U) || (pl_stick_window_gated_ck(self, 1, 0x6000, 0xA000) == 1U)) {
                        fn_8026A678(self, 2);
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 3);
                        fn_8026A678(self, 0x10);
                        fn_8026A678(self, 0x19);
                        fn_8026A678(self, 0x1C);
                    } else if (pl_stick_window_gated_ck(self, 1, 0xA000, 0x2000) == 1U) {
                        fn_8026A678(self, 0x1B);
                        fn_8026A678(self, 0x1C);
                        fn_8026A678(self, 4);
                        fn_8026A678(self, 0x13);
                        fn_8026A678(self, 0x17);
                        fn_8026A678(self, 0x14);
                        fn_8026A678(self, 0x15);
                        fn_8026A678(self, 0x18);
                        if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                            fn_8026A678(self, 0xC);
                        }
                    }
                    if ((s32) (st->flagsBC & 0x100) != 0) {
                        fn_8026A678(self, 6);
                        fn_8026A678(self, 7);
                        if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                            fn_8026A678(self, 8);
                        }
                    }
                    if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                        fn_8026A678(self, 7);
                        if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                            fn_8026A678(self, 8);
                        }
                    }
                }
                if ((s32) (self->field_0x0CC & 0x3C) != 0) {
                    fn_8026A678(self, 0xD);
                    fn_8026A678(self, 0x1F);
                    return;
                }
            }
            break;
        case 1:
            if (fn_8026CBE4(self, 1, 1, 4) == 1U) {
                fn_8026A618(self, 0xF);
                fn_8026A618(self, 0x12);
                fn_8026A618(self, 0x1C);
                fn_8026A678(self, 0);
                fn_8026A678(self, 5);
            }
            if ((st->unk58 <= lbl_8079A020) && ((s32) (st->flagsBC & 0x200) != 0)) {
                if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                    fn_8026A678(self, 0xC);
                }
                fn_8026A678(self, 0x14);
                fn_8026A678(self, 0x15);
            }
            if ((s32) (st->flagsBC & 0x100) != 0) {
                fn_8026A678(self, 9);
            }
            if ((s32) (st->flagsBC & 0x40) != 0) {
                fn_8026A678(self, 6);
                fn_8026A678(self, 7);
                if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 4);
                fn_8026A678(self, 0x13);
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 0x1F);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0x12);
                fn_8026A678(self, 0x13);
                if (st->unk58 > lbl_8079A020) {
                    fn_8026A678(self, 0xF);
                }
                fn_8026A678(self, 0x1C);
                fn_8026A678(self, 7);
                if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                fn_8026A678(self, 0x16);
                fn_8026A678(self, 0x1D);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 0xB);
                    fn_8026A678(self, 0x10);
                    fn_8026A678(self, 0x19);
                } else {
                    if (st->unk54 >= lbl_8079A028) {
                        fn_8026A678(self, 0xB);
                        fn_8026A678(self, 0x18);
                        fn_8026A678(self, 0x1A);
                    }
                }
            }
            if ((s32) (st->flagsBC & 0x200) != 0) {
                if (st->unk58 > lbl_8079A020) {
                    fn_8026A678(self, 0xE);
                }
                fn_8026A678(self, 0x1B);
                fn_8026A678(self, 1);
                fn_8026A678(self, 0x11);
                if (st->unk54 <= lbl_8079A024) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 3);
                    return;
                }
                if (st->unk54 >= lbl_8079A028) {
                    fn_8026A678(self, 2);
                    fn_8026A678(self, 0x17);
                    return;
                }
            } else if (fn_8026B99C(self) == 1U) {
                fn_8026A678(self, 1);
                fn_8026A678(self, 0xA);
                fn_8026A678(self, 0x13);
                fn_8026A678(self, 0x17);
                fn_8026A678(self, 0x18);
                fn_8026A678(self, 0x10);
                fn_8026A678(self, 0xD);
                fn_8026A678(self, 0x1F);
                fn_8026A678(self, 7);
                fn_8026A678(self, 0x1B);
                fn_8026A678(self, 0x1C);
                if (pl_stick_dir_ck(self, 0x1000) == 1U) {
                    fn_8026A678(self, 8);
                }
                if ((s32) pl_act_param_tier_ck(self, 0) == 0) {
                    fn_8026A678(self, 0xC);
                }
            }
            break;
        }
        break;
    }
}

/* 0x8026F7B4: resets the action-state region, or starts a new action from the current area/state. */
extern "C" void fn_8026F7B4(_PLW* self)
{
    u32 value = 0;

    if (Pl_master_ck(self) == 0) {
        memset((u8*)self + 184, 0, 132);
    } else {
        if (PlayMode_ck() == 2) {
            value = self->chunk_ofs;
        }
        pl_input_refresh(self, value);
    }
}

/* 0x8026F828 */
extern "C" void pl_input_clear(_PLW* self)
{
    if (Pl_master_ck(self) == 0 || lobby_input_locked_ck() == 1) {
        memset((u8*)self + 184, 0, 132);
    } else {
        pl_input_refresh(self, 0);
    }
}

#pragma peephole off

/* 0x8026F888: true while the actor has an action-state flag set. */
extern "C" u32 pl_input_active_ck(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if ((self->req_bits_b_0x0BC & 0x3FF) != 0) {
        return 1;
    }
    if ((self->field_0x0CC & 0x3C3C) != 0) {
        return 1;
    }
    return (self->unk134 & 3) != 0;
}

#pragma peephole reset

/* 0x8026F908: maps a part's motion value to an attack level (3/1/0), 0 for the "gun" weapon classes. */
extern "C" u8 pl_act_param_tier_ck(_PLW* self, u32 idx)
{
    u32 level = 0;
    s16 value = (s16)self->part_tbl_b_0xD4[idx];
    /* Dead copies, load-bearing for the allocator: retail's colouring needs the class web to be born
     * after a chain of copies of it (see the unit header). */
    u32 classCopy0 = self->field_0x002;
    u32 classCopy1 = classCopy0;
    u32 classCopy2 = classCopy1;
    u32 weaponClass = self->field_0x002;
    s32 high;
    s32 mid;
    s32 low;

    if ((u32)(weaponClass - 4) <= 2 && self->unk18 == 1 && self->unk5E6 == 1) {
        return 0;
    }
    if (self->unk128 == 2) {
        high = 110;
        mid = 90;
        low = 50;
    } else {
        high = 100;
        mid = 80;
        low = 40;
    }
    if (value >= high) {
        level = 3;
    } else if (value >= mid) {
        level = 1;
    } else if (value >= low) {
        level = 1;
    }
    return level;
}

#pragma peephole off

/* 0x8026F9A4: an attack-level gate plus the part's angle window. */
extern "C" u32 pl_stick_window_gated_ck(_PLW* self, u32 idx, u16 low, u16 high)
{
    u16 flags = self->field_0x0CC;

    if (idx == 0) {
        if ((flags & 0x3C00) == 0) {
            return 0;
        }
    } else {
        if ((flags & 0x3C) == 0) {
            return 0;
        }
    }
    if (pl_act_param_tier_ck(self, idx) >= 1 && (u16)(self->part_tbl_a_0xD0[idx] - low) <= (u16)(high - low)) {
        return 1;
    }
    return 0;
}

#pragma peephole reset

/* 0x8026FA6C: the same window test, relative to the action's start frame. */
extern "C" u32 fn_8026FA6C(_PLW* self, u32 idx, u16 low, u16 high)
{
    u16 offset = 0;

    if (self->field_0x129 != 0) {
        offset = (u16)(self->field_0x0A8 - get_camera_rot_y());
    }
    if (self->part_tbl_b_0xD4[idx] >= 50) {
        u16 value = (u16)(self->part_tbl_a_0xD0[idx] - offset);
        if ((u16)(value - low) <= (u16)(high - low)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8026FB20: picks the angle window for a direction code. */
extern "C" u32 pl_stick_dir_ck(_PLW* self, s32 kind)
{
    switch (kind) {
    case 8192:
        return fn_8026FA6C(self, 0, 8192, 24576);
    case 1024:
        return fn_8026FA6C(self, 0, -8192, 8192);
    case 2048:
        return fn_8026FA6C(self, 0, 24576, -24576);
    case 4096:
        return fn_8026FA6C(self, 0, -24576, -8192);
    case 32:
        return fn_8026FA6C(self, 1, 8192, 24576);
    case 4:
        return fn_8026FA6C(self, 1, -8192, 8192);
    case 8:
        return fn_8026FA6C(self, 1, 24576, -24576);
    case 16:
        return fn_8026FA6C(self, 1, -24576, -8192);
    }
    return 0;
}

/* 0x8026FC40 */
extern "C" u32 pl_stick_window_level_ck(_PLW* self, u32 idx, u16 low, u16 high)
{
    if (pl_act_param_tier_ck(self, idx) >= 1 && (u16)(self->part_tbl_a_0xD0[idx] - low) <= (u16)(high - low)) {
        return 1;
    }
    return 0;
}

/* 0x8026FCCC: the angle window test without the attack-level gate. */
extern "C" u32 pl_stick_window_ck(_PLW* self, u32 idx, u16 low, u16 high)
{
    if (self->part_tbl_b_0xD4[idx] >= 50) {
        if ((u16)(self->part_tbl_a_0xD0[idx] - low) <= (u16)(high - low)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8026FD0C: hands the actor's transform to its physics sub-object. */
extern "C" void pl_model_pos_sync(_PLW* self)
{
    Vec3 vec;
    u32* p;

    VEC3_ctor(&vec);
    p = (u32*)&((PlJointHolder*)self->physics_0x13C)->chr_0x04;
    p[10] = self->param_0x54;
    p[11] = self->field_0x058;
    p[12] = self->rot_z_0x5C;
    copyVec3((nw4r::math::VEC3*)&p[1], (const nw4r::math::VEC3*)((u8*)self + 60));
    vec.x = self->field_0x068;
    vec.y = self->unk6C;
    vec.z = self->unk70;
    mhchar_ofs_set(p, &vec);
}

/* 0x8026FD94: true when the actor is in the area the player is currently in. */
extern "C" u32 pl_now_area_ck(_PLW* self)
{
    return get_now_areano() == self->area_0x16;
}
} /* namespace s_8026BA1C */


/* 0x8026FDD4: master gate - true while the actor's Pl_master is the one in charge. */
u32 Pl_master_ck(_PLW* self)
{
    using s_8026BA1C::GameMode_ck; using s_8026BA1C::my_player_no;
    if (PlayMode_ck() == 2) {
        return 1;
    }
    if (GameMode_ck() == 3) {
        return 1;
    }
    return my_player_no() == self->chunk_ofs;
}
namespace s_8026BA1C {


/* 0x8026FE44: true for the "gun" weapon classes. */
extern "C" u32 pl_item_page2_ck(_PLW* self)
{
    return (u32)(self->field_0x002 - 4) <= 2;
}

#pragma peephole off
} /* namespace s_8026BA1C */


/* 0x8026FE68 */
u32 Pl_act_ck(_PLW* self, u8 action, u16 step)
{
    if (self->field_0x00A == action && self->act_no == step) {
        return 1;
    }
    return 0;
}
namespace s_8026BA1C {


/* 0x8026FE98: tests one bit of the two status words; the mask's top bit picks the word. */
extern "C" u32 pl_status_bit_ck(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        return self->field_0x35C & mask;
    }
    return self->unk360 & (mask & 0x7FFFFFFF);
}

/* 0x8026FEC0 */
extern "C" void pl_act_set_flag(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        self->field_0x35C |= mask;
    } else {
        self->unk360 |= mask & 0x7FFFFFFF;
    }
}

/* 0x8026FEF0 */
extern "C" void pl_cond_flags_clear(_PLW* self, u32 mask)
{
    if ((mask & 0x80000000) == 0) {
        self->field_0x35C &= ~mask;
    } else {
        self->unk360 &= ~(mask & 0x7FFFFFFF);
    }
}

#pragma peephole reset

/* 0x8026FF20: the attack bonus the actor's active skills grant. */
extern "C" s32 fn_8026FF20(_PLW* self)
{
    if (Pl_Skill_ck(self, 13) == 1) {
        return 20;
    }
    if (Pl_Skill_ck(self, 14) == 1) {
        return 50;
    }
    if (Pl_Skill_ck(self, 15) == 1) {
        return -10;
    }
    return Pl_Skill_ck(self, 16) == 1 ? -30 : 0;
}

} /* namespace s_8026BA1C */

/* ==== 0x8026FFBC-0x80270018: the low-health gate ==== */

#include "types.h"
#include "pl.h"
namespace s_8026FFBC {


extern "C" {
extern f32 lbl_8079A02C;
}

/* Whether the actor's health is at or below the 0.4 threshold (the low-health gate the skill code
 * arms its attack modifiers with). */
extern "C" BOOL fn_8026FFBC(_PLW* self)
{
    return (f32)self->health / (f32)self->health_max <= lbl_8079A02C;
}

} /* namespace s_8026FFBC */

/* ==== 0x80270018-0x80273B14: the skill and item layer (optimization level 4) ==== */
#include "Pl/pl_act.h"
#include "Pl/pl_act_step.h" /* pl_item_slot_cycle (the owner's header, rule 2) */
#pragma optimization_level 4

#include "types.h"
namespace s_80270018 {


extern "C" {
extern f32 pl_act_float_zero;
extern f32 pl_act_float_one;
extern f32 pl_act_float_10;
extern f32 lbl_8079A030;
extern f32 lbl_8079A034;
extern f32 lbl_8079A038;
extern f32 lbl_8079A03C;
extern f32 lbl_8079A040;
extern f32 lbl_8079A044;
extern f32 lbl_8079A050;
extern f32 lbl_8079A054;
extern f32 lbl_8079A058;
extern f32 lbl_8079A05C;
extern f32 lbl_8079A060;
extern f32 lbl_8079A064;
extern f32 lbl_8079A068;
extern f32 lbl_8079A06C;
extern f32 lbl_8079A070;
extern f32 lbl_8079A074;
extern f32 lbl_8079A078;
extern f32 lbl_8079A07C;
}


extern "C" {
void equip_record_copy(_EQUIP*, _EQUIP*);
u32 fn_8026FFBC(_PLW*);
u32 fn_80273ED8(_PLW*, int, s8);

int fn_802753E4(_PLW*, u16);
void pl_equip_refresh(_PLW*);
u32 pl_item_page2_ck(_PLW*);
void pl_skill_update(_PLW*, int, int);
void pl_skill_update2(_PLW*, int);
extern u32 lbl_805C5FC8[];
extern u32 lbl_805C5FE0[];
u8 GameMode_ck(void);
u32 lb_smith_deco_page_ck(void);
int pl_weapon_guard_ck(int);
u32 weapon_class_index(u8);
int Pl_item_timer_get(_PLW*, u16);
u32 get_item_species_data(u16);
u32 weapon_kind_slot_index(u8);
void equip_item_id_clamp(_EQUIP*);
s32 pl_equip_model_id_get(_PLW*, _EQUIP*);
u32 pl_item_icon_kind_get(u16);
void equip_columns_skills_get(_EQUIP*, u16*, s8*, u8*);
void* memset(void*, int, u32);
u16 pl_item_slot_next_get(_PLW*, u16, int);
void pl_held_item_refresh(_PLW*);
u8* GetItemData__FUs(u16);
u16 pl_item_slot_find(_PLW*, u16);
s32 item_pair_clear(s16*);

void pl_parts_refresh(_PLW*, s32);


void* equip_slot_rec_get(void);

u32 fn_80274AEC(u8*, u32, u8);

s16 item_take(u16, s16, void*, int, int, int);
u32 pl_obj_ready_ck(void*);
u32 fn_802693C4(u8, int, s32);
u32 pl_parts_set(u8, int, s32);
u32 pl_weapon_parts_refresh(s32, _EQUIP*, _EQUIP*);
void pl_equip_color_apply(_PLW*, u8);
void fn_80272B10(_PLW*, s32);
s8 equip_columns_tree_points_get(u8*, u8);
void fn_802736A0(_PLW*);


extern u8 lbl_80792140;
extern u8 lbl_80792148;
}
} /* namespace s_80270018 */


/* One record of the skill-id table at `lbl_805C01C8`. */
/* size: 0x10 */
struct _SKILLREC {
    /* 0x0 */ u16 id;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u8 unk6;
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 unkD;
    /* 0xE */ u8 unkE;
    /* 0xF */ u8 unkF;
};

/* One equipment/decoration record; only the skill-id/skill-level pairs are named. */
/* size: 0xC */
struct _SKILLITEM {
    /* 0x0 */ u8 unk0[8];
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
};

u32 Pl_cat_skill_ck(_PLW*, u16);
namespace s_80270018 {


/* Sums the player's skill deltas into a display value and writes the rank flag to `out`. */
extern "C" u16 fn_80270018(_PLW* plw, u32 param, u8* out) {
    int value = (u16)param;

    *out = 0;
    if (Pl_master_ck(plw) == 1) {
        if (Pl_item_timer_get(plw, 596) > 0) {
            value += 6;
            *out = 1;
        }
        if (Pl_item_timer_get(plw, 597) > 0) {
            value += 9;
            *out = 1;
        }
        value += plw->unk448;
        value += plw->field_0x449;
        if (Pl_Skill_ck(plw, 74) == 1) {
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 75) == 1) {
            value += 15;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 76) == 1) {
            value += 20;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 77) == 1) {
            value += -5;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 78) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 79) == 1) {
            value += -15;
            *out = 2;
        }
        if (plw->field_0x370 <= 10 && Pl_cat_skill_ck(plw, 18) == 1) {
            value = (int)(lbl_8079A030 * value);
            *out = 1;
        } else if (fn_8026FFBC(plw) == 1) {
            if (Pl_Skill_ck(plw, 165) == 1) {
                value = (int)(lbl_8079A034 * value);
                *out = 2;
            } else if (Pl_Skill_ck(plw, 164) == 1) {
                value = (int)(lbl_8079A038 * value);
                *out = 1;
            }
        }
        if (Pl_Skill_ck(plw, 202) == 1) {
            if (plw->unk446 >= 2) {
                value = (int)(lbl_8079A03C * value);
            } else if (plw->unk446 >= 1) {
                value = (int)(lbl_8079A040 * value);
            }
        }
        if (value <= 0) {
            value = 1;
        }
        if (*out != 0) {
            if ((u16)value >= (u16)param) {
                *out = 1;
            } else {
                *out = 2;
            }
        }
    }
    return (u16)value;
}

/* Recomputes the player's skill point total and the fraction of its 700 point cap. */
extern "C" void pl_gauge_step_a(_PLW* plw) {
    if (Pl_master_ck(plw) == 1) {
        u8 flag;
        int v = fn_80273ED8(plw, 6, 0);

        plw->skill_point_0x3B8 = fn_80270018(plw, (u16)v, &flag);
    }
    if (plw->skill_point_0x3B8 >= 700) {
        plw->skill_point_0x3B8 = 700;
    }
    plw->skill_rate_0x3BC = (f32)plw->skill_point_0x3B8 / lbl_8079A044;
}

/* Recomputes the second skill set's point total and its rank. */
extern "C" u16 fn_802703F4(_PLW* plw, u32 param, u8* out) {
    int value = (u16)param;

    *out = 0;
    if (Pl_master_ck(plw) == 1) {
        if (Pl_item_timer_get(plw, 598) > 0) {
            value += 8;
            *out = 1;
        }
        if (Pl_item_timer_get(plw, 599) > 0) {
            value += 12;
            *out = 1;
        }
        value += plw->unk44C;
        value += plw->buff_b_level_0x44D;
        if (Pl_Skill_ck(plw, 80) == 1) {
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 81) == 1) {
            value = (int)(lbl_8079A050 * value);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 82) == 1) {
            value = (int)(lbl_8079A040 * value);
            value += 10;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 83) == 1) {
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 84) == 1) {
            value = (int)(lbl_8079A054 * value);
            value += -10;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 85) == 1) {
            value = (int)(lbl_8079A058 * value);
            value += -10;
            *out = 2;
        }
        if (fn_8026FFBC(plw) == 1) {
            *out = 1;
            if (Pl_Skill_ck(plw, 165) == 1) {
                value += 21;
            } else if (Pl_Skill_ck(plw, 163) == 1 || Pl_Skill_ck(plw, 164) == 1) {
                value += 45;
            } else {
                value += 30;
            }
        }
        if (plw->field_0x370 <= 10 && Pl_cat_skill_ck(plw, 18) == 1) {
            value = (int)(lbl_8079A05C * value);
            *out = 1;
        }
        if (value <= 0) {
            value = 1;
        }
        if (plw->unk422 > 0) {
            value -= value / 5;
            if (value < 1) {
                value = 1;
            }
            *out = 2;
        }
    }
    return (u16)value;
}

/* Recomputes the second skill set's point meter, clamped to a minimum of ten points. */
extern "C" void pl_gauge_step_b(_PLW* plw) {
    if (Pl_master_ck(plw) == 1) {
        u8 flag;
        int v = fn_80273ED8(plw, 0, 0) + 1;

        plw->unk3BA = fn_802703F4(plw, (u16)v, &flag);
    }
    plw->defense_0x3C0 = (f32)plw->unk3BA;
    if (plw->defense_0x3C0 < pl_act_float_one) {
        plw->defense_0x3C0 = pl_act_float_one;
    }
}

/*
 * Applies one weapon-type skill group to one attack-bonus field: the +20/+15/+10 levels of the group's
 * three positive skills, then the -15/-10 levels of its two negative ones.
 */
#define SKILL_GROUP(field, base)                                 if (Pl_Skill_ck(plw, base) == 1) {                               plw->field += lbl_8079A060;                                     } else if (Pl_Skill_ck(plw, base - 1) == 1) {                    plw->field += lbl_8079A064;                                     } else if (Pl_Skill_ck(plw, base - 2) == 1) {                    plw->field += pl_act_float_10;                                     }                                                            if (Pl_Skill_ck(plw, base + 2) == 1) {                           plw->field -= lbl_8079A064;                                     } else if (Pl_Skill_ck(plw, base + 1) == 1) {                    plw->field -= pl_act_float_10;                                     }

/* Recomputes the five weapon-type attack-bonus fields from the player's active skills. */
extern "C" void fn_802707B4(_PLW* plw) {
    SKILL_GROUP(elem_resist_0x3C4, 96)
    SKILL_GROUP(elem_resist_0x3C8, 102)
    SKILL_GROUP(elem_resist_0x3CC, 108)
    SKILL_GROUP(elem_resist_0x3D0, 120)
    SKILL_GROUP(elem_resist_0x3D4, 114)
}

/* Clamps the five attack-bonus fields to the +-99 range the display and the status code use. */
extern "C" void fn_80270B98(_PLW* plw) {
    if (plw->elem_resist_0x3C4 < lbl_8079A068) {
        plw->elem_resist_0x3C4 = lbl_8079A068;
    }
    if (plw->elem_resist_0x3C8 < lbl_8079A068) {
        plw->elem_resist_0x3C8 = lbl_8079A068;
    }
    if (plw->elem_resist_0x3CC < lbl_8079A068) {
        plw->elem_resist_0x3CC = lbl_8079A068;
    }
    if (plw->elem_resist_0x3D0 < lbl_8079A068) {
        plw->elem_resist_0x3D0 = lbl_8079A068;
    }
    if (plw->elem_resist_0x3D4 < lbl_8079A068) {
        plw->elem_resist_0x3D4 = lbl_8079A068;
    }
    if (plw->elem_resist_0x3C4 > lbl_8079A06C) {
        plw->elem_resist_0x3C4 = lbl_8079A06C;
    }
    if (plw->elem_resist_0x3C8 > lbl_8079A06C) {
        plw->elem_resist_0x3C8 = lbl_8079A06C;
    }
    if (plw->elem_resist_0x3CC > lbl_8079A06C) {
        plw->elem_resist_0x3CC = lbl_8079A06C;
    }
    if (plw->elem_resist_0x3D0 > lbl_8079A06C) {
        plw->elem_resist_0x3D0 = lbl_8079A06C;
    }
    if (plw->elem_resist_0x3D4 > lbl_8079A06C) {
        plw->elem_resist_0x3D4 = lbl_8079A06C;
    }
}

/* The defence delta of one armour-piece slot: -1 for a negative skill, +1 for a positive one. */
extern "C" f32 fn_80270C64(s16* table, s16 slot) {
    f32 delta = pl_act_float_zero;

    if (((s16*)((u8*)table + 0x42E))[slot] > 0) {
        delta -= lbl_8079A070;
    }
    if (((s16*)((u8*)table + 0x438))[slot] > 0) {
        delta += lbl_8079A074;
    }
    return delta;
}

/* Rebuilds the five attack-bonus fields from the player's armour skills and the weapon's own bonuses. */
extern "C" void pl_condition_step(_PLW* plw) {
    plw->elem_resist_0x3C4 = (f32)(s16)fn_80273ED8(plw, 1, 0) + fn_80270C64((s16*)plw, 0);
    plw->elem_resist_0x3C8 = (f32)(s16)fn_80273ED8(plw, 2, 0) + fn_80270C64((s16*)plw, 1);
    plw->elem_resist_0x3CC = (f32)(s16)fn_80273ED8(plw, 3, 0) + fn_80270C64((s16*)plw, 2);
    plw->elem_resist_0x3D0 = (f32)(s16)fn_80273ED8(plw, 4, 0) + fn_80270C64((s16*)plw, 4);
    plw->elem_resist_0x3D4 = (f32)(s16)fn_80273ED8(plw, 5, 0) + fn_80270C64((s16*)plw, 3);
    fn_802707B4(plw);
    plw->elem_resist_0x3C4 += (f32)(s16)fn_802753E4(plw, 6);
    plw->elem_resist_0x3C8 += (f32)(s16)fn_802753E4(plw, 7);
    plw->elem_resist_0x3CC += (f32)(s16)fn_802753E4(plw, 8);
    plw->elem_resist_0x3D0 += (f32)(s16)fn_802753E4(plw, 10);
    plw->elem_resist_0x3D4 += (f32)(s16)fn_802753E4(plw, 9);
    if (Pl_condition_ck(plw, 256) == 1) {
        plw->elem_resist_0x3D4 -= lbl_8079A070;
    }
    fn_80270B98(plw);
}

/* Swaps the player's nine equipment slots for another set, re-derives every skill field and fills the status
 * screen's skill summary; a null equipment set only fills the summary. */
extern "C" void pl_status_sheet_get(_PLW* plw, _EQUIP* equip, u8* out) {
    _EQUIP saved[9];
    int i;

    if (equip != 0) {
        equip_record_copy(&saved[0], &plw->equipA[0]);
        equip_record_copy(&saved[1], &plw->equipA[1]);
        equip_record_copy(&saved[2], &plw->equipA[2]);
        equip_record_copy(&saved[3], &plw->equipA[3]);
        equip_record_copy(&saved[4], &plw->equipA[4]);
        equip_record_copy(&saved[5], &plw->equipA[5]);
        equip_record_copy(&saved[6], &plw->equipB);
        equip_record_copy(&saved[7], &plw->equipC);
        equip_record_copy(&saved[8], &plw->equipD);
        equip_record_copy(&plw->equipA[0], &equip[0]);
        equip_record_copy(&plw->equipA[1], &equip[1]);
        equip_record_copy(&plw->equipA[2], &equip[2]);
        equip_record_copy(&plw->equipA[3], &equip[3]);
        equip_record_copy(&plw->equipA[4], &equip[4]);
        equip_record_copy(&plw->equipA[5], &equip[5]);
        equip_record_copy(&plw->equipB, &equip[6]);
        equip_record_copy(&plw->equipC, &equip[7]);
        equip_record_copy(&plw->equipD, &equip[8]);
        pl_equip_refresh(plw);
        for (int i = 0; i < 8; i++) {
            plw->shown_skill_id[i] = plw->active_skill_id[i];
        }
        pl_skill_update(plw, 0, 0);
        pl_skill_update2(plw, 0);
        pl_gauge_step_a(plw);
        pl_gauge_step_b(plw);
        pl_condition_step(plw);
    }
    out[10] = 0;
    *(u16*)(out + 12) = 0;
    for (i = 1; i < 6; i++) {
        s16 v = (s16)fn_80273ED8(plw, lbl_805C5FC8[i], 0);

        if (v > 0) {
            out[10] = (u8)i;
            *(s16*)(out + 12) = v;
            break;
        }
    }
    out[11] = 0;
    *(u16*)(out + 14) = 0;
    for (i = 0; i < 3; i++) {
        s16 v = (s16)fn_80273ED8(plw, lbl_805C5FE0[i], 0);

        if (v > 0) {
            out[11] = (u8)(i + 12);
            *(s16*)(out + 14) = v;
            break;
        }
    }
    *(s16*)(out + 0) = plw->unk372;
    *(s16*)(out + 2) = plw->unk37A;
    *(s16*)(out + 4) = plw->unk380;
    *(u16*)(out + 6) = plw->skill_point_0x3B8;
    *(u16*)(out + 8) = plw->unk3BA;
    *(s16*)(out + 16) = (s16)plw->elem_resist_0x3C4;
    *(s16*)(out + 18) = (s16)plw->elem_resist_0x3C8;
    *(s16*)(out + 20) = (s16)plw->elem_resist_0x3CC;
    *(s16*)(out + 24) = (s16)plw->elem_resist_0x3D0;
    *(s16*)(out + 22) = (s16)plw->elem_resist_0x3D4;
    for (i = 0; i < 7; i++) {
        out[26 + i] = 0;
    }
    if (equip != 0) {
        equip_record_copy(&plw->equipA[0], &saved[0]);
        equip_record_copy(&plw->equipA[1], &saved[1]);
        equip_record_copy(&plw->equipA[2], &saved[2]);
        equip_record_copy(&plw->equipA[3], &saved[3]);
        equip_record_copy(&plw->equipA[4], &saved[4]);
        equip_record_copy(&plw->equipA[5], &saved[5]);
        equip_record_copy(&plw->equipB, &saved[6]);
        equip_record_copy(&plw->equipC, &saved[7]);
        equip_record_copy(&plw->equipD, &saved[8]);
        pl_equip_refresh(plw);
        for (int i = 0; i < 8; i++) {
            plw->shown_skill_id[i] = 0;
        }
        pl_skill_update(plw, 0, 0);
        pl_skill_update2(plw, 0);
        pl_gauge_step_a(plw);
        pl_gauge_step_b(plw);
        pl_condition_step(plw);
    }
}
} /* namespace s_80270018 */


/* Whether the player's active skill set contains `skill`. */
u32 Pl_Skill_ck(_PLW* plw, u16 skill) {
    using s_80270018::GameMode_ck; using s_80270018::lb_smith_deco_page_ck; 
    bool ok = false;
    int i;

    if (GameMode_ck() == 2) {
        if (lobby_w.state_0x000 == 6) {
            ok = true;
        } else if (lobby_w.state_0x000 == 15 && lb_smith_deco_page_ck() == 1) {
            ok = true;
        }
    }
    if (ok == 1) {
        for (int i = 0; i < 8; i++) {
            if (skill == plw->shown_skill_id[i]) {
                return true;
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (skill == plw->active_skill_id[i]) {
                return true;
            }
        }
    }
    return false;
}
namespace s_80270018 {


/* Whether `skill` is one of the player's eight base skill ids. */
extern "C" u32 pl_skill_have_ck(_PLW* plw, u16 skill) {
    int i;

    for (i = 0; i < 8; i++) {
        if (skill == plw->active_skill_id[i]) {
            return 1;
        }
    }
    return 0;
}

/* The level of the skill in one slot: the live set in the lobby menu, the base set otherwise. */
extern "C" u32 Pl_Skill_slot_item_get(_PLW* plw, u32 slot) {
    int i;

    if (GameMode_ck() == 2 && lobby_w.state_0x000 == 6) {
        for (int i = 0; i < 8; i++) {
            if ((u8)slot == plw->shown_skill_tree[i]) {
                return (u8)plw->shown_skill_id[i];
            }
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if ((u8)slot == plw->active_skill_tree[i]) {
                return (u8)plw->active_skill_id[i];
            }
        }
    }
    return 0;
}

/* Whether the player currently provides `kind` of skill: kind 15-17 ask the equipment, 74 the player
 * type, and anything else the two skill-id tables. */
extern "C" u32 pl_skill_valid_ck(_PLW* plw, u8 kind) {
    u16 mode = (pl_item_page2_ck(plw) == 1);

    switch (kind) {
    case 15:
    case 16:
    case 17:
        return (u32)(pl_weapon_guard_ck((int)plw) == 1);
    case 74: {
        int type = plw->field_0x002;

        if ((u32)(type - 7) <= 1 || type == 0 || type == 2) {
            return 1;
        }
        return 0;
    }
    default: {
        u16* table;

        if (mode == 0) {
            table = lbl_805C0198;
        } else {
            table = lbl_805C01B8;
        }
        while (*table != 0xFFFF) {
            if (kind == *table++) {
                return 0;
            }
        }
        return 1;
    }
    }
}

/* Whether the player's active skill set satisfies one skill query: the equipment-provided kinds
 * 7-10/14-15, the level query kinds 11-13, and the 74 player-type special case. */
extern "C" u32 pl_equip_skill_valid_ck(_PLW* plw, u8* p, u32 a, u8 kind) {
    u32 lvl;
    u32 mode;

    if (p == 0) {
        return 0;
    }
    {
        u8 v = *p;

        switch (v) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
            lvl = weapon_class_index(v);
            mode = 0;
            break;
        case 11:
        case 12:
        case 13:
            lvl = 6;
            mode = 1;
            break;
        default:
            return pl_skill_valid_ck(plw, kind);
        }
    }
    switch (kind) {
    case 15:
    case 16:
    case 17:
        return (fn_80274AEC(p, a, lvl) - 1) == 0;
    case 74: {
        u8 l = (u8)lvl;

        if ((u32)(l - 7) <= 1 || (s32)l == 0 || (s32)l == 2) {
            return 1;
        }
        return 0;
    }
    default: {
        u16* table;

        if (mode == 0) {
            table = lbl_805C0198;
        } else {
            table = lbl_805C01B8;
        }
        while (*table != 0xFFFF) {
            if (kind == *table++) {
                return 0;
            }
        }
        return 1;
    }
    }
}

/* Maps a display skill id to its internal id through the 16-byte record table, or 0 when absent. */
extern "C" u8 pl_skill_tree_get(u16 skill) {
    _SKILLREC* rec = (_SKILLREC*)lbl_805C01C8;

    if (skill == 0) {
        return 0;
    }
    while (rec->id != 0xFFFF) {
        if (rec->unk3 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk5 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk7 == skill) {
            return (u8)rec->id;
        }
        if (rec->unk9 == skill) {
            return (u8)rec->id;
        }
        if (rec->unkB == skill) {
            return (u8)rec->id;
        }
        if (rec->unkD == skill) {
            return (u8)rec->id;
        }
        rec++;
    }
    return 0;
}

/* The one-argument form of pl_skill_valid_ck: the skill is resolved through the id table first. */
extern "C" void pl_skill_id_valid_ck(_PLW* plw, u16 skill) {
    pl_skill_valid_ck(plw, pl_skill_tree_get(skill));
}

/* The three-argument form of pl_equip_skill_valid_ck: the skill is resolved through the id table first. */
extern "C" void pl_equip_skill_id_valid_ck(_PLW* plw, u8* p, u32 a, u16 skill) {
    pl_equip_skill_valid_ck(plw, p, a, pl_skill_tree_get(skill));
}

/* Whether a skill is available in the current quest context: three kinds are gated on the
 * quest mode, one on the player type, everything else is always available. */
extern "C" u32 pl_bonus_skill_valid_ck(_PLW* plw, u16 skill) {
    u16 mode = (pl_item_page2_ck(plw) == 1);

    switch (skill) {
    case 15:
    case 30:
        if (mode == 0) {
            return 0;
        }
        break;
    case 19:
    case 38:
    case 50:
        if (mode == 1) {
            return 0;
        }
        break;
    case 40:
        if ((u32)(plw->field_0x002 - 7) <= 1) {
            return 0;
        }
        break;
    default:
        break;
    }
    return 1;
}

/* The skill level an equipment record contributes for `skill`, or 0. */
extern "C" s8 fn_80271AD8(u16 item, u8 skill) {
    _SKILLITEM* rec = (_SKILLITEM*)get_item_species_data(item);
    s8 lvl = 0;

    if (rec != 0) {
        if (rec->unk8 == skill) {
            lvl = (s8)rec->unk9;
        }
        if (rec->unkA == skill) {
            lvl += rec->unkB;
        }
    }
    return lvl;
}

/* Whether an equipment record provides `skill` at a non-zero level. */
extern "C" u32 fn_80271B4C(u16 item, u8 skill) {
    _SKILLITEM* rec = (_SKILLITEM*)get_item_species_data(item);
    u32 ok = 0;

    if (rec != 0) {
        if (rec->unk8 == skill) {
            if ((s8)rec->unk9 != 0) {
                ok = 1;
            }
        }
        if (rec->unkA == skill) {
            if ((s8)rec->unkB != 0) {
                ok = 1;
            }
        }
    }
    return ok;
}

/* The 4-byte slot-table entry at 0x278: an item id and a signed value. */
#define SLOT_IDX(slot) (((slot) & 0x80) ? (((slot) & 0x7F) + 26) : (slot))

/* Appends one (value, a, b) triple to a bounded parallel-array set and bumps the count. */
extern "C" void fn_802724E8(u16* table, s8* a, u8* b, u8* count, u16 v, u8 bval, s8 aval) {
    if (v == 0) {
        return;
    }
    if (*count >= 8) {
        return;
    }
    table[*count] = v;
    a[*count] = aval;
    b[*count] = bval;
    *count += 1;
}

/* The saved-skill set behind pl_equip_refresh: looks each skill id up in the 16-byte record table, keeps the
 * rank the player's equipment reaches, converts it to a display value and sorts the eight entries. */
extern "C" void equip_columns_skills_get(_EQUIP* equips, u16* table, s8* a, u8* b) {
    _SKILLREC* rec = (_SKILLREC*)lbl_805C01C8;
    s16 tv[8];
    u8 tb[8];
    u8 ta[8];
    u8 count = 0;
    s8 level;
    int i;
    int j;

    for (i = 0; i < 8; i++) {
        table[i] = 0;
        b[i] = 0;
        a[i] = 0;
    }
    while (rec->id != 0xFFFF) {
        level = equip_columns_tree_points_get((u8*)equips, (u8)rec->id);
        if (level <= (s8)rec->unk2) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk3, (u8)rec->id, level);
        } else if (level <= (s8)rec->unk4) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk5, (u8)rec->id, level);
        } else if (level <= (s8)rec->unk6) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk7, (u8)rec->id, level);
        } else if (level >= (s8)rec->unkC) {
            fn_802724E8(table, a, b, &count, (u16)rec->unkD, (u8)rec->id, level);
        } else if (level >= (s8)rec->unkA) {
            fn_802724E8(table, a, b, &count, (u16)rec->unkB, (u8)rec->id, level);
        } else if (level >= (s8)rec->unk8) {
            fn_802724E8(table, a, b, &count, (u16)rec->unk9, (u8)rec->id, level);
        }
        rec++;
    }
    for (i = 0; i < 8; i++) {
        s8 v = (s8)a[i];

        if (v != 0) {
            if (v > 0) {
                tv[i] = (s16)(v + 200);
                tb[i] = b[i];
                ta[i] = (u8)table[i];
            } else {
                tv[i] = (s16)((s16)(-v) + 100);
                tb[i] = b[i];
                ta[i] = (u8)table[i];
            }
        } else {
            tv[i] = 0;
            tb[i] = 0;
            ta[i] = 0;
        }
    }
    for (i = 0; i < 7; i++) {
        for (j = i + 1; j < 8; j++) {
            if (tv[i] < tv[j] || (tv[i] == tv[j] && tb[i] < tb[j])) {
                s16 s = tv[i];
                u8 t = tb[i];

                tv[i] = tv[j];
                tb[i] = tb[j];
                tv[j] = s;
                tb[j] = t;
                {
                    u8 u = ta[i];

                    ta[i] = ta[j];
                    ta[j] = u;
                }
            }
        }
    }
    for (i = 0; i < 8; i++) {
        a[i] = (s8)tv[i];
        b[i] = tb[i];
        table[i] = ta[i];
    }
}
} /* namespace s_80270018 */


/* Whether `skill` is one of the four decoration-slot skill ids. */
u32 Pl_cat_skill_ck(_PLW* plw, u16 skill) {
    int i;
    u16* p = plw->deco_skill_id;

    for (i = 0; i < 2; i++) {
        if (skill == p[0]) {
            return 1;
        }
        if (skill == p[1]) {
            return 1;
        }
        p += 2;
    }
    return 0;
}
namespace s_80270018 {


/* The item id in one equipment slot, or 0 for the empty slot. */
extern "C" u16 pl_item_slot_item_get(_PLW* plw, u8 slot) {
    if (slot == 0xFF) {
        return 0;
    }
    if (slot & 0x80) {
        return *(u16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x278);
    }
    return *(u16*)((u8*)plw + slot * 4 + 0x278);
}

/* The signed skill value of one equipment slot, or 20 for the fixed-value item. */
extern "C" s16 pl_item_slot_count_get(_PLW* plw, u8 slot) {
    if (slot == 0xFF) {
        return 0;
    }
    if (pl_item_slot_item_get(plw, slot) == 0x35) {
        return 20;
    }
    if (slot & 0x80) {
        return *(s16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x27A);
    }
    return *(s16*)((u8*)plw + slot * 4 + 0x27A);
}

/* Recomputes the current slot's skill value and lowers the running minimum. */
extern "C" void fn_80272D5C(_PLW* plw) {
    s16 v = pl_item_slot_count_get(plw, (u8)plw->item_slot_sel);

    plw->magazine_0x270 = v;
    plw->magazine_shown_0x272 = (s16)v;
    if ((s16)v < plw->unk269) {
        plw->unk269 = (u8)(s16)v;
        plw->unk274 = (u8)(s16)v;
    }
}

/* The weapon-type slot count for one equipment slot, 2 for the empty slot. */
extern "C" s32 pl_equip_slot_count_get(_PLW* plw, u16 slot) {
    u16 id = pl_item_slot_find(plw, slot);

    if (id != 0xFFFF) {
        if (id & 0x80) {
            return item_pair_clear((s16*)&plw->slot_id[(id & 0x7F) + 26]);
        }
        return item_pair_clear((s16*)&plw->slot_id[id]);
    }
    return 2;
}

/* Clears the four per-set skill summary words. */
extern "C" void pl_push_clear(_PLW* plw) {
    plw->unk634 = 0;
    plw->unk638 = 0;
    plw->unk63C = 0;
    plw->unk640 = 0;
}

/* Removes the equipment set selected by `idx` and clears its valid bit. */
extern "C" void fn_8027373C(_PLW* plw, u8 idx) {
    u32 mask = 1 << idx;

    if ((plw->equip_valid & mask) == 0) {
        return;
    }
    equip_record_copy((_EQUIP*)((u8*)plw + idx * 12 + 0x140), (_EQUIP*)((u8*)plw + idx * 12 + 0x188));
    plw->equip_valid &= (u16)~mask;
}

/* One byte of the seven-entry decoration skill-id table. */
extern "C" u8 fn_802738B8(u8 idx) {
    if (idx >= 7) {
        return 0;
    }
    return (&lbl_80792140)[idx];
}

/* One byte of the second seven-entry decoration table. */
extern "C" u8 fn_802738D8(u8 idx) {
    return (&lbl_80792148)[idx];
}

/* Stores one skill value into a set slot and updates its valid bit. */
extern "C" void pl_parts_color_set(_PLW* plw, u8 idx, s32 val) {
    if (val == plw->set_applied[idx]) {
        plw->set_valid &= (u16)~(1 << idx);
        return;
    }
    plw->set_pending[idx] = val;
    plw->set_valid |= (u16)(1 << idx);
}

/* Copies the player's nine equipment slots into a local save area and rebuilds the skill set from it. */
extern "C" void pl_equip_refresh(_PLW* plw) {
    _EQUIP saved[9];

    equip_record_copy(&saved[0], &plw->equipA[0]);
    equip_record_copy(&saved[1], &plw->equipA[1]);
    equip_record_copy(&saved[2], &plw->equipA[2]);
    equip_record_copy(&saved[3], &plw->equipA[3]);
    equip_record_copy(&saved[4], &plw->equipA[4]);
    equip_record_copy(&saved[5], &plw->equipA[5]);
    equip_record_copy(&saved[6], &plw->equipB);
    equip_record_copy(&saved[7], &plw->equipC);
    equip_record_copy(&saved[8], &plw->equipD);
    equip_columns_skills_get(&saved[0], plw->active_skill_id, (s8*)plw->unk60A, plw->active_skill_tree);
}

/* Re-selects the skill slot: keeps the current one when it still holds the same skill id, otherwise
 * falls back through the resolved-slot table and drops it when the item is not a valid one. */
extern "C" void fn_80272B10(_PLW* plw, s32 slot) {
    u16* ent;
    u32 v;

    if (slot & 0x80) {
        ent = (u16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x278);
    } else {
        ent = (u16*)((u8*)plw + slot * 4 + 0x278);
    }
    v = pl_item_icon_kind_get(*ent);
    if (slot != plw->item_slot_sel) {
        if (plw->held_item_kind_0x26C != (u8)v) {
            return;
        }
        plw->item_slot_sel = (u16)slot;
    }
    if (slot == plw->item_slot_sel) {
        plw->item_slot_sel = pl_item_slot_next_get(plw, (u16)slot, 2);
        pl_held_item_refresh(plw);
        plw->magazine_shown_0x272 = plw->magazine_0x270;
        plw->capacity_shown_0x275 = plw->ammo_capacity_0x26A;
        plw->unk274 = 0;
        return;
    }
    if (pl_item_page2_ck(plw) != 0) {
        u16 cur = plw->item_slot_sel;

        if (cur == 0xFF) {
            plw->item_slot_sel = pl_item_slot_next_get(plw, 0, 2);
            pl_held_item_refresh(plw);
            return;
        }
        u8* item;

        if (cur & 0x80) {
            item = GetItemData__FUs(plw->slot_id[(cur & 0x7F) + 26].item_id);
        } else {
            item = GetItemData__FUs(plw->slot_id[cur].item_id);
        }
        if (item[0] != 1) {
            plw->item_slot_sel = pl_item_slot_next_get(plw, plw->item_slot_sel, 0);
            pl_held_item_refresh(plw);
        }
    }
}

/* Resolves the equipment slot for a changed slot and re-selects the skill to display. */
extern "C" s16 pl_item_add(_PLW* plw, u16 item, s16 value) {
    u8* data = GetItemData__FUs(item);
    u16 cur = pl_item_slot_find(plw, item);
    s16 v;
    u16 slot;

    for (;;) {
        if (data[0] == 1 && pl_item_page2_ck(plw) == 1 && (cur == 0xFFFF || (cur & 0x80) != 0)) {
            v = item_take(item, value, plw->spare_slot_id, 8, 1, 0);
            slot = pl_item_slot_find(plw, item);
            if ((u32)v <= 4) {
                break;
            }
        }
        v = item_take(item, value, plw->slot_id, 24, 1, 0);
        slot = pl_item_slot_find(plw, item);
        break;
    }
    switch (v) {
    case 0:
        {
            u8* d = GetItemData__FUs(plw->slot_id[plw->field_0x304].item_id);

            if ((d[2] & 8) == 0 || d[0] == 1) {
                plw->field_0x304 = pl_item_slot_cycle(plw, plw->field_0x304, 0);
            }
        }
        fn_80272B10(plw, slot);
        break;
    case 1:
    case 2:
    case 3:
        if (data[0] == 1 && plw->item_slot_sel == slot) {
            fn_80272D5C(plw);
        }
        break;
    case 4:
        if (data[0] == 1) {
            if (pl_item_slot_next_get(plw, plw->item_slot_sel, 1) == 0xFF) {
                plw->item_slot_sel = 0xFF;
                plw->magazine_0x270 = 0;
                plw->ammo_capacity_0x26A = 0;
                plw->unk269 = 0;
            } else if (plw->item_slot_sel == cur) {
                plw->magazine_0x270 = 0;
                plw->ammo_capacity_0x26A = 0;
                plw->unk269 = 0;
            }
            if (plw->item_slot_sel == cur) {
                fn_80272D5C(plw);
            }
        }
        break;
    }
    return v;
}

/* The value one equipment slot contributes to `value`, clamped to the item's own limit. */
extern "C" s32 pl_item_room_get(_PLW* plw, u16 item, s16 value) {
    u16 slot = pl_item_slot_find(plw, item);
    u8* data = GetItemData__FUs(item);
    s16 ent;

    if (slot != 0xFFFF) {
        if (slot & 0x80) {
            ent = *(s16*)((u8*)plw + ((slot & 0x7F) + 26) * 4 + 0x27A);
            if ((s16)value + ent >= data[3]) {
                return data[3] - ent;
            }
            return value;
        }
        ent = *(s16*)((u8*)plw + slot * 4 + 0x27A);
        if ((s16)value + ent >= data[3]) {
            return data[3] - ent;
        }
        return value;
    }
    if (data[0] == 1 && pl_item_page2_ck(plw) == 1) {
        u16* p = (u16*)plw->spare_slot_id;

        for (int i = 0; i < 8; i++) {
            if (p[i * 2] == 0) {
                return value;
            }
        }
    }
    for (int i = 0; i < 24; i++) {
        if (plw->slot_id[i].item_id == 0) {
            return value;
        }
    }
    return 0;
}

/* Flushes the skill sets whose cached values no longer resolve, and the decoration slots. */
extern "C" void pl_parts_refresh(_PLW* plw, s32 arg) {
    u8 i;
    u8 id;
    u32 mask;
    u32 val;
    u32 ok;

    if (!(plw->set_valid != 0 || plw->equip_valid != 0 || plw->deco_dirty != 0)) {
        return;
    }
    ok = pl_obj_ready_ck(pl_global->table + plw->chunk_ofs);
    for (i = 0; i < 7; i++) {
        id = fn_802738B8(i);
        mask = 1 << i;
        if (plw->set_valid & mask) {
            if (ok == 1) {
                if (*(u32*)((u8*)plw->physics_0x13C + i * 0x164 + 0x11C) == 0) {
                    val = fn_802693C4(plw->chunk_ofs, i, plw->set_pending[i]);
                } else {
                    val = pl_parts_set(plw->chunk_ofs, i, plw->set_pending[i]);
                }
                if (val == 1) {
                    plw->set_valid &= (u16)~mask;
                    plw->set_applied[i] = plw->set_pending[i];
                    if (id != 0xFF) {
                        fn_8027373C(plw, id);
                    }
                }
            }
        } else {
            plw->set_valid &= (u16)~mask;
            if (id != 0xFF) {
                fn_8027373C(plw, id);
                pl_equip_color_apply(plw, id);
            }
        }
    }
    if (plw->deco_dirty != 0 && ok == 1) {
        pl_weapon_parts_refresh(plw->chunk_ofs, &plw->equipB2, plw->equipE);
        fn_802736A0(plw);
    }
}

/* Clears every derived skill array and the three valid-bit words. */
extern "C" void fn_80273484(_PLW* plw) {
    memset((u8*)plw + 0x188, 0, 72);
    memset((u8*)plw + 0x1DC, 0, 12);
    memset((u8*)plw + 0x200, 0, 24);
    memset((u8*)plw + 0x218, -1, 28);
    memset((u8*)plw + 0x234, -1, 28);
    plw->equip_valid = 0;
    plw->set_valid = 0;
    plw->deco_dirty = 0;
}

/* Re-derives the player type after restoring the two decoration equipment slots. */
extern "C" void fn_802736A0(_PLW* plw) {
    int i;

    if (plw->deco_dirty == 0) {
        return;
    }
    equip_record_copy(&plw->equipB, &plw->equipB2);
    for (i = 0; i < 2; i++) {
        equip_record_copy((&plw->equipC) + i, &plw->equipE[i]);
    }
    plw->deco_dirty = 0;
    plw->field_0x002 = ::Get_pl_type(&plw->equipB, &plw->equipC);
}

/* The skill level one player equipment record contributes for `skill`: a per-kind record layout,
 * with the per-decoration levels summed for the kinds that carry them. */
extern "C" s8 equip_skill_points_get(u8* raw, u8 skill) {
    _EQUIP* rec = (_EQUIP*)raw;
    s8 v = 0;
    int i;

    if (rec->item_id == 0 || skill == 0) {
        return 0;
    }
    {
        s32 kind = rec->kind;

        switch ((u32)kind) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            {
                u8* r = (u8*)equip_slot_rec_get();

                if (r[14] == skill) {
                    v = (s8)r[15];
                }
                if (r[16] == skill) {
                    v += r[17];
                }
                if (r[18] == skill) {
                    v += r[19];
                }
                if (r[20] == skill) {
                    v += r[21];
                }
                if (r[22] == skill) {
                    v += r[23];
                }
                for (i = 0; i < 3; i++) {
                    if (rec->skill_id[i] != 0) {
                        v += fn_80271AD8(rec->skill_id[i], skill);
                    }
                }
            }
            break;
        case 6:
            if (rec->deco_count != 0) {
                for (i = 0; i < rec->deco_count; i++) {
                    if (rec->skill_id[i] != 0) {
                        v += fn_80271AD8(rec->skill_id[i], skill);
                    }
                }
            }
            if ((s32)rec->deco_count < 3) {
                if (rec->skill_id[rec->deco_count] == skill) {
                    v += (s8)((s8)(u8)rec->deco_level - 10);
                }
                if ((s32)rec->deco_count + 1 < 3 &&
                    rec->skill_id[rec->deco_count + 1] == skill) {
                    v += (s8)((s8)(u8)((rec->deco_level >> 8) & 0xFF) - 10);
                }
            }
            break;
        default:
            for (i = 0; i < 3; i++) {
                if (rec->skill_id[i] != 0) {
                    v += fn_80271AD8(rec->skill_id[i], skill);
                }
            }
            break;
        }
    }
    return v;
}

/* Scales the skill level of one record by how many of the nine player records carry it. */
extern "C" s8 equip_columns_torso_points_get(u8* rec, u8 skill) {
    s8 v = equip_skill_points_get(rec, skill);

    if (v != 0) {
        s8 n = 1;

        if (equip_skill_points_get(rec + 0x30, 1)) {
            n = 2;
        }
        if (equip_skill_points_get(rec + 0x00, 1)) {
            n++;
        }
        if (equip_skill_points_get(rec + 0x0C, 1)) {
            n++;
        }
        if (equip_skill_points_get(rec + 0x18, 1)) {
            n++;
        }
        if (equip_skill_points_get(rec + 0x24, 1)) {
            n++;
        }
        if (equip_skill_points_get(rec + 0x3C, 1)) {
            n++;
        }
        if (equip_skill_points_get(rec + 0x48, 1)) {
            n++;
        }
        if (rec[0x54] == 12 && equip_skill_points_get(rec + 0x54, 1)) {
            n++;
        }
        if (rec[0x60] == 13 && equip_skill_points_get(rec + 0x60, 1)) {
            n++;
        }
        v = v * n;
    }
    return v;
}

/* Whether one player equipment record provides `skill` at a non-zero level. */
extern "C" u32 equip_skill_tree_has_ck(u8* raw, u8 skill) {
    _EQUIP* rec = (_EQUIP*)raw;
    u32 ok = 0;
    int i;

    if (rec->item_id == 0 || skill == 0) {
        return 0;
    }
    {
        s32 kind = rec->kind;

        switch ((u32)kind) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            {
                u8* r = (u8*)equip_slot_rec_get();

                if (r[14] == skill && (s8)r[15] != 0) {
                    ok = 1;
                }
                if (r[16] == skill && (s8)r[17] != 0) {
                    ok = 1;
                }
                if (r[18] == skill && (s8)r[19] != 0) {
                    ok = 1;
                }
                if (r[20] == skill && (s8)r[21] != 0) {
                    ok = 1;
                }
                if (r[22] == skill && (s8)r[23] != 0) {
                    ok = 1;
                }
                for (i = 0; i < 3; i++) {
                    if (rec->skill_id[i] != 0 && fn_80271B4C(rec->skill_id[i], skill) == 1) {
                        ok = 1;
                    }
                }
            }
            break;
        case 6:
            if (rec->deco_count != 0) {
                for (i = 0; i < rec->deco_count; i++) {
                    if (rec->skill_id[i] != 0 && fn_80271B4C(rec->skill_id[i], skill) == 1) {
                        ok = 1;
                    }
                }
            }
            if ((s32)rec->deco_count < 3) {
                if (rec->skill_id[rec->deco_count] == skill &&
                    (s8)(u8)rec->deco_level - 10 != 0) {
                    ok = 1;
                }
                if ((s32)rec->deco_count + 1 < 3 &&
                    rec->skill_id[rec->deco_count + 1] == skill &&
                    (s8)(u8)((rec->deco_level >> 8) & 0xFF) - 10 != 0) {
                    ok = 1;
                }
            }
            break;
        default:
            for (i = 0; i < 3; i++) {
                if (rec->skill_id[i] != 0 && fn_80271B4C(rec->skill_id[i], skill) == 1) {
                    ok = 1;
                }
            }
            break;
        }
    }
    return ok;
}

/* Whether any of the nine player records provides `skill`. */
extern "C" u32 equip_columns_tree_has_ck(u8* rec, u8 skill) {
    u32 ok = 0;

    if (equip_skill_tree_has_ck(rec + 0x30, skill) == 1) {
        ok = 1;
    }
    if (equip_skill_tree_has_ck(rec + 0x0C, skill) == 1) {
        ok = 1;
    }
    if (equip_skill_tree_has_ck(rec + 0x18, skill) == 1) {
        ok = 1;
    }
    if (equip_skill_tree_has_ck(rec + 0x24, skill) == 1) {
        ok = 1;
    }
    if (equip_skill_tree_has_ck(rec + 0x3C, skill) == 1) {
        ok = 1;
    }
    if (equip_skill_tree_has_ck(rec + 0x48, skill) == 1) {
        ok = 1;
    }
    if (rec[0x54] == 12 && equip_skill_tree_has_ck(rec + 0x54, skill) == 1) {
        ok = 1;
    }
    if (rec[0x60] == 13 && equip_skill_tree_has_ck(rec + 0x60, skill) == 1) {
        ok = 1;
    }
    if (equip_skill_tree_has_ck(rec, skill) == 1) {
        ok = 1;
    }
    return ok;
}

/* Sums the skill level of all nine player records, the main record scaled by its carrier count. */
extern "C" s8 equip_columns_tree_points_get(u8* rec, u8 skill) {
    s8 v = equip_skill_points_get(rec + 0x30, skill);

    v += equip_skill_points_get(rec + 0x0C, skill);
    v += equip_skill_points_get(rec + 0x18, skill);
    v += equip_skill_points_get(rec + 0x24, skill);
    v += equip_skill_points_get(rec + 0x3C, skill);
    v += equip_skill_points_get(rec + 0x48, skill);
    if (rec[0x54] == 12) {
        v += equip_skill_points_get(rec + 0x54, skill);
    }
    if (rec[0x60] == 13) {
        v += equip_skill_points_get(rec + 0x60, skill);
    }
    v += equip_columns_torso_points_get(rec, skill);
    return v;
}

/* Sums the skill level of the nine saved equipment slots. */
extern "C" void pl_skill_tree_points_get(_PLW* plw, u8 skill) {
    _EQUIP saved[9];

    equip_record_copy(&saved[0], &plw->equipA[0]);
    equip_record_copy(&saved[1], &plw->equipA[1]);
    equip_record_copy(&saved[2], &plw->equipA[2]);
    equip_record_copy(&saved[3], &plw->equipA[3]);
    equip_record_copy(&saved[4], &plw->equipA[4]);
    equip_record_copy(&saved[5], &plw->equipA[5]);
    equip_record_copy(&saved[6], &plw->equipB);
    equip_record_copy(&saved[7], &plw->equipC);
    equip_record_copy(&saved[8], &plw->equipD);
    equip_columns_tree_points_get((u8*)&saved[0], skill);
}

/* The signed skill value of the equipment slot `slot`, or 0 for the empty slot. */
extern "C" int Pl_item_timer_get(_PLW* plw, u16 slot) {
    u16 id = pl_item_slot_find(plw, slot);

    if (id != 0xFFFF) {
        if (id & 0x80) {
            return *(s16*)((u8*)plw + ((id & 0x7F) + 26) * 4 + 0x27A);
        }
        return *(s16*)((u8*)plw + id * 4 + 0x27A);
    }
    return 0;
}

/* Stores one equipment record into the saved set selected by its own slot id and marks it valid. */
extern "C" void pl_equip_model_set(_PLW* plw, u8* rec) {
    u8 idx = (u8)weapon_kind_slot_index(rec[0]);

    equip_item_id_clamp((_EQUIP*)rec);
    equip_record_copy((_EQUIP*)((u8*)plw + idx * 12 + 0x188), (_EQUIP*)rec);
    plw->equip_valid |= (u16)(1 << idx);
    idx = fn_802738D8(idx);
    if (idx != 0xFF) {
        pl_parts_color_set(plw, idx, pl_equip_model_id_get(plw, (_EQUIP*)rec));
    }
}

/* Scales one attack value by the attack-boost skills the player has and writes the rank flag. */
extern "C" s32 fn_802739F0(_PLW* plw, s16 value, s32 mode, s8* out) {
    f32 v = (f32)value;

    if (mode == 0) {
        if (Pl_Skill_ck(plw, 65) == 1) {
            v *= lbl_8079A03C;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 66) == 1) {
            v *= lbl_8079A078;
            *out = 2;
        }
    } else {
        if (Pl_Skill_ck(plw, 63) == 1 || Pl_cat_skill_ck(plw, 6) == 1) {
            v *= lbl_8079A07C;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 64) == 1) {
            v *= lbl_8079A058;
            *out = 2;
        }
    }
    return (s32)v;
}

/* Stores two decoration equipment records into the spare slots, clearing whichever is absent. */
extern "C" void pl_equip_models_set(_PLW* plw, u8* a2, u8* a3, u8* a4) {
    equip_item_id_clamp((_EQUIP*)a2);
    equip_record_copy((_EQUIP*)((u8*)plw + 0x1DC), (_EQUIP*)a2);
    if (a3 != 0) {
        if (a3[0] != 0) {
            equip_item_id_clamp((_EQUIP*)a3);
            equip_record_copy((_EQUIP*)((u8*)plw + 0x200), (_EQUIP*)a3);
        } else {
            memset((u8*)plw + 0x200, 0, 12);
        }
    } else {
        memset((u8*)plw + 0x200, 0, 12);
    }
    if (a4 != 0) {
        if (a4[0] != 0) {
            equip_item_id_clamp((_EQUIP*)a4);
            equip_record_copy((_EQUIP*)((u8*)plw + 0x20C), (_EQUIP*)a4);
        } else {
            memset((u8*)plw + 0x20C, 0, 12);
        }
    } else {
        memset((u8*)plw + 0x20C, 0, 12);
    }
    plw->deco_dirty = 1;
}

/* Maps a slot's item id to the display slot number, the spare-slot half being marked with 0x80. */
extern "C" u16 pl_item_slot_find(_PLW* plw, u16 slot) {
    int i;

    if (GetItemData__FUs(slot)[0] == 1 && pl_item_page2_ck(plw) == 1) {
        for (i = 0; i < 8; i++) {
            if (slot == plw->spare_slot_id[i].item_id) {
                return (u16)(i | 0x80);
            }
        }
    }
    for (i = 0; i < 24; i++) {
        if (slot == plw->slot_id[i].item_id) {
            return (u16)i;
        }
    }
    return 0xFFFF;
}

} /* namespace s_80270018 */
#pragma optimization_level 3
#pragma peephole off

/* ==== 0x80273B14-0x80276B58: the act-entry front end ==== */
#include "Pl/pl_act_stage_latch_set.h"
#include "mh3_pad/Psw.h"
#include "mh3_pad/lb_param_w.h"
#include "types.h"
#include "hud/Pl_net_send.h"
#include "pl.h"
#include "mh3_pad/control.h"
#include "Pl/pl_act.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "Pl/fn_802693C4.h"   /* the part/motion section's `Pl_chr_set_attr_default`/`fn_8026A2DC`/
                          * `pl_model_aux_flag_clear`/`Pl_chr_setX` (rule 2) */
#include "ef/fn_800CDB2C.h"
#include "unsplit/unknown.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "menu/menu_item.h"   /* `item_category_ck` (owner: `menu/menu_item.cpp`, rule 2) */
#include "ef/eft001.h"
#include "stage/stg_w.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/em_pop.h"   /* quest_flag_80_ck, em_work_slot_pair_get, quest_element_remaining_get (rule 2) */
#include "enemy/em_model.h" /* em_roster_area_hit_find (the owner header, rule 2) */
#include "quest/quest_entry.h"   /* quest_element_supply_state_get, quest_element_item_use (rule 2) */

/* 0x8045F554 - the runtime string copy `pl_info_text_set` uses to arm a hunter name (owner: `MSL_C/alloc.cpp`, rule 2). */
#include "MSL_C/alloc.h"
#include "Pl/fn_8027D684.h" /* PlSkillRec, fn_8027ECAC (this unit's equipment-section header) */

/* 0x80335CE8 - the act-message sender `pl_act_enter_raw`'s tail calls (owner `hud/net_char_sync.cpp`, leaf header
 * `hud/Pl_net_send.h`).  The owner takes a third `u16`; this band's call sites pass two arguments (`mr r3, r30;
 * li r4, X; bl`), so they call it through this two-parameter view. */
typedef void (*PlNetSend2)(struct _PLW* plw, s32 kind);

/* The `equip_weapon_rec_get`/`equip_slot_rec_get` records (`PlWeaponRec`, `PlSlotRec`) and the `equip_blade_rec_get` row (`PlSkillRec`) are
 * shared with `menu/menu_infomation.cpp` and live in `Pl/fn_8027D684.h`. */

/* The per-slot record `fn_8027EE08` hands back through its second argument; `pl_weapon_stat_get` reads one
 * act-kind delta per slot out of it.  It is a different table from the `equip_slot_rec_get` record (`PlSlotRec`)
 * (its +0x0A is a 2-byte value where that one's is a signed byte). size: 0x14 */
typedef struct PlEquipSlot {
    /* +0x00 */ u8 pad_0x00[0xA];
    /* +0x0A */ s16 value_0x0A;   /* the act kind's 2-byte delta (case 6) */
    /* +0x0C */ u8 value_0x0C;    /* the kind-0 delta */
    /* +0x0D */ u8 pad_0x0D[0x1];
    /* +0x0E */ u8 kind_0x0E;     /* the skill-tier id the 7..11 arms compare against */
    /* +0x0F */ s8 value_0x0F;
    /* +0x10 */ u8 kind_0x10;     /* the skill-tier id the 12..14 arms compare against */
    /* +0x11 */ s8 value_0x11;
    /* +0x12 */ u8 pad_0x12[0x2];
} PlEquipSlot; /* size: 0x14 */

namespace s_80273B14 {
using ::_PLW;
using ::_EQUIP;
#include "Pl/fn_80273B14.h"

 /* size: 0x8 */

/* --- the act-kind dispatchers ----------------------------------------------------------------- */

/* Sums the act-kind deltas of the three equipment slots; `mode` selects the delta column (0/6 and the 7..16 skill
 * tiers), `flag` the melee/ranged variant (`fn_80273ED8`/`fn_80274D98` wrap it). */
extern "C" s32 pl_weapon_stat_get(struct _PLW* plw, s32 mode, u8 flag, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s32 value;
    PlSkillRec* slot;
    PlWeaponRec* scratch;

    *out = 0;
    value = 0;
    if ((u32)(mode - 6) <= 10U || mode == 0) {
        if (equip0 == NULL) {
            if (equip1 != NULL && mode == 6 && equip1->kind == 12) {
                value = (s16)equip_gun_level_bonus_get(equip1);
            }
            if (equip2 != NULL && mode == 0 && equip2->kind == 13) {
                value += fn_8027E708(equip2);
            }
        } else {
            switch (fn_8027EE08(equip0, &slot, &scratch)) {
            case 0:
                switch (mode) {
                case 6:
                    value = ((PlEquipSlot*)slot)->value_0x0A;
                    break;
                case 0:
                    value = ((PlEquipSlot*)slot)->value_0x0C;
                    break;
                case 7:
                    if (((PlEquipSlot*)slot)->kind_0x0E == 1) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x0F, 0, out);
                    }
                    break;
                case 8:
                    if (((PlEquipSlot*)slot)->kind_0x0E == 2) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x0F, 0, out);
                    }
                    break;
                case 9:
                    if (((PlEquipSlot*)slot)->kind_0x0E == 3) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x0F, 0, out);
                    }
                    break;
                case 10:
                    if (((PlEquipSlot*)slot)->kind_0x0E == 4) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x0F, 0, out);
                    }
                    break;
                case 11:
                    if (((PlEquipSlot*)slot)->kind_0x0E == 5) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x0F, 0, out);
                    }
                    break;
                case 12:
                    if (((PlEquipSlot*)slot)->kind_0x10 == 1) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x11, 1, out);
                    }
                    break;
                case 13:
                    if (((PlEquipSlot*)slot)->kind_0x10 == 2) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x11, 1, out);
                    }
                    break;
                case 14:
                    if (((PlEquipSlot*)slot)->kind_0x10 == 3) {
                        value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x11, 1, out);
                    }
                    break;
                case 15:
                    value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x0F, 0, out);
                    break;
                case 16:
                    value = fn_802739F0(plw, ((PlEquipSlot*)slot)->value_0x11, 1, out);
                    break;
                }
                if ((s16)value < 0) {
                    if (Pl_Skill_ck(plw, 0xC9) == 1U) {
                        *out = 1;
                        value = (s16)(value - value * 2);
                    } else if (flag == 0) {
                        value = 0;
                    } else {
                        value = (s16)(value - value * 2);
                    }
                }
                break;
            case 1:
                switch (mode) {
                case 6:
                    value = equip_gun_level_bonus_get(equip0);
                    if (equip1 != NULL && equip1->kind == 12) {
                        value += equip_gun_level_bonus_get(equip1);
                    }
                    break;
                case 0:
                    value = fn_8027E708(equip0);
                    if (equip2 != NULL && equip2->kind == 13) {
                        value += fn_8027E708(equip2);
                    }
                    break;
                default:
                    value = 0;
                    break;
                }
                break;
            }
        }
    } else {
        value = 0;
    }
    return value;
}

#pragma optimization_level 4
/* The single-slot wrapper of `pl_weapon_stat_get`: it hands the player's own melee/ranged slots on. */
extern "C" s32 fn_80273ED8(struct _PLW* plw, s32 kind, s32 flag)
{
    struct _EQUIP* slot;
    s32 value = 0;
    s32 n;
    PlSlotRec* rec;
    s8 out;

    switch (kind) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        value = pl_weapon_stat_get(plw, kind, flag, &plw->equipB, &plw->equipC, &plw->equipD, &out);
        break;
    case 0:
        value = pl_weapon_stat_get(plw, kind, flag, &plw->equipB, &plw->equipC, &plw->equipD, &out);
        /* fallthrough */
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        n = 0;
        slot = plw->equipA;
        do {
            rec = (PlSlotRec*)equip_slot_rec_get(slot);
            switch (kind) {
            case 0:
                value += equip_defense_get(slot);
                break;
            case 1:
                value += rec->value_0x08;
                break;
            case 2:
                value += rec->value_0x09;
                break;
            case 3:
                value += rec->value_0x0A;
                break;
            case 4:
                value += rec->value_0x0C;
                break;
            case 5:
                value += rec->value_0x0B;
                break;
            }
            slot++;
            n++;
        } while (n < 6);
        break;
    default:
        value = 0;
        break;
    }
    return value;
}
#pragma optimization_level reset

/* Maps a weapon/skill id to the bonus class the pick tables use. */
extern "C" u8 item_bonus_class_get(u16 kind)
{
    switch (kind) {
    case 0x8B:
        return 1;
    case 0x17D:
        return 3;
    case 0x17E:
        return 4;
    case 0x18B:
        return 2;
    default:
        return 0;
    }
}

/* Maps the player's act-state byte to the act kind `pl_weapon_stat_get` selects its delta column by. */
extern "C" s32 pl_act_kind_column_get(struct _PLW* plw)
{
    switch (plw->field_0x002) {
    case 6:
        return 1;
    case 5:
        pl_weapon_alt_form_ck(plw);
        return 2;
    case 4:
        pl_weapon_alt_form_ck(plw);
        return 0;
    default:
        return 0;
    }
}

/* Clears the player's short-range equipment slot record. */
extern "C" void pl_weapon_alt_form_ck(struct _PLW* plw)
{
    equip_bowgun_frame_kind_get(&plw->equipB);
}

/* Re-reads the player's armed weapon record into the level/attack bytes at +0x56A/+0x56E. */
extern "C" void pl_weapon_record_read(struct _PLW* plw, struct _EQUIP* equip)
{
    PlSkillRec* rec = equip_blade_rec_get(equip);

    plw->field_0x56A = rec->level_a;
    plw->field_0x56E = (s16)(rec->level_b * 0x32 + 0x96);
    if (Pl_Skill_ck(plw, 0x17) == 1U) {
        plw->field_0x56E += 0x32;
    }
    if (plw->field_0x56E > 0x1C2) {
        plw->field_0x56E = 0x1C2;
    }
}

/* Adds the act-kind and skill-tier deltas on top of `base`, and records which way the value moved. */
extern "C" s32 fn_80274174(struct _PLW* plw, s16 base, u8 ranged, s8* out)
{
    s32 value = base;

    if (base > 0 && ranged == 0 && pl_item_page2_ck(plw) == 0) {
        switch (plw->field_0x56B) {
        case 4:
            value += 5;
            break;
        case 5:
            value += 10;
            break;
        case 6:
            value += 15;
            break;
        default:
            break;
        }
    }
    if (ranged == 0) {
        if (Pl_dm_condition_ck(plw, 0x100) == 1U) {
            value -= 0x32;
            *out = 2;
        } else if (Pl_dm_condition_ck(plw, 0x200) == 1U) {
            value -= 0x32;
            *out = 2;
        }
    }
    if (Pl_Skill_ck(plw, 0x19) == 1U) {
        value += 10;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1A) == 1U) {
        value += 20;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1B) == 1U) {
        value += 30;
        *out = 1;
    } else if (Pl_Skill_ck(plw, 0x1C) == 1U) {
        value -= 5;
        *out = 2;
    } else if (Pl_Skill_ck(plw, 0x1D) == 1U) {
        value -= 10;
        *out = 2;
    } else if (Pl_Skill_ck(plw, 0x1E) == 1U) {
        value -= 15;
        *out = 2;
    }
    return value;
}
} /* namespace s_80273B14 */


/* Re-computes the player's critical-hit flag from the current weapon slot. */
s32 Pl_critical_get(struct _PLW* plw)
{
    using s_80273B14::fn_80274174;
    s8 flag;

    return fn_80274174(plw, plw->field_0x570, 0, &flag);
}
namespace s_80273B14 {


#pragma optimization_level 4
/* Sums the points of the two weapon records the act is holding, then folds in the act-kind and
 * skill-tier deltas. */
extern "C" s32 pl_weapon_affinity_get(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s32 value;

    *out = 0;
    switch (equip0->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        value = equip_blade_rec_get(equip0)->value;
        break;
    case 11:
        value = ((PlWeaponRec*)equip_weapon_rec_get(equip0))->bonus_0x12;
        if (equip1 != NULL && equip1->kind == 12) {
            value += ((PlWeaponRec*)equip_weapon_rec_get(equip1))->bonus_0x12;
        }
        if (equip2 != NULL && equip2->kind == 13) {
            value += ((PlWeaponRec*)equip_weapon_rec_get(equip2))->bonus_0x12;
        }
        break;
    case 12:
    case 13:
        value = ((PlWeaponRec*)equip_weapon_rec_get(equip0))->bonus_0x12;
        break;
    default:
        return 0;
    }
    if (plw != NULL) {
        value = fn_80274174(plw, (s16)value, 1, out);
    }
    return value;
}

/* Refreshes the player's armed weapon class and ranged attack value out of the melee slot. */
extern "C" void pl_weapon_class_refresh(struct _PLW* plw)
{
    switch (plw->equipB.kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        plw->field_0x570 = (s32)equip_blade_rec_get(&plw->equipB)->value;
        break;
    case 11:
        plw->field_0x570 = (s32)((PlWeaponRec*)equip_weapon_rec_get(&plw->equipB))->bonus_0x12;
        if (plw->equipD.kind == 13) {
            plw->field_0x570 += ((PlWeaponRec*)equip_weapon_rec_get(&plw->equipD))->bonus_0x12;
        }
        if (plw->equipC.kind == 12) {
            plw->field_0x570 += ((PlWeaponRec*)equip_weapon_rec_get(&plw->equipC))->bonus_0x12;
        }
        break;
    default:
        plw->field_0x570 = 0;
        break;
    }
}
#pragma optimization_level reset

/* True while the player's act is armed with a weapon. */
extern "C" s32 pl_weapon_drawn_ck(struct _PLW* plw)
{
    return plw->field_0x30A != 0;
}

/* Writes the armed byte of the player's own move work record. */
extern "C" void pl_gesture_req(s8 value)
{
    struct _PLW* work = (struct _PLW*)get_move_work_adrs(2);

    if (work != NULL) {
        work = (struct _PLW*)((u8*)work + (s8)my_player_no() * 0xB20);
        work->field_0x5C8 = value;
    }
}
} /* namespace s_80273B14 */


/* The act-number lists `pl_armed_act_no_get` (by armed slot) and `pl_act_class_get` (by act number) walk: the
 * melee family, then the ranged family; slot 0 is unused and 0xFFFF ends a list.  size: 0x1C each */
u16 pl_act_no_tbl_melee[14] = {0x0000, 0x000A, 0x0004, 0x0003, 0x0008, 0x0000, 0x0002,
                               0x0001, 0x0009, 0x0005, 0x0006, 0x000B, 0x0007, 0xFFFF};
u16 pl_act_no_tbl_ranged[14] = {0x000E, 0x0018, 0x0012, 0x0011, 0x0016, 0x000E, 0x0010,
                                0x000F, 0x0017, 0x0013, 0x0014, 0x0019, 0x0015, 0xFFFF};
namespace s_80273B14 {


/* Reads the armed slot's value table for the given kind. */
extern "C" u32 pl_armed_act_no_get(struct _PLW* plw, s16 kind)
{
    u8 slot = plw->field_0x5C8;

    if (slot == 0) {
        return 0;
    }
    if (kind == 0) {
        return pl_act_no_tbl_melee[slot];
    }
    return pl_act_no_tbl_ranged[slot];
}

/* Finds the index the player's act number occupies in one of the two act tables. */
extern "C" s32 pl_act_class_get(struct _PLW* plw)
{
    const u16* row;
    s32 index;

    if (plw->field_0x00A == 0xB) {
        row = &pl_act_no_tbl_melee[1];
        index = 1;
        while (*row != 0xFFFF) {
            switch (*row) {
            case 5:
                if (plw->act_no == 5 || plw->act_no == 12) {
                    return index;
                }
                break;
            case 11:
                if (plw->act_no == 11 || plw->act_no == 13) {
                    return index;
                }
                break;
            default:
                if (plw->act_no == *row) {
                    return index;
                }
                break;
            }
            index++;
            row += 1;
        }
        row = &pl_act_no_tbl_ranged[1];
        index = 1;
        while (*row != 0xFFFF) {
            switch (*row) {
            case 19:
                if (plw->act_no == 19 || plw->act_no == 26) {
                    return index;
                }
                break;
            case 25:
                if (plw->act_no == 25 || plw->act_no == 27) {
                    return index;
                }
                break;
            default:
                if (plw->act_no == *row) {
                    return index;
                }
                break;
            }
            index++;
            row += 1;
        }
    }
    return 0;
}

/* Latches the act's follow-up stage and arms its two-frame hold. */
extern "C" void pl_act_stage_latch_set(struct _PLW* plw, u8 stage)
{
    if (Pl_master_ck(plw) != 0) {
        plw->field_0x01F = stage;
        plw->field_0x396 = 2;
    }
}

/* True when the act's follow-up stage is open and the player is in a state that may use it. */
extern "C" s32 pl_warp_ready_ck(struct _PLW* plw)
{
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if ((u32)(plw->field_0x00A - 8) <= 1U) {
        return 0;
    }
    if (plw->field_0x370 <= 0) {
        return 0;
    }
    return plw->field_0x01F == 0;
}

/* The act's follow-up stage byte. */
extern "C" u8 pl_act_stage_get(struct _PLW* plw)
{
    return plw->field_0x01F;
}

/* The player's own move work record. */
extern "C" struct _PLW* my_player_work_get(void)
{
    struct _PLW* work = (struct _PLW*)get_move_work_adrs(2);

    return &work[(s8)my_player_no()];
}

/* The first move work record whose slot byte differs from this player's. */
extern "C" struct _PLW* pl_partner_work_get(struct _PLW* plw)
{
    struct _PLW* work = (struct _PLW*)get_move_work_adrs(2);
    u16 count = (u16)get_move_work_max(2);
    s32 i;

    for (i = 0; i < count; i++, work++) {
        if (work->chunk_ofs != plw->chunk_ofs) {
            return work;
        }
    }
    return NULL;
}

/* Buckets the player's stagger timer into 0-3. */
extern "C" s32 pl_stagger_level_get(void* self)
{
    struct _PLW* plw = (struct _PLW*)self;
    s32 value = 0;

    if (plw->field_0x386 > 0x50) {
        value = 3;
    } else if (plw->field_0x386 > 0x28) {
        value = 2;
    } else if (plw->field_0x386 > 0) {
        value = 1;
    }
    return value;
}

/* True once the armed-slot table has been read for this player. */
extern "C" s32 fn_80274904(struct _PLW* plw)
{
    return plw->field_0x5C9 != 0;
}

/* Picks the act-name table row for the player's current stance. */
extern "C" u8* pl_act_name_row_get(struct _PLW* plw, s32 force, s32 index)
{
    if (force == 0) {
        return (u8*)(lbl_805C5F30 + index * 0xA);
    }
    if (plw->field_0x5B8 >= 0xA0) {
        return (u8*)(lbl_805C5F30 + index * 0xA);
    }
    if (plw->field_0x036 == 0) {
        return (u8*)(lbl_805C5F50 + index * 0xA);
    }
    return (u8*)(lbl_805C5F70 + index * 0xA);
}

/* True when either of the player's weapon slots resolves to a bonus. */
extern "C" s32 fn_80274988(struct _PLW* plw)
{
    if (pl_item_page2_ck(plw) == 0) {
        return 0;
    }
    if (equip_gun_bonus_ck(&plw->equipB) == 1U) {
        return 1;
    }
    if (plw->equipC.kind == 12 && equip_gun_bonus_ck(&plw->equipC) == 1U) {
        return 1;
    }
    return 0;
}

/* The same test for a caller-supplied slot pair and act kind. */
extern "C" s32 fn_80274A04(struct _EQUIP* equip0, struct _EQUIP* equip1, u8 kind)
{
    if ((u32)(kind - 4) > 2U) {
        return 0;
    }
    if (equip0 != NULL) {
        if (equip0->kind == 11) {
            if (equip_gun_bonus_ck(equip0) == 1U) {
                return 1;
            }
            if (equip1 != NULL && equip1->kind == 12 && equip_gun_bonus_ck(equip1) == 1U) {
                return 1;
            }
        } else if (equip0->kind == 12 && equip_gun_bonus_ck(equip0) == 1U) {
            return 1;
        }
    }
    return 0;
}

/* True when the player's act kind is one the melee/ranged family owns. */
#pragma optimization_level 4
extern "C" s32 pl_weapon_guard_ck(struct _PLW* plw)
{
    switch (plw->field_0x002) {
    case 0:
    case 1:
    case 3:
        return 1;
    case 4:
    case 5:
    case 6:
        return fn_80274988(plw);
    }
    return 0;
}

/* The same test for a standalone act kind. */
extern "C" s32 fn_80274AEC(struct _EQUIP* equip0, struct _EQUIP* equip1, u8 kind)
{
    switch (kind) {
    case 0:
    case 1:
    case 3:
        return 1;
    case 4:
    case 5:
    case 6:
        return fn_80274A04(equip0, equip1, kind);
    }
    return 0;
}
#pragma optimization_level reset

/* Looks an id up in the 4-byte-stride id/value table at 0x805BF490. */
extern "C" u8 pl_item_icon_kind_get(u16 id)
{
    const u16* row = lbl_805BF490;
    while (row[0] != 0) {
        if (id == row[0]) {
            return (u8)row[1];
        }
        row += 2;
    }
    return 0xFF;
}

/* Sums the weapon/skill bonus records of the player's three weapon slots for the given act kind. */
extern "C" s32 pl_gun_clip_get(struct _PLW* plw, u8 kind, struct _EQUIP* equip0, struct _EQUIP* equip1,
                struct _EQUIP* equip2, s8* out)
{
    s32 value;

    *out = 0;
    value = equip_gun_clip_get(equip0, kind);
    if (equip1 != NULL && equip1->kind == 12) {
        value = (u8)(value + equip_gun_clip_get(equip1, kind));
    }
    if (equip2 != NULL && equip2->kind == 13) {
        value = (u8)(value + equip_gun_clip_get(equip2, kind));
    }
    switch (kind) {
    case 0:
    case 1:
    case 2:
        if (Pl_Skill_ck(plw, 0x36) == 1U) {
            value += 3;
            *out = 1;
        }
        break;
    case 3:
        if (Pl_Skill_ck(plw, 0x37) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 4:
    case 5:
        if (Pl_Skill_ck(plw, 0x38) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 6:
        if (Pl_Skill_ck(plw, 0x39) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 7:
    case 8:
        if (Pl_Skill_ck(plw, 0x3A) == 1U) {
            value += 2;
            *out = 1;
        }
        break;
    case 9:
        if (Pl_Skill_ck(plw, 0x3B) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 10:
    case 11:
        if (Pl_Skill_ck(plw, 0x3C) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 12:
        if (Pl_Skill_ck(plw, 0x3D) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    case 13:
    case 14:
        if (Pl_Skill_ck(plw, 0x3E) == 1U) {
            value += 1;
            *out = 1;
        }
        break;
    default:
        break;
    }
    if ((u8)value != 0) {
        if (Pl_Skill_ck(plw, 0xAA) == 1U) {
            value += 1;
            *out = 1;
        }
    }
    return value;
}

/* The player-bound wrapper of `pl_gun_clip_get`. */
extern "C" u8 fn_80274D98(struct _PLW* plw, u8 kind)
{
    return (u8)pl_gun_clip_get(plw, kind, &plw->equipB, &plw->equipC, &plw->equipD,
                            (s8*)&plw->field_0x00E);
}

/* True when the player-bound bonus sum is non-zero. */
extern "C" u8 fn_80274DCC(struct _PLW* plw, u8 kind)
{
    return fn_80274D98(plw, kind) != 0;
}

/* Gates the act's bonus sum on the act id being a live one. */
extern "C" s32 pl_item_pouch_ok_ck(struct _PLW* plw, u16 id)
{
    u8 cls;

    if (pl_item_page2_ck(plw) == 0) {
        return 1;
    }
    cls = pl_item_icon_kind_get(id);
    if (cls == 0xFF) {
        return 1;
    }
    return fn_80274DCC(plw, cls);
}

/* Sums the weapon records' `bonus_0x07` column and folds the skill tiers in, clamped to 0..6. */
extern "C" u8 pl_gun_recoil_get(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    s32 value;

    *out = 0;
    value = 0;
    if (equip0 != NULL && equip0->kind == 11) {
        value = (s8)(u8)((PlWeaponRec*)equip_weapon_rec_get(equip0))->bonus_0x07;
        if (value == 0) {
            value = 1;
        }
    }
    if (equip1 != NULL && equip1->kind == 13) {
        value += ((PlWeaponRec*)equip_weapon_rec_get(equip1))->bonus_0x07;
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0x2D) == 1U) {
            value += 2;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x2E) == 1U) {
            value += 3;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x2F) == 1U) {
            value += 4;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 0x30) == 1U) {
            value -= 1;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x31) == 1U) {
            value -= 2;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x32) == 1U) {
            value -= 3;
            *out = 2;
        }
    }
    if ((s8)value < 0) {
        value = 0;
    }
    if ((s8)value > 6) {
        value = 6;
    }
    return (u8)value;
}

/* Sums the weapon records' `bonus_0x09` column plus a fixed 3, clamped to 0..9. */
extern "C" u8 pl_gun_reload_get(struct _PLW* plw, struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    s32 value;

    *out = 0;
    value = 0;
    if (equip0 != NULL && equip0->kind == 11) {
        value = (s8)(u8)((PlWeaponRec*)equip_weapon_rec_get(equip0))->bonus_0x09;
    }
    if (equip1 != NULL && equip1->kind == 13) {
        value += ((PlWeaponRec*)equip_weapon_rec_get(equip1))->bonus_0x09;
    }
    if (plw != NULL) {
        if (Pl_Skill_ck(plw, 0x27) == 1U) {
            value += 2;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x28) == 1U) {
            value += 3;
            *out = 1;
        } else if (Pl_Skill_ck(plw, 0x29) == 1U) {
            value += 4;
            *out = 1;
        }
        if (Pl_Skill_ck(plw, 0x2A) == 1U) {
            value -= 1;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x2B) == 1U) {
            value -= 2;
            *out = 2;
        } else if (Pl_Skill_ck(plw, 0x2C) == 1U) {
            value -= 3;
            *out = 2;
        }
    }
    value += 3;
    if ((s8)value < 0) {
        value = 0;
    }
    if ((s8)value > 9) {
        value = 9;
    }
    return (u8)value;
}

/* The two weapon records' elemental bonus product, scaled by 1/100. */
extern "C" f32 pl_gun_deviation_get(struct _EQUIP* equip0, struct _EQUIP* equip1, s8* out)
{
    f32 value = pl_act_float_zero;

    *out = 0;
    if (equip0 != NULL) {
        if (equip0->kind == 11) {
            value = (f32)((PlWeaponRec*)equip_weapon_rec_get(equip0))->bonus_0x0A;
        }
        if (equip1 != NULL && equip1->kind == 12) {
            value = value * (f32)((PlWeaponRec*)equip_weapon_rec_get(equip1))->bonus_0x0A / lbl_8079A044;
        }
    } else if (equip1 != NULL && equip1->kind == 12) {
        value = (f32)((PlWeaponRec*)equip_weapon_rec_get(equip1))->bonus_0x0A;
    }
    return value / lbl_8079A044;
}

/* True when the item/act id is one the player may still use.  `mode` is dead in retail - every call
 * site sets r4, the body never reads it (see the owner's header, which marks the name a GUESS). */
extern "C" u32 Pl_item_id_usable_ck(u16 id, s32 mode)
{
    if (item_category_ck(id, 1) != 0) {
        return 0;
    }
    if (item_category_ck(id, 2) != 0) {
        return 0;
    }
    if (item_category_ck(id, 0x10) != 0) {
        return 0;
    }
    if (item_bonus_class_get(id) != 0) {
        return 0;
    }
    if ((u32)(id - 0x1B6) <= 1U || id == 0x2B || id == 0x35 || id == 0xDF) {
        return 0;
    }
    return 1;
}

/* True when the player's act/act-state pair is one of the two held combinations. */
extern "C" s32 pl_act_menu_lock_ck(struct _PLW* plw)
{
    switch (plw->field_0x00A) {
    case 0:
        if (plw->act_no == 0xB1 || plw->act_no == 0xA6) {
            return 1;
        }
        break;
    case 12:
        if (plw->act_no == 0xB) {
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* The per-act bonus the weapon slots contribute, out of the lobby parameter block's three rows. */
extern "C" s16 fn_802753E4(struct _PLW* plw, u16 id)
{
    s16 value = 0;

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if (plw->field_0x446 != 0 && Pl_cat_skill_ck(plw, 0x27) == 0) {
        return 0;
    }
    {
        LbParamWork* param = &lb_param_w;
        s32 key = id;

        if (key == param->flag_0x0C[0]) {
            value = (s16)(value + param->value_0x10[0]);
        }
        if (key == param->flag_0x0C[1]) {
            value += param->value_0x10[1];
        }
        if (key == param->flag_0x0C[2]) {
            value += param->value_0x10[2];
        }
    }
    return value;
}

/* Marks the player as gone. */
extern "C" void pl_model_refresh_req(struct _PLW* plw)
{
    plw->field_0x19 = 1;
}

/* Starts the mode's act for the player's current state. */
extern "C" void pl_act_hold_notify(struct _PLW* plw, u8 flag)
{
    u8 kind = 0;

    if (Pl_master_ck(plw) == 1U) {
        if (PlayMode_ck() == 2) {
            kind = plw->chunk_ofs;
        }
        pad_motor_start_level(kind, flag);
    }
}

/* Copies (or clears) the player's hunter name in the move work record. */
extern "C" void pl_info_text_set(struct _PLW* plw, const char* src)
{
    if (src == NULL) {
        memset(plw->name_0xB05, 0, 10);
        return;
    }
    strcpy((char*)plw->name_0xB05, src);
}
} /* namespace s_80273B14 */


/* The 11 act/motion pick rows `Pl_decide_mot_get` walks (0x805C60A8), four words each: the motion, its parameter and
 * the two control words the row is gated on (the Wii Remote's, then the Classic Controller's); retail indexes it as a
 * flat word table (`index * 4`, `index * 4 + 1`). size: 0x58 */
u16 pl_mot_pick_tbl[11 * 4] = {
    0x02D1, 0x02BD, 0x2000, 0x2000,
    0x02D3, 0x02BF, 0x0800, 0x0800,
    0x02D5, 0x02C3, 0x0400, 0x0400,
    0x02D7, 0x02C4, 0x1000, 0x1000,
    0x02DA, 0x02C7, 0x2000, 0x2000,
    0x02D2, 0x02BE, 0x0800, 0x0800,
    0x02D4, 0x02C0, 0x0400, 0x0400,
    0x02D6, 0x02C5, 0x1000, 0x1000,
    0x02D8, 0x02C6, 0x0040, 0x4000,
    0x02D9, 0x02C1, 0x0080, 0x8000,
    0x02DB, 0x02C2, 0x0030, 0x0014,
};

/* Picks the motion/parameter pair the player's current control state maps to. */
void Pl_decide_mot_get(u16* motion, u16* param)
{
    u16* row;
    s32 i;
    u16 index = (u16)(ran_suu(1) % 5);
    u16 mask;

    for (i = 0, row = pl_mot_pick_tbl; i < 11; row += 4, i++) {
        switch ((u8)get_ControlType(0)) {
        case 1:
            mask = row[2];
            if (i < 4) {
                if ((Psw[0].control_0x104 & mask) != 0) {
                    index = (u16)i;
                }
            } else if (i < 8) {
                if ((Psw[0].control_0x104 & mask) != 0 && (Psw[0].control_0x0FC & 4) != 0) {
                    index = (u16)i;
                }
            } else if ((Psw[0].control_0x0FC & mask) != 0) {
                index = (u16)i;
            }
            break;
        case 2:
            mask = row[3];
            if (i < 4) {
                if ((Psw[0].control_0x0DE & mask) != 0) {
                    index = (u16)i;
                }
            } else if (i < 8) {
                if ((Psw[0].control_0x0DE & mask) != 0 && (Psw[0].control_0x0D6 & 0x88) != 0) {
                    index = (u16)i;
                }
            } else if ((Psw[0].control_0x0D6 & mask) != 0) {
                index = (u16)i;
            }
            break;
        default:
            break;
        }
    }
    *motion = pl_mot_pick_tbl[index * 4];
    *param = pl_mot_pick_tbl[index * 4 + 1];
}
namespace s_80273B14 {


/* Resets every per-act field, then arms the new act's kind/number from its flag word `mask`. */
extern "C" void pl_act_enter_raw(struct _PLW* plw, u8 kind, u16 no, u16 mask)
{
    u32 i;

    plw->field_0x35C = 0;
    plw->field_0x360 = 0;
    plw->act_step_0x05 = 0;
    plw->field_0x006 = 0;
    plw->field_0x007 = 0;
    plw->prev_act_kind = plw->field_0x00A;
    plw->prev_act_no = plw->act_no;
    plw->field_0x00A = (u8)kind;
    plw->act_no = (u16)no;
    plw->field_0x264 = 0;
    plw->field_0x030 = 0;
    if ((mask & 0x20) != 0) {
        plw->field_0x0B6 = (u16)((ran_suu(1) & 0xFF00) | (plw->field_0x0B6 & 0xFF));
    } else {
        plw->field_0x0B6 = (u16)ran_suu(1);
    }
    plw->field_0x354 = lbl_8079A080;
    plw->field_0x318 = 0;
    plw->field_0x313 = 0;
    plw->field_0x314 = 0;
    plw->field_0x31C = 0;
    plw->field_0x320 = 0;
    plw->field_0x31E = 0;
    plw->field_0x36A = 0;
    plw->field_0x388 = 0;
    for (i = 0; i < 0x30; i++) {
        plw->field_0x322[i] = 0;
    }
    if ((mask & 0x400) != 0) {
        plw->field_0x0A8 = (u16)(plw->field_0x058 - 0x2000);
    }
    if ((mask & 0x800) != 0) {
        plw->field_0x0A8 = (u16)(plw->field_0x058 + 0x2000);
    }
    if (Pl_master_ck(plw) == 1U && (mask & 0x1000) != 0) {
        s32 step;

        mask &= 0xEFFF;
        switch (plw->field_0x002) {
        case 2:
            step = 0x1000;
            break;
        case 7:
            step = 0x1000;
            break;
        case 8:
            step = 0x1000;
            break;
        case 3:
            step = 0x1000;
            break;
        default:
            step = 0;
            break;
        }
        if (Pl_master_ck(plw) == 1U) {
            if (pl_stick_dir_ck(plw, 0x800) == 1U) {
                plw->field_0x0A8 += step;
            } else if (pl_stick_dir_ck(plw, 0x400) == 1U) {
                plw->field_0x0A8 -= step;
            }
        }
    }
    if ((mask & 0x4000) == 0) {
        plw->field_0x445 = 0;
    }
    if ((mask & 0x80) == 0) {
        plw->field_0x0AC = 0;
    }
    if ((mask & 0x100) == 0) {
        plw->field_0x567 = (u8)(plw->field_0x567 & 0xFE);
    }
    if ((mask & 1) == 0) {
        plw->field_0x646 = 0;
        plw->field_0x648 = 0;
        plw->field_0x64A = 0;
        plw->field_0x64F = 0;
    }
    if ((mask & 0x2000) == 0) {
        plw->field_0x068 = lbl_8079A084;
        plw->field_0x06C = lbl_8079A084;
        plw->field_0x070 = lbl_8079A084;
    }
    pl_model_hide(plw);
    pl_model_aux_flag_clear(plw);
    pl_shell_ids_set(plw, 0xFF, 0xFF);
    plw->field_0x662 = 0;
    plw->field_0x3A3 = 0;
    plw->field_0x3A2 = 0;
    plw->field_0x565 = 0;
    plw->field_0x566 = 0;
    if (Pl_master_ck(plw) == 1U && (mask & 2) == 0) {
        if ((mask & 0x10) != 0) {
            ((PlNetSend2)Pl_net_send)(plw, 6);
        } else if ((mask & 0x40) != 0) {
            ((PlNetSend2)Pl_net_send)(plw, 9);
        } else {
            ((PlNetSend2)Pl_net_send)(plw, 1);
        }
        plw->field_0x012 = 0xF;
    }
}

/* Sets the per-act marker byte and arms the act. */
extern "C" void pl_act_enter(struct _PLW* plw, s32 a, u16 b, u16 c)
{
    plw->field_0x00E = 1;
    pl_act_enter_raw(plw, a, b, c);
}

/* The "2" marker's act entry, with the +0x256 timer preset. */
extern "C" void pl_act_enter_down(struct _PLW* plw, s32 a, u16 b, u16 c)
{
    plw->field_0x584 = 1;
    plw->field_0x256 = 0x1C2;
    pl_act_enter_raw(plw, a, b, c);
}

/* Writes the player's act kind byte. */
extern "C" void pl_act_kind_set(struct _PLW* plw, u8 value)
{
    plw->kind_0x09 = value;
}

/* Arms the act's three status bits from a packed mask. */
extern "C" void Pl_act_set_motion(struct _PLW* plw, u16 a, u32 b, u32 c)
{
    switch ((u8)(u32)a) {
    default:
        pl_act_kind_set(plw, 0);
        break;
    case 1:
        pl_act_kind_set(plw, 1);
        break;
    case 2:
        pl_act_kind_set(plw, 2);
        break;
    case 3:
        pl_act_kind_set(plw, 3);
        break;
    }
    if ((a & 0x8000) != 0) {
        pl_cond_flags_clear(plw, 1);
    } else {
        pl_act_set_flag(plw, 1);
    }
    if ((u16)b == 0) {
        pl_cond_flags_clear(plw, 2);
    } else {
        pl_act_set_flag(plw, 2);
    }
    if ((u16)c == 0) {
        pl_cond_flags_clear(plw, 4);
        return;
    }
    pl_act_set_flag(plw, 4);
}

/* The act-state dispatcher's motion hand-off. */
extern "C" void fn_80275C18(struct _PLW* plw, u32 motion, u32 a, u32 b, u32 c)
{
    if ((u16)c == 0) {
        Pl_chr_setX(plw, (u16)motion, a, b);
    } else {
        Pl_chr_set_attr_default(plw, (u16)motion, a, b);
    }
}

/* Picks and arms the entry motion for the player's current act state. */
extern "C" void fn_80275C34(struct _PLW* plw, u32 kind, u32 a, u32 b, u32 c, u32 d)
{
    plw->field_0x5C4 = 0;
    plw->field_0x310 = 0;
    plw->kind_0x09 = (u8)kind;
    switch (kind) {
    default:
        if (plw->field_0x585 != 0) {
            if (plw->field_0x018 == 1) {
                if (plw->field_0x002 == 8) {
                    if (pl_mode8_free_ck() == 1U) {
                        fn_80275C18(plw, 0x3E9, a, b, (u16)d);
                    } else {
                        fn_80275C18(plw, 0x3FC, a, b, (u16)d);
                    }
                } else {
                    fn_80275C18(plw, 0x3E9, a, b, (u16)d);
                }
            } else {
                fn_80275C18(plw, 0x18, a, b, (u16)d);
            }
            pl_act_enter(plw, 0, 0x6A, (u16)c);
            return;
        }
        if (plw->field_0x018 == 1) {
            if (plw->field_0x002 == 8) {
                if (pl_mode8_free_ck() == 1U) {
                    fn_80275C18(plw, 0x3E9, a, b, (u16)d);
                } else {
                    fn_80275C18(plw, 0x3FC, a, b, (u16)d);
                }
            } else {
                fn_80275C18(plw, 0x3E9, a, b, (u16)d);
            }
            pl_act_enter(plw, 0, 0, (u16)c);
            return;
        }
        if (pl_item_menu_open_ck(plw) == 1U) {
            fn_80275C18(plw, 0x190, a, b, (u16)d);
            pl_act_enter(plw, 0xA, 0, (u16)c);
            return;
        }
        if (plw->field_0x37A <= 0x96) {
            fn_80275C18(plw, 0x12F, a, b, (u16)d);
        } else {
            switch (pl_act_kind_get(plw->area_0x16)) {
            case 1:
                if (Pl_Skill_ck(plw, 0x7C) == 1U || Pl_Skill_ck(plw, 0x7D) == 1U
                    || Pl_condition_ck(plw, 0x400) == 1U) {
                    fn_80275C18(plw, 1, a, b, (u16)d);
                } else {
                    fn_80275C18(plw, 0x15, a, b, (u16)d);
                }
                break;
            case 3:
                if (Pl_Skill_ck(plw, 0x7D) == 1U || Pl_condition_ck(plw, 0x400) == 1U) {
                    fn_80275C18(plw, 1, a, b, (u16)d);
                } else {
                    fn_80275C18(plw, 0x15, a, b, (u16)d);
                }
                break;
            case 2:
                if (Pl_Skill_ck(plw, 0x80) == 1U || Pl_Skill_ck(plw, 0x81) == 1U
                    || Pl_condition_ck(plw, 0x800) == 1U) {
                    fn_80275C18(plw, 1, a, b, (u16)d);
                } else {
                    fn_80275C18(plw, 0x14B, a, b, (u16)d);
                }
                break;
            case 4:
                if (Pl_Skill_ck(plw, 0x81) == 1U || Pl_condition_ck(plw, 0x800) == 1U) {
                    fn_80275C18(plw, 1, a, b, (u16)d);
                } else {
                    fn_80275C18(plw, 0x14B, a, b, (u16)d);
                }
                break;
            default:
                fn_80275C18(plw, 1, a, b, (u16)d);
                break;
            }
        }
        pl_act_enter(plw, 0, 0, (u16)c);
        return;
    case 1:
        if (plw->field_0x585 != 0) {
            fn_80275C18(plw, 0x18, a, b, (u16)d);
            pl_act_enter(plw, 0, 0x6A, (u16)c);
            return;
        }
        fn_80275C18(plw, 8, a, b, (u16)d);
        if (pl_item_menu_open_ck(plw) == 1U) {
            pl_act_enter(plw, 0xA, 0xE, (u16)c);
            return;
        }
        pl_act_enter(plw, 0, 0x1F, (u16)c);
        return;
    case 3:
        if (plw->field_0x018 == 0) {
            if (fn_80276800(plw, 0) == 1) {
                fn_80275C18(plw, 0x79, a, b, (u16)d);
            } else if (plw->field_0x37A <= 0x96) {
                fn_80275C18(plw, 0x168, a, b, (u16)d);
            } else if (Pl_suimen_ck(plw) == 1U) {
                fn_80275C18(plw, 0x76, a, b, (u16)d);
            } else {
                fn_80275C18(plw, 0x64, a, b, (u16)d);
            }
        } else if (plw->field_0x002 == 8) {
            if (pl_mode8_free_ck() == 1U) {
                fn_80275C18(plw, 0x41A, a, b, (u16)d);
            } else {
                fn_80275C18(plw, 0x438, a, b, (u16)d);
            }
        } else {
            fn_80275C18(plw, 0x41A, a, b, (u16)d);
        }
        pl_act_enter(plw, 0, 0x16, (u16)(c | 0x80));
        return;
    }
}

/* The three no-argument act-state entries. */
extern "C" void Pl_act_set_motion_slot(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 0, 0);
}

extern "C" void pl_act_reset_idle(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 12, 1);
}

extern "C" void pl_act_reset_down(struct _PLW* self, u32 a, u32 b, u32 c)
{
    fn_80275C34(self, a, b, c, 2, 1);
}

extern "C" void pl_aim_motion_set(_PLW*, u16, u16);

/* The motion-parameter act-state entry: enters the act state, then hands the two motion parameters on. */
extern "C" void pl_act_state_enter_aim(struct _PLW* self, u32 a, u32 b, u32 c, u32 param_a, u32 param_b)
{
    fn_80275C34(self, a, b, c, 1, 0);
    pl_aim_motion_set(self, param_a, param_b);
}

/* Clears the act's hold latch and re-enters the act state. */
extern "C" void pl_act_reenter(struct _PLW* self, s32 a, s32 b, s32 c)
{
    self->field_0x442 = 0;
    self->field_0x30A = 0xFF;
    fn_80275C34(self, a, b, c, 0, 0);
}

/* True when the given motion value fits inside the act's stagger budget. */
extern "C" s32 pl_stagger_budget_ck(struct _PLW* self, s32 value)
{
    return self->field_0x378 >= (s16)value;
}

/* True when the given motion value fits inside the act's hold gauge. */
extern "C" u32 pl_hold_gauge_ck(struct _PLW* plw, s32 value)
{
    return plw->field_0x370 >= (s16)value;
}

/* Adds `delta` to the act's hold gauge, clamped to 0..`+0x372`. */
extern "C" void pl_act_add_hold_gauge(struct _PLW* plw, s16 delta)
{
    if (Pl_master_ck(plw) != 0 && (Pl_motion_input_ck(0) != 1U || delta >= 0) && plw->field_0x00A != 8) {
        s16 value;

        plw->field_0x370 += delta;
        value = plw->field_0x370;
        if (value <= 0) {
            plw->field_0x370 = 0;
        }
        if (plw->field_0x370 >= plw->field_0x372) {
            plw->field_0x370 = plw->field_0x372;
        }
        if (plw->field_0x376 < plw->field_0x370) {
            plw->field_0x376 = plw->field_0x370;
        }
    }
}

/* The act's frame step: runs the hold gauge down and either enters the act's next motion or dumps
 * the player out of it.  Returns 1 when the act ended this frame. */
extern "C" s32 pl_health_damage_apply(struct _PLW* plw, s16 delta, s8* out)
{
    nw4r::math::VEC3 pos;
    s16 before;

    VEC3_ctor(&pos);
    if (out != NULL) {
        *out = 0;
    }
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    before = plw->field_0x370;
    pl_act_add_hold_gauge(plw, delta);
    if (plw->field_0x370 <= 0) {
        if (Pl_cat_skill_ck(plw, 0x2C) == 1U) {
            if (plw->field_0x447 == 0 && before >= 0x40) {
                plw->field_0x447++;
                plw->field_0x370 = 1;
                plw->field_0x376 = 1;
                ((PlJointHolder*)plw->physics_0x13C)->chr_0x04.get_joint_wpos(3, &pos);
                eft_model_effect_spawn(&pos, 0, 0, plw->area_0x16, (_CP_VECTOR*)&plw->param_0x54, lbl_8079A080);
                if (out != NULL) {
                    *out = 1;
                }
                return 0;
            }
        }
        plw->field_0x376 = 0;
        if (plw->kind_0x09 != 2) {
            pl_ride_down_set(plw);
            return 1;
        }
        if (plw->field_0x00A != 6 && Pl_act_ck(plw, 2, 1) == 0) {
            pl_act_enter_raw(plw, 2, 1, 0);
        }
    }
    return 0;
}

/* The same step, suppressed while the shell/ammo latch is latched. */
extern "C" s32 pl_act_gauge_step_unlatched(struct _PLW* plw, s16 delta, s8* out)
{
    if (fn_8027E1E4(plw) == 1U) {
        return 0;
    }
    return pl_health_damage_apply(plw, delta, out);
}

/* The same step for a signed gauge delta (a negative one also moves the stagger gauge). */
extern "C" s32 pl_act_gauge_step_signed(struct _PLW* plw, s16 delta, s8* out)
{
    s16 value;

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    value = 0;
    if (delta < 0) {
        plw->field_0x376 += delta;
        if (plw->field_0x376 < plw->field_0x370) {
            value = plw->field_0x376 - plw->field_0x370;
        }
    } else {
        value = delta;
    }
    return pl_health_damage_apply(plw, value, out);
}

/* Applies the skill-tier damage bonus and steps the hold gauge; `from_partner` (0 for the actor itself, 1 for a
 * partner it heals) is unread. */
extern "C" void pl_health_recover(struct _PLW* plw, s16 amount, s32 from_partner)
{
    if (amount > 0) {
        if (Pl_cat_skill_ck(plw, 3) == 1U) {
            amount += (s16)(amount / 10);
        }
        if (Pl_Skill_ck(plw, 0x98) == 1U) {
            amount += (s16)(amount / 4);
        } else if (Pl_Skill_ck(plw, 0x99) == 1U) {
            amount -= (s16)(amount / 4);
        }
    }
    pl_act_add_hold_gauge(plw, amount);
}

/* Grows the act's gauge ceilings (and, when `a` is 0, the soft cap) by `amount`. */
extern "C" void pl_gauge_ceiling_grow(struct _PLW* plw, s16 amount, u8 a)
{
    if (Pl_master_ck(plw) != 0) {
        plw->field_0x372 += amount;
        if (plw->field_0x372 <= 1) {
            plw->field_0x372 = 1;
        }
        if (plw->field_0x372 >= 0x96) {
            plw->field_0x372 = 0x96;
        }
        if (plw->field_0x370 >= plw->field_0x372) {
            plw->field_0x370 = plw->field_0x372;
        }
        if (plw->field_0x376 >= plw->field_0x372) {
            plw->field_0x376 = plw->field_0x372;
        }
        if (a == 0) {
            plw->field_0x374 += amount;
            if (plw->field_0x374 <= 1) {
                plw->field_0x374 = 1;
            }
            if (plw->field_0x374 >= 0x96) {
                plw->field_0x374 = 0x96;
            }
        }
    }
}

/* Adds to the act's stagger budget up to its 150-point ceiling. */
extern "C" void pl_stagger_budget_add(struct _PLW* plw, s16 amount)
{
    if (amount > 0) {
        plw->field_0x382 = 0;
    }
    plw->field_0x380 += amount;
    if (plw->field_0x380 >= 0x96) {
        plw->field_0x380 = 0x96;
    }
    pl_act_add_stagger(plw, 0x96);
}

/* Adds to the act's current stagger value, clamped to 0..`+0x380`. */
extern "C" void pl_act_add_stagger(struct _PLW* plw, s16 amount)
{
    if (amount > 0) {
        plw->field_0x382 = 0;
    }
    plw->field_0x37E += amount;
    if (plw->field_0x37E >= plw->field_0x380) {
        plw->field_0x37E = plw->field_0x380;
        return;
    }
    if (plw->field_0x37E < 0) {
        plw->field_0x37E = 0;
    }
}

/* True while the act's current stagger value is inside `v`. */
extern "C" u32 fn_80276800(struct _PLW* self, s32 v)
{
    return self->field_0x37E <= (s16)v;
}

/* True while the act's hold-gauge re-arm word is set. */
extern "C" u32 Pl_act_state_ck(struct _PLW* plw)
{
    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    return plw->field_0x412 > 0;
}

/* Steps the act's stagger budget by `amount`, clamped to 0..`+0x37A`. */
extern "C" void pl_sub_gauge_add(struct _PLW* plw, s16 amount)
{
    if (Pl_master_ck(plw) != 0 && (amount >= 0 || Pl_act_state_ck(plw) != 1)) {
        s16 value;

        plw->field_0x378 += amount;
        value = plw->field_0x378;
        if (value <= 0) {
            plw->field_0x378 = 0;
        }
        if (plw->field_0x378 >= plw->field_0x37A) {
            plw->field_0x378 = plw->field_0x37A;
        }
    }
}

/* The same step with the defence-skill and combo modifiers folded in. */
extern "C" void pl_act_gauge_gate(struct _PLW* plw, s16 value)
{
    if (Pl_master_ck(plw) == 0) {
        return;
    }
    if (value < 0) {
        if (Pl_dm_condition_ck(plw, 0x80) == 1U) {
            value *= 3;
        } else if (Pl_dm_condition_ck(plw, 0x40) == 1U) {
            value *= 3;
        }
        if (value == -1) {
            if (Pl_Skill_ck(plw, 0xA8) == 1U && (plw->frame_0x020 & 1) != 0) {
                return;
            }
        } else if (Pl_Skill_ck(plw, 0xA8) == 1U) {
            value = value / 2;
        }
        if (Pl_Skill_ck(plw, 0xA9) == 1U && (s32)plw->frame_0x020 % 5 == 0) {
            value = value * 2;
        }
    }
    pl_sub_gauge_add(plw, value);
}

/* The act's guard-break entry: picks the stagger step from the defence skills and applies it. */
extern "C" void pl_guard_break_apply(struct _PLW* plw)
{
    s16 step = -1;
    s32 mask = 0;

    if (Pl_dm_condition_ck(plw, 0x80) == 1U) {
        step = -3;
    } else if (Pl_dm_condition_ck(plw, 0x40) == 1U) {
        step = -3;
    }
    if (Pl_Skill_ck(plw, 0xA8) == 1U) {
        mask = 1;
    }
    if (Pl_cat_skill_ck(plw, 0x17) == 1U) {
        if (mask == 0) {
            mask = 1;
        } else {
            mask = 3;
        }
    }
    if ((u16)mask == 0 || (plw->frame_0x020 & (u16)mask) == 0) {
        if (Pl_Skill_ck(plw, 0xA9) == 1U && (s32)plw->frame_0x020 % 5 == 0) {
            step *= 2;
        }
        pl_sub_gauge_add(plw, step);
    }
}

} /* namespace s_80273B14 */

/* ==== 0x80276B58-0x8027D684: the action state machines ==== */

#include "types.h"
#include "enemy/enemy_control.h"
#include "ef/fn_800CDB2C.h"   /* my_player_no (rule 2: the owner is `ef/system_core.cpp`) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Pl/pl_coll.h" /* the owner of the `.bss` move-work table `pl_move_work` (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "quest/quest_item_slot.h" /* quest_item_work_notify (rule 2: its owner) */
#include "ef/eft019.h" /* ef_inst_spawn (rule 2: its owner) */

/* The actors this unit calls into; the mangling of the source names reproduces the map's spellings. */
u32 Pl_Skill_ck(_PLW*, u16);
u32 Pl_cat_skill_ck(_PLW*, u16);
namespace s_80276B58 {


extern "C" u32 Pl_act_state_ck(_PLW*);
extern "C" void pl_sub_gauge_add(_PLW*, s16);
extern "C" void fn_80276CE8(_PLW*, s16);
extern "C" u8 pl_act_kind_get(u8);
extern "C" u32 pl_mode8_free_ck(void);
extern "C" void eft_pl_weapon_follow_spawn(_PLW*);
extern "C" u32 pl_item_page2_ck(_PLW*);
extern "C" void mhchar_motion_speed_set(u8*, f32);
extern "C" u32 stage_pos_z_low_ck(void*);
extern "C" void pl_cond_flags_clear(_PLW*, s32);
extern "C" void pl_act_enter(_PLW*, s32, u16, u16);

/* In-unit callees that the appended bodies reference before their own definition. */
extern "C" void pl_skill_update2(_PLW*, s32);
extern "C" void pl_camera_lock_clear(_PLW*);
extern "C" u32 fn_80278144(u8, u8*, u8);
extern "C" u32 pl_area_spot_valid_ck(u8, u8*, u8);
extern "C" void pl_dodge_motion_start(_PLW*, s32, s32);
} /* namespace s_80276B58 */


/* condition-bit tests over the actor's flag words, one per flag set. */
u32 Pl_dm_condition_ck(_PLW*, u32);


u32 Pl_act_ck(_PLW*, u8, u16);
void Pl_get_gunner_vec(_PLW*, _CP_VECTOR*);
void Pl_get_gunner_pos(_PLW*, nw4r::math::VEC3*, s32);
void cpSetRotMatrixZXY(_CP_VECTOR*, nw4r::math::MTX34*);
void rotVecXYZ(nw4r::math::VEC3*, _CP_VECTOR*);
namespace s_80276B58 {


extern "C" void eft_rot_vec_copy(_CP_VECTOR*, void*);
extern "C" f32 vec3_distance(void*, void*);
extern "C" u32 stage_map_kind_get(u8);
extern "C" u32 fn_802753E4(_PLW*, s32);
extern "C" void fn_80278D1C(_PLW*);
} /* namespace s_80276B58 */

u8 get_now_mapno(void);
u32 Pl_frame_check(_PLW*, u32, f32, f32);
f32 GetGroundHit2(nw4r::math::VEC3*, u32, u8, u8*);
void rotVecY(nw4r::math::VEC3*, u32);
namespace s_80276B58 {


extern "C" s32 cockpit_supply_notice_spawn(s32);
extern "C" u32 isServerSelectState(s32);
/* 0x80338E04 - `lobby/lb_companion_ui.cpp`'s.  Its header declares `Pl_cat_skill_ck` returning `void` against this
 * file's `u32` (`(10505) illegal overloading`), so the declaration is this unit's view in a linkage block; the
 * three arguments are this unit's call site (the callee reads r3/r4/r5). */
extern "C" {
void lb_entry_handover_send(s32, u8, u8);
}
extern "C" void pl_item_add(_PLW*, u16, s16);
extern "C" void cockpit_pointer_target_set(u16);

extern "C" s32 pl_part_flag_ck(_PLW*, s32);
extern "C" u8 pl_stagger_level_get(_PLW*);

extern "C" u32 pl_carry_item_get(_PLW*);
extern "C" s32 pl_item_slot_item_get(_PLW*, u8);
extern "C" u8 pl_item_icon_kind_get(u16);
extern "C" s16 pl_item_slot_count_get(_PLW*, u8);
extern "C" u8 fn_80274D98(_PLW*, u8);
extern "C" u32 pl_scan_rows_id252_ck(_PLW*);
extern "C" u8* equip_weapon_rec_get(void*);
extern "C" u8* gun_ammo_entry_find(u8*, u8);

extern "C" { extern const f32 lbl_8079A088; }
extern "C" { extern const f32 lbl_8079A08C; }
extern "C" { extern const f32 lbl_8079A090; }
extern "C" { extern const f32 lbl_8079A094; }
extern "C" { extern const f32 lbl_8079A098; }
extern "C" { extern const f32 lbl_8079A09C; }
extern "C" { extern const f64 lbl_8079A0A0; }
extern "C" { extern const f32 lbl_8079A0A8; }
extern "C" { extern const f32 lbl_8079A0AC; }
extern "C" { extern const f32 lbl_8079A0B0; }
extern "C" { extern const f32 lbl_8079A0B4; }
extern "C" { extern const f32 lbl_8079A084; }
extern "C" { extern const f32 lbl_8079A0C8; }
extern "C" { extern const f32 lbl_8079A0CC; }
extern "C" { extern const f32 lbl_8079A0D0; }
extern "C" { extern const f32 lbl_8079A0DC; }
extern "C" { extern const f32 lbl_8079A0E0; }
extern "C" { extern const f32 lbl_8079A0E4; }
extern "C" { extern const f32 lbl_8079A0E8; }
extern "C" { extern const f32 lbl_8079A0FC; }
extern "C" { extern const f32 lbl_8079A0F4; }
extern "C" { extern const f32 lbl_8079A100; }
extern "C" { extern const f32 lbl_8079A104; }
extern "C" { extern const f32 lbl_8079A108; }
extern "C" { extern const f32 lbl_8079A10C; }
extern "C" { extern const f32 lbl_8079A0C4; }
extern "C" { extern const f32 lbl_8079A0EC; }
extern "C" { extern const f32 lbl_8079A110; }

extern "C" { extern u32 pl_steps_kind8_guard[]; }
extern "C" { extern u32 pl_steps_kind8_slot3_guard[]; }

/* 0x80276B58: the attack-scale multiplier the actor's active skills grant for a signed modifier. */
extern "C" void pl_sub_gauge_skill_add(_PLW* self, s32 arg1)
{
    if (Pl_master_ck(self) != 0) {
        f32 f = (f32)(s16)arg1;
        if ((s16)arg1 < 0) {
            if (Pl_Skill_ck(self, 0xB9) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A088;
                } else {
                    f *= lbl_8079A08C;
                }
            } else if (Pl_Skill_ck(self, 0xBA) == 1) {
                f *= lbl_8079A088;
            } else if (Pl_Skill_ck(self, 0xBB) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A090;
                } else {
                    f *= lbl_8079A094;
                }
            } else if (Pl_Skill_ck(self, 0xBC) == 1) {
                if (Pl_cat_skill_ck(self, 4) == 1) {
                    f *= lbl_8079A098;
                } else {
                    f *= lbl_8079A09C;
                }
            } else if (Pl_cat_skill_ck(self, 4) == 1) {
                f *= lbl_8079A08C;
            }
        }
        pl_sub_gauge_add(self, (s16)f);
    }
}

/* 0x80276CE8: adds a signed amount to the actor's stamina pool and clamps it, with a lower bound reset. */
extern "C" void fn_80276CE8(_PLW* self, s16 arg1)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (arg1 < 0 && Pl_act_state_ck(self) == 1) {
        return;
    }
    {
        self->unk37A += arg1;
        if (self->unk37A <= 0x96) {
            self->unk37A = 0x96;
        } else if (self->unk37A > 0x384) {
            self->unk37A = 0x384;
            self->unk37C = 0x2A30;
        }
        s16 v = self->unk37A;
        if (self->unk378 > v) {
            self->unk378 = v;
        }
    }
}

/* 0x80276D94: applies the stamina bonus the two armour skills grant, then the signed amount. */
extern "C" void pl_stamina_add(_PLW* self, s32 arg1)
{
    if ((s16)arg1 > 0 && (Pl_Skill_ck(self, 0x48) == 1 || Pl_Skill_ck(self, 0x49) == 1)) {
        arg1 += 150;
    }
    fn_80276CE8(self, (s16)arg1);
}

/* 0x80276E08: recomputes the actor's attack-range/level modifier from its weapon class and skills. */
extern "C" void pl_area_stamina_drain_step(_PLW* self)
{
    if (Pl_master_ck(self) != 0) {
        s16 cls = pl_act_kind_get(self->area_0x16);
        s32 v = 0;
        if (cls == 2 || cls == 4) {
            if (self->unk466 == 0 && (cls != 2 || (Pl_Skill_ck(self, 0x80) != 1 && Pl_Skill_ck(self, 0x81) != 1))
                && (cls != 4 || Pl_Skill_ck(self, 0x81) != 1)) {
                if (cls == 2) {
                    if (Pl_Skill_ck(self, 0x82) == 1) {
                        v = 3;
                    } else if (Pl_Skill_ck(self, 0x83) == 1) {
                        v = 4;
                    } else {
                        v = 2;
                    }
                } else if (Pl_Skill_ck(self, 0x80) == 1) {
                    v = 1;
                } else if (Pl_Skill_ck(self, 0x82) == 1) {
                    v = 4;
                } else if (Pl_Skill_ck(self, 0x83) == 1) {
                    v = 6;
                } else {
                    v = 3;
                }
            }
        } else {
            v = 0;
        }
        if (Pl_Skill_ck(self, 0x45) == 0) {
            if (self->kind_0x09 == 3) {
                if ((self->unk020 & 1) == 0 && (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 3) != 0)) {
                    v += 1;
                    if (Pl_Skill_ck(self, 0x47) == 1) {
                        v += 1;
                    } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 3) == 0) {
                        v += 1;
                    }
                }
            } else if (Pl_Skill_ck(self, 0x44) != 1 || (self->unk020 & 1) != 0) {
                v += 1;
                if (Pl_Skill_ck(self, 0x47) == 1) {
                    v += 1;
                } else if (Pl_Skill_ck(self, 0x46) == 1 && (self->unk020 & 1) == 0) {
                    v = (s16)(v + 1);
                }
            }
        }
        if ((s16)v > 0) {
            self->unk37C -= (s16)v;
            if ((s16)self->unk37C <= 0) {
                if (Pl_act_state_ck(self) == 1) {
                    self->unk37C = 1;
                    return;
                }
                self->unk37C = 0x2A30;
                fn_80276CE8(self, -0x96);
            }
        }
    }
}

/* 0x802770E0 */
extern "C" u32 pl_act_dash_input_ck(void)
{
    return 0;
}

/* 0x802770E8: initialises the actor's attack-range state - the id/count words, the zeroed tail fields and
 * the 16-byte per-range table. */
extern "C" void Pl_act_set_step_table(_PLW* self, u32 table, s32 arg2)
{
    self->unk318 = table;
    self->unk313 = (s8)arg2;
    self->unk314 = 0;
    self->unk31C = 0;
    self->unk320 = 0;
    self->unk31E = 0;
    for (s16 i = 0; i < 48; i++) {
        self->unk322[i] = 0;
    }
}

/* 0x802771A0: picks the attack-range table for the actor's weapon class and initialises it. */
extern "C" void fn_802771A0(_PLW* self, s32 arg1)
{
    if (self->kind_0x09 != 3) {
        if (self->field_0x002 == 8 && pl_mode8_free_ck() == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_kind8_guard, (s16)arg1);
        } else {
            Pl_act_set_step_table(self, lbl_805BF448[self->field_0x002], (s16)arg1);
        }
    } else {
        if (self->field_0x002 == 8 && pl_mode8_free_ck() == 0) {
            Pl_act_set_step_table(self, (u32)pl_steps_kind8_slot3_guard, (s16)arg1);
        } else {
            Pl_act_set_step_table(self, lbl_805BF46C[self->field_0x002], (s16)arg1);
        }
    }
}

/* 0x80277B44 */
extern "C" void pl_act_set_gauge_arm(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
}

/* 0x80277B4C: scales the actor's attack-range modifier by the two armour skills. */
extern "C" void pl_act_set_gauge_arm_skilled(_PLW* self, s16 arg1)
{
    self->unk396 = arg1;
    if (Pl_cat_skill_ck(self, 28) == 1) {
        self->unk396 = (s16)(self->unk396 * 3);
    } else if (Pl_cat_skill_ck(self, 29) == 1) {
        self->unk396 = (s16)(self->unk396 * 2);
    }
}

/* 0x80277C48 */
extern "C" void pl_act_set_step_time(_PLW* self, s16 arg1)
{
    self->field_0x580 = arg1;
}

/* 0x80277C50 */
extern "C" void pl_act_set_motion_gate(_PLW* self, s16 arg1)
{
    self->field_0x45A = arg1;
}

/* 0x80277FE0 */
extern "C" s32 pl_item_use_free_ck(_PLW* self)
{
    return 1;
}

/* 0x80277FE8 */
extern "C" u32 fn_80277FE8(_PLW* self)
{
    return (u32)(self->field_0x30C - 1) >> 31;
}

/* 0x802784A8 */
extern "C" s32 pl_stance_low_ck(_PLW* self)
{
    return (u32)(self->field_0x30D - 3) >> 31;
}

/* 0x80278564 */
extern "C" void pl_act_arm_flags(_PLW* self, u32 arg1)
{
    self->unk388 |= (u8)arg1;
}

/* 0x80278578 */
extern "C" s32 fn_80278578(_PLW* self, u32 arg1)
{
    return (self->unk388 & (u16)arg1) == 0;
}
} /* namespace s_80276B58 */


/* 0x80276E08 sibling: whether the actor is inside the invulnerability window the suimen skill
 * grants (a +/- one-unit band around its current height). */
u32 Pl_suimen_ck(_PLW* self)
{
    using s_80276B58::lbl_8079A0CC;
    if (self->unk074 != 0) {
        if (self->motion_pos_0x40 >= self->unk064 - lbl_8079A0CC && self->motion_pos_0x40 <= lbl_8079A0CC + self->unk064) {
            return 1;
        }
    }
    return 0;
}

/* 0x802790B4: whether any of the given condition bits is set in the actor's condition word. */
u32 Pl_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3D8 & mask) != 0;
}

/* 0x802790CC: whether any of the given bits is set in the actor's damage-condition word. */
u32 Pl_dm_condition_ck(_PLW* self, u32 mask)
{
    return (self->unk3DC & mask) != 0;
}
namespace s_80276B58 {


/* 0x802790E4 */
extern "C" u32 pl_dm_condition_ex_ck(_PLW* self, u32 mask)
{
    return (self->dm_condition_prev_0x3E0 & mask) != 0;
}

/* 0x80279154 */
extern "C" void pl_attack_up_set(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        if (self->unk448 < arg2) {
            self->unk448 = arg2;
        }
    } else {
        if (self->field_0x449 < arg2) {
            self->field_0x449 = arg2;
        }
    }
}

/* 0x802798FC */
extern "C" s32 pl_reload_needed_ck(_PLW* self)
{
    if ((self->unk269 == 0 || (self->unk269 < self->ammo_capacity_0x26A && self->unk269 < self->magazine_0x270))
        && self->magazine_0x270 > 0) {
        return 1;
    }
    return 0;
}

/* 0x8027A000: adds a signed amount to the actor's charge counter and clamps it to +/-100. */
extern "C" void fn_8027A000(_PLW* self, s32 arg1)
{
    self->gauge_0x64F += (s8)arg1;
    if (self->gauge_0x64F >= 100) {
        self->gauge_0x64F = 100;
    }
    if (self->gauge_0x64F <= -100) {
        self->gauge_0x64F = -100;
    }
}

/* 0x8027A044: adds a signed amount to the actor's aim angle and clamps it to +/-90 degrees. */
extern "C" void fn_8027A044(_PLW* self, s16 arg1)
{
    self->unk5E8 += arg1;
    if (arg1 >= 0) {
        if (self->unk5E8 >= 8192) {
            self->unk5E8 = 8192;
        }
    } else {
        if (self->unk5E8 <= -8192) {
            self->unk5E8 = -8192;
        }
    }
}

/* 0x8027A17C */
extern "C" void pl_camera_lock_clear(_PLW* self)
{
    self->unk5E6 = 0;
    self->unk5E5 = 0;
    self->unk5E8 = 0;
}

/* 0x8027A190 */
extern "C" s32 pl_act_true_ck(_PLW* self)
{
    return 1;
}

/* 0x8027A198 */
extern "C" s32 pl_act_no_68_71_ck(_PLW* self)
{
    if (self->field_0x00A == 0) {
        s32 id = self->act_no;
        if ((u32)(id - 68) <= 1 || id == 71) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027A554 */
extern "C" s32 pl_alt_form_free_ck(_PLW* self)
{
    if (self->field_0x18 == 1 && self->unk5E6 != 0) {
        return 0;
    }
    return 1;
}

/* 0x8027AC00 */
extern "C" void fn_8027AC00(_PLW* self)
{
    self->unk5BB = 1;
}

/* 0x8027AC0C */
extern "C" void pl_act_clear_flag5bb(_PLW* self)
{
    self->unk5BB = 0;
}

/* 0x8027AF34: ticks the actor's residual-velocity timer and integrates its position deltas. */
extern "C" void fn_8027AF34(_PLW* self)
{
    s16 v = self->blend_frames_0x0B4;
    if (v > 0) {
        self->blend_frames_0x0B4--;
        /* unreachable - it makes the compiler reuse the first test's condition register */
        if (v < 0) {
            return;
        }
        self->motion_pos_0x3C += self->blend_step_x_0x09C;
        self->motion_pos_0x40 += self->blend_step_y_0x0A0;
        self->motion_pos_0x44 += self->blend_step_z_0x0A4;
    }
}

/* 0x8027AF80 */
extern "C" s32 fn_8027AF80(u8 dir)
{
    return 1;
}

/* 0x8027C030 */
extern "C" s32 pl_act_kind7_ck(_PLW* self)
{
    if (self->field_0x00A == 7) {
        s32 id = self->act_no;
        if ((u32)(id - 5) <= 1 || id == 2) {
            return 1;
        }
    }
    return 0;
}

/* 0x8027C89C: raises the actor's stored action id to the given one. */
extern "C" void fn_8027C89C(_PLW* self, s16 arg1)
{
    if (self->daze_timer_0x468 < arg1) {
        self->daze_timer_0x468 = arg1;
    }
}

/* 0x8027CA04 */
extern "C" s32 pl_daze_level_ck(_PLW* self, s32 arg1)
{
    if (self->field_0x002 != 7) {
        return 0;
    }
    if (self->daze_timer_0x468 > 0) {
        return 1;
    }
    return self->field_0x384 >= (s16)arg1;
}

/* 0x8027CC2C */
extern "C" u32 pl_camera_close_ck(_PLW* self)
{
    return (self->field_0x5A4 & 0x40) != 0;
}

/* 0x8027CF24 */
extern "C" s32 pl_gauge_icon_ck(_PLW* self)
{
    if (pl_item_page2_ck(self) == 1 && self->unk276 != 0) {
        return 1;
    }
    return 0;
}

/* 0x8027CF70 */
extern "C" s32 pl_condition_ck_20000(_PLW* self)
{
    return Pl_condition_ck(self, 0x20000);
}
} /* namespace s_80276B58 */


/* 0x8027CF78: starts a zanzo (afterimage) trail on the actor's current motion. */
void Pl_zanzo_set(_PLW* self, s32 arg1, u8 arg2)
{
    self->field_0x662 = (s16)arg1;
    self->field_0x65E = arg2;
    self->field_0x664 = Get_motion_no(self);
    self->field_0x666 = 0xFFFF;
}
namespace s_80276B58 {


/* 0x8027D3F0 */
extern "C" void pl_cling_req_set(_PLW* self, u8 arg1)
{
    self->unk3AC |= 1 << arg1;
}

/* 0x8027D4F0 */
extern "C" void fn_8027D4F0(_PLW* self)
{
    self->unk3B4 = 1;
}

/* 0x8027D4FC */
extern "C" s32 pl_shake_off_ck(_PLW* self)
{
    return self->unk3B4 != 0;
}

/* 0x8027D510 */
extern "C" void pl_guard_latch_set(_PLW* self)
{
    self->unk3B5 = 1;
}

/* 0x8027D51C */
extern "C" s32 pl_guard_latch_ck(_PLW* self)
{
    return self->unk3B5 != 0;
}

/* 0x8027D584 */
extern "C" void pl_act_hold_motion_set(_PLW* self, u8* arg1)
{
    self->armed_motion_0x59C = arg1[3] | 0x8000;
    self->unk59E = 0;
    self->unk5A0 = 0;
}

/* 0x80277C58: writes the actor's remaining vertical range from its motion frame data. */
extern "C" void pl_act_set_frame_timer(_PLW* self)
{
    u8* p = ((u8*)self->physics_0x13C);
    f32 v = *(f32*)(p + 72);
    if (v < lbl_8079A084) {
        v = lbl_8079A084;
    }
    self->unk264 = (s16)(*(f32*)(p + 120) - v);
}

/* 0x80277BC4: the attack-range modifier the actor's two armour skills set. */
extern "C" void fn_80277BC4(_PLW* self, u8 arg1)
{
    s32 v;
    if (arg1 == 0) {
        v = 6;
        if (Pl_Skill_ck(self, 0xA1) == 1) {
            v = 10;
        } else if (Pl_Skill_ck(self, 0xA2) == 1) {
            v = 12;
        }
    } else {
        v = 12;
    }
    pl_act_set_gauge_arm(self, v);
}

/* 0x80277EC0: hands the actor's vertical speed to its motion frame and caches the result. */
extern "C" void fn_80277EC0(_PLW* self)
{
    if (self->unk3A2 > 0) {
        mhchar_motion_speed_set(((u8*)self->physics_0x13C) + 4, lbl_8079A084);
    } else if (self->unk3A3 > 0) {
        mhchar_motion_speed_set(((u8*)self->physics_0x13C) + 4, lbl_8079A0C8 * self->unk354);
    } else {
        mhchar_motion_speed_set(((u8*)self->physics_0x13C) + 4, self->unk354);
    }
    self->unk358 = *(f32*)(((u8*)self->physics_0x13C) + 0x60);
}

/* 0x80277F54: whether the actor's current motion is still free to be interrupted. */
extern "C" s32 pl_motion_interruptible_ck(_PLW* self)
{
    if (self->flag_0x30E >= 2) {
        return 0;
    }
    s32 kind = self->kind_0x015;
    if (kind != 9) {
        u8* p = (u8*)get_move_work_adrs(0);
        if (p != 0 && self->area_0x16 == p[0xF6]) {
            return 0;
        }
    } else {
        if (stage_pos_z_low_ck((u8*)self + 60) == 1) {
            return 0;
        }
    }
    return 1;
}

/* 0x802782B8 */
extern "C" s32 pl_spot_use_a_ck(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && fn_80278144(self->area_0x16, (u8*)self + 60, self->water_area_0x5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278450 */
extern "C" s32 pl_spot_use_b_ck(_PLW* self)
{
    if (fn_80277FE8(self) == 1 && pl_area_spot_valid_ck(self->area_0x16, (u8*)self + 60, self->water_area_0x5A6) == 1) {
        return 1;
    }
    return 0;
}

/* 0x80278994: clears the actor's stored per-motion scratch values. */
extern "C" void pl_motion_scratch_clear(_PLW* self)
{
    self->hit_pending_0x39E = 0;
    self->field_0x38A = 0;
    self->field_0x3EC = 0;
    self->field_0x3F8 = 0;
    self->field_0x3FE = 0;
    self->field_0x3F2 = 0;
    self->field_0x40E = 0;
    self->unk414 = 0;
    self->unk41A = 0;
    self->field_0x420 = 0;
    self->unk38C = 0;
    self->unk424 = 0;
    self->unk38E = 0;
    self->unk426 = 0;
    self->unk390 = 0;
    self->unk428 = 0;
    self->unk392 = 0;
    self->unk42A = 0;
    self->unk394 = 0;
    self->unk42C = 0;
}

/* 0x80278B58: starts the actor's dodge/step motion. */
extern "C" void pl_dodge_motion_start(_PLW* self, s32 arg1, s32 arg2)
{
    pl_cond_flags_clear(self, 4);
    pl_cond_flags_clear(self, 16);
    self->health = 0;
    self->unk376 = 0;
    pl_skill_update2(self, 1);
    pl_camera_lock_clear(self);
    pl_act_enter(self, 8, (u16)arg1, (u16)(arg2 | 32));
}

/* 0x80278BE4 */
extern "C" void pl_ride_down_set(_PLW* self)
{
    if (self->field_0x00A == 8) {
        return;
    }
    pl_cond_flags_clear(self, 4);
    pl_cond_flags_clear(self, 16);
    self->health = 0;
    self->unk376 = 0;
    pl_skill_update2(self, 1);
    pl_camera_lock_clear(self);
    if (self->kind_0x09 == 3) {
        pl_dodge_motion_start(self, 1, 0);
    } else {
        pl_dodge_motion_start(self, 0, 0);
    }
}

/* 0x80278C7C */
extern "C" u32 fn_80278C7C(_PLW* self)
{
    if (self->field_0x416 > 0) {
        return 1;
    }
    if (self->field_0x00A == 6) {
        s32 id = self->act_no;
        if ((u32)(id - 46) <= 5 || (u32)(id - 43) <= 1 || id == 73) {
            return 1;
        }
    }
    return 0;
}

/* 0x80278CD0 */
extern "C" u32 fn_80278CD0(_PLW* self)
{
    if (self->field_0x41C > 0) {
        return 1;
    }
    if (self->field_0x00A == 6) {
        s32 id = self->act_no;
        if ((u32)(id - 66) <= 6 || (u32)(id - 63) <= 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x802790FC */
extern "C" s32 pl_call_answer_ck(_PLW* self)
{
    if (self->unk46C == 1 && self->unk46C != self->unk46D && self->unk46E == 0) {
        self->unk46E = 1;
        if (self->field_0x470 == 0) {
            self->field_0x470 = 1800;
            return 1;
        }
        return 0;
    }
    return 0;
}

/* 0x80279194 */
extern "C" u32 pl_defense_up_set(_PLW* self, s32 arg1, s8 arg2)
{
    if (arg1 == 0) {
        self->unk422 = 0;
        if (self->unk44C < arg2) {
            self->unk44C = arg2;
        }
    } else {
        if (self->unk422 > 0) {
            self->unk422 = 0;
            return 0;
        }
        if (self->buff_b_level_0x44D < arg2) {
            self->buff_b_level_0x44D = arg2;
        }
    }
    return 1;
}

/* 0x8027A08C: applies a charge delta to the actor's aim and converts the counter to an angle. */
extern "C" void fn_8027A08C(_PLW* self, s8 arg1)
{
    f32 v;
    self->shell_ang_s_0x583 += arg1;
    if (self->shell_ang_s_0x583 > 100) {
        self->shell_ang_s_0x583 = 100;
    }
    if (self->shell_ang_s_0x583 < -100) {
        self->shell_ang_s_0x583 = -100;
    }
    v = (f32)self->shell_ang_s_0x583 / lbl_8079A0D0;
    if (self->shell_ang_s_0x583 >= 0) {
        v = lbl_8079A0E0 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    } else {
        v = lbl_8079A0E8 * v * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
        self->unk5E8 = (s16)(u16)v;
    }
}

/* 0x8027BE2C */
extern "C" void pl_act_clear_mode5c4(_PLW* self)
{
    if ((self->unk5C4 & 0xF) != 0) {
        self->unk5C4 &= 0xF0;
        eft_pl_weapon_follow_spawn(self);
    }
}

/* 0x8027AC18 */
extern "C" u32 pl_item_menu_open_ck(_PLW* self)
{
    return self->unk5BB != 0;
}

/* 0x8027BDD4 */
extern "C" u32 pl_carry_light_ck(_PLW* self)
{
    if (Pl_Skill_ck(self, 93) == 1) {
        return 1;
    }
    return (u16)pl_carry_item_get(self) == 395;
}

/* 0x8027CBC8: the highest carve-slot value the actor has available. */
extern "C" u8 pl_hit_level_max_get(_PLW* self)
{
    u8 v = 0;
    if (self->unk489 != 0 && self->unk492 != 255) {
        if (v < self->unk4DC) {
            v = self->unk4DC;
        }
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255) {
        if (v < self->unk538) {
            v = self->unk538;
        }
    }
    return v;
}

/* 0x8027D530 */
extern "C" s32 pl_act_grabbed_ck(_PLW* self)
{
    if (Pl_act_ck(self, 0, 64) == 1 && self->unk306 == 206) {
        return 1;
    }
    return 0;
}

/* 0x8027A4EC */
extern "C" s32 pl_aim_act_ck(_PLW* self)
{
    s32 v = 0;
    if (self->field_0x00A == 4) {
        s32 id = self->act_no;
        if ((u32)(id - 19) <= 30 || (u32)(id - 5) <= 8) {
            v = 1;
        }
    }
    if (pl_scan_rows_id252_ck(self) == 1) {
        v = 1;
    }
    return v;
}

/* 0x80279414: looks the given id up in the actor's two reaction tables. */
extern "C" u8* pl_gun_ammo_entry_get(_PLW* self, u8 arg1)
{
    u8* p = gun_ammo_entry_find(equip_weapon_rec_get((u8*)self + 464), arg1);
    if (p != 0) {
        return p;
    }
    u8 n = ((u8*)&self->equipC)[0];
    if (n == 12) {
        u8* q = gun_ammo_entry_find(equip_weapon_rec_get((&self->equipC)), arg1);
        if (q != 0) {
            return q;
        }
    }
    return 0;
}

/* 0x80279360: finds the reaction-table entry the given motion id maps to. */
extern "C" u8* gun_ammo_entry_find(u8* self, u8 arg1)
{
    for (s32 i = 0; i < 4; i++) {
        u8 idx = self[0x18 + i];
        if (idx == 0) {
            break;
        }
        u8* p = lbl_805BF5E0 + idx * 4;
        if (p[0] == arg1) {
            return p;
        }
    }
    return 0;
}

/* 0x80279B84: refreshes the actor's held-item state from its item id. */
extern "C" void pl_held_item_refresh(_PLW* self)
{
    if (pl_item_page2_ck(self) != 0 && self->item_slot_sel != 255) {
        (void)GetItemData((u16)pl_item_slot_item_get(self, (u8)self->item_slot_sel));
        self->held_item_kind_0x26C = pl_item_icon_kind_get((u16)pl_item_slot_item_get(self, (u8)self->item_slot_sel));
        self->magazine_0x270 = pl_item_slot_count_get(self, (u8)self->item_slot_sel);
        self->ammo_capacity_0x26A = fn_80274D98(self, self->held_item_kind_0x26C);
        self->unk269 = 0;
    }
}

/* 0x8027A2A0: whether the actor may still act - master/rage state, the bari timer, the stun flag
 * and the held-item lock all have to agree. */
extern "C" s32 pl_camera_reset_ck(_PLW* self, s32 arg1)
{
    s32 v = 1;
    if (Pl_master_ck(self) == 0) {
        v = 0;
    }
    if (Pl_bari_ck(self, 1) == 0) {
        v = 0;
    }
    if (self->unk5E6 != 0) {
        v = 0;
    }
    if (pl_part_flag_ck(self, 55) == 0 && (u8)arg1 == 0) {
        v = 0;
    }
    return v;
}

/* 0x8027BC48: whether the current motion still takes directional input. */
extern "C" s32 Pl_motion_input_ck(s32 arg1)
{
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return 1;
    }
    s32 x = p[0xFA];
    /* `break` = "this motion still takes directional input"; the loop is the shared `return 1` the
     * retail source reached through a label (the other half, the shared `return 0`, is inlined). */
    do {
        if ((u32)(x - 6) <= 2) {
            break;
        }
        switch (x) {
        case 3:
            if (arg1 != 0) {
                return 0;
            }
            break;
        case 5:
            if (arg1 == 2) {
                return 0;
            }
            break;
        case 4:
            break;
        default:
            return 0;
        }
    } while (0);
    return 1;
}

/* 0x8027CFC0: ticks down the actor's stun timer and clears the stun condition at zero. */
extern "C" void pl_stun_tick(_PLW* self)
{
    if (Pl_master_ck(self) != 0 && self->unk404 > 0) {
        if (Pl_dm_condition_ck(self, 2) == 1) {
            self->unk404 = (s16)(self->unk404 - 90);
        } else {
            self->unk404 = (s16)(self->unk404 - 120);
        }
        if (self->unk404 <= 0) {
            self->unk404 = 0;
            self->unk3DC &= 0xFFFFFFFC;
        }
    }
}

/* 0x8027D050: the actor's stored carve value, scanned out of the item table. */
extern "C" u8 pl_step_se_kind_get(_PLW* self)
{
    s32 i;
    u8 v = 0;
    if (self->unk369 == 0) {
        return 0;
    }
    if ((self->unk364 & 0xE0000007) != 0) {
        for (i = 0; i < 10; i++) {
            if (pl_move_work[self->chunk_ofs][i].carve_0x14 != 0) {
                v = pl_move_work[self->chunk_ofs][i].carve_0x14;
                break;
            }
        }
    }
    return v;
}

/* 0x8027D5A4: applies a signed charge delta to the actor's stored charge, scaled by the two
 * charge skills, and clamps it to 0..100. */
extern "C" void pl_act_add_charge(_PLW* self, s32 arg1)
{
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->field_0x36C += (s16)f;
    if (self->field_0x36C < 0) {
        self->field_0x36C = 0;
    } else if (self->field_0x36C > 100) {
        self->field_0x36C = 100;
    }
}
} /* namespace s_80276B58 */


/* 0x8027CDF0: the gunner's world-space origin, biased by the charge counter. */
void Pl_get_gunner_vec(_PLW* self, _CP_VECTOR* out)
{
    using s_80276B58::lbl_8079A088; using s_80276B58::lbl_8079A0D0; using s_80276B58::lbl_8079A0DC; using s_80276B58::lbl_8079A0E4; using s_80276B58::lbl_8079A110;
    u32 x = self->param_0x54;
    f32 t = (f32)self->gauge_0x64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    out->x = x + (u16)t;
    out->y = self->field_0x058;
    out->z = 0;
}
namespace s_80276B58 {


/* 0x8027CE74: whether the actor's current motion is one of the gunner charge motions. */
extern "C" s32 pl_reload_motion_ck(_PLW* self)
{
    if (pl_item_page2_ck(self) == 1) {
    switch ((u16)Get_motion_no(self)) {
    case 1104:
    case 1114:
    case 1115:
    case 1130:
    case 1131:
    case 1132:
    case 1153:
    case 1163:
    case 1164:
    case 1180:
    case 1181:
    case 1182:
        return 1;
    }
    }
    return 0;
}

/* 0x8027CA48: applies a charge delta to the weapon's charge timer. */
extern "C" void pl_class7_meter_add(_PLW* self, s16 arg1)
{
    if (self->field_0x002 != 7) {
        return;
    }
    self->unk386 += arg1;
    if (arg1 >= 0) {
        self->buff_timer_0x46A = 1800;
        if (self->unk386 >= 100) {
            self->unk386 = 100;
        }
        switch ((u8)pl_stagger_level_get(self)) {
        case 1:
            fn_8027C89C(self, 900);
            break;
        case 2:
            fn_8027C89C(self, 900);
            break;
        case 3:
            fn_8027C89C(self, 900);
            break;
        }
    } else {
        if (self->unk386 < 0) {
            self->unk386 = 0;
        }
    }
}

/* 0x8027CB1C: the carve slot to select, sentinel 255 meaning "none". */
extern "C" u8 pl_hit_result_best_get(_PLW* self)
{
    u8 v = 255;
    if (self->unk489 != 0 && self->unk492 != 255 && (v == 255 || v < self->unk492)) {
        v = self->unk492;
    }
    if (self->unk4E5 != 0 && self->unk4EE != 255 && (v == 255 || v < self->unk4EE)) {
        v = self->unk4EE;
    }
    if (Pl_Skill_ck(self, 24) == 1 && v == 0) {
        v = 2;
    }
    return v;
}

/* 0x8027D40C: the number of set bits in the actor's action-lock word. */
extern "C" s32 fn_8027D40C(_PLW* self)
{
    s32 n = 0;
    for (s32 i = 0; i < 32; i++) {
        if (self->unk3AC & (1 << i)) {
            n++;
        }
    }
    return n;
}

/* 0x8027CD0C: builds the gunner's aim matrix from its rotation and gun position. */
extern "C" void pl_eye_mtx_get(_PLW* self, nw4r::math::MTX34* mtx)
{
    _CP_VECTOR pos;
    nw4r::math::VEC3 v;
    VEC3_ctor(&v);
    eft_rot_vec_copy(&pos, (u8*)self + 84);
    f32 t = (f32)self->gauge_0x64F / lbl_8079A0D0;
    t = lbl_8079A110 * t * lbl_8079A0DC / lbl_8079A0E4 + lbl_8079A088;
    pos.x = pos.x + (u16)t;
    cpSetRotMatrixZXY(&pos, mtx);
    Pl_get_gunner_pos(self, &v, 0);
    mtx->m[0][3] = v.x;
    mtx->m[1][3] = v.y;
    mtx->m[2][3] = v.z;
}
} /* namespace s_80276B58 */


/* 0x8027CC44: the gunner's gun position in actor-local space, biased by the charge counter. */
void Pl_get_gunner_pos(_PLW* self, nw4r::math::VEC3* out, s32 arg2)
{
    using s_80276B58::lbl_8079A0CC; using s_80276B58::lbl_8079A100; using s_80276B58::lbl_8079A104; using s_80276B58::lbl_8079A108; using s_80276B58::lbl_8079A10C;
    if (self->kind_0x09 == 3) {
        out->x = lbl_8079A100;
        out->y = lbl_8079A104;
        out->z = lbl_8079A108;
    } else {
        out->x = lbl_8079A100;
        out->y = lbl_8079A10C;
        out->z = lbl_8079A108;
    }
    if (arg2 != 0) {
        out->z = out->z + lbl_8079A0CC;
    }
    rotVecXYZ(out, (_CP_VECTOR*)((u8*)self + 84));
    out->x = out->x + self->motion_pos_0x3C;
    out->y = out->y + self->motion_pos_0x40;
    out->z = out->z + self->motion_pos_0x44;
}
namespace s_80276B58 {


/* 0x8027AE28: sets the actor's residual-velocity timer from a target position over the given
 * number of frames. */
extern "C" void pl_pos_blend_start(_PLW* self, s32 arg1)
{
    if (vec3_distance((u8*)self + 60, (u8*)self + 144) >= lbl_8079A0F4 || (s16)arg1 == 0) {
        self->blend_frames_0x0B4 = 0;
        self->motion_pos_0x3C = self->target_pos_0x090.x;
        self->motion_pos_0x40 = self->target_pos_0x090.y;
        self->motion_pos_0x44 = self->target_pos_0x090.z;
        self->blend_step_x_0x09C = lbl_8079A084;
        self->blend_step_y_0x0A0 = lbl_8079A084;
        self->blend_step_z_0x0A4 = lbl_8079A084;
    } else {
        self->blend_frames_0x0B4 = arg1;
        self->blend_step_x_0x09C = (self->target_pos_0x090.x - self->motion_pos_0x3C) / (f32)(s16)arg1;
        self->blend_step_y_0x0A0 = (self->target_pos_0x090.y - self->motion_pos_0x40) / (f32)(s16)arg1;
        self->blend_step_z_0x0A4 = (self->target_pos_0x090.z - self->motion_pos_0x44) / (f32)(s16)arg1;
    }
}

/* 0x8027C8B4: applies a signed charge delta to the actor's attack-range counter, with the two
 * charge skills scaling a positive delta. */
extern "C" void pl_act_add_class7_gauge(_PLW* self, s32 arg1)
{
    if (self->field_0x002 != 7) {
        return;
    }
    if ((s16)arg1 < 0 && self->daze_timer_0x468 > 0) {
        return;
    }
    f32 f = (f32)(s16)arg1;
    if (f > lbl_8079A084) {
        if (Pl_Skill_ck(self, 191) == 1) {
            f *= lbl_8079A094;
        } else if (Pl_Skill_ck(self, 192) == 1) {
            f *= lbl_8079A0FC;
        }
    }
    self->field_0x384 += (s16)f;
    if (f >= lbl_8079A084) {
        if (self->field_0x384 >= 100) {
            self->field_0x384 = 100;
            if ((u32)Pl_master_ck(self) == 1 && self->daze_timer_0x468 == 0) {
                ef_inst_spawn(self, 1);
            }
            fn_8027C89C(self, 900);
        }
    } else {
        if (self->field_0x384 < 0) {
            self->field_0x384 = 0;
        }
    }
}
} /* namespace s_80276B58 */


/* 0x8027A2A0 sibling: whether the actor's bari (rage) state covers the given action class. */
u32 Pl_bari_ck(_PLW* self, s32 arg1)
{
    using s_80276B58::lbl_8079A0EC;
    s32 v = 0;
    if (self->field_0x00A == 0) {
        s32 id = self->act_no;
        if ((u32)(id - 169) <= 1 || (u32)(id - 172) <= 1) {
            if (arg1 != 2) {
                v = 1;
            } else {
                s32 m = (u16)Get_motion_no(self);
                if ((m == 314 || m == 365)
                    && Pl_frame_check(self, 1, lbl_8079A0EC, lbl_8079A084) == 1) {
                    v = 1;
                }
            }
        } else if ((id == 171 || id == 174) && (u32)(arg1 - 1) <= 1) {
            v = 1;
        }
    }
    return v;
}
namespace s_80276B58 {


/* 0x802784B8 */
extern "C" s32 fn_802784B8(_PLW* self)
{
    s32 m = (u8)stage_map_kind_get((u8)get_now_mapno());
    switch (m) {
    case 6:
    case 17:
        if (self->area_0x16 == 2) {
            return 0;
        }
        /* falls through */
    default: {
        u8* p = (u8*)get_move_work_adrs(0);
        if (p != 0 && self->area_0x16 == p[0xF6]) {
            return 0;
        }
        break;
    }
    case 9:
        if (stage_pos_z_low_ck((u8*)self + 60) == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278590: the actor's attack-range tier for the current weapon class. */
extern "C" s32 pl_attack_range_tier_get(_PLW* self)
{
    if (pl_item_page2_ck(self) == 1) {
        return 0;
    }
    s16* t = (s16*)(pl_sharpness_tbl[self->field_0x002]
                    + ((*((u8*)self + 0x56A) * 7) << 1));
    s16 v = *((s16*)self + 0x2B6);
    if (v <= t[0]) {
        return 0;
    }
    if (v <= t[1]) {
        return 1;
    }
    if (v <= t[2]) {
        return 2;
    }
    if (v <= t[3]) {
        return 3;
    }
    if (v <= t[4]) {
        return 4;
    }
    return v <= t[5] ? 5 : 6;
}

/* 0x802789EC: resets the actor's whole action state and re-applies the armour skill values. */
extern "C" void pl_skill_update2(_PLW* self, s32 arg1)
{
    pl_motion_scratch_clear(self);
    *(u32*)((u8*)self + 988) = 0;
    *(s16*)((u8*)self + 1042) = 0;
    *(s16*)((u8*)self + 1002) = 0;
    *(s16*)((u8*)self + 1006) = 0;
    *(s16*)((u8*)self + 1024) = 0;
    *(s16*)((u8*)self + 1020) = 0;
    *(s16*)((u8*)self + 1008) = 0;
    *(s16*)((u8*)self + 1012) = 0;
    *(s16*)((u8*)self + 1014) = 0;
    *(s16*)((u8*)self + 1018) = 0;
    *(s16*)((u8*)self + 1026) = 0;
    *(s16*)((u8*)self + 900) = 0;
    *(s16*)((u8*)self + 902) = 0;
    *(s16*)((u8*)self + 1040) = 0;
    *(s16*)((u8*)self + 1048) = 0;
    *(s16*)((u8*)self + 1054) = 0;
    *(s16*)((u8*)self + 1046) = 0;
    *(s16*)((u8*)self + 1052) = 0;
    *(s16*)((u8*)self + 1118) = 0;
    *(s16*)((u8*)self + 1120) = 0;
    *(s16*)((u8*)self + 1122) = 0;
    *(s16*)((u8*)self + 1124) = 0;
    *(s16*)((u8*)self + 1126) = 0;
    *(u8*)((u8*)self + 1096) = 0;
    *(u8*)((u8*)self + 1100) = 0;
    *(u8*)((u8*)self + 1097) = 0;
    *(u8*)((u8*)self + 1101) = 0;
    *(s16*)((u8*)self + 1098) = 0;
    *(s16*)((u8*)self + 1102) = 0;
    *(s16*)((u8*)self + 1028) = 0;
    *(s16*)((u8*)self + 1030) = 0;
    *(s16*)((u8*)self + 1032) = 0;
    *(s16*)((u8*)self + 1034) = 0;
    *(s16*)((u8*)self + 1036) = 0;
    *(s16*)((u8*)self + 1108) = 0;
    *(u8*)((u8*)self + 1104) = 0;
    *(s16*)((u8*)self + 1112) = 0;
    *(u8*)((u8*)self + 1106) = 0;
    *(s16*)((u8*)self + 1128) = 0;
    *(s16*)((u8*)self + 1130) = 0;
    *(s16*)((u8*)self + 1070) = 0;
    *(s16*)((u8*)self + 1080) = 0;
    *(s16*)((u8*)self + 1072) = 0;
    *(s16*)((u8*)self + 1082) = 0;
    *(s16*)((u8*)self + 1074) = 0;
    *(s16*)((u8*)self + 1084) = 0;
    *(s16*)((u8*)self + 1076) = 0;
    *(s16*)((u8*)self + 1086) = 0;
    *(s16*)((u8*)self + 1078) = 0;
    *(s16*)((u8*)self + 1088) = 0;
    *(s16*)((u8*)self + 1058) = 0;
    *(s16*)((u8*)self + 1114) = 0;
    *(s16*)((u8*)self + 1116) = 0;
    if ((u32)Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 39) == 1)) {
        self->unk448 = (s8)fn_802753E4(self, 4);
        self->unk44C = (s8)fn_802753E4(self, 5);
    }
    fn_80278D1C(self);
}

/* 0x8027BCE0: the highest of the actor's 24 stored item ids that is in the carve set. */
extern "C" u32 pl_carry_item_get(_PLW* self)
{
    s32 id;
    u16 v = 0xFFFF;
    for (s32 i = 0; i < 24; i++) {
        if (self->slot_id[i].value > 0) {
            id = self->slot_id[i].item_id;
            if ((u32)(id - 381) <= 1 || id == 139 || id == 395) {
                v = (u16)id;
            }
        }
    }
    return v;
}
} /* namespace s_80276B58 */


/* 0x80279670: the shell multiplier adjustment the two shell tables give. */
f32 Get_Shell_rate_adj(_PLW* self, u8 arg1)
{
    using s_80276B58::equip_weapon_rec_get; using s_80276B58::lbl_8079A0D0;
    s16 raw = ((PlWeaponRec*)equip_weapon_rec_get(&self->equipB))->bonus_0x0A;
    f32 v = (f32)raw;
    if (((u8*)&self->equipC)[0] == 12) {
        raw = ((PlWeaponRec*)equip_weapon_rec_get(&self->equipC))->bonus_0x0A;
        v = v * (f32)raw / lbl_8079A0D0;
    }
    return v / lbl_8079A0D0;
}
namespace s_80276B58 {


/* 0x80277C94: ground-height probe along the actor's facing, returning whether the probe hit
 * inside the requested band. */
extern "C" u32 pl_ground_probe_ahead(_PLW* self, f32 ahead, f32 height, nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 v;
    u8 hit;
    VEC3_ctor(&v);
    out->x = lbl_8079A084;
    v.x = lbl_8079A084;
    v.y = ahead + height;
    v.z = lbl_8079A0C4;
    rotVecY(&v, self->field_0x058);
    v.x = v.x + self->motion_pos_0x3C;
    v.y = v.y + self->motion_pos_0x40;
    v.z = v.z + self->motion_pos_0x44;
    f32 h = GetGroundHit2(&v, -0x1006, self->area_0x16, &hit);
    f32 c = self->motion_pos_0x40 + ahead;
    if (h >= c && h <= height + c && hit != 0) {
        out->x = h;
        return 1;
    }
    return 0;
}

/* 0x8027AF88: refreshes the actor's shell/clutch state from its move work. */
extern "C" void fn_8027AF88(_PLW* self)
{
    *(s16*)((u8*)self + 1474) = 30;
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (isServerSelectState(cockpit_supply_notice_spawn(*(s8*)(q + 1505))) == 1) {
        *(s16*)((u8*)self + 1626) = 900;
        *((u8*)self + 1625) = 1;
        lb_entry_handover_send(1, (u8)my_player_no(), *(u8*)(q + 1505));
        return;
    }
    *(s16*)((u8*)self + 1626) = 0;
    *((u8*)self + 1625) = 0;
    pl_item_add(self, *(u16*)(q + 1506 + *(s8*)(q + 1505) * 4),
                *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4));
    *(u32*)(q + 1668 + (*(s8*)(q + 1505) >> 5) * 4) |= 1 << (*(s8*)(q + 1505) & 31);
    cockpit_pointer_target_set(*(u16*)(q + 1506 + *(s8*)(q + 1505) * 4));
    *(s16*)(q + 1506 + *(s8*)(q + 1505) * 4) = 0;
    *(s16*)(q + 1508 + *(s8*)(q + 1505) * 4) = 0;
}

/* 0x80278D1C: rebuilds the actor's two condition word sets from its stored state, its current action id
 * and the two related-player flags the small helpers report. */
extern "C" void fn_80278D1C(_PLW* self)
{
    s32 bits = 0;
    if (Pl_master_ck(self) != 0) {
        if (*(s16*)((u8*)self + 1002) != 0) {
            bits |= 1;
        }
        if (self->unk448 + self->field_0x449 > 0) {
            bits |= 0x10;
        }
        if (*(s16*)((u8*)self + 900) >= 100) {
            bits |= 0x80000000;
        }
        if (self->unk44C + self->buff_b_level_0x44D > 0) {
            bits |= 0x40;
        }
        if (self->field_0x00A == 6) {
            s32 id = self->act_no;
            if ((u32)(id - 0x1F) > 3) {
                if ((u32)(id - 0x1C) > 2) {
                    if (id != 0x1B && id != 0x23) {
                    } else {
                        bits |= 8;
                    }
                } else {
                    bits |= 2;
                }
            } else {
                bits |= 4;
            }
        }
        if (*(s16*)((u8*)self + 1124) > 0) {
            bits |= 0x400;
        }
        if (*(s16*)((u8*)self + 1126) > 0) {
            bits |= 0x800;
        }
        if (*(s16*)((u8*)self + 1118) > 0) {
            bits |= 0x4000;
        }
        if (fn_80278C7C(self) == 1) {
            bits |= 0x100;
        }
        if (fn_80278CD0(self) == 1) {
            bits |= 0x200;
        }
        if (*(s16*)((u8*)self + 1058) > 0) {
            bits |= 0x80;
        }
        if (*(s16*)((u8*)self + 1040) > 0) {
            bits |= 0x20000;
        }
        if (*(s16*)((u8*)self + 1112) > 0) {
            bits |= 0x10000;
        }
        if (*(u32*)((u8*)self + 944) != 0) {
            bits |= 0x40000;
        }
        if (*(s16*)((u8*)self + 1108) > 0) {
            bits |= 0x2000;
        }
        self->unk3D8 = bits;
        self->unk3DC &= 0xFFF003FF;
        if (*(s16*)((u8*)self + 1070) > 0 || (s16)fn_802753E4(self, 6) < 0) {
            self->unk3DC |= 0x400;
        }
        if (*(s16*)((u8*)self + 1072) > 0 || (s16)fn_802753E4(self, 7) < 0) {
            self->unk3DC |= 0x800;
        }
        if (*(s16*)((u8*)self + 1074) > 0 || (s16)fn_802753E4(self, 8) < 0) {
            self->unk3DC |= 0x1000;
        }
        if (*(s16*)((u8*)self + 1076) > 0 || (s16)fn_802753E4(self, 9) < 0 || fn_80278C7C(self) == 1) {
            self->unk3DC |= 0x2000;
        }
        if (*(s16*)((u8*)self + 1078) > 0 || (s16)fn_802753E4(self, 10) < 0) {
            self->unk3DC |= 0x4000;
        }
        if (*(s16*)((u8*)self + 1080) > 0 || (s16)fn_802753E4(self, 6) > 0) {
            self->unk3DC |= 0x8000;
        }
        if (*(s16*)((u8*)self + 1082) > 0 || (s16)fn_802753E4(self, 7) > 0) {
            self->unk3DC |= 0x10000;
        }
        if (*(s16*)((u8*)self + 1084) > 0 || (s16)fn_802753E4(self, 8) > 0) {
            self->unk3DC |= 0x20000;
        }
        if (*(s16*)((u8*)self + 1086) > 0 || (s16)fn_802753E4(self, 9) > 0) {
            self->unk3DC |= 0x40000;
        }
        if (*(s16*)((u8*)self + 1088) > 0 || (s16)fn_802753E4(self, 10) > 0) {
            self->unk3DC |= 0x80000;
        }
    }
}

extern "C" s32 pl_id_bit_ck(_PLW*, s32);
} /* namespace s_80276B58 */

void sysSE_req(s32);
namespace s_80276B58 {

/* 0x8027B918: steps the actor's clutch state machine from the pad edge events the move work reports. */
extern "C" void fn_8027B918(_PLW* self)
{
    u8* p;
    u8* q;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (*(u8*)((u8*)self + 0x5BD) == 0) {
        return;
    }
    p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    switch (*(u8*)((u8*)self + 0x5C0)) {
    case 0:
        sysSE_req(5);
        *(s16*)((u8*)self + 0x5C2) = 30;
        (*(u8*)((u8*)self + 0x5C0))++;
        return;
    case 1: {
        if (*(s16*)((u8*)self + 0x5C2) != 0) {
            (*(s16*)((u8*)self + 0x5C2))--;
        }
        if (*(s16*)((u8*)self + 0x5C2) != 0) {
            break;
        }
        if ((u32)pl_id_bit_ck(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5BD) = 0;
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x16) == 1) {
            switch (*(u8*)((u8*)self + 0x5BF)) {
            case 0:
                sysSE_req(0);
                *(u8*)((u8*)self + 0x5BD) = 1;
                return;
            case 1:
                if ((u32)Pl_motion_input_ck(0) != 1) {
                    if ((u32)quest_flag_80_ck(NULL) == 1) {
                        if (quest_item_work_notify(1) == 1 || quest_item_work_notify(2) == 1) {
                            (*(u8*)((u8*)self + 0x5C0))++;
                            *(u8*)(q + 0x68D) = 1;
                            sysSE_req(0);
                            return;
                        }
                        sysSE_req(2);
                        return;
                    }
                    sysSE_req(2);
                    return;
                }
                sysSE_req(2);
                return;
            }
        } else {
            if ((u32)pl_id_bit_ck(self, 0x12) == 1) {
                sysSE_req(6);
                if (*(u8*)((u8*)self + 0x5BF) == 0) {
                    *(u8*)((u8*)self + 0x5BF) = 1;
                } else {
                    (*(u8*)((u8*)self + 0x5BF))--;
                }
            }
            if ((u32)pl_id_bit_ck(self, 0x13) == 1) {
                sysSE_req(6);
                if (*(u8*)((u8*)self + 0x5BF) >= 1) {
                    *(u8*)((u8*)self + 0x5BF) = 0;
                    return;
                }
                (*(u8*)((u8*)self + 0x5BF))++;
                return;
            }
        }
        break;
    }
    case 2:
        if (*(s8*)(q + 0x68D) == 0) {
            if ((u32)pl_id_bit_ck(self, 0x17) == 1) {
                *(u8*)(q + 0x68D) = 1;
                sysSE_req(1);
                return;
            }
            if ((u32)pl_id_bit_ck(self, 0x16) == 1) {
                if ((s32)*(u8*)((u8*)self + 0x5BF) == 1 && (u32)Pl_motion_input_ck(0) != 1) {
                    sysSE_req(0);
                    *(u8*)((u8*)self + 0x5BD) = 1;
                    return;
                }
            } else if ((u32)pl_id_bit_ck(self, 0x15) == 1) {
                *(u8*)(q + 0x68D) = 1;
                sysSE_req(3);
                return;
            }
        } else {
            if ((u32)pl_id_bit_ck(self, 0x16) == 1 || (u32)pl_id_bit_ck(self, 0x17) == 1) {
                *(u8*)((u8*)self + 0x5C0) = 1;
                sysSE_req(1);
                return;
            }
            if ((u32)pl_id_bit_ck(self, 0x14) == 1) {
                *(u8*)(q + 0x68D) = 0;
                sysSE_req(3);
            }
        }
        break;
    }
}

extern "C" s32 Pl_item_timer_get(_PLW*, u16);

/* 0x8027B358: steps the shell-selection state machine - the clutch index the move work stores plus the
 * three charge levels - from the pad edge events. */
extern "C" void fn_8027B358(_PLW* self)
{
    u16 sp8;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (*(u8*)((u8*)self + 0x5BE) == 0) {
        return;
    }
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if ((u32)Pl_motion_input_ck(0) == 1) {
        sysSE_req(1);
        *(u8*)((u8*)self + 0x5BE) = 0;
        return;
    }
    em_work_slot_pair_get((u16)*(s8*)(q + 0x68C), (s16*)&sp8);
    s32 r28 = quest_element_remaining_get((u16)*(s8*)(q + 0x68C));
    *(u16*)(q + 0x696) = 0;
    switch (*(u8*)((u8*)self + 0x5C0)) {
    case 0:
        *(u8*)(q + 0x68E) = 1;
        *(u8*)((u8*)self + 0x5C0) = 1;
        return;
    case 1:
        if ((u32)pl_id_bit_ck(self, 0x13) == 1) {
            (*(u8*)(q + 0x68C))++;
            if (*(s8*)(q + 0x68C) >= 3) {
                *(u8*)(q + 0x68C) = 2;
                return;
            }
            sysSE_req(6);
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x12) == 1) {
            (*(u8*)(q + 0x68C))--;
            if (*(s8*)(q + 0x68C) < 0) {
                *(u8*)(q + 0x68C) = 0;
                return;
            }
            sysSE_req(6);
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x16) == 1) {
            switch (quest_element_supply_state_get(*(s8*)(q + 0x68C))) {
            case 0:
                sysSE_req(2);
                return;
            case 1:
                sysSE_req(2);
                return;
            case 2:
                if (Pl_item_timer_get(self, sp8) < 1) {
                    sysSE_req(2);
                    return;
                }
                *(u8*)((u8*)self + 0x5C0) = 2;
                *(u8*)(q + 0x68E) = 1;
                sysSE_req(0);
                return;
            }
        } else if ((u32)pl_id_bit_ck(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            *(u8*)((u8*)self + 0x5BE) = 0;
            return;
        }
        break;
    case 2:
        if (quest_element_supply_state_get(*(s8*)(q + 0x68C)) != 2) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            return;
        }
        if (*(s8*)(q + 0x68E) > r28) {
            sysSE_req(6);
            *(s8*)(q + 0x68E) = r28;
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x13) == 1) {
            if (*(s8*)(q + 0x68E) > 1) {
                sysSE_req(6);
                *(u16*)(q + 0x696) |= 2;
                (*(u8*)(q + 0x68E))--;
                return;
            }
            *(u8*)(q + 0x68E) = 1;
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x12) == 1) {
            u8 t31 = *(u8*)(q + 0x68E);
            if ((s8)t31 < Pl_item_timer_get(self, sp8)) {
                if ((s8)t31 < r28) {
                    sysSE_req(6);
                    *(u16*)(q + 0x696) |= 1;
                    (*(u8*)(q + 0x68E))++;
                    return;
                }
                *(s8*)(q + 0x68E) = r28;
                return;
            }
        } else if ((u32)pl_id_bit_ck(self, 0x15) == 1) {
            u8 t31b = *(u8*)(q + 0x68E);
            if (r28 < Pl_item_timer_get(self, sp8)) {
                *(s8*)(q + 0x68E) = r28;
            } else {
                *(s8*)(q + 0x68E) = Pl_item_timer_get(self, sp8);
            }
            if ((s8)t31b != *(s8*)(q + 0x68E)) {
                sysSE_req(6);
                *(u16*)(q + 0x696) |= 1;
                return;
            }
        } else {
            if ((u32)pl_id_bit_ck(self, 0x14) == 1) {
                if (*(s8*)(q + 0x68E) != 1) {
                    sysSE_req(6);
                    *(u16*)(q + 0x696) |= 2;
                }
                *(u8*)(q + 0x68E) = 1;
                return;
            }
            if ((u32)pl_id_bit_ck(self, 0x16) == 1) {
                sysSE_req(0);
                *(u8*)((u8*)self + 0x5C0) = 3;
                *(u8*)(q + 0x68D) = 0;
                return;
            }
            if ((u32)pl_id_bit_ck(self, 0x17) == 1) {
                sysSE_req(1);
                *(u8*)((u8*)self + 0x5C0) = 1;
                return;
            }
        }
        break;
    case 3:
        if (quest_element_supply_state_get(*(s8*)(q + 0x68C)) != 2) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 1;
            return;
        }
        if (*(s8*)(q + 0x68E) > r28) {
            *(s8*)(q + 0x68E) = r28;
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x13) == 1) {
            if (*(s8*)(q + 0x68D) < 1) {
                sysSE_req(6);
                (*(u8*)(q + 0x68D))++;
                return;
            }
            *(u8*)(q + 0x68D) = 1;
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x12) == 1) {
            if (*(s8*)(q + 0x68D) > 0) {
                sysSE_req(6);
                (*(u8*)(q + 0x68D))--;
                return;
            }
            *(u8*)(q + 0x68D) = 0;
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x16) == 1) {
            if (*(s8*)(q + 0x68D) == 1) {
                sysSE_req(1);
                *(u8*)((u8*)self + 0x5C0) = 2;
                *(u8*)(q + 0x68D) = 0;
                return;
            }
            sysSE_req(8);
            *(u8*)((u8*)self + 0x5C0) = 1;
            quest_element_item_use(self, sp8, *(s8*)(q + 0x68E));
            return;
        }
        if ((u32)pl_id_bit_ck(self, 0x17) == 1) {
            sysSE_req(1);
            *(u8*)((u8*)self + 0x5C0) = 2;
            return;
        }
        break;
    default:
        *(u8*)((u8*)self + 0x5BE) = 0;
        break;
    }
}

extern "C" s32 pl_item_room_get(_PLW*, u16, s32);

/* 0x8027B0BC: steps the handling-direction state machine - an 8-way direction on a five-row grid of
 * stored action entries - from the pad edge events. */
extern "C" void fn_8027B0BC(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if ((s32)*(u8*)((u8*)self + 0x5BC) == 0) {
        return;
    }
    u8* p = (u8*)get_move_work_adrs(0);
    if (p == 0) {
        return;
    }
    u8* q = *(u8**)(p + 220);
    if (q == 0) {
        return;
    }
    if (*(s16*)((u8*)self + 0x5C2) != 0) {
        (*(s16*)((u8*)self + 0x5C2))--;
    }
    if (*(s16*)((u8*)self + 0x65A) != 0) {
        (*(s16*)((u8*)self + 0x65A))--;
        if (*(s16*)((u8*)self + 0x65A) == 0) {
            *(u8*)((u8*)self + 0x659) = 0;
        }
    }
    if ((s32)*(u8*)((u8*)self + 0x659) != 0) {
        return;
    }
    if (*(s16*)((u8*)self + 0x5C2) != 0) {
        return;
    }
    if ((u32)pl_id_bit_ck(self, 0x17) == 1) {
        *(u8*)((u8*)self + 0x5BC) = 0;
        sysSE_req(1);
        return;
    }
    if ((u32)pl_id_bit_ck(self, 0x14) == 1) {
        sysSE_req(3);
        s32 v = *(u8*)(q + 0x5E1);
        if ((s32)((s8)v & 7) != 0) {
            (*(u8*)(q + 0x5E1))--;
        } else {
            (*(u8*)(q + 0x5E1)) += 7;
        }
    } else if ((u32)pl_id_bit_ck(self, 0x15) == 1) {
        sysSE_req(3);
        s32 v = *(u8*)(q + 0x5E1);
        if ((s32)((s8)v & 7) != 7) {
            (*(u8*)(q + 0x5E1))++;
        } else {
            (*(u8*)(q + 0x5E1)) -= 7;
        }
    }
    if ((u32)pl_id_bit_ck(self, 0x13) == 1) {
        sysSE_req(3);
        (*(u8*)(q + 0x5E1)) += 8;
        s32 w = *(u8*)(q + 0x5E1);
        if ((s8)w >= 40) {
            (*(u8*)(q + 0x5E1)) -= 40;
        }
    } else if ((u32)pl_id_bit_ck(self, 0x12) == 1) {
        sysSE_req(3);
        (*(u8*)(q + 0x5E1)) -= 8;
        s32 w = *(u8*)(q + 0x5E1);
        if ((s8)w < 0) {
            (*(u8*)(q + 0x5E1)) += 40;
        }
    }
    u8 dir = *(u8*)(q + 0x5E1);
    if (((1 << ((s8)dir & 0x1F)) & *(u32*)(q + 0x684 + ((s8)dir >> 5) * 4)) == 0) {
        if ((u32)fn_8027AF80(*(u8*)(q + 0x5E1)) == 1) {
            if ((u32)pl_id_bit_ck(self, 0x16) == 1) {
                u16 item = *(u16*)(q + 0x5E2 + (s8)*(u8*)(q + 0x5E1) * 4);
                if ((s32)item != 0) {
                    s16 val = *(s16*)(q + 0x5E4 + (s8)*(u8*)(q + 0x5E1) * 4);
                    if (val > 0) {
                        if (pl_item_room_get(self, item, val) >= val) {
                            sysSE_req(8);
                            fn_8027AF88(self);
                            return;
                        }
                        sysSE_req(2);
                    }
                }
            }
        }
    }
}
} /* namespace s_80276B58 */


/* The two side structures the attack-subsystem setters fill in: `_HIT_W` is the attack entry the
 * caller uses, `_HIT_DATA` the hit record it came from. Only the offsets this unit touches are named. */
struct _HIT_DATA {
    u8 unk00[0x12];
    s8 unk12;
};
namespace s_80276B58 {


extern "C" u8 pl_frame_eft_color_get(_PLW*);
extern "C" s16 fn_80273ED8(_PLW*, s32, s32);
} /* namespace s_80276B58 */

s32 Pl_critical_get(_PLW*);

/* 0x80277284: fills in the attack entry an incoming hit produces - the skill-granted hit flags, the
 * hit rate multipliers and the critical roll - from the attack flags and the actor's skills. */
void Pl_attack_set_sub(_PLW* self, _HIT_DATA* hit, _HIT_W* data, u16 flags)
{
    using s_80276B58::fn_80273ED8; using s_80276B58::pl_stagger_level_get; using s_80276B58::pl_frame_eft_color_get; using s_80276B58::lbl_8079A08C; using s_80276B58::lbl_8079A0A8; using s_80276B58::lbl_8079A0AC; using s_80276B58::lbl_8079A0B0; using s_80276B58::lbl_8079A0B4;
    s32 f8;
    s32 f10;
    data->attack_flags_0x31 = 0xF;
    data->attack_kind_0x32 = 1;
    f10 = flags & 0x10;
    if (f10 != 0 && Pl_Skill_ck(self, 0xC7) == 1) {
        s8 v = hit->unk12;
        if ((s32)v < 0) {
            data->field_0x04E = (s8)-v;
        }
    }
    if ((s8)data->field_0x04E > 0 && Pl_cat_skill_ck(self, 0x28) == 1) {
        data->field_0x04E = (s8)(lbl_8079A0A8 * (f32)(s8)data->field_0x04E);
    }
    if ((s32)(flags & 2) != 0) {
        s32 sel;
        s16 t;
        f8 = flags & 8;
        if (f8 != 0 && pl_frame_eft_color_get(self) == 1) {
            sel = 1;
        } else {
            sel = 0;
            if (Pl_Skill_ck(self, 0xC9) == 1) {
                sel = 1;
            }
        }
        t = fn_80273ED8(self, 7, sel);
        if (t > 0) {
            data->field_0x048 |= 0x10;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 8, sel);
        if (t > 0) {
            data->field_0x048 |= 0x20;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 9, sel);
        if (t > 0) {
            data->field_0x048 |= 0x40;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 0xA, sel);
        if (t > 0) {
            data->field_0x048 |= 0x80;
            data->field_0x04A = (u8)t;
        }
        t = fn_80273ED8(self, 0xB, sel);
        if (t > 0) {
            data->field_0x048 |= 0x200;
            data->field_0x04A = (u8)t;
        }
        if (f8 != 0 && pl_frame_eft_color_get(self) == 3) {
            data->field_0x048 = 0x80;
            t = fn_80273ED8(self, 0xF, 1);
            if (t <= 0) {
                data->field_0x04A = 1;
            } else {
                data->field_0x04A = (u8)t;
            }
        }
        if ((s32)(*(u16*)((u8*)self + 0xB6) % 3) == 0) {
            t = fn_80273ED8(self, 0xC, sel);
            if (t > 0) {
                data->field_0x048 |= 2;
                data->field_0x04A = (u8)t;
            }
            t = fn_80273ED8(self, 0xD, sel);
            if (t > 0) {
                data->field_0x048 |= 4;
                data->field_0x04A = (u8)t;
            }
            t = fn_80273ED8(self, 0xE, sel);
            if (t > 0) {
                data->field_0x048 |= 1;
                data->field_0x04A = (u8)t;
            }
            if (f8 != 0 && pl_frame_eft_color_get(self) == 2) {
                data->field_0x048 = (u16)((data->field_0x048 & 0xFFFC) | 4);
                t = fn_80273ED8(self, 0x10, 1);
                if (t <= 0) {
                    data->field_0x04A = 1;
                } else {
                    data->field_0x04A = (u8)t;
                }
            }
        }
        if ((s32)data->field_0x048 != 0) {
            data->field_0x05A = 0xA;
        }
    }
    if (self->field_0x002 == 7 && (flags & 1) != 0) {
        switch (pl_stagger_level_get(self)) {
        case 1:
            data->life_0x40 = (s16)(lbl_8079A0AC * (f32)data->life_0x40);
            break;
        case 2:
            data->life_0x40 = (s16)(lbl_8079A0A8 * (f32)data->life_0x40);
            break;
        case 3:
            data->life_0x40 = (s16)(lbl_8079A0B0 * (f32)data->life_0x40);
            break;
        }
    }
    if ((flags & 8) != 0) {
        switch ((u8)pl_frame_eft_color_get(self)) {
        case 0:
            data->life_0x40 = (s16)(lbl_8079A0B4 * (f32)data->life_0x40);
            break;
        case 1:
            if ((s32)data->field_0x048 != 0) {
                u8 c = data->field_0x04A;
                if ((s32)c != 0) {
                    data->field_0x04A = (u8)(s32)(lbl_8079A0B4 * (f32)c);
                }
            }
            break;
        }
    }
    if ((flags & 0x20) != 0 && Pl_cat_skill_ck(self, 0x1F) == 1) {
        data->life_0x40 = (s16)(data->life_0x40 * 5);
    }
    if ((flags & 0x40) != 0 && Pl_cat_skill_ck(self, 0x20) == 1) {
        data->life_0x40 = (s16)(data->life_0x40 * 5);
    }
    if ((flags & 0x14) != 0) {
        s32 crit = Pl_critical_get(self);
        s32 sc;
        if (f10 != 0 && Pl_Skill_ck(self, 0xB7) == 1) {
            crit = 100;
        }
        sc = (s16)crit;
        if (sc != 0 && (u32)*(u8*)((u8*)self + 0x56B) >= 1) {
            if (sc > 0) {
                if ((s32)(*(u16*)((u8*)self + 0xB6) % 100) < sc) {
                    data->life_0x40 = (s16)(lbl_8079A0B4 * (f32)data->life_0x40);
                    data->field_0x04C |= 0x80;
                }
            } else {
                if ((s32)(*(u16*)((u8*)self + 0xB6) % 100) < -sc) {
                    if (data->life_0x40 > 0) {
                        data->life_0x40 = (s16)(lbl_8079A08C * (f32)data->life_0x40);
                        if (data->life_0x40 == 0) {
                            data->life_0x40 = 1;
                        }
                        data->field_0x04C |= 0x40;
                    }
                }
            }
        }
    }
}

/* The move-work records `get_move_work_adrs(2)` hands back: an array of actor-shaped entries on a
 * 0xB20 stride. Only the offsets this unit touches are named. */
struct _MOVE_WORK {
    u8 unk000;
    u8 unk001[0x16 - 0x01];
    u8 unk016;
    u8 unk017[0x3DC - 0x17];
    u32 unk3DC;
    u8 unk3E0[0x3EA - 0x3E0];
    s16 unk3EA;
    u8 unk3EC[0x404 - 0x3EC];
    s16 unk404;
    s16 unk406;
    s16 unk408;
    s16 unk40A;
    s16 unk40C;
    u8 unk40E[0x412 - 0x40E];
    s16 unk412;
    u8 unk414[0x42E - 0x414];
    s16 unk42E;
    s16 unk430;
    s16 unk432;
    s16 unk434;
    s16 unk436;
    s16 unk438;
    s16 unk43A;
    s16 unk43C;
    s16 unk43E;
    s16 unk440;
    u8 unk442[0x44A - 0x442];
    s16 unk44A;
    u8 unk44C[0x44E - 0x44C];
    s16 unk44E;
    u8 unk450[0x464 - 0x450];
    s16 unk464;
    s16 unk466;
};
namespace s_80276B58 {


extern "C" void pl_act_add_hold_gauge(_MOVE_WORK*, s32);
extern "C" void pl_defense_up_mark(_MOVE_WORK*, s32, s32);
extern "C" void fn_8027D6C0(_MOVE_WORK*, s32, s32);

/* 0x8027D0D4: applies one scripted action to every move-work record of type 2 whose kind byte matches
 * `arg0` - the per-action clamps and the state-helper calls of the attack/critical chain. */
extern "C" void pl_area_buff_apply(u8 arg0, u8 arg1)
{
    s32 i;
    u8* r29;
    s32 max;
    r29 = (u8*)get_move_work_adrs(2);
    max = (u16)get_move_work_max(2);
    for (i = 0; i < max; i++) {
        _MOVE_WORK* p = (_MOVE_WORK*)r29;
        if (p->unk000 != 0 && p->unk016 == arg0) {
            switch (arg1) {
            case 0:
                pl_act_add_hold_gauge(p, 0x32);
                ef_inst_spawn((_PLW*)p, 0);
                break;
            case 1:
                pl_attack_up_set((_PLW*)p, 1, 0xA);
                p->unk44A = 0x1518;
                ef_inst_spawn((_PLW*)p, 1);
                break;
            case 2:
                if (pl_defense_up_set((_PLW*)p, 1, 0x14) == 1) {
                    p->unk44E = 0x1518;
                }
                ef_inst_spawn((_PLW*)p, 2);
                break;
            case 3:
            case 6:
                if (arg1 == 3) {
                    pl_act_add_hold_gauge(p, 0x14);
                } else {
                    pl_act_add_hold_gauge(p, 0x32);
                }
                ef_inst_spawn((_PLW*)p, 0);
                break;
            case 4:
            case 7:
                if (arg1 == 4) {
                    pl_attack_up_set((_PLW*)p, 1, 3);
                } else {
                    pl_attack_up_set((_PLW*)p, 1, 5);
                }
                p->unk44A = 0x1518;
                ef_inst_spawn((_PLW*)p, 1);
                break;
            case 5:
            case 8:
                if (arg1 == 5) {
                    if (pl_defense_up_set((_PLW*)p, 1, 0xA) == 1) {
                        p->unk44E = 0x1518;
                    }
                } else if (pl_defense_up_set((_PLW*)p, 1, 0x14) == 1) {
                    p->unk44E = 0x1518;
                }
                ef_inst_spawn((_PLW*)p, 2);
                break;
            case 9:
                p->unk3EA = 0;
                ef_inst_spawn((_PLW*)p, 4);
                break;
            case 10:
                if (p->unk466 < 0x2328) {
                    p->unk466 = 0x2328;
                }
                ef_inst_spawn((_PLW*)p, 7);
                break;
            case 11:
                if (p->unk464 < 0x2328) {
                    p->unk464 = 0x2328;
                }
                ef_inst_spawn((_PLW*)p, 6);
                break;
            case 12:
                if (p->unk412 < 0x1518) {
                    p->unk412 = 0x1518;
                }
                ef_inst_spawn((_PLW*)p, 5);
                break;
            case 13:
                pl_defense_up_mark(p, 1, 0x1518);
                fn_8027D6C0(p, 1, 0x1518);
                ef_inst_spawn((_PLW*)p, 9);
                break;
            case 14:
                p->unk42E = 0;
                if (p->unk438 < 0x1518) {
                    p->unk438 = 0x1518;
                }
                p->unk430 = 0;
                if (p->unk43A < 0x1518) {
                    p->unk43A = 0x1518;
                }
                p->unk432 = 0;
                if (p->unk43C < 0x1518) {
                    p->unk43C = 0x1518;
                }
                p->unk434 = 0;
                if (p->unk43E < 0x1518) {
                    p->unk43E = 0x1518;
                }
                p->unk436 = 0;
                if (p->unk440 < 0x1518) {
                    p->unk440 = 0x1518;
                }
                i = 5;
                p->unk404 = 0;
                p->unk408 = 0;
                p->unk406 = 0;
                p->unk40A = 0;
                p->unk40C = 0;
                p->unk3DC &= 0xFFFFFC00;
                ef_inst_spawn((_PLW*)p, 9);
                break;
            }
        }
        r29 += 0xB20;
    }
}

/* 0x8027BE4C: maps the actor's five stored status durations onto the condition word and the shell
 * timers it exposes. */
extern "C" void pl_status_duration_map(_PLW* self)
{
    s16 t;
    u32 bits;
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (Pl_Skill_ck(self, 0xC8) == 1) {
        return;
    }
    t = self->unk38C;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFFC;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 2;
            self->unk404 = 0x1C2;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 1;
            self->unk404 = 0x1C2;
        }
    }
    t = self->unk38E;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFF3;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 8;
            *(s16*)((u8*)self + 0x406) = 0x708;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 4;
            *(s16*)((u8*)self + 0x406) = 0x384;
        }
    }
    t = self->unk390;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFFCF;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x20;
            *(s16*)((u8*)self + 0x408) = 0xE10;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 0x10;
            *(s16*)((u8*)self + 0x408) = 0x708;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        }
    }
    t = self->unk392;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFF3F;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x80;
            *(s16*)((u8*)self + 0x40A) = 0x708;
        } else if (t >= 0x1F) {
            self->unk3DC = bits | 0x40;
            *(s16*)((u8*)self + 0x40A) = 0x384;
        }
    }
    t = self->unk394;
    if (t > 0) {
        bits = self->unk3DC & 0xFFFFFCFF;
        self->unk3DC = bits;
        if (t >= 0x33) {
            self->unk3DC = bits | 0x200;
            *(s16*)((u8*)self + 0x40C) = 0xE10;
            *(s16*)((u8*)self + 0x442) = 0xB4;
            return;
        }
        if (t >= 0x1F) {
            self->unk3DC = bits | 0x100;
            *(s16*)((u8*)self + 0x40C) = 0x708;
            *(s16*)((u8*)self + 0x442) = 0xB4;
        }
    }
}

extern "C" void pl_gauge_ceiling_grow(_PLW*, s16, s32);
extern "C" void pl_stagger_budget_add(_PLW*, s16);
extern "C" s16 fn_8026FF20(_PLW*);

/* 0x8027885C: resets the actor's shell/ammo timers and, when the relevant skill is on, reseeds the
 * three shell tables from the actor's current state. */
extern "C" void pl_skill_update(_PLW* self, s32 arg1, s32 arg2)
{
    *(s16*)((u8*)self + 0x37A) = 0x258;
    *(s16*)((u8*)self + 0x372) = 0x64;
    *(s16*)((u8*)self + 0x380) = 0x64;
    if ((u32)Pl_master_ck(self) == 1 && (arg1 == 0 || Pl_cat_skill_ck(self, 0x27) == 1)) {
        pl_gauge_ceiling_grow(self, (s16)fn_802753E4(self, 1), 0);
        fn_80276CE8(self, (s16)fn_802753E4(self, 2));
        pl_stagger_budget_add(self, (s16)fn_802753E4(self, 3));
    }
    pl_gauge_ceiling_grow(self, fn_8026FF20(self), 0);
    *(s16*)((u8*)self + 0x36C) = 0x32;
    *(s16*)((u8*)self + 0x36E) = 0x96;
    if (arg2 == 0) {
        s16 v;
        *(s16*)((u8*)self + 0x378) = *(s16*)((u8*)self + 0x37A);
        *(s16*)((u8*)self + 0x37C) = 0x2A30;
        *(u8*)((u8*)self + 0x39E) = 0;
        *(s16*)((u8*)self + 0x38A) = 0;
        v = *(s16*)((u8*)self + 0x372);
        *(s16*)((u8*)self + 0x370) = v;
        *(s16*)((u8*)self + 0x376) = v;
        *(s16*)((u8*)self + 0x37E) = *(s16*)((u8*)self + 0x380);
    }
}

/* 0x802791FC: the shell-level cap one equipment slot contributes, before the actor's skill modifiers
 * are applied. */
extern "C" s32 pl_gun_reload_tier_get(_PLW* self, u8 arg1)
{
    if ((s32)pl_item_page2_ck(self) == 0) {
        return 1;
    }
    s32 a = *(u8*)(lbl_805BF538 + 1 + arg1 * 4);
    s32 b = *(u8*)(equip_weapon_rec_get(&self->equipB) + 9);
    if (((u8*)&self->equipD)[0] == 0xD) {
        b += *(u8*)(equip_weapon_rec_get(&self->equipD) + 9);
    }
    if (Pl_Skill_ck(self, 0x27) == 1) {
        b = (s16)(b + 2);
    }
    if (Pl_Skill_ck(self, 0x28) == 1) {
        b = (s16)(b + 3);
    }
    if (Pl_Skill_ck(self, 0x29) == 1) {
        b = (s16)(b + 4);
    }
    if (Pl_Skill_ck(self, 0x2A) == 1) {
        b = (s16)(b - 1);
    }
    if (Pl_Skill_ck(self, 0x2B) == 1) {
        b = (s16)(b - 2);
    }
    if (Pl_Skill_ck(self, 0x2C) == 1) {
        b = (s16)(b - 3);
    }
    a -= b;
    if ((s16)a <= 4) {
        return 0;
    }
    return ((s16)a <= 7) ? 1 : 2;
}

extern "C" { extern u8 lbl_805C6100[]; }
extern "C" { extern u8 lbl_80792150; }
} /* namespace s_80276B58 */


/* 0x80279720: the shell "bure" type (0/1/2) the actor's equipped shell and its skills resolve to. */
u32 Get_Shell_bure_type(_PLW* self, u8 arg1)
{
    using s_80276B58::equip_weapon_rec_get; using s_80276B58::lbl_805C6100; using s_80276B58::lbl_80792150;
    u8* t31 = lbl_805BFCD8 + *(u8*)(equip_weapon_rec_get(&self->equipB) + 0x10) * 2;
    s32 v29 = t31[1];
    v29 += *(s8*)(lbl_805BF538 + 3 + arg1 * 4);
    s32 r29;
    if (((u8*)&self->equipC)[0] == 0xC) {
        v29 -= (&lbl_80792150)[*(u8*)(equip_weapon_rec_get(&self->equipC) + 0x10)];
    }
    if ((s8)v29 <= 5) {
        r29 = 0;
    } else {
        r29 = ((s8)v29 <= 15) ? 1 : 2;
    }
    if (Pl_Skill_ck(self, 0xAC) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 1) {
            if ((u8)r29 != 0) {
                r29 -= 1;
            }
        } else {
            r29 = 0;
        }
    } else if (Pl_Skill_ck(self, 0xAB) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 0 && (u8)r29 != 0) {
            r29 -= 1;
        }
    } else if (Pl_Skill_ck(self, 0xAD) == 1) {
        if (Pl_cat_skill_ck(self, 0x1E) == 1) {
            r29 = 2;
        } else if ((u8)r29 < 2) {
            r29 += 1;
        }
    } else if (Pl_cat_skill_ck(self, 0x1E) == 1 && (u8)r29 < 2) {
        r29 = (u8)((u8)r29 + 1);
    }
    u8 t3 = t31[0];
    return lbl_805C6100[(u8)r29 + (t3 * 4 - t3)];
}
namespace s_80276B58 {


/* 0x80279490: the shell capacity one equipment slot contributes after the actor's skill modifiers,
 * mapped onto the 0-3 capacity class the caller uses. */
extern "C" s32 pl_shell_class_get(_PLW* self, u8 arg1)
{
    if ((s32)pl_item_page2_ck(self) == 0) {
        return 2;
    }
    s32 a = *(u8*)(lbl_805BF538 + arg1 * 4);
    u8* p = pl_gun_ammo_entry_get(self, arg1);
    if (p != 0) {
        return (s16)(*(u8*)(p + 3) + 3);
    }
    s32 b = 0;
    if (pl_gun_ammo_entry_get(self, arg1) == 0) {
        b = *(u8*)(equip_weapon_rec_get(&self->equipB) + 7);
        if (((u8*)&self->equipD)[0] == 0xD) {
            b += *(u8*)(equip_weapon_rec_get(&self->equipD) + 7);
        }
        if (Pl_Skill_ck(self, 0x2D) == 1) {
            b += 2;
        } else if (Pl_Skill_ck(self, 0x2E) == 1) {
            b += 3;
        } else if (Pl_Skill_ck(self, 0x2F) == 1) {
            b = (s16)(b + 4);
        }
        if (Pl_Skill_ck(self, 0x30) == 1) {
            b -= 1;
        } else if (Pl_Skill_ck(self, 0x31) == 1) {
            b -= 2;
        } else if (Pl_Skill_ck(self, 0x32) == 1) {
            b = (s16)(b - 3);
        }
    }
    a -= b;
    s32 r3 = 2;
    if ((s16)a <= 0) {
        a = 0;
    }
    if ((s16)a <= 8) {
        r3 = 0;
    } else if ((s16)a <= 0xA) {
        r3 = 1;
    }
    if ((u32)(arg1 - 0x27) <= 2) {
        if (r3 == 0) {
            return 3;
        }
        return (r3 == 1) ? 8 : 9;
    }
    return r3;
}

extern "C" u32 pl_part_flag2_ck(_PLW*, s32);
extern "C" void pl_aim_motion_set(_PLW*, u16, u16);
extern "C" u32 pl_target_lock_ck(_PLW*);
extern "C" u32 pl_act4_phase_ck(_PLW*, s32);
extern "C" void camera_aim_zoom_get(_PLW*, u8*, u8*, f32*);
extern "C" u8 get_cfg(u8, s32);
extern "C" { extern u8 lbl_805C6118[]; }
extern "C" { extern const f32 lbl_8079A080; }
extern "C" { extern const f32 lbl_8079A0F0; }

/* 0x8027A57C: resolves one motion's stick input into movement - the four pad-direction flags, the direction and
 * magnitude the move table interpolates for the current speed, and the per-motion write-back. */
extern "C" void pl_motion_stick_move(_PLW* self, u16 arg1, u8 arg2)
{
    u8 sp9;
    u8 sp8;
    f32 spC;
    s32* t;
    s32 mag, dir, speed, step;
    u8 f27, f26, f25, f24;
    f27 = 0;
    f26 = 0;
    f25 = 0;
    f24 = 0;
    mag = 0;
    dir = 0;
    step = 0;
    speed = *(s32*)((u8*)self + 0xA8);

    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (arg1 != (u16)Get_motion_no(self)) {
        return;
    }
    if ((s8)self->unk36A != 0) {
        return;
    }
    if ((s32)self->unk5E5 != 0 || (s32)self->unk5E6 != 0) {
        self->unk5E5 = 0xA;
    }
    for (;;) {
        if ((s32)self->unk5E5 == 0 && (s32)self->unk5E6 == 0) {
            if ((u32)pl_id_bit_ck(self, 0xB) == 1) {
                break;
            }
            if (arg2 == 0) {
                if (!(self->field_0x00A == 4 || pl_target_lock_ck(self) != 0)) {
                    break;
                }
            } else if (arg2 == 2) {
                if (pl_part_flag_ck(self, 0x37) == 0) {
                    break;
                }
            }
        }
        switch (arg2) {
        case 0:
            if (pl_target_lock_ck(self) != 0 ||
                ((s32)self->unk5E6 == 0 && (u32)pl_act4_phase_ck(self, 0) == 1)) {
                u8 c;
                u8 r;
                if (pl_part_flag2_ck(self, 0) != 0) {
                    f25 = 1;
                } else if (pl_part_flag2_ck(self, 1) != 0) {
                    f24 = 1;
                }
                if (pl_part_flag2_ck(self, 2) != 0) {
                    f27 = 1;
                } else if (pl_part_flag2_ck(self, 3) != 0) {
                    f26 = 1;
                }
                r = get_cfg(self->chunk_ofs, 2);
                if (r == 1 || r == 3) {
                    c = f25;
                    f25 = f24;
                    f24 = c;
                }
                if ((u8)(r + 0xFE) <= 1) {
                    c = f27;
                    f27 = f26;
                    f26 = c;
                }
                mag = 5;
                if ((s32)(f26 + f24 + (f27 + f25)) != 0) {
                    step = 0x180;
                }
            }
            break;
        case 2:
            if ((s32)self->unk5E6 == 0) {
                u8 r;
                u8 c;
                if (pl_part_flag_ck(self, 0x38) != 0) {
                    f25 = 1;
                } else if (pl_part_flag_ck(self, 0x39) != 0) {
                    f24 = 1;
                }
                if (pl_part_flag_ck(self, 0x3A) != 0) {
                    f27 = 1;
                } else if (pl_part_flag_ck(self, 0x3B) != 0) {
                    f26 = 1;
                }
                r = get_cfg(self->chunk_ofs, 2);
                if (r == 1 || r == 3) {
                    c = f25;
                    f25 = f24;
                    f24 = c;
                }
                if ((u8)(r + 0xFE) <= 1) {
                    c = f27;
                    f27 = f26;
                    f26 = c;
                }
                mag = 5;
                if ((s32)(f26 + f24 + (f27 + f25)) != 0) {
                    step = 0x200;
                }
            }
            break;
        default:
            break;
        }
        if (mag == 0) {
            /* `mag` is still 0 only when no case arm fired: that is this unit's rule-8 replacement for the
             * `block_53` edge, and the pad scan below is what retail's label reached. */
            if ((s32)self->unk5E5 != 0 || (s32)self->unk5E6 != 0) {
                u16 w = *(u16*)((u8*)self + 0xC8);
                u8 r;
                u8 c;
                if ((s32)(w & 0x3C00) != 0 && (u16)*(u16*)((u8*)self + 0xD4) >= 0x32) {
                    if ((s32)(w & 0x2000) != 0) {
                        f25 = 1;
                    } else if ((s32)(w & 0x1000) != 0) {
                        f24 = 1;
                    }
                    if ((s32)(w & 0x800) != 0) {
                        f27 = 1;
                    } else if ((s32)(w & 0x400) != 0) {
                        f26 = 1;
                    }
                    r = get_cfg(self->chunk_ofs, 9);
                    if (r == 1 || r == 3) {
                        c = f25;
                        f25 = f24;
                        f24 = c;
                    }
                    if ((u8)(r + 0xFE) <= 1) {
                        c = f27;
                        f27 = f26;
                        f26 = c;
                    }
                    spC = lbl_8079A080;
                    camera_aim_zoom_get(self, &sp9, &sp8, &spC);
                    mag = 1;
                    step = 0x4C;
                    t = (s32*)lbl_805C6118;
                    while (t[0] != -1) {
                        if ((s32)*(u16*)((u8*)self + 0xD4) >= t[0]) {
                            f32 a = (f32)t[1];
                            f32 m = spC * ((f32)t[2] - a);
                            mag = (s32)(a + m);
                            f32 b = (f32)t[3];
                            f32 s = spC * ((f32)t[4] - b);
                            step = (s32)(b + s);
                            break;
                        }
                        t += 5;
                    }
                }
            }
        {
            s32 c = f25 + f24;
                if (c != 0 && (s32)(f27 + f26) != 0) {
                    step = (s32)(lbl_8079A0F0 * (f32)step);
                    mag = (s32)(lbl_8079A0F0 * (f32)mag);
                    if (mag < 1) {
                        mag = 1;
                    }
                }
                if (c != 0) {
                    dir = mag;
                    if ((s32)f25 == 0) {
                        dir = -mag;
                    }
                    self->unk5E5 = 5;
                }
                if ((s32)f27 != 0) {
                    self->unk5E5 = 0xA;
                    speed += step;
                }
                if ((s32)f26 != 0) {
                    self->unk5E5 = 0xA;
                    speed -= step;
                }
            }
        }
        break;
    }
    switch (arg2) {
    case 0:
        *(s32*)((u8*)self + 0xA8) = speed;
        fn_8027A000(self, dir);
        switch (arg1) {
        case 0x3E9:
            pl_aim_motion_set(self, 0x406, 0x407);
            return;
        case 0x41A:
            pl_aim_motion_set(self, 0x438, 0x439);
            return;
        }
        break;
    case 1:
        *(s32*)((u8*)self + 0xA8) = speed;
        fn_8027A044(self, (s16)(dir << 6));
        return;
    case 2: {
        *(s32*)((u8*)self + 0xA8) = speed;
        u16 m = *(u16*)((u8*)self + 0x38);
        u16 d = speed - m;
        if ((u32)(d - 0x4001) <= 0x3FFF) {
            *(s32*)((u8*)self + 0xA8) = m + 0x4000;
        } else if ((u32)(d - 0x8000) <= 0x3FFF) {
            *(s32*)((u8*)self + 0xA8) = m - 0x4000;
        }
        fn_8027A08C(self, (s8)dir);
        break;
    }
    }
}

/* The callees of the bodies below, under the map's spellings. */
extern "C" void hit_flag_set__FP6_HIT_WUl(void*, u32);
extern "C" void hit_flags_clear(void*);
extern "C" void hud_item_msg_push(s32, s32, s32);
extern "C" void lb_event_request(s32);
extern "C" u16 ran_suu__Fl(s32);
extern "C" void item_pair_copy(void*, void*);
extern "C" u32 fn_80274DCC(_PLW*, u8);
extern "C" s32 fn_8027DC90(void);
extern "C" u32 ai_npc_player_near_ck(s32, f32);
extern "C" void mtx34_identity(void*);
extern "C" void mtx34_rot_xyz_deg(void*, f32, f32, f32);
extern "C" void mtx34_concat_assign(void*, void*);
extern "C" u8 pl_slot_motion_allow_get(_PLW*, u8);
extern "C" void pl_joint_mtx_get(_PLW*, s32, void*);
extern "C" void pl_chr_set_motion_default(_PLW*, s32, u16, s32, s32);
extern "C" void pl_model_set_value(_PLW*, s32, f32);
extern "C" f32 pl_rig_get_motion_frame(_PLW*);
} /* namespace s_80276B58 */


void mulVecMat(nw4r::math::VEC3*, nw4r::math::MTX34*);
void setVector3(nw4r::math::VEC3*, f32, f32, f32);
namespace s_80276B58 {


extern "C" { extern const f32 lbl_8079A0C0; }
extern "C" { extern const f32 lbl_8079A0D4; }
extern "C" { extern const f32 lbl_8079A0D8; }
extern "C" { extern const f32 lbl_8079A0F8; }
extern "C" { extern u8 lbl_805C6168[]; }
extern "C" { extern u8 lbl_805C61D4[]; }

/* 0x80277DAC: whether a ground-height probe along the actor's fractional facing clears the band. */
extern "C" u32 pl_ground_band_ck(_PLW* self, f32 ahead, f32 band, s32 above)
{
    nw4r::math::VEC3 v;
    u8 hit;
    VEC3_ctor(&v);
    v.x = lbl_8079A084;
    v.y = lbl_8079A084;
    v.z = ahead;
    rotVecY(&v, self->field_0x058);
    v.x = v.x + self->motion_pos_0x3C;
    v.y = v.y + self->motion_pos_0x40;
    v.z = v.z + self->motion_pos_0x44;
    f32 h = GetGroundHit2(&v, (u32)-2, self->area_0x16, &hit);
    if ((s32)hit == 0) {
        return 0;
    }
    f32 c = self->motion_pos_0x40 + band;
    if (above == 0) {
        if (h < c) {
            return 1;
        }
    } else {
        if (h > c) {
            return 1;
        }
    }
    return 0;
}

/* 0x80277FF8: whether the given action may start on the current map/weapon combination. */
extern "C" s32 fn_80277FF8(u8 arg0, u8 arg1, s32 arg2)
{
    s32 t = arg2 & 0x7F;
    switch (arg0) {
    case 1:
        if (((u32)(arg1 - 2) <= 4U || (s32)(u8)arg1 == 0xB) && t != 0) {
            return 0;
        }
        break;
    case 2:
        if (((s32)(u8)arg1 == 6 || (s32)(u8)arg1 == 9 || (s32)(u8)arg1 == 0xB) && t != 0) {
            return 0;
        }
        break;
    case 3:
        switch (arg1) {
        case 1:
        case 8:
            if ((u32)t > 1U) {
                return 0;
            }
            break;
        case 5:
        case 6:
        case 9:
            if (t != 0) {
                return 0;
            }
            break;
        }
        break;
    case 4:
        if ((u32)(arg1 - 1) <= 5U && t != 0) {
            return 0;
        }
        break;
    case 5:
        if (((s32)(u8)arg1 == 3 || (s32)(u8)arg1 == 5 || (s32)(u8)arg1 == 8 || (s32)(u8)arg1 == 10) &&
            t != 0) {
            return 0;
        }
        break;
    case 8:
    case 9:
    case 10:
        if (t != 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278144: whether the given action may start in the player's current map/move-work state. */
extern "C" u32 fn_80278144(u8 arg0, u8* arg1, u8 arg2)
{
    u32 m = stage_map_kind_get(get_now_mapno());
    if (fn_80277FF8((u8)m, arg0, arg2) == 0) {
        return 0;
    }
    /* `default:` first and `case 9:` last keeps the two bodies in retail's address order - the else arm
     * is out of line, and this is the rule-8 shape for the two labelled exits retail's source had. */
    switch ((s32)(u8)m) {
    default: {
        u8* w = (u8*)get_move_work_adrs(0);
        if (w != 0 && arg0 == *(u8*)(w + 0xF6)) {
            return 0;
        }
        if ((u32)((u8)m - 6) <= 1) {
            return 0;
        }
        switch ((s32)(u8)m) {
        case 1:
            if ((s32)arg0 == 7 || (s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 2:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 3:
            if ((s32)arg0 == 2 || (s32)arg0 == 4 || (s32)arg0 == 8) {
                return 0;
            }
            break;
        case 5:
            if ((s32)arg0 == 0xA) {
                return 0;
            }
            break;
        case 10:
            return 0;
        }
        break;
    }
    case 9:
        if (stage_pos_z_low_ck(arg1) == 1U) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80278310: the same map/move-work gate as 80278144, for the smaller action set. */
extern "C" u32 pl_area_spot_valid_ck(u8 arg0, u8* arg1, u8 arg2)
{
    u32 m = stage_map_kind_get(get_now_mapno());
    if (fn_80277FF8((u8)m, arg0, arg2) == 0) {
        return 0;
    }
    /* `default:` first and `case 9:` last keeps the two bodies in retail's address order - the else arm
     * is out of line, and this is the rule-8 shape for the two labelled exits retail's source had. */
    switch ((s32)(u8)m) {
    default: {
        u8* w = (u8*)get_move_work_adrs(0);
        if (w != 0 && arg0 == *(u8*)(w + 0xF6)) {
            return 0;
        }
        if ((u32)((u8)m - 6) <= 1) {
            return 0;
        }
        switch ((s32)(u8)m) {
        case 1:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 2:
            if ((s32)arg0 == 0xB) {
                return 0;
            }
            break;
        case 5:
            if ((s32)arg0 == 0xA) {
                return 0;
            }
            break;
        case 10:
            return 0;
        }
        break;
    }
    case 9:
        if (stage_pos_z_low_ck(arg1) == 1U) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x80277974: builds the attack entry an incoming hit produces and runs the shared attack set-up. */
extern "C" void pl_hit_attack_build(_PLW* self, _HIT_W* hit, u8* base, u16 idx, s32* ids, u16 flags)
{
    u8* p = (u8*)((u8*)self->physics_0x13C);
    u8* d = base + idx * 0x1A;
    *(s32*)((u8*)hit + 0x08) = ids[*(u8*)(d + 0xF)];
    hit_flags_clear(hit);
    if ((flags & 1) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x728);
    } else {
        hit_flag_set__FP6_HIT_WUl(hit, 0x708);
    }
    if (self->field_0x002 == 1) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x800);
    }
    if ((flags & 0x80) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x1000);
    }
    if ((flags & 0x100) != 0) {
        hit_flag_set__FP6_HIT_WUl(hit, 0x2000);
    }
    *(u16*)((u8*)hit + 0x18) = Get_motion_no(self);
    *(s16*)((u8*)hit + 0x1A) = 0;
    *(s16*)((u8*)hit + 0x1C) = 0;
    *(s16*)((u8*)hit + 0x1E) = 0;
    f32 f = *(f32*)(p + 0x48) - lbl_8079A0C0;
    if (f < lbl_8079A084 || (*(u32*)(p + 0x50) & 1) != 0) {
        f = lbl_8079A084;
    }
    f32 g = *(f32*)(p + 0x78);
    f32 t = (f32)*(s16*)(d + 0);
    f32 u = (f32)*(s16*)(d + 2);
    *(f32*)((u8*)hit + 0x38) = t;
    *(f32*)((u8*)hit + 0x3C) = u;
    if (*(s16*)(d + 0) != 0) {
        f32 x = t + (f - g);
        *(f32*)((u8*)hit + 0x38) = x;
        if (x < lbl_8079A084) {
            f32 y = u + x;
            *(f32*)((u8*)hit + 0x3C) = y;
            *(f32*)((u8*)hit + 0x38) = lbl_8079A084;
            if (y <= lbl_8079A084) {
                *(u8*)((u8*)hit + 0x05) = 0;
                return;
            }
        }
    }
    hit_data_apply((_HIT_W*)hit, (::_HIT_DATA*)d);
    Pl_attack_set_sub(self, (_HIT_DATA*)d, hit, flags);
}

/* 0x80278674: adds a signed amount to the actor's stamina pool, with the two armour-skill modifiers. */
extern "C" void pl_sharpness_add(_PLW* self, s16 arg1, u8 arg2)
{
    s16 v = arg1;
    s32 changed = 0;
    if (Pl_master_ck(self) != 0 && pl_item_page2_ck(self) != 1U) {
        if (v < 0 && arg2 == 0) {
            if (Pl_Skill_ck(self, 0x15) == 1U) {
                if (v == -1 && (ran_suu__Fl(1) & 1) != 0) {
                    return;
                }
                v = (s16)((((s32)((u32)v >> 31)) + v) >> 1);
                if (v == 0) {
                    v = -1;
                }
            } else if (Pl_Skill_ck(self, 0x16) == 1U) {
                v = (s16)(v * 2);
            }
        }
        *(s16*)((u8*)self + 0x56C) += v;
        if (*(s16*)((u8*)self + 0x56C) <= 0) {
            *(s16*)((u8*)self + 0x56C) = 0;
        }
        s16 lim = *(s16*)((u8*)self + 0x56E);
        if (*(s16*)((u8*)self + 0x56C) > lim) {
            *(s16*)((u8*)self + 0x56C) = lim;
            changed = 1;
        }
        s32 m = pl_attack_range_tier_get(self);
        u8 old = *(u8*)((u8*)self + 0x56B);
        if ((u8)m != old) {
            if (old > (u8)m) {
                hud_item_msg_push(1, 4, 0);
                lb_event_request(0x15);
            } else {
                hud_item_msg_push(1, 5, 0);
            }
            *(u8*)((u8*)self + 0x56B) = m;
        }
        if (changed != 0) {
            hud_item_msg_push(1, 6, 0);
        }
    }
}

/* 0x8027993C: the first item slot the actor may use, scanned round the 33-entry shell ring. */
extern "C" u16 pl_item_slot_next_get(_PLW* self, u16 arg1, u8 arg2)
{
    u16 slot;
    s32 i;
    if (pl_item_page2_ck(self) == 0) {
        return 0xFF;
    }
    u8 ring[33 * 4];
    for (i = 0; i < 0x18; i++) {
        item_pair_copy(ring + i * 4, (u8*)self + 0x278 + i * 4);
    }
    for (i = 0; i < 9; i++) {
        s16 j = (s16)i;
        item_pair_copy(ring + (j + 0x18) * 4, (u8*)self + 0x278 + (j + 0x1A) * 4);
    }
    slot = arg1;
    if ((slot & 0x80) != 0) {
        slot = (u16)((slot & 0x7F) + 0x18);
    }
    if ((s32)slot >= 0x21) {
        slot = 0;
    }
    switch (arg2) {
    case 0:
        slot = (u16)((slot + 1) % 33);
        break;
    case 1:
        slot = (u16)((slot == 0) ? 32 : slot - 1);
        break;
    }
    for (i = 0; i < 0x21; i++) {
        u8* e = ring + slot * 4;
        if (*(u16*)(e + 0) != 0 && *(s16*)(e + 2) > 0) {
            u8* it = (u8*)GetItemData(*(u16*)(e + 0));
            if ((it[2] & 8) != 0 && it[0] == 1 &&
                fn_80274DCC(self, (u8)pl_item_icon_kind_get(*(u16*)(e + 0))) == 1U) {
                if ((u32)slot >= 0x18U) {
                    slot = (u16)((slot - 0x18) | 0x80);
                }
                return slot;
            }
        }
        switch (arg2) {
        case 0:
        case 2:
            slot = (u16)((slot + 1) % 33);
            break;
        case 1:
        case 3:
            slot = (u16)((slot == 0) ? 32 : slot - 1);
            break;
        }
    }
    return 0xFF;
}

/* 0x80279C20: refreshes the actor's held-shell state each frame - the ring position, the shell search,
 * and the save/restore of the fields it rewrites. */
extern "C" void fn_80279C20(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return;
    }
    if (pl_item_page2_ck(self) == 0) {
        return;
    }
    if (Pl_motion_input_ck(1) == 1U) {
        return;
    }
    if (Pl_bari_ck(self, 1) == 1U) {
        return;
    }
    u8 a = self->field_0x00A;
    if ((u32)(a - 8) <= 1U) {
        return;
    }
    switch (a) {
    case 6: {
        s32 id = self->act_no;
        if (id == 0x1B) {
            return;
        }
        if (id == 0x23) {
            return;
        }
        break;
    }
    case 4: {
        s32 id = self->act_no;
        if (id >= 0x17) {
            if (id >= 0x32) {
                break;
            }
            if (id >= 0x1A) {
                return;
            }
        } else {
            if (id >= 0x0E) {
                if (id >= 0x13) {
                    return;
                }
                break;
            }
            if (id < 5) {
                break;
            }
        }
        if (*(u8*)((u8*)self + 5) <= 2) {
            return;
        }
        break;
    }
    }
    if (*(u8*)((u8*)self + 0x26B) == 0) {
        *(u8*)((u8*)self + 0x26B) = 1;
        *(u8*)((u8*)self + 0x26D) = self->held_item_kind_0x26C;
        *(s16*)((u8*)self + 0x272) = *(s16*)((u8*)self + 0x270);
        *(u8*)((u8*)self + 0x275) = *(u8*)((u8*)self + 0x26A);
        *(u8*)((u8*)self + 0x274) = self->unk269;
    }
    if (pl_id_bit_ck(self, 0xB) == 1U) {
        if (pl_id_bit_ck(self, 0xD) == 1U) {
            if (self->item_slot_sel == 0xFF) {
                self->item_slot_sel = pl_item_slot_next_get(self, 0, 3);
            } else {
                self->item_slot_sel = pl_item_slot_next_get(self, 1, 0);
            }
            pl_held_item_refresh(self);
            if (self->item_slot_sel != 0xFF) {
                *(u8*)((u8*)self + 0x309) = (u8)(*(u8*)((u8*)self + 0x309) | 8);
                sysSE_req(6);
            }
        } else if (pl_id_bit_ck(self, 0xC) == 1U) {
            if (self->item_slot_sel == 0xFF) {
                self->item_slot_sel = pl_item_slot_next_get(self, 0, 2);
            } else {
                self->item_slot_sel = pl_item_slot_next_get(self, 0, 0);
            }
            pl_held_item_refresh(self);
            if (self->item_slot_sel != 0xFF) {
                *(u8*)((u8*)self + 0x309) = (u8)(*(u8*)((u8*)self + 0x309) | 0x10);
                sysSE_req(6);
            }
        }
    } else if (self->unk26B != 0) {
        self->unk26B = 0;
        if (self->ammo_icon_kind_0x26D != self->held_item_kind_0x26C) {
            pl_held_item_refresh(self);
        } else {
            *(s16*)((u8*)self + 0x270) = *(s16*)((u8*)self + 0x272);
            *(u8*)((u8*)self + 0x26A) = *(u8*)((u8*)self + 0x275);
            self->unk269 = *(u8*)((u8*)self + 0x274);
        }
    }
    if (self->ammo_icon_kind_0x26D == self->held_item_kind_0x26C) {
        *(s16*)((u8*)self + 0x270) = *(s16*)((u8*)self + 0x272);
        *(u8*)((u8*)self + 0x26A) = *(u8*)((u8*)self + 0x275);
        self->unk269 = *(u8*)((u8*)self + 0x274);
    }
}

/* 0x80279EBC: starts the shell-swap motion, scaled by the actor's stored shell count. */
extern "C" void pl_aim_motion_set(_PLW* self, u16 arg1, u16 arg2)
{
    s32 v = *(s8*)((u8*)self + 0x64F);
    if (v == 0) {
        pl_model_set_value(self, 0, lbl_8079A080);
        pl_model_set_value(self, 1, lbl_8079A084);
        return;
    }
    f32 f = (f32)v;
    *(u8*)((u8*)self + 0x64E) = 1;
    u16 m;
    if (v > 0) {
        if (arg1 == 0x581) {
            m = 0x580;
            *(u8*)((u8*)self + 0x64E) = 0;
        } else {
            m = arg1;
        }
    } else {
        if (arg2 == 0x582) {
            m = 0x580;
            *(u8*)((u8*)self + 0x64E) = 0;
        } else {
            m = arg2;
        }
        f = f * lbl_8079A0D4;
    }
    pl_chr_set_motion_default(self, 1, m, 0, (s32)(lbl_8079A0C0 + pl_rig_get_motion_frame(self)));
    f32 t = f * lbl_8079A0D8;
    pl_model_set_value(self, 0, lbl_8079A080 - t);
    pl_model_set_value(self, 1, t);
}

/* 0x8027A340: whether the actor may act at all this frame - master/bari state, the stun flag and the
 * per-action-class exceptions. */
extern "C" u32 pl_target_lock_ck(_PLW* self)
{
    s32 ok = 1;
    if (Pl_master_ck(self) == 0) {
        ok = 0;
    }
    if (pl_item_page2_ck(self) == 0) {
        ok = 0;
    }
    if (self->unk5E6 != 0) {
        ok = 0;
    }
    if (pl_part_flag_ck(self, 0x1D) == 0) {
        if (self->field_0x00A == 4) {
            s32 id = self->act_no;
            if (id != 0x13 && id != 0x2A && id != 0x2E && id != 0x15 && id != 0x2C && id != 0x30) {
                ok = 0;
            }
        } else {
            ok = 0;
        }
    }
    u8 a = *(u8*)((u8*)self + 0x00A);
    if ((u32)(a - 5) > 6U) {
        switch (a) {
        case 0: {
            s32 id = self->act_no;
            switch (id) {
            case 1:
            case 2:
            case 5:
            case 6:
            case 7:
            case 20:
            case 21:
            case 28:
            case 73:
            case 91:
            case 122:
            case 158:
            case 159:
            case 160:
            case 161:
                ok = 0;
                break;
            }
            break;
        }
        case 4: {
            s32 id = self->act_no;
            if ((u32)(id - 2) <= 1U || (u32)(id - 0x10) <= 1U) {
                ok = 0;
            }
            break;
        }
        case 2:
            ok = 0;
            break;
        }
    } else {
        ok = 0;
    }
    return ok;
}

/* 0x8027AC2C: sets the two stored shell ids (or their per-weapon-class defaults) and scales them by
 * the actor's ammo skill. */
extern "C" void pl_shell_ids_set(_PLW* self, u8 arg1, u8 arg2)
{
    if (arg1 == 0xFF) {
        u8 t = self->field_0x002;
        *(u8*)((u8*)self + 0x568) = lbl_805BFFCC[t * 2];
        if (t == 7 && Pl_condition_ck(self, 0x80000000) == 1U) {
            *(u8*)((u8*)self + 0x568) = 0x71;
        }
    } else {
        *(u8*)((u8*)self + 0x568) = arg1;
    }
    if (arg2 == 0xFF) {
        u8 t = self->field_0x002;
        *(u8*)((u8*)self + 0x569) = lbl_805BFFCC[t * 2 + 1];
    } else {
        *(u8*)((u8*)self + 0x569) = arg2;
    }
    f32 f = (f32)((s8)*(u8*)((u8*)self + 0x452) + 100) / lbl_8079A0D0;
    if (Pl_cat_skill_ck(self, 0x26) == 1U) {
        s32 n = *(u8*)((u8*)self + 0x445);
        if (n != 0) {
            s16 i = 0;
            if (n > 0) {
                for (; i < n; i++) {
                    f = f * lbl_8079A0AC;
                }
            }
        }
    }
    *(u8*)((u8*)self + 0x568) = (u8)(s32)((f32)(*(u8*)((u8*)self + 0x568)) * f);
    *(u8*)((u8*)self + 0x569) = (u8)(s32)((f32)(*(u8*)((u8*)self + 0x569)) * f);
}

/* 0x8027C064: builds the impact vector an attacking part starts from - the per-weapon-class motion, the
 * hit matrix and the actor's world offset. */
extern "C" void pl_impact_pos_get(_PLW* self, nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 v;
    u8 buf[12];
    nw4r::math::MTX34 m;
    nw4r::math::MTX34 m2;
    s32 part;
    MTX34_ctor(&m);
    MTX34_ctor(&m2);
    VEC3_ctor(&v);
    u8 t = pl_slot_motion_allow_get(self, pl_rig_attach_kind_tbl[self->field_0x002]);
    out->x = lbl_8079A084;
    out->y = lbl_8079A084;
    setVector3(&v, lbl_8079A084, lbl_8079A084, lbl_8079A084);
    switch ((s32)t) {
    default:
        part = *(s32*)(lbl_805C6168 + (self->field_0x002 * 3 + 2) * 4);
        out->z = lbl_8079A084;
        break;
    case 1:
        if (self->field_0x002 == 3) {
            copyVec3(&v, (const nw4r::math::VEC3*)em_vec3_copy(buf, pl_rig_pose_drawn + self->field_0x002 * 0x18 + 0xC,
                                                              self->field_0x002 * 0x18));
        }
        /* fall through */
    case 0:
        part = *(s32*)(lbl_805C6168 + (t + self->field_0x002 * 3) * 4);
        out->z = *(f32*)(lbl_805C61D4 + self->field_0x002 * 4);
        break;
    }
    pl_joint_mtx_get(self, part, &m);
    mtx34_identity(&m2);
    mtx34_rot_xyz_deg(&m2, v.x, v.y, v.z);
    mtx34_concat_assign(&m, &m2);
    mulVecMat(out, &m);
    out->x = out->x + m.m[0][3];
    out->y = out->y + m.m[1][3];
    out->z = out->z + m.m[2][3];
}

/* 0x8027C208: the action-class gate - whether the actor may start the given action id. */
extern "C" s32 pl_item_now_usable_ck(_PLW* self, u16 arg1)
{
    nw4r::math::VEC3 sp24;
    s32 sp18;
    s32 sp14;
    s32 sp10;
    s32 spC;
    s16 sp8;
    VEC3_ctor(&sp24);
    if (self->field_0x00A == 7) {
        if (arg1 == 0x15B) {
            if ((u32)self->act_no <= 1U) {
                return 0;
            }
        } else {
            return 0;
        }
    }
    if ((Pl_act_ck(self, 6, 0x1C) == 1U || Pl_act_ck(self, 6, 0x1D) == 1U) && arg1 != 0x176) {
        return 0;
    }
    if ((fn_80278C7C(self) == 1U || fn_80278CD0(self) == 1U) && arg1 != 0x177) {
        return 0;
    }
    switch ((s32)arg1) {
    case 0x177:
        if (fn_80278C7C(self) == 1U || fn_80278CD0(self) == 1U) {
            return 1;
        }
        break;
    case 3:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return pl_spot_use_a_ck(self);
    case 4:
    case 0x31:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return pl_spot_use_b_ck(self);
    case 0x22:
    case 0x23:
    case 0x24:
        if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, (nw4r::math::VEC3*)&self->motion_pos_0x3C,
                                    &sp8, &sp24, (_CP_VECTOR*)&sp18, &sp14) != 0xFFFFU &&
            sp8 == 3) {
            return 1;
        }
        break;
    case 0x25:
    case 0x26:
    case 0x27:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, (nw4r::math::VEC3*)&self->motion_pos_0x3C,
                                    &sp8, &sp24, (_CP_VECTOR*)&sp18, &sp14) != 0xFFFFU &&
            sp8 == 4) {
            return 1;
        }
        break;
    case 0xCA:
    case 0xCB:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0x15F:
    case 0x161:
    case 0x163:
    case 0x185:
    case 0x186:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (em_roster_area_hit_find(self->chunk_ofs, self->area_0x16, (nw4r::math::VEC3*)&self->motion_pos_0x3C,
                                    &sp8, &sp24, (_CP_VECTOR*)&sp18, &sp14) != 0xFFFFU &&
            sp8 == 5) {
            return 1;
        }
        break;
    case 0x159:
    case 0x15A:
    case 0x15B:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0xA6:
    case 0xA7:
    case 0xD0:
    case 0xD6:
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (self->kind_0x09 == 3) {
            return 0;
        }
        return pl_stance_low_ck(self);
    case 0x16E:
        return fn_802784B8(self);
    case 6:
    case 0x2E:
    case 0x8A:
    case 0xC2:
    case 0x184:
        if (self->kind_0x09 == 3) {
            return 1;
        }
        break;
    case 0x28:
    case 0x2F:
    case 0x109:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        if (pl_camera_close_ck(self) == 1U) {
            return 0;
        }
        if (Pl_item_timer_get(self, 0x1D) > 0 && self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x1B6:
        if (self->kind_0x09 == 3) {
            return 0;
        }
        if (pl_camera_close_ck(self) == 1U) {
            return 0;
        }
        if (Pl_item_timer_get(self, 0x1D) > 0 && ai_npc_player_near_ck(4, lbl_8079A0F8) == 1U) {
            return 1;
        }
        break;
    case 0x169:
    case 0x16A:
    case 0x16B:
    case 0x16C:
    case 0x16D:
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x2B:
        if (self->kind_0x09 != 3) {
            return 1;
        }
        break;
    case 0x17F:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 1;
        }
        break;
    case 2:
    case 0x34:
    case 0x246:
        if (*(u8*)((u8*)self + 0x585) != 0) {
            return 0;
        }
        return pl_motion_interruptible_ck(self);
    case 0x108:
        if (self->kind_0x09 == 3) {
            return 0;
        }
    case 1:
    case 0x237:
    case 0x258:
    case 0x259:
        if (*(u8*)((u8*)self + 0x585) == 0) {
            return 1;
        }
        break;
    case 0x30:
    case 0x62:
    case 0xCF:
        if (pl_item_page2_ck(self) == 0) {
            return 1;
        }
        break;
    case 0x180:
        if (fn_8027DC90() == 0) {
            return 0;
        }
    case 0x17C:
        if (pl_stg_weapon_near_find(self, &sp24, &sp10, &spC) == 1) {
            return 1;
        }
        break;
    case 0x247:
        if (pl_stg_weapon_near_find(self, &sp24, &sp10, &spC) == 5) {
            return 1;
        }
        break;
    default:
        return 1;
    }
    return 0;
}

} /* namespace s_80276B58 */

/* ==== 0x8027D684-0x802840DC: the equipment helpers ==== */
#pragma optimization_level 4

#include "types.h"
#include "pl.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "ef/system_core.h" /* str_tbl_skill_get, str_tbl_item_kind_get (rule 2: the owner is `ef/system_core.cpp`) */
#include "stage/get_stg_weapon_work.h"
#include "lobby/npc_talk_flag_get.h"
#include "enemy/note_work_find.h"
#include "lobby/lb_fabsf.h"

/* 0x8079A114 / 0x8079A118: 90000.0 (300 squared, the note-pane reach) and 110.0 (the placed weapon's foot offset), this
 * unit's `.sdata2` pool (loads, never definitions - playbook 29). */
extern "C" {
extern f32 pl_note_near_dist_sq;
extern const f32 pl_stg_weapon_foot_ofs;
}

/* 0x8027FCE0 - whether the piece is at its kind's level cap (`EnableChangeColor`'s fallback; this unit's, its
 * body not yet written).  0x805C6238 / 0x805C6278 - the equipment and armour colour palettes, one 3-byte RGB row
 * per colour index (this unit's `.data`, not yet emitted by this source). */
extern "C" u32 equip_level_max_ck(_EQUIP* equip);
extern "C" u8 equip_color_table[][3];
extern "C" u8 armor_color_table[][3];

namespace s_8027D684 {



extern "C" s32 equip_kind_table_class(u8 kind);
extern "C" u8 weapon_kind_slot_index(u8 kind);
extern "C" u8 weapon_class_index(s32 kind);

/* The 1-based kind's 0x18-byte equipment row (the row stride is the `mulli r0,r0,24` in retail). */
extern "C" void* equip_slot_rec_find(u8 kind, u16 index) {
    u8** base = lbl_806AB810[0];
    if ((u8)equip_kind_table_class(kind) != 0) {
        return 0;
    }
    u32 slot = weapon_kind_slot_index(kind);
    u8* rows = base[slot];
    if ((s32)index >= (s32)lbl_805706C0[slot]) {
        index = 0;
    }
    rows += index * 0x18;
    return rows;
}

/* The sibling 0x1C-byte row of the second table. */
extern "C" void* equip_armor_ex_rec_find(u8 kind, u16 index) {
    u8** base = lbl_806AB810[1];
    if ((u8)equip_kind_table_class(kind) != 0) {
        return 0;
    }
    u32 slot = weapon_kind_slot_index(kind);
    u8* rows = base[slot];
    if ((s32)index >= (s32)lbl_805706C0[slot]) {
        index = 0;
    }
    rows += index * 0x1C;
    return rows;
}

/* The 7-based kind's 0x18-byte row of the fifth table (no kind guard, unlike the 1-based pair). */
extern "C" void* equip_blade_rec_find(u8 kind, u16 index) {
    u8** base = lbl_806AB810[4];
    return base[weapon_class_index(kind)] + index * 0x18;
}

/* The sibling 0x24-byte row of the same table. */
extern "C" void* equip_weapon_rec_find(u8 kind, u16 index) {
    u8** base = lbl_806AB810[4];
    return base[weapon_class_index(kind)] + index * 0x24;
}

/* In-unit callees referenced before their own definition (this TU is their owner). */
extern "C" void* equip_blade_rec_find(u8 kind, u16 item_id);
extern "C" void* equip_weapon_rec_find(u8 kind, u16 item_id);
extern "C" s32 equip_weapon_recs_find(u8 kind, u16 item_id, PlSkillRec** blade, PlWeaponRec** gun);
extern "C" u8 fn_8027FB84(u8 kind, u16 item_id, u8 deco_count);
extern "C" u8 fn_8027FE50(u8 kind, u16 item_id);
extern "C" void* equip_slot_rec_find(u8 kind, u16 index);
extern "C" void* equip_armor_ex_rec_find(u8 kind, u16 index);

/* True while the player's health is at or below zero. */
extern "C" s32 pl_health_out_ck(_PLW* self) {
    return self->field_0x36C <= 0;
}

/* Stores the raw pair the +0x452/+0x458 group holds. */
extern "C" void pl_attack_up_mark(_PLW* self, s8 arg1, s16 arg2) {
    self->field_0x452 = arg1;
    self->field_0x458 = arg2;
}

/* Stores the byte and raises the +0x454 limit to the argument. */
extern "C" void pl_defense_up_mark(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x450 = arg1;
    if (self->field_0x454 < arg2) {
        self->field_0x454 = arg2;
    }
}

/* Stores the byte and raises the +0x456 limit to the argument. */
extern "C" void fn_8027D6C0(_PLW* self, u8 arg1, s16 arg2) {
    self->field_0x451 = arg1;
    if (self->field_0x456 < arg2) {
        self->field_0x456 = arg2;
    }
}

/* Latches the motion-request byte, then sends the motion through the +0x5C8 gate. */
extern "C" void pl_motion_request_latch(_PLW* self, s16 value) {
    self->field_0x5C9 = 1;
    if (self->field_0x5C8 != 0) {
        u32 motion = pl_armed_act_no_get(self, value);
        pl_act_enter_raw(self, 0xB, (u16)motion, 0);
    }
}

/* Reports the byte at +0x268 as a boolean; `u32`, because the item menu's caller compares the result
 * unsigned (`cmplwi r3,0x1` at 0x802A008C), as `Pl/fn_8027D684.h` declares it. */
extern "C" u32 fn_8027D738(_PLW* self) {
    return self->field_0x268 != 0;
}

/* The +0x655 high-bit gate in front of the +0x268 boolean. */
extern "C" s32 fn_8027D74C(_PLW* self) {
    if ((self->field_0x655 & 0x80) == 0) {
        return 0;
    }
    return fn_8027D738(self);
}

/* Ends the charged action once its timer has run out. */
extern "C" void fn_8027D76C(_PLW* self) {
    if (Pl_master_ck(self) != 0 && (self->field_0x655 & 0x80) != 0) {
        self->act_end_request = 1;
        s16 timer = self->field_0x652;
        if (Pl_item_timer_get(self, self->field_0x650) >= timer) {
            pl_item_add(self, self->field_0x650, -timer);
        }
    }
}

/* The act range 0x54-0x56 of the idle kind. */
extern "C" s32 pl_act_84_86_ck(_PLW* self) {
    if (self->field_0x00A == 0 && (u32)(self->act_no - 0x54) <= 2U) {
        return 1;
    }
    return 0;
}

/* 0x8027D8A0 (0xC8): whether the actor stands within 300 units of the live note-pane record of `kind` (0 or 1) while
 * no NPC talk counts down. */
extern "C" u32 pl_note_near_ck(_PLW* self, u8 kind) {
    u8 find;
    NoteWork* note;

    switch (kind) {
    case 0:
        find = 0;
        break;
    case 1:
        find = 1;
        break;
    default:
        return 0;
    }
    if (kind <= 1) {
        if (npc_talk_flag_get() != 0) {
            return 0;
        }
    } else {
        return 0;
    }
    note = note_work_find(find);
    if (note == NULL) {
        return 0;
    }
    return vec3_dist_sq(&self->vec_0x03C, &note->vec_0x170) <= pl_note_near_dist_sq;
}

/* 0x8027D968 (0x2FC): finds the placed stage weapon the actor reaches (an xz radius and a height band; a gunner also
 * needs kind 2's band above the foot offset), copies its position and hands back its id and word; 0 when none. */
extern "C" s32 pl_stg_weapon_near_find(_PLW* self, nw4r::math::VEC3* out_pos, s32* out_id, s32* out_word) {
    nw4r::math::VEC3 pos;
    long count;
    StageWeaponWork* table;
    s32 i;
    StageWeaponWork* rec;
    f32 dist;
    f32 height;

    VEC3_ctor(&pos);
    table = get_stg_weapon_work(self, &count);
    *out_word = 0;
    if (self->kind_0x09 == 3) {
        for (i = 0, rec = table; i < count; rec++, i++) {
            vec_to_mh_vec3(&pos, &rec->pos);
            if (rec->kind == 2) {
                dist = calcDistanceSqXZ(&self->vec_0x03C, &pos);
                height = rec->height * s_80276B58::lbl_8079A088 - lb_fabsf(self->field_0x040 - pos.y);
                if (dist <= rec->radius * rec->radius && height >= s_80276B58::lbl_8079A084) {
                    copyVec3(out_pos, &pos);
                    *out_id = table[i].id;
                    *out_word = table[i].word_0x04;
                    return table[i].kind;
                }
            } else {
                dist = calcDistanceSqXZ(&self->vec_0x03C, &pos);
                height = self->field_0x040 - (pos.y - pl_stg_weapon_foot_ofs);
                if (dist <= rec->radius * rec->radius && height >= s_80276B58::lbl_8079A084 && height <= rec->height) {
                    copyVec3(out_pos, &pos);
                    *out_id = table[i].id;
                    *out_word = table[i].word_0x04;
                    return table[i].kind;
                }
            }
        }
        return 0;
    }
    for (i = 0, rec = table; i < count; i++, rec++) {
        vec_to_mh_vec3(&pos, &rec->pos);
        if (rec->kind == 2) {
            dist = calcDistanceSqXZ(&self->vec_0x03C, &pos);
            height = rec->height * s_80276B58::lbl_8079A088 - lb_fabsf(self->field_0x040 - pos.y);
            if (dist <= rec->radius * rec->radius && height >= s_80276B58::lbl_8079A084) {
                copyVec3(out_pos, &pos);
                *out_id = table[i].id;
                *out_word = table[i].word_0x04;
                return table[i].kind;
            }
        } else if (calcDistanceSqXZ(&self->vec_0x03C, &pos) <= rec->radius * rec->radius) {
            copyVec3(out_pos, &pos);
            *out_id = table[i].id;
            *out_word = table[i].word_0x04;
            return table[i].kind;
        }
    }
    return 0;
}

/* Reports the first of the three per-slot gate bytes at 0x806BB7A0 as empty. */
extern "C" s32 pl_work_flag0_clear_ck(void) {
    return stg_place_timers[0].flag_0x00 == 0;
}

/* Reports the second gate byte as empty. */
extern "C" s32 pl_work_flag8_clear_ck(void) {
    return stg_place_timers[1].flag_0x00 == 0;
}

/* Reports the third gate byte as empty. */
extern "C" s32 fn_8027DC90(void) {
    return stg_place_timers[2].flag_0x00 == 0;
}

/* The +0x460 timer, as a boolean. */
extern "C" s32 Pl_timer_0x460_ck(_PLW* self) {
    return self->field_0x460 > 0;
}

/* Advances the 0-100 counter at +0x445 and saturates it. */
extern "C" void pl_act_timer_step(_PLW* self) {
    if (++self->field_0x445 >= 0x64U) {
        self->field_0x445 = 0x64U;
    }
}


/* The act-number ranges of the held (kind 6) actor. */
extern "C" s32 pl_act6_52_54_61_62_ck(_PLW* self) {
    if (self->field_0x00A == 6) {
        if ((u32)(self->act_no - 0x34) <= 2U || (u32)(self->act_no - 0x3D) <= 1U) {
            return 1;
        }
    }
    return 0;
}

/* The act-number gate the held-state handler asks with the part index. */
extern "C" u32 fn_8027DCE0(_PLW* self, u8 arg1) {
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A == 0) {
        switch (self->act_no) {
        case 34:
        case 35:
        case 111:
        case 112:
        case 113:
        case 114:
        case 119:
        case 120:
            return 1;
        case 36:
        case 115:
        case 116:
            return arg1 != 2;
        case 37:
        case 117:
        case 118:
            return arg1 != 1;
        }
    }
    return 0;
}

/* The second act-number gate of the same handler family. */
extern "C" u32 pl_act_range2_ck(_PLW* self, u8 arg1) {
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (self->field_0x00A == 0) {
        switch (self->act_no) {
        case 63:
        case 64:
            return 1;
        case 65:
        case 154:
            if (arg1 == 2) {
                return 0;
            }
            return 1;
        case 66:
        case 67:
            return arg1 != 1;
        }
    }
    return 0;
}

/* Scans the per-kind item-id table for the first id the slot lookup accepts. */
extern "C" u16 pl_kind_item_slot_find(_PLW* self, s32 arg1) {
    u16* table;
    switch (arg1) {
    case 0:
        table = pl_act_kind2_item_ids;
        break;
    case 1:
        table = pl_act_kind3_item_ids;
        break;
    case 2:
        table = pl_act_kind4_item_ids;
        break;
    default:
        return 0xFFFF;
    }
    while (*table != 0xFFFF) {
        u16 result = pl_item_slot_find(self, *table);
        if (result != 0xFFFF) {
            return result;
        }
        table++;
    }
    return 0xFFFF;
}

/* Scans the same id table for the equipped slot that carries the id. */
extern "C" u16 pl_kind_item_equip_find(_PLW* self, s32 arg1) {
    u16* table;
    switch (arg1) {
    case 0:
        table = pl_act_kind2_item_ids;
        break;
    case 1:
        table = pl_act_kind3_item_ids;
        break;
    case 2:
        table = pl_act_kind4_item_ids;
        break;
    default:
        return 0xFFFF;
    }
    while (*table != 0xFFFF) {
        u16 slot = self->field_0x304;
        if (*table == self->slot_id[slot].item_id && self->slot_id[slot].value > 0) {
            return slot;
        }
        table++;
    }
    return 0xFFFF;
}

/* The act-number class of the held actor, as the handler's return code. */
extern "C" s32 pl_act_held_class_get(_PLW* self) {
    switch (self->field_0x00A) {
    case 0:
        switch (self->act_no) {
        case 0xAB:
        case 0xAE:
            return 1;
        case 0xA7:
            return 3;
        }
        break;
    case 12:
        if (self->act_no == 0xC) {
            return 2;
        }
        break;
    }
    return 0;
}

/* The per-part act-number gate of the held actor. */
extern "C" s32 pl_act6_class_ck(_PLW* self, s32 arg1) {
    if (self->field_0x00A != 6) {
        return 0;
    }
    switch (arg1) {
    case 0:
        if ((u16)(self->act_no - 0x15) <= 1U) {
            return 1;
        }
        break;
    case 1:
        if ((u32)(self->act_no - 0xE) <= 1U || (u32)(self->act_no - 0x24) <= 1U) {
            return 1;
        }
        break;
    case 2:
        if ((u32)(self->act_no - 4) <= 3U || (u32)(self->act_no - 0x1E) <= 2U ||
            (u32)(self->act_no - 0x1A) <= 1U || self->act_no == 0x26) {
            return 1;
        }
        break;
    }
    return 0;
}

/* ORs the act-flag bits into the byte at +0x567. */
extern "C" void pl_atk_act_flag_set(_PLW* self, u8 flags) {
    self->atk_act_flag |= flags;
}
} /* namespace s_8027D684 */


/* Tests the act-flag byte at +0x567 against a mask. */
s32 Pl_atk_act_flag_ck(_PLW* self, u8 mask) {
    return (self->atk_act_flag & mask) != 0;
}
namespace s_8027D684 {


/* The three-way "is this actor held" test: the +0x352 timer, the system flag or the +0x46F byte. */
extern "C" s32 fn_8027E1E4(_PLW* self) {
    if (self->field_0x352 > 0 || system_w.field_0x2a != 0 || self->field_0x46F != 0) {
        return 1;
    }
    return 0;
}

/* Reports the +0x396 timer, or the live player work through the hold test. */
extern "C" u32 pl_act_held_ck(_PLW* self, s32 unused) {
    if (self->field_0x396 > 0) {
        return 1;
    }
    if (Pl_master_ck(self) == 1U && fn_8027E1E4(self) == 1U) {
        return 1;
    }
    return 0;
}

/* The equipment kind the data tables are indexed by, one-based. */
extern "C" u8 weapon_kind_slot_index(u8 kind) {
    return kind - 1;
}

/* The same one-based kind, for the seven-kind table. */
extern "C" u8 weapon_class_index(s32 kind) {
    return kind - 7;
}

/* Looks up the 0x18-byte equipment row an `_EQUIP` record names. */
extern "C" void* equip_slot_rec_get(_EQUIP* equip) {
    return equip_slot_rec_find(equip->kind, equip->item_id);
}

/* Looks up the 0x1C-byte sibling row the same record names. */
extern "C" void* equip_armor_ex_rec_get(_EQUIP* equip) {
    return equip_armor_ex_rec_find(equip->kind, equip->item_id);
}

/* The `_EQUIP` -> data-row lookup the table helper and the sound unit share. */
extern "C" PlSkillRec* equip_blade_rec_get(_EQUIP* equip) {
    return (PlSkillRec*)equip_blade_rec_find(equip->kind, equip->item_id);
}

/* The `_EQUIP` -> item lookup the four Pl consumers share. */
extern "C" void* equip_weapon_rec_get(void* equip) {
    _EQUIP* e = (_EQUIP*)equip;
    return equip_weapon_rec_find(e->kind, e->item_id);
}

/* The three-argument sibling of `equip_weapon_rec_get`. */
extern "C" s32 fn_8027EE08(_EQUIP* equip, PlSkillRec** blade, PlWeaponRec** gun) {
    return equip_weapon_recs_find(equip->kind, equip->item_id, blade, gun);
}

/* The `_EQUIP` -> row lookup that also passes the decoration count. */
extern "C" u8 equip_level_cap_get(_EQUIP* equip) {
    return fn_8027FB84(equip->kind, equip->item_id, equip->deco_count);
}

/* The last `_EQUIP` -> row lookup of the family. */
extern "C" u8 gun_frame_level_cap_get(_EQUIP* equip) {
    return fn_8027FE50(equip->kind, equip->item_id);
}

/* 0x8027E708 (0x24): the +0x06 bonus byte of the weapon record the slot resolves to. */
extern "C" s32 fn_8027E708(_EQUIP* equip) {
    return ((PlWeaponRec*)equip_weapon_rec_get(equip))->bonus_0x06;
}

/* 0x8027E72C (0x48): the `index`-th 3-byte row of the equipment colour table as an opaque RGBA word. */
extern "C" s32 equip_color_rgba_get(u8 index) {
    PlRgba color;

    color.channel[3] = 0xFF;
    color.channel[0] = equip_color_table[index][0];
    color.channel[1] = equip_color_table[index][1];
    color.channel[2] = equip_color_table[index][2];
    return color.word;
}

/* 0x8027E774 (0x48): the `index`-th 3-byte row of the armour colour table as an opaque RGBA word. */
extern "C" s32 armor_color_rgba_get(u8 index) {
    PlRgba color;

    color.channel[3] = 0xFF;
    color.channel[0] = armor_color_table[index][0];
    color.channel[1] = armor_color_table[index][1];
    color.channel[2] = armor_color_table[index][2];
    return color.word;
}

/* 0x8027E7BC (0x34): the colour-table index the slot's kind row stores (0 when the row is absent). */
extern "C" u8 equip_color_index_get(_EQUIP* equip) {
    PlSlotRec* rec = (PlSlotRec*)equip_slot_rec_get(equip);

    if (rec != NULL) {
        return rec->color_index;
    }
    return 0;
}

/* 0x8027E840 (0x4C): the piece's own RGB565 dye colour widened to an opaque RGBA word. */
extern "C" s32 equip_custom_color_rgba_get(_EQUIP* equip) {
    PlRgba color;
    u16 rgb565;

    color.channel[3] = 0xFF;
    rgb565 = equip->color_rgb565;
    color.channel[0] = (u8)((rgb565 & 0xF800) >> 8);
    color.channel[1] = (u8)((rgb565 & 0x07E0) >> 3);
    color.channel[2] = (u8)((rgb565 & 0x001F) << 3);
    return color.word;
}

/* 0x8027EB18 (0x18): the string-table name of item kind `kind` (kind 85 and up read as 0). */
extern "C" u32 item_kind_name_get(u8 kind) {
    if (kind >= 85) {
        kind = 0;
    }
    return str_tbl_item_kind_get(kind);
}

/* 0x8027EB30 (0x18): the string-table name of skill `id` (id 206 and up read as 0). */
extern "C" u32 skill_name_get(u16 id) {
    if (id >= 206) {
        id = 0;
    }
    return str_tbl_skill_get(id);
}

/* 0x8027F0F4 (0x28): the first byte of the species record `id` selects. */
extern "C" u8 item_species_class_get(u16 id) {
    return get_item_species_data(id)->slot_size;
}

/* 0x8027FFFC (0x54): whether a gun slot (kind 11 or 12) resolves to a weapon record with its +0x15 flag set. */
extern "C" u32 equip_gun_bonus_ck(_EQUIP* equip) {
    if (equip->kind != 11 && equip->kind != 12) {
        return 0;
    }
    if (((PlWeaponRec*)equip_weapon_rec_get(equip))->bonus_flag_0x15 != 0) {
        return 1;
    }
    return 0;
}


/* The equipment kind's row-table class: 0 for kinds 1-6, 1 for 7-11 and 14-15, 2 for 12-13, 0xFF for
 * anything else. */
extern "C" s32 equip_kind_table_class(u8 kind) {
    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
        return 1;
    case 12:
    case 13:
        return 2;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return 0;
    }
    return 0xFF;
}

/* The per-kind equipment row count the kind's own table class selects. */
extern "C" s32 equip_kind_item_count(u8 kind) {
    s32 count = 0;
    if ((u32)(kind - 7) > 8U) {
        if ((u32)(kind - 1) <= 5U) {
            count = lbl_805706C0[weapon_kind_slot_index(kind)];
        }
    } else {
        count = lbl_805706D8[weapon_class_index(kind)];
    }
    return count;
}
} /* namespace s_8027D684 */

/* 0x8027E7F0 (0x50): whether the piece may be recoloured - its kind row's dye mode 2, otherwise the level-cap test. */
u32 EnableChangeColor(_EQUIP* equip) {
    PlSlotRec* rec = (PlSlotRec*)s_8027D684::equip_slot_rec_get(equip);

    if (rec != NULL && rec->dye_mode == 2) {
        return 1;
    }
    return equip_level_max_ck(equip);
}

/* 0x8027F0C4 (0x30): whether a bowgun (kind 11) piece takes options - every id but 17-19. */
u32 Gunner_opt_ok_ck(_EQUIP* equip) {
    if (equip->kind != 11) {
        return 0;
    }
    return (u32)(equip->item_id - 17) > 2U;
}

/* The address of the eight per-kind equipment table pointers at 0x806AB810. */
u8*** get_eq_data_ptr(void) {
    return lbl_806AB810;
}

/* ---- the equipment helpers, continued: the level, defence, decoration and drop-roll helpers of 0x8027D7EC-0x8028004C ---- */

#include "Pl/gun_level_bonus_tbl.h" /* gun_level_bonus_tbl (rule 2: the owner is `Pl/pl_act_step_data.cpp`) */

/* The equipment helpers' own tables (this unit's `.data`/`.sdata`): the armour defence step rows (five s16 per
 * `PlArmorLevelRec::step_row`) and the three default armour model lists per player profile. */
extern "C" {
extern s16 pl_armor_defense_steps[][5];
extern u16 pl_armor_default_models_a[4];
extern u16 pl_armor_default_models_b[4];
extern u16 pl_armor_default_models_c[4];
}

namespace s_8027D684 {

extern "C" {

void equip_deco_pack(_EQUIP* equip);

/* 0x8027D7EC (0x88): whether the player's act (kind 0 only) suppresses its effect for `mode`: acts 10-14 and
 * 102-103 in modes 0-1, acts 74-78 and 104-105 in modes 0 and 2. */
u32 pl_act_no_eft_ck(_PLW* self, s32 mode)
{
    if (self->field_0x00A == 0) {
        switch (self->act_no) {
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 102:
        case 103:
            if ((u32)mode <= 1) {
                return 1;
            }
            break;
        case 74:
        case 75:
        case 76:
        case 77:
        case 78:
        case 104:
        case 105:
            if (mode == 0 || mode == 2) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x8027E120 (0x98): whether the actor may take its stamina-gated input: not while a motion input is pending, in
 * act kinds 8/9 or in kind-0 acts 19/110, and only while the +0x370 gauge is positive. */
u32 pl_alive_act_ck(_PLW* self)
{
    if (Pl_motion_input_ck(0) == 1) {
        return 0;
    }
    switch (self->field_0x00A) {
    case 8:
    case 9:
        return 0;
    case 0:
        if (self->act_no == 19 || self->act_no == 110) {
            return 0;
        }
        break;
    }
    if (self->health <= 0) {
        return 0;
    }
    return 1;
}

/* 0x8027E404 (0x10C): the defence stage of an armour piece at `level` (0xFF: the piece's own): 0-4 by the level
 * row's stage ends, 5 for the top step of a piece that has one; a talisman (kind 6) has none. */
u8 equip_level_stage_get(_EQUIP* equip, u8 level)
{
    PlSlotRec* slot;
    PlArmorLevelRec* row;
    u8 stage;
    s32 n;

    if (equip->kind == 6) {
        return 0;
    }
    slot = (PlSlotRec*)equip_slot_rec_get(equip);
    row = (PlArmorLevelRec*)equip_armor_ex_rec_get(equip);
    if (level == 0xFF) {
        n = equip->deco_count + 1;
    } else {
        n = level + 1;
    }
    stage = 0;
    if (n <= row->stage_end[0]) {
        stage = 0;
    } else if (n <= row->stage_end[1]) {
        stage = 1;
    } else if (n <= row->stage_end[2]) {
        stage = 2;
    } else if (n == row->stage_end[2] + 1) {
        if (slot->top_step != 0) {
            stage = 5;
        } else {
            stage = 3;
        }
    } else if (n <= row->stage_end[3]) {
        stage = 3;
    } else if (n <= row->stage_end[4]) {
        stage = 4;
    }
    return stage;
}

/* 0x8027E510 (0xD4): an armour piece's defence: its base plus one step per level, each step the stage's row entry
 * (or the piece's top step). */
s32 equip_defense_get(_EQUIP* equip)
{
    PlSlotRec* slot;
    s16* steps;
    s16 total;
    s32 i;
    s32 end;
    u8 stage;

    if (equip->kind == 6) {
        return 0;
    }
    slot = (PlSlotRec*)equip_slot_rec_get(equip);
    steps = pl_armor_defense_steps[((PlArmorLevelRec*)equip_armor_ex_rec_get(equip))->step_row];
    total = 0;
    end = equip->deco_count + 1;
    for (i = 1; i < end; i++) {
        stage = equip_level_stage_get(equip, i);
        if (stage == 5) {
            total += slot->top_step;
        } else {
            total += steps[stage];
        }
    }
    return (s16)(slot->defense + total);
}

/* 0x8027E5E4 (0x124): a gun part's value: its base plus the first `level` entries of its growth row. */
s32 equip_gun_level_bonus_get(_EQUIP* equip)
{
    u8 level = equip->deco_count;
    PlWeaponRec* rec = (PlWeaponRec*)equip_weapon_rec_get(equip);
    s16* row = &gun_level_bonus_tbl[rec->growth_row * 7 + 1];
    s32 sum = 0;
    s16 i;

    for (i = 0; i < (s16)level; i++) {
        sum += *row;
        row++;
    }
    return (s16)((u16)rec->base_value + sum);
}

/* 0x8027E88C (0x44): stores the dye colour of a piece that can be recoloured. */
void equip_color_set(_EQUIP* equip, u16 color)
{
    if (EnableChangeColor(equip) == 1) {
        equip->color_rgb565 = color;
    }
}

/* 0x8027E8D0 (0x48): the blade dye of a blademaster weapon (kinds 7-10, 14-15), 0 for anything else. */
u8 equip_blade_dye_ck(_EQUIP* equip)
{
    u8 dye = 0;

    if ((u32)(equip->kind - 7) <= 3 || (u32)(equip->kind - 14) <= 1) {
        dye = equip_blade_rec_get(equip)->dye;
    }
    return dye;
}

/* 0x8027E98C (0x64): clamps a piece's item id into its kind's table: past the end it falls back to row 0 (armour)
 * or row 1 (weapons). */
void equip_item_id_clamp(_EQUIP* equip)
{
    if (equip->item_id >= equip_kind_item_count(equip->kind)) {
        if ((u8)equip_kind_table_class(equip->kind) == 0) {
            equip->item_id = 0;
        } else {
            equip->item_id = 1;
        }
    }
}

/* 0x8027EA84 (0x94): the information text of item `id` of kind `kind`, through the kind's text table. */
u32 equip_info_text_get(u8 kind, u16 id)
{
    u32* table;
    u8** base;

    if ((u32)(kind - 7) > 8) {
        if ((u32)(kind - 1) <= 5) {
            base = lbl_806AB810[3];
            table = (u32*)base[weapon_kind_slot_index(kind)];
        }
    } else {
        base = lbl_806AB810[6];
        table = (u32*)base[weapon_class_index(kind)];
    }
    return table[id];
}

/* 0x8027EB48 (0x60): the armour model of the piece for the hunter's body, or the profile's default model from
 * `defaults` when the piece has none. */
s32 pl_armor_model_or_default_get(_PLW* self, _EQUIP* equip, u16* defaults)
{
    u8 model = ((PlSlotRec*)equip_slot_rec_get(equip))->model_id[self->se_name_set];

    if (model != 0) {
        return model;
    }
    return defaults[self->user_profile_0x1C];
}

/* 0x8027EBA8 (0xA8): the model id of an equipped piece: the head/legs model, the profile defaults for the body,
 * arms and waist, 0xFF for a talisman. */
s32 pl_equip_model_id_get(_PLW* self, _EQUIP* equip)
{
    switch (equip->kind) {
    case 3:
    case 5:
        return ((PlSlotRec*)equip_slot_rec_get(equip))->model_id[self->se_name_set];
    case 1:
        return pl_armor_model_or_default_get(self, equip, pl_armor_default_models_a);
    case 2:
        return pl_armor_model_or_default_get(self, equip, pl_armor_default_models_b);
    case 4:
        return pl_armor_model_or_default_get(self, equip, pl_armor_default_models_c);
    case 6:
        return -1;
    }
    return 0;
}

/* 0x8027EE24 (0xAC): the model variant of a weapon piece, 0xFFFF for none. */
s32 equip_variant_get(_EQUIP* equip)
{
    PlWeaponRec* rec;

    switch (equip->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        return equip_blade_rec_get(equip)->variant;
    case 11:
        return ((PlWeaponRec*)equip_weapon_rec_get(equip))->variant;
    case 12:
    case 13:
        rec = (PlWeaponRec*)equip_weapon_rec_get(equip);
        if (equip->item_id == 0 || rec->variant == 0) {
            return 0xFFFF;
        }
        return rec->variant;
    }
    return 0xFFFF;
}

/* 0x8027ED28 (0x44): the bowgun frame kind of a kind-11 piece's variant, 0 for anything else. */
u8 equip_bowgun_frame_kind_get(_EQUIP* equip)
{
    if (equip->kind != 11) {
        return 0;
    }
    return lb_bowgun_frame_kinds[(u16)equip_variant_get(equip)];
}

/* 0x8027F008 (0xBC): how many shells of kind `shell` a gun piece loads: a frame (kind 11) only for the shells its
 * mask enables, a barrel or stock (kinds 12/13) always from its clip row. */
s32 equip_gun_clip_get(_EQUIP* equip, u8 shell)
{
    PlWeaponRec* rec;
    u8 clip = 0;
    u8 kind = equip->kind;

    if ((u32)(kind - 12) > 1) {
        if (kind == 11) {
            rec = (PlWeaponRec*)equip_weapon_rec_get(equip);
            if ((rec->clip_mask[shell >> 5] & (1 << (shell & 31))) != 0) {
                clip = pl_gun_clip_table[rec->clip_row][shell];
            } else {
                clip = 0;
            }
        }
    } else {
        clip = pl_gun_clip_table[((PlWeaponRec*)equip_weapon_rec_get(equip))->clip_row][shell];
    }
    return clip;
}

/* 0x8027F11C (0x9C): the decoration slots of a piece (0-3), out of the row its kind reads; a talisman keeps them
 * in its own +0x01 byte. */
u8 equip_deco_slots_get(_EQUIP* equip)
{
    u8 slots;

    if (equip == NULL) {
        return 0;
    }
    switch (equip->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        slots = equip_blade_rec_get(equip)->deco_slots;
        break;
    case 11:
    case 12:
    case 13:
        slots = ((PlWeaponRec*)equip_weapon_rec_get(equip))->deco_slots;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        slots = ((PlSlotRec*)equip_slot_rec_get(equip))->deco_slots;
        break;
    case 6:
        slots = equip->deco_count;
        break;
    }
    if (slots > 3) {
        slots = 0;
    }
    return slots;
}

/* 0x8027F1B8 (0x64): how many of the piece's decoration slots hold a decoration. */
s32 equip_deco_count(_EQUIP* equip)
{
    s32 count = 0;
    u8 slots = equip_deco_slots_get(equip);
    s32 i;

    for (i = 0; i < slots; i++) {
        if (equip->skill_id[i] != 0) {
            count++;
        }
    }
    return count;
}

/* 0x8027F21C (0xBC): the decoration slots still free: the slots less the sizes (1-3) of the decorations held. */
s32 equip_deco_slots_free(_EQUIP* equip)
{
    s32 i;
    s32 used;
    s32 slots = equip_deco_slots_get(equip);
    u8 size;

    if (slots <= 0) {
        return 0;
    }
    used = 0;
    for (i = 0; i < slots; i++) {
        if (equip->skill_id[i] != 0) {
            size = item_species_class_get(equip->skill_id[i]);
            if (size == 0 || size > 3) {
                size = 1;
            }
            used += size;
        }
    }
    return slots > used ? slots - used : 0;
}

/* 0x8027F2D8 (0x54): whether decoration `deco` fits the piece's free slots. */
s32 equip_deco_fit_ck(_EQUIP* equip, u16 deco)
{
    s32 size = item_species_class_get(deco);

    return size <= equip_deco_slots_free(equip);
}

/* 0x8027F548 (0x7C): removes the decoration in slot `slot` (when the piece has that slot) and packs the rest. */
void equip_deco_detach(_EQUIP* equip, s32 slot)
{
    equip_deco_slots_get(equip);
    if (equip->skill_id[slot] != 0 && (u32)(slot + 1) <= equip_deco_slots_get(equip)) {
        equip->skill_id[slot] = 0;
        equip_deco_pack(equip);
    }
}

/* 0x8027FC84 (0x5C): raises an armour piece's level by one, up to its cap. */
void equip_armor_level_up(_EQUIP* equip)
{
    u8 cap;

    if ((u32)(equip->kind - 1) <= 4) {
        cap = equip_level_cap_get(equip);
        if (equip->deco_count >= cap) {
            equip->deco_count = cap;
        } else {
            equip->deco_count++;
        }
    }
}

/* 0x8027FCE0 (0x9C): whether an armour piece with a top step has reached it. */
u32 equip_level_max_ck(_EQUIP* equip)
{
    PlSlotRec* slot = (PlSlotRec*)equip_slot_rec_get(equip);
    PlArmorLevelRec* row = (PlArmorLevelRec*)equip_armor_ex_rec_get(equip);

    if (equip->kind == 6) {
        return 0;
    }
    if (slot == NULL || row == NULL) {
        return 0;
    }
    if (slot->top_step != 0 && equip->deco_count + 1 >= row->stage_end[2] + 1) {
        return 1;
    }
    return 0;
}

/* 0x8027FF30 (0x58): raises a bowgun frame's level by one, up to its cap. */
void equip_gun_level_up(_EQUIP* equip)
{
    u8 cap;

    if (equip->kind == 11) {
        cap = gun_frame_level_cap_get(equip);
        if (equip->deco_count >= cap) {
            equip->deco_count = cap;
        } else {
            equip->deco_count++;
        }
    }
}

/* 0x8027FF88 (0x74): the colour tier of a blademaster weapon's dye (groups of five), 0 for none. */
u8 equip_color_tier_get(_EQUIP* equip)
{
    u8 tier = 0;

    if ((u32)(equip->kind - 7) <= 3 || (u32)(equip->kind - 14) <= 1) {
        tier = equip_blade_rec_get(equip)->dye;
        if (tier != 0) {
            tier = (tier - 1) / 5 + 1;
        }
    }
    return tier;
}

} /* extern "C" */

} /* namespace s_8027D684 */

/* 0x8027E9F0 (0x94): the name of item `id` of kind `kind`, through the kind's name table. */
u32 GetEquipName(u8 kind, u16 id)
{
    u32* table;
    u8** base;

    if ((u32)(kind - 7) > 8) {
        if ((u32)(kind - 1) <= 5) {
            base = lbl_806AB810[2];
            table = (u32*)base[s_8027D684::weapon_kind_slot_index(kind)];
        }
    } else {
        base = lbl_806AB810[5];
        table = (u32*)base[s_8027D684::weapon_class_index(kind)];
    }
    return table[id];
}

/* 0x8027EED0 (0xE4): the player's weapon type: the blade class for a blademaster weapon, and for a bowgun the light
 * (5)/medium (6)/heavy (4) class its three parts' weight sum gives. */
u8 Get_pl_type(_EQUIP* equipA, _EQUIP* equipB)
{
    u8 type = 0;
    s32 weight;

    switch (equipA->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        type = s_8027D684::weapon_class_index(equipA->kind);
        break;
    case 11:
        weight = ((PlWeaponRec*)s_8027D684::equip_weapon_rec_get(equipA))->defense_bonus;
        if (equipB != NULL) {
            if (equipB[0].kind == 12) {
                weight += ((PlWeaponRec*)s_8027D684::equip_weapon_rec_get(&equipB[0]))->defense_bonus;
            }
            if (equipB[1].kind == 13) {
                weight += ((PlWeaponRec*)s_8027D684::equip_weapon_rec_get(&equipB[1]))->defense_bonus;
            }
        }
        if ((s16)weight >= 71) {
            type = 4;
        } else {
            type = ((s16)weight < 30) ? 5 : 6;
        }
        break;
    }
    return type;
}

/* 0x8027FD7C (0xD4): the rarity of a piece; an armour piece at its top step gains 3 (up to 6). */
u8 Get_equip_rare(_EQUIP* equip)
{
    u8 rare = 0;

    if (equip == NULL) {
        return 0;
    }
    switch (equip->kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        rare = s_8027D684::equip_blade_rec_get(equip)->rarity;
        break;
    case 11:
    case 12:
    case 13:
        rare = ((PlWeaponRec*)s_8027D684::equip_weapon_rec_get(equip))->rarity;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        rare = ((PlSlotRec*)s_8027D684::equip_slot_rec_get(equip))->rarity;
        if (s_8027D684::equip_level_max_ck(equip) == 1) {
            rare += 3;
            if (rare > 6) {
                rare = 6;
            }
        }
        break;
    case 6:
        rare = ((PlSlotRec*)s_8027D684::equip_slot_rec_get(equip))->rarity;
        break;
    }
    return rare;
}

#include "userdata_rank_band_get.h"

/* The talisman and drop roll tables (this unit's `.data`/`.sdata`). */
typedef struct EquipTalismanRow {
    /* +0x0 */ s8 weight;         /* the row's weight in the roll; 0 or less ends the table */
    /* +0x1 */ u8 id;             /* the talisman id less one */
    /* +0x2 */ u8 slot_mode;      /* 0-1: that many slots; 2: one or two; 3: one to three */
    /* +0x3 */ u8 skill1_group;   /* the first skill's group plus one (0: none) */
    /* +0x4 */ s8 skill1_points;  /* the first skill's points (0 ends the skills) */
    /* +0x5 */ u8 skill2_group;   /* the second skill's group plus one (0: none) */
    /* +0x6 */ s8 skill2_points;
    /* +0x7 */ u8 pad_0x7;
} EquipTalismanRow; /* size: 0x8 */

typedef struct EquipDropRow {
    /* +0x0 */ s8 weight;   /* the row's weight in the roll; 0 or less ends the table */
    /* +0x1 */ u8 kind;     /* the `_EQUIP::kind` the row gives */
    /* +0x2 */ u16 item_id; /* the `_EQUIP::item_id` the row gives */
} EquipDropRow; /* size: 0x4 */

extern "C" {
extern u16* talisman_skill_lists[];
extern EquipTalismanRow* talisman_roll_tables[];
extern EquipDropRow* equip_drop_roll_tables[2];
}

namespace s_8027D684 {

extern "C" {

/* 0x8027ED6C (0x9C): resolves a weapon piece to its blademaster row (kinds 7-10/14-15, returns 0) or its gun-part
 * row (kinds 11-13, returns 1); -1 for anything else. */
s32 equip_weapon_recs_find(u8 kind, u16 item_id, PlSkillRec** blade, PlWeaponRec** gun)
{
    *blade = NULL;
    *gun = NULL;
    switch (kind) {
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
        *blade = (PlSkillRec*)equip_blade_rec_find(kind, item_id);
        return 0;
    case 11:
    case 12:
    case 13:
        *gun = (PlWeaponRec*)equip_weapon_rec_find(kind, item_id);
        return 1;
    }
    return -1;
}

/* 0x8027F32C (0xA0): puts decoration `deco` into the piece's first empty slot when it fits. */
void equip_deco_attach(_EQUIP* equip, u16 deco)
{
    s32 free = equip_deco_slots_free(equip);
    u8 slots;
    s32 i;

    if ((s32)item_species_class_get(deco) <= free) {
        slots = equip_deco_slots_get(equip);
        for (i = 0; i < slots; i++) {
            if (equip->skill_id[i] == 0) {
                equip->skill_id[i] = deco;
                return;
            }
        }
    }
}

/* 0x8027F3CC (0x17C): packs the piece's decorations towards slot 0. */
void equip_deco_pack(_EQUIP* equip)
{
    s32 slots = equip_deco_slots_get(equip);
    u16 sum = 0;
    s32 i;

    for (i = 0; i < slots; i++) {
        sum += equip->skill_id[i];
    }
    if (sum != 0) {
        if (equip->skill_id[0] == 0 && slots >= 2) {
            if (equip->skill_id[1] != 0) {
                equip->skill_id[0] = equip->skill_id[1];
                equip->skill_id[1] = 0;
            } else if (equip->skill_id[2] != 0) {
                equip->skill_id[0] = equip->skill_id[2];
                equip->skill_id[2] = 0;
            }
        }
        if (equip->skill_id[1] == 0 && equip->skill_id[2] != 0 && slots >= 3) {
            equip->skill_id[1] = equip->skill_id[2];
            equip->skill_id[2] = 0;
        }
    }
}

/* 0x8027F5C4 (0xB0): lists the piece's decorations into `ids` (three entries, cleared first): one entry per slot
 * a decoration fills (`by_slot` 0), or one per decoration. */
void equip_deco_ids_get(_EQUIP* equip, u16* ids, s32 by_slot)
{
    s32 slots;
    s32 i;
    s32 j;
    ItemSpeciesRecord* data;

    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    slots = equip_deco_slots_get(equip);
    for (i = 0; i < slots; i++) {
        if (equip->skill_id[i] != 0) {
            if (by_slot == 0) {
                data = get_item_species_data(equip->skill_id[i]);
                for (j = 0; j < data->slot_size; j++) {
                    *ids++ = equip->skill_id[i];
                }
            } else {
                *ids++ = equip->skill_id[i];
            }
        }
    }
}

/* 0x8027F674 (0x104): a random skill of talisman skill group `group`, different from `exclude` when that is in the
 * group. */
u16 talisman_skill_pick(u8 group, u16 exclude)
{
    u16* skill = talisman_skill_lists[group];
    u16 count = 0;
    u16 at = 0xFFFF;
    u16 pick;

    while (*skill != 0xFFFF) {
        if (exclude != 0 && *skill == exclude) {
            at = count;
        }
        count++;
        skill++;
    }
    if (at == 0xFFFF) {
        pick = ran_suu(1) % count;
    } else {
        pick = (at + ran_suu(1) % (count - 1) + 1) % count;
    }
    return talisman_skill_lists[group][pick];
}

/* 0x8027F778 (0x280): rolls a talisman from roll table `table` into `out`: the weighted row, its slots and up to
 * two skills with their points. */
void talisman_roll(_EQUIP* out, u8 table)
{
    EquipTalismanRow* row = talisman_roll_tables[table];
    s16 total = 0;
    u16 roll;
    u16 n;

    while (row->weight > 0) {
        total += row->weight;
        row++;
    }
    roll = ran_suu(1) % total;
    row = talisman_roll_tables[table];
    while (roll >= row->weight) {
        roll -= row->weight;
        row++;
    }
    out->kind = 6;
    out->item_id = row->id + 1;
    if (row->slot_mode <= 1) {
        out->deco_count = row->slot_mode;
    } else if (row->slot_mode <= 2) {
        if ((ran_suu(1) & 1) != 0) {
            out->deco_count = 2;
        } else {
            out->deco_count = 1;
        }
    } else if ((ran_suu(1) & 1) != 0) {
        if (ran_suu(1) % 100 < 20) {
            out->deco_count = 3;
        } else {
            out->deco_count = 2;
        }
    } else {
        out->deco_count = 1;
    }
    out->deco_level = 0;
    out->skill_id[0] = 0;
    out->skill_id[1] = 0;
    out->skill_id[2] = 0;
    n = out->deco_count;
    if (n < 3) {
        if (row->skill1_points == 0) {
            return;
        }
        if (row->skill1_group != 0) {
            out->skill_id[n] = talisman_skill_pick(row->skill1_group - 1, 0);
            out->deco_level = (u8)(row->skill1_points + 10);
            n++;
        }
        if (n < 3) {
            if (row->skill2_points == 0) {
                return;
            }
            if (row->skill2_group != 0) {
                out->skill_id[n] = talisman_skill_pick(row->skill2_group - 1, out->skill_id[n - 1]);
                out->deco_level |= (u16)((u8)(row->skill2_points + 10) << 8);
            }
        }
    }
}

/* 0x8027F9F8 (0xCC): rolls a piece from drop table `table` into `out` (the weighted row's kind and id). */
void equip_drop_roll(_EQUIP* out, u8 table)
{
    EquipDropRow* row = equip_drop_roll_tables[table];
    s16 total = 0;
    u16 roll;

    while (row->weight > 0) {
        total += row->weight;
        row++;
    }
    roll = ran_suu(1) % total;
    row = equip_drop_roll_tables[table];
    while (roll >= row->weight) {
        roll -= row->weight;
        row++;
    }
    out->kind = row->kind;
    out->item_id = row->item_id;
}

/* 0x8027FAC4 (0xC0): builds the piece an item grants (the two drop rolls, the three talisman rolls) into `out`. */
void equip_from_item_id(u16 item_id, _EQUIP* out)
{
    memset(out, 0, sizeof(_EQUIP));
    switch (item_id) {
    case 110:
        equip_drop_roll(out, 0);
        break;
    case 111:
        equip_drop_roll(out, 1);
        break;
    case 440:
        talisman_roll(out, 0);
        break;
    case 441:
        talisman_roll(out, 1);
        break;
    case 442:
        talisman_roll(out, 2);
        break;
    }
}

/* 0x8027FB84 (0xEC): the level cap of an armour piece (kinds 1-5) for the player's rank band, less one. */
u8 fn_8027FB84(u8 kind, u16 item_id, u8 level)
{
    PlSlotRec* slot;
    PlArmorLevelRec* row;
    u8 cap = 0;

    if ((u32)(kind - 1) <= 4) {
        slot = (PlSlotRec*)equip_slot_rec_find(kind, item_id);
        row = (PlArmorLevelRec*)equip_armor_ex_rec_find(kind, item_id);
        switch (userdata_rank_band_get()) {
        case 0:
            cap = row->stage_end[2];
            break;
        case 1:
            cap = row->stage_end[2];
            break;
        case 2:
            if (slot->top_step != 0) {
                if (level + 1 == row->stage_end[2]) {
                    cap = row->stage_end[2] + 1;
                } else {
                    cap = row->stage_end[4];
                }
            } else {
                cap = row->stage_end[4];
            }
            break;
        }
    }
    if (cap != 0) {
        cap--;
    }
    return cap;
}

/* 0x8027FE50 (0xD0): the level cap of a bowgun frame (kind 11) for the player's rank band: 3, or 6 at the top band
 * for a frame whose growth row is not flagged. */
u8 fn_8027FE50(u8 kind, u16 item_id)
{
    s32 row;
    s16* table;
    u8 cap = 0;

    if (kind != 11) {
        return 0;
    }
    row = ((PlWeaponRec*)equip_weapon_rec_find(kind, item_id))->growth_row * 7;
    table = gun_level_bonus_tbl;
    switch (userdata_rank_band_get()) {
    case 0:
        if (table[row] == 0) {
            cap = 3;
        }
        break;
    case 1:
        if (table[row] == 0) {
            cap = 3;
        }
        break;
    case 2:
        cap = 3;
        if (table[row] == 0) {
            cap = 6;
        }
        break;
    }
    return cap;
}

} /* extern "C" */

} /* namespace s_8027D684 */

#pragma optimization_level reset

/* ==== 0x80280050-0x802840DC: the weapon act machines ==== */

#include "Pl/pl_hit_result_best_get.h"
#include "camera/camera_main.h" /* camera_pl_quake_req (rule 2: the owner is `camera/camera_main.cpp`) */
#include "ef/eft004.h"          /* eft004_set_pl2 (rule 2: the owner is `ef/eft004_fx.cpp`) */
#include "g3d/g3d_calcworld.h"  /* addVec3To (rule 2) */
#include "ef/eft029_fx.h"       /* eft029_set_scale/_kaihou/_half (rule 2) */
#include "sound/pl_se_charge_level_req.h"

/* One 16-byte row of a weapon-0 act's motion table: the plain motion with its two set-up values and the frames it
 * adds to the actor's frame counters, then the motion the step table drives with its two values and the attack
 * entry it arms.  size: 0x10 (the handlers index the table with `slwi r6,kind,4`) */
typedef struct PlWeapon0Row {
    /* +0x0 */ u16 motion;        /* 0xFFFF: no plain motion, the row goes straight to its step half */
    /* +0x2 */ s16 motion_a;
    /* +0x4 */ s16 motion_b;
    /* +0x6 */ u16 frame_add;     /* frames added to the actor's +0x058/+0x0A8 counters with the plain motion */
    /* +0x8 */ u16 step_motion;
    /* +0xA */ s16 step_motion_a;
    /* +0xC */ s16 step_motion_b;
    /* +0xE */ s16 attack;        /* the attack value `pl_weapon0_attack_set` arms (0: none) */
} PlWeapon0Row;

/* One 4-byte pair of a weapon-0 motion table: a motion (or a first value) and its argument.  size: 0x4 */
typedef struct PlWeapon0Pair {
    /* +0x0 */ s16 first;
    /* +0x2 */ s16 second;
} PlWeapon0Pair;

/* One 4-byte row of the charge table: the gauge frames a stage needs (9999 ends the table) and the stage.
 * size: 0x4 */
typedef struct PlChargeRow {
    /* +0x0 */ s16 frames;
    /* +0x2 */ s16 stage;
} PlChargeRow;

/* The weapon-0 band's own `.data` tables (this unit's range, read until the section is emitted): the attack-id
 * list and attack table `pl_hit_attack_build` walks, and the per-act motion rows with their step tables. */
extern "C" {
extern s32 pl_weapon0_hit_ids[3];
extern u8 pl_weapon0_hit_table[];
extern PlWeapon0Row pl_weapon0_rows_a[];
extern u8 pl_weapon0_steps_a[];
extern PlWeapon0Row pl_weapon0_rows_b[];
extern u8 pl_weapon0_steps_b[];
extern PlWeapon0Pair pl_weapon0_charge_motions[];
extern PlWeapon0Pair pl_weapon0_charge_release_args[];
extern u8 pl_weapon0_steps_charge[];
extern u8 pl_weapon0_steps_lunge[];
extern PlWeapon0Row pl_weapon0_rows_c[];
extern u8 pl_weapon0_steps_c[];
extern PlWeapon0Row pl_weapon0_rows_d[];
extern u8 pl_weapon0_steps_d[];
extern PlWeapon0Row pl_weapon0_rows_e[];
extern u8 pl_weapon0_steps_e[];
extern u8 pl_weapon0_steps_kick[];
extern u8 pl_weapon0_steps_kick2[];
extern PlChargeRow pl_weapon0_charge_table[];
extern s16 pl_weapon0_charge_attacks[16];
extern u8 pl_weapon0_charge_shell_ids[8];
extern u8 pl_weapon0_steps_release[];
extern u8 pl_weapon0_steps_slam[];
extern u8 pl_weapon0_steps_release2[];
extern u8 pl_weapon0_steps_tackle[];
extern u8 pl_weapon0_steps_slam2[];
extern u8 pl_weapon0_steps_release4[];

/* The band's `.sdata2` pool, 0x8079A120-0x8079A1B0 (this unit's range): declared as loads, never defined
 * (playbook 29). */
extern const f32 pl_weapon0_float_zero; /* 0x8079A120: 0.0 */
extern const f32 pl_weapon0_float_neg1; /* 0x8079A124: -1.0 */
extern const f32 pl_weapon0_frame_106; /* 0x8079A128: 106.0 */
extern const f32 pl_weapon0_float_1_3; /* 0x8079A12C: 1.3 */
extern const f32 pl_weapon0_float_neg150; /* 0x8079A130: -150.0 */
extern const f32 pl_weapon0_float_1_2; /* 0x8079A134: 1.2 */
extern const f32 pl_weapon0_frame_10; /* 0x8079A138: 10.0 */
extern const f32 pl_weapon0_frame_24; /* 0x8079A13C: 24.0 */
extern const f32 pl_weapon0_frame_26; /* 0x8079A140: 26.0 */
extern const f32 pl_weapon0_frame_58; /* 0x8079A144: 58.0 */
extern const f32 pl_weapon0_frame_42; /* 0x8079A148: 42.0 */
extern const f32 pl_weapon0_float_160; /* 0x8079A14C: 160.0 */
extern const f32 pl_weapon0_float_1; /* 0x8079A150: 1.0 */
extern const f32 pl_weapon0_float_150; /* 0x8079A154: 150.0 */
extern const f32 pl_weapon0_frame_48; /* 0x8079A158: 48.0 */
extern const f32 pl_weapon0_frame_16; /* 0x8079A15C: 16.0 */
extern const f32 pl_weapon0_float_8; /* 0x8079A160: 8.0 */
extern const f32 pl_weapon0_frame_28; /* 0x8079A164: 28.0 */
extern const f32 pl_weapon0_float_18; /* 0x8079A168: 18.0 */
extern const f32 pl_weapon0_float_0_8; /* 0x8079A16C: 0.8 */
extern const f32 pl_weapon0_float_2; /* 0x8079A178: 2.0 */
extern const f32 pl_weapon0_frame_34; /* 0x8079A17C: 34.0 */
extern const f32 pl_weapon0_float_0_5; /* 0x8079A180: 0.5 */
extern const f32 pl_weapon0_frame_40; /* 0x8079A184: 40.0 */
extern const f32 pl_weapon0_float_100; /* 0x8079A188: 100.0 */
extern const f32 pl_weapon0_frame_44; /* 0x8079A18C: 44.0 */
extern const f32 pl_weapon0_float_1_7; /* 0x8079A190: 1.7 */
extern const f32 pl_weapon0_float_1_4; /* 0x8079A194: 1.4 */
extern const f32 pl_weapon0_frame_66; /* 0x8079A198: 66.0 */
extern const f32 pl_weapon0_frame_52; /* 0x8079A19C: 52.0 */
extern const f32 pl_weapon0_frame_50; /* 0x8079A1A0: 50.0 */
extern const f32 pl_weapon0_frame_54; /* 0x8079A1A4: 54.0 */
extern const f32 pl_weapon0_float_180; /* 0x8079A1A8: 180.0 */
extern const f32 pl_weapon0_frame_74; /* 0x8079A1AC: 74.0 */
}

extern "C" {

/* 0x80280050 (0x118): builds the actor's attack entry from a signed attack value; the sign or the attack-flag
 * predicate sets bit 0x10 and `kind` picks the low flag bits (7, 4 or 0x24). */
void pl_weapon0_attack_set(_PLW* self, s16 value, u8 kind)
{
    u16 magnitude;
    u16 flags;

    if (value < 0) {
        magnitude = (u16)-value;
        flags = 16;
    } else {
        magnitude = (u16)value;
        flags = (Pl_atk_act_flag_ck(self, 1) == 1) ? 16 : 0;
    }
    switch (kind) {
    case 0:
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon0_hit_table, magnitude, pl_weapon0_hit_ids,
                            (u16)(flags | 7));
        break;
    case 1:
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon0_hit_table, magnitude, pl_weapon0_hit_ids,
                            (u16)(flags | 4));
        break;
    case 2:
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon0_hit_table, magnitude, pl_weapon0_hit_ids,
                            (u16)(flags | 0x24));
        break;
    }
}

/* 0x80280168 (0xE0): applies one motion row - its plain motion, or (`mode` 1, or a row without one) its step
 * motion with `step_table` and its attack entry; `mode` 0 on the step half also advances the act step. */
void pl_weapon0_motion_row_apply(_PLW* self, u8 mode, PlWeapon0Row* row, u32 step_table)
{
    u16 motion;
    u16 add;
    s16 a;
    s16 b;
    s16 attack;

    if (mode == 1 || row->motion == 0xFFFF) {
        motion = row->step_motion;
        a = row->step_motion_a;
        b = row->step_motion_b;
        attack = row->attack;
        add = 0;
        Pl_act_set_step_table(self, step_table, 0);
        if (mode == 0) {
            self->act_step_0x05++;
        }
    } else {
        motion = row->motion;
        a = row->motion_a;
        b = row->motion_b;
        attack = 0;
        add = row->frame_add;
    }
    Pl_chr_set_attr_default(self, motion, a, b);
    if (add != 0) {
        self->field_0x058 += add;
        self->field_0x0A8 = self->field_0x058;
    }
    if (attack != 0) {
        pl_weapon0_attack_set(self, attack, 0);
    }
}

/* 0x80280248 (0x164): the hit feedback of a weapon-0 swing - the first landed hit shakes the camera (and a heavy
 * one leaves an afterimage), and a miss past `frame` notifies the hold and shakes it once. */
void pl_weapon0_hit_feedback(_PLW* self, f32 frame)
{
    u8 quake;

    if (self->field_0x565 == 0) {
        if ((u32)(self->act_no - 12) <= 3 || self->act_no == 0) {
            quake = 0x40;
        } else {
            quake = 0;
        }
        switch (pl_hit_result_best_get(self)) {
        case 2:
            self->field_0x565 = 1;
            camera_pl_quake_req(self, quake);
            break;
        case 3:
        case 4:
            self->field_0x565 = 1;
            camera_pl_quake_req(self, quake | 1);
            Pl_zanzo_set(self, self->field_0x3A2 + self->field_0x3A3 + 15, 0);
            break;
        case 0:
        case 1:
        case 5:
            self->field_0x565 = 1;
            break;
        }
    }
    if (self->field_0x566 == 0 && self->field_0x565 == 0 && frame > pl_weapon0_float_zero &&
        Pl_frame_check(self, 0, frame, pl_weapon0_float_zero) == 1) {
        self->field_0x566 = 1;
        s_80273B14::pl_act_hold_notify(self, 2);
        camera_pl_quake_req(self, 0);
    }
}

/* 0x802803AC (0x110): the three-step act of table A: arm the motion and its gate and apply the `kind` row, apply
 * its step half when the motion ends, then hand back to the idle slot or run the hit feedback. */
void pl_weapon0_act_rows_a(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 70);
        pl_weapon0_motion_row_apply(self, 0, &pl_weapon0_rows_a[kind], (u32)pl_weapon0_steps_a);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon0_motion_row_apply(self, 1, &pl_weapon0_rows_a[kind], (u32)pl_weapon0_steps_a);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x802804BC (0x1BC): the table-B act: as table A with the frame timer armed, and a miss at frame 106 spawns the
 * swing's trail effects. */
void pl_weapon0_act_rows_b(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 70);
        pl_weapon0_motion_row_apply(self, 0, &pl_weapon0_rows_b[kind], (u32)pl_weapon0_steps_b);
        pl_act_set_frame_timer(self);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon0_motion_row_apply(self, 1, &pl_weapon0_rows_b[kind], (u32)pl_weapon0_steps_b);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_frame_106);
            if (pl_hit_result_best_get(self) == 0xFF && Pl_frame_check(self, 0, pl_weapon0_frame_106, pl_weapon0_float_zero) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 13, 7, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_neg150, 0);
                    eft004_set_pl2(self, 3, 7, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_neg150, 0);
                } else {
                    eft004_set_pl2(self, 11, 7, pl_weapon0_float_1_2, pl_weapon0_float_zero, pl_weapon0_float_neg150, 0);
                }
            }
        }
        break;
    }
}

} /* extern "C" */

extern "C" {

/* 0x80280678 (0x470): the charged swing: arm the motion and the `kind` charge motion (kinds 4/5 turn the actor
 * by 0x2000), step it through its frame and motion-end gates, release it with its attack entry, then hand back to
 * the idle slot or run the hit feedback and the swing trail. */
void pl_weapon0_act_charge(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        self->field_0x18 = 1;
        pl_act_clear_flag5bb(self);
        pl_act_set_motion_gate(self, 70);
        self->field_0x312 = 0;
        self->field_0x310 = 0;
        Pl_chr_set_attr_default(self, (u16)pl_weapon0_charge_motions[kind].first,
                                (u16)pl_weapon0_charge_motions[kind].second, 0);
        switch (kind) {
        case 4:
            self->field_0x058 -= 0x2000;
            self->field_0x0A8 = self->field_0x058;
            break;
        case 5:
            self->field_0x058 += 0x2000;
            self->field_0x0A8 = self->field_0x058;
            break;
        }
        if (kind == 6) {
            pl_act_timer_step(self);
        }
        break;
    case 1:
        switch (kind) {
        case 0:
        case 6:
            self->act_step_0x05++;
            break;
        case 1:
            if (Pl_frame_check(self, 1, pl_weapon0_frame_10, pl_weapon0_float_zero) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x450, 2, 0);
            }
            break;
        case 2:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x44D, 2, 50);
            }
            break;
        case 3:
            if (Pl_frame_check(self, 1, pl_weapon0_frame_24, pl_weapon0_float_zero) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x469, 2, 0);
            }
            break;
        case 4:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x44D, 0, 52);
            }
            break;
        case 5:
            if (Pl_frame_check(self, 1, pl_weapon0_frame_26, pl_weapon0_float_zero) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x46A, 2, 0);
            }
            break;
        }
        break;
    case 2:
        switch (kind) {
        case 0:
        case 2:
        case 4:
            if (Pl_frame_check(self, 1, pl_weapon0_frame_58, pl_weapon0_float_zero) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x468, pl_weapon0_charge_release_args[kind].first,
                                        pl_weapon0_charge_release_args[kind].second);
                Pl_act_set_step_table(self, (u32)pl_weapon0_steps_charge, 2);
                pl_weapon0_attack_set(self, 1, 0);
            }
            break;
        case 1:
        case 3:
        case 5:
        case 6:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x468, pl_weapon0_charge_release_args[kind].first,
                                        pl_weapon0_charge_release_args[kind].second);
                Pl_act_set_step_table(self, (u32)pl_weapon0_steps_charge, 2);
                if (kind == 1) {
                    pl_weapon0_attack_set(self, -1, 0);
                } else {
                    pl_weapon0_attack_set(self, 1, 0);
                }
            }
            break;
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_frame_42);
            if (pl_hit_result_best_get(self) == 0xFF && Pl_frame_check(self, 0, pl_weapon0_frame_42, pl_weapon0_float_zero) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 13, 7, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                    eft004_set_pl2(self, 3, 7, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                } else {
                    eft004_set_pl2(self, 11, 7, pl_weapon0_float_1, pl_weapon0_float_zero, pl_weapon0_float_150, 0);
                }
            }
        }
        break;
    }
}

/* 0x80280AE8 (0xB4): the lunge: one motion with its step table and the kind-2 attack 7, then back to the idle
 * slot when the motion ends. */
void pl_weapon0_act_lunge(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x457, 4, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_lunge, 0);
        pl_weapon0_attack_set(self, 7, 2);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x80280B9C (0x230): the table-C act (motion set 3): as table A, with the kind-5 row stepping the actor forward
 * along its facing at two frames of its motion. */
void pl_weapon0_act_rows_c(_PLW* self, s32 kind)
{
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x18 = 1;
        pl_act_clear_flag5bb(self);
        pl_weapon0_motion_row_apply(self, 0, &pl_weapon0_rows_c[kind], (u32)pl_weapon0_steps_c);
        pl_act_set_motion_gate(self, 70);
        if (kind == 4) {
            pl_act_timer_step(self);
        }
        break;
    case 1:
        if (((kind == 0 || kind == 5) && Pl_frame_check(self, 1, pl_weapon0_frame_48, pl_weapon0_float_zero) == 1) ||
            (kind != 0 && Pl_motion_end_ck(self) == 1)) {
            self->act_step_0x05++;
            pl_weapon0_motion_row_apply(self, 1, &pl_weapon0_rows_c[kind], (u32)pl_weapon0_steps_c);
        } else if (Get_motion_no(self) == 1150 && kind == 5 && Pl_frame_check(self, 2, pl_weapon0_frame_16, pl_weapon0_float_zero) == 1) {
            step.x = pl_weapon0_float_zero;
            step.y = pl_weapon0_float_zero;
            step.z = pl_weapon0_float_8;
            rotVecXYZ(&step, &self->rot_0x54);
            addVec3To(&self->vec_0x03C, &step);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
            if (kind == 5 && Pl_frame_check(self, 2, pl_weapon0_frame_28, pl_weapon0_float_zero) == 1) {
                step.x = pl_weapon0_float_zero;
                step.y = pl_weapon0_float_zero;
                step.z = pl_weapon0_float_18;
                rotVecXYZ(&step, &self->rot_0x54);
                addVec3To(&self->vec_0x03C, &step);
            }
        }
        break;
    }
}

/* 0x80280DCC (0x110): the table-D act (motion set 3): the three-step row act. */
void pl_weapon0_act_rows_d(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_weapon0_motion_row_apply(self, 0, &pl_weapon0_rows_d[kind], (u32)pl_weapon0_steps_d);
        pl_act_set_motion_gate(self, 70);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon0_motion_row_apply(self, 1, &pl_weapon0_rows_d[kind], (u32)pl_weapon0_steps_d);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x80280EDC (0x110): the table-E act (motion set 3): the three-step row act. */
void pl_weapon0_act_rows_e(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_weapon0_motion_row_apply(self, 0, &pl_weapon0_rows_e[kind], (u32)pl_weapon0_steps_e);
        pl_act_set_motion_gate(self, 70);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_weapon0_motion_row_apply(self, 1, &pl_weapon0_rows_e[kind], (u32)pl_weapon0_steps_e);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x80280FEC (0xB0): the kick (motion set 0): one motion with its step table and a camera shake. */
void pl_weapon0_act_kick(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x4C5, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_kick, 0);
        camera_pl_quake_req(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8028109C (0xB0): the kick of motion set 3. */
void pl_weapon0_act_kick2(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x4F7, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_kick2, 0);
        camera_pl_quake_req(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x8028114C (0x60): plays the charge-level sound at the weapon's impact point once the charge stage rises past
 * the stage last announced (+0x368). */
void pl_weapon0_charge_se(_PLW* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    if (self->field_0x312 != 0 && self->field_0x312 > self->charge_stage_seen_0x368) {
        pl_impact_pos_get(self, &pos);
        pl_se_charge_level_req(self, &pos);
    }
}

/* 0x802811AC (0xB0): the gauge frames a charge stage needs, scaled by the charge skills (0.8 with skill 191,
 * 1.2 with skill 192); 9999 and up pass through. */
s32 pl_weapon0_charge_frames_get(_PLW* self, s16 frames)
{
    f32 v;

    if (frames >= 9999) {
        return 9999;
    }
    v = frames;
    if (Pl_Skill_ck(self, 191) == 1) {
        v *= pl_weapon0_float_0_8;
    } else if (Pl_Skill_ck(self, 192) == 1) {
        v *= pl_weapon0_float_1_2;
    }
    return (s32)v;
}

/* 0x8028125C (0x64): the charge stage the actor's gauge has reached. */
u8 pl_charge_stage_get(_PLW* self)
{
    PlChargeRow* row;

    for (row = pl_weapon0_charge_table;
         self->charge_gauge_0x310 >= (s16)pl_weapon0_charge_frames_get(self, row->frames); row++) {
    }
    return row->stage;
}

/* 0x802812C0 (0x68): for mode 0, the skill-scaled frames left until the stage-3 charge after `elapsed` frames. */
s32 pl_act_frame_get(_PLW* self, u8 mode, s32 elapsed)
{
    PlChargeRow* row;
    s16 left = 0;

    if (mode == 0) {
        row = pl_weapon0_charge_table;
    } else {
        return 0;
    }
    for (; row->frames != 9999; row++) {
        if (row->stage == 3) {
            left = row->frames - elapsed;
            if (left <= 0) {
                left = 0;
            }
            break;
        }
    }
    return pl_weapon0_charge_frames_get(self, left);
}

/* 0x80281328 (0x44): advances the charge gauge one frame, refreshes the stage and announces a new one. */
void pl_weapon0_charge_step(_PLW* self)
{
    self->charge_gauge_0x310++;
    self->field_0x312 = pl_charge_stage_get(self);
    pl_weapon0_charge_se(self);
}

} /* extern "C" */

extern "C" {

/* 0x8028136C (0x460): the charge-up stance: arm the `kind` stance motion and the skill-scaled 90-frame charge
 * budget, step the follow-up motion, then (master only) build the gauge from the start frame with the half-charge
 * effect and enter the charged act at the end frame. */
void pl_weapon0_act_charge_hold(_PLW* self, s32 kind)
{
    f32 end;
    f32 start;

    pl_act_set_motion_gate(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        self->field_0x18 = 1;
        pl_act_clear_flag5bb(self);
        self->field_0x312 = 0;
        self->field_0x310 = 0;
        self->field_0x28 = (s16)pl_weapon0_charge_frames_get(self, 90);
        switch (kind) {
        case 0:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x44D, 4, 0);
            pl_act_set_frame_timer(self);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x46B, 2, 0);
            s_8027D684::pl_atk_act_flag_set(self, 1);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x460, 4, 0);
            break;
        case 3:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x45E, 4, 0);
            break;
        case 4:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x44D, 4, 0);
            pl_act_set_frame_timer(self);
            s_8027D684::pl_atk_act_flag_set(self, 1);
            break;
        case 5:
            self->act_step_0x05++;
            self->field_0x058 += 0x2000;
            self->field_0x0A8 = (u16)self->field_0x058;
            Pl_chr_set_attr_default(self, 0x464, 0, 0);
            break;
        case 6:
            self->field_0x058 -= 0x2000;
            self->field_0x0A8 = (u16)self->field_0x058;
            Pl_chr_set_attr_default(self, 0x466, 0, 0);
            break;
        }
        break;
    case 1:
        switch (kind) {
        default:
            self->act_step_0x05++;
            break;
        case 1:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x44D, 0, 56);
            }
            break;
        case 2:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x44D, 2, 50);
            }
            break;
        case 6:
            if (Pl_motion_end_ck(self) == 1) {
                self->act_step_0x05++;
                Pl_chr_set_attr_default(self, 0x44D, 0, 52);
            }
            break;
        }
        break;
    case 2:
        if (Pl_master_ck(self) != 0) {
            if (kind == 3) {
                end = pl_weapon0_frame_24;
                start = pl_weapon0_float_zero;
            } else if (kind == 5) {
                end = pl_weapon0_frame_26;
                start = pl_weapon0_float_2;
            } else {
                end = pl_weapon0_frame_58;
                start = pl_weapon0_frame_34;
            }
            if (Pl_frame_check(self, 1, start, pl_weapon0_float_zero) == 1) {
                if (self->field_0x007 == 0) {
                    self->field_0x007++;
                    eft029_set_half(self, 0);
                }
                if (pl_part_flag2_ck(self, 5) == 0 || pl_part_flag2_ck(self, 13) == 1) {
                    self->field_0x006 = 1;
                }
                if (self->field_0x006 == 0) {
                    self->field_0x354 = pl_weapon0_float_0_5;
                    pl_weapon0_charge_step(self);
                    if (--self->field_0x28 <= 0) {
                        self->field_0x006 = 1;
                    }
                } else {
                    self->field_0x354 = pl_weapon0_float_1;
                }
            }
            if (Pl_frame_check(self, 1, end, pl_weapon0_float_zero) == 1 && self->field_0x006 == 1) {
                self->field_0x312 = pl_charge_stage_get(self);
                self->field_0x354 = pl_weapon0_float_1;
                switch (kind) {
                case 3:
                    pl_act_enter(self, 4, 32, 4);
                    break;
                case 5:
                    pl_act_enter(self, 4, 33, 4);
                    break;
                default:
                    pl_act_enter(self, 4, 31, 260);
                    break;
                }
            }
        }
        break;
    }
}

/* 0x802817CC (0x84): arms the charged attack of row `kind` for the actor's charge stage (capped at 3) and the
 * stage's shell id. */
void pl_weapon0_charge_attack_set(_PLW* self, s32 kind)
{
    u32 stage = (self->field_0x312 <= 3) ? self->field_0x312 : 3;

    pl_weapon0_attack_set(self, pl_weapon0_charge_attacks[kind * 4 + (u16)stage], 0);
    pl_shell_ids_set(self, pl_weapon0_charge_shell_ids[(u16)stage], 0xFF);
}

/* 0x80281850 (0x388): the charged release: the `kind` release motion with the charged attack, the stage's effect
 * scale, the follow-up release, then the idle slot or the hit feedback with the stage-3 burst and swing trails. */
void pl_weapon0_act_charge_release(_PLW* self, s32 kind)
{
    VEC3 a;
    VEC3 b;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 70);
        switch (kind) {
        case 0:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x468, 2, 0);
            pl_act_set_frame_timer(self);
            Pl_act_set_step_table(self, (u32)pl_weapon0_steps_release, 0);
            pl_weapon0_charge_attack_set(self, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x469, 0, 0);
            pl_act_set_frame_timer(self);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x46A, 0, 0);
            pl_act_set_frame_timer(self);
            break;
        }
        if (self->field_0x312 >= 3) {
            eft029_set_scale(self, 2, pl_weapon0_float_1);
        } else {
            eft029_set_scale(self, 4, pl_weapon0_float_1);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_act_set_frame_timer(self);
            if (kind == 1) {
                Pl_chr_set_attr_default(self, 0x468, 2, 8);
            } else {
                Pl_chr_set_attr_default(self, 0x468, 0, 10);
            }
            Pl_act_set_step_table(self, (u32)pl_weapon0_steps_release, 0);
            pl_weapon0_charge_attack_set(self, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 8, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_frame_42);
            if (pl_hit_result_best_get(self) == 0xFF) {
                if (self->field_0x312 >= 3) {
                    if (Pl_frame_check(self, 0, pl_weapon0_frame_40, pl_weapon0_float_zero) == 1) {
                        eft004_set_pl(self, 16, pl_weapon0_float_1, pl_weapon0_float_zero, pl_weapon0_float_100, 0);
                    }
                    if (Pl_frame_check(self, 0, pl_weapon0_frame_44, pl_weapon0_float_zero) == 1) {
                        eft029_set_kaihou(self, 6);
                    }
                }
                if (Pl_frame_check(self, 0, pl_weapon0_frame_42, pl_weapon0_float_zero) == 1) {
                    if ((self->field_0x5A4 & 6) != 0) {
                        if (self->field_0x312 >= 3) {
                            eft004_set_pl(self, 13, pl_weapon0_float_1_7, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                            eft004_set_pl(self, 3, pl_weapon0_float_1_7, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                        } else {
                            eft004_set_pl(self, 13, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                            eft004_set_pl(self, 3, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                        }
                    } else if (self->field_0x312 >= 3) {
                        eft004_set_pl(self, 7, pl_weapon0_float_1_4, pl_weapon0_float_zero, pl_weapon0_float_150, 0);
                    } else {
                        eft004_set_pl(self, 11, pl_weapon0_float_1_2, pl_weapon0_float_zero, pl_weapon0_float_150, 0);
                    }
                }
            }
        }
        break;
    }
}

/* 0x80281BD8 (0x180): the guard-slam act: the `kind` motion with the attack 11 entry and its step table, a second
 * slam when the motion ends, then the idle slot or the hit feedback. */
void pl_weapon0_act_slam(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 70);
        pl_shell_ids_set(self, 0, 100);
        switch (kind) {
        case 0:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x46C, 4, 0);
            pl_weapon0_attack_set(self, 11, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon0_steps_slam, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x46E, 8, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x46C, 0, 34);
            pl_weapon0_attack_set(self, 11, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon0_steps_slam, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x80281D58 (0x1AC): the second charge-up stance (motion set 0): as the stance act with one motion, entering
 * act 36 at frame 66. */
void pl_weapon0_act_charge_hold2(_PLW* self)
{
    pl_act_set_motion_gate(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x46D, 4, 0);
        self->field_0x312 = 0;
        self->field_0x310 = 0;
        self->field_0x28 = (s16)pl_weapon0_charge_frames_get(self, 90);
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_weapon0_frame_34, pl_weapon0_float_zero) == 1) {
            if (self->field_0x007 == 0) {
                self->field_0x007++;
                eft029_set_half(self, 0);
            }
            if (pl_part_flag2_ck(self, 17) == 0 || pl_part_flag2_ck(self, 13) == 1) {
                self->field_0x006 = 1;
            }
            if (self->field_0x006 == 0) {
                self->field_0x354 = pl_weapon0_float_0_5;
                pl_weapon0_charge_step(self);
                if (--self->field_0x28 <= 0) {
                    self->field_0x006 = 1;
                }
            } else {
                self->field_0x354 = pl_weapon0_float_1;
            }
        }
        if (Pl_frame_check(self, 1, pl_weapon0_frame_66, pl_weapon0_float_zero) == 1 && self->field_0x006 == 1 &&
            Pl_master_ck(self) == 1) {
            self->field_0x312 = pl_charge_stage_get(self);
            self->field_0x354 = pl_weapon0_float_1;
            pl_act_enter(self, 4, 36, 0);
        }
        break;
    }
}

/* 0x80281F04 (0x274): the second charged release (motion set 0): one release motion with the row-1 charged
 * attack, then the idle slot or the hit feedback with the stage-3 burst and swing trails. */
void pl_weapon0_act_charge_release2(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_act_set_motion_gate(self, 70);
        Pl_chr_set_attr_default(self, 0x46F, 4, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_release2, 0);
        pl_weapon0_charge_attack_set(self, 1);
        if (self->field_0x312 >= 3) {
            eft029_set_scale(self, 2, pl_weapon0_float_1);
        } else {
            eft029_set_scale(self, 4, pl_weapon0_float_1);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_frame_52);
            if (pl_hit_result_best_get(self) == 0xFF) {
                if (self->field_0x312 >= 3) {
                    if (Pl_frame_check(self, 0, pl_weapon0_frame_50, pl_weapon0_float_zero) == 1) {
                        eft004_set_pl(self, 16, pl_weapon0_float_1, pl_weapon0_float_zero, pl_weapon0_float_100, 0);
                    }
                    if (Pl_frame_check(self, 0, pl_weapon0_frame_54, pl_weapon0_float_zero) == 1) {
                        eft029_set_kaihou(self, 6);
                    }
                }
                if (Pl_frame_check(self, 0, pl_weapon0_frame_52, pl_weapon0_float_zero) == 1) {
                    if ((self->field_0x5A4 & 6) != 0) {
                        if (self->field_0x312 >= 3) {
                            eft004_set_pl(self, 13, pl_weapon0_float_1_7, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                            eft004_set_pl(self, 3, pl_weapon0_float_1_7, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                        } else {
                            eft004_set_pl(self, 13, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                            eft004_set_pl(self, 3, pl_weapon0_float_1_3, pl_weapon0_float_zero, pl_weapon0_float_160, 0);
                        }
                    } else if (self->field_0x312 >= 3) {
                        eft004_set_pl(self, 7, pl_weapon0_float_1_4, pl_weapon0_float_zero, pl_weapon0_float_150, 0);
                    } else {
                        eft004_set_pl(self, 11, pl_weapon0_float_1_2, pl_weapon0_float_zero, pl_weapon0_float_180, 0);
                    }
                }
            }
        }
        break;
    }
}

/* 0x80282178 (0x258): the charge-up stance of motion set 3: the `kind` motion, then (master only) build the
 * gauge from frame 48 and enter act 48 once the charge stops. */
void pl_weapon0_act_charge_hold3(_PLW* self, s32 kind)
{
    pl_act_set_motion_gate(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x18 = 1;
        self->field_0x312 = 0;
        self->field_0x310 = 0;
        self->field_0x28 = (s16)pl_weapon0_charge_frames_get(self, 90);
        switch (kind) {
        case 0:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x47E, 4, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x48C, 4, 0);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x48A, 4, 0);
            break;
        case 3:
            s_8027D684::pl_atk_act_flag_set(self, 1);
            Pl_chr_set_attr_default(self, 0x48F, 4, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x47E, 2, 48);
        }
        break;
    case 2:
        if (Pl_master_ck(self) != 0 && Pl_frame_check(self, 1, pl_weapon0_frame_48, pl_weapon0_float_zero) == 1) {
            if (self->field_0x007 == 0) {
                self->field_0x007++;
                eft029_set_half(self, 0);
            }
            if (pl_part_flag2_ck(self, 5) == 0 || pl_part_flag2_ck(self, 13) == 1) {
                self->field_0x006 = 1;
            }
            if (self->field_0x006 == 0) {
                self->field_0x354 = pl_weapon0_float_0_5;
                pl_weapon0_charge_step(self);
                if (--self->field_0x28 <= 0) {
                    self->field_0x006 = 1;
                }
            }
            if (self->field_0x006 == 1) {
                self->field_0x354 = pl_weapon0_float_1;
                self->field_0x312 = pl_charge_stage_get(self);
                pl_act_enter(self, 4, 48, 0x184);
            }
        }
        break;
    }
}

/* 0x802823D0 (0xFC): the charged release of motion set 3 (row 2). */
void pl_weapon0_act_charge_release3(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_motion_gate(self, 70);
        Pl_chr_set_attr_default(self, 0x48E, 2, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_c, 0);
        pl_weapon0_charge_attack_set(self, 2);
        if (self->field_0x312 >= 3) {
            eft029_set_scale(self, 2, pl_weapon0_float_1);
        } else {
            eft029_set_scale(self, 4, pl_weapon0_float_1);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x802824CC (0xC0): the shoulder tackle of motion set 3: one motion with the kind-1 attack 13, back to the
 * slot-8 idle at frame 74. */
void pl_weapon0_act_tackle(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x490, 4, 0);
        pl_weapon0_attack_set(self, 13, 1);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_tackle, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_weapon0_frame_74, pl_weapon0_float_zero) == 1) {
            Pl_act_set_motion_slot(self, 3, 8, 0);
        }
        break;
    }
}

/* 0x8028258C (0x180): the guard-slam act of motion set 3 (attack 24). */
void pl_weapon0_act_slam2(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_motion_gate(self, 70);
        pl_shell_ids_set(self, 0, 100);
        switch (kind) {
        case 0:
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x491, 4, 0);
            pl_weapon0_attack_set(self, 24, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon0_steps_slam2, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x493, 4, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x491, 0, 24);
            pl_weapon0_attack_set(self, 24, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon0_steps_slam2, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x8028270C (0x1AC): the charge-up stance of motion set 3 entering act 65 at frame 66. */
void pl_weapon0_act_charge_hold4(_PLW* self)
{
    pl_act_set_motion_gate(self, 2);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x492, 4, 0);
        self->field_0x312 = 0;
        self->field_0x310 = 0;
        self->field_0x28 = (s16)pl_weapon0_charge_frames_get(self, 90);
        break;
    case 1:
        if (Pl_frame_check(self, 1, pl_weapon0_frame_34, pl_weapon0_float_zero) == 1) {
            if (self->field_0x007 == 0) {
                self->field_0x007++;
                eft029_set_half(self, 0);
            }
            if (pl_part_flag2_ck(self, 17) == 0 || pl_part_flag2_ck(self, 13) == 1) {
                self->field_0x006 = 1;
            }
            if (self->field_0x006 == 0) {
                self->field_0x354 = pl_weapon0_float_0_5;
                pl_weapon0_charge_step(self);
                if (--self->field_0x28 <= 0) {
                    self->field_0x006 = 1;
                }
            } else {
                self->field_0x354 = pl_weapon0_float_1;
            }
        }
        if (Pl_frame_check(self, 1, pl_weapon0_frame_66, pl_weapon0_float_zero) == 1 && self->field_0x006 == 1 &&
            Pl_master_ck(self) == 1) {
            self->field_0x312 = pl_charge_stage_get(self);
            self->field_0x354 = pl_weapon0_float_1;
            pl_act_enter(self, 4, 65, 0x80);
        }
        break;
    }
}

/* 0x802828B8 (0xFC): the charged release of motion set 3 (row 3). */
void pl_weapon0_act_charge_release4(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        pl_act_set_motion_gate(self, 70);
        Pl_chr_set_attr_default(self, 0x494, 4, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon0_steps_release4, 0);
        pl_weapon0_charge_attack_set(self, 3);
        if (self->field_0x312 >= 3) {
            eft029_set_scale(self, 2, pl_weapon0_float_1);
        } else {
            eft029_set_scale(self, 4, pl_weapon0_float_1);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else {
            pl_weapon0_hit_feedback(self, pl_weapon0_float_neg1);
        }
        break;
    }
}

/* 0x802829B4 (0x228): latches the charge stage the sound last announced (+0x368) and runs the weapon-0 handler of
 * the actor's act number (0-65). */
void pl_weapon0_act_dispatch(_PLW* self)
{
    self->charge_stage_seen_0x368 = self->field_0x312;
    switch (self->act_no) {
    case 0: pl_weapon0_act_rows_a(self, 0); break;
    case 1: pl_weapon0_act_rows_b(self, 0); break;
    case 2: pl_weapon0_act_charge(self, 0); break;
    case 3: pl_weapon0_act_charge(self, 1); break;
    case 4: pl_weapon0_act_lunge(self); break;
    case 5: pl_weapon0_act_rows_c(self, 0); break;
    case 6: pl_weapon0_act_rows_d(self, 0); break;
    case 7: pl_weapon0_act_rows_e(self, 0); break;
    case 8: pl_weapon0_act_charge(self, 2); break;
    case 9: pl_weapon0_act_charge(self, 3); break;
    case 10: pl_weapon0_act_charge(self, 4); break;
    case 11: pl_weapon0_act_charge(self, 5); break;
    case 12: pl_weapon0_act_rows_a(self, 1); break;
    case 13: pl_weapon0_act_rows_a(self, 2); break;
    case 14: pl_weapon0_act_rows_a(self, 3); break;
    case 15: pl_weapon0_act_rows_a(self, 4); break;
    case 16: pl_weapon0_act_rows_b(self, 1); break;
    case 17: pl_weapon0_act_rows_b(self, 2); break;
    case 18: pl_weapon0_act_rows_b(self, 3); break;
    case 19: pl_weapon0_act_rows_b(self, 4); break;
    case 20: pl_weapon0_act_rows_c(self, 1); break;
    case 21: pl_weapon0_act_kick(self); break;
    case 22: pl_weapon0_act_charge(self, 6); break;
    case 23: pl_weapon0_act_kick2(self); break;
    case 24: pl_weapon0_act_charge_hold(self, 0); break;
    case 25: pl_weapon0_act_charge_hold(self, 1); break;
    case 26: pl_weapon0_act_charge_hold(self, 2); break;
    case 27: pl_weapon0_act_charge_hold(self, 3); break;
    case 28: pl_weapon0_act_charge_hold(self, 4); break;
    case 29: pl_weapon0_act_charge_hold(self, 5); break;
    case 30: pl_weapon0_act_charge_hold(self, 6); break;
    case 31: pl_weapon0_act_charge_release(self, 0); break;
    case 32: pl_weapon0_act_charge_release(self, 1); break;
    case 33: pl_weapon0_act_charge_release(self, 2); break;
    case 34: pl_weapon0_act_slam(self, 0); break;
    case 35: pl_weapon0_act_charge_hold2(self); break;
    case 36: pl_weapon0_act_charge_release2(self, 0); break;
    case 37: pl_weapon0_act_rows_c(self, 2); break;
    case 38: pl_weapon0_act_rows_c(self, 3); break;
    case 39: pl_weapon0_act_rows_d(self, 1); break;
    case 40: pl_weapon0_act_rows_d(self, 2); break;
    case 41: pl_weapon0_act_rows_e(self, 1); break;
    case 42: pl_weapon0_act_rows_e(self, 2); break;
    case 43: pl_weapon0_act_rows_c(self, 4); break;
    case 44: pl_weapon0_act_charge_hold3(self, 0); break;
    case 45: pl_weapon0_act_charge_hold3(self, 1); break;
    case 46: pl_weapon0_act_charge_hold3(self, 2); break;
    case 47: pl_weapon0_act_charge_hold3(self, 3); break;
    case 48: pl_weapon0_act_charge_release3(self, 0); break;
    case 49: pl_weapon0_act_tackle(self); break;
    case 50: pl_weapon0_act_slam(self, 1); break;
    case 51: pl_weapon0_act_charge_release2(self, 1); break;
    case 52: pl_weapon0_act_charge_release2(self, 2); break;
    case 53: pl_weapon0_act_rows_b(self, 5); break;
    case 54: pl_weapon0_act_rows_b(self, 6); break;
    case 55: pl_weapon0_act_rows_a(self, 5); break;
    case 56: pl_weapon0_act_rows_a(self, 6); break;
    case 57: pl_weapon0_act_rows_c(self, 5); break;
    case 58: pl_weapon0_act_slam2(self, 0); break;
    case 59: pl_weapon0_act_slam2(self, 1); break;
    case 60: pl_weapon0_act_rows_e(self, 3); break;
    case 61: pl_weapon0_act_rows_e(self, 3); break;
    case 62: pl_weapon0_act_rows_d(self, 3); break;
    case 63: pl_weapon0_act_rows_d(self, 3); break;
    case 64: pl_weapon0_act_charge_hold4(self); break;
    case 65: pl_weapon0_act_charge_release4(self, 0); break;
    }
}

} /* extern "C" */

/* ---- 0x80282BDC-0x802840DC: the weapon-1 act handlers (their dispatcher, `pl_weapon1_act_dispatch`, is the first
 * function of `Pl/fn_802840DC.cpp`) ---- */

/* The weapon-1 handlers' attack table and attack-id list (this unit's `.data`/`.sdata`), and their step tables -
 * the `.data` run 0x805C8134-0x805C9118 that no registered unit claims (`unsplit/Pl.h`). */
extern "C" {
extern u8 pl_weapon1_hit_table[];
extern s32 pl_weapon1_hit_ids[2];

/* The weapon-1 band's `.sdata2` pool, 0x8079A1B0-0x8079A1D4 (this unit's range): loads, never definitions. */
extern const f32 pl_weapon1_frame_52; /* 0x8079A1B0: 52.0 */
extern const f32 pl_weapon1_float_zero; /* 0x8079A1B4: 0.0 */
extern const f32 pl_weapon1_frame_18; /* 0x8079A1B8: 18.0 */
extern const f32 pl_weapon1_frame_76; /* 0x8079A1BC: 76.0 */
extern const f32 pl_weapon1_float_neg10; /* 0x8079A1C0: -10.0 */
extern const f32 pl_weapon1_float_2pi; /* 0x8079A1C4: 6.2831855 */
extern const f32 pl_weapon1_float_90; /* 0x8079A1C8: 90.0 */
extern const f32 pl_weapon1_frame_58; /* 0x8079A1CC: 58.0 */
extern const f32 pl_weapon1_float_360; /* 0x8079A1D0: 360.0 */
}

extern "C" {

/* 0x80282BDC (0x68): builds the actor's weapon-1 attack entry from a signed attack value; the sign sets bit 0x10
 * and `kind` picks the low flag bits (5 or 7). */
void pl_weapon1_attack_set(_PLW* self, s16 value, u8 kind)
{
    s16 magnitude;
    u16 flags;

    if (value < 0) {
        magnitude = -value;
        flags = 16;
    } else {
        magnitude = value;
        flags = 0;
    }
    if (kind != 0) {
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon1_hit_table, magnitude, pl_weapon1_hit_ids,
                            flags | 5);
    } else {
        pl_hit_attack_build(self, &self->hit_0x484, pl_weapon1_hit_table, magnitude, pl_weapon1_hit_ids,
                            flags | 7);
    }
}

/* 0x80282C44 (0x88): latches the first landed hit of a weapon-1 swing (+0x565); always 0. */
u32 pl_weapon1_hit_latch(_PLW* self)
{
    if (self->field_0x565 == 0) {
        switch (pl_hit_result_best_get(self)) {
        case 2:
            self->field_0x565 = 1;
            break;
        case 3:
        case 4:
            self->field_0x565 = 1;
            break;
        case 0:
        case 1:
        case 5:
            self->field_0x565 = 1;
            break;
        }
    }
    return 0;
}

/* 0x80282CCC (0x20C): the weapon-1 combo: the `kind` swing with its attack entry and step table (kind 2 also
 * opens the shell ids), back to the slot-6 idle when it ends; kind 2 fires its second attack at frame 52. */
void pl_weapon1_act_combo(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x44D, 4, 0);
            pl_weapon1_attack_set(self, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo_a, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x44E, 2, 0);
            pl_weapon1_attack_set(self, 2, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo_b, 0);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x44F, 2, 0);
            pl_weapon1_attack_set(self, 3, 1);
            pl_shell_ids_set(self, 0, 100);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo_c, 0);
            break;
        case 3:
            Pl_chr_set_attr_default(self, 0x44D, 2, 10);
            pl_weapon1_attack_set(self, 1, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo_a, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else if (pl_weapon1_hit_latch(self) != 1 &&
                   Pl_frame_check(self, 0, pl_weapon1_frame_52, pl_weapon1_float_zero) == 1 && kind == 2) {
            pl_shell_ids_set(self, 0xFF, 0xFF);
            pl_weapon1_attack_set(self, 4, 0);
        }
        break;
    }
}

/* 0x80282ED8 (0x138): the weapon-1 shell swing: the `kind` swing (attack 9 or 10, kind 1) with the shell ids
 * opened, back to the idle slot when it ends. */
void pl_weapon1_act_shell_swing(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        pl_shell_ids_set(self, 0, 100);
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x45E, 4, 0);
            pl_weapon1_attack_set(self, 9, 1);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_shell_a, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x45F, 4, 0);
            pl_weapon1_attack_set(self, 10, 1);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_shell_b, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x80283010 (0x1DC): the weapon-1 double swing: attack 5, then its reverse (-5) when the motion ends; kind 1 first
 * plays a lead-in motion and kind 2 also steps the act timer. */
void pl_weapon1_act_double(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        switch (kind) {
        case 0:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x450, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_double, 0);
            pl_weapon1_attack_set(self, 5, 0);
            pl_act_set_motion_gate(self, 100);
            break;
        case 1:
            self->field_0x18 = 1;
            Pl_chr_set_attr_default(self, 0x46A, 2, 0);
            break;
        case 2:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x450, 4, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_double, 0);
            pl_weapon1_attack_set(self, 5, 0);
            pl_act_timer_step(self);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x450, 0, 8);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_double, 0);
            pl_weapon1_attack_set(self, -5, 0);
            pl_act_set_motion_gate(self, 100);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x802831EC (0x208): the weapon-1 turning swing: kinds 1/3/4 swing at once (3/4 turning the actor by 0x2000),
 * kinds 0/2 after a lead-in motion; the frame-18 check latches +0x006. */
void pl_weapon1_act_turn_swing(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        Pl_act_set_motion(self, 0, 1, 0);
        self->field_0x007 = 0;
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x464, 4, 0);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x45D, 2, 0);
            break;
        case 1:
        case 3:
        case 4:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x452, 2, 8);
            pl_weapon1_attack_set(self, 7, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_turn, 0);
            if (kind == 3) {
                self->field_0x058 += 0x2000;
                self->field_0x0A8 = self->field_0x058;
            } else if (kind == 4) {
                self->field_0x058 -= 0x2000;
                self->field_0x0A8 = self->field_0x058;
            }
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x452, 0, 16);
            pl_weapon1_attack_set(self, 7, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_turn, 0);
        } else if (Pl_frame_check(self, 1, pl_weapon1_frame_18, pl_weapon1_float_zero) == 1 &&
                   self->field_0x006 == 0) {
            self->field_0x006++;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x802833F4 (0x118): the weapon-1 second swing: the `kind` swing (attack 6 or 12) with its step table, back to
 * the idle slot when it ends. */
void pl_weapon1_act_swing2(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x460, 4, 0);
            pl_weapon1_attack_set(self, 6, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_swing2_a, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x461, 4, 0);
            pl_weapon1_attack_set(self, 12, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_swing2_b, 0);
            break;
        }
        Pl_act_set_motion(self, 0, 1, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x8028350C (0xCC): the weapon-1 chain swing (attack 11), entering act 1 of class 5 when it ends. */
void pl_weapon1_act_chain(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_chr_set_attr_default(self, 0x45C, 4, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_chain, 0);
        pl_weapon1_attack_set(self, 11, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 5, 1, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x802835D8 (0xCC): the weapon-1 single swing (attack 13), back to the slot-6 idle when it ends. */
void pl_weapon1_act_single(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_chr_set_attr_default(self, 0x451, 4, 0);
        pl_weapon1_attack_set(self, 13, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_single, 0);
        Pl_act_set_motion(self, 0, 1, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 6, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x802836A4 (0x16C): the weapon-1 combo of motion set 3 (attacks 14-16). */
void pl_weapon1_act_combo3(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x47E, 2, 0);
            pl_weapon1_attack_set(self, 14, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo3_a, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x47F, 2, 0);
            pl_weapon1_attack_set(self, 15, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo3_b, 0);
            break;
        case 2:
            Pl_chr_set_attr_default(self, 0x480, 2, 0);
            pl_weapon1_attack_set(self, 16, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo3_c, 0);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x80283810 (0x198): the weapon-1 rising swing of motion set 3 (attack 17); kind 1 bobs the actor vertically
 * along a cosine of its motion frame between frames 18 and 76. */
void pl_weapon1_act_rise(_PLW* self, s32 kind)
{
    VEC3 bob;
    f32 frame;

    VEC3_ctor(&bob);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x482, 2, 0);
        pl_weapon1_attack_set(self, 17, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_rise, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else if (pl_weapon1_hit_latch(self) != 1 && kind == 1 &&
                   Pl_frame_check(self, 3, pl_weapon1_frame_18, pl_weapon1_frame_76) == 1) {
            frame = pl_rig_get_motion_frame(self);
            bob.z = pl_weapon1_float_zero;
            bob.x = pl_weapon1_float_zero;
            bob.y = pl_weapon1_float_neg10 *
                    cosf(pl_weapon1_float_2pi * (pl_weapon1_float_90 * ((frame - pl_weapon1_frame_18) / pl_weapon1_frame_58)) /
                         pl_weapon1_float_360);
            rotVecZXY(&bob, &self->rot_0x54);
            self->vec_0x03C.x += bob.x;
            self->vec_0x03C.y += bob.y;
            self->vec_0x03C.z += bob.z;
        }
        break;
    }
}

/* 0x802839A8 (0x1E0): the weapon-1 double swing of motion set 3: attack 18, then its reverse when the motion ends;
 * kind 0 first plays a lead-in motion and kind 2 also steps the act timer. */
void pl_weapon1_act_double3(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x18 = 1;
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x488, 2, 0);
            break;
        case 1:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x481, 2, 0);
            pl_weapon1_attack_set(self, 18, 0);
            pl_act_set_motion_gate(self, 100);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_double3, 0);
            break;
        case 2:
            self->act_step_0x05 = 2;
            Pl_chr_set_attr_default(self, 0x481, 2, 0);
            pl_weapon1_attack_set(self, 18, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_double3, 0);
            pl_act_timer_step(self);
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x481, 0, 14);
            pl_act_set_motion_gate(self, 100);
            pl_weapon1_attack_set(self, -18, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_double3, 0);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x80283B88 (0xD8): the weapon-1 single swing of motion set 3 (attack 19). */
void pl_weapon1_act_single3(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x18 = 1;
        Pl_chr_set_attr_default(self, 0x484, 2, 0);
        pl_weapon1_attack_set(self, 19, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_single3, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x80283C60 (0x150): the weapon-1 finisher of motion set 3 (attack 20 with the shell ids, or 21), back to slot 10
 * (kind 0) or 4 when it ends. */
void pl_weapon1_act_finisher3(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        self->field_0x18 = 1;
        switch (kind) {
        case 0:
            Pl_chr_set_attr_default(self, 0x483, 2, 0);
            pl_shell_ids_set(self, 0, 100);
            pl_weapon1_attack_set(self, 20, 1);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_finisher3_a, 0);
            break;
        case 1:
            Pl_chr_set_attr_default(self, 0x489, 2, 0);
            pl_weapon1_attack_set(self, 21, 0);
            Pl_act_set_step_table(self, (u32)pl_weapon1_steps_finisher3_b, 0);
            break;
        }
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, kind == 0 ? 10 : 4, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x80283DB0 (0xD0): the weapon-1 chain swing of motion set 3 (attack 22), entering act 7 of class 5 when it
 * ends. */
void pl_weapon1_act_chain3(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_chr_set_attr_default(self, 0x485, 4, 0);
        pl_weapon1_attack_set(self, 22, 0);
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_chain3, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            pl_act_enter(self, 5, 7, 0x80);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

/* 0x80283E80 (0xA4): the weapon-1 kick of motion set 3. */
void pl_weapon1_act_kick3(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x4F7, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_kick3, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 4, 0);
        }
        break;
    }
}

/* 0x80283F24 (0xA4): the weapon-1 kick of motion set 0. */
void pl_weapon1_act_kick(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_act_set_motion(self, 0, 1, 0);
        Pl_chr_set_attr_default(self, 0x4C5, 0, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_kick, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x80283FC8 (0x114): the weapon-1 turning swing of motion set 3 (attack 15; kinds 1/2 turn the actor by
 * 0x2000). */
void pl_weapon1_act_turn3(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        Pl_act_set_motion(self, 3, 1, 0);
        Pl_chr_set_attr_default(self, 0x47F, 2, 2);
        pl_weapon1_attack_set(self, 15, 0);
        Pl_act_set_step_table(self, (u32)pl_weapon1_steps_combo3_b, 0);
        switch (kind) {
        case 1:
            self->field_0x058 += 0x2000;
            self->field_0x0A8 = self->field_0x058;
            break;
        case 2:
            self->field_0x058 -= 0x2000;
            self->field_0x0A8 = self->field_0x058;
            break;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_act_set_motion_slot(self, 3, 6, 0);
        } else if (pl_weapon1_hit_latch(self) == 1) {
            return;
        }
        break;
    }
}

} /* extern "C" */

/* ---- 0x8026A738-0x8026AF08: the part/motion section's pad-input helpers (the bodies of the input block that
 * `_PLW` carries at +0xB8; they sit here because the callees they share are declared further up) ---- */

/* The actor's pad-input block, `_PLW` +0xB8 (0x84 bytes, the size the input refresh clears): the button words,
 * the stick angles, the id flag word and the per-button hold counters.  size: 0x84 */
typedef struct PlInput {
    /* +0x00 */ u16 held;           /* the held buttons */
    /* +0x02 */ u16 held_alt;       /* the second held word */
    /* +0x04 */ u16 pressed;        /* the newly pressed buttons */
    /* +0x06 */ u16 pressed_prev;   /* last frame's `pressed` */
    /* +0x08 */ u16 copy_0x08[4];   /* the four words above, copied for the act layer */
    /* +0x10 */ u16 stick_flags[4]; /* the stick direction words (held, last frame's, went down, last frame's went
                                     * down); bits 2-5 are cleared while a menu holds the pad */
    /* +0x18 */ u16 stick_angle[2]; /* the pad's stick angle pair (`PlayerPad` +0x10C / +0xE6) */
    /* +0x1C */ u16 stick_dist[2];  /* the pad's stick distance pair (`PlayerPad` +0x118 / +0xF2) */
    /* +0x20 */ u16 stick_cam_angle[2]; /* `stick_angle` turned by the camera's heading */
    /* +0x24 */ u32 id_flags;       /* `_PLW::id_flags_0xDC` */
    /* +0x28 */ u32 pad_0x28[8];
    /* +0x48 */ VEC3 accel_b;
    /* +0x54 */ VEC3 accel_a;       /* cleared while a menu holds the pad */
    /* +0x60 */ u8 hold_state[8];   /* per-button hold state: 0 idle, 1 held */
    /* +0x68 */ s8 hold_count[8];   /* per-button hold frames, saturating at 100 */
    /* +0x70 */ u8 pad_kind;        /* 1: Wii Remote, 2: Classic Controller */
    /* +0x71 */ u8 option_0x71;
    /* +0x72 */ u16 extra_pressed;  /* the extension's pressed buttons */
    /* +0x74 */ u32 accel_b_state;
    /* +0x78 */ u32 accel_a_state;
    /* +0x7C */ u8 accel_flags;
    /* +0x7D */ u8 pad_0x7D[0x7];
} PlInput; /* size: 0x84 */

/* The per-button masks the hold counters follow (this unit's `.data`, 0x805C5F90). */
extern "C" {
extern u16 pl_input_button_masks[8];
/* 0x8079A018 / 0x8079A01C: 200.0 and 300.0, the remote's and the Nunchuk's swing threshold (this unit's `.sdata2`
 * pool - loads, never definitions, playbook 29). */
extern f32 pl_input_acc_swing_min;
extern f32 pl_input_fs_acc_swing_min;
}

#include "mh3_pad/pad_acc_hist_get.h"
#include "ef/sys_quest_move.h"
#include "Pl/fn_80288CEC.h"  /* root_mode2_ck (rule 2) */
#include "camera/camera.h"   /* camera_work_ck (rule 2) */

namespace s_802693C4 {

extern "C" {

/* 0x8026A738 (0x12C): raises the item-cycle ids (22/23) from the pad kind's shoulder buttons and the four
 * d-pad ids (18-21) from the pressed and extension words. */
void pl_input_trigger_flags_set(_PLW* self, PlInput* in, u8 pad)
{
    u16 dpad;

    if ((u8)get_ControlType(pad) == 1) {
        if ((in->pressed & 0x200) != 0) {
            pl_id_bit_set(self, 22);
        }
        if ((in->pressed & 0x100) != 0) {
            pl_id_bit_set(self, 23);
        }
    } else {
        if ((in->pressed & 0x20) != 0) {
            pl_id_bit_set(self, 22);
        }
        if ((in->pressed & 0x40) != 0) {
            pl_id_bit_set(self, 23);
        }
    }
    dpad = in->pressed | in->extra_pressed;
    if ((dpad & 0x2000) != 0) {
        pl_id_bit_set(self, 18);
    }
    if ((dpad & 0x1000) != 0) {
        pl_id_bit_set(self, 19);
    }
    if ((dpad & 0x800) != 0) {
        pl_id_bit_set(self, 20);
    }
    if ((dpad & 0x400) != 0) {
        pl_id_bit_set(self, 21);
    }
}

/* 0x8026A864 (0xF0): raises the second set of shoulder ids (25/26) and the two d-pad ids 27/28. */
void pl_input_trigger_flags2_set(_PLW* self, PlInput* in, u8 pad)
{
    u16 pressed;

    if ((u8)get_ControlType(pad) == 1) {
        if ((in->pressed & 0x200) != 0) {
            pl_id_bit_set(self, 25);
        }
        if ((in->pressed & 0x100) != 0) {
            pl_id_bit_set(self, 26);
        }
    } else {
        if ((in->pressed & 0x20) != 0) {
            pl_id_bit_set(self, 25);
        }
        if ((in->pressed & 0x40) != 0) {
            pl_id_bit_set(self, 26);
        }
    }
    pressed = in->pressed;
    if ((pressed & 0x800) != 0) {
        pl_id_bit_set(self, 28);
    }
    if ((pressed & 0x400) != 0) {
        pl_id_bit_set(self, 27);
    }
}

/* 0x8026A954 (0x148): while a menu holds the pad (the item frame, the three cockpit menus or the act's menu lock),
 * clears the input block's buttons, sticks and counters (the item counter keeps running in the item frame). */
void pl_input_menu_clear(_PLW* self, PlInput* in)
{
    s32 i;

    if (menu_item_frame_update((MenuFrameWork*)self) == 1 || self->field_0x5BC != 0 ||
        self->quest_menu_open_0x5BD != 0 || self->take_menu_open_0x5BE != 0 ||
        (u32)s_80273B14::pl_act_menu_lock_ck(self) == 1) {
        in->accel_a_state = 0;
        in->accel_a.x = pl_act_float_zero;
        in->accel_a.y = pl_act_float_zero;
        in->accel_a.z = pl_act_float_zero;
        in->accel_flags = 0;
        in->held = 0;
        in->held_alt = 0;
        in->pressed = 0;
        in->pressed_prev = 0;
        in->copy_0x08[0] = 0;
        in->copy_0x08[1] = 0;
        in->copy_0x08[2] = 0;
        in->copy_0x08[3] = 0;
        in->stick_flags[0] &= 0xFFC3;
        in->stick_flags[1] &= 0xFFC3;
        in->stick_flags[2] &= 0xFFC3;
        in->stick_flags[3] &= 0xFFC3;
        in->stick_angle[1] = 0;
        in->stick_dist[1] = 0;
        in->stick_cam_angle[1] = 0;
        pl_id_bit_clear(self, 11);
        pl_id_bit_clear(self, 9);
        for (i = 0; i < 8; i++) {
            if (i != 6 || menu_item_frame_update((MenuFrameWork*)self) != 1) {
                in->hold_state[i] = 0;
                in->hold_count[i] = 0;
            }
        }
    }
}

/* 0x8026AA9C (0x114): steps the per-button hold counters: a button press starts a hold (the Wii Remote's second
 * slot follows `pressed`), a held button counts up to 100 frames, a release ends the hold. */
void pl_input_button_hold_count(_PLW* self, PlInput* in)
{
    s32 i;
    u16* mask;
    u16 buttons;

    for (i = 0, mask = pl_input_button_masks; i < 8; i++) {
        switch (in->hold_state[i]) {
        case 0:
            in->hold_count[i] = 0;
            if (in->pad_kind == 1 && i == 1) {
                buttons = in->pressed;
            } else {
                buttons = in->held;
            }
            if ((buttons & *mask) != 0 && (menu_item_frame_update((MenuFrameWork*)self) == 0 || i == 6)) {
                in->hold_state[i]++;
                in->hold_count[i] = 1;
            }
            break;
        case 1:
            if ((in->held & *mask) != 0) {
                in->hold_count[i]++;
                if (in->hold_count[i] >= 100) {
                    in->hold_count[i] = 100;
                }
            } else {
                in->hold_state[i] = 0;
            }
            break;
        }
        mask++;
    }
}

/* 0x8026ABB0 (0x7C): raises ids 9 and 11 while button 0x8 has been held past 8 frames, and id 0 on a short tap. */
void pl_input_hold_flags_set(_PLW* self, PlInput* in)
{
    if ((in->held & 8) != 0) {
        if (in->hold_count[6] > 8) {
            pl_id_bit_set(self, 9);
            pl_id_bit_set(self, 11);
        }
    } else if ((u8)(in->hold_count[6] - 1) <= 7) {
        pl_id_bit_set(self, 0);
    }
}

/* 0x8026AC2C (0x1D8): raises the swing ids (5-8, then 1-4 unless a swing was raised outside the field) from the
 * pressed and held words; the motion-control option clears ids 1-4 for a remote during a guard. */
void pl_input_act_flags_set(_PLW* self, PlInput* in)
{
    s32 swung = 0;

    pl_id_bit_set(self, 9);
    if ((in->pressed & 0x800) != 0) {
        pl_id_bit_set(self, 5);
        swung = 1;
    } else if ((in->pressed & 0x400) != 0) {
        pl_id_bit_set(self, 7);
        swung = 1;
    }
    if ((in->held & 0x800) != 0) {
        pl_id_bit_set(self, 6);
        swung = 1;
    } else if ((in->held & 0x400) != 0) {
        pl_id_bit_set(self, 8);
        swung = 1;
    }
    if (swung == 0 || self->kind_0x09 == 3) {
        if ((in->pressed & 0x2000) != 0) {
            pl_id_bit_set(self, 1);
        } else if ((in->pressed & 0x1000) != 0) {
            pl_id_bit_set(self, 3);
        }
        if ((in->held & 0x2000) != 0) {
            pl_id_bit_set(self, 2);
        } else if ((in->held & 0x1000) != 0) {
            pl_id_bit_set(self, 4);
        }
    }
    if (option_cfg_eq_ck(self->chunk_ofs, 10, 1) == 1 && in->pad_kind == 1 && Pl_act_ck(self, 1, 8) == 0 &&
        self->kind_0x09 == 3 && s_8026BA1C::pl_status_bit_ck(self, 0x1C0) != 0) {
        pl_id_bit_clear(self, 1);
        pl_id_bit_clear(self, 2);
        pl_id_bit_clear(self, 3);
        pl_id_bit_clear(self, 4);
    }
}

/* 0x8026AE5C (0xAC): raises id 24 on the pad's pause button (0x40 on a Wii Remote, 0x8000 on a Classic
 * Controller). */
void pl_input_pause_flag_set(_PLW* self, u8 pad)
{
    PlayerPad* rec;

    if ((u8)get_ControlType(pad) == 1) {
        rec = &Psw[pad];
        if ((rec->field_0x100 & 0x40) != 0) {
            pl_id_bit_set(self, 24);
        }
    } else {
        rec = &Psw[pad];
        if ((rec->field_0xda & 0x8000) != 0) {
            pl_id_bit_set(self, 24);
        }
    }
}

/* 0x8026AF08 (0xA2C): refreshes the actor's pad-input block from its pad record: clears it while the actor is held,
 * otherwise copies the pad kind's buttons and sticks, raises the input ids and runs the two action dispatchers. */
void pl_input_refresh(_PLW* self, u32 value)
{
    PlInput* in = (PlInput*)&self->field_0x0B8;
    Vec3 hist[16];
    Vec3* p = hist;
    Vec3 unused;
    SysQuestMove* move;
    PlayerPad* pad;
    s32 swung;
    s16 hold;
    u16 pressed;
    u16 pressed_prev;
    u16 held;
    u16 held_alt;

    do {
        VEC3_ctor(p);
        p++;
    } while (p < &hist[16]);
    VEC3_ctor(&unused);
    move = (SysQuestMove*)get_move_work_adrs(0);
    in->id_flags = 0;
    in->pad_kind = 0;
    if (option_cfg_eq_ck(self->chunk_ofs, 6, 0) == 1) {
        in->option_0x71 = 0;
    } else {
        in->option_0x71 = 1;
    }
    in->accel_flags = 0;
    in->pad_0x28[0] = 0;
    in->pad_0x28[1] = 0;
    in->pad_0x28[2] = 0;
    in->pad_0x28[3] = 0;
    in->pad_0x28[4] = 0;
    in->pad_0x28[5] = 0;
    in->pad_0x28[6] = 0;
    in->pad_0x28[7] = 0;
    if (self->field_0x352 > 0) {
        self->field_0x352--;
    }
    self->field_0x46F = 0;
    if (GameMode_ck() == 1 && move != NULL) {
        if (root_mode2_ck() == 1) {
            if (PlayMode_ck() == 2) {
                hold = move->player_hold_0x004[self->chunk_ofs & 1];
            } else {
                hold = move->player_hold_0x004[0];
            }
            if (move->hold_off_0x001 == 0) {
                if (hold > 0 || self->field_0x453 != 0) {
                    self->field_0x46F = 1;
                }
                self->field_0x453 = hold;
            }
        } else {
            self->field_0x46F = 1;
        }
    }
    if (self->field_0x352 > 0 || system_w.field_0x2a != 0 || self->field_0x46F == 1 ||
        (GameMode_ck() == 1 && (s32)root_mode2_ck() == 0)) {
        s32 count = in->hold_count[6];
        u8 state = in->hold_state[6];

        memset(in, 0, sizeof(PlInput));
        in->hold_count[6] = count;
        in->hold_state[6] = state;
        if (self->field_0x352 > 0) {
            pl_input_pause_flag_set(self, value);
        }
        return;
    }
    if (option_cfg_eq_ck(self->chunk_ofs, 7, 1) == 1) {
        self->held_attack_0x5C5 = 1;
    } else {
        self->held_attack_0x5C5 = 0;
    }
    pl_input_pause_flag_set(self, value);
    if ((u8)get_ControlType((u8)value) == 1) {
        pad = &Psw[(u8)value];
        in->pad_kind = 1;
        in->stick_flags[1] = pad->fs_stick_prev_0x106;
        in->stick_flags[0] = pad->control_0x104;
        in->stick_flags[3] = in->stick_flags[2];
        in->stick_flags[2] = pad->fs_stick_trig_0x108;
        in->stick_angle[0] = pad->fs_angle_0x10C[0];
        in->stick_angle[1] = pad->fs_angle_0x10C[1];
        in->stick_dist[0] = pad->fs_dist_0x118[0];
        in->stick_dist[1] = pad->fs_dist_0x118[1];
        in->held_alt = held_alt = pad->fs_prev_0xFE & ~0x8000;
        in->held = held = pad->control_0x0FC & ~0x8000;
        in->pressed_prev = pressed_prev = in->pressed & ~0x8000;
        in->pressed = pressed = pad->field_0x100 & ~0x8000;
        in->extra_pressed = pad->field_0x11e & ~0x8000;
        in->copy_0x08[0] = held;
        in->copy_0x08[1] = held_alt;
        in->copy_0x08[2] = pressed;
        in->copy_0x08[3] = pressed_prev;
        pl_input_trigger_flags_set(self, in, value);
        pl_input_trigger_flags2_set(self, in, value);
        pl_input_button_hold_count(self, in);
        pl_input_hold_flags_set(self, in);
        pl_input_menu_clear(self, in);
        if (s_8026BA1C::pl_id_bit_ck(self, 11) != 1 && menu_item_frame_update((MenuFrameWork*)self) != 1) {
            pad_acc_hist_get((u8)value, hist, 4);
            copyVec3(&in->accel_a, hist);
            in->accel_a_state = pad_acc_y_delta((u8)value);
            if (vec3_len((f32*)&in->accel_a) >= pl_input_acc_swing_min) {
                in->accel_flags |= 1;
                if (in->accel_a.y < pl_act_float_zero) {
                    in->accel_flags |= 4;
                } else {
                    in->accel_flags |= 0x10;
                }
            }
            pad_fs_acc_hist_get((u8)value, hist, 4);
            copyVec3(&in->accel_b, hist);
            in->accel_b_state = pad_fs_acc_y_delta((u8)value);
            if (vec3_len((f32*)&in->accel_b) >= pl_input_fs_acc_swing_min) {
                in->accel_flags |= 2;
                if (in->accel_a.y < pl_act_float_zero) {
                    in->accel_flags |= 8;
                } else {
                    in->accel_flags |= 0x20;
                }
            }
        }
        if (s_8026BA1C::pl_id_bit_ck(self, 11) == 1) {
            u32 dpad = in->pressed | pad->field_0x11e;

            if ((dpad & 0x400) != 0) {
                pl_id_bit_set(self, 15);
            } else if ((dpad & 0x800) != 0) {
                pl_id_bit_set(self, 14);
            }
            dpad = in->pressed | pad->field_0x11e;
            if ((dpad & 0x1000) != 0) {
                pl_id_bit_set(self, 13);
            } else if ((dpad & 0x2000) != 0) {
                pl_id_bit_set(self, 12);
            }
        }
        if ((s32)pl_id_bit_ck(self, 11) == 0) {
            u16 buttons;

            pl_input_act_flags_set(self, in);
            buttons = in->held;
            if ((buttons & 0x2400) != 0) {
                pl_id_bit_set(self, 16);
            } else if ((buttons & 0x1800) != 0) {
                pl_id_bit_set(self, 17);
            }
        }
        if (self->field_0x308 != 0) {
            in->held &= 0x32C;
            in->held_alt &= 0x32C;
            in->pressed &= 0x32C;
            in->pressed_prev &= 0x32C;
        }
        if (camera_work_ck() == 1) {
            in->held &= 0xFFBF;
            in->held_alt &= 0xFFBF;
            in->pressed &= 0xFFBF;
            in->pressed_prev &= 0xFFBF;
        }
    } else {
        PlayerPad* cl = &Psw[(u8)value];

        in->pad_kind = 2;
        in->stick_flags[1] = cl->cl_stick_prev_0xE0;
        in->stick_flags[0] = cl->control_0x0DE;
        in->stick_flags[3] = in->stick_flags[2];
        in->stick_flags[2] = cl->cl_stick_trig_0xE2;
        in->stick_angle[0] = cl->cl_angle_0xE6;
        in->stick_angle[1] = cl->field_0xe8;
        in->stick_dist[0] = cl->cl_dist_0xF2[0];
        in->stick_dist[1] = cl->cl_dist_0xF2[1];
        in->held_alt = held_alt = cl->cl_prev_0xD8 & ~2;
        in->held = held = cl->control_0x0D6 & ~2;
        in->pressed_prev = pressed_prev = in->pressed & ~2;
        in->pressed = pressed = cl->field_0xda & ~2;
        in->copy_0x08[0] = held;
        in->copy_0x08[1] = held_alt;
        in->copy_0x08[2] = pressed;
        in->copy_0x08[3] = pressed_prev;
        in->extra_pressed = cl->field_0xf8;
        pl_input_trigger_flags_set(self, in, value);
        pl_input_trigger_flags2_set(self, in, value);
        pl_input_button_hold_count(self, in);
        pl_input_hold_flags_set(self, in);
        pl_input_menu_clear(self, in);
        if (s_8026BA1C::pl_id_bit_ck(self, 11) == 1) {
            u16 buttons = in->pressed;

            if ((buttons & 0x20) != 0) {
                pl_id_bit_set(self, 15);
            } else if ((buttons & 0x200) != 0) {
                pl_id_bit_set(self, 14);
            }
            buttons = in->pressed;
            if ((buttons & 0x40) != 0) {
                pl_id_bit_set(self, 13);
            } else if ((buttons & 0x100) != 0) {
                pl_id_bit_set(self, 12);
            }
        }
        pl_input_act_flags_set(self, in);
        if (s_8026BA1C::pl_stick_window_ck(self, 1, 0x6000, 0xA000) == 1 || (in->held & 0x1800) != 0) {
            pl_id_bit_set(self, 17);
        } else if (s_8026BA1C::pl_stick_window_ck(self, 1, 0xE000, 0x2000) == 1 || (in->held & 0x2400) != 0) {
            pl_id_bit_set(self, 16);
        }
        if (self->held_attack_0x5C5 != 0 && s_8026BA1C::pl_act_param_tier_ck(self, 1) >= 1) {
            swung = 0;
            if (s_8026BA1C::pl_stick_window_gated_ck(self, 1, 0x5555, 0xAAAB) == 1) {
                pl_id_bit_set(self, 5);
                swung = 1;
            } else if (s_8026BA1C::pl_stick_window_gated_ck(self, 1, 0xD555, 0x2AAB) == 1) {
                pl_id_bit_set(self, 7);
                swung = 1;
            }
            if (s_8026BA1C::pl_stick_window_level_ck(self, 1, 0x5555, 0xAAAB) == 1) {
                pl_id_bit_set(self, 6);
                swung = 1;
            } else if (s_8026BA1C::pl_stick_window_level_ck(self, 1, 0xD555, 0x2AAB) == 1) {
                pl_id_bit_set(self, 8);
                swung = 1;
            }
            if (swung == 0 || self->kind_0x09 == 3) {
                if (s_8026BA1C::pl_stick_window_gated_ck(self, 1, 0x2AAB, 0x5555) == 1) {
                    pl_id_bit_set(self, 1);
                } else if (s_8026BA1C::pl_stick_window_gated_ck(self, 1, 0xAAAB, 0xD555) == 1) {
                    pl_id_bit_set(self, 3);
                }
                if (s_8026BA1C::pl_stick_window_level_ck(self, 1, 0x2AAB, 0x5555) == 1) {
                    pl_id_bit_set(self, 2);
                } else if (s_8026BA1C::pl_stick_window_level_ck(self, 1, 0xAAAB, 0xD555) == 1) {
                    pl_id_bit_set(self, 4);
                }
            }
        }
        if (self->field_0x308 != 0) {
            in->held &= 0x94;
            in->held_alt &= 0x94;
            in->pressed &= 0x94;
            in->pressed_prev &= 0x94;
        }
        if (camera_work_ck() == 1) {
            in->held &= 0x7FFF;
            in->held_alt &= 0x7FFF;
            in->pressed &= 0x7FFF;
            in->pressed_prev &= 0x7FFF;
        }
    }
    s_8026BA1C::fn_8026BF98(self, value);
    s_8026BA1C::fn_8026CC7C(self, value);
    {
        u32 cam = ::get_camera_rot_y();

        in->stick_cam_angle[0] = in->stick_angle[0] + cam;
        in->stick_cam_angle[1] = in->stick_angle[1] + cam;
    }
}

} /* extern "C" */

} /* namespace s_802693C4 */
