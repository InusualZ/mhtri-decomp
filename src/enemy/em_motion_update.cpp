/* enemy/em_motion_update.cpp - an enemy's per-frame motion/action driver: `fn_8013791C` (an outer phase switch around
 *   an inner action switch) with its init, action callbacks and angle helpers.
 * RANGE. .text 0x8013791C-0x80138074 (7 functions); extab, extabindex.
 * NAMES. `em_motion_update` is a GUESS from what the driver does; the map has only `fn_` stems for the range.
 * RESIDUALS. `fn_80137EE0`: retail narrows the mode (`clrlwi r30,r4,24`) after the first call where ours does it
 *   at entry, and lays out the `-1` return as a branch past a shared block where ours duplicates it.
 *   flipcheck: `.text` differs from the target (53 of 1880 bytes).
 * SHAPES. `#pragma peephole off` over the bodies (retail keeps the `clrlwi`/`rlwinm`/`and` + `cmpwi` bit tests
 *   unfused); `u32 kind_lo = (u8)kind` in `fn_80137EE0` is the only spelling that keeps the mask (`u8 kind_lo = kind`
 *   drops it).
 */

#include "enemy/em_act_advance.h" /* em_act_advance (rule 2: the owner's header) */
#include "enemy/fn_801260BC.h" /* fn_801260BC (rule 2: the owner's header) */
#include "enemy/fn_801260E0.h" /* fn_801260E0 (rule 2: the owner's header) */
#include "enemy/fn_80126844.h" /* fn_80126844 (rule 2: the owner's header) */
#include "enemy/fn_80127D94.h" /* fn_80127D94 (rule 2: the owner's header) */
#include "enemy/fn_8012820C.h" /* fn_8012820C (rule 2: the owner's header) */
#include "enemy/fn_80128A30.h" /* fn_80128A30 (rule 2: the owner's header) */
#include "enemy/em_act_end.h" /* em_act_end (rule 2: the owner's header) */
#include "enemy/em_status_set.h" /* em_status_set (rule 2: the owner's header) */
#include "enemy/fn_80130844.h" /* fn_80130844 (rule 2: the owner's header) */
#include "enemy/fn_80130B28.h" /* fn_80130B28 (rule 2: the owner's header) */
#include "enemy/fn_80131DB4.h" /* fn_80131DB4 (rule 2: the owner's header) */
#include "enemy/fn_80131DF4.h" /* fn_80131DF4 (rule 2: the owner's header) */
#include "enemy/fn_80132064.h" /* fn_80132064 (rule 2: the owner's header) */
#include "enemy/fn_801322B4.h" /* fn_801322B4 (rule 2: the owner's header) */
#include "enemy/em_state_refresh.h" /* em_state_refresh (rule 2: the owner's header) */
#include "enemy/fn_80133C48.h" /* fn_80133C48 (rule 2: the owner's header) */
#include "enemy/fn_80133DB0.h" /* fn_80133DB0 (rule 2: the owner's header) */
#include "types.h"
#include "hud/em_net_send.h" /* the owner's leaf header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/pRoot.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_801260BC_c1 ((u32 (*)(_ENEMY_WORK*))fn_801260BC)
#define fn_801260E0_c1 ((u32 (*)(_ENEMY_WORK*))fn_801260E0)

/* The mangled callees, declared at C++ scope with the signature each mangling encodes. */

/* `get_enemy_data__FP11_ENEMY_WORK` (`enemy/em_common.cpp`); only the record's leading flag word is read, at the
 * offset the target loads, because `enemy.h`'s `EnemyData` cannot be included beside `enemy/ENEMY_WORK.h`. */
struct EnemyData;
struct EnemyData* get_enemy_data(_ENEMY_WORK* work);

/* `em_act_ck__FP11_ENEMY_WORKUcUc` (`enemy/em_common.cpp`). */
u32 em_act_ck(_ENEMY_WORK* work, u8 action, u8 arg);

/* `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);

/* `ran_suu__Fl` (`ef/system_core.cpp`). */
s32 ran_suu(s32 range);

/* `fn_800E0914`'s parameter is a model base, not a `_ENEMY_WORK`; the record embeds one at +0x24. */
struct MHchar;

