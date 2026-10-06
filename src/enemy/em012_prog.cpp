/*
 * enemy/em012_prog.cpp - enemy 012's program: the per-action `_ENEMY_WORK` step machines (arm through
 *   `em_move_mode_set`/`em_mot_set`, wait on `em_mot_end_ck`/`em_frame_check`, finish with `em_action_finish`),
 *   the `state_sub` and `action` dispatchers, the timer and slot setups and the static initializer.
 * RANGE. .text 0x80170600-0x80176C30 (67 functions); extab 0x8000E52C-0x8000E6D4, extabindex
 *   0x800296AC-0x80029928, .ctors 0x8056F334-0x8056F338, .rodata 0x8056FD10-0x8056FDD0, .data 0x805A8370-0x805A94A0
 *   (`em012_prog_tbl` first), .bss 0x806A79A0-0x806A79D0, .sdata 0x80791A58-0x80791A60, .sdata2
 *   0x80797910-0x80797B10.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_80170600` to `fn_80170EF4` and from `fn_80173264` to the
 *   end, on for `fn_80170FA8`..`fn_80171130`.
 * NAMES. The file name follows the runtime dump's `em012_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions (the dump's `l2cu_release_rcb` in the range is an ambiguous join, not
 *   evidence).
 *   The `.bss` record names (`vec_pair_80171194_*`) are GUESSes.
 * RESIDUALS. 37 rows unwritten: 0x80171194-0x80173264, 0x801732B0-0x801740A0, 0x8017413C-0x80176328,
 *   0x801763D8-0x80176C30.
 *   1 partial row, `fn_801740F4`: retail bounds the jump table with `cmplwi r0,13` where the 8 written cases give
 *   `cmplwi r0,7` (cases 8..13 are the default); MWCC bounds by the switch type's range, and the 14 state ids
 *   that would name that range are unknown.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata` claimed, not emitted; `.data` 0x5C against 0x1130, `.sdata2` 0x18
 *   against 0x200; `.text` (0xD2C of 0x6630), extab (0xA8 of 0x1A8) and extabindex (0xFC of 0x27C) short of the
 *   claim and differing.
 */

#include "enemy/em_act_arm_unless_down.h" /* fn_80128A8C (rule 2: the owner's header) */
#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80171194.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "ef/eft_slot.h"     /* enemy_data_find / enemy_data_grp (their owner's header) */
#include "ef.h"
#include "enemy/EnemyData.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_80128A8C_c1 ((void (*)(_ENEMY_WORK *, u8, u8))em_act_arm_unless_down)

extern "C" {
/* The record `enemy_data_find` looks up; only the flag byte at +0x08 is read here.
 * size: 0x09 (approximate - only +0x08 is observed) */
typedef struct ENEMY_ENTRY {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ u8 field_0x08;
} ENEMY_ENTRY;

/* ---- foreign callees the owner's header does not declare yet ---- */

void fn_801706B8(_ENEMY_WORK *self);
}

/* The one mangled callee, declared by its real signature so the front-end emits the map's
 * `em_frame_check__FP11_ENEMY_WORKUsff` (rule 9). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

/* The unit's `.sdata2` pool entries, declared, not defined. */
extern f32 lbl_80797910; /* 0.0f */
extern f32 lbl_80797928; /* 170.0f */

/* The damage-part query, declared as a C++ free function so the front-end emits the map's
 * `em_parts_damage_level_get__FP11_ENEMY_WORKUc` (rule 9). */
u8 em_parts_damage_level_get(_ENEMY_WORK* enemy, u8 part);

