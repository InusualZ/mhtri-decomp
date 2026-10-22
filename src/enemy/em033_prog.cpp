/*
 * enemy/em033_prog.cpp - enemy 033's program: the `em033_prog_tbl` entry points (reset, init, frame update, action
 *   dispatch, event hook, colour update, state query, area start), the per-action state steps the `state_sub`
 *   dispatchers select, the two sound programs, the roster target pick and its gates, the user-data helper's
 *   callbacks, and the head of em035's program (`em035_init`/`em035_noop`/`em035_event_notify`).
 * RANGE. .text 0x8035BAB4-0x8035F2B4 (55 functions); extab 0x80017494-0x800175DC, extabindex 0x80036D44-0x80036F30,
 *   .rodata 0x805709D0-0x80570A10 (the turn sequence), .data 0x805ED370-0x805ED838 (`em033_prog_tbl` first, then the
 *   dispatchers' switch tables, the sound programs and the user-data class table), .sdata 0x80793340-0x80793348
 *   (the two turn rates), .sdata2 0x8079B668-0x8079B71C (the float pool, from literals).
 * SEAM. The `.data` order proves 0x8035BAB4-0x8035E034 is em033's own code: the switch tables of
 *   `em033_event_notify`, `em033_act1_dispatch` and `em033_act2_dispatch` follow `em033_prog_tbl`, so the band is not
 *   the tail of `ef/eft052.cpp`.  0x8035F060-0x8035F2B4 is the head of em035's program: `em035_prog_tbl` opens
 *   `enemy/em035_prog.cpp`'s `.data` (0x805ED838) and the `.sdata2` run turns there; the three functions stay here
 *   while that unit is Matching (seam request #24).
 * FLAGS. `cflags_main` with `#pragma peephole off` and `#pragma fp_contract off` file-wide (the em027/em029
 *   program flags).
 * NAMES. The file name follows the runtime dump's `em033_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em033ProgTbl` slot each fills, after `Em027ProgTbl`): em033_reset, em033_init, em033_frame_update,
 *   em033_action_dispatch, em033_noop, em033_event_notify, em033_motion_events, em033_tev_color_update,
 *   em033_state_ck, em033_event_request, em033_area_start_set.
 *   GUESS (dispatcher slot): em033_act<action>_<first sub-state> and em033_act<action>_dispatch, em033_act10_se_play,
 *   em033_act11_se_play; GUESS (from the bodies): em033_hold_kind_get, em033_color_update, em033_roster_pick,
 *   em033_area_roster_ck, em033_record_area_ck, em033_target_lock, em033_target_track, em033_aim_sync,
 *   em033_point_get; `Em033UserData` (the accessor whose table is 0x805ED808) follows `enemy/res_user_data_ac.h`.
 *   GUESS (data): em033_turn_seq, em033_turn_rate_a/_b, em033_se_tbl_mot<N> / em033_se_rec_mot<N>_<x>,
 *   em033_se_tbl_/_rec_/_branch_act11_<n>; `_ENEMY_WORK::em033_0x328` (`hold_kind_0x33A`, `record_0x33B`, ...).
 * RESIDUALS.
 *   5 rows short: `em033_event_notify` - retail tests hold kinds 1..3 as one range (`subi`/`cmplwi 2`), ours
 *   compares 1 and 3 (a switch with and without `default`, and three `if` shapes measured lower);
 *   `em033_roster_pick`, `em033_area_roster_ck` - the loop's pointer, record and count registers colour
 *   differently (a declaration hill climb took 98.62 to 98.80); `em033_color_update` - the step and the target
 *   byte trade r4/r5 (two product orders measured the same); `em033_act2_3` - the two by-value move-vector
 *   copies take each other's stack slot.
 *   `.sdata2`: the last pool word (0.8f, `em035_init`'s) follows a zero alignment word in retail: the em035 functions'
 *   pool starts em035's own object, which is the seam above.
 *   flipcheck: the callee `em_team_area_alert_set` (registry proposal for 0x8012C300, `enemy/em_common.cpp`) is
 *   not a map name yet.
 * SHAPES. A case-level `return` at the end of a state switch costs a `b`: the steps `break`.  Retail compares most
 *   bytes signed: `(s32)` casts, and the area gates read the area into an `s32` (`area < 1 || area > 4` is retail's
 *   `subi`/`cmplwi`).  A record id stored to `record_0x33B` is re-read from it for the next call (retail's `clrlwi`).
 *   `em_move_vec_set` takes its vector by value (`em_move_vec_set_c1`).  The sound programs are
 *   defined after `em033_act11_se_play` (declared `extern` above) so the dispatchers load each table by name and the
 *   switch tables keep their places around them.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "mh3_pad.h"
#include "mh3_pad/system_w.h"
#include "fn_8004CAD8.h"
#include "fn_80047398.h"
#include "sound/mhchar.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "MSL/new.h" /* the placement `operator new` */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "enemy/fn_80147CE0.h"
#include "enemy/em_model.h"
#include "enemy/em_pop.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_turn_seq_start.h"
#include "enemy/em_turn_seq_step.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/em035_init.h"
#include "enemy/em_team_area_alert_set.h"
#include "ef/eft007.h"
#include "ef/vec3_div.h"
#include "ef/ef_postfield.h"
#include "ef/fn_800CDB2C.h"
#include "g3d/anm_chr_result_scale_get.h"
#include "Pl/pl_coll.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "stage/stg_area_flag_index_ck.h"
#include "stage/shell_set_func_ptr.h"
#include "vec3_scale.h"
#include "unsplit/enemy.h"

#pragma peephole off
#pragma fp_contract off

/* `em_move_vec_set` with its vector passed by value (the caller copies it word by word): a cast call is the same
 * direct call. */
#define em_move_vec_set_c1 ((void (*)(struct _ENEMY_WORK*, Vec))em_move_vec_set)

/* The +0x3C shell-table slot spelled without its trailing table argument: the call site passes none. */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

/* The user-data accessor `em033_init` installs: the base accessor with its own destructor, attach hook and
 * animation item hook (its table is `.data` 0x805ED808).  size: 0xC (`operator new(0xC)`) */
class Em033UserData : public ResUserDataAc {
public:
    Em033UserData();
    virtual ~Em033UserData();
    virtual void Attach();
    virtual void CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
};

/* The work block the aim writer reads.  Only the two fields the body touches are named. */
struct EmAimSource {
    /* +0x000 */ u8 pad_0x000[0xA];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[0x348 - 0xB];
    /* +0x348 */ nw4r::math::VEC3 field_0x348;
}; /* size: 0x354 */

/* The em033 program table (`em033_prog_tbl`): the entry points the shared enemy program driver calls.  Slot names
 * follow `Em027ProgTbl` (GUESS).
 * size: 0x6C */
