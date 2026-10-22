/*
 * enemy/em008_prog.cpp - enemy 008's program: the `em008_prog_tbl` entry points (init, frame update, action run,
 *   motion/event hooks, state queries, part breaks, the area-start check), the per-action dispatchers and their motion
 *   steps, the sound programs of actions 10 and 11, the arena demo, the per-motion event driver, the user-data record
 *   methods and the static initialiser.
 * RANGE. .text 0x8015D860-0x801663E4 (88 functions); extab 0x8000DFA4-0x8000E1D4, extabindex
 *   0x80028E60-0x800291A8, .ctors 0x8056F328-0x8056F32C (`em008_static_init`), .rodata 0x8056FB50-0x8056FC10 (the
 *   three turn tables), .data 0x805A5D48-0x805A6D58 (`em008_prog_tbl` first), .bss 0x806A7868-0x806A7898
 *   (`em008_se_shift_a`/`_b`), .sdata2 0x80797330-0x807974F0.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs,
 *   playbook 39), `#pragma fp_contract off` (no fused multiply-add in the target) and `#pragma pool_data off` (each
 *   sound table is addressed with its own `lis`/`addi`).
 * NAMES. The file name follows the runtime dump's `em008_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the program-table slot each fills): em008_timers_clear, em008_init, em008_frame_update, em008_action_run,
 *   em008_motion_next, em008_event_notify, em008_motion_events, em008_tev_color_update, em008_state_query,
 *   em008_action_ok_ck, em008_death_action_get, em008_aim_reset, em008_alt_part_ck, em008_part_break_ck,
 *   em008_part_break_set, em008_area_start_ck.
 *   GUESS (dispatcher slot): em008_act<action>_<first sub-state that runs it>, em008_act<N>_dispatch,
 *   em008_act2_approach, em008_act10_se_play, em008_act11_se_play; GUESS (from the bodies): em008_part_flags_seed,
 *   em008_part_flag_clear_ck, em008_part_pair_ck, em008_part_flags_count, em008_finish_act6/_act7 (the action they
 *   arm), em008_spawn_rec_set, em008_part_rec_update, em008_alt_mode_fx, em008_effect_spawn, em008_flag491_update,
 *   em008_static_init.
 *   GUESS (data): em008_turn_seq_wait/_back/_side, em008_area_pick_tbl, em008_se_seq_<sub-state> and
 *   em008_se_alt_seq_<sub-state> (the sub-state that first plays each table) with their em008_se_rec/pos records,
 *   em008_se_shift_a/_b, em008_ctor; the `Em008ProgTbl` slot names; `Em008UserData`.
 *   `em_flag_0x8AD_ck` is called through the `_c1` view: its owner's header declares it `void`.
 * RESIDUALS. `em008_area_start_ck`: retail colours self/arg/seat/entry r28/r29/r30/r31, ours r31/r28/r29/r30 (the
 *   declaration order, an if/else or switch select, a u16 argument and dead call results all leave it).
 *   flipcheck: the `em008_area_start_ck` registers.
 * SHAPES. `Em008UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 *   A call written in place as the right operand of `*` is evaluated first (`x * base * get_em_chg_scale`).
 *   The degree-to-angle turn is one expression (`(u16)(s32)(0.5f + (65536.0f * (deg * scale)) / 360.0f)`); split
 *   into temporaries it moves the `mr r3`.  A `u8` predicate result that retail truncates is written `(u8)(...) &
 *   0xFF` in a `u32` function (`em008_state_query` case 4).  A case-level `return` that retail lays out as a `break`
 *   costs a branch; the event and dispatcher switches use `break`.  The data is defined between the functions in
 *   retail order (the act-10 tables before `em008_act10_se_play`, the act-11 tables before `em008_act11_se_play`).
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "types.h"
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
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
#include "MSL/new.h" /* the placement `operator new` */
#include "enemy/EnemyData.h"
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
#define em_res_user_data_flag_set fn_8013A654_hidden_fn_80165FC8_h
#define rotMatrixX rotMatrixX_hidden_fn_80165FC8_h
#define rotMatrixZ rotMatrixZ_hidden_fn_80165FC8_h
#define ResUserDataAc ResUserDataAcBandView
#include "enemy/fn_80165FC8.h"
#undef ResUserDataAc
#undef rotMatrixZ
#undef rotMatrixX
#undef em_res_user_data_flag_set
#undef em_mot_set
#undef em_frame_flag_set
#undef em_fall_height_get
#undef em_act_ck
#undef assignVec3
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */
#include "stage/stg_w.h"
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr and its slots */
#include "enemy/em_se_record.h"  /* EmSeEntry */
#include "enemy/em_se_tbl_play.h" /* em_se_tbl_play */
#include "ef/eft009.h"          /* eft009_spawn_at_joint / eft009_set_pos / eft009_spawn_at_pos */
#include "enemy/em_move_vec2_clr.h" /* em_move_vec2_clr */
#include "enemy/em_turn_seq_start.h" /* em_turn_seq_start */
#include "enemy/em_turn_seq_step.h" /* em_turn_seq_step */
#include "enemy/CancelFade.h" /* CancelFade */
#include "ef/fn_8010D1A8.h"     /* eft_spawn_pos_in_area */
#include "ef/cp_vector.h"       /* _CP_VECTOR */
#include "enemy/em_camera_req.h" /* em_camera_req */
/* Call-site views: each macro casts a callee to the signature its call sites use (the same direct call). */
#define rotMatrixZ_view1 ((void (*)(u32, nw4r::math::MTX34*))rotMatrixZ)
#define rotMatrixX_view1 ((void (*)(u32, nw4r::math::MTX34*))rotMatrixX)
#define fn_8013A654_view1 ((void (*)(ResUserDataAc*, u32))em_res_user_data_flag_set)
#define em_mot_finished_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_mot_finished_ck)
#define em_parts_damage_level_get_view1 ((u8 (*)(_ENEMY_WORK*, u8))em_parts_damage_level_get)
#define em_ground_ck_f ((u32 (*)(_ENEMY_WORK*, f32))em_ground_ck)
#define assignVec3_view1 ((void (*)(nw4r::math::VEC3*, const nw4r::math::VEC3*))assignVec3)

/* The user-data accessor `em008_init` installs: the base accessor with its own destructor, attach hook and
 * end-of-tree hook (its table is `.data` 0x805A6D28).  size: 0xC (`operator new(0xC)`) */
class Em008UserData : public ResUserDataAc {
public:
    Em008UserData();
    virtual ~Em008UserData();
    virtual void Attach();
    virtual void CalcWorldEnd(nw4r::math::MTX34* pMtxArray, nw4r::g3d::ResMdl mdl,
                              nw4r::g3d::FuncObjCalcWorld* pFuncObj);
};

/* The slot +0x3C job spelled without its trailing table argument (the call sites pass none). */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

/* The C++ free functions, declared by their real signatures so the front-end mangles them to the map spellings
 * (rule 9: the mangled spelling is never written). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_after_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wpos_em(_ENEMY_WORK* self, u32 joint, Vec3* out);
u32 em_parts_damage_level_get(_ENEMY_WORK*, u8);
u16 em_get_mot_no(_ENEMY_WORK* self);
f32 get_em_chg_scale(_ENEMY_WORK* self);
s32 calcVecAng2(VEC3* a, VEC3* b);
void rotVecY(VEC3* v, u32 angle);

/* Callees whose owners' headers do not declare them: `lobby/fn_8030121C.cpp`'s effect spawns `eft_em_spawn_tinted`
 * and `eft_em_spawn`, and `enemy/em_model.cpp`'s `em_drop_item_spawn`. */
extern "C" void eft_em_spawn_tinted(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s, s32 id);
extern "C" void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);
extern "C" void em_drop_item_spawn(_ENEMY_WORK* self, void* pos, s32 value);

/* `em_flag_0x8AD_ck` is declared `void` by its owner's header; this unit's call site reads its u32 answer. */
#define em_flag_0x8AD_ck_c1 ((u32 (*)(_ENEMY_WORK*))em_flag_0x8AD_ck)

/* This unit's functions, declared up front (the program table and the dispatchers name them first). */
extern "C" {
void em008_part_flags_seed(_ENEMY_WORK* self);
u32 em008_part_flag_clear_ck(_ENEMY_WORK* self, u8 mask);
u32 em008_part_pair_ck(_ENEMY_WORK* self);
u8 em008_part_flags_count(_ENEMY_WORK* self);
void em008_timers_clear(_ENEMY_WORK* self);
void em008_init(_ENEMY_WORK* self, u32 arg);
void em008_motion_next(_ENEMY_WORK* self, u8* state, u8* sub);
void em008_event_notify(_ENEMY_WORK* self, u8 arg, u8 sub);
void em008_frame_update(_ENEMY_WORK* self);
void em008_finish_act7(_ENEMY_WORK* self);
void em008_finish_act6(_ENEMY_WORK* self);
void em008_act0_0(_ENEMY_WORK* self);
void em008_act0_1(_ENEMY_WORK* self);
void em008_act0_2(_ENEMY_WORK* self);
void em008_act0_7(_ENEMY_WORK* self);
void em008_act0_dispatch(_ENEMY_WORK* self);
void em008_act1_0(_ENEMY_WORK* self);
void em008_act1_1(_ENEMY_WORK* self);
void em008_act1_2(_ENEMY_WORK* self);
void em008_act1_3(_ENEMY_WORK* self);
void em008_act1_4(_ENEMY_WORK* self);
void em008_act1_5(_ENEMY_WORK* self);
void em008_act1_6(_ENEMY_WORK* self);
void em008_act1_7(_ENEMY_WORK* self);
void em008_act1_8(_ENEMY_WORK* self);
void em008_act1_9(_ENEMY_WORK* self);
void em008_act1_dispatch(_ENEMY_WORK* self);
void em008_act2_approach(_ENEMY_WORK* self, u8 kind, u8 mode);
void em008_act2_6(_ENEMY_WORK* self);
void em008_act2_7(_ENEMY_WORK* self);
void em008_act2_8(_ENEMY_WORK* self);
void em008_act2_9(_ENEMY_WORK* self);
void em008_act2_10(_ENEMY_WORK* self);
void em008_act2_11(_ENEMY_WORK* self);
void em008_act2_20(_ENEMY_WORK* self);
void em008_act2_21(_ENEMY_WORK* self);
void em008_act2_dispatch(_ENEMY_WORK* self);
void em008_act6_0(_ENEMY_WORK* self);
void em008_act6_1(_ENEMY_WORK* self);
void em008_act6_2(_ENEMY_WORK* self);
void em008_act6_3(_ENEMY_WORK* self, u8 arg1);
void em008_act6_4(_ENEMY_WORK* self);
void em008_act6_5(_ENEMY_WORK* self, u8 arg1);
void em008_act6_7(_ENEMY_WORK* self);
void em008_act6_8(_ENEMY_WORK* self);
void em008_act6_10(_ENEMY_WORK* self);
void em008_act6_12(_ENEMY_WORK* self);
void em008_act6_13(_ENEMY_WORK* self);
void em008_act6_dispatch(_ENEMY_WORK* self);
void em008_spawn_rec_set(EmSpawnRec* out, u8 a, u32 id, VEC3* pos, u16 b, u16 c);
void em008_act7_0(_ENEMY_WORK* self, u8 arg1);
void em008_act7_1(_ENEMY_WORK* self);
void em008_act7_2(_ENEMY_WORK* self, u8 arg1);
void em008_act7_5(_ENEMY_WORK* self);
void em008_act7_6(_ENEMY_WORK* self, u8 arg1);
void em008_act7_7(_ENEMY_WORK* self, u8 arg1);
void em008_act7_8(_ENEMY_WORK* self);
void em008_act7_9(_ENEMY_WORK* self, u8 arg1);
void em008_act7_11(_ENEMY_WORK* self, u8 arg1);
void em008_act7_18(_ENEMY_WORK* self);
void em008_act7_dispatch(_ENEMY_WORK* self);
void em008_act10_se_play(_ENEMY_WORK* self);
void em008_act11_se_play(_ENEMY_WORK* self);
void em008_act12_0(_ENEMY_WORK* self);
void em008_act12_dispatch(_ENEMY_WORK* self);
void em008_act13_0(_ENEMY_WORK* self);
void em008_act13_1(_ENEMY_WORK* self);
void em008_act13_dispatch(_ENEMY_WORK* self);
void em008_action_run(_ENEMY_WORK* self);
void em008_part_rec_update(_ENEMY_WORK* self);
void em008_alt_mode_fx(_ENEMY_WORK* self);
void em008_effect_spawn(_ENEMY_WORK* self, u8 arg1, u8 arg2, u32 arg3, s32 arg4, f32 scale);
void em008_flag491_update(_ENEMY_WORK* self);
void em008_motion_events(_ENEMY_WORK* self);
void em008_tev_color_update(_ENEMY_WORK* self);
u32 em008_state_query(_ENEMY_WORK* self, u8 kind);
s32 em008_action_ok_ck(_ENEMY_WORK* self);
void em008_death_action_get(_ENEMY_WORK* self, u8* out_state, u8* out_flags);
void em008_aim_reset(_ENEMY_WORK* self);
s32 em008_alt_part_ck(_ENEMY_WORK* self, u8 arg1);
s32 em008_part_break_ck(_ENEMY_WORK* self, u8 part);
void em008_part_break_set(_ENEMY_WORK* self, u8 part);
u32 em008_area_start_ck(_ENEMY_WORK* self, u32 arg1);
void em008_static_init(void);
}

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* The em008 program table (`em008_prog_tbl`): the entry points the shared enemy program driver calls (slot names
 * follow `Em024ProgTbl`'s scheme, a GUESS from the functions they hold).
 * size: 0x6C */
struct Em008ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u32 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(_ENEMY_WORK* self, u8* state, u8* sub);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(_ENEMY_WORK* self);
    /* +0x24 */ u32 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ s32 (*action_ok_ck_0x28)(_ENEMY_WORK* self);
    /* +0x2C */ u32 pad_0x2C;
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*aim_reset_0x34)(_ENEMY_WORK* self);
    /* +0x38 */ s32 (*alt_part_ck_0x38)(_ENEMY_WORK* self, u8 part);
    /* +0x3C */ u32 pad_0x3C[2];
    /* +0x44 */ s32 (*part_break_ck_0x44)(_ENEMY_WORK* self, u8 part);
    /* +0x48 */ u32 pad_0x48[2];
    /* +0x50 */ void (*part_break_set_0x50)(_ENEMY_WORK* self, u8 part);
    /* +0x54 */ u32 (*area_start_ck_0x54)(_ENEMY_WORK* self, u32 arg);
    /* +0x58 */ u32 pad_0x58[5];
};

