/* enemy/em_act_step.cpp - the enemy work record's action-step band: the step function of each of the record's first
 *   four action ids, the motion each of their phases arms, and the record's own class (constructor, vtable, destructor
 *   halves) with the effect-offset and seat-aim helpers.
 * RANGE. .text 0x8032C920-0x80330194 (70 functions); .ctors 0x8056F3A0-0x8056F3A4 (`fn_80330128`),
 *   .data 0x805DFC30-0x805E0510 (from `em026_prog_tbl`; the jump tables, the per-function tables and the class vtable
 *   `lbl_805E04E0` last), .bss 0x806BE328-0x806BE340, .sdata 0x80792D18-0x80792D28, .sdata2 0x8079B108-0x8079B210,
 *   extab, extabindex.  The band's last four functions are `enemy/em_act_step_tail.cpp`.
 * SEAM. The band's end 0x8033041C (against `hud/pl_frame_sync.cpp`) is settled: extabindex entry 57 (0x800357A8)
 *   is `fn_8033041C`; the `.sdata2` run cuts at 0x8079B20C | 0x8079B210 with no label shared and MWCC's u32->f32
 *   magic emitted once per side (0x8079B140 and 0x8079B228); the `.data` run cuts at 0x805E0510.  The cut at
 *   0x80330194 is the reconciled candidate's and unproven.
 * NAMES. Every symbol the unit defines is a GUESS from its body on the module's `em_<noun>_<verb>` scheme:
 *   `em_act_step_<n>` is action id n's step (the dispatcher `fn_8032FA88` switches on `+0x1E5`, cases 0..3);
 *   `em_act_arm_<motion>` one phase that arms a motion and ends on its frame check, `_hit<a>_<b>` the pair handed to
 *   `em_state_set`, `_f<frames>` the frame count it sets, `_mot_mode` a motion chosen by the caller's mode.
 * RESIDUALS. 36 rows unwritten: 0x8032CC90-0x8032CEA8, 0x8032DD04-0x8032DF44, 0x8032E23C-0x80330194 (the static
 *   initializer `fn_80330128` among them).
 *  - `em_act_die_step`: `_EM_CHARA_WORK` declares `field_0x834` after `+0xA39`, so ours reads +0x1234 where retail
 *    reads +0x834;
 *  - `em_act_arm_mot1_hit1_1`, `em_act_arm_mot201_hit7_2`, `em_act_arm_mot201_hit7_6`: retail advances the move-work
 *    pointer by 0xB20 at the loop tail, ours indexes from the base (the `work[i]` stride spelling scores lower); the
 *    two `_hit7_` rows load `lbl_8079B178` (0.8) where retail loads `lbl_8079B150` (800.0);
 *  - `em_eff_offset_set`: retail copies the 12-byte offset through r3, ours through r0, and loads `lbl_8079B110`
 *    where retail loads `lbl_8079B10C`;
 *  - `em_act_arm_mot5`, `em_act_arm_mot7`: retail loads f1 before `li r4,0` for the trailing `em_approach_start`,
 *    ours after;
 *  - `em_act_step_3`: retail's jump-table bound is `cmplwi r0,21`, ours 19 (an empty `case 21` measures the same);
 *  - `em_act_face_away`, `em_act_arm_mot203_204`: register allocation only.
 *   flipcheck: `.bss`/`.ctors`/`.sdata` claimed, not emitted; `.data`/`.sdata2`/`.text`/extab/extabindex short of
 *   the claim.
 * SHAPES. `_EM_CHARA_WORK` is the union of the record's two shared views (`_ENEMY_WORK`, `_PLW`), so callees that
 *   take `_PLW*` are called through a cast of the same pointer; `#pragma peephole off` and `#pragma fp_contract off`
 *   over the bodies.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em_act_step.h"
#include "enemy/em_pop.h" /* quest_move_sub_state_4_get (the owner's header, rule 2) */
#include "unsplit/enemy.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_8028F66C.h"
#include "Pl/Pl_master_ck.h"

