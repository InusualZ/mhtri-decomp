/*
 * enemy/em015_prog.cpp - enemy 015's program: the per-tick entry and its 12-byte helper, the damage/death
 *   bookkeeping and the `(state, sub-state)` transition tables, the per-motion action steps with the `state_sub`
 *   and `action` dispatchers (`fn_80179E98`, `fn_8017F0D8`, `fn_8017F138`), and the MHchar colour/material steps.
 * RANGE. .text 0x80176C30-0x80182C40 (112 functions); extab 0x8000E6D4-0x8000E9E4, extabindex
 *   0x80029928-0x80029DC0, .ctors 0x8056F338-0x8056F33C, .rodata 0x8056FDD0-0x8056FF50, .data 0x805A94A0-0x805AA930
 *   (`em015_prog_tbl` first), .bss 0x806A79D0-0x806A7A00, .sdata2 0x80797B10-0x80797E88.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_80176C58` to `fn_80177774` and for `fn_8017799C` (retail
 *   keeps the unfused `clrlwi` + `cmpwi`/`slwi` pairs, playbook 39), on elsewhere (with it off, `fn_80178128`
 *   gains the indexed f31 restore but its `state_0x05` store gains a `clrlwi`).
 * NAMES. The file name follows the runtime dump's `em015_prog_tbl`, which opens the TU's `.data`; the map has only
 *   `fn_` stems for the functions.  Callees whose call sites disagree with the owner's header are called through
 *   `<name>_viewN` cast macros (the same direct call).
 *   The `.bss` record names (`vec_tbl_80181C88`) are GUESSes.
 * RESIDUALS. 31 rows unwritten: 0x80176C30-0x80176C58, 0x801784D0-0x80178754, 0x8017889C-0x801797F8,
 *   0x80179AC4-0x80179C38, 0x80179E98-0x8017A004, 0x8017A0BC-0x8017B60C, 0x8017B748-0x8017B990,
 *   0x8017BB34-0x8017C504, 0x8017C690-0x8017C918, 0x8017CB34-0x8017DB8C, 0x8017DF9C-0x8017E178,
 *   0x8017E2D4-0x8017EDF0, 0x8017F1F0-0x80181C88.
 *   17 partial rows, including:
 *  - `fn_8017708C`: retail lowers case 5's `{0x19,0x1A}` and case 7's `{0x2A..0x2C}`/`{0x24,0x25}` pairs to a
 *    range test (`addi r0,r4,-0x19; cmplwi r0,1; ble`) on one masked register, ours re-masks and compares twice
 *    (the 0x20-byte gap; an if-chain range test inlines the handlers and scores lower);
 *  - `fn_80178128`: retail restores f31 with `li r0,0x18; psq_lx`, ours with `psq_l f31,0x18(r1)`;
 *  - `fn_80178378`: retail tests the countdown with `subi r0,r3,1; cmpwi r0,0; bgt`, ours needs `< 1` to avoid
 *    `subic.` and then compares with 1 (`cmpwi r0,1; bge`);
 *  - `fn_801797F8`: retail stores the angle's high byte with `srawi r0,r0,8; clrlwi r0,r0,24`, ours folds it to
 *    `extrwi r0,r0,8,16` (`(u8)((s16)angle >> 8)` trades the fold for an `extsh`);
 *  - `fn_80181E24`: the peephole fuses `clrlwi.` and `clrlslwi` and drops two `clrlwi` re-masks retail keeps;
 *  - `fn_801820DC`: retail shares one tail between the action-start chains (a `goto`, rule 8); the nested `if`
 *    with two `switch` selects is the conformant shape, the rest is `cmpwi`/`cmplwi` and re-mask colouring;
 *  - `fn_80182978`: retail builds each `fn_80126278` argument as `clrlwi r0,r0,28; slwi r0,r0,8; clrlwi r4,r0,16`,
 *    MWCC folds every spelling tried into `rlwinm r4,r0,8,20,23`;
 *  - `fn_80176C58`: one compare is `cmplwi r3,4` where retail has `cmpwi`.
 *   The other 9 partial rows have no recorded cause.  `fn_80182B94` calls `copyVec3` where retail calls
 *   `assignVec3` (4 relocations: +0x30/+0x54/+0x74/+0x94); `assignVec3`'s owner declaration collides with
 *   `ef/ef_cylinder.cpp`'s spelling.
 *   flipcheck: `.ctors`/`.rodata` claimed, not emitted; `.data` 0xE0 against 0x1490, `.sdata2` 0x20 against
 *   0x378; `.text` (0x3F04 of 0xC010), extab (0x238 of 0x310) and extabindex (0x354 of 0x498) short of the claim
 *   and differing.
 * SHAPES. The armed-state step is `case 0: { u8 state = self->state; self->state = state + 1; ... }`: the copy
 *   gives retail's load/add/store, `self->state += 1` does not.  The countdown is `self->timer_0x020 -= 1;
 *   if ((s32)self->timer_0x020 < 1)`, the one spelling that does not fuse into `subic.`.  `fn_801797F8` keeps
 *   `(u8)((s32)angle >> 8)` and `fn_80182978` its `(s32)` compares (`cmpwi`).  `fn_803B9BA0` is declared here
 *   with the `(_ENEMY_WORK*, u32, u32)` spelling its call sites were measured with.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/fn_8012A354.h" /* fn_8012A354 (rule 2: the owner's header) */
#include "enemy/fn_8012999C.h" /* fn_8012999C (rule 2: the owner's header) */
#include "enemy/em_action_finish.h" /* em_action_finish (rule 2: the owner's header) */
#include "enemy/fn_80128030.h" /* fn_80128030 (rule 2: the owner's header) */
#include "enemy/fn_8013221C.h" /* fn_8013221C (rule 2: the owner's header) */
#include "enemy/fn_80132224.h" /* fn_80132224 (rule 2: the owner's header) */
#include "enemy/fn_80132264.h" /* fn_80132264 (rule 2: the owner's header) */
#include "enemy/em_move_mode_set.h" /* em_move_mode_set (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/fn_8012F810.h" /* fn_8012F810 (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/em_approach_step.h" /* em_approach_step (rule 2: the owner's header) */
#include "enemy/em_turn_seq_start.h" /* em_turn_seq_start (rule 2: the owner's header) */
#include "enemy/em_turn_seq_step.h" /* em_turn_seq_step (rule 2: the owner's header) */
#include "enemy/em_move_vec2_clr.h" /* em_move_vec2_clr (rule 2: the owner's header) */
#include "enemy/get_em_base_scale.h" /* get_em_base_scale (rule 2: the owner's header) */
#include "enemy/CancelFade.h" /* CancelFade (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "ef.h"
#include "enemy/EnemyData.h"
#include "unsplit/enemy.h"
#include "unsplit/ef.h"
#include "ef/fn_80105314.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_801251D0.h" /* em_se_tbl_play_alt/fn_80128030/fn_8012933C/fn_80129668/fn_80129724 */
#include "enemy/fn_8012BDF4.h" /* em_busy_set/em_busy_ck */
#include "enemy/fn_80176C58.h" /* fn_801775C0 */
#include "enemy/fn_80177890.h" /* fn_80177BA4 */
#include "enemy/fn_80178128.h" /* fn_8017827C (the master dispatcher's case 2) */
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h" /* em_demo_pos_set/fn_8014610C */
#include "gx.h"
#include "sound/mhchar.h"       /* MHchar, with the pointer-taking setTevKColor overload */
#include "unsplit/unknown.h"   /* system_w */
#include "enemy/fn_801251D0.h" /* fn_80126278/fn_80126324 + the 0x80129xxx helpers */
#include "enemy/fn_8012EC74.h" /* fn_8013026C */
#include "enemy/fn_8012BDF4.h" /* fn_8012E5A8 */
#include "enemy/fn_80138074.h" /* fn_8013A654/fn_8013918C + the EmUserData record */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "fn_8004CAD8.h"       /* fn_8005024C/fn_80051378/rotVecY/calcDistanceSqXZ */
#include "mh3_pad.h"           /* VEC3_ctor/copyVec3/setVec3 */
#include "sys_mem.h"           /* operator delete (the `__dl__FPv` global deleter) */
#include "stage/stg_w.h"
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)
#define fn_801823A0_view1 ((void (*)(_ENEMY_WORK*, s32))fn_801823A0)
#define fn_80128A8C_view1 ((void (*)(_ENEMY_WORK*, u32, u32))fn_80128A8C)
#define em_turn_to_target_view1 ((void (*)(_ENEMY_WORK*, u32))em_turn_to_target)
#define em_turn_seq_start_view1 ((void (*)(_ENEMY_WORK*, void*, u32, u32, u32))em_turn_seq_start)
#define em_target_pos_set_view1 ((void (*)(_ENEMY_WORK*, u32))em_target_pos_set)
#define em_state_set_view1 ((void (*)(_ENEMY_WORK*, s32, s32))em_state_set)
#define em_parts_damage_level_get_view1 ((u32 (*)(_ENEMY_WORK*, u8))em_parts_damage_level_get)
#define em_mot_set_blend_view3 ((void (*)(_ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
#define em_mot_set_blend_view1 ((void (*)(_ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
#define em_hit_window_set_view3 ((void (*)(_ENEMY_WORK*, s32, s32, s32))em_hit_window_set)
#define em_hit_window_set_view1 ((void (*)(_ENEMY_WORK*, u32, u32, u32))em_hit_window_set)
#define em_die_ck_view1 ((u32 (*)(_ENEMY_WORK*))em_die_ck)
#define em_approach_start_view1 ((void (*)(_ENEMY_WORK*, f32, s32))em_approach_start)