struct Em033ProgTbl {
    /* +0x00 */ void (*reset_0x00)(struct _ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*noop_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ u8 (*state_ck_0x24)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u8 (*event_request_0x28)(void);
    /* +0x2C */ u32 pad_0x2C;
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ u32 pad_0x34[14];
};

extern "C" {

void em033_reset(struct _ENEMY_WORK* self);
void em033_init(struct _ENEMY_WORK* self, u8 mode);
void em033_noop(void);
void em033_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub);
void em033_frame_update(struct _ENEMY_WORK* self);
void em033_act0_0(struct _ENEMY_WORK* self);
void em033_act0_dispatch(struct _ENEMY_WORK* self);
void em033_act1_0(struct _ENEMY_WORK* self, u8 reset);
void em033_act1_1(struct _ENEMY_WORK* self, u8 variant);
void em033_act1_2(struct _ENEMY_WORK* self, u8 kind);
void em033_act1_3(struct _ENEMY_WORK* self, u8 variant);
void em033_act1_5(struct _ENEMY_WORK* self);
void em033_act1_13(struct _ENEMY_WORK* self);
void em033_act1_14(struct _ENEMY_WORK* self);
void em033_act1_dispatch(struct _ENEMY_WORK* self);
void em033_act2_0(struct _ENEMY_WORK* self, u8 fast);
void em033_act2_1(struct _ENEMY_WORK* self, u8 reach, u8 fast);
void em033_act2_2(struct _ENEMY_WORK* self);
void em033_act2_3(struct _ENEMY_WORK* self);
void em033_act2_4(struct _ENEMY_WORK* self);
void em033_act2_5(struct _ENEMY_WORK* self);
void em033_act2_6(struct _ENEMY_WORK* self, u8 speed);
void em033_act2_dispatch(struct _ENEMY_WORK* self);
void em033_act7_0(struct _ENEMY_WORK* self, u8 lunge);
void em033_act7_1(struct _ENEMY_WORK* self, u8 lunge);
void em033_act7_2(struct _ENEMY_WORK* self);
void em033_act7_dispatch(struct _ENEMY_WORK* self);
void em033_act10_se_play(struct _ENEMY_WORK* self);
void em033_act11_se_play(struct _ENEMY_WORK* self);
void em033_act12_0(struct _ENEMY_WORK* self);
void em033_act12_1(struct _ENEMY_WORK* self);
void em033_act12_2(struct _ENEMY_WORK* self);
void em033_act12_dispatch(struct _ENEMY_WORK* self);
void em033_action_dispatch(struct _ENEMY_WORK* self);
u8 em033_hold_kind_get(struct _ENEMY_WORK* self);
void em033_color_update(struct _ENEMY_WORK* self);
void em033_motion_events(struct _ENEMY_WORK* self);
void em033_tev_color_update(struct _ENEMY_WORK* self);
u8 em033_state_ck(_ENEMY_WORK* self, u8 mode);
void em033_area_start_set(_ENEMY_WORK* self, u8* out_a, u8* out_b);
u8 em033_event_request(void);
u8 em033_roster_pick(_ENEMY_WORK* self, u8 a);
u32 em033_area_roster_ck(u8 a);
u32 em033_record_area_ck(u8 a, u8 b);
void em033_target_lock(_ENEMY_WORK* self, u8 a);
void em033_target_track(_ENEMY_WORK* self);
void em033_aim_sync(_ENEMY_WORK* self);
void em033_point_get(_ENEMY_WORK* self, u8 a, VEC3* out);
void em035_init(_ENEMY_WORK* self, u8 a);
void em035_noop(void);
void em035_event_notify(_ENEMY_WORK* self, u8 a, u8 b);

extern EmSeEntry em033_se_tbl_mot0[];
extern EmSeEntry em033_se_tbl_mot122[];
extern EmSeEntry em033_se_tbl_mot123[];
extern EmSeEntry em033_se_tbl_mot124[];
extern EmSeEntry em033_se_tbl_mot125[];
extern EmSeEntry em033_se_tbl_mot159[];
extern EmSeEntry em033_se_tbl_mot168[];
extern EmSeEntry em033_se_tbl_act11_0[];
extern EmSeEntry em033_se_tbl_act11_29[];
extern EmSeEntry em033_se_tbl_act11_56[];
extern EmSeEntry em033_se_tbl_act11_57[];
extern EmSeEntry em033_se_tbl_act11_58[];

Em033ProgTbl em033_prog_tbl = {
    em033_reset,
    em033_init,
    em033_frame_update,
    em033_action_dispatch,
    em033_noop,
    em033_event_notify,
    em033_motion_events,
    0,
    em033_tev_color_update,
    em033_state_ck,
    em033_event_request,
    0,
    em033_area_start_set,
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

/* The `em_turn_seq_start`/`em_turn_seq_step` table (`.rodata`, 0x40 B): a four-word head, then three four-word
 * rows whose first word carries the motion in its high half. */
const u32 em033_turn_seq[16] = {
    0x00000000, 0x00000046, 0x00000014, 0x40001000,
    0x00020000, 0x00000004, 0x00000000, 0x00000000,
    0x00050000, 0x00000002, 0x00000000, 0x00000000,
    0x00040000, 0x00000002, 0x00000000, 0x00000000,
};

/* The turn rates the two lunging attack steps hand `em_turn_to_target` (`.sdata`). */
s32 em033_turn_rate_a = 0x80;
s32 em033_turn_rate_b = 0x80;

void em033_reset(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 one;

    self->em033_0x328.color_0x340[2] = 0x7F;
    self->em033_0x328.color_0x340[1] = 0x7F;
    self->em033_0x328.color_0x340[0] = 0x7F;
    copyVec3(&self->em033_0x328.scale_0x348, setVec3(&one, 1.0f, 1.0f, 1.0f));
    self->em033_0x328.color_mode_0x339 = 0;
    self->em033_0x328.color_step_0x338 = 0;
    self->em033_0x328.timer_0x328 = 0;
    self->em033_0x328.record_0x33B = 0xFF;
    self->em033_0x328.hit_prev_0x33D = 0;
    self->em033_0x328.attack_kind_0x33E = 0;
    self->em033_0x328.phase_0x33F = 0;
}

void em033_init(struct _ENEMY_WORK* self, u8 mode) {
    Em033UserData* data;

    if ((s32)self->field_0x00A == 1) {
        self->field_0x1CC = 1.25f;
        self->field_0x1C8 = 0x5000;
        em_max_hp_set(self, 100);
    }
    if ((s32)mode == 2) {
        self->field_0x1CC = 1.1f;
        em_act_arm_unless_down(self, 11, 58);
        em_state_refresh(self);
    }
    if (em_res_user_data_ck(self) == 0) {
        data = (Em033UserData*)operator new(sizeof(Em033UserData));
        new (data) Em033UserData;
        em_res_user_data_set(self, data);
    }
}

} /* extern "C" */

/* 0x8035BBE8 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em033UserData::Em033UserData() {
}

extern "C" {

void em033_noop(void) {
}

void em033_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub) {
    EmPopRec* rec;
    u8 id;
    u8 hold;

    switch (kind) {
    case 1:
        switch (sub) {
        case 2:
            rec = em_roster_record_get(self->em033_0x328.record_0x33B);
            if (rec != NULL) {
                switch (rec->field_0x005) {
                case 1:
                    self->em033_0x328.hold_kind_0x33A = 2;
                    break;
                case 5:
                    self->em033_0x328.hold_kind_0x33A = 3;
                    break;
                case 15:
                    self->em033_0x328.hold_kind_0x33A = 1;
                    break;
                case 0:
                case 4:
                case 14:
                    self->em033_0x328.hold_kind_0x33A = 4;
                    break;
                default:
                    self->em033_0x328.hold_kind_0x33A = 0;
                    break;
                }
            } else {
                self->em033_0x328.hold_kind_0x33A = 0;
            }
            break;
        case 3:
            hold = self->em033_0x328.hold_kind_0x33A;
            switch (hold) {
            case 1:
            case 2:
            case 3:
                self->field_0x1E4 = 1;
                break;
            case 4:
                self->field_0x1E4 = 2;
                self->em033_0x328.phase_0x33F = 3;
                break;
            case 0:
                self->field_0x1E4 = 0;
                self->em033_0x328.phase_0x33F = 0;
                em_motion_kind_set(self, 0, 0);
                break;
            default:
                break;
            }
            break;
        case 4:
            id = em033_roster_pick(self, 0);
            if (id != 0xFF) {
                self->em033_0x328.record_0x33B = id;
                copyVec3(&self->vec_0x36C, em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no));
                em_reaction_req(self, 6, 0xFF, 0);
                em_motion_kind_set(self, 5, 1);
            }
            break;
        case 6:
            self->em033_0x328.attack_kind_0x33E = 1;
            id = em033_roster_pick(self, 0);
            if (id != 0xFF) {
                self->em033_0x328.record_0x33B = id;
                copyVec3(&self->vec_0x36C, em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no));
                em_reaction_req(self, 6, 0xFF, 0);
                em_motion_kind_set(self, 5, 1);
            }
            break;
        case 7:
            self->em033_0x328.attack_kind_0x33E = 2;
            id = em033_roster_pick(self, 0);
            if (id != 0xFF) {
                self->em033_0x328.record_0x33B = id;
                copyVec3(&self->vec_0x36C, em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no));
                em_reaction_req(self, 6, 0xFF, 0);
                em_motion_kind_set(self, 5, 1);
            }
            break;
        case 8:
            self->em033_0x328.attack_kind_0x33E = 0;
            id = em033_roster_pick(self, 0);
            if (id != 0xFF) {
                self->em033_0x328.record_0x33B = id;
                copyVec3(&self->vec_0x36C, em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no));
                em_reaction_req(self, 6, 0xFF, 0);
                em_motion_kind_set(self, 5, 1);
            }
            break;
        case 9:
            self->em033_0x328.attack_kind_0x33E = 0;
            id = em033_roster_pick(self, 1);
            if (id != 0xFF) {
                self->em033_0x328.record_0x33B = id;
                if (em033_record_area_ck(self->em033_0x328.record_0x33B, self->area_no) == 1) {
                    self->em033_0x328.phase_0x33F = 1;
                    em033_target_track(self);
                }
            }
            break;
        case 10:
            em033_target_track(self);
            break;
        case 16:
            self->em033_0x328.attack_kind_0x33E = 1;
            em033_target_track(self);
            break;
        case 17:
            self->em033_0x328.attack_kind_0x33E = 2;
            em033_target_track(self);
            break;
        case 18:
            self->em033_0x328.attack_kind_0x33E = 0;
            em033_target_track(self);
            break;
        }
        break;
    }
}

void em033_frame_update(struct _ENEMY_WORK* self) {
    em033_aim_sync(self);
    if ((s32)self->field_0x00A != 1) {
        if (em_die_ck(self) == 0 && self->field_0x1E4 != 1 && (system_w.field_0x0c & 0x1F) == self->group) {
            if ((s32)self->em033_0x328.phase_0x33F == 2) {
                if (em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no) == NULL &&
                    self->em033_0x328.hold_kind_0x33A == 0) {
                    self->em033_0x328.record_0x33B = 0xFF;
                    em_hit_by_set(self, ran_suu(0) & 0xF, 0);
                    em_motion_kind_set(self, 0, 0);
                    self->field_0x1E4 = 0;
                    self->em033_0x328.phase_0x33F = 0;
                }
            } else if (self->field_0x43B == 5 && self->field_0x43C == 1 &&
                       em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no) == NULL) {
                if (em033_area_roster_ck(self->area_no) == 1) {
                    em033_target_lock(self, 1);
                } else {
                    self->em033_0x328.record_0x33B = 0xFF;
                    em_hit_by_set(self, ran_suu(0) & 0xF, 0);
                    em_motion_kind_set(self, 0, 0);
                }
            }
        }
        if (self->field_0x833 == 1) {
            em_team_area_alert_set(0x21, self->area_no);
        }
        if (em_die_ck(self) == 0) {
            if (self->field_0x43D == 1 && self->field_0x43D != self->em033_0x328.hit_prev_0x33D) {
                em_hit_by_set(self, ran_suu(0) & 0xF, 0);
                if (self->em033_0x328.phase_0x33F < 3) {
                    self->em033_0x328.phase_0x33F = 0;
                }
            }
            if ((self->field_0x43B == 0 || self->field_0x43B == 5) && self->field_0x43C == 0 &&
                self->field_0x1E4 != 1 && self->action != 12) {
                if (em033_area_roster_ck(self->area_no) == 1) {
                    self->em033_0x328.phase_0x33F = 0;
                    if (self->field_0x43B == 0) {
                        em_hit_by_set(self, ran_suu(0) & 0xF, 0);
                    }
                    em033_target_lock(self, 0);
                } else {
                    em_motion_kind_set(self, 0, 0);
                }
            }
        }
    }
    self->em033_0x328.hit_prev_0x33D = self->field_0x43D;
}

void em033_act0_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em033_act0_0(self);
        break;
    case 1:
        em033_act0_0(self);
        break;
    case 2:
        em033_act0_0(self);
        break;
    }
}

void em033_act1_0(struct _ENEMY_WORK* self, u8 reset) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 14, 0, 0);
        if (reset == 1) {
            self->field_0x1E4 = 0;
            self->em033_0x328.phase_0x33F = 0;
            em_motion_kind_set(self, 0, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act1_1(struct _ENEMY_WORK* self, u8 variant) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 15, 4, 0);
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act1_2(struct _ENEMY_WORK* self, u8 kind) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 12, 4, 0);
        self->timer_0x020 = 118;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            if (kind == 0) {
                em_state_set(self, 1, 3);
            } else {
                em_state_set(self, 1, 12);
            }
        }
        break;
    }
}

void em033_act1_3(struct _ENEMY_WORK* self, u8 variant) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 13, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act1_5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 14, 0, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 160.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 1, 12, 0);
        }
        break;
    case 2:
        if (em_mot_frame_started_ck(self) == 0) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act1_13(struct _ENEMY_WORK* self) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_motion_param_set(self, 0.0f, 30);
        self->timer_0x020 = 30;
        break;
    case 1:
        if (--self->timer_0x020 <= 0 || self->field_0x1D4 <= 0.0f) {
            em_attack_done_set(self);
        }
        break;
    }
}

void em033_act1_14(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 vec;

    VEC3_ctor(&vec);
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        em_motion_param_set(self, 0.0f, 30);
        self->timer_0x020 = 30;
        self->state_0x006 = 0;
        break;
    case 1:
        vec.x = 0.0f;
        vec.y = -1.0f * (2.0f * self->state_0x006);
        vec.z = 0.0f;
        self->state_0x006++;
        em_move_vec_set_c1(self, *(Vec*)&vec);
        if (--self->timer_0x020 <= 0 || self->field_0x1D4 <= 0.0f) {
            em_attack_done_set(self);
        }
        break;
    }
}

void em033_act1_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em033_act1_0(self, 0);
        break;
    case 1:
        em033_act1_1(self, 0);
        break;
    case 2:
        em033_act1_2(self, 0);
        break;
    case 3:
        em033_act1_3(self, 0);
        break;
    case 4:
        em033_act1_1(self, 1);
        break;
    case 5:
        em033_act1_5(self);
        break;
    case 6:
        em033_act1_1(self, 2);
        break;
    case 7:
        em033_act1_1(self, 3);
        break;
    case 8:
        em033_act1_1(self, 4);
        break;
    case 9:
        em033_act1_1(self, 5);
        break;
    case 10:
        em033_act1_1(self, 6);
        break;
    case 13:
        em033_act1_13(self);
        break;
    case 14:
        em033_act1_14(self);
        break;
    case 15:
        em033_act1_0(self, 1);
        break;
    case 16:
        em033_act1_1(self, 7);
        break;
    case 17:
        em033_act1_1(self, 8);
        break;
    case 18:
        em033_act1_1(self, 9);
        break;
    }
}

void em033_act2_0(struct _ENEMY_WORK* self, u8 fast) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (fast == 0) {
            em_mot_speed_set(self, 0.7f);
        } else {
            em_mot_speed_set(self, 1.2f);
        }
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_1(struct _ENEMY_WORK* self, u8 reach, u8 fast) {
    s32 mask;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        self->timer_0x020 = 130;
        switch (fast) {
        case 0:
            em_mot_speed_set(self, 1.0f);
            break;
        case 1:
            em_mot_speed_set(self, 1.6f);
            break;
        }
        switch (reach) {
        case 0:
            em_approach_start(self, -120.0f, 0);
            break;
        case 1:
            em_approach_start(self, -250.0f, 0);
            break;
        default:
            em_approach_start(self, 0.0f, 0);
            break;
        }
        break;
    case 1:
        if (fast == 1) {
            mask = 0x200;
            self->timer_0x020--;
        } else {
            mask = 0x100;
        }
        if (em_approach_step(self, 0, mask) == 1 || self->timer_0x020 <= 0) {
            self->timer_0x020 = 0;
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_2(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 back;
    s32 i;

    VEC3_ctor(&a);
    VEC3_ctor(&dir);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 2, 0);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (self->field_0x218 != 0) {
            for (i = 0; i < 10; i++) {
                if (self->slots_0x244[i].flags & 8) {
                    em_action_finish(self);
                    em_fx_flag_set(self);
                    vec3_scale(&back, &self->slots_0x244[i].vec, -1.0f);
                    copyVec3(&dir, &back);
                    self->field_0x1C0 = calcVecAng3(&dir);
                    break;
                }
            }
            if (em_approach_step(self, 0, 0x80) == 1) {
                em_action_finish(self);
            }
        } else if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_3(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 shift;
    nw4r::math::VEC3 param;
    nw4r::math::VEC3 point;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 step;

    VEC3_ctor(&shift);
    VEC3_ctor(&param);
    VEC3_ctor(&point);
    em_fx_flag_set(self);
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        copyVec3(&param, &self->move_param_0x21C);
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        subVec3(&diff, &param, &self->move_param_0x21C);
        copyVec3(&shift, &diff);
        shift.y = 0.0f;
        rotVecY(&shift, self->field_0x1C0);
        self->pos.x += shift.x;
        self->pos.z += shift.z;
        vec3_scale_in_place(&shift, -1.0f);
        copyVec3(&self->em033_0x328.vec_0x32C, &shift);
        em_move_vec_set_c1(self, *(Vec*)&shift);
        em_mot_set_ck(self, 3, 2, 0);
        self->timer_0x020 = 11;
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            vec3_div(&step, &self->em033_0x328.vec_0x32C, self->timer_0x020);
            vec3_sub_assign((Vec*)&self->em033_0x328.vec_0x32C, (Vec*)&step);
            em_move_vec_set_c1(self, *(Vec*)&self->em033_0x328.vec_0x32C);
            self->timer_0x020--;
        }
        self->field_0x1BC = (u16)(self->field_0x1BC + 0xFC73);
        if (self->field_0x1BC < 0xC000) {
            self->field_0x1BC = 0xC000;
            self->timer_0x020 = 0;
            self->state++;
        }
        break;
    case 2:
        em_frame_flag_set(self);
        self->field_0x1BC = 0xC000;
        em033_point_get(self, 1, &point);
        if (self->pos.y > point.y) {
            self->state++;
            em_move_vec_clr(self);
        }
        break;
    case 3:
        em_frame_flag_set(self);
        em_flag_0x116_set(self);
        setVector3(&shift, 0.0f, 0.0f, 2.0f);
        rotVecY(&shift, self->field_0x1C0);
        copyVec3(&self->offset_0x30C.vec_0x310, &shift);
        em_move_offset_apply(self);
        self->field_0x1BC += 0x38E;
        if (self->field_0x1BC < 0x1000) {
            self->state++;
            em_move_mode_set(self, 0);
            em_move_sub_mode_set(self, 0);
            em_move_offset_apply(self);
            em033_point_get(self, 1, &point);
            self->pos.y = point.y;
            self->pos.z += 10.0f;
            self->field_0x1BC = 0;
            self->em033_0x328.attack_kind_0x33E = 3;
            em_mot_set(self, 3, 2, 0);
            self->timer_0x020 = 20;
        }
        break;
    case 4:
        em_frame_flag_set(self);
        if (--self->timer_0x020 < 0) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_4(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 2, 0);
        em_approach_start(self, -20.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_5(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 point;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    VEC3_ctor(&point);
    em_fx_flag_set(self);
    em_busy_set(self);
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set_ck(self, 3, 2, 0);
        em_move_vec_clr(self);
        self->offset_0x30C.vec_0x310.y = -2.0f;
        self->timer_0x020 = 11;
        break;
    case 1:
        if (self->pos.y > 200.0f + self->field_0x20C) {
            self->state++;
        }
        break;
    case 2:
        em_flag_0x116_set(self);
        self->field_0x1BC += 0x38E;
        em_move_offset_apply(self);
        if (self->field_0x1BC > 0x4000) {
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->field_0x1BC = 0x4000;
            self->timer_0x020 = 0;
            self->state++;
        }
        break;
    case 3:
        self->field_0x1BC = 0x4000;
        em033_point_get(self, 0, &point);
        if (self->pos.y < 50.0f + point.y) {
            self->state++;
        }
        break;
    case 4:
        self->field_0x1BC -= 0x38E;
        if (self->field_0x1BC > 0x10000) {
            em_move_sub_mode_set(self, 0);
            em_move_offset_apply(self);
            self->pos.y = self->field_0x20C;
            self->pos.z += 10.0f;
            self->field_0x1BC = 0;
            self->em033_0x328.attack_kind_0x33E = 4;
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_6(struct _ENEMY_WORK* self, u8 speed) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em033_turn_seq, 0, 1, 0);
        break;
    case 1:
        switch (speed) {
        case 0:
            em_mot_speed_set(self, 0.7f);
            break;
        case 2:
            em_mot_speed_set(self, 1.7f);
            break;
        }
        if (em_turn_seq_step(self, (void*)em033_turn_seq) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em033_act2_0(self, 0);
        break;
    case 1:
        em033_act2_1(self, 0, 0);
        break;
    case 2:
        em033_act2_2(self);
        break;
    case 3:
        em033_act2_3(self);
        break;
    case 4:
        em033_act2_4(self);
        break;
    case 5:
        em033_act2_5(self);
        break;
    case 6:
        em033_act2_6(self, 0);
        break;
    case 7:
        em033_act2_1(self, 1, 0);
        break;
    case 8:
        em033_act2_6(self, 1);
        break;
    case 9:
        em033_act2_0(self, 1);
        break;
    case 10:
        em033_act2_1(self, 2, 0);
        break;
    case 11:
        em033_act2_1(self, 0, 1);
        break;
    case 12:
        em033_act2_1(self, 1, 1);
        break;
    case 13:
        em033_act2_1(self, 2, 1);
        break;
    case 14:
        em033_act2_6(self, 2);
        break;
    }
}

void em033_act7_0(struct _ENEMY_WORK* self, u8 lunge) {
    nw4r::math::VEC3 a;

    VEC3_ctor(&a);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 0, 0);
        em_hit_window_set_default(self, 0, 1);
        em_mot_speed_set(self, 1.0f);
        if ((s32)lunge == 1) {
            em_move_vec_clr(self);
            setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, 7.0f);
        }
        break;
    case 1:
        if ((s32)lunge == 1) {
            if (em_frame_check(self, 2, 48.0f, 0.0f) == 1) {
                em_turn_to_target(self, (u16)em033_turn_rate_a);
            }
            if (em_frame_check(self, 3, 64.0f, 90.0f) == 1) {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act7_1(struct _ENEMY_WORK* self, u8 lunge) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 0, 0);
        em_hit_window_set_default(self, 0, 2);
        if ((s32)lunge == 1) {
            em_move_vec_clr(self);
            setVector3(&self->offset_0x30C.vec_0x310, 0.0f, 0.0f, 7.0f);
        }
        break;
    case 1:
        if ((s32)lunge == 1) {
            if (em_frame_check(self, 2, 48.0f, 0.0f) == 1) {
                em_turn_to_target(self, (u16)em033_turn_rate_b);
            }
            if (em_frame_check(self, 3, 58.0f, 70.0f) == 1) {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act7_2(struct _ENEMY_WORK* self) {
    EmSpawnRec rec;
    nw4r::math::VEC3 offset;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 11, 0, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 92.0f, 0.0f) == 1) {
            rec.id = 0x17;
            copyVec3(&rec.pos, setVec3(&offset, 0.0f, -10.0f, -20.0f));
            rec.field_0x10 = 0;
            rec.field_0x12 = 0;
            rec.field_0x14 = 0;
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0xF);
            eft007_part_spawn(self, 0x1E, 0, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act7_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em033_act7_0(self, 0);
        break;
    case 1:
        em033_act7_1(self, 0);
        break;
    case 2:
        em033_act7_2(self);
        break;
    case 3:
        em033_act7_0(self, 1);
        break;
    case 4:
        em033_act7_1(self, 1);
        break;
    }
}

void em033_act10_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play(self, em033_se_tbl_mot0, 0, 0);
        break;
    case 23:
        em_se_tbl_play(self, em033_se_tbl_mot0, 0, 23);
        break;
    case 122:
        em_se_tbl_play(self, em033_se_tbl_mot122, 0, 122);
        break;
    case 123:
        em_se_tbl_play(self, em033_se_tbl_mot123, 0, 123);
        break;
    case 159:
        em_se_tbl_play(self, em033_se_tbl_mot159, 0, 159);
        break;
    case 160:
        em_se_tbl_play(self, em033_se_tbl_mot159, 0, 160);
        break;
    case 124:
        em_se_tbl_play(self, em033_se_tbl_mot124, 0, 124);
        break;
    case 141:
        em_se_tbl_play(self, em033_se_tbl_mot159, 0, 141);
        break;
    case 168:
        em_se_tbl_play(self, em033_se_tbl_mot168, 0, 168);
        break;
    case 125:
        em_se_tbl_play(self, em033_se_tbl_mot125, 0, 125);
        break;
    case 142:
        em_se_tbl_play(self, em033_se_tbl_mot159, 0, 142);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

void em033_act11_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_0, 0, 0);
        break;
    case 29:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_29, 0, 29);
        break;
    case 56:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_56, 0, 56);
        break;
    case 57:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_57, 0, 57);
        break;
    case 58:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_58, 0, 58);
        break;
    case 5:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_0, 1, 5);
        break;
    case 30:
        em_se_tbl_play_alt(self, em033_se_tbl_act11_0, 1, 30);
        break;
    }
}

/* The sound-effect programs `em033_act10_se_play`/`em033_act11_se_play` pass to the table walkers. */
EmSeRecord em033_se_rec_mot0_a = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot0[2] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot0_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_mot122_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot122_b = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot122[3] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot122_a},
    {0x00, {0, 0, 0}, &em033_se_rec_mot122_b},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_mot123_a = {103, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot123[2] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot123_a},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_mot124_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot124_b = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot124_c = {103, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot124[4] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot124_a},
    {0x00, {0, 0, 0}, &em033_se_rec_mot124_b},
    {0x00, {0, 0, 0}, &em033_se_rec_mot124_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_mot168_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot168_b = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot168_c = {103, 0, 2, 0, 0, {4, 0, 0, 0}, 0};

EmSeRecord em033_se_rec_mot168_d = {104, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot168[5] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot168_a},
    {0x00, {0, 0, 0}, &em033_se_rec_mot168_b},
    {0x00, {0, 0, 0}, &em033_se_rec_mot168_c},
    {0x00, {0, 0, 0}, &em033_se_rec_mot168_d},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_mot125_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot125_b = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_mot125_c = {103, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot125[4] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot125_a},
    {0x00, {0, 0, 0}, &em033_se_rec_mot125_b},
    {0x00, {0, 0, 0}, &em033_se_rec_mot125_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_mot159_a = {104, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em033_se_tbl_mot159[2] = {
    {0x00, {0, 0, 0}, &em033_se_rec_mot159_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_act11_a = {100, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

extern EmSeBranch em033_se_branch_act11_0;
EmSeEntry em033_se_tbl_act11_0[4] = {
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x52)},
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em033_se_branch_act11_0)},
    {0x00, {0, 0, 0}, &em033_se_rec_act11_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em033_se_tbl_act11_0_b[3] = {
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x64)},
    {0x00, {0, 0, 0}, &em033_se_rec_act11_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeBranch em033_se_branch_act11_0 = {0x04000000, 0, em033_se_tbl_act11_0_b};

EmSeRecord em033_se_rec_act11_29_a = {101, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em033_se_rec_act11_29_b = {102, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em033_se_tbl_act11_29[3] = {
    {0x00, {0, 0, 0}, &em033_se_rec_act11_29_a},
    {0x00, {0, 0, 0}, &em033_se_rec_act11_29_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_act11_56_a = {2, 0, 4, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em033_se_tbl_act11_56[5] = {
    {0x0C, {0, 0, 0}, 0},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x20)},
    {0x00, {0, 0, 0}, &em033_se_rec_act11_56_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em033_se_rec_act11_57_a = {3, 0, 4, 0, 0, {0, 4, 0, 0}, 0};

EmSeEntry em033_se_tbl_act11_57[5] = {
    {0x0C, {0, 0, 0}, 0},
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x20)},
    {0x00, {0, 0, 0}, &em033_se_rec_act11_57_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em033_se_tbl_act11_58[3] = {
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x52)},
    {0x00, {0, 0, 0}, &em033_se_rec_act11_a},
    {0xFF, {0, 0, 0}, 0},
};

void em033_act12_0(struct _ENEMY_WORK* self) {
    f32 speed;

    em_fx_flag_set(self);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state = 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        em_approach_start(self, 0.0f, 0);
        speed = 1.25f - 0.07f * (u8)(self->group & 7);
        em_mot_speed_set(self, speed);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em033_act12_1(struct _ENEMY_WORK* self) {
    em_fx_flag_set(self);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->timer_0x020 = 30;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 3, 2, 0);
        break;
    case 1:
        if (--self->timer_0x020 < 0) {
            self->timer_0x020 = 0;
            em_action_finish(self);
        }
        break;
    }
}

void em033_act12_2(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 vec;

    VEC3_ctor(&vec);
    em_fx_flag_set(self);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        self->state++;
        self->timer_0x020 = (self->group & 7) * 10;
        em_flag_0x116_set(self);
        break;
    case 1:
        self->timer_0x020--;
        em_flag_0x116_set(self);
        if (self->timer_0x020 < 0) {
            self->state++;
            self->timer_0x020 = 60;
            vec.x = 0.0f;
            vec.y = -1.0f * (2.0f * self->timer_0x020);
            vec.z = 0.0f;
            em_move_vec_set_c1(self, *(Vec*)&vec);
        }
        break;
    case 2:
        if (--self->timer_0x020 < 0) {
            self->timer_0x020 = 60;
            self->state++;
            break;
        }
        vec.x = 0.0f;
        vec.y = -1.0f * (2.0f * self->timer_0x020);
        vec.z = 0.0f;
        em_move_vec_set_c1(self, *(Vec*)&vec);
        break;
    case 3:
        if (--self->timer_0x020 < 0) {
            self->timer_0x020 = 0;
            em_action_finish(self);
        }
        break;
    }
}

void em033_act12_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em033_act12_0(self);
        break;
    case 1:
        em033_act12_1(self);
        break;
    case 2:
        em033_act12_2(self);
        break;
    }
}

void em033_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em033_act0_dispatch(self);
        break;
    case 1:
        em033_act1_dispatch(self);
        break;
    case 2:
        em033_act2_dispatch(self);
        break;
    case 7:
        em033_act7_dispatch(self);
        break;
    case 10:
        em033_act10_se_play(self);
        break;
    case 11:
        em033_act11_se_play(self);
        break;
    case 12:
        em033_act12_dispatch(self);
        break;
    }
}

u8 em033_hold_kind_get(struct _ENEMY_WORK* self) {
    return self->em033_0x328.hold_kind_0x33A;
}

void em033_color_update(struct _ENEMY_WORK* self) {
    GXColor colour;
    GXColor base = {0x7F, 0x7F, 0x7F, 0x00};
    GXColor hold_1 = {0xF8, 0x97, 0x34, 0x00};
    GXColor hold_4 = {0xAA, 0xB4, 0xA0, 0x00};
    GXColor hold_2 = {0x7F, 0xC3, 0x53, 0x00};
    GXColor hold_3 = {0x37, 0xA6, 0xCC, 0x00};
    nw4r::math::VEC3 one;
    s32 step;
    f32 scale;

    if (self->field_0x1E4 != 0) {
        switch (self->em033_0x328.hold_kind_0x33A) {
        case 1:
            color_rgba_copy((u8*)&colour, (u8*)&hold_1);
            break;
        case 2:
            color_rgba_copy((u8*)&colour, (u8*)&hold_2);
            break;
        case 3:
            color_rgba_copy((u8*)&colour, (u8*)&hold_3);
            break;
        case 4:
            color_rgba_copy((u8*)&colour, (u8*)&hold_4);
            break;
        default:
            color_rgba_copy((u8*)&colour, (u8*)&base);
            color_rgba_copy((u8*)&colour, (u8*)&base);
            self->em033_0x328.timer_0x328 = 0;
            break;
        }
        self->field_0x761 = 2;
        step = self->em033_0x328.timer_0x328;
        scale = 1.0f + 0.15f * step;
        self->em033_0x328.scale_0x348.z = scale;
        self->em033_0x328.scale_0x348.y = scale;
        self->em033_0x328.scale_0x348.x = scale;
        self->em033_0x328.color_0x340[0] = self->em033_0x328.color_target_0x344[0] +
            step * ((colour.r - self->em033_0x328.color_target_0x344[0]) * 0.25f);
        self->em033_0x328.color_0x340[1] = self->em033_0x328.color_target_0x344[1] +
            step * ((colour.g - self->em033_0x328.color_target_0x344[1]) * 0.25f);
        self->em033_0x328.color_0x340[2] = self->em033_0x328.color_target_0x344[2] +
            step * ((colour.b - self->em033_0x328.color_target_0x344[2]) * 0.25f);
        if (step < 4) {
            self->em033_0x328.timer_0x328 = step + 1;
        }
    } else {
        self->field_0x761 = 1;
        self->em033_0x328.timer_0x328 = 0;
        self->em033_0x328.color_mode_0x339 = 0;
        self->em033_0x328.color_step_0x338 = 0;
        copyVec3(&self->em033_0x328.scale_0x348, setVec3(&one, 1.0f, 1.0f, 1.0f));
    }
}

void em033_motion_events(struct _ENEMY_WORK* self) {
    em033_color_update(self);
}

void em033_tev_color_update(struct _ENEMY_WORK* self) {
    GXColor colour;

    if (self->field_0x1E4 != 0) {
        ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR3, &colour);
        colour.r = self->em033_0x328.color_0x340[0];
        colour.g = self->em033_0x328.color_0x340[1];
        colour.b = self->em033_0x328.color_0x340[2];
        ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR3, &colour);
    }
}

