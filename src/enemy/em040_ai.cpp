/* enemy/em040_ai.cpp - the em040 enemy's program: the `em040_prog_tbl` entry points (init, frame update, action
 *   dispatch, motion-next, event, motion events, tint, state query, area start, gauge, area-record keep), the leap
 *   between two ground points, the wait for the leader's call, the demo (cut-scene) steps and the sound program.
 * RANGE. .text 0x801B98C8-0x801BB758 (31 functions); .data 0x805B2808-0x805B28D0, .sdata2 0x80798D88-0x80798E40,
 *   extab, extabindex.
 * SEAM. Left edge: `fn_801B985C` (`.ctors` 0x8056F34C) is `enemy/fn_801B7020.cpp`'s static initializer, which MWCC
 *   emits last in a TU, and the 0.0 pool entry repeats at `lbl_80798DA4` from `fn_801B98C8` on (window
 *   0x801B98C8-0x801B9968).  Right edge: the 0.0 entry repeats again at `lbl_80798E40`, first read by `fn_801BB758`
 *   alone, and the strong `.sdata2` cut there agrees.
 * FLAGS. `cflags_main`; `#pragma peephole off` file-wide (retail keeps `rlwinm` + `cmpwi` and `addi` + `cmpwi`
 *   pairs), `#pragma pool_data off` (the sound table is addressed by name), `#pragma fp_contract off`.
 * NAMES. `em040_ai` is a GUESS from the data order: `em040_prog_tbl` (0x805B2808) is the first object of the unit's
 *   `.data` (em036's starts at `em036_prog_tbl` 0x805B2118, em006's at `em006_prog_tbl` 0x805B28D0), the shape
 *   `enemy/em019_ai.cpp` and `enemy/em024_ai.cpp` follow.
 *   GUESS (the `Em040ProgTbl` slot each fills, after `Em035ProgTbl`): em040_init, em040_frame_update,
 *   em040_action_dispatch, em040_motion_next, em040_event_notify, em040_motion_events, em040_tint_update,
 *   em040_state_ck, em040_area_start_set, em040_gauge_update, em040_area_keep_set.
 *   GUESS (from the bodies): em040_leader_find (the live kind-9 work this one spawned with), em040_act0_wait,
 *   em040_act0_dispatch, em040_act1_wait_call, em040_act1_dispatch, em040_act2_leap, em040_act2_dispatch,
 *   em040_act10_finish, em040_act11_se_play, em040_act13_<sub-state>, em040_act13_dispatch.
 *   GUESS (data): em040_se_rec_0, em040_se_tbl; the `em040_0x328` view and its fields.
 * RESIDUALS. none in the bodies, `.data` or `.sdata2`; flipcheck: the callee `eft_em_spawn_tinted_ex` (0x80305930) is
 *   the name registry's proposal (lane enemy-small3) and is not a map name yet, so a flip links `undefined` until it
 *   lands.
 * SHAPES. `#pragma fp_contract off` keeps the leap's `fmuls`/`fsubs` pairs.  The em040 fields live in the work's
 *   `em040_0x328` view; the leap's drift vector at +0x34C runs past the +0x328 union, so it is read through
 *   `em040_drift`.  The 16 bytes the area record keeps (`data_0xB04`/`data_0x34`) are read through `EmRunPos`.
 *   VEC3 locals are declared in descending stack order; `default:` is laid out first where retail places it first.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h"
#include "gx.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/em_se_record.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "unsplit/enemy.h"
#include "ef/eft019.h"
#include "ef/get_move_work_adrs.h"
#include "ef/mtx34_trans_get.h"
#include "ef/cp_vector.h"
#include "fn_8004CAD8.h"
#include "fn_8004CAD8/mtx.h"
#include "fn_80047398.h"
#include "g3d/g3d_calcworld.h"
#include "stage/stg_w.h"
#include "stage/shell_set_func_ptr.h"
#include "sound/fn_800D7F54.h"
#include "Pl/pl_coll.h"
#include "lobby/eft_em_spawn_tinted_ex.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
/* The +0x14 shell-table slot spelled without its trailing table argument: the call site passes none. */
typedef void (*ShellPosJobFn)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 kind, u32 area, u16 group);

#define em_water_check_c1 ((u32 (*)(struct _ENEMY_WORK*))em_water_check)
#define em_work_die_ck_c1 ((u32 (*)(struct _ENEMY_WORK*))em_work_die_ck)
#define em_act_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)

/* The leap's drift vector at +0x34C: it runs past the end of the +0x328 union into the +0x354 word, so the
 * `em040_0x328` view holds its first two floats and the code reads the three as a vector. */
#define em040_drift(self) ((nw4r::math::VEC3*)(self)->em040_0x328.drift_0x34C)

/* The em040 program table (`em040_prog_tbl`): the entry points the shared enemy program driver calls.  Slot names
 * follow `Em035ProgTbl` (`enemy/em035_prog.cpp`), a GUESS from the slots the two tables share.
 * size: 0x70 */