#ifdef __cplusplus

extern "C" {
#endif

/* The out-of-range callees whose owner's header does not declare them; each spelling is the callee's own body. */
void fn_80051B7C(void* out, const void* in, f32 scale, s32 a);
void em_action_finish_fall(struct _ENEMY_WORK* self);
void fn_8032DD04(struct _EM_CHARA_WORK* self);
f32 calcVecDistXZ(const void* a, const void* b);
void fn_8012E694(struct _ENEMY_WORK* self);
void fn_80136DF4(struct _ENEMY_WORK* self);
void eft_rot_vec_copy(_CP_VECTOR* dst, const _CP_VECTOR* src);
u32 quest_sub_state_end_ck(u32 a);
void fn_80130350(struct _ENEMY_WORK* self, void* vec);
void fn_8027D3F0(struct _PLW* self, u8 a);
void fn_8012E664(struct _ENEMY_WORK* self);
void* em_res_user_data_ctor(void* self);
u32 fn_801421E4(u16 id, EmGroundRec* rec);
void em_ground_rec_clear(EmGroundRec* rec);

#ifdef __cplusplus
}
#endif

/* The enemy work API this band calls, at C++ scope so the front-end reproduces the map's mangled name
 * (rule 9); each spelling is the callee's own body. */
u32 get_move_work_max(u8 kind);
void* get_move_work_adrs(u8 kind);
s32 em_die_ck(struct _ENEMY_WORK* self);

/* The 16-byte per-slot handle block at +0x328: the effect offset `rotVecY` rotates in place and the
 * state's own stage word beside it.  Both are reached through one pointer in the target, so they are
 * one record here too (rule 3: size from the target's `addi rN, self, 0x328` + `stw .., 12(rN)`).
 * size: 0x10 */
struct EmHandleBlock {
    /* +0x00 */ nw4r::math::VEC3 vec;
    /* +0x0C */ u32 field_0x0C;
};

/* The record every function in this range takes in r3 (size from the largest access, +0xB14, and from
 * `enemy/ENEMY_WORK.h`'s 0xB18; every field named from its use, ascending except `field_0x834`, see RESIDUALS).  Offset +0x000 is the class vtable the constructor
 * `em_work_ctor` stores, so the class's object *is* this record.
 * size: 0xB18 */
