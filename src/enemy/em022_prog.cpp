/* enemy/em022_prog.cpp - enemy 022's program: the `em022_prog_tbl` entry points (work clear, init, frame update,
 *   action run, event notify, motion events, state query, death action, area change test and the two mini-work
 *   hooks), the per-action sub-state machines and their dispatchers, the height easing, aim and jaw helpers,
 *   the user-data node callback and destructor, and the static initializer.
 * RANGE. .text 0x803253BC-0x8032C920 (85 functions); .ctors 0x8056F39C-0x8056F3A0 (`__sinit_\em022_prog_cpp`),
 *   .rodata 0x80570840-0x80570880 (`em022_turn_tbl`), .data 0x805DD9E0-0x805DFC30 (`em022_prog_tbl` first), .bss
 *   0x806BE2C8-0x806BE328, .sdata 0x80792D08-0x80792D18, .sbss 0x80794B88-0x80794B90, .sdata2
 *   0x8079AF48-0x8079B108, extab, extabindex.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi` + `cmpwi` pairs) and
 *   `#pragma pool_data off` (each sound and key table is addressed with its own `lis`/`addi`).
 * NAMES. The unit is enemy 022's program: `em022_prog_tbl` (0x805DD9E0) opens its `.data` and holds this range's
 *   entry points.
 *   GUESS: every `em022_*` function, table and field name, from its body, its program-table slot, the action id
 *   `em022_action_run` dispatches on (`em022_act<N>_*`) or the sub-state that plays a sound table
 *   (`em022_se_tbl_<sub>`, `_b_` for the mode-1 set; records are named after the first table that points at
 *   them).  The pre-existing `em_*` row names are kept; `Em022UserData` is the accessor class.
 * RESIDUALS. All 85 rows written.
 *  - `em022_act6_wander`: retail converts the spread width 20 to float at run time (`li r0,20; xoris; lfd; fsubs`)
 *    after the modulo part; every source spelling folds it (literal, inline, macro) or hoists it (local `s32`).
 *  - `.sdata2`: the claim starts at 0x8079AF48 (2.0f), which only menu/fn_8031EA8C's `draw_lsp_anim_ary` loads;
 *    our pool starts at the 1300.0f word, so the double 0.0 lands 4 bytes later than retail's (seam request).
 *  - `.sbss`: ours is 5 bytes (the 4-byte sound parameter and the 1-byte static-init object), retail's claim 8.
 *   flipcheck: the sdata2 seam and `lbl_8079AF48` (referenced from fn_8031EA8C.o).
 * SHAPES. `Em022UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv`
 *   landing pad retail does not have).
 *   A `u8` compared through `(s32)` gives retail's `cmpwi`; `switch (self->state) { case 0: self->state++;`
 *   reuses the loaded byte.  Float arguments are evaluated in parameter order, so `em_wave_segment` and
 *   `capsule_box_from_points` are called through `_v` views that list the radius before the trailing integer/pointer
 *   arguments (their owners' prototypes list it last).  `em_fall_start` is called with
 *   the f1 load (`_c1` view) except in the two leaps, where retail leaves f1 holding the compared height.  The
 *   static initializer is a file-scope object whose inline constructor runs `assignVec3(dst, setVec3(&tmp, ..))`
 *   per vector, the temporaries declared in reverse; the vectors themselves are zero in `.data`/`.bss`.
 */
#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_DATA.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "MSL/new.h" /* the placement `operator new` */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "enemy/fn_80147CE0.h"
#include "enemy/enemy_control.h"
#include "fn_8004CAD8.h"
#include "ef.h"
#include "g3d/g3d_resanmchr.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "ef/eft009.h"
#include "ef/eft001.h"     /* mtx34_set_trans */
#include "g3d/g3d_calcworld.h" /* addVec3To */
#include "ef/eft007.h"         /* eft007_part_spawn */
#include "Pl/pl_hit_sphere.h"  /* pl_capsule_box_set */
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr and its slots */
#include "mh3_pad/system_w.h"   /* system_w */
#include "enemy/em_se_record.h" /* EmSeEntry / EmSeRecord / EmSeBranch */
#include "enemy/em016_prog_types.h" /* EmUserItem / EmMtxHolder */
#include "enemy/ENEMY_MINI_WORK.h" /* _ENEMY_MINI_WORK */
#include "ef/fn_800CDB2C.h"     /* ran_suu */
#include "lobby/fn_8030121C.h"  /* eft_em_spawn */
#include "nw4r/math_types.h"   /* nw4r::math::MTX34ToMTX33 */
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8028F66C.h"
#include "stage/stg_w.h"
#include "camera/camera.h"
#include "sys_mem.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "enemy/em_curve_key.h" /* EmCurveKey */
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */
#include "ef/eft_shape_init.h"
#include "g3d/g3d_resanmchr.h"

#pragma peephole off
#pragma pool_data off

/* `em_fall_start` and `em_ground_ck` through the signatures their bodies have: both read a float in f1 (the
 * shared declarations take none). */
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
#define em_ground_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)
/* `em_approach_step` with the 16-bit angle and rate its body reads (the shared declaration says `s32`). */
/* the user-data flag setter and destructor over this unit's `EmUserData` view. */
#define em_res_user_data_flag_set_c1 ((void (*)(EmUserData*, u32))em_res_user_data_flag_set)
#define em_approach_step_c1 ((u32 (*)(struct _ENEMY_WORK*, u16, u16))em_approach_step)

/* The +0x3C `ShellSetFuncs` slot spelled without its trailing table argument (the table load is then emitted
 * last, in r6, as retail has it). */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

/* The user-data accessor `em022_init` installs: the base accessor with its own destructor, attach hook and
 * animation item hook (its table is `.data` 0x805DFC00).  size: 0xC (`operator new(0xC)`) */
class Em022UserData : public ResUserDataAc {
public:
    Em022UserData();
    virtual ~Em022UserData();
    virtual void Attach();
    virtual void CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
};

extern "C" {

/* The program-table entry points, defined below in address order. */
void em_act_slots_clr(_ENEMY_WORK* self);
void em_act_entry_start(_ENEMY_WORK* self, u8 mode);
void em022_frame_update(_ENEMY_WORK* self);
void em022_action_run(_ENEMY_WORK* self);
void em_act_noop(void);
void em_act_frame_ck(_ENEMY_WORK* self, u8 motion, u8 state);
void em022_motion_events(_ENEMY_WORK* self);
void em022_tev_color_update(void);
u8 em022_state_query(_ENEMY_WORK* self, u8 kind);
void em022_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub);
u32 em022_area_change_ck(_ENEMY_WORK* self);
void em022_mini_init(_ENEMY_MINI_WORK* mini, u8 mode);
void em022_mini_step(_ENEMY_MINI_WORK* mini);

/* The em022 program table (`em022_prog_tbl`): the entry points the shared enemy program driver calls.  Slot
 * names are a GUESS from the functions they hold (`Em019ProgTbl`'s scheme).
 * size: 0x70 */
struct Em022ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 motion, u8 state);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(void);
    /* +0x24 */ u8 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ u32 pad_0x34[8];
    /* +0x54 */ u32 (*area_change_ck_0x54)(_ENEMY_WORK* self);
    /* +0x58 */ u32 pad_0x58;
    /* +0x5C */ void (*mini_init_0x5C)(_ENEMY_MINI_WORK* mini, u8 mode);
    /* +0x60 */ void (*mini_step_0x60)(_ENEMY_MINI_WORK* mini);
    /* +0x64 */ u32 pad_0x64[3];
};

Em022ProgTbl em022_prog_tbl = {
    em_act_slots_clr,
    em_act_entry_start,
    em022_frame_update,
    em022_action_run,
    em_act_noop,
    em_act_frame_ck,
    em022_motion_events,
    0,
    em022_tev_color_update,
    em022_state_query,
    {0, 0},
    em022_death_action_get,
    {0, 0, 0, 0, 0, 0, 0, 0},
    em022_area_change_ck,
    0,
    em022_mini_init,
    em022_mini_step,
    {0, 0, 0},
};

/* The turn sequence `em022_act6_turn` hands `em_turn_seq_start`/`em_turn_seq_step` (0x40 B): a four-word head,
 * then three four-word rows whose first word carries the effect id in its high half. */
extern const u32 em022_turn_tbl[16] = {
    0x00000006, 0x0000001A, 0x00000000, 0x80002000,
    0x00070000, 0x00000006, 0x00000000, 0x00000000,
    0x00070000, 0x00000006, 0x00000000, 0x00000000,
    0x00070000, 0x00000006, 0x00000000, 0x00000000,
};

/* The motion curves `em_key_curve_eval` reads (frame, value; -1 ends a curve). */
EmCurveKey em022_curve_rise[] = {{0.0f, 10.0f}, {15.0f, 0.0f}, {30.0f, -25.0f}, {44.0f, 0.0f}, {-1.0f, 0.0f}};
EmCurveKey em022_curve_hop_land[] = {{0.0f, -50.0f}, {6.0f, -23.0f}, {13.0f, 3.0f}, {16.0f, 20.0f}, {-1.0f, 20.0f}};
EmCurveKey em022_curve_rise_end[] = {
    {0.0f, 10.0f}, {10.0f, 25.0f}, {13.0f, 20.0f}, {14.0f, 10.0f}, {44.0f, 0.0f}, {-1.0f, 0.0f},
};
EmCurveKey em022_keys_mot16[] = {{0.0f, -300.0f}, {15.0f, -190.0f}, {30.0f, 0.0f}, {88.0f, 0.0f}, {-1.0f, 0.0f}};
EmCurveKey em022_curve_leap[] = {{0.0f, 40.0f}, {8.0f, 50.0f}, {15.0f, 15.0f}, {20.0f, 0.0f}, {-1.0f, 0.0f}};
EmCurveKey em022_curve_land[] = {{0.0f, -50.0f}, {5.0f, -20.0f}, {10.0f, 0.0f}, {-1.0f, 0.0f}};
EmCurveKey em022_curve_land_soft[] = {{0.0f, -25.0f}, {5.0f, -20.0f}, {10.0f, 0.0f}, {-1.0f, 0.0f}};
EmCurveKey em022_curve_drop[] = {
    {0.0f, 90.0f}, {5.0f, 50.0f}, {12.0f, 20.0f}, {19.0f, 0.0f}, {26.0f, -20.0f},
    {35.0f, -50.0f}, {38.0f, -60.0f}, {46.0f, -60.0f}, {-1.0f, 0.0f},
};

/* The capsule zone the action 7 zone charge aims into and leaves from (the static initializer sets the two
 * end points).  size: 0x20 */
struct Em022Zone {
    /* +0x00 */ u8 mode_0x00;
    /* +0x04 */ nw4r::math::VEC3 start_0x04;
    /* +0x10 */ nw4r::math::VEC3 end_0x10;
    /* +0x1C */ f32 radius_0x1C;
};
Em022Zone em022_nest_zone = {1, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 400.0f};

/* Callees whose owner prototypes differ from the spelling retail's call sites were compiled with (the float
 * listed before the trailing arguments, a third argument, a typed record): a cast call is the same direct call. */
#define eft_slot_marks_set_sync_v ((void (*)(_ENEMY_DATA*, u8, u8))eft_slot_marks_set_sync)
#define em_wave_segment_v \
    ((void (*)(nw4r::math::VEC3*, nw4r::math::VEC3*, nw4r::math::VEC3*, f32, u32, u16))em_wave_segment)
#define anm_chr_result_rot_mul_v ((void (*)(nw4r::g3d::ChrAnmResult*, nw4r::math::MTX33*))anm_chr_result_rot_mul)
#define capsule_box_from_points_v \
    ((void (*)(nw4r::math::VEC3*, nw4r::math::VEC3*, f32, PlCapsuleBox*))capsule_box_from_points)
#define eft_shape_init_v ((PlCapsuleBox* (*)(PlCapsuleBox*))eft_shape_init)

/* The unit's own entry points: the original file's definition order is not the address order, so the
 * callers below need these prototypes. */
void em_act_move_step(_ENEMY_WORK* self, u8 mode, f32 step);
void em_act_hold(_ENEMY_WORK* self);
void em_act_follow(_ENEMY_WORK* self);
void em_act_mot21(_ENEMY_WORK* self);
void em_act_aim(_ENEMY_WORK* self);
void em022_sight_count(_ENEMY_WORK* self);
void em022_land_se_play(_ENEMY_WORK* self);
void em022_jaw_update(_ENEMY_WORK* self);

/* Zeroes a 3-vector, then reports whether the current map/area is the tutorial's and the camera is
 * past the given height. */
s32 em_tut_cam_ck(void) {
    nw4r::math::VEC3 zeroed;
    VEC3_ctor(&zeroed);
    if (get_now_mapno() != 0x16) {
        return 1;
    }
    if (get_now_areano() == 2) {
        const nw4r::math::VEC3& cam = get_camera_pos();
        copyVec3(&zeroed, &cam);
        if (zeroed.x >= 1300.0f) {
            return 0;
        }
    }
    return 1;
}

/* Resets the record's three rotation angles; re-arms the two "rotation handed over" angles when the
 * area is 7/8 and the sub-state matches. */
void em_rot_reset(_ENEMY_WORK* self) {
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 2) {
        switch (self->area_no) {
        case 7:
            if ((s32)self->field_0x9F6 == 8) {
                em_move_target_set(self, 1, 2, 700.0f);
                self->field_0x1C0 = 0x8000;
                return;
            }
            em_move_target_set(self, 1, 2, 700.0f);
            return;
        case 8:
            if ((s32)self->field_0x9F6 == 7) {
                em_move_target_set(self, 1, 2, 700.0f);
                self->field_0x1C0 = 0xF000;
                return;
            }
            em_move_target_set(self, 1, 2, 700.0f);
            return;
        default:
            self->pos.z = 0.0f;
            self->pos.y = 0.0f;
            self->pos.x = 0.0f;
            self->field_0x1C4 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1BC = 0;
            return;
        }
    } else {
        self->pos.z = 0.0f;
        self->pos.y = 0.0f;
        self->pos.x = 0.0f;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
    }
}

/* Clears the jaw pulse steps and the sighting counters. */
void em_act_slots_clr(_ENEMY_WORK* self) {
    self->em022_0x328.pulse_a_0x32A = 0;
    self->em022_0x328.pulse_b_0x32C = 0;
    self->em022_0x328.sight_0x334 = 0;
    self->em022_0x328.alert_0x336 = 0;
}

/* Starts the band's entry action for the given mode and installs its state record once. */
void em_act_entry_start(_ENEMY_WORK* self, u8 mode) {
    switch (mode) {
    case 0:
        em_move_mode_set(self, 4);
        em_act_arm_unless_down(self, 6, 0x20);
        em_state_refresh(self);
        break;
    case 3:
        em_move_mode_set(self, 4);
        em_act_arm_unless_down(self, 6, 0x16);
        em_rot_reset(self);
        em_state_refresh(self);
        break;
    }
    self->field_0x835 = 1;
    if (em_res_user_data_ck(self) == 0) {
        Em022UserData* record = (Em022UserData*)operator new(sizeof(Em022UserData));
        new (record) Em022UserData;
        em_res_user_data_set(self, record);
    }
}

} /* extern "C" */

/* 0x8032564C (0x3C): Constructs the accessor: the base first, then this class's table. */
Em022UserData::Em022UserData() {
}