struct Em040ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x14 */ void (*event_notify_0x14)(void);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tint_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ u32 (*state_ck_0x24)(void);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*area_start_set_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*gauge_update_0x34)(void);
    /* +0x38 */ u32 pad_0x38[0x0C];
    /* +0x68 */ void (*area_keep_set_0x68)(struct _ENEMY_MINI_WORK* mini, nw4r::math::VEC3* pos,
                                           struct EmAreaKeepSrc* src, u8 flags);
    /* +0x6C */ u32 pad_0x6C;
};

/* The spawn-placement source the area-record keep slot reads: the leap flag, the rotation and the third angle.
 * size: 0x0C (the extent this unit's body reaches) */
struct EmAreaKeepSrc {
    /* +0x00 */ u32 leap_0x00;
    /* +0x04 */ u32 rot_0x04;
    /* +0x08 */ u32 angle_0x08;
};

extern "C" {

struct _ENEMY_WORK* em040_leader_find(struct _ENEMY_WORK* self);
void em040_init(struct _ENEMY_WORK* self, u8 mode);
void em040_motion_next(struct _ENEMY_WORK* self, u8* action, u8* sub);
void em040_event_notify(void);
void em040_frame_update(struct _ENEMY_WORK* self);
void em040_act0_wait(struct _ENEMY_WORK* self);
void em040_act0_dispatch(struct _ENEMY_WORK* self);
void em040_act1_wait_call(struct _ENEMY_WORK* self);
void em040_act1_dispatch(struct _ENEMY_WORK* self);
void em040_act2_leap(struct _ENEMY_WORK* self, u8 kind);
void em040_act2_dispatch(struct _ENEMY_WORK* self);
void em040_act10_finish(struct _ENEMY_WORK* self);
void em040_act11_se_play(struct _ENEMY_WORK* self);
void em040_act13_0(struct _ENEMY_WORK* self);
void em040_act13_1(struct _ENEMY_WORK* self);
void em040_act13_2(struct _ENEMY_WORK* self);
void em040_act13_3(struct _ENEMY_WORK* self);
void em040_act13_4(struct _ENEMY_WORK* self);
void em040_act13_5(struct _ENEMY_WORK* self);
void em040_act13_6(struct _ENEMY_WORK* self);
void em040_act13_7(struct _ENEMY_WORK* self);
void em040_act13_8(struct _ENEMY_WORK* self);
void em040_act13_9(struct _ENEMY_WORK* self);
void em040_act13_dispatch(struct _ENEMY_WORK* self);
void em040_action_dispatch(struct _ENEMY_WORK* self);
void em040_motion_events(struct _ENEMY_WORK* self);
void em040_tint_update(struct _ENEMY_WORK* self);
u32 em040_state_ck(void);
void em040_area_start_set(struct _ENEMY_WORK* self, u8* action, u8* sub);
void em040_gauge_update(void);
void em040_area_keep_set(struct _ENEMY_MINI_WORK* mini, nw4r::math::VEC3* pos, struct EmAreaKeepSrc* src, u8 flags);

Em040ProgTbl em040_prog_tbl = {
    0,
    em040_init,
    em040_frame_update,
    em040_action_dispatch,
    em040_motion_next,
    em040_event_notify,
    em040_motion_events,
    0,
    em040_tint_update,
    em040_state_ck,
    {0, 0},
    em040_area_start_set,
    em040_gauge_update,
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    em040_area_keep_set,
    0,
};

/* The landing sound program `em040_act11_se_play` hands `em_se_tbl_play_alt`: one record behind a 0x0D entry. */
EmSeRecord em040_se_rec_0 = {1, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em040_se_tbl[3] = {
    {0x0D, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em040_se_rec_0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801B98C8 (0xA0): returns the live kind-9 work this one was spawned with (its order matches ours), or NULL. */
struct _ENEMY_WORK* em040_leader_find(struct _ENEMY_WORK* self) {
    struct _ENEMY_WORK* work = (struct _ENEMY_WORK*)get_move_work_adrs(3);
    u32 max = get_move_work_max(3);
    u16 i;

    if (work != NULL) {
        for (i = 0; i < (u16)max; i++, work++) {
            if (work->active != 0 && work->team == 9 && work->field_0x01A == self->link_order_0x01C) {
                return work;
            }
        }
    }
    return NULL;
}

/* 0x801B9968 (0x670): scales the work by its variant, arms the call timers and, for the leaping variant (8),
 * restores or records the area record and plans the leap from the saved position. */
void em040_init(struct _ENEMY_WORK* self, u8 mode) {
    EmRunPos* keep = &self->run_0xB04;
    nw4r::math::VEC3 v;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    s32 high;
    s32 placed;
    f32 reach;
    f32 top;
    f32 land;
    f32 ground;

    VEC3_ctor(&v);
    high = 0;
    placed = 0;
    self->em040_0x328.tint_done_0x328 = 0;
    self->em040_0x328.escape_0x329 = 0;
    self->em040_0x328.call_delay_0x32C = -1;
    self->em040_0x328.call_state_0x32E = 0;
    if (self->field_0x00A & 2) {
        self->field_0x1CC *= 2.5f;
    }
    if (self->field_0x00A & 4) {
        self->field_0x1CC *= 0.5f;
        em_max_hp_set(self, 20);
    }
    if (self->field_0x00A == 8) {
        self->field_0x1CC = 0.3f;
        em_max_hp_set(self, 5);
        self->em040_0x328.call_timer_0x32A = 1800;
        self->em040_0x328.leader_0x330 = em040_leader_find(self);
    }
    switch (self->field_0x00A) {
    default:
        self->field_0x761 = 1;
        break;
    case 1:
    case 3:
        self->field_0x761 = 2;
        break;
    case 4:
    case 8:
        self->field_0x761 = 4;
        break;
    }
    switch (mode) {
    case 0:
        if (self->field_0x00A == 8 && keep->valid_0x0F != 0) {
            copyVec3(&self->pos, &keep->pos);
            self->field_0x1C0 = keep->angle;
            if (keep->flags & 1) {
                self->field_0x1BC = 0;
                reach = 500.0f;
                land = 250.0f;
            } else {
                reach = 1300.0f;
                land = 300.0f;
            }
            if (keep->flags & 2) {
                self->field_0x1C4 = 0;
            }
            setVector3(&v, 0.0f, 0.0f, reach);
            rotVecY(&v, self->field_0x1C0);
            addVec3(&sp38, &self->pos, &v);
            copyVec3(&self->em040_0x328.peak_0x340, &sp38);
            if (self->pos.y - self->field_0x20C < 250.0f && self->pos.y > self->field_0x20C) {
                top = 250.0f + self->field_0x20C;
            } else {
                top = self->pos.y;
            }
            ground = GetGroundHit(&self->em040_0x328.peak_0x340, 0xDFFB, self->area_no);
            if (ground < top) {
                self->em040_0x328.peak_0x340.y = ground;
            } else {
                high = 1;
                self->em040_0x328.peak_0x340.y = self->pos.y - 250.0f;
                self->em040_0x328.escape_0x329 = 1;
            }
            if (high == 0) {
                setVector3(&v, 0.0f, 0.0f, land);
                rotVecY(&v, self->field_0x1C0);
                addVec3(&sp2C, &self->em040_0x328.peak_0x340, &v);
                copyVec3(&self->em040_0x328.land_0x334, &sp2C);
                ground = GetGroundHit(&self->em040_0x328.land_0x334, 0xDFFB, self->area_no);
                if (ground < 250.0f + self->em040_0x328.peak_0x340.y) {
                    self->em040_0x328.land_0x334.y = ground;
                } else {
                    self->em040_0x328.land_0x334.y = self->em040_0x328.peak_0x340.y;
                    self->em040_0x328.escape_0x329 = 1;
                }
            } else {
                copyVec3(&self->em040_0x328.land_0x334, &self->em040_0x328.peak_0x340);
            }
            copyVec3(&self->pos, &self->em040_0x328.land_0x334);
            if (self->em040_0x328.escape_0x329 == 1) {
                em_act_arm_unless_down(self, 11, 56);
                em_state_refresh(self);
                placed = 1;
            }
        }
        break;
    case 2:
        if (self->field_0x00A == 8) {
            if (keep->valid_0x0F == 0) {
                copyVec3(&keep->pos, &self->pos);
                keep->angle = self->field_0x1C0;
                keep->flags = 0;
                if (self->field_0x1BC != 0) {
                    keep->flags |= 1;
                }
                if (self->field_0x1C4 != 0) {
                    keep->flags |= 2;
                }
                keep->valid_0x0F = 1;
                if (self->field_0x1BC == 0) {
                    em_act_arm_unless_down(self, 2, 0);
                    reach = 1300.0f;
                    land = 300.0f;
                } else {
                    self->field_0x1BC = 0;
                    em_act_arm_unless_down(self, 2, 1);
                    reach = 500.0f;
                    land = 250.0f;
                }
                if (self->field_0x1C4 == 0) {
                    self->em040_0x328.se_alt_0x32F = 0;
                } else {
                    self->field_0x1C4 = 0;
                    self->em040_0x328.se_alt_0x32F = 1;
                }
            } else {
                copyVec3(&self->pos, &keep->pos);
                self->field_0x1C0 = keep->angle;
                if (keep->flags & 1) {
                    self->field_0x1BC = 0;
                    em_act_arm_unless_down(self, 2, 1);
                    reach = 500.0f;
                    land = 250.0f;
                } else {
                    em_act_arm_unless_down(self, 2, 0);
                    reach = 1300.0f;
                    land = 300.0f;
                }
                if (keep->flags & 2) {
                    self->field_0x1C4 = 0;
                    self->em040_0x328.se_alt_0x32F = 1;
                } else {
                    self->em040_0x328.se_alt_0x32F = 0;
                }
            }
            setVector3(&v, 0.0f, 0.0f, reach);
            rotVecY(&v, self->field_0x1C0);
            addVec3(&sp20, &self->pos, &v);
            copyVec3(&self->em040_0x328.peak_0x340, &sp20);
            if (self->pos.y - self->field_0x20C < 250.0f && self->pos.y > self->field_0x20C) {
                top = 250.0f + self->field_0x20C;
            } else {
                top = self->pos.y;
            }
            ground = GetGroundHit(&self->em040_0x328.peak_0x340, 0xDFFB, self->area_no);
            if (ground < top) {
                self->em040_0x328.peak_0x340.y = ground;
            } else {
                high = 1;
                self->em040_0x328.peak_0x340.y = self->pos.y - 250.0f;
                self->em040_0x328.escape_0x329 = 1;
            }
            if (high == 0) {
                setVector3(&v, 0.0f, 0.0f, land);
                rotVecY(&v, self->field_0x1C0);
                addVec3(&sp14, &self->em040_0x328.peak_0x340, &v);
                copyVec3(&self->em040_0x328.land_0x334, &sp14);
                ground = GetGroundHit(&self->em040_0x328.land_0x334, 0xDFFB, self->area_no);
                if (ground < 250.0f + self->em040_0x328.peak_0x340.y) {
                    self->em040_0x328.land_0x334.y = ground;
                } else {
                    self->em040_0x328.land_0x334.y = self->em040_0x328.peak_0x340.y;
                    self->em040_0x328.escape_0x329 = 1;
                }
            } else {
                copyVec3(&self->em040_0x328.land_0x334, &self->em040_0x328.peak_0x340);
            }
            subVec3(&sp8, &self->pos, &self->em040_0x328.land_0x334);
            copyVec3(em040_drift(self), &sp8);
            copyVec3(&self->pos, &self->em040_0x328.land_0x334);
            if (self->em040_0x328.escape_0x329 == 1) {
                keep->flags |= 4;
            }
            em_state_refresh(self);
            placed = 1;
        }
        break;
    }
    if (self->field_0x00A == 8 && placed == 0) {
        em_act_arm_unless_down(self, 1, 0);
        em_state_refresh(self);
    }
    if (self->field_0x00A & 1) {
        em_busy_timer_reset(self);
    }
}

/* 0x801B9FD8 (0x2C): the variant-1 work skips the action-11 entry. */
void em040_motion_next(struct _ENEMY_WORK* self, u8* action, u8* sub) {
    if ((self->field_0x00A & 1) && *action == 11) {
        *action = 0;
        *sub = 0;
    }
}

/* 0x801BA004 (0x4): the event slot; em040 reacts to none. */
void em040_event_notify(void) {}

/* 0x801BA008 (0x6C): the per-frame step: the variant's height, the leaping variant's effect flag and the busy
 * timer. */
void em040_frame_update(struct _ENEMY_WORK* self) {
    if (self->field_0x00A & 4) {
        em_height_set(self, -200.0f);
    }
    if (self->field_0x00A == 8) {
        em_fx_flag2_set(self);
    }
    if (self->field_0x00A & 1) {
        em_busy_timer_reset(self);
    }
}

/* 0x801BA074 (0x7C): plays motion 1 once, then finishes the action. */
void em040_act0_wait(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801BA0F0 (0x3C): action 0's sub-states 0..3. */
void em040_act0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em040_act0_wait(self);
        break;
    case 1:
        em040_act0_wait(self);
        break;
    case 2:
        em040_act0_wait(self);
        break;
    case 3:
        em040_act0_wait(self);
        break;
    }
}

/* 0x801BA12C (0x294): waits for the leader's call (a nearby kind-9 work in its call motion), then takes its
 * place in the line of answering em040s and leaves through action 11. */
void em040_act1_wait_call(struct _ENEMY_WORK* self) {
    struct _ENEMY_WORK* work;
    u32 max;
    u16 i;
    s32 rank;

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        break;
    case 1:
        switch (self->em040_0x328.call_state_0x32E) {
        case 0:
            work = (struct _ENEMY_WORK*)get_move_work_adrs(3);
            max = get_move_work_max(3);
            if (work != NULL) {
                for (i = 0; i < (u16)max; i++, work++) {
                    if (em_work_die_ck_c1(work) != 1 && work->team == 9
                        && work->field_0x01A == self->link_order_0x01C && work->area_no == self->area_no
                        && em_busy_ck(work) == 1 && em_act_ck_c1(work, 7, 0) == 1
                        && em_frame_check(work, 0, 184.0f, 0.0f) == 1
                        && vec3_dist_sq(&self->pos, &work->pos) <= 9000000.0f) {
                        self->em040_0x328.call_state_0x32E = 1;
                    }
                }
            }
            if (em_magma_check(self) == 1) {
                self->em040_0x328.call_state_0x32E = 1;
            }
            if (--self->em040_0x328.call_timer_0x32A <= 0) {
                self->em040_0x328.call_state_0x32E = 1;
            }
            break;
        case 1:
            if (self->em040_0x328.call_delay_0x32C == -1) {
                rank = 0;
                work = (struct _ENEMY_WORK*)get_move_work_adrs(3);
                max = get_move_work_max(3);
                if (work != NULL) {
                    for (i = 0; i < (u16)max; i++, work++) {
                        if (em_work_die_ck_c1(work) != 1 && work->team == 40 && work->field_0x00A == 8
                            && work->area_no == self->area_no && work->em040_0x328.call_state_0x32E == 1) {
                            if (self == work) {
                                self->em040_0x328.call_delay_0x32C = rank * 10 + 5;
                                break;
                            }
                            rank++;
                        }
                    }
                }
            } else if (self->em040_0x328.call_delay_0x32C > 0) {
                self->em040_0x328.call_delay_0x32C -= 1;
            } else {
                self->em040_0x328.call_state_0x32E = 2;
                em_state_set(self, 11, 56);
            }
            break;
        }
        break;
    }
}

/* 0x801BA3C0 (0x14): action 1's sub-state 0. */
void em040_act1_dispatch(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em040_act1_wait_call(self);
    }
}

/* 0x801BA3D4 (0x67C): the leap: rises to the peak point, falls to the landing point (`kind` picks the short or the
 * long arc), then hands over to action 1 or, when the landing is out of reach, to the action-11 exit. */
void em040_act2_leap(struct _ENEMY_WORK* self, u8 kind) {
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3 se_pos;
    nw4r::math::VEC3 spB0;
    nw4r::math::VEC3 spA4;
    nw4r::math::VEC3 sp98;
    nw4r::math::VEC3 sp8C;
    nw4r::math::VEC3 sp80;
    nw4r::math::VEC3 sp74;
    nw4r::math::VEC3 sp68;
    nw4r::math::VEC3 sp5C;
    nw4r::math::VEC3 sp50;
    nw4r::math::VEC3 sp44;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    struct _ENEMY_WORK* leader;

    VEC3_ctor(&unused);
    VEC3_ctor(&se_pos);
    leader = self->em040_0x328.leader_0x330;
    em_hit_arm_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        subVec3(&spA4, &self->em040_0x328.land_0x334, &self->pos);
        addVec3(&spB0, em040_drift(self), &spA4);
        copyVec3(&self->move_0x1A0, &spB0);
        em_move_vec2_clr(self);
        if (kind == 0) {
            self->timer_0x020 = 24;
            self->vec_0x31C.y = -0.3f;
        } else {
            self->timer_0x020 = 10;
            self->vec_0x31C.y = -0.15f;
        }
        self->offset_0x30C.vec_0x310.x = (self->em040_0x328.peak_0x340.x
            - (self->em040_0x328.land_0x334.x + em040_drift(self)->x)) / (f32)self->timer_0x020;
        self->offset_0x30C.vec_0x310.z = (self->em040_0x328.peak_0x340.z
            - (self->em040_0x328.land_0x334.z + em040_drift(self)->z)) / (f32)self->timer_0x020;
        self->offset_0x30C.vec_0x310.y = (self->em040_0x328.peak_0x340.y
            - (self->em040_0x328.land_0x334.y + em040_drift(self)->y)) / (f32)self->timer_0x020
            - 0.5f * self->vec_0x31C.y * (f32)self->timer_0x020;
        em_hit_window_set(self, 0, 2, 0x202);
        em_busy_set(self);
        em_motion_timer_arm(self);
        if (self->area_no == get_now_areano() && leader != NULL && leader->active != 0 && leader->team == 9) {
            addVec3(&sp98, &self->pos, &self->move_0x1A0);
            copyVec3(&se_pos, &sp98);
            if (self->em040_0x328.se_alt_0x32F == 0) {
                shell_se_req(leader->se_0xB14, &se_pos, 0x22, self->field_0x01A);
            } else {
                shell_se_req(leader->se_0xB14, &se_pos, 0x23, self->field_0x01A);
            }
        }
        break;
    case 1:
        self->field_0x1BC += 0x1555;
        addVec3To(em040_drift(self), &self->offset_0x30C.vec_0x310);
        subVec3(&sp80, &self->em040_0x328.land_0x334, &self->pos);
        addVec3(&sp8C, em040_drift(self), &sp80);
        copyVec3(&self->move_0x1A0, &sp8C);
        addVec3To(&self->offset_0x30C.vec_0x310, &self->vec_0x31C);
        if (--self->timer_0x020 < 0) {
            self->state += 1;
            subVec3(&sp74, &self->em040_0x328.peak_0x340, &self->em040_0x328.land_0x334);
            copyVec3(em040_drift(self), &sp74);
            subVec3(&sp5C, &self->em040_0x328.land_0x334, &self->pos);
            addVec3(&sp68, em040_drift(self), &sp5C);
            copyVec3(&self->move_0x1A0, &sp68);
            em_move_vec2_clr(self);
            if (kind == 0) {
                self->timer_0x020 = 21;
                self->vec_0x31C.y = -2.0f;
            } else {
                self->timer_0x020 = 33;
                self->vec_0x31C.y = -0.9f;
            }
            self->offset_0x30C.vec_0x310.x = (self->em040_0x328.land_0x334.x - self->em040_0x328.peak_0x340.x)
                / (f32)self->timer_0x020;
            self->offset_0x30C.vec_0x310.z = (self->em040_0x328.land_0x334.z - self->em040_0x328.peak_0x340.z)
                / (f32)self->timer_0x020;
            self->offset_0x30C.vec_0x310.y = (self->em040_0x328.land_0x334.y - self->em040_0x328.peak_0x340.y)
                / (f32)self->timer_0x020 - 0.5f * self->vec_0x31C.y * (f32)self->timer_0x020;
            self->field_0x1BC = (u16)-(u16)(s32)(0.5f + (65536.0f * (15.0f * (f32)self->timer_0x020)) / 360.0f);
            if (self->area_no == get_now_areano() && leader != NULL && leader->active != 0 && leader->team == 9) {
                addVec3(&sp50, &self->pos, &self->move_0x1A0);
                copyVec3(&se_pos, &sp50);
                se_req_pos_ps(leader->se_0xB14, 0x28, 2, &se_pos);
            }
        } else if (self->area_no == get_now_areano() && leader != NULL && leader->active != 0
                   && leader->team == 9) {
            addVec3(&sp44, &self->pos, &self->move_0x1A0);
            copyVec3(&se_pos, &sp44);
            if (self->em040_0x328.se_alt_0x32F == 0) {
                shell_se_req(leader->se_0xB14, &se_pos, 0x22, self->field_0x01A);
            } else {
                shell_se_req(leader->se_0xB14, &se_pos, 0x23, self->field_0x01A);
            }
        }
        em_busy_set(self);
        em_motion_timer_arm(self);
        break;
    case 2:
        self->field_0x1BC += 0xAAB;
        addVec3To(em040_drift(self), &self->offset_0x30C.vec_0x310);
        subVec3(&sp2C, &self->em040_0x328.land_0x334, &self->pos);
        addVec3(&sp38, em040_drift(self), &sp2C);
        copyVec3(&self->move_0x1A0, &sp38);
        addVec3To(&self->offset_0x30C.vec_0x310, &self->vec_0x31C);
        if (--self->timer_0x020 < 0) {
            copyVec3(&self->pos, &self->em040_0x328.land_0x334);
            setVector3(em040_drift(self), 0.0f, 0.0f, 0.0f);
            subVec3(&sp14, &self->em040_0x328.land_0x334, &self->pos);
            addVec3(&sp20, em040_drift(self), &sp14);
            copyVec3(&self->move_0x1A0, &sp20);
            em_hit_window_clear_all(self);
            if (self->area_no == get_now_areano() && leader != NULL && leader->active != 0 && leader->team == 9) {
                addVec3(&sp8, &self->pos, &self->move_0x1A0);
                copyVec3(&se_pos, &sp8);
                se_req_pos_ps(leader->se_0xB14, 0x28, 2, &se_pos);
            }
            if (self->em040_0x328.escape_0x329 == 1) {
                em_state_set(self, 11, 56);
            } else {
                em_state_set(self, 1, 0);
            }
        } else {
            em_busy_set(self);
            em_motion_timer_arm(self);
        }
        break;
    }
}