struct _EM_CHARA_WORK {
    /* +0x000 */ union {                /* the object's first four bytes carry both readings: the class
                                         * vtable `em_work_ctor` stores here, and the header's own
                                         * `active`/`group`/`team` bytes are the same four bytes. */
        void** vtable;
        struct {                       /* size: 0x04 */
            /* +0x000 */ u8 field_0x000;
            /* +0x001 */ u8 field_0x001;
            /* +0x002 */ u8 group;     /* the enemy group/entry index `fn_8027D3F0` is handed */
            /* +0x003 */ u8 team;
        };
    };
    /* +0x004 */ u8 field_0x004;        /* below 2 the record still counts as alive */
    /* +0x005 */ u8 state;              /* the per-motion step the action machines switch on */
    /* +0x006 */ u8 unused_0x006[0x009 - 0x006];
    /* +0x009 */ u8 field_0x009;        /* 2/3 select the joint the effect is placed on */
    /* +0x00A */ u8 unused_0x00A[0x01A - 0x00A];
    /* +0x01A */ u16 field_0x01A;       /* the ground-record id `fn_801421E4` looks up */
    /* +0x01C */ u8 unused_0x01C[0x020 - 0x01C];
    /* +0x020 */ s32 field_0x20;       /* the step's own countdown; its low 5 bits gate the scan */
    /* +0x024 */ u8 char_0x024[0x40];   /* the embedded `MHchar` base the `em_` frame checks hand on */
    /* +0x064 */ u8 unused_0x064[0x188 - 0x064];
    /* +0x188 */ nw4r::math::VEC3 pos;  /* one position, the effect anchor */
    /* +0x194 */ u8 unused_0x194[0x1BC - 0x194];
    /* +0x1BC */ union {
        _CP_VECTOR rot;                /* the rotation triple `eft_rot_vec_copy` copies in whole */
        struct {                       /* size: 0x0C */
            /* +0x1BC */ u32 rot_x;
            /* +0x1C0 */ u32 rot_y;     /* the angle `rotVecY` turns the effect by */
            /* +0x1C4 */ u32 rot_z;
        };
    };
    /* +0x1C8 */ u32 unused_0x1C8;
    /* +0x1CC */ f32 field_0x1CC;       /* the per-step speed the state steps clamp */
    /* +0x1D0 */ u8 unused_0x1D0[0x1E6 - 0x1D0];
    /* +0x1E6 */ u8 state_sub;          /* the sub-state the dispatchers switch on */
    /* +0x1E7 */ u8 unused_0x1E7[0x1EC - 0x1E7];
    /* +0x1EC */ u16 bits_0x1EC;        /* the low two bits scale the action's speed factor */
    /* +0x1EE */ u8 unused_0x1EE[0x328 - 0x1EE];
    /* +0x328 */ EmHandleBlock handle_0x328;
    /* +0x338 */ u8 field_0x338;
    /* +0x339 */ u8 unused_0x339[0x354 - 0x339];
    /* +0x354 */ f32 field_0x354;       /* the model scale the act entry hands the model layer */
    /* +0x358 */ u8 unused_0x358[0x454 - 0x358];
    /* +0x454 */ f32 values_0x454[4];   /* the per-attacker values the step loop scans */
    /* +0x464 */ u8 unused_0x464[0x565 - 0x464];
    /* +0x565 */ u8 field_0x565;        /* the primary-act latch */
    /* +0x566 */ u8 field_0x566;        /* armed once the entry block has run */
    /* +0x567 */ u8 unused_0x567[0x5C6 - 0x567];
    /* +0x5C6 */ u8 field_0x5C6;
    /* +0x5C7 */ u8 unused_0x5C7[0xA16 - 0x5C7];
    /* +0xA16 */ u8 field_0xA16;        /* 0xFF means "no seat id yet" */
    /* +0xA17 */ u8 unused_0xA17[0xA34 - 0xA17];
    /* +0xA34 */ _PLW* plw_0xA34;       /* the seat's player work the aiming step reads */
    /* +0xA38 */ u8 field_0xA38;        /* nonzero: the seat is not aimable */
    /* +0xA39 */ u8 unused_0xA39[0x7FB];
    /* +0x834 */ u8 field_0x834;        /* the "already seated" byte the step tests against 1 */
    /* +0x835 */ u8 unused_0x835[0xB14 - 0x835];
    /* +0xB14 */ struct _se_w* se_0xB14;
};

#pragma peephole off
#pragma fp_contract off

/* Rotates the effect's local offset vector by the work record's own y angle, then hands it to the
 * effect placer. */
extern "C" void em_eff_offset_set(_EM_CHARA_WORK* self)
{
    nw4r::math::VEC3 offset;
    _CP_VECTOR out;

    VEC3_ctor(&offset);
    offset.x = lbl_8079B108;
    offset.y = lbl_8079B108;
    offset.z = lbl_8079B110;
    rotVecY(&offset, self->rot_y);
    out = *(_CP_VECTOR*)&offset;
    fn_80130350((_ENEMY_WORK*)self, &out);
}

/* Clears the effect offset block and the state word next to it. */
extern "C" void em_eff_offset_clr(_EM_CHARA_WORK* self)
{
    EmHandleBlock* block = &self->handle_0x328;

    setVector3(&block->vec, lbl_8079B108, lbl_8079B108, lbl_8079B108);
    block->field_0x0C = 0;
}

