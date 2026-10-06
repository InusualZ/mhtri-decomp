/*
 * enemy/em008_prog.cpp - enemy 008's program: the per-action `_ENEMY_WORK` state steps (a `state` switch, the
 *   motion hand-off, the wait and the finish), the `state_sub` dispatchers, the flag predicates and countdowns,
 *   the per-area seat selector and the `ResUserDataAc` accessor methods.
 * RANGE. .text 0x8015D860-0x801663E4 (88 functions); extab 0x8000DFA4-0x8000E1D4, extabindex
 *   0x80028E60-0x800291A8, .ctors 0x8056F328-0x8056F32C (`fn_80166330`), .rodata 0x8056FB50-0x8056FC10, .data
 *   0x805A5D48-0x805A6D58 (`em008_prog_tbl` first), .bss 0x806A7868-0x806A7898, .sdata2 0x80797330-0x807974F0.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_8015D860` to `fn_8015E804` (retail keeps the unfused
 *   `clrlwi`/`rlwinm` + `cmpwi` pairs, playbook 39), on from `fn_8015E854`.
 * NAMES. The file name follows the runtime dump's `em008_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.  `fn_801661BC`, `fn_801661FC` and `fn_801662D4` are slots +0x18, +0x2C and
 *   +0x08 of `lbl_805A6D28`, the `ResUserDataAc` table.
 *   Callees whose call sites disagree with the owner's header are called through `<name>_viewN`/`<name>_cN` cast
 *   macros (the same direct call), and `#define <name> <name>_hidden_<header>` hides the disagreeing declaration.
 *   The `.bss` record names (`vec_pair_80165FC8_*`) are GUESSes.
 * RESIDUALS. 26 rows unwritten: 0x8015EB44-0x8015ED00, 0x8015ED94-0x8015EED4, 0x8015F0E8-0x8015F23C,
 *   0x8015F820-0x8015FCB0, 0x8015FD1C-0x80161784, 0x80161A34-0x80161D24, 0x80161E94-0x80162028,
 *   0x80162154-0x801624E4, 0x80162500-0x80163274, 0x80163298-0x80165960, 0x80165964-0x80165BC0,
 *   0x80165C14-0x80165C20, 0x80165CD0-0x80165FC8.
 *   17 partial rows, including:
 *  - `fn_8015D908`: the same 11 instructions; retail lays out the `return 1` block before the `return 0` block,
 *    ours after (every nesting, `else`, temporary and the shapesearch candidates measured lower or flip the test);
 *  - `fn_8015F23C`: retail bounds the table with `cmplwi r0,21`, ours with 14 (cases 15..21 are the default);
 *  - `fn_8015F510`: retail re-masks `clrlwi r0,r31,24` before the compare and restores f31 with `li r0,0x18;
 *    psq_lx`, ours compares r31 and restores with `psq_l f31,0x18(r1)`;
 *  - `fn_80165C64`: retail tests `em_parts_damage_level_get(self, arg) & 1` with `clrlwi r0,r3,24; clrlwi r0,r0,31;
 *    cmpwi; bne`, every spelling tried folds to `clrlwi. r0,r3,31; xori r3,r0,1` (12 bytes shorter).
 *   The other 13 partial rows have no recorded cause.
 *   flipcheck: `.ctors`/`.rodata`/`.sdata2` claimed, not emitted; `.data` 0xE8 against 0x1010; `.text` (0x2658 of
 *   0x8B84), extab (0x178 of 0x230) and extabindex (0x234 of 0x348) short of the claim and differing.
 *   `lbl_805A6D28` lies in this unit's `.data`, but the three slots are flat functions and no class emits the
 *   table (rule 10).
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8015D860.h"
#include "unsplit/enemy.h"
#include "enemy/enemy_control.h" /* em_spawn_request (the owner's header, rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "ef.h"              /* VEC3_ctor */
#include "ef/fn_80105314.h"  /* fn_801057A4 */
#include "draw_shape.h"      /* draw_shape_arm */
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "ef.h"
#include "enemy/EnemyData.h"
#include "enemy/fn_8015D860.h" /* this unit's step functions `fn_8015DDB8`... (rule 2) */
#include "fn_8004CAD8.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
/* `enemy/fn_80165FC8.h` spells these callees with signatures that clash with this file's own views, so each is hidden
 * for the include. */
