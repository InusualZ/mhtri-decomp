/*
 * enemy/em018_prog.cpp - enemy 018's program: the aim-record flag accessors and setup over the `EmActWork` view,
 *   the `action` dispatcher `fn_80199B24` and its six `state_sub` dispatchers (`fn_80192F24`, `fn_80193394`,
 *   `fn_801938D8`, `fn_801953BC`, `fn_80196618`, `fn_801987E4`), the per-motion state machines, and the static
 *   initializer.
 * RANGE. .text 0x80192348-0x8019E670 (128 functions); extab 0x8000EDC4-0x8000F124, extabindex
 *   0x8002A390-0x8002A8A0, .ctors 0x8056F340-0x8056F344 (`fn_8019E604`), .rodata 0x80570050-0x80570150, .data
 *   0x805AD370-0x805AE750 (`em018_prog_tbl` first), .bss 0x806A7A70-0x806A7A88, .sdata 0x80791A78-0x80791A80,
 *   .sdata2 0x80798238-0x80798518.  Left edge: the function after `fn_80192204`, `enemy/em016_prog.cpp`'s static
 *   initializer; right edge: `fn_8019E604` (the two vectors at `lbl_806A7A70`) is this TU's static initializer,
 *   and `fn_8019E670` is the first reader of the next 0.0 pool entry `lbl_80798538`.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_80192348` to `fn_80192630`, on from `fn_80192C10`.
 * NAMES. The file name follows the runtime dump's `em018_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.
 *   The `.bss` record names (`vec_pair_801926EC_0`) are GUESSes.
 * RESIDUALS. 46 rows unwritten: 0x801926EC-0x801928B8, 0x801928BC-0x80192C10, 0x801936A8-0x801938D8,
 *   0x80193AEC-0x80193EF0, 0x80194078-0x80194318, 0x801943A0-0x8019485C, 0x80194AE4-0x80194CC8,
 *   0x80194D6C-0x80195098, 0x80195214-0x8019533C, 0x80195504-0x80195638, 0x80195720-0x80195BCC,
 *   0x80195CA4-0x80195ED4, 0x80196278-0x80196618, 0x8019687C-0x80196A50, 0x80196BF4-0x80197BC0,
 *   0x80197D50-0x801986AC, 0x80198910-0x80198F10, 0x801990E0-0x801993E0, 0x801994F4-0x80199A2C,
 *   0x80199BE0-0x8019D8B8, 0x8019DBDC-0x8019DE90, 0x8019DE98-0x8019E398, 0x8019E604-0x8019E670.
 *   16 partial rows, including:
 *  - `fn_8019D8B8`, `fn_8019D9BC`, `fn_8019DAC0`: retail keeps `clrlwi r0,r3,24` after `stage_map_kind_get`,
 *    which ours folds away (the owner declares a `u32` return);
 *  - `fn_8019E398`, `fn_8019E49C`: register allocation on the `||` blocks and the f30 save of the radius test;
 *  - `fn_801934EC`: retail hoists `mr r3,self` before the height ternary;
 *  - `fn_80194CC8`: the `+0x020` timer loop's `lfs`/`fcmpo` order;
 *  - `fn_80192448`: one callee-saved register coloured differently (the `work`/`i` pair);
 *  - `fn_80193964`, `fn_8019485C`, `fn_8019610C`, `fn_801961C8`, `fn_80195BCC`, `fn_80196A50`: single-row register
 *    colouring or `mr r3` placement around the `fn_8012F9*` wait.
 *   The other 2 partial rows have no recorded cause.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata` claimed, not emitted; `.data` 0x248 against 0x13E0, `.sdata2` 0x4
 *   against 0x2E0; `.text` (0x2FF8 of 0xC328), extab (0x200 of 0x360) and extabindex (0x300 of 0x510) short of the
 *   claim and differing.
 * SHAPES. `u8 >= 2` in `fn_8019238C` compiles to the borrow sequence (`li r3,2; orc; addi; srwi; subf; srwi`):
 *   the source keeps the comparison.  `fn_80192448` declares its locals `max`, `i`, `work`, which gives retail's
 *   r31/r30/r29.
 */

#include "enemy/lbl_80797E88.h" /* lbl_80797E88 (rule 2: the owner's header) */
#include "enemy/lbl_8079800C.h" /* lbl_8079800C (rule 2: the owner's header) */
#include "ef/mtx34_trans_add.h" /* mtx34_trans_add (rule 2: the owner's header) */
#include "ef/mtx34_trans_get.h" /* mtx34_trans_get (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "enemy/em016_prog_types.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/fn_801251D0.h" /* fn_801280F4, fn_80128030 */
#include "enemy/fn_8012BDF4.h" /* em_busy_set */
#include "enemy/fn_8012EC74.h" /* em_camera_req */
#include "enemy/fn_80138074.h"  /* em_res_user_data_set, em_res_user_data_ck, fn_8013A654, fn_8013918C */
#include "enemy/fn_80191598.h" /* fn_80192370, fn_80192618 */
#include "fn_8004CAD8.h"       /* calcDistanceSqXZ */
#include "unsplit/enemy.h"     /* the plain enemy-band callees (rule 2 band header) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define mtx34_trans_add_c1 ((void (*)(MTX34*, VEC3*))mtx34_trans_add)
#define mtx34_trans_get_c1 ((void (*)(MTX34*, VEC3*))mtx34_trans_get)

/* ------------------------------------------------------------------------------------------------ */
/* the unit's pool, declared, not defined                                                            */
/* ------------------------------------------------------------------------------------------------ */

extern const f32 lbl_80798238;

extern const f32 lbl_80798244;
extern const f32 lbl_80798248;
extern const f32 lbl_8079824C;
extern const f32 lbl_80798250;
extern const f32 lbl_80798254;

/* ------------------------------------------------------------------------------------------------ */
/* callees                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* The mangled callees, outside `extern "C"` so the front-end mangles them the way the map spells them
 * (rule 9).  Their parameter widths are the ones the map's manglings encode. */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* work, u8 part);
