/*
 * enemy/em010_prog.cpp - enemy 010's program: the per-area seat and entry selectors (`stage_map_kind_get`,
 *   `area_no` and the entry state pick the `fn_80126324` motion), the per-motion state steps and their
 *   `state_sub` dispatchers, and the static initializer.
 * RANGE. .text 0x801663E4-0x8016D1C4 (77 functions); extab 0x8000E1D4-0x8000E3DC, extabindex
 *   0x800291A8-0x800294B4, .ctors 0x8056F32C-0x8056F330, .rodata 0x8056FC10-0x8056FCD0, .data 0x805A6D58-0x805A7CE8
 *   (`em010_prog_tbl` first), .bss 0x806A7898-0x806A7970, .sdata2 0x807974F0-0x807977D8.
 * FLAGS. `cflags_main`, no `#pragma`.
 * NAMES. The file name follows the runtime dump's `em010_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.  Callees whose call sites disagree with the owner's header are called through
 *   `<name>_viewN`/`<name>_cN` cast macros (the same direct call), and `#define <name> <name>_hidden_<header>`
 *   hides the disagreeing declaration around its `#include`.
 *   The `.bss` record names (`vec_pair_801679B0_0`..`_8`) are GUESSes.
 * RESIDUALS. 47 rows unwritten: 0x80167DB4-0x80168010, 0x8016808C-0x80168828, 0x801688B4-0x80168C8C,
 *   0x80168CDC-0x80169360, 0x801694C4-0x80169750, 0x801697BC-0x8016BFC4, 0x8016C09C-0x8016CB78,
 *   0x8016CB88-0x8016CD50, 0x8016CD54-0x8016D1C4.
 *   9 partial rows:
 *  - `fn_80167BA8`: the body lacks retail's opening `a == 2` pair `em_busy_set`/`em_busy_timer_reset`, and
 *    retail re-masks `a` (`clrlwi r0,r31,24`) before each compare;
 *  - `fn_801679B0`, `fn_80169360`: retail re-masks the `u8` argument before each compare, ours compares the
 *    register (`fn_80169360` also emits one more `b`);
 *  - `fn_80166DF8`: ours fuses `clrlwi.` where retail keeps `clrlwi` + `cmpwi`, narrows with `clrlwi r6,r3,16`
 *    before the copy, and gives each test of the chain its own `li r29,0` exit where retail shares one;
 *  - `fn_801678A0`: ours fuses `subic.` where retail keeps `subi` + `cmpwi`, and schedules `li r4,0x1e` early;
 *  - `fn_801676C4`, `fn_80167770`: retail schedules the `li r4,imm` argument two instructions later;
 *  - `fn_801671AC`: retail sign-extends the masked value (`extsh r4,r0`) that ours uses unextended;
 *  - `fn_801663E4`: one compare is `cmplwi` where retail has `cmpwi`, and ours reloads `lbl_807974FC` twice.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata2` claimed, not emitted; `.data` 0x138 against 0xF90; `.text` (0x1DC8 of
 *   0x6DE0), extab (0xA8 of 0x208) and extabindex (0xFC of 0x30C) short of the claim and differing.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_80165FC8.h"
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */
#include "stage/stg_w.h"
#define assignVec3 assignVec3_hidden_ef_h
#include "ef.h"
#undef assignVec3
#include "enemy/EnemyData.h"
#define em_fall_height_get em_fall_height_get_hidden_enemy_h
#define em_frame_flag_set em_frame_flag_set_hidden_enemy_h
#define em_mot_set em_mot_set_hidden_enemy_h
#include "unsplit/enemy.h"
#undef em_mot_set
#undef em_frame_flag_set
#undef em_fall_height_get
#define em_act_ck em_act_ck_hidden_fn_8012BDF4_h
#include "enemy/fn_8012BDF4.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_hit_window_set_default_c1 ((void (*)(_ENEMY_WORK *, s32, s32))em_hit_window_set_default)
#define em_motion_param_set_view1 ((void (*)(struct _ENEMY_WORK*, s32, f32))em_motion_param_set)
#define em_mot_finished_ck_view1 ((u32 (*)(void))em_mot_finished_ck)
#define em_mot_set_view1 ((void (*)(struct _ENEMY_WORK*, s32, s32, s32))em_mot_set)
#define em_frame_flag_set_view1 ((void (*)(struct _ENEMY_WORK*))em_frame_flag_set)
#define em_fall_height_get_view1 ((f32 (*)(struct _ENEMY_WORK*))em_fall_height_get)
#define em_act_ck_view1 ((s32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
#undef em_act_ck
/* Call-site views: each macro casts a callee to the signature its call sites use (the same direct call). */
#define assignVec3_view1 ((void (*)(Vec*, Vec*))assignVec3)