#define assignVec3 assignVec3_hidden_fn_80165FC8_h
#define em_act_ck em_act_ck_hidden_fn_80165FC8_h
#define em_fall_height_get em_fall_height_get_hidden_fn_80165FC8_h
#define em_frame_flag_set em_frame_flag_set_hidden_fn_80165FC8_h
#define em_mot_set em_mot_set_hidden_fn_80165FC8_h
#define fn_8013918C fn_8013918C_hidden_fn_80165FC8_h
#define fn_8013A654 fn_8013A654_hidden_fn_80165FC8_h
#define fn_8015D934 fn_8015D934_hidden_fn_80165FC8_h
#define rotMatrixX rotMatrixX_hidden_fn_80165FC8_h
#define rotMatrixZ rotMatrixZ_hidden_fn_80165FC8_h
#include "enemy/fn_80165FC8.h"
#undef rotMatrixZ
#undef rotMatrixX
#undef fn_8015D934
#undef fn_8013A654
#undef fn_8013918C
#undef em_mot_set
#undef em_frame_flag_set
#undef em_fall_height_get
#undef em_act_ck
#undef assignVec3
#include "ef/eft_slot.h"    /* enemy_data_find / enemy_data_grp (rule 2: their owner's header) */
#include "stage/stg_w.h"
/* Call-site views: each macro casts a callee to the signature its call sites use (the same direct call). */
#define rotMatrixZ_view1 ((void (*)(u32, nw4r::math::MTX34*))rotMatrixZ)
#define rotMatrixX_view1 ((void (*)(u32, nw4r::math::MTX34*))rotMatrixX)
#define fn_8015D934_view1 ((u8 (*)(struct _ENEMY_WORK*))fn_8015D934)
#define fn_8013A654_view1 ((void (*)(ResUserDataAc*, u32))fn_8013A654)
#define fn_8013918C_view1 ((void (*)(void*, s16))fn_8013918C)
#define em_motion_param_set_view1 ((void (*)(struct _ENEMY_WORK*, u32, f32))em_motion_param_set)
#define em_mot_finished_ck_view1 ((u32 (*)(struct _ENEMY_WORK*))em_mot_finished_ck)
#define em_parts_damage_level_get_view1 ((u8 (*)(_ENEMY_WORK*, u8))em_parts_damage_level_get)
#define em_mot_set_view1 ((void (*)(struct _ENEMY_WORK*, u32, u32, u32))em_mot_set)
#define em_frame_flag_set_view1 ((void (*)(void))em_frame_flag_set)
#define em_fall_height_get_view1 ((void (*)(struct _ENEMY_WORK*))em_fall_height_get)
#define em_act_ck_view1 ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_act_ck)
#define assignVec3_view1 ((void (*)(nw4r::math::VEC3*, const nw4r::math::VEC3*))assignVec3)

/* ---------------------------------------------------------------------------------------------------
 * the 0xC-byte vtable helper this unit allocates
 * ------------------------------------------------------------------------------------------------- */

/* The 12-byte helper `fn_8015D9C8` allocates and `fn_8015DAA8` constructs (+0x00: `lbl_805A6D28`); the same
 * record as `enemy/em001_prog.cpp`'s `Helper_80147CE0`.  size: 0xC (`operator new(0xC)`) */
typedef struct Helper_8015DAA8 {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_8015DAA8;

/* The helper's table, in this unit's `.data`; no class emits it yet (see the unit header). */
extern "C" u8 lbl_805A6D28[];

/* ---------------------------------------------------------------------------------------------------
 * callees
 * ------------------------------------------------------------------------------------------------- */

/* The three C++ free functions, declared by their real signatures so the front-end mangles them to
 * the map spellings (rule 9: the mangled spelling is never written). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);
void get_joint_wpos_em(_ENEMY_WORK* self, u32 joint, Vec3* out);

/* This unit's own forward declarations (each is called above its definition). */
extern "C" Helper_8015DAA8* fn_8015DAA8(Helper_8015DAA8* self);
extern "C" void fn_8015DE48(_ENEMY_WORK* self);
extern "C" void fn_8015DEC4(_ENEMY_WORK* self);
extern "C" void fn_8015DF40(_ENEMY_WORK* self);
extern "C" void fn_8015DFBC(_ENEMY_WORK* self);
extern "C" void fn_8015E0BC(_ENEMY_WORK* self);
extern "C" void fn_8015E148(_ENEMY_WORK* self);
extern "C" void fn_8015E1C4(_ENEMY_WORK* self);
extern "C" void fn_8015E338(_ENEMY_WORK* self);
extern "C" void fn_8015E454(_ENEMY_WORK* self);
extern "C" void fn_8015E4F8(_ENEMY_WORK* self);
extern "C" void fn_8015E568(_ENEMY_WORK* self);
extern "C" void fn_8015E62C(_ENEMY_WORK* self);
extern "C" void fn_8015E6E8(_ENEMY_WORK* self);
extern "C" void fn_8015E788(_ENEMY_WORK* self);

/* Callees whose owners' headers do not declare them: `lobby/fn_8030121C.cpp`'s effect spawns `fn_80305924`
 * and `eft_em_spawn`, and `enemy/em_model.cpp`'s `fn_803B9BA0`. */
extern "C" void fn_80305924(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s, s32 id);
extern "C" void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, void* v, f32 s);
extern "C" void fn_803B9BA0(_ENEMY_WORK* self, void* pos, s32 value);

/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet (values from the DOL). */
extern f32 lbl_80797330; /* 0.0 */
extern f32 lbl_80797334; /* -20.0 */
extern f32 lbl_80797338; /* 100.0 */
extern f32 lbl_8079733C; /* 1.0 */
extern f32 lbl_80797340; /* 50.0 */
extern f32 lbl_80797344; /* 150.0 */
extern f32 lbl_80797348; /* 160.0 */
extern f32 lbl_8079734C; /* -50.0 */
extern f32 lbl_80797350; /* 164.0 */
extern f32 lbl_80797354; /* 264.0 */
extern f32 lbl_80797358; /* 0.6 */
extern f32 lbl_8079735C; /* 120.0 */

extern u32 em_frame_check(_ENEMY_WORK*, u16, f32, f32);
extern u32 em_parts_damage_level_get(_ENEMY_WORK*, u8);