/* The 12-byte helper `fn_80176C58` allocates and `fn_80176E50` constructs (+0x00: `lbl_805AA900`).
 * size: 0xC (`operator new(0xC)`) */
typedef struct Helper_80176E50 {
    /* +0x0 */ void* vtbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_80176E50;

/* The helper's table, in this unit's `.data`. */
extern "C" u8 lbl_805AA900[];

/* --------------------------------------------------------------------------------------------- */
/* callees                                                                                        */
/* --------------------------------------------------------------------------------------------- */

/* The C++ free functions are declared by their real signatures, so the front-end emits the map's manglings. */
extern "C" u8 stage_map_kind_get(u8 id);
extern "C" void fn_80182978(_ENEMY_WORK* self);
extern "C" u32 quest_id_get(void);
extern "C" void fn_801823A0(_ENEMY_WORK* self, u32 a);

/* This unit's own forward declaration (fn_80176C58 calls it before its definition). */
extern "C" Helper_80176E50* fn_80176E50(Helper_80176E50* self);

/* The two mangled free functions, declared by their real signatures (rule 9: never the mangled
 * spelling - the front-end produces `em_die_ck__FP11_ENEMY_WORK` from this declaration). */

/* `em_frame_check` at global scope: the front-end emits `em_frame_check__FP11_ENEMY_WORKUsff` (rule 9). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

extern "C" {
/* Callees whose owners' headers do not declare these spellings: `lobby/fn_8030121C.cpp`'s `eft_em_spawn`
 * and `draw_shape_arm.cpp`'s `draw_shape_arm`. */
void eft_em_spawn(_ENEMY_WORK* self, u32 a, u32 b, VEC3* v, f32 s);
void draw_shape_arm(_ENEMY_WORK* self, u32 a, u32 b);

}

/* The unit's `.sdata2` and `.rodata` constants, declared: the source does not emit them yet. */
extern f32 lbl_80797B18;
extern f32 lbl_80797B28;
extern f32 lbl_80797B2C;
extern f32 lbl_80797B30;
extern f32 lbl_80797B34;
extern f32 lbl_80797B38;
extern f32 lbl_80797B3C;
extern f32 lbl_80797B40;
extern f32 lbl_80797B44;

extern f32 lbl_80797B48;
extern f32 lbl_80797B4C;
extern f32 lbl_80797B50;
extern f32 lbl_80797B54;
extern f32 lbl_80797B58;
extern f32 lbl_80797B5C;
extern u8 lbl_8056FDD0[];

extern "C" {

/* This unit's two entries the dispatcher tail-branches into, defined further down. */
void fn_80177608(_ENEMY_WORK* self);
void fn_80177774(_ENEMY_WORK* self);
}

/* Callees and pool literals, declared by their map spelling. */

/* nw4r math free functions the map carries at global scope (`...__FPQ34nw4r4math4VEC3...`). */
s32 calcVecAng2(VEC3* a, VEC3* b);
void rotVecY(VEC3* v, u32 angle);

/* Mangled enemy-service entry points (declared as C++ prototypes so the compiler mangles them). */
u16 em_get_mot_no(_ENEMY_WORK* self);
f32 get_em_chg_scale(_ENEMY_WORK* self);

extern "C" {

/* This unit's dispatch targets in 0x80177890..0x80178128. */
void fn_80177BEC(_ENEMY_WORK* self, s32 index);
void fn_80177CC8(_ENEMY_WORK* self);
void fn_80177D54(_ENEMY_WORK* self);
void fn_80177F30(_ENEMY_WORK* self, s32 index);
void fn_8017801C(_ENEMY_WORK* self, s32 index);

/* The unit's action tables and aim scale constants, declared: the source does not emit them yet. */
extern u32 lbl_8056FE10[];

extern f32 lbl_80797B60;
extern f32 lbl_80797B64;

/* `enemy/em_model.cpp`'s pose request `fn_8017E178` posts (`self`, the pose vector, the request id); its
 * callers spell it two ways, so each declares its own. */
void fn_803B9BA0(_ENEMY_WORK* self, u32 v, u32 a);
/* This unit's unwritten table writers, which the written ones call. */
void fn_801784D0(_ENEMY_WORK* self, u32 a, u32 b);
void fn_80179E98(_ENEMY_WORK* self);
void fn_8017D5A0(_ENEMY_WORK* self);
void fn_8017D778(_ENEMY_WORK* self);
void fn_8017DF9C(_ENEMY_WORK* self);
void fn_8017E2D4(_ENEMY_WORK* self);
void fn_8017E508(_ENEMY_WORK* self);
void fn_8017E948(_ENEMY_WORK* self);

/* The unit's action tables and aim scale constants, declared: the source does not emit them yet. */
extern u32 lbl_8056FE50[];
extern u32 lbl_8056FE90[];
extern u8 lbl_805AA260[];
extern u8 lbl_805AA2A0[];
extern u8 lbl_805AA2D8[];
extern u8 lbl_805AA320[];
extern u8 lbl_805AA350[];
extern u8 lbl_805AA378[];

extern f32 lbl_80797BF4;

extern f32 lbl_80797BB0;
extern f32 lbl_80797BE0;
extern f32 lbl_80797BF0;
extern f32 lbl_80797CC0;
extern f32 lbl_80797BF8;
extern f32 lbl_80797BFC;
extern f32 lbl_80797CD0;
extern f32 lbl_80797C94;
extern f32 lbl_80797C34;

extern f32 lbl_80797B20;
extern f32 lbl_80797B80;
extern f32 lbl_80797B9C;
extern f32 lbl_80797C1C;
extern f32 lbl_80797C68;
extern f32 lbl_80797B88;
extern f32 lbl_80797CBC;

extern f32 lbl_80797D28;
extern f32 lbl_80797D2C;
extern f32 lbl_80797DC8;
extern f32 lbl_80797D34;
extern f32 lbl_80797D38;
extern f32 lbl_80797D50;
extern f32 lbl_80797DB8;
extern f32 lbl_80797D88;
extern f32 lbl_80797DBC;
extern f32 lbl_80797DC0;
extern f32 lbl_80797DC4;

extern f32 lbl_80797B68;
extern f32 lbl_80797B6C;
extern f32 lbl_80797B70;
}