/* 0x801BAA50 (0x2C): action 2's sub-states: the long (0) and the short (1) leap. */
void em040_act2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em040_act2_leap(self, 0);
        break;
    case 1:
        em040_act2_leap(self, 1);
        break;
    }
}

/* 0x801BAA7C (0x4): action 10 finishes at once. */
void em040_act10_finish(struct _ENEMY_WORK* self) {
    em_action_finish(self);
}

/* 0x801BAA80 (0x8C): action 11 plays the sound program for its sub-state. */
void em040_act11_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em040_se_tbl, 0, 0);
        break;
    case 5:
        em_se_tbl_play_alt(self, em040_se_tbl, 1, 5);
        break;
    case 56:
        em_se_tbl_play_alt(self, em040_se_tbl, 0, 56);
        break;
    case 59:
        em_se_tbl_play_alt(self, em040_se_tbl, 0, 59);
        break;
    default:
        em_se_tbl_play_alt(self, em040_se_tbl, 0, 0);
        break;
    }
}

/* 0x801BAB0C (0x98): demo 0: hidden until frame 702, then shown at its first mark. */
void em040_act13_0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(702) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_demo_pos_set(self, 1678.2299f, 30.0f, 286.8487f);
        }
        break;
    }
}

/* 0x801BABA4 (0x6C): demo 1: placed at demo 0's mark, then done. */
void em040_act13_1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_pos_set(self, 1678.2299f, 30.0f, 286.8487f);
        break;
    case 1:
        em_action_finish(self);
        break;
    }
}