extern "C" {
extern void fn_8015EA24(_ENEMY_WORK*);
extern void fn_8015EAC8(_ENEMY_WORK*);
extern void fn_8015EB44(_ENEMY_WORK*);
extern void fn_8015ED00(_ENEMY_WORK*);
extern void fn_8015ED94(_ENEMY_WORK*);
extern void fn_8015EED4(_ENEMY_WORK*);
extern void fn_8015EFAC(_ENEMY_WORK*);
extern void fn_8015F0E8(_ENEMY_WORK*);
extern void fn_8015F6C4(_ENEMY_WORK*, u8);
extern void fn_8015FD84(_ENEMY_WORK*, u8);
extern void fn_80161660(_ENEMY_WORK*);
extern void fn_80161784(_ENEMY_WORK*, u8);
extern void fn_801618A4(_ENEMY_WORK*);
extern void fn_8016198C(_ENEMY_WORK*, u8);
extern void fn_80161A34(_ENEMY_WORK*, u8);
extern void fn_80161D24(_ENEMY_WORK*);
extern void fn_80161DCC(_ENEMY_WORK*, u8);
extern void fn_80161E94(_ENEMY_WORK*, u8);
extern void fn_8015F820(_ENEMY_WORK*);
extern void fn_8015F8E4(_ENEMY_WORK*);
extern void fn_8015F9FC(_ENEMY_WORK*);
extern void fn_8015FAC0(_ENEMY_WORK*);
extern void fn_8015FB78(_ENEMY_WORK*);
extern void fn_80162500(_ENEMY_WORK*);
extern void fn_801631C4(_ENEMY_WORK*);
extern void fn_8015E854(_ENEMY_WORK* self, u8 arg1, u8 arg2);
extern void fn_801624E4(_ENEMY_WORK* self);
extern void fn_801624EC(_ENEMY_WORK* self);
extern void fn_80163274(_ENEMY_WORK* self);
extern void fn_80165960(void);
extern s32 fn_80165BC0(void);
extern void fn_80165BC8(_ENEMY_WORK*, u8*, u8*);
extern s32 fn_80165C20(_ENEMY_WORK*, u8);
extern s32 fn_80165C64(_ENEMY_WORK*, u32);
}

/* The unit's `.rodata` tables, declared: the source does not emit them yet. */
extern u8 lbl_8056FB50[];

extern f32 lbl_80797330;
extern f32 lbl_8079733C;
extern f32 lbl_80797360;
extern f32 lbl_80797364;
extern f32 lbl_80797368;
extern f32 lbl_8079736C;
extern f32 lbl_80797380;
extern f32 lbl_80797384;
extern f32 lbl_80797390;
extern f32 lbl_80797394;
extern f32 lbl_80797398;
extern f32 lbl_8079739C;
extern f32 lbl_807973A0;
extern f32 lbl_807973A4;

extern f32 lbl_807973B0;
extern f32 lbl_807973B4;
extern f32 lbl_807973B8;
extern f32 lbl_80797340;
extern f32 lbl_807973BC;
extern f32 lbl_807973C0;

extern f32 lbl_807973E4;
extern f32 lbl_807973E8;

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x8015D860 - seeds the action's flag byte: bit 2 for two damage levels on part 2, bit 0 and bit 5 for the
 * mask-2 and mask-1 probes. */
extern "C" void fn_8015D860(_ENEMY_WORK* self) {
    self->field_0x1E4 = 0;
    if (em_parts_damage_level_get_view1(self, 2) >= 2) {
        self->field_0x1E4 |= 4;
    }
    if (em_flags836_ck(self, 2) == 1) {
        self->field_0x1E4 |= 1;
    }
    if (em_flags836_ck(self, 1) == 1) {
        self->field_0x1E4 |= 0x20;
    }
}

/* 0x8015D8F0 - is none of the mask's bits set in the flag byte? */
extern "C" u32 fn_8015D8F0(_ENEMY_WORK* self, u8 mask) {
    return (self->field_0x1E4 & mask) == 0;
}

/* 0x8015D908 - the two-bit gate: 0 only when bit 0 and bit 1 are both set. */
extern "C" u32 fn_8015D908(_ENEMY_WORK* self) {
    if (self->field_0x1E4 & 1) {
        if (self->field_0x1E4 & 2) {
            return 0;
        }
    }
    return 1;
}

/* 0x8015D934 - counts the clear bits of the low six (bit 0 .. bit 5). */
extern "C" u32 fn_8015D934(_ENEMY_WORK* self) {
    u8 count = 0;

    if ((self->field_0x1E4 & 1) == 0) {
        count = 1;
    }
    if ((self->field_0x1E4 & 2) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 4) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 8) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 0x10) == 0) {
        count = count + 1;
    }
    if ((self->field_0x1E4 & 0x20) == 0) {
        count = count + 1;
    }
    return count;
}

/* 0x8015D9B8 - clears the two +0x328/+0x32A countdowns. */
extern "C" void fn_8015D9B8(_ENEMY_WORK* self) {
    self->field_0x328 = 0;
    self->timer_0x32A = 0;
}

/* 0x8015D9C8 - the per-tick entry: hand over the motion on `arg == 2`, re-seed the flag byte, attach the
 * helper when there is none, and spawn the position effect outside the +0x009 gate. */