extern "C" {

/* The program table's empty motion-next slot. */
void em_act_noop(void) {
}

/* Per-step hook of the band's entry action: arms the jaw pulses for the motion and state the caller
 * reports, and asks for the "leave" transition when the enemy data entry disagrees. */
void em_act_frame_ck(_ENEMY_WORK* self, u8 motion, u8 state) {
    u8 kind = self->field_0x00A;

    switch (kind) {
    case 0:
        self->em022_0x328.pulse_a_0x32A = 0;
        self->em022_0x328.pulse_b_0x32C = 0;
        switch (motion) {
        case 6:
            if ((u32)(state - 0x14) <= 1U) {
                self->em022_0x328.pulse_b_0x32C = 1;
            }
            break;
        case 11: {
            _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, kind),
                                                           self->field_0x46C);
            if (entry != NULL && entry->field_0x08 == 1) {
                eft_slot_marks_set_sync_v(entry, 2, self->area_no);
            }
            break;
        }
        case 7:
            if ((s32)state == 5) {
                self->em022_0x328.pulse_a_0x32A = 1;
            }
            break;
        }
        break;
    case 1:
        if (self->field_0x43C != 0) {
            _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, kind),
                                                           self->field_0x46C);
            if (entry != NULL) {
                if (entry->field_0x0B != 1) {
                    em_motion_kind_set(self, 0, 0);
                }
            } else {
                em_motion_kind_set(self, 0, 0);
            }
        }
        break;
    }
}

/* Counts the frames a motion slot carries the "target sighted" bit (0x20), capped at 90 and arming the
 * alert timer; with no such slot the counter clears and the alert timer runs down. */
void em022_sight_count(_ENEMY_WORK* self) {
    s32 i;

    for (i = 0; i < 10; i++) {
        u16 flags = self->slots_0x244[i].flags;
        if (flags == 0) {
            break;
        }
        if (flags & 0x20) {
            if (++self->em022_0x328.sight_0x334 > 90) {
                self->em022_0x328.alert_0x336 = 90;
            }
            return;
        }
    }
    self->em022_0x328.sight_0x334 = 0;
    if (self->em022_0x328.alert_0x336 > 0) {
        self->em022_0x328.alert_0x336--;
    }
}

/* Takes over the data entry's aim point and opens the jaw while the target stands in the entry's area
 * close enough; mode 5 or another area asks for the leave transition. */
void em022_entry_track(_ENEMY_DATA* entry, _ENEMY_WORK* self) {
    copyVec3(&self->aim, &entry->vec_0x24);
    switch (entry->field_0x08) {
    case 1:
        self->em022_0x328.jaw_open_0x328 = 0;
        if ((entry->field_0x0B == 0 || entry->field_0x0B == 2) && self->state_0x46E < entry->field_0x39 &&
            self->area_no == entry->field_0x14) {
            self->em022_0x328.jaw_open_0x328 = 1;
        }
        em022_sight_count(self);
        break;
    case 5:
        self->em022_0x328.jaw_open_0x328 = 0;
        self->em022_0x328.sight_0x334 = 0;
        if (self->field_0x43B != 1) {
            em_motion_kind_set(self, 1, 0);
        }
        break;
    default:
        self->em022_0x328.jaw_open_0x328 = 0;
        self->em022_0x328.sight_0x334 = 0;
        if (self->area_no != entry->field_0x14) {
            em_motion_kind_set(self, 1, 0);
        }
        break;
    }
}

/* Per-frame update: closes the jaw on death, tracks the data entry in mode 0 (bit 15 of +0x836 mirrors the
 * slow pulse), opens the jaw in mode 1 while hit, and starts the attack when it is due. */
void em022_frame_update(_ENEMY_WORK* self) {
    if ((u32)em_die_ck(self) == 1) {
        self->em022_0x328.jaw_open_0x328 = 0;
        self->em022_0x328.pulse_a_0x32A = 0;
        self->em022_0x328.pulse_b_0x32C = 0;
        return;
    }
    switch (self->field_0x00A) {
    case 0: {
        _ENEMY_DATA* entry;
        if (self->em022_0x328.jaw_angle_0x32E != 0) {
            self->flags_0x836 &= 0x7FFF;
        } else {
            self->flags_0x836 |= 0x8000;
        }
        entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
        if (entry != NULL) {
            em022_entry_track(entry, self);
        }
        break;
    }
    case 1:
        if (self->field_0x1E2 == 0 && (self->field_0x228 & ~0x1000) != 0) {
            self->em022_0x328.jaw_open_0x328 = 1;
        } else {
            self->em022_0x328.jaw_open_0x328 = 0;
        }
        break;
    }
    if (self->field_0x834 == 1 || em_hit_timer_ck(self) == 1) {
        em_attack_start(self);
    }
}

/* Plays the landing sound at joint 3, 5 units above the body height. */
void em022_land_se_play(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    copyVec3(&pos, &self->pos);
    cpSetRotMatrix((struct _CP_VECTOR*)&self->field_0x1BC, &mtx);
    mulVecMat(&pos, &mtx);
    mtx34_set_trans(&mtx, &pos);
    get_joint_wpos_em(self, 3, &pos);
    pos.y = 5.0f + self->field_0x20C;
    se_req_pos_landing(self->se_0xB14, &pos);
}

/* Eases the body height toward 0 (mode 0) or toward the base height (mode 1) by `step` a frame. */
void em_act_move_step(_ENEMY_WORK* self, u8 mode, f32 step) {
    switch (mode) {
    case 0: {
        f32 height = self->field_0x1AC;
        if (height < 0.0f) {
            em_height_set(self, height + step);
            if (self->field_0x1AC > 0.0f) {
                em_height_set(self, 0.0f);
            }
        } else if (height > 0.0f) {
            em_height_set(self, height - step);
            if (self->field_0x1AC < 0.0f) {
                em_height_set(self, 0.0f);
            }
        }
        break;
    }
    case 1: {
        f32 base = em_height_base_get(self);
        f32 height = self->field_0x1AC;
        if (height > base) {
            em_height_set(self, height - step);
            if (self->field_0x1AC < base) {
                em_height_set(self, base);
            }
        } else if (height < base) {
            em_height_set(self, height + step);
            if (self->field_0x1AC > base) {
                em_height_set(self, base);
            }
        }
        break;
    }
    }
}

/* Arms the band's fifth action: motion 6 sub-state 7. */
void em_act_arm_m06s07(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    em_act_arm_unless_act11(self, 6, 7);
    em_finish_flag_set(self);
}

/* The band's 50-frame hold: steps its own state byte and hands the follow-up motion to the move
 * engine. */
void em_act_hold(_ENEMY_WORK* self) {
    em_height_set(self, 50.0f);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_ck(self, 1, 6, 0, 3);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

/* The band's follow-up motion: resets the height to zero, then eases it back while the motion runs. */
void em_act_follow(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_blend_ck(self, 1, 6, 0, 3);
        em_height_set(self, 0.0f);
        return;
    case 1:
        em_act_move_step(self, 0, 5.0f);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* The band's fall motion 21: hands it to the move engine, then reports done. */
void em_act_mot21(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_start_c1(self, 0.0f);
        em_mot_set(self, 0x15, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        return;
    }
}

/* Per-step dispatch of action 0's motions on the action sub-state. */
void em_act_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_act_hold(self);
        return;
    case 1:
        em_act_hold(self);
        return;
    case 2:
        em_act_hold(self);
        return;
    case 3:
        em_act_mot21(self);
        return;
    case 7:
        em_act_follow(self);
        return;
    }
}

/* Re-aims the record at its target: the angle to the target position is latched, and clamped into
 * one of the two facing windows when it falls outside them. */
void em_act_aim(_ENEMY_WORK* self) {
    if (self->field_0x314 >= 0.0) {
        s32 angle = calcVecAngX2(&self->vec_0x194, &self->pos);
        self->field_0x1BC = angle;
        if ((u32)(angle + 0xFFFF5FFF) <= 0x3FFEU) {
            self->field_0x1BC = 0xE000;
        }
    } else {
        s32 angle = calcVecAngX2(&self->vec_0x194, &self->pos);
        self->field_0x1BC = angle;
        if ((u32)(angle - 0x2001) <= 0x1FFEU) {
            self->field_0x1BC = 0x2000;
        }
    }
}

/* Degrees to a 16-bit angle, rounded. */
#define EM022_DEG2ANG(deg) ((u16)(0.5f + 65536.0f * (deg) / 360.0f))

/* Action 3's backward jump (modes 0..2): leaps off, turns 2 degrees a frame up to 45 while airborne, lands
 * with the dust effect and the landing sound, and eases the height back. */
void em022_act3_jump_back(_ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 accel;
    f32 turn;
    f32 turn_max;

    setVec3(&move, 0.0f, 50.0f, 40.0f);
    setVec3(&accel, 0.0f, -3.0f, 0.0f);
    turn = 2.0f;
    turn_max = 45.0f;
    em_hit_arm_set(self);
    {
        switch (self->state) {
        case 0:
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x13, 2, 0);
            em_move_vec2_clr(self);
            copyVec3(&self->offset_0x30C.vec_0x310, &move);
            copyVec3(&self->vec_0x31C, &accel);
            if ((s32)mode == 0) {
                self->field_0x318 *= 0.7f;
            } else if (mode == 2) {
                self->field_0x318 *= 1.5f;
                self->field_0x320 *= 1.5f;
            }
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            break;
        case 1:
            CancelFade(self);
            if (em_mot_end_ck(self) == 1U) {
                self->state++;
                em_fall_start_c1(self, 150.0f);
                em_mot_set(self, 0x18, 0, 0);
                em_mot_speed_set(self, 0.9f);
            }
            break;
        case 2:
            CancelFade(self);
            if (em_mot_end_ck(self) == 1U) {
                self->state++;
                em_mot_set(self, 0x15, 0, 0);
                self->field_0x1BC += EM022_DEG2ANG(turn);
                if (self->field_0x1BC > EM022_DEG2ANG(turn_max)) {
                    self->field_0x1BC = EM022_DEG2ANG(turn_max);
                }
            }
            break;
        case 3:
            CancelFade(self);
            self->field_0x1BC += EM022_DEG2ANG(turn);
            if (self->field_0x1BC > EM022_DEG2ANG(turn_max)) {
                self->field_0x1BC = EM022_DEG2ANG(turn_max);
            }
            if (em_ground_ck_c1(self, 0.0f) != 0) {
                self->state++;
                em_height_set(self, self->pos.y - self->field_0x20C);
                em_move_mode_set(self, 4);
                eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
                em022_land_se_play(self);
            }
            break;
        case 4: {
            f32 base = em_height_base_get(self);
            if (self->field_0x1AC < base) {
                em_height_set(self, base);
                self->field_0x1BC = 0;
                em_action_finish_stand(self);
            } else {
                CancelFade(self);
                em_height_move_apply(self);
                self->field_0x1BC += EM022_DEG2ANG(turn);
                if (self->field_0x1BC > EM022_DEG2ANG(turn_max)) {
                    self->field_0x1BC = EM022_DEG2ANG(turn_max);
                }
            }
            break;
        }
        }
    }
}

/* Action 3's leap (modes 0/1): a jump on the height curve that keeps aiming at the target, then the landing
 * and the height easing. */
void em022_act3_leap(_ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 accel;

    setVec3(&move, 0.0f, 90.0f, 40.0f);
    setVec3(&accel, 0.0f, -6.0f, 0.0f);
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x15, 2, 0);
        em_height_set(self, em_height_base_get(self));
        copyVec3(&self->offset_0x30C.vec_0x310, &move);
        copyVec3(&self->vec_0x31C, &accel);
        if ((s32)mode == 0) {
            self->field_0x318 *= 0.7f;
        }
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        break;
    case 1:
        em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_leap));
        self->pos.x += self->offset_0x30C.vec_0x310.x;
        self->pos.z += self->field_0x318;
        em_act_aim(self);
        if (self->field_0x1AC >= 0.0f) {
            self->state++;
            em_fall_start(self);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
        }
        break;
    case 2:
        CancelFade(self);
        em_act_aim(self);
        if (em_ground_ck_c1(self, 0.0f) != 0) {
            self->state++;
            em_height_set(self, self->pos.y - self->field_0x20C);
            em_move_mode_set(self, 4);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            em022_land_se_play(self);
        }
        break;
    case 3: {
        f32 base = em_height_base_get(self);
        if (self->field_0x1AC < base) {
            em_height_set(self, base);
            self->field_0x1BC = 0;
            em_action_finish_stand(self);
        } else {
            CancelFade(self);
            em_height_move_apply(self);
            em_act_aim(self);
        }
        break;
    }
    }
}

/* Action 3's motion 22: plays it out, then ends the action. */
void em022_act3_mot22(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

/* Action 3's turning leap: jumps on the height curve facing back (a half turn plus the body's random
 * offset), drops off a wall into a stagger, or lands and eases the height back. */
void em022_act3_leap_turn(_ENEMY_WORK* self) {
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 accel;
    u32 rot;

    setVec3(&move, 0.0f, 90.0f, 30.0f);
    setVec3(&accel, 0.0f, -6.0f, 0.0f);
    em_hit_arm_set(self);
    rot = self->field_0x1C0;
    self->field_0x1C0 = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x15, 2, 0);
        em_height_set(self, em_height_base_get(self));
        em_move_vec2_clr(self);
        copyVec3(&self->offset_0x30C.vec_0x310, &move);
        copyVec3(&self->vec_0x31C, &accel);
        rot = 0x8000;
        if (self->pos.x >= 0.0f) {
            rot += self->bits_0x1EC % 0x2000;
        } else {
            rot -= self->bits_0x1EC % 0x2000;
        }
        break;
    case 1:
        em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_leap));
        self->pos.x += self->offset_0x30C.vec_0x310.x;
        self->pos.z += self->field_0x318;
        if (self->field_0x1AC >= 0.0f) {
            self->state++;
            em_fall_start(self);
            rotVecY(&self->offset_0x30C.vec_0x310, (u16)calcVecAng2(&self->pos, &self->vec_0x36C));
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
        }
        break;
    case 2:
        CancelFade(self);
        em_act_aim(self);
        if (self->field_0x314 <= 0.0f) {
            u32 angle = self->field_0x1BC;
            if (angle != 0 && angle <= 0x4000) {
                self->field_0x1BC = 0;
            }
        }
        if (em_ground_ck_c1(self, 50.0f) != 0) {
            if ((self->field_0x228 & ~0x1000) != 0) {
                em_state_set(self, 3, 2);
            } else if (em_ground_ck_c1(self, 0.0f) != 0) {
                self->state++;
                em_height_set(self, self->pos.y - self->field_0x20C);
                em_move_mode_set(self, 4);
            }
        }
        break;
    case 3: {
        f32 base = em_height_base_get(self);
        if (self->field_0x1AC < base) {
            em_height_set(self, base);
            self->field_0x1BC = 0;
            rot = 0x8000;
            em_action_finish_stand(self);
        } else {
            CancelFade(self);
            em_height_move_apply(self);
        }
        break;
    }
    }
    self->field_0x1C0 = rot;
}

/* Action 3's backward jump that ends turned round: as `em022_act3_jump_back`, then a 60-frame half-turn
 * shuffle on the ground; a wall contact staggers. */
void em022_act3_jump_back_turn(_ENEMY_WORK* self) {
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 accel;
    f32 turn;
    f32 turn_max;

    setVec3(&move, 0.0f, 50.0f, 40.0f);
    setVec3(&accel, 0.0f, -3.0f, 0.0f);
    turn = 2.0f;
    turn_max = 45.0f;
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 2, 0);
        em_move_vec2_clr(self);
        copyVec3(&self->offset_0x30C.vec_0x310, &move);
        copyVec3(&self->vec_0x31C, &accel);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        break;
    case 1:
        CancelFade(self);
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_fall_start_c1(self, 150.0f);
            em_mot_set(self, 0x18, 0, 0);
            em_mot_speed_set(self, 0.9f);
        }
        break;
    case 2:
        CancelFade(self);
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x15, 0, 0);
            self->field_0x1BC += EM022_DEG2ANG(turn);
            if (self->field_0x1BC > EM022_DEG2ANG(turn_max)) {
                self->field_0x1BC = EM022_DEG2ANG(turn_max);
            }
        }
        break;
    case 3:
        CancelFade(self);
        self->field_0x1BC += EM022_DEG2ANG(turn);
        if (self->field_0x1BC > EM022_DEG2ANG(turn_max)) {
            self->field_0x1BC = EM022_DEG2ANG(turn_max);
        }
        if (em_ground_ck_c1(self, 50.0f) != 0) {
            if ((self->field_0x228 & ~0x1000) != 0) {
                em_state_set(self, 3, 2);
            } else if (em_ground_ck_c1(self, 0.0f) != 0) {
                self->state++;
                em_height_set(self, self->pos.y - self->field_0x20C);
                em_move_mode_set(self, 4);
                eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            }
        }
        break;
    case 4: {
        f32 base = em_height_base_get(self);
        if (self->field_0x1AC < base) {
            self->state++;
            em_height_set(self, base);
            self->field_0x1C0 = 0x8000;
            self->field_0x1BC = 0;
            self->timer_0x020 = 60;
            em_move_vec2_clr(self);
            self->field_0x318 = 5.0f;
        } else {
            CancelFade(self);
            em_height_move_apply(self);
            self->field_0x1BC += EM022_DEG2ANG(turn);
            if (self->field_0x1BC > EM022_DEG2ANG(turn_max)) {
                self->field_0x1BC = EM022_DEG2ANG(turn_max);
            }
        }
        break;
    }
    case 5:
        self->field_0x1C0 = 0x8000;
        if (--self->timer_0x020 <= 0) {
            em_action_finish_stand(self);
        } else {
            em_move_offset_apply(self);
        }
        break;
    }
}