extern "C" {
extern void fn_80168678(_ENEMY_WORK *self);
extern void fn_80168828(_ENEMY_WORK *self);
extern void fn_801688B4(_ENEMY_WORK *self);
extern void fn_80168A78(_ENEMY_WORK *self, s32 a);
extern void fn_80168B7C(_ENEMY_WORK *self);
extern void fn_80168C8C(_ENEMY_WORK *self);
extern void fn_80168CDC(_ENEMY_WORK *self);
extern void fn_8016900C(_ENEMY_WORK *self);
extern void fn_80169750(_ENEMY_WORK *self);
extern void fn_8016BFC4(_ENEMY_WORK *self);

extern void fn_801697BC(_ENEMY_WORK *self);
extern void fn_801698F4(_ENEMY_WORK *self);
extern void fn_80169998(_ENEMY_WORK *self);
extern void fn_80169AB4(_ENEMY_WORK *self);
extern void fn_80169B58(_ENEMY_WORK *self);
extern void fn_80169EAC(_ENEMY_WORK *self);
extern void fn_80169F5C(_ENEMY_WORK *self);
extern void fn_8016A3C4(_ENEMY_WORK *self);
extern void fn_8016A478(_ENEMY_WORK *self);
extern void fn_8016A65C(_ENEMY_WORK *self);
extern void fn_8016A70C(_ENEMY_WORK *self);
extern void fn_8016A8EC(_ENEMY_WORK *self);
extern void fn_8016A998(_ENEMY_WORK *self);
extern void fn_8016AC0C(_ENEMY_WORK *self);
extern void fn_8016ACB0(_ENEMY_WORK *self);
extern void fn_8016AF30(_ENEMY_WORK *self);
extern void fn_8016AFDC(_ENEMY_WORK *self);
extern void fn_8016B114(_ENEMY_WORK *self);
extern void fn_8016B1C0(_ENEMY_WORK *self);
extern void fn_8016B3C0(_ENEMY_WORK *self);
extern void fn_8016B46C(_ENEMY_WORK *self);
extern void fn_8016B60C(_ENEMY_WORK *self);
extern void fn_8016B6B8(_ENEMY_WORK *self);
extern void fn_8016BA24(_ENEMY_WORK *self);
extern void fn_8016BAD0(_ENEMY_WORK *self);
extern void fn_8016BF18(_ENEMY_WORK *self);

extern void fn_80169164(_ENEMY_WORK *self, s32 a);
extern void fn_80169360(_ENEMY_WORK *self, u8 a);
extern void fn_801694C4(_ENEMY_WORK *self, s32 a);

extern f32 lbl_807974FC;
extern f32 lbl_80797518;
extern f32 lbl_8079751C;
extern f32 lbl_80797520;
extern f32 lbl_80797524;
extern f32 lbl_80797528;

extern u32 lbl_8056FC10[];
}