u16 get_move_work_max(u8 kind);
void* get_move_work_adrs(u8 kind);
f32 get_em_scale(struct _ENEMY_WORK* work);
void get_joint_wmat_em(struct _ENEMY_WORK* work, u32 joint, MTX34* mtx);
void senko_set(VEC3* pos, f32 value, u8 a, s16 b);

extern "C" {

void fn_80192348(EmActWork* self, u8 mask);
void fn_80192358(EmActWork* self, u8 mask);
u32 fn_8019238C(EmActWork* self);
u32 fn_801923CC(EmActWork* self);
u32 fn_801923E0(EmActWork* self);
u32 fn_80192410(EmActWork* self);
f32 fn_80192440(EmActWork* self);
s32 fn_80192448(EmActWork* self);
void fn_8019255C(EmActWork* self);
void fn_80192630(EmActWork* self);
}

/* The unit's `.sdata2` and `.data` labels, declared, not defined: the source does not emit them yet. */
extern f32 lbl_80798264;
extern f32 lbl_80798268;
extern f32 lbl_8079826C;
extern f32 lbl_80798270;
extern f32 lbl_80798260;
extern f32 lbl_8079827C;
extern f32 lbl_80798288;
extern f32 lbl_80798290;
extern f32 lbl_80798298;

extern f32 lbl_80798308;
extern f32 lbl_8079830C;
extern f32 lbl_80798310;
extern f32 lbl_80798314;
extern f32 lbl_80798328;
extern f32 lbl_8079832C;
extern f32 lbl_80798338;
extern f32 lbl_80798370;
extern f32 lbl_80798398;
extern f32 lbl_8079839C;
extern f32 lbl_807983A0;
extern f32 lbl_807982E0;
extern f32 lbl_807983B0;
extern f32 lbl_807983F0;
extern f32 lbl_807983F4;
extern f32 lbl_807983F8;
extern f32 lbl_80798510;
/* The `em_turn_seq_start`/`em_turn_seq_step` descriptor blocks this band arms (`.data` tables). */
extern u8 lbl_80570050[];
extern u8 lbl_80570090[];
extern u8 lbl_805700D0[];
extern u8 lbl_80570110[];