extern "C" void fn_8015D9C8(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Helper_8015DAA8* helper;

    VEC3_ctor(&v);
    switch ((u8)arg) {
      case 2:
        em_move_mode_set(self, 4);
        fn_80128A8C(self, 6, 5);
        em_state_refresh(self);
        break;
    }
    fn_8015D860(self);
    if (em_res_user_data_ck(self) == 0) {
        helper = (Helper_8015DAA8*)operator new(0xC);
        if (helper != 0) {
            fn_8015DAA8(helper);
        }
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, lbl_80797330, lbl_80797334, lbl_80797338);
        fn_801057A4(self, 0x14, &v, lbl_8079733C, 0);
    }
}

/* 0x8015DAA8 - the helper's constructor: base first, then this class's table. */
extern "C" Helper_8015DAA8* fn_8015DAA8(Helper_8015DAA8* self) {
    em_res_user_data_ctor(self);
    self->vtbl = lbl_805A6D28;
    return self;
}

/* 0x8015DAE4 - one action pair's (state, sub-state) transitions: in state 7, sub-state 7/0xD goes to 0x10
 * when `fn_8012EC3C` answers and 0xB to 0x11 while flag bit 0 is clear. */
extern "C" void fn_8015DAE4(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (*state) {
      case 7:
        switch (*sub) {
          case 7:
          case 0xD:
            if (fn_8012EC3C(self) == 1) {
                *sub = 0x10;
            }
            break;
          case 0xB:
            if ((self->field_0x1E4 & 1) == 0) {
                *sub = 0x11;
            }
            break;
        }
        break;
    }
}

/* 0x8015DB68 - the second transition table: action 1 arms the +0x32A countdown, the joint effect and the
 * `fn_801376B4` hand-over; action 0xA arms flag bits 0/5 and +0x491, each with an effect. */
extern "C" void fn_8015DB68(_ENEMY_WORK* self, u8 arg, u8 sub) {
    VEC3 v1;
    VEC3 v2;

    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    switch (arg) {
      case 1:
        switch (sub) {
          case 6:
            self->timer_0x32A = 0x384;
            break;
          case 8:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x14, &v2);
                em_spawn_request(self->field_0x01A, 0x21, 0, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF,
                            &v2, 0);
            }
            break;
          case 9:
            fn_801376B4(self);
            break;
        }
        break;
      case 0xA:
        switch (sub) {
          case 0xC3:
            em_part_hit_set(self, 1, 0);
            if (fn_8015D8F0(self, 1) == 1) {
                self->field_0x1E4 |= 1;
                setVector3(&v1, lbl_80797330, lbl_80797340, lbl_80797344);
                fn_80305924(self, 1, 0x14, &v1, lbl_8079733C, -1);
                fn_803B9BA0(self, &self->pos, 0x1E);
            }
            break;
          case 0xC8:
            em_part_hit_set(self, 0, 0);
            self->field_0x491 = 1;
            if (fn_8015D8F0(self, 0x20) == 1) {
                self->field_0x1E4 |= 0x20;
                setVector3(&v1, lbl_80797330, lbl_80797330, lbl_80797330);
                fn_80305924(self, 1, 0x2D, &v1, lbl_8079733C, -1);
                fn_803B9BA0(self, &self->pos, 0x1E);
            }
            break;
        }
        break;
    }
}

/* 0x8015DD6C - the per-tick bookkeeping: mirror "state 4" into the +0x38B flag and run the two
 * +0x328/+0x32A countdowns down while they are positive. */
extern "C" void fn_8015DD6C(_ENEMY_WORK* self) {
    switch (self->field_0x1E2) {
      case 4:
        self->field_0x38B = 1;
        break;
      default:
        self->field_0x38B = 0;
        break;
    }
    if (self->field_0x328 > 0) {
        self->field_0x328--;
    }
    if (self->timer_0x32A > 0) {
        self->timer_0x32A--;
    }
}

/* 0x8015DDB8 - the state-0 step: motion 0, then motion set (7, 0x12). */
extern "C" void fn_8015DDB8(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    fn_80128AAC(self, 7, 0x12);
    fn_80133BB4(self);
}

/* 0x8015DE00 - the state-0 step: motion 4, then motion set (6, 0xC). */
extern "C" void fn_8015DE00(_ENEMY_WORK* self) {
    em_move_mode_set(self, 4);
    fn_80128AAC(self, 6, 0xC);
    fn_80133BB4(self);
}

/* 0x8015DE48 - the two-step state: start motion set (1, 0xA) on entry, finish on the wait. */
extern "C" void fn_8015DE48(_ENEMY_WORK* self) {
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

/* 0x8015DEC4 - the same two-step shape with motion set (2, 6). */
extern "C" void fn_8015DEC4(_ENEMY_WORK* self) {
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

/* 0x8015DF40 - the same two-step shape with motion set (0xE, 6). */
extern "C" void fn_8015DF40(_ENEMY_WORK* self) {
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

/* 0x8015DFBC - the two-step state that also refreshes the scene: motion set (1, 0, 0) and the
 * height pair on entry, `fn_801280F4` on the wait. */
extern "C" void fn_8015DFBC(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    fn_80136D14(self);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_801280F4(self);
        }
        break;
    }
}

/* 0x8015E05C - the sparse sub-state dispatcher: the compare chain for {0,1,2,3,4,6,7}. */
extern "C" void fn_8015E05C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_8015DE48(self);
        break;
      case 1:
        fn_8015DEC4(self);
        break;
      case 2:
        fn_8015DF40(self);
        break;
      case 3:
        fn_8015DE48(self);
        break;
      case 4:
        fn_8015DE48(self);
        break;
      case 6:
        fn_8015DE48(self);
        break;
      case 7:
        fn_8015DFBC(self);
        break;
    }
}