/* 0x8035E034 (0x1AC): em033's distance/id query. */
u8 em033_state_ck(_ENEMY_WORK* self, u8 mode) {
    VEC3 a;
    VEC3 b;
    VEC3 c2;
    VEC3 c3;
    VEC3 c5;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    switch (mode) {
    case 0:
        return self->field_0x1E4;
    case 1:
        return self->em033_0x328.attack_kind_0x33E;
    case 2:
        em033_point_get(self, 0, &a);
        subVec3(&c2, &a, &self->pos);
        copyVec3(&b, &c2);
        return vec3_len((const f32*)&b) <= 150.0f;
    case 3:
        em033_point_get(self, 1, &a);
        subVec3(&c3, &a, &self->pos);
        copyVec3(&b, &c3);
        return vec3_len((const f32*)&b) <= 250.0f;
    case 4:
        return self->em033_0x328.phase_0x33F;
    case 5:
        em033_point_get(self, 2, &a);
        subVec3(&c5, &a, &self->pos);
        copyVec3(&b, &c5);
        return vec3_len((const f32*)&b) <= 100.0f;
    case 6:
        return em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no) != 0;
    default:
        return 0;
    }
}

/* 0x8035E1E0 (0x398): em033's action/out-pair selector. */
void em033_area_start_set(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        switch (self->area_no) {
        case 1:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        case 2:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        case 3:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            em_reaction_req(self, 5, 2, 10);
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 4:
            *out_a = 0xC;
            *out_b = 0;
            if (stg_area_flag_index_ck(3) == 1) {
                em_move_target_set(self, 1, 3, 100.0f);
            } else {
                em_move_target_set(self, 0, 2, 100.0f);
            }
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 2;
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
            self->field_0x1C0 = 0x6C00;
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            em_reaction_req(self, 5, 2, 10);
            break;
        }
        break;
    case 3:
        switch (self->area_no) {
        case 5:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        case 6:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        case 9:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            em_reaction_req(self, 5, 2, 10);
            break;
        }
        break;
    case 5:
        switch (self->area_no) {
        case 2:
            *out_a = 0xC;
            *out_b = 0;
            em_move_target_set(self, 0, 2, 100.0f);
            break;
        default:
            *out_a = 0xC;
            *out_b = 0;
            em_reaction_req(self, 5, 2, 10);
            break;
        }
        break;
    default:
        *out_a = 0xC;
        *out_b = 0;
        em_reaction_req(self, 5, 2, 10);
        break;
    }
}