/* Action 3's sub-state dispatcher. */
void em022_act3_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em022_act3_jump_back(self, 0);
        break;
    case 1:
        em022_act3_jump_back(self, 1);
        break;
    case 2:
        em022_act3_mot22(self);
        break;
    case 3:
        em022_act3_leap(self, 0);
        break;
    case 4:
        em022_act3_leap(self, 1);
        break;
    case 5:
        em022_act3_leap_turn(self);
        break;
    case 6:
        em022_act3_jump_back_turn(self);
        break;
    case 7:
        em022_act3_jump_back(self, 2);
        break;
    }
}

/* Action 6's rise: motion 2 on the rise curve, the height kept between the base and 130. */
void em022_act6_rise(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 2, 4, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 5.0f;
        return;
    case 1: {
        f32 base;
        f32 height;
        em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_rise));
        base = em_height_base_get(self);
        height = self->field_0x1AC;
        if (height > 130.0f) {
            em_height_set(self, 130.0f);
        } else if (height < base) {
            em_height_set(self, base);
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        return;
    }
    }
}

/* Picks the forward speed from the distance and the bearing to the entry's live work: 25 when close or
 * when the work is this one, else 30/34 ahead and 20/16 behind. */
void em022_speed_pick(_ENEMY_WORK* self) {
    _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A),
                                                       self->field_0x46C);
    if (entry != NULL) {
        _ENEMY_WORK* work = entry->work_0x18;
        if (self == work) {
            self->field_0x318 = 25.0f;
        } else if (work != NULL) {
            f32 dist = calcDistanceSqXZ(&self->pos, &work->pos);
            u32 diff = (u16)(calcVecAng2(&self->pos, &entry->work_0x18->pos) - self->field_0x1C0);
            if (diff >= 0x8000) {
                diff = 0x10000 - diff;
            }
            if (diff < 0x4000) {
                if (dist < 640000.0f) {
                    self->field_0x318 = 25.0f;
                } else if (dist < 4000000.0f) {
                    self->field_0x318 = 30.0f;
                } else {
                    self->field_0x318 = 34.0f;
                }
            } else if (dist < 640000.0f) {
                self->field_0x318 = 25.0f;
            } else if (dist < 4000000.0f) {
                self->field_0x318 = 20.0f;
            } else {
                self->field_0x318 = 16.0f;
            }
        } else {
            self->field_0x318 = 25.0f;
        }
    } else {
        self->field_0x318 = 25.0f;
    }
}

/* A random spread of `n` units around `base`, from the body's random bits. */
#define EM022_SPREAD(base, n) ((base) + (f32)(self->bits_0x1EC % (n)) - (f32)(n) * 0.5f)

/* Action 6's hop (modes 0/1): motion 10 jumping off at the picked speed, one of three lift angles, the air
 * time on the move offset and the landing on the hop curve; mode 1 raises dust. */
void em022_act6_hop(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 2, 0);
        em_height_set(self, em_height_base_get(self));
        em_move_vec2_clr(self);
        if ((s32)mode == 0) {
            em022_speed_pick(self);
        } else {
            self->field_0x318 = 20.0f;
        }
        em_mot_speed_set(self, 0.85f);
        self->state_0x007 = self->bits_0x1EC % 3;
        switch (self->state_0x007) {
        case 0:
            self->field_0x314 = 80.0f;
            self->field_0x320 = -6.0f;
            break;
        case 1:
            self->field_0x314 = 90.0f;
            self->field_0x320 = -6.0f;
            break;
        default:
            self->field_0x314 = 95.0f;
            self->field_0x320 = -6.0f;
            break;
        }
        break;
    case 1:
        em_height_move_step(self);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (self->field_0x1AC >= 0.0f) {
            self->state++;
            em_fall_start_c1(self, 0.0f);
            self->pos.y = self->field_0x20C + self->field_0x1AC;
            em_height_set(self, 0.0f);
            if (mode == 1) {
                eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            }
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x314 < -60.0f) {
            self->field_0x314 = -60.0f;
        }
        if (self->pos.y - self->prev_pos.y <= 0.0f && em_ground_ck_c1(self, 50.0f) != 0) {
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 6, 0, 0);
            if (mode == 1) {
                eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            }
            em022_land_se_play(self);
        }
        break;
    case 3:
        em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_hop_land));
        if (em_mot_end_ck(self) == 1U) {
            em_height_set(self, em_height_base_get(self));
            em_action_finish_stand(self);
        } else {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
    }
}

/* Action 6's walk (modes 0..6): starts the approach (or a timed wander, a reversed approach, the far or the
 * close approach, or a turn toward the target), then walks it out until the approach, the motion, the timer
 * or the distance ends it. */
void em022_act6_walk(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 6, 0);
        em_move_vec2_clr(self);
        switch (mode) {
        case 0:
            em_approach_start(self, 0.0f, 0);
            self->field_0x318 = 20.0f;
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
            em022_speed_pick(self);
            break;
        default:
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
            em022_speed_pick(self);
            break;
        case 3:
            em_approach_start(self, 0.0f, 0);
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
            self->field_0x318 = 20.0f;
            self->value_0x378 = 1500.0f - self->value_0x378;
            break;
        case 4:
            em_approach_start(self, -300.0f, 0);
            self->field_0x318 = 20.0f;
            break;
        case 5:
            em_approach_start(self, 0.0f, 0);
            self->value_0x378 = 400.0f;
            self->field_0x318 = 20.0f;
            break;
        case 6:
            self->timer_0x020 = (self->bits_0x1EC & 0xF) + 15;
            self->field_0x318 = 15.0f;
            self->em022_0x328.turn_0x330 = calcVecAng2(&self->pos, &self->vec_0x36C);
            break;
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    case 1:
        switch (mode) {
        default:
            em_act_move_step(self, 0, 10.0f);
            if (em_approach_step(self, 0, 0x100) == 1U) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        case 1:
            em_act_move_step(self, 0, 10.0f);
            if (em_mot_end_ck(self) == 1U) {
                em022_speed_pick(self);
            }
            em_turn_to_target(self, 0x100);
            if (calcDistanceSqXZ(&self->pos, &self->aim) <= 490000.0f) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        case 2:
            if (em_mot_end_ck(self) == 1U) {
                em022_speed_pick(self);
            }
            self->timer_0x020--;
            em_turn_to_target(self, 0x200);
            if (calcDistanceSqXZ(&self->pos, &self->aim) <= 360000.0f) {
                em_action_finish_stand(self);
            } else if (self->timer_0x020 <= 0) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        case 3:
            em_act_move_step(self, 0, 10.0f);
            if (em_approach_step(self, 0, 0) == 1U) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        case 4:
        case 5:
            em_act_move_step(self, 0, 10.0f);
            if (em_approach_step(self, 0, 0x300) == 1U) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        case 6:
            em_act_move_step(self, 1, 10.0f);
            self->field_0x1C0 = em_angle_approach(self->em022_0x328.turn_0x330, self->field_0x1C0, 0x300);
            if (--self->timer_0x020 <= 0) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        }
        break;
    }
}

/* Action 6's run (modes 0..2): motion 4 into the motion 3 run, approaching the target, the far point or
 * the farthest one. */
void em022_act6_run(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 4, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 3, 6, 0);
            if ((s32)mode == 0) {
                em_approach_start(self, 0.0f, 0);
            } else if (mode == 1) {
                em_approach_start(self, -300.0f, 0);
            } else {
                em_approach_start(self, -1200.0f, 0);
            }
            em_move_vec2_clr(self);
            self->field_0x318 = 30.0f;
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            em_action_finish_stand(self);
        } else {
            em_act_move_step(self, 0, 10.0f);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
    }
}

/* Action 6's drop: motion 28 falling on the drop curve until it lands with dust and the landing sound, then
 * motion 26 on the landing curve. */
void em022_act6_drop(_ENEMY_WORK* self) {
    f32 speed = 0.7f;

    switch (self->state) {
    case 0:
        self->state++;
        em_height_set(self, 0.0f);
        em_fall_start_c1(self, 0.0f);
        em_mot_set(self, 0x1C, 0, 0);
        em_mot_speed_set(self, speed);
        break;
    case 1:
        self->pos.y += em_key_curve_eval(self, (void*)em022_curve_drop);
        if (self->pos.y - self->prev_pos.y <= 0.0f && em_ground_ck_c1(self, 50.0f) != 0) {
            self->state++;
            em_height_set(self, self->pos.y - self->field_0x20C);
            em_move_mode_set(self, 4);
            em_mot_set(self, 0x1A, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            em022_land_se_play(self);
        }
        break;
    case 2:
        em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_land));
        if (em_mot_end_ck(self) == 1U) {
            em_height_set(self, em_height_base_get(self));
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 6's turn (modes 0/1): the turn sequence in either direction, easing the height down meanwhile. */
void em022_act6_turn(_ENEMY_WORK* self, u8 mode) {
    f32 speed = 0.7f;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        if ((s32)mode == 0) {
            em_turn_seq_start(self, (void*)em022_turn_tbl, 0, 9, 0);
        } else {
            em_turn_seq_start(self, (void*)em022_turn_tbl, 1, 9, 0);
        }
        em_mot_speed_set(self, speed);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em022_turn_tbl) == 1U) {
            em_action_finish_stand(self);
        }
        em_act_move_step(self, 0, 10.0f);
        break;
    }
}

/* Action 6's rise out (motion 11 on its curve, the height capped at 0), then the motion 6/7 hand-over. */
void em022_act6_rise_end(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xB, 2, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_rise_end));
        if (self->field_0x1AC > 0.0f) {
            em_height_set(self, 0.0f);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_height_set(self, 0.0f);
            em_act_arm_m06s07(self);
        }
        break;
    }
}

/* Action 6's motion 12 at height 0, then the motion 6/7 hand-over. */
void em022_act6_mot12(_ENEMY_WORK* self) {
    em_height_set(self, 0.0f);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xC, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_act_arm_m06s07(self);
        }
        break;
    }
}

/* Action 6's motion 23 turning toward the target, then the motion 6/7 hand-over. */
void em022_act6_face(_ENEMY_WORK* self) {
    em_height_set(self, 0.0f);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x17, 2, 0);
        break;
    case 1:
        em_turn_to_target(self, 0x16C);
        if (em_mot_end_ck(self) == 1U) {
            em_act_arm_m06s07(self);
        }
        break;
    }
}


/* Action 6's jump (modes 0/1): motion 14 takes off at frame 21, mode 0 forward on the move offset and mode 1
 * straight up, then lands with dust and the landing sound and eases out on the landing curve. */
void em022_act6_jump(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xE, 2, 0);
        em_mot_speed_set(self, 0.8f);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if (em_mot_frame_get(self) >= 21.0f) {
            self->state++;
            em_height_set(self, 0.0f);
            em_fall_start_c1(self, 0.0f);
            em_move_vec2_clr(self);
            if ((s32)mode != 1) {
                self->field_0x314 = 100.0f;
                self->field_0x318 = -25.0f;
                self->field_0x320 = -8.0f;
                self->field_0x324 = 0.0f;
            } else {
                self->field_0x314 = 60.0f;
                self->field_0x320 = -5.0f;
            }
        }
        break;
    case 2:
        if ((s32)mode != 1) {
            em_move_offset_step(self, &self->field_0x1BC);
        } else {
            CancelFade(self);
        }
        if (self->pos.y - self->prev_pos.y <= 0.0f && em_ground_ck_c1(self, 50.0f) != 0) {
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 0x1A, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            em022_land_se_play(self);
        }
        break;
    case 3:
        if ((s32)mode != 1) {
            em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_land));
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 0.0f) {
                self->field_0x318 = 0.0f;
            }
        } else {
            em_height_add(self, em_key_curve_eval(self, (void*)em022_curve_land_soft));
        }
        if (em_mot_end_ck(self) == 1U) {
            em_height_set(self, em_height_base_get(self));
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 6's circling: motion 3 around the target at a side angle picked from the distance (or at random),
 * then a quarter-turn sweep, each for 60 frames; the alert timer picks the faster, opposite sweep. */
void em022_act6_circle(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        if (em_get_mot_no(self) == 3) {
            self->state = 2;
            em_mot_set_ck(self, 3, 6, 0);
        } else {
            em_mot_set(self, 4, 6, 0);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) != 1U) {
            break;
        }
        self->state++;
        /* fall through */
    case 2: {
        f32 dist;
        self->state++;
        em_mot_set_ck(self, 3, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 35.0f;
        self->timer_0x020 = 60;
        dist = vec3_dist_sq(&self->pos, &self->vec_0x36C);
        if (dist >= 1440000.0f) {
            if ((self->bits_0x1EC & 1) == 0) {
                self->em022_0x328.turn_0x330 = 0x36C1;
            } else {
                self->em022_0x328.turn_0x330 = 0x3333;
            }
        } else if (dist <= 1000000.0f) {
            if ((self->bits_0x1EC & 1) == 0) {
                self->em022_0x328.turn_0x330 = 0x493F;
            } else {
                self->em022_0x328.turn_0x330 = 0x4CCD;
            }
        } else {
            switch ((u16)(self->bits_0x1EC % 9)) {
            case 0:
                self->em022_0x328.turn_0x330 = 0x4222;
                self->field_0x318 = 35.0f;
                break;
            case 1:
                self->em022_0x328.turn_0x330 = 0x4222;
                self->field_0x318 = 33.0f;
                break;
            case 2:
                self->em022_0x328.turn_0x330 = 0x4222;
                self->field_0x318 = 30.0f;
                break;
            case 3:
                self->em022_0x328.turn_0x330 = 0x4000;
                self->field_0x318 = 35.0f;
                break;
            case 4:
                self->em022_0x328.turn_0x330 = 0x4000;
                self->field_0x318 = 33.0f;
                break;
            case 5:
                self->em022_0x328.turn_0x330 = 0x4000;
                self->field_0x318 = 30.0f;
                break;
            case 6:
                self->em022_0x328.turn_0x330 = 0x3DDE;
                self->field_0x318 = 35.0f;
                break;
            case 7:
                self->em022_0x328.turn_0x330 = 0x3DDE;
                self->field_0x318 = 33.0f;
                break;
            case 8:
                self->em022_0x328.turn_0x330 = 0x3DDE;
                self->field_0x318 = 30.0f;
                break;
            }
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    }
    case 3:
        if (self->em022_0x328.alert_0x336 > 0) {
            if (em_approach_step_c1(self, -self->em022_0x328.turn_0x330, 0x38E) == 1U) {
                em_approach_start(self, 0.0f, 0);
            }
        } else if (em_approach_step_c1(self, self->em022_0x328.turn_0x330, 0x111) == 1U) {
            em_approach_start(self, 0.0f, 0);
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            self->timer_0x020 = 60;
            em_approach_start(self, 0.0f, 0);
        }
        break;
    case 4:
        if (self->em022_0x328.alert_0x336 > 0) {
            if (em_approach_step_c1(self, 0xC000, 0x38E) == 1U) {
                em_approach_start(self, 0.0f, 0);
            }
        } else if (em_approach_step(self, 0x4000, 0x16C) == 1U) {
            em_approach_start(self, 0.0f, 0);
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (--self->timer_0x020 <= 0) {
            em_action_finish_stand(self);
        }
        break;
    }
    em_act_move_step(self, 0, 10.0f);
}

/* Action 6's idle (modes 0/1): motion 1 in place; mode 0 eases the height to 0 until the motion ends, mode 1
 * eases it to the base for a random 30..61 frames. */
void em022_act6_idle(_ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 6, 0);
        if ((s32)mode == 0) {
            em_height_set(self, 0.0f);
        } else {
            em_height_set(self, em_height_base_get(self));
            self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
        }
        break;
    case 1:
        if ((s32)mode == 0) {
            em_act_move_step(self, 0, 10.0f);
            if (em_mot_end_ck(self) == 1U) {
                em_action_finish_stand(self);
            }
        } else {
            em_act_move_step(self, 1, 10.0f);
            if (--self->timer_0x020 <= 0) {
                em_action_finish_stand(self);
            }
        }
        break;
    }
}