extern "C" {
void fn_80171194(_ENEMY_WORK* self, u8 kind, s32 arg);
void fn_80171744(_ENEMY_WORK* self, s32 arg);
void fn_80171EB8(_ENEMY_WORK* self, s32 arg);
void fn_801721B8(_ENEMY_WORK* self);
void fn_80172E10(_ENEMY_WORK* self);
void fn_80172E98(_ENEMY_WORK* self);
void fn_801731BC(_ENEMY_WORK* self);
void fn_80173264(_ENEMY_WORK* self);
void fn_8017326C(_ENEMY_WORK* self);
void fn_80173278(_ENEMY_WORK* self);
void fn_80173280(_ENEMY_WORK* self);
void fn_801732B0(_ENEMY_WORK* self);
void fn_8017394C(_ENEMY_WORK* self);
void fn_80173A04(_ENEMY_WORK* self);
void fn_80173C60(_ENEMY_WORK* self);
void fn_80173D04(_ENEMY_WORK* self);
void fn_80173FF4(_ENEMY_WORK* self);
void fn_801740A0(_ENEMY_WORK* self);

/* This unit's `action_0x1E5` handlers `fn_80170A00`/`fn_80171130`, and `enemy/em_model.cpp`'s `fn_803B9BA0`,
 * declared with the call sites' widths. */
void fn_80170A00(_ENEMY_WORK* self);
void fn_80171130(_ENEMY_WORK* self);
void fn_803B9BA0(_ENEMY_WORK* self, VEC3* pos, s32 value);
}

#pragma peephole off

extern "C" {
/* `enemy_data_grp`/`enemy_data_find` (0x803439D4 / 0x803438E4) come from their owner's header,
 * `ef/eft_slot.h` (rule 2). */

/* ---- the range's functions ---- */
void fn_80170600(_ENEMY_WORK *self) {
    self->field_0x32A = 0;
    self->field_0x328 = 0;
}

void fn_80170610(_ENEMY_WORK *self, u8 a) {
    Vec3 v;
    u8 b, c;

    VEC3_ctor(&v);
    if ((a & 0xFF) == 2) {
        fn_80176090(self, &b, &c);
        fn_80128A8C_c1(self, b, c);
        em_state_refresh(self);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, 0.0f, 45.0f);
        fn_801057A4(self, 0x14, &v, 0.7f, 0x1C72);
    }
}

void fn_801706B8(_ENEMY_WORK *self) {
    if (self->team == 0xC) {
        em_spawn_request(self->field_0x01A, 0xA, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    } else {
        em_spawn_request(self->field_0x01A, 0xD, 0, self->area_no, self->field_0x46C, 1, 2, 8,
                    0xFF, 0, 0);
    }
}

void fn_80170758(_ENEMY_WORK *self, u8 a, u8 b) {
    if ((a & 0xFF) == 1) {
        switch (b) {
        case 5:
            fn_80130F74(self);
            break;
        case 0xA:
            self->field_0x32A = 0;
            break;
        case 0xC:
            if (em_busy_ck(self) == 1) {
                fn_801706B8(self);
            }
            break;
        case 0xD:
            if (em_busy_ck(self) == 1) {
                fn_801706B8(self);
            }
            break;
        case 0xE:
            fn_801376B4(self);
            break;
        }
    }
}

void fn_80170804(_ENEMY_WORK *self) {
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (em_die_ck(self) == 0) {
        u8 v = (u8)enemy_data_grp(self->team, self->field_0x00A);
        ENEMY_ENTRY *entry = (ENEMY_ENTRY *)enemy_data_find(v, self->field_0x46C);
        if (entry != 0 && entry->field_0x08 == 0xFF) {
            self->field_0x46C = 0xFF;
            fn_8013AAC4(self);
        }
    }
}

void fn_8017088C(_ENEMY_WORK *self) {
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

void fn_80170908(_ENEMY_WORK *self) {
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

void fn_80170984(_ENEMY_WORK *self) {
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

void fn_80170A00(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0:
        fn_8017088C(self);
        break;
    case 1:
        fn_80170908(self);
        break;
    case 2:
        fn_80170984(self);
        break;
    case 3:
        fn_8017088C(self);
        break;
    case 4:
        fn_8017088C(self);
        break;
    case 6:
        fn_8017088C(self);
        break;
    }
}

void fn_80170A54(_ENEMY_WORK *self) {
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

void fn_80170AD0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170B4C(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2: {
        f32 t = 0.4f;
        fn_8013221C(self, t, 1, 0xF);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x71, 4, 0);
            fn_80132264(self);
        }
    }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170C68(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        self->timer_0x020 = 0x84;
        break;
    case 1:
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        } else {
            self->state++;
            em_state_set(self, 1, 5);
        }
        break;
    }
}

void fn_80170D04(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set(self, 0xCA, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170D74(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170DF0(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 138.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170E78(_ENEMY_WORK *self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x71, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80170EF4(_ENEMY_WORK *self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC9, 6, 0);
        break;
    case 1:
        if ((u8)a == 1) {
            if (em_frame_check(self, 1, 110.0f, 0.0f) == 1) {
                em_state_set(self, 1, 0xC);
            }
        } else {
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
            }
        }
        break;
    }
}
}

#pragma peephole on

/* state 0: advance and post action 0xC9.  state 1: wait for the frame check, then fire action 0xD. */
extern "C" void fn_80170FA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80797928, lbl_80797910) == 1U) {
            em_state_set(self, 1, 0xD);
        }
        break;
    }
}

