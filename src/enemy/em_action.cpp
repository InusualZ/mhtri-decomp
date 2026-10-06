/* enemy/em_action.cpp - the enemy action band: the entry action and its state record, the action-sub-state
 *   dispatcher and the motions it dispatches, the aim and rotation helpers and the tutorial camera gate.
 * RANGE. .text 0x803253BC-0x8032C920 (85 functions); .ctors 0x8056F39C-0x8056F3A0 (`fn_8032C65C`),
 *   .rodata 0x80570840-0x80570880, .data 0x805DD9E0-0x805DFC30, .bss 0x806BE2C8-0x806BE328,
 *   .sdata 0x80792D08-0x80792D18, .sbss 0x80794B88-0x80794B90, .sdata2 0x8079AF48-0x8079B108, extab, extabindex.
 * SEAM. Data-complete: of the 188 labels the target object relocates against, 184 (the whole `.sdata2` run
 *   0x8079AF4C-0x8079B100, the `.data` run 0x805DDA50-0x805DFC00 and `lbl_80570840`) are referenced by no other target
 *   object, the other four are the `.bss` globals `lbl_806BE2C8`..`lbl_806BE310`, and the `.sdata2` tiling is
 *   disjoint and ordered on both sides.  The range references no string.
 * NAMES. The file and the 14 symbols it defines are GUESSES from the bodies on the module's `em_*` scheme (no dump or
 *   `__FILE__` name covers the band): `em_act_slots_clr` clears the four action halfwords, `em_act_entry_start`
 *   starts the entry action and installs its state record, `em_act_frame_ck` is its per-step hook, `em_act_dispatch`
 *   switches on the action sub-state, `em_act_arm_m06s07` arms motion 6 sub-state 7, `em_act_aim` clamps the aim angle
 *   into a facing window, `em_tut_cam_ck` is the tutorial-map/camera-distance gate, `em_act_rec_init`/`em_act_noop`
 *   are the 0xC-byte action record's constructor and empty step.
 * RESIDUALS. 72 rows unwritten: 0x803257C4-0x80325BFC (`em_act_move_step` among them), 0x80325EC4-0x8032C920 (the
 *   static initializer `fn_8032C65C` among them).
 *  - `em_tut_cam_ck`: ours copies the by-value `get_camera_pos()` result through a temporary where retail hands it
 *    straight to `copyVec3` (frame 0x30 against 0x20; the owner's `const VEC3*` spelling forces the copy);
 *  - `em_act_hold`: ours keeps `state` in a saved register where retail reloads it; `em_act_frame_ck`: an extra `b`
 *    and retail's `clrlwi` + `cmpwi` mode test; `em_act_entry_start`: retail narrows the mode with `clrlwi`;
 *  - `em_act_aim`: retail compares against a double (`lfd` + `fcmpo` + `cror`), ours against a float (`lfs` + `fcmpu`);
 *  - `em_act_mot21`: retail loads `lbl_8079AF54` into f1 before `em_fall_start`, whose owner header declares
 *    `(self)` only;
 *  - `em_rot_reset`: retail compares signed (`cmpwi`), ours unsigned; `em_act_rec_init`, `em_act_follow`: register
 *    allocation only.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.rodata`/`.sbss`/`.sdata`/`.sdata2` claimed, not emitted; `.text`/extab/
 *   extabindex short of the claim; candidate fold with `menu/fn_8031EA8C.cpp` (1 shared pool literal, text
 *   interleaved, confidence low).
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
#include "enemy/fn_80147CE0.h"
#include "enemy/enemy_control.h"
#include "fn_8004CAD8.h"
#include "ef.h"
#include "g3d/g3d_resanmchr.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8028F66C.h"
#include "stage/stg_w.h"
#include "camera/camera.h"
#include "sys_mem.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */

