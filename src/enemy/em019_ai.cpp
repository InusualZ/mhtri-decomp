/*
 * enemy/em019_ai.cpp - enemy 019's AI: the `_ENEMY_WORK` action, motion, state, approach and battle steps and
 *   their dispatchers, the `em_act_prog_*` program steps with `em_act_run`, the parts-damage, effect and colour
 *   helpers, the motion step `em_act_mot_step`, and the static initializer `fn_8038309C`.
 * RANGE. .text 0x80378F9C-0x80383148 (92 functions); extab 0x80017C44-0x80017E8C, extabindex
 *   0x800378CC-0x80037C38, .ctors 0x8056F3A8-0x8056F3AC, .rodata 0x80570AE0-0x80570B20, .data 0x805EE518-0x805EF8C0
 *   (`em019_prog_tbl` first), .bss 0x806C23E8-0x806C2418, .sdata 0x807933B8-0x807933C0, .sbss
 *   0x80794BF8-0x80794C00, .sdata2 0x8079BC88-0x8079BF60.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `em_act_prog_1` to `em_part_damage_meter`, on elsewhere.
 * NAMES. The file name follows the runtime dump's `em019_prog_tbl`, which opens the TU's `.data` and lists this
 *   range's entry points; the `_ai` suffix is a GUESS.  `em019_action_run_flag_set`, `em019_motion_dispatch`,
 *   `em019_state_dispatch`, `em019_approach_dispatch`, `em019_battle_dispatch`, `em019_action_start`,
 *   `em019_action_start_if_idle`, `em_act_prog_1`..`em_act_prog_8`, `em_act_prog_dispatch`, `em_act_run`,
 *   `em_parts_damage_ck`, `em_act_effect_ck`, `em_act_mot_step`, `em_part_reset`, `em_part_colour_lerp` and
 *   `em_part_damage_meter` are GUESSes from their bodies (the dump answers `zz_` or a linker-folded duplicate's
 *   name).  The map has only `fn_` stems for the other rows.
 * RESIDUALS. 64 rows unwritten: 0x80378F9C-0x80379084, 0x80379090-0x803797F4, 0x8037983C-0x80379DE4,
 *   0x80379E2C-0x8037A7F0, 0x8037A848-0x8037AF08, 0x8037AF88-0x8037E0D0, 0x8037E0E8-0x8037EA64,
 *   0x8037F940-0x8038209C (`em_act_mot_step`: a 216-way switch on `em_get_mot_no` over the jump table at
 *   0x805EF52C), 0x80382310-0x80382BB0, 0x80382C80-0x80382DB8, 0x80382DD8-0x80382F94, 0x80382FC0-0x80383148.
 *   6 partial rows:
 *  - `em_act_prog_6`: retail builds the angle constants with `li r30,-0x6000`/`-0x2000` where ours uses
 *    `lis r3,1; subi`, ours narrows with `clrlwi ...,16` at each use, and its state compare chain is laid out
 *    differently;
 *  - `em_act_prog_1`: ours reloads `lbl_8079BC88` three times where retail keeps it in a register;
 *  - `em_part_damage_meter`: retail keeps the clamp constants in f2, ours reloads them into f0;
 *  - `em_act_run`: retail bounds the table with `cmplwi r0,13`; ours bounds at 9 even with cases 10..13 written
 *    on the default's body;
 *  - `fn_80382C00`: retail tests the byte with the unsigned borrow sequence (`li r3,1; subi; orc; srwi; subf`),
 *    ours with `neg; or`;
 *  - `fn_80382F94`: ours adds a `clrlwi r0,r0,24` retail does not have.
 *   flipcheck: `.bss`/`.ctors`/`.rodata`/`.sbss`/`.sdata` claimed, not emitted; `.data` 0x9C against 0x13A8,
 *   `.sdata2` 0x10 against 0x2D8; `.text` (0x140C of 0xA1AC), extab (0x80 of 0x248) and extabindex (0xC0 of
 *   0x36C) short of the claim and differing.
 * SHAPES. The `ShellSetFuncs` +0x2C call goes through a 6-argument spelling: the shared 7-argument slot type
 *   hoists the table load in `em_act_prog_5`.
 */