/* 0x801BAC10 (0x98): demo 2: hidden until frame 702, then shown at its mark. */
void em040_act13_2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(702) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_demo_pos_set(self, 2485.1372f, 30.0f, -1128.8431f);
        }
        break;
    }
}

/* 0x801BACA8 (0x6C): demo 3: placed at demo 2's mark, then done. */
void em040_act13_3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_pos_set(self, 2485.1372f, 30.0f, -1128.8431f);
        break;
    case 1:
        em_action_finish(self);
        break;
    }
}

/* 0x801BAD14 (0x154): demo 4: shown at frame 702, moved at 1266 and 1318, gone in a burst at 1366. */
void em040_act13_4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(702) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_demo_pos_set(self, 1956.5378f, 30.0f, -2168.288f);
        }
        break;
    case 2:
        if (em_demo_time_ck(1266) == 1) {
            self->state += 1;
            em_demo_pos_set(self, 1612.7797f, 30.0f, -1418.6244f);
        }
        break;
    case 3:
        if (em_demo_time_ck(1318) == 1) {
            self->state += 1;
            em_demo_pos_set(self, -1051.3372f, 30.0f, -1620.3401f);
        }
        break;
    case 4:
        if (em_demo_time_ck(1366) == 1) {
            self->state += 1;
            eft019_set(&self->pos, self->area_no, 2);
            em_demo_reset(self, NULL);
        }
        break;
    }
}