/* One 0x16-byte action record `fn_80182AB8` builds (the +0x00 type word, a VEC3 and three scalars).
 * size: 0x16 */
struct EmWorkItem {
    /* +0x00 */ u32 type_0x00;
    /* +0x04 */ VEC3 vec_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ s16 field_0x12;
    /* +0x14 */ s16 field_0x14;
};

extern "C" {
/* `stage_map_kind_get` (the byte-table map lookup) comes from `stage/stg_w.h`. */

/* ------------------------------------------------------------------------------------------------
 * This unit's own functions, declared up front so the later ones can call forward.
 * ------------------------------------------------------------------------------------------------ */
f32 fn_80181C88(f32 value, f32 center, f32 step);
void fn_80181CC0(_ENEMY_WORK* self);
void fn_80181E24(_ENEMY_WORK* self);
u32 fn_80182040(_ENEMY_WORK* self);
void fn_80182080(_ENEMY_WORK* self, u32 part);
u32 fn_801820DC(_ENEMY_WORK* self, u32 arg);
void fn_80182320(_ENEMY_WORK* self);
void fn_801823A0(_ENEMY_WORK* self, u32 value);
void fn_801823C0(EmUserData* self);
u32 fn_80182430(_ENEMY_WORK* self, u32 arg);
u32 fn_8018257C(_ENEMY_WORK* self);
u32 fn_801825A4(_ENEMY_WORK* self, u32 kind);
void fn_80182768(_ENEMY_WORK* self, u8* out_a, u8* out_b);
void fn_80182914(_ENEMY_WORK* self);
u32 fn_80182918(_ENEMY_WORK* self);
void fn_80182978(_ENEMY_WORK* self);
void fn_80182AB8(struct EmWorkItem* out, u32 a, s16 b, s16 c);
void* fn_80182B38(void* p, s16 arg);
void fn_80182B94(void);

/* More of the unit's pool literals, declared, not defined. */
extern f32 lbl_80797B10;
extern f32 lbl_80797B14;

extern f32 lbl_80797BB4;
extern f32 lbl_80797BE4;
extern f32 lbl_80797C14;
extern f32 lbl_80797C24;

extern f32 lbl_80797C90;
extern f32 lbl_80797D08;
extern f32 lbl_80797E6C;

extern f32 lbl_80797E78;
extern f32 lbl_80797E7C;
extern f32 lbl_80797E80;

/* The two argument records `fn_8012A014` takes (the unit's `.data`). */
extern u32 lbl_805A950C[];
extern u32 lbl_805A9518[];
/* The four 0xC-byte vectors `fn_80182B94` seeds. */
extern nw4r::math::VEC3 vec_tbl_80181C88[4];
}

#pragma peephole off

/* --------------------------------------------------------------------------------------------- */
/* functions, in address order                                                                    */
/* --------------------------------------------------------------------------------------------- */
extern "C" void fn_80176C58(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;
    Helper_80176E50* helper;
    u8 kind;
    u8 mode;

    VEC3_ctor(&v);
    if (em_res_user_data_ck(self) == 0) {
        helper = (Helper_80176E50*)operator new(0xC);
        if (helper != 0) {
            fn_80176E50(helper);
        }
        em_res_user_data_set(self, helper);
    }
    fn_80182978(self);
    self->field_0x1E4 = 0;
    mode = arg;
    if ((u32)(mode - 1) <= 1 || mode == 4) {
        /* these modes do not run the motion hand-off */
    } else {
        kind = stage_map_kind_get(self->field_0x1E0);
        switch (kind) {
          case 1:
            if (self->area_no == 5) {
                em_move_mode_set(self, 0);
                fn_80128A8C_view1(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                fn_80128A8C_view1(self, 0, 4);
            }
            break;
          case 8:
            em_move_mode_set(self, 0);
            fn_80128A8C_view1(self, 0, 0);
            break;
          case 9:
            if (self->area_no == 0) {
                em_move_mode_set(self, 0);
                fn_80128A8C_view1(self, 0, 0);
            } else {
                em_move_mode_set(self, 2);
                fn_80128A8C_view1(self, 0, 4);
            }
            break;
          default:
            em_move_mode_set(self, 2);
            fn_80128A8C_view1(self, 0, 4);
            break;
        }
    }
    if ((u16)quest_id_get() == 0x3EC) {
        self->field_0x7B0 = 0.5f;
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, -80.0f, 90.0f);
        fn_801057A4(self, 0x1A, &v, 1.5f, 0x18E4);
        fn_8010A7D4(self, 1);
    }
}

extern "C" Helper_80176E50* fn_80176E50(Helper_80176E50* self) {
    em_res_user_data_ctor(self);
    self->vtbl = lbl_805AA900;
    return self;
}

extern "C" void fn_80176E8C(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (state[0]) {
      case 2:
        switch (sub[0]) {
          case 0:
          case 3:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 4;
            }
            break;
          case 5:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 7;
            }
            break;
          case 6:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 8;
            }
            break;
          case 9:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xA;
            }
            break;
        }
        break;
      case 5:
        switch (sub[0]) {
          case 2:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xD;
            }
            break;
          case 0xA:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xE;
            }
            break;
          case 0xB:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0xF;
            }
            break;
          case 0x1F:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0x20;
            }
            break;
          case 0x24:
            if (em_mot_finished_ck_c1() == 1) {
                sub[0] = 0x25;
            }
            break;
        }
        break;
      case 7:
        switch (sub[0]) {
          case 6:
            if (fn_8012EC3C(self) == 1) {
                sub[0] = 0x17;
            }
            break;
          case 7:
          case 0xF:
            if (fn_8012EC3C(self) == 1) {
                sub[0] = 0x18;
            }
            break;
          case 0x22:
            if (em_mot_finished_ck_c1() == 1) {
                state[0] = 2;
                sub[0] = 4;
            }
            break;
        }
        break;
    }
}