/* 0x8015E0BC - the two-step state with motion set (0xF, 6); the local vector is zeroed but not
 * used by either arm (retail does the same call). */
extern "C" void fn_8015E0BC(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E148 - the same two-step shape with motion set (9, 6). */
extern "C" void fn_8015E148(_ENEMY_WORK* self) {
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

/* 0x8015E1C4 - the long step: motion set (0xF, 6), then three frame windows for the part effect and the
 * ground effect at (0, -50, 100), the last gated on `system_w`'s +0x0C low bits. */
extern "C" void fn_8015E1C4(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
      case 1:
        if (em_frame_check(self, 0, lbl_80797348, lbl_80797330) == 1) {
            em_hit_window_set(self, 1, 0x12, 5);
            draw_shape_arm((u32)self, 0x14, 0xA);
        }
        if (em_frame_check(self, 0, lbl_80797348, lbl_80797330) == 1) {
            setVector3(&v, lbl_80797330, lbl_8079734C, lbl_80797338);
            eft_em_spawn(self, 0, 0x13, &v, lbl_8079733C);
        }
        if (em_frame_check(self, 3, lbl_80797350, lbl_80797354) == 1) {
            if ((system_w.field_0x0c & 7) == 0) {
                setVector3(&v, lbl_80797330, lbl_8079734C, lbl_80797338);
                eft_em_spawn(self, 1, 0x13, &v, lbl_8079733C);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E338 - the four-step state: motion set (0x28, 2), then (0x6E, 4) with the +0x20 counter
 * seeded to 0x708, then the 0.6-ratio step (0x70, 4) once the counter runs out, then the wait. */
extern "C" void fn_8015E338(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x28, 2, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
      case 2:
        fn_8013221C(self, lbl_80797358, 1, 0xA);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            fn_80132264(self);
        }
        break;
      case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E454 - the two-step state: motion set (0xC8, 8) and the +0x20 counter at 3 plus the
 * `fn_80130CDC` arm on entry; the 120-frame window hands over to `em_state_set`. */
extern "C" void fn_8015E454(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 8, 0);
        self->timer_0x020 = 3;
        fn_80130CDC(self, 0x3E8);
        break;
      case 1:
        if (em_frame_check(self, 1, lbl_8079735C, lbl_80797330) == 1) {
            em_state_set(self, 1, 8);
        }
        break;
    }
}

/* 0x8015E4F8 - the two-step state with motion set (0xCA, 2). */
extern "C" void fn_8015E4F8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_mot_set(self, 0xCA, 2, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E568 - the two-step state that counts two motion waits: motion set (0x24, 6) and the hit window,
 * then (1, 7) once the +0x20 counter reaches 2. */
extern "C" void fn_8015E568(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x24, 6, 0);
        em_hit_window_set(self, 1, 0xB, 2);
        self->timer_0x020 = 0;
        /* falls through to the wait arm */
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (self->timer_0x020 >= 2) {
            self->state++;
            em_state_set(self, 1, 7);
        }
        break;
    }
}

/* 0x8015E62C - the three-step state: motion set (0x29, 8) plus a flag re-seed on entry, then
 * (0x70, 2, 0x42), then the wait. */
extern "C" void fn_8015E62C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 8, 0);
        fn_8015D860(self);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x70, 2, 0x42);
        }
        break;
      case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E6E8 - the two-step state: motion set (0xC8, 4) and the +0x20 counter at 3 on entry, then
 * the counter runs down and hands over to `em_state_set`. */
extern "C" void fn_8015E6E8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xC8, 4, 0);
        self->timer_0x020 = 3;
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            if (--self->timer_0x020 <= 0) {
                em_state_set(self, 1, 5);
            }
        }
        break;
    }
}

/* 0x8015E788 - the two-step state with motion set (0xE, 6). */
extern "C" void fn_8015E788(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8015E804 - the contiguous sub-state dispatcher (the jump table at `jumptable_805A5DC0`). */
extern "C" void fn_8015E804(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_8015E0BC(self);
        break;
      case 1:
        fn_8015E148(self);
        break;
      case 2:
        fn_8015E1C4(self);
        break;
      case 3:
        fn_8015E338(self);
        break;
      case 4:
        fn_8015E454(self);
        break;
      case 5:
        fn_8015E4F8(self);
        break;
      case 6:
        fn_8015E568(self);
        break;
      case 7:
        fn_8015E62C(self);
        break;
      case 8:
        fn_8015E6E8(self);
        break;
      case 9:
        fn_8015E788(self);
        break;
    }
}

#pragma peephole on

/* ---- bodies ---- */

