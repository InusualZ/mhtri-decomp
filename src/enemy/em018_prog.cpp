/*
 * enemy/em018_prog.cpp - enemy 018's program: the `em018_prog_tbl` entry points (init, frame update, action
 *   dispatch, event hook, motion events, TEV tint, condition/setup/selection slots), the per-motion state machines
 *   of the six `state_sub` dispatchers, the effect router, the glow fade, the user-data node hook and the static
 *   initializer.
 * RANGE. .text 0x80192348-0x8019E670 (128 functions); extab 0x8000EDC4-0x8000F124, extabindex
 *   0x8002A390-0x8002A8A0, .ctors 0x8056F340-0x8056F344 (`em018_static_init`), .rodata 0x80570050-0x80570150, .data
 *   0x805AD370-0x805AE750 (`em018_prog_tbl` first), .bss 0x806A7A70-0x806A7A88, .sdata 0x80791A78-0x80791A80,
 *   .sdata2 0x80798238-0x80798518.  Left edge: the function after `fn_80192204`, `enemy/em016_prog.cpp`'s static
 *   initializer; right edge: `em018_static_init` (the two vectors at `lbl_806A7A70`) is this TU's static initializer,
 *   and `fn_8019E670` is the first reader of the next 0.0 pool entry `lbl_80798538`.
 * FLAGS. `cflags_main`; `#pragma peephole off` for the whole file (retail keeps the unfused `rlwinm`+`cmpwi`,
 *   `extsh`+`cmpwi` and `subi`+`stw`+`cmpwi` forms throughout).
 * NAMES. The file name follows the runtime dump's `em018_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the program-table slot each fills, `Em024ProgTbl`'s scheme): em018_init, em018_frame_update,
 *   em018_event_notify, em018_motion_events, em018_tev_color_update, em018_action_select, em018_static_init.
 *   GUESS (from the bodies and callers): em018_effect_spawn, em018_drip_effect, em018_glow_fade,
 *   `Em018UserData`; data em018_se_seq_<sub-state>, em018_se_alt_seq_<sub-state>,
 *   em018_demo_keys, em018_pick_a, em018_pick_b; the `EmFadeRec` field names.
 *   The callee names this unit asked for (`enemy-em018#N`) are GUESS except where the owner's comment names the
 *   behaviour.  The `.bss` record names (`vec_pair_801926EC_0`) are GUESSes.
 * RESIDUALS. Partial rows:
 *  - `em018_motion_events`: retail shares the motion 0x55/0x56 water tail (case 0x56 branches into case 0x55's
 *    tail); the source repeats it (160 B long), the only shape without a `goto`;
 *  - `em018_drip_effect`: retail recomputes `angle + 0x8000` at each test, ours keeps it in a register;
 *  - `em018_effect_spawn`, `fn_801991E4`, `fn_80194AE4`, `fn_80198150`, `fn_80195214`: register colouring and
 *    one-instruction schedule differences;
 *  - `fn_8019E398`, `fn_8019E49C`: register allocation on the `||` blocks and the f30 save of the radius test;
 *  - `fn_801934EC`: retail hoists `mr r3,self` before the height ternary;
 *  - `fn_80192448`: one callee-saved register coloured differently (the `work`/`i` pair);
 *  - `em018_glow_fade`: retail compares the fade state 0, 1, 2 with the bodies laid out 1, 2, 0.
 *   The `MHchar::getAmbColor`/`setNumTevStages` calls (0x800E2C38/0x800E2DFC) read as relocation-name
 *   differences while those map rows keep their generated names.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata` claimed, not emitted; `.data` and `.sdata2` not emitted in retail
 *   order (the SE tables, demo keys and pick records are `extern`).
 * SHAPES. `Em018UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 *   `u8 >= 2` in `fn_8019238C` compiles to the borrow sequence (`li r3,2; orc; addi; srwi; subf; srwi`):
 *   the source keeps the comparison.  `fn_80192448` declares its locals `max`, `i`, `work`, which gives retail's
 *   r31/r30/r29.  A one-case parameter test that retail compares signed (`cmpwi`) is a `switch` with `default:`
 *   first (`fn_80196E10`); a `cmplwi` range test with the in-range body last is `if ((u32)(x - 1) > 1) {...} else`
 *   (`fn_80195720`); the four-part frame-cost scan is a `for` loop MWCC unrolls (`fn_801963A8`).  Float literals
 *   are written in place (the pool is not emitted yet).  `em_water_check` is called through a `u32` view: the
 *   owner header declares an `f32` return, which compiles the `== 1` test as a float compare.
 */

#include "enemy/lbl_80797E88.h" /* lbl_80797E88 (rule 2: the owner's header) */
#include "enemy/lbl_8079800C.h" /* lbl_8079800C (rule 2: the owner's header) */
#include "ef/mtx34_trans_add.h" /* mtx34_trans_add (rule 2: the owner's header) */
#include "ef/mtx34_trans_get.h" /* mtx34_trans_get (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "enemy/em016_prog_types.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h" /* fn_801280F4, fn_80128030 */
#include "enemy/fn_8012BDF4.h" /* em_busy_set */
#include "enemy/fn_8012EC74.h" /* em_camera_req */
#include "enemy/fn_80138074.h"  /* em_res_user_data_set, em_res_user_data_ck, ResUserDataAc */
#include "MSL/new.h" /* the placement `operator new` */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "enemy/fn_80191598.h" /* fn_80192370, fn_80192618 */
#include "fn_8004CAD8.h"       /* calcDistanceSqXZ */
#include "unsplit/enemy.h"     /* the plain enemy-band callees (rule 2 band header) */
#include "ef/eft009.h"          /* eft009_set_pos / eft009_spawn_at_joint / eft009_spawn_at_pos */
#include "ef/fn_80105314.h"     /* eft_spawn_type10 / eft_spawn_type11 / eft013_spawn_id / eft013_spawn */
#include "ef/fn_8010D1A8.h"     /* eft_spawn_pos_in_area */
#include "lobby/fn_8030121C.h"  /* eft_em_spawn */
#include "lobby/fn_801E0ADC.h"  /* eft_em_spawn_joint */
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr and its slots */
#include "mh3_pad/system_w.h"   /* system_w */
#include "Pl/plw.h"             /* _PLW */
#include "Pl/Pl_master_ck.h"     /* Pl_act_ck */
#include "Pl/fn_80273B14.h"     /* fn_802754C0 */
#include "enemy/em_model.h"     /* em_roster_record_release / em_roster_slot_effect_set */
#include "g3d/g3d_calcworld.h"  /* addVec3To */
#include "sound/fn_800D7F54.h"   /* se_req_pos_ps */
#include "stage/stg_w.h"        /* get_now_areano */
#include "enemy/em_se_record.h"  /* EmSeEntry */
#include "enemy/em_se_tbl_play.h" /* em_se_tbl_play */
#include "enemy/em_se_tbl_play_alt.h" /* em_se_tbl_play_alt */
#include "ef/eft026_pl_spawn.h"     /* eft026_pl_spawn */
#include "sound/mhchar.h"        /* MHchar */
#include "enemy/em_flags836_ck.h" /* em_flags836_ck */
#include "lobby/lb_quest_screen.h" /* quest_arena_key_set */
#include "MSL/mathf.h"           /* cosf / sinf */
#include "sound/se_req.h"        /* shell_se_req2 */
#include "sound/fn_800DD1F0.h"   /* mhchar_mat_blend_set */
#include "enemy/em_angle_approach.h"  /* em_angle_approach */
#include "enemy/em_turn_in_window.h" /* em_turn_in_window */
#include "Pl/pl_act.h"   /* pl_act_grabbed_ck, pl_act_hold_motion_set */
#include "enemy/fn_8011D448.h"   /* em_gauge_step, em_gauge_set, em_eft_fx_spawn, em_eft_fx_spawn_pair */
#include "menu/hit_list_capsule_ck.h" /* hit_list_capsule_ck */
#include "ef/effect.h"            /* eft_vec_ctor */
#include "ef/eft022_spawn.h"      /* eft022_spawn */
#include "ef/eft_capsule_ctor.h"   /* eft_capsule_ctor */
#include "enemy/em_roll_cooldown_ck.h" /* em_roll_cooldown_ck */
#include "g3d/anm_chr_result_scale_get.h" /* anm_chr_result_scale_get / _trans_get / _scale_set / _trans_set */
#include "camera/camera_mode12_set.h" /* camera_mode12_set */
#pragma peephole off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define mtx34_trans_add_c1 ((void (*)(MTX34*, VEC3*))mtx34_trans_add)
#define mtx34_trans_get_c1 ((void (*)(MTX34*, VEC3*))mtx34_trans_get)
#define eft026_pl_spawn_c1 ((void (*)(struct _ENEMY_WORK*, u8, nw4r::math::VEC3*, u32, f32))eft026_pl_spawn)
#define assignVec3_c1 ((void (*)(VEC3*, VEC3*))assignVec3)
#define em_water_check_c1 ((u32 (*)(struct _ENEMY_WORK*))em_water_check)
#define em_fall_height_get_c1 ((f32 (*)(struct _ENEMY_WORK*))em_fall_height_get)
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
#define em_move_offset_step_update_c1 ((u32 (*)(struct _ENEMY_WORK*, u32*, f32))em_move_offset_step_update)
#define em_area_break_ck_c2 ((u8 (*)(struct _ENEMY_WORK*, u8))em_area_break_ck)
#define em_roll_cooldown_ck_c1 ((s32 (*)(struct _ENEMY_WORK*, u16, u16, s16*))em_roll_cooldown_ck)
#define anm_chr_result_scale_get_c1 ((void (*)(nw4r::g3d::ChrAnmResult*, VEC3*))anm_chr_result_scale_get)
#define anm_chr_result_trans_get_c1 ((void (*)(nw4r::g3d::ChrAnmResult*, VEC3*))anm_chr_result_trans_get)
#define anm_chr_result_scale_set_c1 ((void (*)(nw4r::g3d::ChrAnmResult*, VEC3*))anm_chr_result_scale_set)
#define anm_chr_result_trans_set_c1 ((void (*)(nw4r::g3d::ChrAnmResult*, VEC3*))anm_chr_result_trans_set)
#define camera_mode12_set_c1 ((void (*)(struct _ENEMY_WORK*))camera_mode12_set)

/* ------------------------------------------------------------------------------------------------ */
/* the unit's pool, declared, not defined                                                            */
/* ------------------------------------------------------------------------------------------------ */

extern const f32 lbl_80798238;

extern const f32 lbl_80798244;
extern const f32 lbl_80798248;
extern const f32 lbl_8079824C;
extern const f32 lbl_80798250;
extern const f32 lbl_80798254;

/* ------------------------------------------------------------------------------------------------ */
/* callees                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* The mangled callees, outside `extern "C"` so the front-end mangles them the way the map spells them
 * (rule 9).  Their parameter widths are the ones the map's manglings encode. */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* work, u8 part);
u16 get_move_work_max(u8 kind);
void* get_move_work_adrs(u8 kind);
f32 get_em_scale(struct _ENEMY_WORK* work);
void get_joint_wmat_em(struct _ENEMY_WORK* work, u32 joint, MTX34* mtx);
void senko_set(VEC3* pos, f32 value, u8 a, s16 b);

extern "C" {

void fn_80192348(EmActWork* self, u8 mask);
void fn_80192358(EmActWork* self, u8 mask);
u32 fn_8019238C(EmActWork* self);
u32 fn_801923CC(EmActWork* self);
u32 fn_801923E0(EmActWork* self);
u32 fn_80192410(EmActWork* self);
f32 em018_glow_get(EmActWork* self);
s32 fn_80192448(EmActWork* self);
void fn_8019255C(EmActWork* self);
void fn_80192630(EmActWork* self);
}

/* The unit's `.sdata2` and `.data` labels, declared, not defined: the source does not emit them yet. */
extern f32 lbl_80798264;
extern f32 lbl_80798268;
extern f32 lbl_8079826C;
extern f32 lbl_80798270;
extern f32 lbl_80798260;
extern f32 lbl_8079827C;
extern f32 lbl_80798288;
extern f32 lbl_80798290;
extern f32 lbl_80798298;

extern f32 lbl_80798308;
extern f32 lbl_8079830C;
extern f32 lbl_80798310;
extern f32 lbl_80798314;
extern f32 lbl_80798328;
extern f32 lbl_8079832C;
extern f32 lbl_80798338;
extern f32 lbl_80798370;
extern f32 lbl_80798398;
extern f32 lbl_8079839C;
extern f32 lbl_807983A0;
extern f32 lbl_807982E0;
extern f32 lbl_807983B0;
extern f32 lbl_807983F0;
extern f32 lbl_807983F4;
extern f32 lbl_807983F8;
extern f32 lbl_80798510;
/* The per-sub-state sound programs `fn_80198910`/`fn_80198E00` play (`.data`, entry-list tables of
 * `EmSeEntry`; the names carry the first sub-state that plays each and are a GUESS). */
extern EmSeEntry em018_se_seq_17[]; /* 0x805AD5E0 */
extern EmSeEntry em018_se_seq_18[]; /* 0x805AD608 */
extern EmSeEntry em018_se_seq_7A[]; /* 0x805AD630 */
extern EmSeEntry em018_se_seq_7B[]; /* 0x805AD658 */
extern EmSeEntry em018_se_seq_9F[]; /* 0x805AD680 */
extern EmSeEntry em018_se_seq_7C[]; /* 0x805AD6A8 */
extern EmSeEntry em018_se_seq_7D[]; /* 0x805AD6E8 */
extern EmSeEntry em018_se_seq_8D[]; /* 0x805AD720 */
extern EmSeEntry em018_se_seq_28[]; /* 0x805AD748 */
extern EmSeEntry em018_se_seq_29[]; /* 0x805AD770 */
extern EmSeEntry em018_se_seq_89[]; /* 0x805AD798 */
extern EmSeEntry em018_se_seq_8A[]; /* 0x805AD7C8 */
extern EmSeEntry em018_se_seq_8B[]; /* 0x805AD7F0 */
extern EmSeEntry em018_se_seq_8C[]; /* 0x805AD830 */
extern EmSeEntry em018_se_seq_8F[]; /* 0x805AD860 */
extern EmSeEntry em018_se_seq_34[]; /* 0x805AD8B8 */
extern EmSeEntry em018_se_seq_B2[]; /* 0x805AD908 */
extern EmSeEntry em018_se_seq_E4[]; /* 0x805AD930 */
extern EmSeEntry em018_se_seq_E5[]; /* 0x805AD958 */
extern EmSeEntry em018_se_seq_E6[]; /* 0x805AD980 */
extern EmSeEntry em018_se_seq_E7[]; /* 0x805AD9A8 */
extern EmSeEntry em018_se_seq_E8[]; /* 0x805AD9D0 */
extern EmSeEntry em018_se_seq_B6[]; /* 0x805AD9F8 */
extern EmSeEntry em018_se_seq_B7[]; /* 0x805ADA20 */
extern EmSeEntry em018_se_seq_B8[]; /* 0x805ADA48 */
extern EmSeEntry em018_se_seq_B9[]; /* 0x805ADA70 */
extern EmSeEntry em018_se_seq_BA[]; /* 0x805ADA98 */
extern EmSeEntry em018_se_seq_BB[]; /* 0x805ADAC0 */
extern EmSeEntry em018_se_seq_BF[]; /* 0x805ADB00 */
extern EmSeEntry em018_se_seq_C0[]; /* 0x805ADB60 */
extern EmSeEntry em018_se_seq_C8[]; /* 0x805ADBB8 */
extern EmSeEntry em018_se_seq_D0[]; /* 0x805ADBF8 */
extern EmSeEntry em018_se_alt_seq_00[]; /* 0x805ADF78 */
extern EmSeEntry em018_se_alt_seq_0F[]; /* 0x805ADFA0 */
extern EmSeEntry em018_se_alt_seq_1A[]; /* 0x805ADFE8 */
extern EmSeEntry em018_se_alt_seq_1B[]; /* 0x805AE018 */
extern EmSeEntry em018_se_alt_seq_1C[]; /* 0x805AE040 */
extern EmSeEntry em018_se_alt_seq_0A[]; /* 0x805AE080 */
extern EmSeEntry em018_se_alt_seq_0B[]; /* 0x805AE0D0 */
extern EmSeEntry em018_se_alt_seq_34[]; /* 0x805AE100 */
/* The key path the scripted entrance `fn_801994F4` replays through `em_demo_key3_apply` (`.data`, 0x1E0 bytes;
 * the name is a GUESS). */
extern u8 em018_demo_keys[];
/* The two 0xC-byte pick records `em018_action_select` hands `em_pack_area_pick` for action 0x17 (`.data`; names GUESS). */
extern u8 em018_pick_a[];
extern u8 em018_pick_b[];
/* The `em_turn_seq_start`/`em_turn_seq_step` descriptor blocks this band arms (`.data` tables). */
extern u8 lbl_80570050[];
extern u8 lbl_80570090[];
extern u8 lbl_805700D0[];
extern u8 lbl_80570110[];