extern "C" void fn_8017708C(_ENEMY_WORK* self, u32 kind, u32 sub) {
    s32 step;
    s32 limit;

    switch ((u8)kind) {
      case 1:
        switch ((u8)sub) {
          case 6:
            fn_80130F74(self);
            break;
          case 7:
            fn_801376B4(self);
            break;
        }
        break;
      case 5: {
        int s = (u8)sub;

        switch (s) {
          case 0x19:
          case 0x1A:
            fn_8012A354(self);
            break;
          case 0x1D:
            fn_80130F74(self);
            break;
          case 0x26:
            fn_801376B4(self);
            break;
        }
        break;
      }
      case 7: {
        int s = (u8)sub;

        switch (s) {
          case 6: {
            u8 v;

            step = 0x32;
            if (self->field_0x7C8 >= 0x29) {
                step = 0x64;
            }
            v = em_parts_damage_level_get_view1(self, 3);
            limit = 0xFF;
            if (v >= 3) {
                limit = 0x63;
            }
            if ((s32)self->field_0x1E4 < limit - step) {
                self->field_0x1E4 += (u8)step;
            } else {
                self->field_0x1E4 = (u8)limit;
            }
            break;
          }
          case 7:
          case 0xF:
            if (self->field_0x1E4 > 0x64) {
                self->field_0x1E4 -= 0x64;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
          case 0x24:
          case 0x25:
          case 0x2A:
          case 0x2B:
          case 0x2C:
            if (self->field_0x1E4 > 0x1E) {
                self->field_0x1E4 -= 0x1E;
            } else {
                self->field_0x1E4 = 0;
            }
            break;
        }
        break;
      }
      case 0xA:
        switch ((u8)sub) {
          case 0xB1:
            fn_8012999C(self);
            break;
          case 0xC9:
          case 0xD1:
            em_part_hit_set(self, 0, 0);
            break;
        }
        break;
    }
}

extern "C" void fn_80177258(_ENEMY_WORK* self) {
    fn_80182978(self);
    if (em_die_ck_view1(self) == 0) {
        if (fn_8012EC3C(self) == 1) {
            if (self->field_0x1E2 == 0) {
                self->field_0x32F = 1;
                self->field_0x330 = 1;
            }
        } else {
            self->field_0x32F = 0;
        }
        if (self->field_0x38F != 0) {
            if (self->timer_0x332 < 0x384) {
                self->timer_0x332 += 1;
            }
        } else {
            self->timer_0x332 = 0;
        }
    }
    if (self->field_0x1E2 == 2) {
        self->field_0x38A = 1;
    } else {
        self->field_0x38A = 0;
    }
}

extern "C" void fn_80177314(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 1, 0x14, 0, 1);
        break;
      case 1:
        em_target_pos_set_view1(self, 0);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801773B0(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x14, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80177430(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F5C4(self, 0x1D, 0x14, 0, 1);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_801774B0(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x28, 0x28, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_80177540(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 2);
        fn_8012F5C4(self, 0x36, 0x1E, 0, 3);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

extern "C" void fn_801775C0(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80177314(self);
        break;
      case 1:
        fn_801773B0(self);
        break;
      case 2:
        fn_80177430(self);
        break;
      case 4:
        fn_801774B0(self);
        break;
      case 5:
        fn_80177540(self);
        break;
    }
}

/* The map names these two with a bare `fn_XXXXXXXX` stem (no mangling), so they take C linkage to
 * emit that exact symbol; rule 9 keeps an `fn_` stem legal. */
extern "C" void fn_80177608(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 0xA, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        setVector3(&v, lbl_80797B18, lbl_80797B28, lbl_80797B2C);
        if (em_frame_check(self, 0, lbl_80797B30, lbl_80797B18) == 1U) {
            eft_em_spawn(self, 0, 0x18, &v, lbl_80797B34);
        }
        if (em_frame_check(self, 0, lbl_80797B38, lbl_80797B18) == 1U) {
            draw_shape_arm(self, 0x1A, 0xA);
            em_hit_window_set_view1(self, 0, 0x1C, 5);
        }
        if (em_frame_check(self, 3, lbl_80797B3C, lbl_80797B40) == 1U) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x18, &v, lbl_80797B34);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" void fn_80177774(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state++;
            em_mot_set(self, 4, 0, 0);
            self->timer_0x020 = 0x708;
            fn_80132224(self);
        }
        break;
    case 2:
        fn_8013221C(self, lbl_80797B44, 1, 0xF);
        self->timer_0x020 = self->timer_0x020 - 1;
        if ((s32)self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 5, 4, 0);
            fn_80132264(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

#pragma peephole on

extern "C" {
/* ---- the motion-state updates ---- */

void fn_80177890(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 0x14, 0x14, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80797B48, lbl_80797B18) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_8017791C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 7, 0xa, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
}

#pragma peephole off

extern "C" {
void fn_8017799C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 0xa, 0);
        self->timer_0x020 = 0x12c;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_state_set_view1(self, 1, 6);
        }
        break;
    }
}
}

#pragma peephole on

extern "C" {
void fn_80177A2C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x82, 0xa, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177AA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xa, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177B24(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend_view1(self, 0x1d, 0x14, 0, 1);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The dispatcher: `state_sub` selects the motion update; cases 0/1 make it a dense 0..7 switch, which
 * MWCC lowers to the target's jump table. */
void fn_80177BA4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80177608(self);
        break;
    case 1:
        fn_80177774(self);
        break;
    case 2:
        fn_80177890(self);
        break;
    case 3:
        fn_8017791C(self);
        break;
    case 4:
        fn_8017799C(self);
        break;
    case 5:
        fn_80177A2C(self);
        break;
    case 6:
        fn_80177AA8(self);
        break;
    case 7:
        fn_80177B24(self);
        break;
    }
}

void fn_80177BEC(_ENEMY_WORK* self, s32 arg) {
    fn_801823A0_view1(self, 1);

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        fn_8012F810(self);
        em_mot_set_blend_view1(self, 0x15, 0x28, 0, 1);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, lbl_80797B18, 0);
            break;
        case 1:
            em_approach_start_view1(self, lbl_80797B4C, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177CC8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056FDD0, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, lbl_8056FDD0) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177D54(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1b, 6, 0);
        em_hit_window_set_view3(self, 0, 0x2e, 8);
        em_hit_window_set_view3(self, 1, 0x32, 0x18);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x2f, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x30, 0x18);
            }
            break;
        case 2:
            if (self->field_0xA0D == 0) {
                self->state_0x006++;
                em_hit_window_set_view3(self, 0, 0x31, 0x18);
            }
            break;
        }

        switch (self->state_0x007) {
        case 0:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                em_hit_window_set_view3(self, 1, 0x33, 0x18);
            }
            break;
        case 1:
            if (self->field_0xA69 == 0) {
                self->state_0x007++;
                em_hit_window_set_view3(self, 1, 0x34, 0x18);
            }
            break;
        }

        if (em_frame_check(self, 1, lbl_80797B50, lbl_80797B54) == 1) {
            em_turn_to_target_view1(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_80177F30(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0xa, 0);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, lbl_80797B18, 0);
            break;
        case 1:
            em_approach_start_view1(self, lbl_80797B18, 0);
            if (self->value_0x378 > lbl_80797B58) {
                self->value_0x378 = lbl_80797B58;
            }
            /* falls through to case 2's easing */
        case 2:
            em_approach_start_view1(self, lbl_80797B5C, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_8017801C(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 9, 0xa, 0);
        switch ((u8)arg) {
        default:
            em_approach_start_view1(self, lbl_80797B18, 0);
            break;
        case 1:
            em_approach_start_view1(self, lbl_80797B4C, 0);
            break;
        case 2:
            em_approach_start_view1(self, lbl_80797B18, 0);
            if (self->value_0x378 > lbl_80797B58) {
                self->value_0x378 = lbl_80797B58;
            }
            break;
        case 3:
            em_approach_start_view1(self, lbl_80797B5C, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        break;
    }
}
}

/* Two-phase enemy action: arm the action, then aim at the target for motion ids 0x1F/0x20. */
extern "C" void fn_80178128(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_turn_seq_start_view1(self, lbl_8056FE10, 0, 1, 0);
        em_move_vec2_clr(self);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            s32 ang = calcVecAng2(&self->pos, &self->vec_0x36C);
            u16 rel = (u16)(ang - self->field_0x1C0);

            self->offset_0x30C.vec_0x310.z = lbl_80797B60 * get_em_base_scale(self) * get_em_chg_scale(self);
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + rel);
        }
        break;
    }
    case 1: {
        u32 done = em_turn_seq_step(self, lbl_8056FE10);

        u16 mot = em_get_mot_no(self);
        if (mot - 0x1f <= 1U) {
            if (em_frame_check(self, 3, lbl_80797B50, lbl_80797B64) == 1) {
                CancelFade(self);
            }
        }

        if (done == 1) {
            em_action_finish(self);
        }
        break;
    }
    }
}