u32 fn_801663E4(_ENEMY_WORK* self) {
    u32 stale = 0;
    u8 map = stage_map_kind_get(self->field_0x1E0);

    switch (map) {
    case 1:
        switch (self->area_no) {
        case 1:
            fn_80126324(self, 2, 5, lbl_807974F0);
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 9, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 0x15, 0xA, lbl_807974F0);
                break;
            case 4:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 9, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 9, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 9, lbl_807974F0);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 3:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 9, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 9:
            switch (self->field_0x9F6) {
            case 4:
                fn_80126324(self, 2, 0x11, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 0x11, lbl_807974F0);
                break;
            }
            break;
        case 10:
            fn_80126324(self, 4, 5, lbl_807974F0);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 2:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 4:
                fn_80126324(self, 3, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 0x13, 0x14, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 4, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 0x14, lbl_807974F0);
                break;
            case 4:
                fn_80126324(self, 3, 0x13, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 0x14, lbl_807974F0);
                break;
            }
            break;
        case 4:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 0x24, 0x27, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 0x25, 0x26, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 4:
                fn_80126324(self, 2, 6, lbl_807974F0);
                break;
            case 7:
                fn_80126324(self, 0x18, 7, lbl_807974F0);
                break;
            case 9:
                fn_80126324(self, 3, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 6, lbl_807974F0);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 4:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            case 7:
                fn_80126324(self, 3, 4, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 5, lbl_807974F0);
                break;
            }
            break;
        case 7:
            switch (self->field_0x9F6) {
            case 5:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 9:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 10:
            fn_80126324(self, 1, 3, lbl_807974F4);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 4:
        switch (self->area_no) {
        case 1:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 2:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            case 3:
                fn_80126324(self, 4, 6, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 5, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            }
            break;
        case 3:
            switch (self->field_0x9F6) {
            case 1:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            case 2:
                fn_80126324(self, 4, 6, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 5, 7, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 3, 6, lbl_807974F0);
                break;
            }
            break;
        case 5:
            switch (self->field_0x9F6) {
            case 3:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 6:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 6:
            switch (self->field_0x9F6) {
            case 2:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            case 5:
                fn_80126324(self, 3, 5, lbl_807974F0);
                break;
            default:
                fn_80126324(self, 2, 4, lbl_807974F0);
                break;
            }
            break;
        case 7:
            fn_80126324(self, 0, 2, lbl_807974F4);
            return 1;
        case 9:
            fn_80126324(self, 0, 2, lbl_807974F4);
            return 1;
        default:
            stale = 1;
            break;
        }
        break;
    case 8:
        if (self->area_no == 1) {
            fn_80126324(self, 0, 1, lbl_807974F8);
            return 1;
        }
        stale = 1;
        break;
    case 9:
        if (self->area_no == 0) {
            fn_80126324(self, 0, 1, lbl_807974F8);
            return 1;
        }
        stale = 1;
        break;
    default:
        stale = 1;
        break;
    }
    if (stale == 1) {
        self->pos.z = lbl_807974FC;
        self->pos.y = lbl_807974FC;
        self->pos.x = lbl_807974FC;
        self->field_0x1C4 = 0;
        self->field_0x1C0 = 0;
        self->field_0x1BC = 0;
        fn_8012B380(self, 5, 2, 0xA);
    }
    return 0;
}