/* 0x801BAE68 (0x6C): demo 5: placed at demo 4's third mark, then done. */
void em040_act13_5(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_pos_set(self, -1051.3372f, 30.0f, -1620.3401f);
        break;
    case 1:
        em_action_finish(self);
        break;
    }
}

/* 0x801BAED4 (0x1DC): demo 6: shown at frame 702, moved at 1064 and 1266, a burst at 1298, shown again at 1318
 * and gone in a burst at 1338. */
void em040_act13_6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(702) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_demo_pos_set(self, 805.4473f, 30.0f, -2315.615f);
        }
        break;
    case 2:
        if (em_demo_time_ck(1064) == 1) {
            self->state += 1;
            em_demo_pos_set(self, 59.8664f, 30.0f, -262.3754f);
        }
        break;
    case 3:
        if (em_demo_time_ck(1266) == 1) {
            self->state += 1;
            em_demo_pos_set(self, -394.2979f, 30.0f, -496.32f);
        }
        break;
    case 4:
        if (em_demo_time_ck(1298) == 1) {
            self->state += 1;
            eft019_set(&self->pos, self->area_no, 2);
            em_demo_reset(self, NULL);
        }
        break;
    case 5:
        if (em_demo_time_ck(1318) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_demo_pos_set(self, 59.8664f, 30.0f, -693.1073f);
        }
        break;
    case 6:
        if (em_demo_time_ck(1338) == 1) {
            self->state += 1;
            eft019_set(&self->pos, self->area_no, 2);
            em_demo_reset(self, NULL);
        }
        break;
    }
}