/* Tail-calls this action's per-sub-state handler. */
extern "C" void fn_8017827C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_80177BEC(self, 0);
        break;
    case 1:
        fn_80177CC8(self);
        break;
    case 2:
        fn_80177D54(self);
        break;
    case 3:
        fn_80177F30(self, 0);
        break;
    case 4:
        fn_8017801C(self, 0);
        break;
    case 5:
        fn_80177BEC(self, 1);
        break;
    case 6:
        fn_80177F30(self, 1);
        break;
    case 7:
        fn_8017801C(self, 1);
        break;
    case 8:
        fn_8017801C(self, 2);
        break;
    case 9:
        fn_80177F30(self, 2);
        break;
    case 10:
        fn_8017801C(self, 3);
        break;
    case 11:
        fn_80178128(self);
        break;
    }
}

/* Two-phase action: arm the timer action, then finish on `em_mot_end_ck`. */
extern "C" void fn_801782F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend_view3(self, 0x32, 0x28, 0, 3);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms/steps an enemy action, aiming at the target once the arming frame reports 1. */
extern "C" void fn_80178378(_ENEMY_WORK* self, u32 flag) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FE50, 0, 1, 0);
        em_move_vec_clr(self);
        if ((u8)flag == 1) {
            s32 ang = calcVecAng2(&self->pos, &self->vec_0x36C);
            setVector3(&self->offset_0x30C.vec_0x310, lbl_80797B18, lbl_80797B18, lbl_80797B68);
            rotVecY(&self->offset_0x30C.vec_0x310, ang);
            self->timer_0x020 = 0x28;
        }
        break;
    }
    case 1:
        if ((u8)flag == 1 && self->state_0x006 == 0) {
            em_move_offset_apply(self);
            self->timer_0x020 -= 1;
            if ((s32)self->timer_0x020 < 1) {
                self->state_0x006 = self->state_0x006 + 1;
            }
        }
        if (em_turn_seq_step(self, lbl_8056FE50) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* Arms the 0x3A/0x0A motion, then finishes on `em_mot_end_ck`. */
extern "C" void fn_80178754(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The timed-table variant: same arm, then the table's own completion test. */
extern "C" void fn_801787E4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FE90, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_8056FE90) == 1) {
            fn_80128030(self);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* Arms the 0x2E/0x14 motion, then finishes. */
extern "C" void fn_801798EC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x2E, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Three-phase: arm 0x31/0x14, step to 0x21/0, then `em_state_set(self, 5, 0x1D)`. */
extern "C" void fn_8017996C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 5, 0x1D);
        }
        break;
    }
}

/* Resets the shared action state, arms 0x3D/0 and the (2, 7) motion pair. */
extern "C" void fn_80179A2C(_ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x3D, 0, 0);
        em_camera_req(self, 2, 7);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the 0x68/4 motion (`em_mot_set` four-argument form), then finishes. */
extern "C" void fn_80179C38(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x68, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x22/4 motion, then finishes. */
extern "C" void fn_80179CB4(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x22, 4, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x36/0x1E motion, then finishes. */
extern "C" void fn_80179E18(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x36, 0x1E, 0, 1);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x4D/0x14 and plays `em_hit_window_set_default(self, 0, 1)` while the frame counter runs. */
extern "C" void fn_8017A004(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x4D, 0x14, 0, 1);
        em_hit_window_set_default(self, 0, 1);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797BF4, lbl_80797B18) == 0) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x50/0xA motion and two joint-pair programs keyed on `phase_0x06`/`step_0x07`. */
extern "C" void fn_8017B60C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x50, 0xA, 0);
        em_hit_window_set(self, 0, 7, 8);
        em_hit_window_set(self, 1, 0x20, 0x18);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, lbl_80797B50, lbl_80797B18) == 1) {
            em_turn_to_target(self, 0x30);
        }
        if (self->state_0x006 == 0 && self->field_0xA0D == 0) {
            em_hit_window_set(self, 0, 0x18, 0x18);
        }
        if (self->state_0x007 == 0 && self->field_0xA69 == 0) {
            em_hit_window_set(self, 1, 0x21, 0x18);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x52/4, then two mirrored `em_turn_in_window` poses around a 0x30 fade. */
extern "C" void fn_8017B990(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        fn_80130CDC(self, -5);
        em_mot_set(self, 0x52, 4, 0);
        em_hit_window_set_default(self, 0, 9);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, lbl_80797B18, lbl_80797C1C) == 1) {
            em_turn_to_target(self, 0x30);
        }
        em_turn_in_window(self, lbl_80797C1C, lbl_80797B9C, -0x4000);
        em_turn_in_window(self, lbl_80797B2C, lbl_80797C68, 0x4000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x51/0xA and two joint programs, then `em_turn_in_window(self, 0x8000, ...)`. */
extern "C" void fn_8017BA78(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x51, 0xA, 0);
        em_hit_window_set(self, 0, 0xE, 8);
        em_hit_window_set(self, 1, 0xF, 0x10);
        break;
    }
    case 1:
        em_turn_in_window(self, lbl_80797B20, lbl_80797B80, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x4E/0xA and two joint programs, with the fade on the first frame check. */
extern "C" void fn_8017C504(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x10, 8);
        em_hit_window_set(self, 1, 0x11, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797B88, lbl_80797B18) == 0) {
            fn_80136D4C(self, lbl_80797B70);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms the 0x58/4 motion and `em_hit_window_set_default(self, 0, 0x15)`, then finishes. */
extern "C" void fn_8017C5DC(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x58, 4, 0);
        em_hit_window_set_default(self, 0, 0x15);
        break;
    }
    case 1:
        if (em_frame_check(self, 3, lbl_80797CBC, lbl_80797B64) == 1) {
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x25/6, two `em_hit_window_set_default` cues and the 0x180/0x280 viewport pair. */
extern "C" void fn_8017C918(_ENEMY_WORK* self) {
    VEC3 vec;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        fn_80130CDC(self, -0xA);
        em_mot_set(self, 0x25, 6, 0);
        em_hit_window_set_default(self, 0, 0x19);
        em_hit_window_set_default(self, 1, 0x1A);
        break;
    }
    case 1:
        if (em_frame_check(self, 2, lbl_80797BB0, lbl_80797B18) == 1) {
            fn_80133CC8(self, 0x180, 0x280);
        }
        if (em_frame_check(self, 2, lbl_80797CC0, lbl_80797B18) == 1) {
            fn_80133C3C(self);
        }
        em_turn_in_window(self, lbl_80797BF8, lbl_80797BFC, 0x8000);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x59/6 and one of three 0x... pose constants chosen by the caller's selector. */
extern "C" void fn_8017CA38(_ENEMY_WORK* self, u32 selector) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x59, 6, 0);
        em_hit_window_set(self, 0, 0x1B, 2);
        switch ((u8)selector) {
        default:
            em_approach_start(self, lbl_80797CD0, 0);
            break;
        case 1:
            em_approach_start(self, lbl_80797B18, 0);
            break;
        case 2:
            em_approach_start(self, lbl_80797B9C, 0);
            break;
        }
        break;
    }
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_hit_window_clear(self, 0);
            em_action_finish(self);
        }
        break;
    }
}