#include "types.h"
#include "enemy/ENEMY_WORK.h"

#ifdef __cplusplus

extern "C" {
#endif

/* This unit's unwritten functions the written ones call: declared, never defined. */
void fn_8037983C(struct _ENEMY_WORK* self);
void fn_803799A8(struct _ENEMY_WORK* self);
void fn_80379AC4(struct _ENEMY_WORK* self);
void fn_80379B50(struct _ENEMY_WORK* self);
void fn_80379BD0(struct _ENEMY_WORK* self);
void fn_80379C60(struct _ENEMY_WORK* self);
void fn_80379CDC(struct _ENEMY_WORK* self);
void fn_80379D64(struct _ENEMY_WORK* self);
void fn_8037A848(struct _ENEMY_WORK* self, u32 a);
void fn_8037A964(struct _ENEMY_WORK* self);
void fn_8037AAAC(void);
void fn_8037ACA8(struct _ENEMY_WORK* self, u32 a);
void fn_8037ADE4(struct _ENEMY_WORK* self, u32 a);
void fn_8037A508(struct _ENEMY_WORK* self, u32 a);
void fn_8037A600(struct _ENEMY_WORK* self, u32 a);
void fn_8037A718(struct _ENEMY_WORK* self);
void fn_803794F8(struct _ENEMY_WORK* self);
void fn_80379594(struct _ENEMY_WORK* self);
void fn_80379614(struct _ENEMY_WORK* self);
void fn_80379694(struct _ENEMY_WORK* self);
void fn_8037975C(struct _ENEMY_WORK* self);
}
#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/EM_PART_BLOCK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "ef/fn_80105314.h"
#include "fn_8004CAD8.h"
#include "stage/fn_802B2AA0.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"

/* A 6-argument spelling of `ShellSetFuncs`' +0x2C slot: retail passes no table argument, and the shared
 * 7-argument type hoists the table load (`em_act_prog_5` scores lower). */
typedef void (*ShellJobInjectFn)(struct _ENEMY_WORK* self, s32 a, s32 b, nw4r::math::VEC3* pos, f32 scale,
                                 u16 flags);
/* The unit's `.rodata` motion table `em_turn_seq_start`/`em_turn_seq_step` walk (64 bytes). */
extern "C" u8 lbl_80570AE0[];

/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet. */
extern "C" f32 lbl_8079BC88;   /* 0.0 */
extern "C" f32 lbl_8079BC94;   /* 1.5 */
extern "C" f32 lbl_8079BCA0;   /* 200.0 */
extern "C" f32 lbl_8079BCA8;   /* 1.0 */
extern "C" f32 lbl_8079BCB4;   /* 220.0 */
extern "C" f32 lbl_8079BCC4;   /* 20.0 */
extern "C" f32 lbl_8079BCD4;   /* -900.0 */
extern "C" f32 lbl_8079BCD8;   /* -14.0 */
extern "C" f32 lbl_8079BCDC;   /* 10.0 */
extern "C" f32 lbl_8079BCE0;   /* 50.0 */
extern "C" f32 lbl_8079BCE4;   /* 70.0 */
extern "C" f32 lbl_8079BCE8;   /* 5.0 */
extern "C" f32 lbl_8079BCEC;   /* 196.0 */
extern "C" f32 lbl_8079BCF0;   /* 44.0 */
extern "C" f32 lbl_8079BCF4;   /* 56.0 */
extern "C" f32 lbl_8079BCF8;   /* 62.0 */
extern "C" f32 lbl_8079BD04;   /* 90.0 */
extern "C" f32 lbl_8079BD08;   /* 110.0 */
extern "C" f32 lbl_8079BD24;   /* 100.0 */
extern "C" f32 lbl_8079BE88;   /* -20.0 */

/* `setVec3` through the pointer-returning signature its body has (the shared declaration returns `void`,
 * the call site consumes the result). */
typedef nw4r::math::VEC3* (*Fn80041E8C)(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);

/* This unit's own functions the program steps call, and `enemy/em_model.cpp`'s `fn_803B9BA0`. */