extern "C" {

/* This unit's own functions, declared so the dispatchers can tail-call them. */
void fn_801993E0(struct _ENEMY_WORK* self);
void fn_80199468(struct _ENEMY_WORK* self);
void fn_801994F4(struct _ENEMY_WORK* self);
void fn_80199A2C(struct _ENEMY_WORK* self);
void fn_80199ADC(struct _ENEMY_WORK* self);
void fn_8019E398(struct _ENEMY_WORK* self);

void fn_801928B8(void);

void fn_80192C10(struct _ENEMY_WORK* self);
void fn_80192C8C(struct _ENEMY_WORK* self);
void fn_80192D08(struct _ENEMY_WORK* self);
void fn_80192D84(struct _ENEMY_WORK* self);
void fn_80192E04(struct _ENEMY_WORK* self);
void fn_80192E84(struct _ENEMY_WORK* self);
void fn_80192F24(struct _ENEMY_WORK* self);
void fn_80192F78(struct _ENEMY_WORK* self);
void fn_80192FFC(struct _ENEMY_WORK* self);
void fn_80193078(struct _ENEMY_WORK* self);
void fn_801930F4(struct _ENEMY_WORK* self);
void fn_801931A4(struct _ENEMY_WORK* self);
void fn_80193220(struct _ENEMY_WORK* self);
void fn_8019329C(struct _ENEMY_WORK* self);
void fn_80193318(struct _ENEMY_WORK* self);
void fn_80193394(struct _ENEMY_WORK* self);
void fn_801933DC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801934EC(struct _ENEMY_WORK* self, u32 a);
void fn_801935A0(struct _ENEMY_WORK* self);
void fn_8019362C(struct _ENEMY_WORK* self);
void fn_801936A8(struct _ENEMY_WORK* self);
void fn_801938D8(struct _ENEMY_WORK* self);
void fn_80193964(struct _ENEMY_WORK* self, u8 a);
void fn_80193A34(struct _ENEMY_WORK* self);
void fn_80193AEC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80193CC8(struct _ENEMY_WORK* self);
void fn_80193DE8(struct _ENEMY_WORK* self, u32 a);
void fn_80193EF0(struct _ENEMY_WORK* self);
void fn_80193F70(struct _ENEMY_WORK* self);
void fn_80193FF8(struct _ENEMY_WORK* self);
void fn_80194078(struct _ENEMY_WORK* self, u32 a);
void fn_80194318(struct _ENEMY_WORK* self);
void fn_801943A0(struct _ENEMY_WORK* self, u32 a);
void fn_80194584(struct _ENEMY_WORK* self);
void fn_801946E8(struct _ENEMY_WORK* self, u32 a);
void fn_8019485C(struct _ENEMY_WORK* self, u8 a);
void fn_80194938(struct _ENEMY_WORK* self);
void fn_801949B4(struct _ENEMY_WORK* self);
void fn_80194A64(struct _ENEMY_WORK* self);
void fn_80194AE4(struct _ENEMY_WORK* self);
void fn_80194CC8(struct _ENEMY_WORK* self);
void fn_80194D6C(struct _ENEMY_WORK* self);
void fn_80194FA0(struct _ENEMY_WORK* self);
void fn_80195098(struct _ENEMY_WORK* self);
void fn_8019515C(struct _ENEMY_WORK* self);
void fn_80195214(struct _ENEMY_WORK* self);
void fn_8019533C(struct _ENEMY_WORK* self);
void fn_801953BC(struct _ENEMY_WORK* self);
void fn_80195504(struct _ENEMY_WORK* self, u32 a);
void fn_80195638(struct _ENEMY_WORK* self, u32 a);
void fn_80195720(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80195860(struct _ENEMY_WORK* self);
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a);
void fn_80195CA4(struct _ENEMY_WORK* self);
void fn_80195D80(struct _ENEMY_WORK* self);
void fn_80195ED4(struct _ENEMY_WORK* self);
void fn_80196004(struct _ENEMY_WORK* self);
void fn_8019610C(struct _ENEMY_WORK* self, u8 a);
void fn_801961C8(struct _ENEMY_WORK* self, u8 a);
void fn_80196278(struct _ENEMY_WORK* self);
void fn_801963A8(struct _ENEMY_WORK* self);
void fn_80196618(struct _ENEMY_WORK* self);
void fn_801966DC(struct _ENEMY_WORK* self);
void fn_80196768(struct _ENEMY_WORK* self);
void fn_8019687C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80196A50(struct _ENEMY_WORK* self, u8 a);
void fn_80196B1C(struct _ENEMY_WORK* self);
void fn_80196BF4(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80196E10(struct _ENEMY_WORK* self, u32 a);
void fn_80197294(struct _ENEMY_WORK* self);
void fn_80197404(struct _ENEMY_WORK* self);
void fn_80197570(struct _ENEMY_WORK* self, u32 a);
void fn_801976B8(struct _ENEMY_WORK* self);
void fn_80197BC0(struct _ENEMY_WORK* self);
void fn_80197C84(struct _ENEMY_WORK* self);
void fn_80197D50(struct _ENEMY_WORK* self);
void fn_80197F04(struct _ENEMY_WORK* self);
void fn_80198150(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801986AC(struct _ENEMY_WORK* self);
void fn_801987E4(struct _ENEMY_WORK* self);
void fn_80198F10(struct _ENEMY_WORK* self);
void fn_80198F14(struct _ENEMY_WORK* self);
void em018_glow_fade(struct _ENEMY_WORK* self);
u32 fn_8019E49C(struct _ENEMY_WORK* self, u32 flag);
}

/* The user-data accessor `em018_init` installs: the base accessor with its own destructor, attach hook and
 * animation item hook (its table is `.data` 0x805AE720).  size: 0xC (`operator new(0xC)`) */
class Em018UserData : public ResUserDataAc {
public:
    Em018UserData();
    virtual ~Em018UserData();
    virtual void Attach();
    virtual void CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
};


extern "C" {
/* Sets the aim bits the caller's mask names. */
void fn_80192348(EmActWork* self, u8 mask) {
    self->aim_0x328.flags_0x35 |= mask;
}

/* Clears the aim bits the caller's mask names. */
void fn_80192358(EmActWork* self, u8 mask) {
    self->aim_0x328.flags_0x35 &= ~mask;
}

/* Reports whether every aim bit of the caller's mask is set (narrowed in the source: retail masks the
 * incoming register, `clrlwi r0,r4,24`). */
u32 fn_80192370(struct _ENEMY_WORK* self_, u32 mask) {
    EmActWork* self = (EmActWork*)self_;
    return (self->aim_0x328.flags_0x35 & (u8)mask) != 0;
}

/* Reports whether the damage part's level is in the aim group's admitted range (`u8 >= 2`). */
u32 fn_8019238C(EmActWork* self) {
    u8 v = em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1);

    return v >= 2;
}

/* Reports whether the aim record's third flag byte is set. */
u32 fn_801923CC(EmActWork* self) {
    return self->aim_0x328.flag_0x33 != 0;
}

/* Reports whether the aim record's bit 0x02 is set. */
u32 fn_801923E0(EmActWork* self) {
    return fn_80192370((struct _ENEMY_WORK*)self, 2) == 1;
}

/* Reports whether the aim record's bit 0x08 is set. */
u32 fn_80192410(EmActWork* self) {
    return fn_80192370((struct _ENEMY_WORK*)self, 8) == 1;
}

/* Reads the aim vector's z. */
f32 em018_glow_get(EmActWork* self) {
    return self->aim_0x328.vec_0x1C.z;
}

/* Reports whether another live work of team 0x12 already runs one of the four aim actions. */
s32 fn_80192448(EmActWork* self) {
    u16 max = get_move_work_max(3);
    u8 i;
    EmActWork* work = (EmActWork*)get_move_work_adrs(3);

    for (i = 0; (u32)i < max; i++, work++) {
        if (work->active == 0) {
            continue;
        }
        if (self == work) {
            continue;
        }
        if (work->team != 0x12) {
            continue;
        }
        if (self->act_id != work->act_id) {
            continue;
        }
        if ((u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x1F) == 1 ||
            (u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x20) == 1 ||
            (u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x21) == 1 ||
            (u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x22) == 1) {
            return 0;
        }
    }
    return 1;
}

/* Fires the aim effect at the work's 0x1A joint, scaled by the work's own scale. */
void fn_8019255C(EmActWork* self) {
    VEC3 out;
    VEC3 ofs;
    MTX34 mtx;

    VEC3_ctor(&out);
    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    em_hit_window_set((struct _ENEMY_WORK*)self, 0, 0x12, 0x205);
    setVector3(&ofs, lbl_80797E88, lbl_80797E88, lbl_8079800C);
    get_joint_wmat_em((struct _ENEMY_WORK*)self, 0x1A, &mtx);
    mulVecMat(&ofs, &mtx);
    mtx34_trans_add_c1(&mtx, &ofs);
    mtx34_trans_get_c1(&mtx, &out);
    senko_set(&out, lbl_80798238 * get_em_scale((struct _ENEMY_WORK*)self), self->act_id, 0);
}

/* Arms the two aim values and sets the aim record's bit 0x01. */
void fn_80192618(struct _ENEMY_WORK* self_) {
    EmActWork* self = (EmActWork*)self_;
    self->aim_0x328.value_0x28 = lbl_80798244;
    self->aim_0x328.value_0x2C = lbl_80798248;
    fn_80192348(self, 1);
}

/* Initialises the aim record. */
void fn_80192630(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    VEC3 v1;
    VEC3 v2;

    /* The aim record's +0x00 is read as a scalar, but retail copies a whole 3-float record over it. */
    copyVec3((nw4r::math::VEC3*)&rec->angle_0x00,
             setVec3(&v1, lbl_8079824C, lbl_8079824C, lbl_8079824C));
    copyVec3((nw4r::math::VEC3*)&rec->rot_0x10,
             setVec3(&v2, lbl_8079824C, lbl_8079824C, lbl_8079824C));
    rec->vec_0x1C.x = lbl_80798238;
    rec->vec_0x1C.y = lbl_80798250;
    rec->vec_0x1C.z = lbl_80798254;
    rec->value_0x28 = lbl_8079824C;
    rec->value_0x2C = lbl_8079824C;
    rec->flag_0x30 = 0;
    rec->flag_0x31 = 0;
    rec->flag_0x32 = 0;
    rec->flag_0x33 = 0;
    rec->flag_0x34 = 0;
    rec->rot_0x04.z = (u32)-1;
    rec->flags_0x35 = 0;
    rec->value_0x36 = 0;
    rec->value_0x38 = 0;
    rec->value_0x3A = 0;
}

/* 0x801926EC (0x190): the program's init: attaches the 12-byte user-data record when the work has none, arms the
 * two joint effects for a fresh work and, for `mode == 2` on map kind 3, places the work at the area's start joint. */
void em018_init(struct _ENEMY_WORK* self, u8 mode) {
    VEC3 vec;

    VEC3_ctor(&vec);
    if (em_res_user_data_ck(self) == 0) {
        Em018UserData* data = (Em018UserData*)operator new(sizeof(Em018UserData));
        new (data) Em018UserData;
        em_res_user_data_set(self, data);
    }
    if (self->field_0x009 == 0) {
        setVector3(&vec, 0.0f, -115.0f, 125.0f);
        eft013_spawn_id(self, 0x20, 0x10, &vec, 1.0f, 0x18E4);
        setVector3(&vec, 0.0f, 10.0f, 0.0f);
        eft013_spawn(self, 0xC, 0x1A, &vec);
    }
    if (mode == 2) {
        if (stage_map_kind_get(self->field_0x1E0) == 3) {
            switch (self->area_no) {
            case 3:
                em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 6), &self->pos);
                em_move_mode_set(self, 4);
                em_act_arm_unless_down(self, 6, 5);
                break;
            case 8:
                em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 9), &self->pos);
                em_move_mode_set(self, 4);
                em_act_arm_unless_down(self, 6, 5);
                break;
            }
        }
        em_state_refresh(self);
    }
}

} /* extern "C" */

/* 0x8019287C (0x3C): Constructs the accessor: the base first, then this class's table. */
Em018UserData::Em018UserData() {
}

extern "C" {

/* 0x801928BC (0x298): the program's event hook: releases the roster binding, then reacts to the (kind, sub)
 * event: the hit reactions, the 900/2700-frame cooldowns, the arena key, the gauge drain and the bound-player
 * pick. */
void em018_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub) {
    EmFadeRec* rec = &((EmActWork*)self)->fade_0x328;

    if (rec->roster_0x0C != -1) {
        em_roster_record_release(rec->roster_0x0C);
        rec->roster_0x0C = -1;
    }
    switch (kind) {
    case 1:
        switch (sub) {
        case 5:
            em_bait_react(self);
            break;
        case 7:
            em_act_flag1_set(self);
            break;
        }
        break;
    case 5:
        switch (sub) {
        case 14:
            self->field_0x362 = 900;
            break;
        case 31:
            if (em_busy_ck(self) == 1) {
                camera_mode12_set_c1(self);
            }
            break;
        case 32:
            quest_arena_key_set(0x12, 2);
            self->field_0x360 = 2700;
            break;
        case 33:
            em_gauge_step(self, -100);
            if ((s32)self->field_0x7A0 <= 0) {
                em_gauge_set(self, 1);
            }
            self->field_0x362 = 900;
            break;
        case 36:
            if (em_busy_ck(self) == 1) {
                u16 max = get_move_work_max(2);
                _PLW* plw = (_PLW*)get_move_work_adrs(2);
                u16 i;
                for (i = 0; i < max; i++, plw++) {
                    if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && em_pl_target_ck(self, plw) != 0 &&
                        pl_act_grabbed_ck(plw) != 0 && fn_80192448((EmActWork*)self) != 0) {
                        if (self->owner_slot_0x1F6 != i) {
                            em_owner_slot_set(self, i);
                        }
                        break;
                    }
                }
            }
            break;
        case 37:
            em_act_flag1_set(self);
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 200:
            if (em_water_check_c1(self) == 1) {
                em_part_hit_set(self, 0, 3);
            } else {
                em_part_hit_set(self, 0, 0);
            }
            break;
        case 208:
            em_part_hit_set(self, 0, 0);
            break;
        case 178:
            self->timer_0x020 = 0x3C;
            break;
        }
        break;
    }
}

/* 0x80192B54 (0xBC): the program's per-frame bookkeeping: raises aim bit 0x10 while the live work is on dry land in
 * the alternate state, mirrors the run flag into bit 0x8000 of +0x836, ticks the two cooldowns and clears the
 * per-frame action-start pair. */
void em018_frame_update(struct _ENEMY_WORK* self) {
    if (em_die_ck(self) == 0 && em_eft_active_ck(self) == 1 && em_water_check_c1(self) == 0) {
        fn_80192348((EmActWork*)self, 0x10);
    }
    if (self->field_0x358 == 0) {
        self->flags_0x836 |= 0x8000;
    } else {
        self->flags_0x836 &= 0x7FFF;
    }
    if (self->field_0x360 > 0) {
        self->field_0x360--;
    }
    if (self->field_0x362 > 0) {
        self->field_0x362--;
    }
    self->field_0x359 = 0;
    self->field_0x35B = 0;
}

}