extern "C" Em008ProgTbl em008_prog_tbl = {
    em008_timers_clear,
    em008_init,
    em008_frame_update,
    em008_action_run,
    em008_motion_next,
    em008_event_notify,
    em008_motion_events,
    0,
    em008_tev_color_update,
    em008_state_query,
    em008_action_ok_ck,
    0,
    em008_death_action_get,
    em008_aim_reset,
    em008_alt_part_ck,
    {0, 0},
    em008_part_break_ck,
    {0, 0},
    em008_part_break_set,
    em008_area_start_ck,
    {0, 0, 0, 0, 0},
};

/* The record `em008_area_start_ck` hands `em_pack_area_pick` (word-sized fields; their meaning is a GUESS: a frame
 * count, a packed id pair and an 0xFFFF terminator). */
u32 em008_area_pick_tbl[3] = {0x19, 0x00C8000C, 0xFFFF0000};

/* The `em_turn_seq_start`/`em_turn_seq_step` tables (`.rodata`, 0x40 B each): a four-word head (the frame count,
 * the turn span and the packed rate), then three four-word rows whose first word carries the motion in its high
 * half.  The names are a GUESS from their users. */
extern "C" const u32 em008_turn_seq_wait[16] = {
    0x00000006, 0x00000020, 0x00000000, 0x40002000,
    0x00200000, 0x0000000A, 0x00000000, 0x00000000,
    0x00080000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em008_turn_seq_back[16] = {
    0x00000000, 0x00000032, 0x00000000, 0x80000000,
    0x00200000, 0x0000000A, 0x00000000, 0x00000000,
    0x000D0000, 0x00000004, 0x00000000, 0x00000000,
    0x000C0000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em008_turn_seq_side[16] = {
    0x00000000, 0x0000000E, 0x00000000, 0x55550000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00060000, 0x0000000A, 0x00000000, 0x00000000,
    0x00050000, 0x0000000A, 0x00000000, 0x00000000,
};

/* The two two-vector shift records `em008_static_init` seeds and the action-10 sound records point at (`.bss`). */
extern "C" nw4r::math::VEC3 em008_se_shift_a[2];
extern "C" nw4r::math::VEC3 em008_se_shift_b[2];

/* 0x8015D860 - seeds the action's flag byte: bit 2 for two damage levels on part 2, bit 0 and bit 5 for the
 * mask-2 and mask-1 probes. */
void em008_part_flags_seed(_ENEMY_WORK* self) {
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
u32 em008_part_flag_clear_ck(_ENEMY_WORK* self, u8 mask) {
    return (self->field_0x1E4 & mask) == 0;
}

/* 0x8015D908 - the two-bit gate: 0 only when bit 0 and bit 1 are both set. */
u32 em008_part_pair_ck(_ENEMY_WORK* self) {
    if ((self->field_0x1E4 & 1) == 0 || (self->field_0x1E4 & 2) == 0) {
        return 1;
    }
    return 0;
}

/* 0x8015D934 - counts the clear bits of the low six (bit 0 .. bit 5). */
u8 em008_part_flags_count(_ENEMY_WORK* self) {
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

/* 0x8015D9B8 - clears the two +0x328/+0x32A countdowns. */
void em008_timers_clear(_ENEMY_WORK* self) {
    self->field_0x328 = 0;
    self->timer_0x32A = 0;
}

/* 0x8015D9C8 - the per-tick entry: hand over the motion on `arg == 2`, re-seed the flag byte, attach the
 * helper when there is none, and spawn the position effect outside the +0x009 gate. */
void em008_init(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Em008UserData* helper;

    VEC3_ctor(&v);
    switch ((u8)arg) {
      case 2:
        em_move_mode_set(self, 4);
        em_act_arm_unless_down(self, 6, 5);
        em_state_refresh(self);
        break;
    }
    em008_part_flags_seed(self);
    if (em_res_user_data_ck(self) == 0) {
        helper = (Em008UserData*)operator new(sizeof(Em008UserData));
        new (helper) Em008UserData;
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, -20.0f, 100.0f);
        eft_em_attach_set(self, 0x14, &v, 1.0f, 0);
    }
}

/* 0x8015DAA8 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em008UserData::Em008UserData() {
}

/* 0x8015DAE4 - one action pair's (state, sub-state) transitions: in state 7, sub-state 7/0xD goes to 0x10
 * when `fn_8012EC3C` answers and 0xB to 0x11 while flag bit 0 is clear. */
void em008_motion_next(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (*state) {
      case 7:
        switch (*sub) {
          case 7:
          case 0xD:
            if (em_eft_active_ck(self) == 1) {
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

/* 0x8015DB68 - the second transition table: action 1 arms the +0x32A countdown, the joint effect and the
 * `em_act_flag1_set` hand-over; action 0xA arms flag bits 0/5 and +0x491, each with an effect. */
void em008_event_notify(_ENEMY_WORK* self, u8 arg, u8 sub) {
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
            em_act_flag1_set(self);
            break;
        }
        break;
      case 0xA:
        switch (sub) {
          case 0xC3:
            em_part_hit_set(self, 1, 0);
            if (em008_part_flag_clear_ck(self, 1) == 1) {
                self->field_0x1E4 |= 1;
                setVector3(&v1, 0.0f, 50.0f, 150.0f);
                eft_em_spawn_tinted(self, 1, 0x14, &v1, 1.0f, -1);
                em_drop_item_spawn(self, &self->pos, 0x1E);
            }
            break;
          case 0xC8:
            em_part_hit_set(self, 0, 0);
            self->field_0x491 = 1;
            if (em008_part_flag_clear_ck(self, 0x20) == 1) {
                self->field_0x1E4 |= 0x20;
                setVector3(&v1, 0.0f, 0.0f, 0.0f);
                eft_em_spawn_tinted(self, 1, 0x2D, &v1, 1.0f, -1);
                em_drop_item_spawn(self, &self->pos, 0x1E);
            }
            break;
        }
        break;
    }
}

/* 0x8015DD6C - the per-tick bookkeeping: mirror "state 4" into the +0x38B flag and run the two
 * +0x328/+0x32A countdowns down while they are positive. */
void em008_frame_update(_ENEMY_WORK* self) {
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
void em008_finish_act7(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    em_act_arm_unless_act11(self, 7, 0x12);
    em_finish_flag_set(self);
}

/* 0x8015DE00 - the state-0 step: motion 4, then motion set (6, 0xC). */
void em008_finish_act6(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    em_act_arm_unless_act11(self, 6, 0xC);
    em_finish_flag_set(self);
}

/* 0x8015DE48 - the two-step state: start motion set (1, 0xA) on entry, finish on the wait. */
void em008_act0_0(_ENEMY_WORK* self) {
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
void em008_act0_1(_ENEMY_WORK* self) {
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
void em008_act0_2(_ENEMY_WORK* self) {
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
 * height pair on entry, `em_action_finish_stand` on the wait. */
void em008_act0_7(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* 0x8015E05C - the sparse sub-state dispatcher: the compare chain for {0,1,2,3,4,6,7}. */
void em008_act0_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        em008_act0_0(self);
        break;
      case 1:
        em008_act0_1(self);
        break;
      case 2:
        em008_act0_2(self);
        break;
      case 3:
        em008_act0_0(self);
        break;
      case 4:
        em008_act0_0(self);
        break;
      case 6:
        em008_act0_0(self);
        break;
      case 7:
        em008_act0_7(self);
        break;
    }
}

/* 0x8015E0BC - the two-step state with motion set (0xF, 6); the local vector is zeroed but not
 * used by either arm (retail does the same call). */
void em008_act1_0(_ENEMY_WORK* self) {
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
void em008_act1_1(_ENEMY_WORK* self) {
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

/* 0x8015E1C4 - the long step: motion set (0xF, 6), then three frame windows for the part effect and the
 * ground effect at (0, -50, 100), the last gated on `system_w`'s +0x0C low bits. */
void em008_act1_2(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
      case 1:
        if (em_frame_check(self, 0, 160.0f, 0.0f) == 1) {
            em_hit_window_set(self, 1, 0x12, 5);
            draw_shape_arm((u32)self, 0x14, 0xA);
        }
        if (em_frame_check(self, 0, 160.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 0, 0x13, &v, 1.0f);
        }
        if (em_frame_check(self, 3, 164.0f, 264.0f) == 1) {
            if ((system_w.field_0x0c & 7) == 0) {
                setVector3(&v, 0.0f, -50.0f, 100.0f);
                eft_em_spawn(self, 1, 0x13, &v, 1.0f);
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
void em008_act1_3(_ENEMY_WORK* self) {
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
            em_tint_hold_start(self);
        }
        break;
      case 2:
        em_gauge_tick(self, 0.6f, 1, 0xA);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            em_tint_hold_end(self);
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
 * `em_stamina_add` arm on entry; the 120-frame window hands over to `em_state_set`. */
void em008_act1_4(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 8, 0);
        self->timer_0x020 = 3;
        em_stamina_add(self, 0x3E8);
        break;
      case 1:
        if (em_frame_check(self, 1, 120.0f, 0.0f) == 1) {
            em_state_set(self, 1, 8);
        }
        break;
    }
}

/* 0x8015E4F8 - the two-step state with motion set (0xCA, 2). */
void em008_act1_5(_ENEMY_WORK* self) {
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

/* 0x8015E568 - the two-step state that counts two motion waits: motion set (0x24, 6) and the hit window,
 * then (1, 7) once the +0x20 counter reaches 2. */
void em008_act1_6(_ENEMY_WORK* self) {
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
void em008_act1_7(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 8, 0);
        em008_part_flags_seed(self);
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
void em008_act1_8(_ENEMY_WORK* self) {
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
void em008_act1_9(_ENEMY_WORK* self) {
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
void em008_act1_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        em008_act1_0(self);
        break;
      case 1:
        em008_act1_1(self);
        break;
      case 2:
        em008_act1_2(self);
        break;
      case 3:
        em008_act1_3(self);
        break;
      case 4:
        em008_act1_4(self);
        break;
      case 5:
        em008_act1_5(self);
        break;
      case 6:
        em008_act1_6(self);
        break;
      case 7:
        em008_act1_7(self);
        break;
      case 8:
        em008_act1_8(self);
        break;
      case 9:
        em008_act1_9(self);
        break;
    }
}

/* 0x8015E854 (0x1D0): the approach: motion (0xA, 8) and an approach distance `kind` picks, then either motion
 * (0xB, 4) and its end (`mode` 0) or the finish (`mode` 1) once the approach arrives. */
void em008_act2_approach(_ENEMY_WORK* self, u8 kind, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 8, 0);
        switch (kind) {
        case 0:
            em_approach_start(self, -500.0f, 0);
            break;
        case 1:
            em_approach_start(self, -500.0f, 0);
            break;
        case 2:
            em_approach_start(self, -1000.0f, 0);
            break;
        case 3:
            em_approach_start(self, 0.0f, 0);
            break;
        case 4:
            em_approach_start(self, -400.0f, 0);
            break;
        case 5:
            em_approach_start(self, -1200.0f, 0);
            break;
        case 6:
            em_approach_start(self, -1000.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1 && em_mot_frame_started_ck(self) == 0) {
            switch (mode) {
            case 0:
                self->state++;
                em_mot_set(self, 0xB, 4, 0);
                break;
            case 1:
                em_action_finish(self);
                break;
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

/* 0x8015EA24 (0xA4): the turn on the spot: the wait turn sequence (with the slower speed while +0x482 is set), then
 * the finish. */
void em008_act2_6(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em008_turn_seq_wait, 0, 1, 0);
        if (self->field_0x482 == 1) {
            em_mot_speed_set(self, 1.0f);
        }
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em008_turn_seq_wait) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015EAC8 (0x7C): motion (0x21, 4) and its end. */
void em008_act2_7(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015EB44 (0x1BC): the leap: motion (3, 4), then the fall start with motion (4, 0) and the offset steps while
 * airborne (clamped at -200/20), then the landing motion (0x23, 0) once the ground is near, then the finish. */
void em008_act2_8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_move_vec2_clr(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 4, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 80.0f;
            self->field_0x318 = 40.0f;
            self->field_0x320 = -9.0f;
            self->field_0x324 = -1.0f;
            em_move_offset_step(self, &self->field_0x1BC);
            em_busy_set(self);
            em_busy_timer_reset(self);
        }
        break;
    case 2:
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x314 < -200.0f) {
            self->field_0x314 = -200.0f;
            self->field_0x320 = 0.0f;
        }
        if (self->field_0x318 < 20.0f) {
            self->field_0x318 = 20.0f;
            self->field_0x324 = 0.0f;
        }
        if (em_ground_ck_f(self, 330.0f) == 1) {
            self->state++;
            self->pos.y = self->field_0x20C;
            em_move_mode_set(self, 0);
            em_move_vec2_clr(self);
            em_mot_set(self, 0x23, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015ED00 (0x94): motion (0x10, 0xA) with a zero-length approach, finished when the approach arrives. */
void em008_act2_9(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015ED94 (0x140): the back-step: the back turn sequence with the offset pair rotated toward the target, then the
 * fade cancel and the finish. */
void em008_act2_10(_ENEMY_WORK* self) {
    f32 temp_f31;
    s32 temp_r3;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em008_turn_seq_back, 0, 1, 0);
        if (self->field_0x482 == 1) {
            em_mot_speed_set(self, 1.0f);
        }
        em_move_vec2_clr(self);
        self->field_0x318 = -8.5875f * get_em_base_scale(self) * get_em_chg_scale(self);
        self->field_0x324 = 0.0f;
        temp_r3 = calcVecAng2(&self->pos, &self->vec_0x36C);
        rotVecY(&self->offset_0x30C.vec_0x310, temp_r3);
        rotVecY(&self->vec_0x31C, temp_r3);
        return;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 80.0f) == 1) {
            CancelFade(self);
        }
        if (em_turn_seq_step(self, (void*)em008_turn_seq_back) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015EED4 (0xD8): motion (0x22, 2) turning toward the target by the scaled angle in its window, then the finish. */
void em008_act2_11(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 48.0f) == 1) {
            em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * (2.0833333f * get_em_base_scale(self))) / 360.0f));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015EFAC (0x13C): the closing approach: motion (0xA, 6) toward the target (at most the sight range), then motion
 * (0xB, 4). */
void em008_act2_20(_ENEMY_WORK* self) {
    f32* temp_r31;
    f32 temp_f0;
    f32 temp_f1;

    temp_r31 = em_sight2_get(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 6, 0);
        temp_f1 = vec3_distance(&self->pos, &self->vec_0x36C);
        temp_f1 = -0.5f * temp_f1;
        temp_f0 = -temp_r31[1];
        if (temp_f1 > temp_f0) {
            temp_f1 = temp_f0;
        }
        em_approach_start(self, temp_f1, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            if (em_mot_frame_started_ck(self) == 0) {
                self->state++;
                em_mot_set(self, 0xB, 4, 0);
                em_approach_start(self, 0.0f, 0);
            }
        }
        return;
    case 2:
        em_approach_step(self, 0, 0x80);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015F0E8 (0x154): the side turn: the side turn sequence by 10 or 15 degrees toward the target side, finished at
 * frame 60. */
void em008_act2_21(_ENEMY_WORK* self) {
    f32 var_f1;
    f32 var_f2;
    s32 temp_r0;
    s32 var_r31;

    var_r31 = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if ((s16) (calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0) < 0) {
            var_f1 = 10.0f;
            var_f2 = 15.0f;
        } else {
            var_f1 = 15.0f;
            var_f2 = 10.0f;
        }
        temp_r0 = self->bits_0x1EC & 3;
        switch (temp_r0) {
        case 0:
            var_r31 = (s32) (u16) (s32) (0.5f + ((65536.0f * var_f1) / 360.0f));
            break;
        case 1:
            var_r31 = -(s32) (u16) (s32) (0.5f + ((65536.0f * var_f2) / 360.0f));
            break;
        }
        em_turn_seq_start(self, (void*)em008_turn_seq_side, 0, 1, (u16) var_r31);
        return;
    case 1:
        em_turn_seq_step(self, (void*)em008_turn_seq_side);
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015F23C (0x9C): action 2's dispatcher: `state_sub` picks the approach kind or the step. */
void em008_act2_dispatch(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        em008_act2_approach(self, 0, 0);
        return;
    case 1:
        em008_act2_approach(self, 1, 1);
        return;
    case 2:
        em008_act2_approach(self, 2, 1);
        return;
    case 3:
        em008_act2_approach(self, 3, 1);
        return;
    case 4:
        em008_act2_approach(self, 4, 1);
        return;
    case 5:
        em008_act2_approach(self, 5, 1);
        return;
    case 6:
        em008_act2_6(self);
        return;
    case 7:
        em008_act2_7(self);
        return;
    case 8:
        em008_act2_8(self);
        return;
    case 9:
        em008_act2_9(self);
        return;
    case 10:
        em008_act2_10(self);
        return;
    case 11:
        em008_act2_11(self);
        return;
    case 12:
        em008_act2_approach(self, 6, 1);
        return;
    case 20:
        em008_act2_20(self);
        return;
    case 21:
        em008_act2_21(self);
        return;
    }
}

/* 0x8015F2D8 (0xC4): the down: motion (0x26, 4) with hit window 0xE and a shake at frame 40, then the stand finish. */
void em008_act6_0(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_hit_window_set_default(self, 0, 0xE);
        return;
    case 1:
        if (em_frame_check(self, 1, 200.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x8015F39C (0xB4): the trip: motion (1, 0) on the ground height for 90 frames, then the stand finish. */
void em008_act6_1(_ENEMY_WORK* self) {
    s32 temp_r0;

    em_busy_timer_reset(self);
    em_fx_flag2_set(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        em_height_set(self, em_height_base_get(self));
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 == 0) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x8015F450 (0xC0): the knock-back: motion (6, 0) and a 1000-unit approach, then the stand finish. */
void em008_act6_2(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    em_fx_flag2_set(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 6, 0, 0);
        em_approach_start(self, -200.0f, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x8015F510 (0xFC): the slide: motion (0xA, 0) and an approach `kind` 0 extends, then the stand finish. */
void em008_act6_3(_ENEMY_WORK* self, u8 arg1) {
    f32 f31;

    f31 = 0.0f;
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_fx_flag2_set(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 0, 0);
        if ((u8) arg1 == 0) {
            f31 = -700.0f;
        }
        em_approach_start(self, f31, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x8015F60C (0xB8): the wait turn sequence on the ground height, then the stand finish. */
void em008_act6_4(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    em_fx_flag2_set(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em008_turn_seq_wait, 0, 0, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em008_turn_seq_wait) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x8015F6C4 (0x15C): the fall: motion (0x27, 2) at a height `kind` picks, the shakes and the height drop, then the
 * finish. */
void em008_act6_5(_ENEMY_WORK* self, u8 arg1) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        if ((u8) arg1 == 0) {
            em_mot_set(self, 0x27, 2, 0x24);
            em_height_set(self, -600.0f);
            em_hit_window_set_default(self, 0, 0xF);
        } else {
            em_mot_set(self, 0x27, 2, 0);
            em_height_set(self, 0.0f);
            em_hit_window_set_default(self, 0, 0x11);
        }
        em_shake_req_set(self);
        return;
    case 1:
        if (em_frame_check(self, 2, 46.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if ((u8) arg1 == 0) {
            if (self->field_0x1AC >= 0.0f) {
                em_height_set(self, 0.0f);
            } else {
                em_height_add(self, 50.0f);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8015F820 (0xC4): the trapped rise: motion (0x25, 0) on the ground height, rising by 10 until the height reaches
 * 0. */
void em008_act6_7(_ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0x25, 0, 0);
        em_height_set(self, em_height_base_get(self));
        em008_part_flags_seed(self);
        return;
    case 1:
        em_height_add(self, 10.0f);
        if (self->field_0x1AC >= 0.0f) {
            self->field_0x1AC = 0.0f;
            em008_finish_act6(self);
        }
        return;
    }
}

/* 0x8015F8E4 (0x118): the trapped hold: motion (0x25, 0) for 300 frames or until the damage outruns its limit, then
 * action 6 sub-state 8 or 9. */
void em008_act6_8(_ENEMY_WORK* self) {
    s32 temp_r0;

    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0x25, 0, 0);
        em_height_set(self, 0.0f);
        em008_part_flags_seed(self);
        self->timer_0x020 = 0x12C;
        em_flag_0x8AD_set(self);
        return;
    case 1:
        if ((s16) self->value_0x452 > (s16) self->value_0x44E) {
            self->state++;
            em_state_set(self, 6, 9);
            return;
        }
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            self->state++;
            if ((s32) (self->bits_0x1EC & 0xF) == 0) {
                em_state_set(self, 6, 9);
                return;
            }
            em_state_set(self, 6, 8);
        } else {
            return;
        }
        break;
    }
}

/* 0x8015F9FC (0xC4): the trapped turn: motion (0x25, 0) turning toward the target for 180 frames. */
void em008_act6_10(_ENEMY_WORK* self) {
    s32 temp_r0;

    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0x25, 0, 0);
        em_height_set(self, 0.0f);
        self->timer_0x020 = 0;
        em008_part_flags_seed(self);
        em_flag_0x8AD_set(self);
        return;
    case 1:
        em_turn_to_target(self, 0x5B);
        temp_r0 = self->timer_0x020 + 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 >= 0xB4) {
            em008_finish_act6(self);
        }
        return;
    }
}

/* 0x8015FAC0 (0xB8): the trapped wait: motion (0x25, 0) for 150 frames. */
void em008_act6_12(_ENEMY_WORK* self) {
    s32 temp_r0;

    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0x25, 0, 0);
        em_height_set(self, 0.0f);
        em008_part_flags_seed(self);
        self->timer_0x020 = 0x96;
        em_flag_0x8AD_set(self);
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            em008_finish_act6(self);
        }
        return;
    }
}

/* 0x8015FB78 (0x138): the trapped drift: motion (0x25, 0) with an approach and the dust every 16 frames until it
 * arrives. */
void em008_act6_13(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em_busy_timer_reset(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x25, 0, 0);
        em_height_set(self, 0.0f);
        em008_part_flags_seed(self);
        em_flag_0x8AD_set(self);
        em_move_vec2_clr(self);
        self->field_0x318 = 3.0f;
        em_approach_start(self, 0.0f, 0);
        return;
    case 1:
        if ((s32) (system_w.field_0x0c & 0xF) == 0) {
            setVector3(&vec, 0.0f, 0.0f, 130.0f);
            eft009_spawn_at_pos(self, 0x13, 0x30, 0, 3.0f, &vec);
        }
        if (em_approach_step(self, 0, 0x40) == 1) {
            em008_finish_act6(self);
            return;
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        return;
    }
}

/* 0x8015FCB0 (0x6C): action 6's dispatcher: `state_sub` picks the down/trap step. */
void em008_act6_dispatch(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        em008_act6_0(self);
        return;
    case 1:
        em008_act6_1(self);
        return;
    case 2:
        em008_act6_2(self);
        return;
    case 3:
        em008_act6_3(self, 0);
        return;
    case 4:
        em008_act6_4(self);
        return;
    case 5:
        em008_act6_5(self, 0);
        return;
    case 6:
        em008_act6_3(self, 1);
        return;
    case 7:
        em008_act6_7(self);
        return;
    case 8:
        em008_act6_8(self);
        return;
    case 9:
        em008_act6_5(self, 1);
        return;
    case 10:
        em008_act6_10(self);
        return;
    case 12:
        em008_act6_12(self);
        return;
    case 13:
        em008_act6_13(self);
        return;
    case 11:
        return;
    }
}

/* 0x8015FD1C (0x68): fills an effect spawn record: the type, the position and the three id fields. */
void em008_spawn_rec_set(EmSpawnRec* out, u8 a, u32 id, VEC3* pos, u16 b, u16 c) {
    out->id = id;
    copyVec3(&out->pos, pos);
    out->field_0x10 = a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* 0x8015FD84 (0x18DC): the burrowing attack: motion (0x19, 4) repeated a `kind`-dependent number of times, the dust
 * every cycle and the per-cycle rock records for the burrowing pattern. */
void em008_act7_0(_ENEMY_WORK* self, u8 arg1) {
    EmSpawnRec rec;
    VEC3 vec;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r0_3;
    u8 temp_r0_4;
    u8 temp_r0_5;
    u8 temp_r3_2;

    VEC3_ctor(&vec);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x19, 4, 0);
        switch ((s32) arg1) {
        case 0:
        case 2:
            self->timer_0x020 = 0xF;
            break;
        case 1:
        case 3:
            self->timer_0x020 = 7;
            break;
        }
        break;
    case 1:
        if (em_frame_check(self, 1, 100.0f, 0.0f) == 1) {
            if (self->state_0x007 == 0) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                eft_em_spawn(self, 9, 0x14, &vec, 1.0f);
            }
            if (self->state_0x007 == 3) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 9, 0x11, &vec, 1.0f);
            }
            if (self->state_0x007 == 6) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 9, 3, &vec, 1.0f);
            }
            if (self->state_0x007 > 8) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007++;
            }
        }
        switch ((s32) arg1) {
        case 0:
            temp_r0 = self->state_0x006;
            switch (temp_r0) {
            case 0:
                if ((em008_part_flag_clear_ck(self, 2) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 3, 4, &vec, 0, 0xC500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x14, 4, &vec, 0, 0x9000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 2:
                if ((em008_part_flag_clear_ck(self, 4) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0xF000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 1) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x31, 4, &vec, 0, 0x2000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 4:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0xA500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0xC500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 6:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x12, 4, &vec, 0, 0x1000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 0x10) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x25, 4, &vec, 0, 0x5000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 8:
                if ((em008_part_flag_clear_ck(self, 0x20) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x20, 4, &vec, 0, 0x8000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x35, 4, &vec, 0, 0x3500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 10:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x32, 4, &vec, 0, 0);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 4) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0x2000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 12:
                if ((em008_part_flag_clear_ck(self, 2) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0x6000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 8) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x21, 4, &vec, 0, 0xA000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 14:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 4, 4, &vec, 0, 0x4500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 1) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x33, 4, &vec, 0, 0xE000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            }
            break;
        case 1:
            temp_r0_2 = self->state_0x006;
            switch ((s32) temp_r0_2) {
            case 0:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0, 0x14, &vec, 0, 0x2000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x11, 0x14, &vec, 0, 0xB500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 2:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 2, 0x14, &vec, 0, 0x3000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x13, 0x14, &vec, 0, 0xF000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 4:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 4, 0x14, &vec, 0, 0);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x15, 0x14, &vec, 0, 0x1000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 6:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x10, 0x14, &vec, 0, 0xE000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x21, 0x14, &vec, 0, 0);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            }
            break;
        case 2:
            temp_r0_3 = self->state_0x006;
            switch (temp_r0_3) {
            case 0:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x32, 4, &vec, 0, 0);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 8) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x21, 4, &vec, 0, 0xA000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 2:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0xC500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x35, 4, &vec, 0, 0x3500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 4:
                if ((em008_part_flag_clear_ck(self, 0x10) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x25, 4, &vec, 0, 0x5000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 1) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x33, 4, &vec, 0, 0xE000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 6:
                if ((em008_part_flag_clear_ck(self, 2) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 3, 4, &vec, 0, 0xC500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 1) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x31, 4, &vec, 0, 0x2000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 8:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x14, 4, &vec, 0, 0x9000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 4) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0x2000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 10:
                if ((em008_part_flag_clear_ck(self, 4) == 1) && (em_frame_check(self, 0, 102.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0xF000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 4, 4, &vec, 0, 0x4500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 12:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x12, 4, &vec, 0, 0x1000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 0x20) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x20, 4, &vec, 0, 0x8000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 14:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0xA500);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if ((em008_part_flag_clear_ck(self, 2) == 1) && (em_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0x6000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            }
            break;
        case 3:
            temp_r0_4 = self->state_0x006;
            switch ((s32) temp_r0_4) {
            case 0:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x10, 0x14, &vec, 0, 0xE000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 75.0f, 75.0f);
                    em008_spawn_rec_set(&rec, 0x21, 0x14, &vec, 0, 0);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 2:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0, 0x14, &vec, 0, 0x2000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x13, 0x14, &vec, 0, 0xF000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 4:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x15, 0x14, &vec, 0, 0x1000);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x12, 0x14, &vec, 0, 0x6800);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            case 6:
                if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 4, 0x14, &vec, 0, 0);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                    setVector3(&vec, 0.0f, 0.0f, 0.0f);
                    em008_spawn_rec_set(&rec, 0x11, 0x14, &vec, 0, 0x9800);
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
                }
                break;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            if (++self->state_0x006 >= self->timer_0x020) {
                self->state++;
                self->state_0x007 = 0;
                em_mot_set(self, 0x2A, 0, 0);
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

/* 0x80161660 (0x124): the side-step: motion (0x15, 4) with hit window 4 and the offset pair rotated to the facing,
 * then the fade cancel and the window turn until the motion ends. */
void em008_act7_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_hit_window_set_default(self, 0, 4);
        em_move_vec2_clr(self);
        self->field_0x318 = -3.4482758f * get_em_base_scale(self) * get_em_chg_scale(self);
        self->field_0x324 = 0.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        rotVecY(&self->vec_0x31C, self->field_0x1C0);
        break;
    case 1:
        if (em_frame_check(self, 3, 18.0f, 76.0f) == 1) {
            CancelFade(self);
        }
        em_turn_in_window2(self, 18.0f, 76.0f, -0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80161784 (0x120): the side swipe: motion (0x14, 4) or (0x1A, 4) by `kind`, turning through the window toward one
 * side. */
void em008_act7_2(_ENEMY_WORK* self, u8 arg1) {
    switch (self->state) {
    case 0:
        self->state++;
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
            em_turn_in_window(self, 0.0f, 50.0f, 0x4000);
        } else {
            em_turn_in_window(self, 0.0f, 50.0f, -0x4000);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801618A4 (0xE8): motion (0x13, 4) with hit window 0xC, turning toward the target in its window, then the finish. */
void em008_act7_5(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 4, 0);
        em_hit_window_set_default(self, 0, 0xC);
        return;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 20.0f) == 1) {
            em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * (1.5f * get_em_base_scale(self))) / 360.0f));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8016198C (0xA8): motion (0x16, 4) held until the frame `kind` picks, then action 7 sub-state 0x12. */
void em008_act7_6(_ENEMY_WORK* self, u8 arg1) {
    f32 temp_f1;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        if ((u8) arg1 == 0) {
            temp_f1 = 44.0f;
        } else {
            temp_f1 = 60.0f;
        }
        if (em_frame_check(self, 1, temp_f1, 0.0f) == 1) {
            em008_finish_act7(self);
        }
        return;
    }
}

/* 0x80161A34 (0x2F0): the charge: motion (0x17, 2), an approach whose distance and speed `kind` picks, the step
 * rate per kind while running, then motion (0x18, 6) with the slowed speed and the hit windows and dust. */
void em008_act7_7(_ENEMY_WORK* self, u8 kind) {
    VEC3 vec;
    f32 dist;
    f32 speed;
    s32 step;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 2, 0);
        em_hit_window_set(self, 0, 0x15, 0xB);
        switch (kind) {
        case 0:
            dist = -500.0f;
            speed = 1.0f;
            break;
        case 1:
            dist = -100.0f;
            speed = 1.0f;
            break;
        case 2:
            dist = -100.0f;
            speed = 0.7f;
            break;
        }
        em_approach_start(self, dist, 0);
        em_mot_speed_set(self, speed);
        em_stamina_add(self, -30);
        break;
    case 1:
        if (em_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 1, 0x1B);
        }
        switch (kind) {
        case 0:
            step = 0x80;
            break;
        case 1:
            step = 0x180;
            speed = 0.01f + self->field_0x7C4;
            if (speed > 1.3f) {
                speed = 1.3f;
            }
            em_mot_speed_set(self, speed);
            break;
        case 2:
            step = 0x100;
            break;
        }
        if (em_approach_step(self, 0, step) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 6, 0x2E);
            switch (kind) {
            case 0:
                speed = 1.0f;
                break;
            case 2:
                speed = 0.7f;
                break;
            }
            speed -= 0.15f;
            if (speed < 1.0f) {
                speed = 1.0f;
            }
            em_mot_speed_set(self, speed);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em_hit_window_clear_all(self);
            em_hit_window_set(self, 0, 2, 0x18);
            em_hit_window_set(self, 1, 0x10, 0x10);
        }
        if (em_frame_check(self, 3, 46.0f, 60.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            setVector3(&vec, 0.0f, 150.0f, 100.0f);
            eft_em_spawn(self, 0xC, 0x14, &vec, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80161D24 (0xA8): motion (0x18, 2) with hit windows 2 and 0x10 and the stamina cost, then the finish. */
void em008_act7_8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 2, 0);
        em_hit_window_set_default(self, 0, 2);
        em_hit_window_set_default(self, 1, 0x10);
        em_stamina_add(self, -0x1E);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80161DCC (0xC8): motion (0x1C, 2) with the hit window `kind` picks and the stamina cost, then the finish. */
void em008_act7_9(_ENEMY_WORK* self, u8 arg1) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 2, 0);
        if ((u8) arg1 == 0) {
            em_hit_window_set_default(self, 0, 3);
        } else {
            em_hit_window_set(self, 0, 0x14, 0x40);
        }
        em_stamina_add(self, -0x1E);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80161E94 (0x194): motion (0x1B, 4) with hit window 0xD and, for `kind` 1, two rock records, then the finish. */
void em008_act7_11(_ENEMY_WORK* self, u8 arg1) {
    EmSpawnRec rec;
    VEC3 vec;

    VEC3_ctor(&vec);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        em_hit_window_set_default(self, 0, 0xD);
        return;
    case 1:
        if (arg1 == 1) {
            if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                em008_spawn_rec_set(&rec, 0, 0x14, &vec, 0, 0x2000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                em008_spawn_rec_set(&rec, 0, 0x14, &vec, 0, 0xE000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80162028 (0x7C): motion (0x16, 4, 0x2C) and its end, then action 7 sub-state 0x12. */
void em008_act7_18(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0x2C);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em008_finish_act7(self);
        }
        return;
    }
}

/* 0x801620A4 (0xB0): action 7's dispatcher: `state_sub` picks the attack step and its kind. */
void em008_act7_dispatch(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        em008_act7_0(self, 0);
        return;
    case 1:
        em008_act7_1(self);
        return;
    case 2:
        em008_act7_2(self, 0);
        return;
    case 3:
        em008_act7_0(self, 1);
        return;
    case 4:
        em008_act7_9(self, 1);
        return;
    case 5:
        em008_act7_5(self);
        return;
    case 6:
        em008_act7_6(self, 0);
        return;
    case 7:
        em008_act7_7(self, 0);
        return;
    case 8:
        em008_act7_8(self);
        return;
    case 9:
        em008_act7_9(self, 0);
        return;
    case 10:
        em008_act7_2(self, 1);
        return;
    case 11:
        em008_act7_11(self, 0);
        return;
    case 12:
        em008_act7_6(self, 1);
        return;
    case 13:
        em008_act7_7(self, 1);
        return;
    case 14:
        em008_act7_0(self, 2);
        return;
    case 15:
        em008_act7_0(self, 3);
        return;
    case 16:
        em008_act7_7(self, 2);
        return;
    case 17:
        em008_act7_11(self, 1);
        return;
    case 18:
        em008_act7_18(self);
        return;
    }
}

/* The action-10 sound programs (`.data`), in retail order. */
EmSeRecord em008_se_rec_17_0 = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_17_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_58_0 = {107, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_58[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_58_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_1A_0 = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_1A_2 = {121, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em008_se_rec_1A_3 = {113, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_1A[5] = {
    {0x00, {0, 0, 0}, &em008_se_rec_1A_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em008_se_rec_1A_2},
    {0x00, {0, 0, 0}, &em008_se_rec_1A_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_1B_0 = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_1B_2 = {120, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em008_se_rec_1B_3 = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_1B[5] = {
    {0x00, {0, 0, 0}, &em008_se_rec_1B_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em008_se_rec_1B_2},
    {0x00, {0, 0, 0}, &em008_se_rec_1B_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em008_se_pos_3B_0 = {90.0f, 121.0f, -1, -16384, 0};

EmSeRecord em008_se_rec_3B_1 = {103, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_3B[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em008_se_pos_3B_0},
    {0x00, {0, 0, 0}, &em008_se_rec_3B_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSePos em008_se_pos_3C_0 = {92.0f, 122.0f, 0, 16384, 0};

EmSeRecord em008_se_rec_3C_1 = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_3C[3] = {
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em008_se_pos_3C_0},
    {0x00, {0, 0, 0}, &em008_se_rec_3C_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_5B_0 = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_5B_2 = {121, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em008_se_rec_5B_3 = {113, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_5B[5] = {
    {0x00, {0, 0, 0}, &em008_se_rec_5B_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xF0)},
    {0x00, {0, 0, 0}, &em008_se_rec_5B_2},
    {0x00, {0, 0, 0}, &em008_se_rec_5B_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_5C_0 = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_5C_2 = {120, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em008_se_rec_5C_3 = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_5C[5] = {
    {0x00, {0, 0, 0}, &em008_se_rec_5C_0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xF0)},
    {0x00, {0, 0, 0}, &em008_se_rec_5C_2},
    {0x00, {0, 0, 0}, &em008_se_rec_5C_3},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_1C_0 = {123, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_1C[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_1C_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_7A_0 = {109, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_7A_0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_7B_0 = {110, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em008_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_7B_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_9F_0 = {112, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_9F_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_7C_0 = {111, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em008_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_7C_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_8D_0 = {15, 0, 14, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_8D_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_78_0 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_78[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_78_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_7E_0 = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_7E_1 = {120, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em008_se_rec_7E_2 = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_7E[4] = {
    {0x00, {0, 0, 0}, &em008_se_rec_7E_0},
    {0x02, {0, 0, 0}, &em008_se_rec_7E_1},
    {0x00, {0, 0, 0}, &em008_se_rec_7E_2},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_7F_0 = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_7F_1 = {121, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em008_se_rec_7F_2 = {113, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_7F[4] = {
    {0x00, {0, 0, 0}, &em008_se_rec_7F_0},
    {0x02, {0, 0, 0}, &em008_se_rec_7F_1},
    {0x00, {0, 0, 0}, &em008_se_rec_7F_2},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_8E_0 = {15, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_8E_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_A8_0 = {100, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_A8_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_B6_0 = {117, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_B6_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_B7_0 = {118, 0, 2, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em008_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_B7_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_B8_1 = {4, 0, 4, 0, 0, {0, 128, 3, 0}, (const EmSeOffset*)em008_se_shift_a};

EmSeRecord em008_se_rec_B8_3 = {0, 0, 0, 0, 0, {0, 2, 3, 0}, (const EmSeOffset*)em008_se_shift_b};

EmSeRecord em008_se_rec_B8_5 = {35, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_B8[7] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em008_se_rec_B8_1},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1B8)},
    {0x01, {0, 0, 0}, &em008_se_rec_B8_3},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em008_se_rec_B8_5},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_B9_0 = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_B9_0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_BA_0 = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em008_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em008_se_rec_BA_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_BB_0 = {119, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em008_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_BB_0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_BF_2 = {107, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em008_se_rec_BF_3 = {111, 0, 6, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em008_se_seq_BF[5] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em008_se_rec_BF_2},
    {0x00, {0, 0, 0}, &em008_se_rec_BF_3},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_C3_0 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_C3[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_C3_0},
    {0xFF, {0, 0, 0}, 0},
};

EmSePosRec em008_se_pos_C8_1 = {110.0f, 120.0f, -1, -16384};

EmSePosRec em008_se_pos_C8_3 = {204.0f, 244.0f, -1, -16384};

EmSeRecord em008_se_rec_C8_2 = {122, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em008_se_rec_C8_4 = {122, 0, 0, 200, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_seq_C8[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC8)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em008_se_pos_C8_1},
    {0x00, {0, 0, 0}, &em008_se_rec_C8_2},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em008_se_pos_C8_3},
    {0x00, {0, 0, 0}, &em008_se_rec_C8_4},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x80162154 (0x2E8): action 10's sound programs: `state_sub` picks the per-motion table, the rest finish. */
void em008_act10_se_play(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state_sub;
    switch (temp_r4) {
    case 0x17:
        em_se_tbl_play(self, em008_se_seq_17, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play(self, em008_se_seq_17, 0, 0x18);
        return;
    case 0x19:
        em_se_tbl_play(self, em008_se_seq_17, 0, 0x19);
        return;
    case 0x1A:
        em_se_tbl_play(self, em008_se_seq_1A, 0, 0x1A);
        return;
    case 0x1B:
        em_se_tbl_play(self, em008_se_seq_1B, 0, 0x1B);
        return;
    case 0x1C:
        em_se_tbl_play(self, em008_se_seq_1C, 0, 0x1C);
        return;
    case 0x3B:
        em_se_tbl_play(self, em008_se_seq_3B, 0, 0x3B);
        return;
    case 0x3C:
        em_se_tbl_play(self, em008_se_seq_3C, 0, 0x3C);
        return;
    case 0x58:
        em_se_tbl_play(self, em008_se_seq_58, 0, 0x58);
        return;
    case 0x59:
        em_se_tbl_play(self, em008_se_seq_58, 0, 0x59);
        return;
    case 0x5A:
        em_se_tbl_play(self, em008_se_seq_58, 0, 0x5A);
        return;
    case 0x5B:
        em_se_tbl_play(self, em008_se_seq_5B, 0, 0x5B);
        return;
    case 0x5C:
        em_se_tbl_play(self, em008_se_seq_5C, 0, 0x5C);
        return;
    case 0x7A:
        em_se_tbl_play(self, em008_se_seq_7A, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play(self, em008_se_seq_7B, 0, 0x7B);
        return;
    case 0x9F:
        em_se_tbl_play(self, em008_se_seq_9F, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play(self, em008_se_seq_9F, 0, 0xA0);
        return;
    case 0x7C:
        em_se_tbl_play(self, em008_se_seq_7C, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play(self, em008_se_seq_8D, 0, 0x8D);
        return;
    case 0x78:
        em_se_tbl_play(self, em008_se_seq_78, 0, 0x78);
        return;
    case 0xA8:
        em_se_tbl_play(self, em008_se_seq_A8, 0, 0xA8);
        return;
    case 0x7E:
        em_se_tbl_play(self, em008_se_seq_7E, 0, 0x7E);
        return;
    case 0x7F:
        em_se_tbl_play(self, em008_se_seq_7F, 0, 0x7F);
        return;
    case 0x8E:
        em_se_tbl_play(self, em008_se_seq_8E, 0, 0x8E);
        return;
    case 0xB6:
        em_se_tbl_play(self, em008_se_seq_B6, 0, 0xB6);
        return;
    case 0xB7:
        em_se_tbl_play(self, em008_se_seq_B7, 0, 0xB7);
        return;
    case 0xB8:
        em_se_tbl_play(self, em008_se_seq_B8, 1, 0xB8);
        return;
    case 0xB9:
        em_se_tbl_play(self, em008_se_seq_B9, 0, 0xB9);
        return;
    case 0xBA:
        em_se_tbl_play(self, em008_se_seq_BA, 0, 0xBA);
        return;
    case 0xBB:
        em_se_tbl_play(self, em008_se_seq_BB, 0, 0xBB);
        return;
    case 0xBC:
        em_se_tbl_play(self, em008_se_seq_BB, 0, 0xBC);
        return;
    case 0xBF:
        em_se_tbl_play(self, em008_se_seq_BF, 0, 0xBF);
        return;
    case 0xC1:
        em_se_tbl_play(self, em008_se_seq_8D, 0, 0xC1);
        return;
    case 0xC3:
        em_se_tbl_play(self, em008_se_seq_C3, 0, 0xC3);
        return;
    case 0xC8:
        em_se_tbl_play(self, em008_se_seq_C8, 0, 0xC8);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

/* The action-11 sound programs (`.data`). */
EmSeRecord em008_se_rec_alt_00_0 = {108, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_alt_seq_00[3] = {
    {0x00, {0, 0, 0}, &em008_se_rec_alt_00_0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_alt_0F_0 = {119, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_alt_seq_0F[3] = {
    {0x00, {0, 0, 0}, &em008_se_rec_alt_0F_0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_alt_1A_0 = {109, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em008_se_rec_alt_1A_1 = {110, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_alt_seq_1A[3] = {
    {0x00, {0, 0, 0}, &em008_se_rec_alt_1A_0},
    {0x00, {0, 0, 0}, &em008_se_rec_alt_1A_1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em008_se_rec_alt_1C_0 = {119, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em008_se_alt_seq_1C[2] = {
    {0x00, {0, 0, 0}, &em008_se_rec_alt_1C_0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x8016243C (0xA8): action 11's sound programs: `state_sub` picks the table, 0 by default. */
void em008_act11_se_play(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {
    case 0:
        em_se_tbl_play_alt(self, em008_se_alt_seq_00, 0, 0);
        return;
    case 5:
        em_se_tbl_play_alt(self, em008_se_alt_seq_00, 1, 5);
        return;
    case 15:
        em_se_tbl_play_alt(self, em008_se_alt_seq_0F, 0, 0xF);
        return;
    case 26:
        em_se_tbl_play_alt(self, em008_se_alt_seq_1A, 0, 0x1A);
        return;
    case 28:
        em_se_tbl_play_alt(self, em008_se_alt_seq_1C, 0, 0x1C);
        return;
    default:
        em_se_tbl_play_alt(self, em008_se_alt_seq_00, 0, 0);
        return;
    }
}

/* 0x801624E4 (0x8): the trapped fall: the fall step of kind 0. */
void em008_act12_0(_ENEMY_WORK* self) {
    em008_act6_5(self, 0);
}

/* 0x801624EC (0x14): action 12's dispatcher: sub-state 0 only. */
void em008_act12_dispatch(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em008_act12_0(self);
    }
}

/* 0x80162500 (0xCC4): the arena demo: the timed motion and position keys of the entrance, the burrow cycle with its
 * rock records, the emerge and the roar. */
void em008_act13_0(_ENEMY_WORK* self) {
    EmSpawnRec rec;
    VEC3 vec;

    VEC3_ctor(&vec);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        self->timer_0x020 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_pos_set(self, -574.857f, -26.6507f, -2456.36f);
        em_demo_reset(self, 0);
        self->field_0x1E4 = 0x3F;
        return;
    case 1:
        if (em_demo_time_ck(0x4A) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 0xA, 0, 0);
            self->field_0x1C0 = 0;
            em_demo_pos_set(self, -574.857f, -26.6507f, -2456.36f);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 4, 0);
            return;
        }
        break;
    case 3:
        if (em_demo_time_ck(0xDC) == 1) {
            self->state++;
            em_mot_set(self, 0xB, 0, 0x26);
            self->field_0x1C0 = 0;
            em_demo_pos_set(self, -574.857f, -26.6507f, 412.3778f);
            return;
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_blend(self, 1, 0xC, 0, 1);
            self->field_0x1C0 = 0;
            em_demo_pos_set(self, -574.857f, -26.6507f, 477.3114f);
            return;
        }
        break;
    case 5:
        if (em_demo_time_ck(0x1A4) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 0x24, 0, 0);
            self->field_0x1C0 = 0;
            em_demo_pos_set(self, -574.857f, -26.6507f, 515.1197f);
            return;
        }
        break;
    case 6:
        if ((self->state_0x006 == 0) && (em_demo_time_ck(0x280) == 1)) {
            self->state_0x006 = 1;
            em008_part_flags_seed(self);
        }
        if (em_demo_time_ck(0x2E8) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x29, 0xA, 0, 1);
            return;
        }
        break;
    case 7:
        if (em_demo_time_ck(0x312) == 1) {
            self->state++;
            em_mot_set_blend(self, 0x70, 0, 0x42, 0);
            return;
        }
        break;
    case 8:
        if (em_demo_time_ck(0x382) == 1) {
            self->state++;
            em_mot_set(self, 0x70, 0, 0xB8);
            return;
        }
        break;
    case 9:
        if (em_demo_time_ck(0x38E) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set_blend(self, 0x19, 6, 0, 1);
            self->timer_0x020 = 0xF;
            return;
        }
        break;
    case 10:
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 75.0f, 75.0f);
            eft_em_spawn(self, 9, 0x14, &vec, 1.0f);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 9, 0x11, &vec, 1.0f);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 9, 3, &vec, 1.0f);
        }
        switch (self->state_0x006) {
        case 0:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0x2000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0x6000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 2:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0x1000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0xE000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 4:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0x4500);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0x3500);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 6:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0xF000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x20, 4, &vec, 0, 0xA000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 8:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                em008_spawn_rec_set(&rec, 0x20, 4, &vec, 0, 0x8000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 75.0f, 75.0f);
                em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0xC500);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 10:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0xC500);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 12:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x10, 4, &vec, 0, 0x9000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x20, 4, &vec, 0, 0x5000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        case 14:
            if (em_frame_check(self, 0, 102.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0, 4, &vec, 0, 0xA500);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                setVector3(&vec, 0.0f, 0.0f, 0.0f);
                em008_spawn_rec_set(&rec, 0x30, 4, &vec, 0, 0x2000);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 4);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state_0x006++;
        }
        if (em_demo_time_ck(0x46C) == 1) {
            self->state++;
            em_mot_set(self, 0x2A, 0, 0);
            return;
        }
        break;
    case 11:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xF, 6, 0);
            return;
        }
        break;
    case 12:
        if (em_frame_check(self, 0, 160.0f, 0.0f) == 1) {
            draw_shape_arm((u32)self, 0x14, 0xA);
        }
        if (em_frame_check(self, 0, 160.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 0, 0x13, &vec, 1.0f);
        }
        if ((em_frame_check(self, 3, 164.0f, 264.0f) == 1) && ((s32) (em_demo_frame_get() & 7) == 0)) {
            setVector3(&vec, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 1, 0x13, &vec, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 6, 0);
        }
        break;
    }
}

/* 0x801631C4 (0xB0): the second demo: the demo position, motion (1, 0) and the flag re-seed, then the finish. */
void em008_act13_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -755.027f, -26.6507f, 515.1197f);
        self->field_0x1BC = 0;
        self->field_0x1C0 = 0;
        self->field_0x1C4 = 0;
        em_demo_enable(self);
        em008_part_flags_seed(self);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80163274 (0x24): action 13's dispatcher: sub-state 0 or 1. */
void em008_act13_dispatch(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        em008_act13_0(self);
        return;
    case 1:
        em008_act13_1(self);
        return;
    }
}

/* 0x80163298 (0xAC): the program's action runner: the dispatcher for the current action, then the busy refresh while
 * +0x1E2 is 1. */
void em008_action_run(_ENEMY_WORK* self) {
    u8 temp_r0;

    temp_r0 = self->action;
    switch (temp_r0) {
    case 0:
        em008_act0_dispatch(self);
        break;
    case 1:
        em008_act1_dispatch(self);
        break;
    case 2:
        em008_act2_dispatch(self);
        break;
    case 6:
        em008_act6_dispatch(self);
        break;
    case 7:
        em008_act7_dispatch(self);
        break;
    case 10:
        em008_act10_se_play(self);
        break;
    case 11:
        em008_act11_se_play(self);
        break;
    case 12:
        em008_act12_dispatch(self);
        break;
    case 13:
        em008_act13_dispatch(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* 0x80163344 (0x1B4): mirrors the flag byte into +0x761 (bit 0 also on motion 0x25 and late in motion 0x27) and each
 * flag bit into its part record. */
void em008_part_rec_update(_ENEMY_WORK* self) {
    self->field_0x761 = self->field_0x1E4;
    if ((em_get_mot_no(self) == 0x25) || ((em_get_mot_no(self) == 0x27) && (em_frame_check(self, 2, 36.0f, 0.0f) == 1))) {
        self->field_0x761 = (u8) (self->field_0x761 | 1);
    }
    if (em008_part_flag_clear_ck(self, 1) == 0) {
        em_part_rec_reset(self, 0);
    } else {
        em_part_rec_alt_set(self, 0, 0);
    }
    if (em008_part_flag_clear_ck(self, 2) == 0) {
        em_part_rec_reset(self, 1);
    } else {
        em_part_rec_alt_set(self, 1, 1);
    }
    if (em008_part_flag_clear_ck(self, 4) == 0) {
        em_part_rec_reset(self, 2);
    } else {
        em_part_rec_alt_set(self, 2, 2);
    }
    if (em008_part_flag_clear_ck(self, 8) == 0) {
        em_part_rec_reset(self, 3);
    } else {
        em_part_rec_alt_set(self, 3, 3);
    }
    if (em008_part_flag_clear_ck(self, 0x10) == 0) {
        em_part_rec_reset(self, 4);
    } else {
        em_part_rec_alt_set(self, 4, 4);
    }
    if (em008_part_flag_clear_ck(self, 0x20) == 0) {
        em_part_rec_reset(self, 5);
        return;
    }
    em_part_rec_alt_set(self, 5, 5);
}

/* 0x801634F8 (0xAC): spawns the alt-mode effect every 30 frames outside motion 0x19. */
void em008_alt_mode_fx(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    if ((em_alt_mode_ck(self) == 1) && ((s32) em_get_mot_no(self) != 0x19) && ((s32) (system_w.field_0x0c % 30) == 0)) {
        setVector3(&vec, 0.0f, 150.0f, 170.0f);
        eft_spawn_type10(self, 8, 0x13, &vec, 1.0f);
    }
}

/* 0x801635A4 (0x41C): the effect-spawn router: `kind` 0 spawns the ground effect `type` remaps (at the work's own
 * position and the +0x20C/+0x210 height when `joint` is 0xFF), `kind` 1 the area effect while not blocked. */
void em008_effect_spawn(_ENEMY_WORK* self, u8 kind, u8 type, u32 joint, s32 delta, f32 scale) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (kind) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (type) {
            case 0:
                type = 0xD;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xF, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xF, delta, scale);
                }
                break;
            case 2:
                type = 0xC;
                break;
            case 4:
                type = 0xC;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, delta, scale);
                }
                break;
            case 8:
                type = 0xC;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, delta, scale);
                }
                break;
            case 26:
                type = 0x1B;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                }
                break;
            case 40:
                type = 0xC;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0xE, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, delta, scale);
                }
                break;
            case 43:
                type = 0xC;
                if (joint == 0xFF) {
                    vec.x = self->pos.x;
                    vec.y = 5.0f + self->field_0x210;
                    vec.z = self->pos.z;
                    eft009_set_pos(0x17, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0x17, delta, scale);
                }
                break;
            case 1:
            case 3:
            case 5:
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
            switch (type) {
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 38:
                return;
            case 40:
            case 42:
            case 43:
                type = 2;
                break;
            case 41:
                type = 4;
                break;
            }
            if (joint == 0xFF) {
                vec.x = self->pos.x;
                vec.y = 5.0f + self->field_0x20C;
                vec.z = self->pos.z;
            }
        }
        if (joint == 0xFF) {
            eft009_set_pos(type, &vec, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, joint, type, delta, scale);
        }
        break;
    case 1:
        if ((self->field_0x228 & 6) == 0) {
            if (joint == 0xFF) {
                copyVec3(&vec, &self->pos);
            } else {
                get_joint_wpos_em(self, joint, &vec);
            }
            vec.y = self->field_0x20C;
            scale *= get_em_chg_scale(self);
            eft_spawn_pos_in_area(&vec, self->area_no, type, delta, scale);
        }
        break;
    }
}

/* 0x801639C0 (0x88): clears +0x491 once the mask-1 flag is set and motion 0x7A has passed frame 10 (or another motion
 * runs). */
void em008_flag491_update(_ENEMY_WORK* self) {
    if ((em_flags836_ck(self, 1) == 1) && (self->field_0x491 == 1)) {
        if ((s32) em_get_mot_no(self) == 0x7A) {
            if (em_after_frame_check(self, 1, 10.0f, 0.0f) == 1) {
                self->field_0x491 = 0;
            }
        } else {
            self->field_0x491 = 0;
        }
    }
}

/* 0x80163A48 (0x1F18): the per-motion events: the footstep, dust and ground effects and the camera shakes keyed by
 * motion and frame. */
void em008_motion_events(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    em008_part_rec_update(self);
    em008_flag491_update(self);
    em008_alt_mode_fx(self);
    switch (em_get_mot_no(self)) {
    case 0x2:
        if (em_after_frame_check(self, 0, 122.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x1D, 0, 1.2f);
        }
        break;
    case 0x5:
        if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0x28, 0x25, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0xB, 0x1D, 0x8000, 2.0f);
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0xB, 0x25, 0x8000, 2.0f);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0x28, 0x1D, 0, 1.2f);
        }
        break;
    case 0x6:
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x25, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0xB, 0x1D, 0x8000, 2.0f);
        }
        if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0xB, 0x25, 0x8000, 2.0f);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x1D, 0, 1.0f);
        }
        break;
    case 0xA:
        if (self->field_0x1E2 == 4) {
            if ((s32) (system_w.field_0x0c & 0x1F) == 0) {
                em008_effect_spawn(self, 0, 2, 3, 0, 1.4f);
                break;
            }
        } else {
            if ((em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    setVector3(&vec, 0.0f, 0.0f, -50.0f);
                    eft009_spawn_at_pos(self, 0x25, 0xD, 0, 1.0f, &vec);
                } else {
                    setVector3(&vec, 0.0f, 0.0f, -50.0f);
                    eft009_spawn_at_pos(self, 0x25, 2, 0, 1.4f, &vec);
                }
            }
            if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1)) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    setVector3(&vec, 0.0f, 0.0f, -50.0f);
                    eft009_spawn_at_pos(self, 0x1D, 0xC, 0, 1.2f, &vec);
                    eft009_spawn_at_pos(self, 0x1D, 0xE, 0, 1.0f, &vec);
                } else {
                    setVector3(&vec, 0.0f, 0.0f, -50.0f);
                    eft009_spawn_at_pos(self, 0x28, 0, 0, 1.0f, &vec);
                }
            }
            if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
                if ((s32) (self->field_0x228 & 6) != 0) {
                    setVector3(&vec, 0.0f, 0.0f, -50.0f);
                    eft009_spawn_at_pos(self, 0x25, 0xC, 0, 1.2f, &vec);
                    eft009_spawn_at_pos(self, 0x25, 0xE, 0, 1.0f, &vec);
                    eft009_spawn_at_pos(self, 0x1D, 0xD, 0, 1.0f, &vec);
                    break;
                }
                setVector3(&vec, 0.0f, 0.0f, -50.0f);
                eft009_spawn_at_pos(self, 0x20, 0, 0, 1.0f, &vec);
                break;
            }
        }
        break;
    case 0xB:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0x28, 0x20, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x28, 0, 1.0f);
        }
        break;
    case 0xF:
        if ((s32) (self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xD, 0x25, 0, 1.0f);
            }
            if ((em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 264.0f, 0.0f) == 1)) {
                em008_effect_spawn(self, 0, 0xC, 0x25, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xD, 0x1D, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xC, 0x1D, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xE, 0x25, 0, 1.0f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xB, 0x25, 0x8000, 2.0f);
            }
            if ((em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 144.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 264.0f, 0.0f) == 1)) {
                em008_effect_spawn(self, 0, 0x29, 0x25, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xB, 0x1D, 0x8000, 2.0f);
            }
            if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0x29, 0x1D, 0, 1.0f);
                break;
            }
        }
        break;
    case 0x10:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x28, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x20, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0, 0x20, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0, 0x28, 0, 1.0f);
        }
        break;
    case 0x11:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0xC, 0x1D, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xE, 0x1D, 0, 1.0f);
                break;
            }
            em008_effect_spawn(self, 0, 0x2A, 0x1D, 0, 1.4f);
        }
        break;
    case 0x13:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em_camera_req(self, 0x25, 7);
        }
        if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1)) {
            em008_effect_spawn(self, 0, 2, 0x25, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em_camera_req(self, 0x1D, 7);
        }
        if ((em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1)) {
            em008_effect_spawn(self, 0, 2, 0x1D, 0, 1.5f);
        }
        break;
    case 0x14:
        if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x28, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em_camera_req(self, 0x25, 7);
        }
        break;
    case 0x1A:
        if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x20, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em_camera_req(self, 0x25, 7);
        }
        break;
    case 0x15:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.0f);
        }
        break;
    case 0x17:
        if (em_after_frame_check(self, 1, 24.0f, 0.0f) == 1) {
            if ((s32) (system_w.field_0x0c & 7) == 0) {
                setVector3(&vec, 0.0f, 0.0f, 200.0f);
                eft_em_spawn(self, 0xE, 0x14, &vec, 1.0f);
            }
            if ((s32) (system_w.field_0x0c % 6) == 0) {
                eft_spawn_type_at_area(self, 3);
            }
        }
        if ((s32) (self->field_0x228 & 6) != 0) {
            if ((em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1)) {
                em008_effect_spawn(self, 0, 0xD, 0x25, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xF, 0x25, 0, 1.0f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0xD, 0x1D, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xF, 0x1D, 0, 1.0f);
            }
        } else {
            if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1)) {
                em008_effect_spawn(self, 0, 0x2A, 0x25, 0, 1.4f);
            }
            if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0x2A, 0x1D, 0, 1.4f);
            }
        }
        if ((em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1)) {
            em_camera_req(self, 0x14, 0);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em_camera_req(self, 0x14, 0);
        }
        break;
    case 0x18:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em_camera_req(self, 0x1D, 0);
        }
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0, 0x28, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 2, 0x20, 0, 1.4f);
        }
        if ((em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 118.0f, 0.0f) == 1)) {
            em008_effect_spawn(self, 0, 2, 0x28, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em_camera_req(self, 0x14, 0);
        }
        break;
    case 0x19:
        if (((s32) (self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1)) {
            em008_effect_spawn(self, 0, 0xE, 0x25, 0, 1.0f);
            em008_effect_spawn(self, 0, 0xE, 0x1D, 0, 1.0f);
        }
        break;
    case 0x1B:
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 200.0f);
            eft_em_spawn(self, 0x12, 0x14, &vec, 1.0f);
            em_camera_req(self, 0x14, 7);
        }
        break;
    case 0x1C:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0x28, 0x25, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 64.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.4f);
        }
        break;
    case 0x21:
        if ((em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1)) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.2f);
        }
        break;
    case 0x23:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x14, 0xFF, 0, 1.4f);
                break;
            }
            em008_effect_spawn(self, 1, 4, 0xFF, 0, 0.4f);
        }
        break;
    case 0x24:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, -50.0f, 0.0f);
            eft_em_spawn(self, 0xB, 0x21, &vec, 1.0f);
            setVector3(&vec, 0.0f, -50.0f, 100.0f);
            eft_em_spawn(self, 0xB, 0x29, &vec, 1.0f);
        }
        if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 0x1C, 0, 0.8f);
            eft009_spawn_at_joint(self, 3, 0x1C, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 120.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 75.0f, 75.0f);
            eft_em_spawn(self, 0xA, 0x14, &vec, 1.0f);
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0xA, 3, &vec, 1.0f);
            eft_em_spawn(self, 0xA, 0x2C, &vec, 1.0f);
            eft009_spawn_at_joint(self, 3, 0x1D, 0, 0.8f);
            setVector3(&vec, 0.0f, 0.0f, 400.0f);
            eft009_spawn_at_pos(self, 3, 0x1D, 0, 0.8f, &vec);
        }
        break;
    case 0x26:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            setVector3(&vec, -100.0f, 0.0f, 100.0f);
            eft009_spawn_at_pos(self, 3, 0x1E, 0xC000, 0.5f, &vec);
            em_camera_req(self, 3, 7);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            setVector3(&vec, 100.0f, 0.0f, 100.0f);
            eft009_spawn_at_pos(self, 3, 0x1E, 0xC000, 0.5f, &vec);
            em_camera_req(self, 3, 7);
        }
        if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            setVector3(&vec, -100.0f, 0.0f, 300.0f);
            eft009_spawn_at_pos(self, 3, 0x20, 0, 0.7f, &vec);
            em_camera_req(self, 3, 7);
        }
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            setVector3(&vec, 200.0f, 0.0f, 300.0f);
            eft009_spawn_at_pos(self, 3, 0x1E, 0, 1.0f, &vec);
            em_camera_req(self, 3, 7);
        }
        if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            setVector3(&vec, 0.0f, 0.0f, 300.0f);
            eft009_spawn_at_pos(self, 3, 0x20, 0, 0.7f, &vec);
            em_camera_req(self, 3, 7);
        }
        if ((em_after_frame_check(self, 3, 128.0f, 248.0f) == 1) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft009_spawn_at_pos(self, 3, 0x1F, 0, 1.0f, &vec);
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x27:
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 0x1A, 3, 0, 1.0f);
            em_camera_req(self, 3, 1);
        }
        if (em_after_frame_check(self, 0, 148.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.4f);
        }
        if (((s32) (self->field_0x228 & 6) != 0) && (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1)) {
            setVector3(&vec, 0.0f, 50.0f, 100.0f);
            eft_em_spawn(self, 0x18, 0x14, &vec, 1.0f);
        }
        break;
    case 0x64:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.6f);
        }
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.6f);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, 18.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.2f);
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.6f);
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.6f);
        }
        break;
    case 0x68:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.8f);
            } else {
                em008_effect_spawn(self, 1, 4, 3, 0, 0.8f);
            }
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x69:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.8f);
            } else {
                em008_effect_spawn(self, 1, 4, 3, 0, 0.8f);
            }
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.6f);
        }
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.6f);
        }
        break;
    case 0x6C:
        if ((s32) (self->field_0x228 & 6) != 0) {
            if (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0x10, 3, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.6f);
                em_camera_req(self, 3, 7);
            }
            if (em_after_frame_check(self, 0, 382.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 0, 0x10, 3, 0, 0.6f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.4f);
                break;
            }
        } else {
            if (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 1, 1, 3, 0, 0.6f);
                em_camera_req(self, 3, 7);
            }
            if (em_after_frame_check(self, 0, 382.0f, 0.0f) == 1) {
                em008_effect_spawn(self, 1, 1, 3, 0, 0.6f);
                break;
            }
        }
        break;
    case 0x6D:
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.6f);
            } else {
                em008_effect_spawn(self, 1, 4, 3, 0, 0.8f);
            }
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x70:
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.6f);
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.6f);
        }
        break;
    case 0x75:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x14, 0xFF, 0, 1.5f);
            } else {
                em008_effect_spawn(self, 1, 2, 0xFF, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x77:
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x10, 0xFF, 0, 1.0f);
            } else {
                em008_effect_spawn(self, 1, 2, 0xFF, 0, 0.8f);
            }
            em_camera_req(self, 3, 7);
        }
        break;
    case 0x7A:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x11, 3, 0, 1.0f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.6f);
            } else {
                em008_effect_spawn(self, 1, 4, 3, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) {
            if ((s32) (self->field_0x228 & 6) != 0) {
                em008_effect_spawn(self, 0, 0x11, 3, 0, 0.6f);
                em008_effect_spawn(self, 0, 0xE, 3, 0, 1.2f);
            } else {
                em008_effect_spawn(self, 1, 4, 3, 0, 0.8f);
            }
        }
        if (em_after_frame_check(self, 0, 264.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.4f);
        }
        break;
    case 0x7B:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x25, 0, 1.4f);
        }
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em008_effect_spawn(self, 0, 4, 0x1D, 0, 1.4f);
        }
        break;
    }
}