extern "C" {
u8 fn_80378F9C(struct _ENEMY_WORK* self, u32 a);
u8 fn_8037900C(struct _ENEMY_WORK* self, u32 a);
void em019_motion_dispatch(struct _ENEMY_WORK* self);
void em019_state_dispatch(struct _ENEMY_WORK* self);
void fn_8037A470(struct _ENEMY_WORK* self);
void em019_approach_dispatch(struct _ENEMY_WORK* self);
void em019_battle_dispatch(struct _ENEMY_WORK* self);
void fn_8037DAC8(struct _ENEMY_WORK* self);
void fn_8037DC04(struct _ENEMY_WORK* self);
void fn_8037DFF0(struct _ENEMY_WORK* self);
void em019_action_start_if_idle(struct _ENEMY_WORK* self);
void fn_8037E0E8(struct _ENEMY_WORK* self);
u32 fn_80382E48(struct _ENEMY_WORK* self, u32 a);
void fn_803B9BA0(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a);
}
#include "types.h"
#include "gx.h"
#include "sound/mhchar.h"

/* The part-colour id table the steppers index by slot (`{3, 0, 1, 4}`), in the unit's `.data`: declared,
 * not defined. */
extern "C" u32 lbl_805EE5A0[];

extern "C" f32 lbl_8079BC88;   /* 0.0 */

extern "C" {
}
#include "enemy/fn_8012E968.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/note_work.h" /* NoteWork and the pane/slot/layout types (rule 1) */
#include "lobby/lb_quest_screen.h" /* note_pane_get_motion (rule 2: the owner's header) */
#include "ef/pRoot.h"
#include "lobby/lb_server_sel_trans.h" /* fn_803C7EAC / fn_803C7F88 (owner's header, rule 2) */
#include "Runtime.PPCEABI.H/CPlusLibPPC.h" /* __construct_array (owner's header, rule 2) */

extern "C" {
void em_move_mode_set(_ENEMY_WORK* self, u32 a);
}

extern "C" {

/* The em019 action start: the shared action runner's entry wrapper.
 * 0x8037E0D0 */
void em019_action_start(void)
{
    fn_8037AAAC();
}

/* The em019 action start's idle gate: runs the action only from sub-state 0.
 * 0x8037E0D4 */
void em019_action_start_if_idle(struct _ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        em019_action_start();
    }
}

/* Arms the action's "run" flag at `+0x358`.
 * 0x80379084 */
void em019_action_run_flag_set(struct _ENEMY_WORK* self)
{
    self->field_0x358 = 1;
}

/* The em019 per-motion step dispatcher: `+0x1E6` tail-calls one of the eight step bodies.
 * 0x80379DE4 */
void em019_state_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037983C(self);
        return;
    case 1:
        fn_803799A8(self);
        return;
    case 2:
        fn_80379AC4(self);
        return;
    case 3:
        fn_80379B50(self);
        return;
    case 4:
        fn_80379BD0(self);
        return;
    case 5:
        fn_80379C60(self);
        return;
    case 6:
        fn_80379CDC(self);
        return;
    case 7:
        fn_80379D64(self);
        return;
    }
}

/* The em019 battle step dispatcher: `+0x1E6` selects one of the twelve battle step bodies.
 * 0x8037AF08 */
void em019_battle_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037A848(self, 0);
        return;
    case 1:
        fn_8037A964(self);
        return;
    case 2:
        fn_8037AAAC();
        return;
    case 3:
        fn_8037ACA8(self, 0);
        return;
    case 4:
        fn_8037ACA8(self, 1);
        return;
    case 5:
        fn_8037ADE4(self, 0);
        return;
    case 6:
        fn_8037ACA8(self, 2);
        return;
    case 7:
        fn_8037A848(self, 1);
        return;
    case 8:
        fn_8037ACA8(self, 3);
        return;
    case 9:
        fn_8037ADE4(self, 1);
        return;
    case 10:
        fn_8037ADE4(self, 2);
        return;
    case 11:
        fn_8037A848(self, 2);
        return;
    }
}

/* The em019 approach step dispatcher: `+0x1E6` picks the approach or the retreat body.
 * 0x8037A7F0 */