extern "C" {
/* 0x802B0668 - the map-id lookup: a byte table, `0xFF` meaning "no entry" (the argument comes back). */
u32 stage_map_kind_get(u32 kind);

/* This unit's own functions, declared so the dispatchers can tail-call them. */
void fn_801993E0(struct _ENEMY_WORK* self);
void fn_80199468(struct _ENEMY_WORK* self);
void fn_801994F4(struct _ENEMY_WORK* self);
void fn_80199A2C(struct _ENEMY_WORK* self);
void fn_80199ADC(struct _ENEMY_WORK* self);
void fn_8019E398(struct _ENEMY_WORK* self);

void fn_801928B8(void);

void fn_80192C10(struct _ENEMY_WORK* self);
void fn_80192C8C(struct _ENEMY_WORK* self);
void fn_80192D08(struct _ENEMY_WORK* self);
void fn_80192D84(struct _ENEMY_WORK* self);
void fn_80192E04(struct _ENEMY_WORK* self);
void fn_80192E84(struct _ENEMY_WORK* self);
void fn_80192F24(struct _ENEMY_WORK* self);
void fn_80192F78(struct _ENEMY_WORK* self);
void fn_80192FFC(struct _ENEMY_WORK* self);
void fn_80193078(struct _ENEMY_WORK* self);
void fn_801930F4(struct _ENEMY_WORK* self);
void fn_801931A4(struct _ENEMY_WORK* self);
void fn_80193220(struct _ENEMY_WORK* self);
void fn_8019329C(struct _ENEMY_WORK* self);
void fn_80193318(struct _ENEMY_WORK* self);
void fn_80193394(struct _ENEMY_WORK* self);
void fn_801933DC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801934EC(struct _ENEMY_WORK* self, u32 a);
void fn_801935A0(struct _ENEMY_WORK* self);
void fn_8019362C(struct _ENEMY_WORK* self);
void fn_801936A8(struct _ENEMY_WORK* self);
void fn_801938D8(struct _ENEMY_WORK* self);
void fn_80193964(struct _ENEMY_WORK* self, u8 a);
void fn_80193A34(struct _ENEMY_WORK* self);
void fn_80193AEC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80193CC8(struct _ENEMY_WORK* self);
void fn_80193DE8(struct _ENEMY_WORK* self, u32 a);
void fn_80193EF0(struct _ENEMY_WORK* self);
void fn_80193F70(struct _ENEMY_WORK* self);
void fn_80193FF8(struct _ENEMY_WORK* self);
void fn_80194078(struct _ENEMY_WORK* self, u32 a);
void fn_80194318(struct _ENEMY_WORK* self);
void fn_801943A0(struct _ENEMY_WORK* self, u32 a);
void fn_80194584(struct _ENEMY_WORK* self);
void fn_801946E8(struct _ENEMY_WORK* self, u32 a);
void fn_8019485C(struct _ENEMY_WORK* self, u8 a);
void fn_80194938(struct _ENEMY_WORK* self);
void fn_801949B4(struct _ENEMY_WORK* self);
void fn_80194A64(struct _ENEMY_WORK* self);
void fn_80194AE4(struct _ENEMY_WORK* self);
void fn_80194CC8(struct _ENEMY_WORK* self);
void fn_80194D6C(struct _ENEMY_WORK* self);
void fn_80194FA0(struct _ENEMY_WORK* self);
void fn_80195098(struct _ENEMY_WORK* self);
void fn_8019515C(struct _ENEMY_WORK* self);
void fn_80195214(struct _ENEMY_WORK* self);
void fn_8019533C(struct _ENEMY_WORK* self);
void fn_801953BC(struct _ENEMY_WORK* self);
void fn_80195504(struct _ENEMY_WORK* self, u32 a);
void fn_80195638(struct _ENEMY_WORK* self, u32 a);
void fn_80195720(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80195860(struct _ENEMY_WORK* self);
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a);
void fn_80195CA4(struct _ENEMY_WORK* self);
void fn_80195D80(struct _ENEMY_WORK* self);
void fn_80195ED4(struct _ENEMY_WORK* self);
void fn_80196004(struct _ENEMY_WORK* self);
void fn_8019610C(struct _ENEMY_WORK* self, u8 a);
void fn_801961C8(struct _ENEMY_WORK* self, u8 a);
void fn_80196278(struct _ENEMY_WORK* self);
void fn_801963A8(struct _ENEMY_WORK* self);
void fn_80196618(struct _ENEMY_WORK* self);
void fn_801966DC(struct _ENEMY_WORK* self);
void fn_80196768(struct _ENEMY_WORK* self);
void fn_8019687C(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80196A50(struct _ENEMY_WORK* self, u8 a);
void fn_80196B1C(struct _ENEMY_WORK* self);
void fn_80196BF4(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80196E10(struct _ENEMY_WORK* self, u32 a);
void fn_80197294(struct _ENEMY_WORK* self);
void fn_80197404(struct _ENEMY_WORK* self);
void fn_80197570(struct _ENEMY_WORK* self, u32 a);
void fn_801976B8(struct _ENEMY_WORK* self);
void fn_80197BC0(struct _ENEMY_WORK* self);
void fn_80197C84(struct _ENEMY_WORK* self);
void fn_80197D50(struct _ENEMY_WORK* self);
void fn_80197F04(struct _ENEMY_WORK* self);
void fn_80198150(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_801986AC(struct _ENEMY_WORK* self);
void fn_801987E4(struct _ENEMY_WORK* self);
void fn_80198F10(struct _ENEMY_WORK* self);
void fn_80198F14(struct _ENEMY_WORK* self);
}

#pragma peephole off

extern "C" {
/* Sets the aim bits the caller's mask names. */
void fn_80192348(EmActWork* self, u8 mask) {
    self->aim_0x328.flags_0x35 |= mask;
}

/* Clears the aim bits the caller's mask names. */
void fn_80192358(EmActWork* self, u8 mask) {
    self->aim_0x328.flags_0x35 &= ~mask;
}

/* Reports whether every aim bit of the caller's mask is set (narrowed in the source: retail masks the
 * incoming register, `clrlwi r0,r4,24`). */
u32 fn_80192370(struct _ENEMY_WORK* self_, u32 mask) {
    EmActWork* self = (EmActWork*)self_;
    return (self->aim_0x328.flags_0x35 & (u8)mask) != 0;
}

/* Reports whether the damage part's level is in the aim group's admitted range (`u8 >= 2`). */
u32 fn_8019238C(EmActWork* self) {
    u8 v = em_parts_damage_level_get((struct _ENEMY_WORK*)self, 1);

    return v >= 2;
}

/* Reports whether the aim record's third flag byte is set. */
u32 fn_801923CC(EmActWork* self) {
    return self->aim_0x328.flag_0x33 != 0;
}

/* Reports whether the aim record's bit 0x02 is set. */
u32 fn_801923E0(EmActWork* self) {
    return fn_80192370((struct _ENEMY_WORK*)self, 2) == 1;
}

/* Reports whether the aim record's bit 0x08 is set. */
u32 fn_80192410(EmActWork* self) {
    return fn_80192370((struct _ENEMY_WORK*)self, 8) == 1;
}

/* Reads the aim vector's z. */
f32 fn_80192440(EmActWork* self) {
    return self->aim_0x328.vec_0x1C.z;
}

/* Reports whether another live work of team 0x12 already runs one of the four aim actions. */
s32 fn_80192448(EmActWork* self) {
    u16 max = get_move_work_max(3);
    u8 i;
    EmActWork* work = (EmActWork*)get_move_work_adrs(3);

    for (i = 0; (u32)i < max; i++, work++) {
        if (work->active == 0) {
            continue;
        }
        if (self == work) {
            continue;
        }
        if (work->team != 0x12) {
            continue;
        }
        if (self->act_id != work->act_id) {
            continue;
        }
        if ((u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x1F) == 1 ||
            (u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x20) == 1 ||
            (u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x21) == 1 ||
            (u32)em_act_ck((struct _ENEMY_WORK*)work, 5, 0x22) == 1) {
            return 0;
        }
    }
    return 1;
}

/* Fires the aim effect at the work's 0x1A joint, scaled by the work's own scale. */
void fn_8019255C(EmActWork* self) {
    VEC3 out;
    VEC3 ofs;
    MTX34 mtx;

    VEC3_ctor(&out);
    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    em_hit_window_set((struct _ENEMY_WORK*)self, 0, 0x12, 0x205);
    setVector3(&ofs, lbl_80797E88, lbl_80797E88, lbl_8079800C);
    get_joint_wmat_em((struct _ENEMY_WORK*)self, 0x1A, &mtx);
    mulVecMat(&ofs, &mtx);
    mtx34_trans_add_c1(&mtx, &ofs);
    mtx34_trans_get_c1(&mtx, &out);
    senko_set(&out, lbl_80798238 * get_em_scale((struct _ENEMY_WORK*)self), self->act_id, 0);
}

/* Arms the two aim values and sets the aim record's bit 0x01. */
void fn_80192618(struct _ENEMY_WORK* self_) {
    EmActWork* self = (EmActWork*)self_;
    self->aim_0x328.value_0x28 = lbl_80798244;
    self->aim_0x328.value_0x2C = lbl_80798248;
    fn_80192348(self, 1);
}

/* Initialises the aim record. */
void fn_80192630(EmActWork* self) {
    EmAimRec* rec = &self->aim_0x328;
    VEC3 v1;
    VEC3 v2;

    /* The aim record's +0x00 is read as a scalar, but retail copies a whole 3-float record over it. */
    copyVec3((nw4r::math::VEC3*)&rec->angle_0x00,
             setVec3(&v1, lbl_8079824C, lbl_8079824C, lbl_8079824C));
    copyVec3((nw4r::math::VEC3*)&rec->rot_0x10,
             setVec3(&v2, lbl_8079824C, lbl_8079824C, lbl_8079824C));
    rec->vec_0x1C.x = lbl_80798238;
    rec->vec_0x1C.y = lbl_80798250;
    rec->vec_0x1C.z = lbl_80798254;
    rec->value_0x28 = lbl_8079824C;
    rec->value_0x2C = lbl_8079824C;
    rec->flag_0x30 = 0;
    rec->flag_0x31 = 0;
    rec->flag_0x32 = 0;
    rec->flag_0x33 = 0;
    rec->flag_0x34 = 0;
    rec->rot_0x04.z = (u32)-1;
    rec->flags_0x35 = 0;
    rec->value_0x36 = 0;
    rec->value_0x38 = 0;
    rec->value_0x3A = 0;
}
}

#pragma peephole on

extern "C" {
/* ---------------------------------------------------------------------------------------------- */
/* the per-motion step group dispatched by fn_80192F24 (states 0,1,2,4,5,7)                         */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80192C10 - arms motion 1 for 4 frames; state 1 waits and re-enters the motion. */
void fn_80192C10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192C8C - the same shape with motion 20. */
void fn_80192C8C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192D08 - the same shape with motion 29. */
void fn_80192D08(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 29, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80192D84 - arms motion mode 2 with the four-argument setter (40/20/0/3) and the
 * `fn_80128030` completion. */
void fn_80192D84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 40, 20, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80192E04 - the same shape with (54/20/0/3). */
void fn_80192E04(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 54, 20, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80192E84 - the flag-arming step: sets the two action-start flags, arms mode 4 with motion 0xCE
 * and the fresh-scale pair, then waits on `fn_801280F4`. */
void fn_80192E84(struct _ENEMY_WORK* self) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        fn_80132198(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x80192F24 - the action-0 dispatcher: `state_sub` selects this group's six steps. */
void fn_80192F24(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80192C10(self);
        break;
    case 1:
        fn_80192C8C(self);
        break;
    case 2:
        fn_80192D08(self);
        break;
    case 4:
        fn_80192D84(self);
        break;
    case 5:
        fn_80192E04(self);
        break;
    case 7:
        fn_80192E84(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the step group dispatched by fn_80193394 (states 0..7)                                           */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80192F78 - arms motion 6 for 10 frames; state 1 runs `em_state_set(self, 1, 5)`. */
void fn_80192F78(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 5);
        }
        break;
    }
}

/* 0x80192FFC - arms motion 7 for 4 frames. */
void fn_80192FFC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193078 - arms motion 0x1A for 4 frames. */
void fn_80193078(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801930F4 - arms motion 0x1E for 4 frames; state 1 fires the joint effect when `em_frame_check`
 * reports the frame and then waits on `em_mot_end_ck`. */
void fn_801930F4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1E, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            em_camera_req(self, -1, 7);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801931A4 - arms motion 0x72 for 10 frames. */
void fn_801931A4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x72, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193220 - arms motion 8 for 4 frames. */
void fn_80193220(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
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

/* 0x8019329C - arms motion 20 for 4 frames. */
void fn_8019329C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193318 - arms motion 0x1D for 4 frames. */
void fn_80193318(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1D, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193394 - the action-1 dispatcher (states 0..7). */
void fn_80193394(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80192F78(self);
        break;
    case 1:
        fn_80192FFC(self);
        break;
    case 2:
        fn_80193078(self);
        break;
    case 3:
        fn_801930F4(self);
        break;
    case 4:
        fn_801931A4(self);
        break;
    case 5:
        fn_80193220(self);
        break;
    case 6:
        fn_8019329C(self);
        break;
    case 7:
        fn_80193318(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the step group dispatched by fn_801938D8 (states 0..10)                                          */
/* ---------------------------------------------------------------------------------------------- */

/* 0x801933DC - arms motion `a == 1 ? 9 : 2` and picks the height by `b`; `default:` first (playbook 37),
 * selectors narrowed at use and the motion to `u16` (retail's `clrlwi` pair). */
void fn_801933DC(struct _ENEMY_WORK* self, u32 a, u32 b) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        {
            u16 motion = (u8)a == 1 ? 9 : 2;
            em_mot_set(self, motion, 4, 0);
        }
        switch ((u8)b) {
        default:
            em_approach_start(self, lbl_80798268, 0);
            break;
        case 1:
            em_approach_start(self, lbl_80798238, 0);
            if (self->value_0x378 > lbl_8079826C) {
                self->value_0x378 = lbl_8079826C;
            }
            break;
        case 2:
            em_approach_start(self, lbl_80798270, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801934EC - arms motion 21 and picks the height by `a`. */
void fn_801934EC(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 21, 4, 0);
        {
            f32 height = (u8)a == 1 ? lbl_80798270 : lbl_80798268;
            em_approach_start(self, height, 0);
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801935A0 - arms the `lbl_80570050` descriptor and waits on `em_turn_seq_step`. */
void fn_801935A0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_80570050, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570050) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8019362C - arms motion 0x1B for 4 frames. */
void fn_8019362C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801938D8 - the action-2 dispatcher (states 0..10). */
void fn_801938D8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801933DC(self, 0, 0);
        break;
    case 1:
        fn_801934EC(self, 0);
        break;
    case 2:
        fn_801935A0(self);
        break;
    case 3:
        fn_8019362C(self);
        break;
    case 4:
        fn_801933DC(self, 1, 0);
        break;
    case 5:
        fn_801936A8(self);
        break;
    case 6:
        fn_801933DC(self, 0, 1);
        break;
    case 7:
        fn_801933DC(self, 1, 1);
        break;
    case 8:
        fn_801934EC(self, 1);
        break;
    case 9:
        fn_801933DC(self, 0, 2);
        break;
    case 10:
        fn_801933DC(self, 1, 2);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the large per-action dispatchers                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* 0x801953BC - the action-5 dispatcher (states 0..39). */
void fn_801953BC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80193964(self, 0);
        break;
    case 1:
        fn_80193A34(self);
        break;
    case 2:
        fn_80193AEC(self, 0, 0);
        break;
    case 3:
        fn_80193AEC(self, 1, 0);
        break;
    case 4:
        fn_80193AEC(self, 2, 0);
        break;
    case 5:
        fn_80193AEC(self, 0, 1);
        break;
    case 6:
        fn_80193AEC(self, 1, 1);
        break;
    case 7:
        fn_80193AEC(self, 2, 1);
        break;
    case 8:
        fn_80193CC8(self);
        break;
    case 9:
        fn_80193DE8(self, 0);
        break;
    case 10:
        fn_80193EF0(self);
        break;
    case 11:
        fn_80193F70(self);
        break;
    case 12:
        fn_80193FF8(self);
        break;
    case 13:
        fn_80194078(self, 0);
        break;
    case 14:
        fn_80194318(self);
        break;
    case 15:
        fn_801943A0(self, 0);
        break;
    case 16:
        fn_80194584(self);
        break;
    case 17:
        fn_801946E8(self, 0);
        break;
    case 18:
        fn_8019485C(self, 0);
        break;
    case 19:
        fn_80194938(self);
        break;
    case 20:
        fn_801949B4(self);
        break;
    case 21:
        fn_80193DE8(self, 1);
        break;
    case 22:
        fn_80193AEC(self, 3, 0);
        break;
    case 23:
        fn_80193AEC(self, 3, 1);
        break;
    case 24:
        fn_80193964(self, 1);
        break;
    case 25:
        fn_80193AEC(self, 3, 2);
        break;
    case 26:
        fn_80193AEC(self, 3, 3);
        break;
    case 27:
        fn_801946E8(self, 1);
        break;
    case 28:
        fn_8019485C(self, 1);
        break;
    case 29:
        fn_80194A64(self);
        break;
    case 30:
        fn_80194AE4(self);
        break;
    case 31:
        fn_80194CC8(self);
        break;
    case 32:
        fn_80194D6C(self);
        break;
    case 33:
        fn_80194FA0(self);
        break;
    case 34:
        fn_80195098(self);
        break;
    case 35:
        fn_8019515C(self);
        break;
    case 36:
        fn_80195214(self);
        break;
    case 37:
        fn_8019533C(self);
        break;
    case 38:
        fn_80194078(self, 1);
        break;
    case 39:
        fn_801943A0(self, 1);
        break;
    }
}

/* 0x80196618 - the action-6 dispatcher (states 0..24). */
void fn_80196618(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80195504(self, 0);
        break;
    case 1:
        fn_80195638(self, 0);
        break;
    case 2:
        fn_80195720(self, 0, 2);
        break;
    case 3:
        fn_80195860(self);
        break;
    case 4:
        fn_80195BCC(self, 0);
        break;
    case 5:
        fn_80195CA4(self);
        break;
    case 7:
        fn_80195D80(self);
        break;
    case 10:
        fn_80195ED4(self);
        break;
    case 11:
        fn_80196004(self);
        break;
    case 12:
        fn_8019610C(self, 0);
        break;
    case 13:
        fn_801961C8(self, 0);
        break;
    case 14:
        fn_80196278(self);
        break;
    case 15:
        fn_80195720(self, 1, 0);
        break;
    case 16:
        fn_80195720(self, 0, 1);
        break;
    case 18:
        fn_80195504(self, 1);
        break;
    case 19:
        fn_80195638(self, 1);
        break;
    case 20:
        fn_80195720(self, 2, 0);
        break;
    case 21:
        fn_8019610C(self, 1);
        break;
    case 22:
        fn_801961C8(self, 1);
        break;
    case 23:
        fn_801963A8(self);
        break;
    case 24:
        fn_80195BCC(self, 1);
        break;
    }
}

/* 0x801987E4 - the action-7 dispatcher (states 0..31). */
void fn_801987E4(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_801966DC(self);
        break;
    case 1:
        fn_80196768(self);
        break;
    case 2:
        fn_8019687C(self, 0, 0);
        break;
    case 3:
        fn_8019687C(self, 1, 0);
        break;
    case 4:
        fn_80196A50(self, 0);
        break;
    case 5:
        fn_80196B1C(self);
        break;
    case 6:
        fn_80196BF4(self, 0, 0);
        break;
    case 7:
        fn_80196E10(self, 0);
        break;
    case 8:
        fn_80196E10(self, 1);
        break;
    case 9:
        fn_80197294(self);
        break;
    case 10:
        fn_80196E10(self, 2);
        break;
    case 11:
        fn_80197404(self);
        break;
    case 12:
        fn_80196A50(self, 1);
        break;
    case 13:
        fn_80197570(self, 0);
        break;
    case 14:
        fn_80197570(self, 1);
        break;
    case 15:
        fn_80197570(self, 2);
        break;
    case 16:
        fn_80197570(self, 3);
        break;
    case 17:
        fn_801976B8(self);
        break;
    case 18:
        fn_80197BC0(self);
        break;
    case 19:
        fn_80197C84(self);
        break;
    case 20:
        fn_80196BF4(self, 1, 0);
        break;
    case 21:
        fn_80197D50(self);
        break;
    case 22:
        fn_80197F04(self);
        break;
    case 23:
        fn_80198150(self, 0, 0);
        break;
    case 24:
        fn_801986AC(self);
        break;
    case 25:
        fn_80198150(self, 1, 0);
        break;
    case 26:
        fn_80198150(self, 0, 1);
        break;
    case 27:
        fn_80198150(self, 1, 1);
        break;
    case 28:
        fn_8019687C(self, 0, 1);
        break;
    case 29:
        fn_8019687C(self, 1, 1);
        break;
    case 30:
        fn_80196BF4(self, 0, 1);
        break;
    case 31:
        fn_80196BF4(self, 1, 1);
        break;
    }
}

/* 0x80198F10 - forwards to this band's action-6 state-3 step. */
void fn_80198F10(struct _ENEMY_WORK* self) {
    fn_80195CA4(self);
}

/* 0x80198F14 - runs `fn_80198F10` while the action-8 sub-state is 0. */
void fn_80198F14(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_80198F10(self);
    }
}

/* 0x801928B8 - a bare `return` (the target body is a single `blr`). */
void fn_801928B8(void) {
}

/* ---------------------------------------------------------------------------------------------- */
/* the per-action steps the 0x801953BC/0x80196618/0x801987E4 dispatchers select                 */
/* ---------------------------------------------------------------------------------------------- */

/* 0x80193EF0 - arms mode 2 with (0x32/0x14/0/3) and the `fn_80128030` completion. */
void fn_80193EF0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x32, 0x14, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80193F70 - arms mode 2 with (0x31/0x14/0/1) and the `em_state_set(self, 5, 0x1D)` completion. */
void fn_80193F70(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x1D);
        }
        break;
    }
}

/* 0x80193FF8 - arms mode 2 with (0x3A/0x0A/0/1) and the `fn_80128030` completion. */
void fn_80193FF8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80194938 - arms mode 2 with motion 0x68 for 10 frames; completion `fn_80128030`. */
void fn_80194938(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80194A64 - arms mode 2 with (0x2E/6/0/1); completion `fn_80128030`. */
void fn_80194A64(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 6, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x8019533C - arms mode 2 with (0x36/0x14/0/3); completion `fn_80128030`. */
void fn_8019533C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x14, 0, 3);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x801966DC - arms motion 0x54 for 10 frames with the `em_hit_window_set_default(self, 0, 1)` pair. */
void fn_801966DC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x54, 0xA, 0);
        em_hit_window_set_default(self, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196A50 - arms motion 0x57 for 10 frames and the two joint slots whose ids depend on `a`. */
void fn_80196A50(struct _ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x57, 0xA, 0);
        {
            u32 first, second;
            if (a == 1) {
                first = 0x16;
                second = 0x17;
            } else {
                first = 6;
                second = 7;
            }
            em_hit_window_set(self, 0, first, 8);
            em_hit_window_set(self, 1, second, 0x10);
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80194CC8 - the release step: clears the bit pair, arms motion 0x80 and runs the +0x020 timer
 * down to zero, then completes with `em_state_set(self, 5, 0x20)`. */
void fn_80194CC8(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x80, 4, 0);
        self->timer_0x020 = 0xBE;
        break;
    case 1:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_state_set(self, 5, 0x20);
        }
        break;
    }
}

/* 0x80195BCC - the flag-arming release step: arms motion 0xCD, refreshes the scale, then either
 * the `em_state_set(self, 6, 1)` completion (`a == 1`) or `fn_801280F4`. */
void fn_80195BCC(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCD, 0, 0);
        fn_80132198(self);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (a == 1) {
                em_state_set(self, 6, 1);
            } else {
                fn_801280F4(self);
            }
        }
        break;
    }
}

/* 0x80194318 - arms motion 0x3D with the common `em_action_finish` completion. */
void fn_80194318(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x3D, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80193964 - the requested-motion step: `a == 1` primes the two-state release first; state 1
 * waits on the `lbl_80570090` descriptor and runs `fn_80128030` when it finishes. */
void fn_80193964(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x80193A34 - the same shape for the `lbl_805700D0` descriptor. */
void fn_80193A34(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_805700D0, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_805700D0) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x80198F28 - the same descriptor shape as `fn_80193964` with the
 * `em_state_set(self, 0xD, 1)` completion. */
void fn_80198F28(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570090, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570090) == 1) {
            em_state_set(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x8019515C - the same descriptor shape for `lbl_80570110`. */
void fn_8019515C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_80570110, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_80570110) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80798298);
        } else {
            fn_80136D4C(self, lbl_80798260);
        }
        break;
    }
}