/* 0x80165960 (0x4): the empty TEV colour slot. */
void em008_tev_color_update(_ENEMY_WORK* self) {
}

/* 0x80165964 (0x25C): the program's state query: `kind` 0 the alt-mode stage, 1 the part pair, 2 the nearest of
 * three area points when it is far enough, 3 the area-4/6 event stage, 4 the +0x32A countdown run out. */
u32 em008_state_query(_ENEMY_WORK* self, u8 kind) {
    VEC3 vec;
    f32 best;
    u8 near;
    s32 i;

    switch (kind) {
    case 0:
        if (self->field_0x1E2 == 4) {
            return (em_flag_0x8AD_ck_c1(self) == 1) + 1;
        }
        return 0;
    case 1:
        {
            u8 pair = em008_part_pair_ck(self) == 1;
            return pair;
        }
    case 2:
        if (stage_map_kind_get(self->field_0x1E0) == 2 && self->area_no == 3) {
            VEC3_ctor(&vec);
            for (i = 0; i < 3; i++) {
                em_data_point_get(self, ((self->area_no & 0xF) << 8) | (u8)(i + 7), &vec);
                if (i == 0) {
                    best = calcDistanceSqXZ(&self->pos, &vec);
                    near = i;
                } else {
                    f32 dist = calcDistanceSqXZ(&self->pos, &vec);
                    if (best > dist) {
                        best = dist;
                        near = i;
                    }
                }
            }
            switch (near) {
            case 0:
                if (best > 360000.0f) {
                    return 0;
                }
                return 0xFE;
            case 1:
                if (best > 1210000.0f) {
                    return 1;
                }
                return 0xFE;
            case 2:
                if (best > 810000.0f) {
                    return 2;
                }
                return 0xFE;
            }
        } else {
            return 0xFF;
        }
        break;
    case 3:
        if (stage_map_kind_get(self->field_0x1E0) == 2) {
            switch (self->area_no) {
            case 4:
                return (stg_area_flag_index_ck(3) == 1) + 1;
            case 6:
                return (stg_area_flag_index_ck(4) == 1) + 1;
            }
        }
        break;
    case 4:
        return (u8)(self->timer_0x32A <= 0) & 0xFF;
    default:
        return 0;
    }
    return 0;
}