extern "C" void fn_8015E854(_ENEMY_WORK* self, u8 arg1, u8 arg2) {
    u8 temp_r4;
    u8 temp_a1;
    u8 temp_a2;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 8, 0);
        temp_a1 = arg1;
        switch ((s32) temp_a1) {
        case 0:
            em_approach_start(self, lbl_80797360, 0);
            return;
        case 1:
            em_approach_start(self, lbl_80797360, 0);
            return;
        case 2:
            em_approach_start(self, lbl_80797364, 0);
            return;
        case 3:
            em_approach_start(self, lbl_80797330, 0);
            return;
        case 4:
            em_approach_start(self, lbl_80797368, 0);
            return;
        case 5:
            em_approach_start(self, lbl_8079736C, 0);
            return;
        case 6:
            em_approach_start(self, lbl_80797364, 0);
            return;
        }
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            if (fn_8012F948(self) == 0) {
                temp_a2 = arg2;
                switch ((s32) temp_a2) {
                case 0:
                    self->state = self->state + 1;
                    em_mot_set(self, 0xB, 4, 0);
                    return;
                case 1:
                    em_action_finish(self);
                    return;
                }
            }
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015EA24(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056FB50, 0, 1, 0);
        if (self->field_0x482 == 1U) {
            em_mot_speed_set(self, lbl_8079733C);
        }
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FB50) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015EAC8(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015ED00(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 0xA, 0);
        em_approach_start(self, lbl_80797330, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8016198C(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r5;
    f32 temp_f1;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        if ((u8) arg1 == 0) {
            temp_f1 = lbl_807973E8;
        } else {
            temp_f1 = lbl_807973B0;
        }
        if (em_frame_check(self, 1, temp_f1, lbl_80797330) == 1U) {
            fn_8015DDB8(self);
        }
        return;
    }
}

extern "C" void fn_80162028(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 4, 0x2C);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            fn_8015DDB8(self);
        }
        return;
    }
}

extern "C" void fn_801624E4(_ENEMY_WORK* self) {
    fn_8015F6C4(self, 0);
}

extern "C" void fn_801624EC(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_801624E4(self);
    }
}

extern "C" void fn_80163274(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_80162500(self);
        return;
    case 1:
        fn_801631C4(self);
        return;
    }
}

extern "C" void fn_80165960(void) {
}

extern "C" s32 fn_80165BC0(void) {
    return 1;
}

extern "C" void fn_80165BC8(_ENEMY_WORK* self, u8* out_state, u8* out_flags) {
    em_move_mode_set(self, 4);
    *out_state = 0xC;
    *out_flags = 0;
}

extern "C" s32 fn_80165C20(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4 = arg1;

    if ((u8) (temp_r4 - 3) <= 1U) {
        if (em_alt_mode_ck(self) == 1U) {
            return 1;
        }
    }
    return 0;
}

extern "C" s32 fn_80165C64(_ENEMY_WORK* self, u32 arg1) {
    u8 temp_r4;
    u8 temp_r0;
    u32 temp_r3;

    temp_r4 = (u8) arg1;
    if (temp_r4 <= 2U) {
        temp_r3 = em_parts_damage_level_get(self, temp_r4);
        temp_r0 = (u8) temp_r3;
        if ((temp_r0 & 1) == 0) {
            return 1;
        }
    } else if ((u32) (temp_r4 - 3) <= 1U) {
        if (fn_8012EC3C(self) == 1U) {
            return 1;
        }
    }
    return 0;
}