void em019_approach_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037A508(self, 0);
        break;
    case 1:
        fn_8037A600(self, 0);
        break;
    case 2:
        fn_8037A718(self);
        break;
    case 3:
        fn_8037A600(self, 1);
        break;
    case 4:
        fn_8037A508(self, 1);
        break;
    }
}

/* The em019 motion step dispatcher: `+0x1E6` picks one of the five motion bodies.
 * 0x803797F4 */
void em019_motion_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_803794F8(self);
        break;
    case 1:
        fn_80379594(self);
        break;
    case 2:
        fn_80379614(self);
        break;
    case 3:
        fn_80379694(self);
        break;
    case 7:
        fn_8037975C(self);
        break;
    }
}

#ifdef __cplusplus
}
#endif

#pragma peephole off

/* Program channel 1: phase 0 clears +0x1E4, the four part slots and +0x358/+0x35A and arms motion 20,
 * phase 1 ends the program when the motion is done. */
extern "C" void em_act_prog_1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
        s32 i;

        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 0, 0);
        em_demo_pos_set(self, lbl_8079BC88, lbl_8079BC88, lbl_8079BC88);
        em_demo_rot_set(self, lbl_8079BC88, lbl_8079BC88, lbl_8079BC88);
        em_demo_enable(self);
        self->field_0x1E4 = 0;
        for (i = 0; i < 4; i++) {
            part->colour_id[i] = 0;
            part->timer[i] = 0;
            part->meter[i] = lbl_8079BC88;
            part->flag[i] = 0;
            part->r[i] = 0;
            part->g[i] = 0;
            part->b[i] = 0;
            part->a[i] = 0;
        }
        self->field_0x358 = 0;
        self->tev_0x35A = 0;
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* Program channel 2: phase 0 arms motion 201/4, phase 1 runs four frame windows that latch the part
 * helpers, then hands +0x1E4 to part 13/3. */
extern "C" void em_act_prog_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 201, 4, 0);
        fn_801303EC(self, lbl_8079BC88);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_8079BCEC, lbl_8079BC88) == 1)
            fn_80136D14(self);
        if (em_frame_check(self, 0, lbl_8079BCF0, lbl_8079BC88) == 1)
            em_hit_window_set(self, 0, 13, 10);
        if (em_frame_check(self, 0, lbl_8079BCF4, lbl_8079BC88) == 1)
            em_hit_window_set(self, 0, 24, 26);
        if (em_frame_check(self, 0, lbl_8079BCF8, lbl_8079BC88) == 1)
            em_hit_window_clear(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_move_mode_set(self, 4);
            fn_801303EC(self, fn_8013032C(self));
            em_state_set(self, 13, 3);
        }
        break;
    }
}

/* Program channel 3: phase 0 arms motion 4 and the `lbl_80570AE0` sequence, phase 1 polls it and lands
 * +0x1E4 on part 13/4 (`fn_80131E00` runs every frame). */
extern "C" void em_act_prog_3(_ENEMY_WORK* self) {
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_80570AE0, 0, 0, 0);
        em_mot_speed_set(self, lbl_8079BCE8);
        fn_801303EC(self, fn_8013032C(self));
        fn_80378F9C(self, 255);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570AE0) == 1)
            em_state_set(self, 13, 4);
        break;
    }
}

/* Program channel 4: phase 0 arms motion 89 and the `lbl_80570AE0` table, phase 1 waits 256 frames and
 * lands +0x1E4 on part 13/5. */
extern "C" void em_act_prog_4(_ENEMY_WORK* self) {
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 89, 0, 0);
        em_approach_start(self, lbl_8079BC88, 0);
        em_mot_speed_set(self, lbl_8079BCE8);
        fn_801303EC(self, fn_8013032C(self));
        fn_80378F9C(self, 255);
        break;
    case 1:
        if (em_approach_step(self, 0, 256) == 1)
            em_state_set(self, 13, 5);
        break;
    }
}

/* Program channel 5: phase 0 arms motion 202 and two hit windows, phase 1 runs three frame windows, then
 * lands the part 13/6 hit or ends the program. */