/* 0x801949B4 - arms motion 0x1F for 4 frames, fires the joint effect on the frame and completes with
 * `fn_80128030`. */
void fn_801949B4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x1F, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80798264, lbl_80798238) == 1) {
            self->field_0x358 = 1;
            em_camera_req(self, -1, 7);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80196B1C - arms motion 0x4E with the two joint slots 8/9; while the frame effect has not
 * expired it keeps the two-state release armed. */
void fn_80196B1C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 8, 8);
        em_hit_window_set(self, 1, 9, 0x10);
        fn_80130CDC(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798338, lbl_80798238) == 0) {
            fn_80136D4C(self, lbl_80798260);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80195638 - the flag-armed step: arms motion 0xD0, then holds the two flags while the frame
 * effects run.  The second argument the dispatcher passes is unused. */
void fn_80195638(struct _ENEMY_WORK* self, u32 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD0, 0, 0);
        fn_801303EC(self, lbl_80798238);
        em_shake_req_set(self);
        self->field_0x359 = 1;
        break;
    case 1:
        em_busy_set(self);
        if (em_frame_check(self, 2, lbl_80798290, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 2, lbl_8079827C, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196768 - the three-state step: arm motion 0x59, advance to 0x5A when the frame gate fires,
 * then complete with `em_action_finish`. */
void fn_80196768(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 0xA, 0);
        em_hit_window_set(self, 0, 3, 3);
        em_approach_start(self, lbl_80798328, 0);
        fn_80130CDC(self, -0xC);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0x5A, 2, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079832C, lbl_80798238) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80197C84 - arms motion 0xA for 8 frames and the `lbl_80798270` blend; completes with
 * `fn_80128A70(self, 7, 0)`. */
void fn_80197C84(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xA, 8, 0, 1);
        em_approach_start(self, lbl_80798270, 0x10);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80798370, lbl_80798238) == 1 &&
            (em_approach_step(self, 0, 0x40) == 1 || em_mot_end_ck(self) == 1)) {
            fn_80128A70(self, 7, 0);
        }
        break;
    }
}