void fn_80166DF8(_ENEMY_WORK* self, u32 kind) {
    VEC3 origin;
    u8 a;
    u8 b;
    u32 running;
    u8 map;

    setVec3(&origin, lbl_807974FC, lbl_807974FC, lbl_807974FC);
    self->field_0x835 = 1;
    switch ((u8)kind) {
    case 0:
        if (self->field_0x00C == 3) {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0) {
                if (self->area_no == data->field_0x14) {
                    fn_80127308(self, &data->vec_0x24, &self->pos, 0);
                    em_wave_amp(&self->pos, &self->pos, lbl_807974F0, 0, (u16)ran_suu(0));
                } else {
                    fn_80127308(self, &origin, &self->pos, 0);
                    em_wave_amp(&self->pos, &self->pos, lbl_807974F0, 0, (u16)ran_suu(0));
                }
                self->field_0x1C4 = 0;
                self->field_0x1BC = 0;
                self->field_0x1C0 = (u16)ran_suu(1);
            }
        }
        break;
    case 2:
        fn_8016C674(self, &a, &b);
        fn_80128A8C(self, a, b);
        em_state_refresh(self);
        break;
    case 3:
        map = stage_map_kind_get(self->field_0x1E0);
        running = 0;
        if (self->field_0x00C == 3) {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0 && fn_80176AA8(data->work_0x18) == 1) {
                switch (map) {
                case 1:
                    if (self->area_no == 2 && self->field_0x9F6 == 1) {
                        running = 0;
                    } else {
                        running = 1;
                    }
                    break;
                case 2:
                    if (self->area_no == 2 && self->field_0x9F6 == 1) {
                        running = 0;
                    } else if (self->area_no == 4
                               && (self->field_0x9F6 == 1 || self->field_0x9F6 == 6)) {
                        running = 0;
                    } else if (self->area_no == 5 && self->field_0x9F6 == 9) {
                        running = 0;
                    } else if (self->area_no == 6
                               && (self->field_0x9F6 == 4 || self->field_0x9F6 == 7)) {
                        running = 0;
                    } else if (self->area_no == 7 && self->field_0x9F6 == 6) {
                        running = 0;
                    } else {
                        running = 1;
                    }
                    break;
                case 4:
                    running = 1;
                    break;
                }
            }
        }
        if (running == 0) {
            if (fn_801663E4(self) == 0) {
                em_move_mode_set(self, 0);
                fn_80128A8C(self, 2, 0);
            } else {
                em_move_mode_set(self, 0);
                fn_80128A8C(self, 0xC, 1);
            }
        } else if (map == 2 && self->area_no == 4) {
            em_move_mode_set(self, 0);
            fn_80126324(self, 0x13, 0x14, lbl_80797500);
            fn_80128A8C(self, 0xC, 1);
        } else if (map == 2 && self->area_no == 5) {
            em_move_mode_set(self, 0);
            fn_80126324(self, 0x13, 0x15, lbl_807974F8);
            fn_80128A8C(self, 0xC, 1);
        } else {
            fn_8016C674(self, &a, &b);
            fn_80128A8C(self, a, b);
        }
        em_state_refresh(self);
        break;
    }
}

void fn_801671A8(void) {
}

void fn_801671AC(_ENEMY_WORK* self) {
    if (em_die_ck(self) == 0) {
        if (self->field_0x834 == 1 || fn_801337FC(self) == 1) {
            fn_8012E664(self);
            fn_8013072C(self, 1, 0);
        }
        {
            _ENEMY_DATA* data =
                (_ENEMY_DATA*)enemy_data_find(enemy_data_grp(self->team, self->field_0x00A), self->field_0x46C);
            if (data != 0) {
                if (self->field_0x46C == data->field_0x17) {
                    copyVec3(&self->aim, &data->vec_0x24);
                }
                if (self->field_0x011 == 0) {
                    if (data->work_0x18 != 0 && data->work_0x18->area_no == self->area_no
                        && em_act_ck(data->work_0x18, 1, 6) == 1
                        && em_frame_check(data->work_0x18, 0, lbl_80797504, lbl_807974FC) == 1
                        && self->field_0x916 <= 0) {
                        fn_8012CEB4(self, (s16)(ran_suu(0) & 0x1F), 0);
                    }
                }
                if (data->field_0x08 == 0xFF) {
                    self->field_0x46C = 0xFF;
                    fn_8013AAC4(self);
                    fn_8012B380(self, 6, 0xFF, 0);
                }
            }
        }
    }
}