/* Action 6's wander (modes 0/1): mode 0 approaches at a random pace, slowing near the area's data point;
 * mode 1 turns a random -30..29 degrees and walks out motion 1. */
void em022_act6_wander(_ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 point;

    VEC3_ctor(&point);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_move_vec2_clr(self);
        if ((s32)mode != 1) {
            if (em_get_mot_no(self) == 3) {
                self->state = 2;
                em_mot_set_ck(self, 3, 6, 0);
                self->field_0x318 = EM022_SPREAD(30.0f, 20);
                self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
            } else {
                em_mot_set(self, 4, 6, 0);
            }
            em_approach_start(self, 0.0f, 0);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        } else {
            em_mot_set_ck(self, 1, 6, 0);
            self->field_0x318 = 20.0f;
            em_height_set(self, em_height_base_get(self));
            self->field_0x1C0 += EM022_DEG2ANG(self->bits_0x1EC % 60 - 30);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
    case 1:
        if ((s32)mode != 1) {
            if (em_mot_end_ck(self) == 1U) {
                self->state++;
                self->field_0x318 = EM022_SPREAD(30.0f, 20);
                self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
                em_mot_set(self, 3, 6, 0);
            }
        } else {
            if (em_mot_end_ck(self) == 1U) {
                em_action_finish_stand(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            em_act_move_step(self, 0, 8.0f);
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0x100) == 1U) {
            em_action_finish_stand(self);
        } else {
            if (--self->timer_0x020 <= 0) {
                self->field_0x318 = EM022_SPREAD(30.0f, 20);
                self->timer_0x020 = (self->bits_0x1EC & 0x1F) + 30;
            }
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        em_data_point_get(self, (self->area_no & 0xF) * 0x100, &point);
        if (calcDistanceSqXZ(&self->pos, &point) < 1690000.0f) {
            em_act_move_step(self, 1, 3.0f);
        } else {
            em_act_move_step(self, 0, 10.0f);
        }
        break;
    }
}

/* Action 6's motion 29: lifts the height by 34 a frame over frames 0..18 (capped at 0), then ends. */
void em022_act6_mot29(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x1D, 0, 0);
        break;
    case 1:
        if ((s32)em_frame_check(self, 3, 0.0f, 18.0f) != 0) {
            em_height_add(self, 34.0f);
            if (self->field_0x1AC > 0.0f) {
                em_height_set(self, 0.0f);
            }
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}


/* Action 6's strafe (modes 0/1, left or right): motion 4 into the motion 3 run, then side-steps around the
 * target in two alternating legs (60 and 90 frames) until the time runs out or the target is near. */
void em022_act6_strafe(_ENEMY_WORK* self, u8 mode) {
    em_act_move_step(self, 0, 5.0f);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 4, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 3, 6, 0);
            em_approach_start(self, 0.0f, 0);
            em_move_vec2_clr(self);
            self->field_0x318 = 35.0f;
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            self->timer_0x020 = 0;
            self->em022_0x328.leg_0x332 = 0;
        }
        break;
    case 2:
        switch (self->state_0x006) {
        case 0: {
            u32 angle;
            if (vec3_dist_sq(&self->pos, &self->vec_0x36C) > 2250000.0f) {
                if ((s32)mode == 0) {
                    angle = 0x3333;
                } else {
                    angle = 0xCCCE;
                }
            } else {
                angle = (s32)mode == 0 ? 0x4000 : 0xC000;
            }
            if (self->em022_0x328.alert_0x336 > 0) {
                if (em_approach_step_c1(self, -angle, 0x38E) == 1U) {
                    em_approach_start(self, 0.0f, 0);
                }
            } else if (em_approach_step_c1(self, angle, 0x1F4) == 1U) {
                em_approach_start(self, 0.0f, 0);
            }
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (++self->em022_0x328.leg_0x332 >= 60) {
                self->state_0x006++;
                self->em022_0x328.leg_0x332 = 0;
            }
            break;
        }
        case 1: {
            u32 angle;
            if (vec3_dist_sq(&self->pos, &self->vec_0x36C) > 2250000.0f) {
                if ((s32)mode == 0) {
                    angle = 0x3333;
                } else {
                    angle = 0xCCCE;
                }
            } else {
                angle = (s32)mode == 0 ? 0x38E4 : 0xC71D;
            }
            if (self->em022_0x328.alert_0x336 > 0) {
                if (em_approach_step_c1(self, -angle, 0x38E) == 1U) {
                    em_approach_start(self, 0.0f, 0);
                }
            } else if (em_approach_step_c1(self, angle, 0x1F4) == 1U) {
                em_approach_start(self, 0.0f, 0);
            }
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (++self->em022_0x328.leg_0x332 >= 90) {
                self->state_0x006 = 0;
                self->em022_0x328.leg_0x332 = 0;
            }
            break;
        }
        }
        if (self->timer_0x020 > 150 && calcDistanceSqXZ(&self->pos, &self->vec_0x36C) < 1440000.0f) {
            em_action_finish_stand(self);
        }
        if (++self->timer_0x020 >= 450) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 6's watch: motion 1 at the base height for a random time (shorter when the target is behind), then
 * turns to face it. */
void em022_act6_watch(_ENEMY_WORK* self) {
    em_height_set(self, em_height_base_get(self));
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 6, 0);
        {
            u16 diff = calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0;
            if (diff > 0x2000 && diff < 0x6000) {
                self->timer_0x020 = self->bits_0x1EC & 0x1F;
            } else {
                self->timer_0x020 = (self->bits_0x1EC & 0x3F) + 30;
            }
        }
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            self->field_0x1C0 = (u16)calcVecAng2(&self->pos, &self->vec_0x36C);
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 6's dash (modes 0/1): a motion 3 run that bends toward the target at the slow or the fast rate,
 * paced by the distance, until the run budget at +0x378 is spent, then a slowdown. */
void em022_act6_dash(_ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    self->field_0x1C0 = 0;
    switch (self->state) {
    case 0: {
        f32 dist;
        f32 speed;
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 3, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        dist = calcDistanceSqXZ(&self->pos, &self->vec_0x36C);
        if (dist <= 360000.0f) {
            speed = 8.0f;
        } else if (dist <= 4000000.0f) {
            speed = 14.0f;
        } else {
            speed = 20.0f;
        }
        self->field_0x318 = speed;
        if (self->vec_0x36C.z - self->pos.z >= 0.0f) {
            self->em022_0x328.turn_0x330 = 0;
        } else {
            self->em022_0x328.turn_0x330 = 0x8000;
        }
        break;
    }
    case 1: {
        u16 bearing = calcVecAng2(&self->pos, &self->vec_0x36C);
        u16 dir;
        if ((s32)mode == 0) {
            dir = em_angle_approach(bearing, self->em022_0x328.turn_0x330, 0x500);
        } else {
            dir = em_angle_approach(bearing, self->em022_0x328.turn_0x330, 0x1000);
        }
        copyVec3(&step, &self->offset_0x30C.vec_0x310);
        rotVecY(&step, dir);
        addVec3To(&self->pos, &step);
        self->value_0x378 -= calcVecDistXZ(&self->pos, &self->vec_0x194);
        if (self->value_0x378 <= 0.0f) {
            self->state++;
            self->field_0x324 = -0.5f;
        } else {
            em_act_move_step(self, 0, 10.0f);
        }
        break;
    }
    case 2: {
        u16 bearing = calcVecAng2(&self->pos, &self->vec_0x36C);
        u16 dir;
        if ((s32)mode == 0) {
            dir = em_angle_approach(bearing, self->em022_0x328.turn_0x330, 0x500);
        } else {
            dir = em_angle_approach(bearing, self->em022_0x328.turn_0x330, 0x1000);
        }
        copyVec3(&step, &self->offset_0x30C.vec_0x310);
        rotVecY(&step, dir);
        addVec3To(&self->pos, &step);
        if (self->field_0x318 <= 0.0f) {
            em_action_finish_stand(self);
        } else {
            addVec3To(&self->offset_0x30C.vec_0x310, &self->vec_0x31C);
        }
        break;
    }
    }
    self->field_0x1C0 = 0x8000;
}

/* Action 6's charge: a busy motion 3 run at the target, the targeting flags held, until the run budget is
 * spent; then it snaps onto the target point and stands. */
void em022_act6_charge(_ENEMY_WORK* self) {
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    em_motion_timer_arm(self);
    em_busy_set(self);
    self->field_0x1C0 = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 3, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= 360000.0f) {
            self->field_0x318 = 5.0f;
        } else {
            self->field_0x318 = 20.0f;
        }
        if (self->vec_0x36C.z - self->pos.z >= 0.0f) {
            self->em022_0x328.turn_0x330 = 0;
        } else {
            self->em022_0x328.turn_0x330 = 0x8000;
        }
        em_height_set(self, 5.0f * em_height_base_get(self));
        em_reaction_req(self, 6, 0xFF, 0);
        break;
    case 1: {
        u16 dir;
        em_reaction_req(self, 6, 0xFF, 0);
        dir = em_angle_approach(calcVecAng2(&self->pos, &self->vec_0x36C), self->em022_0x328.turn_0x330, 0x4000);
        copyVec3(&step, &self->offset_0x30C.vec_0x310);
        rotVecY(&step, dir);
        addVec3To(&self->pos, &step);
        self->value_0x378 -= calcVecDistXZ(&self->pos, &self->vec_0x194);
        if (self->value_0x378 <= 0.0f) {
            copyVec3(&self->pos, &self->vec_0x36C);
            em_height_set(self, em_height_base_get(self));
            em_action_finish_stand(self);
        }
        break;
    }
    }
    self->field_0x1C0 = 0x8000;
}

/* Action 6's back-off: a motion 3 run for a random time at a random pace (facing round on odd bits), then
 * a slowdown on the move offset. */
void em022_act6_back_off(_ENEMY_WORK* self) {
    self->field_0x1C0 = 0;
    switch (self->state) {
    case 0: {
        u16 bits;
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 3, 6, 0);
        em_move_vec2_clr(self);
        bits = self->bits_0x1EC;
        self->field_0x318 = 5.0f + (f32)(self->bits_0x1EC % 30) / 10.0f;
        self->timer_0x020 = (bits & 0x1F) + 30;
        if (bits & 1) {
            self->state_0x007 = 1;
        }
        break;
    }
    case 1:
        if (--self->timer_0x020 <= 0) {
            self->state++;
            self->field_0x324 = -0.3f;
        } else {
            if (self->state_0x007 == 1) {
                self->field_0x1C0 = 0x8000;
            }
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
    case 2:
        if (self->field_0x318 <= 0.0f) {
            em_action_finish_stand(self);
        } else {
            if (self->state_0x007 == 1) {
                self->field_0x1C0 = 0x8000;
            }
            em_move_offset_step(self, &self->field_0x1BC);
        }
        break;
    }
    self->field_0x1C0 = 0x8000;
}

/* Action 6's busy run: a 60-frame motion 3 run at a random pace with the targeting flags held, then the
 * state 6/17 hand-over. */
void em022_act6_busy_run(_ENEMY_WORK* self) {
    nw4r::math::VEC3 step;

    VEC3_ctor(&step);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    em_motion_timer_arm(self);
    em_busy_set(self);
    em_act_move_step(self, 1, 10.0f);
    self->field_0x1C0 = 0;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 3, 6, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 50.0f + (f32)(self->bits_0x1EC % 30) / 10.0f;
        self->timer_0x020 = 60;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_state_set(self, 6, 0x11);
        } else {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
    }
    self->field_0x1C0 = 0x8000;
}

/* Action 6's rush (modes 0/1): a 300-frame busy motion 3 run at 20 or -20, easing to the base height; the
 * attack is done once the height reaches the base. */
void em022_act6_rush(_ENEMY_WORK* self, u8 mode) {
    em_frame_flag_set(self);
    em_busy_timer_reset(self);
    em_fx_flag_set(self);
    em_motion_timer_arm(self);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 3, 6, 0);
        em_move_vec2_clr(self);
        if ((s32)mode == 0) {
            self->field_0x318 = 20.0f;
        } else {
            self->field_0x318 = -20.0f;
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        self->timer_0x020 = 300;
        break;
    case 1:
        self->timer_0x020--;
        if (self->field_0x1AC <= em_height_base_get(self)) {
            em_attack_done_set(self);
        } else if (self->timer_0x020 <= 0) {
            em_action_finish_stand(self);
        } else {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if ((s32)mode == 0) {
                em_act_move_step(self, 1, 5.0f);
            } else {
                em_act_move_step(self, 1, 3.0f);
            }
        }
        break;
    }
}