extern "C" void em_act_prog_5(_ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 202, 0, 0);
        fn_801303EC(self, lbl_8079BC88);
        fn_80378F9C(self, 255);
        fn_80136D14(self);
        em_hit_window_set(self, 0, 32, 8);
        em_hit_window_set(self, 1, 31, 24);
        fn_80131E00(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_8079BD04, lbl_8079BC88) == 1)
            fn_80136D14(self);
        if (em_frame_check(self, 2, lbl_8079BCA0, lbl_8079BC88) == 1)
            fn_80131E00(self);
        if (em_frame_check(self, 0, lbl_8079BCB4, lbl_8079BC88) == 1) {
            nw4r::math::VEC3 off;

            em_camera_req(self, -1, 5);
            copyVec3(&pos, ((Fn80041E8C)setVec3)(&off, lbl_8079BC88, lbl_8079BC88,
                                                        lbl_8079BD08));
            ((ShellJobInjectFn)shell_set_func_ptr->method_0x2C)(self, 9, 1, &pos, lbl_8079BC94, self->field_0xAEA);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (fn_80382E48(self, 0) == 1)
                em_state_set(self, 13, 6);
            else
                em_action_finish(self);
        }
        break;
    }
}

/* Program channel 6: phase 0 measures the bearing to +0x36C and arms motion 31 or 32, phases 1 and 2 run the
 * turn and the `lbl_80570AE0` sequence, phase 3 hands on. */
extern "C" void em_act_prog_6(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u16 angle;

        self->state++;
        em_move_mode_set(self, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000)
            self->state_0x006 = 1;
        else
            self->state_0x006 = 0;
        switch (self->state_0x006) {
        case 0:
            em_mot_set(self, 31, 10, 0);
            if (angle < 0x2000)
                angle = 0x2000;
            else if (angle > 0x6000)
                angle = 0x6000;
            break;
        case 1:
            em_mot_set(self, 32, 10, 0);
            if (angle < 0xA000)
                angle = (u16)-0x6000;
            else if (angle > 0xE000)
                angle = (u16)-0x2000;
            break;
        }
        em_move_vec2_clr(self);
        self->field_0x318 = lbl_8079BCD8 * get_em_base_scale(self) * get_em_chg_scale(self);
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
        if (angle > 0x8000)
            angle = (u16)(0x10000 - angle);
        self->timer_0x020 = (s16)angle;
        em_hit_window_set_default(self, 0, 30);
        break;
    }
    case 1:
        switch (self->state_0x006) {
        case 0:
            em_turn_in_window(self, lbl_8079BCDC, lbl_8079BCE0, self->timer_0x020);
            break;
        case 1:
            em_turn_in_window(self, lbl_8079BCDC, lbl_8079BCE0, -self->timer_0x020);
            break;
        }
        if (em_frame_check(self, 3, lbl_8079BCC4, lbl_8079BCE4) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_turn_seq_start(self, lbl_80570AE0, 0, 0, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step(self, lbl_80570AE0) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_move_mode_set(self, 0);
            em_mot_set(self, 2, 10, 0);
            em_approach_start(self, lbl_8079BCD4, 0);
        }
        break;
    case 3:
        switch (self->state_0x006) {
        case 0:
            if (em_approach_step(self, 0, 128) == 1) {
                self->state_0x006++;
                if (em_busy_ck(self) == 0)
                    fn_8012F5C4(self, 20, 20, 0, 1);
            }
            break;
        case 1:
            break;
        }
        if (em_busy_ck(self) == 1) {
            if (fn_80382E48(self, 1) == 1)
                em_state_set(self, 13, 7);
            else
                em_action_finish(self);
        }
        break;
    }
}

/* Program channel 7: phase 0 arms motion 6/10, a 300-frame countdown and the `fn_803B9BA0` job, phase 1
 * lands +0x1E4 on part 13/8 when the countdown ends. */
extern "C" void em_act_prog_7(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        self->timer_0x020 = 300;
        fn_803B9BA0(self, &self->pos, 60);
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 13, 8);
        break;
    }
}