/* state 0: advance and post action 0xC9.  state 1: advance the move when the frame check passes. */
extern "C" void fn_80171038(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* state 0: advance and post action 0xE.  state 1: advance the move when the frame check passes. */
extern "C" void fn_801710B4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = (u8)(self->state + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* The action-code dispatcher: `state_sub` 0..14, codes 0 and 9 return without a handler. */
extern "C" void fn_80171130(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1: fn_80170A54(self); break;
    case 2: fn_80170AD0(self); break;
    case 3: fn_80170B4C(self); break;
    case 4: fn_80170C68(self); break;
    case 5: fn_80170D04(self); break;
    case 6: fn_80170D74(self); break;
    case 7: fn_80170DF0(self); break;
    case 8: fn_80170E78(self); break;
    case 10: fn_80170EF4(self, 1); break;
    case 11: fn_80170EF4(self, 0); break;
    case 12: fn_80170FA8(self); break;
    case 13: fn_80171038(self); break;
    case 14: fn_801710B4(self); break;
    case 0:
    case 9: break;
    }
}

#pragma peephole off

extern "C" {
/* The `state_sub` and `action_0x1E5` dispatchers; `fn_801740F4` is the outer one, and its entries 0 and 1
 * are `fn_80170A00`/`fn_80171130`. */
void fn_80173264(_ENEMY_WORK* self) {
    fn_80171744(self, 2);
}

void fn_8017326C(_ENEMY_WORK* self) {
    fn_80171194(self, 10, 1);
}

void fn_80173278(_ENEMY_WORK* self) {
    fn_80171EB8(self, 0);
}

void fn_80173280(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80173264(self);
        break;
    case 1:
        fn_8017326C(self);
        break;
    case 2:
        fn_80173278(self);
        break;
    }
}

void fn_801740A0(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801732B0(self);
        break;
    case 1:
        fn_8017394C(self);
        break;
    case 2:
        fn_80173A04(self);
        break;
    case 3:
        fn_80173C60(self);
        break;
    case 4:
        fn_80173D04(self);
        break;
    case 5:
        fn_80173FF4(self);
        break;
    }
}

void fn_801740F4(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80170A00(self);
        break;
    case 1:
        fn_80171130(self);
        break;
    case 2:
        fn_801721B8(self);
        break;
    case 3:
        fn_80172E10(self);
        break;
    case 4:
        fn_80172E98(self);
        break;
    case 5:
        fn_801731BC(self);
        break;
    case 6:
        fn_80173280(self);
        break;
    case 7:
        fn_801740A0(self);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
        break;
    }
}

/* The damage-part gates: both read `team` and `pos` and ask `em_parts_damage_level_get` about a part. */
int fn_80176328(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 1 && ((u8)em_parts_damage_level_get(self, 1) & 1) == 0) {
        return 1;
    }
    return 0;
}

void fn_80176374(_ENEMY_WORK* self, u32 arg) {
    if ((u8)arg == 0 && self->team == 14 && em_parts_damage_level_get(self, 0) == 1) {
        fn_803B9BA0(self, &self->pos, 100);
    }
}
}

/* The unit's `.bss`: the two two-vector records `fn_80176B7C` seeds.  Names are GUESSes (each record is a
 * pair of model-space points). */
VEC3 vec_pair_80171194_0[2];  /* +0x806A79A0 */
VEC3 vec_pair_80171194_1[2];  /* +0x806A79B8 */