extern "C" {
void fn_80137C94(_ENEMY_WORK* self);

/* `enemy/fn_80138074.c` */
void fn_8013823C(_ENEMY_WORK* self);
void fn_80138E18(_ENEMY_WORK* self);
void fn_80138E28(_ENEMY_WORK* self);
void fn_80139024(_ENEMY_WORK* self);
void fn_8013A978(_ENEMY_WORK* self);

/* `g3d/g3d_scnmdl.cpp`: finds a scene model by id. */
void g3d_root_model_bind(s32 root, u32 id);

/* `sound/mhchar.cpp`: refreshes the model base at `_ENEMY_WORK::char_0x024`. */
void fn_800E0914(struct MHchar* model);

/* `enemy/fn_8011D448.cpp`'s and `enemy/enemy_control.cpp`'s helpers */
void fn_8011E5EC(_ENEMY_WORK* self);
void fn_8011E960(_ENEMY_WORK* self);
u8 fn_8014278C(u16 a, u16 b);
f32 fn_80142958(u16 a, u16 b);
void fn_80144584(_ENEMY_WORK* self, s32 mode);
}

#pragma peephole off

/* The per-frame driver and its callbacks. */

/* One frame of the record's action state machine. */
extern "C" void fn_8013791C(_ENEMY_WORK* self)
{
    self->field_0x1DE = 0;
    switch (self->field_0x004) {
    case 0:
        self->field_0x004++;
        fn_80137C94(self);
        fn_80131DB4(self);
        fn_80131DF4(self);
        fn_800E0914((struct MHchar*)&self->char_0x024);
        break;

    case 1:
        if (self->field_0xAEE != 0) {
            if ((self->field_0x1C8 & 8) != 0 && (self->field_0x1C8 & 1) != 0) {
                em_net_send(self, 3, 0);
            }
            self->field_0xAEE = 0;
        }
        fn_8013823C(self);
        {
        u32 refresh = 0;

        switch (self->state_0x017) {
        case 0:
            if (self->field_0x011 == 2) {
                em_act_end(self, 1);
            } else {
                self->field_0x1DE = 1;
            }
            break;

        case 1:
            self->field_0x016++;
            self->field_0x012 = 0;
            if ((self->field_0x1C8 & 8) != 0) {
                u32 rnd;

                rnd = ran_suu(1);
                self->field_0x7C8 = fn_8014278C(self->field_0x01A, rnd);
                rnd = ran_suu(1);
                self->field_0x1D0 = fn_80142958(self->field_0x01A, rnd);
                em_net_send(self, 4, 0);
            } else {
                fn_80144584(self, 2);
            }
            em_act_advance(self, 1);
            refresh = 1;
            break;

        case 2:
            if ((self->field_0x1C8 & 8) != 0) {
                u32 rnd;

                self->field_0x012++;
                rnd = ran_suu(1);
                self->field_0x7C8 = fn_8014278C(self->field_0x01A, rnd);
                rnd = ran_suu(1);
                self->field_0x1D0 = fn_80142958(self->field_0x01A, rnd);
                em_net_send(self, 4, 0);
                em_act_advance(self, 1);
                refresh = 1;
            }
            break;

        case 3:
            em_act_end(self, 0);
            break;

        case 4:
            if ((self->field_0x1C8 & 8) != 0) {
                u32 rnd;

                rnd = ran_suu(1);
                self->field_0x7C8 = fn_8014278C(self->field_0x01A, rnd);
            rnd = ran_suu(1);
                self->field_0x1D0 = fn_80142958(self->field_0x01A, rnd);
            }
            em_net_send(self, 4, 1);
            em_act_end(self, 0);
            break;

        case 5:
            em_act_end(self, 2);
            break;
        }

        if (refresh == 1) {
            fn_80131DB4(self);
            fn_80131DF4(self);
            fn_800E0914((struct MHchar*)&self->char_0x024);
        }
        if (self->model_flag_0x058 != 0) {
            g3d_root_model_bind(pRoot, self->field_0x13C);
        }
        }
        break;

    case 2:
        fn_80138E18(self);
        break;

    case 3:
        fn_80138E28(self);
        break;
    }
}

/* Latches the record's static data and reseats the helpers `fn_8013791C` drives. */
extern "C" void fn_80137C20(_ENEMY_WORK* self)
{
    self->field_0x1C8 = *(u32*)get_enemy_data(self);
    self->field_0x8C8 = fn_801260BC_c1(self);
    self->field_0x8CC = fn_801260E0_c1(self);
    fn_8011E5EC(self);
    fn_80127D94(self);
    fn_80132064(self);
    fn_8011E960(self);
    fn_8013A978(self);
}