/* The enemy work's action-program channel 8: phase 0 arms motion 10 sub-motion 6 and sets the
 * 1000-frame wait at +0x020; phase 1 ends the program once `em_mot_end_ck` reports the motion done. */
extern "C" void em_act_prog_8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 6, 0);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* The program dispatcher: tail-calls the step of the program id at +0x1E6 through the 9-entry jump table
 * (case 0 is `fn_8037E0E8`). */
extern "C" void em_act_prog_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8037E0E8(self);
        break;
    case 1:
        em_act_prog_1(self);
        break;
    case 2:
        em_act_prog_2(self);
        break;
    case 3:
        em_act_prog_3(self);
        break;
    case 4:
        em_act_prog_4(self);
        break;
    case 5:
        em_act_prog_5(self);
        break;
    case 6:
        em_act_prog_6(self);
        break;
    case 7:
        em_act_prog_7(self);
        break;
    case 8:
        em_act_prog_8(self);
        break;
    }
}

/* The per-frame action runner: dispatches on the action id, ticks +0x35A while +0x358 is 1 (clamped at 450)
 * and runs the +0x1E2 hook pair. */
extern "C" void em_act_run(_ENEMY_WORK* self) {
    self->field_0x358 = 0;
    switch (self->action) {
    case 0:
        em019_motion_dispatch(self);
        break;
    case 1:
        em019_state_dispatch(self);
        break;
    case 2:
        fn_8037A470(self);
        break;
    case 3:
        em019_approach_dispatch(self);
        break;
    case 4:
        em019_battle_dispatch(self);
        break;
    case 5:
        fn_8037DAC8(self);
        break;
    case 6:
        fn_8037DC04(self);
        break;
    case 7:
        fn_8037DFF0(self);
        break;
    case 8:
        em019_action_start_if_idle(self);
        break;
    case 9:
        em_act_prog_dispatch(self);
        break;
    case 10:
    case 11:
    case 12:
    case 13:
    default:
        em_action_finish(self);
        break;
    }
    if (self->field_0x358 == 1) {
        if (++self->tev_0x35A > 450)
            self->tev_0x35A = 450;
    } else {
        self->tev_0x35A = 0;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* The per-part damage check: per slot, raise the meter (`em_part_rec_reset`) while unarmed and below the
 * threshold, else tick it down (`em_part_rec_alt_set`). */
extern "C" void em_parts_damage_ck(_ENEMY_WORK* self) {
    if (fn_8037900C(self, 2) == 0 && em_parts_damage_level_get(self, 0) < 2)
        em_part_rec_reset(self, 0);
    else
        em_part_rec_alt_set(self, 0, 0);
    if (fn_8037900C(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        em_part_rec_reset(self, 1);
    else
        em_part_rec_alt_set(self, 1, 1);
    if (fn_8037900C(self, 1) == 0 && em_parts_damage_level_get(self, 1) < 2)
        em_part_rec_reset(self, 2);
    else
        em_part_rec_alt_set(self, 2, 2);
    if (fn_8037900C(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 2) < 1 || em_parts_damage_level_get(self, 3) < 1))
        em_part_rec_reset(self, 3);
    else
        em_part_rec_alt_set(self, 3, 3);
    if (fn_8037900C(self, 0) == 0 &&
        (em_parts_damage_level_get(self, 4) < 1 || em_parts_damage_level_get(self, 5) < 1))
        em_part_rec_reset(self, 4);
    else
        em_part_rec_alt_set(self, 4, 4);
    if (fn_8037900C(self, 0) == 0 && em_parts_damage_level_get(self, 6) < 2)
        em_part_rec_reset(self, 5);
    else
        em_part_rec_alt_set(self, 5, 5);
    if (fn_8037900C(self, 3) == 0 && em_parts_damage_level_get(self, 7) < 1)
        em_part_rec_reset(self, 6);
    else
        em_part_rec_alt_set(self, 6, 6);
}

/* Every twentieth frame, drops the record's own ground-stamp vector: builds the (0, -20, 100) offset
 * and hands it to `eft_spawn_type10` with the record's own flag. */
extern "C" void em_act_effect_ck(_ENEMY_WORK* self) {
    nw4r::math::VEC3 off;

    VEC3_ctor(&off);
    if (em_alt_mode_ck(self) == 1) {
        if (system_w.field_0x0c % 20 == 0) {
            setVector3(&off, lbl_8079BC88, lbl_8079BE88, lbl_8079BD24);
            eft_spawn_type10(self, 25, 25, &off, lbl_8079BCA8);
        }
    }
}

extern "C" {

/* Resets one part slot: stores the caller's K-colour index, clears the slot's timer, meter, arm byte
 * and alpha, and takes the slot's RGB either from the caller's colour or from the shared id table. */
extern "C" void em_part_reset(_ENEMY_WORK* self, u8 idx, u8 colour_id, const u8* colour) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    part->colour_id[idx] = colour_id;
    part->timer[idx] = 0;
    part->meter[idx] = lbl_8079BC88;
    part->flag[idx] = 0;
    if (colour == NULL) {
        GXColor tmp;

        ((MHchar*)self->char_0x024)->getTevKColor(lbl_805EE5A0[idx], GX_KCOLOR3, &tmp);
        part->r[idx] = tmp.r;
        part->g[idx] = tmp.g;
        part->b[idx] = tmp.b;
    } else {
        part->r[idx] = colour[0];
        part->g[idx] = colour[1];
        part->b[idx] = colour[2];
    }
    part->a[idx] = 0;
}

/* Lerps one part slot's RGB toward a target colour by the slot's alpha over the caller's ceiling,
 * and stamps the caller's alpha on the result. */
extern "C" void em_part_colour_lerp(_ENEMY_WORK* self, u8* out, u8 idx, const u8* colour, u8 ceiling) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;
    f32 t = (f32)part->a[idx] / (f32)ceiling;

    out[0] = part->r[idx] + (s32)((f32)(colour[0] - part->r[idx]) * t);
    out[1] = part->g[idx] + (s32)((f32)(colour[1] - part->g[idx]) * t);
    out[2] = part->b[idx] + (s32)((f32)(colour[2] - part->b[idx]) * t);
    out[3] = 255;
}

/* Steps one part slot's damage meter by 1/damage, arming the slot when it reaches 1.0 and disarming
 * it when it falls back to 0.0. */
extern "C" void em_part_damage_meter(_ENEMY_WORK* self, u8 idx, f32 damage) {
    EmPartBlock* part = (EmPartBlock*)&self->action_0x328;

    if (damage <= lbl_8079BC88)
        return;
    if (part->flag[idx] == 0) {
        part->meter[idx] += lbl_8079BCA8 / damage;
        if (part->meter[idx] >= lbl_8079BCA8) {
            part->meter[idx] = lbl_8079BCA8;
            part->flag[idx] = 1;
        }
    } else {
        part->meter[idx] -= lbl_8079BCA8 / damage;
        if (part->meter[idx] <= lbl_8079BC88) {
            part->meter[idx] = lbl_8079BC88;
            part->flag[idx] = 0;
        }
    }
}
}

#pragma peephole on

/* 0x80382BB0 */
extern "C" void fn_80382BB0(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    em_move_mode_set(self, 4);
    *out_a = 12;
    *out_b = 0;
}

/* 0x80382BFC */
extern "C" void fn_80382BFC(void) {
}

/* 0x80382C00 */
extern "C" u32 fn_80382C00(_ENEMY_WORK* self) {
    return em_parts_damage_level_get(self, 7) != 0;
}

/* 0x80382C40 */
extern "C" s32 fn_80382C40(_ENEMY_WORK* self) {
    if (self->field_0x1E2 == 4) {
        if (em_alt_mode_ck(self) == 0) {
            return 1;
        }
    }
    return 0;
}

/* 0x80382DB8 */
extern "C" void fn_80382DB8(_ENEMY_WORK* self, u32 flag) {
    if (flag == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* 0x80382F94 */
extern "C" u32 fn_80382F94(_ENEMY_WORK* self) {
    if (self->action == 13) {
        if ((u8)(self->state_sub - 2) <= 6) {
            return 1;
        }
    }
    return 0;
}