/* Picks the program table by `state_sub`. */
extern "C" void fn_8017DB8C(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, lbl_805AA260, 0, 0);
        break;
    case 10:
        em_se_tbl_play_alt(self, lbl_805AA2A0, 2, 0xA);
        break;
    case 15:
        em_se_tbl_play_alt(self, lbl_805AA2D8, 0, 0xF);
        break;
    case 26:
        em_se_tbl_play_alt(self, lbl_805AA320, 0, 0x1A);
        break;
    case 27:
        em_se_tbl_play_alt(self, lbl_805AA350, 2, 0x1B);
        break;
    case 28:
        em_se_tbl_play_alt(self, lbl_805AA378, 0, 0x1C);
        break;
    default:
        em_se_tbl_play_alt(self, lbl_805AA260, 0, 0);
        break;
    }
}

/* Resets the action, then re-arms the 0x37/0x14 motion with both selectors 0. */
extern "C" void fn_8017DC50(_ENEMY_WORK* self) {
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    fn_801784D0(self, 0, 0);
}

/* Runs `fn_8017DC50` while the sub-state is 0. */
extern "C" void fn_8017DC94(_ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        fn_8017DC50(self);
    }
}

/* The table variant: arm the 0x80178378 action table, then switch to 0xD/1. */
extern "C" void fn_8017DCA8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_turn_seq_start(self, lbl_8056FE50, 0, 1, 0);
        break;
    }
    case 1:
        if (em_turn_seq_step(self, lbl_8056FE50) == 1) {
            em_state_set(self, 0xD, 1);
        } else if (self->field_0x482 != 0) {
            fn_80136D4C(self, lbl_80797B6C);
        } else {
            fn_80136D4C(self, lbl_80797B70);
        }
        break;
    }
}

/* Three-phase: 0x37/0x14, then 0x2F/0x28 once `em_approach_step` and `fn_8012F948` agree. */
extern "C" void fn_8017DD68(_ENEMY_WORK* self) {
    fn_801823A0(self, 1);
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x37, 0x14, 0, 1);
        em_approach_start(self, lbl_80797B4C, 0x12);
        break;
    }
    case 1: {
        u32 done = em_approach_step(self, 0, 0x80);

        em_mot_speed_set(self, lbl_80797C94);
        fn_80136D4C(self, lbl_80797C34);
        if (done == 1 && fn_8012F948(self) == 0) {
            self->state = self->state + 1;
            em_mot_set_blend(self, 0x2F, 0x28, 0, 1);
        }
        break;
    }
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 2);
        }
        break;
    }
}

/* Arms 0x4E/0xA and two joint programs; the 0xD/3 switch needs `fn_80182430`. */
extern "C" void fn_8017DE8C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x4E, 0xA, 0);
        em_hit_window_set(self, 0, 0x29, 8);
        em_hit_window_set(self, 1, 0x2A, 0x10);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797B88, lbl_80797B18) == 0) {
            fn_80136D4C(self, lbl_80797B70);
            em_turn_to_target(self, 0x80);
        }
        if (em_mot_end_ck(self) == 1 && em_busy_ck(self) == 1) {
            if (fn_80182430(self, 0) == 1) {
                em_state_set(self, 0xD, 3);
            } else {
                fn_80128030(self);
            }
        }
        break;
    }
}

/* Three-phase: 0x31/0x14 plus the `fn_803B9BA0` pose request, then 0x21/0. */
extern "C" void fn_8017E178(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x31, 0x14, 0, 1);
        fn_803B9BA0(self, (u32)&self->pos, 0x32);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 0xD, 5);
        }
        break;
    }
}

/* Arms 0x21/4 with the 0x3E8 timer, then finishes. */
extern "C" void fn_8017E248(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x21, 4, 0, 1);
        fn_80130CDC(self, 0x3E8);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x28/0 with the two `em_demo_pos_set`/`em_demo_rot_set` pose pairs. */
extern "C" void fn_8017EDF0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_demo_pos_set(self, lbl_80797DC4, lbl_80797D28, lbl_80797D2C);
        em_demo_rot_set(self, lbl_80797B18, lbl_80797DC8, lbl_80797B18);
        em_mot_set(self, 0x28, 0, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x14/0, then the two pose pairs in the other order, ending on `em_action_finish`. */
extern "C" void fn_8017EE94(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 0, 0);
        em_demo_pos_set(self, lbl_80797D34, lbl_80797B18, lbl_80797D38);
        em_demo_rot_set(self, lbl_80797B18, lbl_80797D50, lbl_80797B18);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x28/0 and the stage-side `fn_802B1FEC`, then the two pose pairs. */
extern "C" void fn_8017EF38(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x28, 0, 0);
        fn_802B1FEC();
        em_demo_pos_set(self, lbl_80797DB8, lbl_80797D88, lbl_80797DBC);
        em_demo_rot_set(self, lbl_80797B18, lbl_80797DC0, lbl_80797B18);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Arms 0x79/4, then finishes on `em_action_finish`. */
extern "C" void fn_8017EFE0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x79, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms 0x64/4, then finishes. */
extern "C" void fn_8017F05C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x64, 4, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* The second action-table dispatcher: `state_sub` (0..13) selects a writer, and nothing runs for a value outside
 * the table (the target's `bgtlr`). */
extern "C" void fn_8017F0D8(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8017DCA8(self);
        break;
    case 1:
        fn_8017DD68(self);
        break;
    case 2:
        fn_8017DE8C(self);
        break;
    case 3:
        fn_8017DF9C(self);
        break;
    case 4:
        fn_8017E178(self);
        break;
    case 5:
        fn_8017E248(self);
        break;
    case 6:
        fn_8017E2D4(self);
        break;
    case 7:
        fn_8017E508(self);
        break;
    case 8:
        fn_8017E948(self);
        break;
    case 9:
        fn_8017EDF0(self);
        break;
    case 10:
        fn_8017EE94(self);
        break;
    case 11:
        fn_8017EF38(self);
        break;
    case 12:
        fn_8017EFE0(self);
        break;
    case 13:
        fn_8017F05C(self);
        break;
    }
}

/* The master selector on `action_0x1E5`: actions 0, 1, 2, 5, 7 and 10..13 go to their dispatchers, the rest
 * run `em_action_finish`; the trailing pair ends the frame. */
extern "C" void fn_8017F138(_ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801775C0(self);
        break;
    case 1:
        fn_80177BA4(self);
        break;
    case 2:
        fn_8017827C(self);
        break;
    case 5:
        fn_80179E98(self);
        break;
    case 7:
        fn_8017D5A0(self);
        break;
    case 10:
        fn_8017D778(self);
        break;
    case 11:
        fn_8017DB8C(self);
        break;
    case 12:
        fn_8017DC94(self);
        break;
    case 13:
        fn_8017F0D8(self);
        break;
    default:
        em_action_finish(self);
        break;
    }
    if (self->field_0x1E2 == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
}

/* Arms 0x2D/6, then aims the turn rate at the target: the angle difference picks one of three `step_0x07` turn
 * rates (`0`, `0x80`, or the difference's own high byte). */
extern "C" void fn_801797F8(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        u16 angle;

        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set(self, 0x2D, 6, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle > 0x8000) {
            if (angle > 0xC000) {
                self->state_0x007 = 0;
            } else {
                self->state_0x007 = 0x80;
            }
        } else {
            self->state_0x007 = (u8)((s32)angle >> 8);
        }
        em_busy_set(self);
        break;
    }
    case 1:
        em_turn_in_window(self, lbl_80797BB0, lbl_80797BE0, self->state_0x007 << 8);
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        } else {
            em_busy_set(self);
        }
        break;
    }
}