/* Action 6's sweep: a 60-frame motion 3 run veering at a fixed side angle. */
void em022_act6_sweep(_ENEMY_WORK* self) {
    em_act_move_step(self, 0, 10.0f);
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set(self, 3, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 30.0f;
        self->timer_0x020 = 60;
        self->em022_0x328.turn_0x330 = 0x3555;
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    case 1:
        em_approach_step_c1(self, self->em022_0x328.turn_0x330, 0x2D8);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (--self->timer_0x020 <= 0) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 6's motion 18: plays it out, then stands. */
void em022_act6_mot18(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x12, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 6's sub-state dispatcher. */
void em022_act6_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em022_act6_rise(self);
        break;
    case 1:
        em022_act6_hop(self, 0);
        break;
    case 2:
        em022_act6_walk(self, 0);
        break;
    case 3:
        em022_act6_run(self, 0);
        break;
    case 4:
        em022_act6_drop(self);
        break;
    case 5:
        em022_act6_turn(self, 0);
        break;
    case 6:
        em022_act6_rise_end(self);
        break;
    case 7:
        em022_act6_mot12(self);
        break;
    case 8:
        em022_act6_face(self);
        break;
    case 9:
        em022_act6_jump(self, 0);
        break;
    case 10:
        em022_act6_run(self, 1);
        break;
    case 11:
        em022_act6_run(self, 2);
        break;
    case 12:
        em022_act6_circle(self);
        break;
    case 13:
        em022_act6_idle(self, 0);
        break;
    case 14:
        em022_act6_walk(self, 1);
        break;
    case 15:
        em022_act6_wander(self, 0);
        break;
    case 16:
        em022_act6_walk(self, 2);
        break;
    case 17:
        em022_act6_mot29(self);
        break;
    case 18:
        em022_act6_turn(self, 1);
        break;
    case 19:
        em022_act6_walk(self, 3);
        break;
    case 20:
        em022_act6_strafe(self, 0);
        break;
    case 21:
        em022_act6_strafe(self, 1);
        break;
    case 22:
        em022_act6_wander(self, 1);
        break;
    case 23:
        em022_act6_jump(self, 1);
        break;
    case 24:
        em022_act6_walk(self, 4);
        break;
    case 25:
        em022_act6_walk(self, 5);
        break;
    case 26:
        em022_act6_watch(self);
        break;
    case 27:
        em022_act6_walk(self, 6);
        break;
    case 28:
        em022_act6_hop(self, 1);
        break;
    case 29:
        em022_act6_dash(self, 0);
        break;
    case 30:
        em022_act6_dash(self, 1);
        break;
    case 31:
        em022_act6_charge(self);
        break;
    case 32:
        em022_act6_idle(self, 1);
        break;
    case 33:
        em022_act6_back_off(self);
        break;
    case 34:
        em022_act6_busy_run(self);
        break;
    case 35:
        em022_act6_rush(self, 0);
        break;
    case 36:
        em022_act6_rush(self, 1);
        break;
    case 37:
        em022_act6_sweep(self);
        break;
    case 38:
        em022_act6_mot18(self);
        break;
    }
}


/* Action 7's bite: motion 8 with the default hit window, turning toward the target over frames 0..60. */
void em022_act7_bite(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 8, 6, 0);
        em_hit_window_set_default(self, 0, 2);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 60.0f) == 1U) {
            em_turn_to_target(self, 0xB6);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 7's snap: motion 9 with the default hit window, turning toward the target over frames 0..50. */
void em022_act7_snap(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 9, 6, 0);
        em_hit_window_set_default(self, 0, 3);
        break;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 50.0f) == 1U) {
            em_turn_to_target(self, 0x92);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 7's spit: motion 13 fires the shell (kind 7) at frame 42 toward the target, slows the motion at
 * frame 12 and restores it at frame 20, steering toward the target between frames 12 and 20. */
void em022_act7_spit(_ENEMY_WORK* self) {
    EmSpawnRec rec;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xD, 4, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = -20.0f;
        break;
    case 1:
        if (em_frame_check(self, 0, 42.0f, 0.0f) == 1U) {
            rec.id = 7;
            setVector3(&rec.pos, 0.0f, 0.0f, 50.0f);
            rec.field_0x10 = 0;
            rec.field_0x12 = calcVecAngX2(&self->pos, &self->vec_0x36C);
            rec.field_0x14 = 0;
            eft007_part_spawn(self, 0x1D, 0, 0);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 14);
        }
        if ((s32)em_frame_check(self, 0, 12.0f, 0.0f) != 0) {
            em_mot_speed_set(self, 0.5f);
        }
        if ((s32)em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
            em_mot_speed_set(self, 1.0f);
        }
        if ((s32)em_frame_check(self, 3, 12.0f, 20.0f) != 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            em_turn_to_target(self, 0x16C);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_act_arm_m06s07(self);
        }
        break;
    }
}

/* Action 7's motion 15 with the default hit window, then the action end. */
void em022_act7_mot15(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7's motion 16 with the default hit window, the height following the motion's key curve. */
void em022_act7_mot16(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 6, 0);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        em_height_set(self, getKeyData((f32*)em022_keys_mot16, em_mot_frame_get(self)));
        if (self->field_0x1AC > 0.0f) {
            em_height_set(self, 0.0f);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7's lunge: motion 27 approaching 700 short of the target with hit window 4, then the action end. */
void em022_act7_lunge(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x1B, 4, 0);
        em_approach_start(self, 700.0f, 0);
        em_hit_window_set(self, 0, 4, 3);
        em_move_vec2_clr(self);
        self->field_0x318 = 30.0f;
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        break;
    case 1:
        if (em_approach_step(self, 0, 0) == 1U) {
            em_hit_window_clear(self, 0);
            em_action_finish_stand(self);
        } else {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        em_act_move_step(self, 0, 10.0f);
        break;
    }
}

/* Action 7's charge: motion 17 at the target with hit window 5, then motion 18 to stand. */
void em022_act7_charge(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 0, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 15.0f;
        em_hit_window_set(self, 0, 5, 3);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            em_hit_window_clear(self, 0);
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 0x12, 6, 0);
        } else {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 7's zone charge (modes 0..2): motion 17 toward a point of the nest zone (0), on along the charge
 * (1) or toward the nearer of the area's points 6/7 (2); arriving, mode 0 leaves when the zone holds the body,
 * modes 1/2 stagger when they end near the target. */
void em022_act7_zone_charge(_ENEMY_WORK* self, u8 mode) {
    PlCapsuleBox shape;
    nw4r::math::VEC3 target;
    nw4r::math::VEC3 point;
    nw4r::math::VEC3 diff;

    eft_shape_init_v(&shape);
    VEC3_ctor(&target);
    VEC3_ctor(&point);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 0, 0);
        switch (mode) {
        case 0:
            em_wave_segment_v(&target, &em022_nest_zone.start_0x04, &em022_nest_zone.end_0x10,
                            em022_nest_zone.radius_0x1C, 0, self->bits_0x1EC);
            em_target_pos_set(self, &target);
            em_reaction_req(self, 6, 0xFF, 0);
            break;
        case 2: {
            f32 dist6;
            f32 dist7;
            em_data_point_get(self, ((self->area_no & 0xF) << 8) | 6, &point);
            dist6 = calcDistanceSqXZ(&self->pos, &point);
            em_data_point_get(self, ((self->area_no & 0xF) << 8) | 7, &point);
            dist7 = calcDistanceSqXZ(&self->pos, &point);
            self->em022_0x328.point_0x338 = 6;
            if (dist6 < dist7) {
                self->em022_0x328.point_0x338 = 6;
            } else {
                self->em022_0x328.point_0x338 = 7;
            }
            em_data_point_get(self, ((self->area_no & 0xF) << 8) | (self->em022_0x328.point_0x338 & 0xFF), &target);
            em_target_pos_set(self, &target);
            em_reaction_req(self, 6, 0xFF, 0);
            break;
        }
        }
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 15.0f;
        em_hit_window_set(self, 0, 5, 3);
        break;
    case 1:
        em_reaction_req(self, 6, 0xFF, 0);
        switch (mode) {
        case 0:
            if (em_approach_step(self, 0, 0x300) == 1U) {
                em_hit_window_clear(self, 0);
                em_action_finish(self);
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            if (self->field_0x43C != 1) {
                capsule_box_from_points_v(&em022_nest_zone.start_0x04, &em022_nest_zone.end_0x10, em022_nest_zone.radius_0x1C,
                            &shape);
                pl_capsule_box_set((PlCapsule*)&shape, &shape);
                if (pl_capsule_point_ck(&self->pos, &shape) == 1U) {
                    em_motion_kind_set(self, 0, 1);
                }
            }
            break;
        case 1:
            if (em_approach_step(self, 0, 0x300) == 1U) {
                em_hit_window_clear(self, 0);
                subVec3(&diff, &self->pos, &self->vec_0x194);
                copyVec3(&target, &diff);
                if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= 90000.0f &&
                    self->vec_0x36C.x * target.x >= 0.0f) {
                    em_state_set(self, 3, 6);
                } else {
                    em_action_finish(self);
                }
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        case 2:
            if (em_approach_step(self, 0, 0x300) == 1U) {
                em_hit_window_clear(self, 0);
                em_data_point_get(self, ((self->area_no & 0xF) << 8) | (self->em022_0x328.point_0x338 & 0xFF), &point);
                if (calcDistanceSqXZ(&self->pos, &point) <= 62500.0f) {
                    em_state_set(self, 3, 6);
                } else {
                    em_action_finish(self);
                }
            } else {
                em_move_offset_rot_apply(self, &self->field_0x1BC);
            }
            break;
        }
        break;
    }
}

/* Action 7's motion 17 with hit window 5, then the action end. */
void em022_act7_mot17(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x11, 0, 0);
        em_hit_window_set(self, 0, 5, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_hit_window_clear(self, 0);
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7's sub-state dispatcher. */
void em022_act7_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em022_act7_bite(self);
        break;
    case 1:
        em022_act7_snap(self);
        break;
    case 2:
        em022_act7_spit(self);
        break;
    case 3:
        em022_act7_mot15(self);
        break;
    case 4:
        em022_act7_mot16(self);
        break;
    case 5:
        em022_act7_lunge(self);
        break;
    case 6:
        em022_act7_charge(self);
        break;
    case 7:
        em022_act7_zone_charge(self, 0);
        break;
    case 8:
        em022_act7_zone_charge(self, 1);
        break;
    case 9:
        em022_act7_mot17(self);
        break;
    case 10:
        em022_act7_zone_charge(self, 2);
        break;
    }
}

/* The sound tables' record types this unit adds: the three-vector block a kind 0x1F entry points at (the
 * static initializer sets the vectors) and the quarter-turn record of a kind 0x20 entry.  size: 0x28 */
struct Em022SeVecs {
    /* +0x00 */ nw4r::math::VEC3 a_0x00;
    /* +0x0C */ nw4r::math::VEC3 b_0x0C;
    /* +0x18 */ nw4r::math::VEC3 c_0x18;
    /* +0x24 */ u8 kind_0x24;
};

/* The loop record the 0xFE terminators point at and the three parameter records the kind 0x16 entries step. */
u32 em022_se_loop = 0x06220000;
f32 em022_se_arg_a = -300.0f;
f32 em022_se_arg_b[2] = {50.0f, 0.0f};
f32 em022_se_arg_c;

/* The shift vector pairs the sound records point at (the static initializer sets them). */
Vec em022_se_shift_a[2];
Vec em022_se_shift_b[2];
Vec em022_se_shift_c[2];
Vec em022_se_shift_d[2];

Em022SeVecs em022_se_52_vecs2 = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1};

EmSeRecord em022_se_52_rec0 = {101, 0, 2, 0, 0, {16, 0, 7, 0}, (const EmSeOffset*)em022_se_shift_a};

EmSeRecord em022_se_52_rec3 = {0, 0, 0, 0, 0, {0, 1, 2, 0}, 0};

EmSeRecord em022_se_52_rec5 = {102, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em022_se_52_rec8 = {105, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

Em022SeVecs em022_se_b_52_vecs0 = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1};

Em022SeVecs em022_se_b_52_vecs13 = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 2};

EmSeRecord em022_se_b_52_rec1 = {101, 0, 2, 0, 0, {16, 0, 7, 0}, 0};

EmSeRecord em022_se_b_52_rec3 = {0, 0, 0, 0, 0, {0, 1, 2, 0}, 0};

EmSeRecord em022_se_b_52_rec5 = {102, 0, 0, 0, 0, {0, 2, 2, 0}, 0};

EmSeRecord em022_se_b_52_rec10 = {0, 0, 0, 0, 0, {0, 4, 7, 0}, 0};

EmSeRecord em022_se_b_52_rec17 = {1, 0, 0, 0, 0, {0, 4, 2, 0}, 0};

EmSeRecord em022_se_5_rec1 = {102, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)em022_se_shift_b};

EmSeRecord em022_se_5_rec4 = {105, 0, 0, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em022_se_b_5_sub2_rec3 = {0, 0, 0, 0, 0, {0, 4, 7, 0}, 0};

EmSeRecord em022_se_0_rec1 = {108, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em022_se_0_rec4 = {109, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)em022_se_shift_c};

EmSeRecord em022_se_0_rec6 = {26, 0, 0, 0, 0, {0, 1, 6, 0}, (const EmSeOffset*)em022_curve_land};

EmSeRecord em022_se_5_rec6 = {107, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em022_se_b_141_rec0 = {110, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em022_se_b_52_rec18 = {29, 0, 2, 0, 0, {16, 0, 7, 0}, (const EmSeOffset*)em022_se_shift_d};

EmSeRecord em022_se_b_52_rec20 = {0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em022_se_0_rec0 = {110, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em022_se_tbl_0[9] = {
    {0x00, {0, 0, 0}, &em022_se_0_rec0},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_0[2] = {
    {0x00, {0, 0, 0}, &em022_se_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_5[15] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

u32 em022_se_b_52_turn12[4] = {0x0, 0x8000, 0x0, 0x0};

EmSeEntry em022_se_b_5_sub2[15] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs13},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec17},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec18},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec20},
    {0xFF, {0, 0, 0}, 0},
};

EmSeBranch em022_se_b_5_branch2 = {0x05000000, 0x1000, em022_se_b_5_sub2};

EmSeEntry em022_se_tbl_b_5[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_5_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em022_se_15_rec1 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em022_se_tbl_15[3] = {
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x00, {0, 0, 0}, &em022_se_15_rec1},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_35[15] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_b_35_sub2[15] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs13},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec17},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec18},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec20},
    {0xFF, {0, 0, 0}, 0},
};

EmSeBranch em022_se_b_35_branch2 = {0x05000000, 0x1000, em022_se_b_35_sub2};