extern "C" {

/* The unit's own entry points: the original file's definition order is not the address order, so the
 * callers below need these prototypes. */
void* em_act_rec_init(void* self);
void em_act_move_step(_ENEMY_WORK* self, u8 mode, f32 step);
void em_act_hold(_ENEMY_WORK* self);
void em_act_follow(_ENEMY_WORK* self);
void em_act_mot21(_ENEMY_WORK* self);

/* Zeroes a 3-vector, then reports whether the current map/area is the tutorial's and the camera is
 * past the given height. */
s32 em_tut_cam_ck(void) {
    nw4r::math::VEC3 zeroed;
    nw4r::math::VEC3 cam;
    VEC3_ctor(&zeroed);
    if (get_now_mapno() != 0x16) {
        return 1;
    }
    if (get_now_areano() == 2) {
        cam = get_camera_pos();
        copyVec3(&zeroed, &cam);
        if (zeroed.x >= lbl_8079AF4C) {
            return 0;
        }
    }
    return 1;
}

/* Resets the record's three rotation angles; re-arms the two "rotation handed over" angles when the
 * area is 7/8 and the sub-state matches. */
void em_rot_reset(_ENEMY_WORK* self) {
    if (stage_map_kind_get(self->field_0x1E0) == 2) {
        switch (self->area_no) {
        case 7:
            if (self->field_0x9F6 == 8) {
                fn_80126324(self, 1, 2, lbl_8079AF50);
                self->field_0x1C0 = 0x8000;
                return;
            }
            fn_80126324(self, 1, 2, lbl_8079AF50);
            return;
        case 8:
            if (self->field_0x9F6 == 7) {
                fn_80126324(self, 1, 2, lbl_8079AF50);
                self->field_0x1C0 = 0xF000;
                return;
            }
            fn_80126324(self, 1, 2, lbl_8079AF50);
            return;
        default:
            self->pos.z = lbl_8079AF54;
            self->pos.y = lbl_8079AF54;
            self->pos.x = lbl_8079AF54;
            self->field_0x1C4 = 0;
            self->field_0x1C0 = 0;
            self->field_0x1BC = 0;
            return;
        }
    } else {
        self->pos.z = lbl_8079AF54;
        self->pos.y = lbl_8079AF54;
        self->pos.x = lbl_8079AF54;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
    }
}

/* Clears the band's four halfword action slots. */
void em_act_slots_clr(_ENEMY_WORK* self) {    self->band_0x328.arm_0x32A = 0;
    self->band_0x328.arm_0x32C = 0;
    self->band_0x328.count_0x334 = 0;
    self->band_0x328.limit_0x336 = 0;
}

/* Starts the band's entry action for the given mode and installs its state record once. */
void em_act_entry_start(_ENEMY_WORK* self, u8 mode) {
    switch (mode) {
    case 0:
        em_move_mode_set(self, 4);
        fn_80128A8C(self, 6, 0x20);
        em_state_refresh(self);
        break;
    case 3:
        em_move_mode_set(self, 4);
        fn_80128A8C(self, 6, 0x16);
        em_rot_reset(self);
        em_state_refresh(self);
        break;
    }
    self->field_0x835 = 1;
    if (em_res_user_data_ck(self) == 0) {
        void* record = operator new(0xC);
        if (record != NULL) {
            em_act_rec_init(record);
        }
        em_res_user_data_set(self, record);
    }
}

/* Initialises the 0xC-byte action record's shared vtable word. */
void* em_act_rec_init(void* self) {
    em_res_user_data_ctor(self);
    *(void**)self = (void*)&lbl_805DFC00;
    return self;
}

void em_act_noop(void) {
}

/* Per-step hook of the band's entry action: arms the transition flag for the motion and state the
 * caller reports, and asks for the "leave" transition when the enemy data entry disagrees. */
void em_act_frame_ck(_ENEMY_WORK* self, u8 motion, u8 state) {
    u8 kind = self->field_0x00A;

    switch (kind) {
    case 0:
        self->band_0x328.arm_0x32A = 0;
        self->band_0x328.arm_0x32C = 0;
        switch (motion) {
        case 6:
            if ((u32)(state - 0x14) <= 1U) {
                self->band_0x328.arm_0x32C = 1;
            }
            return;
        case 11: {
            _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, kind),
                                                           self->field_0x46C);
            if (entry != NULL && entry->field_0x08 == 1) {
                fn_80346268(2, self->area_no);
            }
            return;
        }
        case 7:
            if (state == 5) {
                self->band_0x328.arm_0x32A = 1;
            }
            return;
        }
        return;
    case 1:
        if (self->field_0x43C != 0) {
            _ENEMY_DATA* entry = (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, kind),
                                                           self->field_0x46C);
            if (entry != NULL) {
                if (entry->field_0x0B != 1) {
                    fn_8013072C(self, 0, 0);
                }
            } else {
                fn_8013072C(self, 0, 0);
            }
        }
        return;
    }
}

/* Arms the band's fifth action: motion 6 sub-state 7. */
void em_act_arm_m06s07(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 7);
    fn_80133BB4(self);
}

/* The band's 50-frame hold: steps its own state byte and hands the follow-up motion to the move
 * engine. */
void em_act_hold(_ENEMY_WORK* self) {
    u8 state = self->state;

    fn_801303EC(self, lbl_8079AF5C);
    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 1, 6, 0, 3);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

/* The band's follow-up motion: resets the pitch to zero when the move engine reports done. */
void em_act_follow(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_move_mode_set(self, 4);
        fn_8012F5C4(self, 1, 6, 0, 3);
        fn_801303EC(self, lbl_8079AF54);
        return;
    case 1:
        em_act_move_step(NULL, 0, lbl_8079AF58);
        if (em_mot_end_ck(self) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

/* The band's 21-step motion: hands it to the move engine, then reports done. */
void em_act_mot21(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = (u8)(state + 1);
        em_fall_start(self);
        em_mot_set(self, 0x15, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        return;
    }
}

/* Per-step dispatch of the band's four motions on the action sub-state. */
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
    if (self->field_0x314 == lbl_8079AF60) {
        s32 angle = fn_80050C40(&self->vec_0x194, &self->pos);
        self->field_0x1BC = angle;
        if ((u32)(angle + 0xFFFF5FFF) <= 0x3FFEU) {
            self->field_0x1BC = 0xE000;
        }
    } else {
        s32 angle = fn_80050C40(&self->vec_0x194, &self->pos);
        self->field_0x1BC = angle;
        if ((u32)(angle - 0x2001) <= 0x1FFEU) {
            self->field_0x1BC = 0x2000;
        }
    }
}

} /* extern "C" */