/* 0x801986AC - the three-state step: arm motion 0xD6, advance to 0xD3 on completion, then the
 * `lbl_8079839C` joint release and `fn_80128030`. */
void fn_801986AC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 0xD6, 0x14, 0, 1);
        em_hit_window_set(self, 0, 2, 3);
        fn_801303EC(self, lbl_80798238);
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798398, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xD3, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_8079839C, lbl_80798238) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80197BC0 - the two-stage rotation step: state 0 advances the +0x1C0 angle by 0x8000, state 1
 * arms motion 0x7F, state 2 completes with `em_action_finish`. */
void fn_80197BC0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7E, 0, 0);
        self->field_0x1C0 += 0x8000;
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x7F, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801961C8 - the flag-armed step: arms motion 0xCE, refreshes the scale and completes through
 * `fn_801280F4`.  `a` only selects the two-state release at entry. */
void fn_801961C8(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019610C - the same arming as `fn_801961C8` with the `em_turn_to_target(self, 0x300)` completion. */
void fn_8019610C(struct _ENEMY_WORK* self, u8 a) {
    self->field_0x359 = 1;
    self->field_0x35B = 1;
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xCE, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        fn_80132198(self);
        break;
    case 1:
        if (em_turn_to_target(self, 0x300) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8019485C - arms motion 0xCA for 10 frames, advances to 0xDE on completion, then `fn_80128030`. */
void fn_8019485C(struct _ENEMY_WORK* self, u8 a) {
    if (a == 1) {
        em_busy_set(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCA, 0xA, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 2);
            em_mot_set(self, 0xDE, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80195098 - arms motion 0x7F, advances to 0x82 on completion, then `em_action_finish`. */
void fn_80195098(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7F, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x82, 0, 0xBC);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80195ED4 - arms motion 0xDA with the joint slot 0xF; the frame effects re-arm the flag and
 * fire the joint effect, and `em_action_finish` completes the step. */
void fn_80195ED4(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDA, 0, 0);
        em_hit_window_set_default(self, 0, 0xF);
        fn_801303EC(self, lbl_80798238);
        fn_80130CDC(self, -0xC);
        self->field_0x359 = 1;
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798288, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 2, lbl_80798308, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_frame_check(self, 3, lbl_8079830C, lbl_80798288) == 1) {
            em_turn_to_target(self, 0x400);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80196004 - arms motion 0xDB with the joint slot 0x10 and the two frame-effect hooks. */
void fn_80196004(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xDB, 0, 0);
        fn_801303EC(self, lbl_80798238);
        em_hit_window_set_default(self, 0, 0x10);
        fn_80130CDC(self, -0xC);
        self->field_0x359 = 1;
        em_shake_req_set(self);
        break;
    case 1:
        if (em_frame_check(self, 2, lbl_80798310, lbl_80798238) == 1) {
            em_shake_req_set(self);
        }
        if (em_frame_check(self, 2, lbl_80798314, lbl_80798238) == 1) {
            self->field_0x359 = 1;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80198FE8 - arms mode 2 with (0x37/0x14/0/1) and the `lbl_807983A0` blend; state 1 waits on the
 * descriptor and advances to (0x2F/0x28/0/1), completing with `em_state_set(self, 0xD, 2)`. */
void fn_80198FE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        em_approach_start(self, lbl_807983A0, 0x12);
        break;
    case 1:
        fn_80136D4C(self, lbl_807982E0);
        if (em_approach_step(self, 0, 0x80) == 1 && fn_8012F948(self) == 0) {
            self->state++;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 2);
        }
        break;
    }
}

/* 0x801993E0 - the state-advance entry: state 0 arms the motion, state 1 waits for `em_mot_end_ck` and runs
 * `em_state_set(self, 13, 5)`. */
void fn_801993E0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 49, 20, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 13, 5);
        }
        break;
    }
}