extern "C" {
/* ---------------------------------------------------------------------------------------------- */
/* the per-motion step group dispatched by fn_80192F24 (states 0,1,2,4,5,7)                         */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80192C10 - arms motion 1 for 4 frames; state 1 waits and re-enters the motion. */
void fn_80192C10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192C8C - the same shape with motion 20. */
void fn_80192C8C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192D08 - the same shape with motion 29. */
void fn_80192D08(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 29, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192D84 - arms motion mode 2 with the four-argument setter (40/20/0/3) and the
 * `em_action_finish_swim` completion. */
void fn_80192D84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 40, 20, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80192E04 - the same shape with (54/20/0/3). */
void fn_80192E04(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend_ck(self, 54, 20, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80192E84 - the flag-arming step: sets the two action-start flags, arms mode 4 with motion 0xCE
 * and the fresh-scale pair, then waits on `em_action_finish_stand`. */
void fn_80192E84(struct _ENEMY_WORK* self) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0xCE, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_flag_0x8AD_set(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80192F24 - the action-0 dispatcher: `state_sub` selects this group's six steps. */
void fn_80192F24(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80192C10(self);
        break;
    case 1:
        fn_80192C8C(self);
        break;
    case 2:
        fn_80192D08(self);
        break;
    case 4:
        fn_80192D84(self);
        break;
    case 5:
        fn_80192E04(self);
        break;
    case 7:
        fn_80192E84(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the step group dispatched by fn_80193394 (states 0..7)                                           */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80192F78 - arms motion 6 for 10 frames; state 1 runs `em_state_set(self, 1, 5)`. */
void fn_80192F78(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 5);
        }
        break;
    }
}

/* 0x80192FFC - arms motion 7 for 4 frames. */
void fn_80192FFC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193078 - arms motion 0x1A for 4 frames. */
void fn_80193078(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801930F4 - arms motion 0x1E for 4 frames; state 1 fires the joint effect when `em_frame_check`
 * reports the frame and then waits on `em_mot_end_ck`. */
void fn_801930F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1E, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            em_camera_req(self, -1, 7);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801931A4 - arms motion 0x72 for 10 frames. */
void fn_801931A4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x72, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193220 - arms motion 8 for 4 frames. */
void fn_80193220(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019329C - arms motion 20 for 4 frames. */
void fn_8019329C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193318 - arms motion 0x1D for 4 frames. */
void fn_80193318(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193394 - the action-1 dispatcher (states 0..7). */
void fn_80193394(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80192F78(self);
        break;
    case 1:
        fn_80192FFC(self);
        break;
    case 2:
        fn_80193078(self);
        break;
    case 3:
        fn_801930F4(self);
        break;
    case 4:
        fn_801931A4(self);
        break;
    case 5:
        fn_80193220(self);
        break;
    case 6:
        fn_8019329C(self);
        break;
    case 7:
        fn_80193318(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the step group dispatched by fn_801938D8 (states 0..10)                                          */
/* ---------------------------------------------------------------------------------------------- */

/* 0x801933DC - arms motion `a == 1 ? 9 : 2` and picks the height by `b`; `default:` first (playbook 37),
 * selectors narrowed at use and the motion to `u16` (retail's `clrlwi` pair). */
void fn_801933DC(struct _ENEMY_WORK* self, u32 a, u32 b) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        {
            u16 motion = (u8)a == 1 ? 9 : 2;
            em_mot_set(self, motion, 4, 0);
        }
        switch ((u8)b) {
        default:
            em_approach_start(self, lbl_80798268, 0);
            break;
        case 1:
            em_approach_start(self, lbl_80798238, 0);
            if (self->value_0x378 > lbl_8079826C) {
                self->value_0x378 = lbl_8079826C;
            }
            break;
        case 2:
            em_approach_start(self, lbl_80798270, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801934EC - arms motion 21 and picks the height by `a`. */
void fn_801934EC(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 21, 4, 0);
        {
            f32 height = (u8)a == 1 ? lbl_80798270 : lbl_80798268;
            em_approach_start(self, height, 0);
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801935A0 - arms the `lbl_80570050` descriptor and waits on `em_turn_seq_step`. */
void fn_801935A0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570050, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570050) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019362C - arms motion 0x1B for 4 frames. */
void fn_8019362C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801936A8 (0x230): arms motion 0xA with the backward approach, sprays the wake every fourth frame of the
 * window, then switches to motion 0xB and fires the three splash effects. */
void fn_801936A8(struct _ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xA, 8, 0, 1);
        em_approach_start(self, -700.0f, 0);
        break;
    case 1:
        if (em_frame_check(self, 3, 32.0f, 104.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&vec, 0.0f, 0.0f, 120.0f);
            eft_em_spawn(self, 0x3C, 0xF, &vec, 1.0f);
        }
        if (em_approach_step(self, 0, 0x40) == 1 || em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_blend(self, 0xB, 0xA, 0, 1);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 0x41, 3, &vec, 1.0f);
        }
        if (em_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            setVector3(&vec, -200.0f, -50.0f, 140.0f);
            eft_em_spawn(self, 0x40, 3, &vec, 1.0f);
        }
        if (em_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            setVector3(&vec, 200.0f, -50.0f, 140.0f);
            eft_em_spawn(self, 0x40, 3, &vec, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193AEC (0x1DC): the swim-in step: `motion` picks 0x37 or 0x30, `height` the approach target and the
 * +0x378 cap; state 1 waits on the approach and the turn, then motion 0x2F completes with `em_action_finish_swim`. */
void fn_80193AEC(struct _ENEMY_WORK* self, u32 height, u32 motion) {
    if ((u8)((u8)motion - 2) <= 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        switch ((u8)motion) {
        case 0:
        case 2:
            em_mot_set_blend(self, 0x37, 0x14, 0, 1);
            break;
        case 1:
        case 3:
            em_mot_set_blend(self, 0x30, 0x14, 0, 1);
            break;
        }
        switch ((u8)height) {
        default:
            em_approach_start(self, -360.0f, 0x12);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 700.0f;
            break;
        case 2:
            em_approach_start(self, 0.0f, 0x12);
            self->value_0x378 = 2000.0f;
            break;
        case 3:
            em_approach_start(self, 0.0f, 0x12);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1 && em_mot_frame_started_ck(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80193CC8 (0x120): arms motion 0x39; on the frame window it rises by ten times the climb past 30
 * (capped at 100) and turns to the target, completing with `em_action_finish_swim`. */
void fn_80193CC8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x39, 0xA, 0, 1);
        break;
    case 1:
        em_height_follow(self, 10.0f);
        if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
            f32 rise = 10.0f * (em_mot_frame_get(self) - 30.0f);
            if (rise > 100.0f) {
                rise = 100.0f;
            }
            em_surface_clamp(self, rise + em_float_depth_get(self));
        }
        if (em_frame_check(self, 1, 70.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80193DE8 (0x108): arms motion 0x29 (`a == 1`) or 0x35 and fires the wake effect on the frame for the
 * latter, completing with `em_action_finish_swim`. */
void fn_80193DE8(struct _ENEMY_WORK* self, u32 a) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        {
            u32 motion, frames;
            if ((u8)a == 1) {
                motion = 0x29;
                frames = 0xA;
            } else {
                motion = 0x35;
                frames = 0x14;
            }
            em_mot_set_blend(self, motion, frames, 0, 1);
        }
        break;
    case 1:
        if ((u8)a == 0 && em_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -70.0f, 140.0f);
            eft_em_spawn(self, 0x1D, 0xF, &vec, 0.8f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80194078 (0x2A0): the leap out of the water: motion 0x3C, the launch vector on the frame, the arc until it
 * clears the surface (splash effects, motion 0x40), then the fall back down to `em_state_set(self, 5, 0xE)`. */
void fn_80194078(struct _ENEMY_WORK* self, u32 a) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3C, 0x14, 0, 1);
        if ((u8)a == 1) {
            em_user_data_reenter(self);
        }
        break;
    case 1:
        if (em_frame_check(self, 1, 150.0f, 0.0f) == 1) {
            self->state++;
            em_move_vec2_clr(self);
            self->field_0x314 = 30.0f * get_em_base_scale(self);
            self->field_0x318 = 40.0f * get_em_base_scale(self);
        }
        break;
    case 2:
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (self->pos.y >= self->field_0x210) {
            self->state++;
            self->state_0x006 = 0;
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x40, 4, 0);
            eft009_spawn_at_joint(self, 0xE, 0x2C, 0, 1.0f);
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x30, 0xE, &vec, 0.7f);
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x32, 3, &vec, 1.0f);
            self->field_0x320 = -4.0f;
        } else {
            em_surface_clamp(self, 0.0f);
        }
        break;
    case 3: {
        u32 landed = em_move_offset_step_update_c1(self, &self->field_0x1BC, 200.0f);
        if (self->field_0x314 < -40.0f) {
            self->field_0x314 = -40.0f;
        }
        if (landed == 1 && self->field_0x314 < 0.0f) {
            em_move_mode_set(self, 0);
            em_state_set(self, 5, 0xE);
        } else if (self->field_0x314 < 0.0f) {
            f32 scale = get_em_chg_scale(self);
            if (self->field_0x210 > self->pos.y + em_float_depth_get(self) * scale) {
                em_action_finish_swim(self);
            }
        }
        break;
    }
    }
}

/* 0x801943A0 (0x1E4): the dive: motion 0x3E, the fall with the 268 lift and the step-offset arc, then either the
 * surface catch (`em_state_set(self, 5, 0x10)`) or the landing motion 0x3B. */
void fn_801943A0(struct _ENEMY_WORK* self, u32 a) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0x3E, 0xA, 0, 1);
        if ((u8)a == 1) {
            em_user_data_reenter(self);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_start_c1(self, 268.0f);
            em_mot_set(self, 0x41, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 35.0f * get_em_base_scale(self);
            self->field_0x318 = 40.0f * get_em_base_scale(self);
            self->field_0x320 = -5.5f;
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    case 2: {
        u32 landed = em_move_offset_step_update_c1(self, &self->field_0x1BC, 360.0f);
        if (em_water_check_c1(self) == 1) {
            if (self->pos.y <= self->field_0x210) {
                self->pos.y = self->field_0x210;
                em_state_set(self, 5, 0x10);
            }
        } else if (landed == 1 && self->field_0x314 < 0.0f) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3B, 0, 0);
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

/* 0x80194584 (0x164): the surfacing splash: lifts by 400 onto the water line with the three splash effects,
 * then motion 0x42 with the 350 rise, completing with `em_action_finish_swim`. */
void fn_80194584(struct _ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, 400.0f);
        em_mot_set_blend(self, 0x3F, 4, 0, 1);
        self->pos.y = self->field_0x210;
        eft009_spawn_at_joint(self, 3, 0x2C, 0, 1.0f);
        setVector3(&vec, 0.0f, 0.0f, 0.0f);
        eft_em_spawn(self, 0x30, 3, &vec, 0.7f);
        setVector3(&vec, 0.0f, 0.0f, 0.0f);
        eft_em_spawn(self, 0x32, 3, &vec, 0.7f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0x42, 0, 0);
            em_surface_clamp(self, 350.0f);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x801946E8 (0x174): the slide: motion 0xC8 at double speed with the -15 push, the offset step until the
 * ground catches it (motion 0xC9) or the 180-frame timer runs out. */
void fn_801946E8(struct _ENEMY_WORK* self, u32 a) {
    if ((u8)a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0xC8, 0xA, 0);
        em_mot_speed_set(self, 2.0f);
        em_move_vec_clr(self);
        self->field_0x314 = -15.0f;
        self->timer_0x020 = 0xB4;
        break;
    case 1:
        if (em_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 1.0f);
        }
        if (em_frame_check(self, 1, 34.0f, 0.0f) == 1) {
            em_move_offset_apply(self);
            em_dive_depth_get(self);
            if (em_ground_ck(self) == 1) {
                self->state++;
                em_move_mode_set(self, 0);
                em_mot_set(self, 0xC9, 0, 0);
            } else {
                self->timer_0x020--;
                if (self->timer_0x020 <= 0) {
                    em_action_finish_swim(self);
                }
            }
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801938D8 - the action-2 dispatcher (states 0..10). */
void fn_801938D8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801933DC(self, 0, 0);
        break;
    case 1:
        fn_801934EC(self, 0);
        break;
    case 2:
        fn_801935A0(self);
        break;
    case 3:
        fn_8019362C(self);
        break;
    case 4:
        fn_801933DC(self, 1, 0);
        break;
    case 5:
        fn_801936A8(self);
        break;
    case 6:
        fn_801933DC(self, 0, 1);
        break;
    case 7:
        fn_801933DC(self, 1, 1);
        break;
    case 8:
        fn_801934EC(self, 1);
        break;
    case 9:
        fn_801933DC(self, 0, 2);
        break;
    case 10:
        fn_801933DC(self, 1, 2);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the large per-action dispatchers                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80194AE4 (0x1E4): the bound-player grab: motion 0x28 with the 60-frame window; while the bound player is in
 * reach it is pulled into the hold, and its hold action completes the step with `em_state_set(self, 5, 0x1F)`. */
void fn_80194AE4(struct _ENEMY_WORK* self) {
    _PLW* plw;

    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x28, 0x14, 0, 3);
        self->timer_0x020 = 0x3C;
        if (em_busy_ck(self) == 1) {
            plw = &((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6];
            if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && em_pl_target_ck(self, plw) == 1 &&
                pl_act_grabbed_ck(plw) == 1 && fn_80192448((EmActWork*)self) == 1) {
                plw->field_0x59A = self->timer_0x020;
                pl_act_hold_motion_set(plw, (u8*)self);
                pl_act_hold_notify(plw, 1);
            }
        }
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        }
        if (em_busy_ck(self) == 1) {
            plw = &((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6];
            if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && em_pl_target_ck(self, plw) == 1 &&
                fn_80192448((EmActWork*)self) == 1 && Pl_act_ck(plw, 0, 0x9A) != 0) {
                em_state_set(self, 5, 0x1F);
            } else if (self->timer_0x020 <= 0) {
                em_action_finish_swim(self);
            }
        }
        break;
    }
}

/* 0x80194D6C (0x234): the knock-back arc: motion 0x7C with the (110, 70, -4) offset step, the splash when it
 * crosses the water line, then either the landing (`em_state_set(self, 5, 0x21)`) or the surface catch
 * (motion 0xE1 with the 300 rise). */
void fn_80194D6C(struct _ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, 0.0f);
        em_mot_set(self, 0x7C, 4, 0);
        em_move_vec2_clr(self);
        self->field_0x314 = 110.0f;
        self->field_0x318 = 70.0f;
        self->field_0x320 = -4.0f;
        self->state_0x006 = 0;
        break;
    case 1: {
        u32 landed = em_move_offset_step_update_c1(self, &self->field_0x1BC, em_fall_height_get_c1(self));
        if (self->field_0x314 < -70.0f) {
            self->field_0x314 = -70.0f;
        }
        if (self->pos.y >= self->field_0x210 && self->state_0x006 == 0) {
            self->state_0x006 = 1;
            eft009_spawn_at_joint(self, 0xE, 0x2C, 0, 0.8f);
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x30, 0xE, &vec, 0.7f);
            setVector3(&vec, 0.0f, 0.0f, 15.0f);
            eft_em_spawn(self, 0x31, 3, &vec, 1.0f);
        }
        if (self->field_0x314 < 0.0f) {
            if (landed == 1) {
                em_move_mode_set(self, 0);
                em_state_set(self, 5, 0x21);
            } else if (em_water_check_c1(self) == 1 && self->pos.y <= self->field_0x210) {
                self->pos.y = self->field_0x210;
                self->state++;
                em_move_mode_set(self, 2);
                em_mot_set(self, 0xE1, 0, 0);
                em_surface_clamp(self, 300.0f);
            }
        }
        break;
    }
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80194FA0 (0xF8): the stranded step: motion 0x7D with the 100-unit roster binding, then motion 0x7E for
 * 240 frames before `em_state_set(self, 5, 0x22)`. */
void fn_80194FA0(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7D, 0, 0);
        em_drop_item_spawn(self, &self->pos, 0x64);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x7E, 0, 0);
            self->timer_0x020 = 0xF0;
        }
        break;
    case 2:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0 && em_busy_ck(self) == 1) {
            em_state_set(self, 5, 0x22);
        }
        break;
    }
}

/* 0x80195214 (0x128): motion 0x28 with the 60-frame window: a bound player in reach completes the step with
 * `em_state_set(self, 5, 0x1E)`, the timer running out with `em_action_finish_swim`. */
void fn_80195214(struct _ENEMY_WORK* self) {
    _PLW* plw;

    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x28, 0x14, 0, 3);
        self->timer_0x020 = 0x3C;
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        }
        if (em_busy_ck(self) == 1) {
            plw = &((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6];
            if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && em_pl_target_ck(self, plw) == 1 &&
                pl_act_grabbed_ck(plw) == 1) {
                em_state_set(self, 5, 0x1E);
            } else if (self->timer_0x020 <= 0) {
                em_action_finish_swim(self);
            }
        }
        break;
    }
}

/* 0x80195504 (0x134): motion 0xCB, then the flag-armed motion 0xD9 that raises aim bit 0x04 and the dim flag
 * on its frames, completing with `em_action_finish_stand`.  The dispatcher's argument is unused. */
void fn_80195504(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCB, 0, 0);
        break;
    case 1:
        em_busy_set(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 0xD9, 0, 0);
            em_height_set(self, em_height_base_get(self));
        }
        break;
    case 2:
        em_busy_set(self);
        self->field_0x35B = 1;
        if (em_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            fn_80192348((EmActWork*)self, 4);
        }
        if (em_frame_check(self, 1, 10.0f, 0.0f) == 1) {
            self->field_0x359 = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80195720 (0x140): the flag-armed approach: motion 0xD1 at the `speed` pick (0.8 for 1 and 2, 1.2 for 0),
 * the `dist` pick of the approach target, completing with `em_action_finish_stand`. */
void fn_80195720(struct _ENEMY_WORK* self, u32 speed, u32 dist) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if ((u8)speed == 2) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xD1, 0, 0);
        if ((u32)((u8)speed - 1) > 1) {
            if ((u8)speed == 0) {
                em_mot_speed_set(self, 1.2f);
            }
        } else {
            em_mot_speed_set(self, 0.8f);
        }
        {
            f32 target;
            switch ((u8)dist) {
            default:
                target = -200.0f;
                break;
            case 1:
                target = -700.0f;
                break;
            case 2:
                target = 0.0f;
                break;
            }
            em_approach_start(self, target, 0);
        }
        em_height_set(self, em_height_base_get(self));
        em_flag_0x8AD_set(self);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80195860 (0x36C): the bound hold: motion 0xCC, then 0xCF with the roster binding; every 16 frames a part
 * whose frame cost has dropped low ends the hold, otherwise the three timers pick the release action. */
void fn_80195860(struct _ENEMY_WORK* self) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCC, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_flag_0x8AD_set(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            EmFadeRec* rec = &((EmActWork*)self)->fade_0x328;
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 0xCF, 0, 0);
            self->timer_0x020 = 0;
            if (rec->roster_0x0C != -1) {
                em_roster_record_release(rec->roster_0x0C);
                rec->roster_0x0C = -1;
            }
            em_roster_slot_effect_set(self, 2, &rec->roster_0x0C, 4, 0);
        }
        break;
    case 2:
        self->timer_0x020++;
        if (em_busy_ck(self) == 1) {
            s32 done = 0;
            if ((self->timer_0x020 & 0xF) == 0) {
                u8 i;
                for (i = 0; i < 4; i++) {
                    f32 cost = self->frames_0x454[i];
                    if (cost > 0.0f) {
                        if (cost <= 300.0f) {
                            self->state++;
                            self->timer_0x020 = 0x5A;
                            done = 1;
                            break;
                        }
                        if (self->state_0x006 == 0 && cost <= 600.0f) {
                            self->state_0x006 = 1;
                            self->timer_0x020 = 0;
                            break;
                        }
                    }
                }
            }
            if (done == 0) {
                switch (self->state_0x006) {
                case 0:
                    if (self->timer_0x020 >= 0x708) {
                        em_state_set(self, 6, 4);
                    } else if (self->field_0x833 != 0) {
                        self->state_0x006 = 2;
                        self->timer_0x020 = 0;
                    } else if (self->field_0x43D != 1) {
                        u16 max = get_move_work_max(2);
                        _PLW* plw = (_PLW*)get_move_work_adrs(2);
                        u16 j;
                        for (j = 0; j < max; j++, plw++) {
                            if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 &&
                                em_pl_target_ck(self, plw) == 1 && pl_act_grabbed_ck(plw) == 1) {
                                em_state_set(self, 6, 4);
                                return;
                            }
                        }
                    }
                    break;
                case 1:
                    if (self->timer_0x020 >= 0x12C) {
                        em_state_set(self, 6, 0x17);
                    }
                    break;
                case 2:
                    if (self->timer_0x020 >= 0x3C) {
                        em_state_set(self, 6, 0x18);
                    }
                    break;
                }
            }
        }
        break;
    case 3:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        }
        if (em_busy_ck(self) == 1 && self->timer_0x020 <= 0) {
            em_state_set(self, 6, 0xB);
        }
        break;
    }
}

/* 0x80195CA4 (0xDC): the flag-armed rise: motion 0xCE with the -250 drop, then climbs 5 per frame until the
 * +0x1AC height reaches the ground, completing with `em_state_set(self, 6, 1)`. */
void fn_80195CA4(struct _ENEMY_WORK* self) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_height_set(self, -250.0f);
        break;
    case 1:
        em_height_add(self, 5.0f);
        if (self->field_0x1AC >= em_height_base_get(self)) {
            em_height_set(self, em_height_base_get(self));
            em_state_set(self, 6, 1);
        }
        break;
    }
}

/* 0x80195D80 (0x154): the flag-armed hold: motion 0xCC, then 0xCF for 1800 frames with the tint pulse, then
 * motion 0xCD, completing with `em_action_finish_stand`. */