EmSeEntry em022_se_tbl_b_35[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_35_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_52[19] = {
    {0x00, {0, 0, 0}, &em022_se_52_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_52_vecs2},
    {0x01, {0, 0, 0}, &em022_se_52_rec3},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_52_rec5},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_52_rec8},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_52[22] = {
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec3},
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec5},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec10},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs13},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec17},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec18},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec20},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em022_se_122_rec0 = {106, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em022_se_tbl_122[2] = {
    {0x00, {0, 0, 0}, &em022_se_122_rec0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em022_se_b_147_rec11 = {1, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em022_se_123_rec0 = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em022_se_tbl_123[2] = {
    {0x01, {0, 0, 0}, &em022_se_123_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em022_se_b_148_rec0 = {0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em022_se_tbl_b_148[2] = {
    {0x01, {0, 0, 0}, &em022_se_b_148_rec0},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeEntry em022_se_b_129_sub0[3] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x01, {0, 0, 0}, &em022_se_b_148_rec0},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeBranch em022_se_b_129_branch0 = {0x05000000, 0x1000, em022_se_b_129_sub0};

EmSeEntry em022_se_tbl_b_129[4] = {
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_129_branch0},
    {0x04, {0, 0, 0}, 0},
    {0x01, {0, 0, 0}, &em022_se_b_148_rec0},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeRecord em022_se_124_rec0 = {104, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em022_se_tbl_124[2] = {
    {0x00, {0, 0, 0}, &em022_se_124_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em022_se_b_149_rec11 = {1, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em022_se_125_rec0 = {107, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em022_se_tbl_125[2] = {
    {0x00, {0, 0, 0}, &em022_se_125_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em022_se_b_150_rec11 = {1, 0, 6, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em022_se_tbl_128[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_122_rec0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeEntry em022_se_b_128_sub2[8] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_b_147_rec11},
    {0xFD, {0, 0, 0}, 0},
};

EmSeBranch em022_se_b_128_branch2 = {0x05000000, 0x1000, em022_se_b_128_sub2};

EmSeEntry em022_se_tbl_b_128[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_128_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_122_rec0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_130[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_124_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeEntry em022_se_b_130_sub2[8] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_b_149_rec11},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeBranch em022_se_b_130_branch2 = {0x05000000, 0x1000, em022_se_b_130_sub2};

EmSeEntry em022_se_tbl_b_130[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_130_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_124_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_131[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_125_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeEntry em022_se_b_131_sub2[8] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_b_150_rec11},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeBranch em022_se_b_131_branch2 = {0x05000000, 0x1000, em022_se_b_131_sub2};

EmSeEntry em022_se_tbl_b_131[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_131_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_125_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_141[8] = {
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_141[2] = {
    {0x00, {0, 0, 0}, &em022_se_b_141_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_147[11] = {
    {0x00, {0, 0, 0}, &em022_se_52_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_52_vecs2},
    {0x01, {0, 0, 0}, &em022_se_52_rec3},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_52_rec5},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_52_rec8},
    {0x00, {0, 0, 0}, &em022_se_122_rec0},
    {0xFD, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_147[13] = {
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec3},
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec5},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec10},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_b_147_rec11},
    {0xFD, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_149[11] = {
    {0x00, {0, 0, 0}, &em022_se_52_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_52_vecs2},
    {0x01, {0, 0, 0}, &em022_se_52_rec3},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_52_rec5},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_52_rec8},
    {0x00, {0, 0, 0}, &em022_se_124_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_149[13] = {
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec3},
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec5},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec10},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_b_149_rec11},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeEntry em022_se_tbl_150[11] = {
    {0x00, {0, 0, 0}, &em022_se_52_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_52_vecs2},
    {0x01, {0, 0, 0}, &em022_se_52_rec3},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_52_rec5},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_52_rec8},
    {0x00, {0, 0, 0}, &em022_se_125_rec0},
    {0xFE, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_150[13] = {
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec3},
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec5},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec10},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_b_150_rec11},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em022_se_loop},
};

EmSeRecord em022_se_169_rec5 = {107, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em022_se_tbl_169[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_169_rec5},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_b_169_sub2[15] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs13},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec17},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec18},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec20},
    {0xFF, {0, 0, 0}, 0},
};

EmSeBranch em022_se_b_169_branch2 = {0x05000000, 0x1000, em022_se_b_169_sub2};

EmSeEntry em022_se_tbl_b_169[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_169_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_169_rec5},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_175[10] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_175[3] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_176[15] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_b_176_sub2[15] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs13},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec17},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec18},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec20},
    {0xFF, {0, 0, 0}, 0},
};

EmSeBranch em022_se_b_176_branch2 = {0x05000000, 0x1000, em022_se_b_176_sub2};

EmSeEntry em022_se_tbl_b_176[9] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_176_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_178[19] = {
    {0x00, {0, 0, 0}, &em022_se_52_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_52_vecs2},
    {0x01, {0, 0, 0}, &em022_se_52_rec3},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_52_rec5},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4A)},
    {0x00, {0, 0, 0}, &em022_se_52_rec8},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_178[22] = {
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec3},
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec5},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec10},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs13},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x21, {0, 0, 0}, 0},
    {0x22, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec17},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec18},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_c},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec20},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_214[10] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0x00, {0, 0, 0}, &em022_se_0_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_0_rec4},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x00, {0, 0, 0}, &em022_se_0_rec6},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_tbl_b_214[3] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em022_se_5_rec6},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em022_se_alt_0_rec0 = {106, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em022_se_alt_tbl_0[2] = {
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_alt_tbl_5[7] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_alt_b_5_sub2[8] = {
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x16, {0, 0, 0}, (const EmSeRecord*)em022_se_arg_b},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_5_sub2_rec3},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeBranch em022_se_alt_b_5_branch2 = {0x05000000, 0x1000, em022_se_alt_b_5_sub2};

EmSeEntry em022_se_alt_tbl_b_5[8] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_5_rec1},
    {0x11, {0, 0, 0}, (const EmSeRecord*)&em022_se_alt_b_5_branch2},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_5_rec4},
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_alt_tbl_11[11] = {
    {0x00, {0, 0, 0}, &em022_se_52_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_52_vecs2},
    {0x01, {0, 0, 0}, &em022_se_52_rec3},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x32)},
    {0x00, {0, 0, 0}, &em022_se_52_rec5},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em022_se_52_rec8},
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_alt_tbl_b_11[13] = {
    {0x1F, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_vecs0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec3},
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em022_se_b_52_rec5},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x4)},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E)},
    {0x01, {0, 0, 0}, &em022_se_b_52_rec10},
    {0x16, {0, 0, 0}, (const EmSeRecord*)&em022_se_arg_a},
    {0x20, {0, 0, 0}, (const EmSeRecord*)&em022_se_b_52_turn12},
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeEntry em022_se_alt_tbl_56[4] = {
    {0x0D, {0, 0, 0}, 0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em022_se_alt_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};


/* Action 10: plays the sound table of the sub-state at +0x1E6 (the second set while the mode at +0x0A is 1);
 * an unknown sub-state ends the action. */
void em022_act10_se_play(_ENEMY_WORK* self) {
    if ((s32)self->field_0x00A != 1) {
        switch (self->state_sub) {
        case 0:
            em_se_tbl_play(self, em022_se_tbl_0, 0, 0);
            break;
        case 5:
            em_se_tbl_play(self, em022_se_tbl_5, 1, 5);
            break;
        case 15:
            em_se_tbl_play(self, em022_se_tbl_15, 4, 15);
            break;
        case 23:
            em_se_tbl_play(self, em022_se_tbl_0, 0, 23);
            break;
        case 35:
            em_se_tbl_play(self, em022_se_tbl_35, 1, 35);
            break;
        case 52:
            em_se_tbl_play(self, em022_se_tbl_52, 4, 52);
            break;
        case 122:
            em_se_tbl_play(self, em022_se_tbl_122, 0, 122);
            break;
        case 123:
            em_se_tbl_play(self, em022_se_tbl_123, 0, 123);
            break;
        case 124:
            em_se_tbl_play(self, em022_se_tbl_124, 0, 124);
            break;
        case 125:
            em_se_tbl_play(self, em022_se_tbl_125, 0, 125);
            break;
        case 128:
            em_se_tbl_play(self, em022_se_tbl_128, 1, 128);
            break;
        case 129:
            em_se_tbl_play(self, em022_se_tbl_123, 0, 129);
            break;
        case 130:
            em_se_tbl_play(self, em022_se_tbl_130, 1, 130);
            break;
        case 131:
            em_se_tbl_play(self, em022_se_tbl_131, 1, 131);
            break;
        case 141:
            em_se_tbl_play(self, em022_se_tbl_141, 0, 141);
            break;
        case 142:
            em_se_tbl_play(self, em022_se_tbl_141, 0, 142);
            break;
        case 147:
            em_se_tbl_play(self, em022_se_tbl_147, 4, 147);
            break;
        case 148:
            em_se_tbl_play(self, em022_se_tbl_123, 0, 148);
            break;
        case 149:
            em_se_tbl_play(self, em022_se_tbl_149, 4, 149);
            break;
        case 150:
            em_se_tbl_play(self, em022_se_tbl_150, 4, 150);
            break;
        case 159:
            em_se_tbl_play(self, em022_se_tbl_141, 0, 159);
            break;
        case 160:
            em_se_tbl_play(self, em022_se_tbl_141, 0, 160);
            break;
        case 168:
            em_se_tbl_play(self, em022_se_tbl_0, 0, 168);
            break;
        case 169:
            em_se_tbl_play(self, em022_se_tbl_169, 1, 169);
            break;
        case 175:
            em_se_tbl_play(self, em022_se_tbl_175, 0, 175);
            break;
        case 176:
            em_se_tbl_play(self, em022_se_tbl_176, 1, 176);
            break;
        case 178:
            em_se_tbl_play(self, em022_se_tbl_178, 4, 178);
            break;
        case 214:
            em_se_tbl_play(self, em022_se_tbl_214, 0, 214);
            break;
        case 215:
            em_se_tbl_play(self, em022_se_tbl_35, 1, 215);
            break;
        case 217:
            em_se_tbl_play(self, em022_se_tbl_52, 4, 217);
            break;
        default:
            em_action_finish(self);
            break;
        }
    } else {
        switch (self->state_sub) {
        case 0:
            em_se_tbl_play(self, em022_se_tbl_b_0, 0, 0);
            break;
        case 5:
            em_se_tbl_play(self, em022_se_tbl_b_5, 1, 5);
            break;
        case 15:
            em_se_tbl_play(self, em022_se_tbl_15, 4, 15);
            break;
        case 23:
            em_se_tbl_play(self, em022_se_tbl_b_0, 0, 23);
            break;
        case 35:
            em_se_tbl_play(self, em022_se_tbl_b_35, 1, 35);
            break;
        case 52:
            em_se_tbl_play(self, em022_se_tbl_b_52, 4, 52);
            break;
        case 122:
            em_se_tbl_play(self, em022_se_tbl_122, 0, 122);
            break;
        case 123:
            em_se_tbl_play(self, em022_se_tbl_123, 0, 123);
            break;
        case 124:
            em_se_tbl_play(self, em022_se_tbl_124, 0, 124);
            break;
        case 125:
            em_se_tbl_play(self, em022_se_tbl_125, 0, 125);
            break;
        case 128:
            em_se_tbl_play(self, em022_se_tbl_b_128, 1, 128);
            break;
        case 129:
            em_se_tbl_play(self, em022_se_tbl_b_129, 4, 129);
            break;
        case 130:
            em_se_tbl_play(self, em022_se_tbl_b_130, 1, 130);
            break;
        case 131:
            em_se_tbl_play(self, em022_se_tbl_b_131, 1, 131);
            break;
        case 141:
            em_se_tbl_play(self, em022_se_tbl_b_141, 0, 141);
            break;
        case 142:
            em_se_tbl_play(self, em022_se_tbl_b_141, 0, 142);
            break;
        case 147:
            em_se_tbl_play(self, em022_se_tbl_b_147, 4, 147);
            break;
        case 148:
            em_se_tbl_play(self, em022_se_tbl_b_148, 4, 148);
            break;
        case 149:
            em_se_tbl_play(self, em022_se_tbl_b_149, 4, 149);
            break;
        case 150:
            em_se_tbl_play(self, em022_se_tbl_b_150, 4, 150);
            break;
        case 159:
            em_se_tbl_play(self, em022_se_tbl_b_141, 0, 159);
            break;
        case 160:
            em_se_tbl_play(self, em022_se_tbl_b_141, 0, 160);
            break;
        case 168:
            em_se_tbl_play(self, em022_se_tbl_b_0, 0, 168);
            break;
        case 169:
            em_se_tbl_play(self, em022_se_tbl_b_169, 1, 169);
            break;
        case 175:
            em_se_tbl_play(self, em022_se_tbl_b_175, 0, 175);
            break;
        case 176:
            em_se_tbl_play(self, em022_se_tbl_b_176, 1, 176);
            break;
        case 178:
            em_se_tbl_play(self, em022_se_tbl_b_178, 4, 178);
            break;
        case 214:
            em_se_tbl_play(self, em022_se_tbl_b_214, 0, 214);
            break;
        case 215:
            em_se_tbl_play(self, em022_se_tbl_b_35, 1, 215);
            break;
        case 217:
            em_se_tbl_play(self, em022_se_tbl_b_52, 4, 217);
            break;
        default:
            em_action_finish(self);
            break;
        }
    }
}

/* Action 11: plays the alternate sound table of the sub-state at +0x1E6 (table 0 by default). */
void em022_act11_se_play(_ENEMY_WORK* self) {
    if ((s32)self->field_0x00A != 1) {
        switch (self->state_sub) {
        case 0:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_0, 0, 0);
            break;
        case 5:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_5, 1, 5);
            break;
        case 11:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_11, 4, 11);
            break;
        case 56:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_56, 4, 56);
            break;
        default:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_0, 0, 0);
            break;
        }
    } else {
        switch (self->state_sub) {
        case 0:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_0, 0, 0);
            break;
        case 5:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_b_5, 1, 5);
            break;
        case 11:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_b_11, 4, 11);
            break;
        case 56:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_56, 4, 56);
            break;
        default:
            em_se_tbl_play_alt(self, em022_se_alt_tbl_0, 0, 0);
            break;
        }
    }
}

/* Action 12's motion 29 from the base height: lifts the height by 34 a frame over frames 0..18 (capped at 0),
 * then stands. */
void em022_act12_rise(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x1D, 0, 0);
        em_height_set(self, em_height_base_get(self));
        break;
    case 1:
        if ((s32)em_frame_check(self, 3, 0.0f, 18.0f) != 0) {
            em_height_add(self, 34.0f);
            if (self->field_0x1AC > 0.0f) {
                em_height_set(self, 0.0f);
            }
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 12: runs the rise from sub-state 0. */
void em022_act12_run(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em022_act12_rise(self);
    }
}

/* The demo key tables: frame then position (x, y, z) or rotation/height rows, ending at frame -1. */
f32 em022_demo_a_key_pos[128] = {
    300.0f, 2495.37f, -360.89f, -5126.85f,
    318.0f, 2486.39f, -352.82f, -4961.69f,
    337.0f, 2476.37f, -344.3f, -4787.36f,
    353.0f, 2467.36f, -337.13f, -4640.55f,
    368.0f, 2458.93f, -330.17f, -4502.92f,
    376.0f, 2455.08f, -326.17f, -4429.52f,
    382.0f, 2452.81f, -322.97f, -4374.46f,
    391.0f, 2451.13f, -317.34f, -4291.89f,
    399.0f, 2452.71f, -311.28f, -4218.48f,
    408.0f, 2466.36f, -302.23f, -4137.31f,
    418.0f, 2511.73f, -291.06f, -4058.44f,
    428.0f, 2570.26f, -279.88f, -3988.2f,
    438.0f, 2629.34f, -268.71f, -3918.47f,
    448.0f, 2681.63f, -258.43f, -3843.46f,
    457.0f, 2719.47f, -249.76f, -3770.31f,
    466.0f, 2746.8f, -241.72f, -3692.67f,
    481.0f, 2764.5f, -230.1f, -3556.91f,
    494.0f, 2747.48f, -222.4f, -3438.83f,
    508.0f, 2700.6f, -216.77f, -3319.16f,
    522.0f, 2631.57f, -213.69f, -3210.6f,
    529.0f, 2590.6f, -213.01f, -3160.83f,
    537.0f, 2539.9f, -212.81f, -3107.43f,
    547.0f, 2472.4f, -213.23f, -3044.88f,
    558.0f, 2394.37f, -214.32f, -2980.36f,
    573.0f, 2284.0f, -216.47f, -2897.36f,
    586.0f, 2191.16f, -218.82f, -2822.17f,
    604.0f, 2080.28f, -222.61f, -2699.17f,
    622.0f, 1976.94f, -226.4f, -2569.73f,
    679.0f, 1666.24f, -238.77f, -2147.07f,
    736.0f, 1367.64f, -251.14f, -1715.71f,
    770.0f, 1195.61f, -258.67f, -1454.49f,
    -1.0f, 1195.61f, -258.67f, -1454.49f,
};

f32 em022_demo_a_key_rot[86] = {
    300.0f, 2.14f,
    394.0f, 2.14f,
    397.0f, 3.57f,
    399.0f, 5.6f,
    402.0f, 9.51f,
    407.0f, 17.03f,
    410.0f, 21.44f,
    412.0f, 24.01f,
    415.0f, 27.22f,
    418.0f, 29.92f,
    421.0f, 32.09f,
    424.0f, 33.73f,
    427.0f, 34.77f,
    430.0f, 35.14f,
    434.0f, 34.76f,
    438.0f, 33.66f,
    442.0f, 31.92f,
    446.0f, 29.62f,
    451.0f, 26.04f,
    456.0f, 21.81f,
    462.0f, 16.06f,
    469.0f, 8.71f,
    483.0f, -6.78f,
    495.0f, -19.94f,
    502.0f, -27.0f,
    508.0f, -32.43f,
    513.0f, -36.35f,
    518.0f, -39.62f,
    523.0f, -42.24f,
    528.0f, -44.31f,
    534.0f, -46.19f,
    542.0f, -48.01f,
    550.0f, -49.17f,
    558.0f, -49.73f,
    567.0f, -49.83f,
    571.0f, -49.26f,
    575.0f, -48.05f,
    580.0f, -45.91f,
    594.0f, -39.38f,
    598.0f, -38.27f,
    602.0f, -37.86f,
    770.0f, -37.86f,
    -1.0f, -37.86f,
};