/* 0x8035E578 (0x8): a constant predicate. */
u8 em033_event_request(void) {
    return 1;
}

/* 0x8035E580 (0x29C): the em033 roster scan (select the nearest/valid monster record). */
u8 em033_roster_pick(_ENEMY_WORK* self, u8 a) {
    u8 buf[0x10];
    nw4r::math::VEC3 hit;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 tmp;
    u8* p;
    nw4r::math::VEC3* aim;
    s32 i;
    s32 area;
    f32 len;
    u8 n;
    u8 type;
    u8 best;
    u8 prev;

    VEC3_ctor(&hit);
    VEC3_ctor(&diff);
    best = 0xFF;
    n = em_roster_kind_collect(self->area_no, buf, 0x10);
    if (n > 0x10) {
        return 0xFF;
    }
    for (i = 0, p = buf; i < n; i++, p++) {
        type = em_roster_kind_field5_get(*p, self->area_no);
        if ((s32)a == 0) {
            switch (type) {
            case 5:
            case 1:
            case 15:
                aim = em_roster_kind_aim_pos_get(*p, self->area_no);
                if (aim != NULL) {
                    if (findInterSection(&self->pos, aim, &hit, 1, 0xFFFF, self->area_no, em_hit_mask_get(self), 0) > 0) {
                        subVec3(&tmp, &hit, aim);
                        copyVec3(&diff, &tmp);
                        len = vec3_len((const f32*)&diff);
                        if (len > em_roster_record_get(*p)->radius_0x1F0) {
                            break;
                        }
                    }
                    if (best == 0xFF) {
                        best = *p;
                    } else {
                        switch (type) {
                        case 5:
                            best = *p;
                            prev = type;
                            break;
                        case 1:
                            if (prev != 5 && prev != 1) {
                                best = *p;
                                prev = type;
                            }
                            break;
                        }
                    }
                    if (type == 5) {
                        return best;
                    }
                }
                break;
            }
        } else {
            switch ((u8)stage_map_kind_get(get_now_mapno())) {
            case 1:
                area = self->area_no;
                if (area < 1 || area > 4) {
                    return 0xFF;
                }
                break;
            case 2:
                area = self->area_no;
                if (area != 6 && area != 9) {
                    return 0xFF;
                }
                break;
            case 3:
                area = self->area_no;
                if ((area < 5 || area > 6) && area != 9) {
                    return 0xFF;
                }
                break;
            case 5:
                area = self->area_no;
                if (area != 2) {
                    return 0xFF;
                }
                break;
            default:
                return 0xFF;
            }
            switch (type) {
            case 4:
            case 0:
            case 14:
                if (em_roster_kind_aim_pos_get(*p, area) != NULL) {
                    return buf[i];
                }
                break;
            }
        }
    }
    return best;
}