void fn_80195D80(struct _ENEMY_WORK* self) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCC, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_flag_0x8AD_set(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xCF, 0, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick(self, 0.4f, 1, 0xF);
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0xCD, 0, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801953BC - the action-5 dispatcher (states 0..39). */
void fn_801953BC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80193964(self, 0);
        break;
    case 1:
        fn_80193A34(self);
        break;
    case 2:
        fn_80193AEC(self, 0, 0);
        break;
    case 3:
        fn_80193AEC(self, 1, 0);
        break;
    case 4:
        fn_80193AEC(self, 2, 0);
        break;
    case 5:
        fn_80193AEC(self, 0, 1);
        break;
    case 6:
        fn_80193AEC(self, 1, 1);
        break;
    case 7:
        fn_80193AEC(self, 2, 1);
        break;
    case 8:
        fn_80193CC8(self);
        break;
    case 9:
        fn_80193DE8(self, 0);
        break;
    case 10:
        fn_80193EF0(self);
        break;
    case 11:
        fn_80193F70(self);
        break;
    case 12:
        fn_80193FF8(self);
        break;
    case 13:
        fn_80194078(self, 0);
        break;
    case 14:
        fn_80194318(self);
        break;
    case 15:
        fn_801943A0(self, 0);
        break;
    case 16:
        fn_80194584(self);
        break;
    case 17:
        fn_801946E8(self, 0);
        break;
    case 18:
        fn_8019485C(self, 0);
        break;
    case 19:
        fn_80194938(self);
        break;
    case 20:
        fn_801949B4(self);
        break;
    case 21:
        fn_80193DE8(self, 1);
        break;
    case 22:
        fn_80193AEC(self, 3, 0);
        break;
    case 23:
        fn_80193AEC(self, 3, 1);
        break;
    case 24:
        fn_80193964(self, 1);
        break;
    case 25:
        fn_80193AEC(self, 3, 2);
        break;
    case 26:
        fn_80193AEC(self, 3, 3);
        break;
    case 27:
        fn_801946E8(self, 1);
        break;
    case 28:
        fn_8019485C(self, 1);
        break;
    case 29:
        fn_80194A64(self);
        break;
    case 30:
        fn_80194AE4(self);
        break;
    case 31:
        fn_80194CC8(self);
        break;
    case 32:
        fn_80194D6C(self);
        break;
    case 33:
        fn_80194FA0(self);
        break;
    case 34:
        fn_80195098(self);
        break;
    case 35:
        fn_8019515C(self);
        break;
    case 36:
        fn_80195214(self);
        break;
    case 37:
        fn_8019533C(self);
        break;
    case 38:
        fn_80194078(self, 1);
        break;
    case 39:
        fn_801943A0(self, 1);
        break;
    }
}

/* 0x80196278 (0x130): motion 0xCB, then the flag-armed motion 0xD9 sinking 2 per frame, raising aim bit 0x04
 * and the dim flag on its frames and completing with `em_action_finish_stand`. */
void fn_80196278(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCB, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 0xD9, 0, 0);
            em_height_set(self, em_height_base_get(self));
        }
        break;
    case 2:
        self->field_0x35B = 1;
        em_height_add(self, -2.0f);
        if (em_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            fn_80192348((EmActWork*)self, 4);
        }
        if (em_frame_check(self, 1, 10.0f, 0.0f) == 1) {
            self->field_0x359 = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x801963A8 (0x270): the flag-armed motion 0xE2: on its frames it fires the jaw effect ahead of joint 0x1A and the
 * aim effect, then picks the follow-up by whether any part's frame cost has dropped to 800. */
void fn_801963A8(struct _ENEMY_WORK* self) {
    VEC3 pos;
    VEC3 ofs;

    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    self->field_0x35B = 1;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xE2, 0, 0);
        em_flag_0x8AD_set(self);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_frame_check(self, 0, 150.0f, 0.0f) == 1 && fn_8019238C((EmActWork*)self) == 0) {
            get_joint_wpos_em(self, 0x1A, &pos);
            setVector3(&ofs, 0.0f, 20.0f * get_em_scale(self), -50.0f * get_em_scale(self));
            rotVecY(&ofs, self->field_0x1C0);
            addVec3To(&pos, &ofs);
            eft022_spawn(self, &pos, self->area_no);
        }
        if (em_frame_check(self, 0, 210.0f, 0.0f) == 1 && fn_8019238C((EmActWork*)self) == 0) {
            fn_8019255C((EmActWork*)self);
        }
        if (em_frame_check(self, 1, 200.0f, 0.0f) == 1) {
            self->field_0x359 = 1;
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            u32 low = 0;
            s32 i;
            for (i = 0; i < 4; i++) {
                if (self->frames_0x454[i] > 0.0f && self->frames_0x454[i] <= 800.0f) {
                    low = 1;
                    break;
                }
            }
            if (low == 1) {
                em_state_set(self, 6, 0xA);
            } else {
                em_state_set(self, 6, 0x18);
            }
        }
        break;
    }
}

/* 0x8019687C (0x1D4): the tail sweep: motion 0x56 (`side == 1`) or 0x55 with its hit window; with `aim == 1` it
 * stores the clamped turn toward the target (at most 60 degrees) and turns through it while the motion runs. */
void fn_8019687C(struct _ENEMY_WORK* self, u32 side, u32 aim) {
    VEC3 dir;
    VEC3 diff;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0: {
        u32 motion;
        u32 window;
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        if ((u8)side == 1) {
            motion = 0x56;
            window = 5;
        } else {
            motion = 0x55;
            window = 4;
        }
        em_mot_set(self, motion, 4, 0);
        em_hit_window_set_default(self, 0, window);
        em_stamina_add(self, -0xC);
        if ((u8)aim == 1) {
            u32 turn;
            subVec3(&diff, &self->vec_0x36C, &self->pos);
            copyVec3(&dir, &diff);
            calcVecAngXY(&dir, &pitch, &yaw);
            turn = (u16)(yaw - (u16)(self->field_0x1C0 + ((u8)side == 1 ? 0x4000 : 0xC000)));
            if (turn - 1 <= 0x7FFE) {
                if (turn > 0x2AAB) {
                    turn = 0x2AAB;
                }
            } else {
                if (turn < 0xD555) {
                    turn = 0xD555;
                }
                turn = 0x10000 - turn;
                self->state_0x007 = 1;
            }
            self->state_0x006 = (s32)turn >> 8;
        }
        break;
    }
    case 1:
        if ((u8)aim == 1) {
            s32 angle = self->state_0x006 << 8;
            if (self->state_0x007 == 1) {
                angle = -angle;
            }
            em_turn_in_window(self, 80.0f, 130.0f, angle);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196BF4 (0x21C): the bite: motion 0x4F with the two hit windows; on its frames it turns toward the target
 * (clamped to +-0x900), steps forward when `lunge == 1` and turns once more when `track == 1`. */
void fn_80196BF4(struct _ENEMY_WORK* self, u32 lunge, u32 track) {
    VEC3 dir;
    VEC3 diff;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4F, 4, 0);
        em_hit_window_set(self, 0, 0xA, 8);
        em_hit_window_set(self, 1, 0x18, 0x10);
        em_stamina_add(self, -0xC);
        if ((u8)lunge == 1) {
            em_move_vec2_clr(self);
            self->field_0x318 = 40.0f;
        }
        break;
    case 1:
        if (em_frame_check(self, 3, 30.0f, 60.0f) == 1) {
            u16 rot;
            subVec3(&diff, &self->vec_0x36C, &self->pos);
            copyVec3(&dir, &diff);
            calcVecAngXY(&dir, &pitch, &yaw);
            rot = em_angle_approach((u16)pitch, (u16)self->field_0x1BC, 0x100);
            self->field_0x1BC = rot;
            if ((u32)(rot - 0x901) <= 0x76FE) {
                self->field_0x1BC = 0x900;
            } else if ((u32)(rot - 0x8001) <= 0x76FE) {
                self->field_0x1BC = 0xF700;
            }
        }
        if (em_frame_check(self, 3, 30.0f, 154.0f) == 1) {
            em_hit_arm_set(self);
        }
        if ((u8)track == 1 && em_frame_check(self, 3, 30.0f, 120.0f) == 1) {
            em_turn_to_target(self, 0xC0);
        }
        if ((u8)lunge == 1 && em_frame_check(self, 3, 90.0f, 140.0f) == 1) {
            em_move_offset_step(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80196618 - the action-6 dispatcher (states 0..24). */
void fn_80196618(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80195504(self, 0);
        break;
    case 1:
        fn_80195638(self, 0);
        break;
    case 2:
        fn_80195720(self, 0, 2);
        break;
    case 3:
        fn_80195860(self);
        break;
    case 4:
        fn_80195BCC(self, 0);
        break;
    case 5:
        fn_80195CA4(self);
        break;
    case 7:
        fn_80195D80(self);
        break;
    case 10:
        fn_80195ED4(self);
        break;
    case 11:
        fn_80196004(self);
        break;
    case 12:
        fn_8019610C(self, 0);
        break;
    case 13:
        fn_801961C8(self, 0);
        break;
    case 14:
        fn_80196278(self);
        break;
    case 15:
        fn_80195720(self, 1, 0);
        break;
    case 16:
        fn_80195720(self, 0, 1);
        break;
    case 18:
        fn_80195504(self, 1);
        break;
    case 19:
        fn_80195638(self, 1);
        break;
    case 20:
        fn_80195720(self, 2, 0);
        break;
    case 21:
        fn_8019610C(self, 1);
        break;
    case 22:
        fn_801961C8(self, 1);
        break;
    case 23:
        fn_801963A8(self);
        break;
    case 24:
        fn_80195BCC(self, 1);
        break;
    }
}

/* 0x80196E10 (0x484): the charge: `kind == 2` arms motion 0x50 then 0x51, the others the blended 0x46; it runs
 * the approach with the turn clamped to +-0x1800 and the bite windows, then recovers with 0x48/0x49 (kind 1
 * re-arms the windows once more on the way back). */
void fn_80196E10(struct _ENEMY_WORK* self, u32 kind) {
    switch (self->state) {
    case 0:
        self->state++;
        if ((u8)kind == 2) {
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x50, 4, 0);
            self->state_0x006 = 0;
        } else {
            em_move_mode_set(self, 2);
            em_mot_set_blend(self, 0x46, 0xA, 0, 1);
            self->state_0x006 = 1;
        }
        em_approach_start(self, 500.0f, 0x16);
        em_stamina_add(self, -0xC);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (em_mot_end_ck(self) == 1) {
                self->state_0x006++;
                em_move_mode_set(self, 2);
                em_mot_set(self, 0x51, 0, 0);
            }
            break;
        case 1: {
            f32 frame;
            em_approach_step(self, 0, 0x100);
            if ((u32)((u16)self->field_0x1BC - 0x1801) <= 0x67FE) {
                self->field_0x1BC = 0x1800;
            } else if ((u32)((u16)self->field_0x1BC - 0x8001) <= 0x67FE) {
                self->field_0x1BC = 0xE800;
            }
            if ((u8)kind == 2) {
                frame = 44.0f;
            } else {
                frame = 64.0f;
                em_turn_to_target(self, 0x40);
            }
            if (em_frame_check(self, 0, frame, 0.0f) == 1) {
                em_hit_window_set(self, 0, 0xB, 3);
                em_hit_window_set(self, 1, 0x14, 3);
            }
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_mot_set_blend(self, 0x47, 8, 0, 1);
            }
            break;
        }
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            switch ((u8)kind) {
            default:
                em_mot_set_blend(self, 0x48, 8, 0, 1);
                break;
            case 1:
                em_mot_set_blend(self, 0x49, 8, 0, 1);
                break;
            }
        }
        break;
    case 3:
        switch ((u8)kind) {
        default:
                em_turn_in_window(self, 60.0f, 130.0f, 0x8000);
                if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                    em_hit_window_clear(self, 0);
                }
                if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                    em_hit_window_clear(self, 1);
                }
            break;
        case 1:
                em_turn_in_window(self, 40.0f, 80.0f, 0x8000);
                if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
                    em_hit_window_clear(self, 0);
                }
                if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                    em_hit_window_clear(self, 1);
                }
                if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
                    em_hit_window_set(self, 0, 0xB, 3);
                    em_hit_window_set(self, 1, 0x14, 3);
                }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            switch ((u8)kind) {
            default:
                em_action_finish_swim(self);
                break;
            case 1:
                self->state++;
                em_mot_set_blend(self, 0x48, 8, 0, 1);
                break;
            }
        }
        break;
    case 4:
        if (em_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
        }
        em_turn_in_window(self, 60.0f, 130.0f, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80197294 (0x170): motion 0xD8: on its frames the jaw effect ahead of joint 0x1A and the aim effect, then
 * `em_action_finish`. */
void fn_80197294(struct _ENEMY_WORK* self) {
    VEC3 pos;
    VEC3 ofs;

    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD8, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 120.0f, 0.0f) == 1 && fn_8019238C((EmActWork*)self) == 0) {
            get_joint_wpos_em(self, 0x1A, &pos);
            setVector3(&ofs, 0.0f, 20.0f * get_em_scale(self), -60.0f * get_em_scale(self));
            rotVecY(&ofs, self->field_0x1C0);
            addVec3To(&pos, &ofs);
            eft022_spawn(self, &pos, self->area_no);
        }
        if (em_frame_check(self, 0, 180.0f, 0.0f) == 1 && fn_8019238C((EmActWork*)self) == 0) {
            fn_8019255C((EmActWork*)self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80197404 (0x16C): the side swipe: stores the turn toward the target clamped to 60..300 degrees, picks motion
 * 0x52/0x53 by its side and turns through it while the motion runs. */
void fn_80197404(struct _ENEMY_WORK* self) {
    VEC3 dir;
    VEC3 diff;
    u32 pitch;
    u32 yaw;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0: {
        u32 turn;
        self->state++;
        em_move_mode_set(self, 2);
        subVec3(&diff, &self->vec_0x36C, &self->pos);
        copyVec3(&dir, &diff);
        calcVecAngXY(&dir, &pitch, &yaw);
        turn = (u16)(yaw - self->field_0x1C0);
        if (turn < 0x2AAB) {
            turn = 0x2AAB;
        } else if (turn > 0xD555) {
            turn = 0xD555;
        }
        if ((s32)turn > 0x8000) {
            turn = 0x10000 - turn;
            self->state_0x006 = 1;
        }
        self->state_0x007 = (s32)turn >> 8;
        em_mot_set_blend(self, (u16)((self->state_0x006 == 1) + 0x52), 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x13);
        break;
    }
    case 1: {
        s32 angle;
        if (self->state_0x006 == 1) {
            angle = -(self->state_0x007 << 8);
        } else {
            angle = self->state_0x007 << 8;
        }
        em_turn_in_window(self, 10.0f, 50.0f, angle);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
    }
}

/* 0x80197570 (0x148): the backstep: motion 0x3A with the -400 approach, then the `next` pick of the follow-up
 * action (5, 6, 11 or 20 of action 7) or `em_action_finish_swim`. */
void fn_80197570(struct _ENEMY_WORK* self, u32 next) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_approach_start(self, -400.0f, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, 10.0f, 0.0f) == 1 &&
            (em_approach_step(self, 0, 0x40) == 1 || em_mot_end_ck(self) == 1)) {
            switch ((u8)next) {
            case 0:
                em_act_set_unless_act11(self, 7, 5);
                break;
            case 1:
                em_act_set_unless_act11(self, 7, 6);
                break;
            case 2:
                em_act_set_unless_act11(self, 7, 0xB);
                break;
            case 3:
                em_act_set_unless_act11(self, 7, 0x14);
                break;
            default:
                em_action_finish_swim(self);
                break;
            }
        }
        break;
    }
}

/* 0x801976B8 (0x508): the breach: the flag-armed motion 0xCE with its 24-frame build-up (splash and sound on the
 * first frame), motion 0xDD with the bite windows, the rise 0xDF until it clears the surface, the arc 0xE0 with its
 * splashes, then the landing (`em_state_set(self, 7, 0x12)`) or the surface catch 0xE1. */
void fn_801976B8(struct _ENEMY_WORK* self) {
    VEC3 ofs;
    VEC3 pos;

    VEC3_ctor(&ofs);
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 0x18;
        em_stamina_add(self, -0xC);
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        break;
    case 1:
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        if (self->timer_0x020 == 0x18) {
            setVector3(&ofs, 0.0f, 15.0f, 0.0f);
            eft_em_spawn(self, 0x27, 3, &ofs, 1.0f);
            if (self->state_0x006 == 0) {
                if (self->area_no == get_now_areano()) {
                    get_joint_wpos_em(self, 3, &pos);
                    pos.y = 5.0f + self->field_0x20C;
                    se_req_pos_ps(self->se_0xB14, 4, 2, &pos);
                }
                self->state_0x006 = 1;
            }
        }
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDD, 0, 0);
            em_height_set(self, 0.0f);
            em_hit_window_set(self, 0, 0xD, 3);
            em_hit_window_set(self, 1, 0x15, 3);
            em_shake_req_set(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 2, 14.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xDF, 0, 0);
            em_move_vec_clr(self);
            self->field_0x314 = 86.67f;
        }
        break;
    case 3:
        em_move_offset_apply(self);
        if (em_surface_reach_ck(self, 50.0f + em_float_depth_get(self)) == 1) {
            self->state++;
            em_fall_start_c1(self, 0.0f);
        }
        break;
    case 4:
        em_move_offset_apply(self);
        if (self->pos.y >= self->field_0x210) {
            self->state++;
            em_hit_window_clear(self, 0);
            em_hit_window_clear(self, 1);
            em_mot_set_blend(self, 0xE0, 4, 0, 1);
            em_hit_window_set(self, 0, 0x1C, 2);
            em_move_vec2_clr(self);
            self->field_0x314 = 86.67f;
            self->field_0x320 = -3.47f;
            eft009_spawn_at_joint(self, 0xE, 0x2C, 0, 1.0f);
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x30, 0xE, &ofs, 0.7f);
            setVector3(&ofs, 0.0f, -75.0f, 0.0f);
            eft_em_spawn(self, 0x32, 3, &ofs, 1.0f);
        }
        break;
    case 5: {
        u32 landed = em_move_offset_fall_update(self, 200.0f);
        if (em_water_check_c1(self) == 1) {
            if (self->pos.y <= self->field_0x210) {
                self->pos.y = self->field_0x210;
                self->state++;
                em_move_mode_set(self, 2);
                em_mot_set(self, 0xE1, 0, 0);
                em_hit_window_set_default(self, 0, 0x1D);
                em_surface_clamp(self, 300.0f);
                eft009_spawn_at_joint(self, 0xE, 0x2C, 0, 1.0f);
                setVector3(&ofs, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x30, 0xE, &ofs, 0.7f);
                setVector3(&ofs, 0.0f, 0.0f, 15.0f);
                eft_em_spawn(self, 0x31, 3, &ofs, 0.7f);
                em_camera_req(self, -1, 7);
            }
        } else if (landed == 1 && self->field_0x314 < 0.0f) {
            em_move_mode_set(self, 0);
            em_state_set(self, 7, 0x12);
        }
        break;
    }
    case 6:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80197D50 (0x1B4): the flag-armed motion 0xD2: on its frames the jaw effect ahead of joint 0x1A and the aim
 * effect, then the dim flag, completing with `em_action_finish_stand`. */
