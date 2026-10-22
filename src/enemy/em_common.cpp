/* enemy/em_common.cpp - the enemy common control set: the `_ENEMY_WORK` accessors and status bookkeeping
 *   (0x801251D0-0x8012BA00), the motion frame cost `fn_8012BA00`, the action/status checks (`em_act_ck`,
 *   `em_area_ck`, `em_die_ck`, `em_work_die_ck`, `ana_em_ck`, `shibire_em_ck`, 0x8012BDF4-0x8012E968), the area/group
 *   wait set (0x8012E968-0x8012EC74), the per-motion frame-window and motion/effect helpers (`em_sleep_ck`,
 *   `em_frame_check`, `shuffle1`..`shuffle6`, the joint helpers, 0x8012EC74-0x80137604) and one enemy's per-motion
 *   action/rotation accessors (0x80137604-0x8013791C).
 * RANGE. .text 0x801251D0-0x8013791C (514 functions); .ctors 0x8056F318-0x8056F31C (`fn_801378A0`, the light-table
 *   install), .rodata 0x8056F8D0-0x8056F8E0, .data 0x805A0FB0-0x805A1358 (the motion-set defaults and action tables
 *   from `lbl_805A0FB0`), .bss 0x806A4560-0x806A4590 (the light table), .sdata 0x807919D0-0x807919F0,
 *   .sdata2 0x80796C50-0x80796D40, extab, extabindex.
 * SEAM. Probably more than one TU: the `.sdata2` run jumps `lbl_80796CB4` -> `lbl_80796CB8` somewhere in the window
 *   0x8012E968-0x8012EC74 (`tudiscover` prefers 0x8012E968; the referrer sets are disjoint and ordered), the one
 *   strong observation in the band; the int-to-float magic at 0x80796C68 is loaded on both sides of it.
 * NAMES. Runtime-dump names: `get_enemy_data`, `em_act_ck`, `em_area_ck`, `em_die_ck`, `em_work_die_ck`,
 *   `ana_em_ck`(`_sub`), `shibire_em_ck`(`_sub`), `em_sleep_ck`, `em_get_mot_no`, `em_frame_check`,
 *   `em_after_frame_check`, `em_water_check`, `em_magma_check`, `UpdateValue`, `shuffle1`..`shuffle6`, `CancelFade`,
 *   `get_joint_wmat_em`, `get_joint_wpos_em`, `get_em_scale`, `get_em_chg_scale`.  The dump's
 *   `J3DColorBlockLightOff::*`, `GoalOverlay::SceneCreated`, `JASDsp::TChannel::forceStop`, `FormationPos::__ct`,
 *   `nw4hbm::...::GetCharSpace`, `cGameSFX::Init` (0x8012554C), `homebutton::MotorCallback` (0x80126844),
 *   `JASSeqCtrl::setIntrMask` (0x8012CE8C), `DBClose` (0x8012B604), `CBGetBytesAvailableForRead` (0x8012C204) and
 *   `GXGetTexObjMipMap` (0x8013023C) hits are signature matches on 4- to 36-byte bodies, not evidence; every other
 *   non-`fn_` name is a GUESS from its body.
 *   GUESS (from each body and its callers): em_record_hit_ck, em_mini_hit_ck, em_captured_ck,
 *   em_mini_kill_kind_get, em_quest_element_set, em_quest_element_set_large, em_shake_req_set,
 *   em_move_target_set, em_hit_timer_ck, em_fx_flag_set, em_motion_timer_arm, em_attack_done_set,
 *   em_attack_start, pl_torch_ck, em_hit_by_set, em_act_arm_unless_down, em_move_work_pick,
 *   em_roar_latch_set, em_area_entry_near_ck, em_owner_slot_pick, em_owner_slot_update, em_motion_frame_get,
 *   em_motion_blend_scale_get, em_mot_set_blend_ck,
 *   em_mot_layer_blend_set, em_mot_layer_set, em_mot_layer_weight_set, em_mot_layer_blend_step, em_mat_blend_reset,
 *   em_motion_blend_step, em_mot_frame_get, em_mot_frame_end_get, em_mot_no_ck, em_mot_frame_started_ck,
 *   em_motion_attr_get, em_motion_cond_ck, em_joint_visibility_update, em_joint_visibility_update_all,
 *   em_motion_param_step, em_surface_clamp, em_ground_snap_offset_ck, em_height_limit_update, em_surface_reach_ck,
 *   em_in_water_ck, em_phase_get, em_float_depth_get, em_dive_depth_get, em_ceiling_depth_get, em_move_vec_set,
 *   em_fall_speed_set, em_height_set, em_height_add, em_fall_height_latch, em_fall_step, em_joint_drop_start,
 *   em_move_sub_mode_set, em_motion_kind_set, em_motion_kind_tick, em_status_update, em_anger_clear, em_anger_add,
 *   em_alt_mode_tick, em_stamina_reset, em_stamina_add_capped, em_stamina_add, em_stamina_regen, em_tired_tick,
 *   em_niku_search, em_bait_react, em_ally_find, em_poison_add, em_paralysis_add, em_sleep_add, em_sight_get,
 *   em_sight_update, em_sight_copy, em_sight_player_ck, em_sight_cone_ck, em_level_tick, em_sight_player_set,
 *   em_sight_player_ck2, em_player_spotted_set, em_player_watched_ck, em_any_player_seen_ck, em_sight_tick,
 *   em_hide_set, em_ceiling_latch, em_water_surface_latch, em_motion_timer_tick, em_fx_flag2_set,
 *   em_fx_flag3_set, em_fx_timer_tick, em_busy_timer_clear, em_busy_timer_tick, em_timer_0x81C_arm,
 *   em_timer_0x81C_tick, em_timer_0x81E_arm, em_timer_0x81E_tick, em_status_bit_tick, em_max_hp_set, em_max_hp_init,
 *   em_shell_rate_init, em_flag_0x116_set, em_flag_0x115_set, em_rest_flag_set, em_flag_0x8AC_clear,
 *   em_flag_0x8AC_ck, em_flag_0x8AD_set, em_flag_0x8AD_clear, em_flag_0x8AD_ck, em_guard_set,
 *   em_guard_clr, em_flag_0x8AE_ck, em_flag_0x8AF_set, em_flag_0x8AF_clear, em_flag_0x8AF_ck, em_gauge_tick,
 *   em_tint_hold_start, em_tint_hold_end, em_ailment_clear, em_ailment_set, em_ailment_reset, em_sleep_timer_tick,
 *   em_ailment_mask_clear, em_ailment_mask_set, em_ailment_mask_ck, em_ailment_add, em_ailment_decay,
 *   em_ailment_limit_raise, em_poison_init, em_poison_fx_set, em_poison_update, em_sleep_init, em_sleep_start,
 *   em_sleep_fx_update, em_sleep_end, em_paralysis_init, em_paralysis_start, em_paralysis_end,
 *   em_paralysis_fx_update, em_stun_init, em_stun_add, em_stun_start, em_stun_end, em_stun_fx_set, em_exhaust_init,
 *   em_exhaust_add, em_exhaust_end, em_ailment8_init, em_ailment8_add, em_ailment8_start, em_ailment8_end,
 *   em_ailment_fx_update, em_paralysis_fx_scale_get, em_ailment_decay_all, em_recover_timer_set,
 *   em_recover_timer_tick, em_mark_timer_tick, em_timer_0x94A_ck, em_timer_0x94A_tick, em_hit_timer_set,
 *   em_hit_fx_set, em_hit_timer_tick, em_break_fx_set, em_break_timer_tick, em_latch_0x762_roll, em_latch_0x762_set,
 *   em_finish_flag_set, em_idle_flag_get, em_hit_fx_scale_get, em_target_refresh, em_approach_3d_clear,
 *   em_hit_arm_set, em_approach_3d_get, em_turn_to_target_3d, em_angle_approach, em_turn_in_window2,
 *   em_turn_motion_set, em_turn_rate_get, em_turn_step_get, em_turn_apply, em_roll_to_get, em_fly_start,
 *   em_fly_step, em_move_offset_fall_update, em_height_move_apply, em_height_move_step, em_move_rot_ground_step,
 *   em_target_dist_ratio_get, em_hook9_call, em_fx_scale_get, em_data_kind_get, em_state_byte_set,
 *   em_ground_impact_set, em_data_word_get, em_joint_points_get, em_shake_tick, em_eft_hide_ck, em_height_follow,
 *   em_ground_snap, em_record_0x750_clear, em_route_find, em_route_point_get, em_data_point_get,
 *   em_route_entry_get, em_sight2_get, em_part_slot_get, em_data_frames_get, em_frames_get, em_kind_frames_get,
 *   em_data_frames2_get, em_frames2_get, em_kind_frames2_get, em_reaction_req, em_hook_get,
 *   em_special_part_map_get, em_map_table_get, em_area_reach_ck, em_area_enter_ck, em_area_entry_enter_ck,
 *   em_area_next_get, em_area_next, em_kind_area_next, em_data_area_allowed_ck, em_area_allowed_ck,
 *   em_kind_area_allowed_ck, em_area_dist_get, em_special_part_get, em_special_part_set_get,
 *   em_nearest_area_get, em_special_part_nearest, em_part_spot_nearest_get, em_patrol_map_get,
 *   em_patrol_get, em_patrol_point_get, em_patrol_points_get, em_patrol_nearest_get, em_patrol_next_get,
 *   em_patrol_prev_get, em_patrol_step, em_patrol_prev_closer_ck, em_waypoint_map_get, em_waypoint_get,
 *   em_part_rec_reset_all, em_area_rec_get, em_action_finish_swim, em_action_finish_stand, em_action_finish_sub2,
 *   em_action_finish_dive, em_act_queued_arm, em_delay_act_clear, em_delay_act_clear0, em_delay_act_set,
 *   em_delay_act_tick, em_delay_act_fire, em_act_change_notify, em_act_request, em_combat_reset,
 *   em_hit_window_clear_all, em_area_status_init, em_status_timer_tick, em_timers_update, em_idle_flag2_set,
 *   em_area_roam_pick, em_area_stun_move_ck, em_area_break_ck, em_area_player_follow_ck, em_stamina_gate_get,
 *   em_pack_area_pick, em_area_decide, em_area_enter_init, em_ground_probe, em_area_exit_ck, em_target_select,
 *   em_hate_add, em_hate_update, em_spawn_rec_live_ck, em_spawn_rec_live_ck2, em_act_pick, em_wave_offset,
 *   em_wave_segment, em_land_data_init, em_pl_box_init, em_target_reach_ck, em_break_part_motion_clear,
 *   em_break_part_motion_step, em_break_parts_reset, em_break_part_active_ck, em_break_part_return_step,
 *   em_entry_proc_get, em_break_part_spin
 *   GUESS, other units' rows this unit named:
 *   emc_host_ck, mhchar_motion_set, mhchar_motion_layer_set, mhchar_mat_blend_set, mhchar_motion_speed_set,
 *   pl_segment_init, pl_segment_box_init, eft_vec_ctor, pl_segment_sphere_hit, em_gauge_period_step, em_se_stop_all,
 *   eft013_em_place, eft013_spark_set, eft_stun_set, mhchar_motion_flag_ck, camera_em_shake_req,
 *   camera_enemy_focus_pos_set, em_part_limit_default, emc_delay_frames_get, em_kill_sub_kind_ck, emc_kill_report,
 *   hit_motion_set, pl_ground_probe, em_hate_bands_default, vec3_polar_offset, vec3_polar_offset2,
 *   vec3_segment_offset, vec3_segment_offset2, stg_area_ceiling_get, pl_box_point_dist_sq, pl_condition_ck_20000,
 *   em_break_part_data_get, mhchar_mat_hide_set
 * RESIDUALS. 10 rows unwritten: em_act_advance 0x8012555C (0x9F8), em_hook_get 0x801264BC (0x13C, reads the
 *   unclaimed sound-proc table), 0x8012A414 (0x210), 0x8012D498 (0x180), 0x8012D618 (0x1E4), em_act_end
 *   0x8012E728 (0x15C), 0x80135E44 (0x2B8), 0x801363F8 (0x5A8), 0x80136E38 (0x598), 0x801373D0 (0x234); the
 *   breakable-part pair 0x80135E44/0x801363F8 needs `ResMdl` at +0x138 and the matrix helpers declared.
 *  - 97 written rows short of 100 %, most of them register allocation or instruction scheduling only; the
 *    shapes known not to reach retail: `em_delay_act_set`/`em_delay_act_tick` (retail's `li r0,0` + `mulli`
 *    index), `em_motion_param_set` (an extra `frsp`), `em_height_limit_update` (the range switch lowers to
 *    compares), the `u16` fields retail reads as `s16` (+0x812/+0x814/+0x94A/+0x94C),
 *    `em_break_part_active_ck` (retail adds the 0x488 base before the loop);
 *  - `em_act_sleep_side_get`, `fn_8012E5D4`: dense-case dispatch trees; retail shares one tail per constant return and lowers
 *    the quartet 0x24..0x27 with `subi`/`cmplwi`, every shape tried emits inline returns or a two-compare range test
 *    (docs/enemy.md);
 *  - `ana_em_ck`, `shibire_em_ck`: retail re-materialises the two `(u8)` argument masks at each call in the loop,
 *    ours hoists them;
 *  - `fn_8012BDF4`: retail's frame is 0x70 against our 0x50 (an f31 `psq_st` spill and an `f64` temporary), and ours
 *    forms `ainpc_w`'s address, which retail does not reference;
 *  - `em_team_area_alert_set_near`, `fn_8012C4E8`: the distance test (`vec3_dist_sq`, `fmuls`) is laid out with the opposite branch
 *    polarity;
 *  - `em_team_area_alert_set`, `fn_8012C600`, `fn_8012C6F4`, `fn_8012C870`, `fn_8012CDF4`, `fn_8012CF04`, `fn_8012D3E0`: the
 *    `clrlwi` narrowing sits at a different point (at entry in one, at the use or store in the other);
 *  - `em_team_damage_under_ck`, `em_team_damage_over_ck`: retail keeps a dead loop counter (`li r4,0` +
 *    `addi r4,r4,1`) and loads `lbl_80796C68`/`lbl_80796C80` where ours names its own `.sdata2` magic;
 *    `em_motion_window_ck`: the same magic, and ours materialises the final test branch-free (`mfcr` + `extrwi`);
 *  - `em_sleep_ck`: retail shares one `return 0` through both `bne` arms, ours a `li r3,0` per arm;
 *    `em_tall_ck`: the constant is loaded before the field (f0/f1 against retail's f2/f0);
 *  - `em_pl_target_ck`, `em_act_kind6_ck`: ours compares unsigned where retail uses `cmpwi` (`em_pl_target_ck` also an extra
 *    `b`); `fn_8012C0EC`: the branch polarity and one reload differ; `fn_8012CF2C`: the +0x796 store is scheduled two
 *    instructions later; `fn_8012D188`: the two bytes load in the other order; `shibire_em_ck_sub`: ours moves r6
 *    earlier;
 *  - `fn_8012B86C`, `em_team_member_claim`, `em_attack_start`, `em_area_vacant_wait_ck`: register allocation only.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.rodata`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex
 *   short of the claim (the unwritten rows above).
 * SHAPES. `#pragma peephole off` from the first body to the end (retail keeps `clrlwi`/`rlwinm` + `cmpwi` and `clrlwi`
 *   + `slwi` unfused) and `#pragma fp_contract off` around `fn_8012BA00` (retail keeps `fmuls` + `fadds`); the
 *   `em_act_ck__FP11_ENEMY_WORKUcUc`-style map names are declared `extern "C"` under the map's spelling, so the
 *   parameter widths come from the codegen (retail masks them; a C++ declaration loses the `clrlwi`);
 *   `fn_80128590`/`fn_801285A0` take `u8`/`u16` so their stores stay unmasked; `fn_8012BA00`'s branch order, `u32`
 *   parameters, empty `else if (enemy->field_0x1E2 == 2)` arm and local order are load-bearing (docs/enemy.md);
 *   `em_area_vacant_wait_ck` reuses its `mode` argument as the table index; the callees the folded sources declared differently
 *   are called through cast macros (`<name>_viewN`) and the disagreeing header declarations are renamed away around
 *   their `#include`; `em_part_hit_set`'s `switch` puts `default: return;` first (a trailing default costs two
 *   branches), and its material ids compare unsigned against 0xFFFFFFFF (retail's `addis` + `cmplwi 0xFFFF`).
 */

/* Names the included headers declare with the callers' older signatures, hidden while they are read. */
#define em_ground_rec_clear em_ground_rec_clear_hidden_header
#define em_move_target_set em_move_target_set_hidden_header
#define em_get_part_limit em_get_part_limit_hidden_header
#define em_part_rec_reset em_part_rec_reset_hidden_header
#define em_area_change em_area_change_hidden_header
#define em_part_rec_alt_set em_part_rec_alt_set_hidden_header
#include "enemy/em_area_entry_tbl.h" /* em_area_entry_tbl (rule 2: the owner's header) */
#include "types.h"
#include "hud/em_net_send.h" /* the owner's leaf header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "Pl/plw.h"
#include "pl.h" /* MHchar (rule 1: one definition) */
#include "enemy/EmAreaEntry.h" /* EmAreaEntry (rule 1: one definition) */
#include "ef.h"
#include "enemy/EnemyData.h"
#define em_reaction_req em_reaction_req_hidden_owner_h
#define em_area_break_ck em_area_break_ck_hidden_owner_h
#define em_pack_area_pick em_pack_area_pick_hidden_owner_h
#define em_sight2_get em_sight2_get_hidden_owner_h
#define em_target_refresh em_target_refresh_hidden_owner_h
#include "enemy/fn_801251D0.h"
#undef em_target_refresh
#undef em_sight2_get
#undef em_pack_area_pick
#undef em_area_break_ck
#undef em_reaction_req
#define em_alt_mode_ck em_alt_mode_ck_hidden_fn_8012E968_h
#define em_eft_active_ck fn_8012EC3C_hidden_enemy_h
#define em_ground_ck em_ground_ck_hidden_enemy_h
#define em_fall_start em_fall_start_hidden_enemy_h
#define em_move_mode_set em_move_mode_set_hidden_enemy_h
#define em_status_set em_status_set_hidden_enemy_h
#define em_alt_mode_set em_alt_mode_set_hidden_enemy_h
#define em_status_ck em_status_ck_hidden_enemy_h
#define em_key_curve_eval em_key_curve_eval_hidden_enemy_h
#define em_move_offset_rot_apply em_move_offset_rot_apply_hidden_enemy_h
#define em_move_offset_step em_move_offset_step_hidden_enemy_h
#define em_move_offset_step_update em_move_offset_step_update_hidden_enemy_h
#define em_turn_seq_start em_turn_seq_start_hidden_enemy_h
#define em_turn_seq_step em_turn_seq_step_hidden_enemy_h
#define em_ally_find em_ally_find_hidden_enemy_h
#define em_mot_set_blend_ck em_mot_set_blend_ck_hidden_enemy_h
#define em_mot_layer_blend_set em_mot_layer_blend_set_hidden_enemy_h
#define em_surface_reach_ck em_surface_reach_ck_hidden_enemy_h
#define em_stamina_add em_stamina_add_hidden_enemy_h
#define em_stamina_regen em_stamina_regen_hidden_enemy_h
#define em_gauge_tick em_gauge_tick_hidden_enemy_h
#define em_fly_start em_fly_start_hidden_enemy_h
#define em_fly_step em_fly_step_hidden_enemy_h
#define em_move_offset_fall_update em_move_offset_fall_update_hidden_enemy_h
#define em_move_rot_ground_step em_move_rot_ground_step_hidden_enemy_h
#define em_mot_layer_blend_step em_mot_layer_blend_step_hidden_enemy_h
#define em_niku_search em_niku_search_hidden_enemy_h
#define em_flag_0x8AC_ck em_flag_0x8AC_ck_hidden_enemy_h
#define em_flag_0x8AD_set em_flag_0x8AD_set_hidden_enemy_h
#define em_flag_0x8AD_ck em_flag_0x8AD_ck_hidden_enemy_h
#define em_turn_to_target_3d em_turn_to_target_3d_hidden_enemy_h
#define em_break_part_active_ck em_break_part_active_ck_hidden_enemy_h
#include "unsplit/enemy.h"
#undef em_break_part_active_ck
#undef em_turn_to_target_3d
#undef em_flag_0x8AD_ck
#undef em_flag_0x8AD_set
#undef em_flag_0x8AC_ck
#undef em_niku_search
#undef em_mot_layer_blend_step
#undef em_move_rot_ground_step
#undef em_fly_step
#undef em_fly_start
#undef em_gauge_tick
#undef em_stamina_add
#undef em_mot_set_blend_ck
#undef em_ally_find
#undef em_fall_start
#undef em_move_mode_set
#undef em_status_set
#undef em_alt_mode_set
#undef em_status_ck
#undef em_key_curve_eval
#undef em_move_offset_rot_apply
#undef em_move_offset_step
#undef em_move_offset_step_update
#undef em_turn_seq_start
#undef em_turn_seq_step
#undef em_ground_ck
#undef em_eft_active_ck
#undef em_alt_mode_ck
#undef em_mot_layer_blend_set
#undef em_surface_reach_ck
#undef em_stamina_regen
#undef em_move_offset_fall_update
#include "unsplit/enemy_pool.h" /* the enemy band's .data pool labels, declared in the band header */
#include "Pl/Pl_master_ck.h"
#define em_act_ck__FP11_ENEMY_WORKUcUc em_act_ck__FP11_ENEMY_WORKUcUc_hidden_fn_8012BDF4_h
#define em_area_ck em_area_ck_hidden_fn_8012BDF4_h
#define em_pl_target_ck fn_8012D0B4_hidden_fn_8012BDF4_h
#define fn_8012D188 fn_8012D188_hidden_fn_8012BDF4_h
#define em_pl_slot_leaving_ck fn_8012D1A8_hidden_fn_8012BDF4_h
#define em_team_mate_state_ck fn_8012D23C_hidden_fn_8012BDF4_h
#define fn_8012D3E0 fn_8012D3E0_hidden_fn_8012BDF4_h
#define pl_torch_ck fn_8012D7FC_hidden_fn_8012BDF4_h
#define em_act_trap_ck fn_8012D8D0_hidden_fn_8012BDF4_h
#define em_act_paralysis_ck fn_8012DB3C_hidden_fn_8012BDF4_h
#define fn_8012E21C fn_8012E21C_hidden_fn_8012BDF4_h
#define em_act11_wait_ck fn_8012E5A8_hidden_fn_8012BDF4_h
#include "enemy/fn_8012BDF4.h"
#undef em_act11_wait_ck
#undef fn_8012E21C
#undef em_act_paralysis_ck
#undef em_act_trap_ck
#undef pl_torch_ck
#undef fn_8012D3E0
#undef em_team_mate_state_ck
#undef em_pl_slot_leaving_ck
#undef fn_8012D188
#undef em_pl_target_ck
#undef em_area_ck
#undef em_act_ck__FP11_ENEMY_WORKUcUc
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "vec3_scale.h" /* vec3_scale (the owner's leaf header, rule 2) */
#include "stage/shell.h" /* niku_entry_at / niku_find / niku_state_set / niku_enemy_attach (rule 2) */
#include "mh3_pad/system_w.h" /* system_w (rule 2) */
#define vec3_dist_sq vec3_dist_sq_hidden_vec_h
#define vec3_distance vec3_distance_hidden_vec_h
#define calcVecDistXZ calcVecDistXZ_hidden_vec_h
#include "fn_8004CAD8.h" /* calcDistanceSqXZ (rule 2) */
#undef calcVecDistXZ
#undef vec3_distance
#undef vec3_dist_sq
#include "Pl/pl_coll.h" /* findInterSection (rule 2) */
#define Pl_area_flag_get Pl_area_flag_get_hidden_land_h
#define em_land_data_init em_land_data_init_hidden_land_h
#define em_pl_box_init em_pl_box_init_hidden_land_h
#include "Pl/fn_8028F66C.h" /* LandData / PlBox (rule 1) */
#undef em_pl_box_init
#undef em_land_data_init
#undef Pl_area_flag_get
#include "Pl/pl_hit_sphere.h" /* PlSegment / PlSegmentBox / pl_segment_box_set (rule 2) */
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor (rule 2) */
#include "enemy/enemy_control.h" /* emc_work / SenkoRec (rule 2) */
#include "ef/fn_800CDB2C.h" /* ran_suu (rule 2) */
#include "MSL_C/file_io.h" /* abs (rule 2) */
#include "ef/ef_emitter.h" /* ef_truncate_float (rule 2) */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */
#include "g3d/g3d_anmchr.h" /* res_node_ref (rule 2) */
#include "g3d/g3d_draw.h" /* vec3_negate (rule 2) */
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr (rule 2) */
#include "ef/fn_80105314.h" /* eft_spawn_type11 (rule 2) */
#include "stage/get_now_mapno.h" /* get_now_mapno (rule 2) */
#include "lobby/lb_event_request.h" /* lb_event_request (rule 2) */
#include "lobby/lb_quest_screen.h" /* quest_arena_key_set / quest_enemy_kill_dispatch (rule 2) */
#include "menu/menu_item.h" /* hit_flag_set / hit_flags_clear (rule 2) */
#include "menu/hit_attack_list_push.h" /* hit_data_apply (rule 2) */
#include "lobby/lb_server_sel_trans.h" /* event_demo_ck (rule 2) */
#define get_move_work_adrs get_move_work_adrs_hidden_cockpit_h
#define get_move_work_max get_move_work_max_hidden_cockpit_h
#include "hud/cockpit_quest.h" /* quest_target_notice_spawn (rule 2) */
#include "ef/eft_slot.h" /* enemy_data_find / enemy_data_grp (rule 2) */
#include "ef/eft_def_model_block.h" /* eft_def_model_block (rule 2) */
#include "enemy/em_break_part_data_get.h" /* the owner's symbols this unit calls (rule 2) */
#include "ef/vec3_div.h" /* the owner's symbols this unit calls (rule 2) */
#include "Pl/pl_condition_ck_20000.h" /* the owner's symbols this unit calls (rule 2) */
#include "stage/stg_ground_impact_set.h" /* stg_area_ceiling_get (rule 2) */
#include "enemy/em_kill_sub_kind_ck.h" /* the owner's symbols this unit calls (rule 2) */
#include "enemy/emc_delay_frames_get.h" /* the owner's symbols this unit calls (rule 2) */
#include "camera/camera_main.h" /* camera_em_shake_req (rule 2) */
#include "camera/camera.h" /* camera_enemy_focus_pos_set (rule 2) */
#include "ef/effect.h" /* eft_vec_ctor (rule 2) */
/* the camera requests through the byte mode and the position view this unit passes. */
#define camera_em_shake_req_c1 ((void (*)(struct _ENEMY_WORK*, u8))camera_em_shake_req)
#define camera_enemy_focus_pos_set_c1 ((void (*)(struct _ENEMY_WORK*, u8, nw4r::math::VEC3*))camera_enemy_focus_pos_set)
#include "ef/eft_stun_set.h" /* the owner's symbols this unit calls (rule 2) */
#include "ef/eft013_em_place.h" /* eft013_em_place / eft013_spark_set (rule 2) */
/* the placement and spark setters through the argument order and the scale this unit passes. */
#define eft013_em_place_c1 ((void (*)(struct _ENEMY_WORK*, nw4r::math::VEC3*, u32, u32, f32))eft013_em_place)
#define eft013_spark_set_c1 ((void (*)(struct _ENEMY_WORK*, u32, f32))eft013_spark_set)
#include "sound/em_se_stop_all.h" /* the owner's symbols this unit calls (rule 2) */
#include "calcMtxAng3.h" /* the owner's symbols this unit calls (rule 2) */
#include "Pl/pl_segment_sphere_hit.h" /* the owner's symbols this unit calls (rule 2) */
#include "ef/pl_segment_box_init.h" /* the owner's symbols this unit calls (rule 2) */
#include "ef/pl_segment_init.h" /* the owner's symbols this unit calls (rule 2) */
#include "sound/mhchar_motion_set.h" /* the owner's symbols this unit calls (rule 2) */
#include "enemy/ENEMY_DATA.h" /* _ENEMY_DATA (rule 1) */
#undef get_move_work_max
#undef get_move_work_adrs
#include "sound/fn_800DD1F0.h" /* mhchar_joint_mtx_get */
#define stage_map_kind_get stage_map_kind_get_hidden_stg_w_h
#include "stage/stg_w.h" /* stage_map_area_count_get (rule 2) */
#undef stage_map_kind_get
#include "Pl/pl_act.h"
#include "Pl/pl_skill.h"
#define fn_8012BA00 fn_8012BA00_hidden_fn_8012BA00_h
#include "enemy/fn_8012BA00.h"
#undef fn_8012BA00
#include "ai/ainpc.h"   /* `_AINPC_W` (rule 1) */
#include "ai/ainpc_w.h" /* `ainpc_w`, owned by ai/ai_npc.cpp (rule 2) */
#include "ai/ai_torch_ck.h" /* ai_torch_ck (rule 2) */
#include "enemy/fn_80138074.h"
#include "Network/network_pat_control.h" /* the owner's header (rule 2) */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get (the owner's header, rule 2) */
#include "enemy/em_pop.h" /* quest_element_state_find (the owner's header, rule 2) */
#include "ef/pRoot.h"
#undef em_ground_rec_clear
#undef em_move_target_set
#undef em_get_part_limit
#undef em_part_rec_reset
#undef em_area_change
#undef em_part_rec_alt_set
/* Callees the folded sources declared with different parameters: each call keeps its own view through a cast
 * (the same direct call). */
#define stage_map_kind_get_view1 ((u32 (*)(u32))stage_map_kind_get)
#define get_move_work_adrs_view5 ((void* (*)(u8))get_move_work_adrs)
#define get_move_work_adrs_view3 ((_ENEMY_WORK* (*)(u8))get_move_work_adrs)
#define get_move_work_adrs_view1 ((_ENEMY_WORK* (*)(u8))get_move_work_adrs)
#define em_mot_finished_ck_view5 ((u32 (*)(void))em_mot_finished_ck)
#define em_mot_finished_ck_view3 ((u32 (*)(void))em_mot_finished_ck)
#define em_mot_finished_ck_view1 ((u32 (*)(void))em_mot_finished_ck)
#define fn_8012EC3C_view7 ((u32 (*)(struct _ENEMY_WORK*))em_eft_active_ck)
#define fn_8012EC3C_view4 ((u32 (*)(struct _ENEMY_WORK*))em_eft_active_ck)
#define fn_8012EC3C_view1 ((u32 (*)(struct _ENEMY_WORK*))em_eft_active_ck)
#define fn_8012E5A8_view5 ((u32 (*)(struct _ENEMY_WORK*))em_act11_wait_ck)
#define fn_8012E5A8_view3 ((u32 (*)(struct _ENEMY_WORK*))em_act11_wait_ck)
#define fn_8012E5A8_view1 ((u32 (*)(struct _ENEMY_WORK*))em_act11_wait_ck)
#define fn_8012E21C_view5 ((u32 (*)(u32, u32))fn_8012E21C)
#define fn_8012E21C_view3 ((u32 (*)(u32, u32))fn_8012E21C)
#define fn_8012E21C_view1 ((u32 (*)(u32, u32))fn_8012E21C)
#define fn_8012DB3C_view5 ((u32 (*)(struct _ENEMY_WORK*))em_act_paralysis_ck)
#define fn_8012DB3C_view3 ((u32 (*)(struct _ENEMY_WORK*))em_act_paralysis_ck)
#define fn_8012DB3C_view1 ((u32 (*)(struct _ENEMY_WORK*))em_act_paralysis_ck)
#define fn_8012D8D0_view5 ((u32 (*)(struct _ENEMY_WORK*))em_act_trap_ck)
#define fn_8012D8D0_view3 ((u32 (*)(struct _ENEMY_WORK*))em_act_trap_ck)
#define fn_8012D8D0_view1 ((u32 (*)(struct _ENEMY_WORK*))em_act_trap_ck)
#define fn_8012D7FC_view5 ((u32 (*)(struct _ENEMY_WORK*))pl_torch_ck)
#define fn_8012D7FC_view3 ((u32 (*)(struct _ENEMY_WORK*))pl_torch_ck)
#define fn_8012D7FC_view1 ((u32 (*)(struct _ENEMY_WORK*))pl_torch_ck)
#define fn_8012D3E0_view4 ((u8 (*)())fn_8012D3E0)
#define fn_8012D3E0_view2 ((u8 (*)())fn_8012D3E0)
#define fn_8012D23C_view4 ((u32 (*)())em_team_mate_state_ck)
#define fn_8012D23C_view2 ((u32 (*)())em_team_mate_state_ck)
#define fn_8012D1A8_view5 ((s32 (*)(u8))em_pl_slot_leaving_ck)
#define fn_8012D1A8_view3 ((s32 (*)(u8))em_pl_slot_leaving_ck)
#define fn_8012D1A8_view1 ((s32 (*)(u8))em_pl_slot_leaving_ck)
#define fn_8012D188_view5 ((s32 (*)(struct _ENEMY_WORK*, struct _ENEMY_WORK*))fn_8012D188)
#define fn_8012D188_view3 ((s32 (*)(struct _ENEMY_WORK*, struct _ENEMY_WORK*))fn_8012D188)
#define fn_8012D188_view1 ((s32 (*)(struct _ENEMY_WORK*, struct _ENEMY_WORK*))fn_8012D188)
#define fn_8012D0B4_view5 ((s32 (*)(struct _ENEMY_WORK*, void*))em_pl_target_ck)
#define fn_8012D0B4_view3 ((s32 (*)(struct _ENEMY_WORK*, void*))em_pl_target_ck)
#define fn_8012D0B4_view1 ((s32 (*)(struct _ENEMY_WORK*, void*))em_pl_target_ck)
#define fn_8012BA00_view1 ((s32 (*)(struct _ENEMY_WORK*, struct EnemyActionTable*, void*, s32, u16))fn_8012BA00)
#define fn_8012B9BC_view1 ((void (*)(_ENEMY_WORK*, u8, s32))em_hate_em_add)
#define fn_8012B944_view1 ((s32 (*)(_ENEMY_WORK*, u8, s32))em_hate_pl_add)
#define vec3_distance_view1 ((f32 (*)(const VEC3*, const VEC3*))vec3_distance)
#define em_status_ck_view1 ((u32 (*)(_ENEMY_WORK*, u32))em_status_ck)
#define em_area_ck_view5 ((u32 (*)(struct _ENEMY_WORK*))em_area_ck)
#define em_area_ck_view3 ((u32 (*)(struct _ENEMY_WORK*))em_area_ck)
#define em_area_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_area_ck)
#define em_alt_mode_ck_view5 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
#define em_alt_mode_ck_view3 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
#define em_alt_mode_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
u32 em_sleep_ck(_ENEMY_WORK* enemy, u8 kind);

/* Callees whose visible declaration has another signature than this unit's calls: the calls go through cast
 * views (the same direct call). */
#define emc_kill_report_c1 ((void (*)(struct _ENEMY_WORK*, u8))emc_kill_report)
#define mhchar_frame_check_c1 ((u32 (*)(struct MHchar*, u32, s32))mhchar_frame_check)
#define mhchar_mat_blend_set_c1 ((void (*)(struct MHchar*, s32, f32))mhchar_mat_blend_set)

/* Callees outside this unit. */

/* `em_seq_run` (0x574 B), which `enemy/fn_8011D448.cpp` defines and whose header does not declare it. */
extern "C" void em_seq_run(u32 a, u32 b, u8 c);

/* The unit's own next symbols, defined in this TU. */
extern "C" void em_hit_window_set(struct _ENEMY_WORK* self, u8 a, u32 b, u32 c);


/* Pool literals (declared, never defined - playbook 29). */
extern "C" u8 lbl_807919D0;

/* A 4-byte table entry `fn_80127568` indexes: a value word with the payload at +0x2. */
struct _EM_REC4 {
    /* +0x0 */ u16 field_0x0;
    /* +0x2 */ u16 field_0x2;
}; /* size: 0x4 */

/* The status/motion accessors (0x80128A14..0x8012B9FC). */
extern "C" void em_act_request(struct _ENEMY_WORK* self, u8 a, u8 b);
extern "C" void em_act_step_arm(struct _ENEMY_WORK* self, u8 a, u8 b, u32 c);

/* Types. */

/* A 3-float engine vector (`nw4r::math::VEC3`, the C spelling `VEC3`) comes from `nw4r/math.h`
 * (same layout: three `f32` at +0x00/+0x04/+0x08, size 0x0C). */

/* One motion set, the record `fn_8012BA00`'s second argument points at.  The three `.data` defaults
 * the callers fall back to fix the arrays' shapes: `lbl_805A0FF8` -> thresholds `lbl_805A0FB0`
 * (1000/2000/3000/5000), frames `lbl_805A0FC0` (5/3/2/1/0/-5/-30), extra `lbl_805A0FDC`
 * (0xC8/0/0/0/0/0/0), i.e. four band bounds and seven frame entries each.  size: 0x0C */
typedef struct MOTION_SET {
    /* +0x00 */ const f32* thresholds;   /* 4 entries: the upper bound of each distance band */
    /* +0x04 */ const s32* frames;       /* 7 entries: the frame delta of each band */
    /* +0x08 */ const s32* extra_frames; /* 7 entries: the deltas the per-type tests add */
} MOTION_SET;

extern "C" {
/* Callees under the map's spelling: a plain name as `symbols.txt` spells it, a mangled one written verbatim inside
 * `extern "C"`, so the identifier and the relocation pair with the map's symbol. */

 /* vector copy */
                    /* v = (0, 0, 0) */
extern s32 ai_npc_flag_ck(_ENEMY_WORK* other, u16 mask); /* tests a bit of other's `+0x1EC` */

/* The pool constants the source reaches (0x80796C58..0x80796C90, this unit's `.sdata2`), declared and never
 * defined (playbook 29); the int-to-f64 magic at 0x80796C68 is the compiler's own entry. */
extern f32 lbl_80796C58; /* 0.0f */
extern f32 lbl_80796C70; /* 0.5f */
extern f32 lbl_80796C8C; /* 0.7f */
}

/* The player-side work record (`get_move_work_adrs(0)`/`(2)`, 0xB20 apart). */

/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */
/* The root object `get_move_work_adrs(0)` returns - the per-player status bytes that
 * `em_pl_slot_leaving_ck` reads.  It is not the 0xB20 record array; the index is a byte offset into this one
 * object.
 * size: 0x22FD (lower bound; the per-player array bound is approximate) */
struct _PLAYER_ROOT {
    /* +0x00000 */ u8 unused_0x00000[0x22DD];
    /* +0x022DD */ u8 player_state[32];
};

/* The enemy's own action tables: `get_enemy_data(enemy)->field_0x0A0->field_0x024` points at three table pointers,
 * each replaced by a built-in default (`lbl_805A0FF8`, `lbl_805A1034`, `lbl_805A106C`) when the data has none. */

/* One action entry: two condition/value pairs, the value added to the attacker's damage when the
 * matching `Pl_*_condition_ck` passes.
 * size: 0x1C */
struct EnemyActionEntry {
    /* +0x000 */ u8 unused_0x000[0x00C];
    /* +0x00C */ s32 condition_0x00C;
    /* +0x010 */ s32 value_0x010;
    /* +0x014 */ s32 condition_0x014;
    /* +0x018 */ s32 value_0x018;
};

/* A whole action table; the entry the actor uses is at +0x08.
 * size: 0x0C */
struct EnemyActionTable {
    /* +0x000 */ u8 unused_0x000[0x008];
    /* +0x008 */ EnemyActionEntry* entry;
};

/* The three-table set the enemy carries.  It is written as an array so the built-in defaults
 * (`lbl_805A0FF8` and friends) share the type.
 * size: 0x0C */
struct EnemyActionSet {
    /* +0x000 */ EnemyActionTable* table_0x000;
    /* +0x004 */ EnemyActionTable* table_0x004;
    /* +0x008 */ EnemyActionTable* table_0x008;
};

/* The block at `get_enemy_data(enemy)->field_0x0A0`; only the action-set pointer and the blend scale are named.
 * size: 0x58 (lower bound) */
struct EnemyExtraData {
    /* +0x000 */ s32 base_hp;                      /* the base health `em_max_hp_init` scales */
    /* +0x004 */ u8 unused_0x004[0x008 - 0x004];
    /* +0x008 */ s8* part_slots;                   /* the two part indices `em_part_slot_get` matches */
    /* +0x00C */ EmPartRec* part_limits;           /* the per-part limits `em_get_part_limit` returns */
    /* +0x010 */ u8* part_tables;                  /* the default 9-byte part tables `em_part_rec_reset` uses */
    /* +0x014 */ u8* part_tables_alt;              /* the alternate part tables `em_part_rec_alt_set` uses */
    /* +0x018 */ struct EmSight* sights;           /* the sight records `em_sight_get` indexes by +0x38B */
    /* +0x01C */ struct EmSight2* sights2;         /* the second sight records `em_sight2_get` indexes by +0x38A */
    /* +0x020 */ struct EmHateTable* hate;          /* the hate distance bands and gains `em_hate_update` reads */
    /* +0x024 */ EnemyActionSet* action_set;
    /* +0x028 */ struct EmAngerTable* anger;  /* the anger gauge limit and gain rates `em_anger_add` reads */
    /* +0x02C */ f32* blend_scale;   /* the motion blend scale override `em_motion_blend_scale_get` prefers */
    /* +0x030 */ s16* act_timers;                  /* the action timer list `fn_80126494` returns */
    /* +0x034 */ struct EmAilmentTable* poison;    /* the poison buildup block */
    /* +0x038 */ struct EmAilmentTable* sleep;     /* the sleep buildup block */
    /* +0x03C */ struct EmAilmentTable* paralysis; /* the paralysis buildup block */
    /* +0x040 */ struct EmAilmentTable* stun;      /* the stun buildup block */
    /* +0x044 */ struct EmAilmentTable* exhaust;   /* the exhaust buildup block */
    /* +0x048 */ s16* ailment8_limit;              /* the status-8 buildup limit */
    /* +0x04C */ struct EmLevelBlock* levels;      /* the alert-level hold frames `em_level_raise` reads */
    /* +0x050 */ struct _HIT_SIZE_DATA** attack_sizes; /* the hit sizes `em_hit_window_set` indexes by the attack's size index */
    /* +0x054 */ struct _HIT_DATA* attacks;        /* the 0x1A-byte attack records `em_hit_window_set` arms */
};

/* One 0x1C-byte rank record `em_rank_rec_get` returns: the health scale and the two shell rates.
 * size: 0x1C */
struct EmRankRecord {
    /* +0x00 */ f32 hp_scale;
    /* +0x04 */ f32 shell_rate;
    /* +0x08 */ f32 rate_0x08;
    /* +0x0C */ u8 unused_0x0C[0x10 - 0x0C];
    /* +0x10 */ f32 exhaust_scale;   /* scales the exhaust limit */
    /* +0x14 */ f32 stun_scale;      /* scales the stun limit */
    /* +0x18 */ s16 hit_buff;        /* the strength `em_hit_buff_apply` adds */
    /* +0x1A */ u8 unused_0x1A[0x1C - 0x1A];
};


/* One sight record of the enemy data (`EnemyExtraData::sights`, indexed by +0x38B): the joint the eyes follow
 * (0xFFFFFFFF: the body, 50 units up), the near reach, the far reach, the height window above and below, and the
 * half angle of the view cone.
 * size: 0x18 */
struct EmSight {
    /* +0x00 */ u32 joint;
    /* +0x04 */ f32 near_reach;
    /* +0x08 */ f32 far_reach;
    /* +0x0C */ f32 above;
    /* +0x10 */ f32 below;
    /* +0x14 */ u16 half_angle;
    /* +0x16 */ u8 pad_0x16[2];
};




/* Callees and pooled data. */
extern "C" f32 vec3_dist_sq(void* ref, nw4r::math::VEC3* pos);
extern "C" u8 em_idle_flag_get(_ENEMY_WORK* self);
extern "C" void* quest_spawn_rec_find(u16 id);

extern "C" f32 vec3_distance(void* ref, nw4r::math::VEC3* pos);
extern "C" f32 calcVecDistXZ(void* ref, nw4r::math::VEC3* pos);
extern "C" s32 quest_time_limit_get(void);
extern "C" s32 quest_time_elapsed_get(void);

extern "C" s32 fn_8012D3E0(_ENEMY_WORK* enemy, u32 kind);
extern "C" u32 em_team_mate_state_ck(_ENEMY_WORK* enemy, u32 kind, u32 slot);
extern "C" void* em_kill_cut_target_get(u32 selector, _PLW* work, s8* out_flag);
extern "C" s32 pl_torch_ck(void* arg);
extern "C" s32 fn_8012D468(u32 selector, _PLW* work, s8* out_flag);
extern "C" s32 em_act_trap_ck(_ENEMY_WORK* enemy);
extern "C" u32 ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(_ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" void* ana_em_ck__FUcPQ34nw4r4math4VEC3fUc(u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" u32 shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(_ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" void* shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc(u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius);
extern "C" s32 em_act_paralysis_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act_sleep_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act_kind23_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act_wake_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act_stun_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act_sleep_side_get(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E0D8(u32 state, u32 action);
extern "C" s32 fn_8012E158(u32 state, u32 action);
extern "C" s32 fn_8012E21C(u32 state, u32 action);
extern "C" s32 em_act_kind6_ck(u32 state, u32 sub);
extern "C" s32 em_act_leap_ck(u32 state, u32 action);
extern "C" s32 em_act_roar_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act_ball_ck(_ENEMY_WORK* enemy, u32 flag);
extern "C" s32 em_act_kind18_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_act11_wait_ck(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E5D4(u32 actor, u32 flags);
extern "C" s32 em_record_hit_ck(_ENEMY_WORK* record);
extern "C" s32 em_mini_hit_ck(_ENEMY_WORK* record);
extern "C" void em_attack_start(_ENEMY_WORK* enemy);
extern "C" void em_attack_done_set(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012E6A0(u32 kind, u16 id);
extern "C" u8 em_captured_ck(_ENEMY_WORK* enemy);
extern "C" s32 em_mini_kill_kind_get(_ENEMY_WORK* enemy);
extern "C" void em_attack_finished_clear(_ENEMY_WORK* record);
extern "C" s32 fn_8012E8F4(f32 seconds);

extern "C" u8 stage_map_kind_get(u32 map_no);
extern "C" u32 stage_pos_z_low_ck(void* pos);
extern "C" u8 Pl_area_flag_get(u32 value);
extern "C" void em_hate_misc_add(_ENEMY_WORK* enemy, s32 value);
extern "C" void fn_8012CE8C(_ENEMY_WORK* enemy, u16 flag);
extern "C" void fn_8012CE9C(_ENEMY_WORK* enemy, u32 flag);
extern "C" void fn_8012CDF4(_ENEMY_WORK* enemy, u32 state, u32 sub);
extern "C" u32 fn_8012CF04(_ENEMY_WORK* enemy, u32 flag);

/* Map-mangled callees, kept as the map spells them (see the signature note in the file header). */
u8 get_now_areano(void);
EnemyData* get_enemy_data(_ENEMY_WORK* enemy);
_PLW* get_move_work_adrs(u8 kind);
u16 get_move_work_max(u8 kind);

extern "C" EnemyActionTable* lbl_805A0FF8[4];
extern "C" EnemyActionTable* lbl_805A1034[3];
extern "C" EnemyActionTable* lbl_805A106C[3];

extern "C" f32 lbl_80796C58;
extern "C" f32 lbl_80796C90;
extern "C" f32 lbl_80796C94;

extern "C" s32 em_pl_target_ck(_ENEMY_WORK* enemy, _PLW* work);
extern "C" s32 em_pl_slot_leaving_ck(u32 index);
extern "C" s32 fn_8012D188(_ENEMY_WORK* enemy, _PLW* work);
extern "C" u32 em_busy_ck(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012CF2C(_ENEMY_WORK* enemy);
extern "C" s32 em_ally_kind_ck(_ENEMY_WORK* enemy, u32 value);
extern "C" u32 fn_8012C6F4(_ENEMY_WORK* enemy, u32 mask, s32 arg2);
extern "C" u32 em_act_ck__FP11_ENEMY_WORKUcUc(_ENEMY_WORK* enemy, u32 state, u32 sub);
extern "C" u32 fn_8012C870(_ENEMY_WORK* enemy, u32 mask);

/* `Screen_w` (0x8065903C, `mh3_pad.cpp`'s `.bss`): only the frame scale at +0x14 that `main.cpp` writes
 * (`sw->f20 = 60.0f / vcount`); `main.cpp`'s `ScreenWork` documents the rest.
 * size: 0x54 */
typedef struct ScreenFrameScale {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ f32 frame_scale;
    /* +0x18 */ u8 pad_0x18[0x54 - 0x18];
} ScreenFrameScale; /* size: 0x54 */

extern "C" ScreenFrameScale Screen_w;

/* The `.sdata2` pool literals this range reads, in address order (values from `main.elf`). */
extern "C" f32 lbl_80796C9C;   /* 3.0f - mode 0 */
extern "C" f32 lbl_80796CA0;   /* 60.0f - the frame scale every mode is multiplied by */
extern "C" f32 lbl_80796CA4;   /* 5.0f - mode 1 */
extern "C" f32 lbl_80796CA8;   /* 10.0f - mode 2 */
extern "C" f32 lbl_80796CAC;   /* 15.0f - mode 3 */
extern "C" f32 lbl_80796CB0;   /* 25.0f - mode 4 */
extern "C" f32 lbl_80796CB4;   /* 35.0f - mode 5 */




extern "C" f32 lbl_80796C58;   /* 0.0f  - `em_damage_area_ok_ck`'s `team == 0xf` threshold */
extern "C" f32 lbl_80796C5C;   /* 100.0f - the frame-window denominator */

extern "C" f32 lbl_80796CB8;   /* 0.3f  */
extern "C" f32 lbl_80796CBC;   /* 0.2f  */
extern "C" f32 lbl_80796CC0;   /* 0.18f */
extern "C" f32 lbl_80796CC4;   /* 0.15f */
extern "C" f32 lbl_80796CC8;   /* 0.1f  */
extern "C" f32 lbl_80796CCC;   /* 0.05f */
extern "C" f32 lbl_80796CD0;   /* 0.9f  */

/* In-range helpers defined further down or unwritten, and the forward declaration of `fn_8012EC74`. */
extern "C" f32 em_mot_window_base_get(_ENEMY_WORK* self);
extern "C" u32 em_get_rank(_ENEMY_WORK* self);
extern "C" void em_status_set(_ENEMY_WORK* self, u8 kind);
extern "C" void em_alt_mode_set(_ENEMY_WORK* self, u8 value);
extern "C" u32 em_status_ck(_ENEMY_WORK* self, u8 bits);
extern "C" u32 em_in_water_ck(_ENEMY_WORK* self, u32 flag);
extern "C" void em_anger_add(_ENEMY_WORK* self, s16 amount);
extern "C" void em_finish_flag_set(_ENEMY_WORK* self);
extern "C" f32 em_height_base_get(_ENEMY_WORK* self);
extern "C" u32 fn_80132270(_ENEMY_WORK* self);

/* The other in-range mangled entry point, at C++ scope for its mangling. */
u32 em_magma_check(_ENEMY_WORK* self);

/* Other units' C-linkage callees. */
extern "C" s32 em_status_bits_ck(_ENEMY_WORK* self, u32 mask);

extern "C" u32 quest_entry_active_ck(void);

/* Pooled data owned by other units: declared, never defined (playbook 29), so the load operands pair with the
 * target's pool relocations. */
extern f32 lbl_80796C58;
extern f32 lbl_80796D34;
extern f32 lbl_80796D38;
extern f32 lbl_80796D3C;

/* The four 0x0C-byte colour records `setVec3` rebuilds; the table's bytes belong to the data pass. */
extern nw4r::math::VEC3 lbl_806A4560[];

/* `get_move_work_adrs__FUc` / `get_move_work_max__FUc` (owner `ef/system_core.cpp`). */
u16 get_move_work_max(u8 area);

extern "C" {
/* The plain `fn_` callees and this unit's own definitions are `extern "C"`, so they keep the map's names
 * (playbook 42); their owners' headers do not declare them. */

/* this unit's own entry points, used before their definitions below */
void em_act_flag1_set(_ENEMY_WORK* self);
void fn_801376BC(_ENEMY_WORK* self, u8 flag);
void fn_801376DC(_ENEMY_WORK* self, u8 flag);

s16* fn_80126494(_ENEMY_WORK* self);

/* `enemy/em_common.cpp` */
u32 em_busy_ck(_ENEMY_WORK* self);

/* `ef/system_core.cpp` */
u32 my_player_no(void);

/* the 0x803xxxxx helpers the band shares */
void lb_sub0e_send(u8 a, u16 b, u8 c, u16 d);
}

#pragma peephole off

/* The written bodies, in address order. */
extern "C" void em_se_tbl_play(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b)
{
    em_seq_run((u32)self, (u32)tbl, (u8)a);
}

extern "C" void em_se_tbl_play_alt(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b)
{
    em_seq_run((u32)self, (u32)tbl, (u8)a);
}

/* Binds the enemy to a player slot: the local player, or with bit 3 of +0x1C8 the first live player
 * record that is not server-select state 4; +0x1F5 records whether the binding is local. */
extern "C" void em_owner_slot_pick(struct _ENEMY_WORK* self)
{
    s8 me;
    _PLW* work;
    s8 i;
    u16 max;

    me = ((s8 (*)(void))my_player_no)();
    self->owner_slot_0x1F6 = -1;
    if (self->field_0x1C8 & 8) {
        work = get_move_work_adrs(2);
        max = get_move_work_max(2);
        for (i = 0; i < max; i++, work++) {
            if (work->slot_active != 0 && em_pl_slot_leaving_ck(work->chunk_ofs) == 0) {
                self->owner_slot_0x1F6 = i;
                break;
            }
        }
    } else {
        self->owner_slot_0x1F6 = me;
    }
    if (self->owner_slot_0x1F6 == (s8)me || self->owner_slot_0x1F6 == -1) {
        self->field_0x1F5 = 1;
    } else {
        self->field_0x1F5 = 0;
    }
}

extern "C" void em_owner_slot_set(struct _ENEMY_WORK* self, u8 a);

/* Keeps the network owner of the enemy current: rebinds a lost owner to the first live player, counts the
 * 900-frame sync wait, or hands the enemy to the next live player in its area. */
extern "C" void em_owner_slot_update(struct _ENEMY_WORK* self)
{
    s8 me;
    _PLW* work;
    u16 max;
    u32 found;
    u8 i;
    u8 n;
    _PLW* rec;

    me = ((s8 (*)(void))my_player_no)();
    if ((self->field_0x1C8 & 8) && isServerSelectState() == 1) {
        work = get_move_work_adrs(2);
        max = get_move_work_max(2);
        if (em_busy_ck(self) == 0) {
            if (isReadyCountOne() == 1) {
                found = 0;
                if (work[self->owner_slot_0x1F6].slot_active == 0 || em_pl_slot_leaving_ck(self->owner_slot_0x1F6) != 0) {
                    for (i = 0; i < max; i++) {
                        if (work[i].slot_active != 0 && em_pl_slot_leaving_ck(i) == 0) {
                            self->owner_slot_0x1F6 = i;
                            found = 1;
                            break;
                        }
                    }
                }
                if (found == 1) {
                    if (self->owner_slot_0x1F6 == me) {
                        self->field_0x1F5 = 1;
                        em_net_send(self, 2, 0);
                    } else {
                        em_net_send(self, 6, self->owner_slot_0x1F6);
                    }
                } else if (emc_host_ck() == 1) {
                    em_net_send(self, 6, self->owner_slot_0x1F6);
                }
            } else if ((s16)self->sync_flag_0x468 < 900) {
                self->sync_flag_0x468 = (s16)self->sync_flag_0x468 + 1;
            } else {
                em_net_send(self, 7, 0);
            }
        } else if (emc_host_ck() == 1) {
            self->field_0x1F5 = 1;
            em_net_send(self, 2, 0);
        } else if (em_die_ck(self) == 0 && self->area_no != work[self->owner_slot_0x1F6].area_0x16) {
            i = self->owner_slot_0x1F6 + 1;
            for (n = 0; n < max - 1; n++, i++) {
                if (i >= max) {
                    i = 0;
                }
                rec = &work[i];
                if (rec->slot_active != 0 && em_pl_slot_leaving_ck(i) == 0 && self->area_no == rec->area_0x16) {
                    em_owner_slot_set(self, i);
                    em_user_data_reenter(self);
                    return;
                }
            }
        }
    }
}

extern "C" void em_owner_slot_set(struct _ENEMY_WORK* self, u8 a)
{
    self->owner_slot_0x1F6 = (s8)a;
    self->field_0x1F5 = 0;
    em_net_send(self, 2, 0);
}

extern "C" void fn_8012554C(struct _ENEMY_WORK* self)
{
    self->state_0x9F9 = 0;
    self->state_0x9FA = 0;
}

extern "C" u8 fn_80125F88(u32 idx)
{
    return em_kind_index_tbl[(u8)idx];
}

extern "C" u32 enemy_kind_same_ck(u32 a, u32 b)
{
    return fn_80125F88((u8)a) == fn_80125F88((u8)b);
}

extern "C" u32 fn_80125FF0(u32 a, u32 b)
{
    return (u8)stage_map_kind_get_view1((u8)a) == (u8)b;
}

extern "C" u8* em_rank_rec_get(struct _ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    u8* result = em_stat_tbl_get(data->field_0x0E);
    if (result != NULL) {
        return result + self->field_0x7C8 * 0x1C;
    }
    return lbl_805A1078;
}

extern "C" s32 fn_80126098(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x14;
}

extern "C" s32 fn_801260BC(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x1C;
}

extern "C" s32 fn_801260E0(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x20;
}

extern "C" s32 em_hit_parts_get(struct _ENEMY_WORK* work)
{
    return get_enemy_data(work)->field_0x24;
}

EnemyData* get_enemy_data(struct _ENEMY_WORK* work)
{
    return enemy_data_by_team_get(work->team, work->field_0x00A);
}

extern "C" void* fn_80126704(struct _ENEMY_WORK* self);

/* 0x80125F54 (0x34): prepares a ground record (constructs its position). */
extern "C" EmGroundRec* em_ground_rec_clear(EmGroundRec* rec)
{
    VEC3_ctor(&rec->pos_0x08);
    return rec;
}

/* One area route of a route map: the area, its points and its entries.
 * size: 0xC */
struct EmRoute {
    /* +0x0 */ u8 area;              /* 0xFF closes the list */
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ nw4r::math::VEC3* points;
    /* +0x8 */ u8* entries;          /* 8-byte entries `em_route_entry_get` indexes */
};

/* One map's routes of the enemy data's route table (`EnemyData::routes_0x48`).
 * size: 0x8 */
struct EmRouteMap {
    /* +0x0 */ u8 map_kind;          /* 0xFF closes the table */
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ EmRoute* routes;
};

/* 0x80126128 (0xB0): the route of area `area` on the current map; NULL when there is none. */
extern "C" EmRoute* em_route_find(EnemyData* data, u8 area)
{
    EmRouteMap* map;
    EmRoute* route;

    get_now_mapno();
    map = data->routes_0x48;
    if (map != NULL) {
        for (; map->map_kind != 0xFF; map++) {
            if (fn_80125FF0(get_now_mapno(), map->map_kind) == 1) {
                route = map->routes;
                if (route == NULL) {
                    break;
                }
                for (; route->area != 0xFF; route++) {
                    if (route->area == area) {
                        return route;
                    }
                }
                break;
            }
        }
    }
    return NULL;
}

/* 0x801261D8 (0xA0): copies route point `key` (area in the high byte, point in the low byte) into `out`; the
 * origin when the area has no route. */
extern "C" void em_route_point_get(EnemyData* data, u16 key, nw4r::math::VEC3* out)
{
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    u8 point = key;
    EmRoute* route = em_route_find(data, (u8)((key & 0xFF00) >> 8));

    if (route != NULL) {
        nw4r::math::VEC3* p = &route->points[point];
        copyVec3(out, setVec3(&a, p->x, p->y, p->z));
        return;
    }
    copyVec3(out, setVec3(&b, 0.0f, 0.0f, 0.0f));
}

/* 0x80126278 (0x44): copies the enemy's route point `key` into `out`. */
extern "C" void em_data_point_get(_ENEMY_WORK* self, u16 key, nw4r::math::VEC3* out)
{
    em_route_point_get(get_enemy_data(self), key, out);
}

/* 0x801262BC (0x68): entry `index` of the route of the enemy's area; NULL when there is none. */
extern "C" u8* em_route_entry_get(_ENEMY_WORK* self, u8 index)
{
    EmRoute* route = em_route_find(get_enemy_data(self), self->area_no);

    if (route != NULL && route->entries != NULL) {
        return route->entries + index * 8;
    }
    return NULL;
}

extern "C" void em_reaction_req(_ENEMY_WORK* self, u8 kind, u8 sub, u8 index);
extern "C" void em_target_pos_set(_ENEMY_WORK* self, nw4r::math::VEC3* pos);

extern "C" void (*em_hook_get(_ENEMY_WORK* self, s32 index))(_ENEMY_WORK*);
/* 0x80126324 (0xF0): puts the enemy on route point `point` of its area (0xFF: stays), latches the run speed and
 * starts the move state towards the target. */
extern "C" void em_move_target_set(_ENEMY_WORK* self, u8 point, u8 mode, f32 speed)
{
    if (point != 0xFF) {
        em_data_point_get(self, ((self->area_no & 0xF) << 8) | point, &self->pos);
    }
    if (speed > 0.0f) {
        self->field_0x383 = 1;
        self->field_0x384 = speed;
    } else {
        self->field_0x383 = 0;
        self->field_0x384 = 0.0f;
    }
    em_reaction_req(self, 5, 8, mode);
    em_target_pos_set(self, NULL);
    if (point != 0xFF) {
        self->field_0x1BC = 0;
        self->field_0x1C0 = calcVecAng2(&self->pos, &self->vec_0x36C);
        self->field_0x1C4 = 0;
    }
}

/* 0x80126414 (0x40): the sight record for the +0x38B index. */
extern "C" EmSight* em_sight_get(_ENEMY_WORK* self)
{
    return &get_enemy_data(self)->extra->sights[self->field_0x38B];
}

/* One 0x10-byte record of the enemy data's second sight table (`EnemyExtraData::sights2`).
 * size: 0x10 */
struct EmSight2 {
    /* +0x0 */ u8 unused_0x0[0x10];
};

/* 0x80126454 (0x40): the second sight record for the +0x38A index. */
extern "C" EmSight2* em_sight2_get(_ENEMY_WORK* self)
{
    return &get_enemy_data(self)->extra->sights2[self->field_0x38A];
}

/* 0x80126494 (0x28): the enemy data's action timer list. */
extern "C" s16* fn_80126494(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->extra->act_timers;
}

/* One marker callback of an enemy kind; `enemy/enemy_control.cpp` calls it with the area entry. */
typedef void (*EmEntryProc)(EmAreaEntry* entry);

/* The enemy data's marker-callback block (`EnemyData::entry_procs_0x10`): the callbacks of markers 23..26.
 * size: 0x6C (a lower bound: only the callbacks are reached) */
struct EmEntryProcs {
    /* +0x00 */ u8 pad_0x00[0x5C];
    /* +0x5C */ EmEntryProc procs[4];
};

/* 0x801265F8 (0x98): the enemy kind's callback for marker `marker` (23..26) of the area entry; NULL for none. */
extern "C" EmEntryProc em_entry_proc_get(EmAreaEntry* entry, u8 marker)
{
    EmEntryProcs* procs = enemy_data_by_team_get(entry->group, entry->team_0x03)->entry_procs_0x10;

    if (procs != NULL) {
        switch (marker) {
        case 23:
            return procs->procs[0];
        case 24:
            return procs->procs[1];
        case 25:
            return procs->procs[2];
        case 26:
            return procs->procs[3];
        default:
            return NULL;
        }
    }
    return NULL;
}

/* 0x80126690 (0x74): which of the enemy data's two part slots holds part `part` (0/1); -1 for neither. */
extern "C" s32 em_part_slot_get(_ENEMY_WORK* self, u8 part)
{
    s8* slots = get_enemy_data(self)->extra->part_slots;

    if (slots != NULL) {
        if (part == slots[0]) {
            return 0;
        }
        if (part == slots[1]) {
            return 1;
        }
    }
    return -1;
}

/* The frame table `fn_80126704` returns (the enemy's own or the built-in 60/60/30/60 default).
 * size: 0x8 (lower bound) */
struct EmFrameTable {
    /* +0x0 */ s16 first;
    /* +0x2 */ s16 second;
    /* +0x4 */ s16 third;
    /* +0x6 */ s16 rest[1];
};

/* 0x80126718 (0x90): entry `index` (0..3, larger ones read as 2) of the data's frame table plus the data's +0x5C
 * bias and 160. */
extern "C" s16 em_data_frames_get(EnemyData* data, u8 index)
{
    EmFrameTable* frames = (EmFrameTable*)fn_80126704((_ENEMY_WORK*)data);
    s16 value;

    if (index > 3) {
        index = 2;
    }
    switch (index) {
    case 0:
        value = frames->first;
        break;
    case 1:
        value = frames->third;
        break;
    default:
        value = frames->rest[index - 2];
        break;
    }
    return value + data->frames_bias_0x5C + 160;
}

/* 0x801267A8 (0x34): the enemy's frame table entry for its +0x15 index. */
extern "C" s16 em_frames_get(_ENEMY_WORK* self)
{
    return em_data_frames_get(get_enemy_data(self), self->frames_index_0x015);
}

/* 0x801267DC (0x44): the enemy kind's frame table entry for its area slot, plus 300. */
extern "C" s16 em_kind_frames_get(_ENEMY_WORK* self)
{
    return em_data_frames_get(enemy_data_by_team_get(self->group, self->team), self->area_slot_0x013) + 300;
}

/* 0x80126820 (0x24): the data's second frame table entry. */
extern "C" s16 em_data_frames2_get(EnemyData* data)
{
    return ((EmFrameTable*)fn_80126704((_ENEMY_WORK*)data))->second;
}

/* 0x80126844 (0x24): the enemy's second frame table entry. */
extern "C" s16 em_frames2_get(_ENEMY_WORK* self)
{
    return em_data_frames2_get(get_enemy_data(self));
}

/* 0x80126868 (0x30): the enemy kind's second frame table entry. */
extern "C" s16 em_kind_frames2_get(_ENEMY_WORK* self)
{
    return em_data_frames2_get(enemy_data_by_team_get(self->group, self->team));
}

/* The part limits used when the enemy data has none. */
EmPartRec em_part_limit_default = {0, 0, 0, -1};

/* 0x801268B0 (0x50): the enemy data's limits of part `part` (the default when it has none). */
extern "C" EmPartRec* em_get_part_limit(_ENEMY_WORK* self, u8 part)
{
    EmPartRec* limits = get_enemy_data(self)->extra->part_limits;

    if (limits == NULL) {
        return &em_part_limit_default;
    }
    return &limits[part];
}

extern "C" void* fn_80126704(struct _ENEMY_WORK* self)
{
    if (self->field_0x60 != NULL) {
        return self->field_0x60;
    }
    return &lbl_807919D0;
}

extern "C" void fn_80126898(struct _ENEMY_WORK* self)
{
    if (self->field_0x018 > 0) {
        self->field_0x018--;
    }
}

extern "C" u32 fn_801269E8(u32* table, u32 a, u32 b)
{
    if ((u8)a == (u8)b) {
        return 1;
    }
    if ((u8)a == 255 || (u8)b == 255) {
        return 0;
    }
    s8* row = (s8*)table[(u8)a];
    if (row == NULL) {
        return 0;
    }
    return row[(u8)b] != -1;
}

/* One per-map entry of the enemy data's map-keyed tables: the map kind (0xFF closes the table) and that map's
 * payload.
 * size: 0x8 */
struct EmMapEntry {
    /* +0x0 */ u8 map_kind;
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ u8* payload;
};

/* The enemy data's special-part block (`EnemyData::special_parts_0x4C`): the special-part, patrol and waypoint
 * tables.
 * size: 0xC (lower bound) */
struct EmSpecialPartBlock {
    /* +0x0 */ EmMapEntry* maps;
    /* +0x4 */ EmMapEntry* patrols;     /* per map, the area patrols `em_patrol_map_get` returns */
    /* +0x8 */ EmMapEntry* waypoints;   /* per map, the area waypoint keys `em_waypoint_map_get` returns */
};

/* 0x80126900 (0x74): the special-part payload of the current map; NULL when it has none. */
extern "C" u8* em_special_part_map_get(EnemyData* data)
{
    EmMapEntry* entry = data->special_parts_0x4C->maps;
    u8 kind;

    if (entry != NULL) {
        for (; (kind = entry->map_kind) != 0xFF; entry++) {
            if (fn_80125FF0(get_now_mapno(), kind) == 1) {
                return entry->payload;
            }
        }
    }
    return NULL;
}

/* 0x80126974 (0x74): the payload of map `map` in a map-keyed table; NULL when it has none. */
extern "C" u32* em_map_table_get(EmMapEntry* entry, u8 map)
{
    if (entry != NULL) {
        for (; entry->map_kind != 0xFF; entry++) {
            if (fn_80125FF0(map, entry->map_kind) == 1) {
                if (entry->payload != NULL) {
                    return (u32*)entry->payload;
                }
                break;
            }
        }
    }
    return NULL;
}

/* 0x80126A4C (0x84): whether the enemy's area is `area` or reaches it on map `map`. */
extern "C" u32 em_area_reach_ck(_ENEMY_WORK* self, u8 map, u8 area)
{
    EnemyData* data = get_enemy_data(self);
    u32* table;

    if (self->area_no == area) {
        return 1;
    }
    table = em_map_table_get(data->area_maps_0x38, map);
    if (table != NULL) {
        return fn_801269E8(table, self->area_no, area);
    }
    return 0;
}

/* 0x80126AD0 (0x8C): whether area entry `entry` lies in area `area` or reaches it on map `map`. */
extern "C" s32 em_area_entry_near_ck(EmAreaEntry* entry, u8 map, u8 area)
{
    EnemyData* data = enemy_data_by_team_get(entry->group, entry->team_0x03);
    u32* table;

    if (entry->area_no == area) {
        return 1;
    }
    table = em_map_table_get(data->area_maps_0x38, map);
    if (table != NULL) {
        return fn_801269E8(table, entry->area_no, area);
    }
    return 0;
}

extern "C" u32 em_area_allowed_ck(_ENEMY_WORK* self, u8 map, u8 area);
extern "C" u32 em_kind_area_allowed_ck(EmAreaEntry* entry, u8 map, u8 area);

/* 0x80126B5C (0x78): whether the enemy may enter area `area` of map `map` and reaches it. */
extern "C" u32 em_area_enter_ck(_ENEMY_WORK* self, u8 map, u8 area)
{
    if (em_area_allowed_ck(self, map, area) == 0) {
        return 0;
    }
    return em_area_reach_ck(self, map, area) != 0;
}

/* 0x80126BD4 (0x78): whether area entry `entry` may enter area `area` of map `map` and reaches it. */
extern "C" u32 em_area_entry_enter_ck(EmAreaEntry* entry, u8 map, u8 area)
{
    if (em_kind_area_allowed_ck(entry, map, area) == 0) {
        return 0;
    }
    return em_area_entry_near_ck(entry, map, area) != 0;
}

/* 0x80126C4C (0x160): the next area on the way from `from` to `to`: `to` itself when adjacent, else one of the
 * neighbours nearest to it, picked by `seed`; 0xFF when `to` cannot be reached. */
extern "C" u8 em_area_next_get(u32* table, u8 from, u8 to, u16 seed)
{
    u8 count = stage_map_area_count_get(get_now_mapno());
    s8 best = 100;
    u8 found = 0;
    u8 picks[15];
    u8 i;
    s8* row;

    if (fn_801269E8(table, from, to) == 0) {
        return 0xFF;
    }
    if (((u8*)table[from])[to] <= 1) {
        return to;
    }
    for (i = 0; i < count; i++) {
        if (((s8*)table[from])[i] == 1) {
            row = (s8*)table[i];
            if (&table[i] != NULL && row[to] != -1) {
                if (best > row[to]) {
                    best = row[to];
                    picks[0] = i;
                    found = 1;
                } else if (best == row[to]) {
                    picks[found] = i;
                    found++;
                    if (found >= 15) {
                        break;
                    }
                }
            }
        }
    }
    if (found != 0) {
        return picks[(u8)(seed % found)];
    }
    return 0xFF;
}

/* 0x80126DAC (0x90): the enemy's next area towards `to` (`to` itself when there is no route). */
extern "C" u8 em_area_next(_ENEMY_WORK* self, u8 from, u8 to)
{
    EnemyData* data = get_enemy_data(self);
    u32* table = em_map_table_get(data->area_maps_0x38, get_now_mapno());
    u8 next;

    if (table != NULL) {
        next = em_area_next_get(table, from, to, self->bits_0x1EC);
        if (next != 0xFF) {
            return next;
        }
    }
    return to;
}

/* 0x80126E3C (0x98): the next area towards `to` for an enemy of `group`/`team` (`to` itself when there is no
 * route). */
extern "C" u8 em_kind_area_next(u8 group, u8 team, u8 from, u8 to, u16 seed)
{
    EnemyData* data = enemy_data_by_team_get(group, team);
    u32* table = em_map_table_get(data->area_maps_0x38, get_now_mapno());
    u8 next;

    if (table != NULL) {
        next = em_area_next_get(table, from, to, seed);
        if (next != 0xFF) {
            return next;
        }
    }
    return to;
}

/* 0x80126ED4 (0xAC): whether area `area` of map `map` is in the data's allowed-area table (1 without one). */
extern "C" u32 em_data_area_allowed_ck(EnemyData* data, u8 map, u8 area)
{
    EmMapEntry* entry = data->allowed_areas_0x3C;
    u8* areas;

    if (entry == NULL) {
        return 1;
    }
    for (; entry->map_kind != 0xFF; entry++) {
        if (fn_80125FF0(map, entry->map_kind) == 1) {
            for (areas = entry->payload; *areas != 0xFF; areas++) {
                if (*areas == area) {
                    return 1;
                }
            }
            return 0;
        }
    }
    return 1;
}

/* 0x80126F80 (0x44): whether the enemy may enter area `area` of map `map`. */
extern "C" u32 em_area_allowed_ck(_ENEMY_WORK* self, u8 map, u8 area)
{
    return em_data_area_allowed_ck(get_enemy_data(self), map, area);
}

/* 0x80126FC4 (0x50): whether area entry `entry`'s kind may enter area `area` of map `map`. */
extern "C" u32 em_kind_area_allowed_ck(EmAreaEntry* entry, u8 map, u8 area)
{
    return em_data_area_allowed_ck(enemy_data_by_team_get(entry->group, entry->team_0x03), map, area);
}

/* 0x80127014 (0x88): the distance in areas from the enemy's area to `area` (0 when there, 1 without a table). */
extern "C" u8 em_area_dist_get(_ENEMY_WORK* self, u8 unused, u8 area)
{
    EnemyData* data = get_enemy_data(self);
    u32* table;
    u8* row;

    if (self->area_no == area) {
        return 0;
    }
    table = em_map_table_get(data->area_maps_0x38, self->field_0x1E0);
    if (table != NULL) {
        row = (u8*)table[self->area_no];
        if (row != NULL) {
            return row[area];
        }
    }
    return 1;
}

/* One 0xC-byte special-part record of a part set; only its code is read.
 * size: 0xC */
struct EmSpecialPart {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 unused_0x1[0xC - 0x1];
};

/* A special-part set: the record count and the records.
 * size: 0x8 */
struct EmSpecialPartSet {
    /* +0x0 */ u8 area;
    /* +0x1 */ s8 count;
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ EmSpecialPart* parts;
};

/* 0x8012709C (0x60): record `index` of the set, or (index 0xFF) the record with code `code`; NULL when absent. */
extern "C" EmSpecialPart* em_special_part_get(EmSpecialPartSet* set, u8 code, u8 index)
{
    s8 count = set->count;
    EmSpecialPart* part = set->parts;
    s8 i;

    if (index == 0xFF) {
        for (i = count; i > 0; i--, part++) {
            if (part->code == code) {
                return part;
            }
        }
        return NULL;
    }
    if (index < count) {
        return &part[index];
    }
    return NULL;
}

/* 0x801270FC (0x5C): the special-part set of area `area` on the current map; NULL when it has none. */
extern "C" EmSpecialPartSet* em_special_part_set_get(EnemyData* data, u8 area)
{
    EmSpecialPartSet* set = (EmSpecialPartSet*)em_special_part_map_get(data);

    if (set != NULL) {
        for (; set->area != 0xFF; set++) {
            if (set->area == area) {
                return set;
            }
        }
    }
    return NULL;
}

/* 0x80127158 (0x64): latches the special-part record `index` of the enemy's area (by the +0x9F7 code). */
extern "C" void em_special_part_apply(_ENEMY_WORK* self, u8 index)
{
    EmSpecialPartSet* set = em_special_part_set_get(get_enemy_data(self), self->area_no);

    if (set != NULL) {
        self->recs_0x9A4 = (EmCmdRec*)em_special_part_get(set, self->field_0x9F7, index);
    } else {
        self->recs_0x9A4 = NULL;
    }
}

extern "C" u16 fn_80127568(void* unused, struct _EM_REC4* table, u32 idx);

/* One 0xC-byte spot of a special-part set's spot list: the route point key (0xFFFF closes the list) and the
 * spot kind at +0x9.
 * size: 0xC */
struct EmPartSpot {
    /* +0x0 */ u16 key;
    /* +0x2 */ u8 unused_0x2[0x9 - 0x2];
    /* +0x9 */ u8 kind;
    /* +0xA */ u8 sight_mode;     /* 1: never picked, 2: tested with the mode-3 segment test */
    /* +0xB */ u8 pad_0xB;
};

/* One special-part record as the spot search reads it: its code and its spot list.
 * size: 0xC */
struct EmPartSpots {
    /* +0x0 */ u8 code;
    /* +0x1 */ u8 unused_0x1[3];
    /* +0x4 */ EmPartSpot* spots;
    /* +0x8 */ u8 unused_0x8[4];
};

/* 0x801271BC (0x108): the code of the special part of area `area` whose first spot is nearest `pos` (codes 0xFE and
 * 0xFC skipped); 0xFF when there is none. */
extern "C" u8 em_nearest_area_get(EnemyData* data, nw4r::math::VEC3* pos, u8 area)
{
    f32 best = 0.0f;
    u8 code = 0xFF;
    nw4r::math::VEC3 spot;
    EmSpecialPartSet* set;
    EmPartSpots* part;
    s32 i;
    s32 count;
    f32 dist;

    VEC3_ctor(&spot);
    set = em_special_part_set_get(data, area);
    if (set != NULL) {
        part = (EmPartSpots*)set->parts;
        for (i = 0, count = set->count; i < count; i++, part++) {
            if (part->code != 0xFE && part->code != 0xFC) {
                em_route_point_get(data, ((area & 0xF) << 8) | (u8)part->spots->key, &spot);
                dist = calcDistanceSqXZ(pos, &spot);
                if (i == 0) {
                    best = dist;
                    code = part->code;
                } else if (dist < best) {
                    best = dist;
                    code = part->code;
                }
            }
        }
    }
    return code;
}

/* 0x801272C4 (0x44): the enemy's nearest special part in area `area`. */
extern "C" u8 em_special_part_nearest(_ENEMY_WORK* self, nw4r::math::VEC3* pos, u8 area)
{
    return em_nearest_area_get(get_enemy_data(self), pos, area);
}

/* 0x80127308 (0x158): copies the nearest spot of kind `kind` among the enemy area's special parts into `out`. */
extern "C" void em_part_spot_nearest_get(_ENEMY_WORK* self, nw4r::math::VEC3* pos, nw4r::math::VEC3* out, u8 kind)
{
    EnemyData* data = get_enemy_data(self);
    EmSpecialPartSet* set = em_special_part_set_get(data, self->area_no);
    nw4r::math::VEC3 spot;
    nw4r::math::VEC3 best_spot;
    f32 best;
    f32 none;
    EmPartSpots* part;
    EmPartSpot* s;
    s32 i;
    s32 count;
    f32 dist;

    VEC3_ctor(&spot);
    VEC3_ctor(&best_spot);
    best = -1.0f;
    if (set != NULL) {
        part = (EmPartSpots*)set->parts;
        none = best;
        for (i = 0, count = set->count; i < count; i++, part++) {
            for (s = part->spots; s->key != 0xFFFF; s++) {
                if (s->kind == kind) {
                    em_route_point_get(data, ((self->area_no & 0xF) << 8) | (u8)s->key, &spot);
                    dist = calcDistanceSqXZ(pos, &spot);
                    if (best <= none) {
                        best = dist;
                        vec3_copy(&best_spot, &spot);
                    } else if (dist < best) {
                        best = dist;
                        vec3_copy(&best_spot, &spot);
                    }
                }
            }
        }
    }
    vec3_copy(out, &best_spot);
}

/* 0x80127460 (0x74): the patrol table of the enemy's map; NULL when it has none. */
extern "C" u8* em_patrol_map_get(_ENEMY_WORK* self)
{
    EmMapEntry* entry = get_enemy_data(self)->special_parts_0x4C->patrols;

    if (entry != NULL) {
        for (; entry->map_kind != 0xFF; entry++) {
            if (fn_80125FF0(self->field_0x1E0, entry->map_kind) == 1) {
                return entry->payload;
            }
        }
    }
    return NULL;
}

/* One patrol of an area: the area, the point count and the points.
 * size: 0x8 */
struct EmPatrol {
    /* +0x0 */ u8 area;
    /* +0x1 */ u8 count;
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ struct _EM_REC4* points;
};

/* 0x801274D4 (0x5C): the patrol of area `area`; NULL when it has none. */
extern "C" EmPatrol* em_patrol_get(_ENEMY_WORK* self, u8 area)
{
    EmPatrol* patrol = (EmPatrol*)em_patrol_map_get(self);

    if (patrol != NULL) {
        for (; patrol->area != 0xFF; patrol++) {
            if (patrol->area == area) {
                return patrol;
            }
        }
    }
    return NULL;
}

/* 0x80127530 (0x38): copies patrol point `index` into `out` (the default points without a patrol). */
extern "C" void em_patrol_point_get(_ENEMY_WORK* self, struct _EM_REC4* points, u8 index, nw4r::math::VEC3* out)
{
    if (points == NULL) {
        copyVec3(out, &lbl_806A4560[index]);
        return;
    }
    em_data_point_get(self, points[index].field_0x0, out);
}

/* 0x8012758C (0x64): the patrol points of area `area` into `*out`; returns the point count (4 without a patrol). */
extern "C" u8 em_patrol_points_get(_ENEMY_WORK* self, u8 area, struct _EM_REC4** out)
{
    EmPatrol* patrol = em_patrol_get(self, self->area_no);

    if (patrol == NULL) {
        *out = NULL;
        return 4;
    }
    *out = patrol->points;
    if (out == NULL) {
        return 4;
    }
    return patrol->count;
}

/* 0x801275F0 (0xF4): the patrol point with a flag in `mask` nearest to the enemy. */
extern "C" u8 em_patrol_nearest_get(_ENEMY_WORK* self, u16 mask)
{
    nw4r::math::VEC3 point;
    f32 best;
    u8 found;
    u8 i;
    struct _EM_REC4* points;
    u8 count;
    f32 dist;

    VEC3_ctor(&point);
    best = 0.0f;
    found = 0;
    count = em_patrol_points_get(self, self->area_no, &points);
    for (i = 0; i < count; i++) {
        if (mask & fn_80127568(self, points, i)) {
            em_patrol_point_get(self, points, i, &point);
            dist = calcDistanceSqXZ(&self->pos, &point);
            if (i == 0) {
                best = dist;
                found = 0;
            } else if (dist < best) {
                best = dist;
                found = i;
            }
        }
    }
    return found;
}

/* 0x801276E4 (0x84): the next patrol point after `index` with flag 1. */
extern "C" u8 em_patrol_next_get(_ENEMY_WORK* self, struct _EM_REC4* points, u8 count, u8 index)
{
    do {
        if (++index >= count) {
            index = 0;
        }
    } while ((fn_80127568(self, points, index) & 1) == 0);
    return index;
}

/* 0x80127768 (0x8C): the previous patrol point before `index` with flag 1. */
extern "C" u8 em_patrol_prev_get(_ENEMY_WORK* self, struct _EM_REC4* points, u8 count, u8 index)
{
    do {
        if (index-- == 0) {
            index = count - 1;
        }
    } while ((fn_80127568(self, points, index) & 1) == 0);
    return index;
}

/* 0x801277F4 (0x288): picks the next patrol point into +0x382: once the +0x1E7 countdown runs out (without flag
 * 1), the flag-2 point most in front; otherwise the neighbour (previous, next, or the one more in front). Returns
 * 1 for the countdown pick. */
extern "C" u32 em_patrol_step(_ENEMY_WORK* self, u8 flags)
{
    u32 next_only = 0;
    u32 prev_only = 0;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    struct _EM_REC4* points;
    u8 count;
    u8 pick;
    u8 seen;
    u8 i;
    s16 best;
    s16 diff;
    u8 prev;
    u8 next;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    count = em_patrol_points_get(self, self->area_no, &points);
    if ((flags & 1) == 0 && --self->field_0x1E7 == 0) {
        self->field_0x1E7 = 0;
        pick = self->field_0x382;
        for (seen = 0, i = 0; i < count; i++) {
            if (fn_80127568(self, points, i) & 2) {
                if (seen == 0) {
                    em_patrol_point_get(self, points, i, &a);
                    pick = i;
                    best = calcVecAng2(&self->pos, &a) - self->field_0x1C0;
                } else {
                    em_patrol_point_get(self, points, i, &b);
                    diff = calcVecAng2(&self->pos, &b) - self->field_0x1C0;
                    if (abs(best) >= abs(diff)) {
                        copyVec3(&a, &b);
                        pick = i;
                        best = diff;
                    }
                }
                seen++;
            }
        }
        self->field_0x382 = pick;
        return 1;
    }
    if (self->field_0x380 == 10) {
        if (self->state_0x381 == 1) {
            next_only = 1;
        } else if (self->state_0x381 == 2) {
            prev_only = 1;
        }
    }
    if (next_only == 0) {
        prev = em_patrol_prev_get(self, points, count, self->field_0x382);
        if (prev_only == 1) {
            self->field_0x382 = prev;
            return 0;
        }
        em_patrol_point_get(self, points, prev, &a);
    }
    if (prev_only == 0) {
        next = em_patrol_next_get(self, points, count, self->field_0x382);
        if (next_only == 1) {
            self->field_0x382 = next;
            return 0;
        }
        em_patrol_point_get(self, points, next, &b);
    }
    best = calcVecAng2(&self->pos, &a) - self->field_0x1C0;
    if (abs(best) < abs((s16)(calcVecAng2(&self->pos, &b) - self->field_0x1C0))) {
        self->field_0x382 = prev;
    } else {
        self->field_0x382 = next;
    }
    return 0;
}

/* 0x80127A7C (0x12C): whether, seen from patrol point `index`, the previous point turns less than the next one. */
extern "C" u32 em_patrol_prev_closer_ck(_ENEMY_WORK* self, u8 index)
{
    nw4r::math::VEC3 at;
    nw4r::math::VEC3 prev;
    nw4r::math::VEC3 next;
    struct _EM_REC4* points;
    u8 count;
    s32 dir;
    s16 to_prev;

    VEC3_ctor(&at);
    VEC3_ctor(&prev);
    VEC3_ctor(&next);
    count = em_patrol_points_get(self, self->area_no, &points);
    em_patrol_point_get(self, points, index, &at);
    em_patrol_point_get(self, points, em_patrol_prev_get(self, points, count, index), &prev);
    em_patrol_point_get(self, points, em_patrol_next_get(self, points, count, index), &next);
    dir = calcVecAng2(&self->pos, &at);
    to_prev = calcVecAng2(&at, &prev) - dir;
    return abs(to_prev) < abs((s16)(calcVecAng2(&at, &next) - dir));
}

/* 0x80127BA8 (0x74): the waypoint table of the enemy's map; NULL when it has none. */
extern "C" u16* em_waypoint_map_get(_ENEMY_WORK* self)
{
    EmMapEntry* entry = get_enemy_data(self)->special_parts_0x4C->waypoints;

    if (entry != NULL) {
        for (; entry->map_kind != 0xFF; entry++) {
            if (fn_80125FF0(self->field_0x1E0, entry->map_kind) == 1) {
                return (u16*)entry->payload;
            }
        }
    }
    return NULL;
}

/* 0x80127C1C (0x94): copies the waypoint of area `area` into `out`; whether there is one. */
extern "C" u32 em_waypoint_get(_ENEMY_WORK* self, u8 area, nw4r::math::VEC3* out)
{
    u16* key = em_waypoint_map_get(self);

    if (key != NULL) {
        for (; (u8)((*key & 0xFF00) >> 8) != 0xFF; key++) {
            if ((u8)((*key & 0xFF00) >> 8) == area) {
                em_data_point_get(self, *key, out);
                return 1;
            }
        }
    }
    return 0;
}

extern "C" u16 fn_80127568(void* unused, struct _EM_REC4* table, u32 idx)
{
    if (table == NULL) {
        return 3;
    }
    return table[(u8)idx].field_0x2;
}

extern "C" u32 em_attacker_rec_get(struct _ENEMY_WORK* self, u32 idx)
{
    return self->values_0x868[(u8)idx];
}

extern "C" void em_move_mode_set(_ENEMY_WORK* self, u8 mode);
extern "C" void em_move_sub_mode_set(_ENEMY_WORK* self, u8 sub);
extern "C" void em_fall_start(_ENEMY_WORK* self, f32 lift);
extern "C" f32 em_fall_height_get(_ENEMY_WORK* self);
extern "C" s32 em_eft_active_ck(_ENEMY_WORK* self);

/* 0x80127CC4 (0x5C): points part `part`'s table at the enemy data's default 9-byte entry for it. */
extern "C" void em_part_rec_reset(_ENEMY_WORK* self, u8 part)
{
    self->part_tbls_0x868[part] = get_enemy_data(self)->extra->part_tables + part * 9;
}

/* 0x80127D20 (0x74): points part `part`'s table at entry `alt` of the enemy data's alternate tables. */
extern "C" void em_part_rec_alt_set(_ENEMY_WORK* self, u8 part, u8 alt)
{
    u8* tables = get_enemy_data(self)->extra->part_tables_alt;

    if (tables != NULL) {
        self->part_tbls_0x868[part] = tables + alt * 9;
    }
}

/* 0x80127D94 (0x50): resets all eight part tables. */
extern "C" void em_part_rec_reset_all(_ENEMY_WORK* self)
{
    u8 i;

    get_enemy_data(self);
    for (i = 0; i < 8; i++) {
        em_part_rec_reset(self, i);
    }
}

/* 0x80127DE4 (0x94): the enemy data's 8-byte record for the enemy's area on its map (the closing record when the
 * area has none); NULL without a table for the map. */
extern "C" u8* em_area_rec_get(_ENEMY_WORK* self)
{
    EmMapEntry* entry = get_enemy_data(self)->area_recs_0x44;
    u8* rec;

    if (entry != NULL) {
        for (; entry->map_kind != 0xFF; entry++) {
            if (fn_80125FF0(self->field_0x1E0, entry->map_kind) == 1) {
                for (rec = entry->payload; *rec != 0xFF; rec += 8) {
                    if (*rec == self->area_no) {
                        return rec;
                    }
                }
                return rec;
            }
        }
    }
    return NULL;
}

/* 0x80127E78 (0xD0): the collision mask of the enemy's current move mode and flags. */
extern "C" u16 em_hit_mask_get(_ENEMY_WORK* self)
{
    u32 flags = self->field_0x1C8;
    u32 big = flags & 1;
    u16 mask = 8;

    if (big != 0) {
        mask = 4;
    }
    if (flags & 0x10) {
        switch (self->field_0x1E2) {
        case 1:
            mask |= 0xC0;
            break;
        default:
            mask |= 0x40;
            break;
        case 2:
            mask |= 0x80;
            break;
        }
    } else if (flags & 0x1000) {
        mask |= 0x40;
    } else if (flags & 0x2000) {
        mask |= 0x80;
    }
    if (self->field_0x1E2 != 1 || self->field_0x1E3 != 1) {
        mask |= 0x200;
    }
    if ((flags & 0x100) == 0 || self->field_0x1E2 != 4) {
        mask |= 0x800;
    }
    if (big != 0 && self->action != 0xB) {
        mask |= 0x1000;
    }
    return mask;
}

extern "C" void em_act_arm_unless_act11(struct _ENEMY_WORK* self, u32 a, u32 b);

/* 0x80127F48 (0x9C): ends an action on the ground: move mode 0 and the matching idle step. */
extern "C" void em_action_finish(_ENEMY_WORK* self)
{
    em_move_mode_set(self, 0);
    if ((u32)em_eft_active_ck(self) == 1 && (self->field_0x1C8 & 0x40)) {
        em_act_arm_unless_act11(self, 0, 2);
    } else if (self->field_0x43D == 1) {
        em_act_arm_unless_act11(self, 0, 1);
    } else {
        em_act_arm_unless_act11(self, 0, 0);
    }
    em_finish_flag_set(self);
}

/* 0x80127FE4 (0x4C): ends an action in the air: starts the fall and idle step 3. */
extern "C" void em_action_finish_fall(_ENEMY_WORK* self)
{
    em_fall_start(self, em_fall_height_get(self));
    em_act_arm_unless_act11(self, 0, 3);
    em_finish_flag_set(self);
}

/* 0x80128030 (0x7C): ends an action in water: move mode 2 and idle step 4 (5 when angry). */
extern "C" void em_action_finish_swim(_ENEMY_WORK* self)
{
    em_move_mode_set(self, 2);
    if ((u32)em_eft_active_ck(self) == 1 && (self->field_0x1C8 & 0x40)) {
        em_act_arm_unless_act11(self, 0, 5);
    } else {
        em_act_arm_unless_act11(self, 0, 4);
    }
    em_finish_flag_set(self);
}

/* 0x801280AC (0x48): ends an action walking: move mode 3 and idle step 6. */
extern "C" void em_action_finish_walk(_ENEMY_WORK* self)
{
    em_move_mode_set(self, 3);
    em_act_arm_unless_act11(self, 0, 6);
    em_finish_flag_set(self);
}

/* 0x801280F4 (0x48): ends an action burrowed: move mode 4 and idle step 7. */
extern "C" void em_action_finish_stand(_ENEMY_WORK* self)
{
    em_move_mode_set(self, 4);
    em_act_arm_unless_act11(self, 0, 7);
    em_finish_flag_set(self);
}

/* 0x8012813C (0x58): ends an action diving: starts the fall in sub mode 2 and idle step 8. */
extern "C" void em_action_finish_sub2(_ENEMY_WORK* self)
{
    em_fall_start(self, em_fall_height_get(self));
    em_move_sub_mode_set(self, 2);
    em_act_arm_unless_act11(self, 0, 8);
    em_finish_flag_set(self);
}

/* 0x80128194 (0x58): ends an action gliding: starts the fall in sub mode 1 and idle step 9. */
extern "C" void em_action_finish_dive(_ENEMY_WORK* self)
{
    em_fall_start(self, em_fall_height_get(self));
    em_move_sub_mode_set(self, 1);
    em_act_arm_unless_act11(self, 0, 9);
    em_finish_flag_set(self);
}

/* 0x8012820C (0x48): re-arms the queued action (+0x1EA/+0x1EB) once the enemy is idle. */
extern "C" void em_act_queued_arm(_ENEMY_WORK* self)
{
    if (em_busy_ck(self) == 1) {
        em_act_step_arm(self, self->queued_act_0x1EA, self->queued_sub_0x1EB, 0);
    }
}

/* 0x80128254 (0x18): clears delayed action `index`. */
extern "C" void em_delay_act_clear(_ENEMY_WORK* self, u8 index)
{
    self->delays_0xAF0[index].timer = -1;
}

/* 0x8012826C (0x8): clears the delayed action. */
extern "C" void em_delay_act_clear0(_ENEMY_WORK* self)
{
    em_delay_act_clear(self, 0);
}

/* The delayed-action record `index`. */
inline EmDelayRec* em_delay_rec(_ENEMY_WORK* self, s32 index)
{
    return &self->delays_0xAF0[index];
}

/* 0x80128274 (0x94): queues action `act`/`sub` with the current target after the control block's delay, unless one
 * is queued. */
extern "C" void em_delay_act_set(_ENEMY_WORK* self, s8 act, s8 sub)
{
    EmDelayRec* rec = em_delay_rec(self, 0);

    if (rec->timer < 0) {
        rec->timer = emc_delay_frames_get();
        rec->act = act;
        rec->sub = sub;
        rec->target_kind = self->field_0x380;
        rec->target_sub = self->state_0x381;
        rec->target_index = self->field_0x382;
        copyVec3(&rec->target, &self->vec_0x36C);
    }
}

extern "C" void em_delay_act_fire(_ENEMY_WORK* self, EmDelayRec* rec);

/* 0x80128308 (0x38): runs the delayed action's timer down and fires it when it runs out. */
extern "C" void em_delay_act_tick(_ENEMY_WORK* self)
{
    EmDelayRec* rec = em_delay_rec(self, 0);

    if (rec->timer > 0) {
        if (--rec->timer <= 0) {
            em_delay_act_fire(self, rec);
        }
    }
}

/* 0x80128B0C (0x74): fires a delayed action: restores its target and arms the action. */
extern "C" void em_delay_act_fire(_ENEMY_WORK* self, EmDelayRec* rec)
{
    rec->timer = -1;
    self->field_0x380 = rec->target_kind;
    self->state_0x381 = rec->target_sub;
    self->field_0x382 = rec->target_index;
    copyVec3(&self->vec_0x36C, &rec->target);
    em_act_step_arm(self, rec->act, rec->sub, 3);
}

extern "C" void em_paralysis_start(_ENEMY_WORK* self);
extern "C" void em_paralysis_end(_ENEMY_WORK* self);
extern "C" void em_sleep_start(_ENEMY_WORK* self);
extern "C" void em_sleep_end(_ENEMY_WORK* self);
extern "C" void em_stun_start(_ENEMY_WORK* self);
extern "C" void em_stun_end(_ENEMY_WORK* self);
extern "C" void em_exhaust_end(_ENEMY_WORK* self);
extern "C" void em_ailment8_start(_ENEMY_WORK* self);
extern "C" void em_ailment8_end(_ENEMY_WORK* self);
extern "C" void em_ailment_reset(_ENEMY_WORK* self, s32 bits);
extern "C" void fn_80128590(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c);
extern "C" void fn_801285A0(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c);
extern "C" void em_recover_timer_set(_ENEMY_WORK* self, u8 kind);
extern "C" void em_anger_clear(_ENEMY_WORK* self);
extern "C" void em_stamina_reset(_ENEMY_WORK* self);
extern "C" void em_latch_0x762_set(_ENEMY_WORK* self, s16 frames);
extern "C" void em_state_refresh(_ENEMY_WORK* self);
extern "C" void em_tint_hold_end(_ENEMY_WORK* self);
extern "C" void em_flag_0x8AC_clear(_ENEMY_WORK* self);
extern "C" void em_flag_0x8AD_clear(_ENEMY_WORK* self);
extern "C" void em_guard_clr(_ENEMY_WORK* self);
extern "C" void em_flag_0x8AF_clear(_ENEMY_WORK* self);
extern "C" void em_motion_mode_set(_ENEMY_WORK* self, u8 mode);
extern "C" s32 em_alt_mode_ck(_ENEMY_WORK* self);

/* 0x80128340 (0x250): reacts to an action change: the enemy's hook 5, the ailment starts and arena keys of the
 * action-10 sub steps, and for action 11 (death) the full status reset and the kill report. */
extern "C" void em_act_change_notify(_ENEMY_WORK* self, u8 act, u8 sub)
{
    EnemyData* data = get_enemy_data(self);
    void (*hook)(_ENEMY_WORK*, u8, u8) = (void (*)(_ENEMY_WORK*, u8, u8))em_hook_get(self, 5);
    u32 (*death_hook)(_ENEMY_WORK*);
    u32 keep;

    if (hook != NULL) {
        hook(self, act, sub);
    }
    switch (act) {
    case 10:
        switch (sub) {
        case 0x7C:
        case 0x82:
        case 0x8B:
        case 0x95:
        case 0x9C:
        case 0xBB:
        case 0xE8:
        case 0xEE:
            em_paralysis_start(self);
            break;
        case 0x7A:
        case 0x80:
        case 0x89:
        case 0x93:
        case 0x9A:
        case 0xB9:
        case 0xE6:
        case 0xEC:
        case 0xF8:
            em_sleep_start(self);
            break;
        case 0x7D:
        case 0x7E:
        case 0x7F:
        case 0x83:
        case 0x8C:
        case 0x96:
        case 0x9D:
        case 0xBC:
        case 0xE9:
        case 0xEF:
            quest_arena_key_set(self->team, 3);
            em_stun_start(self);
            break;
        case 0xB6:
            quest_arena_key_set(self->team, 1);
            em_recover_timer_set(self, 0);
            break;
        case 0xBF:
        case 0xC0:
            em_recover_timer_set(self, 1);
            break;
        case 0x78:
        case 0x79:
            em_exhaust_end(self);
            break;
        case 0xAF:
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
            quest_arena_key_set(self->team, 5);
            break;
        }
        break;
    case 11:
        if ((u32)(sub - 0x1A) <= 2) {
            em_ailment8_start(self);
        }
        death_hook = (u32 (*)(_ENEMY_WORK*))em_hook_get(self, 10);
        if (death_hook != NULL) {
            keep = death_hook(self);
        } else {
            keep = 0;
        }
        lb_event_request(19);
        em_status_set(self, 0);
        em_anger_clear(self);
        em_stamina_reset(self);
        em_ailment_reset(self, 0xF7);
        self->field_0x916 = 0;
        self->field_0x38E = 0;
        self->field_0x38F = 0;
        if ((keep & 2) == 0) {
            em_latch_0x762_set(self, -1);
        }
        self->field_0x94C = 0;
        self->field_0x812 = 0;
        self->field_0x814 = 0;
        self->field_0x94A = 0;
        if ((keep & 1) == 0) {
            self->field_0x1E4 = 0;
        }
        self->flags_0xA04 = 0;
        self->field_0xA05 = 0xFF;
        self->field_0xA06 = 0;
        em_state_refresh(self);
        if (em_kill_sub_kind_ck(sub, data->kind_0x0F) == 1) {
            emc_kill_report_c1(self, 5);
        } else {
            emc_kill_report_c1(self, 1);
        }
        quest_enemy_kill_dispatch(self, 0);
        break;
    }
}

extern "C" void em_hit_window_clear_all(_ENEMY_WORK* self);

/* 0x801285C0 (0x3D8): arms action `act`/`sub` in `mode`: settles the motion mode, lets hook 4 remap the action,
 * retargets, syncs it in a network game (queuing it behind the control delay), clears the per-action state and
 * ends the ailments the new action does not keep. */
extern "C" void em_act_step_arm(_ENEMY_WORK* self, u8 act, u8 sub, u32 mode)
{
    u8 a = act;
    u8 b = sub;
    void (*hook)(_ENEMY_WORK*, u8*, u8*);
    u16 seed;
    u8 old_a;
    u8 old_b;

    if ((u8)mode != 1 && self->action == 0xB) {
        if (act != 0xC || sub != 0xFF) {
            return;
        }
    }
    self->field_0x1E9 = 0;
    if ((u8)mode == 0 || (u8)mode == 5) {
        if (em_busy_ck(self) == 0) {
            return;
        }
        if (self->field_0x89F == 1) {
            if (em_alt_mode_ck(self) == 0 && fn_8012CF04(self, 4) == 0) {
                em_motion_mode_set(self, 2);
            }
        } else if (self->field_0x89F == 3 && self->field_0x8A2 != 0) {
            em_motion_mode_set(self, 0);
        }
    }
    if ((em_busy_ck(self) == 1 && (u8)mode != 3) || (u8)mode == 1) {
        hook = (void (*)(_ENEMY_WORK*, u8*, u8*))em_hook_get(self, 4);
        if (hook != NULL) {
            do {
                old_a = a;
                old_b = b;
                hook(self, &a, &b);
            } while (old_a != a || old_b != b);
        }
    }
    if ((u8)(mode + 254) > 2) {
        em_target_pos_set(self, NULL);
    }
    if ((u8)(mode + 253) <= 1) {
        seed = self->bits_0x1EC;
    } else {
        seed = ran_suu(1);
    }
    if (((u8)mode == 0 || (u8)mode == 5) && (self->field_0x1C8 & 8)) {
        fn_80128590(self, a, b, seed);
        em_net_send(self, 1, 0);
        if ((u8)mode != 5 && emc_delay_frames_get() > 0) {
            em_delay_act_set(self, a, b);
            return;
        }
    }
    em_hit_window_clear_all(self);
    em_tint_hold_end(self);
    em_flag_0x8AC_clear(self);
    em_flag_0x8AD_clear(self);
    em_guard_clr(self);
    em_flag_0x8AF_clear(self);
    self->field_0x1FB = 0;
    if ((u8)mode != 2) {
        fn_801376DC(self, 2);
        if (fn_8012E0D8(a, b) == 0 && em_status_ck(self, 4) != 0) {
            em_paralysis_end(self);
        }
        if (fn_8012E158(a, b) == 0 && em_status_ck(self, 1) != 0) {
            em_sleep_end(self);
        }
        if (em_act_kind6_ck(a, b) == 0 && em_status_ck(self, 8) != 0) {
            em_ailment8_end(self);
        }
        if (em_act_leap_ck(a, b) == 0 && em_status_ck(self, 0x10) != 0) {
            em_stun_end(self);
        }
        if ((u8)mode == 4) {
            if (fn_8012CF04(self, 2) == 1 && self->field_0x796 <= 0) {
                fn_8012CE9C(self, 2);
            }
            if (fn_8012CF04(self, 8) == 1) {
                fn_8012CE9C(self, 8);
            }
        }
        self->field_0x38C = 3;
        self->flag_0x116 = 0;
        self->field_0x1E8 = 0;
    } else {
        self->field_0x1E8 = 1;
    }
    fn_801285A0(self, a, b, seed);
    em_act_change_notify(self, a, b);
}

/* 0x80128998 (0x7C): arms action `act`/`sub` at once when the enemy is idle, else queues it. */
extern "C" void em_act_request(_ENEMY_WORK* self, u8 act, u8 sub)
{
    if (em_busy_ck(self) == 1) {
        em_act_step_arm(self, act, sub, 5);
        fn_801281EC(self);
        return;
    }
    self->field_0x1E9 = 1;
    self->queued_act_0x1EA = act;
    self->queued_sub_0x1EB = sub;
}

extern "C" void fn_80128590(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c)
{
    self->field_0x1EE = a;
    self->field_0x1EF = b;
    self->field_0x1F0 = c;
}

extern "C" void fn_801285A0(struct _ENEMY_WORK* self, u8 a, u8 b, u16 c)
{
    self->state = 0;
    self->state_0x006 = 0;
    self->state_0x007 = 0;
    self->action = a;
    self->state_sub = b;
    self->bits_0x1EC = c;
}

extern "C" void fn_801281EC(struct _ENEMY_WORK* self)
{
    self->field_0x1F4 = 1;
}

extern "C" void fn_801281F8(struct _ENEMY_WORK* self)
{
    self->field_0x1F4 = 0;
}

extern "C" u32 fn_80128204(struct _ENEMY_WORK* self)
{
    return self->field_0x1F4;
}

extern "C" void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    em_hit_window_set(self, (u8)a, b, 0);
}

/* The 0x20-byte model block `eft_def_model_block` returns: whether it is live, the segment the target weaves along
 * and the weave reach.
 * size: 0x20 */
struct EmModelBlockView {
    /* +0x00 */ u8 live;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ f32 a[3];
    /* +0x10 */ f32 b[3];
    /* +0x1C */ f32 reach;
};

extern "C" u8* em_move_work_pick(u8 mode, u8 mask);
extern "C" void em_wave_offset(nw4r::math::VEC3* out, nw4r::math::VEC3* in, u8 kind, u32 value, f32 reach);
extern "C" void em_wave_segment(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b, u8 kind, u32 value,
                                f32 base);
extern "C" void em_wave_amp(nw4r::math::VEC3* out, nw4r::math::VEC3* in, u8 kind, u32 value, f32 base);

/* 0x80128BF8 (0x744): sets the target position (`pos`, or from the target kind triple: a player, the AI partner,
 * another enemy, a direction, the aim, an enemy-data entry, a route point, a waypoint, a meat, a special part, an
 * area exit, a patrol point, straight ahead or the first player), then weaves it by the +0x383 weave state. */
extern "C" void em_target_pos_set(_ENEMY_WORK* self, nw4r::math::VEC3* pos)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 seg_a;
    nw4r::math::VEC3 seg_b;
    nw4r::math::VEC3 here;
    nw4r::math::VEC3 there;
    s32 use_segment = 0;
    u32 wave_offset = 0;
    u8* rec;
    _ENEMY_WORK* work;
    u32 angle;
    struct _ENEMY_DATA* data;
    EmModelBlockView* blk;
    struct _EM_REC4* points;
    ShellNikuEntry* niku;
    u8 area;
    nw4r::math::VEC3 t0;
    nw4r::math::VEC3 t1;
    nw4r::math::VEC3 t2;
    nw4r::math::VEC3 t3;
    nw4r::math::VEC3 t4;
    nw4r::math::VEC3 t5;
    nw4r::math::VEC3 t6;
    nw4r::math::VEC3 t7;
    nw4r::math::VEC3 t8;
    u8 kind;

    VEC3_ctor(&dir);
    VEC3_ctor(&seg_a);
    VEC3_ctor(&seg_b);
    VEC3_ctor(&here);
    VEC3_ctor(&there);
    if (pos != NULL) {
        copyVec3(&self->vec_0x36C, pos);
    } else {
        switch (self->field_0x380) {
        case 1:
            if (self->field_0x382 == 0xFF) {
                return;
            }
            rec = em_move_work_pick(self->state_0x381, self->field_0x382);
            if (rec[0] == 0 || (u32)em_pl_target_ck(self, (_PLW*)rec) != 1) {
                return;
            }
            copyVec3(&self->vec_0x36C, &((_PLW*)rec)->vec_0x03C);
            break;
        case 2:
            if (self->field_0x382 == 0xFF || ainpc_w.active == 0 || self->area_no != ainpc_w.field_0x1A4) {
                return;
            }
            copyVec3(&self->vec_0x36C, &ainpc_w.vec_0x178);
            break;
        case 3:
            if (self->field_0x382 == 0xFF) {
                return;
            }
            work = &get_move_work_adrs_view3(3)[self->field_0x382];
            if (work->active == 0 || self->area_no != work->area_no) {
                return;
            }
            copyVec3(&self->vec_0x36C, &work->pos);
            break;
        case 5:
            switch (self->state_0x381) {
            case 0:
            case 1:
            case 2:
            case 3:
                dir.x = 0.0f;
                dir.y = 0.0f;
                dir.z = 100.0f * self->field_0x382;
                angle = self->field_0x1C0;
                switch (self->state_0x381) {
                case 0:
                    angle += 0x4000;
                    break;
                case 1:
                    angle -= 0x4000;
                    break;
                case 3:
                    angle += 0x8000;
                    break;
                }
                rotVecY(&dir, angle);
                addVec3(&t0, &self->pos, &dir);
                copyVec3(&self->vec_0x36C, &t0);
                break;
            case 6:
                copyVec3(&self->vec_0x36C, &self->aim);
                break;
            case 7:
                data = (struct _ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A),
                                                            self->field_0x46C);
                if (data == NULL || self->area_no != data->field_0x14) {
                    return;
                }
                if (data->field_0x36 != 0xFF) {
                    blk = (EmModelBlockView*)eft_def_model_block((struct EftSlot*)data, data->field_0x36);
                    if (blk != NULL && blk->live == 1) {
                        use_segment = 1;
                        setVector3(&seg_a, blk->a[0], blk->a[1], blk->a[2]);
                        setVector3(&seg_b, blk->b[0], blk->b[1], blk->b[2]);
                        self->field_0x384 = blk->reach;
                        addVec3(&t1, &seg_a, &seg_b);
                        vec3_scale(&t2, &t1, 0.5f);
                        copyVec3(&self->vec_0x36C, &t2);
                    }
                }
                if (use_segment == 0) {
                    self->field_0x384 = data->speed_0x30;
                    copyVec3(&self->vec_0x36C, &data->vec_0x24);
                }
                break;
            case 8:
                em_data_point_get(self, ((self->area_no & 0xF) << 8) | self->field_0x382, &self->vec_0x36C);
                break;
            case 9:
                if (em_waypoint_get(self, self->area_no, &self->vec_0x36C) == 0) {
                    setVector3(&self->vec_0x36C, 0.0f, 0.0f, 0.0f);
                }
                break;
            case 10:
            case 11:
                dir.x = 0.0f;
                dir.y = 0.0f;
                dir.z = 500.0f;
                angle = self->field_0x382 << 8;
                if (self->state_0x381 == 10) {
                    angle += self->field_0x1C0;
                }
                rotVecY(&dir, angle);
                addVec3(&t3, &self->pos, &dir);
                copyVec3(&self->vec_0x36C, &t3);
                break;
            case 12:
            case 13:
                self->field_0x383 = 1;
                if (self->state_0x381 == 13) {
                    self->field_0x383 |= 0x80;
                }
                self->field_0x384 = 100.0f * self->field_0x382;
                wave_offset = 1;
                copyVec3(&self->vec_0x36C, &self->pos);
                break;
            }
            break;
        case 6:
            return;
        case 7:
            niku = niku_find(self->state_0x381, self->field_0x382);
            if (niku == NULL) {
                return;
            }
            copyVec3(&self->vec_0x36C, &niku->shell_0x00->pos_0x18);
            break;
        case 8:
            em_special_part_apply(self, self->state_0x381);
            if (self->recs_0x9A4 == NULL) {
                return;
            }
            em_data_point_get(self, ((self->area_no & 0xF) << 8) |
                                              (u8)((EmPartSpots*)self->recs_0x9A4)->spots[self->field_0x382].key,
                                    &self->vec_0x36C);
            break;
        case 9:
            area = self->area_no;
            if (self->field_0x382 == 0xFF || self->state_0x381 != self->field_0x9F8) {
                if (em_waypoint_get(self, self->state_0x381, &self->vec_0x36C) == 0) {
                    setVector3(&self->vec_0x36C, 0.0f, 0.0f, 0.0f);
                }
            } else {
                em_data_point_get(self, ((self->state_0x381 & 0xF) << 8) | self->field_0x382, &self->vec_0x36C);
            }
            if (area != self->state_0x381) {
                t4 = get_worldworld_pos(&self->pos, self->area_no);
                copyVec3(&here, &t4);
                t5 = get_worldworld_pos(&self->vec_0x36C, self->state_0x381);
                copyVec3(&there, &t5);
                subVec3(&t6, &there, &here);
                addVec3(&t7, &self->pos, &t6);
                copyVec3(&self->vec_0x36C, &t7);
                self->vec_0x36C.y = 2000.0f + stg_area_ceiling_get(self->area_no);
                self->field_0x1FA = 1;
            } else {
                self->vec_0x36C.y = stg_area_ceiling_get(self->area_no);
                self->field_0x1FA = 0;
            }
            break;
        case 10:
            em_patrol_points_get(self, self->area_no, &points);
            em_patrol_point_get(self, points, self->field_0x382, &self->vec_0x36C);
            break;
        case 0xFF:
            dir.x = 0.0f;
            dir.y = 0.0f;
            dir.z = 1000.0f;
            rotVecY(&dir, self->field_0x1C0);
            addVec3(&t8, &self->pos, &dir);
            copyVec3(&self->vec_0x36C, &t8);
            break;
        default:
            rec = (u8*)get_move_work_adrs(2);
            if (rec[0] == 0) {
                return;
            }
            copyVec3(&self->vec_0x36C, &((_PLW*)rec)->vec_0x03C);
            break;
        }
    }
    switch (self->field_0x383 & ~0x80) {
    case 1:
        self->field_0x383 = (self->field_0x383 & 0x80) | 2;
        self->field_0x388 = ran_suu(1);
    case 2:
        kind = (self->field_0x383 & 0x80) != 0;
        if (use_segment == 0) {
            if (wave_offset == 1) {
                em_wave_offset(&self->vec_0x36C, &self->vec_0x36C, kind, self->field_0x388, self->field_0x384);
            } else {
                em_wave_amp(&self->vec_0x36C, &self->vec_0x36C, kind, self->field_0x388, self->field_0x384);
            }
        } else {
            em_wave_segment(&self->vec_0x36C, &seg_a, &seg_b, kind, self->field_0x388, self->field_0x384);
        }
        break;
    }
}

/* 0x80128B80 (0x78): puts the enemy out of the fight: flags +0x1F8 and clears its anger, stamina, ailments,
 * alert and every status timer. */
extern "C" void em_combat_reset(_ENEMY_WORK* self)
{
    self->field_0x1F8 = 1;
    em_anger_clear(self);
    em_stamina_reset(self);
    self->sleep_timer_0x950 = 0;
    em_ailment_reset(self, 0xF7);
    self->field_0x916 = 0;
    self->recover_timer_0x952 = 0;
    self->field_0x94C = 0;
    self->field_0x812 = 0;
    self->field_0x814 = 0;
    self->field_0x94A = 0;
    em_user_data_reenter(self);
}

extern "C" void em_hit_buff_apply(_ENEMY_WORK* self, _HIT_W* hit);
u16 em_get_mot_no(_ENEMY_WORK* self);

/* 0x8012933C (0x32C): arms hit record `slot` with attack `attack` of the enemy data: the hit power and knock (the
 * window shifted by the motion frame), the guard kind by the flags and the rank, and the shared hit id. */
extern "C" void em_hit_window_set(_ENEMY_WORK* self, u8 slot, u32 attack, u32 flags)
{
    _HIT_DATA* attacks = get_enemy_data(self)->extra->attacks;
    _HIT_W* hit = (_HIT_W*)&self->hit_slots_0xA08[slot];
    _HIT_SIZE_DATA** sizes = get_enemy_data(self)->extra->attack_sizes;
    _HIT_DATA* data;
    f32 shift;
    f32 zero;

    if (attacks == NULL || sizes == NULL) {
        return;
    }
    hit_flags_clear(hit);
    data = &attacks[attack];
    hit->size_data_0x08 = sizes[data->size_index_0x0F];
    self->hit_flags_0xAEC[slot] = 0;
    if ((u16)flags & 2) {
        self->hit_flags_0xAEC[slot] |= 1;
    }
    if (((u16)flags & 1) == 0) {
        hit_motion_set(hit, em_get_mot_no(self), 0, 0, 0);
    }
    if (((u16)flags & 4) == 0) {
        hit_flag_set(hit, 0x200);
    }
    hit_flag_set(hit, 0x10);
    if (data->power_0x00 > 0) {
        shift = self->mot_frame_0x068 - 2.0f;
        if (shift < 0.0f || (self->mot_flags_0x070 & 1)) {
            shift = 0.0f;
        }
        hit->power_0x38 = data->power_0x00;
        hit->knock_0x3C = data->knock_0x02;
        hit->power_0x38 += shift - self->mot_frame_0x098;
        zero = 0.0f;
        if (hit->power_0x38 < zero) {
            hit->knock_0x3C += hit->power_0x38;
            hit->power_0x38 = zero;
            if (hit->knock_0x3C <= zero) {
                hit->active_0x05 = 0;
                return;
            }
        }
    } else {
        hit->power_0x38 = -data->power_0x00;
        hit->knock_0x3C = data->knock_0x02;
    }
    hit_data_apply(hit, data);
    hit->attack_flags_0x31 = 0x17;
    if ((u8)em_get_rank(self) <= 1) {
        hit->attack_flags_0x31 &= 0xEF;
    }
    if ((u16)flags & 0x40) {
        hit->attack_flags_0x31 = 0x10;
    }
    if ((u16)flags & 0x100) {
        hit->attack_flags_0x31 = 2;
    }
    if ((u16)flags & 0x200) {
        hit->attack_flags_0x31 = 5;
    }
    if ((u16)flags & 0x80) {
        hit->attack_flags_0x31 |= 0x20;
    }
    hit->attack_kind_0x32 = 2;
    if (((u16)flags & 0x10) && self->field_0xAE9 == 1) {
        hit->hit_id_0x0C = self->field_0xAEA;
    }
    if ((u16)flags & 8) {
        self->field_0xAEA = hit->hit_id_0x0C;
        self->field_0xAE9 = 1;
    } else if (((u16)flags & 0x20) == 0) {
        self->field_0xAEA = 0;
        self->field_0xAE9 = 0;
    }
    em_hit_buff_apply(self, hit);
}

/* The enemy data's hit buff (`EnemyData::hit_buff_0x68`): the attack mask it applies to and whether it is off.
 * size: 0x4 (lower bound) */
struct EmHitBuff {
    /* +0x0 */ u16 mask;
    /* +0x2 */ u8 off;
    /* +0x3 */ u8 pad_0x3;
};

/* 0x80129674 (0xB0): raises the hit's strength byte by the rank's buff when the enemy data's buff mask matches. */
extern "C" void em_hit_buff_apply(_ENEMY_WORK* self, _HIT_W* hit)
{
    EmHitBuff* buff = get_enemy_data(self)->hit_buff_0x68;
    s16 add;
    u8 cur;

    if (buff != NULL && buff->off == 0) {
        add = ((EmRankRecord*)em_rank_rec_get(self))->hit_buff;
        if (add > 0) {
            cur = hit->field_0x05A;
            if (cur != 0 && (hit->mask_0x46 & buff->mask)) {
                if (cur + add < 0xFF) {
                    hit->field_0x05A = cur + (u8)add;
                } else {
                    hit->field_0x05A = 0xFF;
                }
            }
        }
    }
}

/* 0x80129724 (0x20): switches hit record `slot` off. */
extern "C" void em_hit_window_clear(_ENEMY_WORK* self, u32 slot)
{
    ((_HIT_W*)&self->hit_slots_0xA08[(u8)slot])->active_0x05 = 0;
    self->hit_flags_0xAEC[(u8)slot] = 0;
}

/* 0x80129744 (0x4C): switches both hit records off. */
extern "C" void em_hit_window_clear_all(_ENEMY_WORK* self)
{
    u8 i = 0;

    do {
        em_hit_window_clear(self, i);
        i++;
    } while (i < 2);
}

/* One 8-byte area record of `em_area_rec_get`: the area and its three status frame values.
 * size: 0x8 */
struct EmAreaStatusRec {
    /* +0x0 */ u8 area;
    /* +0x1 */ u8 pad_0x1;
    /* +0x2 */ s16 frames_a;
    /* +0x4 */ s16 frames_b;
    /* +0x6 */ s16 frames_c;
};

/* 0x80129790 (0x7C): loads the area record's status frames (+0x44C/+0x44E/+0x450, default 9000) and rolls the
 * +0x1F2 seed. */
extern "C" void em_area_status_init(_ENEMY_WORK* self)
{
    EmAreaStatusRec* rec = (EmAreaStatusRec*)em_area_rec_get(self);

    if (rec != NULL) {
        self->value_0x44E = rec->frames_a;
        self->field_0x450 = rec->frames_b;
        self->value_0x44C = rec->frames_c;
    } else {
        self->value_0x44E = 0;
        self->field_0x450 = 0;
        self->value_0x44C = 9000;
    }
    self->value_0x452 = 0;
    self->field_0x1F2 = ran_suu(1);
}

/* 0x8012980C (0x58): runs the +0x44C timer down in status 1 outside play mode 2. */
extern "C" void em_status_timer_tick(_ENEMY_WORK* self)
{
    if (PlayMode_ck() != 2 && self->field_0x43D == 1) {
        if (self->value_0x44C > 0) {
            self->value_0x44C--;
        }
    }
}

extern "C" void fn_80129864(_ENEMY_WORK* self);
extern "C" void fn_801299D8(_ENEMY_WORK* self);
extern "C" void fn_80126898(_ENEMY_WORK* self);
extern "C" void em_motion_kind_tick(_ENEMY_WORK* self);
extern "C" void em_sight_tick(_ENEMY_WORK* self);
extern "C" void em_status_update(_ENEMY_WORK* self);
extern "C" void em_alt_mode_tick(_ENEMY_WORK* self);
extern "C" void em_tired_tick(_ENEMY_WORK* self);
extern "C" void em_level_tick(_ENEMY_WORK* self);
extern "C" void em_poison_update(_ENEMY_WORK* self);
extern "C" void em_sleep_timer_tick(_ENEMY_WORK* self);
extern "C" void em_fx_timer_tick(_ENEMY_WORK* self);
extern "C" void em_busy_timer_tick(_ENEMY_WORK* self);
extern "C" void em_timer_0x81C_tick(_ENEMY_WORK* self);
extern "C" void em_timer_0x81E_tick(_ENEMY_WORK* self);
extern "C" void em_status_bit_tick(_ENEMY_WORK* self);
extern "C" void em_recover_timer_tick(_ENEMY_WORK* self);
extern "C" void em_mark_timer_tick(_ENEMY_WORK* self);
extern "C" void em_timer_0x94A_tick(_ENEMY_WORK* self);
extern "C" void em_hit_timer_tick(_ENEMY_WORK* self);
extern "C" void em_break_timer_tick(_ENEMY_WORK* self);
extern "C" void em_ailment_decay_all(_ENEMY_WORK* self);
extern "C" void em_motion_timer_tick(_ENEMY_WORK* self);
extern "C" void em_shake_tick(_ENEMY_WORK* self);

/* 0x8012987C (0x108): runs every per-frame timer of the enemy (outside event demos and the +0x46A freeze). */
extern "C" void em_timers_update(_ENEMY_WORK* self)
{
    if (event_demo_ck() == 0 && self->freeze_0x46A == 0) {
        em_motion_kind_tick(self);
        em_sight_tick(self);
        em_status_update(self);
        em_alt_mode_tick(self);
        em_tired_tick(self);
        em_status_timer_tick(self);
        fn_80129864(self);
        fn_801299D8(self);
        em_level_tick(self);
        em_poison_update(self);
        em_sleep_timer_tick(self);
        em_fx_timer_tick(self);
        em_busy_timer_tick(self);
        em_timer_0x81C_tick(self);
        em_timer_0x81E_tick(self);
        em_status_bit_tick(self);
        em_recover_timer_tick(self);
        em_mark_timer_tick(self);
        em_timer_0x94A_tick(self);
        em_hit_timer_tick(self);
        em_break_timer_tick(self);
        em_ailment_decay_all(self);
        fn_80126898(self);
    }
    em_motion_timer_tick(self);
    em_shake_tick(self);
}

/* 0x8012999C (0x3C): sets the +0x8B0 flag once the enemy is idle. */
extern "C" void em_idle_flag2_set(_ENEMY_WORK* self)
{
    if (em_busy_ck(self) == 1) {
        self->flag_0x8B0 = 1;
    }
}

extern "C" void fn_80129864(struct _ENEMY_WORK* self)
{
    if (self->value_0x452 < 27000) {
        self->value_0x452++;
    }
}

extern "C" void fn_80129984(struct _ENEMY_WORK* self)
{
    self->field_0x1FC = 0;
    self->field_0x1FE = 0;
    self->field_0x1FF = 255;
}

extern "C" void em_motion_kind_set(_ENEMY_WORK* self, u8 kind, u8 sub);
extern "C" void em_record_0x750_clear(_ENEMY_WORK* self);
extern "C" u8 em_phase_get(_ENEMY_WORK* self);
extern "C" void fn_80129984(struct _ENEMY_WORK* self);
extern "C" u32 em_area_leave_ck(struct _ENEMY_WORK* self);
extern "C" u32 em_area_roam_pick(_ENEMY_WORK* self, u16 seed);

/* 0x8012A254 (0x100): the per-frame area decision: keep the area (play mode 2, or an area without special parts),
 * else ask hook 21 or roll the leave and status-move requests; clears the +0x8B0 flag. */
extern "C" void em_area_decide(_ENEMY_WORK* self, u16 seed)
{
    u32 (*hook)(_ENEMY_WORK*, u16);

    if (PlayMode_ck() == 2) {
        fn_80129984(self);
    } else if ((self->field_0x1C8 & 0x200) == 0 && em_special_part_map_get(get_enemy_data(self)) != NULL &&
               em_special_part_set_get(get_enemy_data(self), self->area_no) == NULL) {
        fn_80129984(self);
    } else {
        hook = (u32 (*)(_ENEMY_WORK*, u16))em_hook_get(self, 21);
        if (hook != NULL) {
            if (hook(self, seed) != 1) {
                fn_80129984(self);
            }
        } else if (em_area_roam_pick(self, seed) != 1 && em_area_leave_ck(self) != 1) {
            fn_80129984(self);
        }
    }
    self->flag_0x8B0 = 0;
}

/* 0x8012A354 (0x60): settles a new area: its status frames, the area goal (cleared once reached) and the move
 * timers. */
extern "C" void em_area_enter_init(_ENEMY_WORK* self)
{
    em_area_status_init(self);
    if (self->field_0x9F8 == 0xFF || self->area_no == self->field_0x9F8) {
        fn_80129984(self);
    }
    self->field_0x200 = 0;
    self->field_0x1FD = 0;
    self->field_0x202 = 900;
}

/* 0x8012A3B4 (0x60): probes the ground under the enemy: the water bits, the surface and ceiling heights and, on
 * ground, the floor height. */
extern "C" void em_ground_probe(_ENEMY_WORK* self)
{
    f32 floor;

    self->ground_mode_0x208 = pl_ground_probe(&self->pos, &self->field_0x228, &floor, &self->field_0x210,
                                              &self->field_0x214, &self->field_0x23C, 0xFFFF);
    if (self->ground_mode_0x208 == 1) {
        self->field_0x20C = floor;
    }
}

/* 0x8012A624 (0x34): the `LandData` constructor (constructs the hit position). */
extern "C" LandData* em_land_data_init(LandData* land)
{
    VEC3_ctor(&land->vec_0x08);
    return land;
}

extern "C" void em_reaction_req(_ENEMY_WORK* self, u8 kind, u8 sub, u8 index);

/* 0x8012A658 (0x1B8): moves the enemy into the next area: keeps the previous area, resets the per-area state,
 * steps the area goal (or ends it), resettles and probes the ground, then calls hook 13. */
extern "C" void em_area_change(_ENEMY_WORK* self, u8 quiet)
{
    void (*hook)(_ENEMY_WORK*);

    copyVec3(&self->prev_pos, &self->pos);
    self->state_0x999 = 0;
    self->field_0x99A = 0;
    self->field_0x99B = 0;
    self->field_0x812 = 0;
    self->field_0x9F6 = self->area_no;
    self->area_no = self->field_0x9F7;
    fn_801376BC(self, 2);
    em_hit_window_clear_all(self);
    if (quiet != 1 && self->field_0x380 != 8 && (self->field_0x1C8 & 1)) {
        quest_target_notice_spawn(self, self->field_0x9F6);
    }
    em_motion_kind_set(self, 0, 0);
    em_record_0x750_clear(self);
    em_target_pos_set(self, NULL);
    if (self->field_0x9F8 != 0xFF && self->area_no != self->field_0x9F8) {
        self->field_0x9F7 = em_area_next(self, self->area_no, self->field_0x9F8);
        if (self->field_0x380 == 9) {
            em_reaction_req(self, 9, self->field_0x9F7, self->field_0x382);
        }
    } else {
        if ((self->flags_0xA04 & 1) && self->area_no == self->field_0xA05) {
            self->flags_0xA04 &= 0xFE;
            self->field_0xA05 = 0xFF;
            self->field_0xA06 = 0;
        }
        self->field_0x1F9 = 0;
        self->field_0x9F8 = 0xFF;
        if (self->field_0x380 != 9) {
            em_reaction_req(self, 6, 0xFF, 0);
        }
    }
    em_area_enter_init(self);
    em_ground_probe(self);
    hook = em_hook_get(self, 13);
    if (hook != NULL) {
        hook(self);
    }
}

/* 0x8012A8F8 (0x40): the `PlBox` constructor (constructs its three vectors). */
extern "C" PlBox* em_pl_box_init(PlBox* box)
{
    VEC3_ctor(&box->vec_0x00);
    VEC3_ctor(&box->vec_0x0C);
    VEC3_ctor(&box->vec_0x18);
    return box;
}

/* 0x8012A938 (0xB0): whether the enemy may use exit `exit` here: on a map of kind 3, exit 1 of area 6 and exit 2
 * of area 8 only from water areas 1 and 2. */
extern "C" u32 em_area_exit_ck(_ENEMY_WORK* self, u8 exit)
{
    if (stage_map_kind_get(self->field_0x1E0) == 3) {
        if (self->area_no == 6) {
            if (exit == 1 && em_phase_get(self) != 1) {
                return 0;
            }
        } else if (self->area_no == 8 && exit == 2 && em_phase_get(self) != 2) {
            return 0;
        }
    }
    return 1;
}

extern "C" u32 em_sight_player_ck2(_ENEMY_WORK* self, u8 player);
/* 0x8012A810 (0xE8): the squared distance from the reach shape (mode 2: the segment `a`..`b`, else the point `a`)
 * to `target` into `*dist`; 0 when a limited mode (1/2) finds it beyond `limit`. */
extern "C" u32 em_target_reach_ck(_ENEMY_WORK* self, u8 mode, nw4r::math::VEC3* a, nw4r::math::VEC3* b,
                                  nw4r::math::VEC3* target, f32* dist, f32 limit)
{
    PlSegment seg;
    PlBox box;
    nw4r::math::VEC3 near;

    pl_segment_init(&seg);
    em_pl_box_init(&box);
    if (mode != 2) {
        *dist = vec3_dist_sq(a, target);
    } else {
        copyVec3(&seg.start, a);
        copyVec3(&seg.end, b);
        pl_box_set((PlBox*)&seg, &box);
        *dist = pl_box_point_dist_sq(&box, target, &near);
    }
    if ((u32)(mode - 1) <= 1 && *dist > limit) {
        return 0;
    }
    return 1;
}

extern "C" s32 fn_8012C0EC(_ENEMY_WORK* enemy, u32 index);
extern "C" s32 fn_8012C204(_ENEMY_WORK* enemy);
extern "C" s32 fn_8012C20C(_ENEMY_WORK* enemy, u32 index);
extern "C" u32 em_area_exit_ck(_ENEMY_WORK* self, u8 exit);
extern "C" _ENEMY_WORK* em_ally_find(_ENEMY_WORK* self, u32 team, u32 near);
extern "C" u32 em_break_state_ck(_ENEMY_WORK* self);

/* 0x8012A9E8 (0x998): picks the target of kind `*kind` (0/1: a player, 2: the AI partner, 3: another enemy, ...)
 * by the selection `*sel` (0..8: by hate or nearest, from the enemy, its aim or its data entry, within the
 * reach; 9: a team mate) and the filter bits of `*sel`, and writes the result triple. */
extern "C" void em_target_select(_ENEMY_WORK* self, u8* kind, u8* sel, u8* index)
{
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;
    u8 pick_player = 0xFF;
    u8 pick_enemy = 0xFF;
    u8 pick_npc = 0xFF;
    u8 ties = 0;
    f32 dist = -1.0f;
    f32 best_dist = 0.0f;
    s32 best = -1;
    u8 result = 1;
    u8 picked = 0xFF;
    struct _ENEMY_DATA* data = NULL;
    u32 hidden;
    u32 torch;
    u32 ground;
    u32 water;
    s32 nearest;
    s32 shape;
    u8 reach_mode;
    f32 reach;
    EmModelBlockView* blk;
    u16 max;
    _PLW* plw;
    u8 i;
    s32 hate;
    _ENEMY_WORK* work;

    VEC3_ctor(&from);
    VEC3_ctor(&to);
    if (*sel & 0x80) {
        hidden = 1;
        *sel &= 0x7F;
    } else {
        hidden = 0;
    }
    if (*sel & 0x40) {
        torch = 1;
        *sel &= 0xBF;
    } else {
        torch = 0;
    }
    if (*sel & 0x20) {
        ground = 1;
        *sel &= 0xDF;
    } else {
        ground = 0;
    }
    if (*sel & 0x10) {
        water = 1;
        *sel &= 0xEF;
    } else {
        water = 0;
    }
    switch (*sel) {
    case 0:
        nearest = 0;
        shape = 0;
        break;
    case 3:
        nearest = 0;
        shape = 1;
        copyVec3(&from, &self->pos);
        break;
    case 4:
        nearest = 0;
        shape = 2;
        copyVec3(&from, &self->aim);
        break;
    case 5:
        nearest = 0;
        shape = 3;
        data = (struct _ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        if (data != NULL) {
            copyVec3(&from, &data->vec_0x24);
        } else {
            copyVec3(&from, &self->pos);
        }
        break;
    case 1:
        nearest = 1;
        shape = 0;
        copyVec3(&from, &self->pos);
        break;
    case 6:
        nearest = 1;
        shape = 1;
        copyVec3(&from, &self->pos);
        break;
    case 7:
        nearest = 1;
        shape = 2;
        copyVec3(&from, &self->aim);
        break;
    case 8:
        nearest = 1;
        shape = 3;
        data = (struct _ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        if (data != NULL) {
            copyVec3(&from, &data->vec_0x24);
        } else {
            copyVec3(&from, &self->pos);
        }
        break;
    case 9:
        *kind = 3;
        work = em_ally_find(self, *index, 0);
        if (work != NULL && (u32)fn_8012E5A8_view1(work) == 1) {
            work = NULL;
        }
        if (work != NULL) {
            *sel = 2;
            *index = work->group;
        } else {
            *sel = 2;
            *index = 0xFF;
        }
        return;
    default:
        return;
    }
    switch (shape) {
    case 3:
        if (data != NULL && (reach = data->speed_0x30) >= 0.0f) {
            reach_mode = 1;
            if (data->field_0x36 != 0xFF) {
                blk = (EmModelBlockView*)eft_def_model_block((struct EftSlot*)data, data->field_0x36);
                if (blk != NULL && blk->live == 1) {
                    reach_mode = 2;
                    setVector3(&from, blk->a[0], blk->a[1], blk->a[2]);
                    setVector3(&to, blk->b[0], blk->b[1], blk->b[2]);
                    reach = blk->reach;
                }
            }
            reach = reach * reach;
        } else {
            reach = 100.0f * *index;
            reach = reach * reach;
            reach_mode = 1;
        }
        break;
    case 0:
        reach = 0.0f;
        reach_mode = 0;
        break;
    default:
        reach = 100.0f * *index;
        reach = reach * reach;
        reach_mode = 1;
        break;
    }
    if (*kind <= 1 || *kind == 4) {
        max = get_move_work_max(2);
        plw = get_move_work_adrs(2);
        for (i = 0; i < max; i++, plw++) {
            if (plw->slot_active == 0 || em_pl_slot_leaving_ck(plw->chunk_ofs) != 0 || em_pl_target_ck(self, plw) == 0) {
                continue;
            }
            if (hidden == 1 && pl_condition_ck_20000(plw) == 0) {
                continue;
            }
            if (torch == 1 && pl_torch_ck(plw) == 0) {
                continue;
            }
            if (ground == 1 && plw->kind_0x09 == 3) {
                continue;
            }
            if (water == 1 && plw->kind_0x09 != 3) {
                continue;
            }
            if (em_sight_player_ck2(self, i) == 0 || em_area_exit_ck(self, plw->water_area_0x5A6) == 0) {
                continue;
            }
            hate = fn_8012C0EC(self, i);
            if (nearest == 0 && hate < (s32)self->timer_0x424) {
                continue;
            }
            if ((nearest == 1 || shape != 0) &&
                em_target_reach_ck(self, reach_mode, &from, &to, &plw->vec_0x03C, &dist, reach) == 0) {
                continue;
            }
            if (pick_player == 0xFF) {
                pick_player = i;
                if (nearest == 1) {
                    best_dist = dist;
                } else {
                    best = hate;
                }
            } else if (nearest != 1) {
                if (best < hate) {
                    pick_player = i;
                    best = hate;
                } else if (best == hate) {
                    ties++;
                    if ((u16)ran_suu(1) % (ties + 1) == 0) {
                        pick_player = i;
                        best = hate;
                    }
                }
            } else if (dist < best_dist) {
                pick_player = i;
                best_dist = dist;
            }
        }
        picked = pick_player;
    }
    if (*kind == 0 || *kind == 2 || *kind == 4) {
        if (ainpc_w.active != 0 && (torch == 0 || ai_torch_ck(&ainpc_w) == 1) && hidden == 0 &&
            self->area_no == ainpc_w.field_0x1A4 && em_area_exit_ck(self, ainpc_w.field_0x326) == 1 &&
            ainpc_w.field_0x171 != 5 && self->field_0x38F != 0) {
            hate = fn_8012C204(self);
            if ((ground == 0 || ainpc_w.variant != 2) && (water == 0 || ainpc_w.variant == 2) &&
                (nearest != 0 || hate >= (s32)self->timer_0x424) &&
                ((nearest != 1 && shape == 0) ||
                 em_target_reach_ck(self, reach_mode, &from, &to, &ainpc_w.vec_0x178, &dist, reach) != 0)) {
                if (pick_player == 0xFF) {
                    pick_npc = 0;
                    if (nearest == 1) {
                        best_dist = dist;
                    } else {
                        best = hate;
                    }
                } else if (nearest != 1) {
                    if (best < hate) {
                        pick_npc = 0;
                        best = hate;
                    }
                } else if (dist < best_dist) {
                    pick_npc = 0;
                    best_dist = dist;
                }
            }
        }
        if (pick_npc != 0xFF) {
            result = 2;
            picked = pick_npc;
        }
    }
    if ((u8)(*kind + 253) <= 1) {
        if (torch == 0) {
            max = get_move_work_max(3);
            work = get_move_work_adrs_view3(3);
            for (i = 0; i < max; i++, work++) {
                if (self->group == i || work->active == 0 || self->area_no != work->area_no ||
                    self->team == work->team) {
                    continue;
                }
                if ((self->field_0x1C8 & 1) && (work->field_0x1C8 & 1) == 0) {
                    continue;
                }
                if ((u32)em_ally_kind_ck(self, work->team) == 1 || work->action == 0xB || work->action == 0xC) {
                    continue;
                }
                if (hidden == 1 && em_break_state_ck(work) == 0) {
                    continue;
                }
                if (ground == 1 && work->field_0x1E2 == 2) {
                    continue;
                }
                if (water == 1 && work->field_0x1E2 != 2) {
                    continue;
                }
                if (em_area_exit_ck(self, em_phase_get(work)) == 0) {
                    continue;
                }
                hate = fn_8012C20C(self, i);
                if (nearest == 0 && hate < (s32)self->timer_0x424) {
                    continue;
                }
                if ((nearest == 1 || shape != 0) &&
                    em_target_reach_ck(self, reach_mode, &from, &to, &work->pos, &dist, reach) == 0) {
                    continue;
                }
                if (pick_enemy == 0xFF && pick_player == 0xFF && pick_npc == 0xFF) {
                    pick_enemy = i;
                    if (nearest == 1) {
                        best_dist = dist;
                    } else {
                        best = hate;
                    }
                } else if (nearest != 1) {
                    if (best < hate) {
                        pick_enemy = i;
                        best = hate;
                    }
                } else if (dist < best_dist) {
                    pick_enemy = i;
                    best_dist = dist;
                }
            }
        }
        if (pick_enemy != 0xFF) {
            result = 3;
            picked = pick_enemy;
        }
    }
    *kind = result;
    *sel = 2;
    *index = picked;
}

extern "C" void em_target_select(_ENEMY_WORK* self, u8* kind, u8* sub, u8* index);

/* 0x8012B380 (0x244): sets the target kind triple and resolves it: kinds 0..4 pick a target, kind 5 (a route
 * direction) falls back to kind 6, kind 8 latches the special part and picks its first spot in line of sight. */
extern "C" void em_reaction_req(_ENEMY_WORK* self, u8 kind, u8 sub, u8 index)
{
    nw4r::math::VEC3 eye;
    nw4r::math::VEC3 spot_pos;
    nw4r::math::VEC3 hit;
    EmPartSpot* spot;
    f32 lift;
    u8 i;
    u8 mode;

    VEC3_ctor(&eye);
    VEC3_ctor(&spot_pos);
    VEC3_ctor(&hit);
    self->field_0x380 = kind;
    self->state_0x381 = sub;
    self->field_0x382 = index;
    if ((self->field_0x383 & ~0x80) == 2) {
        self->field_0x383 = 0;
    }
    switch (self->field_0x380) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        em_target_select(self, &self->field_0x380, &self->state_0x381, &self->field_0x382);
        break;
    case 5:
        if (self->state_0x381 <= 3 || (u32)(self->state_0x381 - 10) <= 3) {
            em_target_pos_set(self, NULL);
            em_reaction_req(self, 6, 0xFF, 0);
            return;
        }
        break;
    case 8:
        em_special_part_apply(self, self->state_0x381);
        if (self->field_0x382 == 0xFF && self->recs_0x9A4 != NULL) {
            spot = ((EmPartSpots*)self->recs_0x9A4)->spots;
            copyVec3(&eye, &self->pos);
            if ((u32)(self->field_0x1E2 - 1) <= 1) {
                lift = 0.0f;
            } else {
                lift = 50.0f;
            }
            eye.y += lift;
            for (i = 0; spot->key != 0xFFFF; i++, spot++) {
                if (spot->kind == em_phase_get(self) && spot->sight_mode != 1) {
                    mode = 1;
                    if (spot->sight_mode == 2) {
                        mode = 3;
                    }
                    em_data_point_get(self, ((self->area_no & 0xF) << 8) | (u8)spot->key, &spot_pos);
                    spot_pos.y += lift;
                    if (findInterSection(&eye, &spot_pos, &hit, mode, 0xFFFF, self->area_no, em_hit_mask_get(self),
                                         NULL) == 0) {
                        self->field_0x382 = i;
                        break;
                    }
                }
            }
            if (self->field_0x382 == 0xFF) {
                self->field_0x382 = 0;
            }
        }
        break;
    }
    self->timer_0x424 = 0;
}

/* 0x8012B608 (0x44): adds `amount` to player `player`'s hate, clamped to [0, 20000]. */
extern "C" void em_hate_add(_ENEMY_WORK* self, u8 player, s32 amount)
{
    self->hate_0x428[player] += amount;
    if (self->hate_0x428[player] > 20000) {
        self->hate_0x428[player] = 20000;
    } else if (self->hate_0x428[player] < 0) {
        self->hate_0x428[player] = 0;
    }
}

/* The hate gains of the four distance bands (the default when the enemy data has none).
 * size: 0x10 */
struct EmHateBands {
    /* +0x0 */ s32 band[4];
};

/* The enemy data's hate table: the band distances and the gains.
 * size: 0x8 */
struct EmHateTable {
    /* +0x0 */ f32* distances;
    /* +0x4 */ s32* gains;          /* four band gains, the out-of-range and lost-sight losses, the sight bonus */
};

static const EmHateBands em_hate_bands_default = {300, 15, 10, 5};

/* 0x8012B64C (0x220): updates the hate of every live player by its distance band (half for player kind 1), with
 * a loss out of range or out of sight and a bonus for a seen player in status 2. */
extern "C" void em_hate_update(_ENEMY_WORK* self)
{
    s32 g0 = em_hate_bands_default.band[0];
    s32 g1 = em_hate_bands_default.band[1];
    s32 g2 = em_hate_bands_default.band[2];
    s32 g3 = em_hate_bands_default.band[3];
    s32 out = -2;
    s32 lost = -5;
    s32 bonus = 100;
    f32* dist;
    s32* gains;
    u16 max;
    _PLW* plw;
    u16 i;
    s32 add;
    f32 d;

    if (self->field_0x95C == 3) {
        return;
    }
    dist = get_enemy_data(self)->extra->hate->distances;
    gains = get_enemy_data(self)->extra->hate->gains;
    if (gains != NULL) {
        g0 = gains[0];
        g1 = gains[1];
        g2 = gains[2];
        g3 = gains[3];
        out = gains[4];
        lost = gains[5];
        bonus = gains[6];
    }
    max = get_move_work_max(2);
    plw = get_move_work_adrs(2);
    for (i = 0; i < max; i++, plw++) {
        if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0) {
            if (em_pl_target_ck(self, plw) == 0 || em_sleep_ck(self, 0) == 1) {
                add = lost;
            } else {
                d = self->frames_0x454[i];
                if (d < 0.0f) {
                    add = lost;
                } else if (d <= dist[0]) {
                    add = g0;
                } else if (d <= dist[1]) {
                    add = g1;
                } else if (d <= dist[2]) {
                    add = g2;
                } else if (d <= dist[3]) {
                    add = g3;
                } else {
                    add = out;
                }
                if (plw->kind_0x09 == 1) {
                    if (add > 0) {
                        add /= 2;
                    }
                } else if (self->field_0x43D == 2 && em_sight_player_ck2(self, i) == 1) {
                    add += bonus;
                }
            }
            em_hate_add(self, i, add);
        }
    }
}

extern "C" u32 fn_8012B5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c)
{
    if (self->field_0x380 == (u8)a) {
        if (self->state_0x381 == (u8)b) {
            if (self->field_0x382 == (u8)c) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void em_target_refresh(_ENEMY_WORK* self)
{
}

extern "C" void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_request(self, (u8)a, (u8)b);
}

extern "C" void em_act_request_now(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    em_act_step_arm(self, (u8)a, (u8)b, 1);
    fn_801281EC(self);
}

extern "C" void em_act_set_unless_act11(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_request_now(self, (u8)a, (u8)b);
}

extern "C" void em_act_arm_unless_down(struct _ENEMY_WORK* self, u8 a, u8 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, a, b, 1);
}

extern "C" void em_act_arm_unless_act11(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 2);
}

extern "C" void em_act_request_react(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 5);
}

extern "C" void fn_80128AEC(struct _ENEMY_WORK* self, u32 a, u32 b)
{
    if (self->action == 11) {
        return;
    }
    em_act_step_arm(self, (u8)a, (u8)b, 0);
}

extern "C" void fn_801299D8(struct _ENEMY_WORK* self)
{
    if (--self->field_0x200 <= 0) {
        self->field_0x200 = 0;
    }
    if (--self->field_0x202 <= 0) {
        self->field_0x202 = 0;
    }
}

extern "C" u32 em_roll_cooldown_ck(void* unused, u32 a, u32 b, s16* timer)
{
    if (*timer <= 0) {
        if ((u16)a % 100 < (u16)b) {
            return 1;
        }
        *timer = 900;
    }
    return 0;
}

extern "C" u32 em_hit_timer_ck(_ENEMY_WORK* self);
extern "C" u32 em_roll_cooldown_ck(void* unused, u32 a, u32 b, s16* timer);
extern "C" s32 em_mot_finished_ck(_ENEMY_WORK* self);

/* 0x80129A70 (0x2CC): rolls the "leave the area" request on the +0x202 timer (or the +0x8B0 flag), the chance
 * set by the rank of the other monsters in the area and the hit timer's hit bits; 1 when it is requested. */
extern "C" u32 em_area_roam_pick(_ENEMY_WORK* self, u16 seed)
{
    _ENEMY_WORK* work = get_move_work_adrs_view3(3);
    u16 max = get_move_work_max(3);
    u8 rolls = 0;
    u32 hit;
    u16 chance;
    s32 rival;
    u8 rank;
    u16 i;

    if (self->field_0x202 <= 0) {
        if (em_hit_timer_ck(self) == 1) {
            self->field_0x202 = 450;
        } else {
            self->field_0x202 = 900;
        }
        rolls = 1;
    }
    if (self->flag_0x8B0 == 1) {
        rolls++;
    }
    if (rolls != 0) {
        if (self->flag_0x8B0 == 1) {
            hit = 1;
        } else {
            hit = em_hit_timer_ck(self);
        }
        chance = 0;
        if (hit == 1 && self->field_0x43D != 1) {
            self->field_0x1FC = 1;
            self->field_0x1FE = 0;
            self->field_0x1FF = 0xFF;
            return 1;
        }
        rival = 0;
        rank = em_get_rank(self);
        for (i = 0; i < max; i++, work++) {
            if (self->group != i && work->active != 0 && work->area_no == self->area_no && work->action != 0xB &&
                work->action != 0xC && (u32)em_ally_kind_ck(self, work->team) != 1) {
                if ((u8)em_get_rank(work) > (u8)rank) {
                    rival = 2;
                    break;
                }
                if ((u8)em_get_rank(work) == (u8)rank) {
                    rival = 1;
                }
            }
        }
        for (i = 0; i < rolls; i++) {
            if (rival == 1) {
                if (hit == 1) {
                    chance = 50;
                } else {
                    chance = 20;
                }
            } else if (rival == 2) {
                chance = 40;
                if (hit == 1) {
                    chance = 80;
                }
            } else if (hit == 1) {
                chance = 20;
            }
            if (em_hit_timer_ck(self) == 1) {
                if ((s16)self->field_0x814 & 8) {
                    chance += 50;
                } else if ((s16)self->field_0x814 & 2) {
                    chance += 30;
                } else if ((s16)self->field_0x814 & 4) {
                    chance += 20;
                }
            }
            if (chance > 100) {
                chance = 100;
            }
            if (seed % 100 < chance) {
                self->field_0x1FC = 1;
                self->field_0x1FE = 0;
                self->field_0x1FF = 0xFF;
                return 1;
            }
            hit = em_hit_timer_ck(self);
            seed = seed >> 8;
        }
    }
    return 0;
}

/* 0x80129D3C (0x7C): requests the move to the +0xA05 area the stun flag of +0xA04 asks for, when reachable. */
extern "C" u32 em_area_stun_move_ck(_ENEMY_WORK* self)
{
    if ((self->flags_0xA04 & 1) && self->area_no != self->field_0xA05 &&
        em_area_enter_ck(self, self->field_0x1E0, self->field_0xA05) == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 6;
        self->field_0x1FF = self->field_0xA05;
        return 1;
    }
    return 0;
}

/* 0x80129DB8 (0x90): once the motion is done: 1 when the enemy is in area `area`, else (its status time up)
 * requests the move there and returns 2. */
extern "C" u32 em_area_break_ck(_ENEMY_WORK* self, u8 area)
{
    if ((u32)em_mot_finished_ck(self) == 1) {
        if (self->area_no == area) {
            return 1;
        }
        if (self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1) {
            self->field_0x1FC = 1;
            self->field_0x1FE = 1;
            self->field_0x1FF = area;
            return 2;
        }
    }
    return 0;
}

/* 0x80129E48 (0x114): 1 when a fighting player is reachable here; else requests the move to the area of the first
 * one the enemy can enter and returns 2. */
extern "C" u32 em_area_player_follow_ck(_ENEMY_WORK* self)
{
    _PLW* plw = get_move_work_adrs(2);
    u16 i;
    u16 max = get_move_work_max(2);

    for (i = 0; i < max; i++, plw++) {
        if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && plw->field_0x00A != 8 &&
            (plw->field_0x5C4 & 0xF)) {
            if ((u32)em_pl_target_ck(self, plw) == 1) {
                return 1;
            }
            if (em_area_enter_ck(self, self->field_0x1E0, plw->area_0x16) == 1) {
                em_area_dist_get(self, self->field_0x1E0, plw->area_0x16);
                self->field_0x1FC = 1;
                self->field_0x1FE = 9;
                self->field_0x1FF = plw->area_0x16;
                return 2;
            }
        }
    }
    return 0;
}

/* One stamina gate: the stamina threshold (0: the anger state) and the rate it yields; a threshold of 0xFFFF
 * closes the list.
 * size: 0x4 */
struct EmStaminaGate {
    /* +0x0 */ u16 threshold;
    /* +0x2 */ u16 rate;
};

/* 0x80129F5C (0xB8): the rate of the first gate the enemy's stamina (or anger) passes; 0 when none does. */
extern "C" u16 em_stamina_gate_get(_ENEMY_WORK* self, EmStaminaGate* gate)
{
    u16 rate;
    u32 pass;

    if (gate == NULL) {
        return 0;
    }
    rate = 0;
    for (; gate->threshold != 0xFFFF; gate++) {
        if (gate->threshold == 0) {
            if ((u32)em_eft_active_ck(self) == 1) {
                pass = 1;
            } else {
                pass = 0;
            }
        } else {
            pass = self->field_0x8A2 <= gate->threshold;
        }
        if (pass != 0) {
            rate = gate->rate;
            break;
        }
    }
    return rate;
}

/* 0x8012A014 (0x1F0): once the status time is up, requests a move: to the area of a busy team-`team` mate when
 * one is elsewhere (giving up while any team mate is near), else to area `area`, each when the stamina gates and
 * the move roll allow it. */
extern "C" u32 em_pack_area_pick(_ENEMY_WORK* self, u8 team, u8 area, u16 rate, EmStaminaGate* mate_gates,
                                  EmStaminaGate* area_gates)
{
    u16 mate_rate;
    u16 area_rate;
    u32 blocked;
    u16 max;
    _ENEMY_WORK* work;
    u16 i;

    if (self->value_0x452 < self->field_0x450 && self->field_0x43D == 1) {
        return 0;
    }
    mate_rate = em_stamina_gate_get(self, mate_gates);
    area_rate = em_stamina_gate_get(self, area_gates);
    if (mate_rate != 0 && team != 0) {
        blocked = 0;
        max = get_move_work_max(3);
        work = get_move_work_adrs_view3(3);
        for (i = 0; i < max; i++, work++) {
            if (self->group != i && work->active != 0 && work->team == team && (work->field_0x1C8 & 8)) {
                if (work->action != 0xB && work->action != 0xC && self->area_no != work->area_no &&
                    em_area_enter_ck(self, self->field_0x1E0, work->area_no) == 1 &&
                    em_roll_cooldown_ck(self, rate, mate_rate, &self->field_0x200) == 1) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 4;
                    self->field_0x1FF = work->area_no;
                    return 1;
                }
                blocked = 1;
            }
        }
        if (blocked == 1) {
            return 0;
        }
    }
    if (area_rate != 0 && area != 0xFF && self->area_no != area &&
        em_roll_cooldown_ck(self, rate, area_rate, &self->field_0x200) == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 3;
        self->field_0x1FF = area;
        return 1;
    }
    return 0;
}

extern "C" u32 em_area_leave_ck(struct _ENEMY_WORK* self)
{
    if (self->value_0x44C <= 0
        || (self->field_0x43D != 1 && self->value_0x452 > self->value_0x44E)) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 2;
        self->field_0x1FF = 255;
        return 1;
    }
    return 0;
}

extern "C" u32 fn_8012B86C(struct _ENEMY_WORK* self)
{
    u8 mask = 0;
    for (u32 i = 0; i < 4; i++) {
        if (self->values_0x3A4[33 + i] >= 20000) {
            mask |= 1 << i;
        }
    }
    return mask;
}

extern "C" void fn_8012B8F8(struct _ENEMY_WORK* self)
{
    for (u32 i = 0; i < 4; i++) {
        if (self->values_0x3A4[33 + i] >= 20000) {
            self->values_0x3A4[33 + i] = 0;
        }
    }
}

extern "C" void em_hate_pl_add(struct _ENEMY_WORK* self, u32 a, s32 b)
{
    s32 v;
    self->values_0x390[(u8)a] += b;
    v = self->values_0x390[(u8)a];
    if (v > 20000) {
        self->values_0x390[(u8)a] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x390[(u8)a] = 0;
    }
}

extern "C" void em_hate_misc_add(struct _ENEMY_WORK* self, s32 b)
{
    s32 v = self->values_0x390[4] + b;
    self->values_0x390[4] = v;
    if (v > 20000) {
        self->values_0x390[4] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x390[4] = 0;
    }
}

extern "C" void em_hate_em_add(struct _ENEMY_WORK* self, u32 a, s32 b)
{
    s32 v;
    self->values_0x3A4[(u8)a] += b;
    v = self->values_0x3A4[(u8)a];
    if (v > 20000) {
        self->values_0x3A4[(u8)a] = 20000;
        return;
    }
    if (v < 0) {
        self->values_0x3A4[(u8)a] = 0;
    }
}

#pragma fp_contract off

extern "C" {
/* The frame cost of motion `type` for slot `part`: the per-type extra cost, then the distance band
 * the distance to `other` falls in, plus a fraction of the last entry on the two flagged paths. */
s32 fn_8012BA00(_ENEMY_WORK* enemy, const MOTION_SET* motion, _ENEMY_WORK* other, u32 type, u32 part)
{
    s32 result = 0;
    const f32* thresholds;
    const s32* frames;
    u32 flag; /* "add the extra cost", set by each type's test; unset for any other type */
    u32 add;
    VEC3 vec;
    const s32* extra;
    f32 dist;

    VEC3_ctor(&vec);
    add = 0;

    thresholds = motion->thresholds;
    frames = motion->frames;

    switch ((u8)type) {
    case 1:
        flag = (fn_8012D0B4_view1(enemy, other) == 1);
        extra = motion->extra_frames;
        if (extra != 0) {
            if (pl_status_bit_ck(other, 0x200) != 0) {
                result = extra[0];
            }
            if ((other->flags_0x5C4 & 0xF) != 0) {
                result += extra[1];
            }
            if (flag == 1 && fn_8012D7FC_view1(other) == 1) {
                result += extra[2];
            }
        }
        if (other->field_0x009 == 3) {
            add = 1;
        }
        break;

    case 2:
        flag = (fn_8012D188_view1(enemy, other) == 1);
        copyVec3(&vec, &other->pos_0x178);
        extra = motion->extra_frames;
        if (extra != 0) {
            if (ai_npc_flag_ck(other, 1) != 0) {
                result = extra[0];
            }
            if (other->flag_0x420 == 1 && other->count_0x422 > 0) {
                result += extra[1];
            }
        }
        if (other->state_0x170 == 2) {
            add = 1;
        }
        break;

    case 3:
        flag = (enemy->area_no == other->area_no);
        copyVec3(&vec, &other->pos);
        if (other->field_0x1E2 == 2) {
            add = 1;
        }
        break;
    }

    if (enemy->field_0x43D != 1) {
        result += frames[4];
    } else if (flag == 0 || em_sleep_ck(enemy, 0) == 1) {
        result += frames[5];
    } else {
        if ((u8)type == 1) {
            dist = enemy->frames_0x454[(u16)part];
        } else {
            dist = vec3_distance_view1(&enemy->pos, &vec);
        }

        if (dist < lbl_80796C58) {
            result += frames[5];
        } else if (dist <= thresholds[0]) {
            result += frames[0];
        } else if (dist <= thresholds[1]) {
            result += frames[1];
        } else if (dist <= thresholds[2]) {
            result += frames[2];
        } else if (dist <= thresholds[3]) {
            result += frames[3];
        } else {
            result += frames[4];
        }

        if (enemy->action == 7) {
            if (enemy->field_0x380 == (u8)type && enemy->field_0x382 == (u16)part) {
                if (enemy->field_0x1E2 == 2) {
                    result = (s32)((f32)result + lbl_80796C8C * (f32)frames[6]);
                } else {
                    result += frames[6];
                }
            }
        } else if (enemy->field_0x1E2 == 2 && add == 0) {
            result = (s32)((f32)result + lbl_80796C70 * (lbl_80796C8C * (f32)frames[6]));
        } else if (enemy->field_0x1E2 == 2) {
            /* the state is 2 and the slot is not flagged: nothing to add */
        } else if (add == 1) {
            result = (s32)((f32)result + lbl_80796C70 * (f32)frames[6]);
        }
    }

    return result;
}
}

#pragma fp_contract on


/* Scores every player-side work record against its action table and hands the result to
 * `em_hate_pl_add`; then, when the global state allows it, picks the enemy's target for `em_hate_em_add`. */
extern "C" void fn_8012BDF4(_ENEMY_WORK* enemy)
{
    EnemyActionSet* set;
    EnemyActionTable* table;
    _PLW* work;
    _ENEMY_WORK* target;
    u16 max;
    u16 i;
    u16 enemy_max;
    u16 j;
    s32 value;

    set = get_enemy_data(enemy)->extra->action_set;
    if (set == NULL || (table = set->table_0x000) == NULL) {
        table = lbl_805A0FF8[0];
    }
    max = get_move_work_max(2);
    work = get_move_work_adrs(2);
    for (i = 0; i < max; i++) {
        if (work->slot_active != 0 && em_pl_slot_leaving_ck(work->chunk_ofs) == 0) {
            value = fn_8012BA00_view1(enemy, table, work, 1, i);
            if (value > 0) {
                if (Pl_Skill_ck(work, 0xC) == 1) {
                    value = (s32)((f32)value * lbl_80796C90);
                }
                if (Pl_Skill_ck(work, 0xB) == 1) {
                    value = (s32)((f32)value * lbl_80796C94);
                    if (value <= 0) {
                        value = 1;
                    }
                }
            }
            fn_8012B944_view1(enemy, (u8)i, value);
        }
    }
    if (ainpc_w.active != 0) {
        if (ainpc_w.field_0x171 != 5) {
            if (set == NULL || (table = set->table_0x004) == NULL) {
                table = lbl_805A1034[0];
            }
            value = fn_8012BA00_view1(enemy, table, &ainpc_w, 2, 0);
        } else {
            value = -5;
        }
        em_hate_misc_add(enemy, value);
    }
    if (set == NULL || (table = set->table_0x008) == NULL) {
        table = lbl_805A106C[0];
    }
    enemy_max = get_move_work_max(3);
    target = (_ENEMY_WORK*)get_move_work_adrs(3);
    for (j = 0; j < enemy_max; j++, target++) {
        if (enemy->group == j || target->active == 0 || enemy->team == target->team ||
            em_ally_kind_ck(enemy, target->team) == 1 || (u8)(target->action - 0x0B) <= 1) {
            enemy->values_0x3A4[j] = 0;
        } else {
            fn_8012B9BC_view1(enemy, (u8)j, fn_8012BA00_view1(enemy, table, target, 3, j));
        }
    }
}


/* The attacker's damage against one player-side record: the stored value plus the two table
 * adjustments whose conditions the player passes. */
extern "C" s32 fn_8012C0EC(_ENEMY_WORK* enemy, u32 index)
{
    EnemyActionSet* set;
    EnemyActionTable* table;
    EnemyActionEntry* entry;
    _PLW* work;
    s32 value;

    work = get_move_work_adrs(2);
    set = get_enemy_data(enemy)->extra->action_set;
    value = enemy->values_0x390[(u8)index];
    if (set == NULL || (table = set->table_0x000) == NULL) {
        table = lbl_805A0FF8[0];
    }
    work += (u8)index;
    if (work->slot_active != 0 && em_pl_slot_leaving_ck(work->chunk_ofs) == 0 && em_pl_target_ck(enemy, work) == 1) {
        entry = table->entry;
        if (entry->condition_0x00C != 0 && Pl_condition_ck(work, entry->condition_0x00C) == 1) {
            value += entry->value_0x010;
        }
        if (entry->condition_0x014 != 0 && Pl_dm_condition_ck(work, entry->condition_0x014) == 1) {
            value += entry->value_0x018;
        }
    }
    if (value < 0) {
        value = 0;
    }
    return value;
}


/* The enemy's fourth per-attacker value. */
extern "C" s32 fn_8012C204(_ENEMY_WORK* enemy)
{
    return enemy->values_0x390[4];
}

/* The per-attacker value at `values_0x390[index + 5]`. */
extern "C" s32 fn_8012C20C(_ENEMY_WORK* enemy, u32 index)
{
    return enemy->values_0x3A4[(u8)index];
}


/* Claims the first enemy-side record of `team` that no other record's `state_sub` owns, marking it
 * with `flags_0xA04` bit 0 and returning 1; 0 when none is free. */
extern "C" s32 em_team_member_claim(u32 team, u32 state_sub)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no != (u8)state_sub) {
            if ((work->flags_0xA04 & 1) == 0) {
                work->flags_0xA04 |= 1;
                work->field_0xA05 = state_sub;
                work->field_0xA06 = 0;
            }
            return 1;
        }
    }
    return 0;
}


/* Sets `flags_0xA04` bit 1 on every enemy-side record of `team` whose area matches `state_sub` and
 * returns 1 when at least one was marked. */
extern "C" s32 em_team_area_alert_set(u32 team, u32 state_sub)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 found;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    found = 0;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            work->flags_0xA04 |= 2;
            found = 1;
        }
    }
    return found;
}


/* Sets `flags_0xA04` bit 2 on every enemy-side record of `team` whose area matches `state_sub` and
 * whose position lies within `radius` of `ref`. */
extern "C" s32 em_team_area_alert_set_near(u32 team, u32 state_sub, void* ref, f32 radius)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 found;
    f32 zero;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    found = 0;
    zero = lbl_80796C58;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            if (ref == NULL || radius < zero ||
                vec3_dist_sq(ref, &work->pos) <= radius * radius) {
                work->flags_0xA04 |= 4;
                found = 1;
            }
        }
    }
    return found;
}


/* Sets `flags_0xA04` bit 2 on every enemy-side record of `team` whose area matches `state_sub` and
 * whose position lies within `radius` of `ref`, and hands each one to `em_anger_add`. */
extern "C" void fn_8012C4E8(u32 team, u32 state_sub, s16 arg3, void* ref, f32 radius)
{
    _ENEMY_WORK* work;
    u16 max;
    u16 i;
    f32 zero;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    zero = lbl_80796C58;
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && work->action != 0x0B && work->action != 0x0C && work->team == (u8)team &&
            work->area_no == (u8)state_sub) {
            if (ref == NULL || radius < zero ||
                vec3_dist_sq(ref, &work->pos) <= radius * radius) {
                work->flags_0xA04 |= 4;
                em_anger_add(work, arg3);
            }
        }
    }
}


/* Runs the pending bits of `flags_0xA04` down: the sleep flag (bit 1) is cleared, and the stun flag
 * (bit 0) counts one frame before it is released. */
extern "C" void fn_8012C600(_ENEMY_WORK* enemy)
{
    if (enemy->flags_0xA04 != 0) {
        if ((enemy->flags_0xA04 & 2) != 0) {
            em_status_set(enemy, 1);
            enemy->flags_0xA04 &= 0xFD;
        }
        if ((enemy->flags_0xA04 & 1) != 0) {
            if (enemy->field_0xA06 == 0) {
                enemy->field_0xA06 = enemy->field_0xA06 + 1;
                fn_8012CE8C(enemy, 8);
            } else if (enemy->area_no == enemy->field_0xA05) {
                enemy->flags_0xA04 &= 0xFE;
                enemy->field_0xA05 = 0xFF;
                enemy->field_0xA06 = 0;
                fn_8012CE9C(enemy, 8);
            }
        }
        if ((enemy->flags_0xA04 & 4) != 0) {
            enemy->field_0xA07 = 1;
            enemy->flags_0xA04 &= 0xFB;
        }
    }
}


/* Latches the "hit by" byte unless the enemy is already in the state that consumes it. */
extern "C" void em_hit_latch_set(_ENEMY_WORK* enemy)
{
    if (enemy->field_0x43D != 1) {
        enemy->field_0x43A = 1;
    }
}


/* Marks the enemy as being hit for the given attacker: clears `field_0x798` and, when `mask` names
 * any live attacker, picks the last one whose bit is set, then runs the hit state machine. */
extern "C" u32 fn_8012C6F4(_ENEMY_WORK* enemy, u32 mask, s32 arg2)
{
    _PLW* work;
    u16 max;
    u8 i;

    if ((enemy->field_0x1C8 & 0x80) != 0) {
        return 0;
    }
    if (enemy->field_0x95C == 5) {
        return 0;
    }
    em_status_set(enemy, 1);
    if (fn_8013A884(enemy, 5) == 1) {
        max = get_move_work_max(2);
        work = get_move_work_adrs(2);
        enemy->field_0x798 = 0xFF;
        if ((u8)mask != 0xFF) {
            for (i = 0; i < max; i++, work++) {
                if (work->slot_active != 0 && em_pl_slot_leaving_ck(work->chunk_ofs) == 0 &&
                    ((u8)mask & (1 << i)) != 0 && em_pl_target_ck(enemy, work) != 0) {
                    enemy->field_0x798 = i;
                    break;
                }
            }
        }
        if (arg2 == 0) {
            fn_8012CDF4(enemy, 5, 0);
        } else if (fn_8013A8B4(enemy, 5, 1) == 1) {
            fn_8012CDF4(enemy, 5, 1);
        } else {
            fn_8012CDF4(enemy, 5, 0);
        }
        return 1;
    }
    return 0;
}


/* The knock-down variant of `fn_8012C6F4`: gated on the shell timer, it picks the last live attacker
 * out of `mask` and enters the knock-down state. */
extern "C" u32 fn_8012C870(_ENEMY_WORK* enemy, u32 mask)
{
    _PLW* work;
    u16 max;
    u8 i;

    if (enemy->field_0x95C == 4) {
        return 0;
    }
    if (enemy->field_0x916 > 0) {
        return 0;
    }
    if ((enemy->field_0x38C & 1) == 0) {
        return 0;
    }
    em_status_set(enemy, 2);
    if (fn_8013A884(enemy, 4) == 1) {
        max = get_move_work_max(2);
        work = get_move_work_adrs(2);
        enemy->field_0x798 = 0xFF;
        for (i = 0; i < max; i++, work++) {
            if (work->slot_active != 0 && em_pl_slot_leaving_ck(work->chunk_ofs) == 0 &&
                ((u8)mask & (1 << i)) != 0 && em_pl_target_ck(enemy, work) != 0) {
                enemy->field_0x798 = i;
                break;
            }
        }
        fn_8012CDF4(enemy, 4, 0);
        return 1;
    }
    return 0;
}


/* 0x8012C9AC (0x448): the per-frame action pick: marks the sighted players in status 1, then (when idle and
 * free) tries the attack on the seen players, the alert reactions, the enraged and special state switches and the
 * rest state. */
extern "C" void em_act_pick(_ENEMY_WORK* self)
{
    u32 rest = fn_8012CF2C(self);
    u8 seen;
    s32 force;
    s32 found;
    u16 max;
    _PLW* plw;
    s32 i;
    u8 bit;
    u8 mask;
    u8 kind;

    self->field_0x798 = 0xFF;
    seen = 0;
    force = 0;
    found = 0;
    if (self->field_0x43D == 1) {
        if (self->field_0x439 == 1) {
            max = get_move_work_max(2);
            plw = get_move_work_adrs(2);
            for (i = 0; i < max; i++, plw++) {
                if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && (u32)em_pl_target_ck(self, plw) == 1) {
                    if (self->field_0x38F & (1 << i)) {
                        seen |= (u8)(1 << i);
                        found = 1;
                        self->field_0x439 = 0;
                    }
                }
            }
        }
    } else {
        self->field_0x439 = 0;
    }
    if (self->field_0x79B == 1) {
        if (self->field_0x1E8 == 0 && self->field_0x1E9 == 0) {
            self->field_0x79B = 0;
        }
        return;
    }
    if (em_busy_ck(self) == 0 || self->field_0x784 == 1 || em_status_ck(self, 0xFD) == 1 ||
        (u8)(self->action + 248) <= 5 || em_sleep_ck(self, 0) == 1 || self->field_0x1F8 != 0 ||
        em_userdata_state_kind_get(self) == 12) {
        return;
    }
    mask = fn_8012B86C(self);
    if (self->field_0x43D == 1) {
        if (found != 0 && (self->field_0x1E2 != 1 || self->field_0x1E3 == 1) && self->action != 7 &&
            self->field_0x916 <= 0) {
            force = 1;
        } else {
            seen = 0;
        }
    } else {
        seen = self->field_0x38F & mask;
    }
    if (seen != 0 && fn_8012C6F4(self, seen, force) == 1) {
        if (force == 0) {
            self->field_0x43E = 1;
        }
        return;
    }
    if (self->field_0x43D == 0) {
        if (self->field_0x38F != 0) {
            if (self->field_0x1C8 & 1) {
                if (fn_8012C870(self, self->field_0x38F) == 1) {
                    return;
                }
            } else if (fn_8012C6F4(self, self->field_0x38F, 0) == 1) {
                return;
            }
        } else if (self->field_0x43A != 0 && fn_8012C6F4(self, 0xFF, 0) == 1) {
            return;
        }
    }
    if (self->field_0x43D == 0 && self->field_0x95C != 3 && mask != 0) {
        for (i = 0; i < 4; i++) {
            if (mask & (1 << i)) {
                self->field_0x798 = i;
                break;
            }
        }
        fn_8012B8F8(self);
        if (fn_8013A884(self, 3) == 1) {
            fn_8012CDF4(self, 3, 0);
            return;
        }
    }
    kind = em_userdata_state_kind_get(self);
    if ((u32)(kind - 6) <= 1 || (u32)(kind - 11) <= 1) {
        return;
    }
    if (fn_8012CF04(self, 4) == 1 && fn_8013A884(self, 6) == 1) {
        fn_8012CDF4(self, 6, 0);
        em_alt_mode_set(self, 1);
        return;
    }
    if (fn_8012CF04(self, 8) == 1 && fn_8013A884(self, 11) == 1) {
        fn_8012CDF4(self, 11, 0);
        fn_8012CE9C(self, 8);
        return;
    }
    if (rest == 1) {
        if (fn_8013A884(self, 7) == 1) {
            fn_8012CDF4(self, 7, self->field_0x799);
        } else {
            em_user_data_reenter(self);
            em_finish_flag_set(self);
        }
        fn_8012CE9C(self, 2);
    }
}

/* Switches the enemy to the given state pair, saving the previous state and its label first. */
extern "C" void fn_8012CDF4(_ENEMY_WORK* enemy, u32 state, u32 sub)
{
    enemy->field_0x9DC = enemy->field_0x95C;
    enemy->field_0x9DD = enemy->field_0x95D;
    enemy->field_0x9E0 = enemy->field_0x958;
    enemy->field_0x9E4 = em_idle_flag_get(enemy);
    enemy->field_0x95C = state;
    enemy->field_0x95D = sub;
    enemy->field_0x958 = enemy->field_0x954[(u8)state][(u8)sub];
    enemy->field_0x79A = 1;
    em_finish_flag_set(enemy);
}


/* Sets the given bits in the per-attacker flag word. */
extern "C" void fn_8012CE8C(_ENEMY_WORK* enemy, u16 flag)
{
    enemy->field_0x794 |= flag;
}

/* Clears the given bits in the per-attacker flag word. */
extern "C" void fn_8012CE9C(_ENEMY_WORK* enemy, u32 flag)
{
    enemy->field_0x794 &= ~flag;
}

/* Arms the "hit by" timer and remembers which attack it was, then marks the state word. */
extern "C" void em_hit_by_set(_ENEMY_WORK* enemy, s16 timer, u8 index)
{
    fn_8012CE8C(enemy, 2);
    enemy->field_0x796 = timer;
    enemy->field_0x799 = index;
}

/* Whether any of the given bits is set in the per-attacker flag word. */
extern "C" u32 fn_8012CF04(_ENEMY_WORK* enemy, u32 flag)
{
    return (enemy->field_0x794 & flag) != 0;
}

/* Marks the enemy busy for this frame. */
extern "C" void em_busy_set(_ENEMY_WORK* enemy)
{
    enemy->field_0x79B = 1;
}


/* Runs the "hit by" timer down and reports 1 on the frame it expires. */
extern "C" s32 fn_8012CF2C(_ENEMY_WORK* enemy)
{
    s16 timer;

    if (fn_8012CF04(enemy, 2) == 1) {
        timer = enemy->field_0x796 - 1;
        enemy->field_0x796 = timer;
        if (timer <= 0) {
            enemy->field_0x796 = 0;
            return 1;
        }
    }
    return 0;
}


/* Whether the enemy's own value list contains `value` (0x28 is always a hit). */
extern "C" s32 em_ally_kind_ck(_ENEMY_WORK* enemy, u32 value)
{
    u8* list = get_enemy_data(enemy)->values;

    if ((u8)value == 0x28) {
        return 1;
    }
    if (list != NULL) {
        while (*list != 0) {
            if (*list == (u8)value) {
                return 1;
            }
            list++;
        }
    }
    return 0;
}


/* Whether the enemy record is still alive: an inactive record, or one whose death flag has run past
 * 2, counts as dead. */
s32 em_work_die_ck(_ENEMY_WORK* enemy)
{
    if (enemy->active != 0 && enemy->field_0x004 < 2) {
        return 0;
    }
    return 1;
}

/* Whether the enemy is in its death state. */
s32 em_die_ck(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0B || em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xC, 0xFF) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the area the game is currently in. */
s32 em_area_ck(_ENEMY_WORK* enemy)
{
    return get_now_areano() == enemy->area_no;
}

/* Whether the enemy and the given work record share an area. */
extern "C" s32 fn_8012D188(_ENEMY_WORK* enemy, _PLW* work)
{
    return work->effect_key_0x1A4 == enemy->area_no;
}

/* The enemy's activity byte. */
extern "C" u32 em_busy_ck(_ENEMY_WORK* enemy)
{
    return enemy->field_0x1F5;
}

/* Whether the given player slot is in the "out of action" state; only player 0 is looked at. */
extern "C" s32 em_pl_slot_leaving_ck(u32 index)
{
    _PLAYER_ROOT* root;

    if (isServerSelectState() == 1) {
        root = (_PLAYER_ROOT*)get_move_work_adrs(0);
        if (root != NULL && root->player_state[(u8)index] == 4) {
            return 1;
        }
    }
    return 0;
}

/* Whether the enemy is in the given state pair. */
extern "C" u32 em_act_ck__FP11_ENEMY_WORKUcUc(_ENEMY_WORK* enemy, u32 state, u32 sub)
{
    if (enemy->action == (u8)state && enemy->state_sub == (u8)sub) {
        return 1;
    }
    return 0;
}


/* Whether the enemy may attack the given work record: same area, not already out, and past the
 * map-specific position gate. */
extern "C" s32 em_pl_target_ck(_ENEMY_WORK* enemy, _PLW* work)
{
    if (em_pl_slot_leaving_ck(work->chunk_ofs) == 1) {
        return 0;
    }
    if (work->field_0x00A == 8) {
        return 0;
    }
    if (enemy->area_no == work->area_0x16) {
        if (stage_map_kind_get(enemy->field_0x1E0) == 9) {
            if (Pl_area_flag_get(work->chunk_ofs) == 0) {
                if (stage_pos_z_low_ck(&enemy->pos) == 1) {
                    return 1;
                }
            } else if (stage_pos_z_low_ck(&enemy->pos) == 0) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x8012D23C (0x1A4): whether player `slot` is a live, reachable player in the action family `kind` asks for
 * (kind 0: player kind 7 at act 0/1, or kind 6 act 42 against team 4; kind 1: kind 7 at acts 0/2/5/6, ...). */
extern "C" u32 em_team_mate_state_ck(_ENEMY_WORK* enemy, u32 kind, u32 slot)
{
    _PLW* plw = get_move_work_adrs(2);

    if ((u8)slot >= get_move_work_max(2)) {
        return 0;
    }
    plw = &plw[(u8)slot];
    if (plw->slot_active == 0 || em_pl_slot_leaving_ck(plw->chunk_ofs) != 0) {
        return 0;
    }
    if (em_pl_target_ck(enemy, plw) == 0) {
        return 0;
    }
    switch ((u8)kind) {
    case 0:
        switch (plw->field_0x00A) {
        case 7:
            if (plw->act_no <= 1) {
                return 1;
            }
            break;
        case 6:
            if (enemy->team != 4) {
                return 0;
            }
            if (plw->act_no == 42) {
                return 1;
            }
            break;
        }
        break;
    case 1:
        switch (plw->field_0x00A) {
        case 7:
            if ((u32)(plw->act_no - 5) <= 1 || plw->act_no == 0 || plw->act_no == 2) {
                return 1;
            }
            break;
        case 6:
            if (enemy->team != 4) {
                return 0;
            }
            if (plw->act_no == 42) {
                return 1;
            }
            break;
        }
        break;
    case 2:
        return 0;
    }
    return 0;
}


/* The first attacker slot whose per-kind predicate `em_team_mate_state_ck` accepts the enemy. */
extern "C" s32 fn_8012D3E0(_ENEMY_WORK* enemy, u32 kind)
{
    u16 max;
    s32 i;

    max = get_move_work_max(2);
    for (i = 0; i < max; i++) {
        if (em_team_mate_state_ck(enemy, kind, (u8)i) == 1) {
            return (u8)i;
        }
    }
    return 0xFF;
}

/* Whether the search helper finds any record. */
extern "C" s32 fn_8012D468(u32 selector, _PLW* work, s8* out_flag)
{
    return em_kill_cut_target_get((u8)selector, work, out_flag) != NULL;
}


/* Whether the actor has the "charge" skill, or the pair is on its last frame. */
extern "C" s32 pl_torch_ck(void* arg)
{
    if (Pl_Skill_ck((_PLW*)arg, 0xCD) == 1) {
        return 1;
    }
    return (fn_8027AC18(arg) - 1) == 0;
}

/* Whether the enemy is in the low part of its action state. */
extern "C" s32 em_act_flinch_light_ck(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0A && enemy->state_sub <= 0x12) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the mid-range part of its action state. */
extern "C" s32 em_action_mid_ck(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0A && (u32)(enemy->state_sub - 0x58) <= 0x1F) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the high part of its action state. */
extern "C" s32 em_act_part_flinch_ck(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0A && (u32)(enemy->state_sub - 0xC3) <= 0x0F) {
        return 1;
    }
    return 0;
}

/* The "analysable" action-state list `ana_em_ck` accepts. */
extern "C" s32 em_act_trap_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB7) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBB) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBC) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB9) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBA) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x0F) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1C) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x24) == 1) {
        return 1;
    }
    return 0;
}

/* The "paralysed" action-state list. */
extern "C" s32 em_act_paralysis_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xC0) == 1) {
        return 1;
    }
    return 0;
}

/* The "analysable enemy at position" and "paralysed enemy at position" searches. */

/* Whether one record is an analysable enemy of the given area within `radius` of `pos`. */
extern "C" u32 ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(
    _ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    if (enemy->active == 0) {
        return 0;
    }
    if (enemy->area_no != (u8)area) {
        return 0;
    }
    if ((u8)flag == 0 && enemy->action == 0x0B) {
        return 0;
    }
    if (em_act_trap_ck(enemy) == 0) {
        return 0;
    }
    return calcVecDistXZ(pos, &enemy->pos) <= radius;
}

/* The first analysable enemy of the given area within `radius` of `pos`. */
extern "C" void* ana_em_ck__FUcPQ34nw4r4math4VEC3fUc(
    u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    _ENEMY_WORK* work;
    s16 i;
    u16 max;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (ana_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(work, (u8)area, pos, (u8)flag,
                                                                radius) == 1) {
            return work;
        }
    }
    return NULL;
}

/* Whether one record is a paralysed enemy of the given area within `radius` of `pos`. */
extern "C" u32 shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(
    _ENEMY_WORK* enemy, u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    if (enemy->active == 0) {
        return 0;
    }
    if (enemy->area_no != (u8)area) {
        return 0;
    }
    if (em_act_paralysis_ck(enemy) == 0 &&
        ((u8)flag == 0 || (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1A) == 0 &&
                           em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xB, 0x1B) == 0))) {
        return 0;
    }
    if (enemy->field_0x1E2 == 2) {
        if (vec3_distance(pos, &enemy->pos) <= radius) {
            return 1;
        }
    } else if (calcVecDistXZ(pos, &enemy->pos) <= radius) {
        return 1;
    }
    return 0;
}

/* The first paralysed enemy of the given area within `radius` of `pos`. */
extern "C" void* shibire_em_ck__FUcPQ34nw4r4math4VEC3fUc(
    u32 area, nw4r::math::VEC3* pos, u32 flag, f32 radius)
{
    _ENEMY_WORK* work;
    s16 i;
    u16 max;

    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (shibire_em_ck_sub__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3fUc(work, (u8)area, pos,
                                                                    (u8)flag, radius) == 1) {
            return work;
        }
    }
    return NULL;
}

/* The action-state list predicates. */

/* The "sleeping" action-state list. */
extern "C" s32 em_act_sleep_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE8) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE9) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xE7) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEA) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEE) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xEC) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xED) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in action state 0xF1 or 0xF2 (an `em_act_ck` list). */
extern "C" s32 em_act_kind23_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF1) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF2) == 1) {
        return 1;
    }
    return 0;
}

/* The "waking up" action-state list. */
extern "C" s32 em_act_wake_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF3) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF6) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xF5) == 1) {
        return 1;
    }
    return 0;
}

/* The "stunned" action-state list. */
extern "C" s32 em_act_stun_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xAF) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB0) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB1) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB2) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB3) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB4) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xB5) == 1) {
        return 1;
    }
    return 0;
}

/* Three-way action-state query: 0/1 for the two states it knows, 0xFF otherwise. */
extern "C" s32 em_act_sleep_side_get(_ENEMY_WORK* enemy)
{
    switch (enemy->action) {
    case 0x0A:
        if ((u32)(enemy->state_sub - 0xE6) <= 3) {
            return 0;
        }
        if ((u32)(enemy->state_sub - 0xEC) <= 3) {
            return 1;
        }
        if (enemy->state_sub == 0xE4) {
            return 0;
        }
        if (enemy->state_sub == 0xEA) {
            return 1;
        }
        return 0xFF;
    case 0x0B:
        if (enemy->state_sub == 0x34 || enemy->state_sub == 0x36) {
            return 0;
        }
        if (enemy->state_sub == 0x35 || enemy->state_sub == 0x37) {
            return 1;
        }
        return 0xFF;
    }
    return 0xFF;
}

/* The per-state action id tests. */

/* Whether the action id is one of the "frontal" attacks. */
extern "C" s32 fn_8012E0D8(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7C:
        case 0x82:
        case 0x8B:
        case 0x95:
        case 0x9C:
        case 0xBB:
        case 0xE8:
        case 0xEE:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "rear" attacks. */
extern "C" s32 fn_8012E158(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7A:
        case 0x7B:
        case 0x80:
        case 0x81:
        case 0x89:
        case 0x8A:
        case 0x93:
        case 0x94:
        case 0x9A:
        case 0x9B:
        case 0xB9:
        case 0xBA:
        case 0xE6:
        case 0xE7:
        case 0xEC:
        case 0xED:
        case 0xF8:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "side" attacks. */
extern "C" s32 fn_8012E21C(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7B:
        case 0x81:
        case 0x8A:
        case 0x94:
        case 0x9B:
        case 0xBA:
        case 0xE7:
        case 0xED:
        case 0xF8:
            return 1;
        }
    }
    return 0;
}

/* Whether the enemy is in the three-state part of the second action state. */
extern "C" s32 em_act_kind6_ck(u32 state, u32 sub)
{
    if ((u8)state == 0xB && (u32)((u8)sub - 0x1A) <= 2) {
        return 1;
    }
    return 0;
}

/* Whether the action id is one of the "leap" attacks. */
extern "C" s32 em_act_leap_ck(u32 state, u32 action)
{
    if ((u8)state == 0xA) {
        switch ((u8)action) {
        case 0x7D:
        case 0x7E:
        case 0x7F:
        case 0x83:
        case 0x8C:
        case 0x96:
        case 0x9D:
        case 0xBC:
        case 0xE9:
        case 0xEF:
            return 1;
        }
    }
    return 0;
}

/* Whether the action id is one of the "roar" attacks. */
extern "C" s32 em_act_roar_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x84) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x85) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x86) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x87) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x88) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x97) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x98) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x99) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0x9E) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the action id is one of the "ball" attacks of the given flavour. */
extern "C" s32 em_act_ball_ck(_ENEMY_WORK* enemy, u32 flag)
{
    if ((u8)flag == 0) {
        if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD6) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD7) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD8) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xD9) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDA) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDB) == 1 ||
            em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xDC) == 1) {
            return 1;
        }
    }
    return 0;
}

/* The two-state action list around the "poison" attacks. */
extern "C" s32 em_act_kind18_ck(_ENEMY_WORK* enemy)
{
    if (em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBD) == 1 ||
        em_act_ck__FP11_ENEMY_WORKUcUc(enemy, 0xA, 0xBE) == 1) {
        return 1;
    }
    return 0;
}

/* Whether the enemy is in the four-state part of the second action state. */
extern "C" s32 em_act11_wait_ck(_ENEMY_WORK* enemy)
{
    if (enemy->action == 0x0B && (u32)(enemy->state_sub - 0x10) <= 3) {
        return 1;
    }
    return 0;
}

/* Per-actor hit/guard flags: 0x22..0x27 want the "guard" bit, 0x1B wants exactly 3. */
extern "C" s32 fn_8012E5D4(u32 actor, u32 flags)
{
    if ((u32)((u8)actor - 0x24) <= 3) {
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
    }
    switch ((u8)actor) {
    case 0x22:
        if ((flags & 2) != 0) {
            return 1;
        }
        return 0;
    case 0x1B:
        if (flags == 3) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* The hit test for the actor/mode pair the record carries. */
extern "C" s32 em_record_hit_ck(_ENEMY_WORK* record)
{
    return fn_8012E5D4(record->team, record->field_0x00A);
}

/* The hit test for the record's secondary actor/mode pair. */
extern "C" s32 em_mini_hit_ck(_ENEMY_WORK* record)
{
    return fn_8012E5D4(record->group, record->team);
}

/* Arms the attack timer and, when it was not already armed, tells the attack system about it. */
extern "C" void em_attack_start(_ENEMY_WORK* enemy)
{
    if (enemy->field_0x011 == 0) {
        enemy->field_0x011 = 1;
        if ((enemy->field_0x1C8 & 8) == 0) {
            emc_kill_report(3);
        }
    }
}

/* Marks the attack timer as expired. */
extern "C" void em_attack_done_set(_ENEMY_WORK* enemy)
{
    enemy->field_0x011 = 2;
}

/* Whether the given actor id has a live hit record. */
extern "C" s32 fn_8012E6A0(u32 kind, u16 id)
{
    u8* entry;

    if (Pl_motion_input_ck(0) == 1) {
        return 0;
    }
    entry = (u8*)quest_spawn_rec_find(id);
    if (entry == NULL || (s16)((u16*)entry)[2] <= 0) {
        return 0;
    }
    return 1;
}


/* 0x8012E708 (0x10): whether the enemy's spawn id has a live hit record of its +0xC mode. */
extern "C" s32 em_spawn_rec_live_ck(_ENEMY_WORK* self)
{
    return fn_8012E6A0(self->field_0x00C, self->field_0x01A);
}

/* 0x8012E718 (0x10): the same test for the halfword id at +0x16 (the two counter bytes read as one). */
extern "C" u32 em_spawn_rec_live_ck2(_ENEMY_WORK* self)
{
    return fn_8012E6A0(self->field_0x00C, *(u16*)&self->field_0x016);
}

/* Whether the enemy is in the "charge" state pair. */
extern "C" u8 em_captured_ck(_ENEMY_WORK* enemy)
{
    return (u8)((em_act_kind6_ck(enemy->action, enemy->state_sub) - 1) == 0);
}

/* The two-state mask the record's mode byte selects. */
extern "C" s32 em_mini_kill_kind_get(_ENEMY_WORK* enemy)
{
    return (enemy->field_0x014 == 2) ? 2 : 0;
}

/* Clears the "attack finished" byte once it has been consumed. */
extern "C" void em_attack_finished_clear(_ENEMY_WORK* record)
{
    if (record->field_0x008 == 5) {
        record->field_0x008 = 0;
    }
}

/* Whether more than `seconds` have passed since the last frame stamp. */
extern "C" s32 fn_8012E8F4(f32 seconds)
{
    s32 now = quest_time_limit_get();

    return (f32)(now - quest_time_elapsed_get()) > seconds;
}

/* The mode-dispatched predicate (see the file header). */
extern "C" s32 em_area_vacant_wait_ck(_ENEMY_WORK* self, u8 mode)
{
    _ENEMY_WORK* work;
    EmAreaEntry* entry;
    u16 max;
    s32 idx;

    switch (mode) {
    case 0:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796C9C);
    case 1:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CA4);
    case 2:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CA8);
    case 3:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CAC);
    case 4:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CB0);
    case 5:
        return fn_8012E8F4(lbl_80796CA0 * Screen_w.frame_scale * lbl_80796CB4);
    case 6:
        work = (_ENEMY_WORK*)get_move_work_adrs_view1(3);
        max = get_move_work_max(3);
        for (idx = 0; idx < max; idx++) {
            if (work->active != 0 && work->area_no == self->field_0x00A &&
                work->team == self->group) {
                return 0;
            }
            work++;
        }
        /* The table walk: 0x60 / 3 = the 32 groups the table holds, four records per group.  `mode`
         * is the target's own counter (see the file header, residual). */
        for (entry = &em_area_entry_tbl[0][0], mode = 0; mode < 0x60; mode += 3, entry += 4) {
            if (entry[0].active != 0 && entry[0].area_no == self->field_0x00A &&
                entry[0].group == self->group && entry[0].field_0x0C == 1) {
                return 0;
            }
            if (entry[1].active != 0 && entry[1].area_no == self->field_0x00A &&
                entry[1].group == self->group && entry[1].field_0x0C == 1) {
                return 0;
            }
            if (entry[2].active != 0 && entry[2].area_no == self->field_0x00A &&
                entry[2].group == self->group && entry[2].field_0x0C == 1) {
                return 0;
            }
            if (entry[3].active != 0 && entry[3].area_no == self->field_0x00A &&
                entry[3].group == self->group && entry[3].field_0x0C == 1) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/* Whether `field_0x89F` is 2 or 3 (`enemy/em_kind.cpp`'s handlers gate on it), written as one unsigned range test:
 * retail's branch-free `subfic`/`orc`/`srwi`/`subf`/`srwi` run is how this compiler spells it. */
extern "C" s32 em_eft_active_ck(_ENEMY_WORK* self)
{
    return (u32)(self->field_0x89F - 2) <= 1;
}

/* Whether the record's latched mode is 1 (`em_alt_mode_set` latches it, 0/1). */
extern "C" s32 em_alt_mode_ck(_ENEMY_WORK* self)
{
    return self->mode_0x8AA == 1;
}

/* Whether the enemy's area/group state admits the record (see the file header). */
extern "C" s32 em_mot_finished_ck(_ENEMY_WORK* self)
{
    f32 rate;

    rate = em_mot_window_base_get(self);
    return (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4 <= rate;
}

/* The mode-selected base window: 0.3/0.2/0.18/0.15 s by the motion's sub-window count
 * `em_get_rank` returns.  The target re-reads the count in each arm, so the call is written per arm. */
extern "C" f32 em_mot_window_base_get(_ENEMY_WORK* self)
{
    if ((u8)em_get_rank(self) <= 1) return lbl_80796CB8;
    if ((u8)em_get_rank(self) <= 2) return lbl_80796CBC;
    if ((u8)em_get_rank(self) <= 3) return lbl_80796CC0;
    return lbl_80796CC4;
}

/* The first team-wide window scan: "(100 - kind) % of the motion has run on a live record of `team`".
 * `team`/`kind` are the two bytes the callers at 0x803B5990 mask out of their table row. */
extern "C" u32 em_team_damage_under_ck(u8 team, u8 kind)
{
    _ENEMY_WORK* work;
    u16 max;
    s32 i;
    f32 want;

    want = (f32)(100 - kind) / lbl_80796C5C;
    work = get_move_work_adrs_view3(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && work->team == team) {
            if ((f32)(s32)work->field_0x7A0 / (f32)(s32)work->field_0x7A4 <= want) {
                return 1;
            }
        }
        work++;
    }
    return 0;
}

/* The second team-wide window scan: "the taken part `(field_0x7AC - field_0x7A0) / field_0x7A4` is at
 * least `kind` %". */
extern "C" u32 em_team_damage_over_ck(u8 team, u8 kind)
{
    _ENEMY_WORK* work;
    u16 max;
    u32 i;
    f32 want;

    want = (f32)kind / lbl_80796C5C;
    work = get_move_work_adrs_view3(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && work->team == team) {
            if ((f32)(s32)(work->field_0x7AC - work->field_0x7A0) / (f32)(s32)work->field_0x7A4 >= want) {
                return 1;
            }
        }
        work++;
    }
    return 0;
}

/* "The record's team id is a live map id and the map's frame gate is the first frame". */
extern "C" s32 em_quest_entry_live_ck(_ENEMY_WORK* self)
{
    if (quest_element_state_find(self->team) != 0) {
        if (quest_entry_active_ck() == 1) return 1;
    }
    return 0;
}

/* The area/status gate that guards the per-motion block: the `field_0x1C8` bit-0 request, the area and
 * magma tests, then the team-specific height/map tests. */
extern "C" s32 em_damage_area_ok_ck(_ENEMY_WORK* self)
{
    if (self->field_0x1C8 & 1) {
        if (em_area_ck_view5(self) == 0) return 0;
        if (em_in_water_ck(self, 1) == 1) return 0;
        if (em_magma_check(self) == 1) return 0;
        if (quest_element_state_find(self->team) == 0x404) return 0;
    }
    switch (self->team) {
    case 0xf:
        if (self->field_0x7B0 > lbl_80796C58) return 0;
        break;
    case 0x19:
        if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 2) return 1;
        return 0;
    case 0x14:
        if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) return 1;
        return 0;
    }
    return 1;
}

/* The program-mode gate for one of the two window masks (0xa / 0xb); the "die" test is the only path
 * that keeps the record alive. */
extern "C" s32 fn_8012F110(_ENEMY_WORK* self, u16 mode)
{
    if (self->field_0x818 > 0) return 0;
    switch (mode) {
    case 0xa:
        if (em_status_bits_ck(self, 0x20) != 0) {
            if (self->field_0x1E2 == 0) {
                if ((u32)em_die_ck(self) != 1) break;
            }
        }
        return 0;
    case 0xb:
        if (em_status_bits_ck(self, 0x40) != 0) {
            if (self->field_0x1E2 == 0 || self->field_0x1E2 == 2) {
                if ((u32)em_die_ck(self) != 1) break;
            }
        }
        return 0;
    }
    return 1;
}

/* The window test with the sub-window padding `em_get_rank` selects (0.1 s or 0.05 s). */
extern "C" u32 em_motion_window_ck(_ENEMY_WORK* self)
{
    if (self->field_0x938 > 0) {
        f32 rate = em_mot_window_base_get(self);
        f32 limit;

        if ((u8)em_get_rank(self) <= 1) {
            limit = rate + lbl_80796CC8;
        } else {
            limit = rate + lbl_80796CCC;
        }
        return (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4 <= limit;
    }
    return 0;
}

/* The window test behind the two landed cross-unit gates: either of the two "in this motion" tests,
 * then the padded window test. */
extern "C" s32 em_ailment8_window_ck(_ENEMY_WORK* self)
{
    if (fn_8012D8D0_view5(self) == 1 || fn_8012DB3C_view5(self) == 1) {
        if (em_motion_window_ck(self) == 1) return 1;
    }
    return 0;
}

/* The sleep gate (`u32`): kind 0 is "can sleep this frame", kind 1 the action lookup. */
u32 em_sleep_ck(_ENEMY_WORK* self, u8 kind)
{
    switch (kind) {
    case 0:
        if (em_status_ck_view1(self, 1) == 1 || fn_80132270(self) == 1) return 1;
        return 0;
    case 1:
        if (fn_8012E21C_view5(self->action, self->state_sub) == 1 || fn_80132270(self) == 1) return 1;
        return 0;
    }
    return 0;
}

/* "The record's kind is 4 and its height is above 0.9 * the model scale `em_height_base_get` returns". */
extern "C" s32 em_tall_ck(_ENEMY_WORK* self)
{
    if (self->field_0x1E2 == 4) {
        if (self->field_0x1AC > lbl_80796CD0 * em_height_base_get(self)) return 1;
    }
    return 0;
}

#pragma fp_contract off

/* The motion band (0x8012F3F4..0x8012FB8C): the per-motion frame table, the blend scale, the motion setters
 * over the embedded MHchar and the per-motion attribute and condition tables. */

/* One `{motion, value}` pair of the enemy data's motion attribute table (`EnemyData::motion_attrs`), closed by
 * motion 0xFFFFFFFF.
 * size: 0x8 */
struct EmMotionAttr {
    /* +0x0 */ u32 motion;
    /* +0x4 */ u8 value;
    /* +0x5 */ u8 pad_0x5[3];
};

/* One gate of the enemy data's motion condition table (`EnemyData::motion_conds`): a test `kind` with its
 * argument, and whether the motion is refused when the test fails (`mode` 1) or passes (`mode` 0).
 * size: 0x10 */
struct EmMotionCond {
    /* +0x0 */ u32 motion;
    /* +0x4 */ u16 kind;
    /* +0x6 */ u16 mode;
    /* +0x8 */ u32 arg;
    /* +0xC */ u32 level;
};

/* 0x8012F3F4 (0x80): returns the s16 frame value of `motion` from the enemy data's per-hundred motion tables. */
extern "C" s32 em_motion_frame_get(_ENEMY_WORK* self, u32 motion)
{
    s32 group = (u16)motion / 100;
    s16* frames = get_enemy_data(self)->motion_frames[group];

    if (frames == NULL) {
        return 0;
    }
    return frames[(u16)motion - group * 100];
}

/* 0x8012F474 (0x90): the motion blend scale: the enemy data's override when the +0x43B kind is 2 outside actions
 * 0xA/0xB/0xD, times the two scale fields. */
extern "C" f32 em_motion_blend_scale_get(_ENEMY_WORK* self)
{
    f32 scale = 1.0f;

    if (self->field_0x43B == 2) {
        if (self->action != 0xA && self->action != 0xB && self->action != 0xD) {
            f32* override = get_enemy_data(self)->extra->blend_scale;
            if (override != NULL) {
                scale = *override;
            }
        }
    }
    return scale * (self->field_0x7BC * self->field_0x7C0);
}

extern "C" void em_mat_blend_reset(_ENEMY_WORK* self);

/* 0x8012F504 (0xB4): starts `motion` on the embedded MHchar unless bit 2 of +0x1C8 freezes the motion. */
extern "C" void em_mot_set_blend(_ENEMY_WORK* self, u32 motion, u32 start, u32 frame, u32 loop)
{
    s32 frames;

    if ((self->field_0x1C8 & 4) == 0) {
        self->field_0x7C4 = 1.0f;
        frames = em_motion_frame_get(self, (u16)motion);
        mhchar_motion_set((struct MHchar*)self->char_0x024, 2, self->field_0x00B, (u16)motion, start, frames, loop,
                          (f32)(s32)frame, em_motion_blend_scale_get(self));
        em_mat_blend_reset(self);
    }
}

/* 0x8012F5B8 (0xC): starts `motion` with no loop argument. */
extern "C" void em_mot_set(_ENEMY_WORK* self, s32 motion, s32 start, s32 frame)
{
    em_mot_set_blend(self, (u16)motion, start, frame, 0);
}

extern "C" u32 em_mot_no_ck(_ENEMY_WORK* self, u32 motion);

/* 0x8012F5C4 (0x68): starts `motion` unless it is already the current one. */
extern "C" void em_mot_set_blend_ck(_ENEMY_WORK* self, u32 motion, s32 start, s32 frame, s32 loop)
{
    if (em_mot_no_ck(self, (u16)motion) != 1) {
        em_mot_set_blend(self, (u16)motion, start, frame, loop);
    }
}

/* 0x8012F62C (0x74): starts `motion` with no loop argument unless it is already the current one. */
extern "C" void em_mot_set_ck(_ENEMY_WORK* self, u32 motion, u32 start, u32 frame)
{
    if (em_mot_no_ck(self, (u16)motion) != 1) {
        em_mot_set_blend(self, (u16)motion, start, frame, 0);
    }
}

/* 0x8012F6A0 (0xAC): starts `motion` on MHchar motion layer `layer` unless bit 2 of +0x1C8 freezes the motion. */
extern "C" void em_mot_layer_blend_set(_ENEMY_WORK* self, s32 layer, u32 motion, s32 start, s32 frame, s32 loop)
{
    s32 frames;

    if ((self->field_0x1C8 & 4) == 0) {
        frames = em_motion_frame_get(self, (u16)motion);
        mhchar_motion_layer_set((struct MHchar*)self->char_0x024, layer, 2, self->field_0x00B, (u16)motion, start,
                                frames, loop, (f32)frame, em_motion_blend_scale_get(self));
    }
}

/* 0x8012F74C (0xC): starts `motion` on a layer with no loop argument. */
extern "C" void em_mot_layer_set(_ENEMY_WORK* self, s32 layer, u32 motion, s32 start, s32 frame)
{
    em_mot_layer_blend_set(self, layer, (u16)motion, start, frame, 0);
}

/* 0x8012F758 (0x7C): starts `motion` on layer 1 and weights the two layers `weight` / 1 - `weight`. */
extern "C" void em_mot_layer_weight_set(_ENEMY_WORK* self, u32 motion, s32 frame, f32 weight)
{
    em_mot_layer_set(self, 1, (u16)motion, 0, frame);
    mhchar_mat_blend_set_c1((struct MHchar*)self->char_0x024, 0, weight);
    mhchar_mat_blend_set_c1((struct MHchar*)self->char_0x024, 1, 1.0f - weight);
}

/* 0x8012F7D4 (0x3C): blends towards `motion_lo` or `motion_hi` by the sign of `dir`. */
extern "C" void em_mot_layer_blend_step(_ENEMY_WORK* self, u32 motion_lo, u32 motion_hi, s32 frame, f32 dir)
{
    f32 weight;

    if (dir >= 0.0f) {
        weight = 1.0f - self->frames_0x454[4];
    } else {
        weight = 1.0f + self->frames_0x454[4];
        motion_lo = motion_hi;
    }
    em_mot_layer_weight_set(self, (u16)motion_lo, frame, weight);
}

/* 0x8012F810 (0x50): resets the two layer weights to 1/0 and clears the blend position. */
extern "C" void em_mat_blend_reset(_ENEMY_WORK* self)
{
    mhchar_mat_blend_set_c1((struct MHchar*)self->char_0x024, 0, 1.0f);
    mhchar_mat_blend_set_c1((struct MHchar*)self->char_0x024, 1, 0.0f);
    self->frames_0x454[4] = 0.0f;
}

/* 0x8012F860 (0x68): steps the blend position towards `target` by `step` and clamps it to [-1, 1]. */
extern "C" void em_motion_blend_step(_ENEMY_WORK* self, f32 target, f32 step)
{
    if (target > self->frames_0x454[4]) {
        self->frames_0x454[4] += step;
        if (self->frames_0x454[4] > target) {
            self->frames_0x454[4] = target;
        }
    } else if (target < self->frames_0x454[4]) {
        self->frames_0x454[4] -= step;
        if (self->frames_0x454[4] < target) {
            self->frames_0x454[4] = target;
        }
    }
    if (self->frames_0x454[4] > 1.0f) {
        self->frames_0x454[4] = 1.0f;
    } else if (self->frames_0x454[4] < -1.0f) {
        self->frames_0x454[4] = -1.0f;
    }
}

/* 0x8012F8C8 (0x1C): sets the motion speed and hands the scaled rate to the MHchar. */
extern "C" void em_mot_speed_set(_ENEMY_WORK* self, f32 speed)
{
    self->field_0x7C4 = speed;
    mhchar_motion_speed_set((struct MHchar*)self->char_0x024, self->field_0x7BC * (f32)speed);
}

/* 0x8012F8E4 (0x8): returns the model's base scale. */
extern "C" f32 get_em_base_scale(_ENEMY_WORK* self)
{
    return self->base_scale_0x080;
}

/* 0x8012F8EC (0x8): returns the scale at +0x98. */
extern "C" f32 em_mot_frame_get(_ENEMY_WORK* self)
{
    return self->mot_frame_0x098;
}

/* 0x8012F8F4 (0x8): returns the scale at +0xC8. */
extern "C" f32 em_mot_frame_end_get(_ENEMY_WORK* self)
{
    return self->mot_frame_end_0x0C8;
}

/* 0x8012F8FC (0x8): returns the current motion number. */
u16 em_get_mot_no(_ENEMY_WORK* self)
{
    return self->mot_no_0x074;
}

/* 0x8012F904 (0x18): whether `motion` is the current motion. */
extern "C" u32 em_mot_no_ck(_ENEMY_WORK* self, u32 motion)
{
    return self->mot_no_0x074 == (u16)motion;
}

/* 0x8012F91C (0x10): the MHchar frame test of `motion` before the frame. */
u32 em_frame_check(_ENEMY_WORK* self, u16 motion, f32 lo, f32 hi)
{
    return mhchar_frame_check_c1((struct MHchar*)self->char_0x024, motion, 0);
}

/* 0x8012F92C (0x10): the MHchar frame test of `motion` after the frame. */
u32 em_after_frame_check(_ENEMY_WORK* self, u16 motion, f32 lo, f32 hi)
{
    return mhchar_frame_check_c1((struct MHchar*)self->char_0x024, motion, 1);
}

/* 0x8012F93C (0xC): whether the current motion has ended. */
extern "C" u32 em_mot_end_ck(_ENEMY_WORK* self)
{
    return mhchar_motion_end_ck((struct MHchar*)self->char_0x024, 0);
}

/* 0x8012F948 (0x18): whether the motion frame is past 0. */
extern "C" u32 em_mot_frame_started_ck(_ENEMY_WORK* self)
{
    return self->mot_frame_0x068 > 0.0f;
}

/* 0x8012F960 (0x64): looks `motion` up in the enemy data's motion attribute table; -1 when absent. */
extern "C" s32 em_motion_attr_get(_ENEMY_WORK* self, u32 motion)
{
    EmMotionAttr* attr = get_enemy_data(self)->motion_attrs;

    if (attr != NULL) {
        for (; attr->motion != 0xFFFFFFFF; attr++) {
            if (attr->motion == motion) {
                return attr->value;
            }
        }
    }
    return -1;
}

u32 em_water_check(_ENEMY_WORK* self);

/* 0x8012F9C4 (0x1E8): runs the enemy data's condition gates for `motion`; 0 when one refuses it. */
extern "C" u32 em_motion_cond_ck(_ENEMY_WORK* self, u32 motion)
{
    EmMotionCond* cond = get_enemy_data(self)->motion_conds;
    u32 hit;

    if (cond != NULL) {
        for (; cond->motion != 0xFFFFFFFF; cond++) {
            if (motion != cond->motion) {
                continue;
            }
            hit = 0;
            switch (cond->kind) {
            case 0:
                if (self->field_0x762 != 0) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 1:
                if (em_flags836_ck(self, (u16)cond->arg) == 1) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 3:
                if (self->field_0x1E2 == 2 || em_water_check(self) == 1) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 4:
                if (self->field_0x00A == cond->arg) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 5:
                if (cond->arg == (cond->arg & self->field_0x00A)) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 6:
                if ((u8)em_parts_damage_level_get(self, (u8)cond->arg) >= cond->level) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 7:
                hit = (self->field_0x761 & cond->arg) != 0;
                break;
            }
            switch (cond->mode) {
            case 1:
                if (hit == 0) {
                    return 0;
                }
                break;
            case 0:
                if (hit == 1) {
                    return 0;
                }
                break;
            }
        }
    }
    return 1;
}

/* The 0x84-byte stride of the per-part records the joint visibility test indexes by the motion attribute: only the
 * area byte at +0x48E of the first record is read.
 * size: 0x84 */
struct EmPartAreaView {
    /* +0x00 */ u8 area_no;
    /* +0x01 */ u8 unused_0x01[0x84 - 0x01];
};

/* 0x8012FBAC (0xB4): shows joint `joint` while its part is in the current area and the motion gates admit it. */
extern "C" void em_joint_visibility_update(_ENEMY_WORK* self, u32 joint)
{
    s32 part;
    u8 area;
    u32 visible;

    part = em_motion_attr_get(self, joint);
    area = get_now_areano();
    if ((s8)part != -1 && area != ((EmPartAreaView*)&self->field_0x48E)[(s8)part].area_no) {
        visible = 0;
    } else if ((s8)part == -1 && self->area_no != area) {
        visible = 0;
    } else {
        visible = em_motion_cond_ck(self, joint);
    }
    ((MHchar*)self->char_0x024)->setVisibility(joint, visible);
}

/* 0x8012FC60 (0x64): runs the joint visibility test over every joint of the model. */
extern "C" void em_joint_visibility_update_all(_ENEMY_WORK* self)
{
    u32 count = ((MHchar*)self->char_0x024)->get_joint_num();
    u32 i;

    for (i = 0; i < count; i++) {
        em_joint_visibility_update(self, i);
    }
}

/* 0x8012FCC4 (0x20): sets the +0x1D4 target and the frames to reach it; 0 frames sets it at once. */
extern "C" void em_motion_param_set(_ENEMY_WORK* self, s16 steps, f64 target)
{
    self->param_target_0x1D8 = target;
    self->param_steps_0x1DC = steps;
    if (steps == 0) {
        self->field_0x1D4 = target;
    }
}

/* 0x8012FCE4 (0xBC): steps +0x1D4 one frame towards the target without overshooting it. */
extern "C" void em_motion_param_step(_ENEMY_WORK* self)
{
    if (self->param_steps_0x1DC > 0) {
        if (self->param_steps_0x1DC == 1) {
            em_motion_param_set(self, 0, self->param_target_0x1D8);
        } else {
            f32 target;
            f32 cur;
            f32 diff;

            cur = self->field_0x1D4;
            target = self->param_target_0x1D8;
            diff = target - cur;

            cur += diff / self->param_steps_0x1DC;
            if (diff < 0.0f) {
                if (target - cur > 0.0f) {
                    cur = target;
                }
            } else if (target - cur < 0.0f) {
                cur = target;
            }
            self->field_0x1D4 = cur;
        }
        self->param_steps_0x1DC--;
    }
}

f32 get_em_scale(_ENEMY_WORK* self);
f32 get_em_chg_scale(_ENEMY_WORK* self);
u32 em_water_check(_ENEMY_WORK* self);

/* 0x8012FDA0 (0x9C): holds the position on the floor, lifted by the scaled `lift` in ground mode 1 for move modes
 * 1 and 2; 1 when it clamped. */
extern "C" u32 UpdateValue(_ENEMY_WORK* self, f32 lift)
{
    f32 floor;

    if (self->ground_mode_0x208 == 1) {
        if ((u32)(self->field_0x1E2 - 1) <= 1) {
            floor = self->field_0x20C + lift * get_em_scale(self);
        } else {
            floor = self->field_0x20C;
        }
    } else {
        floor = self->field_0x20C;
    }
    if (self->pos.y <= floor) {
        self->pos.y = floor;
        return 1;
    }
    return 0;
}

extern "C" void em_water_surface_latch(_ENEMY_WORK* self);

/* 0x8012FE3C (0x78): holds the position `depth` (scaled) under the water surface; 1 when it clamped. */
extern "C" u32 em_surface_clamp(_ENEMY_WORK* self, f32 depth)
{
    f32 limit;

    em_water_surface_latch(self);
    limit = self->field_0x210 - depth * get_em_scale(self);
    if (self->pos.y >= limit) {
        self->pos.y = limit;
        return 1;
    }
    return 0;
}

/* 0x8012FEB4 (0x84): holds the position `depth` (scaled) under the +0x240 ceiling while +0x23C is set. */
extern "C" u32 em_ground_snap_offset_ck(_ENEMY_WORK* self, f32 depth)
{
    f32 limit;

    if (self->field_0x23C == 0) {
        return 0;
    }
    limit = self->ceiling_0x240 - depth * get_em_scale(self);
    if (self->pos.y >= limit) {
        self->pos.y = limit;
        return 1;
    }
    return 0;
}

extern "C" f32 em_float_depth_get(_ENEMY_WORK* self);
extern "C" f32 em_dive_depth_get(_ENEMY_WORK* self);
extern "C" f32 em_ceiling_depth_get(_ENEMY_WORK* self);
extern "C" u32 em_ground_snap(_ENEMY_WORK* self);

/* 0x8012FF38 (0xD0): applies the height limits of the current move mode. */
extern "C" void em_height_limit_update(_ENEMY_WORK* self)
{
    switch (self->field_0x1E2) {
    case 0:
    case 3:
    case 4:
        self->pos.y = self->field_0x20C;
        break;
    case 2:
        if (self->no_surface_0x30F == 0) {
            em_surface_clamp(self, em_float_depth_get(self));
        }
        UpdateValue(self, em_dive_depth_get(self));
        break;
    case 1:
        if (self->field_0x1E3 == 2) {
            em_ground_snap(self);
        } else if (self->no_ceiling_0x30E == 0) {
            em_ground_snap_offset_ck(self, em_ceiling_depth_get(self));
        }
        UpdateValue(self, 0.0f);
        break;
    default:
        UpdateValue(self, 0.0f);
        break;
    }
}

/* 0x80130008 (0x80): whether the position is on the floor (lifted by the scaled `lift` in ground mode 1). */
extern "C" u32 em_ground_ck(_ENEMY_WORK* self, f32 lift)
{
    f32 floor;

    if (self->ground_mode_0x208 == 1) {
        floor = self->field_0x20C + lift * get_em_scale(self);
    } else {
        floor = self->field_0x20C;
    }
    if (self->pos.y <= floor) {
        return 1;
    }
    return 0;
}

/* 0x80130088 (0x7C): whether the position is `depth` (scaled) or less under the water surface; 1 out of water. */
extern "C" u32 em_surface_reach_ck(_ENEMY_WORK* self, f32 depth)
{
    if (em_water_check(self) == 0) {
        return 1;
    }
    return self->pos.y >= self->field_0x210 - depth * get_em_chg_scale(self);
}

/* 0x80130104 (0x18): whether the enemy stands in water. */
u32 em_water_check(_ENEMY_WORK* self)
{
    return (self->field_0x228 & 1) != 0;
}

/* 0x8013011C (0x18): whether the enemy stands in magma. */
u32 em_magma_check(_ENEMY_WORK* self)
{
    return (self->field_0x228 & 0x80) != 0;
}

extern "C" u8 em_phase_get(_ENEMY_WORK* self);

/* 0x80130134 (0x108): whether the enemy is in water that counts: a ground or dive move mode, lava water or plain
 * water without bit 5 of +0x1C8, and (for `flag` 0) not in a water area the enemy data excludes. */
extern "C" u32 em_in_water_ck(_ENEMY_WORK* self, u32 flag)
{
    u8* rec;
    u8 sub;

    if ((self->field_0x1E2 == 0 || (u8)(self->field_0x1E2 + 253) <= 1) &&
        ((self->water_area_0x22A & 0x80) || ((self->field_0x1C8 & 0x20) == 0 && (self->field_0x228 & 1)))) {
        rec = get_enemy_data(self)->water_exclude_0x40;
        if ((u8)flag == 0 && rec != NULL) {
            sub = em_phase_get(self);
            for (; rec[0] != 0xFF; rec += 3) {
                if (fn_80125FF0(self->field_0x1E0, rec[0]) == 1 && rec[1] == self->area_no && rec[2] == sub) {
                    return 0;
                }
            }
        }
        return 1;
    }
    return 0;
}

/* 0x8013023C (0xC): returns the water area the enemy stands in. */
extern "C" u8 em_phase_get(_ENEMY_WORK* self)
{
    return self->water_area_0x22A & 0x7F;
}

/* 0x80130248 (0x24): returns the enemy data's fall height. */
extern "C" f32 em_fall_height_get(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->fall_height_0x70;
}

/* 0x8013026C (0x78): returns the enemy data's water depth; 50 for team 0x12 in area 3 of a map of kind 3. */
extern "C" f32 em_float_depth_get(_ENEMY_WORK* self)
{
    f32 depth = get_enemy_data(self)->water_depth_0x74;

    if (self->team == 0x12 && stage_map_kind_get(self->field_0x1E0) == 3 && self->area_no == 3) {
        depth = 50.0f;
    }
    return depth;
}

/* 0x801302E4 (0x24): returns the enemy data's lift height. */
extern "C" f32 em_dive_depth_get(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->lift_height_0x78;
}

/* 0x80130308 (0x24): returns the enemy data's ceiling depth. */
extern "C" f32 em_ceiling_depth_get(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->ceiling_depth_0x7C;
}

/* 0x8013032C (0x24): returns the enemy data's model height. */
extern "C" f32 em_height_base_get(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->model_height_0x80;
}

/* 0x80130350 (0x50): sets the move vector to `vec` scaled by the change scale. */
extern "C" void em_move_vec_set(_ENEMY_WORK* self, nw4r::math::VEC3* vec)
{
    nw4r::math::VEC3 scaled;

    vec3_scale(&scaled, vec, get_em_chg_scale(self));
    copyVec3(&self->move_0x1A0, &scaled);
}

/* 0x801303A0 (0x4C): sets the move vector's y to `y` scaled by the change scale. */
extern "C" void em_fall_speed_set(_ENEMY_WORK* self, f32 y)
{
    self->move_0x1A0.y = y * get_em_chg_scale(self);
}

/* 0x801303EC (0x10): sets the +0x1AC height and flags it set. */
extern "C" void em_height_set(_ENEMY_WORK* self, f32 height)
{
    self->field_0x1AC = height;
    self->height_set_0x1DF = 1;
}

/* 0x801303FC (0xC): adds `delta` to the +0x1AC height. */
extern "C" void em_height_add(_ENEMY_WORK* self, f32 delta)
{
    em_height_set(self, self->field_0x1AC + delta);
}

/* 0x80130408 (0x30): latches the enemy data's fall height. */
extern "C" void em_fall_height_latch(_ENEMY_WORK* self)
{
    self->fall_height_0x368 = em_fall_height_get(self);
}

/* 0x80130438 (0x40): adds the latched fall height (scaled) to the move vector's y. */
extern "C" void em_fall_step(_ENEMY_WORK* self)
{
    self->move_0x1A0.y = self->move_0x1A0.y + self->fall_height_0x368 * get_em_scale(self);
}

extern "C" void em_move_sub_mode_set(_ENEMY_WORK* self, u8 sub);

/* 0x80130478 (0x14C): sets move mode `mode` (0xFE/0xFF: mode 1 with sub mode 1/2), copies its parameter vector and
 * settles the height on a change. */
extern "C" void em_move_mode_set(_ENEMY_WORK* self, u8 mode)
{
    u8 sub = 0;
    f32* param;
    nw4r::math::VEC3 vec;

    if ((u8)mode == 0xFE) {
        mode = 1;
        sub = 1;
    } else if ((u8)mode == 0xFF) {
        mode = 1;
        sub = 2;
    }
    switch ((u8)mode) {
    case 1:
        if (sub == 1) {
            param = get_enemy_data(self)->move_params_0x50 + 6;
        } else {
            param = get_enemy_data(self)->move_params_0x50 + 3;
        }
        break;
    case 2:
        param = get_enemy_data(self)->move_params_0x50 + 3;
        break;
    default:
        param = get_enemy_data(self)->move_params_0x50;
        break;
    }
    copyVec3(&self->move_param_0x21C, setVec3(&vec, param[0], param[1], param[2]));
    em_move_sub_mode_set(self, sub);
    if (self->field_0x1E2 != (u8)mode) {
        switch ((u8)mode) {
        case 0:
            self->pos.y = self->field_0x20C;
            break;
        case 3:
            em_fall_height_latch(self);
            break;
        case 4:
            self->pos.y = self->field_0x20C;
            break;
        }
        self->field_0x1E2 = mode;
    }
}

/* 0x801305C4 (0x78): lifts a grounded enemy by the scaled `lift` and starts move mode 1. */
extern "C" void em_fall_start(_ENEMY_WORK* self, f32 lift)
{
    s32 mode = self->field_0x1E2;

    if (mode == 0 || mode == 3) {
        self->pos.y += lift * get_em_scale(self);
    }
    em_move_mode_set(self, 1);
}

/* 0x8013063C (0x84): lifts a grounded enemy to joint `joint`'s height, stops its vertical motion and starts move
 * mode 1. */
extern "C" void em_joint_drop_start(_ENEMY_WORK* self, u32 joint)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if ((u32)(self->field_0x1E2 - 3) <= 1 || self->field_0x1E2 == 0) {
        get_joint_wpos_em(self, joint, &pos);
        self->pos.y = pos.y;
        self->move_0x1A0.y = 0.0f;
        self->field_0x1AC = 0.0f;
    }
    em_move_mode_set(self, 1);
}

/* 0x801306C0 (0x6C): sets the move sub mode; sub mode 1 copies the dive parameter vector. */
extern "C" void em_move_sub_mode_set(_ENEMY_WORK* self, u8 sub)
{
    nw4r::math::VEC3 vec;
    f32* param;

    if (sub == 1) {
        param = get_enemy_data(self)->move_params_0x50;
        copyVec3(&self->move_param_0x21C, setVec3(&vec, param[6], param[7], param[8]));
    }
    self->field_0x1E3 = sub;
}

extern "C" void em_motion_mode_set(_ENEMY_WORK* self, u8 mode);

/* 0x8013072C (0x34): sets the +0x43B/+0x43C motion kind pair and restarts its frame counter on a change. */
extern "C" void em_motion_kind_set(_ENEMY_WORK* self, u8 kind, u8 sub)
{
    if (self->field_0x43B != kind || self->field_0x43C != sub) {
        self->field_0x43B = kind;
        self->field_0x43C = sub;
        self->field_0x440 = 0;
    }
}

/* 0x80130760 (0x18): counts the motion kind's frames up to 0x7FFF. */
extern "C" void em_motion_kind_tick(_ENEMY_WORK* self)
{
    if (self->field_0x440 < 0x7FFF) {
        self->field_0x440++;
    }
}

/* 0x80130778 (0x2C): sets the +0x43D status and restarts its timer; status 1 clears +0x43A. */
extern "C" void em_status_set(_ENEMY_WORK* self, u8 kind)
{
    if (self->field_0x43D != kind) {
        self->field_0x43D = kind;
        self->status_timer_0x442 = 0;
        if (kind == 1) {
            self->field_0x43A = 0;
        }
    }
}

/* The +0x43D status frame limits of the enemy data (`EnemyData::status_limits_0x58`).
 * size: 0x4 (lower bound) */
struct EmStatusLimits {
    /* +0x0 */ s16 sight_frames;     /* the frames a seen player stays marked (`em_sight_player_set`) */
    /* +0x2 */ s16 status1_frames;   /* the frames status 1 lasts while +0x38F is clear */
};

/* 0x801307A4 (0xA0): runs the +0x43D status down: status 1 ends after the enemy data's frame limit, status 2 once
 * +0x38F clears. */
extern "C" void em_status_update(_ENEMY_WORK* self)
{
    EnemyData* data;

    switch (self->field_0x43D) {
    case 1:
        data = get_enemy_data(self);
        if (self->field_0x38F != 0) {
            self->status_timer_0x442 = 0;
        } else if (++self->status_timer_0x442 > data->status_limits_0x58->status1_frames) {
            em_status_set(self, 0);
        }
        break;
    case 2:
        if (self->field_0x38F == 0) {
            em_status_set(self, 0);
        }
        break;
    }
}

/* 0x80130844 (0x14): clears the anger gauge, the alternate mode and its timer. */
extern "C" void em_anger_clear(_ENEMY_WORK* self)
{
    self->field_0x8A6 = 0;
    self->mode_0x8AA = 0;
    self->field_0x8A8 = 0;
}

/* The enemy data's anger table (`EnemyExtraData::anger`): the gauge limit, the alternate mode's frames and the
 * per-tenth-of-health gain rates.
 * size: 0x38 */
struct EmAngerTable {
    /* +0x00 */ s16 limit;
    /* +0x02 */ s16 alt_frames;
    /* +0x04 */ f32 radius;        /* the effect radius `em_shell_rate_init` sets in the alternate mode */
    /* +0x08 */ f32 shell_scale;   /* scales the shell rate in the alternate mode */
    /* +0x0C */ f32 rate_scale;    /* scales the +0x7B8 rate in the alternate mode */
    /* +0x10 */ f32 rate[10];
};

/* 0x80130858 (0x1B8): adds `amount` (scaled by the health-dependent rate) to the anger gauge; a full gauge raises
 * the anger request unless the alternate mode is running. */
extern "C" void em_anger_add(_ENEMY_WORK* self, s16 amount)
{
    EnemyData* data = get_enemy_data(self);
    EmAngerTable* table;
    s16 limit;
    u8 idx;

    if (self->team == 0x19 && stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 1) {
        return;
    }
    table = data->extra->anger;
    if (table != NULL) {
        limit = table->limit;
        idx = 10.0f * (1.0f - (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4);
        if (idx > 9) {
            idx = 9;
        }
        amount = amount * table->rate[idx];
    } else {
        limit = 600;
    }
    self->field_0x8A6 += amount;
    if (self->field_0x8A6 < 0) {
        self->field_0x8A6 = 0;
    } else if (self->field_0x8A6 > limit) {
        self->field_0x8A6 = limit;
    }
    if (self->field_0x8A6 >= limit) {
        if (self->field_0x8A8 <= 0 && em_alt_mode_ck(self) == 0 && em_eft_active_ck(self) == 0 && self->field_0x89F != 1) {
            fn_8012CE8C(self, 4);
        }
        self->field_0x8A6 = limit;
    }
}

/* 0x80130A10 (0xB0): latches the alternate mode; mode 1 arms its timer from the anger table and raises the
 * +0x44C timer to 900, mode 0 clears the gauge. */
extern "C" void em_alt_mode_set(_ENEMY_WORK* self, u8 value)
{
    EmAngerTable* table;

    if (value != self->mode_0x8AA) {
        self->mode_0x8AA = value;
        switch (value) {
        case 0:
            self->field_0x8A8 = 0;
            self->field_0x8A6 = 0;
            fn_8012CE9C(self, 4);
            break;
        case 1:
            table = get_enemy_data(self)->extra->anger;
            if (table != NULL) {
                self->field_0x8A8 = table->alt_frames;
            } else {
                self->field_0x8A8 = 1800;
            }
            if (self->value_0x44C < 900) {
                self->value_0x44C = 900;
            }
            fn_8012CE9C(self, 4);
            break;
        }
    }
}

/* 0x80130AC0 (0x68): runs the alternate mode's timer down and ends the mode once the enemy is idle. */
extern "C" void em_alt_mode_tick(_ENEMY_WORK* self)
{
    if ((s32)self->mode_0x8AA == 1) {
        if (--self->field_0x8A8 <= 0) {
            self->field_0x8A8 = 0;
            if (em_busy_ck(self) == 1) {
                em_anger_clear(self);
            }
        }
    }
}

/* 0x80130B28 (0x44): fills the stamina to 1000 and returns to motion mode 0. */
extern "C" void em_stamina_reset(_ENEMY_WORK* self)
{
    self->stamina_max_0x8A0 = 1000;
    self->field_0x8A2 = 1000;
    em_motion_mode_set(self, 0);
    self->field_0x8A4 = 0;
}

/* 0x80130B6C (0x170): adds `delta` to the stamina, capped at `cap` (or the current value); running out switches to
 * the tired motion mode 1, a gain leaves it. */
extern "C" void em_stamina_add_capped(_ENEMY_WORK* self, s16 delta, u16 cap)
{
    if ((self->field_0x1C8 & 1) == 0) {
        return;
    }
    if (self->field_0x1C8 & 0x80000) {
        return;
    }
    if (delta < 0 && em_mot_finished_ck(self) == 1) {
        delta = delta * 0.5f;
        if (delta == 0) {
            delta = -1;
        }
    }
    if (self->field_0x8A2 <= -delta) {
        self->field_0x8A2 = 0;
        if (em_eft_active_ck(self) == 0) {
            em_motion_mode_set(self, 1);
        }
    } else {
        if (self->field_0x8A2 > cap) {
            cap = self->field_0x8A2;
        }
        self->field_0x8A2 += (u16)delta;
        if (self->field_0x8A2 > cap) {
            self->field_0x8A2 = cap;
        }
        if (delta > 0) {
            if (em_eft_active_ck(self) == 1) {
                em_motion_mode_set(self, 3);
            } else if (self->field_0x89F == 1) {
                em_motion_mode_set(self, 0);
            }
        }
    }
}

/* 0x80130CDC (0xC): adds `delta` to the stamina, capped at its maximum. */
extern "C" void em_stamina_add(_ENEMY_WORK* self, s16 delta)
{
    em_stamina_add_capped(self, delta, self->stamina_max_0x8A0);
}

/* 0x80130CE8 (0x78): every `period` frames, adds `delta` capped at `rate` times the maximum. */
extern "C" void em_stamina_regen(_ENEMY_WORK* self, s16 delta, s16 period, f32 rate)
{
    if (system_w.field_0x0c % period == 0) {
        em_stamina_add_capped(self, delta, self->stamina_max_0x8A0 * rate);
    }
}

/* 0x80130D60 (0x98): runs the tired timer down in motion mode 2 (twice as fast for team 0x10 in move mode 2) and
 * switches to motion mode 3 once the enemy is idle. */
extern "C" void em_tired_tick(_ENEMY_WORK* self)
{
    if ((s32)self->field_0x89F == 2) {
        if (self->team == 0x10 && self->field_0x1E2 == 2) {
            self->field_0x8A4 -= 2;
        } else {
            self->field_0x8A4 -= 1;
        }
        if (self->field_0x8A4 <= 0) {
            self->field_0x8A4 = 0;
            if (em_busy_ck(self) == 1) {
                em_motion_mode_set(self, 3);
            }
        }
    }
}

/* 0x80130DF8 (0x17C): finds a placed meat in the enemy's area within reach and line of sight and attaches the enemy
 * to it. */
extern "C" ShellNikuEntry* em_niku_search(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;
    nw4r::math::VEC3 hit;
    f32 lift;
    u32 i;
    ShellNikuEntry* niku;

    VEC3_ctor(&from);
    VEC3_ctor(&to);
    VEC3_ctor(&hit);
    if ((u32)(self->field_0x1E2 - 1) <= 1) {
        lift = 0.0f;
    } else {
        lift = 50.0f;
    }
    copyVec3(&from, &self->pos);
    from.y += lift;
    for (i = 0; i < 12; i++) {
        niku = niku_entry_at(i);
        if (niku->state_0x07 == 1 && niku->shell_0x00->area_0x08 == self->area_no &&
            vec3_dist_sq(&self->pos, &niku->shell_0x00->pos_0x18) <= 9000000.0f) {
            copyVec3(&to, &niku->shell_0x00->pos_0x18);
            to.y += lift;
            if (findInterSection(&from, &to, &hit, 1, 0xFFFF, self->area_no, em_hit_mask_get(self), NULL) == 0) {
                niku_enemy_attach(niku, self);
                return niku;
            }
        }
    }
    return NULL;
}

extern "C" u32 em_poison_add(_ENEMY_WORK* self, s16 amount);
extern "C" u32 em_paralysis_add(_ENEMY_WORK* self, s16 amount);
extern "C" u32 em_sleep_add(_ENEMY_WORK* self, s16 amount);

/* 0x80130F74 (0xC0): eats the meat the enemy is attached to: applies the meat's effect and refills the stamina. */
extern "C" void em_bait_react(_ENEMY_WORK* self)
{
    ShellNikuEntry* niku;

    if (self->field_0x380 == 7) {
        niku = niku_find(self->state_0x381, self->field_0x382);
        if (niku != NULL) {
            niku_state_set(niku, 4);
            switch (niku->mode_0x04) {
            case 1:
                em_poison_add(self, 200);
                break;
            case 2:
                em_paralysis_add(self, 200);
                break;
            case 3:
                em_sleep_add(self, 200);
                break;
            }
            em_stamina_add(self, 1000);
        }
    } else {
        em_stamina_add(self, 1000);
    }
}

/* 0x80131034 (0x11C): finds another live enemy of team `team` in the enemy's area that is not in actions
 * 0xB/0xC (or is busy), optionally within 5000 units on the ground plane. */
extern "C" _ENEMY_WORK* em_ally_find(_ENEMY_WORK* self, u32 team, u32 near)
{
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* work = get_move_work_adrs_view3(3);
    s32 i;

    for (i = 0; i < max; i++, work++) {
        if (self->group != i && work->active != 0 && work->team == (u8)team && (work->field_0x1C8 & 8) &&
            (fn_8012E5A8_view1(work) != 0 || (work->action != 0xB && work->action != 0xC)) &&
            work->area_no == self->area_no &&
            ((u8)near == 0 || calcDistanceSqXZ(&self->pos, &work->pos) <= 25000000.0f)) {
            return work;
        }
    }
    return NULL;
}

/* A sphere for the segment test: the centre and the radius.
 * size: 0x10 */
struct EmSightSphere {
    /* +0x00 */ nw4r::math::VEC3 center;
    /* +0x0C */ f32 radius;
};

extern "C" EmSight* em_sight_get(_ENEMY_WORK* self);
void get_joint_wmat_em(_ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out);
extern "C" void em_sight_copy(EmSight* dst, EmSight* src);
extern "C" u32 em_sight_player_ck(_ENEMY_WORK* self, EmSight* sight, nw4r::math::VEC3* eye, nw4r::math::VEC3* target,
                                  u32 angle, f32 dist);
extern "C" u32 em_sight_cone_ck(EmSight* sight, nw4r::math::VEC3* eye, nw4r::math::VEC3* target, u32 angle,
                                f32 reach);
extern "C" void em_sight_player_set(_ENEMY_WORK* self, u8 player);
extern "C" void em_level_raise(_ENEMY_WORK* self, u32 kind);

/* 0x80131150 (0x32C): the per-frame sight test: marks the players the enemy sees (every other frame, by its group's
 * parity) and raises the alert level on a flash it sees. */
extern "C" void em_sight_update(_ENEMY_WORK* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 eye;
    u16 max;
    _PLW* work;
    EmSight* sight;
    u32 angle;
    u16 i;
    SenkoRec* senko;
    s32 n;
    EmSight local;

    MTX34_ctor(&mtx);
    VEC3_ctor(&target);
    VEC3_ctor(&eye);
    max = get_move_work_max(2);
    work = get_move_work_adrs(2);
    self->field_0x38E = 0;
    if ((u8)(self->action + 245) <= 1) {
        return;
    }
    if (em_sleep_ck(self, 0) == 1) {
        return;
    }
    if (self->field_0x1F8 == 1) {
        return;
    }
    sight = em_sight_get(self);
    if (sight->joint == 0xFFFFFFFF) {
        copyVec3(&eye, &self->pos);
        eye.y += 50.0f;
        angle = self->field_0x1C0;
    } else {
        get_joint_wmat_em(self, sight->joint, &mtx);
        eye.x = mtx.m[0][3];
        eye.y = mtx.m[1][3];
        eye.z = mtx.m[2][3];
        angle = calcMtxAng3(&mtx);
    }
    if (self->field_0x916 > 0) {
        return;
    }
    if ((self->field_0x38C & 1) && (system_w.field_0x0c & 1) == (self->group & 1)) {
        for (i = 0; i < max; i++, work++) {
            if (work->slot_active != 0 && em_pl_slot_leaving_ck(work->chunk_ofs) == 0 && em_pl_target_ck(self, work) != 0 &&
                self->frames_0x454[i] >= 0.0f) {
                copyVec3(&target, &work->vec_0x03C);
                target.y += 100.0f;
                if ((self->field_0x1C8 & 0x8000) ||
                    em_sight_player_ck(self, sight, &eye, &target, angle, self->frames_0x454[i]) == 1) {
                    self->field_0x38E |= (u8)(1 << i);
                    em_sight_player_set(self, i);
                }
            }
        }
    }
    if (em_status_bits_ck(self, 4) != 0 && em_busy_ck(self) != 0 && (em_status_bits_ck(self, 8) != 0 || self->field_0x1E2 != 4) &&
        (self->field_0x38C & 2)) {
        senko = emc_work.senko_0x090;
        em_sight_copy(&local, sight);
        local.half_angle = 10012;
        for (n = 0; n < 8; n++, senko++) {
            if (senko->state == 1 && senko->value_0x12 == self->area_no &&
                em_sight_cone_ck(&local, &eye, &senko->pos_0x00, angle, senko->value_0x0C) == 1) {
                em_level_raise(self, 1);
                return;
            }
        }
    }
}

/* 0x8013147C (0x34): copies a sight record field by field. */
extern "C" void em_sight_copy(EmSight* dst, EmSight* src)
{
    dst->joint = src->joint;
    dst->near_reach = src->near_reach;
    dst->far_reach = src->far_reach;
    dst->above = src->above;
    dst->below = src->below;
    dst->half_angle = src->half_angle;
}

/* 0x801314B0 (0x174): whether the enemy sees `target` from `eye`: always within the near reach, otherwise when no
 * smoke sphere crosses the line, the view cone holds it and the stage does not block it. */
extern "C" u32 em_sight_player_ck(_ENEMY_WORK* self, EmSight* sight, nw4r::math::VEC3* eye, nw4r::math::VEC3* target,
                                  u32 angle, f32 dist)
{
    KemuriRec* smoke = emc_work.marker_0x130;
    s32 i;
    PlSegment seg;
    PlSegmentBox box;
    EmSightSphere sphere;
    nw4r::math::VEC3 hit;

    pl_segment_init(&seg);
    pl_segment_box_init(&box);
    eft_vec_ctor(&sphere.center);
    VEC3_ctor(&hit);
    if (dist < sight->near_reach) {
        return 1;
    }
    copyVec3(&seg.start, eye);
    copyVec3(&seg.end, target);
    pl_segment_box_set(&seg, &box);
    for (i = 0; i < 10; i++, smoke++) {
        if (smoke->countdown > 0) {
            copyVec3(&sphere.center, &smoke->pos_0x00);
            sphere.radius = smoke->value_0x0C;
            if ((u8)pl_segment_sphere_hit(&box, &sphere, 0, 0) != 0) {
                return 0;
            }
        }
    }
    if (em_sight_cone_ck(sight, eye, target, angle, sight->far_reach) == 1 &&
        findInterSection(eye, target, &hit, 1, 0xFFFF, self->area_no, 0x10, NULL) == 0) {
        return 1;
    }
    return 0;
}

/* 0x80131624 (0xE8): whether `target` lies in the sight's height window, within `reach` on the ground plane and
 * inside the view cone around `angle`. */
extern "C" u32 em_sight_cone_ck(EmSight* sight, nw4r::math::VEC3* eye, nw4r::math::VEC3* target, u32 angle,
                                f32 reach)
{
    u16 diff;

    if (target->y <= eye->y + sight->above && target->y >= eye->y - sight->below &&
        calcVecDistXZ(eye, target) <= reach) {
        diff = angle - calcVecAng2(eye, target);
        if (diff <= sight->half_angle || diff >= 0x10000 - sight->half_angle) {
            return 1;
        }
    }
    return 0;
}

/* The alert-level frame table of the enemy data (`EnemyExtraData::levels`): the normal, alternate-mode and anger
 * hold frames.
 * size: 0x6 */
struct EmLevelFrames {
    /* +0x0 */ s16 normal;
    /* +0x2 */ s16 alt_mode;
    /* +0x4 */ s16 angry;
};

/* The enemy data's alert-level block; only the frame table is read.
 * size: 0xC (lower bound) */
struct EmLevelBlock {
    /* +0x0 */ EmLevelFrames* recover0;   /* the kind-0 recovery frames `em_recover_timer_set` reads */
    /* +0x4 */ EmLevelFrames* frames;
    /* +0x8 */ EmLevelFrames* recover1;   /* the kind-1 recovery frames */
};

/* 0x8013170C (0x13C): raises the alert level: arms its hold timer from the enemy data (shorter each raise, at least
 * 150 frames) and tells the peers when `kind` is 1 in a network game. */
extern "C" void em_level_raise(_ENEMY_WORK* self, u32 kind)
{
    EmLevelFrames* frames = NULL;
    nw4r::math::VEC3 unused;
    EmLevelBlock* block;

    VEC3_ctor(&unused);
    self->level_kind_0x918 = kind;
    block = get_enemy_data(self)->extra->levels;
    if (block != NULL && block->frames != NULL) {
        frames = block->frames;
    }
    if (frames == NULL) {
        self->field_0x916 = 900;
    } else if ((u32)em_alt_mode_ck(self) == 1) {
        self->field_0x916 = frames->alt_mode;
    } else if ((u32)em_eft_active_ck(self) == 1) {
        self->field_0x916 = frames->angry;
    } else {
        self->field_0x916 = frames->normal;
    }
    self->field_0x916 -= (s16)(self->field_0x919 * 150);
    if (self->field_0x916 < 150) {
        self->field_0x916 = 150;
    }
    if (++self->field_0x919 > 3) {
        self->field_0x919 = 3;
    }
    if (kind == 1 && (self->field_0x1C8 & 8)) {
        em_net_send(self, 5, 1);
    }
}

/* 0x80131848 (0x5C): runs the alert hold timer down; it does not run out while the enemy is busy. */
extern "C" void em_level_tick(_ENEMY_WORK* self)
{
    if (self->field_0x916 > 0) {
        self->field_0x916--;
        if (em_busy_ck(self) == 0 && self->field_0x916 == 0) {
            self->field_0x916 = 1;
        }
    }
}

/* 0x801318A4 (0x6C): marks player `player` seen and arms its sight hold timer from the enemy data. */
extern "C" void em_sight_player_set(_ENEMY_WORK* self, u8 player)
{
    self->field_0x38F |= (u8)(1 << player);
    self->sight_timer_0x444[player] = get_enemy_data(self)->status_limits_0x58->sight_frames;
}

/* 0x80131910 (0x24): whether player `player` is marked seen. */
extern "C" u32 em_sight_player_ck2(_ENEMY_WORK* self, u8 player)
{
    return (self->field_0x38F & (1 << player)) != 0;
}

/* 0x80131934 (0x164): marks player `player` spotted on every enemy that sees it; returns whether the player faces
 * away from the first such enemy. */
extern "C" u32 em_player_spotted_set(u8 player)
{
    _ENEMY_WORK* work = get_move_work_adrs_view3(3);
    _PLW* plws = get_move_work_adrs(2);
    u16 max = get_move_work_max(3);
    u8 bit = 1 << player;
    u32 result = 0;
    _PLW* plw = &plws[player];
    u16 i;

    if (plw->slot_active == 0 || em_pl_slot_leaving_ck(plw->chunk_ofs) != 0) {
        return 0;
    }
    if (Pl_cat_skill_ck(plw, 22) != 0) {
        return 0;
    }
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && (work->field_0x1C8 & 1) && (u32)em_pl_target_ck(work, plw) == 1 &&
            em_sight_player_ck2(work, player) == 1 && (work->spotted_0x438 & bit) == 0) {
            work->spotted_0x438 |= bit;
            if (result != 1) {
                result = (u32)(u16)(calcVecAng2(&plw->vec_0x03C, &work->pos) - plw->field_0x058 + 0x3555) > 0x6AAA;
            }
        }
    }
    return result;
}

/* 0x80131A98 (0x13C): whether a status-1 enemy that sees player `player` lies in front of the player. */
extern "C" u32 em_player_watched_ck(u8 player)
{
    _PLW* plws = get_move_work_adrs(2);
    u16 max = get_move_work_max(3);
    _PLW* plw = &plws[player];
    _ENEMY_WORK* work;
    u16 i;

    if (plw->slot_active == 0 || em_pl_slot_leaving_ck(plw->chunk_ofs) != 0) {
        return 0;
    }
    work = get_move_work_adrs_view3(3);
    for (i = 0; i < max; i++, work++) {
        if (work->active != 0 && (work->team != 0x19 || work->area_no != 1) && (work->field_0x1C8 & 1) &&
            work->field_0x43D == 1 && em_sight_player_ck2(work, player) == 1 && (u32)em_pl_target_ck(work, plw) == 1 &&
            (u32)((u16)(calcVecAng2(&plw->vec_0x03C, &work->pos) - plw->field_0x058) - 0x2000) <= 0xC000) {
            return 1;
        }
    }
    return 0;
}

/* 0x80131BD4 (0xC4): whether the enemy sees any live player it can reach. */
extern "C" u32 em_any_player_seen_ck(_ENEMY_WORK* self)
{
    _PLW* plw = get_move_work_adrs(2);
    u8 i;
    u16 max = get_move_work_max(2);

    for (i = 0; i < max; i++, plw++) {
        if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && em_sight_player_ck2(self, i) == 1 &&
            (u32)em_pl_target_ck(self, plw) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x80131C98 (0xEC): runs the four players' sight hold timers down and clears the expired marks. */
extern "C" void em_sight_tick(_ENEMY_WORK* self)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        self->sight_timer_0x444[i]--;
        if (self->sight_timer_0x444[i] <= 0) {
            self->field_0x38F &= ~(1 << i);
            self->sight_timer_0x444[i] = 0;
        }
    }
}

/* 0x80131D84 (0xC): sets the +0x30C frame flag. */
extern "C" void em_frame_flag_set(_ENEMY_WORK* self)
{
    self->frame_flag_0x30C = 1;
}

/* 0x80131D90 (0xC): sets the +0x30D frame flag. */
extern "C" void em_hide_set(_ENEMY_WORK* self)
{
    self->frame_flag_0x30D = 1;
}

/* 0x80131D9C (0xC): latches the ceiling clamp for the frame. */
extern "C" void em_ceiling_latch(_ENEMY_WORK* self)
{
    self->no_ceiling_0x30E = 1;
}

/* 0x80131DA8 (0xC): latches the water-surface clamp for the frame. */
extern "C" void em_water_surface_latch(_ENEMY_WORK* self)
{
    self->no_surface_0x30F = 1;
}

/* 0x80131DB4 (0xC): arms the one-frame motion timer. */
extern "C" void em_motion_timer_arm(_ENEMY_WORK* self)
{
    self->field_0x818 = 1;
}

/* 0x80131DC0 (0x34): runs the motion timer down unless the enemy is frozen (+0x1E8/+0x1E9). */
extern "C" void em_motion_timer_tick(_ENEMY_WORK* self)
{
    if (self->field_0x1E8 == 1 || self->field_0x1E9 == 1) {
        return;
    }
    if (self->field_0x818 > 0) {
        self->field_0x818--;
    }
}

/* 0x80131DF4 (0xC): arms the first effect timer. */
extern "C" void em_fx_flag_set(_ENEMY_WORK* self)
{
    self->fx_timer_0x8C2 = 1;
}

/* 0x80131E00 (0xC): arms the second effect timer. */
extern "C" void em_fx_flag2_set(_ENEMY_WORK* self)
{
    self->fx_timer_0x8C4 = 1;
}

/* 0x80131E0C (0xC): arms the third effect timer. */
extern "C" void em_fx_flag3_set(_ENEMY_WORK* self)
{
    self->fx_timer_0x8C6 = 1;
}

/* 0x80131E18 (0x5C): runs the three effect timers down unless the enemy is frozen. */
extern "C" void em_fx_timer_tick(_ENEMY_WORK* self)
{
    if (self->field_0x1E8 == 1 || self->field_0x1E9 == 1) {
        return;
    }
    if (self->fx_timer_0x8C2 > 0) {
        self->fx_timer_0x8C2--;
    }
    if (self->fx_timer_0x8C4 > 0) {
        self->fx_timer_0x8C4--;
    }
    if (self->fx_timer_0x8C6 > 0) {
        self->fx_timer_0x8C6--;
    }
}

/* 0x80131E74 (0xC): sets the busy timer to 2 frames. */
extern "C" void em_busy_timer_reset(_ENEMY_WORK* self)
{
    self->field_0x81A = 2;
}

/* 0x80131E80 (0xC): clears the busy timer. */
extern "C" void em_busy_timer_clear(_ENEMY_WORK* self)
{
    self->field_0x81A = 0;
}

/* 0x80131E8C (0x34): runs the busy timer down unless the enemy is frozen. */
extern "C" void em_busy_timer_tick(_ENEMY_WORK* self)
{
    if (self->field_0x1E8 == 1 || self->field_0x1E9 == 1) {
        return;
    }
    if (self->field_0x81A > 0) {
        self->field_0x81A--;
    }
}

/* 0x80131EC0 (0xC): arms the +0x81C timer for 2 frames. */
extern "C" void em_timer_0x81C_arm(_ENEMY_WORK* self)
{
    self->timer_0x81C = 2;
}

/* 0x80131ECC (0x24): runs the +0x81C timer down unless +0x1E9 freezes the enemy. */
extern "C" void em_timer_0x81C_tick(_ENEMY_WORK* self)
{
    if (self->field_0x1E9 != 1) {
        if (self->timer_0x81C > 0) {
            self->timer_0x81C--;
        }
    }
}

/* 0x80131EF0 (0xC): arms the +0x81E timer for 2 frames. */
extern "C" void em_timer_0x81E_arm(_ENEMY_WORK* self)
{
    self->timer_0x81E = 2;
}

/* 0x80131EFC (0x30): runs the +0x81E timer down unless the enemy is frozen. */
extern "C" void em_timer_0x81E_tick(_ENEMY_WORK* self)
{
    if (self->field_0x1E8 != 1 && self->field_0x1E9 != 1) {
        if (self->timer_0x81E > 0) {
            self->timer_0x81E--;
        }
    }
}

/* 0x80131F2C (0x28): sets status bit `bit` for 2 frames. */
extern "C" void em_status_bit_set(_ENEMY_WORK* self, u8 bit)
{
    self->status_bits_0x820 |= (u8)(1 << bit);
    self->status_bits_timer_0x821 = 2;
}

/* 0x80131F54 (0x4C): runs the status-bit timer down and clears the bits when it expires. */
extern "C" void em_status_bit_tick(_ENEMY_WORK* self)
{
    if (self->field_0x1E8 != 1 && self->field_0x1E9 != 1) {
        if (self->status_bits_timer_0x821 > 0) {
            self->status_bits_timer_0x821--;
        }
        if (self->status_bits_timer_0x821 <= 0) {
            self->status_bits_0x820 = 0;
        }
    }
}

/* 0x80131FA0 (0xC4): sets the maximum health to `hp` times the rank's scale (0.55 of it under the two quest
 * flags). */
extern "C" void em_max_hp_set(_ENEMY_WORK* self, s32 hp)
{
    f32 scale = ((EmRankRecord*)em_rank_rec_get(self))->hp_scale;

    self->field_0x7A4 = (s32)(hp * scale);
    if (quest_flag_100000_ck(NULL) == 1 || quest_flag_10_ck(NULL) == 1) {
        self->field_0x7A4 = (s32)((s32)self->field_0x7A4 * 0.55f);
    }
}

/* 0x80132064 (0x40): sets the maximum health from the enemy data's base health. */
extern "C" void em_max_hp_init(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    em_max_hp_set(self, data->extra->base_hp);
}

/* 0x801320A4 (0xB0): sets the two shell rates from the rank record, scaled by the anger table's rates in the
 * alternate mode, and the effect radius. */
extern "C" void em_shell_rate_init(_ENEMY_WORK* self)
{
    EmAngerTable* table = NULL;
    EnemyData* data = get_enemy_data(self);
    EmRankRecord* rank = (EmRankRecord*)em_rank_rec_get(self);

    self->shell_rate_0x7B4 = rank->shell_rate;
    self->field_0x7B8 = rank->rate_0x08;
    if ((u32)em_alt_mode_ck(self) == 1) {
        table = data->extra->anger;
    }
    if (table != NULL) {
        self->shell_rate_0x7B4 *= table->shell_scale;
        self->field_0x7B8 *= table->rate_scale;
        self->field_0x7BC = table->radius;
    } else {
        self->field_0x7BC = 1.0f;
    }
}

/* 0x80132154 (0xC): sets the +0x116 flag. */
extern "C" void em_flag_0x116_set(_ENEMY_WORK* self)
{
    self->flag_0x116 = 1;
}

/* 0x80132160 (0xC): sets the +0x115 flag. */
extern "C" void em_flag_0x115_set(_ENEMY_WORK* self)
{
    self->flag_0x115 = 1;
}

/* 0x8013216C (0xC): sets the +0x8AC flag. */
extern "C" void em_rest_flag_set(_ENEMY_WORK* self)
{
    self->flag_0x8AC = 1;
}

/* 0x80132178 (0xC): clears the +0x8AC flag. */
extern "C" void em_flag_0x8AC_clear(_ENEMY_WORK* self)
{
    self->flag_0x8AC = 0;
}

/* 0x80132184 (0x14): whether the +0x8AC flag is set. */
extern "C" u32 em_flag_0x8AC_ck(_ENEMY_WORK* self)
{
    return self->flag_0x8AC != 0;
}

/* 0x80132198 (0xC): sets the +0x8AD flag. */
extern "C" void em_flag_0x8AD_set(_ENEMY_WORK* self)
{
    self->flag_0x8AD = 1;
}

/* 0x801321A4 (0xC): clears the +0x8AD flag. */
extern "C" void em_flag_0x8AD_clear(_ENEMY_WORK* self)
{
    self->flag_0x8AD = 0;
}

/* 0x801321B0 (0x14): whether the +0x8AD flag is set. */
extern "C" u32 em_flag_0x8AD_ck(_ENEMY_WORK* self)
{
    return self->flag_0x8AD != 0;
}

/* 0x801321C4 (0xC): sets the +0x8AE flag. */
extern "C" void em_guard_set(_ENEMY_WORK* self)
{
    self->flag_0x8AE = 1;
}

/* 0x801321D0 (0xC): clears the +0x8AE flag. */
extern "C" void em_guard_clr(_ENEMY_WORK* self)
{
    self->flag_0x8AE = 0;
}

/* 0x801321DC (0x14): whether the +0x8AE flag is set. */
extern "C" u32 em_flag_0x8AE_ck(_ENEMY_WORK* self)
{
    return self->flag_0x8AE != 0;
}

/* 0x801321F0 (0xC): sets the +0x8AF flag. */
extern "C" void em_flag_0x8AF_set(_ENEMY_WORK* self)
{
    self->flag_0x8AF = 1;
}

/* 0x801321FC (0xC): clears the +0x8AF flag. */
extern "C" void em_flag_0x8AF_clear(_ENEMY_WORK* self)
{
    self->flag_0x8AF = 0;
}

/* 0x80132208 (0x14): whether the +0x8AF flag is set. */
extern "C" u32 em_flag_0x8AF_ck(_ENEMY_WORK* self)
{
    return self->flag_0x8AF != 0;
}

/* 0x8013221C (0x8): regains `amount` health every `period` frames, capped at `rate` of the maximum. */
extern "C" void em_gauge_tick(_ENEMY_WORK* self, s32 amount, s16 period, f32 rate)
{
    em_gauge_period_step(self, amount, period, rate);
}

/* 0x80132224 (0x40): knocks the enemy out: sets the faint flag, ends its status and stops its sounds. */
extern "C" void em_tint_hold_start(_ENEMY_WORK* self)
{
    self->faint_0x8AB = 1;
    em_status_set(self, 0);
    em_se_stop_all(self->field_0x01A);
}

/* 0x80132264 (0xC): clears the faint flag. */
extern "C" void em_tint_hold_end(_ENEMY_WORK* self)
{
    self->faint_0x8AB = 0;
}

/* 0x80132270 (0x14): whether the enemy is fainted. */
extern "C" u32 fn_80132270(_ENEMY_WORK* self)
{
    return self->faint_0x8AB != 0;
}

/* 0x80132284 (0xC): clears every ailment bit. */
extern "C" void em_ailment_clear(_ENEMY_WORK* self)
{
    self->ailment_0x94E = 0;
}

/* 0x80132290 (0x24): sets the ailment bits `bits` and drops them from the immunity mask. */
extern "C" void em_ailment_set(_ENEMY_WORK* self, u8 bits)
{
    self->ailment_0x94E |= bits;
    self->field_0x94F &= ~bits;
}

/* 0x801322B4 (0x18): clears the ailment bits `bits`. */
extern "C" void em_ailment_reset(_ENEMY_WORK* self, s32 bits)
{
    self->ailment_0x94E &= ~bits;
}

/* 0x801322CC (0x1C): whether any of the ailment bits `bits` is set. */
extern "C" u32 em_status_ck(_ENEMY_WORK* self, u8 bits)
{
    return (self->ailment_0x94E & bits) != 0;
}

/* 0x801322E8 (0x18): runs the sleep timer down. */
extern "C" void em_sleep_timer_tick(_ENEMY_WORK* self)
{
    if (self->sleep_timer_0x950 > 0) {
        self->sleep_timer_0x950--;
    }
}

/* 0x80132300 (0xC): clears the +0x94F mask. */
extern "C" void em_ailment_mask_clear(_ENEMY_WORK* self)
{
    self->field_0x94F = 0;
}

/* 0x8013230C (0x10): sets the +0x94F mask bits `bits`. */
extern "C" void em_ailment_mask_set(_ENEMY_WORK* self, u8 bits)
{
    self->field_0x94F |= bits;
}

/* 0x8013231C (0x1C): whether any of the +0x94F mask bits `bits` is set. */
extern "C" u32 em_ailment_mask_ck(_ENEMY_WORK* self, u8 bits)
{
    return (self->field_0x94F & bits) != 0;
}

/* One ailment block of the enemy data (`EnemyExtraData::poison`/`sleep`/`paralysis`): the base limit, the decay
 * period and amount, the duration, the limit's growth per trigger and its cap, and the poison damage and period.
 * size: 0x10 */
struct EmAilmentTable {
    /* +0x0 */ s16 base_limit;
    /* +0x2 */ s16 decay_frames;
    /* +0x4 */ s16 decay_amount;
    /* +0x6 */ s16 duration;
    /* +0x8 */ s16 limit_step;
    /* +0xA */ s16 limit_max;
    /* +0xC */ s16 damage;
    /* +0xE */ s16 damage_period;
};

/* 0x80132338 (0xA0): adds `amount` to an ailment value unless the ailment `bit` is already running; 1 when the
 * value reached `limit`. */
extern "C" u32 em_ailment_add(_ENEMY_WORK* self, s16 amount, EmAilmentTable* table, u8 bit, s16* value, s16* decay,
                              s16 limit)
{
    if ((bit == 0 || em_status_ck(self, bit) == 0) && table != NULL) {
        *value += amount;
        if (amount > 0) {
            *decay = table->decay_frames;
        }
        if (*value >= limit) {
            *value = limit;
            return 1;
        }
    }
    return 0;
}

/* 0x801323D8 (0x5C): decays an ailment value by the table's amount every decay period. */
extern "C" void em_ailment_decay(_ENEMY_WORK* self, EmAilmentTable* table, s16* value, s16* decay)
{
    if (table != NULL && *value > 0) {
        if (--*decay <= 0) {
            *value -= table->decay_amount;
            if (*value < 0) {
                *value = 0;
            }
            *decay = table->decay_frames;
        }
    }
}

/* 0x80132434 (0xAC): after a trigger, counts it (up to 10), grows the limit and clears the value. */
extern "C" void em_ailment_limit_raise(_ENEMY_WORK* self, EmAilmentTable* table, u8* count, s16* limit, s16* value)
{
    get_enemy_data(self);
    if (table != NULL) {
        if (++*count > 10) {
            *count = 10;
        }
        *limit = table->base_limit + table->limit_step * *count;
        if (*limit > table->limit_max) {
            *limit = table->limit_max;
        }
        *value = 0;
    }
}

/* 0x801324E0 (0x64): resets the poison buildup. */
extern "C" void em_poison_init(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    EmAilmentTable* table;

    self->field_0x924 = 0;
    self->field_0x91C = 0;
    self->field_0x926 = 0;
    self->poison_damage_0x928 = 0;
    table = data->extra->poison;
    if (table != NULL) {
        self->poison_limit_0x922 = table->base_limit;
    } else {
        self->poison_limit_0x922 = -1;
    }
}

/* 0x80132544 (0x60): adds `amount` to the poison buildup. */
extern "C" u32 em_poison_add(_ENEMY_WORK* self, s16 amount)
{
    EnemyData* data = get_enemy_data(self);

    return em_ailment_add(self, amount, data->extra->poison, 2, &self->field_0x924,
                          &self->poison_decay_0x92A, self->poison_limit_0x922);
}

/* 0x801325A4 (0x48): decays the poison buildup. */
extern "C" void shuffle1(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    em_ailment_decay(self, data->extra->poison, &self->field_0x924, &self->poison_decay_0x92A);
}

/* 0x801325EC (0x40): starts the poison. */
extern "C" void em_status_start(_ENEMY_WORK* self)
{
    self->field_0x926 = 1;
    em_ailment_set(self, 2);
    self->field_0x924 = 0;
}

/* 0x8013262C (0x68): ends the poison and raises its limit. */
extern "C" void em_status_advance(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->poison;

    em_ailment_reset(self, 2);
    self->field_0x926 = 0;
    em_ailment_limit_raise(self, table, &self->field_0x91C, &self->poison_limit_0x922, &self->field_0x924);
}

/* `EmAilmentFx`/`EmAilmentFxBlock` (the enemy data's ailment effect block) are `enemy/EnemyData.h`'s. */

/* 0x80132694 (0xD4): shows the poison bubbles at the enemy data's joint offset (default: joint 2, 100 up). */
extern "C" void em_poison_fx_set(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    EmAilmentFx* fx;

    VEC3_ctor(&offset);
    if ((u32)em_die_ck(self) != 1) {
        fx = get_enemy_data(self)->ailment_fx_0x9C->poison;
        if (fx != NULL) {
            copyVec3(&offset, setVec3(&a, fx->x, fx->y, fx->z));
            eft013_em_place_c1(self, &offset, fx->joint, 2, fx->scale);
            return;
        }
        copyVec3(&offset, setVec3(&b, 0.0f, 100.0f, 0.0f));
        eft013_em_place_c1(self, &offset, 2, 2, 2.0f);
    }
}

/* 0x80132768 (0xF4): runs the poison: the bubbles every 32 frames, the damage every period, and the end once the
 * duration ran out and the enemy is idle. */
extern "C" void em_poison_update(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->poison;

    self->poison_damage_0x928 = 0;
    if (em_status_ck(self, 2) == 1) {
        if (table != NULL) {
            if ((self->field_0x926 & 0x1F) == 1) {
                em_poison_fx_set(self);
            }
            if (self->field_0x926 % table->damage_period == 1) {
                self->poison_damage_0x928 = table->damage;
            }
            if (++self->field_0x926 > table->duration + 1) {
                if (em_busy_ck(self) == 1) {
                    em_status_advance(self);
                    return;
                }
                self->field_0x926 = table->duration + 1;
            }
        } else {
            em_status_advance(self);
        }
    }
}

/* 0x8013285C (0x5C): resets the sleep buildup. */
extern "C" void em_sleep_init(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    EmAilmentTable* table;

    self->field_0x934 = 0;
    self->sleep_count_0x91E = 0;
    table = data->extra->sleep;
    if (table != NULL) {
        self->sleep_limit_0x932 = table->base_limit;
    } else {
        self->sleep_limit_0x932 = -1;
    }
}

/* 0x801328B8 (0x60): adds `amount` to the sleep buildup. */
extern "C" u32 em_sleep_add(_ENEMY_WORK* self, s16 amount)
{
    EnemyData* data = get_enemy_data(self);

    return em_ailment_add(self, amount, data->extra->sleep, 1, &self->field_0x934,
                          &self->sleep_decay_0x936, self->sleep_limit_0x932);
}

/* 0x80132918 (0x48): decays the sleep buildup. */
extern "C" void shuffle2(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    em_ailment_decay(self, data->extra->sleep, &self->field_0x934, &self->sleep_decay_0x936);
}

/* 0x80132960 (0x6C): puts the enemy to sleep for the table's duration. */
extern "C" void em_sleep_start(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->sleep;

    if (table != NULL) {
        self->sleep_timer_0x950 = table->duration;
        em_ailment_set(self, 1);
        self->field_0x934 = 0;
        em_status_set(self, 0);
        em_se_stop_all(self->field_0x01A);
    }
}

/* 0x801329CC (0x104): shows the sleep effect every 32 frames while the enemy sleeps. */
extern "C" void em_sleep_fx_update(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    EmAilmentFx* fx;

    VEC3_ctor(&offset);
    if ((em_sleep_ck(self, 1) == 1 || (u32)em_act_kind6_ck(self->action, self->state_sub) == 1) &&
        (system_w.field_0x0c & 0x1F) == 0x10) {
        fx = get_enemy_data(self)->ailment_fx_0x9C->sleep;
        if (fx != NULL) {
            copyVec3(&offset, setVec3(&a, fx->x, fx->y, fx->z));
            eft013_em_place_c1(self, &offset, fx->joint, 0xA, fx->scale);
            return;
        }
        copyVec3(&offset, setVec3(&b, 0.0f, 100.0f, 0.0f));
        eft013_em_place_c1(self, &offset, 2, 0xA, 2.0f);
    }
}

/* 0x80132AD0 (0x60): ends the sleep and raises its limit. */
extern "C" void em_sleep_end(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->sleep;

    em_ailment_reset(self, 1);
    em_ailment_limit_raise(self, table, &self->sleep_count_0x91E, &self->sleep_limit_0x932, &self->field_0x934);
}

/* 0x80132B30 (0x5C): resets the paralysis buildup. */
extern "C" void em_paralysis_init(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    EmAilmentTable* table;

    self->field_0x92E = 0;
    self->paralysis_count_0x91D = 0;
    table = data->extra->paralysis;
    if (table != NULL) {
        self->paralysis_limit_0x92C = table->base_limit;
    } else {
        self->paralysis_limit_0x92C = -1;
    }
}

/* 0x80132B8C (0x60): adds `amount` to the paralysis buildup. */
extern "C" u32 em_paralysis_add(_ENEMY_WORK* self, s16 amount)
{
    EnemyData* data = get_enemy_data(self);

    return em_ailment_add(self, amount, data->extra->paralysis, 4, &self->field_0x92E,
                          &self->paralysis_decay_0x930, self->paralysis_limit_0x92C);
}

/* 0x80132BEC (0x48): decays the paralysis buildup. */
extern "C" void shuffle3(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    em_ailment_decay(self, data->extra->paralysis, &self->field_0x92E, &self->paralysis_decay_0x930);
}

/* 0x80132C34 (0x58): paralyses the enemy for the table's duration. */
extern "C" void em_paralysis_start(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->paralysis;

    if (table != NULL) {
        self->sleep_timer_0x950 = table->duration;
        em_ailment_set(self, 4);
        self->field_0x92E = 0;
    }
}

/* 0x80132C8C (0x60): ends the paralysis and raises its limit. */
extern "C" void em_paralysis_end(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->paralysis;

    em_ailment_reset(self, 4);
    em_ailment_limit_raise(self, table, &self->paralysis_count_0x91D, &self->paralysis_limit_0x92C,
                           &self->field_0x92E);
}

extern "C" f32 em_paralysis_fx_scale_get(_ENEMY_WORK* self);

/* 0x80132CEC (0x68): shows the paralysis sparks every 32 frames while the enemy is paralysed. */
extern "C" void em_paralysis_fx_update(_ENEMY_WORK* self)
{
    if ((u32)fn_8012E0D8(self->action, self->state_sub) == 1 && (system_w.field_0x0c & 0x1F) == 0) {
        eft013_spark_set_c1(self, self->joint_0x204, em_paralysis_fx_scale_get(self));
    }
}

/* 0x80132D54 (0xA4): resets the stun buildup; its limit is scaled by the rank. */
extern "C" void em_stun_init(_ENEMY_WORK* self)
{
    EmRankRecord* rank = (EmRankRecord*)em_rank_rec_get(self);
    EnemyData* data = get_enemy_data(self);
    EmAilmentTable* table;

    self->field_0x93E = 0;
    self->stun_count_0x91F = 0;
    table = data->extra->stun;
    if (table != NULL) {
        self->stun_limit_0x940 = table->base_limit;
        self->stun_limit_0x940 = self->stun_limit_0x940 * rank->stun_scale;
    } else {
        self->stun_limit_0x940 = -1;
    }
}

/* 0x80132DF8 (0x60): adds `amount` to the stun buildup. */
extern "C" u32 em_stun_add(_ENEMY_WORK* self, s16 amount)
{
    EnemyData* data = get_enemy_data(self);

    return em_ailment_add(self, amount, data->extra->stun, 0x10, &self->field_0x93E, &self->stun_decay_0x942,
                          self->stun_limit_0x940);
}

/* 0x80132E58 (0x48): decays the stun buildup. */
extern "C" void shuffle4(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    em_ailment_decay(self, data->extra->stun, &self->field_0x93E, &self->stun_decay_0x942);
}

/* 0x80132EA0 (0x58): stuns the enemy for the table's duration. */
extern "C" void em_stun_start(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->stun;

    if (table != NULL) {
        self->sleep_timer_0x950 = table->duration;
        em_ailment_set(self, 0x10);
        self->field_0x93E = 0;
    }
}

/* 0x80132EF8 (0xAC): ends the stun, raises its limit and scales it by the rank. */
extern "C" void em_stun_end(_ENEMY_WORK* self)
{
    EmAilmentTable* table = get_enemy_data(self)->extra->stun;
    EmRankRecord* rank = (EmRankRecord*)em_rank_rec_get(self);

    em_ailment_reset(self, 0x10);
    em_ailment_limit_raise(self, table, &self->stun_count_0x91F, &self->stun_limit_0x940, &self->field_0x93E);
    self->stun_limit_0x940 = self->stun_limit_0x940 * rank->stun_scale;
}

/* 0x80132FA4 (0xC4): shows the stun stars at the enemy data's joint offset (default: joint 2, 100 up). */
extern "C" void em_stun_fx_set(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    EmAilmentFx* fx;

    VEC3_ctor(&offset);
    fx = get_enemy_data(self)->ailment_fx_0x9C->stun;
    if (fx != NULL) {
        copyVec3(&offset, setVec3(&a, fx->x, fx->y, fx->z));
        eft_stun_set(self, fx->joint, &offset, 2, fx->scale);
        return;
    }
    copyVec3(&offset, setVec3(&b, 0.0f, 100.0f, 0.0f));
    eft_stun_set(self, 2, &offset, 2, 2.0f);
}

/* 0x80133068 (0xA4): resets the exhaust buildup; its limit is scaled by the rank. */
extern "C" void em_exhaust_init(_ENEMY_WORK* self)
{
    EmRankRecord* rank = (EmRankRecord*)em_rank_rec_get(self);
    EnemyData* data = get_enemy_data(self);
    EmAilmentTable* table;

    self->field_0x944 = 0;
    self->exhaust_count_0x920 = 0;
    table = data->extra->exhaust;
    if (table != NULL) {
        self->exhaust_limit_0x946 = table->base_limit;
        self->exhaust_limit_0x946 = self->exhaust_limit_0x946 * rank->exhaust_scale;
    } else {
        self->exhaust_limit_0x946 = -1;
    }
}

/* 0x8013310C (0x60): adds `amount` to the exhaust buildup. */
extern "C" u32 em_exhaust_add(_ENEMY_WORK* self, s16 amount)
{
    EnemyData* data = get_enemy_data(self);

    return em_ailment_add(self, amount, data->extra->exhaust, 0, &self->field_0x944, &self->exhaust_decay_0x948,
                          self->exhaust_limit_0x946);
}

/* 0x8013316C (0x48): decays the exhaust buildup. */
extern "C" void shuffle5(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    em_ailment_decay(self, data->extra->exhaust, &self->field_0x944, &self->exhaust_decay_0x948);
}

/* 0x801331B4 (0xA8): after an exhaust trigger, raises its limit and scales it by the rank. */
extern "C" void em_exhaust_end(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    EmRankRecord* rank = (EmRankRecord*)em_rank_rec_get(self);

    self->field_0x944 = 0;
    em_ailment_limit_raise(self, data->extra->exhaust, &self->exhaust_count_0x920, &self->exhaust_limit_0x946,
                           &self->field_0x944);
    self->exhaust_limit_0x946 = self->exhaust_limit_0x946 * rank->exhaust_scale;
}

/* 0x8013325C (0x58): resets the status-8 buildup and takes its limit from the enemy data. */
extern "C" void em_ailment8_init(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);
    s16* limit;

    self->field_0x93A = 0;
    limit = data->extra->ailment8_limit;
    if (limit != NULL) {
        self->field_0x938 = *limit;
    } else {
        self->field_0x938 = -1;
    }
}

/* 0x801332B4 (0x90): adds `amount` to the status-8 buildup; 1 when it reached the limit. */
extern "C" u32 em_ailment8_add(_ENEMY_WORK* self, s16 amount)
{
    if (self->field_0x938 > 0 && em_status_ck(self, 8) == 0) {
        self->field_0x93A += amount;
        if (amount > 0) {
            self->ailment8_decay_0x93C = 300;
        }
        if (self->field_0x93A >= self->field_0x938) {
            self->field_0x93A = self->field_0x938;
            return 1;
        }
    }
    return 0;
}

/* 0x80133344 (0x4C): decays the status-8 buildup by 5 every 300 frames. */
extern "C" void shuffle6(_ENEMY_WORK* self)
{
    if (self->field_0x93A > 0) {
        if (--self->ailment8_decay_0x93C <= 0) {
            self->field_0x93A -= 5;
            if (self->field_0x93A < 0) {
                self->field_0x93A = 0;
            }
            self->ailment8_decay_0x93C = 300;
        }
    }
}

/* 0x80133390 (0x40): starts status 8 for 100 frames. */
extern "C" void em_ailment8_start(_ENEMY_WORK* self)
{
    self->sleep_timer_0x950 = 100;
    em_ailment_set(self, 8);
    self->field_0x93A = 0;
}

/* 0x801333D0 (0x10): ends status 8. */
extern "C" void em_ailment8_end(_ENEMY_WORK* self)
{
    self->field_0x93A = 0;
    em_ailment_reset(self, 8);
}

/* 0x801333E0 (0x34): runs the paralysis and sleep effects. */
extern "C" void em_ailment_fx_update(_ENEMY_WORK* self)
{
    em_paralysis_fx_update(self);
    em_sleep_fx_update(self);
}

/* 0x80133414 (0x54): decays every ailment buildup. */
extern "C" void em_ailment_decay_all(_ENEMY_WORK* self)
{
    shuffle1(self);
    shuffle2(self);
    shuffle3(self);
    shuffle4(self);
    shuffle5(self);
    shuffle6(self);
}

/* 0x80133468 (0x194): arms the +0x952 recovery timer for `kind` 0 or 1 from the alert-level tables (shorter each
 * time, with a floor), counting the triggers up to 20. */
extern "C" void em_recover_timer_set(_ENEMY_WORK* self, u8 kind)
{
    EnemyData* data = get_enemy_data(self);
    EmLevelFrames* frames = NULL;
    u8 count;
    s16 floor;
    s32 step;

    switch (kind) {
    case 0:
        if (data->extra->levels != NULL && data->extra->levels->recover0 != NULL) {
            frames = data->extra->levels->recover0;
        }
        step = 300;
        floor = 240;
        count = self->recover_count_0x91A[0]++;
        if (self->recover_count_0x91A[0] > 20) {
            self->recover_count_0x91A[0] = 20;
        }
        break;
    case 1:
        if (data->extra->levels != NULL && data->extra->levels->recover1 != NULL) {
            frames = data->extra->levels->recover1;
        }
        step = 150;
        floor = 150;
        count = self->recover_count_0x91A[1]++;
        if (self->recover_count_0x91A[1] > 20) {
            self->recover_count_0x91A[1] = 20;
        }
        break;
    default:
        return;
    }
    if (frames == NULL) {
        switch (kind) {
        case 0:
            self->recover_timer_0x952 = 900;
            break;
        case 1:
            self->recover_timer_0x952 = 300;
            break;
        }
    } else if ((u32)em_alt_mode_ck(self) == 1) {
        self->recover_timer_0x952 = frames->alt_mode;
    } else if ((u32)em_eft_active_ck(self) == 1) {
        self->recover_timer_0x952 = frames->angry;
    } else {
        self->recover_timer_0x952 = frames->normal;
    }
    if (self->recover_timer_0x952 - step * count < floor) {
        self->recover_timer_0x952 = floor;
    } else {
        self->recover_timer_0x952 -= (s16)(step * count);
    }
}

/* 0x801335FC (0x18): runs the recovery timer down. */
extern "C" void em_recover_timer_tick(_ENEMY_WORK* self)
{
    if (self->recover_timer_0x952 > 0) {
        self->recover_timer_0x952--;
    }
}

/* 0x80133614 (0x24): marks the enemy (paintball) for 18000 frames, except team 0x28. */
extern "C" void em_net_flag_apply_0(_ENEMY_WORK* self)
{
    if (self->team != 0x28) {
        self->mark_timer_0x810 = 18000;
        self->field_0xAEE |= 1;
    }
}

/* 0x80133638 (0x14): whether the enemy is marked. */
extern "C" u32 em_mark_timer_ck(_ENEMY_WORK* self)
{
    return self->mark_timer_0x810 > 0;
}

/* 0x8013364C (0x54): whether the enemy can be marked: not team 0x28, and not a move-mode-4 enemy below its model
 * height. */
extern "C" u32 em_markable_ck(_ENEMY_WORK* self)
{
    if (self->team == 0x28) {
        return 0;
    }
    if (self->field_0x1E2 == 4 && em_tall_ck(self) == 0) {
        return 0;
    }
    return 1;
}

/* 0x801336A0 (0x18): runs the mark timer down. */
extern "C" void em_mark_timer_tick(_ENEMY_WORK* self)
{
    if (self->mark_timer_0x810 > 0) {
        self->mark_timer_0x810--;
    }
}

/* 0x801336B8 (0x54): arms the +0x94A timer for 2700 frames on a live enemy. */
extern "C" void em_net_flag_apply_1(_ENEMY_WORK* self)
{
    if (self->team != 0x28 && (u32)em_die_ck(self) != 1) {
        self->field_0x94A = 2700;
        self->field_0xAEE |= 2;
    }
}

/* 0x8013370C (0x14): whether the +0x94A timer runs. */
extern "C" u32 em_timer_0x94A_ck(_ENEMY_WORK* self)
{
    return (s16)self->field_0x94A > 0;
}

/* 0x80133720 (0x18): runs the +0x94A timer down. */
extern "C" void em_timer_0x94A_tick(_ENEMY_WORK* self)
{
    s16 timer = self->field_0x94A;

    if (timer > 0) {
        self->field_0x94A = timer - 1;
    }
}

extern "C" u32 em_hit_timer_ck(_ENEMY_WORK* self);

/* 0x80133738 (0x94): arms the 900-frame hit timer with the hit bits `bits` and raises the net flag `flag`. */
extern "C" void em_hit_timer_set(_ENEMY_WORK* self, u8 bits, u8 flag)
{
    if (self->team != 0x28 && (u32)em_die_ck(self) != 1) {
        if (em_hit_timer_ck(self) == 0) {
            self->field_0x202 = 0;
        }
        self->field_0x812 = 900;
        self->field_0x814 = (s16)self->field_0x814 | bits;
        self->field_0xAEE |= flag;
    }
}

/* 0x801337CC (0xC): hit timer with bit 1, net flag 4. */
extern "C" void em_net_flag_apply_2(_ENEMY_WORK* self)
{
    em_hit_timer_set(self, 1, 4);
}

/* 0x801337D8 (0xC): hit timer with bit 2, net flag 0x10. */
extern "C" void em_net_flag_apply_4(_ENEMY_WORK* self)
{
    em_hit_timer_set(self, 2, 0x10);
}

/* 0x801337E4 (0xC): hit timer with bit 4, net flag 0x20. */
extern "C" void em_net_flag_apply_5(_ENEMY_WORK* self)
{
    em_hit_timer_set(self, 4, 0x20);
}

/* 0x801337F0 (0xC): hit timer with bit 8, net flag 0x80. */
extern "C" void em_net_flag_apply_7(_ENEMY_WORK* self)
{
    em_hit_timer_set(self, 8, 0x80);
}

/* 0x801337FC (0x14): whether the hit timer runs. */
extern "C" u32 em_hit_timer_ck(_ENEMY_WORK* self)
{
    return (s16)self->field_0x812 > 0;
}

extern "C" f32 em_hit_fx_scale_get(_ENEMY_WORK* self);

/* 0x80133810 (0xAC): shows the hit effect at the enemy's +0x204 joint (default joint 2). */
extern "C" void em_hit_fx_set(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    if (self->team != 0x28 && (u32)em_die_ck(self) != 1) {
        setVector3(&offset, 0.0f, 0.0f, 0.0f);
        if (self->joint_0x204 != 0xFFFFFFFF) {
            eft013_em_place_c1(self, &offset, self->joint_0x204, 0x12, em_hit_fx_scale_get(self));
            return;
        }
        eft013_em_place_c1(self, &offset, 2, 0x12, em_hit_fx_scale_get(self));
    }
}

/* 0x801338BC (0x6C): runs the hit timer down, showing the effect every 32 frames, and clears the hit bits once it
 * expired. */
extern "C" void em_hit_timer_tick(_ENEMY_WORK* self)
{
    if ((s16)self->field_0x812 > 0) {
        self->field_0x812--;
        if ((system_w.field_0x0c & 0x1F) == 8) {
            em_hit_fx_set(self);
        }
    }
    if ((s16)self->field_0x812 <= 0) {
        self->field_0x814 = 0;
    }
}

/* 0x80133928 (0x60): raises the +0x94C timer to at least 3600 frames on a live enemy. */
extern "C" void em_net_flag_apply_3(_ENEMY_WORK* self)
{
    if (self->team != 0x28 && (u32)em_die_ck(self) != 1) {
        if ((s16)self->field_0x94C < 3600) {
            self->field_0x94C = 3600;
        }
        self->field_0xAEE |= 8;
    }
}

/* 0x80133988 (0x24): sets the +0x94C timer to 7200 frames. */
extern "C" void em_net_flag_apply_6(_ENEMY_WORK* self)
{
    if (self->team != 0x28) {
        self->field_0x94C = 7200;
        self->field_0xAEE |= 0x40;
    }
}

/* 0x801339AC (0x14): whether the +0x94C timer runs. */
extern "C" u32 em_break_state_ck(_ENEMY_WORK* self)
{
    return (s16)self->field_0x94C > 0;
}

/* 0x801339C0 (0x140): shows the +0x94C state's effect: the poison offsets for teams 0x1E/0x1F, the hit effect
 * kind 0xE otherwise. */
extern "C" void em_break_fx_set(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    u32 kind;
    u32 joint;
    f32 scale;
    EmAilmentFx* fx;

    VEC3_ctor(&offset);
    if (self->team != 0x28 && (u32)em_die_ck(self) != 1) {
        if ((u8)(self->team + 226) <= 1) {
            kind = 0x1F;
            fx = get_enemy_data(self)->ailment_fx_0x9C->poison;
            if (fx != NULL) {
                copyVec3(&offset, setVec3(&a, fx->x, fx->y, fx->z));
                joint = fx->joint;
                scale = fx->scale;
            } else {
                copyVec3(&offset, setVec3(&b, 0.0f, 100.0f, 0.0f));
                joint = self->joint_0x204;
                scale = 2.0f;
            }
        } else {
            kind = 0xE;
            copyVec3(&offset, setVec3(&c, 0.0f, 0.0f, 0.0f));
            joint = self->joint_0x204;
            scale = em_hit_fx_scale_get(self);
        }
        if (joint == 0xFFFFFFFF) {
            joint = 2;
        }
        eft013_em_place_c1(self, &offset, joint, kind, scale);
    }
}

/* 0x80133B00 (0x5C): runs the +0x94C timer down, showing its effect every 32 frames. */
extern "C" void em_break_timer_tick(_ENEMY_WORK* self)
{
    if ((s16)self->field_0x94C > 0) {
        if ((system_w.field_0x0c & 0x1F) == 0) {
            em_break_fx_set(self);
        }
        self->field_0x94C = (s16)self->field_0x94C - 1;
    }
}

/* 0x80133B5C (0x48): clears the +0x762 latch and rolls its 60..187-frame timer. */
extern "C" void em_latch_0x762_roll(_ENEMY_WORK* self)
{
    self->field_0x762 = 0;
    self->latch_timer_0x764 = (ran_suu(1) & 0x7F) + 60;
}

/* 0x80133BA4 (0x10): sets the +0x762 latch with the timer `frames`. */
extern "C" void em_latch_0x762_set(_ENEMY_WORK* self, s16 frames)
{
    self->field_0x762 = 1;
    self->latch_timer_0x764 = frames;
}

/* 0x80133BB4 (0xC): sets the idle flag. */
extern "C" void em_finish_flag_set(_ENEMY_WORK* self)
{
    self->idle_flag_0x1F7 = 1;
}

/* 0x80133BC0 (0xC): clears the idle flag. */
extern "C" void em_state_refresh(_ENEMY_WORK* self)
{
    self->idle_flag_0x1F7 = 0;
}

/* 0x80133BCC (0x64): the idle flag of a live, idle enemy outside action 0xD; 0 otherwise. */
extern "C" u8 em_idle_flag_get(_ENEMY_WORK* self)
{
    if ((u32)em_die_ck(self) == 1 || self->action == 0xD) {
        return 0;
    }
    if (em_busy_ck(self) == 1) {
        return self->idle_flag_0x1F7;
    }
    return 0;
}

/* 0x80133C30 (0xC): clears the 3D-approach flag. */
extern "C" void em_approach_3d_clear(_ENEMY_WORK* self)
{
    self->approach_3d_0x785 = 0;
}

/* 0x80133C3C (0xC): sets the 3D-approach flag. */
extern "C" void em_hit_arm_set(_ENEMY_WORK* self)
{
    self->approach_3d_0x785 = 1;
}

/* 0x80133C48 (0x8): returns the 3D-approach flag. */
extern "C" u8 em_approach_3d_get(_ENEMY_WORK* self)
{
    return self->approach_3d_0x785;
}

extern "C" u16 em_angle_approach(u16 target, u16 cur, u16 step);

/* 0x80133C50 (0x78): turns the yaw towards the target position by at most `step`; whether it faces it. */
extern "C" u32 em_turn_to_target(_ENEMY_WORK* self, u32 step)
{
    u16 want;

    em_target_refresh(self);
    want = calcVecAng2(&self->pos, &self->vec_0x36C);
    self->field_0x1C0 = em_angle_approach(want, self->field_0x1C0, step);
    return (u16)self->field_0x1C0 == want;
}

/* 0x80133CC8 (0xE8): turns the pitch and yaw towards the target position by at most `pitch_step`/`yaw_step`;
 * whether it faces it. */
extern "C" u32 em_turn_to_target_3d(_ENEMY_WORK* self, u32 pitch_step, u32 yaw_step)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&dir);
    em_target_refresh(self);
    subVec3(&diff, &self->vec_0x36C, &self->pos);
    copyVec3(&dir, &diff);
    calcVecAngXY(&dir, &pitch, &yaw);
    self->field_0x1BC = em_angle_approach(pitch, self->field_0x1BC, pitch_step);
    self->field_0x1C0 = em_angle_approach(yaw, self->field_0x1C0, yaw_step);
    if ((u16)self->field_0x1BC == pitch && (u16)self->field_0x1C0 == yaw) {
        return 1;
    }
    return 0;
}

/* 0x80133DB0 (0x8C): steps the angle `cur` towards `target` by at most `step` the short way round. */
extern "C" u16 em_angle_approach(u16 target, u16 cur, u16 step)
{
    u16 diff = target - cur;

    if (abs((s16)diff) < step) {
        cur = target;
    } else if (diff < 0x8000) {
        cur = cur + step;
    } else {
        cur = cur - step;
    }
    return cur;
}

/* 0x80133E3C (0x110): turns the yaw by `angle` spread over the motion frames [lo, hi], prorated by the frame
 * window the current step covers. */
extern "C" void em_turn_in_window(_ENEMY_WORK* self, f32 lo, f32 hi, s32 angle)
{
    f32 step = get_em_base_scale(self);
    f32 frame;
    f32 over;

    if (em_frame_check(self, 4, lo, hi + step) == 1) {
        frame = em_mot_frame_get(self);
        over = frame - lo;
        if (over < 0.0f) {
            step += over;
        } else {
            over = frame + step - hi;
            if (over > 0.0f) {
                step -= over;
            }
        }
        self->field_0x1C0 += (s32)(step * ((f32)angle / (hi - lo)));
    }
}

/* 0x80133F4C (0xB8): turns the yaw by twice the per-frame share of `angle` over the motion frames [lo, hi]. */
extern "C" void em_turn_in_window2(_ENEMY_WORK* self, f32 lo, f32 hi, s32 angle)
{
    if (em_frame_check(self, 4, lo, hi) == 1) {
        self->field_0x1C0 += (s32)(2.0f * ((f32)angle / (hi - lo)));
    }
}

/* 0x80134004 (0x110): starts an approach to the target position: the distance to run (3D with flag 2), the
 * fixed yaw with flag 8, and the flags. */
extern "C" void em_approach_start(_ENEMY_WORK* self, f32 speed, u32 flags)
{
    nw4r::math::VEC3 unused;
    f32 extra;

    VEC3_ctor(&unused);
    em_target_refresh(self);
    extra = speed * get_em_chg_scale(self);
    if (((u16)flags & 2) == 2) {
        self->value_0x378 = vec3_distance(&self->pos, &self->vec_0x36C);
        em_hit_arm_set(self);
    } else {
        self->value_0x378 = calcVecDistXZ(&self->pos, &self->vec_0x36C);
    }
    self->value_0x378 += extra;
    if (self->value_0x378 < 0.0f) {
        self->value_0x378 = 0.0f;
        if (((u16)flags & 0x10) == 0) {
            em_flag_0x116_set(self);
        }
    }
    if (((u16)flags & 8) == 8) {
        self->approach_yaw_0x47C = calcVecAng2(&self->pos, &self->vec_0x36C);
    }
    self->approach_flags_0x480 = flags;
    setVector3(&self->pos_offset_0x470, 0.0f, 0.0f, 0.0f);
}

/* 0x80134114 (0x2A4): runs the approach: takes the distance covered along the facing out of the remaining one
 * (1 once it is used up), then steers the yaw, pitch or roll towards the target by `step`. */
extern "C" u32 em_approach_step(_ENEMY_WORK* self, s32 yaw_offset, s32 step)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 d0;
    nw4r::math::VEC3 d1;
    nw4r::math::VEC3 d2;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&dir);
    VEC3_ctor(&move);
    if (self->approach_flags_0x480 & 0x20) {
        setVector3(&dir, 0.0f, 1.0f, 0.0f);
        rotVecZ(&dir, self->field_0x1C4);
    } else {
        setVector3(&dir, 0.0f, 0.0f, 1.0f);
    }
    if ((self->approach_flags_0x480 & 2) == 2) {
        em_hit_arm_set(self);
        rotVecX(&dir, self->field_0x1BC);
    }
    if ((self->approach_flags_0x480 & 8) == 8) {
        rotVecY(&dir, (u16)yaw_offset + self->approach_yaw_0x47C);
    } else {
        rotVecY(&dir, (u16)yaw_offset + self->field_0x1C0);
    }
    copyVec3(&move, &self->pos_offset_0x470);
    self->value_0x378 -= vec3_dot((f32*)&dir, (f32*)&move);
    subVec3(&d0, &self->pos, &self->prev_pos);
    copyVec3(&move, &d0);
    if (self->value_0x378 - vec3_dot((f32*)&dir, (f32*)&move) <= 0.0f) {
        self->value_0x378 = 0.0f;
        return 1;
    }
    if ((self->approach_flags_0x480 & 1) == 0) {
        if (self->approach_flags_0x480 & 0x20) {
            if ((u16)step != 0) {
                subVec3(&d1, &self->vec_0x36C, &self->pos);
                copyVec3(&dir, &d1);
                rotVecY(&dir, -self->field_0x1C0);
                self->field_0x1C4 = em_angle_approach(vec3_angle_xy((f32*)&dir), self->field_0x1C4, step);
            }
        } else {
            if ((self->approach_flags_0x480 & 2) == 2) {
                subVec3(&d2, &self->vec_0x36C, &self->pos);
                copyVec3(&dir, &d2);
                calcVecAngXY(&dir, &pitch, &yaw);
                self->field_0x1BC = em_angle_approach(pitch, self->field_0x1BC, step);
            } else {
                yaw = calcVecAng2(&self->pos, &self->vec_0x36C);
            }
            if ((self->approach_flags_0x480 & 4) != 4) {
                if ((self->approach_flags_0x480 & 8) == 8) {
                    yaw = self->approach_yaw_0x47C;
                }
                yaw -= (u16)yaw_offset;
                self->field_0x1C0 = em_angle_approach(yaw, self->field_0x1C0, step);
            }
        }
    }
    return 0;
}

/* One motion request of a turn table: the motion and the three `em_mot_set_blend` arguments.
 * size: 0x10 */
struct EmMotionReq {
    /* +0x00 */ u16 motion;
    /* +0x02 */ u8 pad_0x02[2];
    /* +0x04 */ u32 start;
    /* +0x08 */ u32 frame;
    /* +0x0C */ u32 loop;
};

/* A turn table: the frame windows, the turn angle, the straight-ahead window and the straight, right and left turn
 * motions.
 * size: 0x40 */
struct EmTurnMotions {
    /* +0x00 */ s32 lo;             /* the turn motion's yaw window, in frames */
    /* +0x04 */ s32 hi;
    /* +0x08 */ s32 straight_end;   /* the straight motion's last turning frame (0: its end) */
    /* +0x0C */ u16 angle;          /* the yaw a turn motion covers */
    /* +0x0E */ u16 window;
    /* +0x10 */ EmMotionReq straight;
    /* +0x20 */ EmMotionReq right;
    /* +0x30 */ EmMotionReq left;
};

/* 0x801343B8 (0xCC): starts the straight motion while `angle` is inside the window, else the left or right turn. */
extern "C" void em_turn_motion_set(_ENEMY_WORK* self, EmTurnMotions* table, s16 angle)
{
    if (table->window != 0 && abs(angle) <= table->window) {
        self->field_0x482 = 1;
        em_mot_set_blend(self, table->straight.motion, table->straight.start, table->straight.frame,
                         table->straight.loop);
        return;
    }
    self->field_0x482 = 0;
    if (angle < 0) {
        em_mot_set_blend(self, table->left.motion, table->left.start, table->left.frame, table->left.loop);
        return;
    }
    em_mot_set_blend(self, table->right.motion, table->right.start, table->right.frame, table->right.loop);
}

/* 0x80134484 (0x150): the yaw a turn motion covers per frame: the straight angle over the straight motion's
 * frames, or the turn angle over the [lo, hi] frame window (the motion's end frame when they are 0). */
extern "C" s32 em_turn_rate_get(_ENEMY_WORK* self, EmTurnMotions* table)
{
    f32 frames;
    f32 per_frame;

    if (self->field_0x482 != 0) {
        if (table->straight_end <= 0) {
            frames = em_mot_frame_end_get(self);
        } else {
            frames = table->straight_end;
        }
        per_frame = frames / get_em_base_scale(self);
        return ef_truncate_float(table->window / per_frame);
    }
    if (table->lo == 0 && table->hi == 0) {
        frames = em_mot_frame_end_get(self);
    } else {
        frames = table->hi;
    }
    frames = frames - table->lo;
    per_frame = frames / get_em_base_scale(self);
    return ef_truncate_float(table->angle / per_frame);
}

/* 0x801345D4 (0x24C): the yaw step of this frame of a turn motion: 0 outside the motion's turn window, else the
 * per-frame rate, capped by the remaining angle prorated over the remaining frames (with flag 1). */
extern "C" s32 em_turn_step_get(_ENEMY_WORK* self, EmTurnMotions* table, u16 angle, u8 flags)
{
    f32 end;
    s32 step;
    f32 left;
    s32 cap;

    if (self->field_0x482 != 0) {
        if (table->straight_end <= 0) {
            end = em_mot_frame_end_get(self);
        } else {
            if (em_frame_check(self, 1, table->straight_end, 0.0f) == 1) {
                return 0;
            }
            end = table->straight_end;
        }
    } else if (table->lo == 0 && table->hi == 0) {
        end = em_mot_frame_end_get(self);
    } else {
        if (em_frame_check(self, 3, table->lo, table->hi) == 0) {
            return 0;
        }
        end = table->hi;
    }
    step = em_turn_rate_get(self, table);
    if (flags & 1) {
        if (flags & 2) {
            left = angle;
        } else if (flags & 4) {
            left = 0x10000 - angle;
        } else {
            left = ef_fabsf((s16)angle);
        }
        end -= em_mot_frame_get(self);
        cap = ef_truncate_float(left / (end / get_em_base_scale(self)));
        if ((u16)cap < (u16)step) {
            step = cap;
        }
    }
    return step;
}

/* 0x80134820 (0xD0): turns `*yaw` by `step` towards the turn's goal, or snaps it there once within `step`. */
extern "C" void em_turn_apply(_ENEMY_WORK* self, u32* yaw, s16 diff, u16 step)
{
    if (abs(diff) < step) {
        *yaw = self->field_0x37C;
        return;
    }
    if (self->turn_flags_0x483 & 2) {
        *yaw += step;
    } else if (self->turn_flags_0x483 & 4) {
        *yaw -= step;
    } else if (diff >= 0) {
        *yaw += step;
    } else {
        *yaw -= step;
    }
}

/* 0x801348F0 (0x74): the roll angle of `target` seen in the enemy's yaw frame. */
extern "C" s32 em_roll_to_get(_ENEMY_WORK* self, nw4r::math::VEC3* target)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;

    VEC3_ctor(&dir);
    subVec3(&diff, target, &self->pos);
    copyVec3(&dir, &diff);
    rotVecY(&dir, (u16)(self->field_0x1C0 * -1));
    return vec3_angle_xy((f32*)&dir);
}

/* 0x80134964 (0x1A8): starts a turn towards the target position (`mode` 1: away from it, 2: to angle 0) plus
 * `offset`: picks the turn motion and takes the first step. */
extern "C" void em_turn_seq_start(_ENEMY_WORK* self, EmTurnMotions* table, u32 mode, u8 flags, u32 offset)
{
    s16 diff;
    s32 step;

    em_target_refresh(self);
    self->field_0x482 = 0;
    self->turn_flags_0x483 = flags;
    if (flags & 0x20) {
        self->field_0x37C = em_roll_to_get(self, &self->vec_0x36C);
    } else {
        self->field_0x37C = calcVecAng2(&self->pos, &self->vec_0x36C);
    }
    switch ((u16)mode) {
    case 1:
        self->field_0x37C += 0x8000;
        break;
    case 2:
        self->field_0x37C = 0;
        break;
    }
    self->field_0x37C += (u16)offset;
    if (self->turn_flags_0x483 & 0x20) {
        diff = self->field_0x37C - self->field_0x1C4;
    } else {
        diff = self->field_0x37C - self->field_0x1C0;
    }
    em_turn_motion_set(self, table, diff);
    step = em_turn_step_get(self, table, diff, self->turn_flags_0x483);
    if (0.0f == table->lo) {
        if (self->turn_flags_0x483 & 0x20) {
            em_turn_apply(self, &self->field_0x1C4, diff, step);
        } else {
            em_turn_apply(self, &self->field_0x1C0, diff, step);
        }
    }
    if (self->field_0x482 != 0 && (self->turn_flags_0x483 & 8) == 0) {
        em_flag_0x116_set(self);
    }
}

/* 0x80134B0C (0x2E8): runs a turn: steps the yaw inside the motion's window and reports 1 once the goal is within
 * a step when the motion ends (or at once with flag 0x40), restarting the turn motion otherwise. */
extern "C" u32 em_turn_seq_step(_ENEMY_WORK* self, EmTurnMotions* table)
{
    u32 done = 0;
    s16 diff;
    s32 step;
    u16 ended;

    if (self->mot_frame_0x068 > 0.0f && mhchar_motion_flag_ck((struct MHchar*)self->char_0x024, 1) == 0) {
        return 0;
    }
    if (self->turn_flags_0x483 & 0x20) {
        diff = self->field_0x37C - self->field_0x1C4;
    } else {
        diff = self->field_0x37C - self->field_0x1C0;
    }
    step = em_turn_step_get(self, table, diff, self->turn_flags_0x483);
    if (self->field_0x482 != 0 || (0.0f == table->lo && 0.0f == table->hi) ||
        em_frame_check(self, 3, table->lo, table->hi) == 1) {
        if (self->turn_flags_0x483 & 0x20) {
            em_turn_apply(self, &self->field_0x1C4, diff, step);
        } else {
            em_turn_apply(self, &self->field_0x1C0, diff, step);
        }
    }
    if (self->field_0x482 == 0) {
        if (self->turn_flags_0x483 & 0x10) {
            ended = 1;
        } else {
            ended = 0;
        }
    } else if (table->straight_end <= 0.0f) {
        if (self->turn_flags_0x483 & 0x10) {
            ended = 1;
        } else {
            ended = 0;
        }
    } else {
        ended = em_frame_check(self, 1, table->straight_end, 0.0f);
    }
    if (em_mot_end_ck(self) == 1 || ended != 0) {
        if (self->turn_flags_0x483 & 0x20) {
            diff = self->field_0x37C - self->field_0x1C4;
        } else {
            diff = self->field_0x37C - self->field_0x1C0;
        }
        if (abs(diff) <= (u16)em_turn_rate_get(self, table)) {
            done = 1;
        } else if (em_mot_end_ck(self) == 1) {
            if (self->turn_flags_0x483 & 0x40) {
                done = 1;
            } else {
                em_turn_motion_set(self, table, diff);
            }
        }
    }
    if (self->field_0x482 != 0 && (self->turn_flags_0x483 & 8) == 0) {
        em_flag_0x116_set(self);
    }
    return done;
}

/* 0x80134DF4 (0x34): starts a lift: clears the move vectors and sets the rise speed 20. */
extern "C" void em_lift_start(_ENEMY_WORK* self)
{
    em_move_vec2_clr(self);
    self->offset_0x30C.vec_0x310.y = 20.0f;
}

extern "C" void em_move_offset_rot_apply(_ENEMY_WORK* self, u32* rot);

/* 0x80134E28 (0x64): moves the lift, slowing the rise to at most 100 once 500 above the floor. */
extern "C" void em_lift_step(_ENEMY_WORK* self)
{
    em_move_offset_rot_apply(self, &self->field_0x1BC);
    if (self->pos.y > 500.0f + self->field_0x20C) {
        if (self->offset_0x30C.vec_0x310.y < 100.0f) {
            self->offset_0x30C.vec_0x310.y += 0.85f;
        }
    }
}

/* 0x80134E8C (0x8C): starts a dive: sets the fall speed -10 and accelerates it until the fall from 1000 above the
 * floor would reach the floor, down to -200. */
extern "C" void em_dive_start(_ENEMY_WORK* self)
{
    f32 height;

    em_move_vec2_clr(self);
    self->offset_0x30C.vec_0x310.y = -10.0f;
    height = self->pos.y - (1000.0f + self->field_0x20C);
    while (height > 0.0f && self->offset_0x30C.vec_0x310.y >= -200.0f) {
        self->offset_0x30C.vec_0x310.y -= 0.85f;
        height += self->offset_0x30C.vec_0x310.y;
    }
}

/* 0x80134F18 (0x58): moves the dive and slows the fall back to -10. */
extern "C" void em_dive_step(_ENEMY_WORK* self)
{
    em_move_offset_rot_apply(self, &self->field_0x1BC);
    if (self->offset_0x30C.vec_0x310.y < -10.0f) {
        self->offset_0x30C.vec_0x310.y += 0.85f;
        if (self->offset_0x30C.vec_0x310.y > -10.0f) {
            self->offset_0x30C.vec_0x310.y = -10.0f;
        }
    }
}

/* A flight table: the glide and flap motions, the two blend motions, and the flight speed's ceiling and floor.
 * size: 0x2C */
struct EmFlyMotions {
    /* +0x00 */ EmMotionReq glide;
    /* +0x10 */ EmMotionReq flap;
    /* +0x20 */ u16 bank_lo;
    /* +0x22 */ u16 bank_hi;
    /* +0x24 */ f32 speed_max;
    /* +0x28 */ f32 speed_min;
};

/* 0x80134F70 (0x90): starts a flight: the flap motion, the start position, cleared vectors and at least the
 * table's minimum speed. */
extern "C" void em_fly_start(_ENEMY_WORK* self, EmFlyMotions* table)
{
    em_mot_set_blend(self, table->flap.motion, table->flap.start, table->flap.frame, table->flap.loop);
    self->fly_state_0x786 = 0;
    copyVec3(&self->fly_start_0x788, &self->pos);
    self->vec_0x31C.z = 0.0f;
    self->vec_0x31C.y = 0.0f;
    self->vec_0x31C.x = 0.0f;
    self->offset_0x30C.vec_0x310.x = 0.0f;
    self->offset_0x30C.vec_0x310.y = 0.0f;
    if (self->offset_0x30C.vec_0x310.z < table->speed_min) {
        self->offset_0x30C.vec_0x310.z = table->speed_min;
    }
    em_mat_blend_reset(self);
}

/* 0x80135000 (0x3E4): steers a flight towards the target: banks into the turn, pitches towards it (unless flag
 * 1), switches between flapping and gliding by the speed; 1 once the target has been passed. */
extern "C" u32 em_fly_step(_ENEMY_WORK* self, u32 flags, EmFlyMotions* table)
{
    nw4r::math::VEC3 to_target;
    nw4r::math::VEC3 to_start;
    nw4r::math::VEC3 d0;
    nw4r::math::VEC3 d1;
    u16 turn;
    s32 rate;
    u16 bank;
    f32 lean;
    u16 roll;
    u16 want;
    u16 motion;

    VEC3_ctor(&to_target);
    VEC3_ctor(&to_start);
    em_target_refresh(self);
    self->field_0x784 = 1;
    subVec3(&d0, &self->vec_0x36C, &self->pos);
    copyVec3(&to_target, &d0);
    switch (self->fly_state_0x786) {
    case 0:
        subVec3(&d1, &self->vec_0x36C, &self->fly_start_0x788);
        copyVec3(&to_start, &d1);
        if (vec3_dot((f32*)&to_target, (f32*)&to_start) < 0.0f) {
            self->fly_state_0x786++;
            turn = 0;
        } else {
            turn = (u16)calcVecAng3(&to_target) - self->field_0x1C0;
        }
        break;
    case 1:
        turn = 0;
        break;
    }
    if (turn != 0) {
        lean = 360.0f * (s16)turn / 65536.0f / 90.0f;
    } else {
        lean = 0.0f;
    }
    em_motion_blend_step(self, lean, 0.05f);
    lean = ef_fabsf(self->frames_0x454[4]);
    if (lean <= 0.1f) {
        rate = 0x80;
        bank = 0;
    } else {
        lean -= 0.1f;
        rate = 128.0f + 1664.0f * lean;
        self->offset_0x30C.vec_0x310.z -= 1.7f * lean;
        bank = 0x3000;
        if ((u16)rate * 10 <= 0x3000) {
            bank = (u16)rate * 10;
        }
        if (self->frames_0x454[4] > 0.0f) {
            bank = bank * -1;
        }
    }
    if ((flags & 1) == 0) {
        em_hit_arm_set(self);
        self->field_0x1BC = em_angle_approach(calcVecAngX(&to_target), self->field_0x1BC, 0x80);
    }
    if (self->frames_0x454[4] < 0.0f) {
        if (turn >= 0x10000 - (u16)rate) {
            self->field_0x1C0 += turn;
        } else {
            self->field_0x1C0 -= (u16)rate;
        }
    } else if (turn <= (u16)rate) {
        self->field_0x1C0 += turn;
    } else {
        self->field_0x1C0 += (u16)rate;
    }
    roll = self->field_0x1C4 + 0x8000;
    want = bank + 0x8000;
    if (roll < want) {
        roll += 0x280;
        if (roll > want) {
            roll = want;
        }
    } else {
        roll -= 0x280;
        if (roll < want) {
            roll = want;
        }
    }
    self->field_0x1C4 = roll - 0x8000;
    motion = table->flap.motion;
    if (motion == em_get_mot_no(self)) {
        if (em_mot_end_ck(self) == 1) {
            em_mot_set_blend(self, table->glide.motion, table->glide.start, table->glide.frame, table->glide.loop);
            em_mat_blend_reset(self);
        }
        if (self->offset_0x30C.vec_0x310.z < table->speed_max) {
            if (flags & 2) {
                self->offset_0x30C.vec_0x310.z += 1.6f;
            } else {
                self->offset_0x30C.vec_0x310.z += 0.8f;
            }
        }
    } else if (self->offset_0x30C.vec_0x310.z <= table->speed_min) {
        em_mot_set_blend(self, motion, table->flap.start, table->flap.frame, table->flap.loop);
        em_mat_blend_reset(self);
    }
    if ((flags & 0x20) == 0 && table->glide.motion == em_get_mot_no(self)) {
        em_mot_layer_blend_step(self, table->bank_lo, table->bank_hi, em_mot_frame_get(self), self->frames_0x454[4]);
    }
    return self->fly_state_0x786 != 0;
}

/* 0x801353E4 (0x14): clears the move vector. */
extern "C" void em_move_vec_clr(_ENEMY_WORK* self)
{
    self->offset_0x30C.vec_0x310.x = 0.0f;
    self->offset_0x30C.vec_0x310.y = 0.0f;
    self->offset_0x30C.vec_0x310.z = 0.0f;
}

/* 0x801353F8 (0x20): clears the move vector and its acceleration. */
extern "C" void em_move_vec2_clr(_ENEMY_WORK* self)
{
    self->offset_0x30C.vec_0x310.x = 0.0f;
    self->offset_0x30C.vec_0x310.y = 0.0f;
    self->offset_0x30C.vec_0x310.z = 0.0f;
    self->vec_0x31C.x = 0.0f;
    self->vec_0x31C.y = 0.0f;
    self->vec_0x31C.z = 0.0f;
}

/* 0x80135418 (0x10): adds the move vector to the position. */
extern "C" void em_move_offset_apply(_ENEMY_WORK* self)
{
    addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
}

/* 0x80135428 (0x40): adds the move vector to the position and the acceleration to the move vector. */
extern "C" void CancelFade(_ENEMY_WORK* self)
{
    addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
    addVec3To(&self->offset_0x30C.vec_0x310, &self->vec_0x31C);
}

/* 0x80135468 (0x44): moves and accelerates, then holds the floor lifted by `lift`. */
extern "C" void em_move_offset_fall_update(_ENEMY_WORK* self, f32 lift)
{
    CancelFade(self);
    UpdateValue(self, lift);
}

/* 0x801354AC (0x8): adds the move vector's y to the +0x1AC height. */
extern "C" void em_height_move_apply(_ENEMY_WORK* self)
{
    em_height_add(self, self->offset_0x30C.vec_0x310.y);
}

/* 0x801354B4 (0x40): adds the move vector's y to the height and the acceleration's y to it. */
extern "C" void em_height_move_step(_ENEMY_WORK* self)
{
    em_height_add(self, self->offset_0x30C.vec_0x310.y);
    self->offset_0x30C.vec_0x310.y += self->vec_0x31C.y;
}

/* 0x801354F4 (0x90): adds the move vector, rotated by the pitch/yaw/roll `rot` (the roll and pitch only in move
 * modes 1, 2 and 4), to the position. */
extern "C" void em_move_offset_rot_apply(_ENEMY_WORK* self, u32* rot)
{
    nw4r::math::VEC3 move;

    VEC3_ctor(&move);
    copyVec3(&move, &self->offset_0x30C.vec_0x310);
    if ((u32)(self->field_0x1E2 - 1) <= 1 || self->field_0x1E2 == 4) {
        rotVecZ(&move, rot[2]);
        rotVecX(&move, rot[0]);
    }
    rotVecY(&move, rot[1]);
    addVec3To(&self->pos, &move);
}

/* 0x80135584 (0x44): moves by the rotated move vector, then holds the floor lifted by `lift`. */
extern "C" void em_move_rot_ground_step(_ENEMY_WORK* self, u32* rot, f32 lift)
{
    em_move_offset_rot_apply(self, rot);
    UpdateValue(self, lift);
}

/* 0x801355C8 (0x38): moves by the rotated move vector and accelerates it. */
extern "C" void em_move_offset_step(_ENEMY_WORK* self, u32* rot)
{
    em_move_offset_rot_apply(self, rot);
    addVec3To(&self->offset_0x30C.vec_0x310, &self->vec_0x31C);
}

/* 0x80135600 (0x44): moves by the rotated move vector, accelerates it and holds the floor lifted by `lift`. */
extern "C" void em_move_offset_step_update(_ENEMY_WORK* self, u32* rot, f32 lift)
{
    em_move_offset_step(self, rot);
    UpdateValue(self, lift);
}

/* 0x80135644 (0x64): evaluates the key curve `keys` at the current motion frame, scaled by the base scale. */
extern "C" f32 em_key_curve_eval(_ENEMY_WORK* self, f32* keys)
{
    f32 scale = get_em_base_scale(self);

    return scale * getKeyData(keys, em_mot_frame_get(self));
}

/* 0x801356A8 (0xA0): the distance to the target on the ground plane over `div`, clamped to [lo, hi]. */
extern "C" f32 em_target_dist_ratio_get(_ENEMY_WORK* self, f32 hi, f32 lo, f32 div)
{
    f32 ratio;

    em_target_refresh(self);
    ratio = calcVecDistXZ(&self->pos, &self->vec_0x36C) / div;
    if (ratio > hi) {
        return hi;
    }
    if (ratio < lo) {
        ratio = lo;
    }
    return ratio;
}

/* 0x80135748 (0x1C): whether any of the part bits `mask` of +0x836 is set. */
extern "C" u32 em_flags836_ck(_ENEMY_WORK* self, u32 mask)
{
    return (self->flags_0x836 & (u16)mask) != 0;
}

/* 0x80135764 (0x7C): offsets the target `in` into `out` by the weave `value` (kind 0: one angle in the high
 * byte; kind 1: the same angle twice from the high nibble), its reach scaled by the low byte over 255. */
extern "C" void em_wave_amp(nw4r::math::VEC3* out, nw4r::math::VEC3* in, u8 kind, u32 value, f32 base)
{
    u16 v = value;
    f32 reach = base * ((s32)(u8)v / 255.0f);

    switch (kind) {
    case 0:
        vec3_polar_offset(out, in, value & 0xFF00, reach);
        break;
    case 1:
        vec3_polar_offset2(out, in, v & 0xF000, v & 0xF000, reach);
        break;
    }
}

/* 0x801357E0 (0x3C): offsets `in` into `out` by `value` at the full reach (kind 0: the whole word as the angle;
 * kind 1: the high byte and the low byte as the two angles). */
extern "C" void em_wave_offset(nw4r::math::VEC3* out, nw4r::math::VEC3* in, u8 kind, u32 value, f32 reach)
{
    switch (kind) {
    case 0:
        vec3_polar_offset(out, in, (u16)value, reach);
        break;
    case 1:
        vec3_polar_offset2(out, in, value & 0xFF00, (u16)((u8)(u16)value << 8), reach);
        break;
    }
}

/* 0x8013581C (0xB4): offsets along the segment `a`..`b` into `out` by the weave `value`: the reach scaled by the
 * low byte, the segment position by the high nibble of the low byte, over 255. */
extern "C" void em_wave_segment(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b, u8 kind, u32 value,
                                f32 base)
{
    u16 v = value;
    f32 reach = base * ((s32)(u8)v / 255.0f);

    switch (kind) {
    case 0:
        vec3_segment_offset(out, a, b, value & 0xFF00, reach, (s32)(u8)(v >> 4) / 255.0f);
        break;
    case 1:
        vec3_segment_offset2(out, a, b, (v << 8) & 0xFF00, value & 0xFF00, reach, (s32)(u8)(v >> 4) / 255.0f);
        break;
    }
}

/* 0x801358D0 (0x60): calls the enemy's hook 9 with `arg`; 0 when it has none. */
extern "C" s32 em_hook9_call(_ENEMY_WORK* self, u8 arg)
{
    s32 (*hook)(_ENEMY_WORK*, u8) = (s32 (*)(_ENEMY_WORK*, u8))em_hook_get(self, 9);

    if (hook != NULL) {
        return hook(self, arg);
    }
    return 0;
}

/* 0x80135930 (0x8): the world matrix of joint `joint`. */
void get_joint_wmat_em(_ENEMY_WORK* self, u32 joint, nw4r::math::MTX34* out)
{
    mhchar_joint_mtx_get((MHchar*)self->char_0x024, joint, out);
}

/* 0x80135938 (0x8): the world position of joint `joint`. */
void get_joint_wpos_em(_ENEMY_WORK* self, u32 joint, Vec3* out)
{
    ((MHchar*)self->char_0x024)->get_joint_wpos(joint, out);
}

/* 0x80135940 (0x10): the model scale (the two scale factors' product). */
f32 get_em_scale(_ENEMY_WORK* self)
{
    return self->field_0x1CC * self->field_0x1D0;
}

/* 0x80135950 (0x48): the model scale over the enemy data's base scale. */
f32 get_em_chg_scale(_ENEMY_WORK* self)
{
    EnemyData* data = get_enemy_data(self);

    return get_em_scale(self) / data->base_scale_0x08;
}

/* 0x80135998 (0x58): the enemy data's base effect scale times the model scale. */
extern "C" f32 em_fx_scale_get(_ENEMY_WORK* self)
{
    f32 scale = get_em_scale(self);

    return scale * get_enemy_data(self)->ailment_fx_0x9C->base_scale;
}

/* 0x801359F0 (0x58): the paralysis spark scale times the model scale. */
extern "C" f32 em_paralysis_fx_scale_get(_ENEMY_WORK* self)
{
    f32 scale = get_em_scale(self);

    return scale * get_enemy_data(self)->ailment_fx_0x9C->paralysis_scale;
}

/* 0x80135A48 (0x58): the hit effect scale times the model scale. */
extern "C" f32 em_hit_fx_scale_get(_ENEMY_WORK* self)
{
    f32 scale = get_em_scale(self);

    return scale * get_enemy_data(self)->ailment_fx_0x9C->hit_scale;
}

/* 0x80135AA0 (0x24): the enemy data's +0xC byte. */
extern "C" u8 em_data_kind_get(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->kind_0x0C;
}

/* 0x80135AC4 (0x24): the enemy data's rank byte. */
extern "C" u32 em_get_rank(_ENEMY_WORK* self)
{
    return get_enemy_data(self)->rank_0x0D;
}

/* 0x80135AE8 (0x74): resets both breakable parts to idle in the current area. */
extern "C" void em_break_parts_reset(_ENEMY_WORK* self)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        self->break_parts_0x488[i].hit_kind = 0;
        self->break_parts_0x488[i].area = self->area_no;
        self->break_parts_0x488[i].step = 0;
        self->break_parts_0x488[i].field_0x01 = 0;
        self->break_parts_0x488[i].field_0x02 = 0;
        self->break_parts_0x488[i].field_0x03 = 0;
        self->break_parts_0x488[i].active = 0;
        self->break_parts_0x488[i].timer = 0;
        self->break_parts_0x488[i].duration = 0;
        self->break_parts_0x488[i].effect = -1;
        self->break_parts_0x488[i].kind = 0;
        self->break_parts_0x488[i].hold = 0;
    }
}

/* 0x80135B5C (0x8): sets the per-motion step byte. */
extern "C" void em_state_byte_set(_ENEMY_WORK* self, u8 state)
{
    self->state = state;
}


/* 0x80135B64 (0x20): clears the part's velocity and acceleration. */
extern "C" void em_break_part_motion_clear(EmBreakPart* part)
{
    f32 zero = lbl_80796C58;

    part->vel.x = zero;
    part->vel.y = zero;
    part->vel.z = zero;
    part->accel.x = zero;
    part->accel.y = zero;
    part->accel.z = zero;
}

/* 0x80135B84 (0x40): moves the part by its velocity, then accelerates the velocity. */
extern "C" void em_break_part_motion_step(EmBreakPart* part)
{
    addVec3To(&part->pos, &part->vel);
    addVec3To(&part->vel, &part->accel);
}

/* 0x80135BC4 (0x98): 1 when a breakable part is active (with `any` 0: only one in the current area). */
extern "C" u32 em_break_part_active_ck(_ENEMY_WORK* self, u8 any)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (any == 0) {
            if (self->break_parts_0x488[i].area == get_now_areano() && self->break_parts_0x488[i].active != 0) {
                return 1;
            }
        } else if (self->break_parts_0x488[i].active != 0) {
            return 1;
        }
    }
    return 0;
}



/* 0x80135C5C (0x1E8): breaks part `index` off (the part's pending hit kind 1/2 marks +0x836): picks its fall kind
 * from `flags` (4: none, 1: lifted) and, unless 4, breaks the model part, swaps its materials and places it above
 * the enemy. */
extern "C" void em_part_hit_set(_ENEMY_WORK* self, u32 index, u32 flags)
{
    LandData land;
    EmBreakPart* part;
    EmBreakPartData* data;
    f32 lift;
    s32 i;

    em_land_data_init(&land);
    part = &self->break_parts_0x488[(u8)index];
    switch (part->hit_kind) {
    default:
        return;
    case 2:
        self->flags_0x836 |= 2;
        break;
    case 1:
        self->flags_0x836 |= 1;
        break;
    }
    if ((u8)flags & 4) {
        part->kind = 2;
    } else if ((u8)flags & 1) {
        part->kind = 1;
    } else if (self->field_0x1E2 == 2) {
        part->kind = 1;
    } else {
        part->kind = 0;
    }
    if (((u8)flags & 4) == 0) {
        data = em_break_part_data_get(self, (u8)index);
        if (data != NULL) {
            em_parts_break(self, data->part);
            if ((s32)part->kind == 0) {
                lift = data->lift;
            } else {
                lift = 0.0f;
            }
            for (i = 0; i < 3; i++) {
                if (data->mats[i] != 0xFFFFFFFF) {
                    if ((s32)part->kind == 0) {
                        mhchar_mat_hide_set((struct MHchar*)self->char_0x024, data->mats[i], 0);
                    } else {
                        mhchar_mat_hide_set((struct MHchar*)self->char_0x024, data->mats[i], 1);
                    }
                }
            }
        } else {
            lift = 0.0f;
        }
        if ((u8)flags & 2) {
            lift += 150.0;
        }
        copyVec3(&part->origin, &self->pos);
        if (part->kind == 1 && part->origin.y > self->field_0x210) {
            part->origin.y = self->field_0x210;
        }
        part->origin.y += lift * get_em_scale(self);
        part->area = self->area_no;
    }
}

/* 0x801360FC (0x24): the node's rest translation. */
extern "C" const f32* em_res_node_trans_get(const ResHandle* node)
{
    return res_node_ref(node)->mTranslate;
}

/* 0x80136120 (0x24): the node's rest rotation. */
extern "C" const f32* em_res_node_rot_get(const ResHandle* node)
{
    return res_node_ref(node)->mRotate;
}

/* 0x80136144 (0x24): the node's rest scale. */
extern "C" const f32* em_res_node_scale_get(const ResHandle* node)
{
    return res_node_ref(node)->mScale;
}

/* 0x80136168 (0xEC): moves the part one step back toward its origin: the remaining offset over a quarter of the
 * duration. */
extern "C" void em_break_part_return_step(EmBreakPart* part)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 spare;
    nw4r::math::VEC3 step;
    nw4r::math::VEC3 scaled;
    nw4r::math::VEC3 back;

    VEC3_ctor(&offset);
    VEC3_ctor(&spare);
    VEC3_ctor(&step);
    offset.x = part->pos.x - part->origin.x;
    offset.y = part->pos.y - part->origin.y;
    offset.z = part->pos.z - part->origin.z;
    vec3_negate(&back, &offset);
    vec3_div(&scaled, &back, 0.25f * part->duration);
    copyVec3(&step, &scaled);
    part->pos.x += step.x;
    part->pos.y += step.y;
    part->pos.z += step.z;
}

/* The spin curves of a breakable part (`EmBreakPartData::curves`): the X/Y/Z key tracks in degrees, and the
 * tracks a lifted part uses instead (NULL: the plain one).
 * size: 0x18 */
struct EmBreakPartCurves {
    /* +0x00 */ f32* spin[3];
    /* +0x0C */ f32* lifted_spin[3];
};

/* 0x80136254 (0x1A4): adds the part's spin at its current frame (the curves in degrees) to the rotation `rot`. */
extern "C" void em_break_part_spin(EmBreakPart* part, EmBreakPartData* data, _CP_VECTOR* rot)
{
    nw4r::math::VEC3 spin;
    f32* x;
    f32* y;
    f32* z;

    VEC3_ctor(&spin);
    if (part->kind == 1) {
        x = data->curves->lifted_spin[0];
        y = data->curves->lifted_spin[1];
        z = data->curves->lifted_spin[2];
    } else {
        x = NULL;
        y = NULL;
        z = NULL;
    }
    if (x == NULL) {
        x = data->curves->spin[0];
    }
    if (y == NULL) {
        y = data->curves->spin[1];
    }
    if (z == NULL) {
        z = data->curves->spin[2];
    }
    spin.x = getKeyData(x, part->timer);
    spin.y = getKeyData(y, part->timer);
    spin.z = getKeyData(z, part->timer);
    rot->x += (u16)(0.5f + 65536.0f * spin.x / 360.0f);
    rot->y += (u16)(0.5f + 65536.0f * spin.y / 360.0f);
    rot->z += (u16)(0.5f + 65536.0f * spin.z / 360.0f);
}

/* 0x801369A0 (0x1B0): on the ground, spawns the ground impact (`kind` 1/2: the shell job's dust, at `offset`
 * rotated by the yaw) and, with `fx` 1, the impact effect. */
extern "C" void em_ground_impact_set(_ENEMY_WORK* self, u8 kind, u8 fx, nw4r::math::VEC3* offset, f32 scale)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 rot;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    f32 size;

    VEC3_ctor(&pos);
    VEC3_ctor(&rot);
    if (self->pos.y <= 900.0f + self->field_0x20C) {
        pos.x = self->pos.x;
        pos.y = self->field_0x20C;
        pos.z = self->pos.z;
        size = scale * get_em_chg_scale(self);
        if (offset != NULL) {
            vec3_scale(&a, offset, get_em_chg_scale(self));
            vec3_scale(&b, &a, scale);
            copyVec3(&rot, &b);
            rotVecY(&rot, self->field_0x1C0);
            addVec3To(&pos, &rot);
        }
        switch (kind) {
        case 1:
            shell_set_func_ptr->method_0x28(self, &pos, 1, (u16)0xFFFF, shell_set_func_ptr, scale);
            break;
        case 2:
            shell_set_func_ptr->method_0x28(self, &pos, 2, (u16)0xFFFF, shell_set_func_ptr, scale);
            size *= 0.5f;
            break;
        }
        if (fx == 1) {
            eft_spawn_type11(self, &pos, 0, size);
        }
    }
}

/* 0x80136B50 (0x84): requests the camera on the enemy, aimed at joint `joint` (0xFFFFFFFF: the enemy itself). */
extern "C" void em_camera_req(_ENEMY_WORK* self, u32 joint, u8 mode)
{
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if (joint != 0xFFFFFFFF) {
        get_joint_wpos_em(self, joint, &pos);
        camera_enemy_focus_pos_set_c1(self, mode, &pos);
        return;
    }
    camera_em_shake_req_c1(self, mode);
}

/* 0x80136BD4 (0x4C): the enemy data's +0x28 word for the +0x38D index; 0 without a table. */
extern "C" u32 em_data_word_get(_ENEMY_WORK* self)
{
    u32* table = get_enemy_data(self)->words_0x28;

    if (table != NULL) {
        return table[self->field_0x38D];
    }
    return 0;
}

/* 0x80136C20 (0xF4): transforms a joint point set into world space into `out`; returns the point count. */
extern "C" u8 em_joint_points_get(_ENEMY_WORK* self, EmJointPoints* set, nw4r::math::VEC3* out)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v;
    nw4r::math::VEC3* points;
    u8 i;

    MTX34_ctor(&mtx);
    if (set == NULL) {
        return 0;
    }
    if (set->joint == 0xFFFFFFFF) {
        return 0;
    }
    points = set->points;
    if (points == NULL) {
        return 0;
    }
    get_joint_wmat_em(self, set->joint, &mtx);
    for (i = 0; i < set->count; i++, out++) {
        copyVec3(out, setVec3(&v, points[i].x, points[i].y, points[i].z));
        mulVecMatAddTrans(out, &mtx);
    }
    return i;
}

/* 0x80136D14 (0xC): requests a one-frame camera shake. */
extern "C" void em_shake_req_set(_ENEMY_WORK* self)
{
    self->shake_timer_0x79C = 1;
}

/* 0x80136D20 (0x18): runs the shake request down. */
extern "C" void em_shake_tick(_ENEMY_WORK* self)
{
    if (self->shake_timer_0x79C > 0) {
        self->shake_timer_0x79C--;
    }
}

/* 0x80136D38 (0x14): whether a shake is requested. */
extern "C" u32 em_eft_hide_ck(_ENEMY_WORK* self)
{
    return self->shake_timer_0x79C > 0;
}

/* 0x80136D4C (0xA8): moves the height towards the target's by at most `step`. */
extern "C" void em_height_follow(_ENEMY_WORK* self, f32 step)
{
    f32 diff = self->vec_0x36C.y - self->pos.y;

    em_target_refresh(self);
    if (ef_fabsf(diff) < step) {
        self->pos.y = self->vec_0x36C.y;
    } else if (diff < 0.0f) {
        self->pos.y -= step;
    } else {
        self->pos.y += step;
    }
}

/* 0x80136DF4 (0x24): holds the position on the +0x240 ceiling while +0x23C is set; whether it did. */
extern "C" u32 em_ground_snap(_ENEMY_WORK* self)
{
    if (self->field_0x23C == 0) {
        return 0;
    }
    self->pos.y = self->ceiling_0x240;
    return 1;
}

/* 0x80136E18 (0x20): clears the +0x750 record. */
extern "C" void em_record_0x750_clear(_ENEMY_WORK* self)
{
    self->field_0x750 = 0;
    self->field_0x754 = 0;
    self->field_0x758 = 0;
    self->field_0x75C = -1;
    self->field_0x760 = 0;
}

/* 0x80137604..0x801376B4 - the state accessors. */

/* Stores the record's byte +0x0D. */
extern "C" void em_quest_element_set(_ENEMY_WORK* self, u8 value)
{
    self->field_0x00D = value;
}

/* The second entry point that stores the same byte. */
extern "C" void em_quest_element_set_large(_ENEMY_WORK* self, u8 value)
{
    self->field_0x00D = value;
}

/* Whether the record is on action 11 with its +0x8D3 sub-state byte set. */
extern "C" u32 fn_80137614(_ENEMY_WORK* self)
{
    if (self->action == 11 && self->field_0x8D3 == 1) {
        return 1;
    }
    return 0;
}

/* Sets the field `fn_80137648` reads back. */
extern "C" void em_roar_latch_set(_ENEMY_WORK* self)
{
    self->field_0x43F = 1;
}

/* Whether the `em_roar_latch_set` field is set. */
extern "C" u32 fn_80137648(_ENEMY_WORK* self)
{
    return self->field_0x43F == 1;
}

/* Kicks `lb_sub0e_send` for the record when `em_busy_ck` says the action is armed. */
extern "C" void em_busy_net_notify(_ENEMY_WORK* self, u32 arg1)
{
    if (em_busy_ck(self) == 1) {
        lb_sub0e_send(my_player_no(), arg1, 1, self->field_0x01A);
    }
}

/* Sets the `flags_0x8B3` bit 1. */
extern "C" void em_act_flag1_set(_ENEMY_WORK* self)
{
    fn_801376DC(self, 1);
}

/* Adds `flag` to the record's `flags_0x8B3` bitmap. */
extern "C" void fn_801376BC(_ENEMY_WORK* self, u8 flag)
{
    if ((self->flags_0x8B3 & flag) == 0) {
        self->flags_0x8B3 |= flag;
    }
}

/* Removes `flag` from the record's `flags_0x8B3` bitmap. */
extern "C" void fn_801376DC(_ENEMY_WORK* self, u8 flag)
{
    if (self->flags_0x8B3 & flag) {
        self->flags_0x8B3 &= ~flag;
    }
}

/* Whether `flag` is set in the record's `flags_0x8B3` bitmap. */
extern "C" u32 fn_80137704(_ENEMY_WORK* self, u8 flag)
{
    return (self->flags_0x8B3 & flag) != 0;
}

/* 0x80137720..0x801378A0 - the motion-mode hook, the move-work picker and the light table. */

/* Latches the record's motion mode and, for mode 2, arms the timer `fn_80126494` returns. */
extern "C" void em_motion_mode_set(_ENEMY_WORK* self, u8 mode)
{
    if ((self->field_0x1C8 & 1) == 0 || (self->field_0x1C8 & 0x80000) != 0) {
        self->field_0x89F = 0;
    }
    self->field_0x89F = mode;
    switch (mode) {
    case 2: {
        s16* timer = fn_80126494(self);

        fn_801376BC(self, 1);
        if (timer != NULL) {
            self->field_0x8A4 = *timer;
        } else {
            self->field_0x8A4 = 3600;
        }
        break;
    }
    case 0:
        em_act_flag1_set(self);
        break;
    }
}

/* The move-work record `mode` picks: the slot holding the first bit of `mask` when `mode` is 10, the
 * slot `mask` names otherwise. */
extern "C" u8* em_move_work_pick(u8 mode, u8 mask)
{
    u8* work = (u8*)get_move_work_adrs_view5(2);

    if (mask != 0xFF) {
        u16 max = get_move_work_max(2);
        u8 slot;

        if (mode == 10) {
            u8 i;

            slot = 0xFF;
            for (i = 0; i < max; i++) {
                if (mask & (1 << i)) {
                    slot = i;
                    break;
                }
            }
        } else {
            slot = mask;
        }
        if (slot < max) {
            work += slot * 2848;
        }
    }
    return work;
}

/* Rebuilds the record's four light records. */
extern "C" void fn_801378A0(void)
{
    setVec3(&lbl_806A4560[0], lbl_80796D34, lbl_80796D38, lbl_80796C58);
    setVec3(&lbl_806A4560[1], lbl_80796C58, lbl_80796D38, lbl_80796D3C);
    setVec3(&lbl_806A4560[2], lbl_80796D3C, lbl_80796D38, lbl_80796C58);
    setVec3(&lbl_806A4560[3], lbl_80796C58, lbl_80796D38, lbl_80796D34);
}