void fn_8016730C(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167388(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xB, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167404(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        return fn_8016730C(self);
    case 1:
        return fn_80167388(self);
    case 2:
        return fn_8016730C(self);
    case 3:
        return fn_8016730C(self);
    case 4:
        return fn_8016730C(self);
    case 6:
        return fn_8016730C(self);
    }
}

void fn_80167458(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
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

void fn_801674D4(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167550(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801675CC(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80167648(_ENEMY_WORK* self) {
    u8 state = self->state;

    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_801676C4(_ENEMY_WORK* self) {
    u8 state;

    em_frame_flag_set();
    em_busy_timer_reset(self);
    fn_80131DF4(self);
    state = self->state;
    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 2, 0);
        em_motion_param_set(self, 0x1A, lbl_807974FC);
        break;
    case 1:
        if (self->field_0x1D4 <= lbl_807974FC) {
            fn_8012E694(self);
        }
        break;
    }
}

void fn_80167770(_ENEMY_WORK* self) {
    u8 state;

    em_frame_flag_set();
    em_busy_timer_reset(self);
    fn_80131DF4(self);
    state = self->state;
    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80797508, lbl_807974FC) == 1) {
            self->state = self->state + 1;
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->field_0x314 = lbl_8079750C;
            self->field_0x318 = lbl_80797510;
            self->field_0x320 = lbl_80797514;
            em_motion_param_set(self, 0x10, lbl_807974FC);
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x1D4 <= lbl_807974FC || self->pos.y <= self->field_0x20C) {
            fn_8012E694(self);
        }
        break;
    }
}

void fn_801678A0(_ENEMY_WORK* self) {
    u8 state;

    em_frame_flag_set();
    em_busy_timer_reset(self);
    fn_80131DF4(self);
    state = self->state;
    switch (state) {
    case 0:
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 8, 0);
        em_motion_param_set(self, 0x1E, lbl_807974FC);
        self->timer_0x020 = 0x20;
        break;
    case 1:
        if (--self->timer_0x020 <= 0 || self->field_0x1D4 <= lbl_807974FC) {
            fn_8012E694(self);
        }
        break;
    }
}

void fn_80167968(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        return fn_80167458(self);
    case 2:
        return fn_801674D4(self);
    case 3:
        return fn_80167550(self);
    case 4:
        return fn_801675CC(self);
    case 5:
        return fn_80167648(self);
    case 6:
        return fn_801676C4(self);
    case 7:
        return fn_80167770(self);
    case 8:
        return fn_801678A0(self);
    }
}

/* Advances the action's start-up state and seeds the motion from the action code. */
extern "C" void fn_801679B0(_ENEMY_WORK *self, u8 a, u8 b) {
    if (a == 7) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set_view1(self);
    }

    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        switch (a) {
        case 0:
            em_approach_start(self, lbl_80797518, 0);
            break;
        case 1:
            em_approach_start(self, lbl_8079751C, 0);
            break;
        case 2:
            em_approach_start(self, lbl_80797520, 0);
            break;
        case 3:
            em_approach_start(self, lbl_80797524, 0);
            break;
        case 4:
            em_approach_start(self, lbl_80797520, 0);
            break;
        case 5:
            em_approach_start(self, lbl_8079751C, 0);
            break;
        case 6:
            em_approach_start(self, lbl_80797528, 0);
            break;
        case 7:
            em_approach_start(self, lbl_807974FC, 0);
            break;
        case 8:
            em_approach_start(self, lbl_807974FC, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x200) == 1) {
            switch (b) {
            case 0:
                self->state += 1;
                em_mot_set_view1(self, 0xF, 0, 0);
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

/* ---- small dispatchers and predicates ---- */

extern "C" void fn_80167BA8(_ENEMY_WORK *self, u8 a) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056FC10, 0, 0, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FC10) == 1) {
            if (a == 0 || a == 2) {
                self->state += 1;
                em_mot_set_view1(self, 0x13, 2, 4);
                return;
            }
            em_action_finish(self);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" s32 fn_8016CB78(_ENEMY_WORK *self) {
    return self->field_0x1E2 == 0;
}

extern "C" void fn_8016CD50(void) {}

extern "C" void fn_8016C054(_ENEMY_WORK *self) {
    switch (self->action) {
    case 0:
        fn_80167404(self);
        return;
    case 1:
        fn_80167968(self);
        return;
    case 2:
        fn_80168678(self);
        return;
    case 7:
        fn_80168C8C(self);
        return;
    case 10:
        fn_80168CDC(self);
        return;
    case 11:
        fn_8016900C(self);
        return;
    case 12:
        fn_80169750(self);
        return;
    case 13:
        fn_8016BFC4(self);
        return;
    }
}