void fn_80197D50(struct _ENEMY_WORK* self) {
    VEC3 pos;
    VEC3 ofs;

    VEC3_ctor(&pos);
    VEC3_ctor(&ofs);
    self->field_0x35B = 1;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xD2, 0, 0);
        em_flag_0x8AD_set(self);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_frame_check(self, 0, 150.0f, 0.0f) == 1 && fn_8019238C((EmActWork*)self) == 0) {
            get_joint_wpos_em(self, 0x1A, &pos);
            setVector3(&ofs, 0.0f, 20.0f * get_em_scale(self), -50.0f * get_em_scale(self));
            rotVecY(&ofs, self->field_0x1C0);
            addVec3To(&pos, &ofs);
            eft022_spawn(self, &pos, self->area_no);
        }
        if (em_frame_check(self, 0, 210.0f, 0.0f) == 1 && fn_8019238C((EmActWork*)self) == 0) {
            fn_8019255C((EmActWork*)self);
        }
        if (em_frame_check(self, 1, 200.0f, 0.0f) == 1) {
            self->field_0x359 = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x80197F04 (0x24C): the lunge: the flag-armed motion 0xCE for 12 frames, then motion 0xDC with its hit window and
 * the 50 push: it turns, steps, splashes and fires the joint 0xF effect on its frames, then `em_action_finish`. */
void fn_80197F04(struct _ENEMY_WORK* self) {
    VEC3 ofs;

    VEC3_ctor(&ofs);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 0xC;
        em_stamina_add(self, -0xC);
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        break;
    case 1:
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDC, 0, 0);
            em_hit_window_set_default(self, 0, 0xC);
            em_height_set(self, 0.0f);
            em_shake_req_set(self);
            em_move_vec_clr(self);
            self->field_0x318 = 50.0f;
        }
        break;
    case 2:
        if (em_frame_check(self, 2, 34.0f, 0.0f) == 1) {
            em_turn_to_target(self, 0x300);
        }
        if (em_frame_check(self, 3, 38.0f, 50.0f) == 1) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1C, 3, &ofs, 1.0f);
        }
        if (em_frame_check(self, 2, 28.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            setVector3(&ofs, 0.0f, 0.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &ofs, 1.3f, 0x3C);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801987E4 - the action-7 dispatcher (states 0..31). */
void fn_801987E4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801966DC(self);
        break;
    case 1:
        fn_80196768(self);
        break;
    case 2:
        fn_8019687C(self, 0, 0);
        break;
    case 3:
        fn_8019687C(self, 1, 0);
        break;
    case 4:
        fn_80196A50(self, 0);
        break;
    case 5:
        fn_80196B1C(self);
        break;
    case 6:
        fn_80196BF4(self, 0, 0);
        break;
    case 7:
        fn_80196E10(self, 0);
        break;
    case 8:
        fn_80196E10(self, 1);
        break;
    case 9:
        fn_80197294(self);
        break;
    case 10:
        fn_80196E10(self, 2);
        break;
    case 11:
        fn_80197404(self);
        break;
    case 12:
        fn_80196A50(self, 1);
        break;
    case 13:
        fn_80197570(self, 0);
        break;
    case 14:
        fn_80197570(self, 1);
        break;
    case 15:
        fn_80197570(self, 2);
        break;
    case 16:
        fn_80197570(self, 3);
        break;
    case 17:
        fn_801976B8(self);
        break;
    case 18:
        fn_80197BC0(self);
        break;
    case 19:
        fn_80197C84(self);
        break;
    case 20:
        fn_80196BF4(self, 1, 0);
        break;
    case 21:
        fn_80197D50(self);
        break;
    case 22:
        fn_80197F04(self);
        break;
    case 23:
        fn_80198150(self, 0, 0);
        break;
    case 24:
        fn_801986AC(self);
        break;
    case 25:
        fn_80198150(self, 1, 0);
        break;
    case 26:
        fn_80198150(self, 0, 1);
        break;
    case 27:
        fn_80198150(self, 1, 1);
        break;
    case 28:
        fn_8019687C(self, 0, 1);
        break;
    case 29:
        fn_8019687C(self, 1, 1);
        break;
    case 30:
        fn_80196BF4(self, 0, 1);
        break;
    case 31:
        fn_80196BF4(self, 1, 1);
        break;
    }
}

/* 0x80198910 (0x4F0): the sound program of the current sub-state: plays its SE sequence table, or finishes
 * the action for a sub-state with none. */
void fn_80198910(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em018_se_seq_17, 0, 0x17);
        break;
    case 0x18:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x18);
        break;
    case 0x19:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x19);
        break;
    case 0x1A:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x1C);
        break;
    case 0x1D:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x1D);
        break;
    case 0x1E:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x1E);
        break;
    case 0x28:
        em_se_tbl_play(self, em018_se_seq_28, 2, 0x28);
        break;
    case 0x29:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x29);
        break;
    case 0x2A:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x2A);
        break;
    case 0x2B:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x2B);
        break;
    case 0x2C:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x2C);
        break;
    case 0x2D:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x2D);
        break;
    case 0x2E:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x2E);
        break;
    case 0x2F:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x2F);
        break;
    case 0x34:
        em_se_tbl_play(self, em018_se_seq_34, 0, 0x34);
        break;
    case 0x97:
        em_se_tbl_play(self, em018_se_seq_34, 0, 0x97);
        break;
    case 0xAF:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0xAF);
        break;
    case 0xB1:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0xB1);
        break;
    case 0xB2:
        em_se_tbl_play(self, em018_se_seq_B2, 0, 0xB2);
        break;
    case 0x7A:
        em_se_tbl_play(self, em018_se_seq_7A, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(self, em018_se_seq_7B, 0, 0x7B);
        break;
    case 0x89:
        em_se_tbl_play(self, em018_se_seq_89, 2, 0x89);
        break;
    case 0x8A:
        em_se_tbl_play(self, em018_se_seq_8A, 2, 0x8A);
        break;
    case 0x7C:
        em_se_tbl_play(self, em018_se_seq_7C, 0, 0x7C);
        break;
    case 0x8D:
        em_se_tbl_play(self, em018_se_seq_8D, 0, 0x8D);
        break;
    case 0x8B:
        em_se_tbl_play(self, em018_se_seq_8B, 2, 0x8B);
        break;
    case 0x8F:
        em_se_tbl_play(self, em018_se_seq_8F, 2, 0x8F);
        break;
    case 0x7D:
        em_se_tbl_play(self, em018_se_seq_7D, 0, 0x7D);
        break;
    case 0x8E:
        em_se_tbl_play(self, em018_se_seq_8D, 0, 0x8E);
        break;
    case 0x8C:
        em_se_tbl_play(self, em018_se_seq_8C, 2, 0x8C);
        break;
    case 0x90:
        em_se_tbl_play(self, em018_se_seq_8F, 2, 0x90);
        break;
    case 0x78:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0x78);
        break;
    case 0x79:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0x79);
        break;
    case 0x9F:
        em_se_tbl_play(self, em018_se_seq_9F, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(self, em018_se_seq_9F, 0, 0xA0);
        break;
    case 0xA1:
        em_se_tbl_play(self, em018_se_seq_8F, 2, 0xA1);
        break;
    case 0xA2:
        em_se_tbl_play(self, em018_se_seq_8F, 2, 0xA2);
        break;
    case 0xA3:
        em_se_tbl_play(self, em018_se_seq_34, 0, 0xA3);
        break;
    case 0xA8:
        em_se_tbl_play(self, em018_se_seq_18, 0, 0xA8);
        break;
    case 0xAA:
        em_se_tbl_play(self, em018_se_seq_29, 2, 0xAA);
        break;
    case 0xB6:
        em_se_tbl_play(self, em018_se_seq_B6, 0, 0xB6);
        break;
    case 0xB7:
        em_se_tbl_play(self, em018_se_seq_B7, 0, 0xB7);
        break;
    case 0xB8:
        em_se_tbl_play(self, em018_se_seq_B8, 0, 0xB8);
        break;
    case 0xB9:
        em_se_tbl_play(self, em018_se_seq_B9, 0, 0xB9);
        break;
    case 0xBA:
        em_se_tbl_play(self, em018_se_seq_BA, 0, 0xBA);
        break;
    case 0xBB:
        em_se_tbl_play(self, em018_se_seq_BB, 0, 0xBB);
        break;
    case 0xBC:
        em_se_tbl_play(self, em018_se_seq_BB, 0, 0xBC);
        break;
    case 0xBF:
        em_se_tbl_play(self, em018_se_seq_BF, 0, 0xBF);
        break;
    case 0xC1:
        em_se_tbl_play(self, em018_se_seq_8D, 0, 0xC1);
        break;
    case 0xC0:
        em_se_tbl_play(self, em018_se_seq_C0, 2, 0xC0);
        break;
    case 0xC2:
        em_se_tbl_play(self, em018_se_seq_8F, 2, 0xC2);
        break;
    case 0xC8:
        em_se_tbl_play(self, em018_se_seq_C8, 0, 0xC8);
        break;
    case 0xD0:
        em_se_tbl_play(self, em018_se_seq_D0, 2, 0xD0);
        break;
    case 0xE4:
        em_se_tbl_play(self, em018_se_seq_E4, 0, 0xE4);
        break;
    case 0xE5:
        em_se_tbl_play(self, em018_se_seq_E5, 0, 0xE5);
        break;
    case 0xE6:
        em_se_tbl_play(self, em018_se_seq_E6, 0, 0xE6);
        break;
    case 0xE7:
        em_se_tbl_play(self, em018_se_seq_E7, 0, 0xE7);
        break;
    case 0xE8:
        em_se_tbl_play(self, em018_se_seq_E8, 0, 0xE8);
        break;
    case 0xE9:
        em_se_tbl_play(self, em018_se_seq_E8, 0, 0xE9);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* 0x80198E00 (0x110): the alternate sound program of the current sub-state (the default plays sub-state
 * 0's table). */
void fn_80198E00(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x0:
        em_se_tbl_play_alt(self, em018_se_alt_seq_00, 0, 0);
        break;
    case 0xF:
        em_se_tbl_play_alt(self, em018_se_alt_seq_0F, 0, 0xF);
        break;
    case 0x1A:
        em_se_tbl_play_alt(self, em018_se_alt_seq_1A, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play_alt(self, em018_se_alt_seq_1B, 2, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play_alt(self, em018_se_alt_seq_1C, 0, 0x1C);
        break;
    case 0xA:
        em_se_tbl_play_alt(self, em018_se_alt_seq_0A, 2, 0xA);
        break;
    case 0xB:
        em_se_tbl_play_alt(self, em018_se_alt_seq_0B, 0, 0xB);
        break;
    case 0x34:
        em_se_tbl_play_alt(self, em018_se_alt_seq_34, 0, 0x34);
        break;
    default:
        em_se_tbl_play_alt(self, em018_se_alt_seq_00, 0, 0);
        break;
    }
}

/* 0x80198F10 - forwards to this band's action-6 state-3 step. */
void fn_80198F10(struct _ENEMY_WORK* self) {
    fn_80195CA4(self);
}

/* 0x80198F14 - runs `fn_80198F10` while the action-8 sub-state is 0. */
void fn_80198F14(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_80198F10(self);
    }
}

/* 0x801928B8 - a bare `return` (the target body is a single `blr`). */
void fn_801928B8(void) {
}

/* ---------------------------------------------------------------------------------------------- */
/* the per-action steps the 0x801953BC/0x80196618/0x801987E4 dispatchers select                 */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80193EF0 - arms mode 2 with (0x32/0x14/0/3) and the `em_action_finish_swim` completion. */
void fn_80193EF0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x14, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80193F70 - arms mode 2 with (0x31/0x14/0/1) and the `em_state_set(self, 5, 0x1D)` completion. */
void fn_80193F70(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x1D);
        }
        break;
    }
}

/* 0x80193FF8 - arms mode 2 with (0x3A/0x0A/0/1) and the `em_action_finish_swim` completion. */
void fn_80193FF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80194938 - arms mode 2 with motion 0x68 for 10 frames; completion `em_action_finish_swim`. */
void fn_80194938(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80194A64 - arms mode 2 with (0x2E/6/0/1); completion `em_action_finish_swim`. */
void fn_80194A64(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 6, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x8019533C - arms mode 2 with (0x36/0x14/0/3); completion `em_action_finish_swim`. */
void fn_8019533C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x14, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x801966DC - arms motion 0x54 for 10 frames with the `em_hit_window_set_default(self, 0, 1)` pair. */
void fn_801966DC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x54, 0xA, 0);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196A50 - arms motion 0x57 for 10 frames and the two joint slots whose ids depend on `a`. */
void fn_80196A50(struct _ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x57, 0xA, 0);
        {
            u32 first, second;
            if (a == 1) {
                first = 0x16;
                second = 0x17;
            } else {
                first = 6;
                second = 7;
            }
            em_hit_window_set(self, 0, first, 8);
            em_hit_window_set(self, 1, second, 0x10);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80194CC8 - the release step: clears the bit pair, arms motion 0x80 and runs the +0x020 timer
 * down to zero, then completes with `em_state_set(self, 5, 0x20)`. */
void fn_80194CC8(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x80, 4, 0);
        self->timer_0x020 = 0xBE;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_state_set(self, 5, 0x20);
        }
        break;
    }
}

/* 0x80195BCC - the flag-arming release step: arms motion 0xCD, refreshes the scale, then either
 * the `em_state_set(self, 6, 1)` completion (`a == 1`) or `em_action_finish_stand`. */
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCD, 0, 0);
        em_flag_0x8AD_set(self);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (a == 1) {
                em_state_set(self, 6, 1);
            } else {
                em_action_finish_stand(self);
            }
        }
        break;
    }
}

/* 0x80194318 - arms motion 0x3D with the common `em_action_finish` completion. */
void fn_80194318(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x3D, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193964 - the requested-motion step: `a == 1` primes the two-state release first; state 1
 * waits on the `lbl_80570090` descriptor and runs `em_action_finish_swim` when it finishes. */
void fn_80193964(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
            em_action_finish_swim(self);
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, lbl_80798298);
        } else {
            em_height_follow(self, lbl_80798260);
        }
        break;
    }
}

/* 0x80193A34 - the same shape for the `lbl_805700D0` descriptor. */
void fn_80193A34(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_805700D0, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_805700D0) == 1) {
            em_action_finish_swim(self);
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, lbl_80798298);
        } else {
            em_height_follow(self, lbl_80798260);
        }
        break;
    }
}

/* 0x80198F28 - the same descriptor shape as `fn_80193964` with the
 * `em_state_set(self, 0xD, 1)` completion. */
void fn_80198F28(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
            em_state_set(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, lbl_80798298);
        } else {
            em_height_follow(self, lbl_80798260);
        }
        break;
    }
}

/* 0x8019515C - the same descriptor shape for `lbl_80570110`. */
void fn_8019515C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570110, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570110) == 1) {
            em_action_finish_swim(self);
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, lbl_80798298);
        } else {
            em_height_follow(self, lbl_80798260);
        }
        break;
    }
}

/* 0x801949B4 - arms motion 0x1F for 4 frames, fires the joint effect on the frame and completes with
 * `em_action_finish_swim`. */
void fn_801949B4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x1F, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            em_camera_req(self, -1, 7);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80196B1C - arms motion 0x4E with the two joint slots 8/9; while the frame effect has not
 * expired it keeps the two-state release armed. */
void fn_80196B1C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 8, 8);
        em_hit_window_set(self, 1, 9, 0x10);
        em_stamina_add(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798338, lbl_80798238) == 0) {
            em_height_follow(self, lbl_80798260);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80195638 - the flag-armed step: arms motion 0xD0, then holds the two flags while the frame
 * effects run.  The second argument the dispatcher passes is unused. */
void fn_80195638(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD0, 0, 0);
        em_height_set(self, lbl_80798238);
        em_shake_req_set(self);
        self->field_0x359 = 1;
        break;
    case 1:
        em_busy_set(self);
        if (em_frame_check(self, 2, lbl_80798290, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 2, lbl_8079827C, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196768 - the three-state step: arm motion 0x59, advance to 0x5A when the frame gate fires,
 * then complete with `em_action_finish`. */
void fn_80196768(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 0xA, 0);
        em_hit_window_set(self, 0, 3, 3);
        em_approach_start(self, lbl_80798328, 0);
        em_stamina_add(self, -0xC);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0x5A, 2, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079832C, lbl_80798238) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80197C84 - arms motion 0xA for 8 frames and the `lbl_80798270` blend; completes with
 * `em_act_set_unless_act11(self, 7, 0)`. */
void fn_80197C84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xA, 8, 0, 1);
        em_approach_start(self, lbl_80798270, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798370, lbl_80798238) == 1 &&
            (em_approach_step(self, 0, 0x40) == 1 || em_mot_end_ck(self) == 1)) {
            em_act_set_unless_act11(self, 7, 0);
        }
        break;
    }
}