/* Hands the record's `field_0x00E` to `em_act_advance`. */
extern "C" void fn_80137C94(_ENEMY_WORK* self)
{
    em_act_advance(self, self->field_0x00E);
}

/* The action callback `fn_8013791C` runs each frame: the `fn_80139024` step, or the caller's own
 * callback while `em_act_ck` has not armed the action. */
extern "C" s32 fn_80137C9C(_ENEMY_WORK* self, void (*callback)(_ENEMY_WORK*))
{
    if (self->field_0x1E9 == 0) {
    if (em_act_ck(self, 12, 255) == 1) {
        fn_80139024(self);
    } else {
        callback(self);
    }
    if (self->field_0x011 == 2) {
        if ((self->field_0x1C8 & 8) != 0) {
            em_status_set(self, 0);
            fn_80130844(self);
            fn_80130B28(self);
            fn_801322B4(self, 247);
            self->field_0x916 = 0;
            self->field_0x38E = 0;
            self->field_0x38F = 0;
            self->field_0x94C = 0;
            self->field_0x812 = 0;
            self->field_0x814 = 0;
            self->field_0x94A = 0;
            self->field_0x011 = 0;
            self->field_0x1E7 = 1;
            self->field_0x018 = fn_80126844(self);
            fn_80128A30(self, 12, 255);
            em_state_refresh(self);
            return 1;
        }
        return 0;
    }
    return (u8)(self->state_0x017 + 253) > 1;
    }
    fn_8012820C(self);
    return 1;
}

/* Wraps the record's two angle fields back into their 16-bit range. */
extern "C" void fn_80137DD0(_ENEMY_WORK* self)
{
    if ((u32)(self->field_0x1E2 - 1) <= 3) {
    if (fn_80133C48() == 0) {
        u16 angle = (u16)self->field_0x1BC;

        if (angle < 32768) {
            if (angle < 256) {
                self->field_0x1BC = 0;
            } else {
                self->field_0x1BC -= 256;
            }
        } else if (angle > 65280) {
            self->field_0x1BC = 0;
        } else {
            self->field_0x1BC += 256;
        }
    }
    if (self->field_0x784 == 0) {
        if (self->field_0x1E2 != 1 || self->field_0x1E3 != 1) {
            u16 angle = (u16)self->field_0x1C4;

            if (angle < 32768) {
                if (angle < 256) {
                    self->field_0x1C4 = 0;
                } else {
                    self->field_0x1C4 -= 256;
                }
            } else if (angle > 65280) {
                self->field_0x1C4 = 0;
            } else {
                self->field_0x1C4 += 256;
            }
        }
    }
    } else {
        self->field_0x1BC = 0;
        self->field_0x1C4 = 0;
    }
}

/* The average of the angles `calcVecAngXY` derives from the record's motion slots. */
extern "C" s32 fn_80137EE0(_ENEMY_WORK* self, u32 kind)
{
    u32 angles[10];

    if (self->field_0x218 == 0) {
        return -1;
    }
    {
        u32 kind_lo = (u8)kind;
        u32 count = 0;
        s32 i;

        for (i = 0; i < 10; i++) {
            if (self->slots_0x244[i].flags == 0) {
                continue;
            }
            if (kind_lo == 1) {
                if ((u32)(self->slots_0x244[i].value - 24576) > 16384) {
                    continue;
                }
            }
            if ((self->slots_0x244[i].flags & 0x800) == 0) {
                continue;
            }
            {
                u32 angle_x;
                u32 angle_y;

                calcVecAngXY(&self->slots_0x244[i].vec, &angle_x, &angle_y);
                angles[count] = angle_y;
                count++;
            }
        }
        if (count == 0) {
            return -1;
        }
        if (count == 1) {
            return angles[0] & 0xFFFF;
        }
        {
            s32 sum = angles[0];
            s8 n;

            for (n = 1; (u32)n < count; n++) {
                s32 delta = angles[n] - sum;

                if (delta > 32768) {
                    delta -= 65536;
                } else if (delta < -32768) {
                    delta += 65536;
                }
                sum += delta / (n + 1);
            }
            return sum & 0xFFFF;
        }
    }
}

/* Steps the record's second angle field through `fn_80133DB0`. */
extern "C" void fn_80138024(_ENEMY_WORK* self, u32 a, u32 b)
{
    self->field_0x1C0 = fn_80133DB0((u16)(b + 0x8000), (u16)self->field_0x1C0, (u16)a);
}