/* 0x80199468 - the same shape with this action's own arming pair (mode 46, duration 6, the 1000 ms
 * `fn_80130CDC` timer) and `fn_80128030` as the completion. */
void fn_80199468(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 46, 6, 0, 1);
        fn_80130CDC(self, 1000);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80199A2C - `fn_80199ADC`'s case 7: arm mode 31 with the `em_demo_pos_set`/`em_demo_rot_set` pair and zero
 * the stored height, then wait for `em_mot_end_ck` and run `fn_80128030`. */
void fn_80199A2C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 2);
        em_mot_set(self, 31, 0, 0);
        em_demo_pos_set(self, lbl_807983F0, lbl_807983F4, lbl_807983F8);
        em_demo_rot_set(self, lbl_80798238, lbl_807983B0, lbl_80798238);
        fn_801303EC(self, lbl_80798238);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* 0x80199ADC - the action-id dispatcher: `self->state_sub` (0x1E6) selects this unit's per-action
 * update. */
void fn_80199ADC(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80198F28(self);
        break;
    case 1:
        fn_80198FE8(self);
        break;
    case 2:
        fn_801990E0(self);
        break;
    case 3:
        fn_801991E4(self);
        break;
    case 4:
        fn_801993E0(self);
        break;
    case 5:
        fn_80199468(self);
        break;
    case 6:
        fn_801994F4(self);
        break;
    case 7:
        fn_80199A2C(self);
        break;
    }
}