/* Arms 0x3A/0xA, then re-arms 0x3A/0x18/0x5C when the frame counter reports 1. */
extern "C" void fn_80179D34(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0: {
        u8 state = self->state;
        self->state = state + 1;
        em_move_mode_set(self, 2);
        em_mot_set_blend(self, 0x3A, 0xA, 0, 1);
        em_hit_window_set_default(self, 0, 0x25);
        break;
    }
    case 1:
        if (em_frame_check(self, 1, lbl_80797BF0, lbl_80797B18) == 1) {
            self->state = self->state + 1;
            em_hit_window_clear(self, 0);
            em_mot_set_blend(self, 0x3A, 0x18, 0x5C, 1);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            fn_80128030(self);
        }
        break;
    }
}

/* Steps a scalar toward `center` by `step`, never overshooting it. */
f32 fn_80181C88(f32 value, f32 center, f32 step) {
    if (value < center) {
        value += step;
        if (value < center) {
            return value;
        }
    } else if (value > center) {
        if (value > center + step) {
            return value - step;
        }
    }
    return center;
}

/* Drives the colour scalar and the three K-colour bytes toward the mode's targets (`fn_80182918`'s damage
 * level picks the byte targets). */
void fn_80181CC0(_ENEMY_WORK* self) {
    f32 target = (self->field_0x1E2 == 2) ? lbl_80797B10 : lbl_80797B9C;
    self->color_0x328.field_0x328 = fn_80181C88(self->color_0x328.field_0x328, target, lbl_80797E6C);
    u8 level = (u8)fn_80182918(self);
    f32 byte_a;
    f32 byte_b;
    if (level == 1) {
        target = lbl_80797BB4;
        byte_a = lbl_80797B20;
        byte_b = lbl_80797C24;
    } else if (level == 2) {
        target = lbl_80797B9C;
        byte_a = lbl_80797B40;
        byte_b = lbl_80797B10;
    } else {
        target = lbl_80797B18;
        byte_a = target;
        byte_b = target;
    }
    self->color_0x328.field_0x32C =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32C, target, lbl_80797C14);
    self->color_0x328.field_0x32D =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32D, byte_a, lbl_80797C14);
    self->color_0x328.field_0x32E =
        (u8)fn_80181C88((f32)self->color_0x328.field_0x32E, byte_b, lbl_80797C14);
}

/* The MHchar K-colour material update: rebuild the four channel colours from the work's scalar and byte colours,
 * then set the fifth from the aim/special state. */
void fn_80181E24(_ENEMY_WORK* self) {
    _GXColor color;
    fn_80181CC0(self);
    color.r = (u8)(s32)self->color_0x328.field_0x328;
    color.g = (u8)(s32)self->color_0x328.field_0x328;
    color.b = (u8)(s32)self->color_0x328.field_0x328;
    color.a = 0xFF;
    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR1, &color);
    if ((self->flags_0x836 & 1) != 0) {
        if (self->field_0x48F == 1) {
            color.r = 0xFF;
            color.g = 0xFF;
            color.b = 0xFF;
        } else {
            color.r = 100;
            color.g = 100;
            color.b = 100;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(4, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR3, &color);
    color.r = self->color_0x328.field_0x32C;
    color.g = self->color_0x328.field_0x32D;
    color.b = self->color_0x328.field_0x32E;
    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR3, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (fn_8012EC3C(self) == 1) {
        color.a = 0;
    } else if (em_alt_mode_ck(self) == 1) {
        color.a = (u8)((s32)(lbl_80797BB0 * (lbl_80797B14 *
                    (lbl_80797B34 + fn_8005024C((u16)(system_w.field_0x0c << 13))))) + 225);
    } else {
        color.a = (u8)((s32)(lbl_80797BB0 * (lbl_80797B14 *
                    (lbl_80797B34 + fn_8005024C((u16)(system_w.field_0x0c << 13))))) + 135);
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, &color);
}

/* True for the dead mode (0x1E2 == 2) while the record has not latched its aim state. */
u32 fn_80182040(_ENEMY_WORK* self) {
    if (self->field_0x1E2 == 2 && em_alt_mode_ck(self) == 0) {
        return 1;
    }
    return 0;
}

/* Clamps the 0x1E4 counter: part 3 at full damage caps it at 99. */
void fn_80182080(_ENEMY_WORK* self, u32 part) {
    if ((part & 0xFF) == 3) {
        if ((u8)em_parts_damage_level_get(self, 3) == 3) {
            if (self->field_0x1E4 >= 100) {
                self->field_0x1E4 = 99;
            }
        }
    }
}

/* The area/action start predicate: true when the record's map/area state arms the next action, either by setting
 * the 0x1FC/0x1FE/0x1FF request or by running the 0x8012A014 test. */
u32 fn_801820DC(_ENEMY_WORK* self, u32 arg) {
    u8 mode = (u8)stage_map_kind_get(self->field_0x1E0);
    if (mode != 1 && mode != 3) {
        return 0;
    }
    u32 armed = 0;
    u32 probe;
    switch (mode) {
    case 1:
        probe = 8;
        break;
    case 3:
        probe = 6;
        break;
    default:
        probe = 0xFF;
        break;
    }
    if (probe != 0xFF) {
        u8 state = (u8)fn_80129DB8(self);
        switch (state) {
        case 1:
            armed = 1;
            break;
        case 2:
            return 1;
        default:
            break;
        }
    }
    if (armed == 0) {
        if (self->field_0x1FC == 1 && self->field_0x1FE == 8) {
            return 1;
        }
        if (self->value_0x452 >= self->field_0x450 || self->field_0x43D != 1) {
            if (fn_8012EC3C(self) == 1 && self->color_0x328.field_0x32F == 0) {
                if (mode == 1) {
                    if ((s32)self->area_no == 7) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 12;
                        return 1;
                    }
                } else if (mode == 3) {
                    if ((s32)self->area_no == 4 || (s32)self->area_no == 8) {
                        self->field_0x1FC = 1;
                        self->field_0x1FE = 8;
                        self->field_0x1FF = 3;
                        return 1;
                    }
                }
            } else if (self->color_0x328.field_0x330 != 0) {
                u32 sel;
                switch (mode) {
                case 1:
                    sel = 8;
                    break;
                case 3:
                    sel = 3;
                    break;
                default:
                    sel = 0xFF;
                    break;
                }
                if (fn_8012A014(self, 23, sel, (u16)arg, lbl_805A950C, lbl_805A9518) == 1) {
                    return 1;
                }
            }
        }
    }
    if (fn_80129A70(self, (u16)arg) == 1) {
        return 1;
    }
    return fn_8012A204(self) == 1;
}

/* The area's follow-up arming: when the action has run out (not action 10, 0x81A <= 0) pick the two motion ids
 * the 0x1E2 mode names. */
void fn_80182320(_ENEMY_WORK* self) {
    fn_80128B80(self);
    if (self->action != 10 && self->field_0x81A <= 0) {
        if (self->field_0x1E2 == 0) {
            fn_80128AEC(self, 13, 12);
        } else if (self->field_0x1E2 == 2) {
            fn_80128AEC(self, 13, 13);
        }
    }
}

/* Sets the part flag from a one-value selector. */
void fn_801823A0(_ENEMY_WORK* self, u32 value) {
    if (value == 1) {
        self->part_0x740.field_0x740 = 1;
    } else {
        self->part_0x740.field_0x740 = 0;
    }
}

