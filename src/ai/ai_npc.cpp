/*
 * ai/ai_npc.cpp - the AI companion (`_AINPC_W`, `ai/ainpc.h` and the view `ai/ai_npc.h`): setup and frame, the motion
 *   runner and its per-motion step arms (motions 0..7), the plan decision and plan arms, the enemy scan and item picks,
 *   the motion/attack band, the effect dispatcher, the tuning and status helpers and the state checks (`ai_skill_ck`,
 *   `ai_torch_ck`, `ai_taru_*_ck`, `ai_demo_stop_ck`).  C++; every C-named row is `extern "C"` so objdiff pairs it by
 *   the map's name (playbook 42).
 * RANGE. .text 0x802C2700-0x802D9EA4 (357 functions, every one with a body); extab, extabindex, .ctors 0x8056F388
 *   (`ainpc_w_sinit`, which runs `ainpc_w_construct`), .data 0x805D3A88-0x805D54A0, .bss 0x806BD360-0x806BD808
 *   (`ainpc_w`), .sdata, .sbss, .sdata2.  `jumptable_805D5428` is the 14-entry table MWCC emits for `ai_npc_level_rank_get`.
 * FLAGS. `cflags_main` (configure.py) with a file-wide `#pragma optimization_level 4` (the switch-range lowering
 *   `subi; cmplwi` of `ai_npc_step20_arm`; no row regresses).  `#pragma peephole off` from 0x802C2700 to the end of
 *   0x802D44F4's band, with `peephole on` wrapped around fn_802D1954, fn_802D2024, fn_802D29B8, fn_802D29CC,
 *   fn_802D29E4, fn_802D3A34, fn_802D40A4 and ai_npc_skill_group_get; `fp_contract off` around
 *   `ai_npc_move_target_set` (retail's separate `fmuls`/`fadds`).  Per function (each measured, playbook 120's
 *   narrower forms): `opt_common_subs off` (playbook 208) where retail reloads a value MWCC would keep (a field
 *   across a call, a loop test, the `shell_set_func_ptr` table loaded after the arguments) - ai_npc_step13/31/57_arm,
 *   ai_npc_m2_step0/2_arm, ai_npc_m3_step6/7/19_arm, ai_npc_move_target_set, ai_npc_weak_point_pos_get,
 *   fn_802D32B4 (file-wide it loses 195 bytes); `opt_propagation off` where retail keeps a folded constant or a
 *   re-mask - ai_npc_step4_arm, ai_npc_counter_target_ck (with `opt_lifetimes off`), fn_802D3F1C, fn_802D6690.
 * NAMES. Module `ai` from the record every function takes (`ai_skill_ck__FP8_AINPC_WUc`, `ai_area_ck__FP8_AINPC_W`,
 *   `get_joint_wpos_ai__FP8_AINPC_WUlPQ34nw4r4math4VEC3`); no `__FILE__` string covers the range and the runtime dump
 *   answers `zz_` for the `fn_` rows.  `ainpc_entry_tbl`/`ainpc_page_state` (`hud/cockpit.cpp`'s `.bss`) are GUESSes
 *   from their users.  GUESS (from the bodies): ai_npc_hold_item_arm, ai_npc_hold_ck, ai_npc_arrived_ck,
 *   ai_npc_motion_step_ck, ai_npc_player_in_reach_ck, ai_npc_motion_dispatch, ainpc_move, ai_npc_hand_reach_ck,
 *   ai_npc_item_give, ai_npc_formation_roll, ai_behaviour_scaled_get, ai_skill_mask_get, ainpc_anim_res_set, ainpc_frame,
 *   ainpc_start, ai_npc_wait_arm(_alt), ai_npc_motion0_dispatch .. ai_npc_motion7_dispatch, ai_npc_motion_run,
 *   ai_npc_step_dispatch and the step arms `ai_npc_step<N>_arm` / `ai_npc_m<M>_step<N>_arm` (named by motion M and the
 *   first step N their dispatcher hands them), ai_npc_motion_set/_change/_play, ai_npc_motion_end_ck,
 *   ai_npc_frame_ck, ai_npc_variant_set, ai_npc_base_state_set, ai_npc_attack_hold_ck, ai_npc_water_landing_ck,
 *   ai_npc_demo_frame_ck, ai_npc_target_player_set, ai_npc_in_range_ck, ai_npc_item34_ck, ai_npc_enemy_scan,
 *   ai_npc_item_gauge_ck, ai_npc_buff_item_pick/_use, ai_npc_support_item_pick, ai_npc_carried_ck,
 *   ai_npc_alt_step_get, ai_npc_gather_spot_find, ai_npc_carve_spot_find, ai_npc_engage_range_get,
 *   ai_npc_enemy_target_pick/_engage/_track, ai_npc_move_target_set, ai_npc_target_ahead_set,
 *   ai_npc_counter_target_ck, ai_npc_player_watch, ai_npc_target_refresh, ai_npc_plan_decide, ai_npc_plan_dispatch,
 *   the plan arms `ai_npc_plan<N>_arm` (N = the plan index) and ai_npc_plan_retreat_arm, ai_npc_behaviour_apply,
 *   ai_npc_attack_update, ai_npc_defense_update, ai_npc_weak_point_pick/_pos_get/_roll, ai_npc_item_throw_roll,
 *   ai_npc_tuning_get, ai_npc_item_stock_get, ai_npc_beside_player_ck, ai_npc_player_idle_count,
 *   ai_npc_element_pick_update/_plan_decide/_report_update/_frame, ainpc_element_report_set, ai_npc_status_preview,
 *   ai_npc_bonus_add, ai_npc_item_step_ck, ai_npc_counter_window_get, ai_npc_talk, ai_npc_tutorial_talk,
 *   ainpc_area_buff_apply, ainpc_w_construct, ainpc_w_sinit; the tables ai_npc_gather_motion_tbl, ai_npc_hop_tbl,
 *   ai_npc_leap_tbl, ai_npc_vault_tbl, ai_npc_hop_back_tbl, ai_npc_flurry_frames, ai_npc_spin_frames,
 *   ai_npc_status_motion_tbl(2), ai_npc_tuning_tbl, ai_npc_item_count_tbl, ai_npc_time_left_chance_tbl,
 *   ai_npc_motion_variant_tbl, result_hunt_rank_tbl, result_time_rank_tbl, result_rank_points_tbl,
 *   ai_npc_spawn_pos_tbl, ai_npc_hand_reach_dist_sq, ai_npc_f32_0_7; the record types AINPCBehaviour, AINPCTuning,
 *   AINPCSetup, AINPCStatus, EmWeakPoint, AINPCTimeChance, AINPCSpawnPos, ResultRankPoints; the view's fields
 *   `drift_vel`/`drift_active`, `phase`, `timer_0x1AC`, `hold_kind_0x492`/`hold_time_0x4C0` (`_PLW`).
 * RESIDUALS. 40 rows partial (objdiff); the measured ones:
 *  - `ai_npc_status_preview`, `ai_npc_move_target_set`, `ai_npc_enemy_scan`, `ai_npc_m2_step11_arm`,
 *    `ai_npc_enemy_engage`, `ai_npc_step25_arm`, `ai_npc_item_stock_get`, `fn_802D6A00`, `fn_802D3F1C`,
 *    `fn_802D67F8`/`fn_802D675C`, `fn_802D7F10`: callee-saved or scratch register colouring (declaration orders tried);
 *  - `ai_npc_skill_group_get`: retail loads the list base after masking the id, ours (peephole on) uses `lwzu`;
 *  - `fn_802D30F8`: the `else` arm masks `target` before the subtract; retail writes the result to `target`'s register;
 *  - `ai_npc_step4_arm`/`ai_npc_m4_step5_arm`: retail loads the motion `lha` and masks it (`clrlwi 16`) for the `u16`
 *    parameter; every spelling folds it into `lhz`;
 *  - `ai_npc_m2_step2/5/7_arm`: retail masks the constant ternary it passes as `fn_802D30F8`'s limit
 *    (`li r5,..; clrlwi r5,r5,16`); every spelling folds the constant;
 *  - `ai_npc_m3_step5_arm`: retail masks `spin` (`u16`) at each assignment, ours at the call;
 *  - `fn_802D1AFC`: retail moves `fn_802D89D4`'s result to r0 before masking it for the call;
 *  - `fn_802CD20C`: retail's definition returns a full word (`srawi r3`), its callers want the `s16`;
 *  - `fn_802D77DC`: retail's `li r3,30; bnelr` default (a `switch` gives the signed compares, not the layout);
 *  - `ai_npc_pair_values_get`: retail's `lbzx` operand order (`index, base`); a `[4][4]` table view moves other rows;
 *  - `fn_802D2264`: the final `return 0`/`return 1` blocks are laid out in the other order;
 *  - `fn_802D3398`: retail re-reads `*row` at the top of the second loop's body;
 *  - `fn_802D2F7C`: retail adds the row offset to the table base (`addi; mulli; add`), ours multiplies first;
 *  - `ai_npc_step82_arm`: retail re-masks `flag` for the second test (`opt_common_subs off` does not reach it);
 *  - `ai_taru_move_ck`: retail loads +0x450 into r0 before `extsb r3,r0`;
 *  - data: `.data` 0x9D4 of 0x1A18 and `.sdata2` 0x1DC of 0x208 emitted (the tables are declared, never defined),
 *    `.ctors` (0x4), `.sdata` (0x98) and `.sbss` (0x8) claimed but not emitted; the jump tables are emitted
 *    anonymous, so objdiff leaves the map's `jumptable_*` rows unpaired (playbook 23);
 *  - `ai/result_hunt_rank_get.h` declares `result_hunt_rank_get` `s16` for its callers; the definition returns `s32`
 *    (retail returns the sum unextended), so this file does not include that leaf header.
 * SHAPES. Record views: the bodies written in `namespace view_fn_802D0F34` read `ai/ai_npc.h`'s `_AINPC_W` (the
 *   owner's view, with the model at +0x008); the older global bodies read `ai/ainpc.h`'s.  The namespace's declarations
 *   of this unit's global functions (and the view header `ai/fn_802D0F34.h`) restate them with the view's record;
 *   C linkage keeps one symbol.  `AINPC_VIEW` names `ainpc_w` through the view; the setup functions read it directly
 *   where retail addresses `ainpc_w@l` and through a local pointer elsewhere.  Unifying the views removes the namespace.
 *  - A callee whose retail caller passes a wider or narrower type than its declaration is called through a `_c1`/`_u16`
 *    cast view (`quest_move_state_get_c1`, `em_roster_area_use_take_c1`, `ai_npc_motion_set_u16`,
 *    `AINPC_SELECTOR_SET`, `AINPC_ITEM_ID_GET`); one whose retail caller loads the float argument earlier is called
 *    with the float moved forward (`AINPC_KEY_SPAWN`, `AINPC_FX_SPAWN`, playbook 178), and the view header spells
 *    `coll_wall_slide`, `vec3_move_damp`, `mhchar_motion_set` and `hit_data_apply` in their owners' parameter order.
 *  - Retail's signed `cmpwi` on a `u8` field or `u8` argument is a `switch` (the promoted operand,
 *    playbook 209), its case bodies in retail's layout order; a run of identical
 *    tests over slots 8..11 is a loop MWCC unrolls (`fn_802D40EC`, playbook 184).
 *  - Functions that address `ainpc_w` through a register take a local `_AINPC_W* w = &ainpc_w;` (retail's first test
 *    still reads `ainpc_w@l`); a countdown that stores the raw sum is `x--`/`++x`, never `x = x - 1` (playbook 38).
 *  - Step dispatchers are `switch` statements on `motion_step` whose cases tail-call the arms (retail's `bctr` tables).
 *  - State steps are `self->state++`, never `self->state = state + 1` (a `clrlwi` retail lacks, playbook 38); the
 *    flag parameters of the older bodies are `u32` tested `(flag & 0xFF)`.
 *  - The four motion tables (`lbl_805D4784`..`lbl_805D48D8`) and the step tables are `s16`/`u16` word arrays indexed
 *    `arg * N`: a row struct folds the index into one `mulli`.
 */

#pragma optimization_level 4
#include "types.h"
#include "camera/camera.h"
#include "nw4r/math.h"

/* Owner headers (rule 2): every symbol a registered unit defines is declared in that unit's header,
 * never here.  `unsplit/unknown.h` carries the module-ambiguous ones. */
#include "ef/fn_800CDB2C.h"
#include "fn_80047398.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_anmscn.h"
#include "lobby/lb_npc.h"
#include "mh3_pad.h"
#include "stage/stg_w.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/unknown.h"

/* The record ai_npc_motion0_dispatch dispatches on: its state word is at +0x172 and its two arm handlers are
 * ai_npc_wait_arm / ai_npc_wait_arm_alt. */
typedef struct LightArm {
    /* +0x000 */ u8 unused_0x000[0x172];
    /* +0x172 */ u16 state;
} LightArm; /* size: 0x174 traced (the record is larger; approximation) */
extern "C" void ai_npc_wait_arm(LightArm* self);
extern "C" void ai_npc_wait_arm_alt(LightArm* self);

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                         */
/* ------------------------------------------------------------------------------------------------ */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "pl.h"                /* `_PLW` - the record's +0x650/+0x652 the motion switch reads */
#include "Pl/fn_8027D684.h"    /* `fn_8027D76C` (rule 2: the owner's header) */
#include "Pl/fn_8028F66C.h"    /* `copyVec3`, `fn_8012A624`, `fn_80291B08` */
#include "fn_8004CAD8.h"       /* `rotVecY`, `subVec3`, `addVec3` */
#include "sound/fn_800D7F54.h" /* `fn_800DCC24` */
#include "ai/ainpc.h"
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its `set_target`/`request` slots (rule 2) */

/* The four motion tables the band's rows live in, `.data` 0x805D4784..0x805D4964: the ladder's
 * 5-word rows (0x805D4810, 0x805D4874) and the wider 7-word rows (0x805D4784, 0x805D48D8).  They sit in
 * this unit's `.data` and are `extern`-declared, never defined (playbook 29).
 *
 * They are declared as `u16` word arrays, not as a row struct: a 10/14-byte struct's index scale folds
 * into one `mulli`, and retail's index math is the strength-reduced `arg * 5` / `arg * 7` followed by
 * the u16 scale (`slwi`+`add`+`slwi`), i.e. the source carried the word stride itself.  Word offsets,
 * ascending - the layout a struct would have stated, kept here because the tables are word arrays:
 *
 *   5-word rows                                7-word rows
 *   word +0 (byte +0x00) anim_id   s16         word +0 (byte +0x00) anim_id   s16
 *   word +1 (byte +0x02) motion    u16         word +1 (byte +0x02) motion    u16
 *   word +2 (byte +0x04) param     s16         word +2 (byte +0x04) param     s16
 *   word +3 (byte +0x06) variant   s16         word +3 (byte +0x06) variant   s16
 *   word +4 (byte +0x08) se_id     u16         word +4 (byte +0x08) motion2   u16
 *                                              word +5 (byte +0x0A) param2    s16
 *                                              word +6 (byte +0x0C) se_id     u16
 */
enum {
    ROW5_ANIM = 0, ROW5_MOTION = 1, ROW5_PARAM = 2, ROW5_VARIANT = 3, ROW5_SE = 4, ROW5_WORDS = 5,
};
enum {
    ROW7_ANIM = 0, ROW7_MOTION = 1, ROW7_PARAM = 2, ROW7_VARIANT = 3, ROW7_MOTION2 = 4,
    ROW7_PARAM2 = 5, ROW7_SE = 6, ROW7_WORDS = 7,
};

extern "C" {

extern u16 lbl_805D4784[];   /* 10 x 7 words: the flight's own ladder */
extern u16 lbl_805D4810[];   /* 10 x 5 words */
extern u16 lbl_805D4874[];   /* 10 x 5 words */
extern u16 lbl_805D48D8[];   /* 10 x 7 words */

/* The `.sdata2` constants the state machines compare against and arm (playbook 29: declared, never
 * defined - this unit's own pool). */
extern f32 lbl_8079A69C;
extern f32 lbl_8079A6A0;
extern f32 lbl_8079A6A4;
extern f32 lbl_8079A6A8;
extern f32 lbl_8079A6AC;
extern f32 lbl_8079A6B0;
extern f32 lbl_8079A6B4;
extern f32 lbl_8079A6B8;

/* The motion band's shared helpers (this unit's own, 0x802D2904-0x802D9D30), declared ahead of their
 * definitions in the call sites' own argument shapes; `ai_npc_talk` is not written yet. */
void ai_npc_motion_set(struct _AINPC_W* self, u32 motion, s32 param, u32 flag);
void ai_npc_motion_change(struct _AINPC_W* self, u32 motion, s32 param, u32 flag);
u32 ai_npc_motion_end_ck(struct _AINPC_W* self);
u32 ai_npc_frame_ck(struct _AINPC_W* self, u32 flag, f32 low, f32 high);
void ai_npc_variant_set(struct _AINPC_W* self, u32 variant);
void fn_802D2ABC(struct _AINPC_W* self, u32 flag, u16 a, u16 se);
void ai_npc_base_state_set(struct _AINPC_W* self, u32 state);
u32 ai_npc_flag_ck(struct _AINPC_W* self, u32 flag);
void fn_802D2B88(struct _AINPC_W* self, u32 flag);
void fn_802D31FC(struct _AINPC_W* self);
void fn_802D3210(struct _AINPC_W* self, u32* value);
void fn_802D3A2C(struct _AINPC_W* self, u32 flag);
void fn_802D3CCC(struct _AINPC_W* self, u32 flag);
void fn_802D3CD4(struct _AINPC_W* self, u32 flag);
void fn_802D40A4(struct _AINPC_W* self, u16 motion);
void fn_802D40EC(struct _AINPC_W* self, u16 motion, s16 param);
void fn_802D9A44(struct _AINPC_W* self);
void ai_npc_talk(struct _AINPC_W* self, s32 a, u32 b, u32 c, u32 d);
}

#include "pl.h"
#include "Pl/pl_act.h"
#include "Pl/fn_8028F66C.h"
#include "g3d/g3d_calcworld.h"
#include "ef/fn_800CDB2C.h"
#include "unsplit/ai.h"

#ifdef __cplusplus

extern "C" {
#endif

/* More of the motion band's helpers this band dispatches into (this unit's own), declared ahead of
 * their definitions with the callees' own signatures. */
void fn_802D3A2C(struct _AINPC_W* self, u32 a);
void fn_802D3CCC(struct _AINPC_W* self, u32 a);
void fn_802D3CD4(struct _AINPC_W* self, u32 a);
void fn_802D3CFC(struct _AINPC_W* self, u32 a);
void fn_802D29B8(struct _AINPC_W* self, u32 a);
void ai_npc_variant_set(struct _AINPC_W* self, u32 a);
u32 ai_npc_frame_ck(struct _AINPC_W* self, u32 a, f32 b, f32 c);
void fn_802D2B88(struct _AINPC_W* self, u32 a);
void fn_802D2B98(struct _AINPC_W* self, u32 a);
void ai_npc_base_state_set(struct _AINPC_W* self, u32 a);
void fn_802D35B4(struct _AINPC_W* self, s32 a);
void fn_802D3684(struct _AINPC_W* self);
void fn_802D6690(struct _AINPC_W* self);
void fn_802D9400(struct _AINPC_W* self);
void fn_802D29CC(struct _AINPC_W* self, f32 a);
u32 fn_802D3B34(struct _AINPC_W* self, s32 a, s32 b);
u16 fn_802D30F8(u16 a, u16 b, u32 c);
void fn_802D3210(struct _AINPC_W* self, u32* out);

/* `pl.h` pulls `ef.h` in, which declares `setVec3`/`VEC3_ctor` with their owner's type
 * (`nw4r::math::VEC3*`), the same record this file's locals use. */

/* Callees whose owner is a registered unit but whose header does not declare them yet: each spelling
 * is that owner's own declaration. */
void fn_8010072C(struct _PLW* owner, u8 type, nw4r::math::VEC3* pos, u32 param, f32 scale);
/* `fn_8010072C` with its scale fourth: retail loads f1 before r6. */
#define AINPC_FX_SPAWN ((void (*)(struct _PLW*, u8, nw4r::math::VEC3*, f32, u32))fn_8010072C)
u32 fn_8027DCE0(struct _PLW* self, u8 arg1);
s32 pl_ground_check_all_layers(struct _PLW* self, nw4r::math::VEC3* pos, LandData* land, f32* out, u32 kind);
void fn_80114CC8(void* actor, u8 key);

#ifdef __cplusplus
}
#endif

/* The target's player work (`_PLW`, `pl.h`), stored at +0x16C; its +0x3C triple is the position. */
struct _PLW;

/* `_AINPC_W` and `AINPCFormation` come from `ai/ainpc.h`. */

/* The C++-spelled entry points this unit defines below: their map names carry argument lists, so
 * declaring them at global C++ scope reproduces the target's mangling (rule 9). */
#include "ai/ai_torch_ck.h"
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);

extern "C" {
u32 fn_8027D74C(struct _PLW* plw);
void ai_npc_enemy_track(struct _AINPC_W* self, u32 a);
void fn_802D32B4(struct _AINPC_W* self, void* entry);
u32 pl_act6_class_ck(struct _PLW* plw, u32 a);
u32 fn_802D86D4(struct _AINPC_W* self, u16* a, s16* b);
void fn_802D65CC(struct _AINPC_W* self);
void fn_802D675C(struct _AINPC_W* self);
void fn_802D67F8(struct _AINPC_W* self);
void fn_802D3A20(struct _AINPC_W* self);
void fn_802D3AD8(struct _AINPC_W* self);
f32 ai_npc_tuning_get(struct _AINPC_W* self, u32 a);

/* 0x805D4030 - the 4-pointer table the dispatchers hand to `fn_802D32B4`, in this unit's `.data`:
 * declared, never defined (playbook 29). */
extern void* lbl_805D4030[4];
s32 fn_802D3184(struct _AINPC_W* self, u32 flag);
void fn_802D3B10(struct _AINPC_W* self);
void fn_802D3B24(struct _AINPC_W* self);
void fn_802D4200(struct _AINPC_W* self);
void fn_802D4218(struct _AINPC_W* self);
void fn_802D4230(struct _AINPC_W* self, f32 value);
void fn_802D4238(struct _AINPC_W* self);
void fn_802D2A00(struct _AINPC_W* self, u32 a, u32 b, u32 c);
s16 ai_npc_counter_window_get(void);

/* 0x802CD20C is written below; 0x802CCE18 and 0x802CCF00 are this unit's, not written yet. */
}

/* The dispatch state object.  Only the two bytes the function reads are named; everything else is
 * padding the disassembly does not describe. */
typedef struct DispatchState {
    u8 pad_0x000[0x170];    /* +0x000 */
    u8 variant;             /* +0x170: selects the odd/even table entry */
    u8 pad_0x171[0x2AF];    /* +0x171 */
    u8 state;               /* +0x420: the switch value */
} DispatchState;            /* size: 0x421 */

/* 16 entries, each handed to the tail-called handler.  The table is this unit's `.data`, declared and
 * never defined (playbook 29); the handler `fn_802D3398` is defined in the view namespace below. */
extern "C" {
extern void* const lbl_805D4150[16];
extern void fn_802D3398(DispatchState* self, void* entry);
}
#include "ai/ainpc_w.h" /* `ainpc_w`, defined at the foot of this file (rule 2) */
#include "mh3_pad/lb_param_w.h" /* `lb_param_w`, owned by mh3_pad.cpp (rule 2) */
#include "ai/fn_802D44F4.h"    /* this unit's and `hud/cockpit.cpp`'s declarations (rule 2) */
#include "quest/quest_item_slot.h" /* quest_move_state_valid_ck (rule 2: its owner) */
#include "ai/ainpc_w.h" /* `ainpc_w`, defined at the foot of this file (rule 2) */

/* The C++-spelled entry points of this range: their map names carry argument lists, so they are
 * declared at global C++ scope and the front-end reproduces the mangling (rule 9). */
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill);
extern "C" u32 ai_npc_item_step_ck(struct _AINPC_W* self);
s32 ai_area_ck(struct _AINPC_W* self);

extern "C" {

/* The tables these bodies index, in this unit's `.data`: declared, never defined (playbook 29), each as
 * a **complete** array so the base comes out as `lis`/`addi` like the target's. */
extern u8 lbl_805D4190[0x10];
extern u8 lbl_805D41A0[0x10];
extern u16 lbl_805D41B0[0x10];
extern char* lbl_805D4390[4];
extern struct AINPCBehaviour* lbl_805D4358[7];
extern u16 lbl_805D4560[6];

/* The per-monster hold-slot table at 0x805D43A0: 31 records of four u16 (size 0xF8).  Field 0 of a
 * record is the id the scan compares, fields 1..3 the values `fn_802D7BA4`..`fn_802D7BE4` hand out;
 * the records are reached as `lbl_805D43A0[index * 4 + field]` because that is the addressing the
 * object itself uses (`slwi r3,r0,2` then `addi r0,r3,1; slwi r0,r0,1`). */
extern u16 lbl_805D43A0[0x7C];
/* The matching 0x805D44EC table: 21 records of two u16 (size 0x54). */
extern u16 lbl_805D44EC[0x2A];

void fn_800DCCF8(void* state, nw4r::math::VEC3* pos, s32 enable);

/* The AI-NPC work record's own helpers, all inside this range and defined below. */
s32 ai_npc_level_rank_get(u8 value);
void ai_npc_tutorial_talk(void);
s32 fn_802D77DC(u8 slot);
u32 ai_npc_motion_step_ck(struct _AINPC_W* self, u8 motion, u16 step);
s32 fn_802D66B8(struct _AINPC_W* self);
void fn_802D84D0(struct _AINPC_W* self, s16 value);
}

/* The C++-linkage callees the view below calls: declared at global scope so the calls resolve to the map's
 * manglings (a declaration inside the view namespace would mangle the namespace in). */
#include "ef/get_move_work_adrs.h"
#include "ef/pRoot.h"
struct _HIT_W;
void hit_flag_set(struct _HIT_W* hit, u32 flags);
u16 ai_get_motion_no(struct _AINPC_W* self);
#include "draw_shape.h"                    /* res_file_ctor, res_file_assign (rule 2) */
#include "nw_resource.h"                   /* ckResourceName, ResEntry (rule 2) */
#include "nw_res_list_load.h"
#include "ainpc_model_res_ids.h"
#include "ainpc_anim_res_ids.h"
#include "sound/mhchar.h"                  /* mhchar_reset (rule 2) */
#include "sound/mhchar_model_create.h"
#include "sound/mhchar_res_bank_set.h"
#include "Pl/em_weak_point.h"               /* EmWeakPoint at global scope, before the view namespace re-reads `ai/ai_npc.h` */
#include "menu/hit_attack_list_push.h"     /* hit_source_set, hit_owner_set, hit_id_list_clear (rule 2) */
#include "enemy/em_pop.h"                  /* quest_flag_8_ck, quest_key20_flag_ck (rule 2) */
#include "enemy/em007_model_eft_start.h"
#include "enemy/eft_frame_npc_create.h"
#include "ef/eft_npc_model_set_create.h"
#include "ef/eft_npc_slot_attach.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "enemy/senko_set.h"
#include "ef/eft022_set.h"
#include "ef/eft019.h"                     /* eft019_set (rule 2) */
#include "Pl/pl_area_buff_apply.h"
#include "enemy/em_model.h"                /* em_roster_area_use_take, em_weight_table_pick (rule 2) */
#include "stage/stage_climate_kind_get.h"
#include "fn_8004CAD8/mtx.h"                /* MTX34_ctor (rule 2) */
#include "ef/eft013_npc_motion_spawn.h"
#include "ef/eft_npc_aura_create.h"
#include "sound/se_npc_faint_play.h"
#include "sound/se_npc_revive_play.h"
#include "Pl/pl_quest_end_request.h"
#include "enemy/enemy_control.h"              /* em_demo_time_ck (rule 2) */
#include "stage/stage_zone_hit_ck.h"          /* stage_zone_hit_ck, stage_group_word_get (rule 2) */
#include "Pl/pl_coll.h"                       /* GetGroundHit, findInterSection (rule 2) */
#include "enemy/ENEMY_WORK.h"                 /* _ENEMY_WORK (rule 1) */
#include "enemy/fn_8012EC74.h"                /* em_sleep_ck, get_em_chg_scale (rule 2) */
#include "enemy/fn_801251D0.h"                /* get_enemy_data (rule 2) */
#include "enemy/EnemyData.h"                  /* EnemyData, EmSizeParams (rule 1) */
#include "enemy/em_flag_0x8AD_ck.h"
#include "enemy/em_tall_ck.h"
#include "lobby/lb_fabsf.h"
#include "Pl/pl_area_spot_valid_ck.h"
#include "Pl/pl_act_class_get.h"
#include "Pl/pl_act_range2_ck.h"
#include "Pl/pl_act_84_86_ck.h"
#include "Pl/Pl_master_ck.h"                   /* Pl_act_ck (rule 2) */
#include "ef/sys_quest_move.h"                /* SysQuestMove (rule 1) */
#include "enemy/em_flags836_ck.h"
#include "lobby/lb_quest_screen.h"            /* quest_element_pick_ck (rule 2) */
#include "lobby/lb_server_sel_trans.h"        /* talk_msg_speaker_get, npc_talk_start (rule 2) */
#include "enemy/em020_status_flags_get.h"
#include "mh3_pad/system_w.h"                 /* system_w (rule 2) */
u32 ai_demo_stop_ck(void);
void get_joint_wpos_ai(struct _AINPC_W* self, u32 joint, nw4r::math::VEC3* out);
/* The SE callback the companion's first sound entry is opened with (`.sbss` 0x80794B70, this unit's own). */
extern "C" void (*ainpc_snd_func)(struct _ENEMY_WORK* owner, s32 event);

namespace view_fn_802D0F34 {

namespace nw4r = ::nw4r;  /* the view headers below reopen it */

#include "ai/ai_npc.h"
/* data keeps its map name (a view namespace would mangle it): the header is read with C linkage */
extern "C" {
#include "ai/fn_802D0F34.h"
}

/* This unit's own functions and tables the view's bodies reach, declared with the view's record (the
 * definitions elsewhere in this file take `ai/ainpc.h`'s; C linkage keeps the map's names). */
extern "C" {
void ainpc_anim_res_set(void);
void ainpc_start(struct _AINPC_W* self);
u32 fn_802CCE04(struct _AINPC_W* self);
void ai_npc_step24_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step25_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step27_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step29_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step31_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step32_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step37_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step42_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step47_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step56_arm(struct _AINPC_W* self);
void ai_npc_step57_arm(struct _AINPC_W* self, u8 arg);
void ai_npc_step82_arm(struct _AINPC_W* self, u32 flag);
void ai_npc_step83_arm(struct _AINPC_W* self);
void ai_npc_step85_arm(struct _AINPC_W* self);
void ai_npc_step86_arm(struct _AINPC_W* self, u8 flag);
void ai_npc_step88_arm(struct _AINPC_W* self, u8 flag);
void ai_npc_step90_arm(struct _AINPC_W* self);
void ai_npc_step93_arm(struct _AINPC_W* self);
void ai_npc_m2_step0_arm(struct _AINPC_W* self, u32 flag);
void ai_npc_m2_step2_arm(struct _AINPC_W* self, u32 flag);
void ai_npc_m2_step5_arm(struct _AINPC_W* self, u32 flag);
void ai_npc_m2_step7_arm(struct _AINPC_W* self, u32 flag);
void ai_npc_m2_step9_arm(struct _AINPC_W* self);
s16 fn_802D7CB0(struct _AINPC_W* self);
void fn_802D6B98(struct _AINPC_W* self);
s32 ai_npc_weak_point_roll(struct _AINPC_W* self);
u32 ai_npc_item_throw_roll(struct _AINPC_W* self);
struct EmWeakPoint* ai_npc_weak_point_pick(struct _AINPC_W* self, u8 any_height);
void ai_npc_formation_roll(struct _AINPC_W* self);
u32 fn_802CCD90(struct _AINPC_W* self);
u32 fn_802CD75C(struct _AINPC_W* self);
u32 fn_802CD6FC(struct _AINPC_W* self);
u32 fn_802CDAB8(struct _AINPC_W* self);
u32 fn_802CD2E0(struct _AINPC_W* self);
u32 fn_802CD588(struct _AINPC_W* self);
u32 fn_802D7A74(struct _AINPC_W* self);
void fn_802CDDC4(struct _AINPC_W* self, u8 arg);
void fn_802CDF4C(struct _AINPC_W* self);
void fn_802CE2A0(struct _AINPC_W* self);
void fn_802CE3B4(struct _AINPC_W* self);
void fn_802CE4EC(struct _AINPC_W* self);
void fn_802CF390(struct _AINPC_W* self);
void fn_802CF704(struct _AINPC_W* self);
void fn_802CE618(struct _AINPC_W* self);
void fn_802CF8E4(struct _AINPC_W* self);
void fn_802CDF78(struct _AINPC_W* self);
void fn_802CE0D8(struct _AINPC_W* self);
void fn_802CE170(struct _AINPC_W* self);
void fn_802CFABC(struct _AINPC_W* self);
void fn_802CF48C(struct _AINPC_W* self);
void fn_802CFBC8(struct _AINPC_W* self);
void fn_802CFBF4(struct _AINPC_W* self);
void fn_802CFC94(struct _AINPC_W* self);
void fn_802CFE20(struct _AINPC_W* self);
void fn_802CFFD0(struct _AINPC_W* self);
void fn_802D0068(struct _AINPC_W* self, u8 arg);
void fn_802D0148(struct _AINPC_W* self, u8 arg);
void fn_802D0228(struct _AINPC_W* self);
void fn_802D02C0(struct _AINPC_W* self);
void fn_802D0358(struct _AINPC_W* self, u8 arg);
void fn_802D04CC(struct _AINPC_W* self);
void fn_802D063C(struct _AINPC_W* self);
void fn_802D07AC(struct _AINPC_W* self);
void fn_802D0840(struct _AINPC_W* self);
void fn_802D08D4(struct _AINPC_W* self);
void fn_802D0994(struct _AINPC_W* self);
void fn_802D0A20(struct _AINPC_W* self);
void fn_802D0B28(struct _AINPC_W* self);
void fn_802D0C30(struct _AINPC_W* self);
void ai_npc_weak_point_pos_get(struct _AINPC_W* self, struct EmWeakPoint* point, struct _ENEMY_WORK* enemy, nw4r::math::VEC3* out);
extern s16 ai_npc_tuning_tbl[64];          /* 0x805D53A8: rows of {throw chance, tuning 1, tuning 2} by progress (+0x339) */
extern s16 ai_npc_item_count_tbl[8];       /* 0x805D4640: the stock of each item list of the forest formation */
extern u16* ai_npc_motion_variant_tbl[6];
struct AINPCFormation* ai_npc_behaviour_get(u8 index);
u32 fn_802D7F10(u8* slots, u8 value);
void ai_npc_element_report_update(struct _AINPC_W* self);
void fn_802D9A44(struct _AINPC_W* self);
extern f32 ai_npc_leap_tbl[9];         /* 0x805D3BC4: 3 rows of {launch speed, gravity, forward speed} */
extern f32 ai_npc_vault_tbl[4];        /* 0x805D3BE8: the vault's launch speed, gravity and forward speed */
extern f32 ai_npc_hop_back_tbl[3];     /* 0x805D4B34: the airborne recoil's launch speed, gravity and drift */
extern f32 ai_npc_flurry_frames[5];    /* 0x805D4B40: the frames the flurry arms a hit at, 0-terminated */
extern f32 ai_npc_spin_frames[5];      /* 0x805D4B54: the frames the spinning charge arms a hit at */
extern s16 ai_npc_status_motion_tbl[36];   /* 0x805D4BC8: 6 rows of {variant, motion, param, count, closing motion, its param} */
extern s16 ai_npc_status_motion_tbl2[24];  /* 0x805D4C10: 4 rows of the same shape */

void ai_npc_behaviour_apply(struct _AINPC_W* self);
void fn_802D6964(struct _AINPC_W* self, s32 slot, s8 value);
void fn_802D69A4(struct _AINPC_W* self, s32 slot, s8 value);
void fn_802D69E4(struct _AINPC_W* self, u8 flag, s16 value);
void fn_802D6B6C(struct _AINPC_W* self, s16 delta);
void fn_802D7DF0(struct _AINPC_W* self, s16 value);
void ai_npc_bonus_add(struct _AINPC_W* self, u8 kind, u32 amount);

/* ---- this unit's own `.data` tables the view's bodies read (declared, never defined - playbook 29) ---- */
extern s16 ai_npc_gather_motion_tbl[16];   /* 0x805D4720: 4 rows of {variant, motion, param, closing variant} */
extern f32 ai_npc_hop_tbl[3];              /* 0x805D4740: the hop's launch speed, gravity and floor */
void fn_802D6A00(struct _AINPC_W* self);
void fn_802D7464(struct _AINPC_W* self);
void fn_802D84D0(struct _AINPC_W* self, s16 value);
void fn_802D9400(struct _AINPC_W* self);
u8 ai_npc_level_get(u16 value);
u16 fn_802D7BA4(u8 index);
u16 fn_802D7BC4(u8 index);
u8 fn_802D7C04(u16 value);
u16 fn_802D7C4C(u8 index);
u8 fn_802D7C6C(u16 value);
}

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

/* `ainpc_w` seen through this view's record (`ai/ainpc_w.h` declares it with `ai/ainpc.h`'s tag). */
#define AINPC_VIEW (*(_AINPC_W*)&::ainpc_w)
/* `quest_move_state_get` hands back a byte (its callers store r3 as is). */
#define quest_move_state_get_c1 ((u8 (*)(void))quest_move_state_get)
/* `ai_npc_motion_set` is called with the motion as a `u16` from the motion tables. */
#define ai_npc_motion_set_u16 ((void (*)(_AINPC_W*, u16, s32, s32))ai_npc_motion_set)
/* `em_roster_area_use_take` answers a byte (its callers test it with `clrlwi`). */
#define em_roster_area_use_take_c1 ((u8 (*)(u8, nw4r::math::VEC3*, u32))em_roster_area_use_take)

#pragma peephole off

extern "C" {

extern const f32 ai_npc_f32_0_7;

/* 0x802C2700 (0x160): resets the AI companion record, builds its model from the model resource and opens its
 * two sound entries. */
void ainpc_init(void)
{
    _AINPC_W* self = &AINPC_VIEW;
    u32 file;
    u32 data;
    ResEntry* entry;

    res_file_ctor(&file, 0);
    self->in_area = 0;
    self->phase = 0;
    self->state = 0;
    self->sub_step = 0;
    self->field_0x006 = 0;
    AINPC_VIEW.active = 1;
    self->plw_0x16C = NULL;
    self->pos_0x178.x = 0.0f;
    self->pos_0x178.y = 0.0f;
    self->pos_0x178.z = 0.0f;
    copyVec3(&self->pos_0x184, &self->pos_0x178);
    self->vec_0x190.x = 0;
    self->vec_0x190.y = 0;
    self->vec_0x190.z = 0;
    self->area = 0xFF;
    ai_npc_variant_set(self, 0);
    entry = ckResourceName((char*)ainpc_model_res_ids[0].b_0x04);
    if (entry != NULL) {
        res_file_assign(&file, res_file_ctor(&data, (u32)entry->data));
        mhchar_reset(&self->model);
        mhchar_model_create(&self->model, &file, 0x29, 6);
    }
    self->model.scale_0x1C.x = 0.75f;
    self->model.scale_0x1C.y = 0.75f;
    self->model.scale_0x1C.z = 0.75f;
    ainpc_anim_res_set();
    self->field_0x420 = 0;
    self->field_0x488 = 0;
    self->field_0x48C = 0;
    self->field_0x490 = 0;
    self->field_0x494 = 0;
    self->skill_0x42E[0] = 0;
    self->skill_0x42E[1] = 0;
    self->skill_0x42E[2] = 0;
    self->sound_handle_0x498 = (u32)se_entry_request(9, (struct _ENEMY_WORK*)self, ainpc_snd_func);
    self->sound_handle_0x49C = (u32)se_entry_request(0xB, (struct _ENEMY_WORK*)self, NULL);
    self->field_0x485 = 0;
    self->field_0x484 = 0;
}

/* 0x802C2860 (0x34): requests the AI companion's model and animation resources. */
void ainpc_res_load(void)
{
    nw_res_list_load(ainpc_model_res_ids);
    nw_res_list_load(ainpc_anim_res_ids);
}

/* 0x802C2894 (0x6C): registers the AI companion's animation file with the model banks (kind 3). */
void ainpc_anim_res_set(void)
{
    u32 file;
    u32 data;
    ResEntry* entry;

    res_file_ctor(&file, 0);
    entry = ckResourceName((char*)ainpc_anim_res_ids[0].b_0x04);
    if (entry != NULL) {
        res_file_assign(&file, res_file_ctor(&data, (u32)entry->data));
        mhchar_res_bank_set(3, 0, &file);
    }
}

/* 0x802C2900 (0x88): the per-frame entry: sets the record up on its first frame, then runs it unless a demo
 * stops it. */
void ainpc_frame(void)
{
    _AINPC_W* self = &AINPC_VIEW;

    if (quest_move_state_valid_ck() != 0 && self->active != 0) {
        switch (self->phase) {
        case 0:
            ainpc_start(self);
            break;
        case 1:
            if (ai_demo_stop_ck() == 1) {
                self->in_area = 0;
            } else {
                fn_802D1D90(self);
            }
            break;
        }
    }
}

/* 0x802C2988 (0x448): sets the record up for the quest: the target player, the tuning read from the lobby
 * parameters, the hit entries, the counters and the effects. */
void ainpc_start(_AINPC_W* self)
{
    u32 n;
    u8 i;
    s16 gauge;

    self->phase++;
    self->in_area = 1;
    self->plw_0x16C = (_PLW*)::get_move_work_adrs(2);
    self->pos_0x178.x = 0.0f;
    self->pos_0x178.y = 0.0f;
    self->pos_0x178.z = 0.0f;
    fn_802D1FFC(self);
    self->pos_0x1B0.x = 0.0f;
    self->pos_0x1B0.y = 0.0f;
    self->pos_0x1B0.z = 0.0f;
    mhchar_model_mtx_update(&self->model);
    self->area = 0xFF;
    ai_npc_motion_set(self, 1, 0, 0);
    self->field_0x33C = 0;
    self->field_0x338 = ai_npc_level_get(self->field_0x33E = lb_param_w.sub_0x2C);
    self->field_0x1FE = fn_802D7BA4(self->field_0x338);
    self->field_0x200 = fn_802D7BC4(self->field_0x338);
    self->field_0x210 = 0;
    self->gauge_max_0x1F4 = 100;
    self->gauge_refill_0x1F8 = 100;
    fn_802D3684(self);
    self->field_0x1BC = 0.0f;
    self->field_0x1C0 = 0.0f;
    self->field_0x1CC = 0;
    self->enemy_index = 0xFF;
    self->field_0x1C8 = 0;
    fn_802D41C8(self);
    fn_802D4200(self);
    self->field_0x1D1 = 0;
    self->field_0x214 = 0;
    self->field_0x216 = 0;
    self->field_0x212 = 0;
    self->field_0x218 = 0;
    for (i = 0; i < 2; i++) {
        self->hit[i].active_0x05 = 0;
        self->hit[i].result = 0xFF;
        hit_source_set((::_HIT_W*)&self->hit[i], 3, self);
        hit_owner_set((::_HIT_W*)&self->hit[i], 3, self);
    }
    hit_id_list_clear(self->hit_ids, &self->hit_id_count);
    self->field_0x339 = fn_802D7C04(self->field_0x340 = lb_param_w.sub_0x2E);
    gauge = fn_802D7C4C(self->field_0x339);
    self->field_0x3D6 = gauge;
    self->field_0x3D4 = gauge;
    self->field_0x3DE = 0;
    self->field_0x3F2 = 0;
    if ((self->field_0x3F6 = lb_param_w.sub_0x29) == (self->field_0x3F7 = lb_param_w.sub_0x2A)) {
        self->field_0x3F6 = 0;
        self->field_0x3F7 = 1;
    }
    self->field_0x3F0 = 0xFF;
    fn_802D6A00(self);
    self->field_0x3DA = 0;
    self->field_0x3DC = 0;
    self->field_0x33A = fn_802D7C6C(self->field_0x342 = lb_param_w.sub_0x30);
    self->field_0x420 = quest_move_state_get_c1();
    self->formation_0x41C = (AINPCFormation*)lbl_805D4358[self->field_0x420];
    self->field_0x422 = 0;
    self->field_0x424 = 0;
    ai_npc_behaviour_apply(self);
    self->field_0x426 = 0;
    self->weak_point_0x427[0] = 0;
    self->weak_point_0x427[1] = 0;
    self->weak_point_0x427[2] = 0;
    self->weak_point_0x427[3] = 0;
    self->weak_point_0x427[4] = 0;
    self->field_0x42C = 0;
    fn_802D7464(self);
    self->field_0x356 = 0;
    self->field_0x358 = 0;
    self->field_0x35A = 0;
    fn_802D3AD8(self);
    fn_802D3AF8(self);
    fn_802D3B10(self);
    fn_802D3B24(self);
    fn_802D3A20(self);
    fn_802D3CB8(self);
    self->field_0x3C8 = 0;
    self->field_0x3CA = 0;
    self->field_0x3CE = 0;
    self->field_0x3D0 = 0;
    self->field_0x3D2 = 0;
    self->field_0x35C = 0;
    self->field_0x35E = 0;
    self->field_0x362 = 0;
    fn_802D4238(self);
    self->field_0x377 = 0;
    self->field_0x37C = 0.0f;
    self->field_0x380 = 0;
    fn_802D3CDC(self);
    fn_802D3CEC(self);
    self->field_0x417 = 0;
    self->field_0x418 = 0;
    self->field_0x364 = 0;
    setVector3(&self->pos_0x368, 0.0f, 0.0f, 0.0f);
    self->field_0x416 = 0;
    self->field_0x38C = 0;
    for (n = 0; n < 12; n++) {
        memset(self->slots, 0, sizeof(self->slots));
    }
    self->field_0x356 = 0;
    self->entry_index = 0;
    self->field_0x3C2 = 0;
    self->field_0x43D = 0;
    self->field_0x43E = 0;
    self->field_0x389 = 1;
    self->field_0x440 = 0;
    self->field_0x43C = 0;
    self->field_0x445 = 0;
    self->field_0x446 = 0;
    self->field_0x451 = 0;
    self->field_0x44C = 1.0f;
    self->field_0x383 = 1;
    self->field_0x386 = 0;
    fn_802D84D0(self, 0);
    self->field_0x480 = 0;
    self->field_0x482 = self->field_0x420 + 2;
    self->field_0x460 = 0;
    self->field_0x461 = 0;
    self->field_0x462 = 0;
    self->field_0x464 = 0;
    self->hunt_bonus = 0;
    self->time_bonus = 0;
    em007_model_eft_start((struct _ENEMY_WORK*)self);
    eft_npc_model_set_create((::_AINPC_W*)self);
    eft_frame_npc_create((::_AINPC_W*)self, ai_npc_f32_0_7);
    eft_npc_slot_attach((::_AINPC_W*)self);
    if (quest_flag_8_ck(0) == 1 && move_work_state_ck() == 0) {
        self->field_0x483 = 1;
        fn_802D9400(self);
        self->field_0x486 = 90;
        self->field_0x200 = 40;
    } else if (quest_key20_flag_ck(0) == 1) {
        self->field_0x483 = 2;
    } else if (quest_key20_flag_ck(1) == 1) {
        self->field_0x483 = 3;
    } else {
        self->field_0x483 = 0;
    }
}

/* 0x802C2DD0 (0x9C): waits a random 5..20 frames in the base motion (variant 0), then returns to the base
 * state. */
void ai_npc_wait_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_change(self, 1, 4, 0);
        self->timer_0x1AC = (::ran_suu(1) & 0xF) + 5;
    case 1:
        if (--self->timer_0x1AC <= 0) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C2E6C (0x9C): the same wait in the alternate variant (2). */
void ai_npc_wait_arm_alt(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_change(self, 3, 0x10, 0);
        self->timer_0x1AC = (::ran_suu(1) & 0xF) + 5;
    case 1:
        if (--self->timer_0x1AC <= 0) {
            ai_npc_base_state_set(self, 2);
        }
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

#pragma peephole off

/* Moves the record to the arm its state selects. */
extern "C" void ai_npc_motion0_dispatch(LightArm* self)
{
    switch (self->state) {
    case 0:
        ai_npc_wait_arm(self);
        break;
    case 1:
        ai_npc_wait_arm_alt(self);
        break;
    }
}

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802C2F2C (0x80): step 0 - plays motion 1 once and returns to the base state. */
void ai_npc_step0_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_change(self, 1, 4, 0);
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C2FAC (0x1CC): steps 1, 2 and 21 - plays the idle variant `arg` selects (with a random voice line for the
 * first), then returns to the variant's base state. */
void ai_npc_step1_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        switch (arg) {
        case 0:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 2, 0, 0);
            if (self->field_0x1CC == 0 && fn_802CCE04(self) == 0) {
                switch (::ran_suu(1) % 12) {
                case 0:
                    ai_npc_talk(self, (s8)self->field_0x482, 0x1E, 0, 2);
                    break;
                case 1:
                    ai_npc_talk(self, (s8)self->field_0x482, 0x1F, 0, 2);
                    break;
                }
            }
            break;
        case 1:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x14, 2, 0);
            fn_802D29CC(self, 1.5f);
            break;
        case 2:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x3D, 2, 0);
            fn_802D29CC(self, 1.4f);
            break;
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 2) {
                ai_npc_base_state_set(self, 2);
            } else {
                ai_npc_base_state_set(self, 0);
            }
        }
        break;
    }
}

/* 0x802C3178 (0x268): steps 4, 14, 26 and 52 - the gather motion: plays the row's motion, takes an item from the
 * area roster every 60 frames while it loops, and finishes with the row's closing motion. */
#pragma opt_propagation off
void ai_npc_step4_arm(_AINPC_W* self, u8 arg)
{
    s16* row;
    u16 item;
    u8 state;

    self->field_0x417 = 1;
    state = self->state;
    switch (state) {
    case 0:
        row = &ai_npc_gather_motion_tbl[arg * 4];
        self->state++;
        ai_npc_variant_set(self, row[0]);
        ai_npc_motion_set_u16(self, row[1], row[2], 0);
        self->timer_0x1AC = 60;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            switch (arg) {
            case 0:
            case 2:
                self->field_0x358 = 0;
                if (arg == 0) {
                    ai_npc_motion_set(self, 5, 4, 0);
                } else {
                    ai_npc_motion_set(self, 0x38, 4, 0);
                }
                ai_npc_talk(self, (s8)self->field_0x482, 5, 0, 2);
                break;
            case 1:
            case 3:
                self->field_0x35C = 0;
                if (arg == 1) {
                    fn_802D2ABC(self, 1, 0x17, 0);
                } else {
                    fn_802D2ABC(self, 1, 0x36, 0);
                }
                ai_npc_talk(self, (s8)self->field_0x482, 4, 0, 2);
                break;
            }
        } else {
            if (--self->timer_0x1AC == 0) {
                switch (em_roster_area_use_take_c1(self->area, &self->pos_0x368, self->field_0x364)) {
                case 1:
                    item = em_weight_table_pick((const EmWeightEntry*)self->field_0x364);
                    if (item != 0) {
                        fn_802D40A4(self, item);
                        fn_802D7DF0(self, 90);
                    }
                    break;
                }
                self->field_0x356++;
            }
            if (ai_npc_frame_ck(self, 3, 0.0f, 60.0f) != 0) {
                self->field_0x3C4 = 1;
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            row = &ai_npc_gather_motion_tbl[arg * 4];
            ai_npc_base_state_set(self, row[0]);
        }
        break;
    }
}
#pragma opt_propagation reset

/* 0x802C33E0 (0x1A4): steps 5 and 55 - the hop: arg 1 launches the record along its +0x1D4 velocity with the
 * +0x1E0 gravity until it falls below the table's floor. */
void ai_npc_step5_arm(_AINPC_W* self, u8 arg)
{
    u8 state;

    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x10, 0, 0);
        } else {
            ai_npc_variant_set(self, 2);
            self->vec_0x190.x = 0xC000;
            ai_npc_motion_set(self, 0x3B, 0, 0);
            fn_802D31FC(self);
            self->pos_0x1D4.y = ai_npc_hop_tbl[0];
            self->pos_0x1E0.x = 0.0f;
            self->pos_0x1E0.y = ai_npc_hop_tbl[1];
            self->pos_0x1E0.z = 0.0f;
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        } else {
            if (ai_npc_frame_ck(self, 1, 50.0f, 0.0f) != 0) {
                self->vec_0x190.x = fn_802D30F8(0, (u16)self->vec_0x190.x, 0x600);
            }
            if (arg == 1 && self->pos_0x1D4.y > ai_npc_hop_tbl[2]) {
                addVec3To(&self->pos_0x178, &self->pos_0x1D4);
                addVec3To(&self->pos_0x1D4, &self->pos_0x1E0);
            }
        }
        break;
    }
}

/* 0x802C3584 (0x1E4): steps 6 and 16 - turns toward the move target while the motion plays, with a random voice
 * line, then hands over to the follow-up state. */
void ai_npc_step6_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    u8 state;

    VEC3_ctor(&dir);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x19, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x41, 4, 0);
        }
        if (ai_npc_flag_ck(self, 0x20) == 0) {
            if (::ran_suu(1) & 1) {
                ai_npc_talk(self, (s8)self->field_0x482, 0, 0, 2);
            } else {
                ai_npc_talk(self, (s8)self->field_0x482, 1, 0, 2);
            }
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                fn_802D2ABC(self, 2, 0x14, 0);
            } else {
                fn_802D2ABC(self, 2, 0x15, 0);
            }
        } else {
            subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
            copyVec3(&dir, &diff);
            calcVecAngXY(&dir, &pitch, &yaw);
            self->vec_0x190.y = fn_802D30F8(yaw, (u16)self->vec_0x190.y, 0x600);
            if (arg == 1) {
                self->vec_0x190.x = fn_802D30F8(0, (u16)self->vec_0x190.x, 0x600);
            }
        }
        break;
    }
}

/* 0x802C3768 (0xF0): steps 8 and 15 - plays the motion once with voice line 0x10, then returns to the base
 * state. */
void ai_npc_step8_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x18, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x40, 2, 0);
        }
        ai_npc_talk(self, (s8)self->field_0x482, 0x10, 0, 2);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802C3858 (0x140): step 9 - lands: snaps to the move target (onto the ground unless the player is mid-air) and
 * picks the landing reaction from the ground's kind. */
void ai_npc_step9_arm(_AINPC_W* self)
{
    LandData land;
    f32 height;
    u8 state;

    em_land_data_init(&land);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 0xE, 4, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (self->plw_0x16C->field_0x5A4 & 0x4000) {
                self->pos_0x178.x = self->pos_0x1B0.x;
                self->pos_0x178.z = self->pos_0x1B0.z;
            } else {
                copyVec3(&self->pos_0x178, &self->pos_0x1B0);
                if (pl_ground_check_all_layers(self, &self->pos_0x178, &land, &height, -5) == 1) {
                    self->pos_0x178.y = self->field_0x19C = height;
                }
            }
            copyVec3(&self->pos_0x184, &self->pos_0x178);
            self->field_0x3C6 = 0;
            fn_802D3CCC(self, 1);
            if (land.field_0x00 & 1) {
                fn_802D2ABC(self, 1, 0x37, 0);
            } else {
                fn_802D2ABC(self, 1, 5, 0);
            }
        }
        break;
    }
}

/* 0x802C3998 (0x1C4): steps 10, 11 and 22 - the attack wind-up: plays the motion at speed 1.4 with a voice
 * line, then returns to the variant's base state. */
void ai_npc_step10_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        switch (arg) {
        case 0:
            ai_npc_motion_set(self, 0x18, 6, 0x3E);
            break;
        case 1:
            ai_npc_motion_set(self, 0x18, 6, 0x3E);
            break;
        case 2:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x40, 6, 0x3E);
            break;
        }
        fn_802D29CC(self, 1.4f);
        if (self->field_0x354 == 3) {
            ai_npc_talk(self, (s8)self->field_0x482, 3, 0, 2);
        } else if (self->field_0x1CC == 4) {
            switch (::ran_suu(1) % 12) {
            case 0:
                ai_npc_talk(self, (s8)self->field_0x482, 0x1C, 0, 2);
                break;
            case 1:
                ai_npc_talk(self, (s8)self->field_0x482, 0x1D, 0, 2);
                break;
            }
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 2) {
                ai_npc_base_state_set(self, 2);
            } else {
                ai_npc_base_state_set(self, 0);
            }
        }
        break;
    }
}

/* 0x802C3B5C (0x124): steps 12 and 17 - plays the motion twice, then waits for frame 84 before returning to the
 * base state. */
void ai_npc_step12_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 2;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x1B, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x43, 4, 0);
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (--self->sub_step == 0) {
                self->state++;
            }
        }
        break;
    case 2:
        if (ai_npc_frame_ck(self, 0, 84.0f, 0.0f) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802C3C80 (0x460): steps 13 and 60 - uses the item the record carries (+0x3F0): heals, buffs or throws it,
 * plays its effect, then loops the follow-up motion three times. */
#pragma opt_common_subs off
void ai_npc_step13_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 pos;
    u8 state;

    VEC3_ctor(&pos);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 3;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0xD4, 0, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0xDE, 0, 0);
        }
        if (ai_npc_flag_ck(self, 8) == 0) {
            ai_npc_talk(self, (s8)self->field_0x482, 8, 0, 2);
            switch (self->field_0x3F0) {
            case 6:
                fn_802D35B4(self, 20);
                fn_80114CC8(self, 0);
                pl_area_buff_apply(self->area, 3);
                break;
            case 7:
                fn_802D35B4(self, 50);
                fn_80114CC8(self, 0);
                pl_area_buff_apply(self->area, 6);
                break;
            case 1:
                fn_802D6964(self, 1, self->field_0x1FE * 3 / 100);
                self->field_0x236 = 5400;
                self->field_0x40A = 9000;
                fn_80114CC8(self, 1);
                pl_area_buff_apply(self->area, 4);
                break;
            case 2:
                fn_802D6964(self, 1, self->field_0x1FE * 5 / 100);
                self->field_0x236 = 5400;
                self->field_0x40A = 9000;
                fn_80114CC8(self, 1);
                pl_area_buff_apply(self->area, 7);
                break;
            case 3:
                fn_802D69A4(self, 1, 10);
                self->field_0x23A = 5400;
                self->field_0x40C = 9000;
                fn_80114CC8(self, 2);
                pl_area_buff_apply(self->area, 5);
                break;
            case 4:
                fn_802D69A4(self, 1, 20);
                self->field_0x23A = 5400;
                self->field_0x40C = 9000;
                fn_80114CC8(self, 2);
                pl_area_buff_apply(self->area, 8);
                break;
            case 5:
                self->field_0x21C = 0;
                fn_80114CC8(self, 4);
                pl_area_buff_apply(self->area, 9);
                break;
            case 8:
                switch (stage_climate_kind_get()) {
                case 1:
                    pl_area_buff_apply(self->area, 0xB);
                    break;
                case 2:
                    pl_area_buff_apply(self->area, 0xA);
                    break;
                }
                break;
            case 9:
                fn_802D69E4(self, 1, 5400);
                fn_80114CC8(self, 9);
                self->field_0x410 = 9000;
                pl_area_buff_apply(self->area, 0xD);
                break;
            case 10:
                self->field_0x40E = 9000;
                pl_area_buff_apply(self->area, 0xC);
                break;
            case 13:
                self->field_0x412 = 9000;
                pl_area_buff_apply(self->area, 0xE);
                break;
            case 11:
                ::get_joint_wpos_ai((::_AINPC_W*)self, 0xE, &pos);
                ::senko_set(&pos, 2000.0f, self->area, 0x1B);
                ::eft022_set(&pos, self->area);
                break;
            case 12:
                ::get_joint_wpos_ai((::_AINPC_W*)self, 0xE, &pos);
                shell_set_func_ptr->method_0x10((::_AINPC_W*)self, &pos, 4, self->area, shell_set_func_ptr);
                ::eft019_set(&pos, self->area, 0x21);
                break;
            case 0:
                self->field_0x408 = 900;
                break;
            }
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (self->sub_step == 0) {
                if (arg == 0) {
                    ai_npc_base_state_set(self, 0);
                } else {
                    ai_npc_base_state_set(self, 2);
                }
            } else {
                self->sub_step--;
                if (arg == 0) {
                    ai_npc_motion_change(self, 0xD5, 0, 0);
                } else {
                    ai_npc_motion_change(self, 0xDF, 0, 0);
                }
            }
        }
        break;
    }
}
#pragma opt_common_subs reset

/* 0x802C40E0 (0x7C): step 18 - holds motion 0x15 while +0x468 is 6. */
void ai_npc_step18_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 0x15, 4, 0);
        break;
    case 1:
        if (self->field_0x468 != 6) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C415C (0x7C): step 19 - holds motion 0x1D while +0x468 is 8. */
void ai_npc_step19_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 0x1D, 6, 0);
        break;
    case 1:
        if (self->field_0x468 != 8) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C41D8 (0x458): step 20 - receives an item from the player: reacts to the hunt bonus and the item kind
 * (+0x3C2), and applies the item's gauge, bonus and stat effects at frame 90. */
void ai_npc_step20_arm(_AINPC_W* self)
{
    s32 voiced = 0;
    u8 state;

    self->field_0x417 = 1;
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 7, 4, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (self->hunt_bonus >= 100) {
                ai_npc_talk(self, (s8)self->field_0x482, 2, 0, 2);
                voiced = 1;
            }
            switch (self->field_0x3C2) {
            default:
                ai_npc_motion_set(self, 0x1C, 4, 0);
                if (voiced == 0) {
                    ai_npc_talk(self, (s8)self->field_0x482, 0x15, 0, 2);
                }
                break;
            case 27:
                ai_npc_motion_set(self, 0x1C, 4, 0);
                if (voiced == 0) {
                    ai_npc_talk(self, (s8)self->field_0x482, 0x16, 0, 2);
                }
                break;
            case 28:
            case 30:
            case 31:
            case 32:
                self->field_0x417 = 0;
                ai_npc_talk(self, (s8)self->field_0x482, 0x17, 0, 2);
                fn_802D3A34(self, 50);
                ai_npc_bonus_add(self, 0, 10);
                ai_npc_bonus_add(self, 1, 1);
                switch (self->field_0x3C2) {
                case 28:
                    ai_npc_motion_set(self, 0x18, 4, 0);
                    break;
                case 30:
                    ai_npc_motion_set(self, 0x18, 4, 0);
                    if (::ai_skill_ck((::_AINPC_W*)self, 0x1A) == 0) {
                        self->field_0x21C = 10;
                    }
                    break;
                case 32:
                    self->field_0x3C2 = 0;
                    fn_802D2ABC(self, 4, 10, 0);
                    break;
                case 31:
                    self->field_0x3C2 = 0;
                    fn_802D2ABC(self, 4, 8, 0);
                    break;
                }
                break;
            }
        }
        break;
    case 2:
        if (::ai_get_motion_no((::_AINPC_W*)self) == 0x18) {
            self->field_0x417 = 0;
        } else if (ai_npc_frame_ck(self, 0, 90.0f, 0.0f) == 1) {
            fn_80114CC8(self, 0);
            switch (self->plw_0x16C->field_0x650) {
            case 0x2D:
                fn_802D35B4(self, 50);
                fn_802D6B2C(self, 50);
                ai_npc_bonus_add(self, 0, 15);
                ai_npc_bonus_add(self, 1, 2);
                break;
            case 0x1B:
                fn_802D3694(self, 25);
                fn_802D35B4(self, 50);
                ai_npc_bonus_add(self, 0, 25);
                ai_npc_bonus_add(self, 1, 3);
                break;
            case 0xEF:
                fn_802D3694(self, 25);
                fn_802D35B4(self, 50);
                fn_802D6B6C(self, 25);
                fn_802D6B2C(self, 50);
                ai_npc_bonus_add(self, 0, 40);
                ai_npc_bonus_add(self, 1, 5);
                break;
            case 0x1A:
                fn_802D3694(self, 50);
                fn_802D35B4(self, (s16)self->gauge_max_0x1F4);
                ai_npc_bonus_add(self, 0, 70);
                ai_npc_bonus_add(self, 1, 7);
                break;
            case 0xF0:
                fn_802D3694(self, 50);
                fn_802D35B4(self, (s16)self->gauge_max_0x1F4);
                fn_802D6B6C(self, 50);
                fn_802D6B2C(self, self->field_0x3D6);
                ai_npc_bonus_add(self, 0, 100);
                ai_npc_bonus_add(self, 1, 10);
                break;
            }
        }
        if (ai_npc_motion_end_ck(self) == 1) {
            self->field_0x3C2 = 0;
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C4630 (0x11C): steps 23 and 54 - plays the motion twice, then returns to the base state. */
void ai_npc_step23_arm(_AINPC_W* self, u8 arg)
{
    u8 state;

    if (ai_npc_flag_ck(self, 0x10) != 0) {
        fn_802D3A2C(self, 2);
    }
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 2;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x16, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x3E, 4, 0);
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1 && --self->sub_step == 0) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */


/* The aimed action's first stage: arms the swing, then hands the motion's own ladder to the effect
 * pass once `ai_npc_frame_ck` reports the arm window open. */
extern "C" void ai_npc_step24_arm(struct _AINPC_W* self, u8 arg) {
    u8 state;
    if (ai_npc_flag_ck(self, 0x10) != 0) {
        fn_802D3A2C(self, 2);
    }
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 3;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x17, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x3f, 4, 0);
        }
        break;
    case 1:
        if ((arg == 0 && ai_npc_frame_ck(self, 1, lbl_8079A69C, lbl_8079A670) != 0) ||
            (arg == 1 && ai_npc_frame_ck(self, 1, lbl_8079A6A0, lbl_8079A670) != 0)) {
            if (self->sub_step == 0) {
                self->state++;
            } else {
                self->sub_step--;
                if (arg == 0) {
                    ai_npc_motion_set(self, 0x17, 0, 0x1c);
                } else {
                    ai_npc_motion_set(self, 0x3f, 0, 0x30);
                }
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* The ranged action's arm: reads the enemy's own motion out of the player work it targets and leaves
 * the record in the matching motion before the target's frame pass runs. */
extern "C" void ai_npc_step25_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        struct _PLW* plw;
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 6, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x39, 4, 0);
        }
        plw = self->plw_0x16C;
        switch (plw->field_0x650) {
        case 0x1c:
            self->field_0x3C2 = 0x1c;
            break;
        case 0x1b:
        case 0x2d:
        case 0xef:
            self->field_0x3C2 = 0x1b;
            break;
        case 0x1a:
        case 0xf0:
            self->field_0x3C2 = 0x1a;
            break;
        case 0x1e:
            self->field_0x3C2 = 0x1e;
            break;
        case 0x1f:
            self->field_0x3C2 = 0x1f;
            break;
        case 0x20:
            self->field_0x3C2 = 0x20;
            break;
        default:
            self->field_0x3C2 = 0;
            fn_802D40EC(self, plw->field_0x650, plw->field_0x652);
            if (self->field_0x420 == 3) {
                self->field_0x422 = 0x708;
                self->field_0x42C = plw->field_0x650;
                self->field_0x427 = 0;
                self->field_0x428 = 0;
                self->field_0x429 = 0;
                self->field_0x42A = 0;
                self->field_0x42B = 0;
            }
            break;
        }
        fn_8027D76C(plw);
        break;
    }
    case 1: {
        struct _PLW* plw = self->plw_0x16C;
        if (ai_npc_motion_end_ck(self) != 1) {
            break;
        }
        if (arg == 0) {
            if (self->field_0x3C2 != 0) {
                fn_802D2ABC(self, 1, 0x14, 0);
            } else {
                fn_802D2ABC(self, 1, 0x17, 0);
                ai_npc_talk(self, (s8)self->field_0x482, 0x13, plw->field_0x650, 2);
            }
        } else {
            if (self->field_0x3C2 != 0) {
                fn_802D40A4(self, plw->field_0x650);
            }
            fn_802D2ABC(self, 1, 0x36, 0);
            ai_npc_talk(self, (s8)self->field_0x482, 0x13, plw->field_0x650, 2);
        }
        break;
    }
    }
}

/* The arm-out: cancels the armed motion's tail and returns the record to the variant it started the
 * action with. */
extern "C" void ai_npc_step27_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x25, 8, 0);
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, self->variant);
        }
        break;
    }
}

/* The two-stage aimed action's first half. */
extern "C" void ai_npc_step29_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x12, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x42, 4, 0);
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* The four-stage aimed action: the swing, its hold, the settle (which hands the target to the shell
 * table's own state change) and the arm-out. */
#pragma opt_common_subs off
extern "C" void ai_npc_step31_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x2d, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x51, 2, 0);
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 4, 2, 0);
            } else {
                ai_npc_motion_set(self, 0x37, 2, 0);
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x31, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x53, 6, 0);
            }
            shell_set_func_ptr->set_target(self, shell_set_func_ptr);
            fn_802D9A44(self);
            self->field_0x447 = 0;
            self->field_0x446 = 1;
            ai_npc_talk(self, (s8)self->field_0x482, 0xa, 0, 2);
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}
#pragma opt_common_subs reset

/* The arrow's flight: the row keys the shot, `rotVecY` builds the shot vector out of the record's own
 * yaw, and the per-frame pass integrates the effect vector and the stage timer. */
extern "C" void ai_npc_step32_arm(struct _AINPC_W* self, u8 arg) {
    nw4r::math::VEC3 shot;
    VEC3_ctor(&shot);
    shot.x = lbl_8079A670;
    shot.y = lbl_8079A670;
    shot.z = lbl_8079A6A4;
    switch (self->state) {
    case 0: {
        u16* row;
        nw4r::math::VEC3 out;
        self->state++;
        switch (arg) {
        case 0:
        case 5:
            self->sub_step = 1;
            break;
        }
        row = &lbl_805D4784[arg * ROW7_WORDS];
        ai_npc_variant_set(self, (u8)(s16)row[ROW7_ANIM]);
        ai_npc_motion_set(self, row[ROW7_MOTION], (s16)row[ROW7_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW7_PARAM2];
        }
        if (self->field_0x3F0 == 0) {
            fn_802D2B88(self, 1);
        }
        rotVecY(&shot, self->field_0x194);
        addVec3(&out, &self->vec_0x178, &shot);
        copyVec3(&self->vec_0x3E0, &out);
        self->field_0x3EC = (u16)self->field_0x194;
        self->field_0x3EE = 0;
        fn_800DCC24(self->sound_0x498, 0, 0);
        break;
    }
    case 1: {
        nw4r::math::VEC3 out;
        self->field_0x3EC += 0x1d1;
        self->field_0x194 = self->field_0x3EC;
        rotVecY(&shot, self->field_0x194);
        subVec3(&out, &self->vec_0x3E0, &shot);
        copyVec3(&self->vec_0x178, &out);
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (ai_npc_motion_end_ck(self) == 1) {
            u16* row = &lbl_805D4784[arg * ROW7_WORDS + ROW7_MOTION];
            self->state++;
            ai_npc_motion_set(self, row[0], (s16)row[1], 0);
        } else if (arg < 5) {
            if (ai_npc_frame_ck(self, 3, lbl_8079A670, lbl_8079A6A8) != 0) {
                self->field_0x3EE -= 0x889;
            }
        }
        self->field_0x194 += self->field_0x3EE;
        break;
    }
    case 2: {
        nw4r::math::VEC3 out;
        self->field_0x3EC += 0x1d1;
        self->field_0x194 = self->field_0x3EC;
        rotVecY(&shot, self->field_0x194);
        subVec3(&out, &self->vec_0x3E0, &shot);
        copyVec3(&self->vec_0x178, &out);
        if (ai_npc_motion_end_ck(self) == 1) {
            if (self->sub_step == 0) {
                fn_802D2ABC(self, 1, lbl_805D4784[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
            } else {
                u16* row;
                self->state = 1;
                self->sub_step--;
                row = &lbl_805D4784[arg * ROW7_WORDS + ROW7_MOTION];
                ai_npc_motion_set(self, row[0], (s16)row[1], 0);
                fn_800DCC24(self->sound_0x498, 0, 1);
            }
        } else if (arg < 5) {
            if (ai_npc_frame_ck(self, 3, lbl_8079A670, lbl_8079A6A8) != 0) {
                self->field_0x3EE += 0x889;
            }
        }
        self->field_0x194 += self->field_0x3EE;
        break;
    }
    }
}

/* The three-row ladder's first rung (rows 0x805D4810): arms the row's motion, then re-arms either the
 * row's own settle SE or the next rung depending on how far into the ladder the caller is. */
extern "C" void ai_npc_step37_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D4810[arg * ROW5_WORDS];
        ai_npc_variant_set(self, (u8)(s16)row[ROW5_ANIM]);
        ai_npc_motion_set(self, row[ROW5_MOTION], (s16)row[ROW5_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW5_VARIANT];
            fn_800DCC24(self->sound_0x498, 1, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 1, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0xd3, 0, 0);
                fn_800DCC24(self->sound_0x498, 1, 1);
            } else if (arg == 5) {
                ai_npc_motion_set(self, 0xea, 0, 0);
                fn_800DCC24(self->sound_0x498, 1, 1);
            } else {
                fn_802D2ABC(self, 1, lbl_805D4810[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D4810[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The second rung (rows 0x805D4874), the same ladder one row group on. */
extern "C" void ai_npc_step42_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D4874[arg * ROW5_WORDS];
        ai_npc_variant_set(self, (u8)(s16)row[ROW5_ANIM]);
        ai_npc_motion_set(self, row[ROW5_MOTION], (s16)row[ROW5_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW5_VARIANT];
            fn_800DCC24(self->sound_0x498, 2, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 2, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (ai_npc_motion_end_ck(self) == 1) {
            u16* se;
            self->state++;
            se = &lbl_805D4874[arg * ROW5_WORDS + ROW5_SE];
            if (arg == 0) {
                ai_npc_motion_set(self, 0xcb, 0, 0);
                fn_800DCC24(self->sound_0x498, 2, 1);
            } else if (arg == 5) {
                ai_npc_motion_set(self, 0xe2, 0, 0);
                fn_800DCC24(self->sound_0x498, 2, 1);
            } else {
                fn_802D2ABC(self, 1, *se, self->field_0x1EC);
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D4874[arg * ROW5_WORDS + ROW5_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The four-row ladder (rows 0x805D48D8): the widest of the three, one rung per motion of the volley. */
extern "C" void ai_npc_step47_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0: {
        u16* row;
        self->state++;
        row = &lbl_805D48D8[arg * ROW7_WORDS];
        ai_npc_variant_set(self, (u8)(s16)row[ROW7_ANIM]);
        ai_npc_motion_set(self, row[ROW7_MOTION], (s16)row[ROW7_PARAM], 0);
        if (arg != 4 && arg != 9) {
            self->field_0x3F0 = (u8)(s16)row[ROW7_VARIANT];
            fn_800DCC24(self->sound_0x498, 3, 0);
        } else {
            fn_800DCC24(self->sound_0x498, 3, 1);
        }
        break;
    }
    case 1:
        if (arg >= 5) {
            self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x600);
        }
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            u16* row = &lbl_805D48D8[arg * ROW7_WORDS + ROW7_MOTION2];
            ai_npc_motion_set(self, row[0], (s16)row[1], 0);
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            u16* se;
            self->state++;
            se = &lbl_805D48D8[arg * ROW7_WORDS + ROW7_SE];
            if (arg == 0) {
                ai_npc_motion_set(self, 0xce, 0, 0);
                fn_800DCC24(self->sound_0x498, 3, 1);
            } else if (arg == 5) {
                ai_npc_motion_set(self, 0xe5, 0, 0);
                fn_800DCC24(self->sound_0x498, 3, 1);
            } else {
                fn_802D2ABC(self, 1, *se, self->field_0x1EC);
            }
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0xcf, 0, 0);
            } else if (arg == 5) {
                ai_npc_motion_set(self, 0xe6, 0, 0);
            }
        }
        break;
    case 4:
        if (ai_npc_motion_end_ck(self) == 1) {
            fn_802D2ABC(self, 1, lbl_805D48D8[arg * ROW7_WORDS + ROW7_SE], self->field_0x1EC);
        }
        break;
    }
}

/* The aimed shot's release: raises the effect handle, waits for the arm window, then queries the ground
 * under the shot vector and leaves the record in the landing state it reports. */
extern "C" void ai_npc_step56_arm(struct _AINPC_W* self) {
    LandData land;
    f32 ground;
    em_land_data_init(&land);
    switch (self->state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_change(self, 0xd, 8, 0);
        fn_802D31FC(self);
        self->field_0x1DC = lbl_8079A6AC;
        break;
    case 1:
        fn_802D3210(self, &self->field_0x190);
        self->field_0x190 = (u16)fn_802D30F8(0x4000, self->field_0x190 & 0xffff, 0x800);
        fn_802D3CD4(self, 2);
        if (self->vec_0x178.y > lbl_8079A6B0) {
            self->vec_0x178.y = lbl_8079A6B0;
        }
        if (self->field_0x19C - lbl_8079A6B4 >= self->vec_0x178.y) {
            self->state++;
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x3a, 8, 0);
        }
        break;
    case 2:
        self->field_0x190 = (u16)fn_802D30F8(0, self->field_0x190 & 0xffff, 0x1000);
        if (ai_npc_motion_end_ck(self) == 1) {
            copyVec3(&self->vec_0x178, &self->vec_0x1B0);
            if (pl_ground_check_all_layers((struct _PLW*)self, &self->vec_0x178, &land, &ground, -5) == 1) {
                /* One `lfs` and two stores in retail: the traveller's distance and the
                 * height it settles at are the same value. */
                self->vec_0x178.y = self->field_0x19C = ground;
            }
            self->field_0x3C6 = 0;
            fn_802D3CCC(self, 1);
            if ((land.field_0x00 & 3) != 0) {
                fn_802D2ABC(self, 1, 0x37, 0);
            } else {
                fn_802D2ABC(self, 1, 5, 0);
            }
        } else {
            fn_802D3CD4(self, 2);
            fn_802D84D0(self, 2);
        }
        break;
    }
}

/* The guard/parry action: the arm chooses the shell's own state table by the arg class, and the settle
 * asks the shell to play the class's SE. */
#pragma opt_common_subs off
extern "C" void ai_npc_step57_arm(struct _AINPC_W* self, u8 arg) {
    u8 state = self->state;
    switch (state) {
    case 0:
        self->state++;
        switch (arg) {
        case 0:
        case 2:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x2e, 0, 0);
            break;
        case 1:
        case 3:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x52, 0, 0);
            break;
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            switch (arg) {
            case 0:
            case 2:
                ai_npc_base_state_set(self, 0);
                break;
            case 1:
            case 3:
                ai_npc_base_state_set(self, 2);
                break;
            }
        } else {
            switch (arg) {
            case 0:
            case 2:
                if (ai_npc_frame_ck(self, 0, lbl_8079A68C, lbl_8079A670) == 1) {
                    if (arg == 0) {
                        shell_set_func_ptr->request(self, 0xc, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->request(self, 0xe, shell_set_func_ptr);
                        ai_npc_talk(self, (s8)self->field_0x482, 0x1a, 0, 2);
                        self->field_0x451 = 0;
                    }
                }
                break;
            case 1:
            case 3:
                if (ai_npc_frame_ck(self, 0, lbl_8079A6B8, lbl_8079A670) == 1) {
                    if (arg == 1) {
                        shell_set_func_ptr->request(self, 0xd, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->request(self, 0xf, shell_set_func_ptr);
                        ai_npc_talk(self, (s8)self->field_0x482, 0x1a, 0, 2);
                        self->field_0x451 = 0;
                    }
                }
                break;
            }
        }
        break;
    }
}
#pragma opt_common_subs reset


/* The per-motion step dispatcher: it arms the motion and its scale once, then steps the armed
 * sub-state until the motion ends. */
extern "C" void ai_npc_step82_arm(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 vec;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    fn_802D29B8(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        ai_npc_variant_set(self, 0);
        if ((flag & 0xFF) == 0) {
            ai_npc_motion_set(self, 14, 4, 146);
        } else {
            ai_npc_motion_set(self, 14, 4, 186);
        }
        self->field_0x442 = 0;
        self->field_0x444 = 0;
        break;
    case 1:
        fn_802D3CD4(self, 1);
        if (ai_npc_motion_end_ck(self) == 1) {
            if ((flag & 0xFF) == 0) {
                fn_802D2ABC(self, 1, 83, 0);
            } else {
                fn_802D2ABC(self, 1, 85, 0);
            }
            break;
        }
        if ((flag & 0xFF) == 0) {
            if (ai_npc_frame_ck(self, 1, lbl_8079A6BC, lbl_8079A670) == 1) {
                fn_802D2B88(self, 2);
                f32 y = self->vec_0x178.y;
                f32 height = lbl_8079A6C0 + self->field_0x19C;

                if (y < height) {
                    self->vec_0x178.y = height;
                    fn_802D2ABC(self, 1, 83, 0);
                }
            }
        } else if ((flag & 0xFF) == 1) {
            if (ai_npc_frame_ck(self, 1, lbl_8079A6BC, lbl_8079A670) == 1) {
                fn_802D2B88(self, 2);
            }
        }
        break;
    }
}

/* The four-step variant: it arms a 248-frame motion and a 20-frame ramp, then re-aims the motion
 * through the `+0x005` frame table until the ramp is exhausted. */
extern "C" void ai_npc_step83_arm(struct _AINPC_W* self)
{
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    fn_802D29B8(self, 2);
    fn_802D3CD4(self, 1);
    switch (self->state) {
    case 0:
        self->state += 1;
        fn_802D2B88(self, 2);
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 14, 0, 248);
        fn_802D3CD4(self, 1);
        self->vec_0x024.x = lbl_8079A678;
        self->vec_0x024.y = lbl_8079A678;
        self->vec_0x024.z = lbl_8079A678;
        self->field_0x1AC = 20;
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        self->state += 1;
        fn_802D2B98(self, 2);
        self->field_0x002 = 0;
        copyVec3(&vec, setVec3(&offset, lbl_8079A670, lbl_8079A670, lbl_8079A6C4));
        rotVecY(&vec, self->field_0x194);
        addVec3To(&vec, &self->vec_0x178);
        vec.y = self->field_0x19C;
        AINPC_FX_SPAWN((struct _PLW*)self, 42, &vec, lbl_8079A678, 0);
        self->field_0x1AC = lbl_80792598[(s16)fn_802D66B8(self)];
        self->sub_step = 0;
        break;
    }
    case 2: {
        if (++self->field_0x442 > lbl_80792598[self->sub_step]) {
            u8 slot = ++self->field_0x444;

            if (slot == 3) {
                self->field_0x444 = 4;
            } else if (slot > 4) {
                self->field_0x444 = 4;
            }
            self->sub_step += 1;
        }
        if (self->field_0x442 > self->field_0x1AC) {
            self->field_0x1AC = 20;
            self->state += 1;
        }
        break;
    }
    case 3: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        fn_802D2ABC(self, 1, 87, 0);
        break;
    }
    }
}

/* The three-step variant: it copies the player's position into `vec_0x178` and arms a 14-frame
 * ramp, then re-aims the motion once the ramp hits its second-to-last frame. */
extern "C" void ai_npc_step85_arm(struct _AINPC_W* self)
{
    nw4r::math::VEC3 vec;
    LandData land;
    nw4r::math::VEC3 offset;
    f32 height;

    VEC3_ctor(&vec);
    em_land_data_init(&land);
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 54, 0, 0);
        copyVec3(&self->vec_0x178, &self->plw_0x16C->vec_0x03C);
        self->field_0x194 = self->plw_0x16C->field_0x058;
        self->field_0x1AC = 14;
        fn_802D2B88(self, 2);
        self->vec_0x024.x = lbl_8079A678;
        self->vec_0x024.y = lbl_8079A678;
        self->vec_0x024.z = lbl_8079A678;
        fn_802D3CCC(self, 1);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames <= 0) {
            self->state += 1;
            if (fn_8027DCE0(self->plw_0x16C, 0) == 1) {
                fn_802D2B98(self, 2);
            }
            self->field_0x002 = 0;
        } else if (frames == 2) {
            copyVec3(&vec, setVec3(&offset, 0.0f, 0.0f, 143.0f));
            rotVecY(&vec, self->field_0x194);
            addVec3To(&vec, &self->vec_0x178);
            if (fn_8027DCE0(self->plw_0x16C, 0) == 1) {
                AINPC_FX_SPAWN((struct _PLW*)self, 42, &vec, lbl_8079A678, 0);
            }
        }
        break;
    }
    case 2:
        copyVec3(&self->vec_0x178, &self->plw_0x16C->vec_0x03C);
        self->field_0x194 = self->plw_0x16C->field_0x058;
        if (fn_8027DCE0(self->plw_0x16C, 0) == 0) {
            if (pl_ground_check_all_layers((struct _PLW*)self, &self->vec_0x178, &land, &height, -5) == 1) {
                self->vec_0x178.y = height;
            }
            fn_802D2ABC(self, 1, 86, self->field_0x1EC);
        }
        break;
    }
}

/* The two-step variant: it arms a 30-frame wait and a 0.75 scale, then fires the motion in the
 * direction the player's position implies. */
extern "C" void ai_npc_step86_arm(struct _AINPC_W* self, u8 flag)
{
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 front;
    nw4r::math::VEC3 back;

    VEC3_ctor(&vec);
    fn_802D3A2C(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 30;
        self->field_0x002 = 1;
        self->vec_0x024.x = lbl_8079A674;
        self->vec_0x024.y = lbl_8079A674;
        self->vec_0x024.z = lbl_8079A674;
        if ((flag & 0xFF) == 0) {
            copyVec3(&vec, setVec3(&front, 0.0f, 0.0f, 143.0f));
        } else {
            self->vec_0x178.y = self->field_0x19C;
            copyVec3(&vec, setVec3(&back, lbl_8079A670, lbl_8079A670, lbl_8079A6C4));
        }
        if (ai_npc_flag_ck(self, 2) != 0) {
            break;
        }
        fn_802D2B88(self, 2);
        rotVecY(&vec, self->field_0x194);
        addVec3To(&vec, &self->vec_0x178);
        AINPC_FX_SPAWN((struct _PLW*)self, 42, &vec, lbl_8079A678, 0);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0) {
            break;
        }
        self->state += 1;
        ai_npc_motion_set(self, 16, 0, 0);
        fn_802D2B98(self, 2);
        break;
    }
    case 2:
        if (ai_npc_motion_end_ck(self) != 1) {
            break;
        }
        if ((flag & 0xFF) == 0) {
            ai_npc_base_state_set(self, 0);
        } else {
            fn_802D6690(self);
            fn_802D40EC(self, 29, -1);
            switch (self->field_0x444) {
            case 0:
            case 1:
                fn_802D2ABC(self, 1, 23, 0);
                fn_802D40EC(self, 27, 1);
                break;
            case 2:
            case 3:
                fn_802D40EC(self, 26, 1);
                fn_802D2ABC(self, 1, 23, 0);
                break;
            case 4:
                fn_802D40EC(self, 28, 1);
                fn_802D2ABC(self, 1, 24, 0);
                break;
            }
        }
        self->field_0x440 = 0;
        break;
    }
}

/* The two-state projectile-launch dispatcher: it arms a 6-frame shot or a 57-frame hold depending on
 * the tier the record carries. */
extern "C" void ai_npc_step88_arm(struct _AINPC_W* self, u8 flag)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        if ((flag & 0xFF) == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 6, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 57, 4, 0);
        }
        self->field_0x1AC = 600;
        if ((u32)(self->field_0x420 - 3) <= 1) {
            break;
        }
        ai_npc_talk(self, (s8)self->field_0x482, 20, self->field_0x462, 2);
        break;
    case 1: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames > 0 && self->field_0x461 != 0) {
            break;
        }
        if ((flag & 0xFF) == 0) {
            ai_npc_base_state_set(self, 0);
        } else {
            ai_npc_base_state_set(self, 2);
        }
        self->field_0x461 = 0;
        break;
    }
    }
}

/* The four-state charge dispatcher: it arms a 300-frame charge, releases it into the bite/stomp
 * motion the `+0x484` tier selects, then waits for the motion to end. */
extern "C" void ai_npc_step90_arm(struct _AINPC_W* self)
{
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 14, 4, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->field_0x1AC = 300;
            fn_802D2B88(self, 2);
            self->state += 1;
        }
        break;
    case 2: {
        s32 frames = self->field_0x1AC - 1;

        self->field_0x1AC = frames;
        if (frames >= 0) {
            break;
        }
        fn_802D2B98(self, 2);
        ai_npc_motion_set(self, 16, 0, 0);
        fn_802D9400(self);
        self->field_0x485 = 0;
        fn_802D3684(self);
        if (self->field_0x484 < 3) {
            ai_npc_talk(self, 9, 1, 0, 2);
            self->field_0x486 = 900;
        } else {
            ai_npc_talk(self, 9, 2, 0, 2);
        }
        fn_802D9A44(self);
        self->state += 1;
        break;
    }
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* The two-state tail-swipe dispatcher: it arms a 6-frame wind-up, then fires the swipe through the
 * `+0x1F4` motion as the countdown crosses frame 4. */
extern "C" void ai_npc_step93_arm(struct _AINPC_W* self)
{
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 6;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 29, 6, 0);
        break;
    case 1: {
        s32 frames;

        if (ai_npc_motion_end_ck(self) != 1) {
            break;
        }
        frames = self->field_0x1AC - 1;
        self->field_0x1AC = frames;
        if (frames == 4) {
            fn_802D35B4(self, (s16)self->field_0x1F4);
            fn_802D3CFC(self, 1);
            fn_80114CC8(self, 0);
            break;
        }
        if (frames <= 0) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
    }
}

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802C6908 (0x2D8): runs the arm of the record's motion step (+0x172); steps 3, 7 and 28 have none. */
void ai_npc_step_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_step0_arm(self);
        break;
    case 1:
        ai_npc_step1_arm(self, 0);
        break;
    case 2:
        ai_npc_step1_arm(self, 1);
        break;
    case 3:
        break;
    case 4:
        ai_npc_step4_arm(self, 0);
        break;
    case 5:
        ai_npc_step5_arm(self, 0);
        break;
    case 6:
        ai_npc_step6_arm(self, 0);
        break;
    case 7:
        break;
    case 8:
        ai_npc_step8_arm(self, 0);
        break;
    case 9:
        ai_npc_step9_arm(self);
        break;
    case 10:
        ai_npc_step10_arm(self, 0);
        break;
    case 11:
        ai_npc_step10_arm(self, 1);
        break;
    case 12:
        ai_npc_step12_arm(self, 0);
        break;
    case 13:
        ai_npc_step13_arm(self, 0);
        break;
    case 14:
        ai_npc_step4_arm(self, 2);
        break;
    case 15:
        ai_npc_step8_arm(self, 1);
        break;
    case 16:
        ai_npc_step6_arm(self, 1);
        break;
    case 17:
        ai_npc_step12_arm(self, 1);
        break;
    case 18:
        ai_npc_step18_arm(self);
        break;
    case 19:
        ai_npc_step19_arm(self);
        break;
    case 20:
        ai_npc_step20_arm(self);
        break;
    case 21:
        ai_npc_step1_arm(self, 2);
        break;
    case 22:
        ai_npc_step10_arm(self, 2);
        break;
    case 23:
        ai_npc_step23_arm(self, 0);
        break;
    case 24:
        ai_npc_step24_arm(self, 0);
        break;
    case 25:
        ai_npc_step25_arm(self, 0);
        break;
    case 26:
        ai_npc_step4_arm(self, 1);
        break;
    case 27:
        ai_npc_step27_arm(self, 0);
        break;
    case 28:
        break;
    case 29:
        ai_npc_step29_arm(self, 0);
        break;
    case 30:
        ai_npc_step29_arm(self, 1);
        break;
    case 31:
        ai_npc_step31_arm(self, 0);
        break;
    case 32:
        ai_npc_step32_arm(self, 0);
        break;
    case 33:
        ai_npc_step32_arm(self, 1);
        break;
    case 34:
        ai_npc_step32_arm(self, 2);
        break;
    case 35:
        ai_npc_step32_arm(self, 3);
        break;
    case 36:
        ai_npc_step32_arm(self, 4);
        break;
    case 37:
        ai_npc_step37_arm(self, 0);
        break;
    case 38:
        ai_npc_step37_arm(self, 1);
        break;
    case 39:
        ai_npc_step37_arm(self, 2);
        break;
    case 40:
        ai_npc_step37_arm(self, 3);
        break;
    case 41:
        ai_npc_step37_arm(self, 4);
        break;
    case 42:
        ai_npc_step42_arm(self, 0);
        break;
    case 43:
        ai_npc_step42_arm(self, 1);
        break;
    case 44:
        ai_npc_step42_arm(self, 2);
        break;
    case 45:
        ai_npc_step42_arm(self, 3);
        break;
    case 46:
        ai_npc_step42_arm(self, 4);
        break;
    case 47:
        ai_npc_step47_arm(self, 0);
        break;
    case 48:
        ai_npc_step47_arm(self, 1);
        break;
    case 49:
        ai_npc_step47_arm(self, 2);
        break;
    case 50:
        ai_npc_step47_arm(self, 3);
        break;
    case 51:
        ai_npc_step47_arm(self, 4);
        break;
    case 52:
        ai_npc_step4_arm(self, 3);
        break;
    case 53:
        ai_npc_step25_arm(self, 1);
        break;
    case 54:
        ai_npc_step23_arm(self, 1);
        break;
    case 55:
        ai_npc_step5_arm(self, 1);
        break;
    case 56:
        ai_npc_step56_arm(self);
        break;
    case 57:
        ai_npc_step57_arm(self, 0);
        break;
    case 58:
        ai_npc_step57_arm(self, 1);
        break;
    case 59:
        ai_npc_step31_arm(self, 1);
        break;
    case 60:
        ai_npc_step13_arm(self, 1);
        break;
    case 61:
        ai_npc_step32_arm(self, 5);
        break;
    case 62:
        ai_npc_step32_arm(self, 6);
        break;
    case 63:
        ai_npc_step32_arm(self, 7);
        break;
    case 64:
        ai_npc_step32_arm(self, 8);
        break;
    case 65:
        ai_npc_step32_arm(self, 9);
        break;
    case 66:
        ai_npc_step37_arm(self, 5);
        break;
    case 67:
        ai_npc_step37_arm(self, 6);
        break;
    case 68:
        ai_npc_step37_arm(self, 7);
        break;
    case 69:
        ai_npc_step37_arm(self, 8);
        break;
    case 70:
        ai_npc_step37_arm(self, 9);
        break;
    case 71:
        ai_npc_step42_arm(self, 5);
        break;
    case 72:
        ai_npc_step42_arm(self, 6);
        break;
    case 73:
        ai_npc_step42_arm(self, 7);
        break;
    case 74:
        ai_npc_step42_arm(self, 8);
        break;
    case 75:
        ai_npc_step42_arm(self, 9);
        break;
    case 76:
        ai_npc_step47_arm(self, 5);
        break;
    case 77:
        ai_npc_step47_arm(self, 6);
        break;
    case 78:
        ai_npc_step47_arm(self, 7);
        break;
    case 79:
        ai_npc_step47_arm(self, 8);
        break;
    case 80:
        ai_npc_step47_arm(self, 9);
        break;
    case 81:
        ai_npc_step24_arm(self, 1);
        break;
    case 82:
        ai_npc_step82_arm(self, 0);
        break;
    case 83:
        ai_npc_step83_arm(self);
        break;
    case 84:
        ai_npc_step82_arm(self, 1);
        break;
    case 85:
        ai_npc_step85_arm(self);
        break;
    case 86:
        ai_npc_step86_arm(self, 0);
        break;
    case 87:
        ai_npc_step86_arm(self, 1);
        break;
    case 88:
        ai_npc_step88_arm(self, 0);
        break;
    case 89:
        ai_npc_step88_arm(self, 1);
        break;
    case 90:
        ai_npc_step90_arm(self);
        break;
    case 91:
        ai_npc_step57_arm(self, 2);
        break;
    case 92:
        ai_npc_step57_arm(self, 3);
        break;
    case 93:
        ai_npc_step93_arm(self);
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

/* The two-state approach dispatcher: it picks the near/far ring the flag names, arms a 450-frame
 * approach, and gives up when the target leaves the ring or the timer runs out. */
#pragma opt_common_subs off
extern "C" void ai_npc_m2_step0_arm(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    f32 near_limit;
    f32 far_limit;
    s32 armed;

    VEC3_ctor(&start);
    near_limit = lbl_8079A670;
    far_limit = near_limit;
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        switch (flag & 0xFF) {
        case 0:
        case 1:
            ai_npc_motion_change(self, 8, 4, 0);
            break;
        case 2:
            ai_npc_motion_change(self, 10, 2, 0);
            fn_802D29CC(self, lbl_8079A680);
            break;
        }
        ai_npc_variant_set(self, 0);
        break;
    case 1: {
        s32 limit;
        f32 distance;

        switch (flag & 0xFF) {
        case 0:
            near_limit = lbl_8079A6CC;
            far_limit = lbl_8079A6D0;
            break;
        case 1:
            near_limit = lbl_8079A6D4;
            far_limit = lbl_8079A6D8;
            break;
        case 2:
            near_limit = lbl_8079A68C;
            far_limit = lbl_8079A694;
            armed = 1;
            break;
        }
        limit = (flag & 0xFF) == 2 ? 10 : 90;
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if (fn_802D3B34(self, (s32)limit, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= near_limit * near_limit) {
            ai_npc_base_state_set(self, 0);
        } else if (distance >= far_limit * far_limit) {
            ai_npc_base_state_set(self, 0);
        } else if (--self->field_0x1AC < 0) {
            ai_npc_base_state_set(self, 0);
            break;
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, 2048);
        break;
    }
    }
}
#pragma opt_common_subs reset

/* The two-state guard dispatcher: it waits for the target to close inside the `+0x1BC` ring, then
 * aims the motion at it and reports the aim window through the `+0x194` angle selector. */
#pragma opt_common_subs off
extern "C" void ai_npc_m2_step2_arm(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    f32 limit;
    s32 armed;
    s32 limit2;

    VEC3_ctor(&start);
    limit = lbl_8079A670;
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_change(self, 11, 2, 0);
        break;
    case 1: {
        f32 distance;

        if ((flag & 0xFF) == 0) {
            limit = lbl_8079A6DC;
        }
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if ((flag & 0xFF) == 2) {
            limit2 = 20;
            armed = 1;
        } else {
            limit2 = 90;
        }
        if (fn_802D3B34(self, limit2, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            ai_npc_base_state_set(self, 0);
            break;
        } else if ((flag & 0xFF) == 0) {
            if (distance >= limit * limit) {
                ai_npc_base_state_set(self, 0);
                break;
            }
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194,
                                        distance > lbl_8079A6E0 ? 1536 : 2560);
        break;
    }
    }
}
#pragma opt_common_subs reset

/* The two-state guard dispatcher: it arms one of two motions on the flag, then steers the motion
 * while the target stays inside the 2048-unit aim window and the 90-frame guard lasts. */
extern "C" void ai_npc_m2_step5_arm(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;

    VEC3_ctor(&start);
    fn_802D29B8(self, 2);
    switch (self->state) {
    case 0:
        self->state += 1;
        switch ((u8)flag) {
        case 0:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_change(self, 8, 4, 0);
            break;
        case 1:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_change(self, 12, 4, 0);
            fn_802D31FC(self);
            self->field_0x1DC = lbl_8079A6E4;
            break;
        }
        self->field_0x1AC = 90;
        break;
    case 1:
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        if ((flag & 0xFF) == 1) {
            f32 distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);

            self->field_0x194 =
                fn_802D30F8((u16)ang_b, (u16)self->field_0x194, distance > lbl_8079A6E0 ? 2048 : 6656);
            fn_802D3210(self, &self->field_0x190);
            self->field_0x190 = fn_802D30F8((u16)ang_a, (u16)self->field_0x190, 1536);
        } else {
            self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, 2048);
        }
        if (self->field_0x194 - ang_b < 2048) {
            ai_npc_base_state_set(self, self->variant);
            break;
        }
        if (--self->field_0x1AC <= 0) {
            ai_npc_base_state_set(self, self->variant);
        }
        break;
    }
}

/* The two-state guard dispatcher: it arms a 1.3-scaled motion and a 450-frame guard, then gives up
 * as soon as the target closes inside the `+0x1BC` ring. */
extern "C" void ai_npc_m2_step7_arm(struct _AINPC_W* self, u32 flag)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;
    s32 armed;
    s32 limit;

    VEC3_ctor(&start);
    armed = 0;
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 450;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_change(self, 11, 2, 0);
        fn_802D29CC(self, lbl_8079A6E8);
        break;
    case 1: {
        f32 distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);

        if ((flag & 0xFF) == 1) {
            limit = 20;
            armed = 1;
        } else {
            limit = 90;
        }
        if (fn_802D3B34(self, limit, armed) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            ai_npc_base_state_set(self, 0);
            break;
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194,
                                        distance > lbl_8079A6E0 ? 2048 : 4096);
        break;
    }
    }
}

/* The two-state dying/stagger dispatcher: it rolls one of two motions, then folds the aim every
 * frame until the target closes in or the 300-frame guard runs out. */
extern "C" void ai_npc_m2_step9_arm(struct _AINPC_W* self)
{
    nw4r::math::VEC3 start;
    nw4r::math::VEC3 diff;
    u32 ang_a;
    u32 ang_b;

    VEC3_ctor(&start);
    switch (self->state) {
    case 0:
        self->state += 1;
        self->field_0x1AC = 300;
        ai_npc_variant_set(self, 0);
        self->field_0x194 -= 16384;
        ai_npc_motion_change(self, 200, 6, 0);
        fn_802D31FC(self);
        self->field_0x1D4 = lbl_8079A6EC;
        if (ran_suu(1) & 1) {
            ai_npc_talk(self, (s8)self->field_0x482, 6, 0, 2);
        } else {
            ai_npc_talk(self, (s8)self->field_0x482, 7, 0, 2);
        }
        break;
    case 1: {
        f32 distance;
        u16 selector;

        fn_802D3210(self, &self->field_0x190);
        distance = calcDistanceSqXZ(&self->vec_0x178, &self->vec_0x1B0);
        if (fn_802D3B34(self, 30, 1) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (distance <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 0);
            break;
        } else if (--self->field_0x1AC < 0) {
            ai_npc_base_state_set(self, 0);
            break;
        }
        subVec3(&diff, &self->vec_0x1B0, &self->vec_0x178);
        copyVec3(&start, &diff);
        calcVecAngXY(&start, &ang_a, &ang_b);
        ang_b -= 16384;
        if (distance > lbl_8079A6E0) {
            selector = 1024;
        } else if (distance > lbl_8079A6F0) {
            selector = 2048;
        } else {
            selector = 4096;
        }
        self->field_0x194 = fn_802D30F8((u16)ang_b, (u16)self->field_0x194, selector);
        break;
    }
    }
}

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802C75F4 (0x290): motion 2 steps 10, 14 and 17 - runs toward the move target, steering by its bearing,
 * until it is in range, the attempt times out or (arg 0) it is too far. */
void ai_npc_m2_step10_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    f32 dist;
    f32 limit;
    s32 window;
    s32 armed;
    u16 turn;
    u8 state;

    VEC3_ctor(&dir);
    limit = 0.0f;
    armed = 0;
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = 600;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_change(self, 12, 8, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 10.0f;
        if (self->field_0x420 == 2) {
            self->pos_0x1D4.z *= 1.2f;
            fn_802D29CC(self, 1.2f);
        }
        break;
    case 1:
        switch (arg) {
        case 0:
            limit = 700.0f;
            window = 90;
            break;
        case 1:
            window = 30;
            armed = 1;
            break;
        case 2:
            window = 90;
            break;
        }
        fn_802D3210(self, (u32*)&self->vec_0x190);
        subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
        copyVec3(&dir, &diff);
        ::calcVecAngXY(&dir, &pitch, &yaw);
        if (::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0) > 10000.0f) {
            turn = 0xA00;
        } else {
            turn = 0x1200;
        }
        if (self->field_0x420 == 2) {
            turn += 0x400;
        }
        self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, turn);
        self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x400);
        dist = vec3_dist_sq(&self->pos_0x178, &self->pos_0x1B0);
        if (fn_802D3B34(self, window, armed) == 1) {
            fn_802D2ABC(self, 1, 0x38, 0);
        } else if (dist <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 2);
        } else if (arg == 0 && dist >= limit * limit) {
            ai_npc_base_state_set(self, 2);
        } else if (--self->timer_0x1AC < 0) {
            ai_npc_base_state_set(self, 2);
        }
        break;
    }
}

/* 0x802C7884 (0x240): motion 2 steps 11, 15 and 18 - the faster run toward the move target. */
void ai_npc_m2_step11_arm(_AINPC_W* self, u32 arg)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    f32 dist;
    s32 armed;
    u16 turn;
    u8 state;

    VEC3_ctor(&dir);
    armed = 0;
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = 600;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_change(self, 13, 8, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 15.0f;
        if (self->field_0x420 == 2) {
            self->pos_0x1D4.z *= 1.2f;
            fn_802D29CC(self, 1.2f);
        }
        break;
    case 1:
        switch ((u8)arg) {
        case 0:
            arg = 90;
            break;
        case 1:
            arg = 30;
            armed = 1;
            break;
        case 2:
            arg = 90;
            break;
        }
        fn_802D3210(self, (u32*)&self->vec_0x190);
        subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
        copyVec3(&dir, &diff);
        ::calcVecAngXY(&dir, &pitch, &yaw);
        if (::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0) > 10000.0f) {
            turn = 0xA00;
        } else {
            turn = 0x1200;
        }
        if (self->field_0x420 == 2) {
            turn += 0x400;
        }
        self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, turn);
        self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x400);
        dist = vec3_dist_sq(&self->pos_0x178, &self->pos_0x1B0);
        if (::fn_802D3B34((::_AINPC_W*)self, arg, armed) == 1) {
            fn_802D2ABC(self, 1, 0x38, 0);
        } else if (dist <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 2);
        } else if (--self->timer_0x1AC < 0) {
            ai_npc_base_state_set(self, 2);
        }
        break;
    }
}

/* 0x802C7AC4 (0x80): motion 2 step 12 - plays motion 0x2F once and returns to the alternate base state. */
void ai_npc_m2_step12_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_motion_set(self, 0x2F, 2, 0);
        ai_npc_variant_set(self, 2);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 2);
        }
        break;
    }
}

/* 0x802C7B44 (0x88): motion 2 step 13 - plays motion 0x30 level (pitch cleared) and returns to the base
 * state. */
void ai_npc_m2_step13_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_motion_set(self, 0x30, 2, 0);
        ai_npc_variant_set(self, 2);
        self->vec_0x190.x = 0;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C7BCC (0x27C): motion 2 step 19 - circles the move target sideways (a quarter turn off its bearing) with a
 * voice line, until it is in range or the attempt times out. */
void ai_npc_m2_step19_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 dir;
    _CP_VECTOR rot;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    f32 dist;
    u16 turn;
    u8 state;

    VEC3_ctor(&dir);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = 300;
        ai_npc_variant_set(self, 2);
        self->vec_0x190.y -= 0x4000;
        ai_npc_motion_change(self, 0xDD, 8, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 12.0f;
        if (self->field_0x420 == 2) {
            self->pos_0x1D4.z *= 1.2f;
            fn_802D29CC(self, 1.2f);
        }
        if (::ran_suu(1) & 1) {
            ai_npc_talk(self, (s8)self->field_0x482, 6, 0, 2);
        } else {
            ai_npc_talk(self, (s8)self->field_0x482, 7, 0, 2);
        }
        break;
    case 1:
        eft_rot_vec_copy(&rot, &self->vec_0x190);
        rot.y += 0x4000;
        fn_802D3210(self, (u32*)&rot);
        dist = ::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0);
        if (fn_802D3B34(self, 30, 1) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (dist <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 2);
            break;
        } else if (--self->timer_0x1AC < 0) {
            ai_npc_base_state_set(self, 2);
            break;
        }
        subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
        copyVec3(&dir, &diff);
        ::calcVecAngXY(&dir, &pitch, &yaw);
        yaw -= 0x4000;
        if (dist > 10000.0f) {
            turn = 0x800;
        } else if (dist > 2500.0f) {
            turn = 0xA00;
        } else {
            turn = 0x1200;
        }
        if (self->field_0x420 == 2) {
            turn += 0x400;
        }
        self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, turn);
        self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x400);
        break;
    }
}

/* 0x802C7E48 (0x174): motion 2 step 20 - walks toward the move target, turning slowly, then hands over to the
 * arrival state. */
void ai_npc_m2_step20_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    f32 dist;
    u8 state;

    VEC3_ctor(&dir);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = 600;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_change(self, 0x1A, 2, 0);
        break;
    case 1:
        dist = ::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0);
        if (fn_802D3B34(self, 150, 1) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (dist <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2ABC(self, 1, 0x1D, 0);
            break;
        } else if (--self->timer_0x1AC < 0) {
            fn_802D2ABC(self, 1, 0x1D, 0);
            break;
        }
        subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
        copyVec3(&dir, &diff);
        ::calcVecAngXY(&dir, &pitch, &yaw);
        self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, 0x200);
        break;
    }
}

/* 0x802C7FBC (0x208): motion 2 step 21 - weaves toward the move target (ten frames left, ten right) at speed
 * 1.4 until it is in range or the attempt times out. */
void ai_npc_m2_step21_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    f32 dist;
    u8 state;

    VEC3_ctor(&dir);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->sub_step = 0;
        self->timer_0x1AC = 600;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_change(self, 13, 8, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 7.0f;
        fn_802D29CC(self, 1.4f);
        break;
    case 1:
        fn_802D3210(self, (u32*)&self->vec_0x190);
        subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
        copyVec3(&dir, &diff);
        ::calcVecAngXY(&dir, &pitch, &yaw);
        if (++self->sub_step < 10) {
            yaw += 0x800;
        } else if (self->sub_step < 20) {
            yaw -= 0x800;
        } else {
            self->sub_step = 0;
        }
        self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, 0x400);
        self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x200);
        dist = vec3_dist_sq(&self->pos_0x178, &self->pos_0x1B0);
        if (fn_802D3B34(self, 150, 1) == 1) {
            fn_802D2ABC(self, 1, 0x38, 0);
        } else if (dist <= self->field_0x1BC * self->field_0x1BC) {
            fn_802D2ABC(self, 1, 0x1E, 0);
        } else if (--self->timer_0x1AC < 0) {
            fn_802D2ABC(self, 1, 0x1E, 0);
        }
        break;
    }
}

/* 0x802C81C4 (0xC0): runs the arm of the record's motion-2 step. */
void ai_npc_motion2_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_m2_step0_arm(self, 0);
        break;
    case 1:
        ai_npc_m2_step0_arm(self, 1);
        break;
    case 2:
        ai_npc_m2_step2_arm(self, 0);
        break;
    case 3:
        ai_npc_m2_step2_arm(self, 1);
        break;
    case 4:
        ai_npc_m2_step2_arm(self, 2);
        break;
    case 5:
        ai_npc_m2_step5_arm(self, 0);
        break;
    case 6:
        ai_npc_m2_step0_arm(self, 2);
        break;
    case 7:
        ai_npc_m2_step7_arm(self, 0);
        break;
    case 8:
        ai_npc_m2_step7_arm(self, 1);
        break;
    case 9:
        ai_npc_m2_step9_arm(self);
        break;
    case 10:
        ai_npc_m2_step10_arm(self, 0);
        break;
    case 11:
        ai_npc_m2_step11_arm(self, 0);
        break;
    case 12:
        ai_npc_m2_step12_arm(self);
        break;
    case 13:
        ai_npc_m2_step13_arm(self);
        break;
    case 14:
        ai_npc_m2_step10_arm(self, 1);
        break;
    case 15:
        ai_npc_m2_step11_arm(self, 1);
        break;
    case 16:
        ai_npc_m2_step5_arm(self, 1);
        break;
    case 17:
        ai_npc_m2_step10_arm(self, 2);
        break;
    case 18:
        ai_npc_m2_step11_arm(self, 2);
        break;
    case 19:
        ai_npc_m2_step19_arm(self);
        break;
    case 20:
        ai_npc_m2_step20_arm(self);
        break;
    case 21:
        ai_npc_m2_step21_arm(self);
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802C8284 (0x90): breaks an attack off into the motion-3 recoil of the record's variant unless the attack
 * holds (`fn_802D3F1C`). */
void ai_npc_attack_hold_ck(_AINPC_W* self)
{
    if (fn_802D3F1C(self) == 0) {
        switch (self->variant) {
        default:
            fn_802D2ABC(self, 3, 0xA, 0);
            break;
        case 2:
            fn_802D2ABC(self, 3, 0xB, 0);
            break;
        case 1:
            fn_802D2ABC(self, 3, 0xC, 0);
            break;
        }
    }
}

/* 0x802C8314 (0x128): motion 3 steps 0 and 9 - the basic swing: plays the variant's motion with hit row 1 and
 * returns to the base state, breaking off when the attack is interrupted. */
void ai_npc_m3_step0_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 pos;
    u8 state;

    VEC3_ctor(&pos);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        switch (arg) {
        case 0:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x1E, 0, 0);
            break;
        case 1:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x44, 0, 0);
            if (self->field_0x420 == 2) {
                fn_802D29CC(self, 1.2f);
            }
            break;
        }
        fn_802D2F7C(self, 0, 1);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    }
}

/* 0x802C843C (0x23C): motion 3 step 1 - the jumping slam: winds up twice, leaps (hit row 3), turns the fall into
 * the slam below 160 above the ground and lands with hit row 4. */
void ai_npc_m3_step1_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_motion_set(self, 0x1F, 2, 0);
        self->state = 2;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_motion_set(self, 0x1F, 2, 0);
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_variant_set(self, 1);
            ai_npc_motion_set(self, 0x20, 0, 0);
            fn_802D31FC(self);
            self->pos_0x1D4.y = 30.0f;
            self->pos_0x1E0.y = -2.5f;
            self->pos_0x1D4.z = 6.0f;
            fn_802D2F7C(self, 0, 3);
        }
        break;
    case 3:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (self->pos_0x178.y < self->pos_0x184.y && self->pos_0x178.y < 160.0f + self->field_0x19C) {
            self->state++;
            ai_npc_motion_set(self, 0x21, 0, 0);
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    case 4:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (self->pos_0x178.y < 80.0f + self->field_0x19C) {
            ai_npc_variant_set(self, 0);
            self->pos_0x178.y = self->field_0x19C;
            fn_802D31FC(self);
            self->state++;
            ai_npc_motion_set(self, 0x22, 0, 0);
            fn_802D2F7C(self, 0, 4);
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    case 5:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_variant_set(self, 0);
            self->pos_0x178.y = self->field_0x19C;
            ai_npc_base_state_set(self, 0);
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    }
}

/* 0x802C8678 (0x17C): motion 3 steps 2..4 - the leap attack: crouches to frame 8, leaps with the row of
 * `ai_npc_leap_tbl` the step selects (hit row 2) and lands. */
void ai_npc_m3_step2_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_motion_set(self, 0x23, 0, 0);
        break;
    case 1:
        if (ai_npc_frame_ck(self, 1, 8.0f, 0.0f) == 1) {
            self->state++;
            ai_npc_variant_set(self, 1);
            fn_802D31FC(self);
            self->pos_0x1D4.y = ai_npc_leap_tbl[arg * 3];
            self->pos_0x1E0.y = ai_npc_leap_tbl[arg * 3 + 1];
            self->pos_0x1D4.z = ai_npc_leap_tbl[arg * 3 + 2];
            fn_802D2F7C(self, 0, 2);
        }
        break;
    case 2:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (self->pos_0x178.y <= self->field_0x19C) {
            self->state++;
            ai_npc_variant_set(self, 0);
            self->pos_0x178.y = self->field_0x19C;
            ai_npc_motion_set(self, 0x24, 0, 0);
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C87F4 (0x2B0): motion 3 steps 5 and 17 - the item throw: from frame 64 to 194, every 20 frames, throws the
 * quest's projectile (formation 3 a flash from joint 16, formation 4 a plain one) in a random direction. */
void ai_npc_m3_step5_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    u16 angle;
    u16 spin;
    u8 state;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0xDC, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0xEB, 2, 0);
        }
        self->timer_0x1AC = 0;
        break;
    case 1:
        self->timer_0x1AC++;
        if (ai_npc_frame_ck(self, 3, 64.0f, 194.0f) == 1 && self->timer_0x1AC > 20) {
            angle = (::ran_suu(1) & 0x7FFF) - 0x4000;
            switch (self->field_0x420) {
            case 3:
                setVector3(&pos, 0.0f, 65.0f, 0.0f);
                mhchar_joint_mtx_get(&self->model, 16, &mtx);
                ::mulVecMat(&pos, &mtx);
                pos.x += mtx.m[0][3];
                pos.y += mtx.m[1][3];
                pos.z += mtx.m[2][3];
                ::eft019_set_subtype(0x84, 5, &pos, self->area, &self->vec_0x190, 0, 1.5f);
                spin = ::ran_suu(0) & 0x4000;
                shell_set_func_ptr->method_0x90((::_AINPC_W*)self, 1, ::ran_suu(0) % 3, spin, angle,
                                                shell_set_func_ptr);
                break;
            case 4:
                spin = ::ran_suu(0);
                shell_set_func_ptr->method_0x90((::_AINPC_W*)self, 0, ::ran_suu(0) % 3, spin, angle,
                                                shell_set_func_ptr);
                break;
            }
            self->timer_0x1AC = 0;
        }
        if (ai_npc_motion_end_ck(self) == 1) {
            self->timer_0x1AC = 0;
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802C8AA4 (0x3AC): motion 3 steps 6, 15, 21 and 22 - the aimed shot: winds up, turns toward the move target
 * and fires the quest's projectile at frame 94 (arg 2/3: the shell job at the target). */
#pragma opt_common_subs off
void ai_npc_m3_step6_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    u8 state;

    VEC3_ctor(&dir);
    VEC3_ctor(&offset);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        switch (arg) {
        case 0:
        case 2:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x29, 2, 0);
            break;
        case 1:
        case 3:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x4D, 2, 0);
            if (self->field_0x420 == 2) {
                fn_802D29CC(self, 1.2f);
            }
            break;
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            switch (arg) {
            case 0:
            case 2:
                ai_npc_motion_set(self, 0x2A, 0, 0);
                break;
            case 1:
            case 3:
                ai_npc_motion_set(self, 0x4E, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            switch (arg) {
            case 0:
            case 2:
                ai_npc_base_state_set(self, 0);
                break;
            case 1:
            case 3:
                ai_npc_base_state_set(self, 2);
                break;
            }
        } else if (ai_npc_frame_ck(self, 2, 94.0f, 0.0f) == 1) {
            subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
            copyVec3(&dir, &diff);
            ::calcVecAngXY(&dir, &pitch, &yaw);
            self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, 0x200);
            switch (arg) {
            case 1:
            case 3:
                self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x200);
                if (ai_npc_frame_ck(self, 0, 88.0f, 0.0f) == 1) {
                    setVector3(&offset, 0.0f, 0.0f, 0.0f);
                    eft013_npc_place(self, &offset, 8, 0.5f, 0x22);
                }
                break;
            }
        } else if (ai_npc_frame_ck(self, 0, 94.0f, 0.0f) == 1) {
            switch (arg) {
            case 0:
            case 1:
                switch (self->field_0x420) {
                case 5:
                    shell_set_func_ptr->method_0x04((::_AINPC_W*)self, 13, 2, 0, 0, shell_set_func_ptr);
                    break;
                case 2:
                    shell_set_func_ptr->method_0x90((::_AINPC_W*)self, 3, 2, 0, 0, shell_set_func_ptr);
                    break;
                case 6:
                    if (arg == 0) {
                        shell_set_func_ptr->method_0x5C((::_AINPC_W*)self, 6, shell_set_func_ptr);
                    } else {
                        shell_set_func_ptr->method_0x5C((::_AINPC_W*)self, 8, shell_set_func_ptr);
                    }
                    break;
                }
                break;
            case 2:
            case 3:
                shell_set_func_ptr->method_0x94((::_AINPC_W*)self, 0, &self->pos_0x1B0, shell_set_func_ptr);
                break;
            }
        }
        break;
    }
}
#pragma opt_common_subs reset

/* 0x802C8E50 (0x34C): motion 3 steps 7 and 16 - the triple shot: three rounds of the firing motion, each firing the
 * formation's projectile at frame 94. */
#pragma opt_common_subs off
void ai_npc_m3_step7_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    u8 state;

    VEC3_ctor(&dir);
    VEC3_ctor(&offset);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x29, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x4D, 2, 0);
            if (self->field_0x420 == 2) {
                fn_802D29CC(self, 1.2f);
            }
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            self->sub_step = 3;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x2A, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x4E, 0, 0);
            }
        }
        break;
    case 2:
        if (arg == 1 && ai_npc_frame_ck(self, 0, 88.0f, 0.0f) == 1) {
            setVector3(&offset, 0.0f, 0.0f, 0.0f);
            eft013_npc_place(self, &offset, 8, 0.5f, 0x22);
        }
        if (ai_npc_frame_ck(self, 0, 94.0f, 0.0f) == 1) {
            switch (self->field_0x420) {
            case 1:
                shell_set_func_ptr->method_0x90((::_AINPC_W*)self, 2, 2, 0, 0, shell_set_func_ptr);
                break;
            case 2:
                shell_set_func_ptr->method_0x90((::_AINPC_W*)self, 3, 2, 0, 0, shell_set_func_ptr);
                break;
            }
        }
        if (ai_npc_frame_ck(self, 0, 124.0f, 0.0f) == 1 && --self->sub_step != 0) {
            if (arg == 0) {
                ai_npc_motion_set(self, 0x2A, 4, 0x44);
            } else {
                ai_npc_motion_set(self, 0x4E, 4, 0x44);
            }
        } else if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        } else if (ai_npc_frame_ck(self, 2, 94.0f, 0.0f) == 1) {
            subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
            copyVec3(&dir, &diff);
            ::calcVecAngXY(&dir, &pitch, &yaw);
            self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, 0x200);
            if (arg == 1) {
                self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x200);
            }
        }
        break;
    }
}
#pragma opt_common_subs reset

/* 0x802C919C (0x158): motion 3 step 8 - the vault: winds up, jumps with `ai_npc_vault_tbl` (hit row 5) and
 * lands. */
void ai_npc_m3_step8_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_motion_set(self, 0x32, 4, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_variant_set(self, 1);
            ai_npc_motion_set(self, 0x33, 4, 0);
            fn_802D31FC(self);
            self->pos_0x1D4.y = ai_npc_vault_tbl[0];
            self->pos_0x1E0.y = ai_npc_vault_tbl[1];
            self->pos_0x1D4.z = ai_npc_vault_tbl[2];
            fn_802D2F7C(self, 0, 5);
        }
        break;
    case 2:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (self->pos_0x178.y <= self->field_0x19C) {
            self->state++;
            ai_npc_variant_set(self, 0);
            self->pos_0x178.y = self->field_0x19C;
            ai_npc_motion_set(self, 0x34, 4, 0);
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C92F4 (0x80): motion 3 step 10 - the recoil (variant 0). */
void ai_npc_m3_step10_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 0x25, 8, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802C9374 (0x80): motion 3 step 11 - the recoil (variant 2). */
void ai_npc_m3_step11_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_set(self, 0x4B, 8, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 2);
        }
        break;
    }
}

/* 0x802C93F4 (0xDC): motion 3 step 12 - the airborne recoil: knocked back along `ai_npc_hop_back_tbl` until it
 * lands. */
void ai_npc_m3_step12_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 1);
        ai_npc_motion_set(self, 0x25, 8, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.y = ai_npc_hop_back_tbl[0];
        self->pos_0x1E0.y = ai_npc_hop_back_tbl[1];
        self->pos_0x1D4.z = ai_npc_hop_back_tbl[2];
        break;
    case 1:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        } else {
            if (self->pos_0x178.y <= self->field_0x19C) {
                self->pos_0x178.y = self->field_0x19C;
                ai_npc_variant_set(self, 0);
            }
        }
        break;
    }
}

/* 0x802C94D0 (0xD0): motion 3 step 13 - the lunge (hit row 7). */
void ai_npc_m3_step13_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_set(self, 0x47, 2, 0);
        fn_802D2F7C(self, 0, 7);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 2.0f;
        if (self->field_0x420 == 2) {
            fn_802D29CC(self, 1.2f);
        }
        break;
    case 1:
        fn_802D3210(self, (u32*)&self->vec_0x190);
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 2);
        } else {
            ai_npc_attack_hold_ck(self);
        }
        break;
    }
}

/* 0x802C95A0 (0x174): motion 3 step 14 - the flurry: charges forward and arms hit row 6 at every frame of
 * `ai_npc_flurry_frames`, then recovers. */
void ai_npc_m3_step14_arm(_AINPC_W* self)
{
    u8 i;
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 2);
        ai_npc_motion_set(self, 0x4C, 2, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 10.0f;
        if (self->field_0x420 == 2) {
            fn_802D29CC(self, 1.2f);
        }
        break;
    case 1:
        fn_802D3210(self, (u32*)&self->vec_0x190);
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_motion_set(self, 0x4A, 0, 0);
        } else {
            for (i = 0; ai_npc_flurry_frames[i] > 0.0f; i++) {
                if (ai_npc_frame_ck(self, 0, ai_npc_flurry_frames[i], 0.0f) == 1) {
                    fn_802D2F7C(self, 0, 6);
                }
            }
            ai_npc_attack_hold_ck(self);
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 2);
        }
        break;
    }
}

/* 0x802C9714 (0x1F4): motion 3 step 18 - the spinning charge toward the move target: arms hit row 8 at the frames
 * of `ai_npc_spin_frames` (while `ai_npc_flurry_frames` has entries) until it arrives or times out. */
void ai_npc_m3_step18_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    f32 dist;
    u16 i;
    u8 state;

    VEC3_ctor(&dir);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = 300;
        ai_npc_motion_set(self, 0x28, 2, 0);
        ai_npc_variant_set(self, 0);
        fn_802D31FC(self);
        self->pos_0x1D4.z = 15.0f;
        break;
    case 1:
        for (i = 0; ai_npc_flurry_frames[i] > 0.0f; i++) {
            if (ai_npc_frame_ck(self, 0, ai_npc_spin_frames[i], 0.0f) == 1) {
                fn_802D2F7C(self, 0, 8);
            }
        }
        ai_npc_attack_hold_ck(self);
        fn_802D3210(self, (u32*)&self->vec_0x190);
        dist = ::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0);
        if (fn_802D3B34(self, 20, 1) == 1) {
            fn_802D2ABC(self, 1, 9, 0);
        } else if (dist <= self->field_0x1BC * self->field_0x1BC) {
            ai_npc_base_state_set(self, 0);
            break;
        } else if (--self->timer_0x1AC < 0) {
            ai_npc_base_state_set(self, 0);
            break;
        }
        subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
        copyVec3(&dir, &diff);
        self->vec_0x190.y = fn_802D30F8(::calcVecAng3(&dir), self->vec_0x190.y, 0x800);
        break;
    }
}

/* 0x802C9908 (0x1B8): motion 3 steps 19 and 20 - the carried run: asks the shell for the carry job and runs toward
 * the move target for 300 frames. */
#pragma opt_common_subs off
void ai_npc_m3_step19_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    u32 pitch;
    u32 yaw;
    u8 state;

    VEC3_ctor(&dir);
    self->field_0x450 = 1;
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = 300;
        fn_802D31FC(self);
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x35, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x48, 4, 0);
            self->pos_0x1D4.z = 10.0f;
        }
        shell_set_func_ptr->method_0x5C((::_AINPC_W*)self, 4, shell_set_func_ptr);
        break;
    case 1:
        fn_802D3210(self, (u32*)&self->vec_0x190);
        ::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0);
        if (--self->timer_0x1AC < 0) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        } else {
            subVec3(&diff, &self->pos_0x1B0, &self->pos_0x178);
            copyVec3(&dir, &diff);
            ::calcVecAngXY(&dir, &pitch, &yaw);
            self->vec_0x190.y = fn_802D30F8(yaw, self->vec_0x190.y, 0x400);
            if (arg == 1) {
                self->vec_0x190.x = fn_802D30F8(pitch, self->vec_0x190.x, 0x400);
            }
        }
        break;
    }
}
#pragma opt_common_subs reset

/* 0x802C9AC0 (0xC0): runs the arm of the record's motion-3 step. */
void ai_npc_motion3_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_m3_step0_arm(self, 0);
        break;
    case 1:
        ai_npc_m3_step1_arm(self);
        break;
    case 2:
        ai_npc_m3_step2_arm(self, 0);
        break;
    case 3:
        ai_npc_m3_step2_arm(self, 1);
        break;
    case 4:
        ai_npc_m3_step2_arm(self, 2);
        break;
    case 5:
        ai_npc_m3_step5_arm(self, 0);
        break;
    case 6:
        ai_npc_m3_step6_arm(self, 0);
        break;
    case 7:
        ai_npc_m3_step7_arm(self, 0);
        break;
    case 8:
        ai_npc_m3_step8_arm(self);
        break;
    case 9:
        ai_npc_m3_step0_arm(self, 1);
        break;
    case 10:
        ai_npc_m3_step10_arm(self);
        break;
    case 11:
        ai_npc_m3_step11_arm(self);
        break;
    case 12:
        ai_npc_m3_step12_arm(self);
        break;
    case 13:
        ai_npc_m3_step13_arm(self);
        break;
    case 14:
        ai_npc_m3_step14_arm(self);
        break;
    case 15:
        ai_npc_m3_step6_arm(self, 1);
        break;
    case 16:
        ai_npc_m3_step7_arm(self, 1);
        break;
    case 17:
        ai_npc_m3_step5_arm(self, 1);
        break;
    case 18:
        ai_npc_m3_step18_arm(self);
        break;
    case 19:
        ai_npc_m3_step19_arm(self, 0);
        break;
    case 20:
        ai_npc_m3_step19_arm(self, 1);
        break;
    case 21:
        ai_npc_m3_step6_arm(self, 2);
        break;
    case 22:
        ai_npc_m3_step6_arm(self, 3);
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802C9B80 (0xF0): motion 4 steps 0 and 12 - the flinch: faces the stored hit bearing (+0x218) and plays the
 * flinch once. */
void ai_npc_m4_step0_arm(_AINPC_W* self, u8 arg)
{
    u8 state;

    fn_802D3A2C(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x64, 0, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x70, 0, 0);
        }
        self->vec_0x190.y = self->field_0x218;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802C9C70 (0x1C4): motion 4 steps 1 and 13 - the knock-down: falls, lies and gets up. */
void ai_npc_m4_step1_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 offset;
    u8 state;

    VEC3_ctor(&pos);
    VEC3_ctor(&offset);
    fn_802D3A2C(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x65, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x71, 2, 0);
        }
        self->vec_0x190.y = self->field_0x218;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x66, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x72, 2, 0);
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x67, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x73, 0, 0);
            }
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802C9E34 (0x188): motion 4 step 2 - the blow-away: falls at no more than -5 under gravity -1, lands, gets
 * up. */
void ai_npc_m4_step2_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 offset;
    u8 state;

    VEC3_ctor(&pos);
    VEC3_ctor(&offset);
    fn_802D3A2C(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 1);
        ai_npc_motion_set(self, 0x6D, 0, 0);
        fn_802D31FC(self);
        if (self->pos_0x1D4.y > -5.0f) {
            self->pos_0x1D4.y = -5.0f;
        }
        self->pos_0x1E0.x = 0.0f;
        self->pos_0x1E0.y = -1.0f;
        self->pos_0x1E0.z = 0.0f;
        break;
    case 1:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (self->pos_0x178.y <= self->field_0x19C) {
            self->state++;
            self->pos_0x178.y = self->field_0x19C;
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x6E, 0, 0);
            fn_802D31FC(self);
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_motion_set(self, 0x68, 4, 0);
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_base_state_set(self, 0);
            fn_802D31FC(self);
        }
        break;
    }
}

/* 0x802C9FBC (0x134): motion 4 steps 3, 4, 14, 15, 23 and 24 - the status reaction of `ai_npc_status_motion_tbl`'s
 * row: its motion `count` times, then its closing motion. */
void ai_npc_m4_step3_arm(_AINPC_W* self, u8 arg)
{
    s16* row;
    u8 state = self->state;

    switch (state) {
    case 0:
        row = &ai_npc_status_motion_tbl[arg * 6];
        self->state++;
        ai_npc_variant_set(self, row[0]);
        ai_npc_motion_set(self, (u16)row[1], row[2], 0);
        self->sub_step = row[3];
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (--self->sub_step == 0) {
                row = &ai_npc_status_motion_tbl[arg * 6 + 4];
                self->state++;
                ai_npc_motion_set(self, (u16)row[0], row[1], 0);
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            row = &ai_npc_status_motion_tbl[arg * 6];
            ai_npc_base_state_set(self, row[0]);
        }
        break;
    }
}

/* 0x802CA0F0 (0x154): motion 4 steps 5, 6, 16 and 17 - the same with `ai_npc_status_motion_tbl2`; rows 2 and 3
 * have no closing motion. */
void ai_npc_m4_step5_arm(_AINPC_W* self, u8 arg)
{
    s16* row;
    u8 state = self->state;

    switch (state) {
    case 0:
        row = &ai_npc_status_motion_tbl2[arg * 6];
        self->state++;
        ai_npc_variant_set(self, row[0]);
        ai_npc_motion_set_u16(self, row[1], row[2], 0);
        self->sub_step = row[3];
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (--self->sub_step == 0) {
                if (arg < 2) {
                    row = &ai_npc_status_motion_tbl2[arg * 6 + 4];
                    self->state++;
                    ai_npc_motion_set_u16(self, row[0], row[1], 0);
                } else {
                    ai_npc_base_state_set(self, 2);
                }
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            row = &ai_npc_status_motion_tbl2[arg * 6];
            ai_npc_base_state_set(self, row[0]);
        }
        break;
    }
}

/* 0x802CA244 (0x1F8): motion 4 steps 8 and 19 - the sleep: lies for 150 frames with the sleep effect every 32
 * frames, then gets up. */
void ai_npc_m4_step8_arm(_AINPC_W* self, u8 arg)
{
    u8 state;

    self->sub_step++;
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x6A, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x75, 2, 0);
        }
        self->field_0x228 = 0;
        eft013_npc_motion_spawn((::_AINPC_W*)self, 2, 0.6f);
        self->field_0x22C = 150;
        break;
    case 1:
        if (--self->field_0x22C < 0) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x6B, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x76, 0, 0);
            }
        }
        if ((self->sub_step & 0x1F) == 0) {
            eft013_npc_motion_spawn((::_AINPC_W*)self, 2, 0.6f);
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x68, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x6F, 0, 0);
            }
        }
        if ((self->sub_step & 0x1F) == 0) {
            eft013_npc_motion_spawn((::_AINPC_W*)self, 2, 0.6f);
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802CA43C (0x1B8): motion 4 steps 9 and 20 - the paralysis: shakes for 300 frames under the aura effect, then
 * gets up and clears the status. */
void ai_npc_m4_step9_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x6C, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x77, 2, 0);
        }
        self->field_0x232 = 300;
        self->field_0x22E = 0;
        eft_npc_aura_create((::_AINPC_W*)self);
        break;
    case 1:
        self->field_0x232--;
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x66, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x72, 2, 0);
            }
        }
        break;
    case 2:
        if (--self->field_0x232 <= 0) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x67, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x73, 0, 0);
            }
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            fn_802D3CDC(self);
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802CA5F4 (0x278): motion 4 steps 10 and 21 - the stun: shakes for 300 frames with the stun effect over the
 * head every 30 frames, then gets up and clears the status. */
void ai_npc_m4_step10_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 offset;
    u8 state;

    VEC3_ctor(&offset);
    setVector3(&offset, 0.0f, 50.0f, 0.0f);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x6C, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x77, 2, 0);
        }
        self->field_0x226 = 300;
        self->field_0x222 = 0;
        break;
    case 1:
        self->field_0x226--;
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x66, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x72, 2, 0);
            }
        } else if (self->field_0x226 % 30 == 0) {
            eft013_npc_place(self, &offset, 0xE, 0.75f, 0xA);
        }
        break;
    case 2:
        if (--self->field_0x226 <= 0) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x67, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x73, 0, 0);
            }
        } else if (self->field_0x226 % 30 == 0) {
            eft013_npc_place(self, &offset, 0xE, 0.75f, 0xA);
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            fn_802D3CEC(self);
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802CA86C (0xE8): motion 4 steps 11 and 22 - the light flinch: faces the hit bearing and plays it once. */
void ai_npc_m4_step11_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x2C, 4, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x50, 4, 0);
        }
        self->vec_0x190.y = self->field_0x218;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_base_state_set(self, 0);
            } else {
                ai_npc_base_state_set(self, 2);
            }
        }
        break;
    }
}

/* 0x802CA954 (0xDC): runs the arm of the record's motion-4 step; steps 7 and 18 have none. */
void ai_npc_motion4_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_m4_step0_arm(self, 0);
        break;
    case 1:
        ai_npc_m4_step1_arm(self, 0);
        break;
    case 2:
        ai_npc_m4_step2_arm(self);
        break;
    case 3:
        ai_npc_m4_step3_arm(self, 0);
        break;
    case 4:
        ai_npc_m4_step3_arm(self, 1);
        break;
    case 5:
        ai_npc_m4_step5_arm(self, 0);
        break;
    case 6:
        ai_npc_m4_step5_arm(self, 1);
        break;
    case 7:
        break;
    case 8:
        ai_npc_m4_step8_arm(self, 0);
        break;
    case 9:
        ai_npc_m4_step9_arm(self, 0);
        break;
    case 10:
        ai_npc_m4_step10_arm(self, 0);
        break;
    case 11:
        ai_npc_m4_step11_arm(self, 0);
        break;
    case 12:
        ai_npc_m4_step0_arm(self, 1);
        break;
    case 13:
        ai_npc_m4_step1_arm(self, 1);
        break;
    case 14:
        ai_npc_m4_step3_arm(self, 2);
        break;
    case 15:
        ai_npc_m4_step3_arm(self, 3);
        break;
    case 16:
        ai_npc_m4_step5_arm(self, 2);
        break;
    case 17:
        ai_npc_m4_step5_arm(self, 3);
        break;
    case 18:
        break;
    case 19:
        ai_npc_m4_step8_arm(self, 1);
        break;
    case 20:
        ai_npc_m4_step9_arm(self, 1);
        break;
    case 21:
        ai_npc_m4_step10_arm(self, 1);
        break;
    case 22:
        ai_npc_m4_step11_arm(self, 1);
        break;
    case 23:
        ai_npc_m4_step3_arm(self, 4);
        break;
    case 24:
        ai_npc_m4_step3_arm(self, 5);
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802CAA30 (0x1E8): motion 5 steps 0 and 4 - collapses (falls, lies, rises) and hands over to the faint
 * wait. */
void ai_npc_m5_step0_arm(_AINPC_W* self, u8 arg)
{
    u8 state;

    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (arg == 0) {
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x65, 2, 0);
        } else {
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x71, 2, 0);
        }
        self->vec_0x190.y = self->field_0x218;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x66, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x72, 2, 0);
            }
        }
        break;
    case 2:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            if (arg == 0) {
                ai_npc_motion_set(self, 0x67, 0, 0);
            } else {
                ai_npc_motion_set(self, 0x73, 0, 0);
            }
        }
        break;
    case 3:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            self->field_0x3D2 = 0;
            if (arg == 0) {
                self->pos_0x178.y = self->field_0x19C;
                fn_802D2ABC(self, 5, 5, 0);
            } else {
                fn_802D2ABC(self, 5, 6, 0);
            }
        }
        break;
    }
}

/* 0x802CAC18 (0x4): motion 5 step 1 - no arm work. */
void ai_npc_m5_step1_arm(_AINPC_W* self)
{
}

/* 0x802CAC1C (0x1E0): motion 5 step 2 - the faint: hides the record for 30 or 60 seconds (skill 0x12 shortens it),
 * then revives it beside the player. */
void ai_npc_m5_step2_arm(_AINPC_W* self)
{
    f32 height;
    u8 state;

    fn_802D3A2C(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        if (::ai_skill_ck((::_AINPC_W*)self, 0x12) == 1) {
            self->timer_0x1AC = 1800;
        } else {
            self->timer_0x1AC = 3600;
        }
        fn_802D2B88(self, 2);
        if (::ai_skill_ck((::_AINPC_W*)self, 0x16) == 1) {
            self->state = 2;
            ai_npc_talk(self, (s8)self->field_0x482, 0xC, 0, 2);
        } else {
            ai_npc_talk(self, (s8)self->field_0x482, 0xD, 0, 2);
        }
        break;
    case 1:
        if (--self->timer_0x1AC < 0) {
            fn_802D3684(self);
            fn_802D6B98(self);
            copyVec3(&self->pos_0x178, &self->plw_0x16C->vec_0x03C);
            if (pl_ground_check_all_layers(self, &self->pos_0x178, &self->field_0x324, &height, -5) == 1) {
                self->pos_0x178.y = self->field_0x19C = height;
            }
            copyVec3(&self->pos_0x184, &self->pos_0x178);
            fn_802D2B98(self, 2);
            if (self->plw_0x16C->kind_0x09 == 3) {
                ai_npc_variant_set(self, 2);
                fn_802D2ABC(self, 5, 8, 0);
            } else {
                ai_npc_variant_set(self, 0);
                fn_802D2ABC(self, 5, 7, 0);
            }
            se_npc_revive_play();
            if (::ai_skill_ck((::_AINPC_W*)self, 0x15) != 0) {
                ai_npc_talk(self, (s8)self->field_0x482, 0xB, 0, 2);
            }
        }
        break;
    }
}

/* 0x802CADFC (0x1C0): motion 5 step 3 - the slow recovery: refills the gauge one point per step and revives the
 * record beside the player once it is full. */
void ai_npc_m5_step3_arm(_AINPC_W* self)
{
    f32 height;
    u8 state;

    fn_802D3A2C(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        self->timer_0x1AC = fn_802D7CB0(self);
        fn_802D2B88(self, 2);
        if (::ran_suu(1) & 1) {
            ai_npc_talk(self, (s8)self->field_0x482, 0xE, 0, 2);
        } else {
            ai_npc_talk(self, (s8)self->field_0x482, 0xF, 0, 2);
        }
        break;
    case 1:
        if (--self->timer_0x1AC < 0) {
            fn_802D35B4(self, 1);
            if (self->gauge_0x1F0 == self->gauge_max_0x1F4) {
                self->state++;
                copyVec3(&self->pos_0x178, &self->plw_0x16C->vec_0x03C);
                if (pl_ground_check_all_layers(self, &self->pos_0x178, &self->field_0x324, &height, -5) == 1) {
                    self->pos_0x178.y = self->field_0x19C = height;
                }
                copyVec3(&self->pos_0x184, &self->pos_0x178);
                fn_802D2B98(self, 2);
                if (self->plw_0x16C->kind_0x09 == 3) {
                    ai_npc_variant_set(self, 2);
                    fn_802D2ABC(self, 5, 8, 0);
                } else {
                    ai_npc_variant_set(self, 0);
                    fn_802D2ABC(self, 5, 7, 0);
                }
                se_npc_revive_play();
            } else {
                self->timer_0x1AC = fn_802D7CB0(self);
            }
        }
        break;
    }
}

/* 0x802CAFBC (0x190): motion 5 steps 5 and 6 - lies down (with the dust effect at frame 22) and hands over to the
 * faint (step 2) or the slow recovery (step 3). */
void ai_npc_m5_step5_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 pos;
    u8 state;

    VEC3_ctor(&pos);
    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        switch (arg) {
        case 0:
            ai_npc_variant_set(self, 0);
            ai_npc_motion_set(self, 0x13, 2, 0);
            break;
        case 1:
            ai_npc_variant_set(self, 2);
            ai_npc_motion_set(self, 0x3C, 2, 0);
            break;
        }
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            if (self->field_0x3D2 != 0) {
                fn_802D2ABC(self, 5, 3, 2);
            } else {
                fn_802D2ABC(self, 5, 2, 2);
            }
            se_npc_faint_play();
        } else if (ai_npc_frame_ck(self, 0, 22.0f, 0.0f) != 0) {
            copyVec3(&pos, &self->pos_0x178);
            if (self->pos_0x178.y < self->field_0x1A0 && self->pos_0x178.y > self->field_0x1A0 - 40.0f) {
                pos.y = self->field_0x1A0;
            }
            ::eft019_set_subtype(0x20, 1, &pos, self->area, &self->vec_0x190, 0x50, 0.5f);
        }
        break;
    }
}

/* 0x802CB14C (0xE8): motion 5 step 9 - the final collapse: after 150 frames down it ends the quest. */
void ai_npc_m5_step9_arm(_AINPC_W* self)
{
    u8 state;

    fn_802D3A2C(self, 2);
    fn_802D84D0(self, 2);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 0x65, 2, 0);
        self->vec_0x190.y = self->field_0x218;
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            self->state++;
            ai_npc_motion_set(self, 0x66, 0, 0);
            self->timer_0x1AC = 150;
        }
        break;
    case 2:
        if (--self->timer_0x1AC <= 0) {
            pl_quest_end_request(0);
        }
        break;
    }
}

/* 0x802CB234 (0x68): runs the arm of the record's motion-5 step. */
void ai_npc_motion5_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_m5_step0_arm(self, 0);
        break;
    case 1:
        ai_npc_m5_step1_arm(self);
        break;
    case 2:
        ai_npc_m5_step2_arm(self);
        break;
    case 3:
        ai_npc_m5_step3_arm(self);
        break;
    case 4:
        ai_npc_m5_step0_arm(self, 1);
        break;
    case 5:
        ai_npc_m5_step5_arm(self, 0);
        break;
    case 6:
        ai_npc_m5_step5_arm(self, 1);
        break;
    case 7:
        ai_npc_step5_arm(self, 0);
        break;
    case 8:
        ai_npc_step5_arm(self, 1);
        break;
    case 9:
        ai_npc_m5_step9_arm(self);
        break;
    }
}

/* 0x802CB29C (0xBC): whether the record has fallen into its area's water zone; when it has, it is set floating
 * there (both model flags 10) with a voice line. */
u32 ai_npc_water_landing_ck(_AINPC_W* self)
{
    LandData land;
    u32 zone;
    f32 height;
    u8 area;

    em_land_data_init(&land);
    area = self->area;
    if (area == 0xFF) {
        return 0;
    }
    if (stage_zone_hit_ck(stage_group_word_get(::get_now_mapno(), area), &zone, &self->pos_0x178) == 1 &&
        pl_ground_check_all_layers(self, &self->pos_0x178, &land, &height, 0x4000) == 1) {
        fn_802D31FC(self);
        fn_802D29E4(self, 10);
        fn_802D29B8(self, 10);
        return 1;
    }
    return 0;
}

/* 0x802CB358 (0x148): motion 6 step 0 - the fall: jumps off, falls until the water catches it or it reaches the
 * ground, then lands (step 1). */
void ai_npc_m6_step0_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 pos;
    f32 ground;
    u8 state;

    VEC3_ctor(&pos);
    state = self->state;
    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 1);
        ai_npc_motion_set(self, 0x11, 2, 0);
        self->pos_0x1D4.x = 0.0f;
        self->pos_0x1D4.y = 15.0f;
        self->pos_0x1D4.z = 10.0f;
        self->pos_0x1E0.x = 0.0f;
        self->pos_0x1E0.y = -2.0f;
        self->pos_0x1E0.z = 0.0f;
        break;
    case 1:
        fn_802D327C(self, (u32*)&self->vec_0x190);
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_motion_set(self, 0x26, 2, 0);
            self->state++;
        }
        break;
    case 2:
        if (ai_npc_water_landing_ck(self) != 1) {
            fn_802D327C(self, (u32*)&self->vec_0x190);
            if (self->pos_0x1D4.y < 0.0f) {
                ground = ::GetGroundHit(&self->pos_0x178, -6, self->area);
                if (ground >= self->pos_0x178.y) {
                    self->pos_0x178.y = ground;
                    fn_802D2ABC(self, 6, 1, 0);
                }
            }
        }
        break;
    }
}

/* 0x802CB4A0 (0x80): motion 6 step 1 - the landing (the dispatcher hands it a zero it does not read). */
void ai_npc_m6_step1_arm(_AINPC_W* self, u8 arg)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 0x27, 2, 0);
        break;
    case 1:
        if (ai_npc_motion_end_ck(self) == 1) {
            ai_npc_base_state_set(self, 0);
        }
        break;
    }
}

/* 0x802CB520 (0x28): runs the arm of the record's motion-6 step. */
void ai_npc_motion6_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_m6_step0_arm(self);
        break;
    case 1:
        ai_npc_m6_step1_arm(self, 0);
        break;
    }
}

/* 0x802CB548 (0x64): whether the event demo is at `frame` (the demo clock runs two frames ahead). */
u32 ai_npc_demo_frame_ck(s16 frame)
{
    return em_demo_time_ck((s16)(2.0f + frame)) == 1;
}

/* 0x802CB5AC (0x21C): motion 7 step 0 - the opening demo: hidden until frame 500, placed at the demo's marks at
 * frames 500 and 1146, turning between 1160 and 1174, then walks off. */
void ai_npc_m7_step0_arm(_AINPC_W* self)
{
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state++;
        ai_npc_variant_set(self, 0);
        ai_npc_motion_set(self, 1, 0, 0);
        setVector3(&self->pos_0x178, 10000.0f, 10000.0f, 10000.0f);
        fn_802D2B88(self, 2);
        break;
    case 1:
        if (ai_npc_demo_frame_ck(500) == 1) {
            self->state++;
            fn_802D2B98(self, 2);
            setVector3(&self->pos_0x178, 5.57f, 20.0f, -306.9f);
            copyVec3(&self->pos_0x184, &self->pos_0x178);
            self->vec_0x190.y = 0xDD55;
            ai_npc_motion_set(self, 10, 0, 0);
        }
        break;
    case 2:
        if (ai_npc_demo_frame_ck(1146) == 1) {
            self->state++;
            ai_npc_motion_set(self, 2, 0, 0);
            setVector3(&self->pos_0x178, -279.5f, 20.0f, -56.9f);
            copyVec3(&self->pos_0x184, &self->pos_0x178);
        }
        break;
    case 3:
        if (ai_npc_demo_frame_ck(1160) == 1 && ai_npc_demo_frame_ck(1174) == 0) {
            self->vec_0x190.y += 0xDF1;
        }
        if (ai_npc_frame_ck(self, 0, 60.0f, 0.0f) == 1) {
            self->state++;
            ai_npc_motion_set(self, 1, 4, 0);
            fn_802D29CC(self, 0.5f);
        }
        break;
    case 4:
        if (ai_npc_frame_ck(self, 0, 30.0f, 0.0f) == 1) {
            self->state++;
            fn_802D29CC(self, 1.0f);
            ai_npc_motion_set(self, 0x14, 2, 0);
        }
        break;
    }
}

/* 0x802CB7C8 (0x6C): motion 7 step 1 - skips the demo: puts the record at its end mark. */
void ai_npc_m7_step1_arm(_AINPC_W* self)
{
    u8 state = self->state;

    if (state == 0) {
        self->state++;
        ai_npc_variant_set(self, 0);
        setVector3(&self->pos_0x178, -279.5f, 20.0f, -56.9f);
        self->vec_0x190.y = 0x3EEF;
        ai_npc_base_state_set(self, 0);
    }
}

/* 0x802CB834 (0x24): runs the arm of the record's motion-7 step. */
void ai_npc_motion7_dispatch(_AINPC_W* self)
{
    switch (self->motion_step) {
    case 0:
        ai_npc_m7_step0_arm(self);
        break;
    case 1:
        ai_npc_m7_step1_arm(self);
        break;
    }
}

/* 0x802CB858 (0x50): runs the arm of the record's motion (+0x171) after clearing the setter's flag. */
void ai_npc_motion_run(_AINPC_W* self)
{
    self->field_0x174 = 0;
    switch (self->motion) {
    case 0:
        ai_npc_motion0_dispatch((LightArm*)self);
        break;
    case 1:
        ai_npc_step_dispatch(self);
        break;
    case 2:
        ai_npc_motion2_dispatch(self);
        break;
    case 3:
        ai_npc_motion3_dispatch(self);
        break;
    case 4:
        ai_npc_motion4_dispatch(self);
        break;
    case 5:
        ai_npc_motion5_dispatch(self);
        break;
    case 6:
        ai_npc_motion6_dispatch(self);
        break;
    case 7:
        ai_npc_motion7_dispatch(self);
        break;
    }
}

/* 0x802CB8A8 (0x58): takes the player's position as the move target. */
void ai_npc_target_player_set(_AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    VEC3_ctor(&c);
    copyVec3(&self->pos_0x1B0, &plw->vec_0x03C);
}

/* 0x802CB900 (0x50): whether two points are no farther apart than `limit`. */
u8 ai_npc_in_range_ck(nw4r::math::VEC3* a, nw4r::math::VEC3* b, f32 limit)
{
    return vec3_dist_sq(a, b) <= limit * limit;
}

/* 0x802CB950 (0x14): whether the item kind is 34. */
u32 ai_npc_item34_ck(u8 kind)
{
    return kind == 34;
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802CB964 (0x258): lists the live enemies of the record's area that it may engage, by the `flags` bits (0x08
 * measure from the player, 0x10 only those with +0x1C8 bit 0, 0x02 only action 11, 0x04 only the nearest) into
 * `out`; returns the count (1 when the nearest is kept). */
u8 ai_npc_enemy_scan(_AINPC_W* self, u8 flags, u8* out)
{
    nw4r::math::VEC3 spots[32];
    nw4r::math::VEC3 from;
    nw4r::math::VEC3* spot;
    f32 dist;
    f32 nearest;
    s16 max;
    _ENEMY_WORK* enemy;
    u8 count;
    u8 i;

    enemy = (_ENEMY_WORK*)::get_move_work_adrs(3);
    max = ::get_move_work_max(3);
    spot = spots;
    do {
        VEC3_ctor(spot);
        spot++;
    } while (spot < &spots[32]);
    VEC3_ctor(&from);
    nearest = 0.0f;
    count = 0;
    if (flags & 8) {
        copyVec3(&from, &self->plw_0x16C->vec_0x03C);
    } else {
        copyVec3(&from, &self->pos_0x178);
    }
    for (i = 0; i < max; i++, enemy++) {
        if (enemy->active == 0) {
            continue;
        }
        if (enemy->area_no != self->area) {
            continue;
        }
        if (enemy->team == 40) {
            continue;
        }
        if (::em_sleep_ck(enemy, 1) == 1) {
            continue;
        }
        if (em_flag_0x8AD_ck(enemy) == 1) {
            continue;
        }
        if (enemy->field_0x1E2 == 4 && em_tall_ck(enemy) == 0) {
            continue;
        }
        if (enemy->field_0x1E2 == 1 && enemy->field_0x1E3 == 2) {
            continue;
        }
        if (enemy->field_0x1E2 == 1 && enemy->field_0x1E3 == 1) {
            continue;
        }
        if (ai_npc_item34_ck(enemy->team) == 1 && self->field_0x383 == 1) {
            continue;
        }
        if (enemy->field_0x43D != 1 && self->field_0x383 == 1) {
            continue;
        }
        if ((flags & 0x10) && !(enemy->field_0x1C8 & 1)) {
            continue;
        }
        if (flags & 2) {
            if (enemy->action != 11) {
                continue;
            }
        } else if (enemy->action == 11 || enemy->action == 12) {
            continue;
        }
        if (flags & 4) {
            dist = vec3_dist_sq(&from, &enemy->pos);
            if (nearest == 0.0f) {
                nearest = dist;
            }
            if (dist <= nearest) {
                count = 1;
                *out = i;
            }
        } else {
            *out = i;
            count++;
            out++;
        }
    }
    return count;
}

/* 0x802CBBBC (0x30): whether the item gauge (+0x3D4) is full or past the use threshold (+0x414). */
s32 ai_npc_item_gauge_ck(_AINPC_W* self)
{
    s16 gauge = self->field_0x3D4;

    if (gauge >= self->field_0x3D6) {
        return 1;
    }
    return self->field_0x414 <= gauge;
}

/* 0x802CBBEC (0x198): picks the buff items the record may use now (a 15 % roll with the gauge ready): every slot
 * level bit of +0x3F4 whose cooldown has run out; stores the set in +0x3F2. */
s32 ai_npc_buff_item_pick(_AINPC_W* self)
{
    u16 pick = 0;
    u16 levels;

    if (ai_npc_mode1_ck(self) == 1) {
        return 0;
    }
    if (::ran_suu(1) % 100 > 15) {
        return 0;
    }
    if (ai_npc_item_gauge_ck(self) == 0) {
        return 0;
    }
    levels = self->field_0x3F4;
    if ((levels & 1) && self->field_0x1CC == 4 && self->field_0x408 == 0) {
        pick |= 1;
    }
    if ((levels & 2) && self->field_0x40A == 0) {
        pick |= 2;
    }
    if ((levels & 4) && self->field_0x40C == 0) {
        pick |= 4;
    }
    if ((levels & 0x40) && self->field_0x410 == 0) {
        pick |= 0x40;
    }
    if ((levels & 0x200) && self->field_0x40E == 0) {
        pick |= 0x200;
    }
    if ((levels & 0x100) && self->field_0x412 == 0) {
        pick |= 0x100;
    }
    if (levels & 0x80) {
        pick |= 0x80;
    }
    if (pick != 0) {
        self->field_0x3F2 = pick;
        return 1;
    }
    return 0;
}

/* 0x802CBD84 (0x364): picks a support item for the player (with the gauge ready): the cure for a status, the heal
 * by how low the player's health is, the drink for the area, the antidote, the paralysis cure; latches it in
 * +0x3F2/+0x3DE. */
u32 ai_npc_support_item_pick(_AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    s32 chance = 0;

    if (self->field_0x3DE == 1) {
        return 1;
    }
    if (ai_npc_item_gauge_ck(self) == 0) {
        return 0;
    }
    self->field_0x3F2 = 0;
    if (self->field_0x3F4 & 0x400) {
        if (::Pl_condition_ck(plw, 1) == 1) {
            self->field_0x3F2 = 0x400;
            self->field_0x3DE = 1;
            return 1;
        }
        if (self->field_0x21C > 0 && (::ran_suu(1) & 1)) {
            self->field_0x3F2 = 0x400;
            self->field_0x3DE = 1;
            return 1;
        }
    }
    if (self->field_0x3F4 & 0x10) {
        f32 ratio = (f32)plw->health / (f32)plw->health_max;

        if (ratio <= 0.2f) {
            chance = 100;
        } else if (ratio <= 0.5f) {
            chance = 55;
        } else if (ratio <= 0.8f) {
            chance = 10;
        }
        if (::ran_suu(1) % 100 < chance) {
            self->field_0x3F2 = 0x10;
            self->field_0x3DE = 1;
            return 1;
        }
    }
    if (self->field_0x3F4 & 8) {
        f32 ratio = (f32)plw->health / (f32)plw->health_max;

        if (ratio <= 0.2f) {
            chance = 70;
        } else if (ratio <= 0.5f) {
            chance = 50;
        } else if (ratio <= 0.8f) {
            chance = 20;
        }
        if (::ran_suu(1) % 100 < chance) {
            self->field_0x3F2 = 8;
            self->field_0x3DE = 1;
            return 1;
        }
    }
    if ((self->field_0x3F4 & 0x20) && pl_act_kind_get(self->area) != 0 && ::Pl_condition_ck(plw, 0xC00) == 0) {
        self->field_0x3F2 = 0x20;
        self->field_0x3DE = 1;
        return 1;
    }
    if ((self->field_0x3F4 & 4) && ::Pl_condition_ck(plw, 0x80) == 1) {
        self->field_0x3F2 = 4;
        self->field_0x3DE = 1;
        return 1;
    }
    if ((self->field_0x3F4 & 0x100) && ::Pl_dm_condition_ck(plw, 0x7C00) == 1) {
        self->field_0x3F2 = 0x100;
        self->field_0x3DE = 1;
        return 1;
    }
    return 0;
}

/* 0x802CC0E8 (0x14): whether the record is carried (+0x451 == 1). */
u32 ai_npc_carried_ck(_AINPC_W* self)
{
    return self->field_0x451 == 1;
}

/* 0x802CC0FC (0xB0): the alternate-variant step of an item-use step (32..50 -> 61..79), 0xFF when there is
 * none. */
u16 ai_npc_alt_step_get(u16 step)
{
    switch (step) {
    case 32:
        return 61;
    case 33:
        return 62;
    case 34:
        return 63;
    case 35:
        return 64;
    case 37:
        return 66;
    case 38:
        return 67;
    case 39:
        return 68;
    case 40:
        return 69;
    case 42:
        return 71;
    case 43:
        return 72;
    case 44:
        return 73;
    case 45:
        return 74;
    case 47:
        return 76;
    case 48:
        return 77;
    case 49:
        return 78;
    case 50:
        return 79;
    default:
        return 0xFF;
    }
}

/* 0x802CC1AC (0x148): uses one of the buff items picked in +0x3F2: picks a random slot of the four whose level
 * matches, pays its gauge cost and starts its item step. */
void ai_npc_buff_item_use(_AINPC_W* self)
{
    u8 slots[4];
    u8 n = 0;
    u8 i;
    u8 slot;
    u16 step;
    s32 cost;

    for (i = 0; i < 4; i++) {
        if (self->field_0x3F2 & self->field_0x400[i]) {
            slots[n] = i;
            n++;
        }
    }
    if (n != 0) {
        slot = slots[::ran_suu(1) % n];
        step = self->field_0x3F8[slot];
        if (self->variant == 2) {
            step = ai_npc_alt_step_get(step);
        }
        cost = fn_802D77DC(ai_npc_level_rank_get(self->field_0x3FC[slot]));
        fn_802D6B2C(self, -cost);
        fn_802D2A00(self, 1, step, 0);
    }
}

/* 0x802CC2F4 (0x23C): looks for a gathering spot within reach in the area (farther with skill 0x14 or in the
 * desert formation); when the formation's roll allows, sets the move target beside it and claims it. */
u32 ai_npc_gather_spot_find(_AINPC_W* self)
{
    nw4r::math::VEC3 spot;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 target;
    f32 radius;
    u32 word;
    u32 pitch;
    u32 yaw;
    s16 kind;
    f32 reach;
    f32 height;

    ::get_move_work_adrs(3);
    VEC3_ctor(&spot);
    VEC3_ctor(&dir);
    if (::ai_skill_ck((::_AINPC_W*)self, 0x13) == 1) {
        return 0;
    }
    if (self->field_0x358 != 0) {
        return 1;
    }
    if (self->field_0x35A > 0) {
        return 0;
    }
    if (self->field_0x1CC == 1) {
        return 0;
    }
    if (self->field_0x1CF == 0) {
        return 0;
    }
    reach = 800.0f;
    if (::ai_skill_ck((::_AINPC_W*)self, 0x14) == 1) {
        reach += 400.0f;
    }
    if (self->field_0x420 == 5) {
        reach += 400.0f;
    }
    if (self->variant == 2) {
        height = reach;
    } else {
        height = 800.0f;
    }
    if (em_roster_area_near_find(self->area, &self->pos_0x178, reach, &kind, &spot, &word, &radius) != 0xFFFF &&
        lb_fabsf(self->pos_0x178.y - spot.y) < height && kind == 2) {
        fn_802D39DC(self);
        self->field_0x364 = word;
        if (fn_802D4020(self) == 1) {
            self->field_0x358 = 1;
            subVec3(&diff, &self->pos_0x178, &spot);
            copyVec3(&dir, &diff);
            ::calcVecAngXY(&dir, &pitch, &yaw);
            setVector3(&dir, 0.0f, 0.0f, 0.6f * radius);
            ::rotVecX(&dir, pitch);
            ::rotVecY(&dir, yaw);
            addVec3(&target, &spot, &dir);
            copyVec3(&self->pos_0x1B0, &target);
            copyVec3(&self->pos_0x368, &spot);
            return 1;
        }
        em_roster_area_use_take(self->area, &spot, self->field_0x364);
    }
    return 0;
}

/* 0x802CC530 (0x4C): the range the record engages enemies within (1500 wider while `fn_802D94A0` holds). */
f32 ai_npc_engage_range_get(_AINPC_W* self)
{
    f32 range = 1000.0f;

    if (fn_802D94A0(self) == 1) {
        range += 1500.0f;
    }
    return range;
}

/* 0x802CC57C (0x218): picks the enemy to engage: the one carrying an item first (+0x377), else any of the area;
 * attacks (4) when it is in range or under its hold, keeps watch (5) up to 2000, and stores its index. */
u32 ai_npc_enemy_target_pick(_AINPC_W* self)
{
    _ENEMY_WORK* enemies = (_ENEMY_WORK*)::get_move_work_adrs(3);
    _ENEMY_WORK* enemy;
    s32 restricted = 0;
    u8 found_list[32];
    u8 found;
    f32 dist;
    f32 range;

    if (self->motion == 5) {
        return 0;
    }
    if (self->field_0x376 != 0) {
        return 1;
    }
    switch (self->field_0x420) {
    case 1:
    case 4:
    case 5:
        restricted = 1;
        break;
    }
    self->field_0x377 = 0;
    found = ai_npc_enemy_scan(self, 0x11, found_list);
    if (found != 0) {
        self->field_0x377 = 1;
    }
    if ((found == 0 || restricted != 0) && ai_npc_enemy_scan(self, 5, found_list) == 0) {
        return 0;
    }
    enemy = &enemies[found_list[0]];
    dist = vec3_dist_sq(&self->pos_0x178, &enemy->pos);
    range = ai_npc_engage_range_get(self);
    if ((dist <= ai_npc_engage_range_get(self) * range || self->field_0x38A > 0) && self->field_0x38E == 0) {
        self->enemy_index = enemy->group;
        self->field_0x1CC = 4;
        self->field_0x38A = 300;
    } else if (dist <= 4000000.0f) {
        self->enemy_index = enemy->group;
        if (self->field_0x38E == 1 || self->field_0x377 == 0) {
            self->field_0x1CC = 5;
        } else {
            self->field_0x1CC = 4;
        }
    } else if (self->field_0x377 == 1) {
        self->enemy_index = enemy->group;
        self->field_0x1CC = 5;
    } else {
        self->enemy_index = 0xFF;
        self->field_0x1C8 = 0;
        return 0;
    }
    self->field_0x1C8 = 0;
    return 1;
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

#pragma fp_contract off
/* 0x802CC794 (0x5FC): places the move target around the engaged enemy: watching (5) on the player's side of it,
 * attacking (4) at its reach plus a random spread, swinging the other way round when a wall is in between. */
#pragma opt_common_subs off
void ai_npc_move_target_set(_AINPC_W* self)
{
    _ENEMY_WORK* enemies;
    _ENEMY_WORK* enemy;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 offset2;
    nw4r::math::VEC3 base;
    nw4r::math::VEC3 hit;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 target2;
    f32 reach;
    f32 scale;
    s32 hits;
    u32 bearing;
    u32 to_self;
    s8 spread;

    enemies = (_ENEMY_WORK*)::get_move_work_adrs(3);
    VEC3_ctor(&offset);
    VEC3_ctor(&offset2);
    VEC3_ctor(&base);
    VEC3_ctor(&hit);
    if (self->field_0x376 != 0) {
        return;
    }
    enemy = &enemies[self->enemy_index];
    switch (self->field_0x1CC) {
    case 5:
        bearing = ::calcVecAng2(&self->plw_0x16C->vec_0x03C, &enemy->pos);
        if (self->field_0x378 == 0) {
            if (ai_npc_buff_item_pick(self) != 0) {
                self->field_0x3DE = 1;
            }
            self->field_0x37C = ::ran_suu(1) % 200U;
            self->field_0x380 = (u16)(s32)(0.5f + (65536.0f * (::ran_suu(1) % 90 - 45)) / 360.0f);
            self->field_0x378 = 1;
        }
        reach = 500.0f + self->field_0x37C;
        copyVec3(&base, &self->plw_0x16C->vec_0x03C);
        break;
    case 4:
        if (self->field_0x447 == 1 && self->field_0x377 == 1) {
            if (pl_area_spot_valid_ck(self->area, &self->pos_0x178, self->field_0x326) == 1 &&
                (s8)self->field_0x448 < 5) {
                self->field_0x447 = 2;
            }
        } else if (ai_npc_weak_point_roll(self) != 0) {
            if (self->field_0x382 == 0) {
                self->field_0x382 = 1;
                ai_npc_formation_roll(self);
            }
            self->field_0x1CC = 1;
            self->field_0x1C8 = (u32)ai_npc_weak_point_pick(self, self->field_0x384);
            return;
        }
        bearing = ::calcVecAng2(&enemy->pos, &self->plw_0x16C->vec_0x03C);
        if (self->field_0x377 != 0) {
            reach = 600.0f;
        } else {
            reach = 400.0f;
        }
        if (self->field_0x420 == 5) {
            reach = 1000.0f;
        }
        if (self->field_0x378 == 0) {
            self->field_0x37C = ::ran_suu(1) % 200U;
            scale = ::get_em_chg_scale(enemy);
            self->field_0x37C += ::get_enemy_data(enemy)->size_0x54->radius_0x24 * scale;
            spread = 45;
            switch (self->field_0x420) {
            case 1:
            case 2:
            case 3:
                spread = -45;
                break;
            }
            if (ai_npc_item_throw_roll(self) == 1) {
                self->field_0x34F = 1;
            } else if (ai_npc_buff_item_pick(self) != 0) {
                self->field_0x3DE = 1;
                spread = -45;
            }
            if (enemy->team == 20) {
                spread = -45;
            }
            self->field_0x380 = (u16)(s32)(0.5f + (65536.0f * (spread + ::ran_suu(1) % 90)) / 360.0f);
            self->field_0x378 = 1;
        }
        reach += self->field_0x37C;
        copyVec3(&base, &enemy->pos);
        break;
    default:
        return;
    }
    if (self->field_0x3DE == 1) {
        reach = 1000.0f;
    }
    setVector3(&offset, 0.0f, 0.0f, reach);
    ::rotVecY(&offset, bearing);
    copyVec3(&offset2, &offset);
    bearing = ::calcVecAng2(&enemy->pos, &self->plw_0x16C->vec_0x03C);
    to_self = ::calcVecAng2(&enemy->pos, &self->pos_0x178);
    if (to_self > bearing) {
        ::rotVecY(&offset, self->field_0x380);
    } else {
        ::rotVecY(&offset, -self->field_0x380);
    }
    addVec3(&target, &base, &offset);
    copyVec3(&self->pos_0x1B0, &target);
    if (self->variant == 2) {
        if (self->field_0x1CC == 4) {
            scale = ::get_em_chg_scale(enemy);
            reach = ::get_enemy_data(enemy)->size_0x54->radius_0x24 * scale;
            if (enemy->pos.y < self->plw_0x16C->vec_0x03C.y) {
                reach = -reach;
            }
            self->pos_0x1B0.y += reach;
        }
        hits = ::findInterSection(&self->pos_0x178, &self->pos_0x1B0, &hit, 3, 0xFFFF, self->area, 0x20, NULL);
    } else {
        hits = ::findInterSection(&self->pos_0x178, &self->pos_0x1B0, &hit, 1, 0xFFFF, self->area, 0x20, NULL);
    }
    if (hits > 0) {
        if (to_self > bearing) {
            ::rotVecY(&offset2, -self->field_0x380);
        } else {
            ::rotVecY(&offset2, self->field_0x380);
        }
        addVec3(&target2, &base, &offset2);
        copyVec3(&self->pos_0x1B0, &target2);
        if (self->variant == 2) {
            self->pos_0x1B0.y -= 2.0f * reach;
        }
    }
    if (self->variant == 2 && self->pos_0x1B0.y > -10.0f) {
        self->pos_0x1B0.y = -10.0f;
    }
}
#pragma opt_common_subs reset
#pragma fp_contract on

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802CCE18 (0xE8): engages the nearest enemy of the area (seen from the player with `flags` bit 3): stores its
 * index, rolls the formation and picks its weak point; clears the target when there is none. */
u32 ai_npc_enemy_engage(_AINPC_W* self, u8 flags)
{
    _ENEMY_WORK* enemies = (_ENEMY_WORK*)::get_move_work_adrs(3);
    u8 found_list[32];
    u8 scan = 5;

    if (flags & 8) {
        scan |= 8;
    }
    if (ai_npc_enemy_scan(self, scan, found_list) == 0) {
        self->enemy_index = 0xFF;
        self->field_0x1C8 = 0;
        return 0;
    }
    self->enemy_index = found_list[0];
    if (enemies[found_list[0]].area_no != self->area) {
        self->enemy_index = 0xFF;
        self->field_0x1C8 = 0;
        return 0;
    }
    ai_npc_formation_roll(self);
    self->field_0x1C8 = (u32)ai_npc_weak_point_pick(self, self->field_0x384);
    return 1;
}

/* 0x802CCF00 (0xD8): follows the engaged enemy: drops it (back to watching) once it is gone, dying or in another
 * area; otherwise takes its weak point (`aim` 1) or its position as the move target. */
void ai_npc_enemy_track(_AINPC_W* self, u32 aim)
{
    _ENEMY_WORK* enemies = (_ENEMY_WORK*)::get_move_work_adrs(3);
    _ENEMY_WORK* enemy;

    if (self->enemy_index != 0xFF) {
        enemy = &enemies[self->enemy_index];
        if (enemy->active == 0 || enemy->action == 11 || enemy->action == 12 || enemy->area_no != self->area) {
            self->enemy_index = 0xFF;
            self->field_0x1C8 = 0;
            fn_802D4218(self);
            self->field_0x1CC = 5;
        } else if ((u8)aim == 1 && self->field_0x1C8 != 0) {
            ai_npc_weak_point_pos_get(self, (struct EmWeakPoint*)self->field_0x1C8, enemy, &self->pos_0x1B0);
        } else {
            copyVec3(&self->pos_0x1B0, &enemy->pos);
        }
    }
}

/* 0x802CCFD8 (0x74): sets the move target 800 ahead of the player along its facing. */
void ai_npc_target_ahead_set(_AINPC_W* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 target;

    VEC3_ctor(&offset);
    setVector3(&offset, 0.0f, 0.0f, 800.0f);
    ::rotVecY(&offset, self->plw_0x16C->field_0x058);
    addVec3(&target, &self->plw_0x16C->vec_0x03C, &offset);
    copyVec3(&self->pos_0x1B0, &target);
}

/* 0x802CD04C (0x1C0): the counter-attack watch: while the player is down (+0x492 with a hold, or act group 6) it
 * marks a near enemy to strike ten frames later; then engages it. */
#pragma opt_propagation off
#pragma opt_lifetimes off
u32 ai_npc_counter_target_ck(_AINPC_W* self)
{
    _ENEMY_WORK* enemies = (_ENEMY_WORK*)::get_move_work_adrs(3);
    _PLW* plw = self->plw_0x16C;
    _ENEMY_WORK* enemy;
    u8 down = 0;

    if (plw->hold_kind_0x492 != 0xFF && plw->hold_time_0x4C0 > 0.0f) {
        down = 1;
    } else if (plw->field_0x00A == 6) {
        down = 1;
    }
    if (self->field_0x34A == 0) {
        if (self->field_0x34B == 1) {
            self->enemy_index = self->field_0x34C;
            ai_npc_formation_roll(self);
            self->field_0x1C8 = (u32)ai_npc_weak_point_pick(self, self->field_0x384);
            enemy = &enemies[self->enemy_index];
            if (enemy->active == 0 || enemy->action == 11 || enemy->action == 12 || enemy->area_no != self->area) {
                self->enemy_index = 0xFF;
                self->field_0x1C8 = 0;
                self->field_0x34B = 0;
                return 0;
            }
            self->field_0x34B = 2;
            fn_802D4224(self);
            return 1;
        }
        if (down == 1 && self->field_0x349 == 0 && fn_802D3F70(self, 30, 5) != 0 && ai_npc_enemy_engage(self, 8) == 1) {
            enemy = &enemies[self->enemy_index];
            if (ai_npc_in_range_ck(&plw->vec_0x03C, &enemy->pos, 800.0f) == 1) {
                self->field_0x34C = self->enemy_index;
                self->field_0x34A = 10;
                self->field_0x34B = 1;
            }
            return 0;
        }
    }
    self->field_0x349 = down;
    return 0;
}
#pragma opt_lifetimes reset
#pragma opt_propagation reset

/* 0x802CD348 (0x240): looks for a carving or mining spot within reach (kinds 0, 3 and 4); when the formation's
 * roll allows, sets the move target beside it and claims it, else releases it. */
u32 ai_npc_carve_spot_find(_AINPC_W* self)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 spare;
    nw4r::math::VEC3 spot;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 target;
    f32 radius;
    u32 pitch;
    u32 yaw;
    s16 kind;
    f32 reach;
    f32 height;

    VEC3_ctor(&dir);
    VEC3_ctor(&spare);
    VEC3_ctor(&spot);
    if (::ai_skill_ck((::_AINPC_W*)self, 0x13) == 1) {
        return 0;
    }
    if (self->field_0x35C == 1) {
        return 1;
    }
    if (self->field_0x1CC == 1) {
        return 0;
    }
    if (self->field_0x35E > 0) {
        return 0;
    }
    if (self->field_0x1CF == 0) {
        return 0;
    }
    reach = 800.0f;
    if (::ai_skill_ck((::_AINPC_W*)self, 0x14) == 1) {
        reach += 400.0f;
    }
    if (self->field_0x420 == 5) {
        reach += 400.0f;
    }
    if (self->variant == 2) {
        height = reach;
    } else {
        height = 800.0f;
    }
    if (em_roster_area_near_find(self->area, &self->pos_0x178, reach, &kind, &spot, &self->field_0x364, &radius) !=
            0xFFFF &&
        lb_fabsf(self->pos_0x178.y - spot.y) < height && ((u32)(kind - 3) <= 1 || kind == 0)) {
        self->field_0x35E = 450;
        if (fn_802D4020(self) == 1) {
            self->field_0x35C = 1;
            subVec3(&diff, &self->pos_0x178, &spot);
            copyVec3(&dir, &diff);
            ::calcVecAngXY(&dir, &pitch, &yaw);
            setVector3(&dir, 0.0f, 0.0f, 0.8f * radius);
            ::rotVecX(&dir, pitch);
            ::rotVecY(&dir, yaw);
            addVec3(&target, &spot, &dir);
            copyVec3(&self->pos_0x1B0, &target);
            copyVec3(&self->pos_0x368, &spot);
            return 1;
        }
        em_roster_area_use_take(self->area, &spot, self->field_0x364);
    }
    return 0;
}


/* 0x802CD770 (0x2AC): notes the player's situation in +0x468 and latches a carry spot (+0x451/+0x454) when the
 * player picks the companion up. */
void ai_npc_player_watch(_AINPC_W* self)
{
    u8 watch = 0;
    u32 eating = 0;
    u32 drinking = 0;
    _PLW* plw = self->plw_0x16C;
    SysQuestMove* quest = (SysQuestMove*)::get_move_work_adrs(0);

    do {
        switch ((s16)pl_act_class_get(plw)) {
        case 5:
            watch = 1;
            break;
        case 3:
            watch = 2;
            break;
        case 4:
        case 7:
            watch = 3;
            break;
        case 6:
            watch = 4;
            break;
        case 8:
            watch = 5;
            break;
        case 9:
            watch = 6;
            break;
        case 10:
            watch = 7;
            break;
        case 11:
            watch = 8;
            break;
        case 12:
            watch = 9;
            break;
        case 1:
        case 2:
            watch = 10;
            break;
        }
        if (fn_8027DCE0(plw, 0) == 1) {
            eating = 1;
        }
        if (eating == 1 && fn_8027DCE0(plw, 1) == 0) {
            if (self->field_0x468 == 12 || self->field_0x46A != 0) {
                break;
            }
            watch = 12;
        } else if (eating == 1 && fn_8027DCE0(plw, 2) == 0) {
            if (self->field_0x468 == 16 || self->field_0x46A != 0) {
                break;
            }
            watch = 16;
        } else if (eating == 1) {
            watch = 8;
        }
        if (pl_act_range2_ck(plw, 0) == 1) {
            drinking = 1;
        }
        if (drinking == 1 && pl_act_range2_ck(plw, 1) == 0) {
            if (self->field_0x468 == 12 || self->field_0x46A != 0) {
                break;
            }
            watch = 12;
        } else if (drinking == 1 && pl_act_range2_ck(plw, 2) == 0) {
            if (self->field_0x468 == 16 || self->field_0x46A != 0) {
                break;
            }
            watch = 16;
        } else if (drinking == 1) {
            watch = 6;
        }
        if (pl_act_84_86_ck(plw) == 1 && self->field_0x46A == 0) {
            watch = 13;
        }
        switch (quest->result_0x0FA) {
        case 4:
            watch = 14;
            break;
        case 6:
        case 7:
            watch = 15;
            break;
        }
        self->field_0x468 = watch;
    } while (0);
    if ((::Pl_act_ck(plw, 0, 60) == 1 || ::Pl_act_ck(plw, 0, 80) != 0) && plw->flag_0x30E != 0 &&
        self->field_0x445 == 0 && self->field_0x451 == 0) {
        self->field_0x451 = 1;
        copyVec3(&self->pos_0x454, &plw->vec_0x03C);
    }
}

/* 0x802CDA1C (0x9C): refreshes the move target by the plan (+0x1CC): the player, the engaged enemy's weak point or
 * the carry spot; only while +0x389 allows and the record is near the player. */
void ai_npc_target_refresh(_AINPC_W* self)
{
    if (self->field_0x389 != 0 && (ai_npc_mode1_ck(self) != 1 || self->area == self->plw_0x16C->area_0x16)) {
        switch (self->field_0x1CC) {
        case 0:
            ai_npc_target_player_set(self);
            break;
        case 1:
            ai_npc_enemy_track(self, 1);
            break;
        case 9:
            copyVec3(&self->pos_0x1B0, &self->pos_0x454);
            break;
        }
    }
}

/* 0x802CDB10 (0x214): decides the plan (+0x1CC): follow (0), fight (1), gather (2), carve (3), watch (4/5), plan
 * 6, support item (7), carried (9). */
void ai_npc_plan_decide(_AINPC_W* self)
{
    if (self->field_0x440 == 1) {
        self->field_0x1CC = 0;
        return;
    }
    if (ai_npc_counter_target_ck(self) != 1 && self->field_0x1CD != 0) {
        if (fn_802CDAB8(self) == 1) {
            self->field_0x1CC = 6;
        } else if (fn_802CD588(self) == 1) {
            self->field_0x1CC = 0;
        } else if (fn_802CCD90(self) == 1) {
            if (ai_npc_enemy_engage(self, 0) == 1) {
                self->field_0x1CC = 1;
            } else {
                self->field_0x1CC = 0;
            }
        } else if (fn_802CD6FC(self) == 1) {
            self->field_0x1CC = 0;
        } else if (fn_802CD75C(self) == 1) {
            self->field_0x1CC = 0;
        } else if (self->field_0x34B == 2) {
            self->field_0x1CC = 1;
        } else if (fn_802CD2E0(self) == 1) {
            self->field_0x1CC = 0;
        } else if (ai_npc_support_item_pick(self) == 1) {
            self->field_0x1CC = 7;
            ai_npc_target_ahead_set(self);
        } else if (ai_npc_carried_ck(self) == 1) {
            self->field_0x1CC = 9;
        } else {
            if (self->field_0x358 == 0 && self->field_0x35C == 0) {
                if (ai_npc_enemy_target_pick(self) == 1) {
                    ai_npc_move_target_set(self);
                } else {
                    self->field_0x1CC = 0;
                    fn_802D4238(self);
                }
            }
            if (ai_npc_carve_spot_find(self) == 1) {
                self->field_0x1CC = 3;
            } else if (ai_npc_gather_spot_find(self) == 1) {
                self->field_0x1CC = 2;
            } else if (fn_802D7A74(self) == 1) {
                self->field_0x1CC = 0;
                self->field_0x440 = 2;
            }
        }
    }
}

/* 0x802CDD24 (0xA0): plan arm 0 - starts the 300-range retreat (motion 3 step 18), then steps back (step 3), then
 * closes the plan. */
void ai_npc_plan_retreat_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 3, 0x12, 0);
        self->field_0x1D0++;
        break;
    case 1:
        fn_802D2A00(self, 3, 3, 0);
        self->field_0x1D0++;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802CE698 (0xE0): plan arm 39 - walks up to the player (motion 2 step 7) and cheers (step 25) when the player
 * eats nearby. */
void ai_npc_plan39_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 7, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (fn_8027D74C(self->plw_0x16C) == 1 &&
            ai_npc_in_range_ck(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, 300.0f) == 1) {
            fn_802D2A00(self, 1, 0x19, 0);
        }
        self->field_0x460 = 0;
        self->field_0x1D0++;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE778 (0xC8): plan arm 40 - walks up to the player and plays step 88 nearby. */
void ai_npc_plan40_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 7, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (ai_npc_in_range_ck(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, 300.0f) == 1) {
            fn_802D2A00(self, 1, 0x58, 0);
        }
        self->field_0x1D0++;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE840 (0xE0): plan arm 41 - plan arm 39 in the alternate variant (step 53). */
void ai_npc_plan41_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 0xB, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (fn_8027D74C(self->plw_0x16C) == 1 &&
            ai_npc_in_range_ck(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, 300.0f) == 1) {
            fn_802D2A00(self, 1, 0x35, 0);
        }
        self->field_0x460 = 0;
        self->field_0x1D0++;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE920 (0xC8): plan arm 42 - plan arm 40 in the alternate variant (step 89). */
void ai_npc_plan42_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 0xB, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (ai_npc_in_range_ck(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, 300.0f) == 1) {
            fn_802D2A00(self, 1, 0x59, 0);
        }
        self->field_0x1D0++;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE9E8 (0x5A8): plan arms 43 and 44 - reacts to the player's noted situation (+0x468): comes closer, then
 * plays the matching motion step (cheer, dance, item use, flee, ...). */
void ai_npc_plan43_arm(_AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 target;
    f32 reach;

    VEC3_ctor(&offset);
    switch (self->field_0x1D0) {
    case 0:
        self->field_0x1D0++;
        do {
            switch (self->field_0x468) {
            case 8:
                reach = 280.0f;
                break;
            case 13:
                continue;
            default:
                reach = 180.0f;
                break;
            }
            if (self->field_0x1C0 > reach) {
                fn_802D4230(self, reach);
                if (arg == 0) {
                    fn_802D2A00(self, 2, 7, 0);
                } else {
                    fn_802D2A00(self, 2, 0xB, 0);
                }
                return;
            }
        } while (0);
    case 1:
        self->field_0x1D0++;
        switch (self->field_0x468) {
        case 1:
            if (arg == 0) {
                fn_802D2A00(self, 0, 0, 0);
            } else {
                fn_802D2A00(self, 0, 1, 0);
            }
            break;
        case 2:
        case 16:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0x17, 0);
            } else {
                fn_802D2A00(self, 1, 0x36, 0);
            }
            if (self->field_0x468 == 16) {
                self->field_0x46A = 300;
                ai_npc_talk(self, (s8)self->field_0x482, 0x18, 0, 2);
            }
            break;
        case 3:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0xA, 0);
            } else {
                fn_802D2A00(self, 1, 0x16, 0);
            }
            break;
        case 4:
            if (arg == 0) {
                fn_802D2A00(self, 1, self->field_0x3F8[::ran_suu(1) % 4], 8);
            } else {
                fn_802D2A00(self, 1, ai_npc_alt_step_get(self->field_0x3F8[::ran_suu(1) % 4]), 8);
            }
            break;
        case 5:
            setVector3(&offset, 0.0f, 0.0f, 1000.0f);
            ::rotVecY(&offset, (u16)(self->plw_0x16C->field_0x058 + 0x8000));
            addVec3(&target, &self->pos_0x178, &offset);
            copyVec3(&self->pos_0x1B0, &target);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            if (arg == 0) {
                fn_802D2A00(self, 2, 4, 0);
            } else {
                fn_802D2A00(self, 2, 0xF, 0);
            }
            break;
        case 6:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0x12, 0);
            } else {
                fn_802D2A00(self, 1, 0x51, 0);
            }
            break;
        case 7:
        case 12:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0x18, 0);
            } else {
                fn_802D2A00(self, 1, 0x51, 0);
            }
            if (self->field_0x468 == 12) {
                if (::ran_suu(1) % 3 == 0) {
                    ai_npc_talk(self, (s8)self->field_0x482, 0x19, 0, 2);
                }
                self->field_0x46A = 300;
            }
            break;
        case 8:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0x13, 0);
            } else {
                fn_802D2A00(self, 0, 1, 0);
            }
            break;
        case 9:
            if (arg == 0) {
                if (::ran_suu(1) & 1) {
                    fn_802D2A00(self, 3, 0, 0);
                } else {
                    fn_802D2A00(self, 3, 8, 0);
                }
            } else {
                fn_802D2A00(self, 3, 9, 0);
            }
            break;
        case 10:
            if (arg == 0) {
                fn_802D2A00(self, 1, 1, 0);
            } else {
                fn_802D2A00(self, 0, 1, 0);
            }
            break;
        case 11:
            self->vec_0x190.y = self->plw_0x16C->field_0x058;
            fn_802D2A00(self, 1, 0x12, 0);
            break;
        case 13:
            fn_802D2A00(self, 1, 0x5D, 0);
            self->field_0x46A = 600;
            break;
        case 14:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0x17, 0x10);
            } else {
                fn_802D2A00(self, 1, 0x36, 0x10);
            }
            break;
        case 15:
            if (arg == 0) {
                fn_802D2A00(self, 1, 0x18, 0x10);
            } else {
                fn_802D2A00(self, 1, 0x51, 0x10);
            }
            break;
        }
        break;
    case 2:
        self->field_0x389 = 1;
        self->field_0x468 = 0;
        fn_802D4200(self);
        break;
    }
}

/* 0x802CEF90 (0x200): plan arm 49 - the carry: walks to the player while it holds the companion, gets thrown
 * (step 91), runs 800 off along the throw and recovers (step 6). */
void ai_npc_plan49_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 offset;
    u32 angle;

    VEC3_ctor(&offset);
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x451 == 0) {
            self->field_0x1D0 = 4;
            break;
        }
        if (self->field_0x1C0 > 200.0f) {
            fn_802D4230(self, 200.0f);
            fn_802D2A00(self, 2, 8, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (self->plw_0x16C->flag_0x30E == 0 ||
            ai_npc_in_range_ck(&self->pos_0x178, &self->pos_0x1B0, 400.0f) == 0) {
            if (self->plw_0x16C->flag_0x30E == 0) {
                self->field_0x451 = 0;
            }
            self->field_0x1D0 = 4;
            break;
        }
        fn_802D2A00(self, 1, 0x5B, 0);
        self->field_0x1D0++;
        break;
    case 2:
        self->field_0x389 = 0;
        angle = ::calcVecAng2(&self->pos_0x1B0, &self->pos_0x178);
        setVector3(&offset, 0.0f, 0.0f, 800.0f);
        ::rotVecY(&offset, angle);
        self->pos_0x1B0.x = self->pos_0x178.x + offset.x;
        self->pos_0x1B0.y = self->pos_0x178.y;
        self->pos_0x1B0.z = self->pos_0x178.z + offset.z;
        fn_802D4230(self, 200.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->field_0x1D0++;
        break;
    case 3:
        fn_802D4230(self, 20.0f);
        fn_802D2A00(self, 1, 6, 0x20);
        self->field_0x1D0++;
        break;
    case 4:
        self->field_0x389 = 1;
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF190 (0x200): plan arm 50 - the carry in the alternate variant (steps 92 and 16). */
void ai_npc_plan50_arm(_AINPC_W* self)
{
    nw4r::math::VEC3 offset;
    u32 angle;

    VEC3_ctor(&offset);
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x451 == 0) {
            self->field_0x1D0 = 4;
            break;
        }
        if (self->field_0x1C0 > 200.0f) {
            fn_802D4230(self, 200.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (self->plw_0x16C->flag_0x30E == 0 ||
            ai_npc_in_range_ck(&self->pos_0x178, &self->pos_0x1B0, 400.0f) == 0) {
            if (self->plw_0x16C->flag_0x30E == 0) {
                self->field_0x451 = 0;
            }
            self->field_0x1D0 = 4;
            break;
        }
        fn_802D2A00(self, 1, 0x5C, 0);
        self->field_0x1D0++;
        break;
    case 2:
        self->field_0x389 = 0;
        angle = ::calcVecAng2(&self->pos_0x1B0, &self->pos_0x178);
        setVector3(&offset, 0.0f, 0.0f, 800.0f);
        ::rotVecY(&offset, angle);
        self->pos_0x1B0.x = self->pos_0x178.x + offset.x;
        self->pos_0x1B0.y = self->pos_0x178.y;
        self->pos_0x1B0.z = self->pos_0x178.z + offset.z;
        fn_802D4230(self, 200.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->field_0x1D0++;
        break;
    case 3:
        fn_802D4230(self, 20.0f);
        fn_802D2A00(self, 1, 0x10, 0x20);
        self->field_0x1D0++;
        break;
    case 4:
        self->field_0x389 = 1;
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF588 (0x17C): plan arm 7 - the guard: closes in (on the player while +0x38E holds), then either feints
 * (step 2) or tracks the enemy and strikes. */
void ai_npc_plan7_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x38E != 0) {
            fn_802D4230(self, 50.0f);
            copyVec3(&self->pos_0x1B0, &self->plw_0x16C->vec_0x03C);
            fn_802D2A00(self, 2, 3, 0);
        } else if (self->field_0x1C0 > 600.0f) {
            fn_802D4230(self, 50.0f);
            fn_802D2A00(self, 2, 4, 0);
        } else {
            fn_802D4230(self, 50.0f);
            fn_802D2A00(self, 2, 6, 0);
        }
        self->field_0x1D0++;
        break;
    case 1:
        if (::ran_suu(1) & 1) {
            fn_802D2A00(self, 1, 2, 0);
            self->field_0x1D0 = 3;
        } else {
            ai_npc_enemy_track(self, 0);
            fn_802D2A00(self, 2, 5, 0);
            self->field_0x1D0++;
        }
        break;
    case 2:
        fn_802D2A00(self, 1, 1, 0);
        self->field_0x1D0++;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CF808 (0xDC): plan arm 11 - the alternate-variant guard (steps 14 and 21). */
void ai_npc_plan11_arm(_AINPC_W* self)
{
    switch (self->field_0x1D0) {
    case 0:
        if (self->field_0x38E != 0) {
            fn_802D4230(self, 50.0f);
            copyVec3(&self->pos_0x1B0, &self->plw_0x16C->vec_0x03C);
            fn_802D2A00(self, 2, 0x12, 0);
        }
        fn_802D4230(self, 50.0f);
        fn_802D2A00(self, 2, 0xE, 0);
        self->field_0x1D0++;
        break;
    case 1:
        fn_802D2A00(self, 1, 0x15, 0);
        self->field_0x1D0++;
        break;
    case 2:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CF9E8 (0xD4): plan arms 9 and 33 - the buff item: runs to the player and uses the picked buff item (the
 * dispatcher's argument is not read). */
void ai_npc_plan9_arm(_AINPC_W* self, u8 arg)
{
    u8 step = self->field_0x1D0;

    switch (step) {
    case 0:
        self->field_0x1D0++;
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 9, 0);
        break;
    case 1:
        copyVec3(&self->pos_0x1B0, &self->plw_0x16C->vec_0x03C);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x1D0++;
        break;
    case 2:
        ai_npc_buff_item_use(self);
        self->field_0x3DE = 0;
        self->field_0x1D0++;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFAF0 (0xD8): plan arms 18 and 34 - the buff item in the alternate variant (the argument is not read). */
void ai_npc_plan18_arm(_AINPC_W* self, u8 arg)
{
    switch (self->field_0x1D0) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 0x13, 0);
        self->field_0x1D0++;
        break;
    case 1:
        copyVec3(&self->pos_0x1B0, &self->plw_0x16C->vec_0x03C);
        fn_802D2A00(self, 2, 0x10, 0);
        self->field_0x1D0++;
        break;
    case 2:
        ai_npc_buff_item_use(self);
        self->field_0x3DE = 0;
        self->field_0x1D0++;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802D0C9C (0x130): runs the arm of the record's plan (+0x1CE); 51 and above have none. */
void ai_npc_plan_dispatch(_AINPC_W* self)
{
    switch ((u8)self->field_0x1CE) {
    case 0:
        ai_npc_plan_retreat_arm(self);
        break;
    case 1:
        fn_802CDDC4(self, 0);
        break;
    case 2:
        fn_802CDF4C(self);
        break;
    case 3:
        fn_802CE2A0(self);
        break;
    case 4:
        fn_802CE3B4(self);
        break;
    case 5:
        fn_802CE4EC(self);
        break;
    case 6:
        fn_802CF390(self);
        break;
    case 7:
        ai_npc_plan7_arm(self);
        break;
    case 8:
        fn_802CF704(self);
        break;
    case 9:
        ai_npc_plan9_arm(self, 0);
        break;
    case 10:
        fn_802CE618(self);
        break;
    case 11:
        ai_npc_plan11_arm(self);
        break;
    case 12:
        fn_802CF8E4(self);
        break;
    case 13:
        fn_802CDF78(self);
        break;
    case 14:
        fn_802CE0D8(self);
        break;
    case 15:
        fn_802CE170(self);
        break;
    case 16:
        fn_802CFABC(self);
        break;
    case 17:
        fn_802CF48C(self);
        break;
    case 18:
        ai_npc_plan18_arm(self, 0);
        break;
    case 19:
        fn_802CFBC8(self);
        break;
    case 20:
        fn_802CFBF4(self);
        break;
    case 21:
        fn_802CFC94(self);
        break;
    case 22:
        fn_802CDDC4(self, 1);
        break;
    case 23:
        fn_802CFE20(self);
        break;
    case 24:
        fn_802CFFD0(self);
        break;
    case 25:
        fn_802D0068(self, 0);
        break;
    case 26:
        fn_802D0068(self, 1);
        break;
    case 27:
        fn_802D0148(self, 0);
        break;
    case 28:
        fn_802D0148(self, 1);
        break;
    case 29:
        fn_802D0228(self);
        break;
    case 30:
        fn_802D02C0(self);
        break;
    case 31:
        fn_802D0358(self, 0);
        break;
    case 32:
        fn_802D0358(self, 1);
        break;
    case 33:
        ai_npc_plan9_arm(self, 1);
        break;
    case 34:
        ai_npc_plan18_arm(self, 1);
        break;
    case 35:
        fn_802D04CC(self);
        break;
    case 36:
        fn_802D063C(self);
        break;
    case 37:
        fn_802D0A20(self);
        break;
    case 38:
        fn_802D0B28(self);
        break;
    case 39:
        ai_npc_plan39_arm(self);
        break;
    case 40:
        ai_npc_plan40_arm(self);
        break;
    case 41:
        ai_npc_plan41_arm(self);
        break;
    case 42:
        ai_npc_plan42_arm(self);
        break;
    case 43:
        ai_npc_plan43_arm(self, 0);
        break;
    case 44:
        ai_npc_plan43_arm(self, 1);
        break;
    case 45:
        fn_802D07AC(self);
        break;
    case 46:
        fn_802D0840(self);
        break;
    case 47:
        fn_802D0C30(self);
        break;
    case 48:
        fn_802D08D4(self);
        break;
    case 49:
        ai_npc_plan49_arm(self);
        break;
    case 50:
        ai_npc_plan50_arm(self);
        break;
    case 51:
        fn_802D0994(self);
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802D6BB0 (0x19C): applies the formation's two tuning rows (attack, defence, speed, gauge) at the record's
 * level (+0x33A) and derives the item gauge's refill period (+0x3DA/+0x3DC) from the speed and the gauge cap. */
void ai_npc_behaviour_apply(_AINPC_W* self)
{
    s32 speed = 20;
    u8 i;
    AINPCTuning* row;

    row = self->formation_0x41C->tuning_0x00;
    for (i = 0; i < 2; i++, row++) {
        switch (row->kind) {
        case 0:
            self->field_0x1FE = self->field_0x1FE * (row->values[self->field_0x33A] + 100) / 100;
            break;
        case 1:
            self->field_0x200 = self->field_0x200 * (row->values[self->field_0x33A] + 100) / 100;
            break;
        case 2:
            speed += row->values[self->field_0x33A];
            break;
        case 3:
            self->gauge_max_0x1F4 += row->values[self->field_0x33A];
            self->gauge_0x1F0 = self->gauge_max_0x1F4;
            self->gauge_refill_0x1F8 = self->gauge_max_0x1F4;
            break;
        }
    }
    self->field_0x3DC = self->field_0x3DA = (f32)(s32)(300.0f * (20.0f / speed)) / (self->field_0x3D6 / 50.0f);
}

/* 0x802D6D4C (0xA8): recomputes the attack (+0x1FA) from the base, the two bonuses and the percentage (+0x434),
 * halved with a quarter kept in +0x210 while +0x431 is set; +0x204 is its scale. */
void ai_npc_attack_update(_AINPC_W* self)
{
    s32 attack;

    attack = self->field_0x1FE;
    attack += self->field_0x234;
    attack += (s8)self->field_0x235;
    attack = attack * self->field_0x434 / 100;
    if (self->field_0x431 != 0) {
        self->field_0x210 = attack / 4;
        attack /= 2;
    }
    if (attack <= 0) {
        attack = 1;
    }
    self->field_0x1FA = attack;
    self->field_0x204 = (u16)attack / 100.0f;
}

/* 0x802D6DF4 (0x80): recomputes the defence (+0x1FC) from the base, the two bonuses and the percentage (+0x438);
 * +0x208 is half of it. */
void ai_npc_defense_update(_AINPC_W* self)
{
    s32 defense;

    defense = self->field_0x200;
    defense += self->field_0x238;
    defense += (s8)self->field_0x239;
    defense = defense * self->field_0x438 / 100;
    if (defense <= 0) {
        defense = 1;
    }
    self->field_0x1FC = defense;
    self->field_0x208 = 0.5f * (u16)defense;
}

/* 0x802D6E74 (0x1A0): picks the engaged enemy's nearest open weak point (skipping hidden, broken and disabled
 * ones, the tail of kind 20, and - on the ground - those above reach) or NULL. */
EmWeakPoint* ai_npc_weak_point_pick(_AINPC_W* self, u8 any_height)
{
    _ENEMY_WORK* enemy;
    EmWeakPoint* point;
    EmWeakPoint* best;
    nw4r::math::VEC3 pos;
    f32 nearest;
    f32 dist;

    enemy = (_ENEMY_WORK*)::get_move_work_adrs(3);
    best = NULL;
    VEC3_ctor(&pos);
    nearest = 0.0f;
    if (self->enemy_index == 0xFF) {
        return NULL;
    }
    enemy += self->enemy_index;
    point = (EmWeakPoint*)enemy->field_0x8C8;
    if (point != NULL) {
        for (; point->joint != -1; point++) {
            if (point->flags & 2) {
                continue;
            }
            if (point->flags & 0x100) {
                continue;
            }
            if (point->flags & 0x10) {
                continue;
            }
            if (enemy->team == 20 && (point->kind == 0 || point->kind == 5)) {
                continue;
            }
            if (em_flags836_ck(enemy, point->part) == 1) {
                continue;
            }
            ai_npc_weak_point_pos_get(self, point, enemy, &pos);
            if (self->variant != 2 && any_height == 0 && pos.y > 1000.0f + self->pos_0x178.y) {
                continue;
            }
            dist = ::calcDistanceSqXZ(&self->pos_0x178, &pos);
            if (nearest == 0.0f) {
                nearest = dist;
                best = point;
            } else if (nearest > dist) {
                nearest = dist;
                best = point;
            }
        }
    }
    return best;
}

/* 0x802D7014 (0x154): the world position of a weak point: its fixed position (joint 1000), the enemy's position
 * past its last joint, else its offset on the joint pushed out by its radius toward the companion. */
#pragma opt_common_subs off
void ai_npc_weak_point_pos_get(_AINPC_W* self, EmWeakPoint* point, _ENEMY_WORK* enemy, nw4r::math::VEC3* out)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 sum;
    u32 pitch;
    u32 yaw;

    MTX34_ctor(&mtx);
    VEC3_ctor(&joint);
    VEC3_ctor(&dir);
    if (point->joint == 1000) {
        out->x = point->offset.x;
        out->y = point->offset.y;
        out->z = point->offset.z;
        return;
    }
    if ((u32)point->joint >= ((MHchar*)enemy->char_0x024)->get_joint_num()) {
        copyVec3(out, &enemy->pos);
        return;
    }
    ::get_joint_wmat_em(enemy, point->joint, &mtx);
    joint.x = point->offset.x;
    joint.y = point->offset.y;
    joint.z = point->offset.z;
    ::mulVecMatAddTrans(&joint, &mtx);
    subVec3(&diff, &self->pos_0x178, &joint);
    copyVec3(&dir, &diff);
    ::calcVecAngXY(&dir, &pitch, &yaw);
    setVector3(&dir, 0.0f, 0.0f, point->radius);
    ::rotVecX(&dir, pitch);
    ::rotVecY(&dir, yaw);
    addVec3(&sum, &joint, &dir);
    copyVec3(out, &sum);
}
#pragma opt_common_subs reset

/* 0x802D7168 (0x70): whether to aim at a weak point: always once one was taken (+0x382), else a 25 % roll. */
s32 ai_npc_weak_point_roll(_AINPC_W* self)
{
    if (self->field_0x382 != 0) {
        return 1;
    }
    return ::ran_suu(1) % 100 < 25;
}

/* 0x802D71D8 (0x80): the item-throw roll against the chance of the record's progress row. */
u32 ai_npc_item_throw_roll(_AINPC_W* self)
{
    s16 chance = ai_npc_tuning_tbl[self->field_0x339 * 3];

    return ::ran_suu(1) % 100 < chance;
}

/* 0x802D7258 (0x94): the progress row's second (`which` 1) or first tuning value, as a float. */
f32 ai_npc_tuning_get(_AINPC_W* self, u32 which)
{
    switch ((u8)which) {
    case 1:
        return ai_npc_tuning_tbl[self->field_0x339 * 3 + 2];
    }
    return ai_npc_tuning_tbl[self->field_0x339 * 3 + 1];
}

/* 0x802D72EC (0x178): how many of an item the companion can hand out: always for the staple items, the honey
 * (29) left of ten in the desert formation, the item list's count for the motion in the forest formation. */
s16 ai_npc_item_stock_get(u16 item)
{
    _AINPC_W* self = &AINPC_VIEW;
    s16 left;
    u16* p;
    u8 count;
    u8 i;

    if (AINPC_VIEW.active == 0) {
        return 0;
    }
    switch (item) {
    case 26:
    case 27:
    case 28:
    case 30:
    case 31:
    case 32:
    case 239:
    case 240:
    case 45:
        return 1;
    }
    switch (self->field_0x420) {
    case 4:
        if (item == 29) {
            left = 10;
            if (self->slots[8].id == 29) {
                left = 10 - self->slots[8].count;
            }
            if (self->slots[9].id == 29) {
                left -= self->slots[9].count;
            }
            if (self->slots[10].id == 29) {
                left -= self->slots[10].count;
            }
            if (self->slots[11].id == 29) {
                left -= self->slots[11].count;
            }
            return left;
        }
        break;
    case 3:
        if (self->field_0x422 > 0) {
            return 0;
        }
        count = self->field_0x33A + 1;
        if (count > 4) {
            count = 4;
        }
        for (i = 0; i <= count; i++) {
            for (p = ai_npc_motion_variant_tbl[i]; *p != 0xFF; p++) {
                if (*p == item) {
                    return ai_npc_item_count_tbl[i];
                }
            }
        }
        break;
    }
    return 0;
}

/* 0x802D79A4 (0xAC): whether the companion stands by the player in motion 1 step 85 (within 200). */
u32 ai_npc_beside_player_ck(void)
{
    _AINPC_W* w = &AINPC_VIEW;

    if (::ainpc_w.active == 0) {
        return 0;
    }
    if (w->plw_0x16C == NULL) {
        return 0;
    }
    if (w->area != w->plw_0x16C->area_0x16) {
        return 0;
    }
    if (ai_npc_motion_step_ck(w, 1, 0x55) == 1 && vec3_dist_sq(&w->pos_0x178, &w->plw_0x16C->vec_0x03C) < 40000.0f) {
        return 1;
    }
    return 0;
}

/* 0x802D8478 (0x58): counts the frames the player stands idle on the ground up to 150 and then raises +0x383. */
void ai_npc_player_idle_count(_AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;

    if (plw->field_0x018 == 0 && plw->field_0x00A != 6) {
        if (++self->field_0x386 > 150) {
            self->field_0x386 = 150;
            self->field_0x383 = 1;
        }
    } else {
        self->field_0x383 = 0;
        self->field_0x386 = 0;
    }
}

/* 0x802D9004 (0xE4): during an element-pick quest, steps +0x484 through the three element picks and raises
 * +0x485 for each. */
void ai_npc_element_pick_update(_AINPC_W* self)
{
    SysQuestMove* quest = (SysQuestMove*)::get_move_work_adrs(0);
    QuestWork* work;

    if (quest != NULL && (work = quest->quest_0x0DC) != NULL) {
        switch (self->field_0x484) {
        case 0:
            if (quest_element_pick_ck(work, 2, 1) == 1) {
                self->field_0x484++;
                self->field_0x485 = 1;
            }
            break;
        case 1:
            if (quest_element_pick_ck(work, 1, 1) == 1) {
                self->field_0x484++;
                self->field_0x485 = 1;
            }
            break;
        case 2:
            if (quest_element_pick_ck(work, 0, 1) == 1) {
                self->field_0x484++;
                self->field_0x485 = 1;
            }
            break;
        }
    }
}

/* 0x802D90E8 (0x6C): the plan decision of an element-pick quest: report (8) after a pick, else fight or follow. */
void ai_npc_element_plan_decide(_AINPC_W* self)
{
    if (self->field_0x1CD != 0) {
        if (self->field_0x485 == 1) {
            self->field_0x1CC = 8;
        } else if (ai_npc_enemy_target_pick(self) == 1) {
            ai_npc_move_target_set(self);
        } else {
            self->field_0x1CC = 0;
        }
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802D7F58 (0x4BC): fills a companion status preview from a setup record: the ranks and base values of the
 * attack, defence and gauge, the formation's adjustments, the skills' bonuses and the speed rank. */
void ai_npc_status_preview(AINPCSetup* setup, AINPCStatus* status)
{
    s32 attack;
    s32 defense;
    s32 gauge;
    s32 attack_rate = 100;
    s32 defense_rate = 100;
    s32 gauge_rate = 100;
    s16 speed = 0;
    AINPCTuning* row;
    u8 i;
    s32 value;

    row = ai_npc_behaviour_get(setup->behaviour)->tuning_0x00;
    status->attack_rank = ai_npc_level_get(setup->attack_level);
    status->gauge_rank = fn_802D7C04(setup->gauge_level);
    status->rate = 100;
    status->gauge_bonus = 0;
    status->attack = fn_802D7BA4(status->attack_rank);
    status->status_kind = 0;
    status->attack_bonus = 0;
    status->status_attack = 0;
    status->status_bonus = 0;
    status->defense = fn_802D7BC4(status->attack_rank);
    status->defense_bonus = 0;
    status->gauge = fn_802D7C4C(status->gauge_rank);
    status->gauge_add = 0;
    status->level = fn_802D7C6C(setup->level);
    attack = status->attack;
    defense = status->defense;
    gauge = status->gauge;
    for (i = 0; i < 2; i++, row++) {
        switch (row->kind) {
        case 0:
            attack = attack * (row->values[status->level] + 100) / 100;
            break;
        case 1:
            defense = defense * (row->values[status->level] + 100) / 100;
            break;
        case 3:
            status->gauge_bonus = row->values[status->level];
            break;
        case 2:
            speed = (f32)speed + (f32)row->values[status->level];
            break;
        }
    }
    if (fn_802D7F10(setup->skills, 14) == 1) {
        attack_rate = 120;
    }
    if (fn_802D7F10(setup->skills, 15) == 1) {
        defense_rate = 120;
    }
    if (fn_802D7F10(setup->skills, 19) == 1) {
        attack_rate += 20;
        defense_rate += 20;
        gauge_rate = 120;
    }
    if (fn_802D7F10(setup->skills, 22) == 1) {
        attack_rate += 50;
        defense_rate += 50;
        gauge_rate += 50;
    }
    if (fn_802D7F10(setup->skills, 2) == 1) {
        status->status_kind = 1;
    }
    if (fn_802D7F10(setup->skills, 3) == 1) {
        status->status_kind = 2;
    }
    if (fn_802D7F10(setup->skills, 4) == 1) {
        status->status_kind = 3;
    }
    if (fn_802D7F10(setup->skills, 5) == 1) {
        status->status_kind = 4;
    }
    if (fn_802D7F10(setup->skills, 6) == 1) {
        status->status_kind = 5;
    }
    if (fn_802D7F10(setup->skills, 8) == 1) {
        status->status_kind = 6;
    }
    if (fn_802D7F10(setup->skills, 9) == 1) {
        status->status_kind = 7;
    }
    if (fn_802D7F10(setup->skills, 10) == 1) {
        status->status_kind = 8;
    }
    value = attack * attack_rate / 100;
    if (status->status_kind != 0) {
        status->status_attack = value / 4;
        value /= 2;
    }
    status->attack_bonus = value - status->attack;
    if (fn_802D7F10(setup->skills, 7) == 1) {
        status->status_bonus = status->status_attack * 130 / 100 - status->status_attack;
    } else if (fn_802D7F10(setup->skills, 11) == 1) {
        status->status_bonus = status->status_attack * 130 / 100 - status->status_attack;
    }
    status->defense_bonus = defense * defense_rate / 100 - status->defense;
    status->gauge_add = gauge * gauge_rate / 100 - status->gauge;
    if (speed < -6) {
        status->speed_rank = 4;
    } else if (speed < -2) {
        status->speed_rank = 3;
    } else if (speed < 2) {
        status->speed_rank = 0;
    } else if (speed < 6) {
        status->speed_rank = 1;
    } else {
        status->speed_rank = 2;
    }
}

/* 0x802D93B8 (0x48): the element-pick quest's per-frame step: plan, report and pick updates, then holds the
 * record's flag 1. */
void ai_npc_element_frame(_AINPC_W* self)
{
    ai_npc_element_plan_decide(self);
    ai_npc_element_report_update(self);
    ai_npc_element_pick_update(self);
    fn_802D2B88(self, 1);
}

/* 0x802D94C4 (0x80): reports an element pick from the quest (kind 0 or 1): arms the 90-frame report unless one is
 * already in flight (+0x485 == 2). */
void ainpc_element_report_set(u8 kind)
{
    _AINPC_W* self;

    if (quest_move_state_valid_ck() != 0) {
        self = &AINPC_VIEW;
        if (kind == 0) {
            self->field_0x484 = 1;
            self->field_0x486 = 90;
            if (self->field_0x485 == 2) {
                return;
            }
        } else {
            self->field_0x484 = 2;
            self->field_0x486 = 90;
        }
        self->field_0x485 = 0;
    }
}

/* 0x802D9C84 (0x5C): adds to the hunt bonus (kind 0, capped at 100) or the time bonus (capped at 10). */
void ai_npc_bonus_add(_AINPC_W* self, u8 kind, u32 amount)
{
    if (kind == 0) {
        if ((self->hunt_bonus += (u16)amount) >= 100) {
            self->hunt_bonus = 100;
        }
    } else {
        if ((self->time_bonus += (u16)amount) >= 10) {
            self->time_bonus = 10;
        }
    }
}

/* 0x802D9CE0 (0x34): whether the record is in motion 1 step 82, 83 or 87 (the item steps). */
u32 ai_npc_item_step_ck(_AINPC_W* self)
{
    if (self->motion == 1) {
        switch (self->motion_step) {
        case 82:
        case 83:
        case 87:
            return 1;
        }
    }
    return 0;
}

/* 0x802D9D14 (0x1C): the counter window of the formation: 150 in formations 4 and 5, else 75. */
s16 ai_npc_counter_window_get(_AINPC_W* self)
{
    switch (self->field_0x420) {
    case 4:
    case 5:
        return 150;
    }
    return 75;
}

/* 0x802D9D30 (0xB8): plays a companion voice line unless a talk of the report kinds is already up. */
u32 ai_npc_talk(_AINPC_W* self, s32 npc, u32 msg, u32 arg, u32 kind)
{
    if ((u32)((s8)npc - 9) > 1) {
        switch (self->field_0x483) {
        case 1:
            return 0;
        case 2:
        case 3:
            if ((s8)talk_msg_speaker_get() == 9 || (s8)talk_msg_speaker_get() == 10) {
                return 0;
            }
            break;
        }
    }
    return npc_talk_start((s8)npc, (u8)msg, (u16)arg, (u8)kind);
}

/* 0x802D9DE8 (0x2C): adds to a 16-bit counter, saturating at 0xFFFF. */
void u16_add_sat(u16* dst, s32 value)
{
    u32 sum = *dst + value;

    if (sum > 0xFFFF) {
        *dst = 0xFFFF;
    } else {
        *dst = sum;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802D9E20 (0x84): constructs the record's members: the model, its vectors and its land record. */
_AINPC_W* ainpc_w_construct(_AINPC_W* self)
{
    mhchar_construct(&self->model);
    VEC3_ctor(&self->pos_0x178);
    VEC3_ctor(&self->pos_0x184);
    VEC3_ctor(&self->pos_0x1B0);
    VEC3_ctor(&self->pos_0x1D4);
    VEC3_ctor(&self->pos_0x1E0);
    ::em_land_data_init((LandData*)&self->field_0x324);
    VEC3_ctor(&self->pos_0x368);
    VEC3_ctor(&self->pos_0x3E0);
    VEC3_ctor(&self->pos_0x454);
    VEC3_ctor(&self->drift_vel);
    return self;
}

/* 0x802D9E14 (0xC): the static initializer `.ctors` runs: constructs `ainpc_w`. */
void ainpc_w_sinit(void)
{
    ainpc_w_construct(&AINPC_VIEW);
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802D9154 (0x1A4): the element-pick report: near the player (within 2000) it voices the pick (3/4/5, or 9
 * for the last, which ends the quest 150 frames later); away from the player it calls out every 900 frames. */
void ai_npc_element_report_update(_AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;

    if (self->area == plw->area_0x16) {
        switch (self->field_0x485) {
        case 0:
            if (vec3_distance(&self->pos_0x178, &plw->vec_0x03C) <= 2000.0f) {
                switch (self->field_0x484) {
                case 0:
                    ai_npc_talk(self, 9, 3, 0, 2);
                    self->field_0x485 = 2;
                    break;
                case 1:
                    ai_npc_talk(self, 9, 4, 0, 2);
                    self->field_0x485 = 2;
                    break;
                case 2:
                    ai_npc_talk(self, 9, 5, 0, 2);
                    self->field_0x485 = 2;
                    break;
                case 3:
                    ai_npc_talk(self, 9, 9, 0, 2);
                    self->field_0x485 = 3;
                    self->field_0x486 = 150;
                    break;
                }
            }
            break;
        case 3:
            if (self->field_0x486 == 0) {
                pl_quest_end_request(1);
                self->field_0x485++;
            }
            break;
        }
    } else if (self->field_0x484 < 3 && self->field_0x486 == 0 && self->field_0x485 != 1 &&
               ai_npc_talk(self, 9, 0, 0, 2) == 1) {
        fn_802D9A44(self);
        self->field_0x486 = 900;
    }
}

/* 0x802D9A78 (0x20C): applies an area buff the player shares to the companion (kind 0..8: heal, attack, defence,
 * cure, ...) while it is with the player and not fainted. */
void ainpc_area_buff_apply(u8 kind)
{
    _AINPC_W* self;

    if (quest_move_state_valid_ck() != 0) {
        self = &AINPC_VIEW;
        if (AINPC_VIEW.active != 0 && self->plw_0x16C != NULL && self->area == self->plw_0x16C->area_0x16 &&
            self->motion != 5) {
            switch (kind) {
            case 0:
                fn_802D35B4(self, 10);
                fn_80114CC8(self, 0);
                break;
            case 4:
                fn_802D35B4(self, 30);
                fn_80114CC8(self, 0);
                break;
            case 5:
                fn_802D35B4(self, 50);
                fn_80114CC8(self, 0);
                break;
            case 1:
                fn_802D6964(self, 1, self->field_0x1FE * 3 / 100);
                self->field_0x236 = 5400;
                fn_80114CC8(self, 1);
                break;
            case 6:
                fn_802D6964(self, 1, self->field_0x1FE * 5 / 100);
                self->field_0x236 = 5400;
                fn_80114CC8(self, 1);
                break;
            case 2:
                fn_802D69A4(self, 1, 10);
                self->field_0x23A = 5400;
                fn_80114CC8(self, 2);
                break;
            case 7:
                fn_802D69A4(self, 1, 20);
                self->field_0x23A = 5400;
                fn_80114CC8(self, 2);
                break;
            case 3:
                self->field_0x21C = 0;
                fn_80114CC8(self, 4);
                break;
            case 8:
                fn_802D69E4(self, 1, 5400);
                fn_80114CC8(self, 9);
                break;
            }
        }
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802D9544 (0x4FC): the tutorial companion's coaching lines by the player's area: the greeting, the enemy's
 * status (trap, ailments, weak spots) and the next tutorial hint, each repeated after its own cooldown. */
void ai_npc_tutorial_talk(_AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    _ENEMY_WORK* enemies = (_ENEMY_WORK*)::get_move_work_adrs(3);
    u8 status;

    switch (plw->area_0x16) {
    case 0:
        switch (self->field_0x484) {
        case 0:
            break;
        case 1:
            if (self->field_0x486 == 0 && ai_npc_talk(self, 10, 0, 0, 2) == 1) {
                self->field_0x486 = 900;
            }
            break;
        default:
            if (self->field_0x486 == 0 && ai_npc_talk(self, 10, 1, 0, 2) == 1) {
                self->field_0x486 = 900;
            }
            break;
        }
        break;
    case 1:
        if (self->enemy_index != 0xFF) {
            status = em020_status_flags_get(&enemies[self->enemy_index]);
            if (self->field_0x485 != 2) {
                if ((status & 0x80) && ai_npc_talk(self, 10, 8, 0, 2) == 1) {
                    self->field_0x485 = 2;
                }
                if (self->field_0x483 == 2 && self->field_0x486 == 0) {
                    if (self->field_0x485 == 0) {
                        if (ai_npc_talk(self, 10, 7, 0, 2) == 1) {
                            self->field_0x486 = 3600;
                            self->field_0x485++;
                        }
                    } else if (ai_npc_talk(self, 10, 2, 0, 2) == 1) {
                        self->field_0x486 = 3600;
                    }
                }
            }
        }
        break;
    case 2:
        if (self->enemy_index != 0xFF) {
            status = em020_status_flags_get(&enemies[self->enemy_index]);
            switch (self->field_0x484) {
            case 1:
                if (self->field_0x485 == 2) {
                    break;
                }
                if ((status & 0x80) && ai_npc_talk(self, 10, 8, 0, 2) == 1) {
                    self->field_0x485 = 2;
                }
                if (status & 0x20) {
                    if (self->field_0x486 == 0 && self->field_0x483 == 2 && ai_npc_talk(self, 10, 2, 0, 2) == 1) {
                        self->field_0x486 = 2700;
                    }
                } else if ((status & 0x10) && self->field_0x486 == 0 && ai_npc_talk(self, 10, 9, 0, 2) == 1) {
                    self->field_0x486 = 1200;
                }
                break;
            }
        }
        break;
    case 3:
        if (self->enemy_index != 0xFF) {
            status = em020_status_flags_get(&enemies[self->enemy_index]);
            if (status & 0x40) {
                if (self->field_0x485 == 0 && ai_npc_talk(self, 10, 6, 0, 2) == 1) {
                    self->field_0x485++;
                    break;
                }
            } else {
                self->field_0x485 = 0;
            }
            if (self->field_0x483 == 2) {
                switch (self->field_0x484) {
                case 2:
                    if (status & 4) {
                        self->field_0x484 = 4;
                        self->field_0x486 = 60;
                    } else if (status & 2) {
                        self->field_0x484 = 3;
                        self->field_0x486 = 60;
                    }
                    if (self->field_0x486 == 0 && ai_npc_talk(self, 10, 2, 0, 2) == 1) {
                        self->field_0x486 = 1800;
                    }
                    break;
                case 3:
                    if (status & 4) {
                        self->field_0x484 = 4;
                        self->field_0x486 = 60;
                    }
                    if (self->field_0x486 == 0 && ai_npc_talk(self, 10, 3, 0, 2) == 1) {
                        self->field_0x486 = 1800;
                    }
                    break;
                case 4:
                    if (Pl_motion_input_ck(0) == 1) {
                        self->field_0x484 = 5;
                    }
                    if (self->field_0x486 == 0 && ai_npc_talk(self, 10, 4, 0, 2) == 1) {
                        self->field_0x486 = 1800;
                    }
                    break;
                }
            } else {
                switch (self->field_0x484) {
                case 2:
                    if (Pl_motion_input_ck(0) == 1) {
                        self->field_0x484++;
                    } else if ((status & 8) && self->field_0x486 == 0 && ai_npc_talk(self, 10, 5, 0, 2) == 1) {
                        self->field_0x486 = 1800;
                    }
                    break;
                }
            }
        }
        break;
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */


#pragma peephole off

extern "C" {

/* 0x802CCD90 - the 0x350-frame gate: once the countdown has run out it latches +0x354 and arms the
 * 0x708-frame re-check. */
s32 fn_802CCD90(struct _AINPC_W* self)
{
    if (self->field_0x354 != 0) {
        return 1;
    }
    if (self->field_0x350 >= ai_npc_counter_window_get()) {
        self->field_0x354 = 1;
        self->field_0x352 = 0x708;
        fn_802D4218(self);
        return 1;
    }
    return 0;
}

/* 0x802CCE04 - true while the gate byte is exactly 2. */
u32 fn_802CCE04(struct _AINPC_W* self)
{
    return self->field_0x354 == 2;
}

/* 0x802CD75C - true while the +0x468 flag is set. */
u32 fn_802CD75C(struct _AINPC_W* self)
{
    return self->field_0x468 != 0;
}

/* 0x802CDF4C - queue the variant's intro command. */
void fn_802CDF4C(struct _AINPC_W* self)
{
    if (self->variant == 2) {
        fn_802D2A00(self, 1, 0xF, 0);
    } else {
        fn_802D2A00(self, 1, 8, 0);
    }
}

/* 0x802CDF78 - the 4-step "recover from stagger" handler. */
void fn_802CDF78(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 150.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        if (fn_802D3184(self, 0x2000) == 0) {
            fn_802D2A00(self, 2, 0x10, 0);
            self->step += 1;
        } else {
            if ((u16)ran_suu(1) & 1) {
                fn_802D2A00(self, 3, 9, 0);
            } else {
                fn_802D2A00(self, 3, 0xD, 0);
            }
            self->step = 3;
        }
        break;
    case 2:
        if ((u16)ran_suu(1) & 1) {
            fn_802D2A00(self, 3, 9, 0);
        } else {
            fn_802D2A00(self, 3, 0xD, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE0D8 - the 3-step "approach" handler. */
void fn_802CE0D8(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 3, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE170 - rebuilds `vec_0x1B0` from `vec_0x178` rotated by the angle the two make (the rotated stack offset
 * `addVec3` turns into the new aim point), then queues the follow-up command. */
void fn_802CE170(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
    {
        nw4r::math::VEC3 offset;
        s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

        setVector3(&tmp, 0.0f, 0.0f, 800.0f);
        rotVecY(&tmp, angle);
        addVec3(&offset, &self->vec_0x178, &tmp);
        copyVec3(&self->vec_0x1B0, &offset);
        self->field_0x389 = 0;
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 3, 0x12, 0);
        self->step += 1;
        break;
    }
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0xA, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF390 - the 3-step "hold/flinch" handler. */
void fn_802CF390(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 7, 0);
        } else if (self->field_0x1C0 > 30.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 3, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (self->field_0x358 != 0) {
            fn_802D2A00(self, 1, 4, 0);
        } else if (self->field_0x35C != 0) {
            fn_802D2A00(self, 1, 0x1A, 0);
        }
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFABC - latch +0x3D2 and queue the variant's command. */
void fn_802CFABC(struct _AINPC_W* self)
{
    self->field_0x3D2 = 1;
    if (self->variant == 2) {
        fn_802D2A00(self, 5, 6, 0);
    } else {
        fn_802D2A00(self, 5, 5, 0);
    }
}

/* 0x802CFBC8 - queue the +0x440-dependent command. */
void fn_802CFBC8(struct _AINPC_W* self)
{
    if (self->field_0x440 == 1) {
        fn_802D2A00(self, 1, 0x54, 0);
    } else {
        fn_802D2A00(self, 1, 0x52, 0);
    }
}

/* 0x802CF704 - the 4-step "use item" handler. */
void fn_802CF704(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 8, 0);
        } else {
            fn_802D4230(self, 50.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFFD0 - the 3-step "guard" handler. */
void fn_802CFFD0(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D07AC - the 3-step "roar" handler. */
void fn_802D07AC(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0xA, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D08D4 - the 4-step "pick target" handler. */
void fn_802D08D4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CD6FC - true once the +0x460 latch is set or the player work reports state 1. */
u32 fn_802CD6FC(struct _AINPC_W* self)
{
    if (self->field_0x460 == 1) {
        return 1;
    }
    if (fn_8027D74C(self->plw_0x16C) == 1) {
        self->field_0x460 = 1;
        return 1;
    }
    return 0;
}

/* 0x802CDAB8 - true while the elapsed frame count has not passed the formation's budget-scaled
 * factor. */
u32 fn_802CDAB8(struct _AINPC_W* self)
{
    f32 budget = self->formation_0x41C->budget_0x0C;

    return self->field_0x1F0 <= (s32)((f32)self->field_0x1F4 * budget);
}

/* 0x802CDDC4 - the 4-step "switch to another action" handler. */
void fn_802CDDC4(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 130.0f);
        if (arg == 0) {
            fn_802D2A00(self, 2, 4, 0);
        } else {
            fn_802D2A00(self, 3, 0x12, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (fn_802D3184(self, 0x2000) == 0) {
            fn_802D2A00(self, 2, 5, 0);
            self->step += 1;
        } else {
            if (self->vec_0x1B0.y - self->vec_0x178.y > 180.0f) {
                fn_802D2A00(self, 3, 8, 0);
            } else {
                fn_802D32B4(self, lbl_805D4030[2]);
            }
            self->step = 3;
        }
        break;
    case 2:
        if (self->vec_0x1B0.y - self->vec_0x178.y > 180.0f) {
            fn_802D2A00(self, 3, 8, 0);
        } else {
            fn_802D32B4(self, lbl_805D4030[2]);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE2A0 - the distance-banded dispatcher behind the "watch" action. */
void fn_802CE2A0(struct _AINPC_W* self)
{
    if (self->field_0x1C0 > 700.0f) {
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 7, 0);
        return;
    }
    if (self->field_0x1C0 > 400.0f) {
        fn_802D4230(self, 250.0f);
        fn_802D2A00(self, 2, 2, 0);
        return;
    }
    if (self->field_0x1C0 > 200.0f) {
        fn_802D2A00(self, 2, 0, 0);
        return;
    }
    if (fn_802D3184(self, 0x5000) == 0 && self->field_0x1C0 > 50.0f) {
        fn_802D2A00(self, 2, 5, 0);
        return;
    }
    if (fn_802CCE04(self) == 1) {
        fn_802D32B4(self, lbl_805D4030[3]);
        return;
    }
    fn_802D32B4(self, lbl_805D4030[0]);
}

/* 0x802CE618 - the distance-banded dispatcher behind the "hold ground" action. */
void fn_802CE618(struct _AINPC_W* self)
{
    if (self->field_0x1C0 > 700.0f) {
        fn_802D4230(self, 400.0f);
        fn_802D2A00(self, 2, 0xB, 0);
        return;
    }
    if (self->field_0x1C0 > 300.0f) {
        fn_802D4230(self, 150.0f);
        fn_802D2A00(self, 2, 0xA, 0);
    }
}

/* 0x802D0068 - the 4-step "turn" handler. */
void fn_802D0068(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        if (arg == 0) {
            fn_802D2A00(self, 3, 6, 0);
        } else {
            fn_802D2A00(self, 3, 7, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0148 - the 4-step "turn" handler, the long variant. */
void fn_802D0148(struct _AINPC_W* self, u8 arg)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 1000.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        if (arg == 0) {
            fn_802D2A00(self, 3, 0xF, 0);
        } else {
            fn_802D2A00(self, 3, 0x10, 0);
        }
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0228 - the 3-step "ready stance" handler. */
void fn_802D0228(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 3, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0x13, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D02C0 - the 3-step "ready stance" handler, the long variant. */
void fn_802D02C0(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 800.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0x14, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0358 - the 2-step "sidestep" handler: it rotates the +0x178 aim-offset into +0x1B0. */
void fn_802D0358(struct _AINPC_W* self, u8 arg)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
    {
        s32 angle;

        ai_npc_enemy_track(self, 0);
        angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);
        setVector3(&v, 0.0f, 0.0f, 800.0f);
        if (arg == 0) {
            setVector3(&v, 0.0f, 0.0f, ai_npc_tuning_get(self, 0));
        } else {
            setVector3(&v, 0.0f, 0.0f, ai_npc_tuning_get(self, 1));
        }
        rotVecY(&v, angle);
        self->vec_0x1B0.x = self->vec_0x178.x + v.x;
        self->vec_0x1B0.y = self->vec_0x178.y;
        self->vec_0x1B0.z = self->vec_0x178.z + v.z;
        fn_802D4230(self, 30.0f);
        if (arg == 0) {
            fn_802D2A00(self, 1, 6, 0);
        } else {
            fn_802D2A00(self, 1, 0x10, 0);
        }
        self->step += 1;
        break;
    }
    case 1:
        fn_802D4200(self);
        fn_802D3A20(self);
        break;
    }
}

/* 0x802D04CC - the 4-step "step in" handler. */
void fn_802D04CC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 <= 400.0f) {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&v, 0.0f, 0.0f, 800.0f);
            rotVecY(&v, angle);
            self->vec_0x1B0.x = self->vec_0x178.x + v.x;
            self->vec_0x1B0.y = self->vec_0x178.y;
            self->vec_0x1B0.z = self->vec_0x178.z + v.z;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->field_0x389 = 0;
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 1);
        self->field_0x389 = 1;
        fn_802D2A00(self, 2, 5, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 3, 0x15, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D063C - the 4-step "step in" handler, the far variant. */
void fn_802D063C(struct _AINPC_W* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 <= 400.0f) {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&v, 0.0f, 0.0f, 800.0f);
            rotVecY(&v, angle);
            self->vec_0x1B0.x = self->vec_0x178.x + v.x;
            self->vec_0x1B0.y = self->vec_0x178.y;
            self->vec_0x1B0.z = self->vec_0x178.z + v.z;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->field_0x389 = 0;
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 1);
        self->field_0x389 = 1;
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 3, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0840 - the 3-step "roar" handler. */
void fn_802D0840(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0994 - the 2-step "post-roar" handler. */
void fn_802D0994(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->variant == 2) {
            fn_802D2A00(self, 1, 0x16, 0);
        } else {
            fn_802D2A00(self, 1, 0xA, 0);
        }
        self->step += 1;
        break;
    case 1:
        fn_802D4200(self);
        fn_802D3AD8(self);
        break;
    }
}

/* 0x802D0A20 - the 4-step "taunt" handler. */
void fn_802D0A20(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D4230(self, 500.0f);
        fn_802D2A00(self, 2, 4, 0);
        self->step += 1;
        break;
    case 2:
        if (pl_area_spot_valid_ck(self->field_0x1A4, &self->vec_0x178, self->field_0x326) == 1) {
            fn_802D2A00(self, 1, 0x1F, 0);
        }
        self->field_0x448 += 1;
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802D0B28 - the 4-step "taunt" handler, the long variant. */
void fn_802D0B28(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 30.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D4230(self, 500.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 2:
        if (pl_area_spot_valid_ck(self->field_0x1A4, &self->vec_0x178, self->field_0x326) == 1) {
            fn_802D2A00(self, 1, 0x3B, 0);
        }
        self->field_0x448 += 1;
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CD20C - the player-action predicate chain the state machines test. */
s16 fn_802CD20C(struct _AINPC_W* self)
{
    struct _PLW* plw = self->plw_0x16C;

    if (Pl_condition_ck(plw, 0x30A) == 1) {
        return 0;
    }
    if (Pl_condition_ck(plw, 4) == 1) {
        return 1;
    }
    if (Pl_condition_ck(plw, 0x40000) == 1) {
        return ai_torch_ck(self) != 1;
    }
    if (pl_act6_class_ck(plw, 0) == 1) {
        return 0;
    }
    return pl_act6_class_ck(plw, 1) == 1 ? 0 : -1;
}

/* 0x802CD2E0 - latch the +0x34D gate once the predicate chain answers non-negative. */
u32 fn_802CD2E0(struct _AINPC_W* self)
{
    s16 r;

    if (self->field_0x34D != 0) {
        return 1;
    }
    r = fn_802CD20C(self);
    if (r >= 0) {
        self->field_0x34D = 1;
        self->field_0x34E = (u8)r;
        return 1;
    }
    return 0;
}

/* 0x802CD588 - the stance/guard dispatcher. */
u32 fn_802CD588(struct _AINPC_W* self)
{
    u16 a;
    s16 b;

    if (self->field_0x461 == 1) {
        return 1;
    }
    if (self->field_0x43E <= 0) {
        self->field_0x43D = 0;
    }
    if (self->field_0x171 == 5) {
        return 0;
    }
    if (self->field_0x43D == 1) {
        self->field_0x43D = 2;
        fn_802D4218(self);
        switch (self->field_0x420) {
        case 1:
            fn_802D65CC(self);
            break;
        case 2:
            fn_802D675C(self);
            break;
        case 5:
            fn_802D67F8(self);
            break;
        case 3:
        case 4:
            if (fn_802D86D4(self, &a, &b) == 1) {
                if (a != 0) {
                    self->field_0x462 = a;
                    self->field_0x464 = b;
                    self->field_0x461 = 1;
                    ai_npc_talk(self, (s8)self->field_0x482, 0x12, 0, 2);
                }
            } else {
                ai_npc_talk(self, (s8)self->field_0x482, 0x11, 0, 2);
            }
            break;
        default:
            ai_npc_talk(self, (s8)self->field_0x482, 0x11, 0, 2);
            break;
        }
    }
    return self->field_0x43D != 0;
}

/* 0x802CE3B4 - the 4-step "flinch" handler. */
void fn_802CE3B4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 180.0f) {
            fn_802D4230(self, 180.0f);
            fn_802D2A00(self, 2, 8, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (fn_802CD20C(self) < 0) {
            fn_802D3B10(self);
            fn_802D4200(self);
        } else {
            fn_802D2A00(self, 2, 5, 0);
            self->step += 1;
        }
        break;
    case 2:
        if (self->field_0x34E == 0) {
            fn_802D2A00(self, 3, 0, 4);
        } else {
            fn_802D2A00(self, 3, 1, 4);
        }
        ai_npc_talk(self, (s8)self->field_0x482, 0x1B, 0, 2);
        self->step += 1;
        break;
    case 3:
        fn_802D3B10(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CE4EC - the 4-step "flinch" handler, the long variant. */
void fn_802CE4EC(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 180.0f);
        fn_802D2A00(self, 2, 0xB, 0);
        self->step += 1;
        break;
    case 1:
        if (fn_802CD20C(self) < 0) {
            fn_802D3B10(self);
            fn_802D4200(self);
        } else {
            fn_802D2A00(self, 2, 0x10, 0);
            self->step += 1;
        }
        break;
    case 2:
        if (self->field_0x34E == 0) {
            fn_802D2A00(self, 3, 9, 4);
        } else {
            fn_802D2A00(self, 3, 0xD, 4);
        }
        ai_npc_talk(self, (s8)self->field_0x482, 0x1B, 0, 2);
        self->step += 1;
        break;
    case 3:
        fn_802D3B10(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF48C - the 3-step "call" handler. */
void fn_802CF48C(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xB, 0);
        } else if (self->field_0x1C0 > 30.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xE, 0);
        }
        self->step += 1;
        break;
    case 1:
        if (self->field_0x358 != 0) {
            fn_802D2A00(self, 1, 0xE, 0);
        } else if (self->field_0x35C != 0) {
            fn_802D2A00(self, 1, 0x34, 0);
        }
        self->step += 1;
        break;
    case 2:
        fn_802D4200(self);
        break;
    }
}

/* 0x802CF8E4 - the 4-step "call" handler, the long variant. */
void fn_802CF8E4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 30.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        } else {
            fn_802D4230(self, 50.0f);
            fn_802D2A00(self, 2, 0xE, 0);
        }
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D2A00(self, 2, 0x10, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x16, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D4200(self);
        fn_802D4238(self);
        break;
    }
}

/* 0x802CFBF4 - the 3-step "call" handler, the short variant. */
void fn_802CFBF4(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D4230(self, 300.0f);
        fn_802D2A00(self, 2, 0xF, 0);
        self->step += 1;
        break;
    case 1:
        fn_802D2A00(self, 3, 0xE, 0);
        self->step += 1;
        break;
    case 2:
        fn_802D3B24(self);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFC94 - the 4-step "sidestep" handler. */
void fn_802CFC94(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 800.0f);
            fn_802D2A00(self, 2, 4, 0);
        } else {
            s32 angle = calcVecAng2(&self->vec_0x1B0, &self->vec_0x178);

            setVector3(&tmp, 0.0f, 0.0f, 800.0f);
            rotVecY(&tmp, angle);
            addVec3(&offset, &self->vec_0x178, &tmp);
            copyVec3(&self->vec_0x1B0, &offset);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 4, 0);
        }
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D2A00(self, 2, 5, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x39, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D2A00(self, 1, 0xA, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802CFE20 - the 4-step "sidestep" handler, the far variant. */
void fn_802CFE20(struct _AINPC_W* self)
{
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3 delta;
    nw4r::math::VEC3 offset;
    u32 x, z;

    VEC3_ctor(&tmp);
    switch (self->step) {
    case 0:
        if (self->field_0x1C0 > 800.0f) {
            fn_802D4230(self, 1100.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        } else {
            subVec3(&delta, &self->vec_0x178, &self->vec_0x1B0);
            copyVec3(&tmp, &delta);
            calcVecAngXY(&tmp, &x, &z);
            setVector3(&tmp, 0.0f, 0.0f, 800.0f);
            rotVecX(&tmp, x);
            rotVecY(&tmp, z);
            addVec3(&offset, &self->vec_0x178, &tmp);
            copyVec3(&self->vec_0x1B0, &offset);
            self->field_0x389 = 0;
            fn_802D4230(self, 20.0f);
            fn_802D2A00(self, 2, 0xF, 0);
        }
        self->step += 1;
        break;
    case 1:
        ai_npc_enemy_track(self, 0);
        fn_802D2A00(self, 2, 0x10, 0);
        self->field_0x389 = 1;
        self->step += 1;
        break;
    case 2:
        fn_802D2A00(self, 1, 0x3A, 0);
        self->step += 1;
        break;
    case 3:
        fn_802D2A00(self, 1, 0x16, 0);
        fn_802D4200(self);
        break;
    }
}

/* 0x802D0C30 - the 2-step "sleep" handler. */
void fn_802D0C30(struct _AINPC_W* self)
{
    switch (self->step) {
    case 0:
        fn_802D2A00(self, 1, 0x5A, 0);
        self->step += 1;
        break;
    case 1:
        self->field_0x485 = 0;
        fn_802D4200(self);
        break;
    }
}

#ifdef __cplusplus
}
#endif


extern "C" void fn_802D0DCC(DispatchState* self)
{
    switch (self->state) {
    default:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[15]);
        } else {
            fn_802D3398(self, lbl_805D4150[14]);
        }
        break;
    case 1:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[3]);
        } else {
            fn_802D3398(self, lbl_805D4150[2]);
        }
        break;
    case 2:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[5]);
        } else {
            fn_802D3398(self, lbl_805D4150[4]);
        }
        break;
    case 3:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[7]);
        } else {
            fn_802D3398(self, lbl_805D4150[6]);
        }
        break;
    case 4:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[9]);
        } else {
            fn_802D3398(self, lbl_805D4150[8]);
        }
        break;
    case 5:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[11]);
        } else {
            fn_802D3398(self, lbl_805D4150[10]);
        }
        break;
    case 6:
        if (self->variant == 2) {
            fn_802D3398(self, lbl_805D4150[13]);
        } else {
            fn_802D3398(self, lbl_805D4150[12]);
        }
        break;
    }
}

namespace view_fn_802D0F34 {

/* One entry of the motion table `lbl_805D3AD8` (`fn_802D2F7C` walks it with a 0x1A stride): the two
 * s16 distances and the u8 motion row `lbl_80792508` is indexed with. size: 0x1A */
struct MotionEntry {
    /* +0x00 */ s16 approach_0x00;
    /* +0x02 */ s16 retreat_0x02;
    /* +0x04 */ u8 unused_0x04[0x0F - 0x04];
    /* +0x0F */ u8 motion_id_0x0F;
    /* +0x10 */ u8 unused_0x10[0x1A - 0x10];
};

extern "C" {

/* 0x802D0F34 - the attack driver: the counter-attack countdown, the "blocked" arm and the whole
 * attack-plan dispatch on +0x1CC. */
void fn_802D0F34(struct _AINPC_W* self)
{
    u8* enemy_work = (u8*)::get_move_work_adrs(3);

    if (self->motion != 7 &&
        (ai_npc_mode1_ck(self) != 1 || self->area == self->plw_0x16C->area_0x16 ||
         self->field_0x485 == 1)) {
        if (self->field_0x1D1 != 0) {
            self->field_0x35C = 0;
            self->field_0x358 = 0;
            fn_802D3A20(self);
            if (self->field_0x1D1 == 1) {
                fn_802D4200(self);
                self->field_0x389 = 1;
                fn_802D41C8(self);
            }
            self->field_0x1D1 = 0;
        }
        if (fn_802D3DE8(self) != 1) {
            if (self->variant == 2) {
                self->field_0x1C0 = vec3_distance(&self->pos_0x1B0, &self->pos_0x178);
            } else {
                self->field_0x1C0 = calcVecDistXZ(&self->pos_0x1B0, &self->pos_0x178);
            }
            if (self->field_0x1CD != 0) {
                if (self->field_0x1CF != 0) {
                    fn_802D4238(self);
                    switch (self->field_0x1CC) {
                    case 0:
                        if (self->field_0x354 == 1) {
                            fn_802D41E0(self, 2, 0);
                            self->field_0x354 = 2;
                        } else if (self->field_0x354 == 3) {
                            fn_802D41E0(self, 0x33, 1);
                        } else if (self->variant == 2) {
                            if (self->field_0x460 == 1) {
                                fn_802D41E0(self, 0x29, 1);
                            } else if (self->field_0x461 == 1) {
                                fn_802D41E0(self, 0x2A, 1);
                            } else if (self->field_0x468 != 0) {
                                fn_802D41E0(self, 0x2C, 1);
                            } else if (self->field_0x34D == 1) {
                                fn_802D41E0(self, 5, 1);
                            } else {
                                fn_802D41E0(self, 0x0A, 0);
                            }
                        } else if (self->field_0x460 == 1) {
                            fn_802D41E0(self, 0x27, 1);
                        } else if (self->field_0x461 == 1) {
                            fn_802D41E0(self, 0x28, 1);
                        } else if (self->field_0x468 != 0) {
                            fn_802D41E0(self, 0x2B, 1);
                        } else if (self->field_0x440 != 0) {
                            fn_802D41E0(self, 0x13, 0);
                        } else if (self->field_0x34D == 1) {
                            fn_802D41E0(self, 4, 1);
                        } else {
                            fn_802D41E0(self, 3, 0);
                        }
                        break;
                    case 1:
                        if (self->field_0x354 == 1) {
                            self->field_0x1CE = 2;
                            self->field_0x354 = 2;
                        } else if (self->field_0x354 == 2) {
                            fn_802D0DCC(self);
                        } else if (self->field_0x354 == 3) {
                            fn_802D41E0(self, 0x33, 1);
                        } else {
                            if (self->variant == 2) {
                                if (self->field_0x420 == 6) {
                                    fn_802D3398(self, (u16*)lbl_805D4150[1]);
                                } else if (self->field_0x420 == 5) {
                                    fn_802D41E0(self, 0x2E, 1);
                                } else if (self->field_0x384 == 1) {
                                    fn_802D41E0(self, 0x24, 1);
                                } else {
                                    fn_802D41E0(self, 0x0D, 1);
                                }
                            } else {
                                if (self->field_0x420 == 6) {
                                    fn_802D3398(self, (u16*)lbl_805D4150[0]);
                                } else if (self->field_0x420 == 5) {
                                    u8* enemy = enemy_work;

                                    if (self->enemy_index != 0xFF) {
                                        enemy += self->enemy_index * 0xB18;
                                    }
                                    if ((u8)(enemy[0x1E5] + 0xF8) <= 1) {
                                        fn_802D41E0(self, 0x19, 1);
                                    } else {
                                        fn_802D41E0(self, 0x2D, 1);
                                    }
                                } else if (self->field_0x384 == 1) {
                                    fn_802D41E0(self, 0x23, 1);
                                } else {
                                    fn_802D41E0(self, 1, 1);
                                }
                            }
                        }
                        break;
                    case 4:
                        if (self->variant == 2) {
                            if (self->field_0x447 == 2) {
                                self->field_0x447 = 1;
                                fn_802D41E0(self, 0x26, 1);
                            } else if (self->field_0x3DE == 1) {
                                fn_802D41E0(self, 0x12, 1);
                            } else if (self->field_0x34F == 1) {
                                fn_802D41E0(self, 0x20, 1);
                            } else {
                                fn_802D41E0(self, 0x0C, 1);
                            }
                        } else if (self->field_0x447 == 2) {
                            self->field_0x447 = 1;
                            fn_802D41E0(self, 0x25, 1);
                        } else if (self->field_0x3DE == 1) {
                            fn_802D41E0(self, 9, 1);
                        } else if (self->field_0x34F == 1) {
                            fn_802D41E0(self, 0x1F, 1);
                        } else {
                            fn_802D41E0(self, 8, 1);
                        }
                        self->field_0x376 = 1;
                        break;
                    case 5:
                        if (self->variant == 2) {
                            if (self->field_0x3DE == 1) {
                                fn_802D41E0(self, 0x12, 1);
                            } else {
                                fn_802D41E0(self, 0x0B, 1);
                            }
                        } else if (self->field_0x3DE == 1) {
                            fn_802D41E0(self, 9, 1);
                        } else {
                            fn_802D41E0(self, 7, 1);
                        }
                        self->field_0x376 = 1;
                        break;
                    case 2:
                    case 3:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x11, 1);
                        } else {
                            fn_802D41E0(self, 6, 1);
                        }
                        break;
                    case 6:
                        self->field_0x1CE = 0x10;
                        break;
                    case 7:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x22, 1);
                        } else {
                            fn_802D41E0(self, 0x21, 1);
                        }
                        break;
                    case 8:
                        fn_802D41E0(self, 0x2F, 1);
                        break;
                    case 9:
                        if (self->variant == 2) {
                            fn_802D41E0(self, 0x32, 1);
                        } else {
                            fn_802D41E0(self, 0x31, 1);
                        }
  
                        break;
                    }
                }
                ai_npc_plan_dispatch(self);
                fn_802D41D4(self);
            }
        }
    }
}

/* 0x802D15DC - the move solver: fills the two probe vectors, walks the ~0x20-radius band and hands
 * the result to the collision query. */
void fn_802D15DC(struct _AINPC_W* self)
{
    nw4r::math::VEC3 probe;
    nw4r::math::VEC3 current;
    u32 flags;
    s32 blocked;

    VEC3_ctor(&probe);
    VEC3_ctor(&current);
    flags = 0x20;
    blocked = 0;
    if (self->field_0x3CA > 0) {
        self->field_0x3C8 = 0;
        return;
    }
    if (ai_npc_motion_step_ck(self, 2, 9) == 1 || ai_npc_motion_step_ck(self, 2, 0x13) == 1) {
        blocked = 1;
    }
    if (fn_802D7B24(self) == 1) {
        blocked = 1;
    }
    if ((u32)(self->motion - 4) > 1 && blocked == 0) {
        flags |= 0xC0;
    }
    copyVec3(&probe, &self->pos_0x184);
    probe.y = probe.y + lbl_8079A7A8;
    copyVec3(&current, &self->pos_0x178);
    current.y = current.y + lbl_8079A7A8;
    self->field_0x3C8 = (u8)coll_wall_slide(&probe, &current, lbl_8079A7A8, &self->pos_0x178, 0xFFFF, flags, 1,
                                        self->area);
}

/* 0x802D1710 - the step-height/ground probe: walks the two forward probes and re-seeds the motion
 * when the NPC has to step up or down. */
void fn_802D1710(struct _AINPC_W* self)
{
    ::LandData base;
    f32 value;

    em_land_data_init(&base);
    self->field_0x3CE = 0;
    if (self->field_0x3D0 <= 0 && ai_npc_flag_ck(self, 2) == 0) {
        if (pl_ground_check_all_layers(self, &self->pos_0x178, &self->field_0x324, &value, -6) == 1) {
            self->field_0x19C = value;
            switch (self->variant) {
            case 0:
                if (self->pos_0x178.y - self->field_0x19C <= lbl_8079A6A8) {
                    self->pos_0x178.y = self->field_0x19C;
                } else {
                    fn_802D2A00(self, 6, 0, 0);
                }
                break;
            case 2:
                if (ai_npc_motion_step_ck(self, 2, 0x0D) != 1) {
                    fn_802D2A00(self, 2, 0x0D, 0);
                    ai_npc_variant_set(self, 0);
                    fn_802D4200(self);
                }
                break;
            }
        } else if (pl_ground_check_all_layers(self, &self->pos_0x178, &self->field_0x324, &value, 1) == 1) {
            self->field_0x19C = value;
            switch (self->variant) {
            case 2:
                if (self->pos_0x178.y > lbl_8079A6B0) {
                    self->pos_0x178.y = lbl_8079A6B0;
                    self->field_0x3CE = 1;
                } else if (self->pos_0x178.y < lbl_8079A754 + self->field_0x19C) {
                    self->pos_0x178.y = lbl_8079A754 + self->field_0x19C;
                    self->field_0x3CE = 1;
                }
                break;
            default:
                fn_802D2A00(self, 2, 0x0C, 0);
                ai_npc_variant_set(self, 2);
                fn_802D4200(self);
                break;
            }
        }
        if (pl_ground_check_all_layers(self, &self->pos_0x178, &base, &value, 4) == 1) {
            self->field_0x1A0 = value;
        }
    }
}

/* 0x802D18E8 - mirrors the model's visibility flag onto the engine model when it moved. */
void fn_802D18E8(struct _AINPC_W* self)
{
    u8 visible = self->field_0x417;

    if (self->field_0x418 != visible) {
        if (visible == 1) {
            self->model.setVisibility(0x15, false);
        } else {
            self->model.setVisibility(0x15, true);
        }
    }
    self->field_0x418 = self->field_0x417;
}

#pragma peephole on
/* 0x802D1954 - re-seeds the NPC from the player work it targets: area, position, motion and the
 * whole attack bookkeeping. */
void fn_802D1954(struct _AINPC_W* self)
{
    _PLW* plw;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 position;

    VEC3_ctor(&offset);
    plw = self->plw_0x16C;
    if (ai_npc_mode1_ck(self) != 1) {
        if (fn_802D94A0(self) == 1 && plw->area_0x16 == 0) {
            self->field_0x486 = 0x5A;
        }
        self->area = plw->area_0x16;
        eft_rot_vec_copy(&self->vec_0x190, (_CP_VECTOR*)&plw->param_0x54);
        setVector3(&offset, lbl_8079A7AC, lbl_8079A670, lbl_8079A7B0);
        rotVecY(&offset, self->vec_0x190.y);
        addVec3(&position, &plw->vec_0x03C, &offset);
        copyVec3(&self->pos_0x178, copyVec3(&self->pos_0x184, &position));
        switch (plw->kind_0x09) {
        case 3:
            ai_npc_variant_set(self, 2);
            break;
        default:
            ai_npc_variant_set(self, 0);
            break;
        }
        if (self->motion == 5) {
            if ((u32)(self->motion_step - 5) <= 1) {
                fn_802D2A00(self, self->motion, self->motion_step, 0);
            }
        } else {
            ai_npc_base_state_set(self, self->variant);
        }
        fn_802D1710(self);
        self->enemy_index = 0xFF;
        self->field_0x1C8 = 0;
        fn_802D4200(self);
        self->field_0x389 = 1;
        fn_802D41C8(self);
        fn_802D3D84(self);
        self->field_0x383 = 1;
        self->field_0x447 = fn_802D8550(self);
        self->field_0x448 = 0;
        self->field_0x38A = 0;
        self->field_0x440 = 0;
        self->model.scale_0x1C.x = lbl_8079A674;
        self->model.scale_0x1C.y = lbl_8079A674;
        self->model.scale_0x1C.z = lbl_8079A674;
        fn_802D1FFC(self);
        mhchar_model_mtx_update(&self->model);
    }
}
#pragma peephole off

/* 0x802D1AFC - the weak-point roll: after the counter-attack window it picks one of the five
 * weak-point rates, bumps it and books the two hit effects. */
/* `fn_802D89D4` read as a full word: retail masks its result at the argument, not at the return. */
#define AINPC_ITEM_ID_GET ((u32 (*)(struct _AINPC_W*, u16))fn_802D89D4)

void fn_802D1AFC(struct _AINPC_W* self)
{
    u16 id;
    s16 count;

    switch (self->field_0x420) {
    case 3:
        if (fn_802D86D4(self, &id, &count) == 1) {
            if (self->field_0x422 <= 0 && self->field_0x461 == 0) {
                self->field_0x422 = 0x384;
                self->field_0x426 = ::ran_suu(1) % 5;
                if (::ran_suu(1) % 100 < self->weak_point_0x427[self->field_0x426]) {
                    self->field_0x426 = ::ran_suu(1) % 5;
                }
                self->weak_point_0x427[self->field_0x426] += 0x0A;
                if (self->weak_point_0x427[self->field_0x426] >= 0x64) {
                    self->weak_point_0x427[self->field_0x426] = 0x64;
                }
                fn_802D40EC(self, id, (s16)-count);
                if (self->field_0x42C != 0) {
                    fn_802D40EC(self, (u16)AINPC_ITEM_ID_GET(self, self->field_0x42C), count);
                }
            }
        } else {
            self->field_0x422 = 0;
            self->field_0x426 = 0;
        }
        break;
    }
}

/* 0x802D1C90 - the aim timer: bands the target's distance and runs the two aim-build limits. */
void fn_802D1C90(struct _AINPC_W* self)
{
    if (ai_npc_in_range_ck(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, lbl_8079A6F0) != 0) {
        if (ai_npc_in_range_ck(&self->pos_0x178, &self->plw_0x16C->vec_0x03C, lbl_8079A774) != 0) {
            self->field_0x416 = 2;
        } else {
            self->field_0x416 = 1;
        }
    } else {
        self->field_0x416 = 0;
    }
    if (self->field_0x416 == 0) {
        if (++self->field_0x38C >= 0x12C) {
            self->field_0x38C = 0x12C;
            self->field_0x38E = 1;
        }
    } else if (self->field_0x416 == 1) {
        if (++self->field_0x38C >= 0x96) {
            self->field_0x38C = 0x96;
            self->field_0x38E = 1;
        }
    } else {
        self->field_0x38C = 0;
        self->field_0x38E = 0;
    }
}

/* 0x802D1D90 - the band's per-frame driver: runs the whole chain of sub-steps and then the model
 * transform. */
void fn_802D1D90(struct _AINPC_W* self)
{
    _PLW* plw = self->plw_0x16C;
    u8 i;

    copyVec3(&self->pos_0x184, &self->pos_0x178);
    self->field_0x3C4 = 0;
    self->field_0x417 = 0;
    self->field_0x450 = 0;
    self->field_0x46C = 0;
    if (self->area != plw->area_0x16) {
        fn_802D1954(self);
    }
    fn_802D36B8(self);
    fn_802D1AFC(self);
    fn_802D1C90(self);
    ai_npc_player_idle_count(self);
    ai_npc_player_watch(self);
    if (fn_802D94A0(self) == 1) {
        fn_802D9A40(self);
    }
    if (ai_npc_mode1_ck(self) == 1) {
        ai_npc_element_frame(self);
    } else {
        ai_npc_plan_decide(self);
    }
    ai_npc_target_refresh(self);
    if (fn_802D2264(self) == 0 && self->motion != 4 && self->motion != 5) {
        fn_802D0F34(self);
    }
    ai_npc_attack_update(self);
    ai_npc_defense_update(self);
    fn_802D214C(self);
    ai_npc_motion_run(self);
    if (self->field_0x174 != 0) {
        ai_npc_motion_run(self);
    }
    if (ai_npc_flag_ck(self, 2) != 0 || self->area != ::get_now_areano()) {
        self->in_area = 0;
    } else {
        self->in_area = 1;
    }
    self->model.field_0x34 = self->in_area;
    if (self->field_0x002 == 0) {
        self->model.field_0x34 = self->field_0x002;
    }
    fn_802D18E8(self);
    fn_802D6888(self);
    fn_802D1FFC(self);
    self->model.move(0);
    copyVec3(&self->pos_0x178, &self->model.pos_0x04);
    switch (self->variant) {
    case 1:
        break;
    default:
        vec3_move_damp(&self->pos_0x184, &self->pos_0x178, &self->pos_0x32C, lbl_8079A7B4,
                     &self->pos_0x178);
        break;
    case 2:
        if (self->pos_0x184.y < lbl_8079A688 + self->field_0x19C) {
            vec3_move_damp(&self->pos_0x184, &self->pos_0x178, &self->pos_0x32C, lbl_8079A73C,
                         &self->pos_0x178);
        }
        break;
    }
    fn_802D15DC(self);
    fn_802D1710(self);
    fn_802D1FFC(self);
    mhchar_model_mtx_update(&self->model);
    for (i = 0; i < 2; i++) {
        hit_attack_list_push(&self->hit[i]);
    }
    g3d_root_model_bind(pRoot, self->model.field_0x118);
}

/* 0x802D1FFC - copies the model-transform words into the engine model and the position into its
 * own +0x04. */
void fn_802D1FFC(struct _AINPC_W* self)
{
    self->model.field_0x28 = self->vec_0x190.x;
    self->model.field_0x2C = self->vec_0x190.y;
    self->model.field_0x30 = self->vec_0x190.z;
    copyVec3(&self->model.pos_0x04, &self->pos_0x178);
}

#pragma peephole on
/* 0x802D2024 - advances the four counter-attack counter blocks. */
u32 fn_802D2024(struct _AINPC_W* self)
{
    if (self->field_0x21E > 0 && ::ai_skill_ck((::_AINPC_W*)self, 0x1A) == 0) {
        self->field_0x21C = self->field_0x21C + self->field_0x21E;
        self->field_0x220 = 0x1E;
    }
    if (self->field_0x230 > 0) {
        self->field_0x22E = self->field_0x22E + self->field_0x230;
    }
    if (self->field_0x22A > 0) {
        self->field_0x228 = self->field_0x228 + self->field_0x22A;
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x13, 0);
        } else {
            fn_802D2ABC(self, 4, 8, 0);
        }
        return 1;
    }
    if (self->field_0x224 > 0) {
        self->field_0x222 = self->field_0x222 + self->field_0x224;
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x15, 0);
        } else {
            fn_802D2ABC(self, 4, 0x0A, 0);
        }
        return 1;
    }
    return 0;
}
#pragma peephole off

/* 0x802D214C - runs down the aim-build timer and books the effect when it expires. */
void fn_802D214C(struct _AINPC_W* self)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    setVector3(&offset, lbl_8079A670, lbl_8079A68C, lbl_8079A670);
    if (self->motion != 5) {
        s16 count = self->field_0x21C;

        if (count > 0) {
            if (--self->field_0x220 <= 0) {
                self->field_0x21C--;
                self->field_0x220 = 0x1E;
                eft013_npc_place(self, &offset, 0x0E, lbl_8079A674, 2);
                if (fn_802D35EC(self, -1) == 1) {
                    return;
                }
            }
        }
    }
}

/* 0x802D21F8 - the attack selectors that need no counter-attack support. */
u8 fn_802D21F8(u8 selector)
{
    if ((u32)(selector - 4) <= 5 || (u32)(selector - 0x1B) <= 4 || (u32)(selector - 0x12) <= 1 ||
        (s32)selector == 0x24) {
        return 1;
    }
    return 0;
}

/* 0x802D2238 - starts the "raise guard" motion. */
void fn_802D2238(struct _AINPC_W* self)
{
    if (self->variant == 2) {
        fn_802D2ABC(self, 4, 0x16, 0);
    } else {
        fn_802D2ABC(self, 4, 0x0B, 0);
    }
}

/* 0x802D2264 - the counter-attack gate: rolls the guard, drains the window and dispatches the
 * support attack on +0x21A. */
s32 fn_802D2264(struct _AINPC_W* self)
{
    u8 selector;
    s32 step = 0;
    s16 window;
    s32 move;
    s32 commit;
    s32 near;
    s32 on_target;

    if (self->field_0x216 == 0) {
        return 0;
    }
    selector = self->field_0x21A;
    window = self->field_0x212;
    move = 0;
    commit = 1;
    near = 0;
    on_target = 0;
    if (fn_802D7B24(self) == 1) {
        near = 1;
    }
    if (fn_802D3F08(self) == 1 && (on_target = 1, move = -1, fn_802D21F8(selector) == 0)) {
        fn_802D2238(self);
    } else {
        if (fn_802D2024(self) == 1) {
            step = 1;
        }
        if (window != 0 && (fn_802D3A34(self, window), fn_802D35EC(self, (s16)-window) == 1)) {
            move = 1;
        } else if (self->variant == 1 && fn_802D21F8(selector) == 0) {
            fn_802D2A00(self, 4, 2, 0);
        } else if (step == 0) {
            switch (selector) {
            case 1:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0C, 0);
                } else {
                    fn_802D2A00(self, 4, 0, 0);
                }
                break;
            case 2:
            case 3:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0D, 0);
                } else {
                    fn_802D2A00(self, 4, 1, 0);
                }
                move = 1;
                break;
            case 4:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0E, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 3, 0);
                }
                break;
            case 36:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x18, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 0x17, 0);
                }
                break;
            case 5:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x18) == 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0F, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 4, 0);
                }
                break;
            case 6:
            case 7:
            case 18:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x19) == 1 || self->field_0x23C >= 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x10, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 5, 0);
                }
                break;
            case 8:
            case 9:
            case 19:
                if (::ai_skill_ck((::_AINPC_W*)self, 0x19) == 1 || self->field_0x23C >= 1) {
                    move = -1;
                    commit = -1;
                } else if (on_target != 0) {
                    fn_802D2238(self);
                } else if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x11, 0);
                } else if (self->variant == 1) {
                    fn_802D2A00(self, 4, 2, 0);
                } else {
                    fn_802D2A00(self, 4, 6, 0);
                }
                break;
            default:
                if (self->variant == 2) {
                    fn_802D2A00(self, 4, 0x0C, 0);
                } else {
                    fn_802D2A00(self, 4, 0, 0);
                }
                break;
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
                move = -1;
                commit = -1;
                break;
            }
        }
    }
    if (move >= 0) {
        fn_800DCB74(self->sound_handle_0x498, move, &self->pos_0x178);
    }
    if (commit >= 0) {
        fn_802D4200(self);
        fn_802D41C8(self);
        fn_802D3D84(self);
        self->field_0x389 = 1;
        if (near != 0) {
            fn_800DCC24(self->sound_handle_0x498, 7, 0);
            ai_npc_talk(self, (s8)self->field_0x482, 9, 0, 2);
        }
        return 1;
    }
    return 0;
}
}

/* -------------------------------------------------------------------------------------------------
 * 0x802D27E0 .. 0x802D44F4
 * ---------------------------------------------------------------------------------------------- */

extern "C" {

/* 0x802D27E0 - the address of the band's shared `.bss` work block. */
u8* ainpc_work_get(void)
{
    return (u8*)&ainpc_w;
}

/* 0x802D282C - the two-level motion-table walk: row `index / 100` of `lbl_805D3FB8` holds that
 * row's 100-entry column and the entry is `column[index % 100]`. */
s32 fn_802D282C(s32 index)
{
    u16 value = index;
    s32 group = value / 100;
    s32* row = (s32*)lbl_805D3FB8[group];

    if (row == NULL) {
        return 0;
    }
    return row[value - group * 100];
}

/* 0x802D287C - plays `motion` on the model with the row `fn_802D282C` resolves. */
void ai_npc_motion_play(struct _AINPC_W* self, s32 motion, s32 a, s32 b, s32 c)
{
    mhchar_motion_set(&self->model, 3, 0, (u16)motion, a, (f32)b, fn_802D282C((u16)motion), c,
                lbl_8079A678);
}

/* 0x802D2904 - the same with the default row. */
void ai_npc_motion_set(struct _AINPC_W* self, u32 motion, s32 a, s32 b)
{
    ai_npc_motion_play(self, (u16)motion, a, b, 0);
}

/* 0x802D2910 - plays the motion only when the model is not already on it. */
void ai_npc_motion_change(struct _AINPC_W* self, s32 motion, s32 a, s32 b)
{
    if ((u16)motion != ::ai_get_motion_no((::_AINPC_W*)self)) {
        ai_npc_motion_play(self, (u16)motion, a, b, 0);
    }
}

/* 0x802D2984 - whether the model's current motion has ended. */
u32 ai_npc_motion_end_ck(struct _AINPC_W* self)
{
    return mhchar_motion_end_ck(&self->model, 0);
}

/* 0x802D2990 - whether the model's motion layer `layer` is inside the frame window `low`..`high`. */
u32 ai_npc_frame_ck(struct _AINPC_W* self, u32 layer, f32 low, f32 high)
{
    return mhchar_frame_check(&self->model, (u16)layer, 0, low, high);
}

/* 0x802D29A0 - the same frame-window test with the second flag set. */
u32 fn_802D29A0(struct _AINPC_W* self, u32 layer, f32 low, f32 high)
{
    return mhchar_frame_check(&self->model, (u16)layer, 1, low, high);
}

#pragma peephole on
/* 0x802D29B8 - arms the model's second state flag. */
void fn_802D29B8(struct _AINPC_W* self, s8 value)
{
    self->model.field_0xF2 = value;
}
#pragma peephole off

/* 0x802D29C0 - clears it. */
void fn_802D29C0(struct _AINPC_W* self)
{
    self->model.field_0xF2 = 0;
}

#pragma peephole on
/* 0x802D29CC - sets the model's speed to the record's +0x44C times `scale`. */
void fn_802D29CC(struct _AINPC_W* self, f32 scale)
{
    mhchar_motion_speed_set(&self->model, self->field_0x44C * scale);
}
#pragma peephole off

/* 0x802D29DC - the model rotation component the scale setters read back. */
f32 ai_npc_time_scale_get(struct _AINPC_W* self)
{
    return self->model.field_0x5C;
}

#pragma peephole on
/* 0x802D29E4 - arms the model's first state flag. */
void fn_802D29E4(struct _AINPC_W* self, s8 value)
{
    self->model.field_0xF1 = value;
}
#pragma peephole off

/* 0x802D29EC - clears it. */
void fn_802D29EC(struct _AINPC_W* self)
{
    self->model.field_0xF1 = 0;
}

/* 0x802D29F8 - selects the locomotion arm. */
void ai_npc_variant_set(struct _AINPC_W* self, u8 variant)
{
    self->variant = variant;
}

/* 0x802D2A00 - the motion state setter: latches the gauge word, saves the outgoing motion and
 * sub-step, clears the three follow-up bytes, re-arms the model flags and sets the speed. */
void fn_802D2A00(struct _AINPC_W* self, u8 motion, u16 step, u16 gauge)
{
    self->field_0x1EC = gauge;
    self->state = 0;
    self->sub_step = 0;
    self->field_0x006 = 0;
    self->prev_motion = self->motion;
    self->prev_motion_step = self->motion_step;
    self->motion = motion;
    self->motion_step = step;
    fn_802D29C0(self);
    fn_802D29EC(self);
    self->field_0x002 = 1;
    self->field_0x44C = lbl_8079A678;
    if (::ai_skill_ck((::_AINPC_W*)self, 0x11) == 1 && motion == 2 && self->variant != 2) {
        self->field_0x44C = lbl_8079A6F8;
    }
    fn_802D29CC(self, lbl_8079A678);
}

/* 0x802D2ABC - the same setter through the "pending" byte the drivers test. */
void fn_802D2ABC(struct _AINPC_W* self, s32 motion, s32 step, s32 gauge)
{
    self->field_0x174 = 1;
    fn_802D2A00(self, motion, step, gauge);
}

/* 0x802D2AD4 - switches the locomotion arm and re-seeds the motion. */
void ai_npc_base_state_set(struct _AINPC_W* self, u32 variant)
{
    self->variant = variant;
    if (variant != 2) {
        fn_802D2ABC(self, 0, 0, 0);
    } else {
        fn_802D2ABC(self, 0, 1, 0);
    }
    fn_802D41C8(self);
}

/* 0x802D2B38 - whether the record is already on the given motion and sub-step. */
u32 ai_npc_motion_step_ck(struct _AINPC_W* self, s32 motion, s32 step)
{
    if (self->motion == (u8)motion && self->motion_step == (u16)step) {
        return 1;
    }
    return 0;
}

/* 0x802D2B68 - copies the world matrix of the model's joint `joint` to `out`. */
void ai_npc_joint_mtx_get(struct _AINPC_W* self, u32 joint, nw4r::math::MTX34* out)
{
    mhchar_joint_mtx_get(&self->model, joint, out);
}

/* 0x802D2B78 - tests the record's flag word. */
s32 ai_npc_flag_ck(struct _AINPC_W* self, s32 mask)
{
    return self->field_0x1EC & (u16)mask;
}

/* 0x802D2B88 - sets flags in it. */
void fn_802D2B88(struct _AINPC_W* self, u16 flags)
{
    self->field_0x1EC |= flags;
}

/* 0x802D2B98 - clears flags in it. */
void fn_802D2B98(struct _AINPC_W* self, u16 flags)
{
    self->field_0x1EC &= ~flags;
}

/* 0x802D2BB0 - fills hit entry `hit` for the running attack: the selector bytes, one OR per owned skill, the random
 * weak-point roll, and the "blocked" case that zeroes the entry again. */
void fn_802D2BB0(struct _AINPC_W* self, struct _HIT_W* hit)
{
    u8 applied = 0;

    hit->attack_flags_0x31 = 0x0A;
    hit->attack_kind_0x32 = 4;
    if (::ai_skill_ck((::_AINPC_W*)self, 2) == 1) {
        hit->field_0x048 |= 0x10;
        hit->field_0x04A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 3) == 1) {
        hit->field_0x048 |= 0x20;
        hit->field_0x04A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 4) == 1) {
        hit->field_0x048 |= 0x200;
        hit->field_0x04A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 5) == 1) {
        hit->field_0x048 |= 0x40;
        hit->field_0x04A = self->field_0x210;
        applied = 1;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 6) == 1) {
        hit->field_0x048 |= 0x80;
        hit->field_0x04A = self->field_0x210;
        applied = 1;
    }
    if (::ran_suu(1) % 3 == 0) {
        if (::ai_skill_ck((::_AINPC_W*)self, 8) == 1) {
            hit->field_0x048 |= 2;
            hit->field_0x04A = self->field_0x210;
            applied |= 2;
        }
        if (::ai_skill_ck((::_AINPC_W*)self, 9) == 1) {
            hit->field_0x048 |= 4;
            hit->field_0x04A = self->field_0x210;
            applied |= 2;
        }
        if (::ai_skill_ck((::_AINPC_W*)self, 0x0A) == 1) {
            hit->field_0x048 |= 1;
            hit->field_0x04A = self->field_0x210;
            applied |= 2;
        }
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 7) == 1 && (applied & 1) != 0) {
        hit->field_0x04A = (hit->field_0x04A * 0x82) / 100;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x0B) == 1 && (applied & 2) != 0) {
        hit->field_0x04A = (hit->field_0x04A * 0x82) / 100;
    }
    if (self->field_0x420 == 1 && self->field_0x422 > 0) {
        hit->kind_0x42 = 0x11;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x0D) == 1 && ::ran_suu(1) % 100 < 0x14) {
        hit->life_0x40 = (hit->life_0x40 * 0x7D) / 100;
        hit->field_0x04C |= 0x80;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x0C) == 1) {
        hit->field_0x04E = (s8)hit->field_0x04E * 0x78 / 100;
    }
    if (ai_npc_flag_ck(self, 4) != 0) {
        hit->attack_flags_0x31 = 1;
        hit->life_0x40 = 0;
        if (self->field_0x34E == 1) {
            hit->kind_0x42 = 2;
        }
        hit->field_0x04E = 0;
        hit->field_0x048 = 0;
        hit->field_0x04A = 0;
    }
    if ((applied & 3) != 0) {
        ::hit_flag_set((::_HIT_W*)hit, 0x800);
    }
}

/* 0x802D2F7C - fills hit entry `index` from motion-table entry `row` and hands it to the attack
 * filler. */
void fn_802D2F7C(struct _AINPC_W* self, u8 index, s32 row)
{
    struct _HIT_W* hit = &self->hit[index];
    struct MotionEntry* entry = (struct MotionEntry*)(lbl_805D3AD8 + row * 0x1A);
    f32 limit;
    f32 frame;

    hit->size_data_0x08 = (_HIT_SIZE_DATA*)lbl_80792508[entry->motion_id_0x0F];
    hit_flags_clear(hit);
    ::hit_flag_set((::_HIT_W*)hit, 0x320);
    hit->field_0x018 = ::ai_get_motion_no((::_AINPC_W*)self);
    hit->field_0x01A = 0;
    hit->field_0x01C = 0;
    hit->field_0x01E = 0;
    limit = self->model.field_0x44 - lbl_8079A730;
    if (limit < lbl_8079A670 || (self->model.field_0x4C & 1) != 0) {
        limit = lbl_8079A670;
    }
    frame = self->model.field_0x74;
    hit->delay_0x38 = entry->approach_0x00;
    hit->time_0x3C = entry->retreat_0x02;
    if (entry->approach_0x00 != 0) {
        hit->delay_0x38 = hit->delay_0x38 + (limit - frame);
        if (hit->delay_0x38 < lbl_8079A670) {
            hit->time_0x3C = hit->time_0x3C + hit->delay_0x38;
            hit->delay_0x38 = lbl_8079A670;
            if (hit->time_0x3C <= lbl_8079A670) {
                hit->active_0x05 = 0;
                return;
            }
        }
    }
    hit_data_apply(hit, (struct _HIT_DATA*)entry);
    fn_802D2BB0(self, hit);
}

/* 0x802D30F8 - clamps the angle difference to `limit` degrees, taking the short way round. */
u16 fn_802D30F8(u16 angle, u16 target, u32 limit)
{
    u16 diff = angle - target;

    if (abs((s16)diff) < (u16)limit) {
        target = angle;
    } else {
        target = (diff < 0x8000) ? target + limit : target - limit;
    }
    return target;
}

/* 0x802D3184 - whether the model's facing is outside `limit` degrees of the record's +0x194. */
s32 fn_802D3184(struct _AINPC_W* self, u32 limit)
{
    u16 diff = self->vec_0x190.y - (u16)::calcVecAng2(&self->pos_0x178, &self->pos_0x1B0);

    if (diff <= (u16)-limit && diff >= (u16)limit) {
        return 0;
    }
    return 1;
}

/* 0x802D31FC - zeroes the model offset vector at +0x1D4. */
void fn_802D31FC(struct _AINPC_W* self)
{
    self->pos_0x1D4.x = lbl_8079A670;
    self->pos_0x1D4.y = lbl_8079A670;
    self->pos_0x1D4.z = lbl_8079A670;
}

/* 0x802D3210 - builds the offset vector at +0x1D4 from the two angles at `angles[0]`/`angles[1]`
 * and adds it to the position. */
void fn_802D3210(struct _AINPC_W* self, u32* angles)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    copyVec3(&offset, &self->pos_0x1D4);
    rotVecX(&offset, angles[0]);
    rotVecY(&offset, angles[1]);
    addVec3To(&self->pos_0x178, &offset);
}

/* 0x802D327C - the stored variant, then the same offset walk from +0x1D4. */
void fn_802D327C(struct _AINPC_W* self, u32* angles)
{
    fn_802D3210(self, angles);
    addVec3To(&self->pos_0x1D4, &self->pos_0x1E0);
}

/* 0x802D32B4 - rolls one of the six-byte attack rows the skill table points at, weighted by the
 * rows' first word, and starts the motion the row names. */
#pragma opt_common_subs off
s32 fn_802D32B4(struct _AINPC_W* self, u16* row)
{
    u16 walked;
    u16 roll;
    u16* walk = row;
    u16 total = 0;

    while (*walk != 0xFFFF) {
        total += *walk;
        walk += 3;
    }
    if (total == 0) {
        return 0;
    }
    roll = ::ran_suu(1) % total;
    walked = 0;
    for (; *row != 0xFFFF; row += 3) {
        walked += *row;
        if (roll < walked) {
            fn_802D2A00(self, (u8)row[1], row[2], 0);
            return 1;
        }
    }
    return 0;
}
#pragma opt_common_subs reset

/* `fn_802D41E0` with an unsigned selector: retail passes the row's byte unextended. */
#define AINPC_SELECTOR_SET ((void (*)(struct _AINPC_W*, u8, s32))fn_802D41E0)

/* 0x802D3398 - the same for the four-byte rows the dispatchers use. */
s32 fn_802D3398(struct _AINPC_W* self, u16* row)
{
    u16 walked;
    u16 roll;
    u16* walk = row;
    u16 total = 0;

    while (*walk != 0xFFFF) {
        total += *walk;
        walk += 2;
    }
    if (total == 0) {
        return 0;
    }
    roll = ::ran_suu(1) % total;
    walked = 0;
    for (; *row != 0xFFFF; row += 2) {
        walked += *row;
        if (roll < walked) {
            AINPC_SELECTOR_SET(self, row[1], 1);
            return 1;
        }
    }
    return 0;
}
#pragma peephole off

/* 0x802D3474 - the same motion setter for the "blocked" arm. */
void fn_802D3474(struct _AINPC_W* self, s32 motion, s32 step)
{
    fn_802D2ABC(self, 5, (u16)motion, (u16)step);
}

/* 0x802D348C - drops the aim/attack state: clears the gauge, resets the attack bookkeeping and
 * re-seeds the motion (the recorder's counter bumps below 7 while the skill is held). */
void fn_802D348C(struct _AINPC_W* self)
{
    if (self->motion != 5) {
        self->gauge_0x1F0 = 0;
        fn_802D3AD8(self);
        fn_802D3CFC(self, 0);
        fn_802D3D84(self);
        fn_802D4200(self);
        self->field_0x389 = 1;
        if (ai_npc_mode1_ck(self) == 1) {
            fn_802D3474(self, 9, 0);
            return;
        }
        switch (self->variant) {
        case 2:
            fn_802D3474(self, 4, 0);
            break;
        default:
            fn_802D3474(self, 0, 0);
            break;
        }
        if (::ai_skill_ck((::_AINPC_W*)self, 0x15) == 1) {
            if (self->field_0x43C < 7) {
                self->field_0x43C += 1;
                self->field_0x1FE = (self->field_0x1FE * 0x6A) / 100;
                self->field_0x200 = (self->field_0x200 * 0x6A) / 100;
            }
        }
    }
}

/* 0x802D35B4 - adjusts the motion gauge and clamps it to [0, +0x1F4]. */
void fn_802D35B4(struct _AINPC_W* self, s32 delta)
{
    s32 value = self->gauge_0x1F0 + (s16)delta;

    self->gauge_0x1F0 = value;
    if (value <= 0) {
        self->gauge_0x1F0 = 0;
    }
    if (self->gauge_0x1F0 >= self->gauge_max_0x1F4) {
        self->gauge_0x1F0 = self->gauge_max_0x1F4;
    }
}

/* 0x802D35EC - drains the gauge (zeroed while the player is down) and reports whether it ran out. */
u32 fn_802D35EC(struct _AINPC_W* self, s32 delta)
{
    if (Pl_motion_input_ck(0) == 1) {
        delta = 0;
    }
    fn_802D35B4(self, (s16)delta);
    if (ai_npc_mode1_ck(self) == 1) {
        fn_802D92F8(self);
    }
    if (self->gauge_0x1F0 <= 0 && self->variant != 1) {
        fn_802D348C(self);
        return 1;
    }
    return 0;
}

/* 0x802D3684 - re-seeds the gauge from the +0x1F8 value. */
void fn_802D3684(struct _AINPC_W* self)
{
    self->gauge_max_0x1F4 = self->gauge_refill_0x1F8;
    self->gauge_0x1F0 = self->gauge_refill_0x1F8;
}

/* 0x802D3694 - raises the gauge cap by `delta`, clamped at 150. */
void fn_802D3694(struct _AINPC_W* self, s32 delta)
{
    s32 value = self->gauge_max_0x1F4 + (s16)delta;

    self->gauge_max_0x1F4 = value;
    if (value >= 0x96) {
        self->gauge_max_0x1F4 = 0x96;
    }
}

/* 0x802D36B8 - the frame step: runs down every one of the band's countdowns. */
void fn_802D36B8(struct _AINPC_W* self)
{
    if (self->field_0x214 > 0) {
        self->field_0x214--;
    }
    if (self->field_0x35A > 0) {
        self->field_0x35A--;
    }
    if (self->field_0x35E > 0) {
        self->field_0x35E--;
    }
    if (self->field_0x352 > 0) {
        self->field_0x352--;
        if (self->field_0x352 == 1) {
            self->field_0x354 = 3;
        }
    }
    if (self->field_0x34A != 0) {
        self->field_0x34A--;
    }
    if (self->field_0x3CA > 0) {
        self->field_0x3CA--;
    }
    if (self->field_0x3D0 > 0) {
        self->field_0x3D0--;
    }
    if (self->field_0x43E > 0) {
        self->field_0x43E--;
    }
    if (self->field_0x422 > 0) {
        self->field_0x422--;
        if (self->field_0x420 == 1 && (self->field_0x422 == 0 || self->variant == 2)) {
            self->field_0x422 = 0;
            fn_800DCCF8(self->sound_handle_0x498, &self->pos_0x178, 1);
        }
    }
    if (self->field_0x424 > 0) {
        self->field_0x424--;
    }
    if (self->field_0x374 > 0) {
        self->field_0x374--;
    }
    if (self->field_0x236 > 0) {
        self->field_0x236--;
        if (self->field_0x236 == 0) {
            self->field_0x235 = 0;
        }
    }
    if (self->field_0x23A > 0) {
        self->field_0x23A--;
        if (self->field_0x23A == 0) {
            self->field_0x239 = 0;
        }
    }
    if (self->field_0x3D4 < self->field_0x3D6) {
        self->field_0x3DC--;
        if (self->field_0x3DC <= 0) {
            fn_802D6B2C(self, 1);
        }
    }
    if (self->field_0x23E > 0) {
        self->field_0x23E--;
        if (self->field_0x23E <= 0) {
            self->field_0x23E = 0;
            self->field_0x23C = 0;
        }
    }
    if (self->field_0x408 > 0) {
        self->field_0x408--;
    }
    if (self->field_0x40A > 0) {
        self->field_0x40A--;
    }
    if (self->field_0x40C > 0) {
        self->field_0x40C--;
    }
    if (self->field_0x40E > 0) {
        self->field_0x40E--;
    }
    if (self->field_0x410 > 0) {
        self->field_0x410--;
    }
    if (self->field_0x412 > 0) {
        self->field_0x412--;
    }
    if (self->field_0x47C > 0) {
        self->field_0x47C--;
    }
    if (self->field_0x46A > 0) {
        self->field_0x46A--;
    }
    if (self->field_0x38A > 0) {
        self->field_0x38A--;
    }
    if (self->field_0x486 > 0) {
        self->field_0x486--;
    }
}
#pragma peephole off

/* 0x802D3984 - the 0..0x14 progress interpolation `high` -> `low`. */
s16 fn_802D3984(struct _AINPC_W* self, s32 high, s32 low, u32 reverse)
{
    s16 progress;

    if ((u8)reverse == 0) {
        progress = self->field_0x339;
    } else {
        progress = 0x14 - self->field_0x339;
    }
    return low + (progress * ((s16)high - (s16)low)) / 20;
}

/* 0x802D39DC - seeds the entry timer from that interpolation. */
void fn_802D39DC(struct _AINPC_W* self)
{
    self->field_0x35A = fn_802D3984(self, 0x1E, 0x0A, 1) * 0x1E;
}

/* 0x802D3A20 - clears the +0x34F counter. */
void fn_802D3A20(struct _AINPC_W* self)
{
    self->field_0x34F = 0;
}

/* 0x802D3A2C - arms the +0x214 countdown. */
void fn_802D3A2C(struct _AINPC_W* self, s16 value)
{
    self->field_0x214 = value;
}

#pragma peephole on
/* 0x802D3A34 - adds to the +0x350 counter (halved without the skill) and clamps it to the
 * model's limit. */
void fn_802D3A34(struct _AINPC_W* self, s32 delta)
{
    s16 limit;

    if (::ai_skill_ck((::_AINPC_W*)self, 0x17) == 1) {
        self->field_0x350 += delta;
    } else {
        self->field_0x350 += (s16)((s16)delta / 2);
    }
    limit = ai_npc_counter_window_get(self);
    if (self->field_0x350 > limit) {
        self->field_0x350 = limit;
    }
    if (self->field_0x350 < 0) {
        self->field_0x350 = 0;
    }
}
#pragma peephole off

/* 0x802D3AD8 - clears the three +0x350..+0x354 words. */
void fn_802D3AD8(struct _AINPC_W* self)
{
    if (self->field_0x354 != 0) {
        self->field_0x350 = 0;
        self->field_0x354 = 0;
        self->field_0x352 = 0;
    }
}

/* 0x802D3AF8 - clears the four +0x348..+0x34B bytes. */
void fn_802D3AF8(struct _AINPC_W* self)
{
    self->field_0x348 = 0;
    self->field_0x349 = 0;
    self->field_0x34A = 0;
    self->field_0x34B = 0;
}

/* 0x802D3B10 - clears +0x34D and arms +0x34E. */
void fn_802D3B10(struct _AINPC_W* self)
{
    self->field_0x34D = 0;
    self->field_0x34E = 0xFF;
}

/* 0x802D3B24 - clears +0x382 and +0x34B. */
void fn_802D3B24(struct _AINPC_W* self)
{
    self->field_0x382 = 0;
    self->field_0x34B = 0;
}

/* 0x802D3B34 - advances the three +0x3C6/+0x3CC/+0x480 timers and reports whether the move the
 * caller handed in is a repeat of the last accepted one. */
u32 fn_802D3B34(struct _AINPC_W* self, u16 limit, u8 copy)
{
    u32 moved = 0;

    if (self->field_0x3C8 > 0) {
        if (++self->field_0x3C6 >= (s32)limit) {
            self->field_0x3C6 = limit;
            moved = 1;
        }
    } else {
        self->field_0x3C6 = 0;
    }
    if (self->field_0x3CE > 0) {
        if (++self->field_0x3CC >= (s32)limit) {
            self->field_0x3CC = limit;
            moved = 1;
        }
    } else {
        self->field_0x3CC = 0;
    }
    if (self->drift_active != 0) {
        if (++self->field_0x480 >= (s32)limit) {
            self->field_0x480 = limit;
            moved = 1;
            self->field_0x389 = 0;
        }
    } else {
        self->field_0x480 = 0;
    }
    if (moved == 1) {
        if (copy == 1) {
            copyVec3(&self->pos_0x1B0, &self->pos_0x178);
            fn_802D3CB8(self);
            return 0;
        }
        if ((self->field_0x358 != 0 || self->field_0x35C != 0) &&
            ::calcDistanceSqXZ(&self->pos_0x178, &self->pos_0x1B0) < lbl_8079A6E0) {
            copyVec3(&self->pos_0x1B0, &self->pos_0x178);
            fn_802D3CB8(self);
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 0x802D3CB8 - clears the three move timers. */
void fn_802D3CB8(struct _AINPC_W* self)
{
    self->field_0x3C6 = 0;
    self->field_0x3CC = 0;
    self->field_0x480 = 0;
}

/* 0x802D3CCC - arms the +0x3CA countdown. */
void fn_802D3CCC(struct _AINPC_W* self, s16 value)
{
    self->field_0x3CA = value;
}

/* 0x802D3CD4 - arms the +0x3D0 countdown. */
void fn_802D3CD4(struct _AINPC_W* self, s16 value)
{
    self->field_0x3D0 = value;
}

/* 0x802D3CDC - clears the two +0x22E block words. */
void fn_802D3CDC(struct _AINPC_W* self)
{
    self->field_0x22E = 0;
    self->field_0x232 = 0;
}

/* 0x802D3CEC - clears the two +0x222 block words. */
void fn_802D3CEC(struct _AINPC_W* self)
{
    self->field_0x222 = 0;
    self->field_0x226 = 0;
}

/* 0x802D3CFC - clears the whole motion-gauge block; `full` also drops the +0x234..+0x23E run. */
void fn_802D3CFC(struct _AINPC_W* self, u8 full)
{
    fn_802D3CDC(self);
    fn_802D3CEC(self);
    self->field_0x21C = 0;
    self->field_0x220 = 0;
    self->field_0x228 = 0;
    self->field_0x22C = 0;
    if (full == 0) {
        self->field_0x234 = 0;
        self->field_0x235 = 0;
        self->field_0x238 = 0;
        self->field_0x239 = 0;
        self->field_0x236 = 0;
        self->field_0x23A = 0;
        self->field_0x23C = 0;
        self->field_0x23E = 0;
    }
    fn_802D6888(self);
}

/* 0x802D3D84 - resets the band's whole attack/aim bookkeeping. */
void fn_802D3D84(struct _AINPC_W* self)
{
    self->field_0x35C = 0;
    self->field_0x358 = 0;
    self->field_0x3DE = 0;
    self->field_0x461 = 0;
    self->field_0x451 = 0;
    fn_802D3AF8(self);
    fn_802D3B10(self);
    fn_802D3B24(self);
    fn_802D3A20(self);
    fn_802D4238(self);
}

/* 0x802D3DE8 - reports (and starts the follow-up motion) when the +0x22E counter has run out. */
u32 fn_802D3DE8(struct _AINPC_W* self)
{
    if (self->field_0x22E >= 0x96) {
        if (self->variant == 2) {
            fn_802D2ABC(self, 4, 0x14, 0);
        } else {
            fn_802D2ABC(self, 4, 9, 0);
        }
        return 1;
    }
    return 0;
}

/* 0x802D3E4C - the counter-attack roll. */
s32 ai_npc_counter_roll(struct _AINPC_W* self)
{
    s32 chance = 0x0A;

    self->field_0x321 = 0;
    if ((u32)(self->field_0x21A - 0x1B) <= 4) {
        return 0;
    }
    if (::ai_skill_ck((::_AINPC_W*)self, 0x10) == 1) {
        chance = 0x28;
    }
    if (self->variant != 1 && ::ran_suu(1) % 100 < chance) {
        self->field_0x321 = 1;
        return 1;
    }
    return 0;
}

/* 0x802D3F08 - whether the counter-attack roll latched. */
u32 fn_802D3F08(struct _AINPC_W* self)
{
    return self->field_0x321 != 0;
}

/* 0x802D3F1C - the highest weak-point rate of the two hit entries. */
#pragma opt_propagation off
u8 fn_802D3F1C(struct _AINPC_W* self)
{
    u8 highest = 0xFF;
    u8 value = self->hit[0].result;

    if (value != 0xFF && (highest == 0xFF || highest < value)) {
        highest = value;
    }
    value = self->hit[1].result;
    if (value != 0xFF && (highest == 0xFF || highest < value)) {
        highest = value;
    }
    return highest;
}
#pragma opt_propagation reset

/* 0x802D3F70 - the percentage roll the +0x339 progress interpolates between `high` and `low`. */
s32 fn_802D3F70(struct _AINPC_W* self, u8 high, u8 low)
{
    return ::ran_suu(1) % 100 < low + ((self->field_0x339 * (high - low)) / 20);
}

/* 0x802D4020 - the formation's attack roll, raised by the skill. */
u32 fn_802D4020(struct _AINPC_W* self)
{
    u8 chance = self->formation_0x41C->attack_rate;

    if (::ai_skill_ck((::_AINPC_W*)self, 0x14) == 1) {
        chance += 0x14;
    }
    if (::ran_suu(1) % 100 < chance) {
        return 1;
    }
    return 0;
}

#pragma peephole on
/* 0x802D40A4 - appends an entry to the 8-slot ring at +0x390. */
void fn_802D40A4(struct _AINPC_W* self, u16 id)
{
    self->slots[self->entry_index].id = id;
    self->slots[self->entry_index].count = 1;
    self->entry_index = self->entry_index + 1;
    if (self->entry_index >= 8) {
        self->entry_index = 0;
    }
}
#pragma peephole off

/* 0x802D40EC - adds `delta` to the ring/pending entry `id` carries; a new id goes to the first free
 * pending slot. */
void fn_802D40EC(struct _AINPC_W* self, u16 id, s16 delta)
{
    u8 i;

    for (i = 8; i < 12; i++) {
        if (id == self->slots[i].id) {
            self->slots[i].count += delta;
            if (self->slots[i].count <= 0) {
                self->slots[i].count = 0;
                self->slots[i].id = 0;
            }
            return;
        }
    }
    for (i = 8; i < 12; i++) {
        if (self->slots[i].id == 0 && delta > 0) {
            self->slots[i].id = id;
            self->slots[i].count = delta;
            return;
        }
    }
}
#pragma peephole off

/* 0x802D41C8 - latches +0x1CD. */
void fn_802D41C8(struct _AINPC_W* self)
{
    self->field_0x1CD = 1;
}

/* 0x802D41D4 - clears it. */
void fn_802D41D4(struct _AINPC_W* self)
{
    self->field_0x1CD = 0;
}

/* 0x802D41E0 - latches the attack selector; `reset` clears the two follow-up bytes. */
void fn_802D41E0(struct _AINPC_W* self, s8 selector, s32 reset)
{
    self->field_0x1CE = selector;
    if ((u8)reset == 1) {
        self->field_0x1CF = 0;
        self->field_0x1D0 = 0;
    }
}

/* 0x802D4200 - re-arms the attack step. */
void fn_802D4200(struct _AINPC_W* self)
{
    self->field_0x1D0 = 0;
    self->field_0x1CF = 1;
    self->field_0x389 = 1;
}

/* 0x802D4218 - selects the first follow-up. */
void fn_802D4218(struct _AINPC_W* self)
{
    self->field_0x1D1 = 1;
}

/* 0x802D4224 - selects the second. */
void fn_802D4224(struct _AINPC_W* self)
{
    self->field_0x1D1 = 2;
}

/* 0x802D4230 - stores the model offset factor. */
void fn_802D4230(struct _AINPC_W* self, f32 value)
{
    self->field_0x1BC = value;
}

/* 0x802D4238 - clears the two +0x376 bytes. */
void fn_802D4238(struct _AINPC_W* self)
{
    self->field_0x376 = 0;
    self->field_0x378 = 0;
}

/* `eft004_key_spawn` (`ef/eft004_fx.cpp`) with its scale third: retail loads f1 between r4 and r5. */
#define AINPC_KEY_SPAWN ((void (*)(u8, nw4r::math::VEC3*, f32, s32, u8, s32))eft004_key_spawn)

/* 0x802D4248 - spawns the attack's effect: takes the joint's world position (or the record's own),
 * drops it by the entry's offset and maps the attack kind onto the effect id the emitter takes. */
void fn_802D4248(struct _AINPC_W* self, u8 a1, u8 kind, u32 joint, s32 effect, f32 scale)
{
    nw4r::math::VEC3 pos;
    u8 code = kind;

    VEC3_ctor(&pos);
    if ((self->field_0x324 & 0x4000) != 0) {
        return;
    }
    if (a1 != 0) {
        return;
    }
    if (joint != 0xFF) {
        self->model.get_joint_wpos(joint, &pos);
    } else {
        copyVec3(&pos, &self->pos_0x178);
    }
    if ((self->field_0x324 & 6) != 0) {
        pos.y = lbl_8079A7B8 + self->field_0x1A0;
        switch (code) {
        case 0:
        case 17:
            code = 1;
            AINPC_KEY_SPAWN(2, &pos, scale, effect, self->area, 2);
            break;
        case 6:
        case 7:
        case 41:
            code = 0x23;
            break;
        case 18:
            code = 1;
            AINPC_KEY_SPAWN(2, &pos, scale, effect, self->area, 2);
            break;
        case 40:
            code = 0x0E;
            break;
        case 42:
        case 43:
        case 44:
            AINPC_KEY_SPAWN(0x23, &pos, scale, effect, self->area, 3);
            return;
        case 45:
            AINPC_KEY_SPAWN(0x23, &pos, scale, effect, self->area, 1);
            return;
        case 46:
            AINPC_KEY_SPAWN(2, &pos, scale, effect, self->area, 1);
            return;
        case 47:
            AINPC_KEY_SPAWN(1, &pos, scale, effect, self->area, 1);
            AINPC_KEY_SPAWN(2, &pos, scale, effect, self->area, 1);
            return;
        case 48:
            AINPC_KEY_SPAWN(1, &pos, scale, effect, self->area, 1);
            return;
        }
    } else {
        pos.y = lbl_8079A7B8 + self->field_0x19C;
        switch (code) {
        case 2:
        case 42:
        case 46:
            return;
        case 40:
        case 43:
        case 45:
            code = 6;
            break;
        case 41:
            code = 0;
            break;
        case 44:
            code = 7;
            break;
        case 47:
            code = 0x12;
            break;
        case 48:
            code = 1;
            break;
        }
    }
    AINPC_KEY_SPAWN(code, &pos, scale, effect, self->area, 2);
}
}
}  /* namespace view_fn_802D0F34 */

namespace view_fn_802D0F34 {

extern "C" {

/* 0x802D44F4 (0x2040): spawns the effects of the companion's current motion at their frames (dust, splashes,
 * swings, the water ripples while it swims), keyed on `ai_get_motion_no`. */
void ai_npc_motion_dispatch(_AINPC_W* self)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    if (self->in_area != 0) {
        if ((fn_802CCE04(self) == 1) && ((self->field_0x352 % 45) == 0)) {
            setVector3(&offset, -20.0f, 30.0f, 0.0f);
            eft013_npc_place(self, &offset, 0xE, 1.0f, 0x21);
        }
        switch (::ai_get_motion_no((::_AINPC_W*)self)) {
        case 0x2:
            if (ai_npc_frame_ck(self, 0, 42.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.5f);
            }
            break;
        case 0x8:
            if (ai_npc_frame_ck(self, 0, 10.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2E, 4, 0, 1.0f);
            }
            break;
        case 0x9:
            if ((ai_npc_frame_ck(self, 0, 10.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 40.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 70.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 100.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2E, 4, 0, 1.0f);
            }
            break;
        case 0xA:
            if ((ai_npc_frame_ck(self, 0, 10.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 40.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 70.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2E, 4, 0, 1.0f);
            }
            break;
        case 0xB:
            if ((ai_npc_frame_ck(self, 0, 18.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 42.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 70.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 96.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 118.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 138.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2F, 0x14, 0, 0.8f);
            }
            if ((ai_npc_frame_ck(self, 0, 30.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 56.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 82.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 106.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 128.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2F, 0x12, 0, 0.8f);
            }
            break;
        case 0xE:
            if ((ai_npc_frame_ck(self, 0, 10.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 44.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 80.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 106.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 130.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 152.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 8, 0, 1.2f);
                } else {
                    fn_802D4248(self, 0, 0, 8, 0, 1.2f);
                }
            }
            if ((ai_npc_frame_ck(self, 0, 66.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 94.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 120.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 142.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 162.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0xC, 0, 1.2f);
                } else {
                    fn_802D4248(self, 0, 0, 0xC, 0, 1.2f);
                }
            }
            if (ai_npc_frame_ck(self, 0, 216.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x23, 0xFF, 0, 1.0f);
                } else {
                    fn_802D4248(self, 0, 6, 0xFF, 0, ai_npc_f32_0_7);
                    fn_802D4248(self, 0, 0, 0xFF, 0, 2.0f);
                }
            }
            break;
        case 0x10:
            if (ai_npc_frame_ck(self, 0, 36.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x23, 0xFF, 0, 1.0f);
                } else {
                    fn_802D4248(self, 0, 6, 0xFF, 0, ai_npc_f32_0_7);
                    fn_802D4248(self, 0, 0, 0xFF, 0, 2.0f);
                }
            }
            if (ai_npc_frame_ck(self, 0, 70.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x23, 0xFF, 0, 1.0f);
                } else {
                    fn_802D4248(self, 0, 6, 0xFF, 0, 0.4f);
                }
            }
            break;
        case 0x12:
            if (ai_npc_frame_ck(self, 0, 40.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0, 4, 0, 1.0f);
            }
            break;
        case 0x16:
            if (ai_npc_frame_ck(self, 0, 28.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0, 0x14, 0, 1.0f);
            }
            if (ai_npc_frame_ck(self, 0, 68.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0, 0x12, 0, 1.0f);
            }
            break;
        case 0x17:
            if (ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0, 0xE, 0, 1.5f);
            }
            if ((ai_npc_frame_ck(self, 0, 34.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 54.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 8, 0, 1.0f);
                } else {
                    fn_802D4248(self, 0, 0, 8, 0, 1.0f);
                }
            }
            if ((ai_npc_frame_ck(self, 0, 46.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 66.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0xC, 0, 1.0f);
                } else {
                    fn_802D4248(self, 0, 0, 0xC, 0, 1.0f);
                }
            }
            break;
        case 0x18:
            if ((ai_npc_frame_ck(self, 0, 16.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 50.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x11, 4, 0, 1.0f);
                } else {
                    fn_802D4248(self, 0, 0, 4, 0, 1.0f);
                }
            }
            break;
        case 0x19:
            if (ai_npc_frame_ck(self, 0, 40.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 4, 0, 0.5f);
            }
            break;
        case 0x22:
            if (ai_npc_frame_ck(self, 0, 6.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.6f);
            }
            break;
        case 0x24:
            if (ai_npc_frame_ck(self, 0, 6.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.6f);
            }
            if (ai_npc_frame_ck(self, 0, 32.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.6f);
            }
            if (ai_npc_frame_ck(self, 0, 110.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.6f);
            }
            break;
        case 0x25:
            if ((ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 42.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0, 0xFF, 0, 1.0f);
            }
            if (ai_npc_frame_ck(self, 0, 76.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2D, 0xFF, 0, 0.5f);
            }
            break;
        case 0x27:
            if (ai_npc_frame_ck(self, 0, 8.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.6f);
            }
            break;
        case 0x28:
            if ((ai_npc_frame_ck(self, 0, 12.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 40.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 68.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 94.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 116.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 138.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0x14, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0x12, 0x14, 0, 0.8f);
                }
            }
            if ((ai_npc_frame_ck(self, 0, 26.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 54.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 82.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 104.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 128.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0x12, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0x12, 0x12, 0, 0.8f);
                }
            }
            break;
        case 0x2A:
            if (ai_npc_frame_ck(self, 0, 116.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.4f);
            }
            break;
        case 0x2C:
            if (ai_npc_frame_ck(self, 0, 36.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2C, 0xFF, 0, 0.5f);
            }
            break;
        case 0x30:
            if (ai_npc_frame_ck(self, 0, 2.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 0xE, 0, 0.6f);
            }
            break;
        case 0x31:
            if ((ai_npc_frame_ck(self, 0, 92.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 110.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 140.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.5f);
            }
            break;
        case 0x34:
            if (ai_npc_frame_ck(self, 0, 18.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0, 4, 0, 1.0f);
            }
            break;
        case 0x35:
            if ((ai_npc_frame_ck(self, 0, 2.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 28.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 2, 0xFF, 0, 1.0f);
            }
            break;
        case 0x3B:
            if (((self->field_0x324 & 1) != 0) && (ai_npc_frame_ck(self, 0, 2.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2B, 4, 0, 1.0f);
            }
            break;
        case 0x65:
            if (ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 7, 0xFF, 0, 0.5f);
            }
            if (ai_npc_frame_ck(self, 0, 60.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 7, 0xFF, 0, 0.4f);
            }
            if (ai_npc_frame_ck(self, 0, 96.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 7, 0xFF, 0, 0.5f);
            }
            break;
        case 0x67:
            if (ai_npc_frame_ck(self, 0, 54.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.3f);
            }
            break;
        case 0x6B:
            if (ai_npc_frame_ck(self, 0, 52.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.5f);
            }
            break;
        case 0x6C:
            if (ai_npc_frame_ck(self, 0, 208.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 0xFF, 0, 0.5f);
            }
            break;
        case 0x6E:
            if ((ai_npc_frame_ck(self, 0, 2.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.4f);
            }
            break;
        case 0x78:
            if (ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 6, 4, 0, 0.6f);
            }
            break;
        case 0x79:
            if (ai_npc_frame_ck(self, 0, 22.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 4, 0, 0.3f);
            }
            break;
        case 0x7A:
            if (ai_npc_frame_ck(self, 0, 88.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 4, 0, 0.5f);
            }
            break;
        case 0xC8:
            if ((ai_npc_frame_ck(self, 0, 6.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 20.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 38.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x14, 0, 0.5f);
                } else {
                    fn_802D4248(self, 0, 0, 0x14, 0, 0.5f);
                }
            }
            if ((ai_npc_frame_ck(self, 0, 14.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 28.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 44.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x12, 0, 0.5f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.5f);
                }
            }
            break;
        case 0xCA:
            if ((ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 56.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 86.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 118.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 140.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.5f);
            }
            break;
        case 0xCB:
            if ((ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 54.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 82.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 110.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 136.0f, 0.0f) == 1)) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.5f);
            }
            break;
        case 0xCC:
            if ((ai_npc_frame_ck(self, 0, 12.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 18.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 28.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 36.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 44.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 52.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 60.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 68.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x14, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x14, 0, 0.8f);
                }
            }
            if ((ai_npc_frame_ck(self, 0, 16.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 32.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 40.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 48.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 58.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 64.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x12, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.8f);
                }
            }
            break;
        case 0xCD:
            if (ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.5f);
            }
            if ((ai_npc_frame_ck(self, 0, 70.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 118.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x12, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.8f);
                }
            }
            break;
        case 0xCE:
            if ((ai_npc_frame_ck(self, 0, 24.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 32.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 50.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 58.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 64.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x14, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x14, 0, 0.8f);
                }
            }
            if ((ai_npc_frame_ck(self, 0, 14.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 28.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 38.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 46.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 54.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 62.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x12, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.8f);
                }
            }
            break;
        case 0xCF:
            if (ai_npc_frame_ck(self, 0, 20.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.5f);
            }
            if ((ai_npc_frame_ck(self, 0, 78.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 98.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x30, 0x12, 0, 0.4f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.4f);
                }
            }
            break;
        case 0xD0:
            if (ai_npc_frame_ck(self, 0, 30.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x2A, 0xFF, 0, 0.5f);
                } else {
                    fn_802D4248(self, 0, 0x29, 0xFF, 0, 1.0f);
                }
            }
            break;
        case 0xD1:
            if (ai_npc_frame_ck(self, 0, 28.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 0x2A, 0xFF, 0, 0.5f);
                } else {
                    fn_802D4248(self, 0, 0x29, 0xFF, 0, 1.0f);
                }
            }
            break;
        case 0xD2:
            if ((ai_npc_frame_ck(self, 0, 26.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 60.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 84.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 106.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0x14, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x14, 0, 0.8f);
                }
            }
            if (ai_npc_frame_ck(self, 0, 44.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0x12, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.8f);
                }
            }
            break;
        case 0xD3:
            if ((ai_npc_frame_ck(self, 0, 26.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 60.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 84.0f, 0.0f) == 1) || (ai_npc_frame_ck(self, 0, 106.0f, 0.0f) == 1)) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0x12, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x12, 0, 0.8f);
                }
            }
            if (ai_npc_frame_ck(self, 0, 44.0f, 0.0f) == 1) {
                if ((self->field_0x324 & 6) != 0) {
                    fn_802D4248(self, 0, 1, 0x14, 0, 0.8f);
                } else {
                    fn_802D4248(self, 0, 0, 0x14, 0, 0.8f);
                }
            }
            break;
        case 0xD4:
            if (ai_npc_frame_ck(self, 0, 34.0f, 0.0f) == 1) {
                fn_802D4248(self, 0, 0x2B, 0xFF, 0, 0.5f);
            }
            break;
        }
        if (self->variant == 3) {
            switch (::ai_get_motion_no((::_AINPC_W*)self)) {
            case 3:
            case 57:
            case 61:
            case 62:
            case 64:
            case 67:
            case 79:
                if ((system_w.field_0x0c & 0xF) == 0) {
                    if ((50.0f + self->pos_0x178.y) >= self->field_0x1A0) {
                        fn_802D4248(self, 0, 3, 0xFF, 0, 0.5f);
                    }
                }
                break;
            case 63:
                break;
            case 0xD:
                if (ai_npc_frame_ck(self, 0, 38.0f, 0.0f) == 1) {
                    if (self->pos_0x178.y <= (self->field_0x1A0 - 50.0f)) {
                        setVector3(&offset, 0.0f, 0.0f, 0.0f);
                        eft013_npc_place(self, &offset, 0xE, 1.0f, 3);
                    }
                }
                break;
            }
        } else {
            switch (::ai_get_motion_no((::_AINPC_W*)self)) {
            case 1:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 20:
            case 21:
            case 24:
            case 27:
            case 28:
            case 29:
            case 37:
            case 41:
            case 43:
            case 44:
            case 45:
            case 100:
            case 102:
            case 105:
            case 106:
                if (((self->field_0x324 & 6) != 0) && ((19.5f + self->pos_0x178.y) < self->field_0x1A0) && ((system_w.field_0x0c & 0xF) == 0)) {
                    fn_802D4248(self, 0, 3, 0xFF, 0, 0.9f);
                }
                break;
            case 42:
                break;
            }
        }
        if (self->variant == 3) {
            if ((self->pos_0x178.y <= (self->field_0x1A0 - 100.0f)) && (::ai_get_motion_no((::_AINPC_W*)self) != 0xD)) {
                if (self->pos_0x178.y <= (50.0f + self->field_0x1A0)) {
                    if (system_w.field_0x0c % 90 == 0) {
                        setVector3(&offset, 0.0f, -5.0f, 20.0f);
                        eft013_npc_place(self, &offset, 0xE, 1.0f, 0);
                    }
                }
            }
        }
    }
}

}  /* extern "C" */

}  /* namespace view_fn_802D0F34 */

/* 0x802D27EC - whether the NPC is in the area the game is currently in. */
s32 ai_area_ck(struct _AINPC_W* self)
{
    return self->field_0x1A4 == get_now_areano();
}

/* 0x802D29B0 - the motion number the model is playing. */
u16 ai_get_motion_no(struct _AINPC_W* self)
{
    return ((view_fn_802D0F34::_AINPC_W*)self)->model.motion_no_0x50;
}

/* 0x802D2B70 - the model's joint world position. */
void get_joint_wpos_ai(struct _AINPC_W* self, u32 joint, nw4r::math::VEC3* out)
{
    ((view_fn_802D0F34::_AINPC_W*)self)->model.get_joint_wpos(joint, out);
}


#include "ai/ainpc_move.h"           /* this unit's own leaf header */
#include "lobby/lb_quest_screen.h"   /* quest_time_limit_get, quest_time_elapsed_get (rule 2) */
#include "lobby/lb_server_sel_trans.h" /* event_demo_ck, get_demo_no (rule 2) */
#include "menu/menu_item.h"          /* GetItemData, ItemDataRecord (rule 2) */
#include "mh3_pad/Screen_w.h"        /* Screen_w (rule 2) */
#include "Pl/pl_act.h"               /* pl_item_room_get (rule 2) */
#include "Pl/fn_80262940.h"          /* pl_model_state_set (rule 2) */
#include "Pl/fn_80273B14.h"          /* Pl_item_id_usable_ck (rule 2) */

#pragma peephole off

extern "C" {

/* 0x802D6534 - arms the "hold item" timer when the player is within reach: latches the +0x43D gate once and gives the
 * +0x43E timer the long or short value by the distance test. */
void ai_npc_hold_item_arm(void)
{
    _AINPC_W* w;

    if (quest_move_state_valid_ck() == 0) {
        return;
    }
    w = &ainpc_w;
    if (ainpc_w.active == 0) {
        return;
    }
    if (w->field_0x171 == 5) {
        return;
    }
    if (w->field_0x43D != 0) {
        return;
    }
    w->field_0x43D = 1;
    if (vec3_distance(&w->vec_0x178, (nw4r::math::VEC3*)&w->plw_0x16C->motion_pos_0x3C) >= lbl_8079A770) {
        w->field_0x43E = 0x258;
    } else {
        w->field_0x43E = 0x12C;
    }
}

/* 0x802D65CC - the hold-item countdown: past 0x384 frames it fires the release animation and the
 * effect, otherwise it re-arms from the per-record tuning table. */
void fn_802D65CC(struct _AINPC_W* self)
{
    if (self->field_0x422 > 0x384) {
        self->field_0x422 = 0;
        fn_800DCCF8(self->sound_0x498, &self->vec_0x178, 1);
        ai_npc_talk(self, (s8)self->field_0x482, 0x11, 0, 2);
    } else if (self->variant != 2) {
        self->field_0x422 = (s16)(lbl_805D5354[self->field_0x33A] * 0x1E);
        fn_800DCCF8(self->sound_0x498, &self->vec_0x178, 0);
        ai_npc_talk(self, (s8)self->field_0x482, 0x12, 0, 2);
    }
}

/* 0x802D6690 - the hold countdown's frame budget, from the per-record tuning table's +0x3 byte. */
#pragma opt_propagation off
void fn_802D6690(struct _AINPC_W* self)
{
    struct AINPCTuning* row = &lbl_805D5360[self->field_0x33A];

    self->field_0x424 = row->field_0x3 * 0x708;
}
#pragma opt_propagation reset

/* 0x802D66B8 - the weighted three-way choice: 1 while the roll is under the first threshold, 2 at
 * or past the sum of both, 0 in between. */
s32 fn_802D66B8(struct _AINPC_W* self)
{
    s16 roll = (s16)((u16)ran_suu(1) % 100);
    s16 first = (s16)lbl_805D5360[self->field_0x33A].field_0x0;

    if (roll < first) {
        return 1;
    }
    return (roll < first + lbl_805D5360[self->field_0x33A].field_0x1) ? 0 : 2;
}

/* 0x802D675C - holds the sub-state while the frame budget is running, then re-arms both timers from
 * the second tuning table and starts the release animation. */
void fn_802D675C(struct _AINPC_W* self)
{
    if (self->variant != 2) {
        return;
    }
    if (self->field_0x424 > 0) {
        if (self->field_0x422 == 0) {
            ai_npc_talk(self, (s8)self->field_0x482, 0x11, 0, 2);
        }
    } else {
        s16 first = (s16)(lbl_805D5378[self->field_0x33A * 2] * 0x1E);
        self->field_0x422 = first;
        self->field_0x424 = (s16)(first + lbl_805D5378[self->field_0x33A * 2 + 1] * 0x1E);
        ai_npc_talk(self, (s8)self->field_0x482, 0x12, 0, 2);
    }
}

/* 0x802D67F8 - the same hold/ re-arm pair as `fn_802D675C` but from the third tuning table. */
void fn_802D67F8(struct _AINPC_W* self)
{
    if (self->field_0x424 > 0) {
        if (self->field_0x422 == 0) {
            ai_npc_talk(self, (s8)self->field_0x482, 0x11, 0, 2);
        }
    } else {
        s16 first = (s16)(lbl_805D5390[self->field_0x33A * 2] * 0x1E);
        self->field_0x422 = first;
        self->field_0x424 = (s16)(first + lbl_805D5390[self->field_0x33A * 2 + 1] * 0x1E);
        ai_npc_talk(self, (s8)self->field_0x482, 0x12, 0, 2);
    }
}

/* 0x802D6888 - rebuilds the behaviour flag word at +0x20C from the hold counters and the sub-state
 * of the +0x171 == 4 reaction state. */
void fn_802D6888(struct _AINPC_W* self)
{
    u16 flags = 0;

    if (self->field_0x21C != 0) {
        flags |= 1;
    }
    if (self->field_0x23C != 0) {
        flags |= 0x2000;
    }
    if ((s16)(self->field_0x234 + self->field_0x235) > 0) {
        flags |= 0x10;
    }
    if ((s16)(self->field_0x238 + self->field_0x239) > 0) {
        flags |= 0x40;
    }
    if (self->field_0x171 == 4) {
        switch (self->field_0x172) {
        case 8:
        case 0x13:
            flags |= 4;
            break;
        case 9:
        case 0x14:
            flags |= 8;
            break;
        case 0xA:
        case 0x15:
            flags |= 2;
            break;
        }
    }
    self->field_0x20C = flags;
}

/* 0x802D6964 - raises one of the two halves of the first held-item counter. */
void fn_802D6964(struct _AINPC_W* self, s32 slot, s8 value)
{
    if (slot == 0) {
        if (self->field_0x234 < value) {
            self->field_0x234 = value;
        }
    } else if (self->field_0x235 < value) {
        self->field_0x235 = value;
    }
}

/* 0x802D69A4 - raises one of the two halves of the second held-item counter. */
void fn_802D69A4(struct _AINPC_W* self, s32 slot, s8 value)
{
    if (slot == 0) {
        if (self->field_0x238 < value) {
            self->field_0x238 = value;
        }
    } else if (self->field_0x239 < value) {
        self->field_0x239 = value;
    }
}

/* 0x802D69E4 - latches the +0x23C flag and raises the +0x23E timer. */
void fn_802D69E4(struct _AINPC_W* self, u8 flag, s16 value)
{
    self->field_0x23C = flag;
    if (self->field_0x23E < value) {
        self->field_0x23E = value;
    }
}

/* 0x802D6A00 - re-rolls the four per-direction slot levels: the two 0/1 rolls at +0x3F6/+0x3F7
 * pick four entries of the shared level tables, and the best of them becomes the +0x414 timer. */
void fn_802D6A00(struct _AINPC_W* self)
{
    s16 best = 0;
    u16 w[4];
    u8 i;

    self->field_0x414 = 0;
    w[0] = (u16)(self->field_0x3F6 * 4 + self->field_0x3F6);
    w[1] = (u16)(self->field_0x3F6 * 4 + self->field_0x3F7);
    w[2] = (u16)(self->field_0x3F7 * 4 + self->field_0x3F7);
    w[3] = (u16)(self->field_0x3F7 * 4 + self->field_0x3F6);
    for (i = 0; i < 4; i++) {
        u16 v = w[i];
        s16 value;

        self->field_0x3F8[i] = lbl_805D4190[v];
        self->field_0x3FC[i] = lbl_805D41A0[v];
        self->field_0x400[i] = lbl_805D41B0[v];
        value = (u8)ai_npc_level_rank_get(self->field_0x3FC[i]);
        if (best <= value) {
            best = value;
        }
    }
    self->field_0x414 = fn_802D77DC((u8)best);
    {
        u16 mask = self->field_0x400[0];

        for (i = 1; i < 4; i++) {
            mask = mask | self->field_0x400[i];
        }
        self->field_0x3F4 = mask;
    }
}

/* 0x802D6B2C - moves the +0x3D4 gauge and clamps it into [0, +0x3D6], then mirrors +0x3DA. */
void fn_802D6B2C(struct _AINPC_W* self, s16 delta)
{
    self->field_0x3D4 += delta;
    if (self->field_0x3D4 > self->field_0x3D6) {
        self->field_0x3D4 = self->field_0x3D6;
    }
    if (self->field_0x3D4 < 0) {
        self->field_0x3D4 = 0;
    }
    self->field_0x3DC = self->field_0x3DA;
}

/* 0x802D6B6C - raises the +0x3D6 cap by `delta`, clamped at 0x96. */
void fn_802D6B6C(struct _AINPC_W* self, s16 delta)
{
    if (self->field_0x3D6 < 0x96) {
        self->field_0x3D6 += delta;
        if (self->field_0x3D6 >= 0x96) {
            self->field_0x3D6 = 0x96;
        }
    }
}

/* 0x802D6B98 - latches the +0x3D8 gauge into both the value and its cap, and mirrors +0x3DA. */
void fn_802D6B98(struct _AINPC_W* self)
{
    self->field_0x3D6 = self->field_0x3D8;
    self->field_0x3D4 = self->field_0x3D8;
    self->field_0x3DC = self->field_0x3DA;
}

/* 0x802D7464 - the skill load-out: copies the three skill slots out of `lb_param_w`, bumps the two skill counters per
 * unlocked skill and scales the +0x3D4 gauge by the resulting percentage. */
void fn_802D7464(struct _AINPC_W* self)
{
    s32 scale = 100;

    self->field_0x431 = 0;
    self->field_0x434 = 100;
    self->field_0x438 = 100;
    self->skill_0x42E[0] = lb_param_w.sub_0x26;
    self->skill_0x42E[1] = lb_param_w.sub_0x27;
    self->skill_0x42E[2] = lb_param_w.sub_0x28;
    if (ai_skill_ck(self, 2) == 1 || ai_skill_ck(self, 3) == 1 || ai_skill_ck(self, 4) == 1 ||
        ai_skill_ck(self, 5) == 1 || ai_skill_ck(self, 6) == 1 || ai_skill_ck(self, 8) == 1 ||
        ai_skill_ck(self, 9) == 1 || ai_skill_ck(self, 0xA) == 1) {
        self->field_0x431 = 1;
    }
    if (ai_skill_ck(self, 0xE) == 1) {
        self->field_0x434 += 0x14;
    }
    if (ai_skill_ck(self, 0xF) == 1) {
        self->field_0x438 += 0x14;
    }
    if (ai_skill_ck(self, 0x13) == 1) {
        self->field_0x434 += 0x14;
        self->field_0x438 += 0x14;
        scale = 0x78;
    }
    if (ai_skill_ck(self, 0x16) == 1) {
        self->field_0x434 += 0x32;
        self->field_0x438 += 0x32;
        scale += 0x32;
    }
    self->field_0x3D4 = (s16)(self->field_0x3D4 * scale / 100);
    self->field_0x3D8 = self->field_0x3D6 = self->field_0x3D4;
}

/* 0x802D7688 - which of the four behaviour lists the given id belongs to (3 when in none). */
#pragma peephole on
s32 ai_npc_skill_group_get(u8 value)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        u8* p = (u8*)lbl_805D4390[i];

        while (*p != 0) {
            if (*p == value) {
                return i;
            }
            p++;
        }
    }
    return 3;
}
#pragma peephole reset

}

/* One behaviour record the 0x805D4358 table points at: the byte rows `ef/eft050.cpp` reads, then the scale
 * `ai_behaviour_scaled_get` multiplies by.  size: 0x18 (approximate, evidenced to +0x14) */
struct AINPCBehaviour {
    /* +0x00 */ u8 rows[0x14];
    /* +0x14 */ f32 scale;
};

extern "C" {

/* 0x802D773C - the behaviour-list record for the given index. */
struct AINPCBehaviour* ai_npc_behaviour_get(u8 index)
{
    return lbl_805D4358[index];
}

/* 0x802D7754 - the four hold-countdown tuning values for the two 0/1 rolls at +0x3F6/+0x3F7. */
void ai_npc_pair_values_get(u8 first, u8 second, u16* out)
{
    out[0] = lbl_805D41A0[first * 5];
    out[1] = lbl_805D41A0[first * 4 + second];
    out[2] = lbl_805D41A0[second * 5];
    out[3] = lbl_805D41A0[second * 4 + first];
}

/* 0x802D77A0 - the 14-entry level lookup: which of the three level values a slot level index maps
 * to (the table at 0x805D5428 is MWCC's own output for this switch). */
s32 ai_npc_level_rank_get(u8 value)
{
    switch (value) {
    default:
        return 0;
    case 1:
    case 3:
    case 6:
    case 9:
    case 12:
    case 13:
        return 1;
    case 2:
    case 4:
    case 7:
    case 10:
    case 11:
        return 2;
    }
}

/* 0x802D77DC - the +0x414 timer for the best slot level, in frames. */
s32 fn_802D77DC(u8 slot)
{
    switch (slot) {
    case 1:
        return 0x32;
    case 2:
        return 0x46;
    }
    return 0x1E;
}

/* 0x802D7A50 - raises the player-side AI NPC's "release" flag when the record is live, then
 * forwards to the shared dispatcher. */
void ainpc_call_request(void)
{
    _AINPC_W* w = &ainpc_w;

    if (w->active == 0) {
        return;
    }
    w->field_0x440 = 1;
    fn_802D4218(w);
}

/* 0x802D7A74 - whether one of the four hold slots is holding the live-chain item (0x1D) while the
 * AI NPC is in the +0x420 == 4 state. */
s32 fn_802D7A74(struct _AINPC_W* self)
{
    if (self->field_0x420 != 4) {
        return 0;
    }
    if (self->field_0x1CC == 0 || self->field_0x1CC == 5) {
        if (self->item_slots[8].id == 0x1D && self->field_0x424 == 0) {
            return 1;
        }
        if (self->item_slots[9].id == 0x1D && self->field_0x424 == 0) {
            return 1;
        }
        if (self->item_slots[10].id == 0x1D && self->field_0x424 == 0) {
            return 1;
        }
        if (self->item_slots[11].id == 0x1D && self->field_0x424 == 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x802D7B24 - whether the +0x171 == 1 state's sub-state is one of the two 0x20/0x3D.. ranges. */
s32 fn_802D7B24(struct _AINPC_W* self)
{
    if (self->field_0x171 == 1
        && ((u32)(self->field_0x172 - 0x20) <= 0x13 || (u32)(self->field_0x172 - 0x3D) <= 0x13)) {
        return 1;
    }
    return 0;
}

/* 0x802D7B5C - the hold-slot record index the item id falls in. */
u8 ai_npc_level_get(u16 value)
{
    u8 i = 1;

    while (value >= lbl_805D43A0[(i + 1) * 4]) {
        if (++i >= 30) {
            break;
        }
    }
    return i;
}

/* 0x802D7BA4 - the second field of the hold-slot record. */
u16 fn_802D7BA4(u8 index)
{
    return lbl_805D43A0[index * 4 + 1];
}

/* 0x802D7BC4 - the third field of the hold-slot record. */
u16 fn_802D7BC4(u8 index)
{
    return lbl_805D43A0[index * 4 + 2];
}

/* 0x802D7BE4 - the fourth field of the hold-slot record. */
u16 fn_802D7BE4(u8 index)
{
    return lbl_805D43A0[index * 4 + 3];
}

/* 0x802D7C04 - the hold-range record index the item id falls in. */
u8 fn_802D7C04(u16 value)
{
    u8 i = 1;

    while (value >= lbl_805D44EC[(i + 1) * 2]) {
        if (++i >= 20) {
            break;
        }
    }
    return i;
}

/* 0x802D7C4C - the upper bound of the hold-range record. */
u16 fn_802D7C4C(u8 index)
{
    return lbl_805D44EC[index * 2 + 1];
}

/* 0x802D7C6C - the hold-range record index the item id falls in, over the 0x805D4560 table. */
u8 fn_802D7C6C(u16 value)
{
    u8 i = 0;

    while (value >= lbl_805D4560[i + 1]) {
        if (++i >= 5) {
            break;
        }
    }
    return i;
}

/* 0x802D7CB0 - the hang time the AI NPC's wake-up skill grants. */
s32 fn_802D7CB0(struct _AINPC_W* self)
{
    if (ai_skill_ck(self, 0x12) == 1) {
        return 5;
    }
    return 0xA;
}

/* 0x802D7CE4 - the "found the player" reaction: state 0x1A in a live area drops the AI NPC into
 * step 7. */
void ainpc_demo_start(u8 state)
{
    _AINPC_W* w = &ainpc_w;

    if (state != 0x1A) {
        return;
    }
    if (w->active == 0) {
        return;
    }
    if (ai_area_ck(w) == 0) {
        return;
    }
    fn_802D2A00(w, 7, 0, 0);
}

/* 0x802D7D4C - the matching "lost the player" reaction for the +0x171 == 7 state. */
void ainpc_demo_end(void)
{
    _AINPC_W* w = &ainpc_w;

    if (w->active == 0) {
        return;
    }
    if (ai_area_ck(w) == 0) {
        return;
    }
    if (w->field_0x171 != 7) {
        return;
    }
    fn_802D2A00(w, 7, 1, 0);
}

/* 0x802D7DB4 - whether the +0x420 == 5 state's timer is running. */
s32 ai_npc_hold_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    if (self->field_0x420 != 5) {
        return 0;
    }
    return self->field_0x422 > 0;
}

/* 0x802D7DF0 - sets the +0x374 gauge. */
void fn_802D7DF0(struct _AINPC_W* self, s16 value)
{
    self->field_0x374 = value;
}

/* 0x802D7DF8 - whether the +0x374 gauge is running. */
s32 ai_npc_wait_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return self->field_0x374 > 0;
}

/* 0x802D7E20 - whether the AI NPC has arrived (its step counter is at 1). */
s32 ai_npc_arrived_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return ai_npc_motion_step_ck(self, 5, 3) == 1;
}

/* 0x802D7E68 - the same arrival test for the other motion slot. */
s32 ai_npc_motion5_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return ai_npc_motion_step_ck(self, 5, 2) == 1;
}

/* 0x802D7F10 - whether the id is one of the three slots of the list. */
s32 fn_802D7F10(u8* slots, u8 value)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (slots[i] == value) {
            return 1;
        }
    }
    return 0;
}

/* 0x802D84D0 - sets the +0x47C gauge. */
void fn_802D84D0(struct _AINPC_W* self, s16 value)
{
    self->field_0x47C = value;
}

/* 0x802D948C - whether the item page at +0x483 is the first one. */
u32 ai_npc_mode1_ck(struct _AINPC_W* self)
{
    return self->field_0x483 == 1;
}

/* 0x802D94A0 - whether the item page at +0x483 is the first or the third one. */
s32 fn_802D94A0(struct _AINPC_W* self)
{
    if (self->field_0x483 == 2) {
        return 1;
    }
    return self->field_0x483 == 3;
}

/* 0x802D9A40 - forwards to the page-arrow routine. */
void fn_802D9A40(void)
{
    ai_npc_tutorial_talk();
}

/* 0x802D9A44 - marks the AI NPC as having acknowledged the current item page. */
void fn_802D9A44(struct _AINPC_W* self)
{
    self->field_0x46C = 1;
}

/* 0x802D9A50 - whether the acknowledgement has already happened. */
s32 ai_npc_call_ck(struct _AINPC_W* self)
{
    if (self->active == 0) {
        return 0;
    }
    return self->field_0x46C == 1;
}

/* 0x802D7804 - whether the AI NPC is close enough to the player to act on it. */
s32 ai_npc_player_near_ck(u8 index, f32 distance)
{
    _AINPC_W* w = &ainpc_w;

    if (w->active == 0) {
        return 0;
    }
    if (w->plw_0x16C == 0) {
        return 0;
    }
    if (distance < lbl_8079A670) {
        return 1;
    }
    if (w->field_0x1A4 != w->plw_0x16C->area_0x16) {
        return 0;
    }
    if (w->field_0x171 == 5) {
        return 0;
    }
    if (index == 4 && ai_npc_item_step_ck(w) == 1) {
        return 0;
    }
    if (vec3_dist_sq(&w->vec_0x178, (nw4r::math::VEC3*)&w->plw_0x16C->motion_pos_0x3C) < distance * distance) {
        return 1;
    }
    return 0;
}

/* 0x802D78FC - whether the player is inside the AI NPC's current attack reach. */
s32 ai_npc_player_in_reach_ck(void)
{
    _AINPC_W* w = &ainpc_w;

    if (w->active == 0) {
        return 0;
    }
    if (w->field_0x420 != 2) {
        return 0;
    }
    if (w->plw_0x16C == 0) {
        return 0;
    }
    if (w->field_0x1A4 != w->plw_0x16C->area_0x16) {
        return 0;
    }
    if (w->field_0x422 == 0) {
        return 0;
    }
    if (vec3_dist_sq(&w->vec_0x178, (nw4r::math::VEC3*)&w->plw_0x16C->motion_pos_0x3C) < lbl_8079A870) {
        return 1;
    }
    return 0;
}

/* The tables and constants the companion and result helpers below read, in this unit's `.data` / `.sdata2`:
 * declared, never defined (playbook 29). */

/* One entry of `ai_npc_time_left_chance_tbl`.  size: 0x4 */
struct AINPCTimeChance {
    /* +0x0 */ s16 time_left;   /* the quest time left the entry needs (scaled by the frame rate); 0 ends the table */
    /* +0x2 */ s16 chance;      /* the percentage the entry grants */
};

/* One entry of `ai_npc_spawn_pos_tbl`, indexed by +0x484.  size: 0x14 */
struct AINPCSpawnPos {
    /* +0x00 */ u8 area;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ Vec pos;
    /* +0x10 */ u32 yaw;
};

/* One entry of `result_rank_points_tbl`.  size: 0x8 */
struct ResultRankPoints {
    /* +0x0 */ u16 count_index;  /* the count both runs are tested at; 0xFF ends the table */
    /* +0x2 */ u8 pad_0x2[2];
    /* +0x4 */ u16* bits;        /* the point bits the entry sets, 0xFF-terminated */
};

extern struct AINPCTimeChance ai_npc_time_left_chance_tbl[6];
extern u16* ai_npc_motion_variant_tbl[6];
extern u16 result_hunt_rank_tbl[0x2A];
extern s16 result_time_rank_tbl[0x10];
extern struct ResultRankPoints result_rank_points_tbl[0x10];
extern struct AINPCSpawnPos ai_npc_spawn_pos_tbl[4];
extern const f32 ai_npc_hand_reach_dist_sq;

/* 0x802D8550 (0x13C): rolls whether the companion uses its time-left skill, with the chance of the last
 * table entry the quest time left exceeds. */
u32 fn_802D8550(struct _AINPC_W* self)
{
    struct AINPCTimeChance* entry = ai_npc_time_left_chance_tbl;
    s32 left = quest_time_limit_get() - quest_time_elapsed_get();
    u8 chance;

    if (ai_skill_ck(self, 1) == 0) {
        return 0;
    }
    if (self->field_0x446 != 0) {
        return 0;
    }
    chance = entry->chance;
    for (; entry->time_left > 0; entry++) {
        if ((f32)left > (f32)entry->time_left * (lbl_8079A688 * Screen_w.frame_scale)) {
            chance = entry->chance;
        }
    }
    return ran_suu(1) % 100 < chance;
}

/* 0x802D86D4 (0xB4): reports the first held slot (8-11) carrying a giveable item. */
u32 fn_802D86D4(struct _AINPC_W* self, u16* id, s16* count)
{
    if (self->item_slots[8].id != 0 && self->item_slots[8].id != 0x1D) {
        *id = self->item_slots[8].id;
        *count = self->item_slots[8].value;
        return 1;
    }
    if (self->item_slots[9].id != 0 && self->item_slots[9].id != 0x1D) {
        *id = self->item_slots[9].id;
        *count = self->item_slots[9].value;
        return 1;
    }
    if (self->item_slots[10].id != 0 && self->item_slots[10].id != 0x1D) {
        *id = self->item_slots[10].id;
        *count = self->item_slots[10].value;
        return 1;
    }
    if (self->item_slots[11].id != 0 && self->item_slots[11].id != 0x1D) {
        *id = self->item_slots[11].id;
        *count = self->item_slots[11].value;
        return 1;
    }
    *id = 0;
    *count = 0;
    return 0;
}

/* 0x802D8788 (0xC4): returns 1 while the companion's hand-over motion is within reach of its partner. */
s32 ai_npc_hand_reach_ck(void)
{
    struct _AINPC_W* self = &ainpc_w;
    _PLW* plw;

    if (ainpc_w.active == 0) {
        return 0;
    }
    plw = self->plw_0x16C;
    if (plw == 0) {
        return 0;
    }
    if (self->field_0x1A4 != plw->area_0x16) {
        return 0;
    }
    if (ai_npc_motion_step_ck(self, 1, 0x58) == 1 || ai_npc_motion_step_ck(self, 1, 0x59) == 1) {
        if (vec3_dist_sq(&self->vec_0x178, (nw4r::math::VEC3*)&self->plw_0x16C->motion_pos_0x3C) <
            ai_npc_hand_reach_dist_sq) {
            return 1;
        }
    }
    return 0;
}

/* 0x802D895C (0x78): rolls the companion's formation choice against its formation record's percentage. */
void ai_npc_formation_roll(struct _AINPC_W* self)
{
    if (ran_suu(1) % 100 < self->formation_0x41C->roll_percent) {
        self->field_0x384 = 0;
    } else {
        self->field_0x384 = 1;
    }
}

/* 0x802D89D4 (0xE8): picks a random weak-point variant of the motion from the first table rows that list it. */
u16 fn_802D89D4(struct _AINPC_W* self, u16 motion)
{
    u8 count = self->field_0x33A + 1;
    u8 i;

    if (count > 4) {
        count = 4;
    }
    for (i = 0; i <= count; i++) {
        u16* p;

        for (p = ai_npc_motion_variant_tbl[i]; *p != 0xFF; p++) {
            if (*p == motion) {
                u16* row = ai_npc_motion_variant_tbl[i + 1];

                return row[self->weak_point_index * 2 + ran_suu(1) % 2];
            }
        }
    }
    return 0;
}

/* 0x802D8ABC (0x20C): the quest's hunt rank: both count runs weighed by the rank table, plus the bonus. */
s32 result_hunt_rank_get(struct Q_CountSet* counts_c, struct Q_CountSet* counts_d)
{
    s32 sum = 0;
    struct _AINPC_W* self = &ainpc_w;
    u8 i;

    if (quest_move_state_valid_ck() == 0) {
        return 0;
    }
    if (ai_npc_mode1_ck(self) == 1) {
        return 0;
    }
    for (i = 0; i < 0x29; i++) {
        sum += (u16)(result_hunt_rank_tbl[i] * ((u16*)counts_c)[i]);
        sum += (u16)(result_hunt_rank_tbl[i] * ((u16*)counts_d)[i]);
    }
    sum += self->hunt_bonus;
    return sum;
}

/* 0x802D8CC8 (0xCC): the rank of the elapsed minutes, plus the time bonus. */
u16 result_time_rank_get(u16 minutes)
{
    u8 i = 0;
    u8 rank = 0;
    struct _AINPC_W* self = &ainpc_w;

    if (quest_move_state_valid_ck() == 0) {
        return 0;
    }
    if (ai_npc_mode1_ck(self) == 1) {
        return 0;
    }
    for (; result_time_rank_tbl[i] >= 0; i += 2) {
        if (minutes >= result_time_rank_tbl[i]) {
            rank = i;
        }
    }
    return self->time_bonus + result_time_rank_tbl[rank + 1];
}

/* 0x802D8D94 (0x60): returns a value scaled by the behaviour record of the given index. */
u16 ai_behaviour_scaled_get(u16 value, u8 index)
{
    struct AINPCBehaviour* record = ai_npc_behaviour_get(index);

    return (f32)value * record->scale;
}

/* 0x802D8DF4 (0xB4): copies the twelve item slots into the result rows, blanking those not usable. */
void result_rank_rows_fill(struct IdValue* rows)
{
    s32 i;
    s32 locked = 0;
    struct _AINPC_W* self = &ainpc_w;

    if (quest_move_state_valid_ck() == 0) {
        locked = 1;
    }
    if (ai_npc_mode1_ck(self) == 1) {
        locked = 1;
    }
    for (i = 0; i < 12; i++, rows++) {
        if (Pl_item_id_usable_ck(self->item_slots[i].id, 2) == 1 && locked == 0) {
            rows->id = self->item_slots[i].id;
            rows->value = self->item_slots[i].value;
        } else {
            rows->id = 0;
            rows->value = 0;
        }
    }
}

/* 0x802D8EA8 (0xDC): the OR of the point bits of every table entry either count run reached. */
u32 result_rank_points_get(struct Q_CountSet* counts_c, struct Q_CountSet* counts_d)
{
    u32 mask = 0;
    struct _AINPC_W* self = &ainpc_w;
    struct ResultRankPoints* entry;

    if (quest_move_state_valid_ck() == 0) {
        return 0;
    }
    if (ai_npc_mode1_ck(self) == 1) {
        return 0;
    }
    for (entry = result_rank_points_tbl; entry->count_index != 0xFF; entry++) {
        if (((u16*)counts_c)[entry->count_index] != 0 || ((u16*)counts_d)[entry->count_index] != 0) {
            u16* bit;

            for (bit = entry->bits; *bit != 0xFF; bit++) {
                mask |= 1 << *bit;
            }
        }
    }
    return mask;
}

/* 0x802D8F84 (0x80): returns the OR of `1 << n` over the non-zero skill numbers of slots 1..count. */
u32 ai_skill_mask_get(u8 count)
{
    u8 i;
    u32 mask = 0;

    for (i = 1; i <= count; i++) {
        u16 skill = fn_802D7BE4(i);

        if (skill != 0) {
            mask |= 1 << skill;
        }
    }
    return mask;
}

/* 0x802D9400 (0x8C): places the companion at the spawn point of its +0x484 index. */
void fn_802D9400(struct _AINPC_W* self)
{
    struct AINPCSpawnPos* spawn = &ai_npc_spawn_pos_tbl[self->field_0x484];
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    self->field_0x1A4 = spawn->area;
    vec_to_mh_vec3(&pos, &spawn->pos);
    copyVec3(&self->vec_0x178, copyVec3(&self->prev_pos, &pos));
    self->field_0x190 = 0;
    self->field_0x194 = spawn->yaw;
    self->rot_z = 0;
}

#ifdef __cplusplus
}
#endif

/* 0x802D7640 - whether `skill` occupies any of the three skill slots copied out of `lb_param_w`. */
u32 ai_skill_ck(struct _AINPC_W* self, u8 skill)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (skill == self->skill_0x42E[i]) {
            return 1;
        }
    }
    return 0;
}

/* 0x802D7EB0 (0x20): the barrel state while the companion is in its barrel motion (3), -1 otherwise. */
s32 ai_taru_move_ck(struct _AINPC_W* self)
{
    if (self->field_0x171 != 3) {
        return -1;
    }
    return self->field_0x450;
}

/* 0x802D7ED0 (0x40): whether the companion is within the barrel reach of its last move target. */
u8 ai_taru_range_ck(struct _AINPC_W* self)
{
    return vec3_dist_sq(&self->vec_0x178, &self->vec_0x1B0) < lbl_8079A870;
}

/* 0x802D8414 (0x64): whether the AI companion is carrying a lit torch. */
u32 ai_torch_ck(struct _AINPC_W* self)
{
    if (self->field_0x420 != 1) {
        return 0;
    }
    if (ai_npc_flag_ck(self, 2) != 0) {
        return 0;
    }
    return self->field_0x422 > 0;
}

/* 0x802D868C (0x48): whether an event demo other than 26 is running, which stops the companion. */
u32 ai_demo_stop_ck(void)
{
    if (event_demo_ck() == 0) {
        return 0;
    }
    return get_demo_no() != 26;
}

namespace view_fn_802D0F34 {

extern "C" {

extern const f32 ai_npc_f32_0_7;

/* 0x802D84D8 (0x78): the companion's per-frame move step - drift, sub-states, model and motion. */
void ainpc_move(void)
{
    _AINPC_W* self = (_AINPC_W*)&::ainpc_w;

    if (::ai_demo_stop_ck() == 1) {
        return;
    }
    if (self->drift_active != 0) {
        addVec3To(&self->pos_0x178, &self->drift_vel);
        fn_802D15DC(self);
        fn_802D1710(self);
        fn_802D1FFC(self);
        mhchar_model_mtx_update(&self->model);
    }
    ::ai_npc_motion_dispatch((::_AINPC_W*)self);
}

/* 0x802D884C (0x110): hands the companion's held item to its partner and reports the item and count given. */
void ai_npc_item_give(u16* item, s16* count)
{
    _AINPC_W* self = (_AINPC_W*)&::ainpc_w;

    if (::ainpc_w.active == 0) {
        return;
    }
    if (self->plw_0x16C == 0) {
        return;
    }
    if (self->area != self->plw_0x16C->area_0x16) {
        return;
    }
    self->field_0x461 = 0;
    self->field_0x464 = ::pl_item_room_get(self->plw_0x16C, self->field_0x462, self->field_0x464);
    if (self->field_0x464 == 0) {
        ::pl_model_state_set(self->plw_0x16C, 2, 0x11, self->field_0x462);
        *item = 0;
        *count = 0;
    } else {
        ::ItemDataRecord* data;

        ::pl_model_state_set(self->plw_0x16C, 2, 8, self->field_0x462);
        *item = self->field_0x462;
        data = ::GetItemData(self->field_0x462);
        if (self->field_0x464 > data->max_num_0x003) {
            self->field_0x464 = data->max_num_0x003;
        }
        *count = self->field_0x464;
        fn_802D40EC(self, self->field_0x462, (s16)-self->field_0x464);
    }
}

/* 0x802D92F8 (0xC0): books the gauge-ratio message (high, middle or low) on item page 2. */
void fn_802D92F8(struct _AINPC_W* self)
{
    f32 ratio;

    if (self->field_0x485 != 2) {
        return;
    }
    ratio = (f32)self->gauge_0x1F0 / (f32)self->gauge_max_0x1F4;
    if (ratio > ai_npc_f32_0_7) {
        ai_npc_talk(self, 9, 6, 0, 2);
    } else if (ratio > lbl_8079A7B4) {
        ai_npc_talk(self, 9, 7, 0, 2);
    } else {
        ai_npc_talk(self, 9, 8, 0, 2);
    }
}

}

}  /* namespace view_fn_802D0F34 */

/* One 0x20-byte entry of `ainpc_entry_tbl`: `ai_slots_clear` clears the sixteen of them with `fn_802DA200`. */
struct AinpcEntry {
    /* +0x00 */ u8 pad_0x00[0x20];
}; /* size: 0x20 */

/* The page state `ainpc_page_update` and its helpers keep at `ainpc_page_state`: +0x00 and +0x26 are cleared per
 * page, +0x01 is saved across the clear. */
struct AinpcPageState {
    /* +0x00 */ u8 active_0x00;
    /* +0x01 */ u8 saved_0x01;
    /* +0x02 */ u8 pad_0x02[0x24];
    /* +0x26 */ u8 flag_0x26;
    /* +0x27 */ u8 pad_0x27[0x29];
}; /* size: 0x50 */

/* This unit's own `.bss` (0x806BD360-0x806BD808), defined at the foot of the file, after every use.
 * `ainpc_w` is constructed by the `.ctors` word `ainpc_w_sinit` (a tail call into the record's constructor
 * `ainpc_w_construct`, not reconstructed here). */
_AINPC_W ainpc_w;                      /* +0x806BD360 */