/* 0x80198150 (0x55C): the spin: motion 0xD4, then 0xD5 with the spin window; for 180 frames the two material blends
 * follow a cosine and the heading swings with it, the capsule hit plays the shell SE, a pinned player ends the
 * sweep early, and the recovery runs 30 frames before `em_state_set(self, 7, 0x18)`. */
void fn_80198150(struct _ENEMY_WORK* self, u32 alt, u32 reverse) {
    VEC3 unused;
    VEC3 ofs;
    VEC3 hit;
    nw4r::math::CAPSULE capsule;
    f32 blend;

    VEC3_ctor(&unused);
    VEC3_ctor(&ofs);
    eft_vec_ctor(&hit);
    eft_capsule_ctor(&capsule);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD4, 0, 0);
        em_height_set(self, 0.0f);
        fn_80192358((EmActWork*)self, 8);
        em_stamina_add(self, -0xC);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            self->state_0x007 = 0;
            em_mat_blend_reset(self);
            em_mot_set(self, 0xD5, 0, 0);
            em_mot_layer_set(self, 1, 0xD7, 0, 0);
            if ((u8)alt == 1) {
                em_hit_window_set(self, 0, 0x19, 2);
            } else {
                em_hit_window_set(self, 0, 0xE, 2);
            }
            self->timer_0x020 = 0;
            setVector3(&ofs, 0.0f, -80.0f, 0.0f);
            eft026_pl_spawn_c1(self, 3, &ofs, 0xF, 1.0f);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.x = 0.0f;
        }
        break;
    case 2:
        if (self->timer_0x020 < 0xB4) {
            self->timer_0x020++;
        }
        if (self->timer_0x020 < 0xB4) {
            s16 deg = self->timer_0x020 * 4;
            f32 c;
            if (deg >= 360) {
                deg %= 360;
            }
            blend = 0.5f * (1.0f + cosf((6.2831855f * deg) / 360.0f));
            c = cosf((6.2831855f * deg) / 360.0f);
            sinf((6.2831855f * deg) / 360.0f);
            self->field_0x1C0 = (u16)(self->field_0x1C0 + (s32)(224.0f * c));
        } else {
            blend = 1.0f;
        }
        if (blend < 0.0f) {
            blend = 0.0f;
        } else if (blend > 1.0f) {
            blend = 1.0f;
        }
        mhchar_mat_blend_set((MHchar*)self->char_0x024, 0, blend);
        mhchar_mat_blend_set((MHchar*)self->char_0x024, 1, 1.0f - blend);
        if (self->state_0x006 <= 1) {
            if (0.0f == self->offset_0x30C.vec_0x310.x && em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = 1.0f;
            }
            if (self->offset_0x30C.vec_0x310.x > 0.0f && hit_list_capsule_ck(self->hit_0xA08, &capsule, &hit, 0) == 1 &&
                self->area_no == get_now_areano()) {
                shell_se_req2(self->se_0xB14, &capsule, 7, self->field_0x01A);
            }
        }
        switch (self->state_0x006) {
        case 0: {
            u16 max;
            _PLW* plw;
            s32 i;
            if ((u8)reverse == 1) {
                self->field_0x1C0 = (u16)(self->field_0x1C0 - 0x60);
            } else {
                self->field_0x1C0 = (u16)(self->field_0x1C0 + 0x60);
            }
            if (self->timer_0x020 >= 0xAA) {
                self->state_0x006 = 2;
                break;
            }
            max = get_move_work_max(2);
            plw = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < max; i++, plw++) {
                if (plw->slot_active != 0 && em_pl_slot_leaving_ck(plw->chunk_ofs) == 0 && em_pl_target_ck(self, plw) != 0 &&
                    Pl_act_ck(plw, 6, 0x29) != 0 && plw->sub_area_0x3B6 == self->field_0x01A) {
                    self->state_0x006 = 1;
                    self->state_0x007 = 0;
                }
            }
            break;
        }
        case 1:
            if (self->timer_0x020 >= 0xAA) {
                self->state_0x006 = 2;
            } else if (self->state_0x007 < 10) {
                self->state_0x007++;
            } else {
                self->state_0x006 = 2;
            }
            break;
        case 2:
            self->state_0x006++;
            self->state_0x007 = 0;
            em_hit_window_clear(self, 0);
            fn_80192348((EmActWork*)self, 8);
        default:
            if (self->state_0x007 < 0x1E) {
                self->state_0x007++;
            } else {
                em_state_set(self, 7, 0x18);
            }
            break;
        }
        break;
    }
}

/* 0x801986AC - the three-state step: arm motion 0xD6, advance to 0xD3 on completion, then the
 * `lbl_8079839C` joint release and `em_action_finish_swim`. */
void fn_801986AC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xD6, 0x14, 0, 1);
        em_hit_window_set(self, 0, 2, 3);
        em_height_set(self, lbl_80798238);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798398, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xD3, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079839C, lbl_80798238) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80197BC0 - the two-stage rotation step: state 0 advances the +0x1C0 angle by 0x8000, state 1
 * arms motion 0x7F, state 2 completes with `em_action_finish`. */
void fn_80197BC0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7E, 0, 0);
        self->field_0x1C0 += 0x8000;
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x7F, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801961C8 - the flag-armed step: arms motion 0xCE, refreshes the scale and completes through
 * `em_action_finish_stand`.  `a` only selects the two-state release at entry. */
void fn_801961C8(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x8019610C - the same arming as `fn_801961C8` with the `em_turn_to_target(self, 0x300)` completion. */
void fn_8019610C(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_flag_0x8AD_set(self);
        break;
    case 1:
        if (em_turn_to_target(self, 0x300) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x8019485C - arms motion 0xCA for 10 frames, advances to 0xDE on completion, then `em_action_finish_swim`. */
void fn_8019485C(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCA, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xDE, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80195098 - arms motion 0x7F, advances to 0x82 on completion, then `em_action_finish`. */
void fn_80195098(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7F, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x82, 0, 0xBC);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80195ED4 - arms motion 0xDA with the joint slot 0xF; the frame effects re-arm the flag and
 * fire the joint effect, and `em_action_finish` completes the step. */
void fn_80195ED4(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDA, 0, 0);
        em_hit_window_set_default(self, 0, 0xF);
        em_height_set(self, lbl_80798238);
        em_stamina_add(self, -0xC);
        self->field_0x359 = 1;
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798288, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 2, lbl_80798308, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 3, lbl_8079830C, lbl_80798288) == 1) {
            em_turn_to_target(self, 0x400);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196004 - arms motion 0xDB with the joint slot 0x10 and the two frame-effect hooks. */
void fn_80196004(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDB, 0, 0);
        em_height_set(self, lbl_80798238);
        em_hit_window_set_default(self, 0, 0x10);
        em_stamina_add(self, -0xC);
        self->field_0x359 = 1;
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798310, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 2, lbl_80798314, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80198FE8 - arms mode 2 with (0x37/0x14/0/1) and the `lbl_807983A0` blend; state 1 waits on the
 * descriptor and advances to (0x2F/0x28/0/1), completing with `em_state_set(self, 0xD, 2)`. */
void fn_80198FE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        em_approach_start(self, lbl_807983A0, 0x12);
        break;
    case 1:
        em_height_follow(self, lbl_807982E0);
        if (em_approach_step(self, 0, 0x80) == 1 && em_mot_frame_started_ck(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 2);
        }
        break;
    }
}

/* 0x801990E0 (0x104): motion 0x4E with the two hit windows, holding the two-state release until its frame; on the
 * end it chases the hunted record (`em_state_set(self, 0xD, 3)`) when it is in reach, else `em_action_finish_swim`. */
void fn_801990E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x1A, 8);
        em_hit_window_set(self, 1, 0x1B, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 0) {
            em_height_follow(self, 10.0f);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (fn_8019E49C(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
            } else {
                em_action_finish_swim(self);
            }
        }
        break;
    }
}

/* 0x801991E4 (0x1FC): the closing turn: arms the `lbl_80570090` descriptor and a 50-frame glide toward the
 * target that stops 600 short (at most 50 per frame), then picks the hunted-record chase or action 5's step 10. */
void fn_801991E4(struct _ENEMY_WORK* self) {
    VEC3 diff;
    VEC3 dir;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0: {
        u16 angle;
        f32 step;
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        self->timer_0x020 = 0x32;
        subVec3(&diff, &self->vec_0x36C, &self->pos);
        copyVec3(&dir, &diff);
        angle = calcVecAng3(&dir);
        dir.y = 0.0f;
        step = (vec3_len((const f32*)&dir) - 600.0f * get_em_chg_scale(self)) / self->timer_0x020;
        if (step > 50.0f) {
            step = 50.0f;
        } else if (step < -50.0f) {
            step = -50.0f;
        }
        setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, step);
        rotVecY(&self->offset_0x30C.vec_0x310, angle);
        break;
    }
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
            em_move_offset_apply(self);
        }
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
            if (em_busy_ck(self) == 1) {
                if (fn_8019E49C(self, 1) == 1) {
                    em_state_set(self, 0xD, 4);
                } else {
                    em_state_set(self, 5, 0xA);
                }
            }
        } else if (self->field_0x482 != 0) {
            em_height_follow(self, 5.0f);
        } else {
            em_height_follow(self, 10.0f);
        }
        break;
    }
}

/* 0x801993E0 - the state-advance entry: state 0 arms the motion, state 1 waits for `em_mot_end_ck` and runs
 * `em_state_set(self, 13, 5)`. */
void fn_801993E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 49, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 13, 5);
        }
        break;
    }
}

/* 0x80199468 - the same shape with this action's own arming pair (mode 46, duration 6, the 1000 ms
 * `em_stamina_add` timer) and `em_action_finish_swim` as the completion. */
void fn_80199468(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 46, 6, 0, 1);
        em_stamina_add(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x801994F4 (0x538): the scripted entrance: a chain of demo-timed motions (0xCE, 0xCF, 0xD4, 0xD5, 0xD3 along the
 * `em018_demo_keys` path, 0x28, 0x1F) with the demo position and rotation pins, the joint 0xF effects and the
 * depth clamp at -1110. */
void fn_801994F4(struct _ENEMY_WORK* self) {
    VEC3 vec;
    MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em_demo_pos_set(self, -65.0f, -1622.0f, -1709.0f);
        em_demo_rot_set(self, 0.0f, 182.0f, 0.0f);
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        break;
    case 1:
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        if (em_demo_time_ck(0x17E) == 1) {
            self->state++;
            em_mot_set(self, 0xCF, 0, 0);
            em_demo_pos_set(self, -3.5f, -1622.0f, -868.0f);
        }
        break;
    case 2:
        self->field_0x359 = 1;
        self->field_0x35B = 1;
        if (em_demo_time_ck(0x2B4) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set_blend(self, 0xD4, 0xE, 0, 1);
            em_height_set(self, 0.0f);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            setVector3(&vec, 0.0f, -80.0f, 0.0f);
            eft026_pl_spawn_c1(self, 3, &vec, 0xF, 1.0f);
            em_move_mode_set(self, 0);
            em_mot_set_blend(self, 0xD5, 2, 0, 1);
            em_demo_pos_set(self, -2.4f, -1622.0f, -836.0f);
            em_demo_rot_set(self, 0.0f, 182.0f, 0.0f);
            em_height_set(self, 0.0f);
        }
        break;
    case 4:
        if (em_demo_time_ck(0x448) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xD3, 0, 0);
            em_hit_window_set(self, 0, 0x1E, 3);
            em_demo_key3_apply(self, em_demo_frame_get(), em018_demo_keys, 0);
            em_demo_rot_set(self, 0.0f, -178.0f, 0.0f);
            if (self->pos.y < -1110.0f) {
                em_height_set(self, 1110.0f + self->pos.y);
            } else {
                em_height_set(self, 0.0f);
            }
        }
        break;
    case 5:
        if (em_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -60.0f, 500.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.5f, 0x3C);
        }
        if (em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
            setVector3(&vec, 0.0f, -60.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 0.9f, 0x3C);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), em018_demo_keys, 0);
        if (self->pos.y < -1110.0f) {
            em_height_set(self, 1110.0f + self->pos.y);
        } else {
            em_height_set(self, 0.0f);
        }
        if (em_demo_time_ck(0x4B6) == 1) {
            self->state++;
            em_mot_set(self, 0xD3, 0, 0x6E);
            em_demo_pos_set(self, -596.0f, -788.8f, -2324.0f);
            em_demo_rot_set(self, 0.0f, 182.0f, 0.0f);
        }
        break;
    case 6:
        if (em_demo_time_ck(0x552) == 1) {
            self->state++;
            em_mot_set(self, 0x28, 0xA, 0);
        }
        break;
    case 7:
        if (em_frame_check(self, 0, 144.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -20.0f, 200.0f);
            get_joint_wmat_em(self, 0xF, &mtx);
            mulVecMat(&vec, &mtx);
            vec.x += mtx.m[0][3];
            vec.y += mtx.m[1][3];
            vec.z += mtx.m[2][3];
            eft_spawn_type10(self, 0x11, 0xF, &vec, 1.0f);
        }
        if (em_demo_time_ck(0x6EA) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x1F, 2, 0, 1);
            em_demo_pos_set(self, -592.5f, -788.5f, -2266.0f);
        }
        break;
    }
}

/* 0x80199A2C - `fn_80199ADC`'s case 7: arm mode 31 with the `em_demo_pos_set`/`em_demo_rot_set` pair and zero
 * the stored height, then wait for `em_mot_end_ck` and run `em_action_finish_swim`. */
void fn_80199A2C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 31, 0, 0);
        em_demo_pos_set(self, lbl_807983F0, lbl_807983F4, lbl_807983F8);
        em_demo_rot_set(self, lbl_80798238, lbl_807983B0, lbl_80798238);
        em_height_set(self, lbl_80798238);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_swim(self);
        }
        break;
    }
}

/* 0x80199ADC - the action-id dispatcher: `self->state_sub` (0x1E6) selects this unit's per-action
 * update. */
void fn_80199ADC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80198F28(self);
        break;
    case 1:
        fn_80198FE8(self);
        break;
    case 2:
        fn_801990E0(self);
        break;
    case 3:
        fn_801991E4(self);
        break;
    case 4:
        fn_801993E0(self);
        break;
    case 5:
        fn_80199468(self);
        break;
    case 6:
        fn_801994F4(self);
        break;
    case 7:
        fn_80199A2C(self);
        break;
    }
}

/* 0x80199B24 - the `action` (0x1E5) dispatcher, plus the shared tail: the +0x1E2 gate that runs
 * `em_busy_set`/`em_busy_timer_reset`, then `fn_8019E398`. */
void fn_80199B24(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80192F24(self);
        break;
    case 1:
        fn_80193394(self);
        break;
    case 2:
        fn_801938D8(self);
        break;
    case 5:
        fn_801953BC(self);
        break;
    case 6:
        fn_80196618(self);
        break;
    case 7:
        fn_801987E4(self);
        break;
    case 10:
        fn_80198910(self);
        break;
    case 11:
        fn_80198E00(self);
        break;
    case 12:
        fn_80198F14(self);
        break;
    case 13:
        fn_80199ADC(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    fn_8019E398(self);
}


/* 0x80199BE0 (0x3D4): the effect-spawn router: `kind` picks the effect family, `joint` the source (0xFF: the
 * work's own position at the +0x20C/+0x210 height), then `eft009_set_pos`, `eft009_spawn_at_joint`,
 * `eft_spawn_pos_in_area` or `eft_spawn_type11`. */
void em018_effect_spawn(struct _ENEMY_WORK* self, u8 kind, u8 type, u32 joint, s32 delta, f32 scale) {
    VEC3 vec;
    u8 id;

    id = type;
    VEC3_ctor(&vec);
    switch (kind) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (id) {
            case 2:
            case 8:
                id = 0xC;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, delta, scale);
                }
                break;
            case 0:
            case 1:
            case 3:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 36:
            case 37:
            case 41:
            case 42:
                return;
            default:
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                }
                break;
            }
        } else {
            if ((u32)(id - 12) <= 13) {
                return;
            }
            switch (id) {
            case 38:
                return;
            case 43:
                eft009_spawn_at_joint(self, joint, 0x11, delta, scale);
                return;
            }
            if (joint == 0xFF) {
                vec.x = self->pos.x;
                vec.y = 5.0f + self->field_0x20C;
                vec.z = self->pos.z;
            }
        }
        if (joint == 0xFF) {
            eft009_set_pos(id, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
            return;
        }
        eft009_spawn_at_joint(self, joint, id, delta, scale);
        return;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            if ((u32)(id - 3) <= 1) {
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x11, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                    return;
                }
                eft009_spawn_at_joint(self, joint, 0x11, delta, scale);
                return;
            }
            if (id == 5) {
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x14, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                    return;
                }
                eft009_spawn_at_joint(self, joint, 0x14, delta, scale);
                return;
            }
        }
        if (joint == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &vec);
        }
        vec.y = self->field_0x20C;
        eft_spawn_pos_in_area(&vec, self->area_no, id, self->field_0x1C0 + delta, scale * get_em_chg_scale(self));
        return;
    case 2:
        if (joint == 0xFF) {
            copyVec3(&vec, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &vec);
        }
        vec.y = self->field_0x20C;
        eft_spawn_type11(self, &vec, id, scale * get_em_chg_scale(self));
        return;
    }
}

/* 0x80199FB4 (0x220): while the alternate mode holds and the head pitch is past the limit, drips the water or
 * steam effect from joint 0xF every 32 frames (all but a few motions). */