/* Faces the work record away from the player: with the player in play and fewer than three of its act bits set,
 * turns the record's y angle to the reverse bearing and stores the sub-step. */
extern "C" u32 em_act_face_away(_EM_CHARA_WORK* self, _PLW* pl)
{
    nw4r::math::VEC3* offset = &self->handle_0x328.vec;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 seed;
    nw4r::math::VEC3 target;
    u32 ang_a;
    u32 ang_b;
    u32 bits;
    s32 i;
    s32 count;

    if (pl == NULL) {
        return 0;
    }
    if (pl->slot_active == 0) {
        return 0;
    }
    if (Pl_master_ck(pl) != 1) {
        return 0;
    }
    if (quest_sub_state_end_ck(1) != 0) {
        return 0;
    }
    if (quest_move_sub_state_4_get() != 0) {
        return 0;
    }

    bits = pl->field_0x3B0;
    count = 0;
    for (i = 0; i < 32; i++) {
        if ((bits & (1u << i)) != 0) {
            count++;
        }
    }
    if (count >= 3) {
        return 0;
    }

    VEC3_ctor(&dir);
    setVec3(&seed, lbl_8079B108, lbl_8079B108, lbl_8079B110);
    copyVec3(offset, &seed);

    subVec3(&target, &self->pos, &pl->vec_0x03C);
    copyVec3(&dir, &target);
    calcVecAngXY(&dir, &ang_a, &ang_b);
    rotVecY(offset, (u16)(ang_a - pl->field_0x058));

    subVec3(&target, &pl->vec_0x03C, &self->pos);
    copyVec3(&dir, &target);
    calcVecAngXY(&dir, &ang_a, &ang_b);
    self->rot_y = ang_a - pl->field_0x058;
    self->handle_0x328.field_0x0C = 3;
    fn_8027D3F0(pl, self->group);
    return 1;
}

/* Places the effect at the work record's position when the ground record for its +0x1A id is found,
 * and copies that record's rotation into the record. */
extern "C" void em_eff_ground_set(_EM_CHARA_WORK* self)
{
    EmGroundRec rec;

    em_ground_rec_clear(&rec);
    if (fn_801421E4(self->field_0x01A, &rec) != 0) {
        fn_80051B7C(&self->pos, &rec.pos_0x08, lbl_8079B114, 0);
        eft_rot_vec_copy(&self->rot, (_CP_VECTOR*)&rec.field_0x14);
    }
}

/* The class constructor: runs the base constructor, then installs this unit's vtable. */
extern "C" _EM_CHARA_WORK* em_work_ctor(_EM_CHARA_WORK* self)
{
    em_res_user_data_ctor((_ENEMY_WORK*)self);
    self->vtable = (void**)lbl_805E04E0;
    return self;
}

/* The class destructor's deleting half, called from the vtable's slot 2 by `fn_803300CC`. */
extern "C" void em_work_dtor_del(void)
{
}

/* The range's other empty stub - a virtual override whose body the target does not emit either. */
extern "C" void em_work_noop(void)
{
}

/* Steps the death action: while `em_die_ck` refuses it and the +0x834 byte is set, advances the
 * +0x1CC speed toward its ceiling. */
extern "C" void em_act_die_step(_EM_CHARA_WORK* self)
{
    if (em_die_ck((_ENEMY_WORK*)self) != 0) {
        return;
    }
    if (self->field_0x834 == 1) {
        fn_8012E664((_ENEMY_WORK*)self);
    }
    if (self->field_0x338 == 1) {
        f32 speed = self->field_0x1CC;

        if (speed < lbl_8079B11C) {
            f32 next = speed + lbl_8079B120;

            self->field_0x1CC = next;
            if (next > lbl_8079B11C) {
                self->field_0x1CC = lbl_8079B11C;
            }
        }
    }
}