/* 0x80165BC0 (0x8): the action-allowed query: always 1. */
s32 em008_action_ok_ck(_ENEMY_WORK* self) {
    return 1;
}

/* 0x80165BC8 (0x4C): the death action: move mode 4 and action 0xC sub-state 0. */
void em008_death_action_get(_ENEMY_WORK* self, u8* out_state, u8* out_flags) {
    em_move_mode_set(self, 4);
    *out_state = 0xC;
    *out_flags = 0;
}

/* 0x80165C14 (0xC): clears the +0x328 seat countdown. */
void em008_aim_reset(_ENEMY_WORK* self) {
    self->field_0x328 = 0;
}

/* 0x80165C20 (0x44): are parts 3 and 4 in the alternate mode. */
s32 em008_alt_part_ck(_ENEMY_WORK* self, u8 arg1) {
    switch (arg1) {
    case 3:
    case 4:
        if (em_alt_mode_ck(self) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80165C64 (0x6C): is the part breakable now: parts 0-2 while their damage level is even, parts 3-4 while the
 * effect is active. */
s32 em008_part_break_ck(_ENEMY_WORK* self, u8 part) {
    switch (part) {
    case 0:
    case 1:
    case 2:
        if ((em_parts_damage_level_get_view1(self, part) & 1) == 0) {
            return 1;
        }
        break;
    case 3:
    case 4:
        if (em_eft_active_ck(self) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x80165CD0 (0x2F8): breaks part `part` once: sets its flag bit, spawns the break effect and drops the item. */
void em008_part_break_set(_ENEMY_WORK* self, u8 part) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (part) {
    case 0:
        if (em008_part_flag_clear_ck(self, 1) == 1) {
            self->field_0x1E4 |= 1;
            setVector3(&vec, 0.0f, 50.0f, 150.0f);
            eft_em_spawn_tinted(self, 1, 0x14, &vec, 1.0f, -1);
            em_drop_item_spawn(self, &self->pos, 0x1E);
        }
        break;
    case 1:
        if (em008_part_flag_clear_ck(self, 2) == 1) {
            self->field_0x1E4 |= 2;
            setVector3(&vec, 0.0f, -200.0f, 0.0f);
            eft_em_spawn_tinted(self, 1, 4, &vec, 1.2f, -1);
            setVector3(&vec, 0.0f, 100.0f, 0.0f);
            eft_em_spawn_tinted(self, 1, 4, &vec, 1.0f, -1);
            em_drop_item_spawn(self, &self->pos, 0x1E);
        }
        break;
    case 2:
        if (em008_part_flag_clear_ck(self, 4) == 1) {
            self->field_0x1E4 |= 4;
            setVector3(&vec, 0.0f, -200.0f, 100.0f);
            eft_em_spawn_tinted(self, 1, 4, &vec, 1.0f, -1);
            em_drop_item_spawn(self, &self->pos, 0x1E);
        }
        break;
    case 3:
        if (em008_part_flag_clear_ck(self, 8) == 1) {
            self->field_0x1E4 |= 8;
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_tinted(self, 1, 0x23, &vec, 1.0f, -1);
            em_drop_item_spawn(self, &self->pos, 0x1E);
        }
        break;
    case 4:
        if (em008_part_flag_clear_ck(self, 0x10) == 1) {
            self->field_0x1E4 |= 0x10;
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_tinted(self, 1, 0x1B, &vec, 1.0f, -1);
            em_drop_item_spawn(self, &self->pos, 0x1E);
        }
        break;
    case 5:
        if (em008_part_flag_clear_ck(self, 0x20) == 1) {
            self->field_0x1E4 |= 0x20;
            setVector3(&vec, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_tinted(self, 1, 0x2D, &vec, 1.0f, -1);
            em_drop_item_spawn(self, &self->pos, 0x1E);
        }
        break;
    }
}

/* 0x80165FC8 (0x1F4): the area-start check: the stun move, the area break, the area pick on the stage's kind-2 map,
 * the roll cooldown and the roam/leave picks. */
u32 em008_area_start_ck(_ENEMY_WORK* self, u32 arg1) {
    s16* seat = &self->field_0x328;
    u8 entry;
    u32 sel;

    get_move_work_adrs(3);
    get_move_work_max(3);
    entry = stage_map_kind_get(self->field_0x1E0);
    if ((s32)entry != 2) {
        return 0;
    }
    if (em_area_stun_move_ck(self) == 1) {
        return 1;
    }
    if ((s32)entry == 2) {
        sel = 3;
    } else {
        sel = 0xFF;
    }
    if (sel != 0xFF && (s32)em_area_break_ck(self) == 2) {
        return 1;
    }
    if (em_mot_finished_ck_view1(self) == 0 && self->value_0x452 >= 0x384) {
        u32 motion;
        if ((s32)self->area_no != 6) {
            if (stg_area_flag_index_ck(3) == 1) {
                motion = 6;
            } else {
                motion = 4;
            }
        } else {
            motion = 6;
        }
        if (em_pack_area_pick(self, 0, motion, (u16)arg1, 0, em008_area_pick_tbl) == 1) {
            return 1;
        }
    }
    if (em_mot_finished_ck_view1(self) == 0 && self->value_0x452 >= 0x384 && em008_part_flags_count(self) <= 2 && (s32)entry == 2
        && self->area_no != 3 && em_roll_cooldown_ck(self, (u16)arg1, 0x1E, seat) == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 3;
        return 1;
    }
    if (em_area_roam_pick(self, (u16)arg1) == 1) {
        return 1;
    }
    return em_area_leave_ck(self) == 1;
}

/* 0x801661BC (0x40): Sets the accessor's flag word to 7 once it is installed. */
void Em008UserData::Attach() {
    nw4r::g3d::ResNode pad((void*)0);
    fn_8013A654_view1(this, 7);
}

/* 0x801661FC (0xD8): Rotates node 0x18 of the model by the work's +0x608/+0x610 angles. */
void Em008UserData::CalcWorldEnd(nw4r::math::MTX34* mtx, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj) {
    nw4r::g3d::ResNode head((void*)0);
    s32 idx;
    MTX34 out;
    MTX34 local;

    (void)pFuncObj;
    MTX34_ctor(&out);
    MTX34_ctor(&local);
    idx = (s32)(u32)mdl.GetResNode(0x18).mpData;
    res_node_assign(&head, &idx);
    {
        _ENEMY_WORK* work = this->work;
        mtx34_copy(&local, &mtx[fn_8006FDCC(&head)]);
        mtx34_identity(&out);
        rotMatrixX_view1(work->field_0x608, &out);
        rotMatrixZ_view1(work->field_0x610, &out);
        mtx34_concat_assign(&local, &out);
        copyMat33(&mtx[fn_8006FDCC(&head)], &local);
    }
}

/* 0x801662D4 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em008UserData::~Em008UserData() {
}

/* 0x80166330 (0xB4): the static initialiser: seeds the two sound shift records. */
void em008_static_init(void) {
    VEC3 v0;
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    /* `setVec3` returns its first argument, so the cast back to `VEC3*` costs no instruction. */
    assignVec3_view1(&em008_se_shift_a[0], (VEC3*)setVec3(&v0, 0.0f, 100.0f, 0.0f));
    assignVec3_view1(&em008_se_shift_a[1], (VEC3*)setVec3(&v1, 0.0f, -8.0f, 0.0f));
    assignVec3_view1(&em008_se_shift_b[0], (VEC3*)setVec3(&v2, 0.0f, 0.0f, 0.0f));
    assignVec3_view1(&em008_se_shift_b[1], (VEC3*)setVec3(&v3, 0.0f, -8.0f, 0.0f));
}

/* The `.ctors` word (0x8056F328) the split assigns to this unit - it points at the static initialiser. */
__declspec(section ".ctors") void* const em008_ctor = (void*)em008_static_init;

/* The unit's `.bss`: the two two-vector shift records `em008_static_init` seeds (GUESS names). */
nw4r::math::VEC3 em008_se_shift_a[2];
nw4r::math::VEC3 em008_se_shift_b[2];
