/*
 * lobby/lb_npc.cpp - the lobby NPC group: the NPC tables and motion lookups, the NPC work state machines, the
 *   lobby player's act layer and control, the arm-wrestle and shop screens and the lobby screen dispatcher.
 * RANGE. .text 0x801FBF78-0x80212810 (389 functions); .ctors 0x8056F35C-0x8056F360 (`lb_npc_static_init`), .data
 *   0x805B8D44-0x805B97BC, .bss 0x806A7B90-0x806AA790 (`lb_npc`, the 0x12 x 0x268 NPC work array), .sdata
 *   0x80791D50-0x80791DC0, .sbss 0x80794AB0-0x80794B08, .sdata2 0x80799880-0x80799C18, extab, extabindex.  Four bands,
 *   each keeping its own declarations in a namespace: the NPC tables and motion lookups 0x801FBF78-0x802029B4; the NPC
 *   work band 0x802029B4-0x802076D4; the act layer 0x802076D4-0x8020C588; the control band 0x8020C588-0x80212810.
 *   Seam evidence against one TU: the `.sdata2` pool restarts its duplicate values at 0x80799990 and 0x80799B28 (one
 *   pool per TU), `__sinit` (`lb_npc_static_init`) sits mid-range, and the two vtables at 0x805B8D80/0x805B8DA0 are followed
 *   by `jumptable_805B8DC0` (a seam near 0x801FFCA0); not re-drawn here.
 * FLAGS. `cflags_lobby`; `#pragma optimization_level 4` for the whole file (retail lowers a switch range to
 *   `addi` + `cmplwi`, which -O4 reproduces with no row lost); `#pragma peephole off` around `fn_801FC268`, from
 *   `fn_80203114` to the end of the NPC work band, and over the act layer; the control band is under
 *   `#pragma dont_inline on` and `#pragma peephole off`.
 * NAMES. `lb_npc`, `npc_data_town`, `npc_data_village`, `npc_lp_tbl`, `npc_model_*`, `npc_sub_data`,
 *   `npc_event_set_data`, `lb_npc_Get_motion_no`, `lb_npc_area_ck` and `get_talk_npc_data_ptr` are the map's; the dump
 *   gives only placeholders for the rest.
 *   GUESS (from each body and its callers): lb_npc_model_release_all, lb_npc_map_setup, lobby_frame_update,
 *   lb_player_spawn_set, lb_cmd_pressed_ck, lb_cmd_held_ck, lb_cmd_repeat_ck, lobby_cmd_release_ck, lb_cmd_repeat_get,
 *   lobby_cmd_trig_get, lobby_cmd_release_get, lobby_input_locked_ck, lb_yes_no_step, lobby_cmd_toggle_step_raw,
 *   lb_chacha_skill_name_get, lb_chacha_mask_name_get, lb_cat_name_get, lb_pig_name_get, lb_npc_find, lb_npc_event_set,
 *   lb_npc_motion_restart, lb_npc_act_set, lb_player_act_latch, lb_player_angle_to, lb_npc_motion_play, lb_npc_id_ck,
 *   lb_item_db_toggle, weight_table_index, lb_npc_scene_work, lb_npc_view_dist, lb_quest_return_apply,
 *   lb_npc_motion_table_select, lb_npc_dress_apply, lb_npc_gate_place, lb_npc_place_beside_player,
 *   lb_npc_motion_table_start, lb_npc_view_range_ck, lb_npc_on_screen_ck, lb_npc_ground_drop, lb_npc_ground_snap,
 *   lb_npc_model19_variant1_ck, lb_npc_model_inscreen_get, lb_npc_model_height_get, lb_npc_talk_ok_npc23,
 *   lb_npc_npc23_state_save, lb_npc_dress_model18, lb_npc_talk_ok_npc73, lb_npc_talk_ok_npc14,
 *   lb_npc_joint_effect_spawn, lb_npc_motion_effects, lb_npc_dress_refresh_all, lb_npc_area_light_set,
 *   lb_npc_area_light_clear, lb_npc_char_load, lb_npc_char_move, lb_npc_frame, lb_npc_talk_partner_set,
 *   lb_npc_idle_model14, lb_npc_idle_model11, lb_npc_idle_model15, lb_npc_idle_model1, lb_npc_idle_model7,
 *   lb_npc_idle_model2, lb_npc_idle_model3, lb_npc_idle_model13, lb_npc_idle_model4, lb_npc_idle_model4_variant,
 *   lb_npc_idle_model5, lb_npc_idle_npc53, lb_npc_idle_model16, lb_npc_idle_model12, lb_npc_idle_model6,
 *   lb_npc_idle_model20, lb_npc_idle, lb_npc_walk_leg, lb_npc_turn_leg, lb_npc_warp_leg, lb_npc_pig_release,
 *   lb_npc_bench_scene, lb_npc_motion_dispatch, lb_npc_scene_step, lb_npc_model18_hooks, lb_npc_cat_farm_follow,
 *   lb_npc_model12_follow, lb_npc_prop_sync, lb_npc_talk_reaction_start, lb_player_input_hold, lb_player_frame,
 *   lb_player_draw, lb_player_screen_ok_ck, lb_player_latch_release, lb_player_first_frame, lb_player_place_entry,
 *   lb_player_facing_away_ck, lb_player_menu_area_ck, lb_player_menu_area_return, lb_player_update,
 *   lb_player_motion_effects, lb_player_remote_ease, lb_player_turn_step, lb_player_idle_motion_sync, lb_act_idle,
 *   lb_act_bench, lb_act_doze, lb_act_sit, lb_act_seat_turn, lb_act_greet_variant, lb_act_greet, lb_act_pose603,
 *   lb_act_pose_return, lb_spot_find, lb_act_invite_walk, lb_act_invite_pose, lb_act_invite_jump, lb_act_invite_wave,
 *   lb_act_stretch, lb_act_emote_611, lb_act_emote_612, lb_act_emote_616_hold, lb_act_emote_617,
 *   lb_act_emote_613_hold, lb_act_emote_614, lb_act_emote_615_hold, lb_act_emote_626, lb_act_emote_618_hold,
 *   lb_act_emote_625, lb_act_emote_627_hold, lb_act_emote_628, lb_act_gate_demo, lb_act_msg_wait,
 *   lb_act_invite_answer, lb_act_wait_short, lb_act_invite_answer2, lb_act_hold_item, lb_act_put_item, lb_act_ticket,
 *   lb_act_pose512, lb_act_msg_wait2, lb_act_family0, lb_act_stand, lb_act_walk, lb_act_run, lb_act_stop, lb_act_turn,
 *   lb_act_family1, lb_act_armed_reenter, lb_act_armed_slot_get, lb_act_item_leave, lb_act_item_use, lb_act_item_once,
 *   lb_act_item_503, lb_act_item_504, lb_act_item_507, lb_act_item_513, lb_act_item_514, lb_act_item_515,
 *   lb_act_item_515_hold, lb_act_item_518, lb_act_family11, lb_act_carry_stand, lb_act_carry_walk, lb_act_carry_run,
 *   lb_act_carry_stop, lb_act_carry_turn, lb_act_carry_put, lb_act_carry_leave, lb_act_carry_idle, lb_act_family12,
 *   lb_player_spot_walk, lb_player_gate_walk, lb_player_spot_walk_act6, lb_player_npc_talk_start,
 *   lb_player_area_enter, lb_player_trade_notice, lb_player_item_request, lb_player_control, lb_player_remote_follow,
 *   lb_player_walk_hook, lb_player_carry_control, lb_player_carry_hook, lb_player_act_run, lb_player_arm_wrestle_ck,
 *   lb_arm_wrestle_lost_ck, lb_arm_wrestle_open, lb_arm_wrestle_close, lb_arm_wrestle_step, lb_arm_wrestle_draw,
 *   lb_shop_open, lb_shop_close, lb_shop_stock_get, lb_shop_buy_list_build, lb_shop_halfprice_ck, lb_shop_price_get,
 *   lb_shop_sellable_ck, lb_shop_box_upgrade_ck, lb_shop_buy_flags_get, lb_shop_buy_max_get, lb_shop_box_preview_set,
 *   lb_shop_buy_commit, lb_shop_sell_commit, lb_shop_buy_step, lb_shop_sell_list_build, lb_shop_sell_open,
 *   lb_shop_sell_step, lb_shop_buy_open, lb_shop_step, lb_shop_buy_help_draw, lb_shop_sell_help_draw,
 *   lb_shop_banner_draw, lb_shop_buy_list_draw, lb_shop_sell_list_draw, lb_shop_draw, lb_area_light_ids,
 *   lb_slot_act_tbl, lb_item_motion_tbl, lb_item_motion_once_tbl, lb_shop_banner_wide, lb_shop_banner, lb_shop_frame,
 *   lb_shop_buy_frame, lb_shop_buy_cells, lb_shop_sell_frame, lb_shop_sell_cells, lb_shop_pouch_frame,
 *   lb_shop_pouch_cells, lb_shop_choice_sprites, lb_shop_choice_rows, lb_npc_static_init, lb_npc_array_dtor,
 *   lb_npc_dtor, lb_npc_ctor, lb_npc_char_ctor, lb_npc_char_dtor, lb_npc_char_bind, lb_npc_dtor_chain,
 *   LbNpcMatCallback, lb_npc_mat_callback_ctor, lb_npc_mat_callback_base_ctor, lb_npc_mat_callback_dtor,
 *   lb_npc_mat_callback_calc_mat.
 *   Foreign names proposed through the registry (GUESS; the evidence is in `config/RMHE08/name_registry.jsonl`):
 *   the leaf headers this file includes after `Network/network_pat_control.h` and before the class group, and
 *   `npc_model_res_ids`.
 * RESIDUALS. 2 rows unwritten (0x4C bytes): the material hook's constructor 0x801FF938-0x801FF974 and its callback
 *   base's 0x801FF974-0x801FF984, which store the vtables 0x805B8DA0/0x805B8D80 (rule 10: emitted by MWCC from the
 *   `IScnObjCallback` subclass, never stored by hand).  The array's `__sinit` (`lb_npc_static_init`), its ctor/dtor
 *   chain and the hook's destructor and CALC_MAT override are written as free functions with the retail bodies; the
 *   class form needs `_LB_NPC` +0x54 modelled as a `LbNpcChar` member and the `VEC3` constructor calls
 *   (`VEC3_ctor`) as member construction, so the `.ctors` word and both vtables stay unemitted.
 *   57 rows below 100 (`python tools/objdiff/unitscore.py lobby/lb_npc --threshold 100`); the recorded causes:
 *  - `fn_801FDFA4`: retail's first store is `rlwinm r0,r4,0,24,24` and it materialises the `setVector3` pointer before
 *    the constant load; `(u8)(flag & 0x80)` matches the mask but MWCC schedules the `lfs` ahead of the `addi`;
 *  - `lb_npc_motion_play`, `fn_801FE1DC`, `fn_801FE1EC`: retail sets up the tail call with `addi r3,r3,0x58` first;
 *  - `fn_801FC8C8`: retail keeps `lbz; extsb; stb`, MWCC forwards the byte and drops the `extsb`; `fn_801FC810`:
 *    retail bumps the pointer before the counter; `fn_801FC874`: retail keeps a `clrlwi r0,r0,24` ours drops;
 *  - `fn_802050AC`: retail schedules `lwz r4,44(r31)` between the `(s16)(u16)` mask and the signed `/ 5`;
 *  - `lb_npc_map_setup`, `lb_npc_model_release_all`: the loop counter/base registers are coloured differently;
 *  - `fn_801FCB68`: retail loads `npc_lp_tbl` early as an sda21 pointer, ours addresses it with `lis`/`addi`;
 *  - `fn_801FD0F8`: retail calls `fn_80207B28` before setting up the table base, ours after;
 *  - `fn_801FD174`: retail loads its float constant once into f1, ours reloads it, and keeps one more `clrlwi`;
 *  - `fn_801FDFE4`: ours loads a compiler-pooled double where retail uses `lbl_807998D0`;
 *  - `fn_801FE0AC`: retail narrows the motion id (`clrlwi r4,r29,16`) before the compare;
 *  - `lb_npc_motion_restart`, `fn_802076D4`: the float register of a constant (f1/f0, f31/f30);
 *  - `fn_80207938`: retail increments the state byte (`addi r0,rX,1`) where ours stores the constant 1;
 *  - `fn_80208928`, `lb_player_remote_follow`, `lb_arm_wrestle_step`, `lb_shop_box_preview_set`: retail branches to one
 *    shared tail where ours inverts the tests and returns early (the shared tail needs a `goto`, rule 8);
 *  - `lb_player_*`, `lb_act_*`, `lb_shop_*` rows at 89-99: register colouring and the order of independent loads.
 *   flipcheck: `.bss`/`.ctors`/`.sbss`/`.sdata` claimed, not emitted; `.data` and `.sdata2` short of the claim (the
 *   tables and the float pool are written as literals and externs); `.text`/extab/extabindex short of the claim;
 *   `mhchar_construct` (MHchar's compiler-generated constructor in retail) builds its `nw4r::g3d::ResMdl` at +0x114
 *   with a placement `new`, which adds the `__nw__FUlPv` call and its null test retail's member construction lacks;
 *   90 functions are defined out of address order (the `.text` and `extabindex` order differ from the target's);
 *   lb_npc, lb_npc_func, lb_cat_name, lb_pig_name, lb_tr_flag_data, lb_item_get_data, lobby_hidden_flag and the
 *   `.data` tables other units read are not defined here.
 * SHAPES. `__declspec(noinline)` on the helpers retail keeps out of line (`fn_801FDE3C`, `fn_801FE11C`, `fn_801FE130`,
 *   `fn_801FCAC8`, and the eight rows `fn_80203114` dispatches to).  `fn_801FCB68` computes the value before the range
 *   test (`< 0x3E8`).  `fn_801FDE74` writes `if (a <= 0) {clear} else {tick}`.  `fn_801FE1AC`/`lb_npc_motion_play`/
 *   `fn_801FE1DC` take a full-width id narrowed with `(u16)` at the use.  Every `ran_suu` use narrows with `(u16)`.
 *   Negative angles are `(u16)-8192` and the like.  A two-arm chain on one value is a `switch`.  The act handlers step
 *   their state with `self->act_step_0x05++` in the switch arm (retail's `addi` from the switch value), and a byte
 *   parameter is `u32` narrowed with `(u8)`/`(u16)` at the use where retail keeps the `clrlwi`; `s32 id = (u16)x`
 *   where retail compares signed; a one-case `switch` on a byte where retail keeps `cmpwi`.  The order of the
 *   leading local declarations is chosen per function for retail's saved-register colouring, and the player
 *   rows take the rig's first model (`&rig->models_0x004[0].model`) into a local up front.
 */
#pragma optimization_level 4
#include "types.h"
#include "nw4r/math.h"

#include "lobby/lb_npc.h"
#include "pl.h"   /* MHchar (the NPC body at +0x58) and its members */
#include "gx.h"   /* _GXColor */
#include "enemy/lobby_state_block.h"   /* lobby_state_block (owned by enemy/em020_prog.cpp) */

/* The engine-vector to nw4r-vector copy (`vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`); the NPC tables keep their points
 * as engine `Vec`s. */
void vec_to_mh_vec3(VEC3* out, Vec* src);

/* One pig release spot: its centre and radius (a radius of 0 ends a region's list).  size: 0x10 */
typedef struct LbPigSpot {
    /* +0x0 */ Vec pos_0x0;
    /* +0xC */ f32 radius_0xC;
} LbPigSpot;

/* One pig release region: its spots and the release motion table (null spots end an area's list).  size: 0x8 */
typedef struct LbPigRegion {
    /* +0x0 */ LbPigSpot* spots_0x0;
    /* +0x4 */ LbNpcMotionEntry* table_0x4;
} LbPigRegion;

namespace nw4r { namespace g3d { class ScnObj; } }

/* The material hook an NPC's character block carries: in retail an `nw4r::g3d::IScnObjCallback` subclass (its vtable
 * 0x805B8DA0 over the base's 0x805B8D80, both in this unit's `.data`) whose CALC_MAT slot paints the body's
 * materials.  Laid out here without the class until its constructors and vtables can be emitted.  size: 0xC */
typedef struct LbNpcMatCallback {
    /* +0x00 */ u8 pad_0x00[4];      /* the vtable pointer the unwritten constructors store */
    /* +0x04 */ s32 kind_0x04;       /* 6 paints materials 6 and 7 white, any other kind material 5 */
    /* +0x08 */ MHchar* chr_0x08;    /* the body whose materials are painted */
} LbNpcMatCallback;

/* The NPC's character block at `_LB_NPC` +0x54 (the bytes `field_0x054` starts): its owner, the model body and the
 * NPC id.  Only the members this unit's model functions touch are named.  size: 0x178 */
typedef struct LbNpcChar {
    /* +0x000 */ struct _LB_NPC* owner_0x000;
    /* +0x004 */ MHchar body_0x004;
    /* +0x144 */ u8 pad_0x144[0x24];
    /* +0x168 */ s32 npc_id_0x168;
    /* +0x16C */ LbNpcMatCallback callback_0x16C;
} LbNpcChar;
#include "lobby/lobby_w.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Network/network_pat_control.h" /* isNetCityMode (rule 2: its owner's header) */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "g3d/g3d_scnmdl.h"   /* g3d_root_model_bind, pRoot */
#include "g3d/scn_root_get_current_camera.h"
#include "npc_model_res_ids.h"
#include "lobby/lobby_hunter_cards.h"
#include "ef/eft019.h"   /* ef_inst_spawn */
#include "sound/fn_800D7F54.h"   /* sysSE_stop, sysSE_bank32_req */
#include "ef/get_move_work_adrs.h"   /* get_move_work_max */
#include "lobby/lb_spot_tbl.h"
#include "lobby/lb_page_arrow_ids.h"
/* The foreign callees the registry names (one leaf header each, beside its owner unit). */
#include "Pl/coll_wall_slide.h"
#include "Pl/npc_ground_check.h"
#include "Pl/pl_act_status_step.h"
#include "Pl/pl_coll_shape_set.h"
#include "Pl/pl_condition_step.h"
#include "Pl/pl_equip_refresh.h"
#include "Pl/pl_gauge_step_a.h"
#include "Pl/pl_gauge_step_b.h"
#include "Pl/pl_ground_check.h"
#include "Pl/pl_item_page2_ck.h"
#include "Pl/pl_info_text_set.h"
#include "Pl/pl_input_active_ck.h"
#include "Pl/pl_input_clear.h"
#include "Pl/pl_item_pouch_ok_ck.h"
#include "Pl/pl_joint_mtx_get.h"
#include "Pl/pl_model_aux_flag_clear.h"
#include "Pl/pl_model_hide.h"
#include "Pl/pl_model_pos_sync.h"
#include "Pl/pl_model_visible_set.h"
#include "Pl/pl_motion_speed_apply.h"
#include "Pl/pl_net_slot_ready_ck.h"
#include "Pl/pl_push_clear.h"
#include "Pl/pl_se_frame_callback.h"
#include "Pl/pl_skill_update.h"
#include "Pl/pl_skill_update2.h"
#include "ai/u16_add_sat.h"
#include "camera/camera_lobby_mode_req.h"
#include "camera/camera_mode_cur_get.h"
#include "ef/ef_pl_effects_init.h"
#include "ef/eft004_hold_item_spawn.h"
#include "ef/eft_pl_model_slot_bind.h"
#include "ef/system_lobby_flag_get.h"
#include "enemy/lb_screen16_step.h"
#include "enemy/lb_spot_screen_open.h"
#include "g3d/camera_world_to_screen.h"
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMdl */
#include "g3d/g3d_scninline.h"   /* the placement operator new */
#include "hud/cockpit_lobby_reset.h"
#include "light/light_area_release.h"
#include "light/light_area_select.h"
#include "lobby/lb_area_select_screen_step.h"
#include "lobby/lb_area_view_set.h"
#include "lobby/lb_award_check_all.h"
#include "lobby/lb_award_rows_apply.h"
#include "lobby/lb_box_screen_step.h"
#include "lobby/lb_daily_menu_roll.h"
#include "lobby/lb_deli_screen_step.h"
#include "lobby/lb_event_kind_get.h"
#include "lobby/lb_fq_result_items_take.h"
#include "lobby/lb_item_get_screen_step.h"
#include "lobby/lb_npc_talk_flag_ck.h"
#include "lobby/lb_npc_talk_open.h"
#include "lobby/lb_pig_dress_refresh.h"
#include "lobby/lb_pl_se_frame_callback.h"
#include "lobby/lb_place_rec_get.h"
#include "lobby/lb_plsearch_target_get.h"
#include "lobby/lb_price_cell_draw.h"
#include "lobby/lb_progress_rewards_check.h"
#include "lobby/lb_scene_npc_refresh_all.h"
#include "lobby/lb_screen03_step.h"
#include "lobby/lb_box_menu_step.h"
#include "lobby/lb_screen07_step.h"
#include "lobby/lb_screen11_step.h"
#include "lobby/lb_screen13_step.h"
#include "lobby/lb_screen15_step.h"
#include "lobby/lb_screen26_step.h"
#include "lobby/lb_screen_work_busy_ck.h"
#include "lobby/lb_server_select_open.h"
#include "lobby/lb_server_select_screen_step.h"
#include "lobby/lb_spot_cur_get.h"
#include "lobby/lb_talk_screen_open.h"
#include "lobby/lb_target_npc_get.h"
#include "lobby/pl_rig_draw_mode_begin.h"
#include "lobby/pl_rig_draw_mode_end.h"
#include "lobby/userdata_quest_slot_open_ck.h"
#include "lobby/userdata_server_rows_reset.h"
#include "menu/lb_city_area_ck.h"
#include "menu/lb_city_move_confirm_step.h"
#include "menu/lb_city_screen_open.h"
#include "menu/lb_city_select_open.h"
#include "menu/lb_city_select_screen_step.h"
#include "menu/lb_name_entry_screen_step.h"
#include "menu/lb_plsearch_open.h"
#include "menu/lb_plsearch_screen_step.h"
#include "menu/lb_pop_screen_open.h"
#include "menu/lb_quest_accept_screen_step.h"
#include "menu/lb_quest_join_screen_step.h"
#include "menu/lb_screen24_step.h"
#include "menu/lb_screen30_step.h"
#include "menu/lb_screen34_step.h"
#include "sound/bgm_lobby_return_req.h"
#include "sound/mhchar_draw_mode_set.h"
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_model_mtx_update.h"
#include "sound/mhchar_sound_job_request.h"
#include "userdata_equip_item_id_get.h"
#include "userdata_item_new_marks_clear.h"
#include "userdata_item_seen_set.h"
#include "userdata_item_total_count.h"
namespace s_801FBF78 {


/* ---------------------------------------------------------------------------------------------------
 * Data
 */
extern "C" LbNpcUserData* lobby_world_block;  /* .sbss pointer */
extern "C" _LB_NPC lb_npc[0x12];        /* .bss 0x806A7BA0 */
extern "C" f32 lbl_80799880;
extern "C" f32 lbl_80799884;
extern "C" f64 lbl_807998D0;
extern "C" f32 lbl_807998D8;
extern "C" LbNpcSystemWork system_w;   /* .bss 0x806585E0 */
extern "C" u8 lobby_state_block[];
extern "C" u8 lobby_hidden_flag;
extern "C" u8* lobby_wp;
extern "C" void* npc_data_town[];
extern "C" void* npc_data_village[];
extern "C" void* npc_lp_tbl[];
extern "C" LbResId lbl_80582A68[];

/* ---------------------------------------------------------------------------------------------------
 * Declarations: mangled map names are C++ linkage (rule 9), plain fn_/lbl_ names are C.
 */
extern "C" {
s32 userdata_progress_flag_ck(s32);
u32 userdata_flag_ck(s32);
void lb_progress_unlock(u16);
u16 lb_unlock_cond_top_get(s32);
u8 fn_802FB948(void);
void fn_80220078(void);
void fn_801E9AA8(u8);
s32 mhchar_motion_set(void* self, u8 kind, s32 a3, u16 id, s32 a5, s32 motion, s32 a7, f32 c, f32 k);
u32 mhchar_frame_check(void* self, u16 a, s32 b, f32 frame, f32 speed);
u32 mhchar_motion_end_ck(void* self, s32 a);
void mhchar_motion_speed_set(void* self);
s32 fn_8026A00C(u16 id);
s32 fn_802D282C(u16 id);
s32 abs(s32 a);
}
} /* namespace s_801FBF78 */


void wii_sysmsg_gen(s32 id, char* buf, s32 flag);
void put_message_sys(s8* buf, u32 a, u32 b, u32 c);
void put_message_sys_ok_button(void);
s32 ran_suu(s32 n);
s32 calcVecAng2(VEC3* a, VEC3* b);
u16 lb_npc_Get_motion_no(_LB_NPC* self);
namespace s_801FBF78 {


/* ---------------------------------------------------------------------------------------------------
 * Bodies
 */
extern "C" {

/* 0x801FBF78 - the lobby "world update" state dispatcher: each state runs its own quest-flag gate.
 * `lb_unlock_cond_top_get(1)` returns the current mode; the switch's 10 arms are states 0x1D..0x26. */
void fn_801FBF78(void)
{
    switch (lb_unlock_cond_top_get(1)) {
    case 0x1D:
        if (userdata_progress_flag_ck(0x3C) == 0 && userdata_flag_ck(0x2711) == 1 && userdata_flag_ck(0x2714) == 1 &&
            userdata_flag_ck(0x2715) == 1 && userdata_flag_ck(0x2718) == 1 && userdata_flag_ck(0x271D) == 1) {
            lb_progress_unlock(0x3C);
        }
        break;
    case 0x1E:
        if (userdata_flag_ck(0x36B0) == 1) {
            lb_progress_unlock(0x3D);
        }
        break;
    case 0x1F:
        if (userdata_progress_flag_ck(0x3E) == 0 && userdata_flag_ck(0x2743) == 1 && userdata_flag_ck(0x274A) == 1 &&
            userdata_flag_ck(0x274B) == 1 && userdata_flag_ck(0x274D) == 1 && userdata_flag_ck(0x2750) == 1) {
            lb_progress_unlock(0x3E);
        }
        break;
    case 0x20:
        if (userdata_flag_ck(0x36B1) == 1) {
            lb_progress_unlock(0x3F);
        }
        break;
    case 0x21:
        if (userdata_progress_flag_ck(0x40) == 0 && userdata_flag_ck(0x2774) == 1 && userdata_flag_ck(0x2775) == 1 &&
            userdata_flag_ck(0x277A) == 1 && userdata_flag_ck(0x277C) == 1 && userdata_flag_ck(0x2780) == 1) {
            lb_progress_unlock(0x40);
        }
        break;
    case 0x22:
        if (userdata_flag_ck(0x36B2) == 1) {
            lb_progress_unlock(0x41);
        }
        break;
    case 0x23:
        if (userdata_progress_flag_ck(0x42) == 0 && userdata_flag_ck(0x3A9B) == 1 && userdata_flag_ck(0x3A9E) == 1 &&
            userdata_flag_ck(0x3A9F) == 1 && userdata_flag_ck(0x3AA1) == 1 && userdata_flag_ck(0x3AA6) == 1) {
            lb_progress_unlock(0x42);
        }
        break;
    case 0x24:
        if (userdata_flag_ck(0x4A38) == 1) {
            lb_progress_unlock(0x43);
        }
        break;
    case 0x25:
        if (userdata_progress_flag_ck(0x44) == 0 && userdata_flag_ck(0x3ACB) == 1 && userdata_flag_ck(0x3ACD) == 1 &&
            userdata_flag_ck(0x3AD0) == 1 && userdata_flag_ck(0x3AD4) == 1 && userdata_flag_ck(0x3AD6) == 1) {
            lb_progress_unlock(0x44);
        }
        break;
    case 0x26:
        if (userdata_flag_ck(0x4A39) == 1) {
            lb_progress_unlock(0x45);
        }
        break;
    }
}

/* 0x801FC268 - refresh the user-data "talk" byte from the current mode. */
#pragma peephole off
void fn_801FC268(void)
{
    switch (lb_unlock_cond_top_get(0)) {
    case 0x1A:
        if (fn_802FB948() == 0) {
            lobby_world_block->field_0x3E01 = 9;
        } else {
            lobby_world_block->field_0x3E01 = 0xFF;
        }
        break;
    case 0:
        lobby_world_block->field_0x3E01 = 0;
        break;
    default:
        lobby_world_block->field_0x3E01 = 0xFF;
        break;
    }
}
#pragma peephole on

/* 0x801FC2F0 - reset the menu layer and leave the menu. */
void fn_801FC2F0(void)
{
    fn_80220078();
    fn_801E9AA8(0);
}

/* 0x801FC6A0 - show the system message indexed by *value when it is positive. */
void fn_801FC6A0(u32 unused, s32* value)
{
    s8 buf[0x108];

    if (*value > 0) {
        wii_sysmsg_gen(*value, (char*)buf, 0);
        put_message_sys(buf, 0, 0, 0);
        put_message_sys_ok_button();
    }
}

/* 0x801FC810 - seed the lobby's 13-entry random slot table. */
void fn_801FC810(void)
{
    s32 i = 0;

    do {
        lobby_w.field_0x036[i] = (s16)ran_suu(1);
        i++;
    } while (i < 13);
    lobby_w.field_0x003 = 0xFF;
}

/* 0x801FC874 - pick one of the three lobby variants at random. */
void fn_801FC874(void)
{
    s32 value = (u16)ran_suu(1);

    lobby_w.field_0x080 = (u8)(value % 3);
}

/* 0x801FC8C8 - copy the lobby's signed state byte into the shadow byte. */
void fn_801FC8C8(void)
{
    s32 value = lobby_w.field_0x002_s;

    lobby_w.field_0x079 = (s8)value;
}

}
} /* namespace s_801FBF78 */
 /* extern "C" */


/* ---------------------------------------------------------------------------------------------------
 * The lobby's quest-return step (0x801FC318).
 */
#include "enemy/em_set_work.h"
#include "get_FqResult_work.h"
#include "fn_8004CAD8/get_qResult_work.h"
#include "quest/quest_result_work.h"
#include "mh3_pad/lb_param_w.h"
namespace s_801FBF78 {
extern "C" {
void fn_801FC268(void);
void fn_801FC2F0(void);
void fn_801FBF78(void);
u32 userdata_item_slot_count(LbNpcUserData* data);
_SLOTENT* userdata_equip_item_slots_get(LbNpcUserData* data);
void lb_item_db_toggle(void);
u16 ai_behaviour_scaled_get(u16 value, u32 kind);
void item_take(u16 item, s16 count, u8* box, s32 a, s32 b, s32 c);
void item_pair_copy(_SLOTENT* dst, IdValue* src);
void note_timer_tick(void);
u32 game_ready_ck(void);
void lb_item_list_select(u8 index);
void lb_page_refresh(void);
void lb_entry_publish(void);
void lobby_pig_dress_view_set(u8 dress);
u16 lb_unlock_rank_get(void);

/* 0x801FC318 - folds the finished quest's result into the user data on the return to the lobby (the free-hunt or
 * the quest result: pouch, rank values, rank items and points), then refreshes the lobby screens. */
void lb_quest_return_apply(void)
{
    struct FqResultWork* free_hunt = get_FqResult_work();
    struct Q_ResultWork* result = get_qResult_work();
    s32 i;

    if (system_w.field_0x02D != 0) {
        u32 bytes;
        LbNpcUserData* data;

        fn_801FC268();
        bytes = userdata_item_slot_count(lobby_world_block) * 4;
        memcpy(userdata_equip_item_slots_get(lobby_world_block), free_hunt->pouch_0x1B4, bytes);
        lb_fq_result_items_take();
        if (free_hunt->seconds_0x1AC >= 2) {
            lb_item_db_toggle();
        }
        u16_add_sat(&lobby_world_block->hunt_rank_0x51A0, free_hunt->hunt_rank_0x242);
        u16_add_sat(&lobby_world_block->time_rank_0x51A2, free_hunt->time_rank_0x244);
        data = lobby_world_block;
        u16_add_sat(&data->cat_rank_0x51A6[data->field_0x3E03 & 0x7F],
                    ai_behaviour_scaled_get(free_hunt->time_rank_0x244, data->field_0x3E03 & 0x7F));
        for (i = 0; i < 12; i++) {
            item_take(lobby_world_block->rank_items_0x51B4[i].item_id, lobby_world_block->rank_items_0x51B4[i].value,
                      lobby_world_block->tail_0x51E4, 30, 0, 99);
            item_pair_copy(&lobby_world_block->rank_items_0x51B4[i], &free_hunt->rank_rows_0x246[i]);
            free_hunt->rank_rows_0x246[i].id = 0;
            free_hunt->rank_rows_0x246[i].value = 0;
        }
        lobby_world_block->field_0x519C |= free_hunt->rank_points_0x278;
        free_hunt->hunt_rank_0x242 = 0;
        free_hunt->time_rank_0x244 = 0;
        userdata_item_new_marks_clear();
    } else {
        switch (result->kind_0x1E3) {
        case 1: {
            u32 bytes;
            LbNpcUserData* data;

            fn_801FC268();
            bytes = userdata_item_slot_count(lobby_world_block) * 4;
            memcpy(userdata_equip_item_slots_get(lobby_world_block), result->items_0x154, bytes);
            if (result->phase_0x1E2 != 3 && result->seconds_0x14C >= 2) {
                lb_item_db_toggle();
            }
            if (result->progress_0x1E0 < 100) {
                system_w.field_0x02D = 1;
            }
            u16_add_sat(&lobby_world_block->hunt_rank_0x51A0, result->rank_0x3A6);
            u16_add_sat(&lobby_world_block->time_rank_0x51A2, result->time_rank_0x3A8);
            data = lobby_world_block;
            u16_add_sat(&data->cat_rank_0x51A6[data->field_0x3E03 & 0x7F],
                        ai_behaviour_scaled_get(result->time_rank_0x3A8, data->field_0x3E03 & 0x7F));
            for (i = 0; i < 12; i++) {
                item_take(lobby_world_block->rank_items_0x51B4[i].item_id, lobby_world_block->rank_items_0x51B4[i].value,
                          lobby_world_block->tail_0x51E4, 30, 0, 99);
                item_pair_copy(&lobby_world_block->rank_items_0x51B4[i], &result->rank_rows_0x3AA[i]);
                result->rank_rows_0x3AA[i].id = 0;
                result->rank_rows_0x3AA[i].value = 0;
            }
            lobby_world_block->field_0x519C |= result->rank_points_0x3DC;
            result->rank_0x3A6 = 0;
            result->time_rank_0x3A8 = 0;
            if (result->phase_0x1E2 != 3) {
                userdata_item_new_marks_clear();
            }
            break;
        }
        case 3: {
            u32 bytes = userdata_item_slot_count(lobby_world_block) * 4;

            memcpy(userdata_equip_item_slots_get(lobby_world_block), result->items_0x154, bytes);
            fn_801FBF78();
            if (result->phase_0x1E2 != 3) {
                userdata_item_new_marks_clear();
                note_timer_tick();
                userdata_server_rows_reset();
            }
            break;
        }
        }
    }
    if (game_ready_ck() == 0) {
        lb_progress_rewards_check();
        lb_daily_menu_roll(lobby_world_block->name_0x4068);
        lb_item_list_select(lobby_world_block->field_0x3E00);
        lb_page_refresh();
        lb_entry_publish();
        fn_801FC2F0();
        lobby_pig_dress_view_set(lobby_world_block->field_0x483B);
        if (lb_unlock_rank_get() == 1) {
            lb_param_w.hunt_option_0x0A = 0;
        } else {
            lb_param_w.hunt_option_0x0A = 1;
        }
    }
    lb_award_check_all();
    lb_award_rows_apply();
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* 0x801FC8E0 - the talk-NPC data block. */
u8* get_talk_npc_data_ptr(void)
{
    return lobby_w.talk_0x0C0;
}
namespace s_801FBF78 {


extern "C" {

/* 0x801FCAC8 - copy the 8-byte sprite record `src` into `dst`. */
__declspec(noinline) void fn_801FCAC8(u32* dst, const u32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

} /* extern "C" */

/* =================================================================================================== */

/* This unit's own functions, forward-declared in address order (the bodies below call later rows). */
extern "C" {
s32 fn_801FCB68(u32 motion_id);
s32 fn_801FDE3C(_LB_NPC* self, VEC3* target);
void fn_801FDFE4(_LB_NPC* self, u32 motion_id, s32 a, s32 b);
void fn_801FE11C(_LB_NPC* self);
void fn_801FE130(_LB_NPC* self);
void fn_801FE1F8(_LB_NPC* self);
}

extern "C" {

/* 0x801FDD9C - aim the NPC at its target over `duration`: latch the tick count and the per-tick step. */
void fn_801FDD9C(_LB_NPC* self, f32 duration)
{
    self->field_0x1E2 = (s16)duration;
    self->vel_0x1F8.x = (self->target_0x1EC.x - self->pos_0x10.x) / duration;
    self->vel_0x1F8.y = (self->target_0x1EC.y - self->pos_0x10.y) / duration;
    self->vel_0x1F8.z = (self->target_0x1EC.z - self->pos_0x10.z) / duration;
}

/* 0x801FDDF4 - advance the timed move by one tick. */
void fn_801FDDF4(_LB_NPC* self)
{
    if (self->field_0x1E2 == 0) {
        return;
    }
    self->pos_0x10.x += self->vel_0x1F8.x;
    self->pos_0x10.y += self->vel_0x1F8.y;
    self->pos_0x10.z += self->vel_0x1F8.z;
    self->field_0x1E2--;
}

/* 0x801FDE3C - the NPC's heading minus the stored reference angle. */
__declspec(noinline) s32 fn_801FDE3C(_LB_NPC* self, VEC3* target)
{
    return calcVecAng2(&self->pos_0x10, target) - self->field_0x02C;
}

/* 0x801FDE74 - tick the scripted turn; when it runs out clear the turn flags. */
void fn_801FDE74(_LB_NPC* self)
{
    if (self->field_0x1E4 <= 0) {
        self->field_0x1E4 = 0;
        self->field_0x229 = 0;
        self->field_0x228 = 0;
    } else {
        self->field_0x1E4--;
        self->field_0x02C += (s16)self->field_0x1E8;
    }
}

/* 0x801FDEB4 - start a turn from the current angle to `to` over `ticks`. */
void fn_801FDEB4(_LB_NPC* self, s32 to, s32 ticks)
{
    self->field_0x1D4 = (s16)to - (s16)self->field_0x02C;
    self->field_0x1E8 = (u16)((s16)self->field_0x1D4 / (s16)ticks);
    self->field_0x1E4 = (s16)ticks;
}

/* 0x801FDEE4 - turn towards the motion record's first vector over `ticks`, latching the turn flag. */
void fn_801FDEE4(_LB_NPC* self, s32 ticks)
{
    s32 angle;

    self->field_0x1D4 = fn_801FDE3C(self, &self->field_0x214->vec_0x03C);
    angle = abs((s16)self->field_0x1D4);
    if (angle > 10 && self->field_0x229 == 0) {
        self->field_0x228 = 1;
        self->field_0x218 = self->field_0x02C;
    }
    self->field_0x1E8 = (u16)((s16)self->field_0x1D4 / (s16)ticks);
    self->field_0x1E4 = (s16)ticks;
}

/* 0x801FDF70 - start a turn back to the latched angle over `ticks`. */
void fn_801FDF70(_LB_NPC* self, s32 ticks)
{
    self->field_0x1D4 = (u16)(self->field_0x218 - self->field_0x02C);
    self->field_0x1E8 = (s16)self->field_0x1D4 / (s16)ticks;
    self->field_0x1E4 = (s16)ticks;
    self->field_0x229 = 1;
}

/* 0x801FDFA4 - flag the NPC and zero its effect vector. */
void fn_801FDFA4(_LB_NPC* self, u32 flag)
{
    self->field_0x264 = (u8)(flag & 0x80);
    setVector3(&self->field_0x258, lbl_80799880, lbl_80799880, lbl_80799880);
}

/* 0x801FDFC0 - set the NPC's action byte. */
void fn_801FDFC0(_LB_NPC* self, u8 value)
{
    self->field_0x225 = value;
}

/* 0x801FDFC8 - set the NPC's secondary action byte. */
void fn_801FDFC8(_LB_NPC* self, u8 value)
{
    self->field_0x256 = value;
}

/* 0x801FDFD0 - true while the lobby is in its idle state. */
u32 fn_801FDFD0(_LB_NPC* self)
{
    return lobby_w.field_0x000 == 0;
}

/* 0x801FDFE4 - spawn the NPC's motion resource and hand it to the model layer. */
void fn_801FDFE4(_LB_NPC* self, u32 motion_id, s32 a, s32 b)
{
    s32 motion;

    if (self->model_0x050->flag_0x006 == 0) {
        if ((u16)motion_id < 0x3E8) {
            motion = fn_8026A00C((u16)motion_id);
        } else {
            motion = fn_801FCB68((u16)motion_id);
        }
    } else {
        motion = fn_802D282C((u16)motion_id);
    }
    mhchar_motion_set(self->body_0x058, self->field_0x210, 0, (u16)motion_id, a, motion, 0, (f32)b,
                 lbl_80799884);
}

/* 0x801FE0AC - switch the NPC's motion when the requested one differs from the current one. */
void fn_801FE0AC(_LB_NPC* self, u16 motion_id, s32 a, s32 b)
{
    if ((u16)motion_id != lb_npc_Get_motion_no(self)) {
        fn_801FDFE4(self, motion_id, a, b);
    }
}

/* 0x801FE11C - clear the NPC's first state byte. */
__declspec(noinline) void fn_801FE11C(_LB_NPC* self)
{
    self->field_0x149 = 0;
}

/* 0x801FE128 - set the NPC's second state byte. */
void fn_801FE128(_LB_NPC* self, u8 value)
{
    self->field_0x14A = value;
}

/* 0x801FE130 - clear the NPC's second state byte. */
__declspec(noinline) void fn_801FE130(_LB_NPC* self)
{
    self->field_0x14A = 0;
}

/* 0x801FE13C - restart the NPC's motion state machine. */
void lb_npc_motion_restart(_LB_NPC* self, u16 motion_id)
{
    self->field_0x006 = 0;
    self->field_0x007 = 0;
    self->field_0x008 = 0;
    self->field_0x1D0 = self->field_0x1CC;
    self->field_0x1CC = motion_id;
    self->field_0x1D8 = (s16)ran_suu(1);
    self->field_0x1DC = lbl_80799884;
    fn_801FE1F8(self);
    fn_801FE11C(self);
    fn_801FE130(self);
}

/* 0x801FE1AC - true when `motion_id` is the NPC's current motion. */
u32 fn_801FE1AC(_LB_NPC* self, u32 motion_id)
{
    return (u16)motion_id - self->field_0x1CC == 0;
}

/* 0x801FE1CC / 0x801FE1DC - true when the NPC's motion crosses frame `frame` (once, or on every loop). */
u32 lb_npc_motion_play(_LB_NPC* self, u32 motion_id, f32 frame, f32 speed)
{
    return mhchar_frame_check(self->body_0x058, (u16)motion_id, 0, frame, speed);
}

u32 fn_801FE1DC(_LB_NPC* self, u32 motion_id, f32 frame, f32 speed)
{
    return mhchar_frame_check(self->body_0x058, (u16)motion_id, 1, frame, speed);
}

/* 0x801FE1EC - true when the NPC's motion has run out. */
u32 fn_801FE1EC(_LB_NPC* self)
{
    return mhchar_motion_end_ck(self->body_0x058, 0);
}

/* 0x801FE1F8 - finalise the NPC's model. */
void fn_801FE1F8(_LB_NPC* self)
{
    mhchar_motion_speed_set(self->body_0x058);
}

}
} /* namespace s_801FBF78 */
 /* extern "C" */

/* 0x801FE1C4 - the NPC's current motion number. */
u16 lb_npc_Get_motion_no(_LB_NPC* self)
{
    return self->motion_0x0A8;
}

/* ===================================================================================================
 * The NPC model/kind helpers and the per-frame effect dispatcher (0x801FE200..0x801FF5FC).
 */
#include "ef/eft004_set.h"
#include "ef/eft004_scaled_spawn.h"
#include "fn_8004CAD8/mtx.h"
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* out);
void mulVecMatAddTrans(VEC3* v, nw4r::math::MTX34* m);
void* get_move_work_adrs(u8 idx);
namespace s_801FBF78 {

extern "C" {
void em_land_data_init(LbNpcGround* out);
void eft_rot_vec_copy(_CP_VECTOR* dst, _CP_VECTOR* src);
void addVec3To(VEC3* a, VEC3* b);
void mtx34_set_trans(nw4r::math::MTX34* m, VEC3* v);
void lb_quest_board_effect_spawn(_LB_NPC* self, s32 kind, s32 joint, VEC3* offset, f32 scale);
s8 my_player_no(void);
u32 fn_80208AC0(_PLW* self);
void lb_npc_dress_apply(_LB_NPC* self, LbNpcSubOp* list);
u32 lb_npc_motion_play(_LB_NPC* self, u32 motion_id, f32 frame, f32 speed);
u32 fn_801FE1DC(_LB_NPC* self, u32 motion_id, f32 frame, f32 speed);

extern LbNpcSubOp*** npc_sub_data;          /* .sbss: per model, per variant, the dressing op list */
extern f32* npc_model_scale_data;           /* .sbss: per model, the model scale */
extern f32* npc_model_inscreen_data;        /* .sbss: per model, the on-screen radius */
extern f32* npc_model_cur_height_data;      /* .sbss: per model, the height */
}

/* The NPC's model body (the `MHchar` that starts at +0x58). */
static inline MHchar* lb_npc_body(_LB_NPC* self)
{
    return (MHchar*)self->body_0x058;
}

#pragma peephole off
extern "C" {

/* 0x801FE200 - shows or hides one node of the NPC's model. */
void fn_801FE200(_LB_NPC* self, u32 index, bool visible)
{
    lb_npc_body(self)->setVisibility((u8)index, visible);
}

/* 0x801FE20C - drops the NPC from 48 units above its last position onto the ground below its current one. */
void lb_npc_ground_drop(_LB_NPC* self)
{
    VEC3 from;
    VEC3 to;

    VEC3_ctor(&from);
    VEC3_ctor(&to);
    if (self->field_0x22B == 0) {
        copyVec3(&from, &self->field_0x01C);
        from.y += 48.0f;
        copyVec3(&to, &self->pos_0x10);
        to.y += 48.0f;
        self->field_0x22A = coll_wall_slide(&from, &to, &self->pos_0x10, 0xFFFF, 0, 1, self->field_0x004);
    }
}

/* 0x801FE2B4 - snaps the NPC onto the ground height below it and latches the ground attribute. */
void lb_npc_ground_snap(_LB_NPC* self)
{
    LbNpcGround ground;
    VEC3 a;
    VEC3 b;
    f32 height;

    em_land_data_init(&ground);
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    if (npc_ground_check(self, &self->pos_0x10, &ground, &height, 0xFFFF) == 1) {
        self->pos_0x10.y = self->field_0x034 = height;
        self->field_0x230 = ground.attr_0x00;
    }
}

/* 0x801FE32C - true when the NPC's placement record has model kind `kind`. */
u32 lb_npc_id_ck(_LB_NPC* self, u32 kind)
{
    return self->model_0x050->model_0x002 == (u8)kind;
}

/* 0x801FE348 - model 4, variant 0. */
u32 fn_801FE348(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 4) == 1 && self->model_0x050->variant_0x004 == 0) {
        return 1;
    }
    return 0;
}

/* 0x801FE39C - model 4, variant 1 or 2. */
u32 fn_801FE39C(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 4) == 1) {
        u8 variant = self->model_0x050->variant_0x004;
        if (variant == 1) {
            return 1;
        }
        if (variant == 2) {
            return 1;
        }
    }
    return 0;
}

/* 0x801FE400 - model 15 as NPC 0x29. */
u32 fn_801FE400(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 15) == 1 && self->field_0x002 == 0x29) {
        return 1;
    }
    return 0;
}

/* 0x801FE450 - model 15 as NPC 0x40. */
u32 fn_801FE450(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 15) == 1 && self->field_0x002 == 0x40) {
        return 1;
    }
    return 0;
}

/* 0x801FE4A0 - true for NPC 0x35. */
u32 fn_801FE4A0(_LB_NPC* self)
{
    return self->field_0x002 == 0x35;
}

/* 0x801FE4B4 - model 19, variant 0. */
u32 fn_801FE4B4(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 19) == 1 && self->model_0x050->variant_0x004 == 0) {
        return 1;
    }
    return 0;
}

/* 0x801FE508 - model 19, variant 1. */
u32 lb_npc_model19_variant1_ck(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 19) == 1 && self->model_0x050->variant_0x004 == 1) {
        return 1;
    }
    return 0;
}

/* 0x801FE55C - model 20, variant 1. */
u32 fn_801FE55C(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 20) == 1 && self->model_0x050->variant_0x004 == 1) {
        return 1;
    }
    return 0;
}

/* 0x801FE5B0 - the live NPC whose id is `kind`, or null. */
_LB_NPC* lb_npc_find(u32 kind)
{
    _LB_NPC* npc;
    s32 i;

    for (i = 0, npc = lb_npc; i < 0x12; i++) {
        if (npc->field_0x000 != 0 && npc->field_0x002 == (u8)kind) {
            return npc;
        }
        npc++;
    }
    return 0;
}

/* 0x801FE6B0 - the on-screen radius of NPC model `model`. */
f32 lb_npc_model_inscreen_get(s32 model)
{
    return npc_model_inscreen_data[(s16)model];
}

/* 0x801FE6C4 - the current height of NPC model `model`. */
f32 lb_npc_model_height_get(s32 model)
{
    return npc_model_cur_height_data[(s16)model];
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* 0x801FE6D8 - true when the NPC stands in area `area`. */
s32 lb_npc_area_ck(_LB_NPC* self, u8 area)
{
    return self->field_0x004 == area;
}

namespace s_801FBF78 {
extern "C" {

/* 0x801FE6F0 - whether NPC `id` may be talked to: only NPC 0x17 is gated, by the player's idle state and the area
 * the lobby last latched. */
s32 lb_npc_talk_ok_npc23(u32 id)
{
    _PLW* plw;

    if ((u8)id != 0x17) {
        return 1;
    }
    if (lobby_w.field_0x077 == 1) {
        return 0;
    }
    plw = (_PLW*)get_move_work_adrs(2);
    plw += my_player_no();
    if (fn_80208AC0(plw) == 1) {
        lobby_w.talk_area_0x055 = (s8)lobby_w.field_0x002;
        return 1;
    }
    if (lobby_w.field_0x076 == 1) {
        lobby_w.talk_area_0x055 = (s8)lobby_w.field_0x002;
        return 1;
    }
    return lobby_w.talk_area_0x055 == lobby_w.field_0x002;
}

/* 0x801FE7C0 - NPC 0x17 records its position, rotation and motion table into the lobby work. */
void lb_npc_npc23_state_save(_LB_NPC* self)
{
    if (self->field_0x002 == 0x17) {
        copyVec3(&lobby_w.npc_pos_0x058, &self->pos_0x10);
        eft_rot_vec_copy(&lobby_w.npc_rot_0x064, &self->rot_0x028);
        lobby_w.npc_motion_0x070 = self->field_0x204;
        lobby_w.npc_motion_index_0x074 = self->field_0x208;
    }
}

/* 0x801FE82C - applies dressing list `index` of model 18 to the NPC. */
void lb_npc_dress_model18(_LB_NPC* self, u32 index)
{
    lb_npc_dress_apply(self, npc_sub_data[18][(u8)index]);
}

/* 0x801FE844 - whether NPC `id` may be talked to: NPC 0x49 needs the player's +0xB03 flag or a free lobby. */
s32 lb_npc_talk_ok_npc73(u32 id)
{
    _PLW* plw = (_PLW*)get_move_work_adrs(2);

    plw += my_player_no();
    if ((u8)id != 0x49) {
        return 1;
    }
    if (plw->field_0xB03 != 0) {
        return 1;
    }
    return lobby_w.field_0x077 == 0;
}

/* 0x801FE8CC - model 18 as any NPC but 0x42. */
u32 fn_801FE8CC(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 18) == 0) {
        return 0;
    }
    return self->field_0x002 != 0x42;
}

/* 0x801FE920 - whether NPC `id` may be talked to in talk mode `mode`: NPC 0x0E depends on the lobby area. */
s32 lb_npc_talk_ok_npc14(u32 id, u32 mode)
{
    if ((u8)id != 0xE) {
        return 1;
    }
    switch (lobby_w.field_0x002) {
    case 0:
        if ((s32)(u8)mode == 1 && lobby_w.field_0x080 == 2) {
            return 0;
        }
        break;
    case 1:
        switch ((u8)mode) {
        case 0:
            return 0;
        case 1:
            if (lobby_w.field_0x080 != 2) {
                return 0;
            }
            break;
        case 2:
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x801FE9C0 - spawns effect `type` at model joint `joint` (0xFF: the NPC's feet), 5 units above the NPC's ground. */
void lb_npc_joint_effect_spawn(_LB_NPC* self, u8 type, u32 joint, f32 scale)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    if (joint != 0xFF) {
        lb_npc_body(self)->get_joint_wpos(joint, &pos);
        pos.y = 5.0f + self->field_0x034;
    } else {
        copyVec3(&pos, &self->pos_0x10);
        pos.y = 5.0f + self->field_0x034;
    }
    eft004_set(type, &pos, scale, self->field_0x02C, self->field_0x004);
}

/* 0x801FEA80 - the NPC's per-frame motion effects: on the given frames of the given motions spawn the model kind's
 * dust, sparks and steam. */
void lb_npc_motion_effects(_LB_NPC* self)
{
    VEC3 offset;
    VEC3 pos;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&offset);
    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    if (self->field_0x001 == 0) {
        return;
    }
    switch (self->model_0x050->model_0x002) {
    case 1:
        switch (lb_npc_Get_motion_no(self)) {
        case 1001:
            if (fn_801FE1DC(self, 0, 460.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 0.0f, 0.0f);
                lb_quest_board_effect_spawn(self, 6, 14, &offset, 1.0f);
            }
            break;
        case 1012:
            if (fn_801FE1DC(self, 0, 200.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 0.0f, 0.0f);
                lb_quest_board_effect_spawn(self, 6, 14, &offset, 1.0f);
            }
            break;
        }
        break;
    case 5:
        switch (lb_npc_Get_motion_no(self)) {
        case 1171:
            if (fn_801FE1DC(self, 0, 24.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, -20.0f, 120.0f);
                lb_quest_board_effect_spawn(self, 0, 1, &offset, 1.0f);
            }
            break;
        case 1172:
            if (fn_801FE1DC(self, 0, 82.0f, 0.0f) == 1) {
                lb_npc_body(self)->get_joint_wpos(10, &pos);
                cpSetRotMatrix(&self->rot_0x028, &mtx);
                setVector3(&offset, 0.0f, 5.0f, 5.0f);
                mulVecMatAddTrans(&offset, &mtx);
                addVec3To(&pos, &offset);
                mtx34_set_trans(&mtx, &pos);
                eft004_scaled_spawn(&pos, self->field_0x004, 1.0f);
            }
            break;
        }
        break;
    case 18:
        switch (lb_npc_Get_motion_no(self)) {
        case 1093:
            if (fn_801FE1DC(self, 1, 110.0f, 0.0f) == 1 && (system_w.frame_0x00C & 0x3F) == 0) {
                setVector3(&offset, 0.0f, 20.0f, 30.0f);
                lb_quest_board_effect_spawn(self, 7, 10, &offset, 1.0f);
            }
            break;
        case 1101:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 26.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 46.0f, 0.0f) == 1 ||
                    fn_801FE1DC(self, 0, 56.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 1, 2, 1.8f);
                    lb_npc_joint_effect_spawn(self, 2, 2, 1.8f);
                }
            }
            break;
        case 1102:
            break;
        case 1103:
            if (fn_801FE1DC(self, 0, 2.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 25.0f, 25.0f);
                lb_quest_board_effect_spawn(self, 4, 10, &offset, 1.0f);
            }
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 18.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 2, 2, 2.5f);
                }
                if (fn_801FE1DC(self, 0, 42.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 2, 2, 2.0f);
                }
            }
            break;
        case 1104:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 14.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 56.0f, 0.0f) == 1 ||
                    fn_801FE1DC(self, 0, 90.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 112.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 2, 11, 2.0f);
                }
                if (fn_801FE1DC(self, 0, 56.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 1, 2, 3.5f);
                }
            }
            if (fn_801FE1DC(self, 0, 20.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 80.0f, 0.0f);
                lb_quest_board_effect_spawn(self, 1, 10, &offset, 1.0f);
            }
            if (fn_801FE1DC(self, 0, 64.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 80.0f, 0.0f);
                lb_quest_board_effect_spawn(self, 2, 0, &offset, 1.0f);
            }
            if (fn_801FE1DC(self, 0, 228.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 40.0f, 0.0f);
                lb_quest_board_effect_spawn(self, 3, 10, &offset, 1.0f);
            }
            break;
        case 1105:
            break;
        case 1106:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 18.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 34.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 1, 11, 2.0f);
                    lb_npc_joint_effect_spawn(self, 2, 11, 2.0f);
                }
            } else if (fn_801FE1DC(self, 0, 18.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 34.0f, 0.0f) == 1) {
                lb_npc_joint_effect_spawn(self, 0, 11, 2.0f);
            }
            break;
        case 1107:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 2.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 22.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 1, 11, 2.0f);
                    lb_npc_joint_effect_spawn(self, 2, 11, 2.0f);
                }
            } else if (fn_801FE1DC(self, 0, 2.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 22.0f, 0.0f) == 1) {
                lb_npc_joint_effect_spawn(self, 0, 11, 2.0f);
            }
            break;
        case 1109:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 126.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 138.0f, 0.0f) == 1) {
                    lb_npc_body(self)->get_joint_wpos(11, &pos);
                    setVector3(&offset, 0.0f, 0.0f, -50.0f);
                    rotVecY(&offset, self->field_0x02C);
                    addVec3To(&pos, &offset);
                    pos.y = 5.0f + self->field_0x034;
                    eft004_set(1, &pos, 1.8f, self->field_0x02C, self->field_0x004);
                }
            }
            break;
        case 1173:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 48.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 92.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 1, 2, 3.2f);
                    lb_npc_joint_effect_spawn(self, 2, 2, 3.2f);
                }
                if (fn_801FE1DC(self, 0, 124.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 130.0f, 0.0f) == 1 ||
                    fn_801FE1DC(self, 0, 136.0f, 0.0f) == 1) {
                    copyVec3(&pos, &self->pos_0x10);
                    setVector3(&offset, 0.0f, 20.0f, -10.0f);
                    rotVecY(&offset, self->field_0x02C);
                    addVec3To(&pos, &offset);
                    eft004_set(1, &pos, 2.5f, self->field_0x02C, self->field_0x004);
                }
            }
            break;
        case 1174:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 3, 2.0f, 128.0f) == 1 && (system_w.frame_0x00C & 7) == 0) {
                    copyVec3(&pos, &self->pos_0x10);
                    setVector3(&offset, 0.0f, 10.0f, 0.0f);
                    addVec3To(&pos, &offset);
                    eft004_set(1, &pos, 2.5f, self->field_0x02C, self->field_0x004);
                }
            }
            break;
        case 1190:
            if ((self->field_0x230 & 2) != 0) {
                if (fn_801FE1DC(self, 0, 12.0f, 0.0f) == 1 || fn_801FE1DC(self, 0, 44.0f, 0.0f) == 1) {
                    lb_npc_joint_effect_spawn(self, 15, 2, 2.6f);
                }
                if (fn_801FE1DC(self, 1, 50.0f, 0.0f) == 1 && (system_w.frame_0x00C & 0x1F) == 0) {
                    lb_npc_body(self)->get_joint_wpos(2, &pos);
                    eft004_set(2, &pos, 1.8f, self->field_0x02C, self->field_0x004);
                }
            }
            break;
        }
        break;
    case 4:
        if (fn_801FE39C(self) == 1) {
            switch (lb_npc_Get_motion_no(self)) {
            case 1074:
                if (fn_801FE1DC(self, 0, 140.0f, 0.0f) == 1) {
                    lb_npc_body(self)->get_joint_wpos(7, &pos);
                    cpSetRotMatrix(&self->rot_0x028, &mtx);
                    mtx34_set_trans(&mtx, &pos);
                    setVector3(&offset, 0.0f, 8.0f, 10.0f);
                    mulVecMatAddTrans(&offset, &mtx);
                    eft004_scaled_spawn(&offset, self->field_0x004, 1.0f);
                }
                break;
            case 1171:
                if (fn_801FE1DC(self, 0, 24.0f, 0.0f) == 1) {
                    setVector3(&offset, 0.0f, -9.0f, 120.0f);
                    lb_quest_board_effect_spawn(self, 0, 2, &offset, 1.0f);
                }
                break;
            }
        }
        break;
    }
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ---------------------------------------------------------------------------------------------------
 * The NPC work array's construction and destruction (0x801FF700..0x801FF984).
 */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h"   /* __construct_array */
#include "Runtime.PPCEABI.H/__destroy_arr.h"
#include "Runtime.PPCEABI.H/__register_global_object.h"
#include "g3d/scn_obj_callback_dtor.h"
namespace s_801FBF78 {
extern "C" {
MHchar* mhchar_construct(MHchar* self);
_LB_NPC* lb_npc_ctor(_LB_NPC* self);
_LB_NPC* lb_npc_dtor(_LB_NPC* self, s16 flag);
LbNpcChar* lb_npc_char_ctor(LbNpcChar* self);
LbNpcChar* lb_npc_char_dtor(LbNpcChar* self, s16 flag);
LbNpcMatCallback* lb_npc_mat_callback_ctor(LbNpcMatCallback* self);
LbNpcMatCallback* lb_npc_mat_callback_dtor(LbNpcMatCallback* self, s16 flag);
/* untyped: an opaque handle passed through - the destructor chain's registered object (unused) */
void lb_npc_array_dtor(void* object, int flag);
extern DestructorChain lb_npc_dtor_chain;   /* .bss 0x806A7B90: the array's global destructor chain node */

/* 0x801FF700 (0x58): constructs the 18 NPC work records and registers their array destructor. */
void lb_npc_static_init(void)
{
    __construct_array(lb_npc, (void*)lb_npc_ctor, (void*)lb_npc_dtor, sizeof(_LB_NPC), 18);
    __register_global_object(0, lb_npc_array_dtor, &lb_npc_dtor_chain);
}

/* 0x801FF758 (0x1C): destroys the 18 NPC work records. */
/* untyped: an opaque handle passed through - the destructor chain's registered object (unused) */
void lb_npc_array_dtor(void* object, int flag)
{
    __destroy_arr(lb_npc, (void*)lb_npc_dtor, sizeof(_LB_NPC), 18);
}

/* 0x801FF774 (0x60): destroys an NPC work record's character block, then frees the record when `flag` asks. */
_LB_NPC* lb_npc_dtor(_LB_NPC* self, s16 flag)
{
    if (self != 0) {
        lb_npc_char_dtor((LbNpcChar*)self->field_0x054, -1);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801FF7D4 (0x60): destroys a character block's material hook, then frees the block when `flag` asks. */
LbNpcChar* lb_npc_char_dtor(LbNpcChar* self, s16 flag)
{
    if (self != 0) {
        lb_npc_mat_callback_dtor(&self->callback_0x16C, -1);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801FF834 (0x5C): destroys the material hook through the scene-object callback base, then frees it when `flag`
 * asks. */
LbNpcMatCallback* lb_npc_mat_callback_dtor(LbNpcMatCallback* self, s16 flag)
{
    if (self != 0) {
        scn_obj_callback_dtor(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x801FF890 (0x6C): constructs an NPC work record: its vectors and its character block. */
_LB_NPC* lb_npc_ctor(_LB_NPC* self)
{
    VEC3_ctor(&self->pos_0x10);
    VEC3_ctor(&self->field_0x01C);
    VEC3_ctor(&self->field_0x038);
    VEC3_ctor(&self->field_0x044);
    lb_npc_char_ctor((LbNpcChar*)self->field_0x054);
    VEC3_ctor(&self->target_0x1EC);
    VEC3_ctor(&self->vel_0x1F8);
    VEC3_ctor(&self->field_0x258);
    return self;
}

/* 0x801FF8FC (0x3C): constructs a character block: the model body, then the material hook. */
LbNpcChar* lb_npc_char_ctor(LbNpcChar* self)
{
    mhchar_construct(&self->body_0x004);
    lb_npc_mat_callback_ctor(&self->callback_0x16C);
    return self;
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ---------------------------------------------------------------------------------------------------
 * The NPC character block's model functions (0x801FF984..0x801FFB78).
 */
namespace s_801FBF78 {
extern "C" {
u32* res_file_ctor(u32* dst, u32 file_data);
u32* res_file_assign(u32* dst, u32* src);
void mhchar_reset(MHchar* self);
}
}
LbResource* ckResourceName(char* name);
namespace s_801FBF78 {
extern "C" {

/* 0x801FF984 - constructs an NPC's model body: its vectors and empty resource handles. */
MHchar* mhchar_construct(MHchar* self)
{
    VEC3_ctor(&self->pos_0x04);
    VEC3_ctor(&self->field_0x10);
    VEC3_ctor(&self->scale_0x1C);
    res_file_ctor((u32*)&self->field_0x110, 0);
    new (&self->field_0x114) nw4r::g3d::ResMdl((void*)0);
    return self;
}

/* 0x801FF9E0 - binds the character block to NPC `owner`: reset the model and take the NPC's id. */
void lb_npc_char_bind(LbNpcChar* self, _LB_NPC* owner)
{
    mhchar_reset(&self->body_0x004);
    self->npc_id_0x168 = owner->field_0x002;
    self->owner_0x000 = owner;
}

/* 0x801FFA28 - loads the owner's model kind into the body (the model or the village variant) and scales it. */
void lb_npc_char_load(LbNpcChar* self)
{
    u32 res;
    LbNpcModel* model;
    LbResource* file;

    res_file_ctor(&res, 0);
    model = self->owner_0x000->model_0x050;
    file = ckResourceName((char*)npc_model_res_ids[model->model_0x002].b_0x04);
    if (file != 0) {
        u32 tmp;

        res_file_assign(&res, res_file_ctor(&tmp, (u32)file->payload_0x44));
        switch (model->flag_0x006) {
        case 0:
            mhchar_model_create(&self->body_0x004, &res, 141, 0);
            break;
        case 1:
            mhchar_model_create(&self->body_0x004, &res, 1, 0);
            break;
        }
        mhchar_draw_mode_set(&self->body_0x004, 2);
        self->body_0x004.scale_0x1C.x = self->body_0x004.scale_0x1C.y = self->body_0x004.scale_0x1C.z =
            npc_model_scale_data[model->model_0x002];
    }
}

/* 0x801FFB18 - moves the loaded body for this frame (with the record's area byte) and binds it to the scene root. */
void lb_npc_char_move(LbNpcChar* self)
{
    u8 area = self->owner_0x000->field_0x001;

    if (self->body_0x004.field_0x118 != 0) {
        self->body_0x004.field_0x34 = area;
        self->body_0x004.move(0);
    }
    if (self->body_0x004.field_0x118 != 0) {
        g3d_root_model_bind(pRoot, self->body_0x004.field_0x118);
    }
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ---------------------------------------------------------------------------------------------------
 * The material hook's CALC_MAT pass (0x801FFB78), the override in its vtable's +0x10 slot.
 */
#include "sound/mhchar_mat_num_get.h"
namespace s_801FBF78 {
extern "C" {

/* 0x801FFB78 (0x128): on the CALC_MAT pass (timing 4), paints the body's materials: kind 6 lights materials 6 and 7,
 * any other kind material 5; every other material is painted black. */
/* untyped: caller-owned payload - the pass's info block */
void lb_npc_mat_callback_calc_mat(LbNpcMatCallback* self, s32 timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
    _GXColor black = {0, 0, 0, 0};
    _GXColor white = {255, 255, 255, 255};
    u32 i;

    if (timing == 4) {
        if (self->kind_0x04 == 6) {
            for (i = 0; i < mhchar_mat_num_get(self->chr_0x08); i++) {
                if (i != 6 && i != 7) {
                    self->chr_0x08->setMatColor(i, GX_COLOR0A0, black, true);
                } else {
                    self->chr_0x08->setMatColor(i, GX_COLOR0A0, white, true);
                }
            }
        } else {
            for (i = 0; i < mhchar_mat_num_get(self->chr_0x08); i++) {
                if (i != 5) {
                    self->chr_0x08->setMatColor(i, GX_COLOR0A0, black, true);
                } else {
                    self->chr_0x08->setMatColor(i, GX_COLOR0A0, white, true);
                }
            }
        }
    }
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ===================================================================================================
 * The NPC slot refresh, the motion-table walker and the per-kind idle machines (0x801FF5FC..0x80201C80).
 */
namespace s_801FBF78 {

extern "C" {
void fn_801FD4BC(_LB_NPC* self);
void fn_801FDFC0(_LB_NPC* self, u8 value);
void fn_801FDFC8(_LB_NPC* self, u8 value);
void lb_npc_talk_reaction_start(_LB_NPC* self);
void lb_npc_scene_step(_LB_NPC* self);
void lb_npc_model18_hooks(_LB_NPC* self);
void lb_npc_cat_farm_follow(_LB_NPC* self);
void lb_npc_model12_follow(_LB_NPC* self);
void fn_80207A44(_LB_NPC* self);
void lb_npc_motion_dispatch(_LB_NPC* self);
void fn_801FDDF4(_LB_NPC* self);
void fn_801FDE74(_LB_NPC* self);
void lb_npc_prop_sync(_LB_NPC* self);
void fn_801FDD9C(_LB_NPC* self, f32 duration);
void fn_801FDEB4(_LB_NPC* self, s32 to, s32 ticks);
void fn_801FDEE4(_LB_NPC* self, s32 ticks);
void fn_801FDF70(_LB_NPC* self, s32 ticks);
u32 fn_801FDFD0(_LB_NPC* self);
void fn_801FE0AC(_LB_NPC* self, u16 motion_id, s32 a, s32 b);
void fn_801FE128(_LB_NPC* self, u8 value);
VEC3* em_vec3_copy(VEC3* out, LbNpcMotionEntry* entry);
f32 vec3_dist_sq(VEC3* a, VEC3* b);
extern u8 lb_area_light_ids[];   /* .data: the lobby areas' light ids, 0xFF-terminated */
}

extern "C" {

/* 0x801FF5FC - stores the NPC's talk mode byte; a null NPC is skipped. */
void lb_npc_talk_mode_set(_LB_NPC* self, u8 mode)
{
    if (self != 0) {
        self->field_0x255 = mode;
    }
}

/* 0x801FF60C - re-applies every live NPC's dressing. */
void lb_npc_dress_refresh_all(void)
{
    _LB_NPC* npc;
    s32 i;

    for (i = 0, npc = lb_npc; i < 0x12; i++) {
        if (npc->field_0x000 != 0) {
            fn_801FD4BC(npc);
        }
        npc++;
    }
}

/* 0x801FF668 - selects the light of lobby area `area` and refreshes the NPCs under it. */
void lb_npc_area_light_set(u32 area)
{
    u8 i = 0;
    u8* table = lb_area_light_ids;

    for (; table[i] != 0xFF; i++) {
        if ((u8)area == table[i]) {
            lobby_w.field_0x003 = i;
            light_area_select();
            lb_npc_dress_refresh_all();
            return;
        }
    }
}

/* 0x801FF6CC - drops the area light and refreshes the NPCs. */
void lb_npc_area_light_clear(void)
{
    light_area_release();
    lobby_w.field_0x003 = 0xFF;
    lb_npc_dress_refresh_all();
}

s32 fn_801FFE44(_LB_NPC* self);

/* 0x801FFCA0 - the NPC's per-frame update: the act hooks, the scripted move and turn, and the motion table's step
 * counter (a count, -1 for "hold", -2 for "until the motion ends"). */
void lb_npc_frame(_LB_NPC* self)
{
    LbNpcMotionEntry* entry;

    fn_801FDFC0(self, 0);
    lb_npc_talk_reaction_start(self);
    lb_npc_scene_step(self);
    lb_npc_model18_hooks(self);
    lb_npc_cat_farm_follow(self);
    lb_npc_model12_follow(self);
    if ((u32)(self->field_0x002 - 0x3B) <= 2) {
        fn_801FDFC8(self, 1);
    }
    switch (self->model_0x050->flag_0x006) {
    case 1:
        if (self->field_0x004 != lobby_w.field_0x079) {
            self->field_0x001 = 0;
            self->field_0x003 = 0;
            return;
        }
        fn_80207A44(self);
        break;
    default:
        lb_npc_motion_dispatch(self);
        break;
    }
    fn_801FDDF4(self);
    fn_801FDE74(self);
    lb_npc_prop_sync(self);
    if (self->field_0x204 == 0) {
        return;
    }
    entry = &self->field_0x204[self->field_0x208];
    switch (self->field_0x1CC) {
    case 0x03:
    case 0x04:
    case 0x07:
    case 0x0B:
    case 0x0D:
    case 0x13:
    case 0x14:
    case 0x1F:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x28:
    case 0x29:
    case 0x2C:
    case 0x2F:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x3F:
    case 0x41:
    case 0x43:
        break;
    default:
        if (self->field_0x1CC == 5 && entry->count_0x10 == -1) {
            fn_801FFE44(self);
            break;
        }
        if (entry->count_0x10 == -1) {
            break;
        }
        if (entry->count_0x10 == -2) {
            if (++self->field_0x20A > 2 && fn_801FE1EC(self) == 1) {
                fn_801FFE44(self);
            }
        } else if (++self->field_0x20A >= entry->count_0x10) {
            fn_801FFE44(self);
        }
        break;
    }
}

/* 0x801FFE44 - steps the NPC to the next motion table entry (wrapping, and skipping a leading "hold" entry 5),
 * restarts its motion and walks it to the entry's position; 1 when the table wrapped. */
s32 fn_801FFE44(_LB_NPC* self)
{
    VEC3 pos;
    LbNpcMotionEntry* table;
    LbNpcMotionEntry* entry;
    s32 wrapped = 0;

    self->field_0x208++;
    table = self->field_0x204;
    entry = &table[self->field_0x208];
    if (entry->motion_0x0C == 0xFFFF) {
        self->field_0x208 = 0;
        entry = &table[self->field_0x208];
        wrapped = 1;
        if (entry->motion_0x0C == 5 && entry->count_0x10 == -1) {
            self->field_0x208 = 1;
            entry = &table[self->field_0x208];
        }
    }
    if (entry->motion_0x0C != self->field_0x1CC) {
        lb_npc_motion_restart(self, entry->motion_0x0C);
        if ((entry->count_0x10 == -1 || entry->count_0x10 == -2 || entry->motion_0x0C == 0) &&
            entry->pos_0x00.y > -10000.0f) {
            copyVec3(&self->target_0x1EC, em_vec3_copy(&pos, entry));
            fn_801FDD9C(self, 8.0f);
        }
    }
    self->field_0x20A = 0;
    return wrapped;
}

/* 0x801FFF5C - makes `player` the NPC's talk partner, turning back first if it is mid-turn, and restarts motion 7. */
void lb_npc_talk_partner_set(_PLW* player, _LB_NPC* self)
{
    self->field_0x214 = player;
    if (self->field_0x228 == 1) {
        fn_801FDF70(self, 10);
    }
    lb_npc_motion_restart(self, 7);
}

/* 0x801FFFAC - model 14's idle: loop motion 1040 a random 0..7 times, then play 1044 once. */
void lb_npc_idle_model14(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FDFE4(self, 1040, 12, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1044, 8, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1040, 4, 0);
        }
        break;
    }
}

/* 0x802000A8 - model 11's idle: loop 1015 and break into 1016/1017, or (talk mode 1, at random) a longer
 * 1026/1027/1028 cycle. */
void lb_npc_idle_model11(_LB_NPC* self, u32 mode)
{
    switch (self->field_0x006) {
    case 0:
        if ((u8)mode == 1 && ((u16)ran_suu(1) & 1) != 0) {
            self->field_0x006 = 3;
        } else {
            self->field_0x006++;
        }
        self->field_0x00C = (u16)ran_suu(1) % 3;
        fn_801FE0AC(self, 1015, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                if (((u16)ran_suu(1) & 1) != 0) {
                    fn_801FDFE4(self, 1016, 4, 0);
                } else {
                    fn_801FDFE4(self, 1017, 4, 0);
                }
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 0;
            fn_801FDFE4(self, 1015, 4, 0);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                fn_801FDFE4(self, 1026, 4, 0);
                if (((u16)ran_suu(1) & 1) != 0) {
                    self->field_0x006++;
                    self->field_0x00C = (u16)ran_suu(1) % 3;
                } else {
                    self->field_0x006 = 5;
                    self->field_0x00C = (u16)ran_suu(1) % 2 + 3;
                }
            }
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                self->field_0x00C = (u16)ran_suu(1) % 2 + 2;
                fn_801FDFE4(self, 1027, 10, 0);
            }
        }
        break;
    case 5:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1028, 12, 0);
            }
        }
        break;
    case 6:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006 = 0;
                self->field_0x00C = (u16)ran_suu(1) % 3;
                fn_801FDFE4(self, 1015, 4, 0);
            }
        }
        break;
    }
}

/* 0x802003FC - model 15's idle: arm motion 1146 once. */
void lb_npc_idle_model15(_LB_NPC* self)
{
    if (self->field_0x006 == 0) {
        fn_801FE0AC(self, 1146, 4, 0);
        self->field_0x006++;
    }
}

/* 0x8020044C - model 1's idle by talk mode: 1001, 1004, or the 1008 loop broken by 1012. */
void lb_npc_idle_model1(_LB_NPC* self, u32 mode)
{
    switch (self->field_0x006) {
    case 0:
        switch ((u8)mode) {
        case 0:
            fn_801FE0AC(self, 1001, 4, 0);
            break;
        case 1:
            self->field_0x006++;
            self->field_0x00C = (u16)ran_suu(1) & 5;
            fn_801FE0AC(self, 1008, 4, 0);
            break;
        case 2:
            fn_801FE0AC(self, 1004, 4, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1012, 6, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 0;
            fn_801FDFE4(self, 1008, 4, 0);
        }
        break;
    }
}

/* 0x8020057C - model 7's idle: loop 1030, break into 1031. */
void lb_npc_idle_model7(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1030, 20, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1031, 6, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1030, 8, 0);
        }
        break;
    }
}

/* 0x80200678 - model 2's idle by talk mode: the 1048 loop broken by 1059 (mode 0, then 1145) or 1052 (mode 1),
 * or 1053 (mode 2). */
void lb_npc_idle_model2(_LB_NPC* self, u8 mode)
{
    switch (self->field_0x006) {
    case 0:
        switch ((u8)mode) {
        case 0:
        case 1:
            self->field_0x006++;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FE0AC(self, 1048, 4, 0);
            break;
        case 2:
            fn_801FE0AC(self, 1053, 4, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                switch ((u8)mode) {
                case 0:
                    self->field_0x006++;
                    fn_801FDFE4(self, 1059, 6, 0);
                    self->field_0x00C = (u16)ran_suu(1) & 7;
                    break;
                case 1:
                    self->field_0x006 = 3;
                    fn_801FDFE4(self, 1052, 10, 0);
                    break;
                }
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1145, 6, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1048, 4, 0);
        }
        break;
    }
}

/* 0x80200844 - model 3's idle: loop 1060 with nodes 24/25 hidden, break into 1061. */
void lb_npc_idle_model3(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FE0AC(self, 1060, 12, 0);
        fn_801FE200(self, 25, 0);
        fn_801FE200(self, 24, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1061, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 3;
            fn_801FE0AC(self, 1060, 12, 0);
        }
        break;
    }
}

/* 0x80200960 - model 13's idle: loop 1075, break into 1076. */
void lb_npc_idle_model13(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1075, 10, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1076, 10, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1075, 10, 0);
        }
        break;
    }
}

/* 0x80200A5C - model 4 variant 0's idle: loop 1070, then settle into 1072. */
void lb_npc_idle_model4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1070, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                fn_801FE0AC(self, 1072, 2, 0);
            }
        }
        break;
    }
}

/* 0x80200B04 - model 4 variants 1/2's idle by talk mode: the 1070 loop broken by 1158, or 1171. */
void lb_npc_idle_model4_variant(_LB_NPC* self, u32 mode)
{
    switch (self->field_0x006) {
    case 0:
        switch ((u8)mode) {
        case 0:
            self->field_0x006++;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FE0AC(self, 1070, 4, 0);
            break;
        case 1:
            fn_801FE0AC(self, 1171, 4, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1158, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1070, 4, 0);
        }
        break;
    }
}

/* 0x80200C2C - model 5's idle: loop 1156 broken by 1157 (talk mode 0), or hold 1156. */
void lb_npc_idle_model5(_LB_NPC* self, u32 mode)
{
    switch (self->field_0x006) {
    case 0:
        if ((u8)mode == 0) {
            self->field_0x006++;
            self->field_0x00C = (u16)ran_suu(1) & 6;
        } else {
            self->field_0x006 = 3;
        }
        fn_801FE0AC(self, 1156, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1157, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 6;
            fn_801FE0AC(self, 1156, 4, 0);
        }
        break;
    }
}

/* 0x80200D40 - NPC 0x35's idle: arm motion 1179 once. */
void lb_npc_idle_npc53(_LB_NPC* self)
{
    if (self->field_0x006 == 0) {
        self->field_0x006++;
        fn_801FE0AC(self, 1179, 2, 0);
    }
}

/* 0x80200D68 - model 16's idle: loop 1137, break into 1138. */
void lb_npc_idle_model16(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1137, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1138, 12, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1137, 4, 0);
        }
        break;
    }
}

/* 0x80200E64 - model 12's idle: one motion per NPC (0x0A..0x0C) and talk mode. */
void lb_npc_idle_model12(_LB_NPC* self, u32 mode)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    switch ((u8)mode) {
    case 0:
        switch (self->field_0x002) {
        case 10:
            fn_801FE0AC(self, 1152, 6, 0);
            break;
        case 11:
            fn_801FE0AC(self, 1153, 6, 0);
            break;
        case 12:
            fn_801FE0AC(self, 1154, 6, 0);
            break;
        }
        break;
    case 1:
        if ((u32)(self->field_0x002 - 11) > 1) {
            switch (self->field_0x002) {
            case 10:
                fn_801FE0AC(self, 1151, 8, 0);
                break;
            }
        } else {
            fn_801FE0AC(self, 1168, 8, 0);
        }
        break;
    }
}

/* 0x80200F1C - model 6's idle: loop 1110, break into 1112. */
void lb_npc_idle_model6(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1110, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1112, 2, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1110, 4, 0);
        }
        break;
    }
}

/* 0x80201018 - model 20's idle: loop 1115, break into 1121. */
void lb_npc_idle_model20(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 10;
        fn_801FE0AC(self, 1115, 16, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (--self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1121, 6, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 10;
            fn_801FDFE4(self, 1115, 16, 0);
        }
        break;
    }
}

/* 0x80201114 - the NPC's idle by model kind; the kinds with no machine arm one looping motion. */
void lb_npc_idle(_LB_NPC* self, u32 mode)
{
    if (lb_npc_id_ck(self, 14) == 1) {
        lb_npc_idle_model14(self);
        return;
    }
    if (lb_npc_id_ck(self, 15) == 1) {
        lb_npc_idle_model15(self);
        return;
    }
    if (lb_npc_id_ck(self, 1) == 1) {
        lb_npc_idle_model1(self, (u8)mode);
        return;
    }
    if (lb_npc_id_ck(self, 11) == 1) {
        lb_npc_idle_model11(self, (u8)mode);
        return;
    }
    if (lb_npc_id_ck(self, 7) == 1) {
        lb_npc_idle_model7(self);
        return;
    }
    if (lb_npc_id_ck(self, 2) == 1) {
        lb_npc_idle_model2(self, (u8)mode);
        return;
    }
    if (lb_npc_id_ck(self, 3) == 1) {
        lb_npc_idle_model3(self);
        return;
    }
    if (lb_npc_id_ck(self, 13) == 1) {
        lb_npc_idle_model13(self);
        return;
    }
    if (fn_801FE4A0(self) == 1) {
        lb_npc_idle_npc53(self);
    } else if (lb_npc_id_ck(self, 16) == 1) {
        lb_npc_idle_model16(self);
        return;
    } else if (fn_801FE348(self) == 1) {
        lb_npc_idle_model4(self);
        return;
    } else if (fn_801FE39C(self) == 1) {
        lb_npc_idle_model4_variant(self, (u8)mode);
        return;
    } else if (lb_npc_id_ck(self, 5) == 1) {
        lb_npc_idle_model5(self, (u8)mode);
        return;
    } else if (lb_npc_id_ck(self, 12) == 1) {
        lb_npc_idle_model12(self, (u8)mode);
        return;
    } else if (lb_npc_id_ck(self, 6) == 1) {
        lb_npc_idle_model6(self);
        return;
    } else if (lb_npc_id_ck(self, 20) == 1) {
        lb_npc_idle_model20(self);
    }
    if (self->field_0x006 == 0) {
        self->field_0x006++;
        if (lb_npc_id_ck(self, 4) == 1) {
            fn_801FE0AC(self, 1070, 4, 0);
        } else if (lb_npc_id_ck(self, 10) == 1 || lb_npc_id_ck(self, 9) == 1) {
            fn_801FE0AC(self, 1045, 16, 0);
        } else if (lb_npc_id_ck(self, 18) == 1) {
            fn_801FE0AC(self, 1090, 8, 0);
        } else if (lb_npc_id_ck(self, 19) == 1) {
            fn_801FE0AC(self, 1115, 16, 0);
        } else if (lb_npc_id_ck(self, 17) == 1) {
            fn_801FE0AC(self, 63, 4, 0);
        } else {
            fn_801FE0AC(self, 1, 12, 0);
        }
    }
}

/* 0x80201464 - the NPC's walk leg: arm the kind's walk motion, then each frame turn towards the motion table's
 * point and step on to the next entry once within 30 units. */
void lb_npc_walk_leg(_LB_NPC* self)
{
    VEC3 target;
    VEC3 step;
    s32 divisor;
    u32 spun;
    f32 dist;

    VEC3_ctor(&target);
    VEC3_ctor(&step);
    divisor = 10;
    spun = 0;
    if (self->field_0x006 == 0) {
        self->field_0x006++;
        switch (self->field_0x1CC) {
        case 3:
            if (lb_npc_id_ck(self, 10) == 1 || lb_npc_id_ck(self, 9) == 1) {
                fn_801FE0AC(self, 1046, 4, 0);
            } else if (lb_npc_id_ck(self, 13) == 1) {
                fn_801FE0AC(self, 1079, 10, 0);
            } else if (lb_npc_id_ck(self, 19) == 1 || lb_npc_id_ck(self, 20) == 1) {
                fn_801FE0AC(self, 1116, 4, 0);
            } else if (lb_npc_id_ck(self, 17) == 1) {
                fn_801FE0AC(self, 64, 4, 0);
            } else if (lb_npc_id_ck(self, 7) == 1) {
                fn_801FE0AC(self, 1037, 4, 0);
            } else if (lb_npc_id_ck(self, 4) == 1) {
                fn_801FE0AC(self, 1162, 4, 0);
            } else if (lb_npc_id_ck(self, 18) == 1) {
                fn_801FE0AC(self, 1091, 4, 0);
            } else {
                fn_801FE0AC(self, 2, 4, 0);
            }
            break;
        case 4:
            if (lb_npc_id_ck(self, 13) == 1) {
                fn_801FE0AC(self, 1084, 10, 0);
            } else if (lb_npc_id_ck(self, 18) == 1) {
                fn_801FE0AC(self, 1092, 4, 0);
            } else if (lb_npc_id_ck(self, 19) == 1 || lb_npc_id_ck(self, 20) == 1) {
                fn_801FE0AC(self, 1117, 4, 0);
            } else {
                fn_801FE0AC(self, 3, 20, 0);
            }
            break;
        case 34:
            fn_801FDFE4(self, 1192, 0, 0);
            break;
        case 41:
            fn_801FDFE4(self, 1190, 0, 0);
            break;
        case 53:
        case 54:
            fn_801FDFE4(self, 1106, 4, 0);
            fn_801FE1F8(self);
            fn_801FDFC8(self, 1);
            break;
        case 63:
            fn_801FDFE4(self, 52, 4, 0);
            break;
        }
        return;
    }
    switch (self->field_0x1CC) {
    case 4:
        divisor = 2;
        break;
    case 34:
        if (fn_801FE1EC(self) == 1) {
            if (((u16)ran_suu(1) & 1) != 0) {
                lb_npc_motion_restart(self, 36);
            } else {
                lb_npc_motion_restart(self, 35);
            }
        }
        break;
    case 41:
        if (lb_npc_motion_play(self, 1, 40.0f, 0.0f) != 0) {
            LbNpcMotionEntry* entry;
            s32 angle;

            self->field_0x02C += 0x200;
            entry = &self->field_0x204[self->field_0x208];
            vec_to_mh_vec3(&self->target_0x1EC, (Vec*)entry);
            vec_to_mh_vec3(&target, (Vec*)entry);
            target.y = self->pos_0x10.y;
            angle = calcVecAng2(&self->pos_0x10, &target);
            setVector3(&step, 0.0f, 0.0f, 4.0f);
            rotVecY(&step, angle);
            addVec3To(&self->pos_0x10, &step);
        }
        spun = 1;
        break;
    case 53:
        divisor = 4;
        self->field_0x22C = 1;
        fn_801FDFC8(self, 1);
        break;
    case 54:
        divisor = 4;
        fn_801FDFC8(self, 1);
        break;
    }
    if (spun != 1) {
        LbNpcMotionEntry* entry = &self->field_0x204[self->field_0x208];
        s32 turn;

        vec_to_mh_vec3(&self->target_0x1EC, (Vec*)entry);
        vec_to_mh_vec3(&target, (Vec*)entry);
        target.y = self->pos_0x10.y;
        turn = (s16)(u16)fn_801FDE3C(self, &target) / divisor;
        self->field_0x1D4 = turn;
        self->field_0x02C += turn;
    }
    dist = vec3_dist_sq(&self->pos_0x10, &target);
    if (dist < 0.0f) {
        dist *= -1.0f;
    }
    if (dist <= 900.0f) {
        fn_801FFE44(self);
    }
}

/* 0x80201980 - the NPC's turn-in-place leg: turn to the entry's stored angle over its count, then step on. */
void lb_npc_turn_leg(_LB_NPC* self)
{
    fn_801FE128(self, 1);
    if (self->field_0x006 == 0) {
        LbNpcMotionEntry* entry;

        self->field_0x006++;
        entry = &self->field_0x204[self->field_0x208];
        fn_801FDEB4(self, (u32)entry->pos_0x00.x, (s16)entry->count_0x10);
        fn_801FFE44(self);
    }
}

/* 0x80201A00 - the NPC's warp leg: arm motion 1, jump to the entry's point and take its angle. */
void lb_npc_warp_leg(_LB_NPC* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    if (self->field_0x006 == 0) {
        LbNpcMotionEntry* entry;

        self->field_0x006++;
        fn_801FE0AC(self, 1, 4, 0);
        entry = &self->field_0x204[self->field_0x208];
        vec_to_mh_vec3(&pos, (Vec*)entry);
        copyVec3(&self->pos_0x10, copyVec3(&self->target_0x1EC, &pos));
        self->field_0x1D4 = self->field_0x02C = entry->motion_0x0C;
    }
}

/* 0x80201AA0 - the NPC's greeting: turn to the player when he stands off to the side, play the greeting motion
 * (1005 after 1004, 1003 after 1001, else 1009), then turn back and resume the table. */
void fn_80201AA0(_LB_NPC* self, s32 flag)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (lb_npc_Get_motion_no(self) == 1004) {
            self->field_0x007 = 1;
            fn_801FDEE4(self, 10);
            fn_801FE0AC(self, 1005, 4, 0);
            break;
        }
        if ((u32)((u16)(self->field_0x02C - calcVecAng2(&self->pos_0x10, &self->field_0x214->vec_0x03C)) - 0x1601) <=
            0xD3FE) {
            fn_801FDEE4(self, 10);
            self->field_0x007 = 1;
        }
        if (lb_npc_Get_motion_no(self) == 1001) {
            fn_801FE0AC(self, 1003, 6, 0);
        } else {
            fn_801FE0AC(self, 1009, 6, 0);
        }
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            if (self->field_0x007 == 1) {
                fn_801FDF70(self, 10);
            }
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80201BE4 - the NPC's nod: turn to the player, play 1043, turn back and resume the table. */
void fn_80201BE4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FE0AC(self, 1043, 4, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ===================================================================================================
 * The NPC talk reactions (0x80201C80..0x802029B4): each turns to the player, plays the reaction motions and resumes
 * the motion table.
 */
namespace s_801FBF78 {
extern "C" {

/* 0x80201C80 - the reaction of the NPC that sits: stand up turning towards the player (1020/1029 by side, 1018
 * from the front), hold 1021, then sit back down (1022) and turn back to its fixed angle. */
void fn_80201C80(_LB_NPC* self)
{
    VEC3 unused;
    u16 angle;

    VEC3_ctor(&unused);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        angle = self->field_0x02C - calcVecAng2(&self->pos_0x10, &self->field_0x214->vec_0x03C);
        if (lb_npc_Get_motion_no(self) == 1021 || lb_npc_Get_motion_no(self) == 1022) {
            fn_801FDFE4(self, 1021, 4, 0);
            fn_801FDEE4(self, 10);
        } else if ((u32)(angle - 0x4001) <= 0x7FFE) {
            if (angle < 0x8000) {
                fn_801FDFE4(self, 1020, 6, 0);
            } else {
                fn_801FDFE4(self, 1029, 6, 0);
            }
            self->field_0x007 = 1;
        } else {
            fn_801FDFE4(self, 1018, 6, 0);
            self->field_0x006 = 2;
        }
        break;
    case 1:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1 || fn_801FDFD0(self) == 1) {
            fn_801FE0AC(self, 1021, 4, 0);
            self->field_0x006 = 3;
        } else if (self->field_0x007 == 1 && lb_npc_motion_play(self, 0, 62.0f, 0.0f) != 0) {
            fn_801FDEE4(self, 23);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDFE4(self, 1019, 6, 0);
            self->field_0x006 = 4;
        }
        break;
    case 3:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDFE4(self, 1022, 6, 0);
            self->field_0x006++;
        }
        break;
    case 4:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        } else if (lb_npc_motion_play(self, 0, 42.0f, 0.0f) != 0 && lb_npc_Get_motion_no(self) == 1022) {
            u16 diff;

            self->field_0x218 = 0xD8E5;
            diff = self->field_0x02C - self->field_0x218;
            self->field_0x1D4 = diff;
            self->field_0x1E8 = (u16)-(diff / 23U);
            self->field_0x1E4 = 23;
            self->field_0x229 = 1;
        }
        break;
    }
}

/* 0x80201F60 - the reaction of model 2's NPC: 1049, 1050, 1051 in turn (1054 after 1053, turning to the player). */
void fn_80201F60(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        if (lb_npc_Get_motion_no(self) == 1053) {
            self->field_0x006 = 4;
            fn_801FDEE4(self, 10);
            fn_801FDFE4(self, 1054, 8, 0);
        } else {
            self->field_0x006++;
            fn_801FDFE4(self, 1049, 8, 0);
        }
        break;
    case 1:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1050, 2, 0);
            self->field_0x006++;
        }
        break;
    case 2:
        fn_801FDFC0(self, 0);
        if (fn_801FDFD0(self) == 1) {
            fn_801FDFE4(self, 1051, 2, 0);
            self->field_0x006++;
        }
        break;
    case 3:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    case 4:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802020FC - the reaction of model 13's NPC: 1087/1089/1085 after its idle poses, else 1077 then 1078. */
void fn_802020FC(_LB_NPC* self)
{
    fn_801FE128(self, 1);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (lb_npc_Get_motion_no(self) == 1086) {
            fn_801FDFE4(self, 1087, 6, 0);
            self->field_0x006 = 4;
        } else if (lb_npc_Get_motion_no(self) == 1088) {
            fn_801FDFE4(self, 1089, 8, 0);
            fn_801FDEE4(self, 10);
            self->field_0x007 = 1;
        } else if (lb_npc_Get_motion_no(self) == 1084) {
            fn_801FDFE4(self, 1085, 4, 0);
            self->field_0x007 = 1;
            fn_801FDEE4(self, 5);
        } else {
            fn_801FDFE4(self, 1077, 6, 0);
            self->field_0x006 = 2;
            fn_801FDEE4(self, 10);
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1 || fn_801FDFD0(self) == 1) {
            fn_801FDFE4(self, 1077, 6, 74);
            self->field_0x006 = 4;
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDFE4(self, 1078, 6, 0);
            self->field_0x006++;
        }
        break;
    case 3:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    case 4:
        if (fn_801FDFD0(self) == 1) {
            if (self->field_0x007 == 1) {
                fn_801FDF70(self, 10);
                self->field_0x006++;
            } else {
                lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
            }
        }
        break;
    case 5:
        fn_801FDFC0(self, 0);
        if (self->field_0x229 == 0) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202394 - the reaction of model 4 variant 0's NPC: 1073 out of its settled pose, else 1071. */
void fn_80202394(_LB_NPC* self)
{
    VEC3 unused;

    VEC3_ctor(&unused);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (lb_npc_Get_motion_no(self) == 1072 && lb_npc_motion_play(self, 1, 60.0f, 0.0f) != 0) {
            fn_801FDFE4(self, 1073, 6, 0);
        } else {
            fn_801FDFE4(self, 1071, 4, 0);
            self->field_0x006 = 2;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1 || fn_801FDFD0(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1071, 4, 0);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802024C4 - the reaction of model 4 variants 1/2's NPC: turn to the player and play 1071. */
void fn_802024C4(_LB_NPC* self)
{
    VEC3 unused;

    VEC3_ctor(&unused);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1071, 4, 0);
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202570 - the reaction of model 3's NPC: 1066, 1067, 1068, showing node 25 between frames 92 of each. */
void fn_80202570(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1066, 4, 0);
        fn_801FE200(self, 25, 0);
        fn_801FE200(self, 24, 0);
        break;
    case 1:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1 || fn_801FDFD0(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1067, 4, 0);
        } else if (lb_npc_motion_play(self, 0, 92.0f, 0.0f) != 0) {
            fn_801FE200(self, 25, 1);
        }
        break;
    case 2:
        fn_801FDFC0(self, 0);
        if (fn_801FDFD0(self) == 1) {
            fn_801FE200(self, 25, 1);
            fn_801FDFE4(self, 1068, 4, 0);
            self->field_0x006++;
        }
        break;
    case 3:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        } else if (lb_npc_motion_play(self, 0, 92.0f, 0.0f) != 0) {
            fn_801FE200(self, 25, 0);
        }
        break;
    }
}

/* 0x80202728 - the reaction of model 3's NPC while busy: 1062 with nodes 24/25 hidden. */
void fn_80202728(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1062, 16, 0);
        fn_801FE200(self, 25, 0);
        fn_801FE200(self, 24, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802027CC - the reaction of model 7's NPC by talk mode: 1033 then 1034 (swapping nodes 22/23 on frames 62 and
 * 18), or 1032. */
void fn_802027CC(_LB_NPC* self, u32 mode)
{
    switch (self->field_0x006) {
    case 0:
        fn_801FDEE4(self, 10);
        switch ((u8)mode) {
        case 0:
            self->field_0x006++;
            fn_801FDFE4(self, 1033, 4, 0);
            break;
        case 1:
            self->field_0x006 = 3;
            fn_801FDFE4(self, 1032, 6, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDFE4(self, 1034, 4, 0);
            self->field_0x006++;
        } else if (lb_npc_motion_play(self, 0, 62.0f, 0.0f) != 0) {
            fn_801FE200(self, 22, 0);
            fn_801FE200(self, 23, 1);
        }
        break;
    case 2:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        } else if (lb_npc_motion_play(self, 0, 18.0f, 0.0f) != 0) {
            fn_801FE200(self, 23, 0);
            fn_801FE200(self, 22, 1);
        }
        break;
    case 3:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ===================================================================================================
 * The NPC slot setup and per-frame slot update (0x801FCBC8, 0x801FD338..0x801FDD9C).
 */
#include "camera/camera.h"
#include "fn_8004CAD8/get_qResult_work.h"
#include "quest/quest_result_work.h"
#include "mh3_pad/lb_param_w.h"
u32 event_demo_ck(void);
u32 LbCheckKujiraEvent(void);
namespace s_801FBF78 {

extern "C" {
u32 game_ready_ck(void);
void lb_npc_motion_table_start(_LB_NPC* self);
void lb_npc_gate_place(_LB_NPC* self);
void lb_npc_place_beside_player(_LB_NPC* self);
u32 lb_npc_on_screen_ck(_LB_NPC* self);
u32 lb_npc_view_range_ck(_LB_NPC* self);
u8 lb_world_mode_get(void);
u32 lb_npc_talk_mark_ck(_LB_NPC* self);
u32 kujira_event_over_ck(void);
u32 fn_80207EAC(_LB_NPC* self);
void fn_80207FD0(_LB_NPC* self, u8 kind, s32 flag);
u32 fn_802080C0(_LB_NPC* self);
void lb_npc_talk_select(_LB_NPC* self);
void lb_npc_char_load(LbNpcChar* chr);
void lb_npc_char_move(LbNpcChar* chr);
void lb_quest_board_follow_spawn(_LB_NPC* self);
void addVec3(VEC3* out, const VEC3* a, const VEC3* b);
VEC3* setVec3(VEC3* v, f32 x, f32 y, f32 z);
void word_copy_return_dst(u32* dst, u32* src);
f32 vec3_distance(VEC3* a, VEC3* b);
extern LbNpcMoveTables move_npc_tbl_ptr;    /* .bss: the NPC motion tables by NPC, area and event */
extern LbNpcEventSets npc_event_set_data;   /* .sbss: the event motion-table overrides */
extern f32 lb_npc_view_dist;                /* .sdata: the distance an NPC stays drawn within */
}

extern "C" {

/* 0x801FCBC8 - picks the NPC's motion table: the running event's override, else by NPC id (and area, talk state
 * and lobby layout for the NPCs that have several). */
void lb_npc_motion_table_select(_LB_NPC* self)
{
    u8 event;
    u32 from_event = 0;
    LbNpcEventSet* set = 0;

    self->field_0x208 = 0;
    self->field_0x254 = 0;
    event = lb_event_kind_get();
    if (event == 0xFF) {
        if (game_ready_ck() == 0) {
            event = lb_world_mode_get();
        }
        if (event != 0xFF) {
            set = npc_event_set_data.by_mode_0x0[event];
        }
    } else {
        set = npc_event_set_data.by_event_0x4[event];
        from_event = 1;
    }
    if (set != 0) {
        for (; set->npc_0x0 != 0xFF; set++) {
            if (self->field_0x002 == set->npc_0x0) {
                self->field_0x204 = set->table_0x4;
                if (lb_npc_talk_ok_npc14(self->field_0x002, 0) == 0) {
                    self->field_0x004 = 0xFF;
                }
                return;
            }
        }
        if (from_event == 1) {
            self->field_0x254 = 1;
        }
    }
    switch (self->field_0x002) {
    case 0x01:
        self->field_0x204 = move_npc_tbl_ptr.by_area_0x04[self->field_0x004];
        return;
    case 0x02:
        if (lb_npc_talk_mark_ck(self) == 1 || lb_npc_talk_flag_ck(self) == 1) {
            self->field_0x204 = *move_npc_tbl_ptr.npc2_talk_0x30;
        } else {
            self->field_0x204 = move_npc_tbl_ptr.npc2_0x08[lobby_w.field_0x080];
        }
        return;
    case 0x04:
        if (kujira_event_over_ck() == 1) {
            self->field_0x204 = *move_npc_tbl_ptr.npc4_kujira_0x2C;
            return;
        }
        break;
    case 0x09:
        self->field_0x204 = move_npc_tbl_ptr.npc9_0x0C[lobby_w.field_0x080];
        return;
    case 0x0E:
        if (lb_npc_talk_mark_ck(self) == 1 || lb_npc_talk_flag_ck(self) == 1) {
            self->field_0x204 = *move_npc_tbl_ptr.npc14_talk_0x34;
            if (lb_npc_talk_ok_npc14(self->field_0x002, 2) == 0) {
                self->field_0x004 = 0xFF;
            }
        } else {
            self->field_0x204 = move_npc_tbl_ptr.npc14_0x10[lobby_w.field_0x080];
            if (lb_npc_talk_ok_npc14(self->field_0x002, 1) == 0) {
                self->field_0x004 = 0xFF;
            }
        }
        return;
    case 0x0F:
        self->field_0x204 = move_npc_tbl_ptr.npc15_0x14[lobby_w.field_0x080];
        return;
    case 0x17:
        {
            LbNpcMotionEntry* table;

            self->field_0x004 = lobby_w.talk_area_0x055;
            table = lobby_w.npc_motion_0x070;
            if (table == 0) {
                self->field_0x204 = move_npc_tbl_ptr.npc23_0x18[self->field_0x004];
            } else {
                self->field_0x204 = table;
                self->field_0x208 = lobby_w.npc_motion_index_0x074;
                if (table[self->field_0x208].motion_0x0C == 0xFFFF) {
                    self->field_0x208 = 0;
                }
            }
        }
        return;
    case 0x30:
        if (LbCheckKujiraEvent() == 1) {
            self->field_0x204 = move_npc_tbl_ptr.npc48_0x1C[1];
        } else {
            self->field_0x204 = move_npc_tbl_ptr.npc48_0x1C[0];
        }
        return;
    case 0x47:
        switch (self->field_0x004) {
        case 5:
            self->field_0x204 = move_npc_tbl_ptr.npc71_0x20[0];
            break;
        case 6:
            self->field_0x204 = move_npc_tbl_ptr.npc71_0x20[1];
            break;
        case 7:
            self->field_0x204 = move_npc_tbl_ptr.npc71_0x20[2];
            break;
        }
        return;
    case 0x49:
        switch (self->field_0x004) {
        case 5:
            self->field_0x204 = move_npc_tbl_ptr.npc73_0x24[0];
            break;
        case 6:
            self->field_0x204 = move_npc_tbl_ptr.npc73_0x24[1];
            break;
        case 7:
            self->field_0x204 = move_npc_tbl_ptr.npc73_0x24[2];
            break;
        }
        return;
    }
    if (fn_801FE4B4(self) == 1) {
        u32 kind = fn_80207EAC(self);

        self->field_0x234[0] = lobby_w.npc_event_0x07A[(u8)kind];
        if (self->field_0x234[0] != 0xFF) {
            fn_80207FD0(self, kind, 0);
            return;
        }
    }
    if (lb_npc_id_ck(self, 12) == 1) {
        u32 id = fn_802080C0(self);

        if (userdata_quest_slot_open_ck((u8)id) == 1) {
            self->field_0x204 = move_npc_tbl_ptr.model12_0x28[(u8)id];
            return;
        }
    }
    self->field_0x204 = move_npc_tbl_ptr.by_npc_0x00[self->field_0x002];
}

/* 0x801FD338 - fills NPC slot `self` from placement record `data`: build the model, pick the talk table and apply
 * the model variant's dressing. */
void fn_801FD338(_LB_NPC* self, LbNpcModel* data)
{
    self->field_0x002 = data->npc_id_0x000;
    self->model_0x050 = data;
    self->field_0x000 = 1;
    lb_npc_char_load((LbNpcChar*)self->field_0x054);
    lb_npc_talk_select(self);
    if (fn_801FE8CC(self) == 1) {
        lb_npc_dress_model18(self, lobby_w.pig_dress_0x078);
    } else {
        LbNpcModel* model = self->model_0x050;
        u8 variant = model->variant_0x004;

        if (variant != 0xFF) {
            LbNpcSubOp** lists = npc_sub_data[model->model_0x002];

            if (lists != 0) {
                lb_npc_dress_apply(self, lists[variant]);
            }
        }
    }
}

/* 0x801FD3E4 - runs one dressing op list on the NPC's model: hide/show nodes and set material colours. */
void lb_npc_dress_apply(_LB_NPC* self, LbNpcSubOp* op)
{
    MHchar* body = lb_npc_body(self);

    for (; op->op_0x0 != 0xFF; op++) {
        switch (op->op_0x0) {
        case 1: {
            _GXColor color;

            color.r = op->color_0x4 >> 24;
            color.g = (op->color_0x4 >> 16) & 0xFF;
            color.b = (op->color_0x4 >> 8) & 0xFF;
            color.a = 0xFF;
            body->setMatColor(op->node_0x1, GX_COLOR0A0, color, false);
            break;
        }
        case 0:
            body->setVisibility(op->node_0x1, false);
            break;
        case 2:
            body->setVisibility(op->node_0x1, true);
            break;
        }
    }
}

/* 0x801FD4BC - (re)activates the NPC in the current area: pick its motion table, place it, and set its model's
 * render kind (an NPC in the player's group follows the lobby area). */
void fn_801FD4BC(_LB_NPC* self)
{
    self->field_0x005 = 1;
    self->field_0x001 = 1;
    self->field_0x003 = 1;
    self->field_0x1DC = 1.0f;
    self->field_0x004 = lobby_wp[2];
    lb_npc_motion_table_select(self);
    lb_npc_motion_table_start(self);
    self->field_0x228 = 0;
    self->field_0x229 = 0;
    switch (self->model_0x050->flag_0x006) {
    case 0:
        self->field_0x210 = 4;
        break;
    case 1:
        if ((lb_param_w.entry_0x08 & 0x80) != 0) {
            lobby_w.field_0x079 = lobby_w.field_0x002;
        }
        self->field_0x210 = 3;
        break;
    }
    self->field_0x20C = lb_npc_view_dist;
    self->field_0x255 = 0xFF;
    self->field_0x256 = 0;
}

/* 0x801FD594 - spawns the NPC's attached effect: NPC 2's steam, NPC 17's follower. */
void fn_801FD594(_LB_NPC* self)
{
    VEC3 offset;

    VEC3_ctor(&offset);
    switch (self->field_0x002) {
    case 2:
        setVector3(&offset, -20.0f, 25.0f, 10.0f);
        lb_quest_board_effect_spawn(self, 5, 12, &offset, 1.0f);
        break;
    case 17:
        lb_quest_board_follow_spawn(self);
        break;
    }
}

/* 0x801FD614 - places NPC 1 for the quest return: at the gate (by the session's lobby kind) after an online
 * quest, at the counter after a cleared offline one, else beside the player. */
void lb_npc_gate_place(_LB_NPC* self)
{
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    VEC3 offset;
    struct Q_ResultWork* result;

    plw += my_player_no();
    VEC3_ctor(&offset);
    result = get_qResult_work();
    if (system_w.field_0x02D != 0) {
        if (system_w.field_0x7CE == 2) {
            VEC3 pos;

            copyVec3(&self->pos_0x10, setVec3(&pos, 676.0f, 459.0f, 154.0f));
        } else {
            VEC3 pos;

            copyVec3(&self->pos_0x10, setVec3(&pos, 1700.0f, 461.0f, -451.0f));
        }
        self->field_0x028 = 0;
        self->field_0x02C = 0xC001;
        self->field_0x030 = 0;
    } else if (result != 0 && result->kind_0x1E3 == 1 && self->field_0x004 == 0 && lobby_w.field_0x12C == 1) {
        VEC3 pos;

        copyVec3(&self->pos_0x10, setVec3(&pos, -1007.0f, 179.0f, -391.0f));
        self->field_0x028 = 0;
        self->field_0x02C = 0x4000;
        self->field_0x030 = 0;
    } else {
        VEC3 pos;

        setVector3(&offset, -40.0f, 0.0f, -60.0f);
        rotVecY(&offset, plw->field_0x058);
        addVec3(&pos, &plw->vec_0x03C, &offset);
        copyVec3(&self->pos_0x10, &pos);
        eft_rot_vec_copy(&self->rot_0x028, &plw->rot_0x54);
    }
    copyVec3(&self->field_0x01C, &self->pos_0x10);
}

/* 0x801FD7AC - places the NPC beside the player, facing his way. */
void lb_npc_place_beside_player(_LB_NPC* self)
{
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    VEC3 offset;
    VEC3 pos;

    plw += my_player_no();
    VEC3_ctor(&offset);
    setVector3(&offset, 40.0f, 0.0f, -60.0f);
    rotVecY(&offset, plw->field_0x058);
    addVec3(&pos, &plw->vec_0x03C, &offset);
    copyVec3(&self->pos_0x10, &pos);
    eft_rot_vec_copy(&self->rot_0x028, &plw->rot_0x54);
}

/* 0x801FD848 - puts the NPC at its motion table's start (a leading "hold" entry 5 gives the angle and the next
 * entry the place; NPC 1 back from a quest stands at the gate) and restarts the entry's motion. */
void lb_npc_motion_table_start(_LB_NPC* self)
{
    LbNpcMotionEntry* entry = &self->field_0x204[self->field_0x208];

    if (entry->motion_0x0C == 5 && entry->count_0x10 == -1) {
        if (self->field_0x002 == 1 && (lb_param_w.entry_0x08 & 0x80) != 0) {
            lb_npc_gate_place(self);
            fn_801FDFA4(self, lb_param_w.entry_0x08);
            self->field_0x208++;
        } else {
            self->field_0x028 = 0;
            self->field_0x02C = (u32)self->field_0x204->pos_0x00.x;
            self->field_0x030 = 0;
            self->field_0x208++;
            vec_to_mh_vec3(&self->pos_0x10, (Vec*)&self->field_0x204[self->field_0x208]);
        }
    } else {
        self->field_0x028 = 0;
        self->field_0x02C = 0;
        self->field_0x030 = 0;
        vec_to_mh_vec3(&self->pos_0x10, (Vec*)self->field_0x204);
    }
    self->field_0x218 = self->field_0x02C;
    if (self->field_0x002 == 0x17) {
        if (lobby_w.field_0x076 == 1) {
            lb_npc_place_beside_player(self);
            lb_npc_motion_restart(self, 55);
            return;
        }
        if (lobby_w.npc_motion_0x070 != 0) {
            copyVec3(&self->pos_0x10, &lobby_w.npc_pos_0x058);
            eft_rot_vec_copy(&self->rot_0x028, &lobby_w.npc_rot_0x064);
        }
    }
    lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
}

/* 0x801FD9C4 - true while the NPC is within its view distance of the camera (always during a demo or for a
 * record flagged 0x08). */
u32 lb_npc_view_range_ck(_LB_NPC* self)
{
    VEC3 cam;

    VEC3_ctor(&cam);
    if (event_demo_ck() == 1) {
        return 1;
    }
    if ((self->model_0x050->flags_0x005 & 8) != 0) {
        return 1;
    }
    copyVec3(&cam, &get_camera_pos());
    return !(vec3_distance(&self->pos_0x10, &cam) >= self->field_0x20C);
}

/* 0x801FDA60 - true while the NPC's head projects inside the screen (always during a demo or for a record flagged
 * 0x08). */
u32 lb_npc_on_screen_ck(_LB_NPC* self)
{
    VEC3 screen;
    VEC3 pos;
    u32 camera;
    u32 root;
    f32 bottom;
    f32 top;

    VEC3_ctor(&screen);
    VEC3_ctor(&pos);
    root = scn_root_get_current_camera(pRoot);
    word_copy_return_dst(&camera, &root);
    if (event_demo_ck() == 1) {
        return 1;
    }
    if ((self->model_0x050->flags_0x005 & 8) != 0) {
        return 1;
    }
    copyVec3(&pos, &self->pos_0x10);
    pos.y += lb_npc_model_inscreen_get(self->model_0x050->model_0x002);
    camera_world_to_screen(&camera, &screen, &pos);
    bottom = 500.0f;
    top = -20.0f;
    if (camera_mode_cur_get() == 1 || camera_mode_cur_get() == 2) {
        bottom += 100.0f;
        top -= 100.0f;
    }
    if (screen.x >= 700.0f || screen.x <= -60.0f || screen.y >= bottom || screen.y <= top || screen.z <= 0.0f) {
        return 0;
    }
    return 1;
}

/* 0x801FDBD0 - the NPC's per-frame slot update: decide whether it is shown in this area, run its update, then
 * move its model to it and settle it on the ground. */
void fn_801FDBD0(_LB_NPC* self)
{
    MHchar* body = lb_npc_body(self);

    if (self->field_0x004 != lobby_wp[2]) {
        self->field_0x001 = 0;
        return;
    }
    if (self->field_0x254 == 1) {
        self->field_0x001 = 0;
        return;
    }
    if (lb_npc_on_screen_ck(self) == 0) {
        self->field_0x001 = 0;
    } else if (lb_npc_view_range_ck(self) == 0) {
        self->field_0x001 = 0;
    } else {
        self->field_0x001 = 1;
        self->field_0x003 = 1;
    }
    if (self->field_0x001 == 0 && (self->model_0x050->flags_0x005 & 0x10) == 0) {
        self->field_0x003 = 0;
    }
    self->field_0x22E = 0;
    self->field_0x22B = 0;
    self->field_0x22D = 0;
    self->field_0x22C = 0;
    self->field_0x256 = 0;
    copyVec3(&self->field_0x01C, &self->pos_0x10);
    lb_npc_frame(self);
    if (self->field_0x003 != 0) {
        body->field_0x28 = self->field_0x028;
        body->field_0x2C = self->field_0x02C;
        body->field_0x30 = self->field_0x030;
        copyVec3(&body->pos_0x04, &self->pos_0x10);
        lb_npc_char_move((LbNpcChar*)self->field_0x054);
        copyVec3(&self->pos_0x10, &body->pos_0x04);
        if ((self->model_0x050->flags_0x005 & 0x20) != 0 || self->field_0x22C == 1) {
            lb_npc_ground_drop(self);
        }
        if ((self->field_0x204[self->field_0x208].count_0x10 != -1 && self->field_0x22E == 0) ||
            self->field_0x22D == 1) {
            lb_npc_ground_snap(self);
        }
        copyVec3(&body->pos_0x04, &self->pos_0x10);
        mhchar_model_mtx_update(body);
        lb_npc_motion_effects(self);
        lb_npc_npc23_state_save(self);
    }
}

} /* extern "C" */
} /* namespace s_801FBF78 */

/* ===================================================================================================
 * The per-frame world-update group (0x801FC6EC..0x801FCB68).
 */
void prim_init_all(void);
void player_control_move(void);
void light_move(void);
struct _g3d_work;
void push_g3d_wk(struct _g3d_work* work);
u8 get_now_mapno(void);
void* ckResourceName(s8* name);
namespace s_801FBF78 {


extern "C" {
void fn_80212370(void);
void fn_8021261C(void);
void demo_sound_update(void);
void lobby_net_update(void);
void ainpc_page_reset(void);
void fn_80211E68(void);
void fn_802680C4(void);
void camera_update(void);
void fn_802ADB98(void);
void fn_800F8DA4(void);
void hud_msg_frame(void);
void ai_npc_reaction_forward(void);
void ainpc_page_idle(void);
void menu_work_update(void);
void fn_8021DC24(void);
void fn_80324274(void);
void ainpc_page_update(void);
void fn_801FD0F8(void);
void fn_801FD174(_LB_NPC* self);
void fn_801FD338(_LB_NPC* self, LbNpcModel* data);
u32 fn_802FB600(void);
void em007_npc_model_eft_start(u8 index);
void mhchar_model_release(u8* self);
void fn_800E3358(s32 a, u8* b, u32* res);
void fn_800D5CAC(void* rec);
void* memset(void* dst, s32 c, u32 n);
}

extern "C" {

/* 0x801FC6EC - the lobby's per-frame world update: refresh the systems, then either arm the village
 * transition or run the normal frame's subsystem ticks. */
void lobby_frame_update(void)
{
    prim_init_all();
    fn_80212370();
    fn_8021261C();
    demo_sound_update();
    lobby_w.field_0x027 = lobby_w.field_0x000;
    if (isNetCityMode() == 1) {
        lobby_net_update();
    }
    player_control_move();
    lobby_hidden_flag = 0;
    if (isNetCityMode() == 1 && lobby_state_block[3] == 0 && lobby_w.field_0x16F == 0 &&
        lobby_w.field_0x163 == 0) {
        lobby_w.field_0x12C = 1;
        lobby_w.field_0x172 = 10;
    } else {
        ainpc_page_reset();
        fn_801FD0F8();
        fn_80211E68();
        fn_802680C4();
        camera_update();
        fn_802ADB98();
        light_move();
        fn_800F8DA4();
        hud_msg_frame();
        ai_npc_reaction_forward();
        ainpc_page_idle();
        system_w.field_0x8C0 = 0;
        if (lobby_w.field_0x12C != 1 && (lobby_w.field_0x000 == 0 || lobby_w.field_0x000 == 0x24)) {
            menu_work_update();
        }
        fn_8021DC24();
        fn_80324274();
        ainpc_page_update();
    }
}

/* 0x801FC8F0 - reset the 0x12 NPC slots and refill them from the current map's NPC table. */
void lb_npc_map_setup(void)
{
    s32 i;
    u8* data;
    _LB_NPC* npc;
    s32 count;

    for (i = 0; i < 0x12; i++) {
        fn_801FD174(&lb_npc[i]);
    }
    if (lobby_wp[1] == 0x15) {
        data = (u8*)npc_data_town[lobby_wp[2]];
    } else {
        data = (u8*)npc_data_village[lobby_wp[2]];
    }
    npc = lb_npc;
    count = 0;
    while (data[0] != 0) {
        if (count >= 0x12) {
            break;
        }
        if (fn_802FB600() == 1) {
            fn_801FD338(npc, (LbNpcModel*)data);
            npc++;
            count++;
        }
        data += 8;
    }
    memset(&lb_npc_scene_work, 0, 0x1C);
    if (get_now_mapno() == 0x16) {
        for (i = 0; i < 7; i++) {
            em007_npc_model_eft_start((u8)i);
        }
    }
}

/* 0x801FCA00 - release every live NPC's model slot. */
void lb_npc_model_release_all(void)
{
    _LB_NPC* npc;
    s32 i;

    npc = lb_npc;
    for (i = 0; i < 0x12; i++) {
        if (npc->field_0x000 != 0) {
            if (npc->field_0x164 != 0) {
                push_g3d_wk((struct _g3d_work*)npc->field_0x164);
                npc->field_0x164 = 0;
                npc->field_0x170 = 0;
            }
            mhchar_model_release(npc->body_0x058);
        }
        npc++;
    }
}

/* 0x801FCA80 - queue the `index`-th row of the lobby resource table for loading. */
void fn_801FCA80(u32 index)
{
    LbResRec rec;

    fn_801FCAC8((u32*)&rec, (const u32*)&lbl_80582A68[index]);
    rec.c_0x08 = 0;
    fn_800D5CAC(&rec);
}

/* 0x801FCADC - load the `index`-th resource table entry and hand it to effect id 4. */
void fn_801FCADC(u8* self, u32 index)
{
    u32 dst;
    LbResource* res;

    res_file_ctor(&dst, 0);
    res = ckResourceName((char*)lbl_80582A68[index].b_0x04);
    if (res != 0) {
        u32 src;

        res_file_assign(&dst, res_file_ctor(&src, (u32)res->payload_0x44));
        fn_800E3358(4, self, &dst);
    }
}

/* 0x801FCB68 - the NPC's friendship/level value for a scripted motion id >= 0x3E8. */
s32 fn_801FCB68(u32 id)
{
    u16 value = (u16)(id - 0x3E8);

    if ((u16)id < 0x3E8) {
        return 0;
    }
    return ((s16*)npc_lp_tbl[value / 100])[value % 100];
}

} /* extern "C" */

/* ===================================================================================================
 * The NPC slot update group (0x801FD0F8..0x801FDBD0).
 */
extern "C" {
void fn_80207B28(void);
void fn_801FD4BC(_LB_NPC* self);
void fn_801FD594(_LB_NPC* self);
void fn_801FDBD0(_LB_NPC* self);
void lb_npc_char_bind(LbNpcChar* chr, _LB_NPC* owner);
}

extern "C" {

/* 0x801FD0F8 - tick every live NPC slot. */
void fn_801FD0F8(void)
{
    _LB_NPC* npc;
    s32 i;

    npc = lb_npc;
    fn_80207B28();
    for (i = 0; i < 0x12; i++) {
        if (npc->field_0x000 != 0) {
            if (npc->field_0x005 == 0) {
                fn_801FD4BC(npc);
                fn_801FD594(npc);
            }
            fn_801FDBD0(npc);
        }
        npc++;
    }
}

/* 0x801FD174 - clear one NPC slot back to its reset state. */
void fn_801FD174(_LB_NPC* self)
{
    u16 last = 0xFFFF;

    self->field_0x000 = 0;
    self->field_0x001 = 0;
    self->field_0x002 = 0;
    self->field_0x003 = 0;
    self->field_0x004 = 0xFF;
    self->field_0x005 = 0;
    self->field_0x006 = 0;
    self->field_0x007 = 0;
    self->field_0x008 = 0;
    self->field_0x00C = 0;
    self->pos_0x10.x = lbl_80799880;
    self->pos_0x10.y = lbl_80799880;
    self->pos_0x10.z = lbl_80799880;
    copyVec3(&self->field_0x01C, &self->pos_0x10);
    self->field_0x028 = 0;
    self->field_0x02C = 0;
    self->field_0x030 = 0;
    self->field_0x034 = lbl_80799880;
    setVector3(&self->field_0x038, lbl_80799880, lbl_80799880, lbl_80799880);
    setVector3(&self->field_0x044, lbl_80799880, lbl_80799880, lbl_80799880);
    self->model_0x050 = 0;
    lb_npc_char_bind((LbNpcChar*)self->field_0x054, self);
    self->field_0x1CC = last;
    self->field_0x1CE = 0;
    self->field_0x1D0 = last;
    self->field_0x1D4 = 0;
    self->field_0x1D8 = 0;
    self->field_0x1DC = lbl_80799884;
    self->field_0x1E0 = 0;
    self->field_0x1E2 = 0;
    self->field_0x1E4 = 0;
    self->field_0x1E8 = 0;
    setVector3(&self->target_0x1EC, lbl_80799880, lbl_80799880, lbl_80799880);
    setVector3(&self->vel_0x1F8, lbl_80799880, lbl_80799880, lbl_80799880);
    self->field_0x204 = 0;
    self->field_0x208 = 0;
    self->field_0x20A = 0;
    self->field_0x20C = lbl_80799880;
    self->field_0x210 = 0xFF;
    self->field_0x214 = 0;
    self->field_0x218 = 0;
    self->field_0x21C = 0;
    self->field_0x222 = 0;
    self->field_0x21E = 0;
    self->field_0x223 = 0;
    self->field_0x220 = 0;
    self->field_0x224 = 0;
    self->field_0x225 = 0;
    self->field_0x226 = 0;
    self->field_0x227 = 0;
    self->field_0x228 = 0;
    self->field_0x229 = 0;
    self->field_0x22A = 0;
    self->field_0x22B = 0;
    self->field_0x22C = 0;
    self->field_0x22D = 0;
    self->field_0x22E = 0;
    self->field_0x230 = 0;
    memset(self->field_0x234, 0, 0x20);
    self->field_0x254 = 0;
    self->field_0x255 = 0xFF;
    self->field_0x256 = 0;
    setVector3(&self->field_0x258, lbl_80799880, lbl_80799880, lbl_80799880);
    self->field_0x264 = 0;
}

} /* extern "C" */

} /* namespace s_801FBF78 */

/* The NPC work band (0x802029B4-0x802076D4). */
#include "types.h"
#include "nw4r/math.h"

#include "lobby/lb_npc.h"
#include "unsplit/lobby.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
namespace s_802029B4 {


/* ---------------------------------------------------------------------------------------------------
 * Declarations: the first band's helpers (`lobby/lb_npc.h` publishes `_LB_NPC` but not these prototypes).  Unmangled
 * names are C linkage; the mangled callees are declared at C++ scope (rule 9).
 */
extern "C" {
f32 vec3_distance(VEC3* a, VEC3* b);
void addVec3(VEC3* out, const VEC3* a, const VEC3* b);
void fn_801FDD9C(_LB_NPC* self, f32 duration);
s32 fn_801FDE3C(_LB_NPC* self, VEC3* target);
void fn_801FDEB4(_LB_NPC* self, s32 to, s32 ticks);
void fn_801FDEE4(_LB_NPC* self, s32 frames);
void fn_801FDF70(_LB_NPC* self, s32 frames);
void fn_801FDFC0(_LB_NPC* self, u8 value);
void fn_801FDFC8(_LB_NPC* self, u8 value);
u32 fn_801FDFD0(_LB_NPC* self);
void fn_801FDFE4(_LB_NPC* self, u16 id, s32 a3, s32 a4);
void fn_801FE0AC(_LB_NPC* self, u16 id, s32 a3, s32 a4);
void lb_npc_motion_restart(_LB_NPC* self, u16 id);
u32 lb_npc_motion_play(_LB_NPC* self, u16 id, f32 a, f32 b);
u32 fn_801FE1DC(_LB_NPC* self, u16 id, f32 a, f32 b);
u32 fn_801FE1EC(_LB_NPC* self);
void fn_801FE200(_LB_NPC* self, u8 visible, bool flag);
void fn_801FE128(_LB_NPC* self, u8 value);
u32 lb_npc_id_ck(_LB_NPC* self, u8 kind);
u32 fn_801FE348(_LB_NPC* self);
u32 fn_801FE39C(_LB_NPC* self);
u32 fn_801FE400(_LB_NPC* self);
u32 fn_801FE450(_LB_NPC* self);
u32 fn_801FE4A0(_LB_NPC* self);
u32 fn_801FE4B4(_LB_NPC* self);
u32 fn_801FE55C(_LB_NPC* self);
u32 fn_801FE8CC(_LB_NPC* self);
u8 lb_world_mode_get(void);
u8 fn_802FB948(void);
u8 lb_world_upgrade_get(s32 idx);
void camera_quake_req(s32 id);
void fn_80201AA0(_LB_NPC* self, s32 flag);
void fn_80201BE4(_LB_NPC* self);
void fn_80201C80(_LB_NPC* self);
void fn_80201F60(_LB_NPC* self);
void fn_802020FC(_LB_NPC* self);
void fn_80202394(_LB_NPC* self);
void fn_802024C4(_LB_NPC* self);
void fn_80202570(_LB_NPC* self);
void fn_80202728(_LB_NPC* self);
void fn_802027CC(_LB_NPC* self, s32 action);
void fn_801FFE44(_LB_NPC* self);
void fn_801E9A90(u8 a, u8 b);
s32 fn_80207EAC(_LB_NPC* self);
LbNpcMotionEntry* fn_80207DC4(_LB_NPC* self, u8* kind);
void fn_80207FD0(_LB_NPC* self, u8 kind, s32 flag);
s32 fn_802080C0(_LB_NPC* self);
void lb_quest_board_effect_spawn(_LB_NPC* self, s32 a, s32 b, VEC3* v, f32 f);
}
} /* namespace s_802029B4 */


u16 lb_npc_Get_motion_no(_LB_NPC* self);
s32 ran_suu(s32 n);
s32 calcVecAng2(VEC3* a, VEC3* b);
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
u32 LbCheckKujiraEvent(void);
void* get_move_work_adrs(u8 idx);
namespace s_802029B4 {

extern "C" s32 my_player_no(void);
} /* namespace s_802029B4 */

u8 get_now_mapno(void);
namespace s_802029B4 {


/* ---------------------------------------------------------------------------------------------------
 * Bodies
 */
extern "C" {

/* 0x802029B4 - the NPC's `state` machine 0: arm motion 1147, wait for the 0x47C motion to run out and
 * select one of two follow-up arms, then restart the motion the motion table names. */
__declspec(noinline) void fn_802029B4(_LB_NPC* self)
{
    VEC3 unused;
    VEC3_ctor(&unused);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1147, 4, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            if (lb_npc_motion_play(self, 2, lbl_807999C4, lbl_807999A0) == 1) {
                fn_801FDFE4(self, 1148, 12, 50);
            } else {
                fn_801FDFE4(self, 1148, 8, 0);
            }
            self->field_0x006++;
        }
        break;
    case 2:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202AC4 - arm 1167 off motion 1151, or 1169 off motion 1168, then wait for the flag and restart
 * the table's motion. */
__declspec(noinline) void fn_80202AC4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (lb_npc_Get_motion_no(self) == 1151) {
            fn_801FDFE4(self, 1167, 4, 0);
        } else if (lb_npc_Get_motion_no(self) == 1168) {
            fn_801FDFE4(self, 1169, 4, 0);
        }
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202B88 - arm 1163 over 16 frames plus motion 10, then wait for the flag, advance motion 10 and
 * restart the table's motion. */
__declspec(noinline) void fn_80202B88(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1163, 16, 0);
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202C24 - play motion 10, arm 1130; on motion 1 arm 1129 over 16 frames and 24 frames of delay,
 * else step on once the flag is up; then wait for the flag, advance motion 10 and restart the table's. */
__declspec(noinline) void fn_80202C24(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1130, 4, 0);
        break;
    case 1:
        if (lb_npc_motion_play(self, 1, lbl_807999C8, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 16, 24);
        } else if (fn_801FDFD0(self) == 1) {
            self->field_0x006++;
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202D24 - play motion 10 and arm 1131, then wait for the flag, advance motion 10 and restart the
 * table's motion. */
__declspec(noinline) void fn_80202D24(_LB_NPC* self)
{
    VEC3 unused;
    VEC3_ctor(&unused);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1131, 8, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202DD0 - play motion 10 and arm 1129 over 8 frames, then wait for the flag, advance motion 10 and
 * restart the table's motion. */
__declspec(noinline) void fn_80202DD0(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1129, 8, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202E6C - arm 1180 over 2 frames, then wait for the flag and restart the table's motion. */
__declspec(noinline) void fn_80202E6C(_LB_NPC* self)
{
    VEC3 unused;
    VEC3_ctor(&unused);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1180, 2, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80202F00 - the 4-arm motion machine: arm 1092 (1188 with motion 37 up), wait for the stop, then walk the last leg
 * (a 20-tick turn onto the motion's vector) or hand the motion off once the lobby is idle. */
__declspec(noinline) void fn_80202F00(_LB_NPC* self)
{
    VEC3 offset;
    VEC3 delta;

    VEC3_ctor(&offset);
    self->field_0x22D = 1;
    self->field_0x22C = 1;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        if (self->field_0x1D0 == 37) {
            fn_801FDFE4(self, 1188, 4, 0);
        } else {
            fn_801FE0AC(self, 1092, 4, 0);
            self->field_0x006 = 2;
        }
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FE0AC(self, 1092, 4, 0);
        }
        break;
    case 2:
        if (self->field_0x1E4 <= 0) {
            self->field_0x006++;
            self->field_0x00C = 300;
        }
        break;
    case 3:
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->field_0x214->vec_0x03C) / 10;
        self->field_0x02C += self->field_0x1D4;
        if (vec3_distance(&self->pos_0x10, &self->field_0x214->vec_0x03C) < lbl_807999CC) {
            setVector3(&offset, lbl_807999A0, lbl_807999A0, lbl_807999CC);
            rotVecY(&offset, calcVecAng2(&self->field_0x214->vec_0x03C, &self->pos_0x10));
            addVec3(&delta, &self->field_0x214->vec_0x03C, &offset);
            copyVec3(&self->target_0x1EC, &delta);
            fn_801FDD9C(self, lbl_807999D0);
            lb_npc_motion_restart(self, 44);
        } else if (fn_801FDFD0(self) == 1) {
            if (lobby_w.field_0x076 == 1) {
                lb_npc_motion_restart(self, 55);
            } else {
                lb_npc_motion_restart(self, 44);
            }
        }
        break;
    }
}

#pragma peephole off

/* 0x80203114 - the band's per-frame dispatcher: forces the action byte to 1 and routes on the model kind, arming the
 * kind's motion where it has no handler. */
void fn_80203114(_LB_NPC* self)
{
    fn_801FDFC0(self, 1);

    if (lb_npc_id_ck(self, 1) == 1) {
        fn_80201AA0(self, 1);
    } else if (lb_npc_id_ck(self, 14) == 1) {
        fn_80201BE4(self);
    } else if (lb_npc_id_ck(self, 11) == 1) {
        fn_80201C80(self);
    } else if (lb_npc_id_ck(self, 2) == 1) {
        fn_80201F60(self);
    } else if (lb_npc_id_ck(self, 13) == 1) {
        fn_802020FC(self);
    } else if (fn_801FE348(self) == 1) {
        fn_80202394(self);
    } else if (fn_801FE39C(self) == 1) {
        fn_802024C4(self);
    } else if (lb_npc_id_ck(self, 3) == 1) {
        if (LbCheckKujiraEvent() == 1) {
            fn_80202728(self);
        } else {
            fn_80202570(self);
        }
    } else if (lb_npc_id_ck(self, 7) == 1) {
        if (lb_world_mode_get() == 9) {
            fn_802027CC(self, 1);
        } else if (((u16)ran_suu(1) & 1) != 0) {
            fn_802027CC(self, 0);
        } else {
            fn_802027CC(self, 1);
        }
    } else if (fn_801FE400(self) == 1) {
        fn_802029B4(self);
    } else if (lb_npc_id_ck(self, 12) == 1) {
        fn_80202AC4(self);
    } else if (lb_npc_id_ck(self, 5) == 1) {
        fn_80202B88(self);
    } else if (fn_801FE4A0(self) == 1) {
        fn_80202E6C(self);
    } else if (fn_801FE8CC(self) == 1) {
        fn_80202F00(self);
    } else if (fn_801FE4B4(self) == 1) {
        fn_80202DD0(self);
    } else if (fn_801FE55C(self) == 1) {
        fn_80202D24(self);
    } else if (lb_npc_id_ck(self, 19) == 1 || lb_npc_id_ck(self, 20) == 1) {
        fn_80202C24(self);
    } else {
        switch (self->field_0x006) {
        case 0:
            if (lb_npc_id_ck(self, 10) == 1 || lb_npc_id_ck(self, 9) == 1) {
                fn_801FE0AC(self, 1047, 16, 0);
            } else if (lb_npc_id_ck(self, 8) == 1) {
                if (lb_npc_Get_motion_no(self) == 52 || lb_npc_Get_motion_no(self) == 51) {
                    fn_801FE0AC(self, 51, 4, 0);
                } else {
                    fn_801FE0AC(self, 1136, 4, 0);
                }
            } else if (lb_npc_id_ck(self, 17) == 1) {
                fn_801FE0AC(self, 1136, 4, 0);
            } else if (fn_801FE450(self) == 1) {
                fn_801FE0AC(self, 1144, 4, 0);
            } else if (lb_npc_id_ck(self, 16) == 1) {
                if (lb_npc_Get_motion_no(self) == 1137 || lb_npc_Get_motion_no(self) == 1138) {
                    fn_801FE0AC(self, 1139, 4, 0);
                } else {
                    fn_801FE0AC(self, 1136, 4, 0);
                }
            } else if (lb_npc_id_ck(self, 6) == 1) {
                fn_801FE0AC(self, 1111, 4, 0);
            } else if (lb_npc_id_ck(self, 4) == 1) {
                fn_801FE0AC(self, 1071, 4, 0);
            } else if (lb_npc_id_ck(self, 18) == 1) {
                fn_801FE0AC(self, 1098, 4, 0);
            } else {
                lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
                break;
            }
            self->field_0x006++;
            fn_801FDEE4(self, 10);
            break;
        case 1:
            if (fn_801FDFD0(self) == 1) {
                fn_801FDF70(self, 10);
                self->field_0x006++;
            }
            break;
        case 2:
            if (self->field_0x1E4 <= 0) {
                lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
            }
            break;
        }
    }
}


/* 0x80203664 - the 5-arm "look around" cycle: hold motion 1040 for 100..355 random frames, then move on or hand the
 * action over; states 2..4 wait for the stop and pick the next hold or restart. */
void fn_80203664(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
        fn_801FDFE4(self, 1040, 16, 0);
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C > 0) {
            break;
        }
        if (((u16)ran_suu(1) & 1) == 0) {
            self->field_0x006++;
            self->field_0x00C = 1;
            fn_801FDFE4(self, 1041, 6, 0);
        } else {
            self->field_0x006 = 4;
            fn_801FDFE4(self, 1044, 8, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1042, 0, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
            fn_801FDFE4(self, 1040, 16, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
            fn_801FDFE4(self, 1040, 8, 0);
        }
        break;
    }
}

/* 0x80203834 - the NPC's "wait" opener: arm motion 1088 over 4 frames once. */
void fn_80203834(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1088, 4, 0);
}

/* 0x8020385C - the NPC's "sit down" opener: arm motion 1086 over 6 frames once. */
void fn_8020385C(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1086, 6, 0);
}

/* 0x80203884 - the idle machine: motion 10, 1081 when the turn runs out, 1082 on the lobby flag, then release the
 * action byte and arm 1078 or restart the table's motion. */
void fn_80203884(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (self->field_0x1E4 == 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1081, 6, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            if (fn_801FDFD0(self) == 1) {
                fn_801FDFE4(self, 1082, 6, 0);
                self->field_0x006++;
            }
        }
        break;
    case 3:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1078, 6, 0);
            self->field_0x006++;
        }
        break;
    case 4:
        fn_801FDFC0(self, 0);
        if (fn_801FE1EC(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802039D4 - pick the NPC's "sit" motion variant once, 1134 or 1135, off the kind byte. */
void fn_802039D4(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    switch (kind) {
    case 0:
        fn_801FE0AC(self, 1134, 6, 0);
        break;
    case 1:
        fn_801FE0AC(self, 1135, 6, 0);
        break;
    }
}

/* 0x80203A24 - `fn_80203664`'s sibling for the NPC that holds motion 1040 for a random 100..355 frames
 * and restarts it (rather than handing over) from state 4. */
void fn_80203A24(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
        fn_801FDFE4(self, 1040, 4, 0);
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C > 0) {
            break;
        }
        if (((u16)ran_suu(1) & 1) == 0) {
            self->field_0x006++;
            self->field_0x00C = 1;
            fn_801FDFE4(self, 1041, 6, 0);
        } else {
            self->field_0x006 = 4;
            fn_801FDFE4(self, 1044, 8, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1042, 0, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = 120;
            fn_801FDFE4(self, 1040, 4, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u8)((u16)ran_suu(1) + 100);
            fn_801FDFE4(self, 1040, 8, 0);
        }
        break;
    }
}

/* 0x80203BE4 - the NPC's three-arm "idle fidget": hold motion 1142 for a random 0..7 frames, then hold
 * it again (restarting the counter) or pick 1143/1149 for the next two frames off a coin flip. */
void fn_80203BE4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 7;
        fn_801FE0AC(self, 1142, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                if (((u16)ran_suu(1) & 1) != 0) {
                    fn_801FDFE4(self, 1143, 2, 0);
                } else {
                    fn_801FDFE4(self, 1149, 4, 0);
                }
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 7;
            fn_801FDFE4(self, 1142, 4, 0);
        }
        break;
    }
}

/* 0x80203D10 - the 4-arm "notice and greet" machine: hold 1063 with the shell visible, then walk 1064/1065 (a coin
 * flip) or flash the shell; states 2/3 restart 1063 or the table's motion. */
void fn_80203D10(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1063, 4, 0);
        fn_801FE200(self, 24, 1);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            if (((u16)ran_suu(1) & 1) != 0) {
                fn_801FDFE4(self, 1064, 24, 0);
                self->field_0x006 = 2;
            } else {
                fn_801FDFE4(self, 1065, 24, 0);
                self->field_0x006 = 3;
            }
        } else if (lb_npc_motion_play(self, 0, lbl_807999D4, lbl_807999A0) != 0) {
            fn_801FE200(self, 25, 1);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, 0);
        } else if (lb_npc_motion_play(self, 0, lbl_807999D8, lbl_807999A0) != 0) {
            fn_801FE200(self, 25, 0);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            fn_801FDFE4(self, 1063, 4, 0);
        } else if (lb_npc_motion_play(self, 0, lbl_807999DC, lbl_807999A0) != 0) {
            fn_801FE200(self, 25, 0);
        }
        break;
    }
}

/* 0x80203EC4 - the 4-arm "map greeting" machine: 1055 or 1051 by whether the map is 22, two motions counted down, then
 * 1056/1058 the same way and the table's motion. */
void fn_80203EC4(_LB_NPC* self)
{
    s32 other_map;

    fn_801FDFC0(self, 0);
    other_map = ((u8)get_now_mapno() != 22);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = 2;
        switch (other_map) {
        case 0:
            fn_801FDFE4(self, 1055, 16, 12);
            self->field_0x006 = 2;
            break;
        case 1:
            fn_801FDFE4(self, 1051, 2, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1057, 4, 0);
            self->field_0x006++;
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                switch (other_map) {
                case 0:
                    fn_801FDFE4(self, 1056, 4, 0);
                    break;
                case 1:
                    fn_801FDFE4(self, 1058, 6, 0);
                    break;
                }
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80204070 - the NPC's three-arm hold: motion 1035 for a random 0..3 frames, then 1036 for the next
 * 6, then release the action byte. */
void fn_80204070(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FDFE4(self, 1035, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1036, 6, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, 0);
        }
        break;
    }
}

/* 0x80204318 - arm motion 1155 over 8 or 6 frames off the kind byte, then motion 2 with the 102-frame
 * delay and hand the NPC over to the band above. */
void fn_80204318(_LB_NPC* self, u8 kind)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        switch (kind) {
        case 0:
            fn_801FDFE4(self, 1155, 8, 12);
            break;
        case 1:
            fn_801FDFE4(self, 1155, 6, 0);
            break;
        }
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 2, 6, 102);
            fn_801FFE44(self);
        }
        break;
    }
}

/* 0x802043D4 - arm motion 511 or 502 over 12 frames off the kind byte, then play the NPC's motion 0 at
 * the kind's own speed and, once it is running, flag the NPC's part slot and clear its flag byte. */
void fn_802043D4(_LB_NPC* self, u8 kind)
{
    f32 speed = lbl_807999A0;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        switch (kind) {
        case 0:
            fn_801FDFE4(self, 511, 12, 0);
            break;
        case 1:
            fn_801FDFE4(self, 502, 12, 0);
            break;
        }
        break;
    case 1:
        switch (kind) {
        case 0:
            speed = lbl_807999E4;
            break;
        case 1:
            speed = lbl_807999E8;
            break;
        }
        if (lb_npc_motion_play(self, 0, speed, lbl_807999A0) == 1) {
            s32 slot = fn_802080C0(self);

            self->field_0x234[0] = 2;
            fn_801E9A90((u8)slot, 2);
            self->field_0x001 = 0;
        }
        break;
    }
}

/* 0x80204148 - the 4-arm "beckon the player" machine: motion 10, 1184 with the three shell flags, 1185 on the lobby
 * flag, then re-flag the shell or hand over, and restart the table's motion. */
void fn_80204148(_LB_NPC* self)
{
    fn_801FDFC0(self, 0);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (self->field_0x1E4 == 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1184, 4, 0);
            fn_801FE200(self, 24, 1);
            fn_801FE200(self, 22, 0);
            fn_801FE200(self, 23, 0);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1185, 4, 0);
            fn_801FE200(self, 25, 0);
            if (fn_802FB948() == 0) {
                fn_801FE200(self, 24, 1);
            }
        } else if (lb_npc_motion_play(self, 0, lbl_807999C8, lbl_807999A0) != 0) {
            fn_801FE200(self, 24, 0);
            fn_801FE200(self, 25, 1);
        } else if (lb_npc_motion_play(self, 0, lbl_807999E0, lbl_807999A0) != 0) {
            camera_quake_req(8);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDF70(self, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802044C0 - arm one of the three "look" motions 1159/1160/1161 over 8 frames, once, off the kind
 * byte. */
void fn_802044C0(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    switch (kind) {
    case 0:
        fn_801FE0AC(self, 1159, 8, 0);
        break;
    case 1:
        fn_801FE0AC(self, 1160, 8, 0);
        break;
    case 2:
        fn_801FE0AC(self, 1161, 8, 0);
        break;
    }
}

/* 0x802046E4 - point the NPC at the lobby's move table and restart its first motion. */
void fn_802046E4(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    self->field_0x204 = lb_npc_move_data.table_0x0C;
    self->field_0x208 = 0;
    lb_npc_motion_restart(self, self->field_0x204[0].motion_0x0C);
}

/* 0x8020471C - clear the action byte, flag the NPC's second state byte, arm 1074 over 4 frames and then
 * restart the table's motion. */
void fn_8020471C(_LB_NPC* self)
{
    fn_801FDFC0(self, 0);
    fn_801FE128(self, 1);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1074, 4, 0);
        fn_801FDEE4(self, 4);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802047C8 - the 4-arm "keep the player company" machine: 1174, 1173, then 1101, during which it half the time moves
 * to a neighbouring move table and steps 910 units along the path while a pad check answers. */
void fn_802047C8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 8;
        fn_801FE0AC(self, 1174, 8, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                self->field_0x00C = 3;
                fn_801FDFE4(self, 1173, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1101, 4, 0);
            }
        } else if (lb_npc_motion_play(self, 0, lbl_807999C4, lbl_807999A0) != 0) {
            if ((u16)ran_suu(1) % 100 < 50) {
                self->field_0x204 = lb_npc_move_data.table_0x10;
                self->field_0x208 = 0;
                lb_npc_motion_restart(self, self->field_0x204[0].motion_0x0C);
            }
        } else if (lb_npc_motion_play(self, 5, lbl_807999F8, lbl_8079999C) == 1 ||
                   lb_npc_motion_play(self, 5, lbl_807999FC, lbl_80799A00) == 1) {
            self->field_0x02C -= 910;
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x802049C4 - play motion 1181, then either hand the NPC over to the band above or restart motion 34
 * off the kind byte. */
void fn_802049C4(_LB_NPC* self, u8 kind)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1181, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            switch (kind) {
            case 0:
                fn_801FFE44(self);
                break;
            case 1:
                lb_npc_motion_restart(self, 34);
                break;
            }
        }
        break;
    }
}

/* 0x80204A68 - arm motion 1182 for one motion, then restart motion 36. */
void fn_80204A68(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1182, 0, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, 36);
        }
        break;
    }
}

/* 0x80204ADC - play motion 1183 over 14 frames, arm 1095 next, then restart motion 33. */
void fn_80204ADC(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1183, 14, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1095, 4, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, 33);
        }
        break;
    }
}

/* 0x80204B88 - the NPC's three-arm "back off" machine: arm 1175, wait, then arm 1176 over 16 frames
 * (or 1191 over 24) off a coin flip and restart the countdown. */
void fn_80204B88(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = 0;
        fn_801FE0AC(self, 1175, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x00C = (u16)ran_suu(1) % 2;
                self->field_0x006++;
                fn_801FE0AC(self, 1176, 16, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x00C = (u16)ran_suu(1) % 2 + 2;
                self->field_0x006 = 1;
                fn_801FE0AC(self, 1191, 24, 0);
            }
        }
        break;
    }
}

/* 0x80204CB8 - play motion 1186, then arm 1187 over 2 frames or, while motion 0 has not started yet,
 * turn the NPC onto motion 26943/27853 over 17 ticks off the kind byte. */
void fn_80204CB8(_LB_NPC* self, u8 kind)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1186, 8, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FE0AC(self, 1187, 2, 0);
        } else if (lb_npc_motion_play(self, 0, lbl_807999D0, lbl_807999A0) != 0) {
            switch (kind) {
            case 0:
                fn_801FDEB4(self, 26943, 17);
                break;
            case 1:
                fn_801FDEB4(self, 27853, 17);
                break;
            }
        }
        break;
    }
}

/* 0x8020505C - arm motion 1190 over 4 frames with the 74-frame delay, once. */
void fn_8020505C(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1190, 4, 74);
}

/* 0x80205084 - arm motion 1109 over 20 frames, once. */
void fn_80205084(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    fn_801FE0AC(self, 1109, 20, 0);
}

/* 0x80205294 - arm motion 1098 or 1095 over 4 frames, once, off the kind byte. */
void fn_80205294(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    if (kind == 0) {
        fn_801FDFE4(self, 1098, 4, 0);
    } else {
        fn_801FDFE4(self, 1095, 4, 0);
    }
}

/* 0x802052D8 - arm motion 1093 over 4 frames, or re-arm it over 0 frames with the 88-frame delay. */
void fn_802052D8(_LB_NPC* self, u8 kind)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    if (kind == 0) {
        fn_801FDFE4(self, 1093, 4, 0);
    } else {
        fn_801FE0AC(self, 1093, 0, 88);
    }
}

/* 0x8020531C - hold motion 1105 for 20 frames, then 1103 for 5, and finally clear the lobby's field and
 * restart motion 0's table entry (or motion 52 when the map is not map 21). */
void fn_8020531C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1105, 4, 38);
        self->field_0x00C = 20;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            self->field_0x00C = 5;
            fn_801FDFE4(self, 1103, 4, 0);
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            lobby_w.field_0x076 = 0;
            if ((u8)get_now_mapno() == 21) {
                self->field_0x208 = 0;
                lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
            } else {
                self->field_0x208 = 0;
                lb_npc_motion_restart(self, 52);
            }
        }
        break;
    }
}

/* 0x80205424 - arm 1101, then either 1095 (and step on) or, while the NPC's looped motion 0 has not
 * started, spawn the effect at the offset vector and hand the NPC over. */
void fn_80205424(_LB_NPC* self)
{
    VEC3 offset;

    VEC3_ctor(&offset);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1101, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDFE4(self, 1095, 4, 0);
            self->field_0x006++;
        } else if (fn_801FE1DC(self, 0, lbl_807999D0, lbl_807999A0) == 1) {
            setVector3(&offset, lbl_807999A0, lbl_8079999C, lbl_807999A0);
            lb_quest_board_effect_spawn(self, 3, 10, &offset, lbl_80799A14);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            self->field_0x208 = 0;
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80205544 - arm motion 1104 and set the NPC's secondary action byte; once the motion is done restart
 * motion 55 and set the lobby's field. */
void fn_80205544(_LB_NPC* self)
{
    fn_801FDFC8(self, 1);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1104, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, 55);
            lobby_w.field_0x076 = 1;
        }
        break;
    }
}

/* 0x802055D8 - picks the move table by map and part kind and points the NPC at the first spot it is past in x and z
 * (the 10000 x terminator leaves the table base). */
void fn_802055D8(_LB_NPC* self)
{
    LbNpcMotionEntry** list;
    LbNpcMotionEntry* selected;
    LbNpcMoveSpot* current;
    LbNpcMoveSpot* spot;
    u8 i = 0;

    if (self->field_0x006 != 0) {
        return;
    }
    switch ((u8)get_now_mapno()) {
    case 22:
        switch (self->field_0x004) {
        case 0:
            list = lb_npc_move_data.list_0x18;
            spot = lbl_805B8EF0;
            break;
        case 1:
            list = lb_npc_move_data.list_0x1C;
            spot = lbl_805B8F28;
            break;
        case 2:
            list = lb_npc_move_data.list_0x20;
            spot = lbl_805B8F38;
            break;
        }
        break;
    case 21:
        switch (self->field_0x004) {
        case 5:
            list = lb_npc_move_data.list_0x24;
            spot = lbl_805B8F50;
            break;
        case 6:
            list = lb_npc_move_data.list_0x28;
            spot = lbl_805B8F60;
            break;
        case 7:
            list = lb_npc_move_data.list_0x2C;
            spot = lbl_805B8F70;
            break;
        }
        break;
    }

    /* The target's no-match path stores the table base itself as the entry pointer. */
    selected = (LbNpcMotionEntry*)list;
    for (current = spot; current->x < lbl_80799A18; i++, current++) {
        if (current->x > self->pos_0x10.x && current->z > self->pos_0x10.z) {
            selected = list[i];
            break;
        }
    }
    self->field_0x208 = 0;
    self->field_0x204 = selected;
    lb_npc_motion_restart(self, selected[0].motion_0x0C);
}

/* 0x80205978 - set the NPC's secondary action byte and clear its two flag bytes, then set the lobby's
 * second field once. */
void fn_80205978(_LB_NPC* self)
{
    fn_801FDFC8(self, 1);
    self->field_0x001 = 0;
    self->field_0x003 = 0;

    if (self->field_0x006 == 0) {
        self->field_0x006++;
        lobby_w.field_0x077 = 1;
    }
}

/* 0x80205F1C - hand the NPC over to the band above with the kind its own query answers. */
void fn_80205F1C(_LB_NPC* self)
{
    s32 kind = fn_80207EAC(self);

    fn_80207FD0(self, (u8)kind, 1);
}

/* 0x80206044 - step the NPC's motion table onto its +0x10 branch and restart the first motion there. */
void fn_80206044(_LB_NPC* self)
{
    self->field_0x204 = self->field_0x204[self->field_0x208].branch_0x10;
    self->field_0x208 = 0;
    lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
}

/* 0x80207698 - restart motion 67 when the NPC's own flag byte is set, otherwise re-arm motion 1 over
 * 4 frames. */
void fn_80207698(_LB_NPC* self)
{
    if (self->field_0x006 != 0) {
        return;
    }
    self->field_0x006++;
    if (self->field_0x264 != 0) {
        lb_npc_motion_restart(self, 67);
    } else {
        fn_801FE0AC(self, 1, 4, 0);
    }
}

/* 0x80205F5C - play motion 10 and arm 1131, then restart the table's motion. */
void fn_80205F5C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDEE4(self, 10);
        fn_801FDFE4(self, 1131, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        }
        break;
    }
}

/* 0x80205FEC - set the NPC's action byte and arm motion 54 over 4 frames, once. */
void fn_80205FEC(_LB_NPC* self)
{
    fn_801FDFC0(self, 1);

    if (self->field_0x006 == 0) {
        self->field_0x006++;
        fn_801FDFE4(self, 54, 4, 0);
    }
}

/* 0x80206074 - the 3-arm "walk a set route" machine: aim at the first point (heading latched 95 frames), the second
 * (80), then restart. */
void fn_80206074(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1115, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A24, lbl_80799A28, lbl_80799A2C);
        self->field_0x02C = (u16)-8192;
        self->field_0x00C = 95;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1121, 0, 0);
            setVector3(&self->pos_0x10, lbl_80799A30, lbl_80799A28, lbl_80799A34);
            self->field_0x00C = 80;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1115, 0, 0);
            setVector3(&self->pos_0x10, lbl_80799A24, lbl_80799A28, lbl_80799A2C);
        }
        break;
    }
}

/* 0x8020618C - aim the NPC's position at the third route point, latch its heading for 449 frames and
 * then arm motion 1178. */
void fn_8020618C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1177, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A38, lbl_80799A3C, lbl_80799A40);
        self->field_0x02C = (u16)-7281;
        self->field_0x00C = 449;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1178, 0, 0);
        }
        break;
    }
}

/* 0x802059D8 - the NPC's three-arm "pace on the spot" machine: hold motion 1177 for a random 0..8
 * frame wait, then 1178, then restart 1177 with the 92-frame delay. */
void fn_802059D8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 8;
        fn_801FE0AC(self, 1177, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1178, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 1;
            self->field_0x00C = (u16)ran_suu(1) & 8;
            fn_801FDFE4(self, 1177, 4, 92);
        }
        break;
    }
}

/* 0x80205AD4 - the NPC's three-arm "pace" machine: hold motion 1123 for a random 0..3 frame wait, then
 * 1124, then restart 1123 with the 130-frame delay. */
void fn_80205AD4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FE0AC(self, 1123, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1124, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 0;
            self->field_0x00C = (u16)ran_suu(1) & 3;
            fn_801FDFE4(self, 1123, 4, 130);
        }
        break;
    }
}

/* 0x80205BD0 - the NPC's five-arm "hand over" machine: hold motion 1197 for a random 0..3 frame wait,
 * then 1194 (same wait), then 1196, then 1195, then back to the start. */
void fn_80205BD0(_LB_NPC* self)
{
    fn_801FE128(self, 1);

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x00C = (u16)ran_suu(1) & 3;
        fn_801FDFE4(self, 1197, 20, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                self->field_0x00C = (u16)ran_suu(1) & 3;
                fn_801FDFE4(self, 1194, 4, 0);
            }
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
                fn_801FDFE4(self, 1196, 4, 0);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1195, 4, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006 = 0;
        }
        break;
    }
}

/* 0x80205D54 - the 4-arm "settle in" machine: 1115 after 0..14 random frames, 1193 over 8 with a coin-flip sign
 * (re-armed while the player approaches), then count down and restart. */
void fn_80205D54(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1115, 4, 0);
        self->field_0x00C = (u16)ran_suu(1) % 15;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1193, 8, 0);
            self->field_0x00C = (u16)ran_suu(1) % 2;
        }
        break;
    case 2:
        if (self->field_0x00C > 0) {
            if (lb_npc_motion_play(self, 0, lbl_807999B0, lbl_807999A0) == 1) {
                fn_801FDFE4(self, 1193, 4, 0);
                self->field_0x00C--;
            }
        } else if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            self->field_0x00C = (u16)ran_suu(1) & 0x9D;
            if (((u16)ran_suu(1) & 1) != 0) {
                fn_801FDFE4(self, 1121, 16, 0);
            } else {
                fn_801FDFE4(self, 1129, 8, 0);
            }
        }
        break;
    case 3:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006 = 1;
        }
        break;
    }
}

/* 0x80206DE8 - arm motion 1121 onto the route's second point with the heading latched, then motion 1129
 * once the player's approach is over. */
void fn_80206DE8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1121, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AAC, lbl_80799A28, lbl_80799AB0);
        self->field_0x02C = (u16)-1819;
        break;
    case 1:
        if (lb_npc_motion_play(self, 0, lbl_80799AB4, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 4, 0);
        }
        break;
    }
}

/* 0x80206E9C - arm motion 1194 onto the route's third point, hold the heading for 210 frames, then arm
 * motion 1129. */
void fn_80206E9C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1194, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AB8, lbl_80799A28, lbl_80799ABC);
        self->field_0x02C = (u16)-909;
        self->field_0x00C = 210;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 8, 0);
        }
        break;
    }
}

/* 0x80206F4C - arm motion 1193 onto the route's fourth point, hold the heading for 215 frames, then arm
 * motion 1194. */
void fn_80206F4C(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1193, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AC0, lbl_80799A28, lbl_80799AC4);
        self->field_0x02C = 5461;
        self->field_0x00C = 215;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1194, 4, 0);
        }
        break;
    }
}

/* 0x80206FF8 - the route walk's turn-around: aim the NPC at the route's fifth point for 168 frames,
 * then motion 1121 for 72, then motion 1194. */
void fn_80206FF8(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1194, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AC8, lbl_80799A28, lbl_80799ACC);
        self->field_0x02C = 16384;
        self->field_0x00C = 168;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1121, 4, 0);
            self->field_0x00C = 72;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1194, 4, 0);
        }
        break;
    }
}

/* 0x802070E4 - the route walk's turn: aim the NPC at the route's sixth point for 168 frames, then
 * motion 1194 for 57, then motion 1129. */
void fn_802070E4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1121, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AD0, lbl_80799A28, lbl_80799AD4);
        self->field_0x02C = 14564;
        self->field_0x00C = 168;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1194, 4, 0);
            self->field_0x00C = 57;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 4, 0);
        }
        break;
    }
}

/* 0x802071D0 - the route walk's last turn: aim the NPC at the route's seventh point for 168 frames and
 * then hold motion 1129 for 72. */
void fn_802071D0(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1129, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AD8, lbl_80799A28, lbl_80799ADC);
        self->field_0x02C = 13653;
        self->field_0x00C = 168;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1129, 4, 0);
            self->field_0x00C = 72;
        }
        break;
    }
}

/* 0x802069D0 - the NPC's three-arm "walk to the meeting point" machine: motion 1 onto the meeting
 * point for 470 frames, then 1046 over 18 frames (clearing its flag byte each frame) and 1045. */
void fn_802069D0(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A88, lbl_80799A5C, lbl_80799A8C);
        self->field_0x02C = 12379;
        self->field_0x00C = 470;
        self->field_0x001 = 0;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1046, 18, 0);
            self->field_0x00C = 79;
        } else {
            self->field_0x001 = 0;
        }
        break;
    case 2:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1045, 18, 0);
        }
        break;
    }
}

/* 0x80206CB4 - the NPC's four-arm walk to the meeting point: motion 1 onto the point for 540 frames
 * (clearing its flag byte), then 1084, then 1085, then 1077 with the 74-frame delay. */
void fn_80206CB4(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799AA4, lbl_80799A5C, lbl_80799AA8);
        self->field_0x02C = (u16)-17202;
        self->field_0x00C = 540;
        self->field_0x001 = 0;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1084, 0, 0);
        } else {
            self->field_0x001 = 0;
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1085, 6, 0);
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1077, 4, 74);
        }
        break;
    }
}

/* 0x80204528 - the 5-arm "turn to face" machine: 1170, then 1171 or a latched 15-tick turn to 0xC001, a count down,
 * 1172, then restart the move table or aim over 11 ticks. */
void fn_80204528(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1170, 4, 0);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1171, 0, 0);
            self->field_0x00C = 1;
        } else if (lb_npc_motion_play(self, 0, lbl_807999EC, lbl_807999A0) != 0) {
            fn_801FDEB4(self, 0xC001, 15);
            self->field_0x218 = self->field_0x02C;
        }
        break;
    case 2:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x00C--;
            if (self->field_0x00C <= 0) {
                self->field_0x006++;
            }
        }
        break;
    case 3:
        if (lb_npc_motion_play(self, 1, lbl_807999F0, lbl_807999A0) != 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1172, 0, 0);
        }
        break;
    case 4:
        if (fn_801FE1EC(self) == 1) {
            fn_801FDEB4(self, (u16)-909, 10);
            lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
        } else if (lb_npc_motion_play(self, 0, lbl_807999F4, lbl_807999A0) != 0) {
            fn_801FDEE4(self, 11);
        }
        break;
    }
}

/* 0x80206824 - the 6-arm "meeting-point route" machine: 1012 for 123 frames, over 24 and over 16, 1013 over 40
 * (141-frame latch), 1009 after 292, 1010 over 6. */
void fn_80206824(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1012, 0, 0);
        setVector3(&self->pos_0x10, lbl_80799A74, lbl_80799A78, lbl_80799A7C);
        self->field_0x02C = 0;
        self->field_0x00C = 123;
        break;
    case 1:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1012, 24, 0);
        }
        break;
    case 2:
        if (lb_npc_motion_play(self, 0, lbl_80799A80, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1012, 16, 0);
        }
        break;
    case 3:
        if (lb_npc_motion_play(self, 0, lbl_80799A84, lbl_807999A0) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1013, 40, 6);
            self->field_0x00C = 141;
        }
        break;
    case 4:
        self->field_0x00C--;
        if (self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1009, 0, 292);
        }
        break;
    case 5:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1010, 6, 0);
        }
        break;
    }
}

/* 0x80205764 - keeps station on the move work's record: takes its vector as the target and walks the 80/200/400-unit
 * distance bands, turning a tenth of the heading error each tick. */
void fn_80205764(_LB_NPC* self)
{
    LbNpcMoveWorkEntry* work;
    f32 near;
    f32 far;
    f32 dist;

    work = (LbNpcMoveWorkEntry*)get_move_work_adrs(2);
    copyVec3(&self->target_0x1EC, &work[(s8)my_player_no()].vec_0x3C);
    self->field_0x22C = 1;
    self->field_0x22D = 1;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1091, 4, 0);
        break;
    case 1:
        near = lbl_80799A10;
        far = lbl_80799A1C;
        dist = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);
        if (dist <= near * near) {
            self->field_0x006 = 3;
            fn_801FE0AC(self, 1095, 4, 0);
        } else if (dist >= far * far) {
            self->field_0x006 = 2;
            fn_801FE0AC(self, 1092, 4, 0);
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    case 2:
        far = lbl_80799A20;
        dist = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);
        if (dist <= far * far) {
            self->field_0x006 = 0;
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    case 3:
        far = lbl_80799A20;
        dist = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);
        if (dist >= far * far) {
            self->field_0x006 = 0;
        }
        break;
    }
}

/* 0x802050AC - the "find a move table entry" machine: 1098, then on the lobby flag motion 55 or a table entry for the
 * part (two map-22 kinds gated on the shop), aiming a fifth of the way each frame. */
void fn_802050AC(_LB_NPC* self)
{
    LbNpcMotionEntry* entry;
    u8 kind = 0xFF;
    s32 skip = 0;
    s32 turn;

    self->field_0x22D = 1;

    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1098, 4, 0);
        break;
    case 1:
        if (fn_801FDFD0(self) == 1) {
            if (lobby_w.field_0x076 == 1) {
                lb_npc_motion_restart(self, 55);
            } else if ((u8)get_now_mapno() == 22) {
                entry = fn_80207DC4(self, &kind);
                if ((u8)get_now_mapno() == 22 && self->field_0x004 == 2) {
                    switch ((u32)kind) {
                    case 1:
                        if (lb_world_upgrade_get(2) == 0) {
                            skip = 1;
                        }
                        break;
                    case 5:
                        if (lb_world_upgrade_get(3) == 0) {
                            skip = 1;
                        }
                        break;
                    }
                }
                if (entry != 0 && skip == 0) {
                    self->field_0x208 = 0;
                    self->field_0x204 = entry;
                    lb_npc_motion_restart(self, entry[0].motion_0x0C);
                } else {
                    self->field_0x208 = 0;
                    self->field_0x204 = &lbl_805B8EC8;
                    lb_npc_motion_restart(self, 48);
                }
            } else {
                self->field_0x208 = 0;
                lb_npc_motion_restart(self, self->field_0x204[self->field_0x208].motion_0x0C);
            }
        } else {
            turn = (s16)(u16)fn_801FDE3C(self, &self->field_0x214->vec_0x03C) / 5;
            self->field_0x02C += turn;
        }
        break;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * The scripted NPC scenes (the pig release, the opening demo, the bench scene, the event placements) and the NPC's
 * per-motion dispatcher (0x80204DA8..0x802076D4).
 */
void fn_801FDFC8(_LB_NPC* self, u8 value);
void mtx34_trans_get(nw4r::math::MTX34* m, VEC3* out);
void subVec3(VEC3* out, const VEC3* a, const VEC3* b);
s16 camera_frame_get(u8 full);
u32 fn_80208AC0(_PLW* self);
LbNpcMotionEntry* fn_80207DC4(_LB_NPC* self, u8* kind);
void lb_npc_idle(_LB_NPC* self, u32 mode);
void lb_npc_walk_leg(_LB_NPC* self);
void lb_npc_turn_leg(_LB_NPC* self);
void lb_npc_warp_leg(_LB_NPC* self);
} /* extern "C" */
} /* namespace s_802029B4 */
#include "fn_8004CAD8/mtx.h"
namespace s_802029B4 {
extern "C" {

/* 0x80204DA8 - the pig-release scene: ride on the player's back (joint 7), get set down in front of him, then
 * take the area's release table (or the default walk). */
void lb_npc_pig_release(_LB_NPC* self)
{
    VEC3 offset;
    nw4r::math::MTX34 joint;
    nw4r::math::MTX34 unused;
    u8 kind = 0xFF;
    s32 locked = 0;

    VEC3_ctor(&offset);
    MTX34_ctor(&joint);
    MTX34_ctor(&unused);
    fn_801FDFC8(self, 1);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 1189, 30, 0);
        break;
    case 1:
        self->field_0x22E = 1;
        self->field_0x02C = self->field_0x214->rot_0x54.y;
        setVector3(&offset, 8.9f, -22.2f, -13.0f);
        pl_joint_mtx_get(self->field_0x214, 7, &joint);
        mulVecMat(&offset, &joint);
        mtx34_trans_get(&joint, &self->pos_0x10);
        self->pos_0x10.x += offset.x;
        self->pos_0x10.y += offset.y;
        self->pos_0x10.z += offset.z;
        if (fn_80208AC0(self->field_0x214) == 0) {
            VEC3 pos;

            self->field_0x006++;
            setVector3(&offset, 0.0f, 0.0f, 80.0f);
            rotVecY(&offset, self->field_0x214->rot_0x54.y);
            addVec3(&pos, &self->pos_0x10, &offset);
            copyVec3(&self->target_0x1EC, &pos);
            fn_801FDD9C(self, 20.0f);
            fn_801FE0AC(self, 1101, 20, 0);
        }
        break;
    case 2:
        self->field_0x22C = 1;
        self->field_0x22D = 1;
        if (fn_801FE1EC(self) == 1) {
            LbNpcMotionEntry* table = fn_80207DC4(self, &kind);

            if (get_now_mapno() == 0x16 && self->field_0x004 == 2) {
                switch (kind) {
                case 1:
                    if (lb_world_upgrade_get(2) == 0) {
                        locked = 1;
                    }
                    break;
                case 5:
                    if (lb_world_upgrade_get(3) == 0) {
                        locked = 1;
                    }
                    break;
                }
            }
            if (table != 0 && locked == 0) {
                self->field_0x208 = 0;
                self->field_0x204 = table;
                lb_npc_motion_restart(self, table->motion_0x0C);
            } else {
                self->field_0x208 = 0;
                self->field_0x204 = lb_npc_move_data.table_0x14;
                lb_npc_motion_restart(self, lb_npc_move_data.table_0x14->motion_0x0C);
            }
        }
        break;
    case 3:
        if (fn_801FE1EC(self) == 1) {
            lb_npc_motion_restart(self, 46);
        }
        break;
    }
}

/* 0x8020623C - the opening demo's NPC: stand at the gate, cut to the plaza, then spin round the stored centre. */
void fn_8020623C(_LB_NPC* self)
{
    VEC3* center = &self->event_pos_0x234;
    VEC3 radius;

    VEC3_ctor(&radius);
    radius.x = 0.0f;
    radius.y = 0.0f;
    radius.z = -60.0f;
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FE0AC(self, 11, 0, 0);
        setVector3(&self->pos_0x10, 2046.5f, 475.3f, -487.0f);
        copyVec3(&self->field_0x01C, &self->pos_0x10);
        self->field_0x02C = 0xC001;
        break;
    case 1:
        if (lb_npc_motion_play(self, 0, 138.0f, 0.0f) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1, 0, 0);
            self->field_0x00C = 174;
        }
        break;
    case 2:
        if (--self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 11, 0, 12);
            setVector3(&self->pos_0x10, 13.7f, 179.0f, -336.5f);
            self->field_0x02C = 0xA99B;
        }
        break;
    case 3:
        if (lb_npc_motion_play(self, 0, 28.0f, 0.0f) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 2, 8, 0);
            self->field_0x00C = 19;
        }
        break;
    case 4:
        if (lb_npc_motion_play(self, 0, 46.0f, 0.0f) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 2, 6, 10);
        } else if (--self->field_0x00C <= 0) {
            fn_801FDEB4(self, 0x8445, 8);
        }
        break;
    case 5:
        if (camera_frame_get(0) >= 307) {
            self->field_0x006++;
            fn_801FDFE4(self, 210, 0, 0);
            setVector3(&self->pos_0x10, -79.9f, 179.0f, -393.9f);
            self->field_0x02C = 0x84FB;
        }
        break;
    case 6:
        if (lb_npc_motion_play(self, 0, 142.0f, 0.0f) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 211, 0, 0);
        }
        break;
    case 7:
        if (lb_npc_motion_play(self, 0, 142.0f, 0.0f) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 210, 0, 0);
        }
        break;
    case 8:
        if (lb_npc_motion_play(self, 0, 142.0f, 0.0f) == 1) {
            VEC3 pos;

            self->field_0x006++;
            fn_801FDFE4(self, 208, 22, 0);
            rotVecY(&radius, self->field_0x02C);
            addVec3(&pos, &self->pos_0x10, &radius);
            copyVec3(center, &pos);
            self->event_angle_0x240 = self->field_0x02C;
            self->event_spin_0x242 = 0;
        }
        break;
    case 9: {
        VEC3 pos;

        self->field_0x02C = self->event_angle_0x240 += 0x1D1;
        rotVecY(&radius, self->event_angle_0x240);
        subVec3(&pos, center, &radius);
        copyVec3(&self->pos_0x10, &pos);
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 209, 0, 0);
        } else if (lb_npc_motion_play(self, 3, 0.0f, 30.0f) != 0) {
            self->event_spin_0x242 += 0x889;
        }
        self->field_0x02C += self->event_spin_0x242;
        break;
    }
    case 10: {
        VEC3 pos;

        self->field_0x02C = self->event_angle_0x240 += 0x1D1;
        rotVecY(&radius, self->event_angle_0x240);
        subVec3(&pos, center, &radius);
        copyVec3(&self->pos_0x10, &pos);
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 208, 0, 0);
        } else if (lb_npc_motion_play(self, 3, 0.0f, 30.0f) != 0) {
            self->event_spin_0x242 -= 0x889;
        }
        self->field_0x02C += self->event_spin_0x242;
        break;
    }
    case 11: {
        VEC3 pos;

        self->field_0x02C = self->event_angle_0x240 += 0x1D1;
        rotVecY(&radius, self->event_angle_0x240);
        subVec3(&pos, center, &radius);
        copyVec3(&self->pos_0x10, &pos);
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 209, 0, 0);
        } else if (lb_npc_motion_play(self, 3, 0.0f, 30.0f) != 0) {
            self->event_spin_0x242 += 0x889;
        }
        self->field_0x02C += self->event_spin_0x242;
        break;
    }
    case 12: {
        VEC3 pos;

        self->field_0x02C = self->event_angle_0x240 += 0x1D1;
        rotVecY(&radius, self->event_angle_0x240);
        subVec3(&pos, center, &radius);
        copyVec3(&self->pos_0x10, &pos);
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1, 0, 0);
        } else if (lb_npc_motion_play(self, 3, 0.0f, 30.0f) != 0) {
            self->event_spin_0x242 += 0x889;
        }
        self->field_0x02C += self->event_spin_0x242;
        break;
    }
    }
}

/* 0x80206AD0 - the bench scene's NPC: wait off screen, then walk in and sit down at the bench. */
void lb_npc_bench_scene(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        fn_801FDFE4(self, 1, 0, 0);
        setVector3(&self->pos_0x10, -758.6f, 179.0f, 0.6f);
        self->field_0x02C = 0x5555;
        self->field_0x00C = 340;
        self->field_0x001 = 0;
        break;
    case 1:
        if (--self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1084, 0, 0);
            self->field_0x00C = 40;
        } else {
            self->field_0x001 = 0;
        }
        break;
    case 2:
        if (--self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1085, 6, 0);
            self->field_0x00C = 60;
        }
        break;
    case 3:
        self->field_0x00C--;
        if (lb_npc_motion_play(self, 0, 100.0f, 0.0f) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1077, 0, 74);
        }
        break;
    case 4:
        if (--self->field_0x00C <= 0) {
            self->field_0x006++;
            fn_801FDFE4(self, 1075, 0, 0);
            setVector3(&self->pos_0x10, -104.0f, 179.0f, -185.2f);
            self->field_0x02C = 0x760B;
        }
        break;
    case 5:
        if (fn_801FE1EC(self) == 1) {
            self->field_0x006++;
            fn_801FDFE4(self, 1076, 0, 0);
        }
        break;
    }
}

/* 0x80207284 - the event scene's NPCs: hidden for a while, then each appears at its spot in its pose. */
void fn_80207284(_LB_NPC* self)
{
    switch (self->field_0x006) {
    case 0:
        self->field_0x006++;
        self->field_0x001 = 0;
        self->field_0x00C = 530;
        break;
    case 1:
        if (--self->field_0x00C <= 0) {
            self->field_0x006++;
            switch (self->field_0x002) {
            case 1:
                fn_801FDFE4(self, 1, 0, 0);
                setVector3(&self->pos_0x10, 69.0f, 179.0f, -166.0f);
                self->field_0x02C = 0x9555;
                break;
            case 2:
                fn_801FDFE4(self, 1001, 0, 0);
                setVector3(&self->pos_0x10, -95.0f, 179.0f, -591.0f);
                self->field_0x02C = 0;
                break;
            case 3:
                fn_801FDFE4(self, 1053, 0, 0);
                setVector3(&self->pos_0x10, -210.0f, 179.0f, -550.0f);
                self->field_0x02C = 0x238E;
                break;
            case 4:
                fn_801FDFE4(self, 1023, 0, 0);
                setVector3(&self->pos_0x10, 50.0f, 179.0f, -420.0f);
                self->field_0x02C = 0xD556;
                break;
            case 6:
                fn_801FDFE4(self, 1043, 0, 0);
                setVector3(&self->pos_0x10, -320.0f, 179.0f, -220.0f);
                self->field_0x02C = 0x671C;
                break;
            case 8:
                fn_801FDFE4(self, 1163, 0, 0);
                setVector3(&self->pos_0x10, -60.0f, 179.0f, -160.0f);
                self->field_0x02C = 0x8001;
                break;
            case 14:
                fn_801FDFE4(self, 1088, 0, 0);
                setVector3(&self->pos_0x10, -160.0f, 179.0f, -95.0f);
                self->field_0x02C = 0x638E;
                break;
            case 15:
                fn_801FDFE4(self, 1075, 0, 0);
                setVector3(&self->pos_0x10, 90.0f, 179.0f, -300.0f);
                self->field_0x02C = 0x9C72;
                break;
            case 16:
                fn_801FDFE4(self, 1045, 0, 0);
                setVector3(&self->pos_0x10, -290.0f, 179.0f, -400.0f);
                self->field_0x02C = 0x5555;
                break;
            }
        } else {
            self->field_0x001 = 0;
        }
        break;
    }
}

/* 0x802074E8 - runs the NPC's current motion-table entry by its motion kind. */
void lb_npc_motion_dispatch(_LB_NPC* self)
{
    switch (self->field_0x1CC) {
    case 0x00:
        lb_npc_idle(self, 0);
        break;
    case 0x01:
        lb_npc_idle(self, 1);
        break;
    case 0x02:
        lb_npc_idle(self, 2);
        break;
    case 0x03:
        lb_npc_walk_leg(self);
        break;
    case 0x04:
        lb_npc_walk_leg(self);
        break;
    case 0x05:
        lb_npc_turn_leg(self);
        break;
    case 0x06:
        lb_npc_warp_leg(self);
        break;
    case 0x07:
        fn_80203114(self);
        break;
    case 0x08:
        fn_802039D4(self, 0);
        break;
    case 0x09:
        fn_802039D4(self, 1);
        break;
    case 0x0A:
        fn_80203664(self);
        break;
    case 0x0B:
        fn_80203834(self);
        break;
    case 0x0C:
        fn_8020385C(self);
        break;
    case 0x0D:
        fn_80203884(self);
        break;
    case 0x0E:
        fn_80203A24(self);
        break;
    case 0x0F:
        fn_80203BE4(self);
        break;
    case 0x10:
        fn_80203D10(self);
        break;
    case 0x11:
        fn_80203EC4(self);
        break;
    case 0x12:
        fn_80204070(self);
        break;
    case 0x13:
        fn_80204148(self);
        break;
    case 0x14:
        fn_80204318(self, 0);
        break;
    case 0x15:
        fn_80204318(self, 1);
        break;
    case 0x1A:
        fn_802043D4(self, 0);
        break;
    case 0x1B:
        fn_802043D4(self, 1);
        break;
    case 0x17:
        fn_802044C0(self, 0);
        break;
    case 0x18:
        fn_802044C0(self, 1);
        break;
    case 0x19:
        fn_802044C0(self, 2);
        break;
    case 0x1C:
        fn_80204528(self);
        break;
    case 0x1D:
        fn_802046E4(self);
        break;
    case 0x1E:
        fn_8020471C(self);
        break;
    case 0x1F:
        fn_802047C8(self);
        break;
    case 0x20:
        fn_802049C4(self, 0);
        break;
    case 0x21:
        fn_802049C4(self, 1);
        break;
    case 0x22:
        lb_npc_walk_leg(self);
        break;
    case 0x23:
        fn_80204A68(self);
        break;
    case 0x24:
        fn_80204ADC(self);
        break;
    case 0x25:
        fn_80204B88(self);
        break;
    case 0x26:
        fn_80204CB8(self, 0);
        break;
    case 0x27:
        fn_80204CB8(self, 1);
        break;
    case 0x28:
        lb_npc_pig_release(self);
        break;
    case 0x29:
        lb_npc_walk_leg(self);
        break;
    case 0x2A:
        fn_8020505C(self);
        break;
    case 0x2B:
        fn_80205084(self);
        break;
    case 0x2C:
        fn_802050AC(self);
        break;
    case 0x2D:
        fn_80205294(self, 0);
        break;
    case 0x2E:
        fn_802052D8(self, 0);
        break;
    case 0x2F:
        fn_802052D8(self, 1);
        break;
    case 0x30:
        fn_80205294(self, 1);
        break;
    case 0x31:
        fn_8020531C(self);
        break;
    case 0x32:
        fn_80205424(self);
        break;
    case 0x33:
        fn_80205544(self);
        break;
    case 0x34:
        fn_802055D8(self);
        break;
    case 0x35:
        lb_npc_walk_leg(self);
        break;
    case 0x36:
        lb_npc_walk_leg(self);
        break;
    case 0x37:
        fn_80205764(self);
        break;
    case 0x38:
        fn_80205978(self);
        break;
    case 0x39:
        fn_802059D8(self);
        break;
    case 0x3A:
        fn_80205AD4(self);
        break;
    case 0x3B:
        fn_80205BD0(self);
        break;
    case 0x3C:
        fn_80205D54(self);
        break;
    case 0x3D:
        fn_80205F1C(self);
        break;
    case 0x3E:
        fn_80205F5C(self);
        break;
    case 0x3F:
        lb_npc_walk_leg(self);
        break;
    case 0x40:
        fn_80205FEC(self);
        break;
    case 0x41:
        fn_80206044(self);
        break;
    case 0x45:
        fn_80206074(self);
        break;
    case 0x46:
        fn_8020618C(self);
        break;
    case 0x48:
        fn_80206824(self);
        break;
    case 0x49:
        fn_802069D0(self);
        break;
    case 0x4A:
        lb_npc_bench_scene(self);
        break;
    case 0x4B:
        fn_80206CB4(self);
        break;
    case 0x4C:
        fn_80206DE8(self);
        break;
    case 0x4D:
        fn_80206E9C(self);
        break;
    case 0x4E:
        fn_80206F4C(self);
        break;
    case 0x4F:
        fn_80206FF8(self);
        break;
    case 0x50:
        fn_802070E4(self);
        break;
    case 0x51:
        fn_802071D0(self);
        break;
    case 0x52:
        fn_80207284(self);
        break;
    }
}

#pragma peephole on

} /* extern "C" */

} /* namespace s_802029B4 */

/* The NPC/character act layer (0x802076D4-0x8020C588). */
#define EF_FN_800CDB2C_NO_RAN_SUU 1
#include "types.h"
#include "nw4r/math.h"

#include "lobby/lb_npc.h"
#include "pl.h"

#include "ef/fn_800CDB2C.h"      /* my_player_no */
#include "unsplit/Pl.h"          /* Get_motion_no, Pl_chr_setX, Pl_frame_check */
#include "unsplit/lobby.h"       /* LbStr, lbl_80799Bxx */
#include "Network/network_pat_control.h" /* checkOtherInvite (rule 2: its owner's header) */
#include "Pl/Pl_master_ck.h"        /* Pl_master_ck, Pl_act_ck, pl_model_pos_sync */
#include "Pl/fn_802693C4.h"
#include "ef/eft004.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
namespace s_802076D4 {


extern "C" int memcmp(const void* a, const void* b, u32 n);
} /* namespace s_802076D4 */


/* ---------------------------------------------------------------------------------------------------
 * The callees this batch needs.  A `fn_XXXXXXXX` stem is the map's own placeholder (not a mangling), so
 * those are `extern "C"`; a name the map spells with an argument list is the real C++ declaration.
 */
void* get_move_work_adrs(u8 kind);
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
#pragma peephole off
namespace s_802076D4 {

extern "C" int memcmp(const void* a, const void* b, u32 n);

extern "C" {
__declspec(noinline) u32 fn_80208AB8(_PLW* self);
__declspec(noinline) u32 fn_80208AC0(_PLW* self);
__declspec(noinline) u32 fn_80208AE8(_PLW* self);
void lb_npc_act_set(_PLW* self, u32 a, u16 b, u16 c);
__declspec(noinline) void lb_npc_act_req(_PLW* self, u32 a, s32 b, u16 c);
__declspec(noinline) void fn_8020A5EC(_PLW* self, u8 id);
void fn_8020A5F4(_PLW* self, u32 a, s32 b, s32 c, u16 d);
__declspec(noinline) void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c);

/* The act handlers of the unit before this range (the seam at 0x80207698 is unproven, so they are the
 * neighbour's). */
void fn_80206044(_LB_NPC* self);
void fn_8020623C(_LB_NPC* self);
void fn_80207284(_LB_NPC* self);
void fn_80207698(_LB_NPC* self);

/* The Pl-band helpers, and this group's own neighbours. */
u32 Pl_motion_end_ck(_PLW* self);
u32 game_ready_ck(void);
u32 lb_menu_kind_pair_ck(u8 a, u8 b);
u32 fn_8021B8F4(u8 a, u8 b);
void fn_801FDEE4(_LB_NPC* self, u32 motion);
u32 fn_801FDFD0(_LB_NPC* self);
void fn_801FE0AC(_LB_NPC* self, u16 motion_id, s32 a, s32 b);
void lb_npc_motion_restart(_LB_NPC* self, u16 motion_id);
u32 fn_801FE1EC(_LB_NPC* self);
u32 lb_npc_id_ck(_LB_NPC* self, u16 id);
void fn_801FE200(_LB_NPC* self, u16 a, u16 b);
s32 fn_801FDE3C(_LB_NPC* self, VEC3* target);
}

/* The map spells this range's symbols `fn_XXXXXXXX` (placeholders, not manglings), so every definition
 * below is `extern "C"` - a C++ definition would mangle and pair nothing (playbook 42/48). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x802076D4 - the first act-state handler: faces the move work's target and runs the close/far distance ladder. */
void fn_802076D4(_LB_NPC* self)
{
    u8* work;

    work = (u8*)get_move_work_adrs(2);
    work += (s8)my_player_no() * 0xB20;
    if (lobby_w.field_0x12C == 1) {
        return;
    }
    copyVec3(&self->target_0x1EC, (VEC3*)(work + 0x3C));
    self->field_0x22D = 1;

    switch (self->field_0x006) {
    case 0: {
        f32 near;

        self->field_0x006++;
        near = lbl_80799B18;
        if (calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC) <= near * near) {
            self->field_0x006 = 3;
            fn_801FE0AC(self, 1, 4, 0);
        } else {
            fn_801FE0AC(self, 8, 4, 0);
        }
        break;
    }
    case 1: {
        f32 near = lbl_80799B18;
        f32 far = lbl_80799B1C;
        f32 d = calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC);

        if (d <= near * near) {
            self->field_0x006 = 3;
            fn_801FE0AC(self, 1, 4, 0);
        } else if (d >= far * far) {
            self->field_0x006 = 2;
            fn_801FE0AC(self, 11, 4, 0);
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    }
    case 2: {
        f32 far = lbl_80799B20;

        if (calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC) <= far * far) {
            self->field_0x006 = 0;
        }
        self->field_0x1D4 = (s16)(u16)fn_801FDE3C(self, &self->target_0x1EC) / 10;
        self->field_0x02C += self->field_0x1D4;
        break;
    }
    case 3: {
        f32 far = lbl_80799B20;

        if (calcDistanceSqXZ(&self->pos_0x10, &self->target_0x1EC) >= far * far) {
            self->field_0x006 = 0;
        }
        break;
    }
    }
}

/* 0x80207938 - the second act-state handler: zero a scratch vector, play motion 10, wait for the
 * motion/talk predicates, then clear the action and the +0x208 word. */
__declspec(noinline) void fn_80207938(_LB_NPC* self)
{
    VEC3 scratch;

    VEC3_ctor(&scratch);
    switch (self->field_0x006) {
    case 0:
        self->field_0x006 = 1;
        fn_801FE0AC(self, 2, 4, 0);
        fn_801FDEE4(self, 10);
        break;
    case 1:
        if (fn_801FE1EC(self) == 1 || fn_801FDFD0(self) == 1) {
            self->field_0x006 = 2;
            fn_801FE0AC(self, 1, 4, 0);
        }
        break;
    case 2:
        if (fn_801FDFD0(self) == 1) {
            self->field_0x208 = 0;
            lb_npc_motion_restart(self, 0);
        }
        break;
    }
}

/* 0x80207A1C - the third act-state handler: arm motion 22 once and hand over. */
__declspec(noinline) void fn_80207A1C(_LB_NPC* self)
{
    if (self->field_0x006 == 0) {
        self->field_0x006++;
        fn_801FE0AC(self, 22, 4, 0);
    }
}

/* 0x80207A44 - dispatch the current action id (+0x1CC) onto its per-action handler. */
void fn_80207A44(_LB_NPC* self)
{
    switch (self->field_0x1CC) {
    case 0:
        fn_80207698(self);
        break;
    case 67:
        fn_802076D4(self);
        break;
    case 7:
        fn_80207938(self);
        break;
    case 68:
        fn_80207A1C(self);
        break;
    case 65:
        fn_80206044(self);
        break;
    case 71:
        fn_8020623C(self);
        break;
    case 82:
        fn_80207284(self);
        break;
    }
}

/* 0x80207AA4 - arm the lobby sub-scene latch block for a scene id. */
void fn_80207AA4(u8 scene)
{
    memset(&lb_npc_scene_work, 0, 28);
    lb_npc_scene_work.scene_0x10 = scene;
    lb_npc_scene_work.active_0x02 = 1;
}

/* 0x80207AF8 - whether the latch block is armed and its +3 byte is set. */
u32 fn_80207AF8(void)
{
    if (lb_npc_scene_work.active_0x02 == 0) {
        return 1;
    }
    return lb_npc_scene_work.done_0x03 != 0;
}

/* 0x80207B28 - release the latch block. */
void fn_80207B28(void)
{
    lb_npc_scene_work.ticked_0x11 = 0;
}

/* 0x80207E9C - raises the +0x234 flag with `state` beside it; the two small id decoders follow. */
__declspec(noinline) void lb_npc_event_set(_LB_NPC* self, u8 state)
{
    self->field_0x234[0] = 1;
    self->field_0x234[1] = state;
}

__declspec(noinline) u32 fn_80207EAC(_LB_NPC* self)
{
    u32 value = 0;

    switch (self->field_0x002) {
    case 20:
        value = 0;
        break;
    case 21:
        value = 1;
        break;
    case 22:
        value = 2;
        break;
    }
    return value;
}

__declspec(noinline) u32 fn_802080C0(_LB_NPC* self)
{
    u32 value = 0;

    switch (self->field_0x002) {
    case 10:
        value = 0;
        break;
    case 11:
        value = 1;
        break;
    case 12:
        value = 2;
        break;
    }
    return value;
}

/* ---------------------------------------------------------------------------------------------------
 * The 0x80208928..0x80208B48 block: the lobby-act latch (`_PLW+0xB00`) gate.
 */

/* 0x80208928 - the gate the lobby act handlers test before taking over the player work: same area as
 * the scene, no master act, the talk predicates clear, and the hunter name equal to the move work's. */
u32 fn_80208928(_PLW* self)
{
    u8* work;

    if (self->area_0x16 != lobby_w.area_0x002) {
        return 0;
    }
    if (Pl_master_ck(self) != 0) {
        return 1;
    }
    if (fn_8021B8F4(self->kind_0x015, self->area_0x16) == 1) {
        return 0;
    }
    if (lb_menu_kind_pair_ck(self->kind_0x015, self->area_0x16) != 1) {
        return 1;
    }
    work = (u8*)get_move_work_adrs(2);
    work += (s8)my_player_no() * 0xB20;
    if (memcmp(self->name_0xB05, work + 0xB05, 10) != 0) {
        return 0;
    }
    return 1;
}

/* 0x802089F4 - the id range the lobby act layer scans. */
__declspec(noinline) u16 fn_802089F4(_PLW* self)
{
    return 0xFFFF;
}

/* 0x80208A00 - latch the act step when the master act is running. */
__declspec(noinline) void lb_player_act_latch(_PLW* self)
{
    if (Pl_master_ck(self) == 1) {
        self->field_0xB00 = 2;
    }
}

/* 0x80208A3C - clear the act latch. */
__declspec(noinline) void fn_80208A3C(_PLW* self)
{
    self->field_0xB00 = 0;
}

/* 0x80208A48 - clear the act latch when the decoder says the id is free. */
__declspec(noinline) void fn_80208A48(_PLW* self)
{
    if (fn_80208AC0(self) == 1) {
        self->field_0xB00 = 0;
    }
}

/* 0x80208A84 - whether the act layer is idle: an unknown id and a clear +0x30E byte. */
__declspec(noinline) u32 fn_80208A84(_PLW* self)
{
    int id = self->field_0x00A;

    if (!((u32)(id - 11) <= 1 || id == 9)) {
        return self->flag_0x30E != 0;
    }
    return 0;
}

/* 0x80208AB8 - always "no". */
__declspec(noinline) u32 fn_80208AB8(_PLW* self)
{
    return 0;
}

/* 0x80208AC0 - whether the act latch holds the "armed" step for the lobby act id. */
__declspec(noinline) u32 fn_80208AC0(_PLW* self)
{
    if (self->field_0x00A != 12) {
        return 0;
    }
    return self->field_0xB00 == 2;
}

/* 0x80208AE8 - whether the act latch holds the "taken over" step. */
__declspec(noinline) u32 fn_80208AE8(_PLW* self)
{
    return self->field_0xB00 == 3;
}



} /* extern "C" */
} /* namespace s_802076D4 */

/* ---------------------------------------------------------------------------------------------------
 * The NPC event hooks the per-frame update runs (the sub-scenes, the pig/cat farm tables, talk reactions) and the
 * player act layer's frame (0x80207B3C..0x80208928).
 */
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);
void sysSE_req(s32 id);
namespace s_802076D4 {
extern "C" {
u32 fn_801FE1AC(_LB_NPC* self, u32 motion_id);
u32 fn_801FE4B4(_LB_NPC* self);
u32 lb_npc_model19_variant1_ck(_LB_NPC* self);
void lb_npc_dress_model18(_LB_NPC* self, u32 index);
void fn_801FDFC8(_LB_NPC* self, u8 value);
u32 fn_80207EAC(_LB_NPC* self);
u32 fn_802080C0(_LB_NPC* self);
u8 lb_world_upgrade_get(s32 idx);
u8 lb_world_mode_get(void);
u8 fn_802FB948(void);
extern LbPigRegion** village_pig_release;        /* .sbss: per area, the pig release regions */
extern LbNpcMotionEntry*** move_cat_farm;        /* .sbss: per event state, the cat farm's tables by kind */
extern LbNpcMotionEntry*** move_cat_farm_init;   /* .sbss: the same, entered from the slot setup */

/* 0x80207B3C - steps the running sub-scene on its NPC (scene 0: NPC 0x38's sequence, scene 1: NPC 8's), with a
 * 90-second timeout. */
void lb_npc_scene_step(_LB_NPC* self)
{
    LbNpcSceneWork* work = &lb_npc_scene_work;

    if (work->active_0x02 == 0) {
        return;
    }
    if (work->ticked_0x11 == 0) {
        work->ticked_0x11 = 1;
        if (++work->timer_0x06 >= 5400) {
            work->active_0x02 = 0;
            work->done_0x03 = 1;
            return;
        }
        if (work->delay_0x04 > 0) {
            work->delay_0x04--;
        }
    }
    switch (work->scene_0x10) {
    case 0:
        if (self->field_0x002 == 0x38) {
            switch (work->step_0x00) {
            case 0:
                work->step_0x00++;
                lb_npc_motion_restart(self, 29);
                break;
            case 1:
                if (fn_801FE1AC(self, 0) == 1) {
                    sysSE_bank32_req(21);
                    work->step_0x00++;
                }
                break;
            case 2:
                if (fn_801FE1AC(self, 30) == 1) {
                    work->step_0x00++;
                }
                break;
            case 3:
                if (fn_801FE1AC(self, 0) == 1) {
                    work->step_0x00++;
                    work->active_0x02 = 0;
                    work->done_0x03 = 1;
                }
                break;
            }
        }
        break;
    case 1:
        if (self->field_0x002 == 8) {
            switch (work->step_0x00) {
            case 0:
                work->step_0x00++;
                lb_npc_motion_restart(self, 28);
                break;
            case 1:
                if (fn_801FE1AC(self, 0) == 1) {
                    work->step_0x00++;
                    work->active_0x02 = 0;
                    work->done_0x03 = 1;
                }
                break;
            }
        }
        break;
    default:
        work->active_0x02 = 0;
        work->done_0x03 = 1;
        break;
    }
}

/* 0x80207D14 - model 18's hooks: follow the latched player (motion 40), and apply a pending dressing change. */
void lb_npc_model18_hooks(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 18) != 0) {
        _PLW* plw = (_PLW*)get_move_work_adrs(2);

        plw += (s8)my_player_no();
        if (fn_80208AC0(plw) == 1 && fn_801FE1AC(self, 40) == 0) {
            self->field_0x214 = plw;
            lb_npc_motion_restart(self, 40);
        }
        if (self->field_0x234[0] == 1) {
            lb_npc_dress_model18(self, self->field_0x234[1]);
            self->field_0x234[0] = 0;
        }
    }
}

/* 0x80207DC4 - the pig release table of the region (of the NPC's area) it stands in, its index in `kind`. */
LbNpcMotionEntry* fn_80207DC4(_LB_NPC* self, u8* kind)
{
    VEC3 pos;
    LbNpcMotionEntry* found;
    LbPigRegion* region;
    u8 index;

    index = 0;
    VEC3_ctor(&pos);
    found = 0;
    region = village_pig_release[self->field_0x004];
    if (region != 0) {
        for (; region->spots_0x0 != 0; region++, index++) {
            LbPigSpot* spot;

            for (spot = region->spots_0x0; spot->radius_0xC > 0.0f; spot++) {
                vec_to_mh_vec3(&pos, &spot->pos_0x0);
                if (calcDistanceSqXZ(&pos, &self->pos_0x10) < spot->radius_0xC * spot->radius_0xC) {
                    found = region->table_0x4;
                    *kind = index;
                    break;
                }
            }
        }
    }
    return found;
}

void fn_80207FD0(_LB_NPC* self, u8 kind, s32 flag);

/* 0x80207EEC - the cat farm NPC follows the farm's event state: a new state picks its table. */
void lb_npc_cat_farm_follow(_LB_NPC* self)
{
    if (fn_801FE4B4(self) != 0) {
        u32 kind = fn_80207EAC(self);
        u8 state = lobby_w.npc_event_0x07A[(u8)kind];

        if (state != 0xFF) {
            u8 old = self->field_0x234[0];

            if (old != state) {
                self->field_0x234[0] = state;
                self->field_0x208 = 0;
                switch (old) {
                case 2:
                    self->field_0x204 = lb_npc_move_data.cat_farm_0x08;
                    break;
                case 3:
                    self->field_0x204 = lb_npc_move_data.cat_farm_0x04;
                    break;
                case 4:
                    self->field_0x204 = lb_npc_move_data.cat_farm_0x00;
                    break;
                default:
                    fn_80207FD0(self, kind, 1);
                    return;
                }
                lb_npc_motion_restart(self, self->field_0x204->motion_0x0C);
            }
        }
    }
}

/* 0x80207FD0 - the cat farm NPC's table for event state +0x234 and farm kind `kind` (shifted past the farm
 * upgrades), from the running or the setup set. */
void fn_80207FD0(_LB_NPC* self, u8 kind, s32 flag)
{
    LbNpcMotionEntry** tables;
    LbNpcMotionEntry* table;

    if (self->field_0x234[0] == 0xFF) {
        return;
    }
    if (self->field_0x234[0] == 1) {
        switch (lb_world_upgrade_get(0)) {
        case 1:
            if (kind >= 1) {
                kind += 2;
            }
            break;
        case 2:
            if (kind >= 2) {
                kind += 2;
            }
            break;
        }
    }
    if ((u8)flag == 0) {
        tables = move_cat_farm_init[self->field_0x234[0]];
    } else {
        tables = move_cat_farm[self->field_0x234[0]];
    }
    self->field_0x208 = 0;
    table = tables[kind];
    self->field_0x204 = table;
    lb_npc_motion_restart(self, table->motion_0x0C);
}

/* 0x80208100 - model 12 follows its talk id's event state: a new state picks its table; a busy state holds the
 * NPC's secondary action and state 2 hides it. */
void lb_npc_model12_follow(_LB_NPC* self)
{
    if (lb_npc_id_ck(self, 12) != 0) {
        u32 id = fn_802080C0(self);
        u8 state = lobby_w.slots_0x07D[(u8)id];

        if (self->field_0x234[0] != state) {
            self->field_0x234[0] = state;
            self->field_0x208 = 0;
            if ((u8)id == 0 && lb_world_mode_get() == 1) {
                self->field_0x204 = lb_npc_move_data.model12_mode1_0x34;
            } else {
                self->field_0x204 = lb_npc_move_data.model12_0x30[(u8)id];
            }
            lb_npc_motion_restart(self, self->field_0x204->motion_0x0C);
        }
        if (self->field_0x234[0] != 0) {
            fn_801FDFC8(self, 1);
        }
        if (self->field_0x234[0] == 2) {
            self->field_0x001 = 0;
            self->field_0x003 = 0;
        }
    }
}

/* 0x80208200 - keeps the models' props in step with their motions (model 19's node 21, model 11's node 22, model
 * 7's node 24 once its event is on). */
void lb_npc_prop_sync(_LB_NPC* self)
{
    if (fn_801FE4B4(self) == 1) {
        if ((u32)(lb_npc_Get_motion_no(self) - 1177) <= 1) {
            if (self->field_0x1E0 == 0) {
                self->field_0x1E0 = 1;
                fn_801FE200(self, 21, 1);
            }
        } else if (self->field_0x1E0 == 1) {
            self->field_0x1E0 = 0;
            fn_801FE200(self, 21, 0);
        }
    } else if (lb_npc_id_ck(self, 11) == 1) {
        if ((u32)(lb_npc_Get_motion_no(self) - 1023) <= 2) {
            if (self->field_0x1E0 == 1) {
                self->field_0x1E0 = 0;
                fn_801FE200(self, 22, 0);
            }
        } else if (self->field_0x1E0 == 0) {
            self->field_0x1E0 = 1;
            fn_801FE200(self, 22, 1);
        }
    } else if (lb_npc_id_ck(self, 7) == 1 && fn_802FB948() == 1 && self->field_0x1E0 == 0) {
        self->field_0x1E0 = 1;
        fn_801FE200(self, 24, 0);
    }
}

/* 0x80208360 - starts the reaction the talk mode +0x255 asks of this model kind; 1 when one started. */
u32 lb_npc_talk_reaction_start(_LB_NPC* self)
{
    if (self->field_0x255 == 0xFF) {
        return 0;
    }
    if (lb_npc_id_ck(self, 13) == 1) {
        if (self->field_0x255 == 0) {
            lb_npc_motion_restart(self, 13);
            self->field_0x255 = 0xFF;
            return 1;
        }
    } else if (lb_npc_id_ck(self, 2) == 1) {
        if (self->field_0x255 == 1) {
            lb_npc_motion_restart(self, 17);
            self->field_0x255 = 0xFF;
            return 1;
        }
    } else if (lb_npc_id_ck(self, 7) == 1) {
        if (self->field_0x255 == 2) {
            lb_npc_motion_restart(self, 19);
            self->field_0x255 = 0xFF;
            return 1;
        }
    } else if (lb_npc_id_ck(self, 4) == 1) {
        if (self->field_0x255 == 3) {
            lb_npc_motion_restart(self, 30);
            self->field_0x255 = 0xFF;
            return 1;
        }
    } else if (lb_npc_model19_variant1_ck(self) == 1 && self->field_0x255 == 4) {
        lb_npc_motion_restart(self, 62);
        self->field_0x255 = 0xFF;
    }
    return 0;
}

} /* extern "C" */
} /* namespace s_802076D4 */
namespace s_802076D4 {
extern "C" {


/* ---------------------------------------------------------------------------------------------------
 * The lobby player's per-frame frame and draw steps and the party reset (0x802084BC..0x80208928).
 */
} /* extern "C" */
} /* namespace s_802076D4 */
#include "Pl/pl_se_rig.h"
u32 event_demo_ck(void);
namespace s_802076D4 {
extern "C" {
void pl_parts_refresh(void);
u32 event_demo_running_ck(void);
u32 fn_80208928(_PLW* self);
void lb_player_first_frame(_PLW* self);
void lb_player_update(_PLW* self);
void lb_player_motion_effects(_PLW* self);
u32 split_screen_size_class(_PLW* self);
void yure_player_move(_PLW* self);
void score_add_clamped(s32 value, s32* score);
void quest_board_flags_send(u8 a, u8 b);
extern LbNpcUserData* lobby_world_block;

/* 0x802084BC - the local player drops his pad history while the lobby is held (+0x09) and his part table's first
 * word while a screen is open. */
void lb_player_input_hold(_PLW* self)
{
    if (Pl_master_ck(self) == 1) {
        if (lobby_w.community_flag_0x009 == 1) {
            memset(&self->field_0x0B8, 0, 0x84);
        } else if (lobby_w.active_0x008 == 1) {
            self->part_tbl_b_0xD4[0] = 0;
        }
    }
}

/* 0x8020852C - the lobby player's frame: set up on the first frame, then (outside demos) run the act layer. */
void lb_player_frame(_PLW* self)
{
    if (self->slot_active == 0) {
        return;
    }
    pl_parts_refresh();
    self->drawn_0x01A = 0;
    switch (self->field_0x004) {
    case 0:
        if (pl_net_slot_ready_ck(self) != 0) {
            lb_player_first_frame(self);
        }
        break;
    case 1:
        pl_input_clear(self);
        if (event_demo_running_ck() == 0 && event_demo_ck() == 0) {
            lb_player_input_hold(self);
            if (fn_80208928(self) == 0) {
                self->field_0x001 = 0;
            } else {
                self->field_0x001 = 1;
            }
            if (Pl_master_ck(self) == 0 && NetCtrlWk::hasFriendDetail((NetId*)self->hunter_0x5CA.id_0x5DB) == 1) {
                self->field_0xB04 = 0;
                self->field_0xB01 = 0;
                self->field_0xB02 = 0;
                self->field_0x001 = 0;
            }
            lb_player_update(self);
        }
        break;
    }
}

/* 0x80208630 - draws the lobby player: the local player's models take the split-screen class and the camera's
 * yaw, the hidden ones skip their shadow, then the effects and the shake. */
void lb_player_draw(_PLW* self)
{
    s32 i;
    s32 hide;
    PlSeModel* model;
    u32 flags;

    if (self->drawn_0x01A == 0) {
        return;
    }
    model = ((PlSeRig*)self->physics_0x13C)->models_0x004;
    if (Pl_master_ck(self) == 1) {
        flags = split_screen_size_class(self) | 0x200;
        if (Pl_master_ck(self) == 1 && system_lobby_flag_get() != 0) {
            model->model.field_0x28 = 0;
            if (system_lobby_flag_get() != 0) {
                model->model.field_0x2C = self->field_0x032;
            } else {
                model->model.field_0x2C = 0;
            }
            model->model.field_0x30 = 0;
            mhchar_model_mtx_update(&model->model);
        }
    } else {
        flags = 0;
    }
    if (system_lobby_flag_get() == 2 || self->field_0x19 > 0) {
        hide = 1;
    } else {
        hide = 0;
    }
    for (i = 0; i < 7; i++, model++) {
        if (model->model.field_0x118 != 0) {
            if (hide != 0) {
                model->model.field_0x34 = 0;
            }
            mhchar_sound_job_request(&model->model, flags);
        }
    }
    if (system_lobby_flag_get() == 1 || self->field_0x19 > 0) {
        hide = 1;
    } else {
        hide = 0;
    }
    model = ((PlSeRig*)self->physics_0x13C)->sub_0x0A14;
    for (i = 0; i < 3; i++, model++) {
        if (model->model.field_0x118 != 0) {
            if (hide != 0) {
                model->model.field_0x34 = 0;
            }
            mhchar_sound_job_request(&model->model, flags);
        }
    }
    lb_player_motion_effects(self);
    yure_player_move(self);
}

/* 0x802087D4 - resets the lobby party state when the party is dissolved: refunds the held fee, clears the
 * party bits and leaves the party screens. */
void lb_party_state_reset(void)
{
    _PLW* plw = (_PLW*)get_move_work_adrs(2);

    plw += (s8)my_player_no();
    plw->field_0xB01 &= 0xF8;
    if (lobby_w.page_0x052 == 1 && lb_param_w.field_0x04 != 0) {
        score_add_clamped(lb_param_w.field_0x04, &lobby_world_block->money_0x18);
    }
    lb_param_w.field_0x04 = 0;
    lb_param_w.field_0x00 = 0;
    lobby_w.page_0x052 = 0;
    lobby_w.field_0x050 = 0;
    quest_board_flags_send(plw->field_0xB01, plw->field_0xB02);
    switch (lobby_w.state_0x000) {
    case 20:
    case 10:
        lobby_w.state_0x000 = 0;
        lobby_w.active_0x008 = 0;
        break;
    }
}

/* 0x802088B4 - false while the player's free-hunt screen (6) is blocked or he is in act 6/7 of family 0. */
u32 lb_player_screen_ok_ck(_PLW* self)
{
    if (lobby_w.state_0x000 == 6 && lb_screen_work_busy_ck() == 0) {
        return 0;
    }
    if (self->field_0x00A == 0 && (u32)(self->act_no - 6) <= 1) {
        return 0;
    }
    return 1;
}



/* ---------------------------------------------------------------------------------------------------
 * The lobby player's setup, placement and per-frame update, and its footstep effects (0x80208B94..0x8020A058).
 */
} /* extern "C" */
} /* namespace s_802076D4 */
u8 get_now_areano(void);
u8 get_now_mapno(void);
void eft004_set_pl2(_PLW* self, u8 type, u32 joint, u32 a, f32 scale, f32 b, f32 c);
void eft007_set(_PLW* self, u8 a, u8 b, u32 c, VEC3* d, f32 e);
void eft013_set(_PLW* self, u8 kind);
void* LbStr(u8 kind, u16 idx);
namespace s_802076D4 {
extern "C" {
u32 lb_player_menu_area_ck(_PLW* self);
LbNpcGround* em_land_data_init(LbNpcGround* out);
_SLOTENT* userdata_equip_item_slots_get(LbNpcUserData* data);
void item_pair_copy(_SLOTENT* dst, _SLOTENT* src);
void lb_player_act_run(_PLW* self);
void lb_player_trade_notice(_PLW* self);
void lb_player_turn_step(_PLW* self);
void lb_player_remote_ease(_PLW* self);
void pl_rig_model_move(PlSeRig* rig);
s32 hud_msg_push(u32 ch, const char* text);
u16 fn_802089F4(_PLW* self);
void addVec3To(VEC3* a, VEC3* b);
void yure_player_init(_PLW* self);
void yure_player_clear_flags(_PLW* self);
u8 GameMode_ck(void);
char* strcpy(char* dst, const char* src);
struct _se_w* se_entry_request(s32 kind, _PLW* plw, void (*func)(_PLW*, s32));
u32 lobby_net_master_ck(_PLW* plw);
void lobby_net_action_update(_PLW* plw, u16 action);
void lobby_net_state_request_send(u32 state);
void lobby_net_profile_part1_send(LbNpcUserData* data);
void lobby_net_place_send(_PLW* self, u8 area, VEC3* pos, u16 angle);
void quest_board_flags_send(u8 a, u8 b);
void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c);
VEC3* stage_entry_pos_get(u8 map, u8 area);
u32 stage_entry_angle_get(u8 map, u8 area);
VEC3* setVec3(VEC3* v, f32 x, f32 y, f32 z);
u32 userdata_progress_flag_ck(s32 flag);
u8 pl_act_stage_get(_PLW* self);
void pl_act_stage_latch_set(_PLW* self, u8 stage);
void lobby_player_place(u8 mode, VEC3* pos, u16 angle);
s32 lb_panel_close(void);
int memcmp(const void* a, const void* b, u32 n);
extern LbNpcUserData* lobby_world_block;
extern LbNpcSystemWork system_w;

void lb_player_place_entry(_PLW* self, u8 map, u8 area);

/* 0x80208B94 - the lobby player's first frame: reset the act state, place him (the local one, or one not yet
 * shown), arm the idle act, his SE handles and, for the local player online, the session's view of him. */
void lb_player_first_frame(_PLW* self)
{
    MHchar* model = &((PlSeRig*)self->physics_0x13C)->models_0x004[0].model;

    self->field_0x004++;
    self->field_0x267 = 1;
    self->field_0x001 = 1;
    self->kind_0x09 = 0;
    self->field_0x354 = 1.0f;
    self->drawn_0x01A = 0;
    self->field_0x0B0 = 0;
    self->field_0x266 = 0;
    self->field_0x369 = 0;
    if (Pl_master_ck(self) == 1 || self->field_0xB04 == 0) {
        u8 area = get_now_areano();

        lb_player_place_entry(self, get_now_mapno(), area);
        fn_8020A70C(self, 0, 0, 0);
        if (game_ready_ck() == 1) {
            lb_npc_act_req(self, 0, 0, (u16)0x8000);
        }
    } else {
        self->kind_0x015 = get_now_mapno();
    }
    self->field_0x580 = 0;
    self->field_0x45A = 0;
    self->field_0x45C = 0;
    self->field_0x396 = 0;
    self->field_0x38A = 0;
    self->field_0x352 = 10;
    self->field_0xB1A = 0;
    self->field_0x036 = 0;
    pl_skill_update(self, 0, 0);
    pl_skill_update2(self, 0);
    eft_pl_model_slot_bind(self);
    ef_pl_effects_init(self);
    yure_player_init(self);
    pl_model_pos_sync(self);
    mhchar_model_mtx_update(model);
    if (Pl_master_ck(self) == 1 && GameMode_ck() == 2) {
        strcpy(self->hunter_0x5CA.id_0x5DB, lobby_hunter_cards[0].id_0x03);
    }
    self->field_0xAF4 = se_entry_request(1, self, pl_se_frame_callback);
    self->field_0xAF8 = se_entry_request(2, self, 0);
    self->field_0xAFC = se_entry_request(3, self, lb_pl_se_frame_callback);
    if (Pl_master_ck(self) == 1 && lobby_net_master_ck(self) == 1) {
        lobby_net_action_update(self, 0x800C);
        lobby_net_state_request_send(0);
        lobby_net_profile_part1_send(lobby_world_block);
        self->field_0xB01 = 0;
        self->field_0xB02 = 0;
        quest_board_flags_send(0, 0);
    }
}

/* 0x80208DCC - places the lobby player for map `map`, area `area`: the area entry (online), the quest-return gate,
 * the first visit's spot, or the entry point after a quest; then snaps his walk target and model to it. */
void lb_player_place_entry(_PLW* self, u8 map, u8 area)
{
    VEC3 near_pos;
    VEC3 far_pos;
    VEC3 gate_pos;
    MHchar* model = &((PlSeRig*)self->physics_0x13C)->models_0x004[0].model;
    struct Q_ResultWork* result = get_qResult_work();

    self->kind_0x015 = map;
    self->area_0x16 = area;
    if (NetCtrlWk::isCityModeAlias() == 0) {
        if (game_ready_ck() == 0) {
            self->vec_0x03C.x = -896.0f;
            self->vec_0x03C.y = 179.0f;
            self->vec_0x03C.z = -259.0f;
            self->rot_0x54.y = 0x8000;
        }
    } else {
        VEC3* pos;
        u8 entry_area;

        entry_area = get_now_areano();
        pos = stage_entry_pos_get(get_now_mapno(), entry_area);
        self->vec_0x03C.x = pos->x;
        self->vec_0x03C.y = pos->y;
        self->vec_0x03C.z = pos->z;
        entry_area = get_now_areano();
        self->rot_0x54.y = stage_entry_angle_get(get_now_mapno(), entry_area);
    }
    if (system_w.field_0x02D != 0) {
        system_w.field_0x02D = 0;
        if (system_w.field_0x7CE == 2) {
            copyVec3(&self->vec_0x03C, setVec3(&near_pos, 505.0f, 461.0f, 343.0f));
        } else {
            copyVec3(&self->vec_0x03C, setVec3(&far_pos, 1600.0f, 458.0f, -354.0f));
        }
        system_w.field_0x7CE = 0;
        self->rot_0x54.y = 0xC000;
        self->field_0x0A8 = 0xC000;
    } else if (system_w.field_0x7D3 == 1 && system_w.field_0x86A == 1) {
        copyVec3(&self->vec_0x03C, setVec3(&gate_pos, 0.0f, 0.0f, 127.0f));
        self->rot_0x54.y = 0x8000;
    } else if (game_ready_ck() == 0 && Pl_master_ck(self) == 1) {
        if (userdata_progress_flag_ck(99) == 0) {
            self->vec_0x03C.x = -100.0f;
            self->vec_0x03C.y = 179.0f;
            self->vec_0x03C.z = -319.0f;
            self->rot_0x54.y = 0x8000;
        } else if (result == 0 || result->kind_0x1E3 == 0) {
            VEC3* pos;
            u8 entry_area;

            entry_area = get_now_areano();
            pos = stage_entry_pos_get(get_now_mapno(), entry_area);
            self->vec_0x03C.x = pos->x;
            self->vec_0x03C.y = pos->y;
            self->vec_0x03C.z = pos->z;
            entry_area = get_now_areano();
            self->rot_0x54.y = stage_entry_angle_get(get_now_mapno(), entry_area);
        }
    }
    self->rot_0x54.x = 0;
    self->rot_0x54.z = 0;
    self->field_0x0A8 = self->rot_0x54.y;
    copyVec3(&self->target_pos_0x090, &self->vec_0x03C);
    lb_npc_walk_blend_set(self, 0);
    pl_model_pos_sync(self);
    mhchar_model_mtx_update(model);
}

/* 0x80209070 - places the local player at spawn point `pos` facing `angle` in map `map`, area `area`, and resets
 * his act and the session's view of him. */
void lb_player_spawn_set(VEC3* pos, u16 angle, u8 map, u8 area)
{
    MHchar* model;
    _PLW* plw;
    _PLW* base = (_PLW*)get_move_work_adrs(2);

    plw = &base[(s8)my_player_no()];
    model = &((PlSeRig*)plw->physics_0x13C)->models_0x004[0].model;
    lb_player_place_entry(plw, map, area);
    plw->field_0x0A8 = angle;
    plw->rot_0x54.y = angle;
    copyVec3(&plw->vec_0x03C, pos);
    plw->field_0x352 = 5;
    plw->field_0x0B0 = 0;
    plw->field_0x0B2 = 0;
    plw->field_0x65C = 0;
    plw->field_0x65D = 0;
    plw->blend_frames_0x0B4 = 0;
    plw->field_0x36A = 0;
    if (plw->field_0xB03 == 1) {
        plw->field_0xB03 = 2;
    } else if (plw->field_0xB03 == 2) {
        plw->field_0xB03 = 0;
    }
    fn_8020A70C(plw, plw->kind_0x09, 0, 0);
    if (game_ready_ck() == 1) {
        lb_npc_act_req(plw, 0, 0, (u16)0x8000);
    }
    pl_push_clear(plw);
    pl_model_pos_sync(plw);
    mhchar_model_mtx_update(model);
    yure_player_clear_flags(plw);
    switch (pl_act_stage_get(plw)) {
    case 5:
        if (lobby_net_master_ck(plw) == 1) {
            lobby_net_action_update(plw, 0x800C);
            lobby_net_state_request_send(0);
            plw->field_0xB01 = 0;
            plw->field_0xB02 = 0;
            quest_board_flags_send(plw->field_0xB01, plw->field_0xB02);
        }
        break;
    case 6:
        plw->field_0xB1A = 60;
        break;
    }
    pl_act_stage_latch_set(plw, 0);
}

/* 0x80209328 - online, sends a local player latched in a paired menu area (and not listed ready among the
 * session's peers) back to the plaza spot; 1 when it did. */
u32 lb_player_menu_area_return(_PLW* self)
{
    s32 i;
    LobbyPeerCopy* peer;
    LobbyTimerBlock* state;
    VEC3 pos;
    s32 seen;

    state = &lobby_state_block;
    setVec3(&pos, 83.71f, 7.41f, -886.41f);
    if (lobby_w.area_change_0x026 != 0) {
        return 0;
    }
    seen = 0;
    if (lb_player_menu_area_ck(self) != 1) {
        return 0;
    }
    for (i = 0, peer = state->peers_0x0008; i < 10; i++, peer++) {
        if ((s8)peer->id_0x00[0] != 0) {
            if (memcmp(self->name_0xB05, peer->id_0x00, 10) == 0) {
                if (state->peers_0x0008[i].card_0x20[0x92] != 0) {
                    return 0;
                }
                break;
            }
            if (++seen >= state->peer_count_0x0004) {
                break;
            }
        }
    }
    if (lobby_w.state_0x000 != 0) {
        lb_panel_close();
    }
    lb_npc_act_req(self, 0, 0, (u16)0x8000);
    lobby_player_place(3, &pos, 0);
    pl_act_stage_latch_set(self, 6);
    lobby_net_place_send(self, 3, &pos, 0);
    return 1;
}

/* 0x80209488 - the lobby player's per-frame update: the act layer and its latches, the turn and walk, the rig, then
 * the ground under him and the paired-area check. */
void lb_player_update(_PLW* self)
{
    PlSeRig* rig = (PlSeRig*)self->physics_0x13C;
    LbNpcGround ground;
    f32 height;

    em_land_data_init(&ground);
    copyVec3(&self->vec_0x048, &self->vec_0x03C);
    self->field_0x5C9 = 0;
    self->flag_0x30E = 0;
    self->field_0x30A = 1;
    self->field_0x268 = 0;
    if (Pl_master_ck(self) == 1) {
        _SLOTENT* slots = userdata_equip_item_slots_get(lobby_world_block);
        s32 i;

        for (i = 0; i < 24; i++) {
            item_pair_copy(&self->slot_id[i], &slots[i]);
        }
        self->deco_skill_id[0] = lb_param_w.value_0x16;
        self->deco_skill_id[1] = lb_param_w.value_0x18;
        self->deco_skill_id[2] = lb_param_w.value_0x1A;
        self->deco_skill_id[3] = lb_param_w.value_0x1C;
    }
    pl_act_status_step(self);
    if (self->field_0xB1A > 0) {
        if (--self->field_0xB1A == 0) {
            hud_msg_push(0, (const char*)LbStr(3, 489));
        }
    }
    pl_skill_update(self, 0, 0);
    pl_skill_update2(self, 0);
    pl_gauge_step_a(self);
    pl_gauge_step_b(self);
    pl_condition_step(self);
    if (self->field_0x00A == 12 && fn_80208AC0(self) == 0 && fn_802089F4(self) == 0xFFFF && Pl_act_ck(self, 12, 6) == 0) {
        self->field_0x5C4 = 0;
        self->field_0xB00 = 0;
        fn_8020A70C(self, 0, 4, 0);
    }
    pl_equip_refresh(self);
    lb_player_act_run(self);
    self->field_0x654 = 0;
    if (self->act_state_0x00E[0] != 0) {
        lb_player_act_run(self);
    }
    if ((u32)(self->field_0x00A - 11) <= 1) {
        self->field_0x30A = 0;
    } else if (self->field_0x00A == 0 && self->act_no != 0) {
        self->field_0x30A = 0;
    }
    if (Pl_act_ck(self, 0, 1) == 1) {
        self->field_0x268 = 1;
    }
    lb_player_trade_notice(self);
    pl_motion_speed_apply(self);
    lb_player_turn_step(self);
    lb_player_remote_ease(self);
    pl_model_pos_sync(self);
    self->drawn_0x01A = 1;
    pl_rig_draw_mode_begin(rig);
    pl_rig_model_move(rig);
    pl_rig_draw_mode_end(rig);
    copyVec3(&self->vec_0x03C, &rig->models_0x004[0].model.pos_0x04);
    if (Pl_master_ck(self) != 0) {
        pl_coll_shape_set(self, 0, 194);
        if (pl_ground_check(self, &self->vec_0x03C, (LbNpcGround*)&self->field_0x5A4, &height, -6) == 1) {
            self->ground_y_0x060 = height;
            self->vec_0x03C.y = height;
        }
        if (game_ready_ck() == 0) {
            if (pl_ground_check(self, &self->vec_0x03C, &ground, &height, 4) == 1) {
                self->field_0x064 = height;
                self->field_0x074 = 1;
            } else {
                self->field_0x074 = 0;
            }
        }
    } else if (pl_ground_check(self, &self->vec_0x03C, (LbNpcGround*)&self->field_0x5A4, &height, -6) == 1) {
        self->ground_y_0x060 = height;
        self->vec_0x03C.y = height;
    }
    lb_player_menu_area_return(self);
    pl_model_pos_sync(self);
    mhchar_model_mtx_update(&rig->models_0x004[0].model);
}

/* 0x802097E8 - the lobby player's footstep, splash and steam effects by motion and frame (the hot-spring map 22 has
 * its own set). */
void lb_player_motion_effects(_PLW* self)
{
    VEC3 offset;

    VEC3_ctor(&offset);
    if ((u8)get_now_mapno() == 22) {
        switch (Get_motion_no(self)) {
        case 2:
            if ((self->field_0x5A4 & 6) != 0) {
                if (Pl_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                    eft004_set_pl2(self, 2, 17, 1.0f, 0.0f, 0.0f, 0);
                }
                if (Pl_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                    eft004_set_pl2(self, 2, 20, 1.0f, 0.0f, 0.0f, 0);
                }
                if (Pl_frame_check(self, 0, 92.0f, 0.0f) == 1) {
                    eft004_set_pl2(self, 2, 17, 1.0f, 0.0f, 0.0f, 0);
                }
            }
            break;
        case 3:
            if (Pl_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 17, 20, 1.0f, 0.0f, 0.0f, 0);
            }
            if (Pl_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 17, 17, 1.0f, 0.0f, 0.0f, 0);
            }
            if (Pl_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 17, 20, 1.0f, 0.0f, 0.0f, 0);
            }
            if (Pl_frame_check(self, 0, 56.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 17, 17, 1.0f, 0.0f, 0.0f, 0);
            }
            break;
        case 30:
            if (Pl_frame_check(self, 0, 2.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 15, 17, 1.0f, 0.0f, 0.0f, 0);
                    eft004_set_pl2(self, 2, 17, 1.0f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 10, 17, 1.0f, 0.0f, 0.0f, 0);
                }
            }
            break;
        case 52:
            if (Pl_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 17, 17, 0.6f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 0, 17, 0.6f, 0.0f, 0.0f, 0);
                }
            }
            if (Pl_frame_check(self, 0, 48.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 17, 20, 0.6f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 0, 20, 0.6f, 0.0f, 0.0f, 0);
                }
            }
            break;
        case 53:
            if (Pl_frame_check(self, 0, 14.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 17, 17, 0.8f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 0, 17, 0.8f, 0.0f, 0.0f, 0);
                }
            }
            if (Pl_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 17, 20, 0.8f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 0, 20, 0.8f, 0.0f, 0.0f, 0);
                }
            }
            break;
        case 59:
            if (Pl_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 17, 17, 0.6f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 0, 17, 0.6f, 0.0f, 0.0f, 0);
                }
            }
            if (Pl_frame_check(self, 0, 22.0f, 0.0f) == 1) {
                if ((self->field_0x5A4 & 6) != 0) {
                    eft004_set_pl2(self, 17, 20, 0.6f, 0.0f, 0.0f, 0);
                } else {
                    eft004_set_pl2(self, 0, 20, 0.6f, 0.0f, 0.0f, 0);
                }
            }
            break;
        case 326:
            if (Pl_frame_check(self, 0, 194.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 50.0f, 170.0f);
                rotVecY(&offset, self->rot_0x54.y);
                addVec3To(&offset, &self->vec_0x03C);
                eft004_set(5, &offset, 1.0f, 0, self->area_0x16);
            }
            break;
        case 327:
        case 328:
            if (system_w.frame_0x00C % 50 == 0) {
                eft013_set(self, 25);
            }
            break;
        }
    } else {
        switch (Get_motion_no(self)) {
        case 326:
            if (self->field_0x001 != 0 && Pl_frame_check(self, 0, 194.0f, 0.0f) == 1) {
                setVector3(&offset, 0.0f, 50.0f, 170.0f);
                rotVecY(&offset, self->rot_0x54.y);
                addVec3To(&offset, &self->vec_0x03C);
                eft004_set(5, &offset, 1.0f, 0, self->area_0x16);
            }
            break;
        case 327:
        case 328:
            if (system_w.frame_0x00C % 50 == 0) {
                eft013_set(self, 25);
            }
            break;
        case 631:
        case 632:
            if ((system_w.frame_0x00C & 0x1F) == 0) {
                eft007_set(self, 19, 0, 13, 0, 1.0f);
            }
            break;
        case 633:
            if (Pl_frame_check(self, 0, 2.0f, 0.0f) == 1 || Pl_frame_check(self, 0, 68.0f, 0.0f) == 1 ||
                Pl_frame_check(self, 0, 88.0f, 0.0f) == 1) {
                eft007_set(self, 19, 0, 13, 0, 1.0f);
            }
            break;
        case 634:
            if (Pl_frame_check(self, 0, 2.0f, 0.0f) == 1 || Pl_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                eft007_set(self, 19, 0, 13, 0, 1.0f);
            }
            if (Pl_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                eft004_set_pl2(self, 5, 3, 1.0f, 0.0f, 0.0f, 0);
            }
            break;
        }
    }
}



/* ---------------------------------------------------------------------------------------------------
 * The player act layer's latch, turn and walk helpers and the idle/sit act family (0x80208AFC..0x8020AEF0).
 */
void lb_player_walk_hook(_PLW* self);
u32 menu_busy_ck(void);
u32 stage_work_idle_ck(void);
void lb_kitchen_open(_PLW* self, u32 kind);
u32 lb_menu_kind_pair_ck(u8 a, u8 b);
s32 lb_cmd_pressed_ck(u16 mask);
f32 vec3_dist_sq(VEC3* a, VEC3* b);

/* 0x80208AFC - drops the "taken over" act latch of the local player. */
void lb_player_latch_release(_PLW* self)
{
    if (Pl_master_ck(self) == 1 && fn_80208AE8(self) == 1) {
        self->field_0xB00 = 0;
    }
}

/* 0x80208B48 - true for the act family 0 acts that keep the player busy (3-4, 8-11, 16, 26-35). */
s32 lb_npc_act_busy_ck(u32 kind, u32 act)
{
    if ((u8)kind == 0) {
        s32 id = (u16)act;

        if ((u32)(id - 26) <= 9 || (u32)(id - 8) <= 3 || (u32)(id - 3) <= 1 || id == 16) {
            return 1;
        }
    }
    return 0;
}

/* 0x80209218 - true when the player faces away from his move target by a quarter turn or more (outside the
 * lobby act latches). */
u32 lb_player_facing_away_ck(_PLW* self)
{
    if (fn_80208AB8(self) != 0 || fn_80208AC0(self) != 0 || fn_80208AE8(self) != 0) {
        return 0;
    }
    pl_act_set_cam_ang(self);
    return (u32)((u16)(self->field_0x0A8 + 0x10000 - self->rot_0x54.y) - 0x6000) <= 0x4000;
}

/* 0x802092AC - true online while the local player stands latched in one of the paired menu areas. */
u32 lb_player_menu_area_ck(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        return 0;
    }
    if (game_ready_ck() == 0) {
        return 0;
    }
    if (lb_menu_kind_pair_ck(self->kind_0x015, self->area_0x16) == 1 && self->field_0xB03 == 2) {
        return 1;
    }
    return 0;
}

/* 0x8020A058 - starts a `frames`-frame walk of the player to his move target (snaps there when it is 800 units
 * away or more, or for no frames). */
void lb_npc_walk_blend_set(_PLW* self, s32 frames)
{
    if (vec3_dist_sq(&self->vec_0x03C, &self->target_pos_0x090) >= 640000.0f || (s16)frames == 0) {
        self->blend_frames_0x0B4 = 0;
        copyVec3(&self->vec_0x03C, &self->target_pos_0x090);
        self->blend_step_x_0x09C = 0.0f;
        self->blend_step_y_0x0A0 = 0.0f;
        self->blend_step_z_0x0A4 = 0.0f;
        return;
    }
    self->blend_frames_0x0B4 = frames;
    self->blend_step_x_0x09C = (self->target_pos_0x090.x - self->vec_0x03C.x) / (s16)frames;
    self->blend_step_y_0x0A0 = (self->target_pos_0x090.y - self->vec_0x03C.y) / (s16)frames;
    self->blend_step_z_0x0A4 = (self->target_pos_0x090.z - self->vec_0x03C.z) / (s16)frames;
}

/* 0x8020A158 - eases a remote player's heading to his target and steps the walk blend. */
void lb_player_remote_ease(_PLW* self)
{
    if (Pl_master_ck(self) == 0) {
        self->rot_0x54.y += (s16)(self->field_0x0A8 - self->rot_0x54.y) / 4;
    }
    if (self->blend_frames_0x0B4 > 0) {
        if (self->blend_frames_0x0B4-- >= 0) {
            self->vec_0x03C.x += self->blend_step_x_0x09C;
            self->vec_0x03C.y += self->blend_step_y_0x0A0;
            self->vec_0x03C.z += self->blend_step_z_0x0A4;
        }
    }
}

/* 0x8020A1F4 - turns the local player towards his target heading at the act's turn rate and leans the body into
 * the turn. */
void lb_player_turn_step(_PLW* self)
{
    s32 rate = 4004;
    u16 diff;
    s32 lean;

    if (Pl_master_ck(self) == 0) {
        self->field_0x0B0 = 0;
        return;
    }
    if (self->field_0x00A == 1) {
        switch (self->act_no) {
        case 1:
        case 4:
            rate = 1820;
            break;
        case 2:
            rate = 1456;
            break;
        default:
            rate = 4004;
            break;
        }
    } else if (self->field_0x00A == 12) {
        switch (self->act_no) {
        case 2:
        case 5:
            rate = 1820;
            break;
        case 3:
            rate = 1456;
            break;
        default:
            rate = 4004;
            break;
        }
    }
    diff = self->field_0x0A8 - self->rot_0x54.y;
    switch (Get_motion_no(self)) {
    case 3:
    case 53:
        lean = 128;
        break;
    case 4:
        lean = 160;
        break;
    default:
        lean = 0;
        break;
    }
    if ((u16)(diff + rate) < (u32)(rate * 2)) {
        self->rot_0x54.y = self->field_0x0A8;
        if (self->field_0x0B0 + 768 <= 1536) {
            self->field_0x0B0 = 0;
        } else if (self->field_0x0B0 < 0x8000) {
            self->field_0x0B0 -= 768;
        } else {
            self->field_0x0B0 += 768;
        }
    } else if (diff < 0x8000) {
        self->rot_0x54.y = (u16)(self->rot_0x54.y + rate);
        self->field_0x0B0 -= lean;
    } else {
        self->rot_0x54.y = (u16)(self->rot_0x54.y - rate);
        self->field_0x0B0 += lean;
    }
    if ((u16)(self->field_0x0B0 + 0x10000 - 2560) <= 60416) {
        if (self->field_0x0B0 >= 0x8000) {
            self->field_0x0B0 = 0xF600;
        } else {
            self->field_0x0B0 = 2560;
        }
    }
}

/* 0x8020A714 - re-arms the idle motion that matches the act latch (5 when taken over, 1 otherwise); 1 when it
 * changed the motion. */
u32 lb_player_idle_motion_sync(_PLW* self, u32 taken)
{
    if ((u8)taken == 0) {
        if (fn_80208AE8(self) == 1) {
            Pl_chr_setX(self, 5, 6, 0);
            return 1;
        }
    } else if (fn_80208AE8(self) == 0) {
        Pl_chr_setX(self, 1, 6, 0);
        return 1;
    }
    return 0;
}

/* 0x8020A79C - the idle act: count the idle frames, fall into the long-idle poses after half a second and keep the
 * idle motion in step with the act latch. */
void lb_act_idle(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0;
        fn_8020A5EC(self, 0);
        self->field_0x006 = 0;
    case 1:
        self->field_0x28++;
        if (++self->field_0x036 >= 150) {
            self->field_0x036 = 150;
        }
        switch (Get_motion_no(self)) {
        case 1:
            if (lb_player_idle_motion_sync(self, 0) == 0 && self->field_0x28 > 30) {
                if (self->se_name_set == 0) {
                    Pl_chr_set_attr_default(self, 66, -6, 0);
                } else {
                    Pl_chr_set_attr_default(self, 63, -6, 0);
                }
            }
            break;
        case 5:
            if (lb_player_idle_motion_sync(self, 1) == 0 && Pl_master_ck(self) == 1 && self->field_0x28 > 60) {
                lb_npc_act_req(self, 0, 37, 0);
                return;
            }
            break;
        case 21:
            if (lb_player_idle_motion_sync(self, 1) == 0 && Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 5, 0, 145);
                self->field_0x28 = 0;
            }
            break;
        case 63:
            if (lb_player_idle_motion_sync(self, 0) == 0 && Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 65, -2, 0);
            }
            break;
        case 66:
            if (lb_player_idle_motion_sync(self, 0) == 0 && Pl_motion_end_ck(self) == 1) {
                Pl_chr_set_attr_default(self, 67, -2, 0);
            }
            break;
        case 65:
        case 67:
            lb_player_idle_motion_sync(self, 0);
            break;
        default:
            if (lb_player_idle_motion_sync(self, 0) == 0) {
                Pl_chr_setX(self, 1, -4, 0);
            }
            break;
        }
        if (self->field_0x006 == 0) {
            self->field_0x006++;
        } else {
            lb_player_walk_hook(self);
        }
        break;
    }
}

/* 0x8020AA2C - the bench act: sit down (or start seated), then take the bench menu's commands. */
void lb_act_bench(_PLW* self, s32 seated)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        self->field_0x18 = 0;
        if (Pl_master_ck(self) == 1) {
            lobby_w.field_0x005 = 0;
        }
        if (seated == 1) {
            self->act_step_0x05 = 2;
            self->field_0x28 = 60;
            Pl_chr_setX(self, 609, 6, 0);
        } else {
            Pl_chr_set_attr_default(self, 608, -4, 0);
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->field_0x28 = 60;
            self->act_step_0x05++;
            Pl_chr_setX(self, 609, 4, 0);
            self->act_no = 4;
            Pl_master_ck(self);
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1 && lobby_w.field_0x000 == 0 && menu_busy_ck() == 0) {
            if (game_ready_ck() == 1) {
                if (lb_cmd_pressed_ck(0x80) != 0) {
                    lobby_w.field_0x005 ^= 1;
                }
                if (self->field_0xB01 == 0 && stage_work_idle_ck() == 1 && lb_cmd_pressed_ck(0x10) != 0) {
                    lb_kitchen_open(self, 0);
                } else if (lb_cmd_pressed_ck(0x20) != 0) {
                    lb_npc_act_set(self, 0, 5, 0x20);
                } else if (lb_cmd_pressed_ck(0x100) != 0) {
                    lb_npc_act_set(self, 0, 27, 0x20);
                } else if (lb_cmd_pressed_ck(0x200) != 0) {
                    lb_npc_act_set(self, 0, 26, 0x20);
                } else if (lb_cmd_pressed_ck(1) != 0) {
                    lb_npc_act_set(self, 0, 32, 0x20);
                } else if (lb_cmd_pressed_ck(2) != 0) {
                    lb_npc_act_set(self, 0, 30, 0x20);
                } else if (lb_cmd_pressed_ck(4) != 0) {
                    lb_npc_act_set(self, 0, 28, 0x20);
                } else if (lb_cmd_pressed_ck(8) != 0) {
                    lb_npc_act_set(self, 0, 34, 0x20);
                }
            } else if (lb_cmd_pressed_ck(0x20) != 0) {
                lb_npc_act_set(self, 0, 5, 0x20);
            }
        }
        break;
    }
}

/* 0x8020ADD8 - the doze act: nod off into motion 327 and now and then stir (328). */
void lb_act_doze(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 326, 6, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_setX(self, 327, 0, 0);
        }
        break;
    case 2:
        if (Get_motion_no(self) == 327 && ((u16)ran_suu(1) & 0x3F) == 0) {
            Pl_chr_setX(self, 328, 0, 0);
        } else if (Get_motion_no(self) == 328 && Pl_motion_end_ck(self) != 0) {
            Pl_chr_setX(self, 327, 0, 0);
        }
        break;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * The act entry (0x8020A3E4): its callers see it through two prototypes, so it is defined on its own.
 */
} /* extern "C" */
} /* namespace s_802076D4 */
#include "menu/menu_item.h"   /* menu_work, the trade partner name the act entry copies */
namespace lb_npc_act_entry {
extern "C" {
u32 lobby_net_master_ck(_PLW* plw);
void lobby_net_trade_send(_PLW* plw);
void lobby_net_action_update(_PLW* plw);
char* strcpy(char* dst, const char* src);

/* 0x8020A3E4 - enters act `sub` of act family `act`: clears the act state and scratch, keeps (flag 0x20) or re-rolls
 * the act's random byte and tells the session (a trade act, flag 0x10, also sends the partner's name). */
void lb_npc_act_set(_PLW* self, u8 act, u16 sub, u32 flags)
{
    s32 i;

    self->field_0x35C = 0;
    self->field_0x360 = 0;
    self->act_step_0x05 = 0;
    self->field_0x006 = 0;
    self->field_0x007 = 0;
    self->act_state_0x00E[1] = self->field_0x00A;
    self->prev_act_no = self->act_no;
    self->field_0x00A = act;
    self->act_no = sub;
    self->field_0x264 = 0;
    if (((u16)flags & 0x20) != 0) {
        u8 low = self->field_0x0B6;

        self->field_0x0B6 = (ran_suu(1) & 0xFF00) | low;
    } else {
        self->field_0x0B6 = ran_suu(1);
    }
    self->field_0x354 = 1.0f;
    self->field_0x318 = 0;
    self->field_0x313 = 0;
    self->field_0x31C = 0;
    self->field_0x320 = 0;
    self->field_0x31E = 0;
    for (i = 0; i < 0x30; i++) {
        self->field_0x322[i] = 0;
    }
    pl_model_hide(self);
    pl_model_aux_flag_clear(self);
    if (lobby_net_master_ck(self) == 1 && ((u16)flags & 2) == 0) {
        if (((u16)flags & 0x10) != 0) {
            strcpy(self->partner_0xB0F, menu_work.slot[0].label_0x1C8);
            lobby_net_trade_send(self);
        } else {
            lobby_net_action_update(self);
        }
    }
}

} /* extern "C" */
} /* namespace lb_npc_act_entry */
namespace s_802076D4 {
extern "C" {

/* ---------------------------------------------------------------------------------------------------
 * The 0x8020A5xx act-family entry points.
 */

/* 0x8020A5D4 - set the act latch byte at +0xE and re-enter the family's main handler. */
__declspec(noinline) void lb_npc_act_req(_PLW* self, u32 a, s32 b, u16 c)
{
    self->act_state_0x00E[0] = 1;
    lb_npc_act_set(self, (u8)a, (u16)b, (u16)c);
}

/* 0x8020A5EC - store the act id byte. */
__declspec(noinline) void fn_8020A5EC(_PLW* self, u8 id)
{
    self->kind_0x09 = id;
}

/* 0x8020A70C - the five-argument form of the family's main handler, with the trailing word clear. */
__declspec(noinline) void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c)
{
    fn_8020A5F4(self, a, b, c, 0);
}

/* 0x8020A5F4 - the act-family handler: arm the act id, set the +0x30E flag and select the motion
 * from the latch state. */
void fn_8020A5F4(_PLW* self, u32 id, s32 arg, s32 speed, u16 flags)
{
    if (speed > 0) {
        speed = -speed;
    }
    self->kind_0x09 = (u8)id;
    self->flag_0x30E = 1;
    if (fn_80208AB8(self) != 0 || fn_80208AC0(self) != 0) {
        Pl_chr_set_attr_default(self, 51, speed, arg);
        lb_npc_act_req(self, 12, 0, flags);
    } else if (fn_80208AE8(self) == 1) {
        Pl_chr_set_attr_default(self, 21, speed, arg);
        lb_npc_act_req(self, 0, 0, flags);
    } else {
        Pl_chr_set_attr_default(self, 1, speed, arg);
        lb_npc_act_req(self, 0, 0, flags);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * The 0x8020AD14..0x8020B264 act state machines: wait for the frame predicate, then hand the motion to
 * the player character setter.
 */

/* 0x8020AD14 - */
void fn_8020AD14(_LB_NPC* self)
{
    switch (self->field_0x005) {
    case 0: {
        u8 index;

        self->field_0x005++;
        fn_8020A5EC((_PLW*)self, 0);
        Pl_chr_setX((_PLW*)self, 610, 4, 0);
        if (Pl_master_ck((_PLW*)self) == 1 && game_ready_ck() == 1) {
            index = self->field_0x182;
            checkOtherInvite(index + 1, (s8*)&lobby_w.param_0x12F);
        }
        break;
    }
    case 1:
        if (Pl_motion_end_ck((_PLW*)self) == 1) {
            fn_8020A70C((_PLW*)self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020AEF0 - */
void fn_8020AEF0(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0: {
        u32 timer;

        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        timer = self->field_0x058 + 0x18000;
        self->field_0x058 = timer & 0xFFFF;
        self->field_0x0A8 = timer & 0xFFFF;
        Pl_chr_setX(self, 329, 0, 0);
        break;
    }
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 6, 0);
        }
        break;
    }
}

/* 0x8020AF90 - */
void fn_8020AF90(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_setX(self, 602, 4, 0);
        fn_8020A5EC(self, 1);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            Pl_chr_setX(self, 609, 6, 0);
            lb_npc_act_set(self, 0, 4, 32);
        }
        break;
    }
}

/* 0x8020B264 - */
void fn_8020B264(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_setX(self, 207, 2, 0);
        fn_8020A5EC(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}


/* ---------------------------------------------------------------------------------------------------
 * The 0x8020B02C..0x8020C588 player act handlers (sit, greet, invite walks, emotes).
 */
} /* extern "C" */
} /* namespace s_802076D4 */
s32 ran_suu(s32 n);
namespace s_802076D4 {
extern "C" {
void eft051_spawn(_PLW* self, u8 kind);
void lb_npc_walk_blend_set(_PLW* plw, s32 frames);
u32 menu_busy_ck(void);
s32 lobby_cmd_release_ck(u16 mask);
s32 lb_cmd_pressed_ck(u16 mask);
void pl_model_set_state(_PLW* self, u8 state);
LbSpot* lb_spot_find(_PLW* self, s16 kind, s16 id);
#pragma peephole off

/* 0x8020B02C - the sit-down act: sit for 10 frames with effect `kind`, then stand up into motion 609. */
void lb_act_sit(_PLW* self, u32 kind)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 622, 6, 0);
        self->field_0x28 = 10;
        eft051_spawn(self, (u8)kind);
        pl_model_visible_set(self, 1);
        fn_8020A5EC(self, 1);
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            self->act_step_0x05++;
            pl_model_hide(self);
        } else {
            pl_model_visible_set(self, 1);
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_setX(self, 609, 6, 0);
        }
        break;
    }
}

/* 0x8020B124 - Turns the seated player (motion 0x26F), then sits on (0xD2) for a random 60..91 frames before standing up. */

void lb_act_seat_turn(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_setX(self, 0x26F, 4, 0);
        fn_8020A5EC(self, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_setX(self, 0xD2, 0, 0x92);
            self->field_0x0A8 += 0x4000;
            self->rot_0x54.y += 0x4000;
            self->field_0x28 = ((u16)ran_suu(1) & 0x1F) + 0x3C;
            return;
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1) {
            if (--self->field_0x28 <= 0) {
                if ((Pl_master_ck(self) == 1) && (game_ready_ck() == 1)) {
                    checkOtherInvite((u8) ((u8) self->field_0x0B6 + 1), (s8*)&lobby_w.param_0x12F);
                }
                lb_npc_act_set(self, 0, 0xD, 0x20);
            }
        }
        break;
    }
}

/* 0x8020B2EC - The greeting variant: plays the greeting motions, spawns its effect and answers a pending invite. */

void lb_act_greet_variant(_PLW* self, s32 a2) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        if ((u8) a2 == 2) {
            self->field_0xB00 = 3;
        }
        Pl_chr_setX(self, 0x262, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            if ((u8) a2 == 1) {
                Pl_chr_setX(self, 0x133, 6, 0x14);
                ef_inst_spawn(self, 0);
                return;
            }
            Pl_chr_setX(self, 0x270, 6, 0);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            if ((Pl_master_ck(self) == 1) && (game_ready_ck() == 1)) {
                checkOtherInvite((u8) ((u8) self->field_0x0B6 + 1), (s8*)&lobby_w.param_0x12F);
            }
            fn_8020A70C(self, 0, 6, 0);
        }
        break;
    }
}

/* 0x8020B438 - arms the greeting motion (620, or 621 for the second name set). */
void lb_act_greet(_PLW* self)
{
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 620, 2, 0);
        } else {
            Pl_chr_setX(self, 621, 2, 0);
        }
        fn_8020A5EC(self, 1);
    }
}

/* 0x8020B4B0 - arms motion 603. */
void lb_act_pose603(_PLW* self)
{
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        Pl_chr_setX(self, 603, 6, 0);
        fn_8020A5EC(self, 0);
    }
}

/* 0x8020B508 - Plays the return-from-pose motion (0x26B), then hands back to the idle act. */
void lb_act_pose_return(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        if (fn_80208AC0(self) != 0) {
            fn_8020A70C(self, 0, 0x32, 0);
            return;
        }
        Pl_chr_setX(self, 0x26B, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8020B5B8 - the lobby spot of kind `kind` and id `id` in the player's area (the online or offline table), or
 * null. */
LbSpot* lb_spot_find(_PLW* self, s16 kind, s16 id)
{
    LbSpot* spot;

    if (game_ready_ck() == 1) {
        spot = lb_spot_net_tbl[self->area_0x16];
    } else {
        spot = lb_spot_tbl[self->area_0x16];
    }
    for (; spot->kind_0x00 != 0; spot++) {
        if (spot->kind_0x00 == kind && spot->id_0x02 == id) {
            return spot;
        }
    }
    return 0;
}

/* 0x8020B668 - walks the player to his invite spot, waits for the answer and leaves the act. */
void lb_act_invite_walk(_PLW* self, s32 kind)
{
    switch (self->act_step_0x05) {
    case 0: {
        LbSpot* spot;

        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 630, 4, 0);
        spot = lb_spot_find(self, 8, (u8)self->field_0x0B6);
        if (spot != 0) {
            vec_to_mh_vec3(&self->target_pos_0x090, (Vec*)&spot->pos_0x04);
            self->field_0x0A8 = spot->angle_0x10;
            lb_npc_walk_blend_set(self, 20);
        }
        break;
    }
    case 1:
        if (Pl_master_ck(self) == 1 && lobby_w.field_0x145 != 0) {
            self->act_step_0x05++;
            self->field_0x28 = 20;
            if (Pl_master_ck(self) == 1 && game_ready_ck() == 1) {
                checkOtherInvite((u8)((u8)self->field_0x0B6 + 13), (s8*)&lobby_w.param_0x131);
            }
        }
        break;
    case 2:
        if (--self->field_0x28 <= 0) {
            lb_npc_act_req(self, 0, 0, 0x8000);
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020B7D0 - The invite pose: plays motions 0x277/0x278, answers a pending invite, then requests the next act. */

void lb_act_invite_pose(_PLW* self, s32 a2) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0;
        fn_8020A5EC(self, 0);
        if (a2 == 0) {
            Pl_chr_setX(self, 0x277, 4, 0);
            return;
        }
        Pl_chr_setX(self, 0x278, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            if (a2 == 0) {
                Pl_chr_set_attr_default(self, 0x277, 0x12, 0x158);
            } else {
                Pl_chr_set_attr_default(self, 0x278, 0x12, 0x158);
            }
            if ((Pl_master_ck(self) == 1) && (lobby_w.field_0x145 != 0)) {
                self->act_step_0x05++;
                self->field_0x28 = 0x14;
                if (game_ready_ck() == 1) {
                    checkOtherInvite((u8) ((u8) self->field_0x0B6 + 0xD), (s8*)&lobby_w.param_0x131);
                }
            }
        }
        break;
    case 2:
        if (--self->field_0x28 <= 0) {
            lb_npc_act_req(self, 0, 0, 0x8000);
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020B954 - The invite jump: plays the act's motion, answers a pending invite, then walks back to the spot. */

void lb_act_invite_jump(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x27A, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x1F8, 6, 0);
            if ((Pl_master_ck(self) == 1) && (game_ready_ck() == 1)) {
                checkOtherInvite((u8) ((u8) self->field_0x0B6 + 0xD), (s8*)&lobby_w.param_0x131);
                return;
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xCF, 4, 0x3C);
            return;
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            if (Pl_master_ck(self) == 1) {
                lobby_w.state_0x000 = 0;
            }
            self->field_0x0A8 = self->rot_0x54.y;
            copyVec3(&self->target_pos_0x090, &self->vec_0x03C);
            lb_npc_walk_blend_set(self, 0);
            lb_npc_act_req(self, 0, 0, 0x8000);
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020BAE0 - The invite wave: plays the act's motion, answers a pending invite, then returns to idle. */

void lb_act_invite_wave(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x279, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x1F6, 0, 0x16);
            if ((Pl_master_ck(self) == 1) && (game_ready_ck() == 1)) {
                checkOtherInvite((u8) ((u8) self->field_0x0B6 + 0xD), (s8*)&lobby_w.param_0x131);
                return;
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x1F6, 0, 0x24);
            return;
        }
        break;
    case 3:
        if (Pl_frame_check(self, 1, 126.0f, 0.0f) == 1) {
            if (Pl_master_ck(self) == 1) {
                lobby_w.state_0x000 = 0;
            }
            fn_8020A70C(self, 0, 0xE, 0);
        }
        break;
    }
}

/* 0x8020BC3C - Plays the stretch motion (0x25C), then returns to idle. */
void lb_act_stretch(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 0x25C, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0x157, 4, 0x2C);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD8, 6, 0x24);
            return;
        }
        break;
    case 3:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 8, 0);
            self->field_0x313 = 2;
        }
        break;
    }
}

/* 0x8020BD3C - Plays emote motion 0x263 once, then returns to act 4. */
void lb_act_emote_611(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x263, 6, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_set(self, 0, 4, 0x20);
        }
        return;
    }
}

/* 0x8020BDC4 - Plays emote motion 0x264 once, then returns to act 4. */
void lb_act_emote_612(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x264, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_set(self, 0, 4, 0x20);
        }
        return;
    }
}

/* 0x8020BE4C - Plays emote motion 0x268 and holds it until a pad command ends it (act 0x1D). */

void lb_act_emote_616_hold(_PLW* self) {
    s32 temp_r3;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x268, 8, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->field_0x28 = 0x3C;
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        if (Pl_master_ck(self) == 1) {
            temp_r3 = self->field_0x28;
            if (temp_r3 <= 0) {
                if ((menu_busy_ck() == 0) && ((lobby_cmd_release_ck(0x3C00) != 0) || (lb_cmd_pressed_ck(0x20) != 0))) {
                    lb_npc_act_set(self, 0, 0x1D, 0x20);
                    return;
                }
            } else {
                self->field_0x28 = temp_r3 - 1;
            }
        }
        break;
    }
}

/* 0x8020BF44 - Plays emote motion 0x269 once, then returns to act 4. */
void lb_act_emote_617(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x269, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_set(self, 0, 4, 0x20);
        }
        return;
    }
}

/* 0x8020BFCC - Plays emote motion 0x265 and holds it until a pad command ends it (act 0x1F). */

void lb_act_emote_613_hold(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x265, 6, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        if ((Pl_master_ck(self) == 1) && (menu_busy_ck() == 0) && ((lobby_cmd_release_ck(0x3C00) != 0) || (lb_cmd_pressed_ck(0x20) != 0))) {
            lb_npc_act_set(self, 0, 0x1F, 0x20);
        }
        break;
    }
}

/* 0x8020C0A4 - Plays emote motion 0x266 once, then returns to act 4. */
void lb_act_emote_614(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x266, 8, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_set(self, 0, 4, 0x20);
        }
        return;
    }
}

/* 0x8020C12C - Plays emote motion 0x267 and holds it until a pad command ends it (act 0x21). */

void lb_act_emote_615_hold(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x267, 6, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        if ((Pl_master_ck(self) == 1) && (menu_busy_ck() == 0) && ((lobby_cmd_release_ck(0x3C00) != 0) || (lb_cmd_pressed_ck(0x20) != 0))) {
            lb_npc_act_set(self, 0, 0x21, 0x20);
        }
        break;
    }
}

/* 0x8020C204 - Plays emote motion 0x272 once, then returns to act 4. */
void lb_act_emote_626(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x272, 8, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_set(self, 0, 4, 0x20);
        }
        return;
    }
}

/* 0x8020C28C - Plays emote motion 0x26A and holds it until a pad command ends it (act 0x23). */

void lb_act_emote_618_hold(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x26A, 6, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            return;
        }
        break;
    case 2:
        if ((Pl_master_ck(self) == 1) && (menu_busy_ck() == 0) && ((lobby_cmd_release_ck(0x3C00) != 0) || (lb_cmd_pressed_ck(0x20) != 0))) {
            lb_npc_act_set(self, 0, 0x23, 0x20);
        }
        break;
    }
}

/* 0x8020C364 - Plays emote motion 0x271 once, then returns to act 4. */
void lb_act_emote_625(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_setX(self, 0x271, 8, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_set(self, 0, 4, 0x20);
        }
        return;
    }
}

/* 0x8020C3EC - Plays the act's held emote, switching the model state on its frames, until a pad command ends it (act 0x26). */

void lb_act_emote_627_hold(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_set_attr_default(self, 0x273, 4, 0);
        break;
    case 1:
        if ((Pl_master_ck(self) == 1) && (Pl_frame_check(self, 1, 480.0f, 0.0f) == 1)) {
            pl_model_set_state(self, 2);
            if ((menu_busy_ck() == 0) && ((lobby_cmd_release_ck(0x3C00) != 0) || (lb_cmd_pressed_ck(0x20) != 0))) {
                lb_npc_act_set(self, 0, 0x26, 0);
            }
        }
        return;
    }
}

/* 0x8020C4C8 - Plays the act's emote once, then returns to idle. */
void lb_act_emote_628(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 1);
        Pl_chr_set_attr_default(self, 0x274, 8, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD8, 4, 0x36);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}


#pragma peephole on
#ifdef __cplusplus
}
#endif

} /* namespace s_802076D4 */

/* The player-character control band (0x8020C588-0x80212810). */
#include "menu/menu_message.h"
/* C++-linkage declarations of this section: they stay at global scope (their manglings are made there). */
void* LbStr(u8 kind, u16 idx);
s32 get_fade_stat(s32 slot);
#include "types.h"


/* Retail keeps the accessors' `bl lobby_input_locked_ck` out of line and the species-id index's unfused `clrlwi` + `slwi`. */
#pragma dont_inline on
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * The 0x8020C588..0x8020EBF4 player act handlers and their family dispatchers.
 */
s32 ran_suu(s32 n);
void fade_set(s32 slot, s32 table_index);
u32 event_demo_ck(void);
#include "lobby/lb_npc_func.h"
namespace lb_npc_player_acts {
extern "C" {
extern LbNpcUserData* lobby_world_block;
void lobby_player_place(u8 mode, VEC3* pos, u16 angle);
s32 lb_cmd_pressed_ck(u16 mask);
s32 lobby_cmd_release_ck(u16 mask);
void fn_8020A5EC(_PLW* self, u8 id);
void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c);
void lb_npc_act_req(_PLW* self, u32 a, s32 b, u16 c);
void lb_npc_act_set(_PLW* self, u32 a, u16 b, u16 c);
u32 fn_80208AB8(_PLW* self);
u32 fn_80208AC0(_PLW* self);
u32 fn_80208AE8(_PLW* self);
void fn_80208A3C(_PLW* self);
void fn_80208A48(_PLW* self);
void lb_player_latch_release(_PLW* self);
void lb_npc_walk_blend_set(_PLW* plw, s32 frames);
LbSpot* lb_spot_find(_PLW* self, s16 kind, s16 id);
void lb_act_idle(_PLW* self);
void lb_act_bench(_PLW* self, s32 seated);
void fn_8020AD14(_PLW* self);
void lb_act_doze(_PLW* self);
void fn_8020AEF0(_PLW* self);
void fn_8020AF90(_PLW* self);
void lb_act_sit(_PLW* self, u32 kind);
void lb_act_seat_turn(_PLW* self);
void fn_8020B264(_PLW* self);
void lb_act_greet_variant(_PLW* self, s32 kind);
void lb_act_greet(_PLW* self);
void lb_act_pose603(_PLW* self);
void lb_act_pose_return(_PLW* self);
void lb_act_invite_walk(_PLW* self, s32 kind);
void lb_act_invite_pose(_PLW* self, s32 kind);
void lb_act_invite_jump(_PLW* self);
void lb_act_invite_wave(_PLW* self);
void lb_act_stretch(_PLW* self);
void lb_act_emote_611(_PLW* self);
void lb_act_emote_612(_PLW* self);
void lb_act_emote_616_hold(_PLW* self);
void lb_act_emote_617(_PLW* self);
void lb_act_emote_613_hold(_PLW* self);
void lb_act_emote_614(_PLW* self);
void lb_act_emote_615_hold(_PLW* self);
void lb_act_emote_626(_PLW* self);
void lb_act_emote_618_hold(_PLW* self);
void lb_act_emote_625(_PLW* self);
void lb_act_emote_627_hold(_PLW* self);
void lb_act_emote_628(_PLW* self);
void lb_player_spot_walk(_PLW* self, LbSpot* spot);
void lb_arm_wrestle_open(_PLW* self);
void lb_player_walk_hook(_PLW* self);
void lb_player_carry_hook(_PLW* self);
void lb_npc_talk_partner_set(_PLW* self, _LB_NPC* npc);
u32 Pl_motion_end_ck(_PLW* self);
void Pl_chr_set_attr_default(_PLW* self, u16 motion, s32 a, s32 b);
void pl_act_set_cam_ang(_PLW* self);
u8 pl_act_param_tier_ck(_PLW* self, u32 idx);
u32 pl_part_flag_ck(_PLW* self, u32 part);
void pl_act_set_flag(_PLW* self, u32 flag);
void pl_model_set_state(_PLW* self, u8 state);
u32 game_ready_ck(void);
u32 menu_busy_ck(void);
s32 hud_msg_push(u32 ch, const char* text);
void userdata_progress_flag_set(u32 flag);
void cockpit_ticket_acquire(_PLW* self, u16* tickets);
void place_info_set(u8* info);
void lobby_pig_dress_view_set(u8 dress);
void lobby_net_place_send(_PLW* self, u8 area, VEC3* pos, u16 angle);
int strcmp(const char* a, const char* b);
VEC3* setVec3(VEC3* v, f32 x, f32 y, f32 z);
extern u16 lb_slot_act_tbl[];            /* .data: the act each armed item slot runs (0xFFFF ends it) */
extern LbMotionArgs lb_item_motion_tbl[];      /* .data: the looping item acts' motions */
extern LbMotionArgs lb_item_motion_once_tbl[]; /* .data: the one-shot item acts' motions */
}
extern "C" {
/* 0x8020C588 - The gate demo: fades out, sets the progress flag, runs the gate scene and shows its message before idling. */

void lb_act_gate_demo(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0x1E;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, 6, 0);
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            self->act_step_0x05++;
            fade_set(0, 1);
            self->field_0x28 = 0x14;
            return;
        }
        break;
    case 2:
        if (--self->field_0x28 <= 0) {
            self->act_step_0x05++;
            fade_set(0, 2);
            self->field_0x28 = 0xA;
            userdata_progress_flag_set(0x58);
            lb_pig_dress_refresh();
            lb_scene_npc_refresh_all();
            self->vec_0x03C.x = 1700.0f;
            self->vec_0x03C.y = 358.0f;
            self->field_0x0A8 = 0xFC72;
            self->rot_0x54.y = 0xFC72;
            return;
        }
        break;
    case 3:
        if (--self->field_0x28 <= 0) {
            self->act_step_0x05++;
            self->field_0x28 = 0x96;
            Pl_chr_set_attr_default(self, 0x1F6, 4, 0);
            hud_msg_push(0, (const char*)LbStr(3, 0x1E7));
            bgm_lobby_return_req();
            return;
        }
        break;
    case 4:
        if (--self->field_0x28 <= 0) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020C720 - Shows the act's message after its wait, then returns to idle. */

void lb_act_msg_wait(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0x1E;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, 6, 0);
        hud_msg_push(0, (const char*)LbStr(3, 0x1E8));
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            fn_8020A70C(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8020C7CC - waits for the invite answer: accepted walks to the spot, refused waits 20 frames and leaves. */
void lb_act_invite_answer(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, -6, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (lobby_w.invite_0x12E == 1) {
                LbSpot* spot = lb_spot_find(self, 5, (u8)self->field_0x0B6);

                if (spot == 0) {
                    if (game_ready_ck() == 1) {
                        checkOtherInvite((u8)((u8)self->field_0x0B6 + 1), (s8*)&lobby_w.param_0x12F);
                    }
                } else {
                    lb_player_spot_walk(self, spot);
                }
            } else if (lobby_w.invite_0x12E < 0) {
                self->field_0x28 = 20;
                self->act_step_0x05++;
            }
        } else {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    case 2:
        if (--self->field_0x28 <= 0) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020C91C - Waits a short count in the stand motion, then returns to idle. */
void lb_act_wait_short(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, 6, 0);
        self->field_0x28 = 0x1E;
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            fn_8020A70C(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8020C9B0 - waits for the second invite answer: accepted enters act 19/20, refused waits 20 frames. */
void lb_act_invite_answer2(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, 6, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (lobby_w.invite_0x130 == 1) {
                lb_arm_wrestle_open(self);
                if ((u8)self->field_0x0B6 == 0) {
                    lb_npc_act_set(self, 0, 19, 0x20);
                } else {
                    lb_npc_act_set(self, 0, 20, 0x20);
                }
            } else if (lobby_w.invite_0x130 < 0) {
                self->field_0x28 = 20;
                self->act_step_0x05++;
            }
        } else {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    case 2:
        if (--self->field_0x28 <= 0) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020CAE8 - Holds the carried item: spawns its effect and, for the master player, requests the next act once the part flag clears. */

void lb_act_hold_item(_PLW* self) {
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x3C, 4, 0);
        self->act_end_request = 0;
        eft004_hold_item_spawn(self);
        break;
    case 1:
        if ((Pl_master_ck(self) == 1) && ((pl_part_flag_ck(self, 4) == 1) || (self->act_end_request != 0))) {
            lb_npc_act_req(self, 0, 2, 0);
        }
        return;
    }
}

/* 0x8020CBA8 - Plays the put-down motion, then returns to idle. */
void lb_act_put_item(_PLW* self) {
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x3D, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8020CC38 - the ticket act: hold until the handler is asked to end or 600 frames pass. */
void lb_act_ticket(_PLW* self)
{
    self->act_handler_entered = 1;
    cockpit_ticket_acquire(self, &self->field_0x598);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, 6, 0);
        self->act_end_request = 0;
        self->field_0x28 = 0;
        break;
    case 1:
        if (Pl_master_ck(self) != 0 && (++self->field_0x28 > 600 || self->act_end_request != 0)) {
            if (self->act_end_request == 1) {
                lb_npc_act_req(self, 0, 44, 0);
            } else {
                lb_npc_act_req(self, 0, 0, 0x8000);
            }
        }
        break;
    }
}

/* 0x8020CD30 - Plays the act's pose motion, then returns to idle. */
void lb_act_pose512(_PLW* self) {
    self->act_handler_entered = 1;
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x200, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8020CDC0 - Shows the master player's message after the stand motion, then returns to idle. */

void lb_act_msg_wait2(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x28 = 0x3C;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 1, 6, 0);
        if (Pl_master_ck(self) == 1) {
            hud_msg_push(0, (const char*)LbStr(3, 0x1EB));
            return;
        }
        break;
    case 1:
        if (--self->field_0x28 <= 0) {
            fn_8020A70C(self, 0, 4, 0);
        }
        break;
    }
}

/* 0x8020CE7C - Runs act family 0 (idle, seats, greetings, invites and emotes) by the act number. */
void lb_act_family0(_PLW* self) {
    u16 temp_r0;

    temp_r0 = self->act_no;
    switch (temp_r0) {
    case 0:
        lb_act_idle(self);
        break;
    case 1:
        lb_act_hold_item(self);
        break;
    case 2:
        lb_act_put_item(self);
        break;
    case 3:
        lb_act_bench(self, 0);
        break;
    case 4:
        lb_act_bench(self, 1);
        break;
    case 5:
        fn_8020AD14(self);
        break;
    case 6:
        lb_act_doze(self);
        break;
    case 7:
        fn_8020AEF0(self);
        break;
    case 8:
        fn_8020AF90(self);
        break;
    case 9:
        lb_act_sit(self, 0);
        break;
    case 10:
        lb_act_sit(self, 1);
        break;
    case 11:
        lb_act_sit(self, 2);
        break;
    case 12:
        lb_act_seat_turn(self);
        break;
    case 13:
        fn_8020B264(self);
        break;
    case 14:
        lb_act_greet_variant(self, 0);
        break;
    case 15:
        lb_act_greet_variant(self, 1);
        break;
    case 16:
        lb_act_greet(self);
        break;
    case 17:
        lb_act_pose603(self);
        break;
    case 18:
        lb_act_pose_return(self);
        break;
    case 19:
        lb_act_invite_walk(self, 0);
        break;
    case 20:
        lb_act_invite_walk(self, 1);
        break;
    case 21:
        lb_act_invite_pose(self, 0);
        break;
    case 22:
        lb_act_invite_pose(self, 1);
        break;
    case 23:
        lb_act_invite_jump(self);
        break;
    case 24:
        lb_act_invite_wave(self);
        break;
    case 25:
        lb_act_stretch(self);
        break;
    case 26:
        lb_act_emote_611(self);
        break;
    case 27:
        lb_act_emote_612(self);
        break;
    case 28:
        lb_act_emote_616_hold(self);
        break;
    case 29:
        lb_act_emote_617(self);
        break;
    case 30:
        lb_act_emote_613_hold(self);
        break;
    case 31:
        lb_act_emote_614(self);
        break;
    case 32:
        lb_act_emote_615_hold(self);
        break;
    case 33:
        lb_act_emote_626(self);
        break;
    case 34:
        lb_act_emote_618_hold(self);
        break;
    case 35:
        lb_act_emote_625(self);
        break;
    case 36:
        lb_act_greet_variant(self, 2);
        break;
    case 37:
        lb_act_emote_627_hold(self);
        break;
    case 38:
        lb_act_emote_628(self);
        break;
    case 39:
        lb_act_gate_demo(self);
        break;
    case 40:
        lb_act_msg_wait(self);
        break;
    case 41:
        lb_act_invite_answer(self);
        break;
    case 42:
        lb_act_wait_short(self);
        break;
    case 43:
        lb_act_invite_answer2(self);
        break;
    case 45:
        lb_act_ticket(self);
        break;
    case 44:
        lb_act_pose512(self);
        break;
    case 46:
        lb_act_msg_wait2(self);
        return;
    default:
        return;
    }
}

/* 0x8020CF90 - The stand act: picks the stand motion, applies the walk tier and the camera angle, then hands over to the walk hook. */
void lb_act_stand(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x266 = 0xA;
        if (fn_80208AE8(self) == 1) {
            Pl_chr_setX(self, 0x258, 4, 0);
        } else if (self->se_name_set == 0) {
            Pl_chr_setX(self, 2, 6, 0);
        } else {
            Pl_chr_setX(self, 0x40, 6, 0);
        }
        fn_8020A5EC(self, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
            } else if (self->field_0x266 == 0) {
                fn_8020A70C(self, 0, 6, 0);
            }
        }
        break;
    }
    lb_player_walk_hook(self);
}

/* 0x8020D0A8 - the slow walk: step one unit a frame along the facing while the stick is held. */
void lb_act_walk(_PLW* self)
{
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        if (fn_80208AE8(self) == 1) {
            Pl_chr_setX(self, 39, 4, 0);
        } else {
            Pl_chr_setX(self, 3, 6, 0);
            self->field_0x266 = 10;
        }
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                step.x = 0.0f;
                step.y = 0.0f;
                step.z = 1.0f;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.y += step.y;
                self->vec_0x03C.z += step.z;
            } else if (self->field_0x266 <= 0) {
                lb_npc_act_set(self, 1, 3, 0);
            }
        }
        break;
    }
    lb_player_walk_hook(self);
}

/* 0x8020D1FC - the run: step five units a frame along the facing while the stick is held. */
void lb_act_run(_PLW* self)
{
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        if (fn_80208AE8(self) == 1) {
            Pl_chr_setX(self, 39, 4, 0);
            self->field_0x354 = 1.1f;
        } else {
            if (Get_motion_no(self) == 42) {
                Pl_chr_setX(self, 3, 0, 38);
            } else {
                Pl_chr_setX(self, 3, 4, 0);
            }
            if (fn_80208AB8(self) == 0 && fn_80208AC0(self) == 0) {
                self->field_0x354 = 1.25f;
            }
        }
        self->field_0x266 = 10;
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                step.x = 0.0f;
                step.y = 0.0f;
                step.z = 5.0f;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.y += step.y;
                self->vec_0x03C.z += step.z;
                if (pl_part_flag_ck(self, 0) == 0 && self->field_0x266 <= 0) {
                    lb_npc_act_set(self, 1, 1, 0);
                }
            } else if (self->field_0x266 <= 0) {
                lb_npc_act_set(self, 1, 3, 0);
            }
        }
        break;
    }
    lb_player_walk_hook(self);
}

/* 0x8020D3E4 - The stop act: plays the stop motion and returns to idle when it ends (or the walk resumes). */

void lb_act_stop(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        pl_act_set_flag(self, 0xC0);
        self->field_0x0A8 = self->rot_0x54.y;
        Pl_chr_set_attr_default(self, 0x1E, 4, 0);
        if ((fn_80208AB8(self) != 0) || (fn_80208AC0(self) != 0)) {
            self->field_0x28 = 0xA;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 0xA, 0);
        } else if ((fn_80208AB8(self) != 0) || (fn_80208AC0(self) != 0)) {
            if (--self->field_0x28 <= 0xA) {
                fn_8020A70C(self, 0, 0xA, 0);
            }
        }
        break;
    }
    lb_player_walk_hook(self);
}

/* 0x8020D4FC - The turn act: plays the turn motion, then requests the next act. */

void lb_act_turn(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x266 = 0xA;
        fn_8020A5EC(self, 0);
        pl_act_set_flag(self, 0x40);
        self->rot_0x54.y = self->field_0x0A8;
        Pl_chr_set_attr_default(self, 0x2A, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_req(self, 1, 2, 0);
        }
        return;
    }
}

/* 0x8020D5A0 - Runs act family 1 (stand, walk, run, stop and turn) by the act number. */
void lb_act_family1(_PLW* self) {
    switch (self->act_no) {
    case 0:
        lb_act_stand(self);
        break;
    case 1:
        lb_act_walk(self);
        break;
    case 2:
        lb_act_run(self);
        break;
    case 3:
        lb_act_stop(self);
        break;
    case 4:
        lb_act_turn(self);
        return;
    }
}

/* 0x8020D5E8 - marks the handler entered and re-enters the act family the armed slot names. */
void lb_act_armed_reenter(_PLW* self)
{
    u8 slot;

    self->field_0x5C9 = 1;
    slot = self->field_0x5C8;
    if (slot != 0 && slot <= 21) {
        lb_npc_act_req(self, 11, lb_slot_act_tbl[slot], 0);
    }
}

/* 0x8020D624 - the slot of `lb_slot_act_tbl` whose act the player is running (act family 11 only), or 0. */
s32 lb_act_armed_slot_get(_PLW* self)
{
    u16* entry;
    s32 slot;

    if ((u8)self->field_0x00A == 11) {
        for (entry = &lb_slot_act_tbl[1], slot = 1; *entry != 0xFFFF; slot++, entry++) {
            switch (*entry) {
            case 6:
                if (self->act_no == 6 || self->act_no == 13) {
                    return slot;
                }
                break;
            case 17:
                if ((u32)(self->act_no - 17) <= 2) {
                    return slot;
                }
                break;
            case 12:
                if (self->act_no == 12 || self->act_no == 14) {
                    return slot;
                }
                break;
            default:
                if (self->act_no == *entry) {
                    return slot;
                }
                break;
            }
        }
    }
    return 0;
}

/* 0x8020D6E0 - Leaves the item act: returns to idle. */
void lb_act_item_leave(_PLW* self) {
    fn_8020A70C(self, 0, 4, 0);
    self->field_0x313 = 2;
}

/* 0x8020D720 - the item-use act `index`: play its motion, wait a second, then chain the next armed act. */
void lb_act_item_use(_PLW* self, s32 index)
{
    switch (self->act_step_0x05) {
    case 0: {
        LbMotionArgs* args;

        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        args = &lb_item_motion_tbl[index];
        Pl_chr_set_attr_default(self, args->motion_0x0, args->blend_0x2, args->start_0x4);
        self->field_0x28 = 60;
        break;
    }
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_master_ck(self) == 1 && self->field_0x28 <= 0) {
            if (pl_act_param_tier_ck(self, 0) != 0) {
                lb_npc_act_set(self, 11, 0, 0);
            } else {
                lb_act_armed_reenter(self);
            }
        }
        break;
    }
}

/* 0x8020D818 - the one-shot item act `index`: play its motion and leave. */
void lb_act_item_once(_PLW* self, s32 index)
{
    switch (self->act_step_0x05) {
    case 0: {
        LbMotionArgs* args;

        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        args = &lb_item_motion_once_tbl[index];
        Pl_chr_set_attr_default(self, args->motion_0x0, args->blend_0x2, args->start_0x4);
        break;
    }
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
            self->field_0x313 = 2;
        }
        break;
    }
}

/* 0x8020D8C8 - Item act 503: plays the item motion and re-enters the armed act on its frame. */

void lb_act_item_503(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x1F7, 4, 0);
        self->field_0x28 = 0x3C;
        break;
    case 1:
        if ((Pl_master_ck(self) == 1) && (Pl_frame_check(self, 1, 90.0f, 0.0f) == 1)) {
            if ((s32) pl_act_param_tier_ck(self, 0) != 0) {
                lb_npc_act_req(self, 0xB, 0xD, 0);
                return;
            }
            lb_act_armed_reenter(self);
        } else {
            return;
        }
        break;
    }
}

/* 0x8020D998 - Item act 504: plays the item motion, sets the model state, then returns to idle. */

void lb_act_item_504(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x1F8, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            pl_model_set_state(self, 2);
            Pl_chr_set_attr_default(self, 0xCF, -4, 0x3C);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
            self->field_0x313 = 2;
        }
        break;
    }
}

/* 0x8020DA6C - the item act 507: hold a second, then chain the next armed act. */
void lb_act_item_507(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 507, 4, 0);
        self->field_0x28 = 60;
        break;
    case 1:
        if (self->field_0x28 > 0) {
            self->field_0x28--;
        }
        if (Pl_master_ck(self) == 1 && self->field_0x28 <= 0) {
            if (pl_act_param_tier_ck(self, 0) != 0) {
                lb_npc_act_set(self, 11, 0, 0);
            } else {
                lb_act_armed_reenter(self);
            }
        }
        break;
    }
}

/* 0x8020DB44 - Item act 513: plays the item motion and re-enters the armed act on its frame. */

void lb_act_item_513(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x201, 4, 0);
        break;
    case 1:
        if ((Pl_master_ck(self) == 1) && (Pl_frame_check(self, 1, 150.0f, 0.0f) == 1)) {
            if ((s32) pl_act_param_tier_ck(self, 0) != 0) {
                lb_npc_act_req(self, 0xB, 0xE, 0);
                return;
            }
            lb_act_armed_reenter(self);
        } else {
            return;
        }
        break;
    }
}

/* 0x8020DC0C - Item act 514: plays the item motion, then returns to idle. */
void lb_act_item_514(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x202, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
            self->field_0x313 = 2;
        }
        return;
    }
}

/* 0x8020DC9C - Item act 515: plays the item motion and moves on to its hold (act 0xB/0x12). */

void lb_act_item_515(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x203, 4, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 1, 396.0f, 0.0f) == 1) {
            lb_npc_act_set(self, 0xB, 0x12, 2);
        }
        return;
    }
}

/* 0x8020DD30 - Item act 515's hold: loops motion 0x203 and re-enters the armed act on its frame. */

void lb_act_item_515_hold(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 0x203, 4, 0x18E);
        break;
    case 1:
        if ((Pl_master_ck(self) == 1) && (Pl_frame_check(self, 1, 396.0f, 0.0f) == 1)) {
            if ((s32) pl_act_param_tier_ck(self, 0) != 0) {
                lb_npc_act_req(self, 0xB, 0x13, 0);
                return;
            }
            lb_act_armed_reenter(self);
        } else {
            return;
        }
        break;
    }
}

/* 0x8020DDF8 - Item act 518: plays the item motion, then returns to idle. */
void lb_act_item_518(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_set_attr_default(self, 0x206, 4, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            self->act_step_0x05++;
            Pl_chr_set_attr_default(self, 0xD8, 4, 0x36);
            return;
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
            self->field_0x313 = 2;
        }
        break;
    }
}

/* 0x8020DEC0 - Runs act family 11 (the item acts) by the act number. */
void lb_act_family11(_PLW* self) {
    u16 temp_r0;

    temp_r0 = self->act_no;
    switch (temp_r0) {
    case 0:
        lb_act_item_leave(self);
        break;
    case 1:
        lb_act_item_use(self, 0);
        break;
    case 2:
        lb_act_item_use(self, 1);
        break;
    case 3:
        lb_act_item_use(self, 2);
        break;
    case 4:
        lb_act_item_use(self, 3);
        break;
    case 5:
        lb_act_item_once(self, 0);
        break;
    case 6:
        lb_act_item_503(self);
        break;
    case 13:
        lb_act_item_504(self);
        break;
    case 7:
        lb_act_item_use(self, 4);
        break;
    case 8:
        lb_act_item_507(self);
        break;
    case 9:
        lb_act_item_once(self, 1);
        break;
    case 10:
        lb_act_item_use(self, 5);
        break;
    case 11:
        lb_act_item_once(self, 2);
        break;
    case 12:
        lb_act_item_513(self);
        break;
    case 14:
        lb_act_item_514(self);
        break;
    case 15:
        lb_act_item_use(self, 6);
        break;
    case 16:
        lb_act_item_use(self, 7);
        break;
    case 17:
        lb_act_item_515(self);
        break;
    case 18:
        lb_act_item_515_hold(self);
        break;
    case 19:
        lb_act_item_518(self);
        break;
    case 20:
        lb_act_item_use(self, 8);
        return;
    default:
        return;
    }
}

/* 0x8020DF6C - the carried-item stand: hold while the stick is idle. */
void lb_act_carry_stand(_PLW* self)
{
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x266 = 10;
        Pl_chr_set_attr_default(self, 52, 4, 0);
        fn_8020A5EC(self, 0);
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
            } else if (self->field_0x266 == 0) {
                fn_8020A70C(self, 0, 6, 0);
                return;
            }
        }
        break;
    }
    lb_player_carry_hook(self);
}

/* 0x8020E03C - the carried-item walk: step one unit a frame along the facing while the stick is held. */
void lb_act_carry_walk(_PLW* self)
{
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 53, 4, 0);
        } else {
            Pl_chr_setX(self, 59, 4, 0);
        }
        self->field_0x266 = 10;
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                step.x = 0.0f;
                step.y = 0.0f;
                step.z = 1.0f;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.y += step.y;
                self->vec_0x03C.z += step.z;
            } else if (self->field_0x266 <= 0) {
                lb_npc_act_set(self, 12, 4, 0);
                return;
            }
        }
        break;
    }
    lb_player_carry_hook(self);
}

/* 0x8020E190 - the carried-item run: step five units a frame along the facing while the stick is held. */
void lb_act_carry_run(_PLW* self)
{
    VEC3 step;

    VEC3_ctor(&step);
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        if (self->se_name_set == 0) {
            Pl_chr_setX(self, 53, 4, 0);
        } else {
            Pl_chr_setX(self, 59, 4, 0);
        }
        if (fn_80208AB8(self) == 0 && fn_80208AC0(self) == 0) {
            self->field_0x354 = 1.25f;
        }
        self->field_0x266 = 10;
        break;
    case 1:
        if (Pl_master_ck(self) == 1) {
            if (pl_act_param_tier_ck(self, 0) >= 1) {
                pl_act_set_cam_ang(self);
                step.x = 0.0f;
                step.y = 0.0f;
                step.z = 5.0f;
                rotVecY(&step, self->rot_0x54.y);
                self->vec_0x03C.x += step.x;
                self->vec_0x03C.y += step.y;
                self->vec_0x03C.z += step.z;
                if (pl_part_flag_ck(self, 0) == 0 && self->field_0x266 <= 0) {
                    lb_npc_act_set(self, 12, 2, 0);
                }
            } else if (self->field_0x266 <= 0) {
                lb_npc_act_set(self, 12, 4, 0);
                return;
            }
        }
        break;
    }
    lb_player_carry_hook(self);
}

/* 0x8020E344 - The carry stop act: plays the stop motion and returns to the carry idle when it ends. */

void lb_act_carry_stop(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        pl_act_set_flag(self, 0xC0);
        self->field_0x0A8 = self->rot_0x54.y;
        Pl_chr_set_attr_default(self, 0x35, 4, 0);
        if ((fn_80208AB8(self) != 0) || (fn_80208AC0(self) != 0)) {
            self->field_0x28 = 0xA;
            return;
        }
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 0xA, 0);
            return;
        }
        if ((fn_80208AB8(self) != 0) || (fn_80208AC0(self) != 0)) {
            if (--self->field_0x28 <= 0xA) {
                fn_8020A70C(self, 0, 0xA, 0);
            }
        }
        break;
    }
}

/* 0x8020E454 - The carry turn act: plays the turn motion, then requests the next act. */

void lb_act_carry_turn(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        self->field_0x266 = 0xA;
        fn_8020A5EC(self, 0);
        pl_act_set_flag(self, 0x40);
        self->rot_0x54.y = self->field_0x0A8;
        Pl_chr_set_attr_default(self, 0x34, 0, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            lb_npc_act_req(self, 0xC, 3, 0);
        }
        return;
    }
}

/* 0x8020E4F8 - The carry put act: plays the put-down motion, drops the carried prop on its frame, then returns to idle. */

void lb_act_carry_put(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        Pl_chr_set_attr_default(self, 0x36, 4, 0);
        fn_8020A5EC(self, 0);
        break;
    case 1:
        if (Pl_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            self->act_step_0x05++;
            if (fn_80208AB8(self) == 1) {
                fn_80208A3C(self);
            } else if (fn_80208AC0(self) == 1) {
                fn_80208A48(self);
            }
        }
        break;
    case 2:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 8, 0);
            self->field_0x313 = 2;
        }
        break;
    }
}

/* 0x8020E5EC - Leaves the carry act once its motion ends. */
void lb_act_carry_leave(_PLW* self) {
    switch (self->act_step_0x05) {
    case 0:
        self->act_step_0x05++;
        fn_8020A70C(self, 0, 0x32, 0);
        break;
    case 1:
        if (Pl_motion_end_ck(self) == 1) {
            fn_8020A70C(self, 0, 4, 0);
        }
        return;
    }
}

/* 0x8020E668 - the carried-item idle: arm motion 51, then run the carry hook. */
void lb_act_carry_idle(_PLW* self)
{
    if (self->act_step_0x05 == 0) {
        self->act_step_0x05++;
        fn_8020A5EC(self, 0);
        Pl_chr_setX(self, 51, 4, 0);
    }
    lb_player_carry_hook(self);
}

/* 0x8020E6C8 - Runs act family 12 (the carry acts) by the act number, dropping the prop on a foreign act. */
void lb_act_family12(_PLW* self) {
    u16 temp_r0;

    temp_r0 = self->act_no;
    switch (temp_r0) {
    case 0:
        lb_act_carry_idle(self);
        break;
    case 1:
        lb_act_carry_stand(self);
        break;
    case 2:
        lb_act_carry_walk(self);
        break;
    case 3:
        lb_act_carry_run(self);
        break;
    case 4:
        lb_act_carry_stop(self);
        break;
    case 5:
        lb_act_carry_turn(self);
        break;
    case 6:
        lb_act_carry_put(self);
        break;
    case 7:
        lb_act_carry_leave(self);
        return;
    default:
        fn_80208A3C(self);
        fn_80208A48(self);
        fn_8020A70C(self, 0, 8, 0);
        return;
    }
}

/* 0x8020E778 - the heading from the player towards `target`. */
u32 lb_player_angle_to(_PLW* self, VEC3* target)
{
    return calcVecAng2(&self->vec_0x03C, target);
}

/* 0x8020E780 - walks the player to lobby spot `spot` and takes its angle and id. */
void lb_player_spot_walk(_PLW* self, LbSpot* spot)
{
    VEC3 pos;
    u32 angle;

    VEC3_ctor(&pos);
    lb_player_latch_release(self);
    lobby_w.active_0x008 = 0;
    vec_to_mh_vec3(&pos, (Vec*)&spot->pos_0x04);
    angle = spot->angle_0x10;
    copyVec3(&self->target_pos_0x090, &pos);
    lb_npc_walk_blend_set(self, 20);
    self->field_0x0A8 = angle;
    self->field_0x0B6 = spot->id_0x02;
    lb_npc_act_req(self, 0, 3, 0x20);
}

/* 0x8020E824 - walks the player to the gate spot (-500, 170, -605). */
void lb_player_gate_walk(_PLW* self)
{
    VEC3 pos;

    VEC3_ctor(&pos);
    self->field_0xB00 = 0;
    pos.x = -500.0f;
    pos.y = 170.0f;
    pos.z = -605.0f;
    copyVec3(&self->target_pos_0x090, &pos);
    lb_npc_walk_blend_set(self, 20);
    self->field_0x0A8 = 0x98E4;
    self->field_0x0B6 = 0;
    lb_npc_act_req(self, 0, 3, 0x20);
}

/* 0x8020E8B8 - walks the player to lobby spot `spot` and arms act 6. */
void lb_player_spot_walk_act6(_PLW* self, LbSpot* spot)
{
    VEC3 pos;
    VEC3 unused1;
    VEC3 unused2;
    u16 angle;

    VEC3_ctor(&pos);
    VEC3_ctor(&unused1);
    VEC3_ctor(&unused2);
    vec_to_mh_vec3(&pos, (Vec*)&spot->pos_0x04);
    angle = spot->angle_0x10;
    copyVec3(&self->target_pos_0x090, &pos);
    lb_npc_walk_blend_set(self, 20);
    self->field_0x0A8 = angle;
    lb_npc_act_set(self, 0, 6, 0);
}

/* 0x8020E948 - starts the talk with NPC `npc`: fire its talk hook, turn it to the player and open the talk. */
void lb_player_npc_talk_start(_PLW* self, _LB_NPC* npc)
{
    if (npc->field_0x225 == 1) {
        return;
    }
    lb_npc_func.talk_0x08((u16)npc->field_0x002, &npc->pos_0x10);
    lb_npc_talk_partner_set(self, npc);
    lb_npc_talk_open(self, npc);
    lb_player_latch_release(self);
    if (self->field_0x00A != 0 || self->act_no != 0) {
        fn_8020A70C(self, 0, -4, 0);
    }
}

/* 0x8020E9F4 - places the player at the area entry `place`, refreshes the place info (unless `quiet`) and sends
 * the move to the session. */
void lb_player_area_enter(_PLW* self, LbPlaceRec* place, u32 quiet)
{
    LobbyTimerBlock* state = &lobby_state_block;
    VEC3 pos;

    setVec3(&pos, place->x_0x24, place->y_0x28, place->z_0x2C);
    lobby_player_place(place->area_0x00, &pos, (u16)place->angle_0x30);
    lb_area_view_set(place->area_0x00);
    if ((u8)quiet == 0) {
        if (lb_city_area_ck(place->area_0x00) == 1) {
            place_info_set(lobby_world_block->place_0x483C);
            if (game_ready_ck() == 1) {
                pl_info_text_set(self, state->self_id_0x16B0);
            } else {
                pl_info_text_set(self, 0);
            }
        } else {
            place_info_set(0);
            pl_info_text_set(self, 0);
        }
        lobby_pig_dress_view_set(lobby_world_block->field_0x483B);
    }
    lobby_net_place_send(self, place->area_0x00, &pos, (u16)place->angle_0x30);
}

/* 0x8020EB00 - stores the player's lobby act latch byte. */
void lb_act_latch_set(_PLW* self, u8 value)
{
    self->field_0xB00 = value;
}

/* 0x8020EB08 - online, when another player is mid-trade with this player's hunter id, opens the trade notice. */
void lb_player_trade_notice(_PLW* self)
{
    _PLW* plw;
    s16 i;
    s16 count;

    if (Pl_master_ck(self) != 0 && game_ready_ck() != 0 && NetCtrlWk::isCityModeAlias() != 0) {
        plw = (_PLW*)get_move_work_adrs(2);
        count = (u16)get_move_work_max(2);
        for (i = 0; i < count; i++, plw++) {
            if (plw->slot_active != 0 && Pl_master_ck(plw) != 1 && Pl_act_ck(plw, 0, 1) != 0 &&
                strcmp(self->hunter_0x5CA.id_0x5DB, plw->partner_0xB0F) == 0) {
                cockpit_lobby_reset();
                return;
            }
        }
    }
}

} /* extern "C" */
} /* namespace lb_npc_player_acts */

/* ---------------------------------------------------------------------------------------------------
 * The local player's lobby control: item requests, the interact button, the remote players' act follower and the
 * arm-wrestle setup (0x8020EBF4..0x8020FE18).
 */
namespace nw4r { namespace math { u16 Atan2Idx(f32 y, f32 x); } }
using nw4r::math::Atan2Idx;
namespace lb_npc_player_ctrl {
extern "C" {
extern LbNpcUserData* lobby_world_block;
extern LbLobbyWork* lobby_wp;
u32 game_ready_ck(void);
u32 menu_busy_ck(void);
s32 lb_cmd_pressed_ck(u16 mask);
u32 fn_80208928(_PLW* plw);
u32 fn_80208AB8(_PLW* self);
u32 fn_80208AC0(_PLW* self);
void lb_player_latch_release(_PLW* self);
u32 lb_player_facing_away_ck(_PLW* self);
void fn_8020A70C(_PLW* self, u32 a, s32 b, s32 c);
void lb_npc_act_req(_PLW* self, u32 a, s32 b, u16 c);
void lb_npc_act_set(_PLW* self, u8 act, u16 sub, u32 flags);
u32 lb_npc_act_busy_ck(u32 kind, u32 act);
void lb_act_armed_reenter(_PLW* self);
void lb_player_spot_walk(_PLW* self, LbSpot* spot);
void lb_player_gate_walk(_PLW* self);
void lb_player_npc_talk_start(_PLW* self, _LB_NPC* npc);
void lb_player_area_enter(_PLW* self, LbPlaceRec* place, u32 quiet);
void lb_player_input_hold(_PLW* self);
void lb_act_family0(_PLW* self);
void lb_act_family1(_PLW* self);
void lb_act_family11(_PLW* self);
void lb_act_family12(_PLW* self);
u8 pl_act_param_tier_ck(_PLW* self, u32 idx);
u32 pl_part_flag_ck(_PLW* self, u32 part);
void pl_model_set_state(_PLW* self, u8 state);
u32 Pl_motion_end_ck(_PLW* self);
void Pl_chr_set_attr_default(_PLW* self, u16 motion, s32 a, s32 b);
u32 userdata_progress_flag_ck(s32 flag);
u32 lb_unlock_cond_ck(s32 cond);
f32 vec3_dist_sq(VEC3* a, VEC3* b);
f32 calcVecDistXZ(VEC3* a, VEC3* b);
int strcmp(const char* a, const char* b);
void* memset(void* dst, s32 c, u32 n);
u16 get_hunter_rank_max(LbNpcUserData* data);
void lb_quest_board_open_board(_PLW* self);
void lb_quest_board_open_ready(_PLW* self);
void lb_quest_board_open_notice(_PLW* self);
void em020_false_ck(_PLW* self);
void lobby_net_position_send(_PLW* self, u32 flag);
void lobby_net_item_request_send(_PLW* self, _PLW* owner);
u32 userdata_item_slot_count(LbNpcUserData* data);
_SLOTENT* userdata_equip_item_slots_get(LbNpcUserData* data);
s32 item_pair_index_find(u16 item, _SLOTENT* slots, u32 count);
void hud_item_msg_push(u32 kind, u32 a, u32 item);
void camera_talk_reset(void);
void lb_panel_close(void);

/* 0x8020EBF4 - online, requests the item the nearby player of this area offers in trade; 1 when it handled the
 * button. */
u32 lb_player_item_request(_PLW* self)
{
    s16 i;
    _PLW* plw;
    s16 count;
    f32 range;

    if (game_ready_ck() == 0) {
        return 0;
    }
    if (NetCtrlWk::isCityModeAlias() == 0) {
        return 0;
    }
    if (self->field_0x658 != 0) {
        return 0;
    }
    plw = (_PLW*)get_move_work_adrs(2);
    count = (u16)get_move_work_max(2);
    i = 0;
    range = 90000.0f;
    for (; i < count; i++, plw++) {
        if (plw->slot_active != 0 && Pl_master_ck(plw) != 1 && plw->area_0x16 == self->area_0x16 &&
            fn_80208928(plw) != 0 && Pl_act_ck(plw, 0, 1) != 0 && !(vec3_dist_sq(&self->vec_0x03C, &plw->vec_0x03C) > range) &&
            GetItemData(plw->field_0x650)->level_0x01 < 3 &&
            strcmp(self->hunter_0x5CA.id_0x5DB, plw->partner_0xB0F) == 0) {
            self->field_0x658 = 90;
            if (userdata_item_total_count(lobby_world_block, plw->field_0x650) < plw->field_0x652) {
                u32 slots = userdata_item_slot_count(lobby_world_block);

                if (item_pair_index_find(plw->field_0x650, userdata_equip_item_slots_get(lobby_world_block), slots) < 0) {
                    hud_item_msg_push(1, 0, 0);
                } else {
                    hud_item_msg_push(2, 30, plw->field_0x650);
                }
                return 1;
            }
            self->field_0x650 = plw->field_0x650;
            self->field_0x652 = plw->field_0x652;
            lb_npc_act_req(self, 0, 45, 0);
            if (Pl_master_ck(self) == 1) {
                lobby_net_item_request_send(self, plw);
            }
            return 1;
        }
    }
    return 0;
}

/* 0x8020EE14 - the local player's lobby control frame: the stick moves (walk/run/turn by tier) and the interact
 * button (talk to an NPC, use a spot or a place entry), then the position goes to the session. */
void lb_player_control(_PLW* self)
{
    if (game_ready_ck() == 0 && userdata_progress_flag_ck(99) == 0) {
        if (self->field_0x036 >= 60) {
            _LB_NPC* npc = lb_target_npc_get();

            if (npc != 0) {
                lb_player_npc_talk_start(self, npc);
            }
        }
    } else {
        if (game_ready_ck() == 1) {
            if (self->field_0x654 != 0) {
                lb_npc_act_set(self, 0, 1, 0x10);
                return;
            }
            if (lobby_w.state_0x000 == 0 && menu_busy_ck() == 0 && lb_cmd_pressed_ck(0x10) != 0 &&
                lb_player_item_request(self) == 1) {
                return;
            }
        }
        if (pl_act_param_tier_ck(self, 0) >= 2) {
            if (self->field_0x00A != 1) {
                if (pl_part_flag_ck(self, 0) != 0) {
                    if (lb_player_facing_away_ck(self) != 0) {
                        lb_npc_act_set(self, 1, 4, 0);
                    } else {
                        lb_npc_act_set(self, 1, 2, 0);
                    }
                } else if (lb_player_facing_away_ck(self) != 0) {
                    lb_npc_act_set(self, 1, 4, 0);
                } else {
                    lb_npc_act_set(self, 1, 1, 0);
                }
            } else {
                switch (self->act_no) {
                case 0:
                    if (pl_part_flag_ck(self, 0) != 0) {
                        if (lb_player_facing_away_ck(self) != 0) {
                            lb_npc_act_set(self, 1, 4, 0);
                        } else {
                            lb_npc_act_set(self, 1, 2, 0);
                        }
                    } else {
                        lb_npc_act_set(self, 1, 1, 0);
                    }
                    break;
                case 1:
                    if (pl_part_flag_ck(self, 0) != 0) {
                        if (lb_player_facing_away_ck(self) != 0) {
                            lb_npc_act_set(self, 1, 4, 0);
                        } else {
                            lb_npc_act_set(self, 1, 2, 0);
                        }
                    }
                    break;
                }
            }
        } else if (pl_act_param_tier_ck(self, 0) >= 1 && self->field_0x00A != 1) {
            if (pl_part_flag_ck(self, 0) != 0) {
                if (lb_player_facing_away_ck(self) != 0) {
                    lb_npc_act_set(self, 1, 4, 0);
                } else {
                    lb_npc_act_set(self, 1, 2, 0);
                }
            } else {
                lb_npc_act_set(self, 1, 0, 0);
            }
        }
        if ((lobby_w.state_0x000 == 0 || lobby_w.state_0x000 == 10) && menu_busy_ck() == 0 &&
            lb_cmd_pressed_ck(0x10) != 0) {
            _LB_NPC* npc = lb_target_npc_get();
            struct LbPlSearch* search = lb_plsearch_target_get();
            LbSpot* spot = lb_spot_cur_get();
            LbPlaceRec* place = lb_place_rec_get();
            u8 family = self->field_0x00A;
            u16 act = self->act_no;

            if (npc != 0) {
                if (family != 0 || act != 0) {
                    lb_npc_act_req(self, 0, 0, (u16)0x8000);
                }
                lb_player_npc_talk_start(self, npc);
            } else if (search != 0) {
                lb_plsearch_open(search);
                lb_act_armed_reenter(self);
            } else if (spot != 0) {
                s32 idle = 1;
                s32 used = 1;

                switch (spot->kind_0x00) {
                case 11:
                case 15:
                    lb_player_latch_release(self);
                    lb_city_select_open(self, spot->kind_0x00);
                    break;
                case 10:
                    lb_server_select_open(self);
                    break;
                case 1:
                    if (self->field_0xB03 == 0) {
                        lb_player_latch_release(self);
                        lb_talk_screen_open(self, 0);
                        break;
                    }
                    idle = 0;
                    used = 0;
                    break;
                case 9:
                    lb_player_latch_release(self);
                    lb_talk_screen_open(self, 1);
                    break;
                case 6:
                    if (self->field_0xB03 == 0) {
                        lb_player_latch_release(self);
                        lb_spot_screen_open(self, spot);
                        break;
                    }
                    idle = 0;
                    used = 0;
                    break;
                case 12:
                    lb_pop_screen_open(self);
                    break;
                case 7:
                    lb_player_latch_release(self);
                    if (get_hunter_rank_max(lobby_world_block) == 0) {
                        lb_npc_act_set(self, 0, 46, 0);
                    } else {
                        lb_quest_board_open_board(self);
                    }
                    break;
                case 3:
                    lb_player_latch_release(self);
                    Pl_chr_set_attr_default(self, 1, -4, 0);
                    lb_quest_board_open_ready(self);
                    break;
                case 4:
                    if (lobby_w.page_0x052 == 6) {
                        lb_quest_board_open_notice(self);
                        break;
                    }
                    idle = 0;
                    used = 0;
                    break;
                case 5:
                    if (game_ready_ck() == 1) {
                        self->field_0x0B6 = spot->id_0x02;
                        lobby_w.invite_0x12E = 0;
                        if (NetCtrlWk::checkOwnInvite(spot->id_0x02 + 1, &lobby_w.invite_0x12E) == 1) {
                            pl_model_set_state(self, 1);
                            lb_npc_act_set(self, 0, 41, 0x20);
                        } else {
                            lb_npc_act_set(self, 0, 42, 0x20);
                        }
                    } else {
                        lb_player_spot_walk(self, spot);
                    }
                    break;
                case 8:
                    lb_player_latch_release(self);
                    self->field_0x0B6 = spot->id_0x02;
                    lobby_w.invite_0x130 = 0;
                    if (NetCtrlWk::checkOwnInvite(spot->id_0x02 + 13, &lobby_w.invite_0x130) == 1) {
                        lb_npc_act_set(self, 0, 43, 0x20);
                    } else {
                        lb_npc_act_set(self, 0, 42, 0x20);
                    }
                    break;
                case 13:
                    if (lb_unlock_cond_ck(22) == 1 && userdata_progress_flag_ck(88) == 0) {
                        lb_npc_act_set(self, 0, 39, 0);
                    } else {
                        lb_npc_act_set(self, 0, 40, 0);
                    }
                    break;
                case 14:
                    lb_city_screen_open(self);
                    break;
                default:
                    used = 0;
                    break;
                }
                if (used == 0) {
                    if (idle != 0 && (family != 0 || act != 0)) {
                        lb_npc_act_set(self, 0, 0, (u16)0x8000);
                    } else {
                        lb_act_armed_reenter(self);
                    }
                }
            } else if (place != 0) {
                if (self->kind_0x015 == 21 && place->area_0x00 == 0) {
                    em020_false_ck(self);
                } else {
                    lb_player_area_enter(self, place, 0);
                }
            } else if (fn_80208AC0(self) == 1) {
                self->flag_0x30E = 0;
                lb_npc_act_set(self, 12, 6, 0);
            } else {
                lb_act_armed_reenter(self);
            }
        } else {
            lb_act_armed_reenter(self);
        }
        lobby_net_position_send(self, 0);
    }
}

/* 0x8020F544 - walks a remote player's act along his move target: start, keep or stop the walk/run by the
 * distance left. */
void lb_player_remote_follow(_PLW* self)
{
    VEC3 d;
    f32 dist;

    VEC3_ctor(&d);
    if (self->kind_0x09 == 1 || lb_npc_act_busy_ck(self->field_0x00A, self->act_no) == 1) {
        return;
    }
    self->target_pos_0x090.y = self->vec_0x03C.y;
    dist = calcVecDistXZ(&self->vec_0x03C, &self->target_pos_0x090);
    d.x = self->target_pos_0x090.x - self->vec_0x03C.x;
    d.z = self->target_pos_0x090.z - self->vec_0x03C.z;
    switch (self->field_0x00A) {
    case 1:
        switch (self->act_no) {
        case 1:
        case 2:
            if (self->blend_frames_0x0B4 == lobby_state_block.move_frames_0x168C - 1 && dist < 150.0f) {
                lb_npc_act_set(self, 1, 0, 0);
            } else if (dist < 6.0f) {
                self->blend_frames_0x0B4 = 0;
                copyVec3(&self->vec_0x03C, &self->target_pos_0x090);
                lb_npc_act_set(self, 1, 3, 0);
            } else {
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
            }
            break;
        case 0:
            if (dist < 6.0f) {
                self->blend_frames_0x0B4 = 0;
                copyVec3(&self->vec_0x03C, &self->target_pos_0x090);
                fn_8020A70C(self, 0, 4, 0);
            } else if (dist > 150.0f) {
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
                lb_npc_act_set(self, 1, 2, 0);
            } else {
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
            }
            break;
        case 3:
            if (dist > 150.0f) {
                lb_npc_act_set(self, 1, 2, 0);
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
            } else if (dist > 20.0f) {
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
                lb_npc_act_set(self, 1, 0, 0);
            }
            break;
        }
        break;
    case 0:
        switch (self->act_no) {
        case 1:
        case 2:
        case 6:
        case 7:
        case 12:
        case 25:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
            break;
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
            return;
        default:
            if (dist > 150.0f) {
                lb_npc_act_set(self, 1, 2, 0);
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
            } else if (dist > 20.0f) {
                self->field_0x0A8 = Atan2Idx(d.x, d.z);
                lb_npc_act_set(self, 1, 0, 0);
            }
            break;
        }
        break;
    }
    pl_model_set_state(self, 2);
}

void lb_player_carry_control(_PLW* self);

/* 0x8020F7EC - the walk acts' control hook: offline it takes the quest-board latch requests, then the local
 * player's control frame runs. */
void lb_player_walk_hook(_PLW* self)
{
    if (game_ready_ck() == 0) {
        if (self->field_0xB00 == 4) {
            lb_player_gate_walk(self);
            return;
        }
        if (self->field_0xB00 == 5) {
            self->flag_0x30E = 0;
            lb_npc_act_set(self, 12, 6, 0);
            return;
        }
    }
    if (Pl_master_ck(self) != 0) {
        self->flag_0x30E = 1;
        lb_player_control(self);
    }
}

/* 0x8020F880 - the carried-item control frame: the stick moves and the interact button. */
void lb_player_carry_control(_PLW* self)
{
    if (pl_act_param_tier_ck(self, 0) >= 2) {
        switch (self->act_no) {
        case 0:
            if (pl_part_flag_ck(self, 0) != 0) {
                if (lb_player_facing_away_ck(self) != 0) {
                    lb_npc_act_set(self, 12, 5, 0);
                } else {
                    lb_npc_act_set(self, 12, 3, 0);
                }
            } else if (lb_player_facing_away_ck(self) != 0) {
                lb_npc_act_set(self, 12, 5, 0);
            } else {
                lb_npc_act_set(self, 12, 2, 0);
            }
            break;
        case 1:
            if (pl_part_flag_ck(self, 0) != 0) {
                if (lb_player_facing_away_ck(self) != 0) {
                    lb_npc_act_set(self, 12, 5, 0);
                } else {
                    lb_npc_act_set(self, 12, 3, 0);
                }
            } else {
                lb_npc_act_set(self, 12, 2, 0);
            }
            break;
        case 2:
            if (pl_part_flag_ck(self, 0) != 0) {
                if (lb_player_facing_away_ck(self) != 0) {
                    lb_npc_act_set(self, 12, 5, 0);
                } else {
                    lb_npc_act_set(self, 12, 3, 0);
                }
            }
            break;
        }
    } else if (pl_act_param_tier_ck(self, 0) >= 1 && self->act_no == 0) {
        if (pl_part_flag_ck(self, 0) != 0) {
            if (lb_player_facing_away_ck(self) != 0) {
                lb_npc_act_set(self, 12, 5, 0);
            } else {
                lb_npc_act_set(self, 12, 3, 0);
            }
        } else {
            lb_npc_act_set(self, 12, 1, 0);
        }
    }
    if ((lobby_w.state_0x000 == 0 || lobby_w.state_0x000 == 10) && menu_busy_ck() == 0 && lb_cmd_pressed_ck(0x10) != 0) {
        _LB_NPC* npc = lb_target_npc_get();
        LbSpot* spot = lb_spot_cur_get();
        LbPlaceRec* place = lb_place_rec_get();

        if (npc != 0) {
            lb_player_npc_talk_start(self, npc);
        } else if (spot == 0) {
            if (place != 0) {
                lb_player_area_enter(self, place, 0);
            } else if (fn_80208AC0(self) == 1) {
                self->flag_0x30E = 0;
                lb_npc_act_set(self, 12, 6, 0);
            }
        }
    }
}

/* 0x8020FB7C - the carried-item acts' control hook. */
void lb_player_carry_hook(_PLW* self)
{
    if (Pl_master_ck(self) != 0) {
        self->flag_0x30E = 1;
        lb_player_carry_control(self);
    }
}

/* 0x8020FBC0 - runs the player's current act by family, then walks a remote player along his target. */
void lb_player_act_run(_PLW* self)
{
    self->act_state_0x00E[0] = 0;
    lb_player_input_hold(self);
    switch (self->field_0x00A) {
    case 0:
        lb_act_family0(self);
        break;
    case 1:
        lb_act_family1(self);
        break;
    case 11:
        lb_act_family11(self);
        break;
    case 12:
        lb_act_family12(self);
        break;
    }
    if (Pl_master_ck(self) == 0) {
        lb_player_remote_follow(self);
    }
    self->field_0x5C8 = 0;
}

/* 0x8020FC68 - true while the player is in act family 0's arm-wrestle acts (19..24). */
u32 lb_player_arm_wrestle_ck(_PLW* self)
{
    if (self->field_0x00A != 0) {
        return 0;
    }
    return (u32)(self->act_no - 19) <= 5;
}

/* 0x8020FCA0 - true when `other` can no longer arm-wrestle `self` (gone, hidden, in another area or out of the
 * acts). */
u32 lb_arm_wrestle_lost_ck(_PLW* self, _PLW* other)
{
    if (other->slot_active == 0 || other->field_0x001 == 0) {
        return 1;
    }
    if (self->area_0x16 != other->area_0x16) {
        return 1;
    }
    return lb_player_arm_wrestle_ck(other) == 0;
}

/* 0x8020FD04 - opens the arm-wrestle screen for `self` (screen 25) with a cleared work block. */
void lb_arm_wrestle_open(_PLW* self)
{
    LbArmWrestleWork* work = lobby_w.arm_wrestle_0x0AC;

    memset(work, 0, 0x2000);
    work->self_0x10 = self;
    work->opponent_0x14 = 0;
    lobby_w.state_0x000 = 25;
    lobby_w.active_0x008 = 1;
    lobby_wp->field_0x145 = 0;
    lobby_wp->field_0x146 = 0;
    lobby_wp->field_0x147 = 0;
    lobby_wp->field_0x148 = 0;
}

/* 0x8020FD9C - closes the arm-wrestle screen. */
void lb_arm_wrestle_close(void)
{
    camera_talk_reset();
    lb_panel_close();
}

/* 0x8020FDC0 / 0x8020FDEC - raise the arm-wrestle "go" and "give up" flags while the screen is open. */
void lb_npc_net_flag_on(void)
{
    if (lobby_w.state_0x000 == 25 && lobby_w.field_0x146 == 0) {
        lobby_w.field_0x146 = 1;
    }
}

void lb_npc_net_flag_off(void)
{
    if (lobby_w.state_0x000 == 25 && lobby_w.field_0x147 == 0) {
        lobby_w.field_0x147 = 1;
    }
}

} /* extern "C" */
} /* namespace lb_npc_player_ctrl */


/* ---------------------------------------------------------------------------------------------------
 * The arm-wrestle screen (screen 25): find the opponent, count down, push, and settle (0x8020FE18..0x802104D8).
 */
/* A sprite-data record copied out of the layout table so its scale can be set (`hud/layout_types.h`'s
 * `_SPR_DATA_`, which cannot be included beside `unsplit/lobby.h`'s copy of `_mh_ivec2_`).  size: 0x24 */
typedef struct LbSprCopy {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ s16 u_scale_0x0C;
    /* +0x0E */ s16 v_scale_0x0E;
    /* +0x10 */ u8 pad_0x10[0x14];
} LbSprCopy;
void subTransSet(u32 draw, s32 a, u32* b);
namespace lb_npc_arm_wrestle {
extern "C" {
s32 lb_cmd_pressed_ck(u16 mask);
u32 lb_arm_wrestle_lost_ck(_PLW* self, _PLW* other);
void lb_arm_wrestle_close(void);
void lb_npc_act_set(_PLW* self, u8 act, u16 sub, u32 flags);
void lobby_net_flag_send(u32 flag);
s32 checkOtherInvite(u8 id, s8* result);
void lb_arm_wrestle_draw(void);

/* 0x8020FE18 - the arm-wrestle screen's frame. */
void lb_arm_wrestle_step(void)
{
    LbArmWrestleWork* work = lobby_w.arm_wrestle_0x0AC;

    switch (work->step_0x00) {
    case 0:
        work->step_0x00++;
        break;
    case 1: {
        _PLW* plw = (_PLW*)get_move_work_adrs(2);
        s16 i;
        u16 count = (u16)get_move_work_max(2);

        for (i = 0; i < count; i++, plw++) {
            if (lb_arm_wrestle_lost_ck(work->self_0x10, plw) != 1 && Pl_master_ck(plw) != 1 &&
                (Pl_act_ck(plw, 0, 19) == 1 || Pl_act_ck(plw, 0, 20) == 1) && Get_motion_no(plw) == 630 &&
                Pl_frame_check(plw, 1, 200.0f, 0.0f) == 1) {
                work->step_0x00++;
                work->opponent_0x14 = plw;
                work->wait_0x04 = 300;
                lobby_net_flag_send(0);
                return;
            }
        }
        if (Get_motion_no(work->self_0x10) == 630 && Pl_frame_check(work->self_0x10, 1, 200.0f, 0.0f) == 1 &&
            lb_cmd_pressed_ck(0x20) == 1) {
            lobby_w.field_0x145 = 1;
            lb_arm_wrestle_close();
        }
        break;
    }
    case 2:
        if (work->wait_0x04 > 0) {
            work->wait_0x04--;
        }
        if (lb_arm_wrestle_lost_ck(work->self_0x10, work->opponent_0x14) == 1 || work->wait_0x04 <= 0) {
            lobby_w.field_0x145 = 1;
            lb_arm_wrestle_close();
        } else if (lobby_w.field_0x146 != 0) {
            camera_lobby_mode_req(work->step_0x00++);
            work->count_0x06 = 89;
            sysSE_bank32_req(30);
        }
        break;
    case 3:
        if (lb_arm_wrestle_lost_ck(work->self_0x10, work->opponent_0x14) == 1) {
            lobby_w.field_0x145 = 1;
            lb_arm_wrestle_close();
            return;
        }
        if (--work->count_0x06 <= 0) {
            work->step_0x00++;
            work->power_0x0C = 50;
            work->tick_0x08 = 10;
            if ((u8)work->self_0x10->field_0x0B6 == 0) {
                lb_npc_act_set(work->self_0x10, 0, 21, 0x20);
                lb_npc_act_set(work->opponent_0x14, 0, 22, 0x22);
            } else {
                lb_npc_act_set(work->self_0x10, 0, 22, 0x20);
                lb_npc_act_set(work->opponent_0x14, 0, 21, 0x22);
            }
        } else if (work->count_0x06 % 30 == 0) {
            switch (work->count_0x06 / 30) {
            case 1:
                sysSE_bank32_req(32);
                break;
            case 2:
                sysSE_bank32_req(31);
                break;
            }
        }
        break;
    case 4:
        if (lb_arm_wrestle_lost_ck(work->self_0x10, work->opponent_0x14) == 1) {
            lobby_w.field_0x145 = 1;
            lb_arm_wrestle_close();
            return;
        }
        if (--work->tick_0x08 <= 0) {
            work->tick_0x08 = 10;
            work->power_0x0C--;
        }
        if (work->cooldown_0x0A <= 0) {
            if (pl_input_active_ck(work->self_0x10) == 1) {
                work->cooldown_0x0A = 3;
                work->power_0x0C--;
            }
        } else {
            work->cooldown_0x0A--;
        }
        if (work->power_0x0C <= 0) {
            work->step_0x00++;
            lobby_w.invite_0x132 = 0;
            if (NetCtrlWk::checkOwnInvite(15, &lobby_w.invite_0x132) == 1) {
                work->step_0x00 = 5;
            } else {
                work->step_0x00 = 7;
                work->tick_0x08 = 90;
                lb_npc_act_set(work->self_0x10, 0, 23, 0x20);
                lb_npc_act_set(work->opponent_0x14, 0, 24, 0x22);
            }
        } else if (lobby_w.field_0x147 != 0) {
            work->step_0x00 = 7;
            work->tick_0x08 = 90;
            lb_npc_act_set(work->self_0x10, 0, 23, 0x20);
            lb_npc_act_set(work->opponent_0x14, 0, 24, 0x22);
        }
        break;
    case 5:
        if (lobby_w.invite_0x132 == 1) {
            work->step_0x00 = 6;
            work->tick_0x08 = 90;
            lb_npc_act_set(work->self_0x10, 0, 24, 0x20);
            lb_npc_act_set(work->opponent_0x14, 0, 23, 0x22);
            lobby_net_flag_send(1);
        } else if (lobby_w.invite_0x132 < 0) {
            work->step_0x00 = 7;
            work->tick_0x08 = 90;
            lb_npc_act_set(work->self_0x10, 0, 23, 0x20);
            lb_npc_act_set(work->opponent_0x14, 0, 24, 0x22);
        }
        break;
    case 6:
        if (--work->tick_0x08 <= 0) {
            work->step_0x00 = 8;
            checkOtherInvite(15, &lobby_w.invite_0x133);
        }
        break;
    case 7:
        if (--work->tick_0x08 <= 0) {
            work->step_0x00 = 8;
        }
        break;
    case 8:
        lb_arm_wrestle_close();
        break;
    }
    if (work->step_0x00 == 3) {
        subTransSet((u32)lb_arm_wrestle_draw, 0, 0);
    }
}

/* 0x802103AC - draws the arm-wrestle countdown digit, pulsing over each second. */
void lb_arm_wrestle_draw(void)
{
    LbArmWrestleWork* work = lobby_w.arm_wrestle_0x0AC;

    if (work->step_0x00 == 3) {
        _mh_ivec2_ pos;
        LbSprCopy spr;
        s16 digit;
        u16 id;
        f32 scale;

        get_lsp_data(10401, &pos);
        digit = work->count_0x06 / 30 + 1;
        if (digit >= 3) {
            id = 10402;
        } else {
            id = 10404 - (digit == 2);
        }
        spr_data_copy((s16*)&spr, get_lsp_data(id, 0));
        scale = 0.9f + 0.1f * (work->count_0x06 % 30);
        spr.u_scale_0x0C = spr.v_scale_0x0E = 100.0f * scale;
        draw_sprite(*(struct _SPR_DATA_*)&spr, &pos);
    }
}

} /* extern "C" */
} /* namespace lb_npc_arm_wrestle */

/* ---------------------------------------------------------------------------------------------------
 * The NPC shop screen (screen 2), the lobby screen dispatcher and the pad command masks (0x802104D8..0x80212540).
 */
/* The head of the sprite-data record `get_lsp_data` returns (`hud/layout_types.h`'s `_SPR_DATA_`, whose header
 * cannot be included beside `unsplit/lobby.h`'s copy of `_mh_ivec2_`). */
typedef struct LbSprHead {
    /* +0x00 */ _mh_ivec2_ pos;
} LbSprHead; /* size: 0x4 (approximation: the head of the 0x24-byte record) */
void draw_sprite_ary(const u16* ids, const _mh_ivec2_* pos);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
void PutPageArrow(u16* ids, s16 page, s16 pages, u16 blink, const _mh_ivec2_* pos, u8 flags);
u32 ck_WideMode(void);
void set_zmode(bool test, u8 func, bool update);
void set_blendmode(u8 mode, u8 src, u8 dst);
void sysSE_req(s32 id);
void subTransSet(u32 draw, s32 a, u32* b);
s32 chk_pointer(void);
namespace lb_npc_shop {
extern "C" {
extern LbNpcUserData* lobby_world_block;
struct LbList;
void lb_list_init(struct LbList* list, s16 count, u8 kind, s16 str_base, s16 help_str, u16 off_mask, s32 se_ok,
                  u32 flags, u16 blink_mask);
s32 lb_choice_step(struct LbList* list);
void lb_choice_draw(struct LbList* list, u16 panel, const u16* sprites, const u16* rows, u16 help_panel, u8 flag);
s32 lb_panel_msg_draw(s32 panel, s16 str);
void lb_panel_str_print(u16 panel, s16 str, s16 color);
void lb_panel_line_draw(u16 panel, s16 str, s16 color, s32 row);
void lb_panel_yes_no_draw(u16 panel, s32 sel);
s32 lb_help_num_put(s32 panel, s32 str, s32 value);
void lb_count_window_draw(s32 panel, s32 sub_panel, s32 value, s32 max, s32 price, s32 flag, s32 moved);
void lb_page_arrow_draw(u16 panel, s16 page, s16 pages, u16 x, s32 active);
s32 lb_panel_close(void);
void lb_talk_page_open(s8 page);
s32 lb_talk_page_mode_reset(void);
u32 game_ready_ck(void);
u32 menu_busy_ck(void);
s32 lb_cmd_pressed_ck(u16 mask);
s32 lb_cmd_repeat_ck(u16 mask);
u16 lb_cmd_repeat_get(void);
u16 lobby_cmd_trig_get(void);
u32 lobby_input_locked_ck(void);
u32 kujira_event_over_ck(void);
u32 lb_unlock_cond_ck(s32 cond);
u32 userdata_item_slot_count(LbNpcUserData* data);
_SLOTENT* userdata_equip_item_slots_get(LbNpcUserData* data);
s32 item_pair_index_find(u16 item, _SLOTENT* slots, u32 count);
s32 item_count_find(u16 item, _SLOTENT* slots, u32 count);
u32 userdata_box_item_ck(LbNpcUserData* data, u16 item);
u16 userdata_box_capacity(LbNpcUserData* data);
u16 item_slots_free_count(_SLOTENT* slots, u16 count);
s32 eft052_page_count_ck(u16 item, u8 kind);
void eft052_page_take(_PLW* plw, u16 item, s16 count, u32 flag);
s32 eft052_item_value_get(u16 item);
void item_box_store(u16 item, s16 count, u16* slot);
s32 userdata_item_give(LbNpcUserData* data, u16 item, s16 count, u32 flag);
void score_add_clamped(s32 value, s32* score);
void lb_item_cell_draw_layout1(u16 item, s32 price, _mh_ivec2_* pos, s32 enabled, s32 picked, s32 active, u16 flag);
void lb_item_detail_panel_draw(u16 panel, u16 item, u32 flag);
void menu_item_row_draw_by_lsp(u16 panel, u16* item);
void menu_money_draw(u16 panel);
void lb_page_walk_head(_mh_ivec2_* pos, s16 slot);
void ainpc_page_hold_clear(void);
void ainpc_page_hold_set(void);
void camera_talk_lock_set(u8 value);
s32 hud_msg_push(u32 ch, const char* text);
_PLW* my_player_work_get(void);
extern u16 lb_shop_banner_wide[];
extern u16 lb_shop_banner[];
extern u16 lb_shop_frame[];
extern u16 lb_shop_buy_frame[];
extern u16 lb_shop_buy_cells[];
extern u16 lb_shop_sell_frame[];
extern u16 lb_shop_sell_cells[];
extern u16 lb_shop_pouch_frame[];
extern u16 lb_shop_pouch_cells[];
extern u16 lb_shop_choice_sprites[];
extern u16 lb_shop_choice_rows[4];

void lb_shop_draw(void);
s32 lb_shop_price_get(u16 item, u8* discount);

/* 0x802104D8 - opens NPC `npc`'s shop for `plw` (screen 2) with a cleared work block. */
void lb_shop_open(_PLW* plw, _LB_NPC* npc)
{
    LbShopWork* work = lobby_w.shop_0x0AC;

    memset(work, 0, 0x2000);
    work->plw_0x03C = plw;
    work->npc_0x040 = npc;
    lobby_w.state_0x000 = 2;
    lobby_w.active_0x008 = 1;
    lb_talk_page_open(0);
}

/* 0x80210548 - closes the shop. */
void lb_shop_close(void)
{
    lb_panel_close();
}

/* 0x8021054C - the stock list of the shop NPC `npc`, or null. */
u16* lb_shop_stock_get(u32 npc)
{
    switch ((u16)npc) {
    case 5:
        return lb_shop_items_npc5;
    case 39:
        return lb_shop_items_npc39;
    case 41:
        return lb_shop_items_npc41;
    case 50:
        return lb_shop_items_npc50;
    default:
        return 0;
    }
}

u16 lb_shop_buy_flags_get(u32 item, s16 count);

/* 0x802105AC - rebuilds the buy list: the stock unlocked so far (plus the online extras), then the eight rows of
 * page `page` with their prices and states. */
void lb_shop_buy_list_build(LbShopWork* self, s16 page)
{
    s32 first = page * 8;
    s32 count;
    s32 i;
    u16* list;

    self->rows_0x012 = 0;
    self->page_0x008 = page;
    memset(self->items_0x044, 0, 0x100);
    memset(self->rows_0x144, 0, 0x60);
    list = lb_shop_stock_get(self->npc_0x040->field_0x002);
    if (list != 0) {
        count = 0;
        for (; list[0] != 0; list += 2) {
            if (lb_unlock_cond_ck(list[1]) == 1) {
                self->items_0x044[count] = list[0];
                count++;
                if (count >= 0x80) {
                    break;
                }
            }
        }
    }
    if (game_ready_ck() == 1) {
        list = 0;
        switch (self->npc_0x040->field_0x002) {
        case 39:
            if (lobby_w.field_0x160 != 0) {
                list = lb_shop_items_npc39_net;
            }
            break;
        case 41:
            if (lobby_w.field_0x15D != 0) {
                list = lb_shop_items_npc41_net;
            }
            break;
        }
        if (list != 0) {
            for (; list[0] != 0; list += 2) {
                if (lb_unlock_cond_ck(list[1]) == 1) {
                    self->items_0x044[count] = list[0];
                    count++;
                    if (count >= 0x80) {
                        break;
                    }
                }
            }
        }
    }
    for (i = 0; i < 8; i++) {
        if (first + i < count) {
            u16 item = self->items_0x044[first + i];

            self->rows_0x144[i].item_0x0 = item;
            self->rows_0x144[i].price_0x4 = lb_shop_price_get(item, &self->rows_0x144[i].discount_0x2);
            self->rows_0x144[i].flags_0x8 = lb_shop_buy_flags_get(item, 1);
            self->rows_0x012++;
        }
    }
    if (self->cursor_0x006 >= self->rows_0x012) {
        self->cursor_0x006 = self->rows_0x012 - 1;
    }
    self->pages_0x00A = menu_page_count(count, 8);
}

/* 0x802107B8 - true unless `item` is on the half-price event's skip list. */
u32 lb_shop_halfprice_ck(u32 item)
{
    u16* entry;

    for (entry = lb_shop_halfprice_skip; *entry != 0; entry += 2) {
        if ((u16)item == *entry) {
            return 0;
        }
    }
    return 1;
}

/* 0x802107F0 - the shop price of `item` (halved during the half-price event, marking `discount`), at least 1. */
s32 lb_shop_price_get(u16 item, u8* discount)
{
    s32 price = GetItemData(item)->field_0x00C;

    if (game_ready_ck() == 0) {
        if (kujira_event_over_ck() == 1 && lb_shop_halfprice_ck(item) == 1) {
            price >>= 1;
            if (discount != 0) {
                *discount = 1;
            }
        }
    } else if (lobby_w.field_0x15C != 0 && lb_shop_halfprice_ck(item) == 1) {
        price >>= 1;
        if (discount != 0) {
            *discount = 1;
        }
    }
    if (price == 0) {
        price = 1;
    }
    return price;
}

/* 0x802108C0 - false for the items the shop never sells (0, 53, 139, 223, 381, 382, 395). */
u32 lb_shop_sellable_ck(u32 item)
{
    s32 id = (u16)item;

    if ((u32)(id - 381) <= 1 || id == 0 || id == 53 || id == 223 || id == 139 || id == 395) {
        return 0;
    }
    return 1;
}

/* 0x80210908 - true for the two box-upgrade items (580, 581). */
u32 lb_shop_box_upgrade_ck(s32 item)
{
    return (u32)((s16)item - 580) <= 1;
}

/* 0x8021092C - the buy-row state of `count` of `item`: 0x80 not for sale (0x02 no room), 0x04 too expensive. */
u16 lb_shop_buy_flags_get(u32 item, s16 count)
{
    LbShopWork* work = lobby_w.shop_0x0AC;
    LbNpcUserData* data = lobby_world_block;
    u16 flags = 0;

    if (lb_shop_sellable_ck((u16)item) == 0) {
        flags |= 0x80;
    }
    if (lb_shop_box_upgrade_ck((s16)item) == 1) {
        if (userdata_box_item_ck(lobby_world_block, item) == 1 || (s16)count > 1) {
            return flags | 0x80;
        }
    } else {
        s32 room = eft052_page_count_ck(item, work->kind_0x01C);

        if (room < (s16)count) {
            s32 free;

            flags |= 0x80;
            free = 0;
            if (work->kind_0x01C == 2) {
                u16 capacity = userdata_box_capacity(lobby_world_block);

                free = (u16)item_slots_free_count(lobby_world_block->box_0x180, capacity);
            } else if (room >= 0) {
                free = 1;
            }
            if (free == 0) {
                flags |= 2;
            }
        }
    }
    if (data->money_0x18 < (s16)count * lb_shop_price_get(item, 0)) {
        flags |= 4;
    }
    return flags;
}

/* 0x80210A5C - how many of `item` can be bought at once (up to 99). */
s32 lb_shop_buy_max_get(u16 item)
{
    s32 count;

    for (count = 1; count <= 100; count++) {
        if (lb_shop_buy_flags_get(item, count) != 0) {
            return count - 1;
        }
    }
    return 99;
}

/* 0x80210AC0 - fills the box preview's eight cells around the bought item's pouch slot. */
void lb_shop_box_preview_set(LbShopWork* self)
{
    u32 slots;
    s32 pos;
    s16 index;
    s32 i;

    memset(self->box_0x1A4, 0, 0xC0);
    slots = userdata_item_slot_count(lobby_world_block);
    pos = item_pair_index_find(self->item_0x00E, userdata_equip_item_slots_get(lobby_world_block), slots);
    self->box_page_0x264 = pos / 8;
    index = self->box_page_0x264 * 8;
    for (i = 0; i < 8; i++) {
        if (index == pos) {
            self->box_0x1A4[i].selected_0x02 = 1;
        }
        self->box_0x1A4[i].index_0x06 = index;
        index++;
    }
    if (pl_item_page2_ck(self->plw_0x03C) != 0) {
        self->box_mode_0x266 = 4;
    } else {
        self->box_mode_0x266 = 3;
    }
}

/* 0x80210C20 - pays for and hands over the bought items (a box upgrade is applied at once). */
void lb_shop_buy_commit(LbShopWork* self)
{
    score_add_clamped(-(lb_shop_price_get(self->item_0x00E, 0) * (s16)self->count_0x00C), &lobby_world_block->money_0x18);
    sysSE_req(9);
    if (lb_shop_box_upgrade_ck(self->item_0x00E) == 1) {
        userdata_item_seen_set(lobby_world_block, self->item_0x00E);
        switch (self->item_0x00E) {
        case 580:
            hud_msg_push(0, (const char*)LbStr(0, 652));
            break;
        case 581:
            hud_msg_push(0, (const char*)LbStr(0, 653));
            break;
        }
        lb_shop_box_preview_set(self);
    } else if (self->kind_0x01C == 0) {
        eft052_page_take(self->plw_0x03C, self->item_0x00E, self->count_0x00C, 0);
        lb_shop_box_preview_set(self);
    } else {
        item_box_store(self->item_0x00E, self->count_0x00C, &self->slot_0x018);
    }
}

/* 0x80210D24 - sells the chosen row's items and credits the price. */
void lb_shop_sell_commit(LbShopWork* self)
{
    s32 value = self->rows_0x144[self->cursor_0x006].price_0x4 * self->count_0x00C;

    if ((u16)userdata_item_give(lobby_world_block, self->rows_0x144[self->cursor_0x006].item_0x0, -self->count_0x00C,
                                1) == 4) {
        self->rows_0x144[self->cursor_0x006].item_0x0 = 0;
    }
    score_add_clamped(value, &lobby_world_block->money_0x18);
    sysSE_req(9);
}

/* 0x80210DBC - the buy list's input: pick a row, set the count, confirm; 2 when the player backs out. */
s32 lb_shop_buy_step(LbShopWork* self)
{
    s32 result = 0;

    switch (self->sub_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            self->item_0x00E = self->rows_0x144[self->cursor_0x006].item_0x0;
            if (self->rows_0x144[self->cursor_0x006].flags_0x8 == 0) {
                ainpc_page_hold_clear();
                self->sub_0x001++;
                self->slot_0x018 = 0xFFFF;
                self->count_0x00C = 1;
                self->count_max_0x010 = lb_shop_buy_max_get(self->item_0x00E);
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->cursor_0x006 = menu_cursor_step(self->cursor_0x006, self->rows_0x012, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->page_0x008 = menu_cursor_step_forward(self->page_0x008, self->pages_0x00A, lb_cmd_repeat_get(), 4, 8, 6,
                                                        (u16*)&self->blink_0x004);
            lb_shop_buy_list_build(self, self->page_0x008);
        }
        break;
    case 1:
        ainpc_page_hold_clear();
        switch (menu_cursor_page_move((s16*)&self->count_0x00C, self->count_max_0x010, lobby_cmd_trig_get(), lb_cmd_repeat_get(),
                                      (u16*)&self->blink_0x004)) {
        case 1:
            self->sub_0x001++;
            self->yes_no_0x014 = 0;
            sysSE_req(0);
            break;
        case 2:
            self->sub_0x001 = 0;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        switch (toggle_word_step(&self->yes_no_0x014, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_shop_buy_commit(self);
            lb_shop_buy_list_build(self, self->page_0x008);
            self->sub_0x001 = 3;
            break;
        case 2:
            self->sub_0x001 = 1;
            break;
        }
        break;
    case 3:
        if (lb_cmd_pressed_ck(0x30) != 0) {
            self->sub_0x001 = 0;
            sysSE_req(0);
        }
        break;
    }
    return result;
}

/* 0x80211044 - rebuilds the sell list's page `page`: the pouch (pages 0..2) or the carried supplies (page 3). */
void lb_shop_sell_list_build(LbShopWork* self, s16 page)
{
    s32 i;

    self->rows_0x012 = 0;
    memset(self->rows_0x144, 0, 0x60);
    if (page < 3) {
        for (i = 0; i < 8; i++) {
            if (page * 8 + i < 24) {
                u16 item = userdata_equip_item_id_get(lobby_world_block, page * 8 + i);

                self->rows_0x144[i].item_0x0 = item;
                self->rows_0x144[i].price_0x4 = eft052_item_value_get(item);
                self->rows_0x012++;
            }
        }
    } else {
        for (i = 0; i < 8; i++) {
            u16 item = lobby_world_block->supplies_0x160[i].item_id;

            self->rows_0x144[i].item_0x0 = item;
            self->rows_0x144[i].price_0x4 = eft052_item_value_get(item);
            self->rows_0x012++;
        }
    }
    if (self->cursor_0x006 >= self->rows_0x012) {
        self->cursor_0x006 = self->rows_0x012 - 1;
    }
    self->page_0x008 = page;
    self->pages_0x00A = menu_page_count(24, 8);
    if (pl_item_page2_ck(self->plw_0x03C) == 1) {
        self->pages_0x00A++;
    }
}

/* 0x80211180 - opens the sell list on its first page. */
void lb_shop_sell_open(LbShopWork* self)
{
    self->page_0x008 = 0;
    self->cursor_0x006 = 0;
    lb_shop_sell_list_build(self, 0);
}

/* 0x80211194 - the sell list's input: pick a row, set the count, confirm; 2 when the player backs out. */
s32 lb_shop_sell_step(LbShopWork* self)
{
    s32 result = 0;

    switch (self->sub_0x001) {
    case 0:
        if (lb_cmd_pressed_ck(0x10) != 0) {
            self->item_0x00E = self->rows_0x144[self->cursor_0x006].item_0x0;
            if (self->item_0x00E != 0) {
                u32 slots;

                ainpc_page_hold_clear();
                self->sub_0x001++;
                self->count_0x00C = 1;
                slots = userdata_item_slot_count(lobby_world_block);
                self->count_max_0x010 =
                    item_count_find(self->item_0x00E, userdata_equip_item_slots_get(lobby_world_block), slots);
                sysSE_req(0);
            } else {
                sysSE_req(2);
            }
        } else if (lb_cmd_pressed_ck(0x20) != 0) {
            result = 2;
            sysSE_req(1);
        } else if (lb_cmd_repeat_ck(3) != 0) {
            self->cursor_0x006 = menu_cursor_step(self->cursor_0x006, self->rows_0x012, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC) != 0) {
            self->page_0x008 = menu_cursor_step_forward(self->page_0x008, self->pages_0x00A, lb_cmd_repeat_get(), 4, 8, 6,
                                                        (u16*)&self->blink_0x004);
            lb_shop_sell_list_build(self, self->page_0x008);
        }
        break;
    case 1:
        ainpc_page_hold_clear();
        switch (menu_cursor_page_move((s16*)&self->count_0x00C, self->count_max_0x010, lobby_cmd_trig_get(), lb_cmd_repeat_get(),
                                      (u16*)&self->blink_0x004)) {
        case 1:
            self->sub_0x001++;
            self->yes_no_0x014 = 0;
            sysSE_req(0);
            break;
        case 2:
            self->sub_0x001 = 0;
            sysSE_req(1);
            break;
        }
        break;
    case 2:
        switch (toggle_word_step(&self->yes_no_0x014, lobby_cmd_trig_get(), 4, 8, 0xFFFF)) {
        case 1:
            lb_shop_sell_commit(self);
            lb_shop_sell_list_build(self, self->page_0x008);
            self->sub_0x001 = 0;
            break;
        case 2:
            self->sub_0x001 = 1;
            break;
        }
        break;
    }
    return result;
}

/* 0x802113FC - opens the buy list on its first page. */
void lb_shop_buy_open(LbShopWork* self)
{
    self->page_0x008 = 0;
    self->cursor_0x006 = 0;
    lb_shop_buy_list_build(self, 0);
}

/* 0x80211410 - the shop screen's frame: the buy/sell choice, the two lists, and the closing talk page. */
void lb_shop_step(void)
{
    LbShopWork* work = lobby_w.shop_0x0AC;

    work->blink_0x004 = 0;
    switch (work->step_0x000) {
    case 0:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            work->step_0x000++;
            lb_list_init((struct LbList*)work->choice_0x01C, 3, 0, 153, 43, 0, 5, 3, 0);
            camera_talk_lock_set(1);
            sysSE_stop(41);
        }
        break;
    case 1:
        ainpc_page_hold_set();
        switch (lb_choice_step((struct LbList*)work->choice_0x01C)) {
        case 1:
            work->sub_0x001 = 0;
            switch (work->kind_0x01C) {
            case 0:
            case 2:
                work->step_0x000 = 2;
                lb_shop_buy_open(work);
                break;
            case 1:
                work->step_0x000 = 3;
                lb_shop_sell_open(work);
                break;
            }
            break;
        case 2:
            work->step_0x000 = 4;
            lb_talk_page_open(1);
            camera_talk_lock_set(0);
            break;
        }
        break;
    case 2:
        ainpc_page_hold_set();
        if (lb_shop_buy_step(work) == 2) {
            work->step_0x000 = 1;
        }
        break;
    case 3:
        ainpc_page_hold_set();
        if (lb_shop_sell_step(work) == 2) {
            work->step_0x000 = 1;
        }
        break;
    case 4:
        if ((u32)(lb_talk_page_mode_reset() - 1) <= 1) {
            lb_shop_close();
        }
        break;
    }
    if (work->step_0x000 != 4) {
        subTransSet((u32)lb_shop_draw, 0, 0);
    }
}

/* 0x802115D4 - the buy list's help panel: why the row cannot be bought, or the count and confirm windows. */
void lb_shop_buy_help_draw(LbShopWork* self, u32 item)
{
    u16 line;
    s32 row;

    row = 1;
    switch (self->sub_0x001) {
    case 0: {
        u16 flags;

        line = 0xFFFF;
        flags = self->rows_0x144[self->cursor_0x006].flags_0x8;
        if ((flags & 0x80) != 0) {
            if ((flags & 2) != 0) {
                if (self->kind_0x01C == 2) {
                    line = 48;
                } else {
                    line = 47;
                }
            } else {
                line = 49;
            }
            row = 2;
        } else if ((flags & 4) != 0) {
            line = 82;
            row = 2;
        } else if (pl_item_pouch_ok_ck(self->plw_0x03C, self->rows_0x144[self->cursor_0x006].item_0x0) == 0) {
            line = 57;
        }
        lb_panel_msg_draw(0x1877, 46);
        if (line != 0xFFFF) {
            lb_panel_line_draw(0x1877, line, 2, row);
        } else if (self->kind_0x01C == 2) {
            lb_panel_str_print(0x1877, 50, 0);
        }
        break;
    }
    case 1:
        lb_panel_msg_draw(0x1877, 52);
        lb_count_window_draw(0x14CA, 0x14D5, (s16)self->count_0x00C, (s16)self->count_max_0x010,
                             lb_shop_price_get(item, 0), 0, (u16)self->blink_0x004);
        break;
    case 2:
        lb_help_num_put(0x1877, 54, (s16)self->count_0x00C * lb_shop_price_get(item, 0));
        lb_panel_yes_no_draw(0x1879, self->yes_no_0x014);
        break;
    case 3:
        lb_panel_msg_draw(0x1878, 56);
        break;
    }
}

/* 0x8021178C - the sell list's help panel. */
void lb_shop_sell_help_draw(LbShopWork* self, u32 item)
{
    switch (self->sub_0x001) {
    case 0:
        lb_panel_msg_draw(0x1877, 51);
        break;
    case 1:
        lb_panel_msg_draw(0x1877, 53);
        lb_count_window_draw(0x14CA, 0x14D5, (s16)self->count_0x00C, (s16)self->count_max_0x010,
                             eft052_item_value_get(item), 0, (u16)self->blink_0x004);
        break;
    case 2:
        lb_help_num_put(0x1877, 55, (s16)self->count_0x00C * eft052_item_value_get(item));
        lb_panel_yes_no_draw(0x1879, self->yes_no_0x014);
        break;
    }
}

/* 0x80211850 - draws the shop's banner (wide or 4:3) and frame. */
void lb_shop_banner_draw(void)
{
    _mh_ivec2_ pos;

    if (ck_WideMode() != 0) {
        get_lsp_data(0x14F7, &pos);
        draw_sprite_ary(lb_shop_banner_wide, &pos);
    } else {
        get_lsp_data(0x14F2, &pos);
        draw_sprite_ary(lb_shop_banner, &pos);
    }
    get_lsp_data(0x14E1, &pos);
    draw_sprite_ary(lb_shop_frame, &pos);
}

/* 0x802118D0 - draws the buy list's frame, page arrows and rows. */
void lb_shop_buy_list_draw(LbShopWork* self)
{
    u16* ids;
    u32 active;
    LbShopCell cell;
    _mh_ivec2_ pos;
    s16 i;
    _mh_ivec2_ base;

    active = self->sub_0x001 == 0;
    get_lsp_data(0x1464, &base);
    draw_sprite_ary(lb_shop_buy_frame, &base);
    lb_page_arrow_draw(0x144E, self->page_0x008, self->pages_0x00A, active == 1 ? (u16)self->blink_0x004 : 0, active);
    for (i = 0, ids = lb_shop_buy_cells; i < self->rows_0x012; i++, ids++) {
        LbSprHead* spr;
        s32 enabled;
        s32 picked;

        cell.item_0x0 = self->rows_0x144[i].item_0x0;
        cell.price_0x2 = self->rows_0x144[i].price_0x4;
        spr = (LbSprHead*)get_lsp_data(*ids, 0);
        pos.x = base.x + spr->pos.x;
        pos.y = base.y + spr->pos.y;
        enabled = self->rows_0x144[i].flags_0x8 == 0;
        if (i == self->cursor_0x006) {
            picked = 1;
            lb_item_detail_panel_draw(0x147D, cell.item_0x0, self->kind_0x01C != 0);
            menu_item_row_draw_by_lsp(0x1452, &cell.item_0x0);
        } else {
            picked = 0;
        }
        lb_item_cell_draw_layout1(self->rows_0x144[i].item_0x0, self->rows_0x144[i].price_0x4, &pos, enabled, picked,
                                  active, 2);
        if (self->rows_0x144[i].discount_0x2 != 0) {
            draw_sprite_idx(0x10F, &pos);
        }
    }
}

/* 0x80211A54 - draws the sell list's frame, page arrows and rows (the supplies page has its own frame). */
void lb_shop_sell_list_draw(LbShopWork* self)
{
    u16* ids;
    u8 arrow;
    s16 i;
    LbShopCell cell;
    _mh_ivec2_ pos;
    u16* frame;
    _mh_ivec2_ base;
    u32 active;

    active = self->sub_0x001 == 0;
    if (self->page_0x008 == 3) {
        frame = lb_shop_pouch_frame;
        ids = lb_shop_pouch_cells;
    } else {
        frame = lb_shop_sell_frame;
        ids = lb_shop_sell_cells;
    }
    get_lsp_data(0x1490, &base);
    draw_sprite_ary(frame, &base);
    arrow = 1;
    if (active == 0) {
        arrow |= 0x80;
    }
    get_lsp_data(0x144F, &pos);
    PutPageArrow(lb_page_arrow_ids, self->page_0x008, self->pages_0x00A, self->blink_0x004, &pos, arrow);
    for (i = 0; i < 8; i++, ids++) {
        LbSprHead* spr;
        s32 picked;
        u16 mode;

        cell.item_0x0 = self->rows_0x144[i].item_0x0;
        cell.price_0x2 = self->rows_0x144[i].price_0x4;
        spr = (LbSprHead*)get_lsp_data(*ids, 0);
        pos.x = base.x + spr->pos.x;
        pos.y = base.y + spr->pos.y;
        if (i == self->cursor_0x006) {
            picked = 1;
            pl_item_pouch_ok_ck(self->plw_0x03C, cell.item_0x0);
            lb_item_detail_panel_draw(0x14AC, cell.item_0x0, 0);
            menu_item_row_draw_by_lsp(0x1452, &cell.item_0x0);
        } else {
            picked = 0;
        }
        mode = 2;
        if (self->rows_0x144[i].item_0x0 == 0) {
            mode = 0x80;
        }
        if (self->page_0x008 == 3) {
            lb_price_cell_draw(self->rows_0x144[i].price_0x4, &pos, 1, picked, active, mode);
        } else {
            lb_item_cell_draw_layout1(self->rows_0x144[i].item_0x0, self->rows_0x144[i].price_0x4, &pos, 1, picked,
                                      active, mode);
        }
    }
}

/* 0x80211C18 - the shop screen's draw: banner and money, then the choice window or the active list and its help. */
void lb_shop_draw(void)
{
    _mh_ivec2_ pos;
    s16 page;
    LbShopWork* work;
    u16 sell_item;
    u16 buy_item;
    s32 flags;

    work = lobby_w.shop_0x0AC;
    flags = 0x800;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    if (work->step_0x000 != 0) {
        lb_shop_banner_draw();
        menu_money_draw(0x14FC);
    }
    switch (work->step_0x000) {
    case 1:
        lb_choice_draw((struct LbList*)work->choice_0x01C, 0x1454, lb_shop_choice_sprites, lb_shop_choice_rows, 0x1877, 0);
        break;
    case 2: {
        buy_item = work->rows_0x144[work->cursor_0x006].item_0x0;

        if (work->sub_0x001 == 3) {
            if (work->kind_0x01C == 0) {
                if (lb_shop_box_upgrade_ck((s16)work->item_0x00E) == 0) {
                    get_lsp_data(0x1450, &pos);
                    page = work->box_page_0x264;
                    if (page == 3) {
                        flags |= 1;
                    }
                    menu_frame_draw_blocks((IdValue*)userdata_equip_item_slots_get(lobby_world_block),
                                           (MenuListEntry*)work->box_0x1A4, page + 1, work->box_mode_0x266, 3, flags,
                                           &pos);
                }
            } else if (lb_shop_box_upgrade_ck((s16)work->item_0x00E) == 0) {
                get_lsp_data(0x1AE1, &pos);
                lb_page_walk_head(&pos, work->slot_0x018);
            }
        } else {
            lb_shop_buy_list_draw(work);
        }
        lb_shop_buy_help_draw(work, buy_item);
        break;
    }
    case 3: {
        sell_item = work->rows_0x144[work->cursor_0x006].item_0x0;

        lb_shop_sell_list_draw(work);
        lb_shop_sell_help_draw(work, sell_item);
        break;
    }
    }
}

} /* extern "C" */
} /* namespace lb_npc_shop */

/* ---------------------------------------------------------------------------------------------------
 * The lobby screen dispatcher and the pad command masks (0x80211E68, 0x80212370, 0x802124DC).
 */
#include "mh3_pad/Psw.h"
namespace lb_npc_screens {
extern "C" {
u32 game_ready_ck(void);
u32 menu_busy_ck(void);
s32 lb_cmd_pressed_ck(u16 mask);
u32 lobby_input_locked_ck(void);
_PLW* my_player_work_get(void);
void* memset(void* dst, s32 c, u32 n);
void lb_kitchen_step(void);
void note_trade_step(void);
void lb_shop_step(void);
void pig_menu_step(void);
void name_menu_step(void);
void lb_trade_step(void);
void lb_quest_board_step_screen(void);
void lb_quest_board_party_step(void);
void lb_quest_board_ready_step(void);
void lb_arm_wrestle_step(void);
}
}
s32 chk_pointer(void);
void sysSE_req(s32 id);
namespace lb_npc_screens {
extern "C" {

/* 0x80211E68 - runs the open lobby screen's frame (screens 0..37); the idle screen toggles the HUD on the 0x80
 * command. */
void fn_80211E68(void)
{
    _PLW* plw = my_player_work_get();

    if ((u32)(lobby_w.state_0x000 - 35) > 1 && lobby_w.state_0x000 != 0) {
        lobby_w.active_0x008 = 1;
    }
    switch (lobby_w.state_0x000) {
    case 0:
        if (plw != 0 && game_ready_ck() == 1 && menu_busy_ck() == 0 && Pl_act_ck(plw, 0, 4) == 0 &&
            lb_cmd_pressed_ck(0x80) != 0) {
            lobby_w.field_0x004 ^= 1;
            if (lobby_w.field_0x004 == 0) {
                sysSE_req(5);
            } else {
                sysSE_req(1);
            }
        }
        break;
    case 28:
    case 33:
    case 37:
        lb_city_select_screen_step();
        break;
    case 27:
        lb_server_select_screen_step();
        break;
    case 7:
        lb_screen07_step();
        break;
    case 17:
        lb_kitchen_step();
        break;
    case 23:
        note_trade_step();
        break;
    case 24:
        lb_screen24_step();
        break;
    case 2:
        lb_shop_step();
        break;
    case 21:
        pig_menu_step();
        break;
    case 22:
        name_menu_step();
        break;
    case 3:
        lb_screen03_step();
        break;
    case 15:
        lb_screen15_step();
        break;
    case 4:
        lb_deli_screen_step();
        break;
    case 11:
        lb_screen11_step();
        break;
    case 5:
        lb_item_get_screen_step();
        break;
    case 6:
        lb_box_menu_step();
        break;
    case 8:
        lb_box_screen_step();
        break;
    case 12:
        lb_quest_accept_screen_step();
        break;
    case 29:
        lb_quest_join_screen_step();
        break;
    case 13:
        lb_screen13_step();
        break;
    case 14:
        lb_area_select_screen_step();
        break;
    case 16:
        lb_screen16_step();
        break;
    case 18:
        lb_trade_step();
        break;
    case 19:
        lb_quest_board_step_screen();
        break;
    case 10:
        lb_quest_board_party_step();
        break;
    case 20:
        lb_quest_board_ready_step();
        break;
    case 26:
        lb_screen26_step();
        break;
    case 30:
        lb_screen30_step();
        break;
    case 31:
        lb_name_entry_screen_step();
        break;
    case 32:
        lb_city_move_confirm_step();
        break;
    case 34:
        lb_screen34_step();
        break;
    case 25:
        lb_arm_wrestle_step();
        break;
    case 35:
        lb_plsearch_screen_step();
        break;
    }
}

/* 0x80212370 - rebuilds the four players' lobby command masks from their pads (cleared while the pointer is in use
 * or the screen is locked). */
void fn_80212370(void)
{
    u8 i;

    if ((u32)chk_pointer() == 1 || lobby_input_locked_ck() == 1) {
        lobby_w.cmd_mask_0x084[0][0] = 0;
        lobby_w.cmd_mask_0x084[0][2] = 0;
        lobby_w.cmd_mask_0x084[0][1] = 0;
        lobby_w.cmd_mask_0x084[0][4] = 0;
        lobby_w.cmd_mask_0x084[0][3] = 0;
        lobby_w.cmd_mask_0x084[1][0] = 0;
        lobby_w.cmd_mask_0x084[1][2] = 0;
        lobby_w.cmd_mask_0x084[1][1] = 0;
        lobby_w.cmd_mask_0x084[1][4] = 0;
        lobby_w.cmd_mask_0x084[1][3] = 0;
        lobby_w.cmd_mask_0x084[2][0] = 0;
        lobby_w.cmd_mask_0x084[2][2] = 0;
        lobby_w.cmd_mask_0x084[2][1] = 0;
        lobby_w.cmd_mask_0x084[2][4] = 0;
        lobby_w.cmd_mask_0x084[2][3] = 0;
        lobby_w.cmd_mask_0x084[3][0] = 0;
        lobby_w.cmd_mask_0x084[3][2] = 0;
        lobby_w.cmd_mask_0x084[3][1] = 0;
        lobby_w.cmd_mask_0x084[3][4] = 0;
        lobby_w.cmd_mask_0x084[3][3] = 0;
        return;
    }
    for (i = 0; i < 4; i++) {
        PadButtons* pad = &Psw[i].button_0x2C0;
        u16 pressed = pad->pressed_0x04;

        lobby_w.cmd_mask_0x084[i][0] = pressed;
        lobby_w.cmd_mask_0x084[i][2] = pad->held_0x14 | pressed;
        lobby_w.cmd_mask_0x084[i][1] = pad->hold_0x00;
        lobby_w.cmd_mask_0x084[i][4] = pad->edge_on_0x1F | (pad->edge_0x1D & ~pad->edge_off_0x1E);
        lobby_w.cmd_mask_0x084[i][3] = pad->trigger_0x0C;
    }
}

/* 0x802124DC - drops the local player's pending commands and pad history and holds his input for two frames. */
void lb_pad_input_flush(void)
{
    _PLW* plw = my_player_work_get();

    lobby_w.cmd_mask_0x084[0][0] = 0;
    lobby_w.cmd_mask_0x084[0][2] = 0;
    lobby_w.cmd_mask_0x084[0][1] = 0;
    lobby_w.cmd_mask_0x084[0][4] = 0;
    lobby_w.cmd_mask_0x084[0][3] = 0;
    memset(&plw->field_0x0B8, 0, 0x84);
    plw->field_0x352 = 2;
}

} /* extern "C" */
} /* namespace lb_npc_screens */
namespace s_8020C588 {
#include "lobby/lb_cmd_pressed_ck.h"
#include "lobby/lb_npc_callees.h"
#include "lobby/lb_menu_pos_tbl.h"
#include "lobby/lb_equip_page.h"



/* The four species-id string tables, indexed by the id byte (this accessor and the three after it). */
extern "C" s32 lb_chacha_skill_name_get(u32 id) {
    return lb_chacha_skill_str[(u8)id];
}

extern "C" s32 lb_chacha_mask_name_get(u32 id) {
    return lb_chacha_mask_str[(u8)id];
}

extern "C" s32 lb_cat_name_get(u32 id) {
    return lb_cat_name[(u8)id];
}

extern "C" s32 lb_pig_name_get(u32 id) {
    return lb_pig_name[(u8)id];
}
} /* namespace s_8020C588 */


/* Returns group `kind`'s string `idx`, or group 1's first string when either index is out of range. */
void* LbStr(u8 kind, u16 idx) {
    using s_8020C588::lb_str_tbl; using s_8020C588::lbl_805B9450;
    if ((u32)kind >= 5 || (s32)idx >= lbl_805B9450[kind]) {
        return (void*)lb_str_tbl[1][0];
    }
    return (void*)lb_str_tbl[kind][idx];
}
namespace s_8020C588 {


/* Nonzero while the lobby screen is owned by a transition or a fade. */
extern "C" u32 lobby_input_locked_ck(void) {
    if (lobby_w.busy_0x172 != 0) {
        return 1;
    }
    if (get_fade_stat(0) == 1 || get_fade_stat(0) == 2) {
        return 1;
    }
    if (get_fade_stat(1) == 1 || get_fade_stat(1) == 2) {
        return 1;
    }
    if (get_fade_stat(2) == 1 || get_fade_stat(2) == 2) {
        return 1;
    }
    if (get_fade_stat(3) == 1 || get_fade_stat(3) == 2) {
        return 1;
    }
    return lobby_net_busy_ck();
}

/* The four player-0 command-mask testers; all report "clear" while the screen is not owned. */
extern "C" s32 lb_cmd_pressed_ck(u16 mask) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][0] & mask) != 0;
}

extern "C" s32 lb_cmd_held_ck(u16 mask) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][1] & mask) != 0;
}

extern "C" s32 lb_cmd_repeat_ck(u16 mask) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][2] & mask) != 0;
}

extern "C" s32 lobby_cmd_release_ck(u16 mask) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return (lobby_w.cmd_mask_0x084[0][3] & mask) != 0;
}

/* The four player-0 command-mask getters. */
extern "C" u16 lb_cmd_repeat_get(void) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][2];
}

extern "C" u16 lobby_cmd_trig_get(void) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][0];
}

extern "C" u16 glplatTextureGetHeight(void) {
    return lobby_w.cmd_mask_0x084[0][0];
}

extern "C" u16 lobby_cmd_release_get(void) {
    if (lobby_input_locked_ck() == 1) {
        return 0;
    }
    return lobby_w.cmd_mask_0x084[0][3];
}

/* Steps the caller's 4/8 sprite stepper over player 0's first mask. */
extern "C" s32 lb_yes_no_step(void* self) {
    return toggle_word_step_dpad(self, lobby_cmd_trig_get(), 4, 8);
}

/* Steps the caller's 4/8 sprite stepper over the same mask, ignoring the screen guard. */
extern "C" s32 lobby_cmd_toggle_step_raw(void* self) {
    return toggle_word_step_dpad(self, glplatTextureGetHeight(), 4, 8);
}

/* Toggles the item database's display byte and redraws the lobby when the pad loop is idle. */
extern "C" void lb_item_db_toggle(void) {
    if (game_ready_ck() == 0) {
        lobby_world_block->field_0x3E00 ^= 1;
        if (getItemListSelection() == 1) {
            fn_801E9888();
        }
        fn_801E9C58();
        fn_802FF2C0();
        fn_80220114();
    }
}

/* Decays the lobby countdown and ramps the +-0x3C cursor slide, then ticks the frame counter. */
extern "C" void fn_8021261C(void) {
    if (lobby_w.countdown_0x028 != 0) {
        lobby_w.countdown_0x028--;
    }
    if (getItemListSelection() == 0) {
        if (lobby_w.slide_0x034 < 0) {
            lobby_w.slide_0x034 = 0;
        }
        if (lobby_w.slide_0x034 < 0x3C) {
            lobby_w.slide_0x034++;
        }
    } else {
        if (lobby_w.slide_0x034 > 0) {
            lobby_w.slide_0x034 = 0;
        }
        if (lobby_w.slide_0x034 > -0x3C) {
            lobby_w.slide_0x034--;
        }
    }
    lobby_w.counter_0x030++;
}

/* The index of the first `count` entries whose running total reaches `value`, over s16 weights. */
extern "C" s16 weight_table_index(s16* table, s16 count, u32 value) {
    s16 i;
    s16 sum;

    i = 0;
    sum = 0;
    do {
        sum += *table;
        if (value < (u32)sum) {
            break;
        }
        table++;
        i++;
    } while (i < count);
    return i;
}

/* The same walk over byte weights. */
extern "C" s16 fn_80212724(u8* table, s16 count, u32 value) {
    s16 i;
    s16 sum;

    i = 0;
    sum = 0;
    do {
        sum += table[i];
        if (value < (u32)sum) {
            break;
        }
        i++;
    } while (i < count);
    return i;
}

/* Maps a lobby part kind to its string id and returns the string. */
extern "C" void fn_80212760(u32 kind) {
    u16 str_id;

    switch ((u8)kind) {
    default:
        str_id = 0xF0;
        break;
    case 1:
        str_id = 0xF1;
        break;
    case 2:
        str_id = 0xF2;
        break;
    case 3:
        str_id = 0xF3;
        break;
    case 4:
        str_id = 0xF4;
        break;
    case 5:
        str_id = 0xF5;
        break;
    case 6:
        str_id = 0xF6;
        break;
    case 7:
        str_id = 0xFA;
        break;
    case 8:
        str_id = 0xFB;
        break;
    case 9:
        str_id = 0xFC;
        break;
    case 10:
        str_id = 0xFD;
        break;
    case 11:
        str_id = 0xFE;
        break;
    case 12:
        str_id = 0xF7;
        break;
    case 13:
        str_id = 0xFF;
        break;
    case 14:
        str_id = 0x100;
        break;
    case 15:
        str_id = 0x101;
        break;
    case 18:
        str_id = 0xE8;
        break;
    }
    LbStr(0, str_id);
}

} /* namespace s_8020C588 */
#pragma dont_inline reset
#pragma peephole reset