/* 0x8035E81C (0x168): em033's map/area gate over the roster. */
u32 em033_area_roster_ck(u8 a) {
    u8 buf[0x10];
    s32 area;
    u8 n;
    int i;

    switch ((u8)stage_map_kind_get(get_now_mapno())) {
    case 1:
        area = get_now_areano();
        if (area < 1 || area > 4) {
            return 0;
        }
        break;
    case 2:
        area = get_now_areano();
        if (area != 6 && area != 9) {
            return 0;
        }
        break;
    case 3:
        area = get_now_areano();
        if ((area < 5 || area > 6) && area != 9) {
            return 0;
        }
        break;
    case 5:
        area = get_now_areano();
        if (area != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    n = (u8)em_roster_kind_collect(a, buf, 0x10);
    if (n > 0x10) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        s32 type = (s8)em_roster_kind_field5_get(buf[i], a);
        if (type == 5 || type == 1 || type == 0xF) {
            return 1;
        }
    }
    return 0;
}

/* 0x8035E984 (0x11C): the same gate for one record id. */
u32 em033_record_area_ck(u8 a, u8 b) {
    s32 type;
    s32 area;

    switch ((u8)stage_map_kind_get(get_now_mapno())) {
    case 1:
        area = get_now_areano();
        if (area < 1 || area > 4) {
            return 0;
        }
        break;
    case 2:
        area = get_now_areano();
        if (area != 6 && area != 9) {
            return 0;
        }
        break;
    case 3:
        area = get_now_areano();
        if ((area < 5 || area > 6) && area != 9) {
            return 0;
        }
        break;
    case 5:
        area = get_now_areano();
        if (area != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    type = (s8)em_roster_kind_field5_get(a, b);
    if (type == 4 || type == 0 || type == 0xE) {
        return 1;
    }
    return 0;
}

/* 0x8035EAA0 (0xE0): locks the current target in, or picks a random action. */
void em033_target_lock(_ENEMY_WORK* self, u8 a) {
    u8 id = em033_roster_pick(self, 0);

    if (id != 0xFF) {
        self->em033_0x328.record_0x33B = id;
        copyVec3(&self->vec_0x36C, em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no));
        em_reaction_req(self, 6, 0xFF, 0);
        em_motion_kind_set(self, 5, 1);
    } else {
        self->em033_0x328.record_0x33B = 0xFF;
        if (a == 1) {
            em_hit_by_set(self, ran_suu(0) & 0xF, 0);
            em_motion_kind_set(self, 0, 0);
        } else {
            em_motion_kind_set(self, 5, 0);
        }
    }
}

/* 0x8035EB80 (0x158): keeps tracking the cached record, or drops it. */
void em033_target_track(_ENEMY_WORK* self) {
    nw4r::math::VEC3 hit;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 tmp;
    nw4r::math::VEC3* aim;
    f32 len;

    VEC3_ctor(&hit);
    VEC3_ctor(&diff);
    if (em033_record_area_ck(self->em033_0x328.record_0x33B, self->area_no) == 1) {
        aim = em_roster_kind_aim_pos_get(self->em033_0x328.record_0x33B, self->area_no);
        if (findInterSection(&self->pos, aim, &hit, 1, 0xFFFF, self->area_no, em_hit_mask_get(self), 0) > 0) {
            subVec3(&tmp, &hit, aim);
            copyVec3(&diff, &tmp);
            len = vec3_len((const f32*)&diff);
            if (len > em_roster_record_get(self->em033_0x328.record_0x33B)->radius_0x1F0) {
                return;
            }
        }
        copyVec3(&self->vec_0x36C, aim);
        em_reaction_req(self, 6, 0xFF, 0);
        self->em033_0x328.phase_0x33F = 2;
    } else {
        self->em033_0x328.record_0x33B = 0xFF;
        em_hit_by_set(self, ran_suu(0) & 0xF, 0);
        self->em033_0x328.phase_0x33F = 0;
        em_motion_kind_set(self, 0, 0);
    }
}

/* 0x8035ECD8 (0x168): syncs the aim vector for the current action. */
void em033_aim_sync(_ENEMY_WORK* self) {
    s32 area;

    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        area = self->area_no;
        if (area >= 1 && area <= 4) {
            em_data_point_get(self, (u16)(((area & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 2:
        switch (self->area_no) {
        case 4:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 2), &self->aim);
            break;
        case 6:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 2), &self->aim);
            break;
        case 9:
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | 2), &self->aim);
            break;
        }
        break;
    case 3:
        area = self->area_no;
        if ((area >= 5 && area <= 6) || area == 9) {
            em_data_point_get(self, (u16)(((area & 0xF) << 8) | 2), &self->aim);
        }
        break;
    case 5:
        area = self->area_no;
        if (area == 2) {
            em_data_point_get(self, (u16)(((area & 0xF) << 8) | 2), &self->aim);
        }
        break;
    }
}