f32 em022_demo_a_key_land[124] = {
    1392.0f, 1219.5f, -247.0f, -829.0f,
    1397.0f, 1212.23f, -95.81f, -837.15f,
    1401.0f, 1206.41f, 36.71f, -843.67f,
    1405.0f, 1200.59f, 143.97f, -850.2f,
    1409.0f, 1194.77f, 235.52f, -856.72f,
    1413.0f, 1188.95f, 313.9f, -863.24f,
    1417.0f, 1183.14f, 376.59f, -869.76f,
    1420.0f, 1178.77f, 416.28f, -874.65f,
    1423.0f, 1174.41f, 450.41f, -879.54f,
    1426.0f, 1170.04f, 479.23f, -884.43f,
    1429.0f, 1165.68f, 502.91f, -889.32f,
    1433.0f, 1159.86f, 526.8f, -895.85f,
    1437.0f, 1154.04f, 541.88f, -902.37f,
    1441.0f, 1148.23f, 547.26f, -908.89f,
    1445.0f, 1142.41f, 542.55f, -915.41f,
    1449.0f, 1136.59f, 529.8f, -921.93f,
    1453.0f, 1130.77f, 510.81f, -928.45f,
    1456.0f, 1126.41f, 491.31f, -933.35f,
    1459.0f, 1122.04f, 469.65f, -938.24f,
    1462.0f, 1117.68f, 442.54f, -943.13f,
    1465.0f, 1113.32f, 413.33f, -948.02f,
    1469.0f, 1107.5f, 367.97f, -954.54f,
    1473.0f, 1101.68f, 317.23f, -961.06f,
    1477.0f, 1095.86f, 261.76f, -967.58f,
    1481.0f, 1090.04f, 202.43f, -974.11f,
    1485.0f, 1084.23f, 131.04f, -980.63f,
    1489.0f, 1078.41f, 49.13f, -987.15f,
    1493.0f, 1072.59f, -41.76f, -993.67f,
    1497.0f, 1066.77f, -150.32f, -1000.19f,
    1500.0f, 1062.41f, -247.0f, -1005.08f,
    -1.0f, 1062.41f, -247.0f, -1005.08f,
};

/* Action 13's demo A: walks in at the demo start, turns along the key tables, drops in at the demo time with
 * dust, lands on the landing keys and roars, puffing dust every sixth frame. */
void em022_act13_demo_a(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if ((s32)em_demo_time_ck(66) != 0) {
            self->state++;
            em_mot_set(self, 3, 0, 0);
            em_demo_pos_set(self, 2605.0f, -215.5f, -7220.5f);
            em_demo_rot_set(self, 0.0f, -3.0f, 0.0f);
            em_demo_enable(self);
            em_move_vec_clr(self);
            self->field_0x318 = 18.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            em_move_offset_apply(self);
        }
        break;
    case 2:
        em_move_offset_apply(self);
        if ((s32)em_demo_time_ck(300) != 0) {
            self->state++;
            em_demo_rot_set(self, 0.0f, 2.14f, 0.0f);
        }
        break;
    case 3:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_a_key_pos, em022_demo_a_key_rot, 7, 2);
        if ((s32)em_demo_time_ck(770) != 0) {
            self->state++;
            em_demo_reset(self, 0);
        }
        break;
    case 4:
        if ((s32)em_demo_time_ck(1392) != 0) {
            self->state++;
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x65, 0, 0);
            em_demo_pos_set(self, 1219.5f, -247.0f, -829.0f);
            em_demo_rot_set(self, 0.0f, 120.0f, 0.0f);
            em_demo_enable(self);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
        }
        break;
    case 5:
        em_demo_key3_apply(self, em_demo_frame_get(), em022_demo_a_key_land, 0);
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
        }
        break;
    case 6:
        em_demo_key3_apply(self, em_demo_frame_get(), em022_demo_a_key_land, 0);
        if (em_frame_check(self, 0, 18.0f, 0.0f) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x69, 0, 0);
            eft009_spawn_at_joint(self, 3, 0x57, 0, get_em_scale(self));
        }
        break;
    case 7:
        if ((s32)em_demo_time_ck(1534) != 0) {
            self->state++;
            em_mot_set(self, 0x6B, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    case 8:
        if ((s32)(system_w.field_0x0c % 6) == 0) {
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    }
}

/* Action 13's demo A end: motion 108 at the landing point, then stands. */
void em022_act13_demo_a_end(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x6C, 0, 0);
        em_demo_pos_set(self, 1219.5f, -247.0f, -829.0f);
        em_demo_rot_set(self, 0.0f, 120.0f, 0.0f);
        em_demo_enable(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* The demo key tables: frame then position (x, y, z) or rotation/height rows, ending at frame -1. */
f32 em022_demo_b_key_pos[108] = {
    300.0f, 2082.31f, -394.57f, -5666.29f,
    323.0f, 2072.76f, -392.81f, -5449.85f,
    349.0f, 2064.7f, -388.58f, -5205.22f,
    372.0f, 2060.32f, -382.59f, -4988.82f,
    392.0f, 2058.94f, -375.12f, -4800.64f,
    415.0f, 2057.94f, -363.98f, -4584.24f,
    435.0f, 2051.24f, -352.54f, -4396.07f,
    450.0f, 2035.98f, -343.51f, -4256.04f,
    465.0f, 1997.24f, -334.42f, -4120.89f,
    478.0f, 1919.46f, -328.77f, -4027.3f,
    493.0f, 1829.82f, -322.65f, -3918.54f,
    517.0f, 1724.6f, -311.76f, -3719.56f,
    535.0f, 1698.26f, -303.85f, -3553.47f,
    547.0f, 1714.74f, -297.98f, -3442.01f,
    562.0f, 1764.95f, -287.57f, -3310.68f,
    582.0f, 1885.25f, -270.81f, -3168.64f,
    599.0f, 2021.79f, -254.52f, -3087.13f,
    615.0f, 2156.79f, -236.21f, -3022.69f,
    628.0f, 2258.46f, -221.96f, -2956.21f,
    644.0f, 2367.14f, -207.2f, -2853.13f,
    665.0f, 2491.82f, -189.78f, -2700.7f,
    685.0f, 2601.38f, -173.27f, -2548.28f,
    706.0f, 2714.42f, -154.07f, -2387.01f,
    730.0f, 2843.98f, -129.53f, -2203.59f,
    745.0f, 2925.17f, -113.27f, -2089.11f,
    770.0f, 3057.35f, -85.68f, -1896.31f,
    -1.0f, 3057.35f, -85.68f, -1896.31f,
};

f32 em022_demo_b_key_rot[78] = {
    300.0f, -2.99f,
    434.0f, -2.99f,
    439.0f, -4.11f,
    445.0f, -7.14f,
    451.0f, -12.04f,
    457.0f, -18.84f,
    464.0f, -28.62f,
    470.0f, -34.7f,
    476.0f, -37.53f,
    479.0f, -37.96f,
    482.0f, -37.89f,
    490.0f, -35.64f,
    499.0f, -30.41f,
    504.0f, -26.62f,
    510.0f, -21.46f,
    518.0f, -13.9f,
    542.0f, 9.9f,
    552.0f, 20.27f,
    559.0f, 28.04f,
    569.0f, 40.01f,
    576.0f, 48.02f,
    581.0f, 53.06f,
    588.0f, 58.31f,
    595.0f, 60.08f,
    600.0f, 60.26f,
    605.0f, 59.9f,
    609.0f, 59.18f,
    617.0f, 55.57f,
    626.0f, 50.25f,
    634.0f, 45.7f,
    646.0f, 41.3f,
    654.0f, 39.3f,
    663.0f, 37.64f,
    673.0f, 36.37f,
    685.0f, 35.47f,
    699.0f, 35.04f,
    704.0f, 35.01f,
    770.0f, 35.01f,
    -1.0f, 35.01f,
};

f32 em022_demo_b_key_land[66] = {
    1484.0f, -187.0f,
    1486.0f, -186.99f,
    1489.0f, -107.46f,
    1494.0f, -8.47f,
    1498.0f, 48.61f,
    1502.0f, 96.9f,
    1506.0f, 134.98f,
    1510.0f, 162.72f,
    1514.0f, 184.27f,
    1518.0f, 200.54f,
    1521.0f, 209.83f,
    1524.0f, 217.05f,
    1527.0f, 222.48f,
    1529.0f, 225.05f,
    1531.0f, 226.69f,
    1533.0f, 227.32f,
    1535.0f, 226.86f,
    1537.0f, 225.25f,
    1539.0f, 222.26f,
    1542.0f, 218.81f,
    1545.0f, 213.06f,
    1548.0f, 205.21f,
    1552.0f, 193.06f,
    1556.0f, 178.45f,
    1560.0f, 159.38f,
    1564.0f, 137.77f,
    1568.0f, 113.01f,
    1572.0f, 86.8f,
    1576.0f, 37.39f,
    1580.0f, -33.19f,
    1584.0f, -126.26f,
    1586.0f, -187.0f,
    -1.0f, -187.0f,
};

/* Action 13's demo B: as demo A from another start, landing on its own keys. */
void em022_act13_demo_b(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if ((s32)em_demo_time_ck(66) != 0) {
            self->state++;
            em_mot_set(self, 3, 0, 0);
            em_demo_pos_set(self, 2228.5f, -292.5f, -7758.0f);
            em_demo_rot_set(self, 0.0f, -4.0f, 0.0f);
            em_demo_enable(self);
            em_move_vec_clr(self);
            self->field_0x318 = 18.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            em_move_offset_apply(self);
        }
        break;
    case 2:
        em_move_offset_apply(self);
        if ((s32)em_demo_time_ck(300) != 0) {
            self->state++;
            em_demo_rot_set(self, 0.0f, -2.99f, 0.0f);
        }
        break;
    case 3:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_b_key_pos, em022_demo_b_key_rot, 7, 2);
        if ((s32)em_demo_time_ck(770) != 0) {
            self->state++;
            em_demo_reset(self, 0);
        }
        break;
    case 4:
        if ((s32)em_demo_time_ck(1484) != 0) {
            self->state++;
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x65, 0, 0);
            em_demo_pos_set(self, 1582.5f, -187.0f, -152.0f);
            em_demo_rot_set(self, 0.0f, 32.24f, 0.0f);
            em_demo_enable(self);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
        }
        break;
    case 5:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_b_key_land, NULL, 2, 0);
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
        }
        break;
    case 6:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_b_key_land, NULL, 2, 0);
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x69, 2, 2);
            eft009_spawn_at_joint(self, 3, 0x57, 0, get_em_scale(self));
        }
        break;
    case 7:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x6B, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    case 8:
        if ((s32)(system_w.field_0x0c % 6) == 0) {
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    }
}

/* Action 13's demo B end: motion 108 at the landing point, then stands. */
void em022_act13_demo_b_end(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x6C, 0, 0);
        em_demo_pos_set(self, 1582.5f, -187.0f, -152.0f);
        em_demo_rot_set(self, 0.0f, 45.0f, 0.0f);
        em_demo_enable(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* The demo key tables: frame then position (x, y, z) or rotation/height rows, ending at frame -1. */
f32 em022_demo_c_key_land[40] = {
    1650.0f, 254.63f,
    1653.0f, 254.36f,
    1656.0f, 251.46f,
    1659.0f, 247.56f,
    1662.0f, 242.03f,
    1665.0f, 236.1f,
    1668.0f, 228.15f,
    1671.0f, 216.41f,
    1674.0f, 205.95f,
    1677.0f, 191.83f,
    1680.0f, 176.5f,
    1683.0f, 159.27f,
    1686.0f, 140.51f,
    1689.0f, 121.78f,
    1692.0f, 94.33f,
    1695.0f, 49.66f,
    1698.0f, -6.93f,
    1701.0f, -74.23f,
    1704.0f, -159.5f,
    -1.0f, -159.5f,
};

/* Action 13's demo C: walks in, waits, drops in at the demo time with dust, lands on its keys and roars. */
void em022_act13_demo_c(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if ((s32)em_demo_time_ck(66) != 0) {
            self->state++;
            em_mot_set(self, 3, 0, 0);
            em_demo_pos_set(self, 1730.0f, -377.0f, -7574.0f);
            em_demo_rot_set(self, 0.0f, -13.0f, 0.0f);
            em_demo_enable(self);
            em_move_vec_clr(self);
            self->field_0x318 = 18.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            em_move_offset_apply(self);
        }
        break;
    case 2:
        em_move_offset_apply(self);
        if ((s32)em_demo_time_ck(300) != 0) {
            self->state++;
            em_demo_reset(self, 0);
        }
        break;
    case 3:
        if ((s32)em_demo_time_ck(1650) != 0) {
            self->state++;
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x65, 0, 0x30);
            em_demo_pos_set(self, 1674.0f, -159.5f, 312.0f);
            em_demo_rot_set(self, 0.0f, 61.0f, 0.0f);
            em_demo_enable(self);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
        }
        break;
    case 4:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_c_key_land, NULL, 2, 0);
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
        }
        break;
    case 5:
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x69, 2, 2);
            eft009_spawn_at_joint(self, 3, 0x57, 0, get_em_scale(self));
        }
        break;
    case 6:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x6B, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    case 7:
        if ((s32)(system_w.field_0x0c % 6) == 0) {
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    }
}

/* Action 13's demo C end: motion 108 at the landing point, then stands. */
void em022_act13_demo_c_end(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x6C, 0, 0);
        em_demo_pos_set(self, 1674.0f, -159.5f, 312.0f);
        em_demo_rot_set(self, 0.0f, 61.0f, 0.0f);
        em_demo_enable(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* The demo key tables: frame then position (x, y, z) or rotation/height rows, ending at frame -1. */
f32 em022_demo_d_key_land[70] = {
    1382.0f, -266.0f,
    1384.0f, -242.01f,
    1386.0f, -161.77f,
    1388.0f, -89.85f,
    1391.0f, 1.74f,
    1394.0f, 81.42f,
    1397.0f, 148.6f,
    1400.0f, 208.39f,
    1403.0f, 258.7f,
    1406.0f, 298.75f,
    1409.0f, 331.67f,
    1412.0f, 358.55f,
    1415.0f, 380.12f,
    1418.0f, 397.05f,
    1421.0f, 409.99f,
    1424.0f, 419.53f,
    1427.0f, 425.84f,
    1431.0f, 429.08f,
    1435.0f, 426.24f,
    1441.0f, 413.24f,
    1444.0f, 403.9f,
    1447.0f, 390.31f,
    1450.0f, 373.51f,
    1453.0f, 354.44f,
    1456.0f, 331.07f,
    1459.0f, 304.32f,
    1462.0f, 274.26f,
    1465.0f, 240.68f,
    1468.0f, 205.67f,
    1472.0f, 150.06f,
    1477.0f, 53.45f,
    1482.0f, -60.78f,
    1487.0f, -185.5f,
    1490.0f, -266.0f,
    -1.0f, -266.0f,
};

/* Action 13's demo D: walks in, waits, drops in at the demo time with dust, lands on its keys and roars. */
void em022_act13_demo_d(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        em_height_set(self, 0.0f);
        break;
    case 1:
        if ((s32)em_demo_time_ck(66) != 0) {
            self->state++;
            em_mot_set(self, 3, 0, 0);
            em_demo_pos_set(self, 1185.0f, -360.0f, -7858.0f);
            em_demo_rot_set(self, 0.0f, -15.0f, 0.0f);
            em_demo_enable(self);
            em_move_vec_clr(self);
            self->field_0x318 = 18.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            em_move_offset_apply(self);
        }
        break;
    case 2:
        em_move_offset_apply(self);
        if ((s32)em_demo_time_ck(300) != 0) {
            self->state++;
            em_demo_reset(self, 0);
        }
        break;
    case 3:
        if ((s32)em_demo_time_ck(1382) != 0) {
            self->state++;
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x65, 0, 0);
            em_demo_pos_set(self, 686.0f, -266.0f, -673.5f);
            em_demo_rot_set(self, 0.0f, 191.0f, 0.0f);
            em_demo_enable(self);
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
        }
        break;
    case 4:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_d_key_land, NULL, 2, 0);
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x66, 0, 0);
        }
        break;
    case 5:
        em_demo_key_apply(self, em_demo_frame_get(), em022_demo_d_key_land, NULL, 2, 0);
        if (em_frame_check(self, 0, 18.0f, 0.0f) == 1U) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x69, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x57, 0, get_em_scale(self));
        }
        break;
    case 6:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 0x6B, 2, 0);
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    case 7:
        if ((s32)(system_w.field_0x0c % 6) == 0) {
            eft009_spawn_at_joint(self, 3, 0x56, 0, get_em_scale(self));
        }
        break;
    }
}