/* 0x80199B24 - the `action` (0x1E5) dispatcher, plus the shared tail: the +0x1E2 gate that runs
 * `em_busy_set`/`em_busy_timer_reset`, then `fn_8019E398`. */
void fn_80199B24(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_80192F24(self);
        break;
    case 1:
        fn_80193394(self);
        break;
    case 2:
        fn_801938D8(self);
        break;
    case 5:
        fn_801953BC(self);
        break;
    case 6:
        fn_80196618(self);
        break;
    case 7:
        fn_801987E4(self);
        break;
    case 10:
        fn_80198910(self);
        break;
    case 11:
        fn_80198E00(self);
        break;
    case 12:
        fn_80198F14(self);
        break;
    case 13:
        fn_80199ADC(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    fn_8019E398(self);
}

/* 0x8019D8B8 - the per-mode "this action may run" predicate the band's dispatchers gate on. */
u32 fn_8019D8B8(struct _ENEMY_WORK* self, u8 mode) {
    switch (mode) {
    case 0:
        if (em_water_check(self) == 1) {
            return 1;
        }
        break;
    case 1:
        if ((self->field_0x35C & 1) == 0) {
            return 1;
        }
        break;
    case 2:
        if ((self->field_0x35C & 2) == 0) {
            return 1;
        }
        break;
    case 3:
        if ((self->field_0x228 & 3) != 0) {
            return 1;
        }
        break;
    case 4:
        if (self->vec_0x36C.z - self->pos.y >= lbl_80798288) {
            return 1;
        }
        break;
    case 5:
        if (fn_80192370((struct _ENEMY_WORK*)self, 16) == 0) {
            return 1;
        }
        break;
    case 6:
        if (self->field_0x360 <= 0) {
            return 1;
        }
        break;
    case 7:
        if (self->field_0x362 <= 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8019D9BC - the "load the action's joint position" init: arm mode 4, publish the two record
 * bytes and ask `fn_80126278` for the area's joint, then run the band's common tail. */
void fn_8019D9BC(struct _ENEMY_WORK* self, u8* out_a, u8* out_b) {
    em_move_mode_set(self, 4);
    *out_a = 12;
    *out_b = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 3:
        switch (self->area_no) {
        case 3:
            fn_80126278(self, (u16)(((self->area_no & 0xF) << 8) | 6), &self->pos);
            break;
        case 8:
            fn_80126278(self, (u16)(((self->area_no & 0xF) << 8) | 9), &self->pos);
            break;
        }
        break;
    case 9:
    case 11:
        if (self->area_no == 1) {
            fn_80126278(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        }
        break;
    }
    fn_80192618((struct _ENEMY_WORK*)self);
}

/* 0x8019DAC0 - the area dispatch of the "already in mode" state: the map lookup picks the area
 * group, this area decides whether the state is armed. */
void fn_8019DAC0(struct _ENEMY_WORK* self) {
    u32 state = 0;
    if (stage_map_kind_get(self->field_0x1E0) == 3) {
        switch (self->area_no) {
        case 1:
            state = 1;
            break;
        case 2:
            state = 2;
            break;
        case 3:
            if (self->field_0x9F6 == 2) {
                state = 1;
            }
            break;
        }
    }
    if (state == 1) {
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
    } else if (state == 2) {
        em_move_mode_set(self, 2);
        em_mot_set(self, 40, 0, 0);
    }
}

/* 0x8019DB9C - the "action 2 still running" gate: true only in mode 4 of the enemy-control state
 * machine while `em_alt_mode_ck` reports not-yet-armed. */
u32 fn_8019DB9C(struct _ENEMY_WORK* self) {
    if (self->field_0x1E2 == 4 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* 0x8019DE90 - the one-line tail call the band's teardown uses: `fn_8013A654(self, 1)`. */
void fn_8019DE90(struct _ENEMY_WORK* self) {
    fn_8013A654(self, 1);
}

/* 0x8019E398 - the two +0x35C completion flags: each is set while its action pair matches and
 * cleared once the enemy has moved on to another action. */
void fn_8019E398(struct _ENEMY_WORK* self) {
    if (em_act_ck(self, 6, 0) == 1 || em_act_ck(self, 6, 1) == 1) {
        if ((self->field_0x35C & 1) == 0) {
            self->field_0x35C |= 1;
        }
    } else if (self->action != 0 && (self->field_0x35C & 1) != 0) {
        self->field_0x35C &= ~1;
    }
    if (em_act_ck(self, 5, 17) == 1 || em_act_ck(self, 5, 18) == 1) {
        if ((self->field_0x35C & 2) == 0) {
            self->field_0x35C |= 2;
        }
    } else if (self->action != 0 && (self->field_0x35C & 2) != 0) {
        self->field_0x35C &= ~2;
    }
}

/* 0x8019E49C - "is the record we are hunting within reach": true when the paired record exists and
 * the xz distance is inside the scaled radius (or when the caller asked for the trivial answer). */
u32 fn_8019E49C(struct _ENEMY_WORK* self, u32 flag) {
    struct _ENEMY_WORK* other = fn_80131034(self, 23, 0);
    if (other != 0 && fn_8012E5A8(other) == 1) {
        if ((u8)flag == 0) {
            return 1;
        }
        f32 dist = calcDistanceSqXZ(&self->pos, &other->pos);
        f32 range = get_em_chg_scale(self) * lbl_80798510;
        if (dist < range * (get_em_chg_scale(self) * lbl_80798510)) {
            return 1;
        }
    }
    return 0;
}

/* 0x8019E580 - "action 13 in its first five sub-states". */
u32 fn_8019E580(struct _ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* 0x8019E5A8 - the record's deleting destructor: reset through `fn_8013918C`, `operator delete` on a
 * positive flag, and return the argument. */
void* fn_8019E5A8(void* p, s16 flags) {
    if (p != 0) {
        fn_8013918C((struct _ENEMY_WORK*)p, 0);
        if (flags > 0) {
            operator delete(p);
        }
    }
    return p;
}
}

/* The unit's `.bss`: the two-vector record `fn_8019E604` seeds.  The name is a GUESS (a pair of model-space
 * points). */
VEC3 vec_pair_801926EC_0[2];  /* +0x806A7A70 */