void em018_drip_effect(struct _ENEMY_WORK* self) {
    VEC3 vec;
    MTX34 mtx;
    u16 angle;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    if (em_alt_mode_ck(self) == 0) {
        return;
    }
    angle = calcVecAngX(&self->vec_0x76C);
    if ((u16)(angle + 0x8000) <= 0x671B) {
        return;
    }
    switch (em_get_mot_no(self)) {
    case 0x1F:
    case 0x32:
    case 0x35:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x6A:
    case 0x73:
    case 0xD3:
    case 0xD4:
    case 0xD5:
    case 0xD6:
        return;
    }
    setVector3(&vec, 0.0f, -20.0f, 70.0f);
    get_joint_wmat_em(self, 0xF, &mtx);
    mulVecMat(&vec, &mtx);
    vec.x += mtx.m[0][3];
    vec.y += mtx.m[1][3];
    vec.z += mtx.m[2][3];
    if (em_water_check_c1(self) == 1 && vec.y < self->field_0x210) {
        if ((system_w.field_0x0c & 0x1F) == 0) {
            if ((u16)(angle + 0x8000) > 0x6E38) {
                eft_spawn_type10(self, 0x12, 0xF, &vec, 1.0f);
            } else {
                eft_spawn_type10(self, 0x11, 0xF, &vec, 1.0f);
            }
        }
    } else if ((system_w.field_0x0c & 0x1F) == 0) {
        if ((u16)(angle + 0x8000) > 0x6E38) {
            eft_spawn_type10(self, 0x10, 0xF, &vec, 1.0f);
        } else {
            eft_spawn_type10(self, 0xF, 0xF, &vec, 1.0f);
        }
    }
}

/* 0x8019A1D4 (0x3030): the per-motion frame-window driver: each `em_after_frame_check` window that fires spawns
 * an effect (`field_0x228 & 6` picks the large/air variant), then the idle drips and the glow state run. */
void em018_motion_events(struct _ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em018_drip_effect(self);
    switch (em_get_mot_no(self)) {
    case 0x2:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
        }
        break;
    case 0x6:
        if (em_after_frame_check(self, 0, 246.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 246.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.5f);
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.3f);
        }
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.3f);
            em_camera_req(self, 8, 7);
        }
        break;
    case 0x14:
        if ((self->field_0x1E2 == 2) && (self->pos.y < (self->field_0x210 - 200.0f))) {
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 30.0f, 140.0f);
                eft_spawn_type10(self, 0x12, 0xF, &vec, 0.8f);
            }
            if (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 25.0f, 150.0f);
                eft_spawn_type10(self, 0x12, 0xF, &vec, 0.6f);
            }
        }
        break;
    case 0x32:
        if ((self->field_0x1E2 == 2) && (self->pos.y < (self->field_0x210 - 200.0f))) {
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, -30.0f, 160.0f);
                eft_spawn_type10(self, 0x12, 0xF, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, -35.0f, 180.0f);
                eft_spawn_type10(self, 0x12, 0xF, &vec, 0.8f);
            }
        }
        break;
    case 0x15:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.2f);
        }
        break;
    case 0x16:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.2f);
        }
        break;
    case 0x18:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
        }
        break;
    case 0x19:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        break;
    case 0x1A:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        break;
    case 0x1F:
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -60.0f, 120.0f);
            eft_em_spawn(self, 0x1D, 0xF, &vec, 1.0f);
        }
        break;
    case 0x1B:
        if (em_water_check_c1(self) == 1) {
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0xD, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0x25, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 8, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, -80.0f, 0.0f);
                eft_em_spawn(self, 0x5D, 0xF, &vec, 0.7f);
            }
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0x29, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0xD, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0x25, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, -80.0f, 0.0f);
                eft_em_spawn(self, 0x5D, 0xF, &vec, 0.7f);
            }
            if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 8, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0xD, &vec, 1.0f);
            }
            if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x40, 0x29, &vec, 1.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
            if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
            if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
            if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 8, 0xE, 0, 2.5f);
            }
        }
        break;
    case 0x2E:
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -40.0f, 180.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.4f, 0x3C);
        }
        break;
    case 0x31:
        if (em_after_frame_check(self, 0, 244.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -20.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.5f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 276.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 0.9f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 310.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.2f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 358.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -30.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.0f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 412.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.0f, 0x3C);
        }
        break;
    case 0x39:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em_eft_fx_spawn(self, 0, 0x2A, 8, 1.0f);
            vec.x = 0.0f;
            vec.y = -100.0f;
            vec.z = 100.0f;
            shell_set_func_ptr->method_0x2C(self, 3, 1, &vec, 0.7f, 0xFFFF, shell_set_func_ptr);
        }
        break;
    case 0x3B:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em018_effect_spawn(self, 1, 4, 3, 0, 0.7f);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x3D:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0x11, 0xF, 0, 1.4f);
            } else {
                em018_effect_spawn(self, 1, 3, 0x21, 0, 0.8f);
            }
        }
        break;
    case 0x3E:
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 2, 2, 0x2A, 0, 0.5f);
            } else {
                em018_effect_spawn(self, 1, 6, 0x2A, 0, 2.0f);
            }
        }
        break;
    case 0x46:
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair(self, 2, 0x2A, 0xF, 0x1E, 0.9f);
        }
        break;
    case 0x47:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair(self, 2, 0x2A, 0xF, 0x1E, 0.9f);
        }
        break;
    case 0x49:
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair(self, 2, 0x2A, 0xF, 0x18, 0.7f);
        }
        break;
    case 0x4E:
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 85.0f, 350.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.3f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -100.0f, 250.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.3f, 0x3C);
        }
        break;
    case 0x4F:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 70.0f);
            eft_em_spawn(self, 0x2A, 0xF, &vec, 1.0f);
        }
        if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            setVector3(&vec, 140.0f, 120.0f, 80.0f);
            eft_em_spawn(self, 0x2C, 0xF, &vec, 1.0f);
            setVector3(&vec, 0.0f, 130.0f, 30.0f);
            eft_em_spawn(self, 0x2C, 0xF, &vec, 1.0f);
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 100.0f, 0.0f);
            eft_em_spawn(self, 0x2B, 3, &vec, 1.0f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x50:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1C, 0xF, &vec, 1.0f);
        }
        break;
    case 0x51:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, -120.0f, 0.0f, 130.0f);
            eft_em_spawn(self, 0x37, 0xF, &vec, 1.3f);
        }
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 120.0f, 0.0f, 130.0f);
            eft_em_spawn(self, 0x37, 0xF, &vec, 1.3f);
        }
        break;
    case 0x52:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0x13);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -100.0f, 250.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.3f, 0x3C);
        }
        break;
    case 0x53:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_hit_window_set_default(self, 0, 0x13);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -100.0f, 250.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.3f, 0x3C);
        }
        break;
    case 0x54:
        if (em_water_check_c1(self) == 1) {
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, -50.0f, 250.0f);
                eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.3f, 0x3C);
            }
        } else if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0xF, 0x15, 0, 1.0f, &vec);
            } else {
                eft009_spawn_at_pos(self, 0xF, 7, 0, 0.4f, &vec);
            }
        }
        break;
    case 0x55:
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        if (em_water_check_c1(self) == 0) {
            if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em018_effect_spawn(self, 0, 0x11, 3, 0, 1.2f);
                } else {
                    em018_effect_spawn(self, 1, 6, 3, 0, 2.0f);
                }
            }
            if (em_after_frame_check(self, 0, 220.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
            }
        }
        if (em_water_check_c1(self) == 1) {
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 200.0f, 0.0f);
                eft_em_spawn(self, 0x5C, 3, &vec, 1.1f);
            }
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x42, 0xF, &vec, 0.8f);
            }
        }
        break;
    case 0x56:
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        if (em_water_check_c1(self) == 0) {
            if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em018_effect_spawn(self, 0, 0x11, 3, 0, 1.2f);
                } else {
                    em018_effect_spawn(self, 1, 6, 3, 0, 2.0f);
                }
            }
            if (em_after_frame_check(self, 0, 220.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
            }
        }
        if (em_water_check_c1(self) == 1) {
            if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 200.0f, 0.0f);
                eft_em_spawn(self, 0x5C, 3, &vec, 1.1f);
            }
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x42, 0xF, &vec, 0.8f);
            }
        }
        break;
    case 0x57:
        if (em_water_check_c1(self) == 1) {
            if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn_joint(self, 0x2D, 0xB, &vec, 1.2f, 0x1E);
                eft_em_spawn_joint(self, 0x2D, 0xE, &vec, 1.2f, 0x3C);
            }
            if (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn_joint(self, 0x2D, 0xE, &vec, 1.1f, 0x3C);
            }
        } else {
            if (em_after_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em018_effect_spawn(self, 0, 0x12, 0x2C, 0, 1.3f);
                } else {
                    em018_effect_spawn(self, 0, 2, 0x2C, 0, 2.5f);
                }
            }
            if (em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
                if ((self->field_0x228 & 6) != 0) {
                    em018_effect_spawn(self, 0, 0x12, 0x2C, 0, 1.3f);
                } else {
                    em018_effect_spawn(self, 0, 2, 0x2C, 0, 2.5f);
                }
            }
        }
        break;
    case 0x59:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 1.5f);
            em_camera_req(self, -1, 0);
        }
        break;
    case 0x5A:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 2, 2, 0xF, 0, 0.6f);
            } else {
                em018_effect_spawn(self, 1, 3, 3, 0, 0.6f);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x6A:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 140.0f);
            eft_em_spawn_joint(self, 0x10, 0xD, &vec, 1.4f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 140.0f);
            eft_em_spawn_joint(self, 0x10, 0xD, &vec, 1.5f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 184.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 140.0f);
            eft_em_spawn_joint(self, 0x10, 0xD, &vec, 1.1f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 274.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 140.0f);
            eft_em_spawn_joint(self, 0x10, 0xD, &vec, 1.2f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 306.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 140.0f);
            eft_em_spawn(self, 0x3B, 3, &vec, 1.0f);
        }
        break;
    case 0x6E:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0xC, 8, 0, 1.0f);
                em018_effect_spawn(self, 0, 0xE, 8, 0, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
        }
        break;
    case 0x6F:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0xC, 0xD, 0, 0.8f);
            } else {
                em018_effect_spawn(self, 0, 2, 0xD, 0, 1.8f);
            }
        }
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0xC, 8, 0, 0.8f);
            } else {
                em018_effect_spawn(self, 0, 2, 8, 0, 1.8f);
            }
        }
        break;
    case 0x71:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x11, 3, 0, 1.2f);
            }
        } else if (em_after_frame_check(self, 0, 154.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 1, 0, 3, 0, 1.0f);
        }
        break;
    case 0x72:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x12, 0xD, 0, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 0, 2, 0xD, 0, 2.0f);
        }
        break;
    case 0x73:
        if (em_after_frame_check(self, 0, 326.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0x10, 3, 0, 1.2f);
            } else {
                em018_effect_spawn(self, 1, 1, 3, 0, 0.9f);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x74:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 2, 2, 3, 0, 0.8f);
            }
            if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1)) {
                em018_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 1, 0, 3, 0, 1.5f);
            }
            if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1)) {
                em018_effect_spawn(self, 1, 6, 3, 0, 2.5f);
            }
        }
        break;
    case 0x75:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
            } else {
                em018_effect_spawn(self, 1, 2, 3, 0, 1.0f);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x76:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0xC, 8, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0xC, 0xD, 0, 1.0f);
            }
        } else {
            if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 2, 8, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 2, 0xD, 0, 1.0f);
            }
        }
        break;
    case 0x77:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x11, 0x2A, 0, 1.0f);
            }
        } else if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 1, 6, 0x2A, 0, 2.0f);
        }
        break;
    case 0x78:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x11, 0xF, 0, 0.7f);
            }
        } else if (em_after_frame_check(self, 0, 140.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 1, 0, 0xE, 0, 1.0f);
        }
        break;
    case 0x79:
    case 0x7B:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1C, 0xF, &vec, 1.0f);
        }
        break;
    case 0x7C:
        if ((system_w.field_0x0c & 3) == 0) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 0xF, &vec, 1.0f);
        }
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1)) {
            em018_effect_spawn(self, 0, 0x11, 0xF, 0, 0.8f);
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em018_effect_spawn(self, 0, 0x11, 0xF, 0, 1.0f);
            } else {
                em018_effect_spawn(self, 1, 4, 0xF, 0, 0.7f);
            }
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7E:
        if ((self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1)) {
                em018_effect_spawn(self, 2, 2, 0xF, 0, 0.6f);
            }
        } else if ((em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1)) {
            em018_effect_spawn(self, 1, 6, 0xF, 0, 1.7f);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7F:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 2, 2, 0xF, 0, 0.6f);
            }
        } else if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em018_effect_spawn(self, 1, 6, 0xF, 0, 1.7f);
        }
        break;
    case 0x80:
        if ((system_w.field_0x0c & 3) == 0) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x5E, 0xF, &vec, 1.0f);
        }
        if ((em_water_check_c1(self) == 1) && ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 82.0f, 0.0f) == 1))) {
            em018_effect_spawn(self, 0, 0x2B, 0xF, 0, 0.8f);
        }
        break;
    case 0x82:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1C, 3, &vec, 1.0f);
        }
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x11, 0xE, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 288.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x12, 8, 0, 0.6f);
            }
        } else {
            if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 1, 4, 0xE, 0, 0.7f);
            }
            if (em_after_frame_check(self, 0, 288.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 2, 8, 0, 1.2f);
            }
        }
        break;
    case 0xC9:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x42, 3, &vec, 1.0f);
        }
        break;
    case 0xCA:
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x42, 3, &vec, 1.0f);
        }
        break;
    case 0xCB:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x3D, 3, &vec, 1.0f);
        }
        break;
    case 0xCC:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            eft_em_spawn(self, 0x3A, 0xF, &vec, 1.2f);
        }
        break;
    case 0xCD:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 50.0f, 200.0f);
            eft_em_spawn(self, 0x3A, 0xF, &vec, 1.4f);
        }
        break;
    case 0xD0:
        if (((self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1)) {
            em018_effect_spawn(self, 2, 2, 0xF, 0, 0.5f);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0x43, 3, &vec, 1.0f);
        }
        break;
    case 0xD2:
        if ((fn_8019238C((EmActWork*)self) == 0) && ((em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1))) {
            setVector3(&vec, 0.0f, 10.0f, 0.0f);
            eft_em_spawn(self, 0x29, 0x1A, &vec, 1.0f);
        }
        break;
    case 0xD4:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1C, 3, &vec, 1.0f);
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 120.0f);
            eft_em_spawn(self, 0x1D, 0xF, &vec, 1.0f);
        }
        break;
    case 0xD6:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -75.0f, 100.0f);
            eft_em_spawn(self, 0x1F, 0xF, &vec, 1.0f);
        }
        break;
    case 0xD8:
        if ((fn_8019238C((EmActWork*)self) == 0) && ((em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1))) {
            setVector3(&vec, 0.0f, 10.0f, 0.0f);
            eft_em_spawn(self, 0x29, 0x1A, &vec, 1.0f);
        }
        break;
    case 0xDA:
        if (em_after_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x43, 3, &vec, 1.0f);
        }
        if (em_after_frame_check(self, 0, 212.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 250.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.3f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 268.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.1f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 316.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 1.1f, 0x3C);
        }
        if (em_after_frame_check(self, 0, 368.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 180.0f);
            eft_em_spawn_joint(self, 0xF, 0xD, &vec, 0.9f, 0x3C);
        }
        break;
    case 0xDB:
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x1C, 3, &vec, 1.0f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xDC:
        if ((self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 2, 2, 0xF, 0, 0.5f);
            }
            if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
                em018_effect_spawn(self, 0, 0x11, 0xF, 0, 1.0f);
            }
        } else if ((em_water_check_c1(self) == 0) && (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1)) {
            em018_effect_spawn(self, 1, 4, 0xF, 0, 0.8f);
        }
        break;
    case 0xDD:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x28, 3, &vec, 1.0f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xDF:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em_eft_fx_spawn_pair(self, 2, 0x2A, 0xF, 0x1E, 0.9f);
        }
        break;
    case 0xE2:
        if ((fn_8019238C((EmActWork*)self) == 0) && ((em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1))) {
            setVector3(&vec, 0.0f, 10.0f, 0.0f);
            eft_em_spawn(self, 0x29, 0x1A, &vec, 1.0f);
        }
        break;
    }
    if ((self->field_0x1E2 == 4) && ((system_w.field_0x0c & 0x3F) == 0)) {
        if ((self->field_0x228 & 2) != 0) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x38, 0xF, &vec, 1.0f);
        } else if (em_water_check_c1(self) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            eft_em_spawn(self, 0x37, 0xF, &vec, 1.0f);
        }
    }
    if ((em_alt_mode_ck(self) != 0) && (fn_801923CC((EmActWork*)self) == 0)) {
        if ((self->field_0x358 == 0) && (em_get_mot_no(self) != 0x1E) && (em_get_mot_no(self) != 0x1F)) {
            self->field_0x358 = 1;
        }
    } else if (self->field_0x358 != 0) {
        self->field_0x358 = 0;
    }
    em018_glow_fade(self);
}

/* 0x8019D204 (0x6B4): the per-frame tint: ramps the glint alpha and the ambient level with the aim flag, publishes
 * them to the model's TEV K-colours and ambient channels 4 and 5, pulses K-colour 3's alpha with the frame counter
 * and swings K-colour 1's grey level between the ranges the damage and mode flags pick. */