/* 0x801BB0B0 (0x6C): demo 7: placed at demo 6's last mark, then done. */
void em040_act13_7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_pos_set(self, 59.8664f, 30.0f, -693.1073f);
        break;
    case 1:
        em_action_finish(self);
        break;
    }
}

/* 0x801BB11C (0x6C): demo 8: placed and turned at its mark, and held there. */
void em040_act13_8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_pos_set(self, 1926.4f, 97.4f, -1156.6f);
        em_demo_rot_set(self, 0.0f, 180.0f, 0.0f);
        break;
    }
}

/* 0x801BB188 (0x80): demo 9: placed and turned at demo 8's mark, then done. */
void em040_act13_9(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_demo_pos_set(self, 1926.4f, 97.4f, -1156.6f);
        em_demo_rot_set(self, 0.0f, 180.0f, 0.0f);
        break;
    case 1:
        em_action_finish(self);
        break;
    }
}

/* 0x801BB208 (0x50): action 13's demo steps. */
void em040_act13_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em040_act13_0(self);
        break;
    case 1:
        em040_act13_1(self);
        break;
    case 2:
        em040_act13_2(self);
        break;
    case 3:
        em040_act13_3(self);
        break;
    case 4:
        em040_act13_4(self);
        break;
    case 5:
        em040_act13_5(self);
        break;
    case 6:
        em040_act13_6(self);
        break;
    case 7:
        em040_act13_7(self);
        break;
    case 8:
        em040_act13_8(self);
        break;
    case 9:
        em040_act13_9(self);
        break;
    }
}