/* 0x8035EE40 (0x110): the small per-field aim dispatcher. */
void em033_point_get(_ENEMY_WORK* self, u8 a, VEC3* out) {
    s32 area;

    switch (a) {
    case 0:
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 3) {
            area = self->area_no;
            if (area == 6) {
                em_data_point_get(self, (u16)(((area & 0xF) << 8) | 7), out);
            }
        }
        break;
    case 1:
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 3) {
            area = self->area_no;
            if (area == 6) {
                em_data_point_get(self, (u16)(((area & 0xF) << 8) | 8), out);
            }
        }
        break;
    case 2:
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 3) {
            area = self->area_no;
            if (area == 6) {
                em_data_point_get(self, (u16)(((area & 0xF) << 8) | 5), out);
            }
        }
        break;
    }
}

} /* extern "C" */

/* 0x8035EF50 (0x8): Sets the accessor's flag word to 1 once it is installed. */
void Em033UserData::Attach() {
    em_res_user_data_flag_set((_ENEMY_WORK*)this, 1);
}

/* 0x8035EF58 (0xAC): For an item of kind 0xFF, sets node 0x17's scale from the work's +0x348 vector and scales
 * nodes 5/6 by 1.5 while the work's +0x00A flag is set. */
void Em033UserData::CalcAnmItemEx(nw4r::g3d::ChrAnmResult* b, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 e, UserDataItem* item) {
    VEC3 v;
    struct EmAimSource* p = (struct EmAimSource*)this->work;

    VEC3_ctor(&v);
    if ((s32)item->field_0x04 == 0xFF) {
        switch (e) {
        case 0x17:
            anm_chr_result_scale_set((struct ResAnmChrObj*)b, (f32*)&p->field_0x348);
            break;
        case 5:
        case 6:
            if (((struct EmAimSource*)this->work)->field_0x00A == 1) {
                anm_chr_result_scale_get((struct ResAnmChrObj*)b, (f32*)&v);
                vec3_scale_in_place(&v, 1.5f);
                anm_chr_result_scale_set((struct ResAnmChrObj*)b, (f32*)&v);
            }
            break;
        }
    }
}