extern "C" void fn_8015EED4(_ENEMY_WORK* self) {
    u8 temp_r4;
    f32 temp_f1;
    f32 t1;
    f32 t2;
    f32 t3;
    f32 t4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x22, 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80797330, lbl_80797390) == 1U) {
            temp_f1 = get_em_base_scale(self);
            t1 = lbl_8079739C * temp_f1;
            t2 = t1 * lbl_80797398;
            t3 = t2 / lbl_807973A0;
            t4 = lbl_80797394 + t3;
            em_turn_to_target(self, (u16) (s32) t4);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015EFAC(_ENEMY_WORK* self) {
    f32* temp_r31;
    u8 temp_r3;
    f32 temp_f0;
    f32 temp_f1;

    temp_r31 = fn_80126454(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 6, 0);
        temp_f1 = fn_80050EF4(&self->pos, &self->vec_0x36C);
        temp_f1 = lbl_807973A4 * temp_f1;
        temp_f0 = -temp_r31[1];
        if (temp_f1 > temp_f0) {
            temp_f1 = temp_f0;
        }
        em_approach_start(self, temp_f1, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1U) {
            if (fn_8012F948(self) == 0) {
                self->state = self->state + 1;
                em_mot_set(self, 0xB, 4, 0);
                em_approach_start(self, lbl_80797330, 0);
            }
        }
        return;
    case 2:
        em_approach_step(self, 0, 0x80);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_801618A4(_ENEMY_WORK* self) {
    u8 temp_r4;
    f32 temp_f1;
    f32 t1;
    f32 t2;
    f32 t3;
    f32 t4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 4, 0);
        em_hit_window_set_default(self, 0, 0xC);
        return;
    case 1:
        if (em_frame_check(self, 3, lbl_80797330, lbl_80797384) == 1U) {
            temp_f1 = get_em_base_scale(self);
            t1 = lbl_807973E4 * temp_f1;
            t2 = t1 * lbl_80797398;
            t3 = t2 / lbl_807973A0;
            t4 = lbl_80797394 + t3;
            em_turn_to_target(self, (u16) (s32) t4);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80161D24(_ENEMY_WORK* self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x18, 2, 0);
        em_hit_window_set_default(self, 0, 2);
        em_hit_window_set_default(self, 1, 0x10);
        fn_80130CDC(self, -0x1E);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80161DCC(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1C, 2, 0);
        if ((u8) arg1 == 0) {
            em_hit_window_set_default(self, 0, 3);
        } else {
            em_hit_window_set(self, 0, 0x14, 0x40);
        }
        fn_80130CDC(self, -0x1E);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015F2D8(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_hit_window_set_default(self, 0, 0xE);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_807973B4, lbl_80797330) == 1U) {
            fn_80136D14(self);
        }
        if (em_mot_end_ck(self) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F39C(_ENEMY_WORK* self) {
    u8 temp_r3;
    s32 temp_r0;

    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_mot_set_ck(self, 1, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        self->timer_0x020 = 0x5A;
        return;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 == 0) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F450(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_mot_set(self, 6, 0, 0);
        em_approach_start(self, lbl_80797380, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F510(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r3;
    f32 f31;

    f31 = lbl_80797330;
    em_busy_set(self);
    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 0, 0);
        if ((u8) arg1 == 0) {
            f31 = lbl_807973B8;
        }
        em_approach_start(self, f31, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F60C(_ENEMY_WORK* self) {
    u8 temp_r3;

    em_busy_timer_reset(self);
    fn_80131E00(self);
    fn_80136D14(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, lbl_8056FB50, 0, 0, 0);
        fn_801303EC(self, fn_8013032C(self));
        return;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FB50) == 1U) {
            fn_801280F4(self);
        }
        return;
    }
}

extern "C" void fn_8015F6C4(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r3;

    em_busy_set(self);
    em_busy_timer_reset(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 4);
        if ((u8) arg1 == 0) {
            em_mot_set(self, 0x27, 2, 0x24);
            fn_801303EC(self, lbl_807973BC);
            em_hit_window_set_default(self, 0, 0xF);
        } else {
            em_mot_set(self, 0x27, 2, 0);
            fn_801303EC(self, lbl_80797330);
            em_hit_window_set_default(self, 0, 0x11);
        }
        fn_80136D14(self);
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_807973C0, lbl_80797330) == 1U) {
            fn_80136D14(self);
        }
        if ((u8) arg1 == 0) {
            if (self->field_0x1AC >= lbl_80797330) {
                fn_801303EC(self, lbl_80797330);
            } else {
                fn_801303FC(self, lbl_80797340);
            }
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_80161784(_ENEMY_WORK* self, u8 arg1) {
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        em_move_mode_set(self, 0);
        if ((u8) arg1 == 0) {
            em_mot_set(self, 0x14, 4, 0);
            em_hit_window_set_default(self, 0, 5);
            em_hit_window_set_default(self, 1, 6);
        } else {
            em_mot_set(self, 0x1A, 4, 0);
            em_hit_window_set_default(self, 0, 7);
            em_hit_window_set_default(self, 1, 8);
        }
        return;
    case 1:
        if ((u8) arg1 == 0) {
            em_turn_in_window(self, lbl_80797330, lbl_80797340, 0x4000);
        } else {
            em_turn_in_window(self, lbl_80797330, lbl_80797340, -0x4000);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

extern "C" void fn_8015FCB0(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015F2D8(self);
        return;
    case 1:
        fn_8015F39C(self);
        return;
    case 2:
        fn_8015F450(self);
        return;
    case 3:
        fn_8015F510(self, 0);
        return;
    case 4:
        fn_8015F60C(self);
        return;
    case 5:
        fn_8015F6C4(self, 0);
        return;
    case 6:
        fn_8015F510(self, 1);
        return;
    case 7:
        fn_8015F820(self);
        return;
    case 8:
        fn_8015F8E4(self);
        return;
    case 9:
        fn_8015F6C4(self, 1);
        return;
    case 10:
        fn_8015F9FC(self);
        return;
    case 11:
        fn_8015FAC0(self);
        return;
    case 12:
        fn_8015FB78(self);
        return;
    case 13:
        return;
    }
}

extern "C" void fn_8015F23C(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015E854(self, 0, 0);
        return;
    case 1:
        fn_8015E854(self, 1, 1);
        return;
    case 2:
        fn_8015E854(self, 2, 1);
        return;
    case 3:
        fn_8015E854(self, 3, 1);
        return;
    case 4:
        fn_8015E854(self, 4, 1);
        return;
    case 5:
        fn_8015E854(self, 5, 1);
        return;
    case 6:
        fn_8015EA24(self);
        return;
    case 7:
        fn_8015EAC8(self);
        return;
    case 8:
        fn_8015EB44(self);
        return;
    case 9:
        fn_8015ED00(self);
        return;
    case 10:
        fn_8015ED94(self);
        return;
    case 11:
        fn_8015EED4(self);
        return;
    case 12:
        fn_8015E854(self, 6, 1);
        return;
    case 13:
        fn_8015EFAC(self);
        return;
    case 14:
        fn_8015F0E8(self);
        return;
    }
}

extern "C" void fn_801620A4(_ENEMY_WORK* self) {
    switch ((s32) self->state_sub) {
    case 0:
        fn_8015FD84(self, 0);
        return;
    case 1:
        fn_80161660(self);
        return;
    case 2:
        fn_80161784(self, 0);
        return;
    case 3:
        fn_8015FD84(self, 1);
        return;
    case 4:
        fn_80161DCC(self, 1);
        return;
    case 5:
        fn_801618A4(self);
        return;
    case 6:
        fn_8016198C(self, 0);
        return;
    case 7:
        fn_80161A34(self, 0);
        return;
    case 8:
        fn_80161D24(self);
        return;
    case 9:
        fn_80161DCC(self, 0);
        return;
    case 10:
        fn_80161784(self, 1);
        return;
    case 11:
        fn_80161E94(self, 0);
        return;
    case 12:
        fn_8016198C(self, 1);
        return;
    case 13:
        fn_80161A34(self, 1);
        return;
    case 14:
        fn_8015FD84(self, 2);
        return;
    case 15:
        fn_8015FD84(self, 3);
        return;
    case 16:
        fn_80161A34(self, 2);
        return;
    case 17:
        fn_80161E94(self, 1);
        return;
    case 18:
        fn_80162028(self);
        return;
    }
}

/* ---- the range's functions, in address order ---- */

u32 fn_80165FC8(_ENEMY_WORK* self, u32 arg1) {
    s16* seat = &self->field_0x328;
    u8 entry;
    u32 sel;

    get_move_work_adrs(3);
    get_move_work_max(3);
    entry = stage_map_kind_get(self->field_0x1E0);
    if ((s32)entry != 2) {
        return 0;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    sel = 0xFF;
    if ((s32)entry == 2) {
        sel = 3;
    }
    if (sel != 0xFF && (s32)fn_80129DB8(self) == 2) {
        return 1;
    }
    if (em_mot_finished_ck_view1(self) == 0 && self->value_0x452 >= 0x384) {
        u32 motion;
        if ((s32)self->area_no != 6) {
            if (fn_802B0998(3) == 1) {
                motion = 6;
            } else {
                motion = 4;
            }
        } else {
            motion = 6;
        }
        if (fn_8012A014(self, 0, motion, (u16)arg1, 0, lbl_805A5DB4) == 1) {
            return 1;
        }
    }
    if (em_mot_finished_ck_view1(self) == 0 && self->value_0x452 >= 0x384 && fn_8015D934_view1(self) <= 2 && (s32)entry == 2
        && self->area_no != 3 && fn_80129A1C(self, (u16)arg1, 0x1E, seat) == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 3;
        return 1;
    }
    if (fn_80129A70(self, (u16)arg1) == 1) {
        return 1;
    }
    return fn_8012A204(self) == 1;
}

void fn_801661BC(ResUserDataAc* self) {
    u8 pad[0x1C];

    fn_8005D1AC(pad, 0);
    fn_8013A654_view1(self, 7);
}

void fn_801661FC(ResUserDataAc* self, MTX34* mtx, void* cursor, s32 arg3) {
    u32 head[1];
    s32 idx;
    MTX34 local;
    MTX34 out;

    (void)arg3;
    fn_8005D1AC(head, 0);
    MTX34_ctor(&out);
    MTX34_ctor(&local);
    idx = (s32)(u32)fn_80097EB0(cursor, 0x18);
    fn_8005D0CC(head, &idx);
    {
        _ENEMY_WORK* work = self->work;
        fn_800532DC(&local, &mtx[fn_8006FDCC(head)]);
        mtx34_identity(&out);
        rotMatrixX_view1(work->field_0x608, &out);
        rotMatrixZ_view1(work->field_0x610, &out);
        mtx34_concat_assign(&local, &out);
        copyMat33(&mtx[fn_8006FDCC(head)], &local);
    }
}

ResUserDataAc* fn_801662D4(ResUserDataAc* self, s32 flags) {
    if (self != 0) {
        fn_8013918C_view1(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

void fn_80166330(void) {
    VEC3 v0;
    VEC3 v1;
    VEC3 v2;
    VEC3 v3;

    /* `setVec3` returns its first argument, so the cast back to `VEC3*` costs no instruction. */
    assignVec3_view1(&vec_pair_80165FC8_0[0], (VEC3*)setVec3(&v0, lbl_80797330, lbl_80797338, lbl_80797330));
    assignVec3_view1(&vec_pair_80165FC8_0[1], (VEC3*)setVec3(&v1, lbl_80797330, lbl_807974EC, lbl_80797330));
    assignVec3_view1(&vec_pair_80165FC8_1[0], (VEC3*)setVec3(&v2, lbl_80797330, lbl_80797330, lbl_80797330));
    assignVec3_view1(&vec_pair_80165FC8_1[1], (VEC3*)setVec3(&v3, lbl_80797330, lbl_807974EC, lbl_80797330));
}

/* The unit's `.bss`: the two two-vector records `fn_80166330` seeds.  Names are GUESSes (each record is a
 * pair of model-space points). */
VEC3 vec_pair_80165FC8_0[2];  /* +0x806A7868 */
VEC3 vec_pair_80165FC8_1[2];  /* +0x806A7880 */