/* 0x801BB258 (0x54): runs the current action's dispatcher. */
void em040_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em040_act0_dispatch(self);
        break;
    case 1:
        em040_act1_dispatch(self);
        break;
    case 2:
        em040_act2_dispatch(self);
        break;
    case 10:
        em040_act10_finish(self);
        break;
    case 11:
        em040_act11_se_play(self);
        break;
    case 13:
        em040_act13_dispatch(self);
        break;
    }
}

/* 0x801BB2AC (0x21C): once the action-11 exit starts (sub-states 0, 5 and 56), plays the variant's exit: a dust
 * burst and a sound, a ground burst, or the leaping variant's burst and the leader's call. */
void em040_motion_events(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 tmp;
    nw4r::math::MTX34 mtx;
    struct _ENEMY_WORK* leader;

    VEC3_ctor(&ofs);
    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    em_get_mot_no(self);
    if ((em_act_ck_c1(self, 11, 0) == 1 || em_act_ck_c1(self, 11, 5) == 1 || em_act_ck_c1(self, 11, 56) == 1)
        && self->em040_0x328.escape_0x329 == 0) {
        switch (self->field_0x00A) {
        case 0:
        case 2:
            copyVec3(&ofs, setVec3(&tmp, 0.0f, 50.0f, 0.0f));
            eft_em_spawn_tinted_ex(self, 0, 0, &ofs, 1.5f, -1, 0);
            get_joint_wmat_em(self, 0, &mtx);
            mtx34_trans_get(&mtx, &pos);
            mulVecMat(&ofs, &mtx);
            addVec3To(&pos, &ofs);
            se_req_pos_ps(self->se_0xB14, 1, 2, &pos);
            break;
        case 4:
            eft019_set_subtype(1, 1, &self->pos, self->area_no, (_CP_VECTOR*)&self->field_0x1BC, 0, 1.0f);
            shell_set_func_ptr->method_0x08(&self->pos, 1, self->area_no, 0);
            se_req_pos_ps(self->se_0xB14, 2, 2, &self->pos);
            break;
        case 8:
            eft019_set(&self->pos, self->area_no, 2);
            leader = em040_leader_find(self);
            if (leader == NULL) {
                shell_set_func_ptr->method_0x08(&self->pos, 0x2E, self->area_no, 0);
            } else {
                ((ShellPosJobFn)shell_set_func_ptr->method_0x14)(leader, &self->pos, 0x2E, self->area_no, 0xFFFF);
            }
            break;
        }
        self->em040_0x328.escape_0x329 = 1;
    }
}