/* Resets the work's part state after the user-data teardown. */
void fn_801823C0(EmUserData* self) {
    fn_8013A654((_ENEMY_WORK*)self, 5);
    self->work_0x04->part_0x740.field_0x740 = 1;
    self->work_0x04->part_0x740.field_0x742 = 0;
    self->work_0x04->part_0x740.field_0x744 = 0;
    self->work_0x04->part_0x740.field_0x746 = 0;
    self->work_0x04->part_0x740.field_0x748 = 0;
    self->work_0x04->part_0x740.field_0x74A = 0;
    self->work_0x04->part_0x740.field_0x74C = 0;
}

/* Is the work within the aimed target's attack range: compare the squared XZ distance to the rotated
 * offset point against the scaled range. */
u32 fn_80182430(_ENEMY_WORK* self, u32 arg) {
    VEC3 a;
    VEC3 b;
    VEC3 rel;
    VEC3 probe;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    _ENEMY_WORK* target = fn_80131034(self, 23, 0);
    if (target == 0) {
        return 0;
    }
    if (fn_8012E5A8(target) != 1) {
        return 0;
    }
    if ((arg & 0xFF) == 0) {
        return 1;
    }
    setVec3(&rel, lbl_80797B18, lbl_80797B18, lbl_80797D08 * get_em_chg_scale(self));
    copyVec3(&b, &rel);
    rotVecY(&b, self->field_0x1C0);
    addVec3(&probe, &self->pos, &b);
    copyVec3(&a, &probe);
    f32 dist = calcDistanceSqXZ(&a, &target->pos);
    f32 range = lbl_80797BE4 * get_em_chg_scale(self);
    if (dist >= range * (lbl_80797BE4 * get_em_chg_scale(self))) {
        return 0;
    }
    return 1;
}

/* The "action 13, sub-state <= 5" predicate. */
u32 fn_8018257C(_ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 5) {
        return 1;
    }
    return 0;
}

/* The per-kind aim/approach query the action band dispatches over. */
u32 fn_801825A4(_ENEMY_WORK* self, u32 kind) {
    switch (kind & 0xFF) {
    case 0: {
        f32 delta = self->vec_0x36C.y - self->pos.y;
        if (delta >= lbl_80797B58) {
            return 2;
        }
        if (delta >= lbl_80797B2C) {
            return 1;
        }
        if (delta <= lbl_80797B5C) {
            return 4;
        }
        if (delta <= lbl_80797C90) {
            return 3;
        }
        return 0;
    }
    case 1:
        return fn_80182918(self);
    case 2:
        return self->color_0x328.field_0x32F;
    case 3: {
        _ENEMY_WORK* target = fn_80131034(self, 23, 0);
        if (target != 0) {
            return fn_8012E5A8(target) == 1;
        }
        return 0;
    }
    case 4:
        return self->color_0x328.field_0x332 < 900;
    case 5: {
        f32 scale = get_em_chg_scale(self);
        f32 limit = self->field_0x210 -
                    (lbl_80797B9C + fn_8013026C(self)) * scale;
        return self->pos.y < limit;
    }
    case 6:
        return self->color_0x328.field_0x330 != 0;
    case 7:
        if ((u8)em_parts_damage_level_get(self, 3) >= 3) {
            return 0;
        }
        return fn_80182918(self) != 0;
    default:
        return 0;
    }
}

/* Area/action -> motion pair: fills the two out-bytes and arms the motion ids the
 * map/area state names. */
void fn_80182768(_ENEMY_WORK* self, u8* out_a, u8* out_b) {
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if (self->area_no == 7) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 14, 15, lbl_80797B18);
        } else if (self->area_no == 12) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 23, 24, lbl_80797B18);
        }
        break;
    case 3:
        if (self->area_no == 4) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 6, 7, lbl_80797B18);
        } else if (self->area_no == 8) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 6, 0, lbl_80797B18);
        }
        break;
    case 9:
    case 11:
        if (self->area_no == 1) {
            em_move_mode_set(self, 2);
            *out_a = 12;
            *out_b = 0;
            fn_80126324(self, 0, 1, lbl_80797B18);
        }
        break;
    default:
        break;
    }
}

/* Tail-calls `fn_80182978`. */
void fn_80182914(_ENEMY_WORK* self) {
    fn_80182978(self);
}

/* The 0x1E4 damage counter's level: 0 below 1, 1 below 100, and past that the part-3 damage level maps to 5, 4,
 * 3, ... (retail's branchless `subfc`/`adde` run). */
u32 fn_80182918(_ENEMY_WORK* self) {
    u8 health = self->field_0x1E4;
    if (health < 1) {
        return 0;
    }
    if (health < 100) {
        return 1;
    }
    u8 part = (u8)em_parts_damage_level_get(self, 3);
    return 5 - part + (part >= 3 ? -1 : 0);
}

/* The map/area aim/rotation setter: clears the aim vector then arms the rotation id
 * the map/area state names. */
void fn_80182978(_ENEMY_WORK* self) {
    f32 zero = lbl_80797B18;
    self->aim.x = zero;
    self->aim.y = zero;
    self->aim.z = zero;
    switch ((u8)stage_map_kind_get(self->field_0x1E0)) {
    case 1:
        if ((s32)self->area_no == 7 || (s32)self->area_no == 12) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256), &self->aim);
        }
        break;
    case 3:
        if ((s32)self->area_no == 3 || (s32)self->area_no == 6) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 8), &self->aim);
        } else if ((s32)self->area_no == 4) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 3), &self->aim);
        } else if ((s32)self->area_no == 8) {
            fn_80126278(self, (u16)((self->area_no & 0xF) * 256 + 4), &self->aim);
        }
        break;
    case 9:
    case 11:
        if ((s32)self->area_no == 1) {
            f32 v = lbl_80797B18;
            self->aim.x = v;
            self->aim.y = lbl_80797E78;
            self->aim.z = v;
        }
        break;
    default:
        break;
    }
}

/* Builds one 0x16-byte action record: the type, a fixed vector and the three scalars. */
void fn_80182AB8(struct EmWorkItem* out, u32 a, s16 b, s16 c) {
    VEC3 vec;
    setVec3(&vec, lbl_80797B18, lbl_80797E7C, lbl_80797B64);
    out->type_0x00 = 0x1A;
    copyVec3(&out->vec_0x04, &vec);
    out->field_0x10 = (u8)a;
    out->field_0x12 = b;
    out->field_0x14 = c;
}

/* The record's release step: unregister, then free when the caller asks (arg > 0); returns the record so a caller
 * can chain. */
void* fn_80182B38(void* p, s16 arg) {
    if (p != 0) {
        fn_8013918C((_ENEMY_WORK*)p, 0);
        if (arg > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* Installs the four static vectors of the shared 0x806A79D0 table. */
void fn_80182B94(void) {
    VEC3 a;
    VEC3 b;
    VEC3 c;
    VEC3 d;
    setVec3(&a, lbl_80797B18, lbl_80797B18, lbl_80797E80);
    copyVec3(&vec_tbl_80181C88[0], &a);
    setVec3(&b, lbl_80797B18, lbl_80797B18, lbl_80797B18);
    copyVec3(&vec_tbl_80181C88[1], &b);
    setVec3(&c, lbl_80797C34, lbl_80797B18, lbl_80797B18);
    copyVec3(&vec_tbl_80181C88[2], &c);
    setVec3(&d, lbl_80797B18, lbl_80797B18, lbl_80797B18);
    copyVec3(&vec_tbl_80181C88[3], &d);
}

/* The unit's `.bss`: the four-vector table `fn_80182B94` fills (the map's second row at +0x18 is folded
 * in).  The name is a GUESS. */
VEC3 vec_tbl_80181C88[4];  /* +0x806A79D0 */