void em018_tev_color_update(struct _ENEMY_WORK* self) {
    EmFadeRec* rec = &((EmActWork*)self)->fade_0x328;
    _GXColor color;
    f32 ceiling;
    f32 step;

    if (fn_801923CC((EmActWork*)self) != 0) {
        if ((rec->glint_0x1C = 8.5f + rec->glint_0x1C) > 255.0f) {
            rec->glint_0x1C = 255.0f;
        }
    } else if ((rec->glint_0x1C -= 8.5f) < 0.0f) {
        rec->glint_0x1C = 0.0f;
    }
    if (rec->glint_0x1C > 0.0f) {
        ((MHchar*)self->char_0x024)->getTevKColor(4, GX_KCOLOR2, &color);
        color.a = (u8)rec->glint_0x1C;
        ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR2, &color);
        ((MHchar*)self->char_0x024)->setNumTevStages(4, 4);
    } else {
        ((MHchar*)self->char_0x024)->setNumTevStages(4, 3);
    }
    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR2, &color);
    if (em_flags836_ck(self, 1) == 1) {
        if (color.a != 0) {
            s16 a = color.a - 1;
            if (a < 0) {
                a = 0;
            }
            color.a = a;
        }
    } else {
        color.a = (u8)rec->glint_0x1C;
    }
    if (color.a != 0) {
        ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR2, &color);
        ((MHchar*)self->char_0x024)->setNumTevStages(5, 4);
    } else {
        ((MHchar*)self->char_0x024)->setNumTevStages(5, 3);
    }
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 9:
    case 11:
        ceiling = 160.0f;
        step = 1.1f;
        break;
    default:
        ceiling = 255.0f;
        step = 4.266667f;
        break;
    }
    if (fn_801923CC((EmActWork*)self) != 0) {
        if ((rec->ambient_0x20 += step) > ceiling) {
            rec->ambient_0x20 = ceiling;
            if (fn_80192370(self, 2) == 0) {
                fn_80192348((EmActWork*)self, 2);
            }
        }
    } else if ((rec->ambient_0x20 -= step) < 127.0f) {
        rec->ambient_0x20 = 127.0f;
    }
    if (((MHchar*)self->char_0x024)->getAmbColor(4, GX_COLOR0A0, &color) != 0) {
        color.r = color.g = color.b = (u8)rec->ambient_0x20;
        ((MHchar*)self->char_0x024)->setAmbColor(4, GX_COLOR0A0, color, false);
    }
    if (((MHchar*)self->char_0x024)->getAmbColor(5, GX_COLOR0A0, &color) != 0) {
        if (em_flags836_ck(self, 1) == 1) {
            if (color.r != 0) {
                s16 v = color.r - 1;
                if (v < 0) {
                    v = 0;
                }
                color.r = color.g = color.b = v;
            }
        } else {
            color.r = color.g = color.b = (u8)rec->ambient_0x20;
        }
        ((MHchar*)self->char_0x024)->setAmbColor(5, GX_COLOR0A0, color, false);
    }
    if (fn_80192370(self, 4) == 1) {
        fn_80192348((EmActWork*)self, 2);
        fn_80192358((EmActWork*)self, 4);
    }
    if (fn_801923CC((EmActWork*)self) == 0 && fn_80192370(self, 2) == 1) {
        fn_80192358((EmActWork*)self, 2);
    }
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    if (em_eft_active_ck(self) == 1) {
        color.a = 0;
    } else if (em_alt_mode_ck(self) == 1) {
        color.a = (s32)(30.0f * (0.5f * (1.0f + sin_idx_get((u16)(system_w.field_0x0c * 0x2000))))) + 0xE1;
    } else {
        color.a = (s32)(30.0f * (0.5f * (1.0f + sin_idx_get((u16)(system_w.field_0x0c * 0x2000))))) + 0x87;
    }
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    if (fn_8019238C((EmActWork*)self) == 1) {
        if (em_eft_active_ck(self) == 1) {
            if (rec->pulse_up_0x32 > 0) {
                if ((rec->pulse_0x24 += 8.333333f) >= 50.0f) {
                    rec->pulse_0x24 = 50.0f;
                    rec->pulse_up_0x32 = 0;
                }
            } else if ((rec->pulse_0x24 -= 8.333333f) <= 0.0f) {
                rec->pulse_0x24 = 0.0f;
                rec->pulse_up_0x32 = 1;
            }
        } else if (rec->pulse_up_0x32 > 0) {
            if ((rec->pulse_0x24 += 4.0f) >= 80.0f) {
                rec->pulse_0x24 = 80.0f;
                rec->pulse_up_0x32 = 0;
            }
        } else if ((rec->pulse_0x24 -= 4.0f) <= 0.0f) {
            rec->pulse_0x24 = 0.0f;
            rec->pulse_up_0x32 = 1;
        }
    } else if (em_eft_active_ck(self) == 1) {
        if (rec->pulse_up_0x32 > 0) {
            if ((rec->pulse_0x24 += 4.0f) >= 80.0f) {
                rec->pulse_0x24 = 80.0f;
                rec->pulse_up_0x32 = 0;
            }
        } else if ((rec->pulse_0x24 -= 4.0f) <= 0.0f) {
            rec->pulse_0x24 = 0.0f;
            rec->pulse_up_0x32 = 1;
        }
    } else if (rec->pulse_up_0x32 > 0) {
        if ((rec->pulse_0x24 += 3.9166667f) >= 255.0f) {
            rec->pulse_0x24 = 255.0f;
            rec->pulse_up_0x32 = 0;
        }
    } else if ((rec->pulse_0x24 -= 3.9166667f) <= 20.0f) {
        rec->pulse_0x24 = 20.0f;
        rec->pulse_up_0x32 = 1;
    }
    color.r = color.g = color.b = (u8)rec->pulse_0x24;
    color.a = 0;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, &color);
}

/* 0x8019D8B8 - the per-mode "this action may run" predicate the band's dispatchers gate on. */
u32 fn_8019D8B8(struct _ENEMY_WORK* self, u8 mode) {
    switch (mode) {
    case 0:
        if (em_water_check_c1(self) == 1) {
            return 1;
        }
        break;
    case 1:
        if ((self->field_0x35C & 1) == 0) {
            return 1;
        }
        break;
    case 2:
        if ((self->field_0x35C & 2) == 0) {
            return 1;
        }
        break;
    case 3:
        if ((self->field_0x228 & 3) != 0) {
            return 1;
        }
        break;
    case 4:
        if (self->vec_0x36C.z - self->pos.y >= lbl_80798288) {
            return 1;
        }
        break;
    case 5:
        if (fn_80192370((struct _ENEMY_WORK*)self, 16) == 0) {
            return 1;
        }
        break;
    case 6:
        if (self->field_0x360 <= 0) {
            return 1;
        }
        break;
    case 7:
        if (self->field_0x362 <= 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8019D9BC - the "load the action's joint position" init: arm mode 4, publish the two record
 * bytes and ask `em_data_point_get` for the area's joint, then run the band's common tail. */
void fn_8019D9BC(struct _ENEMY_WORK* self, u8* out_a, u8* out_b) {
    em_move_mode_set(self, 4);
    *out_a = 12;
    *out_b = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 3:
        switch (self->area_no) {
        case 3:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 6), &self->pos);
            break;
        case 8:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 9), &self->pos);
            break;
        }
        break;
    case 9:
    case 11:
        if (self->area_no == 1) {
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        }
        break;
    }
    fn_80192618((struct _ENEMY_WORK*)self);
}

/* 0x8019DAC0 - the area dispatch of the "already in mode" state: the map lookup picks the area
 * group, this area decides whether the state is armed. */
void fn_8019DAC0(struct _ENEMY_WORK* self) {
    u32 state = 0;
    if (stage_map_kind_get(self->field_0x1E0) == 3) {
        switch (self->area_no) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            if (self->field_0x9F6 == 2) {
                state = 1;
            }
            break;
        }
    }
    if (state == 1) {
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
    } else if (state == 2) {
        em_move_mode_set(self, 2);
        em_mot_set(self, 40, 0, 0);
    }
}

/* 0x8019DB9C - the "action 2 still running" gate: true only in mode 4 of the enemy-control state
 * machine while `em_alt_mode_ck` reports not-yet-armed. */
u32 fn_8019DB9C(struct _ENEMY_WORK* self) {
    if (self->field_0x1E2 == 4 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* 0x8019DBDC (0x2B4): the action selection of map kind 3: the part-break reply, the bound-aim request per area, the
 * table pick of action 0x17, the pack call when another live team-0x12 work shares the area, then the common
 * selectors. */
u32 em018_action_select(struct _ENEMY_WORK* self, u32 arg) {
    u8 kind = stage_map_kind_get(self->field_0x1E0);
    u32 hit;
    u32 probe;

    switch (kind) {
    case 3:
        break;
    default:
        return 0;
    }
    hit = 0;
    switch (kind) {
    case 3:
        probe = 6;
        break;
    default:
        probe = 0xFF;
        break;
    }
    if (probe != 0xFF) {
        switch (em_area_break_ck_c2(self, probe)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0) {
        if (fn_80192370(self, 0x10) == 0) {
            if (em_eft_active_ck(self) == 1 && (self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1)) {
                u32 area = 0xFF;
                switch (kind) {
                case 3:
                    switch (self->area_no) {
                    case 2:
                        area = 1;
                        break;
                    case 4:
                        area = 3;
                        break;
                    case 6:
                        area = 8;
                        break;
                    case 8:
                        area = 4;
                        break;
                    }
                    break;
                }
                if (area != 0xFF) {
                    self->field_0x1FC = 1;
                    self->field_0x1FE = 8;
                    self->field_0x1FF = area;
                    return 1;
                }
            }
        } else {
            switch (kind) {
            case 3:
                probe = 3;
                break;
            default:
                probe = 0xFF;
                break;
            }
            if (em_pack_area_pick(self, 0x17, (u8)probe, (u16)arg, em018_pick_a, em018_pick_b) == 1) {
                return 1;
            }
        }
    }
    if (hit == 0 && self->value_0x452 >= 900) {
        u32 found = 0;
        struct _ENEMY_WORK* work = (struct _ENEMY_WORK*)get_move_work_adrs(3);
        u16 max = get_move_work_max(3);
        u16 i;
        for (i = 0; i < max; i++, work++) {
            if (self->active != 0 && self->group != i && em_work_die_ck(work) != 1 && work->team == 0x12 &&
                work->area_no == self->area_no) {
                found = 1;
                break;
            }
        }
        if (found == 1 && em_roll_cooldown_ck_c1(self, (u16)arg, 0x32, (s16*)&self->tev_0x35E) == 1) {
            self->field_0x1FC = 1;
            self->field_0x1FE = 2;
            self->field_0x1FF = 0xFF;
            return 1;
        }
    }
    if (em_area_roam_pick(self, (u16)arg) == 1) {
        return 1;
    }
    return (em_area_leave_ck(self) - 1) == 0;
}

} /* extern "C" */

/* 0x8019DE90 (0x8): Sets the accessor's flag word to 1 once it is installed. */
void Em018UserData::Attach() {
    em_res_user_data_flag_set((struct _ENEMY_WORK*)this, 1);
}

/* 0x8019DE98 (0x22C): For an admitted item, scales the joint node's glow (kinds 27/28, clamped), lifts its
 * translation by 25 while aim bit 0x01 is set (kind 24) and rescales the height by the two fades (kinds 24/26). */
void Em018UserData::CalcAnmItemEx(nw4r::g3d::ChrAnmResult* holder, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 kind, UserDataItem* item) {
    VEC3 vec;
    struct _ENEMY_WORK* work;
    EmFadeRec* rec;

    VEC3_ctor(&vec);
    work = this->work;
    rec = &((EmActWork*)work)->fade_0x328;
    switch (item->field_0x04) {
    case 0xFF:
        switch (kind) {
        case 27:
            anm_chr_result_scale_get_c1(holder, &vec);
            vec.x *= rec->glow_0x00.x;
            vec.y *= rec->glow_0x00.y;
            vec.z *= rec->glow_0x00.z;
            if (vec.x > 2.0f) {
                vec.x = 2.0f;
            }
            if (vec.y > 17.0f) {
                vec.y = 17.0f;
            }
            if (vec.z > 1.0f) {
                vec.z = 1.0f;
            }
            anm_chr_result_scale_set_c1(holder, &vec);
            break;
        case 28:
            anm_chr_result_scale_get_c1(holder, &vec);
            vec.x *= rec->glow_0x10.x;
            vec.y *= rec->glow_0x10.y;
            vec.z *= rec->glow_0x10.z;
            if (vec.x > 3.0f) {
                vec.x = 3.0f;
            }
            if (vec.y > 1.8f) {
                vec.y = 1.8f;
            }
            if (vec.z > 3.8f) {
                vec.z = 3.8f;
            }
            anm_chr_result_scale_set_c1(holder, &vec);
            break;
        case 24:
            anm_chr_result_trans_get_c1(holder, &vec);
            if (fn_80192370(work, 1) == 1) {
                vec.y -= 25.0f;
            }
            anm_chr_result_trans_set_c1(holder, &vec);
            anm_chr_result_scale_get_c1(holder, &vec);
            vec.y *= rec->fade_0x28;
            anm_chr_result_scale_set_c1(holder, &vec);
            break;
        case 26:
            anm_chr_result_scale_get_c1(holder, &vec);
            vec.y /= rec->fade_0x28;
            vec.y *= rec->fade_0x2C;
            anm_chr_result_scale_set_c1(holder, &vec);
            break;
        }
        break;
    }
}

extern "C" {

/* 0x8019E0C4 (0x2D4): steps the two glow triples by the fade state and fades the dim pair in or out. */
void em018_glow_fade(struct _ENEMY_WORK* self) {
    EmFadeRec* rec = &((EmActWork*)self)->fade_0x328;

    switch (rec->state_0x30) {
    case 1:
        if ((rec->glow_0x00.x += 0.5f) > 2.0f) {
            rec->glow_0x00.x = 2.0f;
        }
        if ((rec->glow_0x00.y += 8.0f) > 17.0f) {
            rec->glow_0x00.y = 17.0f;
        }
        if (rec->glow_0x00.z > 1.0f) {
            rec->glow_0x00.z = 1.0f;
        }
        rec->state_0x30++;
        break;
    case 2:
        if ((rec->glow_0x00.x += 0.5f) > 2.0f) {
            rec->glow_0x00.x = 2.0f;
        }
        if ((rec->glow_0x00.y += 8.0f) > 17.0f) {
            rec->glow_0x00.y = 17.0f;
        }
        if (rec->glow_0x00.z > 1.0f) {
            rec->glow_0x00.z = 1.0f;
        }
        if ((rec->glow_0x10.x += 0.4333333f) > 2.3f) {
            rec->glow_0x10.x = 2.3f;
        }
        if ((rec->glow_0x10.y += 0.13333333f) > 1.4f) {
            rec->glow_0x10.y = 1.4f;
        }
        if ((rec->glow_0x10.z += 0.59999996f) > 2.8f) {
            rec->glow_0x10.z = 2.8f;
        }
        break;
    case 0:
        if ((rec->glow_0x00.x -= 0.033333335f) < 1.0f) {
            rec->glow_0x00.x = 1.0f;
        }
        if ((rec->glow_0x00.y -= 0.53333336f) < 1.0f) {
            rec->glow_0x00.y = 1.0f;
        }
        if (rec->glow_0x00.z < 1.0f) {
            rec->glow_0x00.z = 1.0f;
        }
        if ((rec->glow_0x10.x -= 0.043333333f) < 1.0f) {
            rec->glow_0x10.x = 1.0f;
        }
        if ((rec->glow_0x10.y -= 0.013333333f) < 1.0f) {
            rec->glow_0x10.y = 1.0f;
        }
        if ((rec->glow_0x10.z -= 0.06f) < 1.0f) {
            rec->glow_0x10.z = 1.0f;
        }
        break;
    }
    if (rec->dim_0x31 != 0) {
        if ((rec->fade_0x28 -= 0.075f) < 0.0001f) {
            rec->fade_0x28 = 0.0001f;
            fn_80192348((EmActWork*)self, 1);
        }
        if ((rec->fade_0x2C -= 0.05f) < 0.5f) {
            rec->fade_0x2C = 0.5f;
        }
    } else {
        if ((rec->fade_0x28 += 0.075f) > 1.5f) {
            rec->fade_0x28 = 1.5f;
        }
        if ((rec->fade_0x2C += 0.05f) > 1.0f) {
            rec->fade_0x2C = 1.0f;
        }
        if (fn_80192370(self, 1) == 1) {
            fn_80192358((EmActWork*)self, 1);
        }
    }
}
/* 0x8019E398 - the two +0x35C completion flags: each is set while its action pair matches and
 * cleared once the enemy has moved on to another action. */
void fn_8019E398(struct _ENEMY_WORK* self) {
    if (em_act_ck(self, 6, 0) == 1 || em_act_ck(self, 6, 1) == 1) {
        if ((self->field_0x35C & 1) == 0) {
            self->field_0x35C |= 1;
        }
    } else if (self->action != 0 && (self->field_0x35C & 1) != 0) {
        self->field_0x35C &= ~1;
    }
    if (em_act_ck(self, 5, 17) == 1 || em_act_ck(self, 5, 18) == 1) {
        if ((self->field_0x35C & 2) == 0) {
            self->field_0x35C |= 2;
        }
    } else if (self->action != 0 && (self->field_0x35C & 2) != 0) {
        self->field_0x35C &= ~2;
    }
}

/* 0x8019E49C - "is the record we are hunting within reach": true when the paired record exists and
 * the xz distance is inside the scaled radius (or when the caller asked for the trivial answer). */
u32 fn_8019E49C(struct _ENEMY_WORK* self, u32 flag) {
    struct _ENEMY_WORK* other = em_ally_find(self, 23, 0);
    if (other != 0 && em_act11_wait_ck(other) == 1) {
        if ((u8)flag == 0) {
            return 1;
        }
        f32 dist = calcDistanceSqXZ(&self->pos, &other->pos);
        f32 range = get_em_chg_scale(self) * lbl_80798510;
        if (dist < range * (get_em_chg_scale(self) * lbl_80798510)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8019E580 - "action 13 in its first five sub-states". */
u32 em018_act13_early_ck(struct _ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

}

/* 0x8019E5A8 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em018UserData::~Em018UserData() {
}

/* The unit's `.bss`: the two-vector record `em018_static_init` seeds.  The name is a GUESS (a pair of model-space
 * points). */
VEC3 vec_pair_801926EC_0[2];  /* +0x806A7A70 */

/* 0x8019E604 (0x6C): the static initializer: seeds the two vectors of the pair. */
extern "C" void em018_static_init(void) {
    VEC3 lo;
    VEC3 hi;

    assignVec3_c1(&vec_pair_801926EC_0[0], setVec3(&hi, 0.0f, 54.0f, 0.0f));
    assignVec3_c1(&vec_pair_801926EC_0[1], setVec3(&lo, 0.0f, -2.4f, 0.0f));
}