/* 0x801BB4C8 (0x14C): once, scales the material colour by the ratio of the stage cell colours of the work's cell
 * and its dry/underwater counterpart. */
void em040_tint_update(struct _ENEMY_WORK* self) {
    GXColor from;
    GXColor to;
    GXColor mat;
    u32 from_word;
    u32 to_word;
    u8 cell_to;
    u8 cell_from;

    if (self->em040_0x328.tint_done_0x328 == 0) {
        self->em040_0x328.tint_done_0x328 = 1;
        cell_from = 0;
        cell_to = self->cell_0x22F;
        if (em_water_check_c1(self) == 1 && self->pos.y < self->field_0x210) {
            cell_from = 3;
            cell_to += 3;
        }
        if (cell_from != cell_to) {
            from_word = stage_cell_get(cell_from, 1);
            color_rgba_copy((u8*)&from, (u8*)&from_word);
            to_word = stage_cell_get(cell_to, 1);
            color_rgba_copy((u8*)&to, (u8*)&to_word);
            if (((MHchar*)&self->char_0x024)->getMatColor(0, GX_COLOR0A0, &mat) != 0) {
                mat.r = (to.r * mat.r) / from.r;
                mat.g = (to.g * mat.g) / from.g;
                mat.b = (to.b * mat.b) / from.b;
                ((MHchar*)&self->char_0x024)->setMatColor(0, GX_COLOR0A0, mat, false);
            }
        }
    }
}

/* 0x801BB614 (0x8): the state query; em040 reports nothing. */
u32 em040_state_ck(void) {
    return 0;
}

/* 0x801BB61C (0x5C): the area start: action 0, sub-state 0, with the rotation reset. */
void em040_area_start_set(struct _ENEMY_WORK* self, u8* action, u8* sub) {
    *action = 0;
    *sub = 0;
    setVector3(&self->pos, 0.0f, 0.0f, 0.0f);
    self->field_0x1BC = 0;
    self->field_0x1C0 = 0;
    self->field_0x1C4 = 0;
}

/* 0x801BB678 (0x4): the gauge slot; em040 has none. */
void em040_gauge_update(void) {}

/* 0x801BB67C (0xDC): the leaping variant's area record keeps the spawn position (or a point above it), the
 * rotation and the leap/angle flags. */
void em040_area_keep_set(struct _ENEMY_MINI_WORK* mini, nw4r::math::VEC3* pos, struct EmAreaKeepSrc* src, u8 flags) {
    EmRunPos* keep = (EmRunPos*)mini->data_0x34;
    nw4r::math::VEC3 tmp;

    if (mini->team_0x03 == 8) {
        if (flags & 0x10) {
            copyVec3(&keep->pos, setVec3(&tmp, 0.0f, 100.0f, 0.0f));
            keep->angle = 0;
        } else {
            copyVec3(&keep->pos, pos);
            keep->angle = src->rot_0x04;
        }
        keep->flags = 0;
        if (src->leap_0x00 != 0) {
            src->leap_0x00 = 0;
            keep->flags |= 1;
        }
        if (src->angle_0x08 != 0) {
            src->angle_0x08 = 0;
            keep->flags |= 2;
        }
        keep->valid_0x0F = 1;
    }
}

}