extern "C" void fn_80168C8C(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_80168828(self);
        return;
    case 1:
        fn_801688B4(self);
        return;
    case 2:
        fn_80168A78(self, 0);
        return;
    case 3:
        fn_80168A78(self, 1);
        return;
    case 4:
        fn_80168B7C(self);
        return;
    }
}

/* The start-up steps: advance `state_0x05` and seed the motion. */
extern "C" void fn_80167CBC(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_view1(self, 0x15, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80167D38(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_view1(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80168010(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_view1(self, 0x14, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80169448(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_view1(self, 3, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* The dispatchers on `action_0x1E5` or `state_sub`. */
extern "C" void fn_80169750(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_80169164(self, 0);
        return;
    case 1:
        fn_80169360(self, 0);
        return;
    case 2:
        fn_80169448(self);
        return;
    case 3:
        fn_80169164(self, 1);
        return;
    case 4:
        fn_801694C4(self, 1);
        return;
    case 5:
        fn_801694C4(self, 0);
        return;
    case 6:
        fn_80169164(self, 2);
        return;
    case 7:
        fn_801694C4(self, 2);
        return;
    case 8:
        fn_80169360(self, 1);
        return;
    }
}

extern "C" void fn_80168828(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_view1(self, 0x1B, 4, 0);
        em_hit_window_set_default_c1(self, 0, 1);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016BFC4(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_801697BC(self);
        return;
    case 1:
        fn_801698F4(self);
        return;
    case 2:
        fn_80169998(self);
        return;
    case 3:
        fn_80169AB4(self);
        return;
    case 4:
        fn_80169B58(self);
        return;
    case 5:
        fn_80169EAC(self);
        return;
    case 6:
        fn_80169F5C(self);
        return;
    case 7:
        fn_8016A3C4(self);
        return;
    case 8:
        fn_8016A478(self);
        return;
    case 9:
        fn_8016A65C(self);
        return;
    case 10:
        fn_8016A70C(self);
        return;
    case 11:
        fn_8016A8EC(self);
        return;
    case 12:
        fn_8016A998(self);
        return;
    case 13:
        fn_8016AC0C(self);
        return;
    case 14:
        fn_8016ACB0(self);
        return;
    case 15:
        fn_8016AF30(self);
        return;
    case 16:
        fn_8016AFDC(self);
        return;
    case 17:
        fn_8016B114(self);
        return;
    case 18:
        fn_8016B1C0(self);
        return;
    case 19:
        fn_8016B3C0(self);
        return;
    case 20:
        fn_8016B46C(self);
        return;
    case 21:
        fn_8016B60C(self);
        return;
    case 22:
        fn_8016B6B8(self);
        return;
    case 23:
        fn_8016BA24(self);
        return;
    case 24:
        fn_8016BAD0(self);
        return;
    case 25:
        fn_8016BF18(self);
        return;
    }
}

extern "C" void fn_80169360(_ENEMY_WORK *self, u8 a) {
    if (a == 1) {
        em_frame_flag_set_view1(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xE, 8, 0);
        em_approach_start(self, lbl_807974FC, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0xA0) == 1) {
            self->state += 1;
            em_mot_set_view1(self, 0xCA, 6, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The unit's `.bss` (0x806A7898..0x806A7970): the nine two-vector records `fn_8016CF18` seeds.  Names are
 * GUESSes (each record is a pair of model-space points). */
VEC3 vec_pair_801679B0_0[2];  /* +0x806A7898 */
VEC3 vec_pair_801679B0_1[2];  /* +0x806A78B0 */
VEC3 vec_pair_801679B0_2[2];  /* +0x806A78C8 */
VEC3 vec_pair_801679B0_3[2];  /* +0x806A78E0 */
VEC3 vec_pair_801679B0_4[2];  /* +0x806A78F8 */
VEC3 vec_pair_801679B0_5[2];  /* +0x806A7910 */
VEC3 vec_pair_801679B0_6[2];  /* +0x806A7928 */
VEC3 vec_pair_801679B0_7[2];  /* +0x806A7940 */
VEC3 vec_pair_801679B0_8[2];  /* +0x806A7958 */