/* Action 13's demo D end: motion 108 at the landing point, then stands. */
void em022_act13_demo_d_end(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0x6C, 0, 0);
        em_demo_pos_set(self, 686.0f, -266.0f, -673.5f);
        em_demo_rot_set(self, 0.0f, 191.0f, 0.0f);
        em_demo_enable(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_stand(self);
        }
        break;
    }
}

/* Action 13's sub-state dispatcher: the four demo entrances and their ends. */
void em022_act13_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em022_act13_demo_a(self);
        break;
    case 1:
        em022_act13_demo_a_end(self);
        break;
    case 2:
        em022_act13_demo_b(self);
        break;
    case 3:
        em022_act13_demo_b_end(self);
        break;
    case 4:
        em022_act13_demo_c(self);
        break;
    case 5:
        em022_act13_demo_c_end(self);
        break;
    case 6:
        em022_act13_demo_d(self);
        break;
    case 7:
        em022_act13_demo_d_end(self);
        break;
    }
}


/* The program table's action run: dispatches the current action's sub-state machine. */
void em022_action_run(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em_act_dispatch(self);
        return;
    case 3:
        em022_act3_dispatch(self);
        return;
    case 6:
        em022_act6_dispatch(self);
        return;
    case 7:
        em022_act7_dispatch(self);
        return;
    case 10:
        em022_act10_se_play(self);
        return;
    case 11:
        em022_act11_se_play(self);
        return;
    case 12:
        em022_act12_run(self);
        return;
    case 13:
        em022_act13_dispatch(self);
        return;
    }
}

/* Per-frame motion events: the breath puff on motion 3, the dust trail on motion 17, the landing dust of
 * motions 22 and 26 (with the landing sound in action 10), then the jaw update. */
void em022_motion_events(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (em_get_mot_no(self)) {
    case 3:
        if ((system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 40.0f);
            eft_em_spawn(self, 0x5A, 7, &pos, 2.0f);
        }
        break;
    case 17:
        if ((system_w.field_0x0c & 7) == 0) {
            eft009_spawn_at_joint(self, 3, 0x56, 0, 0.5f);
        }
        break;
    case 22:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 3, 0x57, 0, 0.5f);
        }
        break;
    case 26:
        if (self->action == 10 && em_after_frame_check(self, 0, 2.0f, 0.0f) == 1U) {
            eft009_spawn_at_joint(self, 3, 0x55, 0, get_em_scale(self));
            em022_land_se_play(self);
        }
        break;
    }
    em022_jaw_update(self);
}

/* The program table's empty TEV colour slot. */
void em022_tev_color_update(void) {
}

/* The program table's state query: kind 0 the action-10 entry flag, 1/2 the height below -300 or at 0 and
 * above, 3 the hit bit 0x1000, 4 whether the body stands on the data entry's side, 5 the entry's report byte,
 * 6 the attack timer. */
u8 em022_state_query(_ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        if (self->field_0x380 == 10 && self->field_0x1E7 != 0) {
            return 1;
        }
        return 0;
    case 1:
        return self->field_0x1AC <= -300.0f;
    case 2:
        return self->field_0x1AC >= 0.0f;
    case 3:
        return (self->field_0x228 & 0x1000) != 0;
    case 4: {
        _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A),
                                                           self->field_0x46C);
        if (entry != NULL && ((entry->vec_0x24.x >= 0.0f && self->pos.x >= 0.0f) ||
                              (entry->vec_0x24.x < 0.0f && self->pos.x < 0.0f))) {
            return 1;
        }
        return 0;
    }
    case 5: {
        _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A),
                                                           self->field_0x46C);
        if (entry != NULL) {
            return entry->field_0x36;
        }
        return 0;
    }
    case 6:
        return self->field_0x011 != 0;
    }
    return 0;
}

/* The program table's death action: action 12/0 at the origin, re-entering the nest map's area (kind 2), or
 * approaching the entry's point on the kind-6 map; anywhere else action 0/7. */
void em022_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub) {
    em_move_mode_set(self, 4);
    *action = 12;
    *sub = 0;
    setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
    self->field_0x1C4 = 0;
    self->field_0x1C0 = 0;
    self->field_0x1BC = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 2: {
        _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A),
                                                           self->field_0x46C);
        if (entry != NULL) {
            self->area_no = entry->field_0x14;
            self->field_0x9F6 = entry->field_0x15;
        }
        em_rot_reset(self);
        break;
    }
    case 6: {
        _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A),
                                                           self->field_0x46C);
        if (entry != NULL) {
            if (entry->vec_0x24.x >= 0.0f) {
                em_move_target_set(self, 4, 0xFF, 600.0f);
            } else {
                em_move_target_set(self, 5, 0xFF, 600.0f);
            }
        }
        break;
    }
    default:
        *action = 0;
        *sub = 7;
        setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        break;
    }
}

/* The program table's area change test on the nest map (kind 2): asks for the move to the entry's other area
 * while the motion kind is 1 and the body stands elsewhere, or for the leave transition without an entry. */
u32 em022_area_change_ck(_ENEMY_WORK* self) {
    _ENEMY_DATA* entry;

    if (stage_map_kind_get(self->field_0x1E0) != 2) {
        return 0;
    }
    entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
    if (entry != NULL) {
        if (self->field_0x43B == 1 && ((entry->field_0x0F == 3 && self->area_no != entry->field_0x10) ||
                                       (entry->field_0x0F != 3 && self->area_no != entry->field_0x14))) {
            if (entry->field_0x10 == 0xFF) {
                return 0;
            }
            self->field_0x1FC = 1;
            self->field_0x1FE = 0;
            self->field_0x1FF = 0xFF;
            return 1;
        }
    } else if (em_area_leave_ck(self) == 1) {
        return 1;
    }
    return 0;
}

} /* extern "C" */

/* 0x8032C1FC (0x8): Sets the accessor's flag word to 1 once it is installed. */
void Em022UserData::Attach() {
    em_res_user_data_flag_set_c1((EmUserData*)this, 1);
}

/* 0x8032C204 (0xA8): For an admitted item of node 9, turns the jaw joint by the jaw angle about X. */
void Em022UserData::CalcAnmItemEx(nw4r::g3d::ChrAnmResult* holder, nw4r::g3d::ResMdl mdl,
                                  nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 kind, UserDataItem* item) {
    _ENEMY_WORK* work = this->work;
    nw4r::math::MTX34 mtx;
    nw4r::math::MTX33 rot;

    MTX34_ctor(&mtx);
    MTX33_ctor(&rot);
    if ((s32)item->field_0x04 == 0xFF && kind == 9) {
        u32 angles[3];
        angles[0] = work->em022_0x328.jaw_angle_0x32E;
        angles[2] = 0;
        angles[1] = 0;
        cpSetRotMatrix((struct _CP_VECTOR*)angles, &mtx);
        nw4r::math::MTX34ToMTX33(&rot, &mtx);
        anm_chr_result_rot_mul_v(holder, &rot);
    }
}

extern "C" {

/* Steps the jaw angle: the fast pulse (action 7's bite) and the slow pulse (motion 6) swing it between the open
 * limits once the motion has started, otherwise it eases open while the target is in sight and shut after;
 * action 13 keeps it shut. */
void em022_jaw_update(_ENEMY_WORK* self) {
    if (self->action == 13) {
        self->em022_0x328.jaw_angle_0x32E = 0;
        return;
    }
    if (self->em022_0x328.pulse_a_0x32A != 0) {
        if (self->em022_0x328.jaw_angle_0x32E < 0x18D2) {
            if ((s32)em_mot_frame_started_ck(self) == 0) {
                self->em022_0x328.jaw_angle_0x32E += 0x634;
                if (self->em022_0x328.jaw_angle_0x32E > 0x18D2) {
                    self->em022_0x328.jaw_angle_0x32E = 0x18D2;
                }
            }
        } else {
            if (self->em022_0x328.pulse_a_0x32A <= 2) {
                self->em022_0x328.jaw_angle_0x32E += 0xAAA;
                if ((s16)self->em022_0x328.jaw_angle_0x32E > 0x2E39) {
                    self->em022_0x328.jaw_angle_0x32E = 0x2E39;
                }
            } else {
                self->em022_0x328.jaw_angle_0x32E -= 0xAAA;
                if ((s16)self->em022_0x328.jaw_angle_0x32E < 0x18E4) {
                    self->em022_0x328.jaw_angle_0x32E = 0x18E4;
                }
            }
            if ((s32)++self->em022_0x328.pulse_a_0x32A > 4) {
                self->em022_0x328.pulse_a_0x32A = 1;
            }
        }
    } else if (self->em022_0x328.pulse_b_0x32C != 0) {
        if (self->em022_0x328.jaw_angle_0x32E < 0x18D2) {
            if ((s32)em_mot_frame_started_ck(self) == 0) {
                self->em022_0x328.jaw_angle_0x32E += 0x31A;
                if (self->em022_0x328.jaw_angle_0x32E > 0x18D2) {
                    self->em022_0x328.jaw_angle_0x32E = 0x18D2;
                }
            }
        } else {
            if (self->em022_0x328.pulse_b_0x32C <= 5) {
                self->em022_0x328.jaw_angle_0x32E += 0x444;
                if ((s16)self->em022_0x328.jaw_angle_0x32E > 0x2E39) {
                    self->em022_0x328.jaw_angle_0x32E = 0x2E39;
                }
            } else {
                self->em022_0x328.jaw_angle_0x32E -= 0x444;
                if ((s16)self->em022_0x328.jaw_angle_0x32E < 0x18E4) {
                    self->em022_0x328.jaw_angle_0x32E = 0x18E4;
                }
            }
            if ((s32)++self->em022_0x328.pulse_b_0x32C > 10) {
                self->em022_0x328.pulse_b_0x32C = 1;
            }
        }
    } else if (self->em022_0x328.jaw_open_0x328 != 0) {
        self->em022_0x328.jaw_angle_0x32E += 0x666;
        if ((s16)self->em022_0x328.jaw_angle_0x32E > 0x4000) {
            self->em022_0x328.jaw_angle_0x32E = 0x4000;
        }
    } else {
        self->em022_0x328.jaw_angle_0x32E -= 0x666;
        if ((s16)self->em022_0x328.jaw_angle_0x32E < 0) {
            self->em022_0x328.jaw_angle_0x32E = 0;
        }
    }
}

/* The program table's mini-work init: disarms the move timer and, for modes 1 and 4, takes the entry's two
 * areas. */
void em022_mini_init(_ENEMY_MINI_WORK* mini, u8 mode) {
    mini->move_timer_0x22 = -1;
    if ((s32)mode == 1 || (s32)mode == 4) {
        _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(mini->monster_0x02, mini->team_0x03),
                                                           mini->entry_0x09);
        if (entry != NULL) {
            mini->area_0x0A = entry->field_0x14;
            mini->area_prev_0x0B = entry->field_0x15;
        }
    }
}

/* The program table's mini-work step: arms a random 30..329-frame move timer when the mini work stands outside
 * the entry's area (or the entry is in mode 5), and moves it to the entry's area when the timer runs out. */
void em022_mini_step(_ENEMY_MINI_WORK* mini) {
    _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(mini->monster_0x02, mini->team_0x03),
                                                       mini->entry_0x09);
    if (entry != NULL) {
        if (mini->move_timer_0x22 <= -1) {
            if (mini->area_0x0A != entry->field_0x14 || entry->field_0x08 == 5) {
                mini->move_timer_0x22 = ran_suu(1) % 300 + 30;
            }
        } else if (--mini->move_timer_0x22 <= 0) {
            mini->move_timer_0x22 = 0;
            mini->area_prev_0x0B = mini->area_0x0A;
            if (entry->field_0x0F == 3) {
                mini->area_0x0A = entry->field_0x10;
            } else {
                mini->area_0x0A = entry->field_0x14;
            }
        }
    }
}

} /* extern "C" */

/* 0x8032C600 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em022UserData::~Em022UserData() {
}

extern "C" {



/* The file-scope object whose constructor seeds the nest zone's end points, the sound shift pairs and the
 * three-vector sound blocks (the compiler runs it from the static initializer).  size: 0x01 */
struct Em022StaticInit {
    Em022StaticInit() {
        nw4r::math::VEC3 v18;
        nw4r::math::VEC3 v17;
        nw4r::math::VEC3 v16;
        nw4r::math::VEC3 v15;
        nw4r::math::VEC3 v14;
        nw4r::math::VEC3 v13;
        nw4r::math::VEC3 v12;
        nw4r::math::VEC3 v11;
        nw4r::math::VEC3 v10;
        nw4r::math::VEC3 v9;
        nw4r::math::VEC3 v8;
        nw4r::math::VEC3 v7;
        nw4r::math::VEC3 v6;
        nw4r::math::VEC3 v5;
        nw4r::math::VEC3 v4;
        nw4r::math::VEC3 v3;
        nw4r::math::VEC3 v2;
        nw4r::math::VEC3 v1;
        nw4r::math::VEC3 v0;

        assignVec3((Vec*)&em022_nest_zone.start_0x04, (Vec*)setVec3(&v0, 15.6f, 242.7f, 427.5f));
        assignVec3((Vec*)&em022_nest_zone.end_0x10, (Vec*)setVec3(&v1, -1.4f, 242.7f, -165.5f));
        assignVec3(&em022_se_shift_a[0], (Vec*)setVec3(&v2, 0.0f, 60.0f, 0.0f));
        assignVec3(&em022_se_shift_a[1], (Vec*)setVec3(&v3, 0.0f, 0.0f, 0.0f));
        assignVec3((Vec*)&em022_se_52_vecs2.a_0x00, (Vec*)setVec3(&v4, 0.0f, 60.0f, 0.0f));
        assignVec3((Vec*)&em022_se_52_vecs2.b_0x0C, (Vec*)setVec3(&v5, 0.0f, -2.3f, 0.0f));
        assignVec3((Vec*)&em022_se_52_vecs2.c_0x18, (Vec*)setVec3(&v6, 0.0f, 8.0f, 0.0f));
        assignVec3((Vec*)&em022_se_b_52_vecs0.a_0x00, (Vec*)setVec3(&v7, 0.0f, 60.0f, 0.0f));
        assignVec3((Vec*)&em022_se_b_52_vecs0.b_0x0C, (Vec*)setVec3(&v8, 0.0f, -2.3f, 0.0f));
        assignVec3((Vec*)&em022_se_b_52_vecs0.c_0x18, (Vec*)setVec3(&v9, 0.0f, 5.0f, 0.0f));
        assignVec3((Vec*)&em022_se_b_52_vecs13.a_0x00, (Vec*)setVec3(&v10, 0.0f, 0.0f, -20.0f));
        assignVec3((Vec*)&em022_se_b_52_vecs13.b_0x0C, (Vec*)setVec3(&v11, 0.0f, 0.0f, 0.0f));
        assignVec3((Vec*)&em022_se_b_52_vecs13.c_0x18, (Vec*)setVec3(&v12, 0.0f, 0.0f, 10.0f));
        assignVec3(&em022_se_shift_b[0], (Vec*)setVec3(&v13, 0.0f, 60.0f, 0.0f));
        assignVec3(&em022_se_shift_b[1], (Vec*)setVec3(&v14, 0.0f, -12.0f, 0.0f));
        assignVec3(&em022_se_shift_c[0], (Vec*)setVec3(&v15, 0.0f, 85.0f, 0.0f));
        assignVec3(&em022_se_shift_c[1], (Vec*)setVec3(&v16, 0.0f, -5.0f, 0.0f));
        assignVec3(&em022_se_shift_d[0], (Vec*)setVec3(&v17, 0.0f, 34.0f, 0.0f));
        assignVec3(&em022_se_shift_d[1], (Vec*)setVec3(&v18, 0.0f, 0.0f, 0.0f));
    }
};

static Em022StaticInit em022_static_init;

} /* extern "C" */