/* 0x8035F004 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em033UserData::~Em033UserData() {
}

extern "C" {

/* 0x8035F060 (0x114): the em035 run-state constructor. */
void em035_init(_ENEMY_WORK* self, u8 a) {
    EmRunPos* run = &self->run_0xB04;

    self->field_0x328 = 0x708;
    self->timer_0x32A = 0xB4;
    self->effect_latch_0x32C = 0;
    self->kcolor_latch_0x32D = 0;
    switch (a) {
    case 2:
        if (self->field_0x00A == 2) {
            em_fall_height_get(self);
            em_fall_start(self);
            em_act_arm_unless_down(self, 3, 0);
        } else {
            em_act_arm_unless_down(self, 1, 0);
        }
        self->field_0x1CC = 0.8f;
        if (run->flags & 1) {
            copyVec3(&self->pos, &run->pos);
            self->field_0x1C0 = run->angle;
        } else {
            run->angle = (u16)self->field_0x1C0;
            copyVec3(&run->pos, &self->pos);
        }
        em_state_refresh(self);
        break;
    case 0:
        if (self->field_0x00A != 0) {
            copyVec3(&self->pos, &run->pos);
            self->field_0x1C0 = run->angle;
        }
        break;
    default:
        break;
    }
}

/* 0x8035F174 (0x4): an empty virtual slot. */
void em035_noop(void) {
}

/* 0x8035F178 (0x13C): em035's effect-summon hook. */
void em035_event_notify(_ENEMY_WORK* self, u8 a, u8 b) {
    u32 s[3];

    switch (a) {
    case 1:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u16)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            em_spawn_request(self->field_0x01A, 0x1A, 4, self->area_no, 0xFF, 1, 0x20, 1,
                        0xFF, &self->pos, (s32)s);
            self->timer_0x32A = 0xF0;
        }
        break;
    case 3:
        if (b == 1) {
            s[0] = 0;
            s[1] = (u16)((ran_suu(0) & 0xFF) << 8);
            s[2] = 0;
            em_spawn_request(self->field_0x01A, 0x1A, 5, self->area_no, 0xFF, 1, 0x20, 1,
                        0xFF, &self->pos, (s32)s);
            self->timer_0x32A = 0xF0;
        }
        break;
    default:
        break;
    }
}

}