/* Arms the +0x06 action on the first step, then ends the step when the action's frame check passes. */
extern "C" void em_act_arm_mot1(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set_ck((_ENEMY_WORK*)self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xC9 action (with the two effect resets that precede it), then ends the step. */
extern "C" void em_act_arm_mot201(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set_ck((_ENEMY_WORK*)self, 201, 6, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the two step functions this band holds. */
extern "C" void em_act_step_0(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot1(self);
        break;
    case 1:
        em_act_arm_mot1(self);
        break;
    case 2:
        em_act_arm_mot1(self);
        break;
    case 3:
        em_act_arm_mot201(self);
        break;
    }
}

/* Arms the +0x02 action on the first step, then ends the step when the action's frame check passes. */
extern "C" void em_act_arm_mot2(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 2, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the action matching the caller's mode on the first step, then ends the step when the action's
 * frame check passes; mode 1 first re-arms the record's own sub-state. */
extern "C" void em_act_arm_mot_mode(_EM_CHARA_WORK* self, u32 action)
{
    if ((action & 0xFF) == 1) {
        fn_80131E00((_ENEMY_WORK*)self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        switch ((u8)action) {
        case 0:
            action = 3;
            break;
        case 1:
            action = 4;
            break;
        case 2:
            action = 6;
            break;
        case 3:
            action = 9;
            break;
        }
        em_mot_set((_ENEMY_WORK*)self, action, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Advances the record's two motion counters, then arms the +0x0B action or ends the step. */
extern "C" void em_act_arm_mot11(_EM_CHARA_WORK* self)
{
    fn_80131DB4((_ENEMY_WORK*)self);
    fn_80131DF4((_ENEMY_WORK*)self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 11, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            fn_8012E694((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Advances the record's motion counters and scans the per-attacker values for one still inside the
 * band, handing the first such attacker to the damage handler. */
extern "C" void em_act_arm_mot1_hit1_1(_EM_CHARA_WORK* self)
{
    fn_80131DB4((_ENEMY_WORK*)self);
    fn_80131DF4((_ENEMY_WORK*)self);
    fn_80136D14((_ENEMY_WORK*)self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 1, 0, 0);
        fn_801303EC((_ENEMY_WORK*)self, lbl_8079B124);
        self->field_0x20 = 0;
        break;
    case 1: {
        u32 max;
        _PLW* work;
        u8 i;

        fn_801303EC((_ENEMY_WORK*)self, lbl_8079B124);
        if ((self->field_0x20 & 0x1F) != 0) {
            break;
        }
        max = get_move_work_max(2);
        work = (_PLW*)get_move_work_adrs(2);
        for (i = 0; i < (u16)max; i++) {
            f32 value;

            if (work == NULL) {
                continue;
            }
            if (work->slot_active == 0) {
                continue;
            }
            if (work->field_0x3B0 == 0) {
                continue;
            }
            value = self->values_0x454[i];
            if (value > lbl_8079B128) {
                continue;
            }
            if (value < lbl_8079B108) {
                continue;
            }
            em_state_set((_ENEMY_WORK*)self, 1, 1);
            return;
        }
        self->field_0x20 += 1;
        break;
    }
    }
}

/* Arms the +0x14 action with a 20-frame frame check, then ends the step when that check passes. */
extern "C" void em_act_arm_mot1_f20(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set_blend((_ENEMY_WORK*)self, 1, 20, 0, 1);
        break;
    case 1:
        if (em_frame_check((_ENEMY_WORK*)self, 0, lbl_8079B12C, lbl_8079B108) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Dispatches the record's sub-state: each arm is one of this band's step functions, and the table
 * MWCC emits for the eight arms is this unit's own `.data` (0x805DFC9C, 8 entries). */
extern "C" void em_act_step_1(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot2(self);
        break;
    case 1:
        em_act_arm_mot_mode(self, 0);
        break;
    case 2:
        em_act_arm_mot_mode(self, 1);
        break;
    case 3:
        em_act_arm_mot_mode(self, 2);
        break;
    case 4:
        em_act_arm_mot_mode(self, 3);
        break;
    case 5:
        em_act_arm_mot11(self);
        break;
    case 6:
        em_act_arm_mot1_hit1_1(self);
        break;
    case 7:
        em_act_arm_mot1_f20(self);
        break;
    }
}

/* Arms the +0x05 action with a speed factor from the record's own two low bits and a 150-frame
 * countdown, then ends the step when the frame check passes or the countdown runs out. */
extern "C" void em_act_arm_mot5(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 5, 4, 0);
        em_mot_speed_set((_ENEMY_WORK*)self,
                    lbl_8079B130 + lbl_8079B134 * (f32)(self->bits_0x1EC & 3));
        em_approach_start((_ENEMY_WORK*)self, lbl_8079B138, 0);
        self->field_0x20 = 150;
        break;
    case 1:
        if (em_approach_step((_ENEMY_WORK*)self, 0, 128) == 1 || --self->field_0x20 <= 0) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* The same step as `em_act_arm_mot5` for the +0x07 action, with its own speed factor and 120 frames. */
extern "C" void em_act_arm_mot7(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 7, 4, 0);
        em_mot_speed_set((_ENEMY_WORK*)self,
                    lbl_8079B148 + lbl_8079B14C * (f32)(self->bits_0x1EC & 3));
        em_approach_start((_ENEMY_WORK*)self, lbl_8079B138, 0);
        self->field_0x20 = 120;
        break;
    case 1:
        if (em_approach_step((_ENEMY_WORK*)self, 0, 128) == 1 || --self->field_0x20 <= 0) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x0C action and waits for its 512-frame check. */
extern "C" void em_act_arm_mot12(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 12, 4, 0);
        break;
    case 1:
        if (em_turn_to_target((_ENEMY_WORK*)self, 512) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x0D action and waits for its 1024-frame check. */
extern "C" void em_act_arm_mot13(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 13, 4, 0);
        break;
    case 1:
        if (em_turn_to_target((_ENEMY_WORK*)self, 1024) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x0A action: its 2048-frame check runs first, then the action's own frame check. */
extern "C" void em_act_arm_mot10(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 10, 4, 0);
        break;
    case 1:
        em_turn_to_target((_ENEMY_WORK*)self, 2048);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0x72 action with a 120-frame countdown, then ends the step when the countdown runs out. */
extern "C" void em_act_arm_mot114(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set((_ENEMY_WORK*)self, 0);
        em_mot_set((_ENEMY_WORK*)self, 114, 4, 0);
        self->field_0x20 = 120;
        break;
    case 1:
        if (--self->field_0x20 <= 0) {
            em_action_finish((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the six step functions above. */
extern "C" void em_act_step_2(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot5(self);
        break;
    case 1:
        em_act_arm_mot7(self);
        break;
    case 2:
        em_act_arm_mot12(self);
        break;
    case 3:
        em_act_arm_mot13(self);
        break;
    case 4:
        em_act_arm_mot10(self);
        break;
    case 5:
        em_act_arm_mot114(self);
        break;
    }
}

/* The +0xCA action's step: the two resets and the motion setter first, then the same frame check. */
extern "C" void em_act_arm_mot202(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 202, 6, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Steps the +0xC9 action and, every sixteenth step, hands the first attacker close enough to the
 * record's position to the damage handler. */
extern "C" void em_act_arm_mot201_hit7_2(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 201, 0, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        self->field_0x20 = 0;
        break;
    case 1: {
        u32 max;
        _PLW* work;
        u8 i;

        fn_80136DF4((_ENEMY_WORK*)self);
        if ((self->field_0x20 & 0xF) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++) {
                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (calcVecDistXZ((VEC3*)&work->vec_0x03C, &self->pos) > lbl_8079B178) {
                    continue;
                }
                em_state_set((_ENEMY_WORK*)self, 7, 2);
                return;
            }
        }
        self->field_0x20 += 1;
        break;
    }
    }
}

/* Arms the +0xCB/+0xCC action - the mode picks between them - then ends the step at the frame check. */
extern "C" void em_act_arm_mot203_204(_EM_CHARA_WORK* self, u32 action)
{
    if ((u8)action == 1) {
        fn_80131E00((_ENEMY_WORK*)self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, (u16)(203 + ((action & 0xFF) == 1)), 0, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xCD action with a 120-frame countdown, then ends the step at the frame check. */
extern "C" void em_act_arm_mot205(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 205, 4, 0);
        em_approach_start((_ENEMY_WORK*)self, lbl_8079B124, 0);
        self->field_0x20 = 120;
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_approach_step((_ENEMY_WORK*)self, 0, 64) == 1 || --self->field_0x20 <= 0) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xCF action and waits for its 1024-frame check. */
extern "C" void em_act_arm_mot207(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 207, 4, 0);
        fn_80136DF4((_ENEMY_WORK*)self);
        break;
    case 1:
        fn_80136DF4((_ENEMY_WORK*)self);
        if (em_turn_to_target((_ENEMY_WORK*)self, 1024) == 1) {
            em_action_finish_fall((_ENEMY_WORK*)self);
        }
        break;
    }
}

/* Arms the +0xD2 action, and on the frame check hands the target to the damage handler; the effect
 * placement runs at the end of every step. */
extern "C" void em_act_arm_mot210_hit3_10(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 210, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_state_set((_ENEMY_WORK*)self, 3, 10);
        }
        break;
    }
    em_eff_offset_set(self);
}

/* Steps the +0xC9 action and, every sixteenth step, hands the first attacker close enough to the
 * record's position to the damage handler. */
extern "C" void em_act_arm_mot201_hit7_6(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 201, 4, 0);
        self->field_0x20 = 0;
        break;
    case 1: {
        u32 max;
        _PLW* work;
        u8 i;

        if ((self->field_0x20 & 0xF) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++) {
                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (calcVecDistXZ((VEC3*)&work->vec_0x03C, &self->pos) > lbl_8079B178) {
                    continue;
                }
                em_state_set((_ENEMY_WORK*)self, 7, 6);
                return;
            }
        }
        self->field_0x20 += 1;
        break;
    }
    }
}

/* Arms the +0xCB action, then hands the target to the damage handler at the frame check. */
extern "C" void em_act_arm_mot203_hit3_20(_EM_CHARA_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get((_ENEMY_WORK*)self);
        em_fall_start((_ENEMY_WORK*)self);
        em_mot_set((_ENEMY_WORK*)self, 203, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck((_ENEMY_WORK*)self) == 1) {
            em_state_set((_ENEMY_WORK*)self, 3, 20);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the band's step functions; unlisted states do nothing. */
extern "C" void em_act_step_3(_EM_CHARA_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot202(self);
        break;
    case 1:
        em_act_arm_mot201_hit7_2(self);
        break;
    case 2:
        em_act_arm_mot203_204(self, 0);
        break;
    case 3:
        em_act_arm_mot203_204(self, 1);
        break;
    case 4:
        em_act_arm_mot205(self);
        break;
    case 5:
        em_act_arm_mot207(self);
        break;
    case 10:
        fn_8032DD04(self);
        break;
    case 11:
        em_act_arm_mot210_hit3_10(self);
        break;
    case 18:
        em_act_arm_mot201_hit7_6(self);
        break;
    case 19:
        em_act_arm_mot203_hit3_20(self);
        break;
    }
}

/* 1 while the record's seat is armed and its player is close enough for the aim step, -1 while the
 * seat is armed but the aim is refused, 0 when there is no seat to aim. */
extern "C" s32 em_seat_aim_ck(_EM_CHARA_WORK* self)
{
    if (self->field_0xA16 != 0xFF && self->field_0xA38 == 0) {
        if (em_act_face_away(self, self->plw_0xA34) == 1) {
            return 1;
        }
        return -1;
    }
    return 0;
}

