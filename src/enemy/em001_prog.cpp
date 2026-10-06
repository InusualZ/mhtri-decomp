/*
 * enemy/em001_prog.cpp - enemy 001's program: the `_ENEMY_WORK` action steps (two-level `state`/`state_sub`
 *   machines), their jump-table dispatchers, the shell spawn path, the part-colour and attack tests, and the
 *   static initializer that seeds the three vector records.
 * RANGE. .text 0x80147C94-0x80154E40 (124 functions); extab 0x8000D9DC-0x8000DCFC, extabindex
 *   0x800285B4-0x80028A64, .ctors 0x8056F320-0x8056F324 (`fn_80154D44`), .rodata 0x8056F8E0-0x8056FA38, .data
 *   0x805A1C58-0x805A44A8, .bss 0x806A77D8-0x806A7820 (three 0x18-byte vector records), .sdata 0x80791A20-0x80791A38,
 *   .sdata2 0x80796E08-0x807970C0.  The left edge 0x80147C94 is where `enemy/enemy_control.cpp`'s `.text` and
 *   extabindex runs end; nothing else proves it (the base helper table `lbl_805A1368` is `enemy/fn_80138074.c`'s
 *   data, not a link to `enemy_control`).  The right edge is the TU end: `fn_80154D44` is the `.ctors` entry and
 *   MWCC places a TU's static initializer last.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_80147CE0` to `fn_80149D6C` and from `fn_80151074` to the
 *   end except `fn_801545B8` (retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs, playbook 39); the band
 *   `fn_8014A1BC`..`fn_80150FCC` and `fn_801545B8` compile with it on.
 * NAMES. The file name follows the runtime dump's `em001_prog_tbl`, which opens `.data`.  `em_res_user_data_ctor`
 *   (the resource user-data helper's base constructor) and `em_spawn_rec_init` (the spawn record's position
 *   reset) are GUESSes from their bodies; the dump answers `zz_` for both.  The map has only `fn_` stems for the
 *   other rows.  Callees whose call sites disagree with the owner's header are called
 *   through `<name>_viewN`/`<name>_cN` cast macros (the same direct call), and `#define <name>
 *   <name>_hidden_<header>` hides the disagreeing declaration around its `#include`.  The `.bss` record names
 *   (`vec_pair_801502C8_*`) are GUESSes.  `_ENEMY_WORK` is 0xB18 bytes: `create_move_work` (`ef/system_core.cpp`)
 *   strides kind 3 with `mulli r30,r3,2840` at 0x800CFA0C.
 * RESIDUALS. 3 rows unwritten: 0x80147C94-0x80147CE0; 0x801514BC-0x80154184 (`fn_801514BC`, the case table over
 *   `jumptable_805A4104`); 0x801544F0-0x801545B8 (`fn_801544F0` reads a 32-bit pointer at `_ENEMY_WORK` +0x04,
 *   which `enemy/ENEMY_WORK.h` types as the byte `field_0x004`).
 *   61 partial rows, including:
 *  - `fn_80147CE0`: retail re-masks the argument at each use (`clrlwi r0,r31,24`) where ours keeps it in r31
 *    (`(u8)arg` per use flips the compares to `cmplwi`), and loads the 0.0f argument before `li r4,0xa`;
 *  - `fn_80147F48`: retail lowers case 3's 22/23 pair to a range test (`subi r0,r4,0x16; cmplwi r0,1`), ours to
 *    two compares (an if-chain scores lower); the rest is the `li r6,0`/r0 zero-source colouring;
 *  - `fn_801493A8`: retail materialises `shell_set_func_ptr` (the 4th argument) before it loads the callee from
 *    the table's +0x3C slot, ours loads the table first;
 *  - `fn_8014BDAC`: retail keeps `clrlwi r0,r5,24; cmpwi r0,0` where every source form tried fuses `clrlwi.`;
 *  - `fn_8014CEF8`: its stack locals are typed `VEC3` from their use sites and the frame does not line up;
 *  - `fn_80154184`: the same instructions, but MWCC hoists the call's argument setup (`lfs f1,0x1d4(self)`,
 *    `addi r3`/`li r4`/`li r5`) above the two `u32 -> f32 -> int` conversions, retail keeps it after them;
 *  - `fn_801542D0`: retail keeps four `cmpwi`s where MWCC folds `case 1/2/3` into a `(kind-1) <= 2` range test
 *    (the 20-byte gap; the if-chain scores lower).
 *   The other 54 partial rows have no recorded cause.  A fully scored row can still call the wrong
 *   symbol: `fn_801502C8` (fully scored) and `fn_801507CC` call `em_after_frame_check` where retail calls
 *   `em_frame_check` (3 relocations: `fn_801502C8`+0x158, `fn_801507CC`+0x19c, +0x334; `relocdiff --by-owner`).
 *   flipcheck: `.ctors`/`.rodata`/`.sdata` claimed, not emitted; `.data` 0x30C against 0x2850, `.sdata2` 0x10
 *   against 0x2B8; `.text` (0xA304 of 0xD1AC), extab (0x310 of 0x320) and extabindex (0x498 of 0x4B0) short of
 *   the claim and differing.
 * SHAPES. `(u8)arg`/`(u8)sub` per use in `fn_80147F48` (its three `clrlwi r0,r4,24` and the `li r6,0` zero
 *   source); `(s32)` parameters so `arg & 0xFF` is an `int` and compares with `cmpwi` (`fn_80149068`,
 *   `fn_801492B8`).  `fn_801493A8` reads its case-2 local `v` uninitialised in case 3, as retail does: `v` is
 *   declared at function scope with no initialiser.  `fn_80149D6C` increments `state_0x05`/`phase_0x06` (retail
 *   reuses the loaded value, `addi r0,r3,1`), writes the shared tail inside `case 1`, writes `default:` first in
 *   both nested switches (playbook 37) and lists `case 2: case 3: case 4:` before `case 0:`.  `fn_8014B6CC` and
 *   `fn_8014C16C` write `default:` first; `>=`, not `==`, gives the `fcmpo` + `cror eq,gt,eq` float tests.
 *   `fn_801512E8`'s `buf_0x18` is a 0x18-byte buffer whose first 0xC bytes are the helper record: its size sets
 *   the frame.
 */

#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck (rule 2: the owner's header) */
#include "enemy/em_motion_param_set.h" /* em_motion_param_set (rule 2: the owner's header) */
#include "enemy/lbl_805A1368.h" /* lbl_805A1368 (rule 2: the owner's header) */
#include "enemy/em_busy_set.h" /* em_busy_set (rule 2: the owner's header) */
#include "enemy/fn_8012EC3C.h" /* fn_8012EC3C (rule 2: the owner's header) */
#include "enemy/fn_80131BD4.h" /* fn_80131BD4 (rule 2: the owner's header) */
#include "enemy/fn_801277F4.h" /* fn_801277F4 (rule 2: the owner's header) */
#include "enemy/em_action_finish_fall.h" /* em_action_finish_fall (rule 2: the owner's header) */
#include "enemy/em_approach_start.h" /* em_approach_start (rule 2: the owner's header) */
#include "enemy/em_action_finish_walk.h" /* em_action_finish_walk (rule 2: the owner's header) */
#include "enemy/em_hit_window_set_default.h" /* em_hit_window_set_default (rule 2: the owner's header) */
#include "enemy/em_hit_window_set.h" /* em_hit_window_set (rule 2: the owner's header) */
#include "enemy/em_hit_window_clear.h" /* em_hit_window_clear (rule 2: the owner's header) */
#include "enemy/fn_80128A70.h" /* fn_80128A70 (rule 2: the owner's header) */
#include "enemy/em_mot_set.h" /* em_mot_set (rule 2: the owner's header) */
#include "enemy/em_mot_end_ck.h" /* em_mot_end_ck (rule 2: the owner's header) */
#include "enemy/em_fall_start.h" /* em_fall_start (rule 2: the owner's header) */
#include "enemy/fn_801369A0.h" /* fn_801369A0 (rule 2: the owner's header) */
#include "enemy/fn_8012E5A8.h" /* fn_8012E5A8 (rule 2: the owner's header) */
#include "enemy/em_alt_mode_ck.h" /* em_alt_mode_ck (rule 2: the owner's header) */
#include "enemy/fn_80129A70.h" /* fn_80129A70 (rule 2: the owner's header) */
#include "enemy/fn_80129D3C.h" /* fn_80129D3C (rule 2: the owner's header) */
#include "enemy/fn_80129DB8.h" /* fn_80129DB8 (rule 2: the owner's header) */
#include "enemy/fn_80129E48.h" /* fn_80129E48 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#define em_hit_window_set em_hit_window_set_hidden_fn_801251D0_h
#include "enemy/fn_801251D0.h"
#undef em_hit_window_set
#define fn_8012E5A8 fn_8012E5A8_hidden_fn_8012BDF4_h
#include "enemy/fn_8012BDF4.h"
#undef fn_8012E5A8
#include "ef/get_move_work_adrs.h"
#include "enemy/fn_80138074.h"
#include "ef.h"
#include "ef/fn_80105314.h"
#include "ef/eft007.h"
#include "ef/eft009.h"
#include "sound/fn_800DD1F0.h"
#define fn_8012EC3C fn_8012EC3C_hidden_fn_8012E968_h
#define em_alt_mode_ck em_alt_mode_ck_hidden_fn_8012E968_h
#include "unsplit/enemy.h"
#undef em_alt_mode_ck
#undef fn_8012EC3C
#include "unsplit/unknown.h"
#include "enemy/fn_80147CE0.h"
#include "sys_mem.h"
#include "fn_8004CAD8.h"
#define draw_shape_arm draw_shape_arm_hidden_draw_shape_h
#include "draw_shape.h"
#undef draw_shape_arm
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#define fn_80149D6C fn_80149D6C_hidden_fn_80149D6C_h
#include "enemy/fn_80149D6C.h"
#undef fn_80149D6C
#include "enemy/fn_8012EC74.h" /* fn_80136D4C (its owner) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "unsplit/enemy_pool.h" /* this unit's own `.data` symbols, declared in the band header (rule 2 inverted) */
#include "enemy/fn_80147CE0.h" /* EmSpawnRec + em_res_user_data_ctor (the owner's header) */
#include "enemy/fn_801502C8.h" /* this unit's own declarations (rule 2) */
#include "unsplit/unknown.h" /* SystemWork / system_w */
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
/* the call sites use the argument-less view: a cast call is the same direct call. */
#define em_mot_finished_ck_c1 ((u32 (*)(void))em_mot_finished_ck)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_busy_set_c3 ((void (*)(_ENEMY_WORK*))em_busy_set)
#define fn_8012EC3C_c1 ((u32 (*)(_ENEMY_WORK*))fn_8012EC3C)
#define em_hit_window_set_default_c1 ((void (*)(_ENEMY_WORK *, u32, u32))em_hit_window_set_default)
#define fn_80128A70_c1 ((void (*)(_ENEMY_WORK *, u32, u32))fn_80128A70)
#define fn_8012E5A8_c1 ((u32 (*)(struct _ENEMY_WORK*))fn_8012E5A8)
#define em_alt_mode_ck_c1 ((u32 (*)(struct _ENEMY_WORK*))em_alt_mode_ck)
#define em_fall_height_get_f32 ((f32 (*)(struct _ENEMY_WORK*))em_fall_height_get)
/* Unprototyped call shapes: each call keeps its argument shape through a cast (the same direct call). */
#define em_busy_set_c1 ((void (*)(_ENEMY_WORK*))em_busy_set)
#define em_busy_set_c2 ((void (*)(void))em_busy_set)
#define em_move_offset_step_update_c1 ((s32 (*)(_ENEMY_WORK*, void*))em_move_offset_step_update)
#define fn_80149AFC_c1 ((void (*)(_ENEMY_WORK*))fn_80149AFC)
#define em_ground_ck_c1 ((u32 (*)(_ENEMY_WORK*))em_ground_ck)
#define fn_801303FC_c1 ((void (*)(_ENEMY_WORK*, f32))fn_801303FC)
#define fn_801493A8_c1 ((void (*)(_ENEMY_WORK*, u32, u8, u32))fn_801493A8)
#define fn_80133F4C_c1 ((void (*)(_ENEMY_WORK*, f32, f32))fn_80133F4C)
#define em_ground_ck_c2 ((u32 (*)(_ENEMY_WORK*, f32))em_ground_ck)
#define fn_801303FC_c2 ((void (*)(_ENEMY_WORK*))fn_801303FC)
#define fn_8012D3E0_c1 ((u8 (*)(_ENEMY_WORK*, u32))fn_8012D3E0)
#define fn_8012B380_c1 ((void (*)(_ENEMY_WORK*, u32, u32))fn_8012B380)
#define fn_80131EC0_c1 ((void (*)(void))fn_80131EC0)
#define fn_8012D23C_c1 ((u32 (*)(_ENEMY_WORK*, u32, u32))fn_8012D23C)
#define fn_8011E6EC_c1 ((void (*)(_ENEMY_WORK*, s32, u32, f32, f32))fn_8011E6EC)
#define fn_801493A8_c2 ((void (*)(u32, u32, u32))fn_801493A8)
#define em_se_tbl_play_alt_c1 ((void (*)(void*, u32, u32))em_se_tbl_play_alt)
#define fn_80149AFC_c2 ((void (*)(void))fn_80149AFC)
#define em_se_tbl_play_c1 ((void (*)(void*, u32, u32))em_se_tbl_play)
#define eft009_set_pos_c1 ((void (*)(u32, void*, void*, u32, f32))eft009_set_pos)
#define calcVecAngXY_c1 ((void (*)(void*, void*, void*))calcVecAngXY)
#define em_move_offset_step_update_c2 ((s32 (*)(_ENEMY_WORK*, void*, f32))em_move_offset_step_update)
#define em_spawn_rec_init_c1 ((EmSpawnRec* (*)(void*))em_spawn_rec_init)
#define fn_801545B8_c1 ((void (*)(void*, u32, u32, u32))fn_801545B8)
#define getKeyData_c1 ((f32 (*)(void*))getKeyData)
#define em_mot_speed_set_c1 ((void (*)(_ENEMY_WORK*))em_mot_speed_set)
#define fn_801545B8_c2 ((void (*)(void*, u32, u16, u32))fn_801545B8)
#define fn_800AD9C0_c1 ((void (*)(void*, void*, f32))fn_800AD9C0)
#define fn_80131EC0_c2 ((void (*)(_ENEMY_WORK*))fn_80131EC0)
#define mhchar_mat_tev_set_view1 ((void (*)(void*, s32, s32, u8, s32, s32, u8, f32))mhchar_mat_tev_set)
#define fn_8013918C_view1 ((void (*)(void*, s32))fn_8013918C)
#define fn_80131034_view1 ((struct _ENEMY_WORK* (*)(struct _ENEMY_WORK*, s32, s32))fn_80131034)
#define fn_80130CDC_view1 ((void (*)(s32))fn_80130CDC)
#define fn_801303FC_view1 ((void (*)(f32))fn_801303FC)
#define fn_8012B380_view1 ((void (*)(_ENEMY_WORK*, u8, u8, u8))fn_8012B380)
#define fn_8012A204_view1 ((s32 (*)(struct _ENEMY_WORK*))fn_8012A204)
#define fn_8012A014_view1 ((u32 (*)(struct _ENEMY_WORK*, s32, s32, u16, void*, void*))fn_8012A014)
#define fn_80128A70_view1 ((void (*)(_ENEMY_WORK*, u8, u8))fn_80128A70)
#define fn_80050EF4_view1 ((f32 (*)(const void*, const void*))fn_80050EF4)
#define em_mot_set_blend_view1 ((void (*)(struct _ENEMY_WORK*, s32, s32, s32, s32))em_mot_set_blend)
#define em_magma_check_view1 ((u8 (*)(struct _ENEMY_WORK*))em_magma_check)
#define em_hit_window_set_default_view1 ((void (*)(struct _ENEMY_WORK*, s32, s32))em_hit_window_set_default)
#define em_hit_window_set_view1 ((void (*)(struct _ENEMY_WORK*, u8, u32, u32))em_hit_window_set)
#define em_fall_height_get_view1 ((void (*)(struct _ENEMY_WORK*))em_fall_height_get)
#define em_approach_start_view1 ((void (*)(_ENEMY_WORK*, f32, u16))em_approach_start)
#define eft_spawn_type11_view1 ((void (*)(struct _ENEMY_WORK*, void*, u8, f32))eft_spawn_type11)
#define eft_spawn_type10_view1 ((void (*)(struct _ENEMY_WORK*, u32, u32, void*, f32))eft_spawn_type10)
#define eft_em_spawn_view3 ((void (*)(struct _ENEMY_WORK*, s32, s32, void*, f32))eft_em_spawn)
#define eft_em_spawn_view1 ((void (*)(_ENEMY_WORK*, u32, u32, void*, f32))eft_em_spawn)
#define eft009_spawn_at_joint_view1 ((void (*)(struct _ENEMY_WORK*, s32, u8, s32, f32))eft009_spawn_at_joint)
#define draw_shape_arm_view3 ((void (*)(struct _ENEMY_WORK*, s32, s32))draw_shape_arm)
#define draw_shape_arm_view1 ((void (*)(u32, u32, u32))draw_shape_arm)
#define assignVec3_view1 ((void (*)(void*, s32))assignVec3)
#define addVec3_view2 ((void (*)(void*, const void*, const void*))addVec3)

/* The 12-byte helper `fn_80147CE0` allocates and `fn_80147DF0` constructs; +0x00 holds a `.data` table
 * address.  `enemy/em015_prog.cpp`'s `Helper_80176E50` is the same record.  size: 0xC (`operator new(0xC)`) */
typedef struct Helper_80147CE0 {
    /* +0x0 */ void* tbl;
    /* +0x4 */ u32 unused_0x4;
    /* +0x8 */ u32 unused_0x8;
} Helper_80147CE0;

/* The derived table `fn_80147DF0` installs over the base `lbl_805A1368` (both 0x30 bytes). */
extern "C" u8 lbl_805A4478[];

/* Callees whose owners' headers do not declare them: `camera/camera_main.cpp`'s `fn_802B9574` and
 * `enemy/em_model.cpp`'s `fn_803B9BA0`. */
extern "C" void fn_802B9574(u32 a);
extern "C" void fn_803B9BA0(_ENEMY_WORK* self, void* pos, s32 value);

/* The unit's `.sdata2` pool, declared, not defined: the source does not emit it yet (values from the DOL). */
extern f32 lbl_80796E18; /* 5000.0 */
extern f32 lbl_80796E1C; /* 0.0 */
extern f32 lbl_80796E20; /* 1.0 */
extern f32 lbl_80796E24; /* -50.0 */
extern f32 lbl_80796E28; /* 15.0 */
extern f32 lbl_80796E2C; /* 0.9 */
extern f32 lbl_80796E30; /* -1.0 */
extern f32 lbl_80796E34; /* 110.0 */
extern f32 lbl_80796E38; /* -20.0 */
extern f32 lbl_80796E3C; /* 80.0 */
extern f32 lbl_80796E40; /* 112.0 */
extern f32 lbl_80796E44; /* 214.0 */
extern f32 lbl_80796E48; /* 52.0 */
extern f32 lbl_80796E4C; /* 92.0 */
extern f32 lbl_80796E50; /* 262.0 */
extern f32 lbl_80796E54; /* -30.0 */
extern f32 lbl_80796E58; /* 70.0 */
extern f32 lbl_80796E5C; /* 96.0 */
extern f32 lbl_80796E60; /* 50.0 */
extern f32 lbl_80796E64; /* 1.4 */
extern f32 lbl_80796E68; /* 136.0 */
extern f32 lbl_80796E6C; /* 152.0 */
extern f32 lbl_80796E70; /* 344.0 */
extern f32 lbl_80796E74; /* 368.0 */
extern f32 lbl_80796E78; /* 156.0 */
extern f32 lbl_80796E7C; /* 190.0 */
extern f32 lbl_80796E80; /* 386.0 */
extern f32 lbl_80796E84; /* 0.6 */
extern f32 lbl_80796E88; /* 398.0 */
extern f32 lbl_80796E8C; /* 426.0 */
extern f32 lbl_80796E90; /* 458.0 */
extern f32 lbl_80796E94; /* 478.0 */
extern f32 lbl_80796E98; /* 0.8 */
extern f32 lbl_80796E9C; /* 434.0 */
extern f32 lbl_80796EA0; /* 12.0 */
extern f32 lbl_80796EA4; /* 16.0 */
extern f32 lbl_80796EA8; /* 40.0 */
extern f32 lbl_80796EAC; /* 46.0 */
extern f32 lbl_80796EB0; /* 0.7 */
extern f32 lbl_80796EB4; /* 66.0 */
extern f32 lbl_80796EB8; /* 72.0 */
extern f32 lbl_80796EBC; /* 76.0 */
extern f32 lbl_80796EC0; /* -500.0 */
extern f32 lbl_80796EC4; /* 700.0 */
extern f32 lbl_80796EC8; /* 6.0 */
extern f32 lbl_80796ECC; /* 0.5 */
extern f32 lbl_80796ED0; /* 360.0 */
extern f32 lbl_80796ED4; /* 65536.0 */
extern f32 lbl_80796ED8; /* 60.0 */
extern f32 lbl_80796EDC; /* 0.04 */

/* the two 0x40-byte `.rodata` tables the joint-lookup family indexes */
extern "C" u8 lbl_8056F8E0[];
extern "C" u8 lbl_8056F920[];
/* the two `.data` tables `em_key_curve_eval`/`em_move_offset_rot_apply` read */
extern "C" u8 lbl_805A1DA0[];
extern "C" u8 lbl_805A1F70[];
/* ------------------------------------------------------------------------------------------------ *
 * this unit's own forward declarations
 * ------------------------------------------------------------------------------------------------ */
extern "C" Helper_80147CE0* fn_80147DF0(Helper_80147CE0* self);
extern "C" void fn_80147F00(_ENEMY_WORK* self, u32 arg);

f32 calcDistanceSqXZ(nw4r::math::VEC3*, nw4r::math::VEC3*);

extern "C" {
/* The unit's pool literals, declared, not defined. */
extern u8 lbl_8056F9E0[];
extern f32 lbl_80796E1C; /* 0.0f */
extern f32 lbl_80796EE8; /* 16000000.0f */
}

/* size: 0x18 (lower bound: the unit reads +0x12 and +0x14) */
typedef struct ShellParams {
    /* +0x00 */ u8 pad_0x00[0x12];
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ u16 field_0x14;
    /* +0x16 */ u8 pad_0x16[0x02];
} ShellParams;

extern "C" {

}

extern "C" {

/* Pool literals and callees the `fn_8014A1BC` block reads (declared by their map spelling). */
extern f32 lbl_80796EEC;
extern f32 lbl_80796EF0;
extern f32 lbl_80796EF4;

extern f32 fn_802B0430(u8 id);

extern f32 lbl_80796E1C;
extern f32 lbl_80796E58;
extern f32 lbl_80796EF8;
extern f32 lbl_80796EFC;
extern f32 lbl_80796F00;
extern f32 lbl_80796F04;
extern f32 lbl_80796F08;
extern f32 lbl_80796F0C;
extern f32 lbl_80796F10;
extern f32 lbl_80796F14;
extern f32 lbl_80796F18;
extern void fn_80154CA4(_ENEMY_WORK *self);
extern f32 lbl_80796EBC;
extern f32 lbl_80796F1C;
extern f32 lbl_80796E20;
extern f32 lbl_80796EA8;
extern u32 lbl_8056F9A0[];
extern u32 lbl_8056FA0C[];
extern f32 lbl_80796E38;
extern f32 lbl_80796E3C;
extern f32 lbl_80796E4C;
extern f32 lbl_80796ED8;
extern f32 lbl_80796F20;
extern f32 lbl_80796F24;
extern f32 lbl_80796F28;
extern f32 lbl_80796F2C;
extern void draw_shape_arm(_ENEMY_WORK *self, u32 a, u32 b);
}

void setVector3(nw4r::math::VEC3*, f32, f32, f32);

extern "C" {
extern void eft_em_spawn(_ENEMY_WORK *self, u32 a, u32 b, VEC3 *v, f32 s);
extern f32 lbl_80796E0C;
extern f32 lbl_80796E34;
extern f32 lbl_80796E60;
extern f32 lbl_80796F30;
extern f32 lbl_80796F34;
extern f32 lbl_80796F38;
extern f32 lbl_80796F3C;
extern void fn_80154C74(_ENEMY_WORK *self);

extern f32 lbl_80796E24;
extern f32 lbl_80796EA4;
extern f32 lbl_80796F60;
extern f32 lbl_80796F74;
extern f32 lbl_80796F78;
extern f32 lbl_80796F7C;

extern f32 lbl_805A2AA0[];
extern f32 lbl_805A2B00[];
extern f32 lbl_805A2B50[];
extern f32 lbl_805A2B98[];
extern f32 lbl_805A2BC8[];
extern f32 lbl_80796F84;
extern f32 lbl_80796F9C;
extern f32 lbl_80796FA0;
extern void fn_8011E6EC();

extern f32 lbl_80796FA4;
extern f32 lbl_80796FA8;

extern f32 lbl_805A2338[];
extern f32 lbl_805A2360[];
extern f32 lbl_805A2388[];
extern f32 lbl_805A23E0[];
extern f32 lbl_805A2450[];
extern f32 lbl_805A2490[];
extern f32 lbl_805A2518[];
extern f32 lbl_805A2588[];
extern f32 lbl_805A25B0[];
extern f32 lbl_805A25F0[];
extern f32 lbl_805A2620[];
extern f32 lbl_805A2678[];
extern f32 lbl_805A26E0[];
extern f32 lbl_805A2718[];
extern f32 lbl_805A27A0[];
extern f32 lbl_805A2808[];
extern f32 lbl_805A2830[];
extern f32 lbl_805A2858[];
extern f32 lbl_805A2880[];
extern f32 lbl_805A28F0[];
extern f32 lbl_805A2950[];
extern f32 lbl_805A2978[];
extern f32 lbl_805A29A0[];
extern f32 lbl_805A29E0[];
extern f32 lbl_805A2A58[];

extern f32 lbl_80796E2C;
extern f32 lbl_80796E50;
extern f32 lbl_80796E54;
extern f32 lbl_80796E5C;
extern f32 lbl_80796E64;
extern f32 lbl_80796E68;
extern f32 lbl_80796E6C;
extern f32 lbl_80796E70;
extern f32 lbl_80796E74;
extern f32 lbl_80796E78;
extern f32 lbl_80796E7C;
extern f32 lbl_80796E80;
extern f32 lbl_80796E84;
extern f32 lbl_80796E88;
extern f32 lbl_80796E8C;
extern f32 lbl_80796E90;
extern f32 lbl_80796E94;
extern f32 lbl_80796E98;
extern f32 lbl_80796E9C;
extern f32 lbl_80796F6C;
extern f32 lbl_80796F70;

extern f32 lbl_8056F960[];
extern f32 lbl_80796F94;
extern f32 lbl_80796F98;

extern f32 lbl_80796F8C;
extern f32 lbl_80796F90;

extern f32 lbl_80796F40;
extern f32 lbl_80796F44;
extern f32 lbl_80796F48;
extern f32 lbl_80796F5C;
extern f32 lbl_80796F80;

extern f32 lbl_80796F88;
extern void fn_800AD9C0();

extern f32 lbl_80796ECC;
extern f32 lbl_80796FAC;
extern f32 lbl_80796FB0;
extern f32 lbl_80796FB4;

void fn_8014A1BC(_ENEMY_WORK *self, s32 arg1, s32 arg2);
void fn_8014A334(_ENEMY_WORK *self, s32 arg1);
void fn_8014A418(_ENEMY_WORK *self);
void fn_8014A7B0(_ENEMY_WORK *self, s32 arg1);
void fn_8014A860(_ENEMY_WORK *self);
void fn_8014A8EC(_ENEMY_WORK *self, s32 arg1);
void fn_8014AA4C(_ENEMY_WORK *self);
void fn_8014AB24(_ENEMY_WORK *self);
void fn_8014ABEC(_ENEMY_WORK *self);
void fn_8014ACAC(_ENEMY_WORK *self);
void fn_8014ADC8(_ENEMY_WORK *self);
void fn_8014AEAC(_ENEMY_WORK *self);
void fn_8014AF44(_ENEMY_WORK *self);
void fn_8014B020(_ENEMY_WORK *self);
void fn_8014B0E8(_ENEMY_WORK *self);
void fn_8014B1F8(_ENEMY_WORK *self, s32 arg1);
void fn_8014B378(_ENEMY_WORK *self);
void fn_8014B4EC(_ENEMY_WORK *self, s32 arg1);
void fn_8014B5C0(_ENEMY_WORK *self);
void fn_8014B630(_ENEMY_WORK *self);
void fn_8014B6CC(_ENEMY_WORK *self, s32 arg1);
void fn_8014BDAC(_ENEMY_WORK *a0, s32 arg1, s32 arg2, s32 arg3);
void fn_8014BDF8(_ENEMY_WORK *self, s32 arg1);
void fn_8014BFE4(_ENEMY_WORK *self, s32 arg1);
void fn_8014C16C(_ENEMY_WORK *self, s32 arg1);
void fn_8014CBA0(_ENEMY_WORK *self);
void fn_8014CC98(_ENEMY_WORK *self);
void fn_8014CDD0(_ENEMY_WORK *self, s32 arg1);
void fn_8014C6DC(_ENEMY_WORK *self, u8 arg1);
void fn_8014D504(_ENEMY_WORK *self);
void fn_8014D714(_ENEMY_WORK *self);
void fn_8014E160(_ENEMY_WORK *self);
void fn_8014E3AC(_ENEMY_WORK *self);
void fn_8014EA70(_ENEMY_WORK *self);
void fn_8014EA84(_ENEMY_WORK *self);
void fn_8014EC50(_ENEMY_WORK *self);
void fn_8014EDC8(_ENEMY_WORK *self);
void fn_8014F010(void);
void fn_8014F020(void);
void fn_8014F030(_ENEMY_WORK *self);
void fn_8014F530(_ENEMY_WORK *self);
void fn_8014F5D8(void);
void fn_8014F5DC(_ENEMY_WORK *self);
void fn_8014F5F0(_ENEMY_WORK *self);
void fn_8014FA34(_ENEMY_WORK *self);
void fn_8014FE00(_ENEMY_WORK *self);
void fn_8014F138(_ENEMY_WORK *self);
void fn_8014F078(_ENEMY_WORK *self);
void fn_8014FF10(_ENEMY_WORK *self);
void fn_8014CEF8(_ENEMY_WORK *self, u8 arg1, u8 arg2);
u32 fn_8014E670(_ENEMY_WORK *self, u8 arg1);
void fn_8014FC24(_ENEMY_WORK *self);
void fn_8014E290(_ENEMY_WORK *self);
void fn_8014BB40(_ENEMY_WORK *self, u8 arg1);
void fn_8014C464(_ENEMY_WORK *self, u8 arg1);
void fn_8014D8F0(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void fn_8014DAF4(_ENEMY_WORK *self, u8 arg1, u8 arg2);
void fn_8014DDEC(_ENEMY_WORK *self);
void fn_8014E774(_ENEMY_WORK *self);
void fn_8014F71C(_ENEMY_WORK *self);

extern f32 lbl_80796F4C;

extern f32 lbl_80796EA0;

extern f32 lbl_80796F50;
extern f32 lbl_80796F54;
extern f32 lbl_80796F58;
extern f32 lbl_80796F64;
extern f32 lbl_80796F68;
extern f32 fn_80050EF4(void *a, void *b);

/* Callees declared with the signatures their call sites use. */

/* `enemy/enemy_control.cpp`'s demo helpers and `stage/stg_w.cpp`'s `stage_map_kind_get`. */
s16 em_demo_frame_get(void);
u32 em_demo_time_ck(s32 label);
void em_demo_pos_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_rot_set(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void em_demo_reset(struct _ENEMY_WORK* self, s32 a);
void em_demo_enable(struct _ENEMY_WORK* self);
void em_demo_key3_apply(struct _ENEMY_WORK* self, s16 a, void* b, void* c);
void em_demo_key_apply(struct _ENEMY_WORK* self, s16 a, void* b, void* c, s32 d, s32 e);
u8 stage_map_kind_get(u8 a);

/* This unit's own dispatch targets, declared before use. */
void fn_801484D4(struct _ENEMY_WORK* self);
void fn_80149004(struct _ENEMY_WORK* self);
void fn_80149814(struct _ENEMY_WORK* self);
void fn_8014AA4C(struct _ENEMY_WORK* self);
void fn_8014B5C0(struct _ENEMY_WORK* self);
void fn_8014E3AC(struct _ENEMY_WORK* self);
void fn_8014EA70(struct _ENEMY_WORK* self);
void fn_8014F030(struct _ENEMY_WORK* self);
void fn_8014F138(struct _ENEMY_WORK* self);
void fn_8014F530(struct _ENEMY_WORK* self);
void fn_8014F5DC(struct _ENEMY_WORK* self);
void fn_8014F5F0(struct _ENEMY_WORK* self);
void fn_8014F71C(struct _ENEMY_WORK* self);
void fn_8014FA34(struct _ENEMY_WORK* self);
void fn_8014FC24(struct _ENEMY_WORK* self);
void fn_8014FE00(struct _ENEMY_WORK* self);
void fn_8014FF10(struct _ENEMY_WORK* self);

/* The effect helper the range drives. */
void eft_spawn_pos_in_area(void* pos, u8 a, u8 b, s32 c, f32 d);

}

/* the C++-mangled callees (rule 9: declared with the real signature, called through it). */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
u16 em_get_mot_no(struct _ENEMY_WORK* self);
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
f32 get_em_chg_scale(struct _ENEMY_WORK* self);
void get_joint_wpos_em(struct _ENEMY_WORK* self, u32 joint, nw4r::math::VEC3* out);
u16 calcVecAngX(nw4r::math::VEC3* v);
s32 calcVecAng2(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
f32 calcDistanceSqXZ(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* More of the unit's `.sdata2` pool, declared, not defined. */
extern f32 lbl_80796E08;

extern f32 lbl_80796E14;
extern f32 lbl_80796E18;

extern f32 lbl_80796FB8;
extern f32 lbl_80796FBC;
extern f32 lbl_80796FC0;
extern f32 lbl_80796FC4;
extern f32 lbl_80796FC8;
extern f32 lbl_80796FCC;
extern f32 lbl_80796FD0;
extern f32 lbl_80796FD4;
extern f32 lbl_80796FD8;
extern f32 lbl_80796FDC;
extern f32 lbl_80796FE0;
extern f32 lbl_80796FE4;
extern f32 lbl_80796FE8;
extern f32 lbl_80796FEC;
extern f32 lbl_80796FF0;
extern f32 lbl_80796FF4;
extern f32 lbl_80796FF8;
extern f32 lbl_80796FFC;
extern f32 lbl_80797000;
extern f32 lbl_80797004;
extern f32 lbl_80797044;
extern f32 lbl_80797074;
extern f32 lbl_80797098;
extern f32 lbl_8079709C;
extern f32 lbl_807970A0;
extern f32 lbl_807970A4;
extern f32 lbl_807970A8;
extern f32 lbl_807970AC;
extern f32 lbl_807970B0;
extern f32 lbl_807970B4;
extern f32 lbl_807970B8;

/* The unit's `.data` parameter tables the code loads; the source does not emit them yet. */
extern u8 lbl_805A2BD8[];
extern u8 lbl_805A20C8[];
extern u8 lbl_805A20D4[];
extern u8 lbl_805A2D68[];
extern u8 lbl_805A2F28[];
extern u8 lbl_805A30DC[];
extern u8 lbl_805A3280[];
extern u8 lbl_805A3860[];
extern u8 lbl_805A3AE0[];
extern u8 lbl_805A3C10[];
extern u8 lbl_805A3DD0[];
extern u8 lbl_805A3E90[];
extern u8 lbl_805A3FC0[];
extern VEC3 vec_pair_801502C8_0[2];
extern VEC3 vec_pair_801502C8_1[2];
extern VEC3 vec_pair_801502C8_2[2];

#pragma peephole off

extern "C" void* em_res_user_data_ctor(void* self);
extern "C" void fn_801493A8(_ENEMY_WORK* self, u32 arg1, s32 arg2, u32 arg3);
extern "C" void fn_801545B8(EmSpawnRec* rec, u8 arg1, s16 arg2, s16 arg3);
extern "C" s32 fn_80154638(struct _ENEMY_WORK* self, u8 arg1);
extern "C" void fn_80154B04(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 farg0);

/* ------------------------------------------------------------------------------------------------ *
 * functions, in address order
 * ------------------------------------------------------------------------------------------------ */

/* The action's per-tick entry: attach the vtable helper when the enemy has none, advance the effect
 * timer for the "hit" step, arm the two frame counters and spawn the 0x1C72 effect. */
extern "C" void fn_80147CE0(_ENEMY_WORK* self, s32 arg) {
    VEC3 v;
    Helper_80147CE0* helper;

    VEC3_ctor(&v);
    if ((arg & 0xFF) == 2) {
        self->pos.y += lbl_80796E18;
        em_fall_height_get(self);
        em_fall_start(self);
        fn_80128A8C(self, 0, 3);
    }
    if ((arg & 0xFF) != 0) {
        em_motion_param_set(self, 0, lbl_80796E1C);
        em_motion_param_set(self, 10, lbl_80796E20);
    }
    if (em_res_user_data_ck(self) == 0) {
        helper = (Helper_80147CE0*)operator new(0xC);
        if (helper != 0) {
            fn_80147DF0(helper);
        }
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, lbl_80796E1C, lbl_80796E24, lbl_80796E28);
        fn_801057A4(self, 27, &v, lbl_80796E2C, 7282);
    }
}

/* The derived helper's constructor: run the base and override the table with its own. */
extern "C" Helper_80147CE0* fn_80147DF0(Helper_80147CE0* self) {
    em_res_user_data_ctor(self);
    self->tbl = lbl_805A4478;
    return self;
}

/* The helper's base constructor: install the engine's joint table, then the base table, and return
 * `self`. */
extern "C" void* em_res_user_data_ctor(void* self) {
    Helper_80147CE0* helper = (Helper_80147CE0*)self;

    fn_800E3B2C((MHchar*)self);
    helper->tbl = lbl_805A1368;
    return helper;
}

/* One action-cancel/transition step: on a `2` command byte, map the state byte through the three
 * sub-steps (`em_mot_finished_ck` reporting the motion as finished) and write the new state. */
extern "C" void fn_80147E68(_ENEMY_WORK* self, u8* in, u8* out) {
    if (in[0] == 2) {
        switch (out[0]) {
          case 0:
            if (em_mot_finished_ck_c1() == 1) {
                out[0] = 3;
            }
            break;
          case 7:
            if (em_mot_finished_ck_c1() == 1) {
                out[0] = 8;
            }
            break;
          case 9:
            if (em_mot_finished_ck_c1() == 1) {
                out[0] = 10;
            }
            break;
        }
    }
}

/* The motion-selection tail: `fn_8012C220`/`fn_8012C4E8` with the area byte and the position. */
extern "C" void fn_80147F00(_ENEMY_WORK* self, u32 arg) {
    u32 id = self->team == 1 ? 2 : 1;

    if ((arg & 0xFF) == 0) {
        fn_8012C220((u8)id, self->area_no);
    } else {
        fn_8012C4E8((u8)id, self->area_no, 100, &self->pos, lbl_80796E30);
    }
}

/* The 14-way dispatch on the action argument: clears the action's flag bytes and runs the state-family
 * steps for the argument values 1/3/4/7/8/9/10/13 (the rest return). */
extern "C" void fn_80147F48(_ENEMY_WORK* self, u32 arg, u32 sub) {
    self->field_0x38B = 0;
    if ((u8)arg != 0) {
        self->field_0x356 = 0;
    }
    if ((u8)arg != 3 || (u8)sub != 11) {
        self->field_0x358 = 0;
    }
    if ((u8)arg > 13) {
        return;
    }
    switch ((u8)arg) {
      case 1:
        switch ((u8)sub) {
          case 3:
            self->field_0x356 = 1;
            break;
          case 5:
            fn_80130F74(self);
            break;
          case 9:
            if (em_mot_finished_ck_c1() == 1) {
                fn_80147F00(self, 0);
            }
            fn_80147F00(self, 1);
            break;
          case 10:
            fn_801376B4(self);
            break;
        }
        break;
      case 3:
        switch ((u8)sub) {
          case 9:
            self->field_0x1FB = 1;
            break;
          case 14:
          case 22:
          case 23:
            self->field_0x38B = 1;
            self->field_0x358 = 1;
            break;
        }
        break;
      case 4:
        switch ((u8)sub) {
          case 1:
            self->field_0x356 = 1;
            break;
          case 11:
            if (em_mot_finished_ck_c1() == 1) {
                fn_80147F00(self, 0);
            }
            fn_80147F00(self, 1);
            break;
        }
        break;
      case 7:
        switch ((u8)sub) {
          case 5:
          case 20:
          case 31:
          case 33:
          case 45:
            self->field_0x356 = 1;
            break;
        }
        break;
      case 8:
      case 9:
        self->field_0x356 = 1;
        break;
      case 10:
        switch ((u8)sub) {
          case 202:
            fn_80147F00(self, 0);
            fn_80147F00(self, 1);
            em_part_hit_set(self, 0, 0);
            break;
          case 26:
          case 27:
          case 35:
          case 126:
          case 127:
          case 182:
            fn_80147F00(self, 0);
            fn_80147F00(self, 1);
            break;
        }
        break;
      case 13:
        if ((u8)sub == 7) {
            self->field_0x357 = 1;
        }
        break;
    }
}

/* The 50-frame action counter: tick `field_0x354` and wrap it. */
extern "C" void fn_801481D8(_ENEMY_WORK* self) {
    if (++self->field_0x354 >= 50) {
        self->field_0x354 = 0;
    }
}

/* The action's opening step. */
extern "C" void fn_801481FC(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    fn_80128AAC(self, 3, 11);
    fn_80133BB4(self);
}

/* The action's first step: arm the motion `em_mot_set_ck(self, 1, 10, 0)` with a 150-frame timer and
 * close the action when it runs out. */
extern "C" void fn_80148248(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        self->timer_0x020 = 150;
        em_mot_set_ck(self, 1, 10, 0);
        break;
      case 1:
        if (--self->timer_0x020 < 0) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the motion `em_mot_set_ck(self, 2, 10, 0)` and closes the action when `em_mot_end_ck` reports it
 * finished. */
extern "C" void fn_801482D0(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 10, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same two-step shape with the motion `em_mot_set_ck(self, 17, 6, 0)`. */
extern "C" void fn_8014834C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 17, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same two-step shape with the motion `em_mot_set_ck(self, 26, 6, 0)` and the second action-end tail
 * (`em_action_finish_fall`). */
extern "C" void fn_801483C8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 26, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* The same shape with `fn_80154CA4`'s clamp on both steps and `em_action_finish_walk` as the end tail. */
extern "C" void fn_80148448(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 3);
        fn_80154CA4(self);
        em_mot_set_ck(self, 27, 6, 0);
        break;
      case 1:
        fn_80154CA4(self);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* The sub-state dispatcher: sub-states 0/4 share the first step, 1/2/3 the three motion variants and
 * 6 the last one. */
extern "C" void fn_801484D4(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80148248(self);
        break;
      case 1:
        fn_801482D0(self);
        break;
      case 2:
        fn_8014834C(self);
        break;
      case 3:
        fn_801483C8(self);
        break;
      case 4:
        fn_80148248(self);
        break;
      case 6:
        fn_80148448(self);
        break;
    }
}

/* The action's two-step body: arm `em_mot_set(self, 5, 10, 0)` and a 19-frame sub-timer, then run three
 * `em_frame_check` windows (one spawns the effect) before the action-end test. */
extern "C" void fn_80148528(_ENEMY_WORK* self) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 10, 0);
        em_hit_window_set_default_c1(self, 0, 19);
        self->timer_0x020 = 0;
        break;
      case 1:
        if (em_frame_check(self, 0, lbl_80796E34, lbl_80796E1C) == 1) {
            em_hit_window_set_view1(self, 1, 8, 5);
            draw_shape_arm_view1((u32)self, 27, 10);
        }
        setVector3(&v, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check(self, 0, lbl_80796E34, lbl_80796E1C) == 1) {
            eft_em_spawn_view1(self, 0, 26, &v, lbl_80796E20);
        }
        if (em_frame_check(self, 3, lbl_80796E40, lbl_80796E44) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn_view1(self, 1, 26, &v, lbl_80796E20);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same two-step shape with `em_mot_set(self, 18, 10, 0)`. */
extern "C" void fn_801486A4(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 18, 10, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same two-step shape with `em_mot_set(self, 124, 2, 0)`. */
extern "C" void fn_80148720(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 124, 2, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same shape with the five-argument motion setter `em_mot_set_blend(self, 2, 50, 0, 1)`. */
extern "C" void fn_8014879C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 2, 50, 0, 1);
        break;
      case 1:
        if (em_frame_check(self, 1, lbl_80796E48, lbl_80796E1C) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The action's long body: a 300-frame timer with `em_mot_set(self, 19, 6, 0)`, six effect windows on
 * `arg == 1`, then release through `em_state_set(self, 1, 8)` (5 on the other argument). */
extern "C" void fn_80148828(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;

    VEC3_ctor(&v);
    if ((arg & 0xFF) == 0) {
        em_busy_set_c3(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 19, 6, 0);
        self->timer_0x020 = 300;
        break;
      case 1:
        if ((arg & 0xFF) == 1) {
            if (em_frame_check(self, 0, lbl_80796E4C, lbl_80796E1C) == 1 ||
                em_frame_check(self, 0, lbl_80796E50, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 51, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 0, lbl_80796E5C, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
                fn_801049D0(self, 26, 47, 0, &v, lbl_80796E64);
            }
            if (em_frame_check(self, 3, lbl_80796E68, lbl_80796E6C) == 1 ||
                em_frame_check(self, 3, lbl_80796E70, lbl_80796E74) == 1) {
                if ((system_w.field_0x0c & 3) == 0) {
                    setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                    eft_em_spawn_view1(self, 52, 26, &v, lbl_80796E20);
                }
            }
            if (em_frame_check(self, 0, lbl_80796E78, lbl_80796E1C) == 1 ||
                em_frame_check(self, 0, lbl_80796E7C, lbl_80796E1C) == 1 ||
                em_frame_check(self, 0, lbl_80796E80, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 51, 26, &v, lbl_80796E84);
            }
            if (em_frame_check(self, 0, lbl_80796E50, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
                fn_801049D0(self, 26, 47, 0, &v, lbl_80796E64);
            }
            if (em_frame_check(self, 3, lbl_80796E88, lbl_80796E8C) == 1 ||
                em_frame_check(self, 3, lbl_80796E90, lbl_80796E94) == 1) {
                if ((system_w.field_0x0c & 3) == 0) {
                    setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                    eft_em_spawn_view1(self, 52, 26, &v, lbl_80796E98);
                }
            }
            if (em_frame_check(self, 0, lbl_80796E9C, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 51, 26, &v, lbl_80796E2C);
            }
        }
        if (--self->timer_0x020 <= 0) {
            if ((arg & 0xFF) == 1) {
                em_state_set(self, 1, 8);
            } else {
                em_state_set(self, 1, 5);
            }
        }
        break;
    }
}

/* The second action's long body: `em_mot_set(self, 20, 6, 0)`, then six effect windows, three of them
 * gated by the low two bits of `system_w`'s +0x0C word. */
extern "C" void fn_80148BD0(_ENEMY_WORK* self, u32 arg) {
    VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 20, 6, 0);
        break;
      case 1:
        if ((arg & 0xFF) == 1) {
            if (em_frame_check(self, 0, lbl_80796EA0, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 51, 26, &v, lbl_80796E98);
            }
            if (em_frame_check(self, 3, lbl_80796EA4, lbl_80796EA8) == 1 &&
                (system_w.field_0x0c & 3) == 0) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 52, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 0, lbl_80796EAC, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 51, 26, &v, lbl_80796EB0);
            }
            if (em_frame_check(self, 3, lbl_80796E60, lbl_80796EB4) == 1 &&
                (system_w.field_0x0c & 3) == 0) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 52, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 0, lbl_80796EB8, lbl_80796E1C) == 1) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 51, 26, &v, lbl_80796E20);
            }
            if (em_frame_check(self, 3, lbl_80796EBC, lbl_80796E40) == 1 &&
                (system_w.field_0x0c & 3) == 0) {
                setVector3(&v, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
                eft_em_spawn_view1(self, 52, 26, &v, lbl_80796E20);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The four-step body of the second action: motion 23, then 24 with a 1800-frame timer
 * (`fn_80132224`), then 25 (`fn_80132264`) once the timer runs out, then the action-end test. */
extern "C" void fn_80148E6C(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 23, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 24, 0, 0);
            self->timer_0x020 = 1800;
            fn_80132224(self);
        }
        break;
      case 2:
        fn_8013221C(self, lbl_80796E84, 1, 10);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 25, 4, 0);
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

/* The same two-step shape with `em_mot_set(self, 17, 6, 0)`. */
extern "C" void fn_80148F88(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 17, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The second action's sub-state dispatcher (sub-states 0..10; 4/7 share the long body with 0/1 and
 * 5/8 the second one). */
extern "C" void fn_80149004(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80148528(self);
        break;
      case 1:
        fn_801486A4(self);
        break;
      case 2:
        fn_80148720(self);
        break;
      case 3:
        fn_8014879C(self);
        break;
      case 4:
        fn_80148828(self, 0);
        break;
      case 5:
        fn_80148BD0(self, 0);
        break;
      case 6:
        fn_80148E6C(self);
        break;
      case 7:
        fn_80148828(self, 1);
        break;
      case 8:
        fn_80148BD0(self, 1);
        break;
      case 9:
        fn_80148528(self);
        break;
      case 10:
        fn_80148F88(self);
        break;
    }
}

/* The third action's body: the argument picks `em_approach_start`'s seed (-500, 0, or 0 clamped to 700),
 * then `em_mot_set(self, 6, 6, 0)` and the 64-frame release test. */
extern "C" void fn_80149068(_ENEMY_WORK* self, s32 arg1, u32 arg2) {
    if ((arg2 & 0xFF) == 1) {
        em_busy_set_c3(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg1 & 0xFF) {
          default:
            em_approach_start(self, lbl_80796EC0, 0);
            break;
          case 1:
            em_approach_start(self, lbl_80796E1C, 0);
            break;
          case 2:
            em_approach_start(self, lbl_80796E1C, 0);
            if (self->value_0x378 > lbl_80796EC4) {
                self->value_0x378 = lbl_80796EC4;
            }
            break;
        }
        em_mot_set(self, 6, 6, 0);
        break;
      case 1:
        if (em_approach_step(self, 0, 64) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The fourth action's body: the joint-table pair `lbl_8056F8E0` (`em_turn_seq_start` to arm, `em_turn_seq_step`
 * to test). */
extern "C" void fn_80149170(_ENEMY_WORK* self, u32 arg) {
    if ((arg & 0xFF) == 1) {
        em_busy_set_c3(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056F8E0, 0, 0, 0);
        break;
      case 1:
        if (em_turn_seq_step(self, lbl_8056F8E0) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same shape with the second joint-table pair `lbl_8056F920` (armed with the 1 that picks its
 * second entry). */
extern "C" void fn_80149214(_ENEMY_WORK* self, u32 arg) {
    if ((arg & 0xFF) == 1) {
        em_busy_set_c3(self);
    }
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, lbl_8056F920, 0, 1, 0);
        break;
      case 1:
        if (em_turn_seq_step(self, lbl_8056F920) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The spawn record's position reset: zero the VEC3 at +0x04 and hand the record back. */
extern "C" EmSpawnRec* em_spawn_rec_init(EmSpawnRec* rec) {
    VEC3_ctor(&rec->pos);
    return rec;
}

/* The spawn helper the record is built for: fill it and hand it to `fn_801493A8`. */
extern "C" void fn_801497BC(_ENEMY_WORK* self, u32 arg, u32 flag) {
    if ((flag & 0xFF) != 0) {
        em_busy_set_c3(self);
    }
    fn_801493A8(self, (u8)arg, 0, 0);
}

/* The third action's second half: the same `em_approach_start` seed pick as `fn_80149068` with the motion
 * `em_mot_set(self, 7, 10, 0)` and the same 64-frame release test. */
extern "C" void fn_801492B8(_ENEMY_WORK* self, s32 arg) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (arg & 0xFF) {
          default:
            em_approach_start(self, lbl_80796EC0, 0);
            break;
          case 1:
            em_approach_start(self, lbl_80796E1C, 0);
            break;
          case 2:
            em_approach_start(self, lbl_80796E1C, 0);
            if (self->value_0x378 > lbl_80796EC4) {
                self->value_0x378 = lbl_80796EC4;
            }
            break;
        }
        em_mot_set(self, 7, 10, 0);
        break;
      case 1:
        if (em_approach_step(self, 0, 64) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The fourth action's four-step body: motions 41 and 67, the `lbl_805A1DA0` scale curve, then motion 26
 * and `em_turn_to_target`'s release. */
extern "C" void fn_801498C8(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 41, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 67, 0, 0);
            em_move_vec_clr(self);
        }
        break;
      case 2:
        self->field_0x314 = em_key_curve_eval(self, lbl_805A1DA0);
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 26, 6, 0);
        }
        break;
      case 3:
        em_turn_to_target(self, 256);
        if (em_mot_end_ck(self) == 1) {
            fn_80128A70_c1(self, 3, 2);
        }
        break;
    }
}

/* The same shape as `fn_801498C8` without its fourth step: the release tail is `em_action_finish_fall`. */
extern "C" void fn_80149A08(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 41, 6, 0);
        break;
      case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 67, 0, 0);
            em_move_vec_clr(self);
        }
        break;
      case 2:
        self->field_0x314 = em_key_curve_eval(self, lbl_805A1DA0);
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* The fifth action's empty step (retail compiles it to a bare `blr`). */
extern "C" void fn_80149C54(_ENEMY_WORK* self) {
}

/* The sixth action's sub-state dispatcher (sub-states 0..13; 4/5/6 all spawn through `fn_801497BC`
 * with a different flag pair, and 7/9/11/8/10/12/13 share the other handlers' argument picks). */
extern "C" void fn_80149814(_ENEMY_WORK* self) {
    switch (self->state_sub) {
      case 0:
        fn_80149068(self, 0, 0);
        break;
      case 1:
        fn_80149170(self, 0);
        break;
      case 2:
        fn_80149214(self, 0);
        break;
      case 3:
        fn_801492B8(self, 0);
        break;
      case 4:
        fn_801497BC(self, 0, 0);
        break;
      case 5:
        fn_801497BC(self, 1, 0);
        break;
      case 6:
        fn_801497BC(self, 0, 1);
        break;
      case 7:
        fn_80149068(self, 1, 0);
        break;
      case 8:
        fn_801492B8(self, 1);
        break;
      case 9:
        fn_80149068(self, 2, 0);
        break;
      case 10:
        fn_801492B8(self, 2);
        break;
      case 11:
        fn_80149068(self, 0, 1);
        break;
      case 12:
        fn_80149170(self, 1);
        break;
      case 13:
        fn_80149214(self, 1);
        break;
    }
}

/* The sixth action's five-step body: motions 26 (`em_dive_start`), 29, 46 and 65, the `em_ground_ck`
 * gate on the second step and the `em_dive_step` joint refresh on it. */
extern "C" void fn_80149AFC(_ENEMY_WORK* self) {
    switch (self->state) {
      case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 26, 6, 0);
        em_dive_start(self);
        break;
      case 1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 29, 6, 0);
        }
        break;
      case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 46, 6, 0);
        }
        break;
      case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 65, 0, 0);
        }
        break;
      case 4:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The seventh action's body: motion 31, then release toward the target with the angle rate (per frame at
 * 60 fps) fed to `fn_8012F860` and `fn_8012F8EC`'s frame count to `fn_8012F7D4`. */
extern "C" void fn_80149C58(_ENEMY_WORK* self) {
    u16 angle;
    u16 diff;
    f32 rate;

    switch (self->state) {
      case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        fn_8012F810(self);
        em_mot_set(self, 31, 0, 0);
        break;
      case 1:
        em_target_pos_set(self, 0);
        angle = (u16)calcVecAng2(&self->pos, &self->vec_0x36C);
        diff = (u16)(angle - self->field_0x1C0);
        if (diff != 0) {
            rate = (f32)(s16)diff * lbl_80796ED0 / lbl_80796ED4 / lbl_80796ED8;
        } else {
            rate = lbl_80796E1C;
        }
        fn_8012F860(self, rate, lbl_80796EDC);
        fn_8012F7D4(self, 35, 36, (s32)fn_8012F8EC(self), self->field_0x464);
        break;
    }
}

/* The sixth action's six-state spawn body: motions 214 and 215, the shell call and the two spawn paths the
 * arguments pick, then motion 27/216/217 and `em_action_finish_walk`. */
extern "C" void fn_801493A8(_ENEMY_WORK* self, u32 arg1, s32 arg2, u32 arg3) {
    EmSpawnRec rec;
    u32 v;

    em_spawn_rec_init(&rec);
    switch (self->state) {
      case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 214, 4, 0);
        break;
      case 1:
        if ((arg2 & 0xFF) == 2 || (arg2 & 0xFF) == 4) {
            em_turn_to_target(self, 1024);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            fn_801303EC(self, lbl_80796E60);
            em_mot_set(self, 215, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x318 = em_key_curve_eval(self, lbl_805A1F70);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if ((arg3 & 0xFF) == 1) {
                fn_803B9BA0(self, &self->pos, 100);
                fn_802B9574(0);
            }
        }
        break;
      case 2:
        v = (u8)arg2 - 1;
        if (v <= 3 && em_frame_check(self, 0, lbl_80796EC8, lbl_80796E1C) == 1) {
            fn_801545B8(&rec, 1, 7646, 0);
            if (v <= 1) {
                rec.field_0x10 |= 64;
                if ((arg3 & 0xFF) == 1) {
                    rec.field_0x10 |= 128;
                }
                shell_set_func_ptr->method_0x3C(self, &rec, 0, shell_set_func_ptr);
                eft007_part_spawn(self, 23, rec.field_0x12, rec.field_0x14);
                eft007_part_spawn(self, 2, rec.field_0x12, rec.field_0x14);
            } else if ((u8)arg2 - 3 <= 1) {
                shell_set_func_ptr->method_0x3C(self, &rec, 1, shell_set_func_ptr);
                eft007_part_spawn(self, 4, rec.field_0x12, rec.field_0x14);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            self->field_0x324 = lbl_80796ECC;
            em_move_offset_step(self, &self->field_0x1BC);
            em_mot_set(self, 27, 6, 0);
        } else {
            self->field_0x318 = em_key_curve_eval(self, lbl_805A1F70);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        break;
      case 3:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 > lbl_80796E1C) {
            self->field_0x318 = lbl_80796E1C;
        }
        if (em_mot_end_ck(self) == 1) {
            switch ((u8)v) {
              case 0:
                switch (self->state_0x006) {
                  case 0:
                    self->state_0x006++;
                    em_mot_set(self, 27, 0, 0);
                    break;
                  case 1:
                    self->state++;
                    em_mot_set(self, 216, 6, 0);
                    break;
                }
                break;
              case 1:
                em_action_finish_walk(self);
                break;
            }
        }
        break;
      case 4:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 217, 0, 0);
        }
        break;
      case 5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

extern "C" {
/* Runs one enemy action step: advances the two-state machine and dispatches the per-action body. */
void fn_80149D6C(_ENEMY_WORK* self, u8 arg)
{
    em_busy_set_c1(self);

    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        fn_80134F70(self, lbl_8056F9E0);
        em_approach_start_view1(self, lbl_80796E1C, 25);
        em_fall_height_get(self);
        fn_80135584(self, (Vec3*)&self->field_0x1BC);
        return;

    case 1:
        if ((arg & 0xFF) == 1) {
            switch (self->state_0x006) {
            case 0: {
                _ENEMY_WORK* v;

                if (self->field_0x1F9 != 0) {
                    break;
                }
                self->state_0x006++;
                if (fn_8012EC3C_c1(self) == 1 || self->field_0x8A2 < 500) {
                    v = fn_80131034(self, 27, 0);
                    if (v != 0 && fn_8012E5A8_c1(v) == 1) {
                        v = 0;
                    }
                } else {
                    v = 0;
                }
                if (v != 0) {
                    self->state_0x007 = 1;
                    break;
                }
                if (self->field_0x43D == 1 && fn_80131BD4(self) == 1) {
                    self->state_0x007 = 2;
                }
                break;
            }

            case 1:
                switch (self->state_0x007) {
                case 1: {
                    _ENEMY_WORK* v = fn_80131034(self, 27, 1);

                    if (v != 0 && fn_8012E5A8_c1(v) == 1) {
                        v = 0;
                    }
                    if (v != 0) {
                        fn_8012B380_view1(self, 3, 2, v->group);
                        em_state_set(self, 13, 0);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }

                case 2:
                    if (calcDistanceSqXZ(
                            &self->pos, &self->vec_0x36C) <= lbl_80796EE8) {
                        em_state_set(self, 3, 8);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }
                break;
            }
        }

        if (em_approach_step(self, 0, 0) == 1) {
            switch (arg) {
            case 0:
                fn_801481FC(self);
                break;

            case 1:
                if (self->area_no == self->field_0x9F8 || self->field_0x9F8 == 255) {
                    em_state_set(self, 3, 10);
                } else {
                    em_state_set(self, 3, 9);
                }
                break;

            case 2:
            case 3:
            case 4:
                if (self->field_0x1E7 == 0) {
                    switch (arg) {
                    default:
                        fn_801481FC(self);
                        break;

                    case 3:
                        fn_8012B380_view1(self, 0, 0, 0);
                        if (self->field_0x382 == 255) {
                            fn_8012B380_view1(self, 5, 9, 0);
                        }
                        em_state_set(self, 7, 28);
                        break;

                    case 4:
                        fn_8012B380_view1(self, 0, 0, 0);
                        if (self->field_0x382 == 255) {
                            fn_8012B380_view1(self, 5, 9, 0);
                        }
                        if (fn_8012EC3C_c1(self) == 1) {
                            em_state_set(self, 7, 57);
                        } else {
                            em_state_set(self, 7, 29);
                        }
                        break;
                    }
                } else {
                    fn_801277F4(self, 0);
                    switch (arg) {
                    default:
                        fn_80128A70_view1(self, 3, 14);
                        break;
                    case 3:
                        fn_80128A70_view1(self, 3, 22);
                        break;
                    case 4:
                        fn_80128A70_view1(self, 3, 23);
                        break;
                    }
                }
                break;
            }
        } else {
            switch (arg) {
            case 2:
            case 3:
            case 4:
                fn_80135000(self, 2, lbl_8056F9E0);
                break;

            case 0:
                fn_80135000(self, 1, lbl_8056F9E0);
                break;

            default:
                fn_80135000(self, 0, lbl_8056F9E0);
                break;
            }
            em_fall_height_get(self);
            fn_80135584(self, (Vec3*)&self->field_0x1BC);
        }
        break;
    }
}
}

#pragma peephole on

extern "C" {
void fn_8014A1BC(_ENEMY_WORK *self, s32 arg1, s32 arg2) {
    f32 temp_f2;
    f32 var_f3;
    u8 temp_r0;
    u8 temp_r3;

    if ((arg2 & 0xFF) == 1) {
        em_busy_set_c2();
    }
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0U;
        self->state_0x007 = 0U;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_move_vec_clr(self);
        return;
    case 1:
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            self->offset_0x30C.vec_0x310.y = em_key_curve_eval(self, &lbl_805A1CC8);
            em_move_offset_apply(self);
            if (em_mot_end_ck(self) == 1U) {
                if (++self->state_0x007 >= 2U) {
                    self->state_0x006 = (u8) (self->state_0x006 + 1);
                    em_lift_start(self);
                }
            }
            break;
        case 1:
            em_lift_step(self);
            break;
        }
        if ((arg1 & 0xFF) != 1) {
            var_f3 = fn_802B0430(self->area_no) - lbl_80796EEC;
            temp_f2 = self->field_0x20C;
            if ((var_f3 - temp_f2) < lbl_80796EF0) {
                var_f3 = lbl_80796EF0 + temp_f2;
            }
        } else {
            var_f3 = lbl_80796EF4 + self->vec_0x36C.y;
        }
        if (self->pos.y >= var_f3) {
            em_action_finish_fall(self);
        }
        return;
    }
}

/* Second-level state machine: advance to the arrival substate on a frame check. */
void fn_8014A334(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r3;

    if ((u8) arg1 == 1) {
        em_busy_set_c2();
    }
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1U) {
            if ((arg1 & 0xFF) != 2) {
                if ((arg1 & 0xFF) != 3) {
                    em_action_finish_fall(self);
                    break;
                }
                em_state_set(self, 3, 0x15);
                break;
            }
            em_state_set(self, 3, 0x14);
        }
        break;
    }
}

/* The charge/scale walk: step through the approach substates and scale the offset. */
void fn_8014A418(_ENEMY_WORK *self) {
    u32 var_r30;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r3;

    em_busy_set_c2();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0x3D, 6, 0);
            self->vec_0x36C.y = self->vec_0x36C.y + (lbl_80796EF8 * get_em_chg_scale(self));
            self->state_0x006 = 0U;
            return;
        }
        return;
    case 2:
        fn_80133C3C(self);
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            fn_80133CC8(self, 0x100, 0x100);
            if (em_frame_check(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                em_move_vec2_clr(self);
                self->offset_0x30C.vec_0x310.z = lbl_80796F00;
                self->field_0x324 = lbl_80796F04;
                em_approach_start(self, lbl_80796F08, 0x10);
            }
            break;
        case 1:
            em_fall_height_get(self);
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            em_approach_step(self, 0, 0x80U);
            if (self->offset_0x30C.vec_0x310.z > lbl_80796E58) {
                self->offset_0x30C.vec_0x310.z = lbl_80796E58;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0x3E, 6, 0);
            return;
        }
        break;
    case 3:
        fn_80133C3C(self);
        if (em_approach_step(self, 0, 0x80U) == 1) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0x2D, 6, 0);
            self->state_0x006 = 0U;
            self->offset_0x30C.vec_0x310.y = lbl_80796F0C;
            self->field_0x324 = lbl_80796F10;
            em_fall_height_get(self);
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            return;
        }
        em_fall_height_get(self);
        em_move_offset_step_update_c1(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > lbl_80796E58) {
            self->offset_0x30C.vec_0x310.z = lbl_80796E58;
            return;
        }
        break;
    case 4:
        temp_r0_2 = self->state_0x006;
        switch ((s32) temp_r0_2) {
        case 0:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                self->field_0x324 = lbl_80796F14;
            }
            break;
        case 1:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z < lbl_80796E1C) {
                self->offset_0x30C.vec_0x310.z = lbl_80796E1C;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            if (var_r30 == 1U) {
                self->state = (u8) (self->state + 1);
                em_move_mode_set(self, 3);
                em_mot_set(self, 0x1D, 6, 0);
                return;
            }
            if (em_busy_ck(self) == 1U) {
                em_action_finish_fall(self);
                return;
            }
        }
        break;
    case 5:
        if (em_mot_end_ck(self) == 1U) {
            em_move_mode_set(self, 3);
            em_action_finish_walk(self);
        }
        break;
    }
}

/* The short two-state variant: arm a hit check, then run the frame check. */
void fn_8014A7B0(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r3;

    em_busy_set_c2();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        break;
    case 1:
        if ((arg1 & 0xFF) == 1) {
            em_turn_to_target(self, 0x180);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* A plain countdown state: arm a 60-frame timer, then fire when it runs out. */
void fn_8014A860(_ENEMY_WORK *self) {
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1F, 4, 0);
        self->timer_0x020 = 0x3C;
        break;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            fn_801481FC(self);
        }
        break;
    }
}

/* The arrival substate machine: wait for the frame check, then hand off to the caller. */
void fn_8014A8EC(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r0;
    u8 temp_r3;

    em_busy_set_c2();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0U;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x3D, 6, 0);
        break;
    case 1:
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            if (em_frame_check(self, 1, lbl_80796E58, lbl_80796E1C) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                em_move_vec2_clr(self);
                self->offset_0x30C.vec_0x310.z = lbl_80796F00;
                self->field_0x324 = lbl_80796F04;
            }
            break;
        case 1:
            em_fall_height_get(self);
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z > lbl_80796F18) {
                self->offset_0x30C.vec_0x310.z = lbl_80796F18;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    fn_801481FC(self);
                    break;
                }
                em_state_set(self, 3, 0x17);
                break;
            }
            em_state_set(self, 3, 0x16);
            break;
        }
        break;
    }
}

/* The state-0x1E6 dispatcher: tail-call the handler for the current action code. */
void fn_8014AA4C(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0: fn_801498C8(self); break;
    case 1: fn_80149A08(self); break;
    case 2: fn_80149AFC_c1(self); break;
    case 3: fn_80149C54(self); break;
    case 4: fn_80149C58(self); break;
    case 5: fn_80149D6C(self, 0); break;
    case 6: fn_8014A1BC(self, 0, 0); break;
    case 7: fn_8014A334(self, 0); break;
    case 8: fn_8014A418(self); break;
    case 9: fn_80149D6C(self, 1); break;
    case 10: fn_8014A7B0(self, 0); break;
    case 11: fn_8014A860(self); break;
    case 12: fn_8014A8EC(self, 0); break;
    case 13: fn_8014A1BC(self, 1, 0); break;
    case 14: fn_80149D6C(self, 2); break;
    case 15: fn_8014A7B0(self, 1); break;
    case 16: fn_8014A1BC(self, 0, 1); break;
    case 17: fn_8014A334(self, 1); break;
    case 18: fn_8014A334(self, 2); break;
    case 19: fn_8014A334(self, 3); break;
    case 20: fn_8014A8EC(self, 1); break;
    case 21: fn_8014A8EC(self, 2); break;
    case 22: fn_80149D6C(self, 3); break;
    case 23: fn_80149D6C(self, 4); break;
    }
}

/* A three-state handoff: mark the action, wait for the frame check, then relink. */
void fn_8014AB24(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x44, 0, 0);
            return;
        }
        return;
    case 2:
        fn_80154CA4(self);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* The same three-state shape as `fn_8014AB24`, ending in the shared post-action handler. */
void fn_8014ABEC(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x2E, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* Holds the current pose until the frame check, then fades out or returns to the base pose. */
void fn_8014ACAC(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1E, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
            self->state = (u8) (self->state + 1);
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = lbl_80796F00 * get_em_base_scale(self);
            self->field_0x320 = lbl_80796F1C * get_em_base_scale(self);
            return;
        }
        return;
    case 2:
        if (em_frame_check(self, 1, lbl_80796EBC, lbl_80796E1C) == 0) {
            em_move_offset_apply(self);
        } else {
            CancelFade(self);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* Runs a one-shot transition, then hands the finished state to the shared post-action. */
void fn_8014ADC8(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck_c1(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x1D, 6, 0);
            return;
        }
        return;
    case 2:
        fn_80154CA4(self);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* Plays the shared animation table, then hands the finished state to the post-action. */
void fn_8014AEAC(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        em_turn_seq_start(self, &lbl_8056F9A0, 0, 0, 0U);
        break;
    case 1:
        if (em_turn_seq_step(self, &lbl_8056F9A0) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* Arms the approach scale and pushes the offset to its limit while the frame check runs. */
void fn_8014AF44(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 4, 0);
        em_approach_start(self, lbl_80796F08, 0);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = lbl_80796EA8;
        self->field_0x324 = lbl_80796E20;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80U) == 1) {
            em_action_finish_walk(self);
            return;
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > lbl_80796E58) {
            self->offset_0x30C.vec_0x310.z = lbl_80796E58;
        }
        return;
    }
}

/* Plays the stance table until the frame check, then hands off to the post-action. */
void fn_8014B020(_ENEMY_WORK *self) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        fn_80134F70(self, &lbl_8056FA0C);
        em_approach_start(self, lbl_80796E1C, 9);
        break;
    case 1:
        if (em_approach_step(self, 0, 0U) == 1) {
            em_action_finish_walk(self);
            return;
        }
        fn_80135000(self, 0, &lbl_8056FA0C);
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        return;
    }
}

/* Arms the pivot action, then holds the offset and lets the frame check finish it. */
void fn_8014B0E8(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x42, 0, 0);
            self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &lbl_805A1D88);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            return;
        }
        return;
    case 2:
        fn_80154CA4(self);
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &lbl_805A1D88);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* The mirrored strafe: pick the turn direction from the argument and scale the offset. */
void fn_8014B1F8(_ENEMY_WORK *self, s32 arg1) {
    s32 sp10;
    s32 spC;
    s32 sp8;
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        if (arg1 == 0) {
            em_mot_set(self, 0xCE, 6, 0);
        } else {
            em_mot_set(self, 0xCF, 6, 0);
        }
        em_move_vec_clr(self);
        self->field_0x37C = self->field_0x1C0;
        self->float_0x328 = fn_801356A8(self, lbl_80796E20, lbl_80796F20, lbl_80796F24);
        break;
    case 1:
        if (arg1 == 0) {
            em_turn_in_window(self, lbl_80796ED8, lbl_80796F28, -0x4000);
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &lbl_805A1FF0);
        } else {
            em_turn_in_window(self, lbl_80796ED8, lbl_80796F28, 0x4000);
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, &lbl_805A1FF0);
        }
        self->offset_0x30C.vec_0x310.z = self->float_0x328 * em_key_curve_eval(self, &lbl_805A2028);
        sp8 = 0;
        spC = self->field_0x37C;
        sp10 = 0;
        em_move_offset_rot_apply(self, &sp8);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* Sparks the two-stage hit effect while the action runs, at most once every eight frames. */
void fn_8014B378(_ENEMY_WORK *self) {
    VEC3 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp8);
    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1C, 6, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_80796F18, lbl_80796E1C) == 1U) {
            em_hit_window_set(self, 0, 8, 5);
            draw_shape_arm(self, 0x1B, 0xA);
        }
        setVector3(&sp8, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check(self, 0, lbl_80796F18, lbl_80796E1C) == 1U) {
            eft_em_spawn(self, 0, 0x1A, &sp8, lbl_80796E20);
        }
        if (em_frame_check(self, 2, lbl_80796E4C, lbl_80796F2C) == 1U) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x1A, &sp8, lbl_80796E20);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

/* Counts a fixed number of substeps (three or six, by the argument) before finishing. */
void fn_8014B4EC(_ENEMY_WORK *self, s32 arg1) {
    VEC3 sp8;
    u32 var_r31;
    u8 temp_r0;
    u8 temp_r3;

    VEC3_ctor(&sp8);
    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0U;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        break;
    case 1:
        var_r31 = 6U;
        if (arg1 == 0) {
            var_r31 = 3U;
        }
        if (em_mot_end_ck(self) == 1U) {
            temp_r0 = self->state_0x006 + 1;
            self->state_0x006 = temp_r0;
            if (temp_r0 >= var_r31) {
                em_action_finish_walk(self);
            }
        }
        return;
    }
}

/* The state-0x1E6 dispatcher for the second action group: tail-call the handler. */
void fn_8014B5C0(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0: fn_8014AB24(self); break;
    case 1: fn_8014ABEC(self); break;
    case 2: fn_8014ACAC(self); break;
    case 3: fn_8014ADC8(self); break;
    case 4: fn_8014AEAC(self); break;
    case 5: fn_8014AF44(self); break;
    case 6: fn_8014B020(self); break;
    case 7: fn_8014B0E8(self); break;
    case 8: fn_8014B1F8(self, 0); break;
    case 9: fn_8014B1F8(self, 1); break;
    case 10: fn_8014B378(self); break;
    case 11: fn_8014B378(self); break;
    case 12: fn_8014B4EC(self, 0); break;
    case 13: fn_8014B4EC(self, 1); break;
    }
}

/* Arms the two sub-part motions and runs the shared post-action on the frame check. */
void fn_8014B630(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2F, 4, 0);
        em_hit_window_set_default_c1(self, 0, 1);
        em_hit_window_set_default_c1(self, 1, 9);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

/* The long action: approach, hold and release the scale over four outer states. */
void fn_8014B6CC(_ENEMY_WORK *self, s32 arg1) {
    s32 temp_r0_2;
    s32 temp_r0_4;
    s32 temp_r4;
    s32 temp_r4_2;
    u8 temp_r0;
    u8 temp_r0_3;
    u8 temp_r0_5;
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        self->state_0x006 = 0U;
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0xD1, 6, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        fn_801303FC_c1(self, lbl_80796F30);
        if (self->field_0x1AC < lbl_80796F34) {
            self->field_0x1AC = lbl_80796F34;
        }
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            em_turn_to_target(self, 0x100);
            if (em_frame_check(self, 1, lbl_80796E60, lbl_80796E1C) != 1U) {
                return;
            }
            self->state_0x006 = (u8) (self->state_0x006 + 1);
            em_move_vec2_clr(self);
            self->field_0x324 = lbl_80796F04;
            em_approach_start(self, lbl_80796E1C, 0x10);
            em_hit_window_set(self, 0, 0xE, 3);
            return;
        case 1:
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z > lbl_80796E34) {
                self->offset_0x30C.vec_0x310.z = lbl_80796E34;
            }
            if (em_mot_end_ck(self) == 1U) {
                self->state = (u8) (self->state + 1);
                em_mot_set(self, 0xD0, 6, 0);
                return;
            }
            break;
        }
        break;
    case 2:
        fn_801303EC(self, lbl_80796F34);
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > lbl_80796E34) {
            self->offset_0x30C.vec_0x310.z = lbl_80796E34;
        }
        if ((u8) em_approach_step(self, 0, 0U) == 1) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            switch (arg1 & 0xFF) {
            default:
                em_mot_set(self, 0xD2, 6, 0);
                self->field_0x324 = lbl_80796F38;
                self->timer_0x020 = 0;
                return;
            case 1:
                em_move_mode_set(self, 0);
                em_mot_set(self, 0xDD, 0, 0);
                fn_801303EC(self, lbl_80796E1C);
                return;
            case 2:
                em_mot_set(self, 0x35, 0xA, 0);
                return;
            }
        }
        break;
    case 3:
        switch (arg1 & 0xFF) {
        default:
            temp_r4 = self->timer_0x020;
            if (temp_r4 < 0x14) {
                temp_r0_2 = temp_r4 + 1;
                self->timer_0x020 = temp_r0_2;
                fn_801303EC(self, (lbl_80796F3C * (f32) (0x14 - temp_r0_2)) / lbl_80796E0C);
            }
            if (em_frame_check(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                em_hit_window_clear(self, 0);
            }
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z < lbl_80796E1C) {
                self->offset_0x30C.vec_0x310.z = lbl_80796E1C;
            }
            if (em_mot_end_ck(self) == 1U) {
                em_action_finish_walk(self);
                return;
            }
            break;
        case 1:
            temp_r0_5 = self->state_0x006;
            switch ((s32) temp_r0_5) {
            case 0:
                if (em_mot_end_ck(self) == 1U) {
                    self->state_0x006 = (u8) (self->state_0x006 + 1);
                    em_mot_set(self, 0xDA, 0, 0);
                    return;
                }
                break;
            case 1:
                if (em_frame_check(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                    em_hit_window_clear(self, 0);
                }
                if (em_mot_end_ck(self) == 1U) {
                    em_action_finish(self);
                    return;
                }
                break;
            }
            break;
        case 2:
            temp_r0_3 = self->state_0x006;
            switch ((s32) temp_r0_3) {
            case 0:
                temp_r4_2 = self->timer_0x020;
                if (temp_r4_2 < 0xA) {
                    temp_r0_4 = temp_r4_2 + 1;
                    self->timer_0x020 = temp_r0_4;
                    fn_801303EC(self, (lbl_80796F34 * (f32) (5 - temp_r0_4)) / lbl_80796F00);
                }
                if (em_mot_end_ck(self) == 1U) {
                    self->state_0x006 = (u8) (self->state_0x006 + 1);
                    em_hit_window_clear(self, 0);
                    em_move_mode_set(self, 0);
                    em_mot_set(self, 0x3C, 0, 0);
                    return;
                }
                break;
            case 1:
                if (em_mot_end_ck(self) == 1U) {
                    em_action_finish(self);
                }
                break;
            }
            break;
        }
        break;
    }
}

/* Picks one of four action ids from two flags and hands it to the action starter. */
void fn_8014BDAC(_ENEMY_WORK *a0, s32 arg1, s32 arg2, s32 arg3) {
    u8 var_r0;

    {
        u8 a2 = arg2;
        u8 a3 = arg3;
        if (a2 == 0) {
            if (a3 == 0) {
                var_r0 = 1;
            } else {
                var_r0 = 3;
            }
        } else {
            var_r0 = 4;
            if (a3 == 0) {
                var_r0 = 2;
            }
        }
    }
    fn_801493A8_c1(a0, arg1, var_r0, 0);
}

/* The two-motion grab: pick the motion ids from the argument and run both sub-motions. */
void fn_8014BDF8(_ENEMY_WORK *self, s32 arg1) {
    u32 var_r27;
    u32 var_r28;
    u32 var_r29;
    u32 var_r30;
    u8 temp_r3;
    u8 temp_r3_2;
    u8 temp_r5;

    if ((arg1 & 0xFF) != 1) {
        var_r30 = 0x32;
        var_r29 = 0x1D;
        var_r28 = 0x22;
        var_r27 = 0x23;
    } else {
        var_r30 = 0x31;
        var_r29 = 4;
        var_r28 = 0x20;
        var_r27 = 0x21;
    }
    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        self->state_0x006 = 0U;
        em_move_mode_set(self, 0);
        em_mot_set(self, var_r30, 4, 0);
        em_hit_window_set(self, 0, var_r29, 8);
        em_hit_window_set(self, 1, var_r28, 0x18);
        break;
    case 1:
        fn_80133F4C_c1(self, lbl_80796E1C, lbl_80796F28);
        temp_r3 = self->state_0x006;
        if ((temp_r3 == 0) && (self->field_0xA69 == 0)) {
            self->state_0x006 = (u8) (temp_r3 + 1);
            em_hit_window_set(self, 1, var_r27, 0x10);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            em_mot_set(self, var_r30, 0, 0);
            em_hit_window_set(self, 0, var_r29, 8);
            em_hit_window_set(self, 1, var_r28, 0x18);
            return;
        }
        return;
    case 2:
        fn_80133F4C_c1(self, lbl_80796E1C, lbl_80796F28);
        temp_r3_2 = self->state_0x006;
        if ((temp_r3_2 == 0) && (self->field_0xA69 == 0)) {
            self->state_0x006 = (u8) (temp_r3_2 + 1);
            em_hit_window_set(self, 1, var_r27, 0x10);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* The approach-and-turn action: drive the two scale motions and hand off when done. */
void fn_8014BFE4(_ENEMY_WORK *self, s32 arg1) {
    f32 temp_f31;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0xCE, 0xA, 0);
        em_hit_window_set_default_c1(self, 0, 0x1B);
        break;
    case 1:
        em_busy_set_c2();
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            if (arg1 == 0) {
                em_move_mode_set(self, 0);
                em_mot_set(self, 0xCF, 0, 0);
                em_hit_window_set_default_c1(self, 0, 0x16);
                return;
            }
            em_move_mode_set(self, 3);
            em_action_finish_walk(self);
            return;
        }
        if (em_frame_check(self, 1, lbl_80796F4C, lbl_80796E1C) == 1U) {
            temp_f31 = fn_8012F8F4(self) - lbl_80796F4C;
            fn_801303EC(self, lbl_80796F3C * ((fn_8012F8EC(self) - lbl_80796F4C) / temp_f31));
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* The entry action: pick a scale profile from the argument, then run the frame checks. */
void fn_8014C16C(_ENEMY_WORK *self, s32 arg1) {
    s32 temp_r0;
    u16 var_r5;
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        em_move_mode_set(self, 0);
        fn_80154C74(self);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 5, 3);
        self->timer_0x020 = 0;
        switch (arg1) {
        case 0:
            em_approach_start(self, lbl_80796F08, 0x10);
            self->timer_0x020 = 0x14;
            return;
        case 1:
            em_approach_start(self, lbl_80796F50, 0x10);
            self->timer_0x020 = 0xA;
            return;
        case 2:
            em_approach_start(self, lbl_80796F08, 0);
            if (self->value_0x378 == lbl_80796EEC) {
                self->value_0x378 = lbl_80796EEC;
                return;
            }
            return;
        default:
            em_approach_start(self, lbl_80796F08, 0);
            return;
        case 4:
            em_approach_start(self, lbl_80796F54, 0x10);
            return;
        }
        break;
    case 1:
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 <= 0) {
            self->timer_0x020 = 0;
        }
        var_r5 = 0x40;
        if (arg1 == 0) {
            var_r5 = 0x80;
        }
        if ((em_approach_step(self, 0, var_r5) == 1) && (self->timer_0x020 <= 0)) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            switch (arg1) {
            default:
                em_hit_window_clear(self, 0);
                em_mot_set(self, 9, 4, 0);
                return;
            case 2:
                em_mot_set(self, 0x40, 4, 0);
                return;
            case 3:
                em_mot_set(self, 0x30, 4, 0);
                return;
            case 4:
                em_hit_window_clear(self, 0);
                fn_80128A70_c1(self, 4, 7);
                return;
            }
        }
        break;
    case 2:
        switch (arg1) {
        case 2:
            if ((self->state_0x006 == 0) && (em_frame_check(self, 1, lbl_80796F28, lbl_80796E1C) == 1U)) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                em_hit_window_clear(self, 0);
            }
            break;
        case 3:
            if ((self->state_0x006 == 0) && (em_frame_check(self, 1, lbl_80796F58, lbl_80796E1C) == 1U)) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                em_hit_window_set_default_c1(self, 0, 0xD);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the short stance and finishes it on the frame check. */
void fn_8014CBA0(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        fn_80154C74(self);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 5, 2);
        em_approach_start(self, lbl_80796F08, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0x2F, 0, 0);
            em_hit_window_set_default_c1(self, 0, 1);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

/* Scales the offset from the distance to the target and holds it while the frame runs. */
void fn_8014CC98(_ENEMY_WORK *self) {
    f32 temp_f1;
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0x36, 2, 0);
        em_hit_window_set_default_c1(self, 0, 0xC);
        em_move_vec_clr(self);
        temp_f1 = fn_80050EF4(&self->pos, &self->vec_0x36C);
        if (temp_f1 > lbl_80796EF8) {
            self->offset_0x30C.vec_0x310.z = lbl_80796EA0;
            return;
        }
        if (temp_f1 > lbl_80796F64) {
            self->offset_0x30C.vec_0x310.z = (lbl_80796EA0 * temp_f1) / lbl_80796F4C;
            return;
        }
        self->offset_0x30C.vec_0x310.z = lbl_80796E1C;
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80796ED8, lbl_80796E1C) == 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            em_turn_to_target(self, 0x40);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_action_finish_walk(self);
        }
        return;
    }
}

/* The turn-and-hold action: hold the offset while the frame check runs, then finish. */
void fn_8014CDD0(_ENEMY_WORK *self, s32 arg1) {
    u8 temp_r3;

    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0x37, 2, 0);
        em_hit_window_set_default_c1(self, 0, 2);
        if ((arg1 & 0xFF) == 1) {
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.z = lbl_80796E0C;
            self->field_0x324 = lbl_80796F68;
            return;
        }
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_action_finish_walk(self);
            return;
        }
        if (((arg1 & 0xFF) == 1) && (em_frame_check(self, 1, lbl_80796ED8, lbl_80796E1C) == 0)) {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z < lbl_80796E1C) {
                self->offset_0x30C.vec_0x310.z = lbl_80796E1C;
            }
        }
        break;
    }
}

void fn_8014C6DC(_ENEMY_WORK *self, u8 arg1) {
    f32 temp_f31;
    f32 temp_f31_2;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r5;

    temp_r5 = self->state;
    switch ((s32) temp_r5) {
    case 0:
        self->state = (u8) (temp_r5 + 1);
        em_move_mode_set(self, 0);
        fn_80154C74(self);
        em_mot_set(self, 0xCD, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0xCE, 0, 0);
            em_hit_window_set_default_c1(self, 0, 7);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.z = (f32) (lbl_80796F60 * get_em_base_scale(self));
            self->field_0x324 = (f32) (lbl_80796F38 * get_em_base_scale(self));
            return;
        }
        return;
    case 2:
        self->offset_0x30C.vec_0x310.y = em_key_curve_eval(self, &lbl_805A1E50);
        temp_f31 = em_fall_height_get(self);
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if ((em_frame_check(self, 1, lbl_80796EA4, lbl_80796E1C) == 1U) || (self->offset_0x30C.vec_0x310.z < lbl_80796E1C)) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        case 1:
            fn_80135584(self, &self->field_0x1BC);
            break;
        }
        if ((em_ground_ck_c2(self, temp_f31) == 1U) && (em_mot_end_ck(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            switch ((arg1 & 0xFF)) {
            case 0:
                em_move_mode_set(self, 0);
                em_mot_set(self, 0xCF, 0, 0);
                em_hit_window_set_default_c1(self, 0, 0x16);
                return;
            case 1:
                em_move_mode_set(self, 3);
                em_mot_set(self, 0x1B, 6, 0);
                return;
            case 2:
                em_move_mode_set(self, 0);
                em_mot_set(self, 0xD2, 0, 0);
                em_hit_window_set_default_c1(self, 0, 0x16);
                return;
            default:
                return;
            }
        } else {
            return;
        }
        break;
    case 3:
        switch ((arg1 & 0xFF)) {
        case 0:
            if (em_mot_end_ck(self) == 1U) {
                em_action_finish(self);
                return;
            }
            return;
        case 1:
            fn_80154CA4(self);
            if (em_mot_end_ck(self) == 1U) {
                em_action_finish_walk(self);
                return;
            }
            return;
        case 2:
            if (em_mot_end_ck(self) == 1U) {
                self->state = (u8) (self->state + 1);
                em_mot_set(self, 0xCD, 0, 0x46);
                return;
            }
            return;
        default:
            return;
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0xCE, 0, 0);
            em_hit_window_set_default_c1(self, 0, 7);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.z = (f32) (lbl_80796F60 * get_em_base_scale(self));
            self->field_0x324 = (f32) (lbl_80796F38 * get_em_base_scale(self));
            return;
        }
        return;
    case 5:
        self->offset_0x30C.vec_0x310.y = em_key_curve_eval(self, &lbl_805A1E50);
        temp_f31_2 = em_fall_height_get(self);
        temp_r0_2 = self->state_0x006;
        switch ((s32) temp_r0_2) {
        case 0:
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if ((em_frame_check(self, 1, lbl_80796EA4, lbl_80796E1C) == 1U) || (self->offset_0x30C.vec_0x310.z < lbl_80796E1C)) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        case 1:
            fn_80135584(self, &self->field_0x1BC);
            break;
        }
        if ((em_ground_ck_c2(self, temp_f31_2) == 1U) && (em_mot_end_ck(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xCF, 0, 0);
            em_hit_window_set_default_c1(self, 0, 0x16);
            return;
        }
        return;
    case 6:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

void fn_8014D504(_ENEMY_WORK *self) {
    u8 temp_r0;
    u8 temp_r3;

    em_busy_set_c1(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0xC8, 6, 0);
        em_hit_window_set_default_c1(self, 0, 0x17);
        em_move_vec2_clr(self);
        self->float_0x328 = fn_801356A8(self, lbl_80796F74, lbl_80796E20, lbl_80796F78);
        return;
    case 1:
        em_key_curve_eval(self, &lbl_805A1DD8);
        fn_801303FC_c2(self);
        if (self->field_0x1AC < lbl_80796E24) {
            self->field_0x1AC = (f32) lbl_80796E24;
        }
        self->offset_0x30C.vec_0x310.z = (f32) (self->float_0x328 * em_key_curve_eval(self, &lbl_805A1E20));
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        if (em_frame_check(self, 1, lbl_80796F58, lbl_80796E1C) == 0) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDC, 0, 0);
            return;
        }
        return;
    case 2:
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            if (em_frame_check(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
            case 1:
                if (em_busy_ck(self) == 1U) {
                    if (fn_8012D3E0_c1(self, 0) != 0xFF) {
                        fn_8012B380_c1(self, 1, 2);
                        em_state_set(self, 8, 0);
                        return;
                    }
                    em_state_set(self, 1, 3);
                }
            }
            break;
        }
        break;
    }
}

void fn_8014D714(_ENEMY_WORK *self) {
    u8 temp_r3;

    em_busy_set_c1(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0xD1, 6, 0);
        em_hit_window_set(self, 0, 6, 8);
        em_hit_window_set(self, 1, 3, 0x10);
        if ((u8) self->team == 2) {
            self->field_0xa7e = 0xDE;
        } else {
            self->field_0xa7e = 0xD4;
        }
        em_move_vec2_clr(self);
        fn_801303EC(self, lbl_80796E1C);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80796E3C, lbl_80796E1C) == 0) {
            em_turn_to_target(self, 0x80);
        }
        em_key_curve_eval(self, &lbl_805A1E90);
        fn_801303FC_c2(self);
        if (em_frame_check(self, 1, lbl_80796F7C, lbl_80796E1C) == 1U) {
            if (self->field_0x1AC < lbl_80796E1C) {
                self->field_0x1AC = (f32) lbl_80796E1C;
            }
        } else if (self->field_0x1AC < lbl_80796F34) {
            self->field_0x1AC = (f32) lbl_80796F34;
        }
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &lbl_805A1F00);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD4, 0, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        break;
    }
}

void fn_8014E160(_ENEMY_WORK *self) {
    f32 temp_f1;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0x38, 6, 0);
        em_move_vec2_clr(self);
        fn_801303EC(self, lbl_80796E1C);
        em_hit_window_set_default_c1(self, 0, 0x1E);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80796E60, lbl_80796E1C) == 0) {
            em_turn_to_target(self, 0x100);
        }
        em_key_curve_eval(self, &lbl_805A1D18);
        fn_801303FC_c2(self);
        temp_f1 = self->field_0x1AC;
        if (temp_f1 > lbl_80796E1C) {
            self->field_0x1AC = (f32) lbl_80796E1C;
        } else if (temp_f1 < lbl_80796F08) {
            self->field_0x1AC = (f32) lbl_80796F08;
        }
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &lbl_805A1D40);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

void fn_8014E3AC(_ENEMY_WORK *self) {
    switch (self->state_sub) {
    case 0x0: fn_8014B630(self); break;
    case 0x1: fn_8014B6CC(self, 0); break;
    case 0x2: fn_8014BB40(self, 0); break;
    case 0x3: fn_8014BDAC(self, 0, 0, 0); break;
    case 0x4: fn_8014BDF8(self, 0); break;
    case 0x5: fn_8014BFE4(self, 0); break;
    case 0x6: fn_8014C16C(self, 0); break;
    case 0x7: fn_8014C16C(self, 1); break;
    case 0x8: fn_8014C464(self, 0); break;
    case 0x9: fn_8014C6DC(self, 0); break;
    case 0xA: fn_8014BB40(self, 1); break;
    case 0xB: fn_8014BB40(self, 2); break;
    case 0xC: fn_8014BDF8(self, 1); break;
    case 0xD: fn_8014CBA0(self); break;
    case 0xE: fn_8014C6DC(self, 1); break;
    case 0xF: fn_8014BB40(self, 3); break;
    case 0x10: fn_8014C16C(self, 2); break;
    case 0x11: fn_8014CC98(self); break;
    case 0x12: fn_8014CDD0(self, 0); break;
    case 0x13: fn_8014C16C(self, 3); break;
    case 0x14: fn_8014B6CC(self, 1); break;
    case 0x15: fn_8014CDD0(self, 1); break;
    case 0x16: fn_8014CEF8(self, 0, 0); break;
    case 0x17: fn_8014CEF8(self, 1, 0); break;
    case 0x18: fn_8014CEF8(self, 0, 1); break;
    case 0x19: fn_8014CEF8(self, 1, 1); break;
    case 0x1A: fn_8014CEF8(self, 0, 2); break;
    case 0x1B: fn_8014CEF8(self, 1, 2); break;
    case 0x1C: fn_8014CEF8(self, 0, 3); break;
    case 0x1D: fn_8014CEF8(self, 1, 3); break;
    case 0x1E: fn_8014C6DC(self, 2); break;
    case 0x1F: fn_8014D504(self); break;
    case 0x20: fn_8014C16C(self, 4); break;
    case 0x21: fn_8014D714(self); break;
    case 0x22: fn_8014C464(self, 1); break;
    case 0x23: fn_8014D8F0(self, 0, 0); break;
    case 0x24: fn_8014DAF4(self, 0, 0); break;
    case 0x25: fn_8014DDEC(self); break;
    case 0x26: fn_8014DAF4(self, 1, 0); break;
    case 0x27: fn_8014DAF4(self, 2, 0); break;
    case 0x28: fn_8014DAF4(self, 3, 0); break;
    case 0x29: fn_8014BDAC(self, 1, 0, 0); break;
    case 0x2A: fn_8014B6CC(self, 2); break;
    case 0x2B: fn_8014BDAC(self, 0, 1, 0); break;
    case 0x2C: fn_8014BDAC(self, 1, 1, 0); break;
    case 0x2D: fn_8014E160(self); break;
    case 0x2E: fn_8014D8F0(self, 1, 0); break;
    case 0x2F: fn_8014D8F0(self, 2, 0); break;
    case 0x30: fn_8014D8F0(self, 3, 0); break;
    case 0x31: fn_8014DAF4(self, 0, 1); break;
    case 0x32: fn_8014DAF4(self, 1, 1); break;
    case 0x33: fn_8014DAF4(self, 2, 1); break;
    case 0x34: fn_8014DAF4(self, 3, 1); break;
    case 0x35: fn_8014BDAC(self, 0, 0, 1); break;
    case 0x36: fn_8014BDAC(self, 1, 0, 1); break;
    case 0x37: fn_8014BDAC(self, 0, 1, 1); break;
    case 0x38: fn_8014BDAC(self, 1, 1, 1); break;
    case 0x39: fn_8014CEF8(self, 2, 3); break;
    case 0x3A: fn_8014E290(self); break;
    case 0x3B: fn_8014DAF4(self, 4, 0); break;
    case 0x3C: fn_8014DAF4(self, 4, 1); break;
    case 0x3D: fn_8014BFE4(self, 1); break;
    case 0x3E: fn_8014D8F0(self, 0, 1); break;
    case 0x3F: fn_8014D8F0(self, 1, 1); break;
    case 0x40: fn_8014D8F0(self, 2, 1); break;
    case 0x41: fn_8014D8F0(self, 3, 1); break;
    }
}

void fn_8014EA70(_ENEMY_WORK *self) {
    if ((s32) self->state_sub == 0) {
        fn_8014E774(self);
    }
}

void fn_8014EA84(_ENEMY_WORK *self) {
    f32 temp_f2;
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r3;

    fn_80131EC0_c1();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0U;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0xCA, 4, 0);
        return;
    case 1:
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            if (em_mot_end_ck(self) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
            case 1:
                if (em_busy_ck(self) == 1U) {
                    if ((fn_8012D23C_c1(self, 1, self->field_0x382) == 1U) && (fn_8014E670(self, self->field_0x382) == 1U)) {
                        temp_r0_2 = self->field_0x1E7 - 1;
                        self->field_0x1E7 = temp_r0_2;
                        if ((s32) temp_r0_2 == 0) {
                            if ((u8) self->team == 2) {
                                if (fn_8012EC3C_c1(self) == 1U) {
                                    em_state_set(self, 9, 4);
                                    return;
                                }
                                em_state_set(self, 9, 3);
                                return;
                            }
                            em_state_set(self, 9, 2);
                            return;
                        }
                        fn_80130CDC(self, 0x96);
                        temp_f2 = (f32) self->field_0x7A4;
                        fn_8011E6EC_c1(self, (s32) (lbl_80796F9C * temp_f2), 1, lbl_80796E20, temp_f2);
                        em_state_set(self, 9, 1);
                        return;
                    }
                    em_state_set(self, 2, 6);
                }
            } else {
                return;
            }
            break;
        }
        break;
    }
}

void fn_8014EC50(_ENEMY_WORK *self) {
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r3;

    fn_80131EC0_c1();
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0U;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCB, 4, 0);
        temp_r0 = self->field_0x1E7;
        switch ((s32) temp_r0) {
        case 3:
            em_hit_window_set_default_c1(self, 0, 0x18);
            return;
        case 2:
            em_hit_window_set_default_c1(self, 0, 0x19);
            return;
        case 1:
            em_hit_window_set_default_c1(self, 0, 0x1A);
            return;
        }
        break;
    case 1:
        temp_r0_2 = self->state_0x006;
        switch ((s32) temp_r0_2) {
        case 0:
            if (em_mot_end_ck(self) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                em_mot_set(self, 0xCA, 4, 0);
            case 1:
                if (em_busy_ck(self) == 1U) {
                    if ((fn_8012D23C_c1(self, 1, self->field_0x382) == 1U) && (fn_8014E670(self, self->field_0x382) == 1U)) {
                        em_state_set(self, 9, 0);
                        return;
                    }
                    em_state_set(self, 2, 6);
                }
            } else {
                return;
            }
            break;
        }
        break;
    }
}

void fn_8014EDC8(_ENEMY_WORK *self) {
    f32 temp_f31;
    u8 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCD, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            fn_803B9BA0(self, &self->pos, 0x64);
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0xCE, 0, 0);
            em_hit_window_set_default_c1(self, 0, 0x1C);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.z = (f32) (lbl_80796F60 * get_em_base_scale(self));
            self->field_0x324 = (f32) (lbl_80796F38 * get_em_base_scale(self));
            fn_802B9574(0);
            return;
        }
        return;
    case 2:
        self->offset_0x30C.vec_0x310.y = em_key_curve_eval(self, &lbl_805A1E50);
        temp_f31 = em_fall_height_get(self);
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if ((em_frame_check(self, 1, lbl_80796EA4, lbl_80796E1C) == 1U) || (self->offset_0x30C.vec_0x310.z < lbl_80796E1C)) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        case 1:
            fn_80135584(self, &self->field_0x1BC);
            break;
        }
        if ((em_ground_ck_c2(self, temp_f31) == 1U) && (em_mot_end_ck(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xCF, 0, 0);
            if (em_flags836_ck(self, 1) == 0) {
                em_hit_window_set_default_c1(self, 0, 0x16);
                return;
            }
        }
        return;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    default:
        return;
    }
}

void fn_8014F010(void) {
    fn_801493A8_c2(0U, 1U, 1);
}

void fn_8014F020(void) {
    fn_801493A8_c2(0U, 3U, 1);
}

void fn_8014F030(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {
    case 0:
        fn_8014EA84(self);
        return;
    case 1:
        fn_8014EC50(self);
        return;
    case 2:
        fn_8014EDC8(self);
        return;
    case 3:
        fn_8014F010();
        return;
    case 4:
        fn_8014F020();
        return;
    }
}

void fn_8014F530(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {
    case 0:
        em_se_tbl_play_alt_c1(&lbl_805A2AA0, 0, 0);
        return;
    case 5:
        em_se_tbl_play_alt_c1(&lbl_805A2B00, 1, 5);
        return;
    case 15:
        em_se_tbl_play_alt_c1(&lbl_805A2B50, 0, 0xF);
        return;
    case 26:
        em_se_tbl_play_alt_c1(&lbl_805A2B98, 0, 0x1A);
        return;
    case 28:
        em_se_tbl_play_alt_c1(&lbl_805A2BC8, 0, 0x1C);
        return;
    default:
        em_se_tbl_play_alt_c1(&lbl_805A2AA0, 0, 0);
        return;
    }
}

void fn_8014F5D8(void) {
    fn_80149AFC_c2();
}

void fn_8014F5DC(_ENEMY_WORK *self) {
    if ((s32) self->state_sub == 0) {
        fn_8014F5D8();
    }
}

void fn_8014F5F0(_ENEMY_WORK *self) {
    f32 temp_f1;
    f32 temp_f2;
    f32 temp_f31;
    s32 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0xA, 0);
        em_dive_start(self);
        self->timer_0x020 = 0x12C;
        return;
    case 1:
        temp_f1 = self->pos.y;
        temp_f2 = self->vec_0x36C.y - temp_f1;
        if (temp_f2 < lbl_80796FA0) {
            em_dive_step(self);
        } else if (temp_f2 > lbl_80796F54) {
            self->pos.y = (f32) (temp_f1 + lbl_80796F84);
        }
        temp_f31 = self->vec_0x36C.y - self->pos.y;
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if ((temp_r0 <= 0) || ((em_turn_to_target(self, 0x180) == 1U) && (temp_f31 >= lbl_80796FA4) && (temp_f31 <= lbl_80796FA8))) {
            em_state_set(self, 0xD, 1);
        }
        return;
    default:
        return;
    }
}

void fn_8014FA34(_ENEMY_WORK *self) {
    u8 temp_r0;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 3);
        em_mot_set(self, 0xC8, 0x14, 0);
        em_hit_window_set_default_c1(self, 0, 0xB);
        em_move_vec2_clr(self);
        fn_801303EC(self, lbl_80796E1C);
        self->float_0x328 = fn_801356A8(self, lbl_80796F74, lbl_80796E20, lbl_80796F78);
        return;
    case 1:
        em_key_curve_eval(self, &lbl_805A1DD8);
        fn_801303FC_c2(self);
        if (self->field_0x1AC < lbl_80796E24) {
            self->field_0x1AC = (f32) lbl_80796E24;
        }
        self->offset_0x30C.vec_0x310.z = (f32) (self->float_0x328 * em_key_curve_eval(self, &lbl_805A1E20));
        em_fall_height_get(self);
        fn_80135584(self, &self->field_0x1BC);
        if (em_frame_check(self, 1, lbl_80796F58, lbl_80796E1C) == 0) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDC, 0, 0);
            return;
        }
        return;
    case 2:
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            if (em_frame_check(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
            case 1:
                if (em_busy_ck(self) == 1U) {
                    if (fn_80154638(self, 0) == 1U) {
                        em_state_set(self, 0xD, 3);
                        return;
                    }
                    em_state_set(self, 1, 3);
                }
            }
            break;
        }
        break;
    }
}

void fn_8014FE00(_ENEMY_WORK *self) {
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xCB, 4, 0);
        fn_803B9BA0(self, &self->pos, 0x64);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0xCB, 4, 0);
            return;
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0xCB, 4, 0);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            fn_80130CDC(self, 0x3E8);
            em_state_set(self, 2, 6);
        }
        break;
    }
}

void fn_8014F138(_ENEMY_WORK *self) {
    u8 temp_r0;

    temp_r0 = self->state_sub;
    switch ((s32) temp_r0) {
    case 0x17:
        em_se_tbl_play_c1(&lbl_805A2338, 0, 0x17);
        return;
    case 0x18:
        em_se_tbl_play_c1(&lbl_805A2490, 0, 0x18);
        return;
    case 0x19:
        em_se_tbl_play_c1(&lbl_805A2490, 0, 0x19);
        return;
    case 0x1A:
        em_se_tbl_play_c1(&lbl_805A23E0, 0, 0x1A);
        return;
    case 0x1B:
        em_se_tbl_play_c1(&lbl_805A2450, 0, 0x1B);
        return;
    case 0x1C:
        em_se_tbl_play_c1(&lbl_805A2360, 0, 0x1C);
        return;
    case 0x1D:
        em_se_tbl_play_c1(&lbl_805A2388, 0, 0x1D);
        return;
    case 0x1E:
        em_se_tbl_play_c1(&lbl_805A2360, 0, 0x1E);
        return;
    case 0x23:
        em_se_tbl_play_c1(&lbl_805A2518, 1, 0x23);
        return;
    case 0x7A:
        em_se_tbl_play_c1(&lbl_805A2588, 0, 0x7A);
        return;
    case 0x7B:
        em_se_tbl_play_c1(&lbl_805A25B0, 0, 0x7B);
        return;
    case 0x7C:
        em_se_tbl_play_c1(&lbl_805A2620, 0, 0x7C);
        return;
    case 0x8D:
        em_se_tbl_play_c1(&lbl_805A2718, 0, 0x8D);
        return;
    case 0x7E:
        em_se_tbl_play_c1(&lbl_805A2678, 0, 0x7E);
        return;
    case 0x7F:
        em_se_tbl_play_c1(&lbl_805A26E0, 0, 0x7F);
        return;
    case 0x8E:
        em_se_tbl_play_c1(&lbl_805A2718, 0, 0x8E);
        return;
    case 0x78:
        em_se_tbl_play_c1(&lbl_805A2808, 0, 0x78);
        return;
    case 0x84:
        em_se_tbl_play_c1(&lbl_805A27A0, 1, 0x84);
        return;
    case 0x9F:
        em_se_tbl_play_c1(&lbl_805A25F0, 0, 0x9F);
        return;
    case 0xA0:
        em_se_tbl_play_c1(&lbl_805A25F0, 0, 0xA0);
        return;
    case 0xA8:
        em_se_tbl_play_c1(&lbl_805A2830, 0, 0xA8);
        return;
    case 0xA9:
        em_se_tbl_play_c1(&lbl_805A2518, 1, 0xA9);
        return;
    case 0xB6:
        em_se_tbl_play_c1(&lbl_805A2858, 0, 0xB6);
        return;
    case 0xB7:
        em_se_tbl_play_c1(&lbl_805A2880, 0, 0xB7);
        return;
    case 0xB8:
        em_se_tbl_play_c1(&lbl_805A28F0, 1, 0xB8);
        return;
    case 0xB9:
        em_se_tbl_play_c1(&lbl_805A2978, 0, 0xB9);
        return;
    case 0xBA:
        em_se_tbl_play_c1(&lbl_805A29A0, 0, 0xBA);
        return;
    case 0xBB:
        em_se_tbl_play_c1(&lbl_805A2950, 0, 0xBB);
        return;
    case 0xBC:
        em_se_tbl_play_c1(&lbl_805A2950, 0, 0xBC);
        return;
    case 0xBF:
        em_se_tbl_play_c1(&lbl_805A29E0, 0, 0xBF);
        return;
    case 0xC1:
        em_se_tbl_play_c1(&lbl_805A2718, 0, 0xC1);
        return;
    case 0xCA:
        em_se_tbl_play_c1(&lbl_805A2A58, 0, 0xCA);
        return;
    case 0xF0:
        em_se_tbl_play_c1(&lbl_805A2830, 0, 0xF0);
        return;
    default:
        em_action_finish(self);
        return;
    }
}

void fn_8014F078(_ENEMY_WORK *self) {
    f32 spC;
    VEC3 sp8;

    VEC3_ctor(&sp8);
    if ((u32) (em_get_mot_no(self) - 0x70) <= 1U) {
        if (((s32) self->state_0x006 == 0) && (em_magma_check(self) == 1U) && (self->pos.y < self->field_0x214)) {
            self->state_0x006 = (u8) (self->state_0x006 + 1);
            copyVec3(&sp8, &self->pos);
            spC = lbl_80796F00 + self->field_0x214;
            eft009_set_pos_c1(0x88, &sp8, &self->field_0x1BC, self->area_no, lbl_80796E20);
        }
    } else {
        self->state_0x006 = 0U;
    }
}

void fn_8014FF10(_ENEMY_WORK *self) {
    VEC3 sp8;
    u8 temp_r3;

    VEC3_ctor(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        return;
    case 1:
        if ((em_frame_check(self, 0, lbl_80796E4C, lbl_80796E1C) == 1U) || (em_frame_check(self, 0, lbl_80796E50, lbl_80796E1C) == 1U)) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x33, 0x1A, &sp8, lbl_80796E20);
        }
        if (em_frame_check(self, 0, lbl_80796E5C, lbl_80796E1C) == 1U) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
            fn_801049D0(self, 0x1A, 0x2F, 0, &sp8, lbl_80796E64);
        }
        if ((em_frame_check(self, 2, lbl_80796E68, lbl_80796E6C) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x34, 0x1A, &sp8, lbl_80796E20);
        }
        if ((em_frame_check(self, 0, lbl_80796E78, lbl_80796E1C) == 1U) || (em_frame_check(self, 0, lbl_80796E7C, lbl_80796E1C) == 1U) || (em_frame_check(self, 0, lbl_80796E80, lbl_80796E1C) == 1U)) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x33, 0x1A, &sp8, lbl_80796E84);
        }
        if (em_frame_check(self, 0, lbl_80796E50, lbl_80796E1C) == 1U) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E1C, lbl_80796E60);
            fn_801049D0(self, 0x1A, 0x2F, 0, &sp8, lbl_80796E64);
        }
        if ((em_frame_check(self, 2, lbl_80796E70, lbl_80796E74) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x34, 0x1A, &sp8, lbl_80796E20);
        }
        if ((em_frame_check(self, 2, lbl_80796E88, lbl_80796E8C) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x34, 0x1A, &sp8, lbl_80796E98);
        }
        if (em_frame_check(self, 0, lbl_80796E9C, lbl_80796E1C) == 1U) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x33, 0x1A, &sp8, lbl_80796E2C);
        }
        if ((em_frame_check(self, 2, lbl_80796E90, lbl_80796E94) == 1U) && ((s32) (system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp8, lbl_80796E1C, lbl_80796E54, lbl_80796E58);
            eft_em_spawn(self, 0x34, 0x1A, &sp8, lbl_80796E98);
        }
        return;
    }
}

void fn_8014CEF8(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    VEC3 sp1C;
    VEC3 sp10;
    u32 spC;
    s32 sp8;
    s32 temp_r0;
    s32 temp_r0_2;
    s32 temp_r0_4;
    s32 temp_r3_2;
    s32 temp_r3_3;
    u8 temp_r0_3;
    u8 temp_r0_5;
    u8 temp_r3;

    VEC3_ctor(&sp1C);
    em_busy_set_c1(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_fall_height_get_f32(self);
        em_fall_start(self);
        fn_80154C74(self);
        em_mot_set(self, 0x45, 6, 0);
        self->state_0x006 = 0U;
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.y = (f32) lbl_80796F6C;
        self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E60;
        return;
    case 1:
        fn_80133C3C(self);
        fn_80136D4C(self, lbl_80796F00);
        switch ((s32) arg2) {
        case 0:
            spC = 0x2800;
            break;
        case 1:
            spC = 0x2000;
            break;
        case 2:
            spC = 0x1800;
            break;
        default:
            subVec3(&sp10, &self->vec_0x36C, &self->pos);
            copyVec3(&sp1C, &sp10);
            calcVecAngXY_c1(&sp1C, &spC, &sp8);
            if (spC > 0x3000U) {
                spC = 0x3000;
            }
            break;
        }
        self->field_0x1BC = (s32) fn_80133DB0((u16) spC, (u16) self->field_0x1BC, 0x180);
        em_turn_to_target(self, 0x280);
        em_move_offset_step_update_c2(self, &self->field_0x1BC, em_fall_height_get_f32(self) - lbl_80796F4C);
        if (self->offset_0x30C.vec_0x310.z > lbl_80796F28) {
            self->offset_0x30C.vec_0x310.z = (f32) lbl_80796F28;
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0x3E, 6, 0);
            self->timer_0x020 = 0;
            em_hit_window_set(self, 0, 0xE, 3);
            self->field_0x324 = (f32) lbl_80796F04;
            return;
        }
        return;
    case 2:
        fn_80133C3C(self);
        self->timer_0x020 = (s32) (self->timer_0x020 + 1);
        if (em_move_offset_step_update_c2(self, &self->field_0x1BC, em_fall_height_get_f32(self) - lbl_80796F4C) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x3F, 6, 0);
            fn_801303EC(self, lbl_80796F34);
        } else if (((s32) self->timer_0x020 > 0x96) && (em_busy_ck(self) == 1U)) {
            em_action_finish_fall(self);
        }
        if (self->offset_0x30C.vec_0x310.z > lbl_80796E34) {
            self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E34;
            return;
        }
        break;
    case 3:
        fn_801303EC(self, lbl_80796F34);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0xD0, 0, 0);
            self->timer_0x020 = 0;
            return;
        }
        break;
    case 4:
        fn_801303EC(self, lbl_80796F34);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        temp_r0 = self->timer_0x020 + 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 > 6) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    em_mot_set(self, 0xD2, 6, 0);
                    self->field_0x324 = (f32) lbl_80796F70;
                    self->timer_0x020 = 0;
                    return;
                }
                em_mot_set(self, 0x35, 0xA, 0);
                return;
            }
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDD, 0, 0);
            fn_801303EC(self, lbl_80796E1C);
            return;
        }
        break;
    case 5:
        if ((arg1 & 0xFF) != 1) {
            if ((arg1 & 0xFF) != 2) {
                temp_r3_2 = self->timer_0x020;
                if (temp_r3_2 < 0x14) {
                    temp_r0_2 = temp_r3_2 + 1;
                    self->timer_0x020 = temp_r0_2;
                    fn_801303EC(self, (lbl_80796F3C * (f32) (0x14 - temp_r0_2)) / lbl_80796E0C);
                }
                if (em_frame_check(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                    em_hit_window_clear(self, 0);
                }
                em_move_offset_step(self, &self->field_0x1BC);
                if (self->offset_0x30C.vec_0x310.z < lbl_80796E1C) {
                    self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E1C;
                }
                if (em_mot_end_ck(self) == 1U) {
                    em_action_finish_walk(self);
                    return;
                }
            } else {
                temp_r0_3 = self->state_0x006;
                switch ((s32) temp_r0_3) {
                case 0:
                    temp_r3_3 = self->timer_0x020;
                    if (temp_r3_3 < 0xA) {
                        temp_r0_4 = temp_r3_3 + 1;
                        self->timer_0x020 = temp_r0_4;
                        fn_801303EC(self, (lbl_80796F34 * (f32) (5 - temp_r0_4)) / lbl_80796F00);
                    }
                    if (em_mot_end_ck(self) == 1U) {
                        self->state_0x006 = (u8) (self->state_0x006 + 1);
                        em_hit_window_clear(self, 0);
                        em_move_mode_set(self, 0);
                        em_mot_set(self, 0x3C, 0, 0);
                        return;
                    }
                    break;
                case 1:
                    if (em_mot_end_ck(self) == 1U) {
                        em_action_finish(self);
                    }
                    break;
                }
            }
        } else {
            temp_r0_5 = self->state_0x006;
            switch ((s32) temp_r0_5) {
            case 0:
                if (em_mot_end_ck(self) == 1U) {
                    self->state_0x006 = (u8) (self->state_0x006 + 1);
                    em_mot_set(self, 0xDA, 0, 0);
                    return;
                }
                break;
            case 1:
                if (em_frame_check(self, 0, lbl_80796EFC, lbl_80796E1C) == 1U) {
                    em_hit_window_clear(self, 0);
                }
                if (em_mot_end_ck(self) == 1U) {
                    em_action_finish(self);
                    return;
                }
                break;
            }
        }
        break;
    }
}

u32 fn_8014E670(_ENEMY_WORK *self, u8 arg1) {
    VEC3 sp2C;
    VEC3 sp20;
    VEC3 sp14;
    VEC3 sp8;
    s32 temp_r31;
    u8 *temp_r31_2;

    VEC3_ctor(&sp2C);
    VEC3_ctor(&sp20);
    temp_r31 = (s32)get_move_work_adrs(2);
    if ((arg1 & 0xFF) < (s32) get_move_work_max(2)) {
        temp_r31_2 = (u8 *) (temp_r31 + (arg1 * 0xB20));
        if ((s32) *temp_r31_2 != 0) {
            setVector3(&sp2C, lbl_80796E1C, lbl_80796E1C, lbl_80796E20);
            rotVecY(&sp2C, self->field_0x1C0);
            vec3_scale(&sp8, &sp2C, lbl_80796F64 * get_em_chg_scale(self));
            addVec3(&sp14, &self->pos, &sp8);
            copyVec3(&sp20, &sp14);
            if (fn_80050EAC(temp_r31_2 + 0x3C, &sp20) <= lbl_80796F94) {
                return 1U;
            }
        }
    }
    return 0U;
}

void fn_8014FC24(_ENEMY_WORK *self) {
    VEC3 sp3C;
    VEC3 sp30;
    VEC3 sp24;
    VEC3 sp18;
    VEC3 spC;
    u16 sp8;
    f32 temp_f31;
    u8 temp_r3;

    VEC3_ctor(&sp3C);
    VEC3_ctor(&sp30);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        fn_80154B04(self, &sp3C, &sp8, lbl_80796F64);
        subVec3(&sp24, &sp3C, &self->pos);
        copyVec3(&sp30, &sp24);
        em_target_pos_set(self, &sp3C);
        em_turn_seq_start(self, &lbl_8056F960, 2, 1, sp8);
        em_move_vec_clr(self);
        temp_f31 = get_em_base_scale(self);
        vec3_scale(&spC, &sp30, lbl_80796F98);
        vec3_scale(&sp18, &spC, temp_f31);
        copyVec3(&self->offset_0x30C.vec_0x310, &sp18);
        self->offset_0x30C.vec_0x310.y = (f32) lbl_80796E1C;
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_80796EA0, lbl_80796E60) == 1U) {
            em_move_offset_apply(self);
            em_turn_seq_step(self, &lbl_8056F960);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0xCA, 4, 0);
            return;
        }
        return;
    case 2:
        if (em_busy_ck(self) == 1U) {
            if (fn_80154638(self, 1) == 1U) {
                em_state_set(self, 0xD, 4);
                return;
            }
            em_state_set(self, 2, 6);
            return;
        }
        return;
    default:
        return;
    }
}

void fn_8014E290(_ENEMY_WORK *self) {
    ShellParams sp8;
    u8 temp_r3;

    em_spawn_rec_init_c1(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        fn_80154C74(self);
        em_mot_set(self, 0xDB, 4, 0);
        return;
    case 1:
        if (em_frame_check(self, 0, lbl_80796F8C, lbl_80796E1C) == 1U) {
            eft007_part_set(self, 0x16);
        }
        if (em_frame_check(self, 0, lbl_80796F90, lbl_80796E1C) == 1U) {
            fn_801545B8_c1(&sp8, 0, 0x11C7U, 0);
            eft007_part_spawn(self, 0x15, sp8.field_0x12, sp8.field_0x14);
            shell_set_func_ptr->method_0x3C(self, &sp8, 3, shell_set_func_ptr);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

void fn_8014BB40(_ENEMY_WORK *self, u8 arg1) {
    ShellParams sp8;
    u16 var_r31;
    u8 temp_r3;

    em_spawn_rec_init_c1(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        self->timer_0x020 = 0;
        if (arg1 == 3) {
            em_mot_set(self, 0x33, 6, 0);
            return;
        }
        fn_80154C74(self);
        em_mot_set(self, 0x33, 6, 0);
        return;
    case 1:
        if (arg1 == 3) {
            fn_8012F8EC(self);
            getKeyData_c1(&lbl_805A2078);
            em_mot_speed_set_c1(self);
        }
        if (em_frame_check(self, 0, lbl_80796F40, lbl_80796E1C) == 1U) {
            eft007_part_set(self, 3);
        }
        if (em_frame_check(self, 1, lbl_80796F44, lbl_80796E1C) == 0) {
            if ((s32) (self->timer_0x020 & 7) == 0) {
                eft007_part_set(self, 1);
            }
            self->timer_0x020 = (s32) (self->timer_0x020 + 1);
        }
        if (arg1 != 3) {
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    var_r31 = 0;
                } else {
                    var_r31 = 0x71C;
                }
            } else {
                var_r31 = 0xF8E4;
            }
            if (em_frame_check(self, 0, lbl_80796F48, lbl_80796E1C) == 1U) {
                fn_801545B8_c2(&sp8, 0, var_r31, 0);
                shell_set_func_ptr->method_0x3C(self, &sp8, 0, shell_set_func_ptr);
                eft007_part_spawn(self, 0, sp8.field_0x12, sp8.field_0x14);
                eft007_part_spawn(self, 2, sp8.field_0x12, sp8.field_0x14);
            }
        } else if (em_frame_check(self, 0, lbl_80796F48, lbl_80796E1C) == 1U) {
            fn_801545B8_c1(&sp8, 0, 0U, 0);
            shell_set_func_ptr->method_0x3C(self, &sp8, 1, shell_set_func_ptr);
            eft007_part_spawn(self, 4, 0U, 0U);
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

void fn_8014C464(_ENEMY_WORK *self, u8 arg1) {
    ShellParams sp8;
    s32 var_r31;
    u8 temp_r3;

    em_spawn_rec_init_c1(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 0);
        fn_80154C74(self);
        em_mot_set(self, 0xD0, 4, 0);
        return;
    case 1:
        if (arg1 == 1) {
            fn_8012F8EC(self);
            getKeyData_c1(&lbl_805A20A0);
            em_mot_speed_set_c1(self);
        }
        if (em_frame_check(self, 0, lbl_80796F40, lbl_80796E1C) == 1U) {
            eft007_part_set(self, 3);
        }
        if (em_frame_check(self, 1, lbl_80796F44, lbl_80796E1C) == 0) {
            if ((s32) (self->timer_0x020 & 7) == 0) {
                eft007_part_set(self, 1);
            }
            self->timer_0x020 = (s32) (self->timer_0x020 + 1);
        }
        var_r31 = 0;
        if (em_frame_check(self, 0, lbl_80796F48, lbl_80796E1C) == 1U) {
            fn_801545B8_c1(&sp8, 0, 0x38EU, 0);
            var_r31 = 1;
        }
        if (em_frame_check(self, 0, lbl_80796F28, lbl_80796E1C) == 1U) {
            fn_801545B8_c1(&sp8, 0, 0x38EU, 0xF334);
            var_r31 = 1;
        }
        if (em_frame_check(self, 0, lbl_80796F5C, lbl_80796E1C) == 1U) {
            fn_801545B8_c1(&sp8, 0, 0x321U, 0x105B);
            var_r31 = 2;
        }
        if (var_r31 != 0) {
            if ((arg1 & 0xFF) == 0) {
                shell_set_func_ptr->method_0x3C(self, &sp8, 0, shell_set_func_ptr);
                eft007_part_spawn(self, 0, sp8.field_0x12, sp8.field_0x14);
                if ((u32) var_r31 == 2U) {
                    eft007_part_spawn(self, 2, sp8.field_0x12, sp8.field_0x14);
                }
            } else {
                shell_set_func_ptr->method_0x3C(self, &sp8, 1, shell_set_func_ptr);
                eft007_part_spawn(self, 4, sp8.field_0x12, sp8.field_0x14);
            }
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish(self);
        }
        return;
    }
}

void fn_8014D8F0(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    ShellParams sp8;
    u16 var_r5;
    u8 temp_r3;

    em_spawn_rec_init_c1(&sp8);
    fn_80154CA4(self);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_move_mode_set(self, 3);
        fn_80154C74(self);
        em_mot_set(self, 0xCD, 6, 0);
        em_move_vec_clr(self);
        return;
    case 1:
        if (em_frame_check(self, 1, lbl_80796EA8, lbl_80796E1C) == 0) {
            if ((u8) self->field_0x7C8 <= 0x28U) {
                em_turn_to_target(self, 0x80);
            } else {
                em_turn_to_target(self, 0x100);
            }
        }
        if ((s32) arg2 == 0) {
            self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &lbl_805A1F98);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, lbl_80796F80, lbl_80796E1C) == 1U) {
            if ((arg1 & 0xFF) != 1) {
                if ((arg1 & 0xFF) != 2) {
                    var_r5 = 0x127D;
                } else {
                    var_r5 = 0x16C1;
                }
            } else {
                var_r5 = 0xE39;
            }
            fn_801545B8_c2(&sp8, 1, var_r5, 0);
            if (arg1 == 3) {
                shell_set_func_ptr->method_0x3C(self, &sp8, 1, shell_set_func_ptr);
                eft007_part_spawn(self, 4, sp8.field_0x12, sp8.field_0x14);
            } else {
                shell_set_func_ptr->method_0x3C(self, &sp8, 0, shell_set_func_ptr);
                eft007_part_spawn(self, 0x17, sp8.field_0x12, sp8.field_0x14);
                eft007_part_spawn(self, 2, sp8.field_0x12, sp8.field_0x14);
            }
        }
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_walk(self);
        }
        return;
    }
}

void fn_8014DAF4(_ENEMY_WORK *self, u8 arg1, u8 arg2) {
    ShellParams sp8;
    u16 var_r5;
    u8 temp_r0;
    u8 temp_r3;

    em_spawn_rec_init_c1(&sp8);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        fn_80154C74(self);
        em_mot_set(self, 0xD3, 6, 0);
        if ((u32) (arg1 - 2) > 1U) {
            if ((arg1 & 0xFF) != 4) {
                return;
            }
            em_move_vec_clr(self);
            self->offset_0x30C.vec_0x310.z = (f32) lbl_80796F0C;
            self->offset_0x30C.vec_0x310.y = (f32) lbl_80796F6C;
            return;
        }
        em_move_vec_clr(self);
        self->offset_0x30C.vec_0x310.z = (f32) lbl_80796F84;
        return;
    case 1:
        em_turn_to_target(self, 0x40);
        if ((u32) (arg1 - 2) <= 2U) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            em_mot_set(self, 0xD4, 6, 0);
            return;
        }
        return;
    case 2:
        if ((u32) (arg1 - 2) > 2U) {
            if ((arg1 & 0xFF) == 1) {
                em_turn_to_target(self, 0x40);
            }
        } else {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 0, lbl_80796EA8, lbl_80796E1C) == 1U) {
            if ((arg1 & 0xFF) != 1) {
                var_r5 = 0x238E;
            } else {
                temp_r0 = self->state_0x006;
                switch ((s32) temp_r0) {
                case 0:
                    var_r5 = 0x238E;
                    break;
                default:
                    var_r5 = 0x1E94;
                    break;
                case 2:
                    var_r5 = 0x199A;
                    break;
                }
            }
            fn_801545B8_c2(&sp8, 1, var_r5, 0);
            if ((s32) arg2 == 0) {
                shell_set_func_ptr->method_0x3C(self, &sp8, 0, shell_set_func_ptr);
                eft007_part_spawn(self, 0x17, sp8.field_0x12, sp8.field_0x14);
                eft007_part_spawn(self, 2, sp8.field_0x12, sp8.field_0x14);
            } else {
                shell_set_func_ptr->method_0x3C(self, &sp8, 1, shell_set_func_ptr);
                eft007_part_spawn(self, 4, sp8.field_0x12, sp8.field_0x14);
            }
            self->state_0x006 = (u8) (self->state_0x006 + 1);
        }
        if ((((arg1 != 1) && (arg1 != 3) && (arg1 != 4)) || ((u8) self->state_0x006 >= 3U)) && (em_mot_end_ck(self) == 1U)) {
            self->state = (u8) (self->state + 1);
            em_mot_set(self, 0xD5, 6, 0);
            return;
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        break;
    }
}

void fn_8014DDEC(_ENEMY_WORK *self) {
    VEC3 sp14;
    VEC3 sp8;
    f32 temp_f1;
    f32 temp_f1_2;
    f32 temp_f31;
    f32 temp_f31_2;
    f32 temp_f31_3;
    s32 temp_r30;
    u8 temp_r4;

    temp_r4 = self->state;
    switch ((s32) temp_r4) {
    case 0:
        self->state = (u8) (temp_r4 + 1);
        self->state_0x006 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        fn_80154C74(self);
        em_mot_set(self, 0xDB, 6, 0);
        em_hit_window_set_default_c1(self, 0, 0x1F);
        copyVec3(&self->aim, &self->pos);
        self->field_0x32c = (u16) self->field_0x1C0;
        em_move_vec2_clr(self);
        self->timer_0x020 = (s32) (s16) (lbl_80796EA8 / get_em_base_scale(self));
        return;
    case 1:
        em_busy_set_c1(self);
        em_target_pos_set(self, NULL);
        if (em_frame_check(self, 3e-45f, lbl_80796E34, lbl_80796E1C) == 1U) {
            em_turn_to_target(self, 0x300);
        }
        if (em_frame_check(self, 2, lbl_80796ED8, lbl_80796F88) == 1U) {
            if ((s32) self->timer_0x020 > 0) {
                self->offset_0x30C.vec_0x310.x = (f32) lbl_80796E1C;
                temp_f31 = get_em_chg_scale(self);
                temp_f1 = ((self->vec_0x36C.y - self->pos.y) + ((em_fall_height_get(self) - lbl_80796F88) * temp_f31)) / (f32) self->timer_0x020;
                self->offset_0x30C.vec_0x310.y = temp_f1;
                if (temp_f1 > lbl_80796E1C) {
                    self->offset_0x30C.vec_0x310.y = (f32) lbl_80796E1C;
                }
                if ((u16) ((calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0) + 0x4000) < -0x8000U) {
                    temp_f31_2 = lbl_80796F88 * get_em_chg_scale(self);
                    temp_f1_2 = (calcVecDistXZ(&self->vec_0x36C, &self->pos) - temp_f31_2) / (f32) self->timer_0x020;
                    self->offset_0x30C.vec_0x310.z = temp_f1_2;
                    if (temp_f1_2 == lbl_80796EFC) {
                        self->offset_0x30C.vec_0x310.z = (f32) lbl_80796EFC;
                    }
                } else {
                    self->offset_0x30C.vec_0x310.z = (f32) lbl_80796EFC;
                }
                em_fall_height_get(self);
                fn_80135584(self, &self->field_0x1BC);
            }
            self->timer_0x020 = (s32) (self->timer_0x020 - 1);
        }
        if (em_frame_check(self, 1, lbl_80796F7C, lbl_80796E1C) == 1U) {
            self->state = (u8) (self->state + 1);
            temp_f31_3 = get_em_base_scale(self);
            self->timer_0x020 = (s32) (s16) ((lbl_80796F4C - fn_8012F8EC(self)) / temp_f31_3);
            return;
        }
        return;
    case 2:
        self->field_0x1C0 = (s32) fn_80133DB0(self->field_0x32c, (u16) self->field_0x1C0, 0x300);
        em_move_vec_clr(self);
        temp_r30 = self->timer_0x020;
        if (temp_r30 > 0) {
            subVec3(&sp8, &self->aim, &self->pos);
            fn_800AD9C0_c1(&sp14, &sp8, (f32) temp_r30);
            copyVec3(&self->offset_0x30C.vec_0x310, &sp14);
            em_move_offset_apply(self);
        }
        self->timer_0x020 = (s32) (self->timer_0x020 - 1);
        if (em_mot_end_ck(self) == 1U) {
            em_action_finish_fall(self);
        }
        return;
    default:
        return;
    }
}

void fn_8014E774(_ENEMY_WORK *self) {
    VEC3 sp48;
    VEC3 sp3C;
    VEC3 sp30;
    VEC3 sp24;
    VEC3 sp18;
    VEC3 spC;
    u16 sp8;
    f32 temp_f2;
    f32 temp_f31;
    s32 temp_r0;
    u8 temp_r3;

    VEC3_ctor(&sp48);
    VEC3_ctor(&sp3C);
    VEC3_ctor(&sp30);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        fn_80154B04(self, &sp48, &sp8, lbl_80796F64);
        subVec3(&sp24, &sp48, &self->pos);
        copyVec3(&sp3C, &sp24);
        em_target_pos_set(self, &sp48);
        em_turn_seq_start(self, &lbl_8056F960, 2, 1, sp8);
        em_move_vec_clr(self);
        temp_f31 = get_em_base_scale(self);
        vec3_scale(&spC, &sp3C, lbl_80796F98);
        vec3_scale(&sp18, &spC, temp_f31);
        copyVec3(&self->offset_0x30C.vec_0x310, &sp18);
        self->offset_0x30C.vec_0x310.y = (f32) lbl_80796E1C;
        return;
    case 1:
        if (em_frame_check(self, 2, lbl_80796EA0, lbl_80796E60) == 1U) {
            em_move_offset_apply(self);
            em_turn_seq_step(self, &lbl_8056F960);
        }
        if (em_frame_check(self, 1, lbl_80796E60, lbl_80796E1C) == 1U) {
            fn_80131EC0_c2(self);
        }
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            self->timer_0x020 = 0x5A;
            em_mot_set(self, 0xCA, 4, 0);
            return;
        }
        return;
    case 2:
        fn_80131EC0_c2(self);
        temp_r0 = self->timer_0x020 - 1;
        self->timer_0x020 = temp_r0;
        if (temp_r0 < 0) {
            self->timer_0x020 = 0;
        }
        if (em_busy_ck(self) == 1U) {
            if (fn_8014E670(self, self->field_0x382) == 1U) {
                if (fn_8012D23C_c1(self, 1, self->field_0x382) == 1U) {
                    self->field_0x1E7 = 3;
                    if (fn_8012EC3C_c1(self) == 1U) {
                        fn_80130CDC(self, 0x12C);
                    } else {
                        fn_80130CDC(self, 0x96);
                    }
                    temp_f2 = (f32) self->field_0x7A4;
                    fn_8011E6EC_c1(self, (s32) (lbl_80796F9C * temp_f2), 1, lbl_80796E20, temp_f2);
                    em_state_set(self, 9, 1);
                    return;
                }
                if (((s32) self->timer_0x020 <= 0) || (fn_8012D23C_c1(self, 0, self->field_0x382) == 0)) {
                    em_state_set(self, 2, 6);
                    return;
                }
                return;
            }
            em_state_set(self, 2, 6);
            return;
        }
        return;
    default:
        return;
    }
}

void fn_8014F71C(_ENEMY_WORK *self) {
    VEC3 sp1C;
    VEC3 sp10;
    u32 spC;
    s32 sp8;
    u32 var_r0;
    u32 var_r30;
    u8 temp_r0;
    u8 temp_r3;

    VEC3_ctor(&sp1C);
    temp_r3 = self->state;
    switch ((s32) temp_r3) {
    case 0:
        self->state = (u8) (temp_r3 + 1);
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x3D, 6, 0);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E38;
        self->field_0x324 = (f32) lbl_80796ECC;
        return;
    case 1:
        fn_80133C3C(self);
        em_fall_height_get(self);
        em_move_offset_step_update_c1(self, &self->field_0x1BC);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY_c1(&sp1C, &spC, &sp8);
        var_r0 = spC + 0x400;
        spC = var_r0;
        if (var_r0 > 0x3000U) {
            var_r0 = 0x3000;
            spC = 0x3000;
        }
        self->field_0x1BC = (s32) fn_80133DB0((u16) var_r0, (u16) self->field_0x1BC, 0x100);
        self->field_0x1C0 = (s32) fn_80133DB0((u16) sp8, (u16) self->field_0x1C0, 0x40);
        if (em_mot_end_ck(self) == 1U) {
            self->state = (u8) (self->state + 1);
            em_move_vec_clr(self);
            self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E3C;
            self->field_0x324 = (f32) lbl_80796ECC;
            self->offset_0x30C.vec_0x310.y = (f32) lbl_80796F6C;
            em_mot_set(self, 0x3E, 6, 0);
            em_approach_start(self, lbl_80796FAC, 0x12);
            return;
        }
        return;
    case 2:
        fn_80133C3C(self);
        if (em_approach_step(self, 0, 0x80U) == 1) {
            self->state = (u8) (self->state + 1);
            self->state_0x006 = 0U;
            em_mot_set(self, 0x2D, 6, 0);
            self->offset_0x30C.vec_0x310.y = (f32) lbl_80796FB0;
            self->field_0x324 = (f32) lbl_80796FB4;
            em_fall_height_get(self);
            em_move_offset_step_update_c1(self, &self->field_0x1BC);
            return;
        }
        em_fall_height_get(self);
        em_move_offset_step_update_c1(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > lbl_80796F7C) {
            self->offset_0x30C.vec_0x310.z = (f32) lbl_80796F7C;
            return;
        }
        break;
    case 3:
        temp_r0 = self->state_0x006;
        switch ((s32) temp_r0) {
        case 0:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, lbl_80796EFC, lbl_80796E1C) == 1U) {
                self->state_0x006 = (u8) (self->state_0x006 + 1);
                self->field_0x324 = (f32) lbl_80796F14;
            }
            break;
        case 1:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_c1(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z < lbl_80796E1C) {
                self->offset_0x30C.vec_0x310.z = (f32) lbl_80796E1C;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1U) {
            if (var_r30 == 1U) {
                em_move_mode_set(self, 3);
                em_state_set(self, 0xD, 2);
                return;
            }
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801502C8 (0x460) - the 11-state action step: arm the motion, wait out each effect window, and re-arm
 * the next motion at each transition. */
void fn_801502C8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get_view1(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x1D6) == 1) {
            self->state = self->state + 1;
            em_demo_pos_set(self, lbl_80796FB8, lbl_80796FBC, lbl_80796FC0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
            em_demo_enable(self);
        }
        return;
    case 2:
        if (em_demo_time_ck(0x2DA) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x3D, 0, 0);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2BD8, 0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
        }
        return;
    case 3:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2BD8, 0);
        if (em_after_frame_check(self, 0, lbl_80796F18, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_demo_reset(self, 0);
        }
        return;
    case 4:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2BD8, 0);
        if (em_demo_time_ck(0x3A2) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xC8, 0, 0x1E);
            em_demo_enable(self);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2D68, 0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796FC4, lbl_80796E1C);
        }
        return;
    case 5:
        if (em_frame_check(self, 0, lbl_80796F44, lbl_80796E1C) == 1) {
            em_hit_window_set_default_view1(self, 0, 0x17);
        }
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A2D68, 0);
        if (em_demo_time_ck(0x41A) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x13, 0, 0);
            em_demo_pos_set(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F18, lbl_80796E1C);
        }
        return;
    case 6:
        if (em_demo_time_ck(0x578) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x12, 0, 0);
            em_demo_pos_set(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796F18, lbl_80796E1C);
        }
        return;
    case 7:
        if (em_demo_time_ck(0x6A4) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x29, 0, 0);
            em_demo_pos_set(self, lbl_80796FC8, lbl_80796E1C, lbl_80796FCC);
        }
        return;
    case 8:
        if (em_frame_check(self, 0, lbl_80796FD0, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x43, 6, 0);
            em_demo_key_apply(self, (s16)(em_demo_frame_get() + 1), lbl_805A2F28, 0, 2, 0);
        }
        return;
    case 9:
        em_demo_key_apply(self, (s16)(em_demo_frame_get() + 1), lbl_805A2F28, 0, 2, 0);
        if (em_frame_check(self, 0, lbl_80796FD4, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1A, 6, 0);
        }
        return;
    case 10:
        em_demo_key_apply(self, (s16)(em_demo_frame_get() + 1), lbl_805A2F28, 0, 2, 0);
        return;
    }
}

/* 0x80150728 (0xA4) - the two-state step: arm the 0x1A motion, then close the action. */
void fn_80150728(struct _ENEMY_WORK* self) {
    em_busy_set_c3(self);
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get_view1(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_pos_set(self, lbl_80796FB8, lbl_80796FBC, lbl_80796FC0);
        em_demo_rot_set(self, lbl_80796E1C, lbl_80796F88, lbl_80796E1C);
        return;
    case 1:
        em_action_finish_fall(self);
        return;
    }
}

/* 0x801507CC (0x800) - the 17-state step of the long action: each state waits on a frame counter or
 * a motion end and arms the next effect/motion pair. */
void fn_801507CC(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get_view1(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_rot_set(self, lbl_80796E1C, lbl_80796FD8, lbl_80796E1C);
        em_demo_reset(self, 0);
        return;
    case 1:
        if (em_demo_time_ck(0x15A) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1A, 0, 0);
            em_demo_pos_set(self, lbl_80796FDC, lbl_80796FE0, lbl_80796FE4);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
            em_demo_enable(self);
        }
        return;
    case 2:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1A, 0, 0);
        }
        return;
    case 3:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
        if (em_after_frame_check(self, 0, lbl_80796F48, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set_blend_view1(self, 0x3D, 0x1E, 0, 1);
        }
        return;
    case 4:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A30DC, 0, 6, 0);
        if (em_demo_time_ck(0x280) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1F, 0, 0x16);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        }
        return;
    case 5:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x20, 0, 0);
        }
        return;
    case 6:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1F, 0, 0);
        }
        return;
    case 7:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_after_frame_check(self, 0, lbl_80796FE8, lbl_80796E1C) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x3D, 0xA, 0);
        }
        return;
    case 8:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3280, lbl_805A3860);
        if (em_demo_time_ck(0x3EE) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xDB, 0, 0x28);
            em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3AE0, 0);
            em_demo_rot_set(self, lbl_80796E1C, lbl_80796E1C, lbl_80796E1C);
        }
        return;
    case 9:
        em_demo_key3_apply(self, em_demo_frame_get(), lbl_805A3AE0, 0);
        if (em_demo_time_ck(0x434) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1B, 0, 4);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        }
        return;
    case 10:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (em_demo_time_ck(0x468) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xCE, 0, 2);
        }
        return;
    case 11:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0xCF, 0, 2);
        }
        return;
    case 12:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3C10, lbl_805A3DD0, 7, 2);
        if (em_demo_time_ck(0x506) == 1) {
            self->state = self->state + 1;
            em_mot_set_blend_view1(self, 0xCF, 0x14, 0x22, 1);
            em_demo_pos_set(self, lbl_80796FEC, lbl_80796FF0, lbl_80796FF4);
            em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        }
        return;
    case 13:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1B, 0, 0);
        }
        return;
    case 14:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1B, 0, 0);
        }
        return;
    case 15:
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        if (em_mot_end_ck(self) == 1) {
            self->state = self->state + 1;
            em_mot_set(self, 0x1C, 0, 0);
            self->timer_0x020 = 0;
        }
        return;
    case 16:
        if (em_frame_check(self, 0, lbl_80796FF8, lbl_80796E1C) == 1) {
            draw_shape_arm_view3(self, 0x1B, 0xA);
        }
        setVector3(&pos, lbl_80796E1C, lbl_80796E38, lbl_80796E3C);
        if (em_frame_check(self, 0, lbl_80796FF8, lbl_80796E1C) == 1) {
            eft_em_spawn_view3(self, 0, 0x1A, &pos, lbl_80796E20);
        }
        if (em_frame_check(self, 3, lbl_80796FF8, lbl_80796F2C) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn_view3(self, 1, 0x1A, &pos, lbl_80796E20);
            }
            self->timer_0x020 = self->timer_0x020 + 1;
        }
        em_demo_key_apply(self, em_demo_frame_get(), lbl_805A3E90, lbl_805A3FC0, 5, 2);
        return;
    }
}

/* 0x80150FCC (0xA8) - the closing two-state step. */
void fn_80150FCC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state = self->state + 1;
        em_fall_height_get_view1(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 0, 0);
        em_demo_pos_set(self, lbl_80796FFC, lbl_80797000, lbl_80797004);
        em_demo_rot_set(self, lbl_80796E1C, lbl_80796E1C, lbl_80796E1C);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        return;
    }
}
}

#pragma peephole off

extern "C" {
/* 0x80151074 (0x50) - the motion dispatcher: `state_sub` (+0x1E6) selects the step. */
void fn_80151074(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        fn_8014F5F0(self);
        return;
    case 1:
        fn_8014F71C(self);
        return;
    case 2:
        fn_8014FA34(self);
        return;
    case 3:
        fn_8014FC24(self);
        return;
    case 4:
        fn_8014FE00(self);
        return;
    case 5:
        fn_8014FF10(self);
        return;
    case 6:
        fn_801502C8(self);
        return;
    case 7:
        fn_80150728(self);
        return;
    case 8:
        fn_801507CC(self);
        return;
    case 9:
        fn_80150FCC(self);
        return;
    }
}

/* 0x801510C4 (0x58) - the action dispatcher: `action` (+0x1E5) selects the band below. */
void fn_801510C4(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        fn_801484D4(self);
        return;
    case 1:
        fn_80149004(self);
        return;
    case 2:
        fn_80149814(self);
        return;
    case 3:
        fn_8014AA4C(self);
        return;
    case 4:
        fn_8014B5C0(self);
        return;
    case 7:
        fn_8014E3AC(self);
        return;
    case 8:
        fn_8014EA70(self);
        return;
    case 9:
        fn_8014F030(self);
        return;
    case 10:
        fn_8014F138(self);
        return;
    case 11:
        fn_8014F530(self);
        return;
    case 12:
        fn_8014F5DC(self);
        return;
    case 13:
        fn_80151074(self);
        return;
    }
}

/* 0x8015111C (0x158) - the effect arming helper: only when the enemy faces a valid angle does it
 * spawn the per-motion effect. */
void fn_8015111C(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    u16 angle;
    u16 motion;
    u32 kind;

    VEC3_ctor(&pos);
    if (em_alt_mode_ck_c1(self) == 1) {
        angle = calcVecAngX(&self->vec_0x76C);
        if ((u16)(angle + 0x8000) > 0x671B) {
            motion = em_get_mot_no(self);
            if ((u32)(motion - 0x17) > 2U && motion != 0x33 && motion != 0x40 && motion != 0x72 &&
                motion != 0x7D && motion != 0x82) {
                if (self->team == 1 && em_get_mot_no(self) == 0xD0) {
                    kind = 0xFF;
                } else if ((u16)(angle + 0x8000) > 0x6E38) {
                    kind = 6;
                } else {
                    kind = 0;
                }
            } else {
                kind = 0xFF;
            }
            if (kind != 0xFF && (system_w.field_0x0c % 10) == 0) {
                setVector3(&pos, lbl_80796E1C, lbl_80796E54, lbl_80796E60);
                eft_spawn_type10_view1(self, kind, 0x1A, &pos, lbl_80796E20);
            }
        }
    }
}

/* 0x80151274 (0x74) - arms the one-shot effect when the action timer (+0x354) is idle. */
void fn_80151274(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if (self->field_0x354 == 0) {
        setVector3(&pos, lbl_80796E1C, lbl_80796E1C, lbl_80796F88);
        fn_801369A0(self, arg1, 1, &pos, lbl_80796E20);
    }
}

/* 0x801512E8 (0x114) - the effect-spawn dispatcher: `arg1` selects the joint position, the work
 * position or a joint matrix. */
void fn_801512E8(struct _ENEMY_WORK* self, u8 arg1, u8 arg2, s32 arg3, s32 arg4, f32 farg0) {
    char buf_0x18[0x18];
    nw4r::math::VEC3 pos;

    /* `buf_0x18`'s size sets the frame. */
    MTX34_ctor((MTX34*)buf_0x18);
    VEC3_ctor(&pos);
    switch (arg1) {
    case 0:
        eft009_spawn_at_joint_view1(self, arg3, arg2, arg4, farg0);
        return;
    case 1:
        get_joint_wpos_em(self, arg3, &pos);
        pos.y = self->field_0x20C;
        eft_spawn_pos_in_area(&pos, self->area_no, arg2, self->field_0x1C0 + arg4,
                    farg0 * get_em_chg_scale(self));
        return;
    case 2:
        pos.x = self->pos.x;
        pos.y = self->field_0x20C;
        pos.z = self->pos.z;
        eft_spawn_type11_view1(self, &pos, arg2, farg0);
        return;
    }
}

/* 0x801513FC (0xC0) - the shell-attach step: every fourth frame arm the 2/1 effect and, when the
 * gate is set, hand the work position to the shell callback table. */
void fn_801513FC(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    if ((system_w.field_0x0c & 3) == 0) {
        fn_801512E8(self, 2, 1, 0, 0, lbl_80796E20);
        if ((system_w.field_0x0c & 4) != 0) {
            pos.x = self->pos.x;
            pos.y = self->field_0x20C;
            pos.z = self->pos.z;
            shell_set_func_ptr->method_0x28(self, &pos, 0, 0xFFFF, shell_set_func_ptr, lbl_80796E20);
        }
    }
}

/* 0x80154184 (0x14C) - the part-colour refresh: the two body-part damage levels pick the RGBA pair
 * handed to the MHchar. */
void fn_80154184(struct _ENEMY_WORK* self) {
    u32 rgba[2];
    f32 ratio;

    if (em_parts_damage_level_get(self, 1) < 1) {
        rgba[0] = 0x46;
        rgba[1] = 0xFF;
    } else {
        rgba[0] = 0xA0;
        rgba[1] = 0xFF;
    }
    ratio = self->field_0x1D4;
    mhchar_mat_tev_set_view1(self->char_0x024, 1, 6, (u8)(s32)((f32)rgba[0] * ratio), 0, 3,
                (u8)(s32)((f32)rgba[1] * ratio), ratio);

    if (em_parts_damage_level_get(self, 2) < 1) {
        rgba[0] = 0x46;
        rgba[1] = 0xFF;
    } else {
        rgba[0] = 0x78;
        rgba[1] = 0xDC;
    }
    ratio = self->field_0x1D4;
    mhchar_mat_tev_set_view1(self->char_0x024, 2, 6, (u8)(s32)((f32)rgba[0] * ratio), 0, 3,
                (u8)(s32)((f32)rgba[1] * ratio), ratio);
}

/* 0x801542D0 (0x220) - the attack-eligibility test: the enemy's kind/area pair picks the damage
 * interval and the action-record request. */
s32 fn_801542D0(struct _ENEMY_WORK* self, u16 arg1) {
    u8 kind;
    u8 temp;
    u8 temp2;
    u32 range;
    s32 armed;
    s32 arg3;

    kind = stage_map_kind_get(self->field_0x1E0);
    if ((u32)(kind - 1) > 2U && kind != 5) {
        return 0;
    }
    if (kind == 1 && self->field_0x357 == 1) {
        self->field_0x1FC = 1;
        self->field_0x1FE = 0xA;
        self->field_0x1FF = 4;
        return 1;
    }
    if (fn_80129D3C(self) == 1) {
        return 1;
    }
    armed = 0;
    if (kind == 1) {
        range = 0xA;
    } else if (kind == 2) {
        range = 0xA;
    } else if (kind == 3) {
        range = 0xA;
    } else if (kind == 5) {
        range = 5;
    } else {
        range = 0xFF;
    }
    if (range != 0xFF) {
        temp = fn_80129DB8(self);
        if (temp == 2) {
            return 1;
        }
        if (temp == 1) {
            armed = 1;
        }
    }
    if (armed == 0) {
        temp2 = fn_80129E48(self);
        if (temp2 == 1) {
            return 0;
        }
        if (temp2 == 2) {
            return 1;
        }
    }
    if (armed == 0) {
        if ((u32)(kind - 2) <= 1U) {
            arg3 = 3;
        } else if (kind == 1) {
            arg3 = 0xA;
        } else if (kind == 5) {
            arg3 = 3;
        } else {
            arg3 = 0xFF;
        }
        if (fn_8012A014_view1(self, 0x1B, arg3, arg1, lbl_805A20C8, lbl_805A20D4) == 1) {
            return 1;
        }
    }
    if (fn_80129A70(self, arg1) == 1) {
        return 1;
    }
    return (fn_8012A204_view1(self) - 1) == 0;
}
}

#pragma peephole on

extern "C" {
/* 0x801545B8 (0x80) - fills the 0x18-byte spawn record and hands it to the effect queue. */
void fn_801545B8(EmSpawnRec* rec, u8 arg1, s16 arg2, s16 arg3) {
    nw4r::math::VEC3 pos;

    setVec3(&pos, lbl_80796E1C, lbl_80797098, lbl_80796E3C);
    rec->id = 0x1A;
    copyVec3(&rec->pos, &pos);
    rec->field_0x10 = arg1;
    rec->field_0x12 = arg2;
    rec->field_0x14 = arg3;
}
}

#pragma peephole off

extern "C" {
/* 0x80154638 (0x14C) - the range/height test: measure the target's position against the work
 * position through the joint helper. */
s32 fn_80154638(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 v;
    nw4r::math::VEC3 w;
    struct _ENEMY_WORK* target;
    f32 dist;
    f32 limit;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    target = fn_80131034_view1(self, 0x1B, 0);
    if (target == 0) {
        return 0;
    }
    if (fn_8012E5A8_c1(target) != 1) {
        return 0;
    }
    if (arg1 == 0) {
        return 1;
    }
    copyVec3(&b, setVec3(&v, lbl_80796E1C, lbl_80796E1C, lbl_80797074 * get_em_chg_scale(self)));
    rotVecY(&b, self->field_0x1C0);
    addVec3_view2(&w, &self->pos, &b);
    copyVec3(&a, &w);
    dist = calcDistanceSqXZ(&a, &target->pos);
    limit = lbl_8079709C * get_em_chg_scale(self);
    if (dist < (lbl_8079709C * get_em_chg_scale(self)) * limit) {
        return 1;
    }
    return 0;
}

/* 0x80154784 (0x28) - is the action one of the glide steps?  `u32`: `enemy/em030_prog.cpp`'s call
 * site compares it against 1 unsigned. */
u32 fn_80154784(struct _ENEMY_WORK* self) {
    if (self->action == 0xD && self->state_sub <= 4) {
        return 1;
    }
    return 0;
}

/* 0x801547AC (0x17C) - the per-kind attack test (`arg1` selects the kind). */
u8 fn_801547AC(struct _ENEMY_WORK* self, u8 arg1) {
    u8 kind;
    u8 area;

    switch (arg1) {
    case 0:
        return fn_80131034_view1(self, 0x1B, 1) != 0;
    case 1:
        return self->field_0x356 != 0;
    case 2:
        kind = stage_map_kind_get(self->field_0x1E0);
        switch (kind) {
        case 0:
            return 1;
        case 1:
            area = self->area_no;
            if (area == 3 || area == 5) {
                return 1;
            }
            return 0;
        case 5:
            area = self->area_no;
            if (area == 3 || area == 6 || area == 8) {
                return 1;
            }
            return 0;
        case 8:
            if (self->area_no == 1) {
                return 1;
            }
            return 0;
        case 9:
            if (self->area_no == 0) {
                return 1;
            }
            return 0;
        default:
            return 0;
        }
    case 3:
        return (u8)((em_magma_check_view1(self) - 1) == 0);
    case 4:
        if (self->field_0x1E2 == 1 && self->field_0x358 == 1) {
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

/* 0x80154928 (0x54) - steps the work position's y and re-syncs the model. */
void fn_80154928(struct _ENEMY_WORK* self, u8* arg1, u8* arg2) {
    *arg1 = 0xC;
    *arg2 = 0;
    self->pos.y = self->pos.y + lbl_80796E18;
    em_fall_height_get_view1(self);
    em_fall_start(self);
}

/* 0x8015497C (0xC) - clears the action's case-6 flag. */
void fn_8015497C(struct _ENEMY_WORK* self) {
    self->field_0x357 = 0;
}

/* 0x80154988 (0x17C) - the two-way rotation/scale step: the enemy's part state drives the four
 * animation angles toward or away from their limits. */
void fn_80154988(struct _ENEMY_WORK* self) {
    f32 v;

    if (fn_8012EC3C_c1(self) == 1) {
        self->rot_0x328.field_0x330 = self->rot_0x328.field_0x330 - 0x444;
        if ((s16)self->rot_0x328.field_0x330 < -0x2AAA) {
            self->rot_0x328.field_0x330 = 0xD556;
        }
        v = self->rot_0x328.field_0x33C - lbl_807970A0;
        self->rot_0x328.field_0x33C = v;
        if (v > lbl_807970A4) {
            self->rot_0x328.field_0x33C = lbl_807970A4;
        }
        v = self->rot_0x328.field_0x340 - lbl_80796F40;
        self->rot_0x328.field_0x340 = v;
        if (v < lbl_80796E1C) {
            self->rot_0x328.field_0x340 = lbl_80796E1C;
        }
        v = self->rot_0x328.field_0x348 - lbl_80797044;
        self->rot_0x328.field_0x348 = v;
        if (v < lbl_807970A8) {
            self->rot_0x328.field_0x348 = lbl_807970A8;
        }
        v = self->rot_0x328.field_0x34C - lbl_80796F40;
        self->rot_0x328.field_0x34C = v;
        if (v < lbl_80796E1C) {
            self->rot_0x328.field_0x34C = lbl_80796E1C;
        }
    } else {
        self->rot_0x328.field_0x330 = self->rot_0x328.field_0x330 + 0x444;
        if ((s16)self->rot_0x328.field_0x330 > 0) {
            self->rot_0x328.field_0x330 = 0;
        }
        v = self->rot_0x328.field_0x33C + lbl_807970A0;
        self->rot_0x328.field_0x33C = v;
        if (v < lbl_80796E08) {
            self->rot_0x328.field_0x33C = lbl_80796E08;
        }
        v = self->rot_0x328.field_0x340 + lbl_80796F40;
        self->rot_0x328.field_0x340 = v;
        if (v > lbl_80796E0C) {
            self->rot_0x328.field_0x340 = lbl_80796E0C;
        }
        v = self->rot_0x328.field_0x348 + lbl_80797044;
        self->rot_0x328.field_0x348 = v;
        if (v > lbl_80796E14) {
            self->rot_0x328.field_0x348 = lbl_80796E14;
        }
        v = self->rot_0x328.field_0x34C + lbl_80796F40;
        self->rot_0x328.field_0x34C = v;
        if (v > lbl_80796E0C) {
            self->rot_0x328.field_0x34C = lbl_80796E0C;
        }
    }
}

/* 0x80154B04 (0x170) - the aim/rotation step: derive the angle to the work position's stored
 * target and pick the nearest rotation the effect can take. */
void fn_80154B04(struct _ENEMY_WORK* self, nw4r::math::VEC3* out, u16* angleOut, f32 farg0) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 v;
    nw4r::math::VEC3 w;
    u16 angle;
    u16 delta;
    f32 scale;
    u16 base;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    copyVec3(&a, &self->vec_0x36C);
    angle = (u16)calcVecAng2(&self->pos, &a);
    *angleOut = angle;
    delta = (u16)(angle - self->field_0x1C0);
    scale = farg0 * get_em_chg_scale(self);
    if ((u16)(delta + 0xBFFF) > 0x7FFE) {
        setVector3(&b, lbl_80796E1C, lbl_80796E1C, fn_80050EF4_view1(&a, &self->pos) - scale);
        rotVecY(&b, *angleOut);
        addVec3_view2(&v, &self->pos, &b);
        copyVec3(out, &v);
        return;
    }
    base = 0xC000;
    if (delta < (u16)-0x8000) {
        base = 0x4000;
    }
    *angleOut = (u16)(base + self->field_0x1C0);
    setVector3(&b, lbl_80796E1C, lbl_80796E1C, -scale);
    rotVecY(&b, *angleOut);
    addVec3_view2(&w, &a, &b);
    copyVec3(out, &w);
}

/* 0x80154C74 (0x30) - the team/area sound selector. */
void fn_80154C74(struct _ENEMY_WORK* self) {
    if (self->team == 2) {
        if (self->field_0x1E2 == 3) {
            fn_80130CDC_view1(-7);
            return;
        }
        fn_80130CDC_view1(-0xE);
        return;
    }
    fn_80130CDC_view1(-0xE);
}

/* 0x80154CA4 (0x44) - clamps the work position's height. */
void fn_80154CA4(struct _ENEMY_WORK* self) {
    fn_801303FC_view1(lbl_807970AC);
    if (self->field_0x1AC < lbl_807970B0) {
        self->field_0x1AC = lbl_807970B0;
    }
}

/* 0x80154CE8 (0x5C) - releases the 0x0C-byte helper when the action closes. */
s32 fn_80154CE8(s32 self, s16 flag) {
    if (self != 0) {
        fn_8013918C_view1(0, 0);
        if (flag > 0) {
            operator delete((void*)self);
        }
    }
    return self;
}

/* 0x80154D44 (0xFC) - seeds the six 0xC-byte animation vectors of the three global records. */
void fn_80154D44(void) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 c;
    nw4r::math::VEC3 d;
    nw4r::math::VEC3 e;
    nw4r::math::VEC3 f;

    assignVec3_view1(vec_pair_801502C8_0, (s32)setVec3(&a, lbl_80796E1C, lbl_80796F84, lbl_80796E1C));
    assignVec3_view1(&vec_pair_801502C8_0[1], (s32)setVec3(&b, lbl_80796E1C, lbl_807970B4, lbl_80796E1C));
    assignVec3_view1(vec_pair_801502C8_1, (s32)setVec3(&c, lbl_80796E1C, lbl_80796F84, lbl_80796E1C));
    assignVec3_view1(&vec_pair_801502C8_1[1], (s32)setVec3(&d, lbl_80796E1C, lbl_807970B4, lbl_80796E1C));
    assignVec3_view1(vec_pair_801502C8_2, (s32)setVec3(&e, lbl_80796E1C, lbl_80796E58, lbl_80796E1C));
    assignVec3_view1(&vec_pair_801502C8_2[1], (s32)setVec3(&f, lbl_80796E1C, lbl_807970B8, lbl_80796E1C));
}
}

/* The unit's `.bss`: the three two-vector records `fn_80154D44` seeds.  Names are GUESSes (each record
 * is a pair of model-space points). */
VEC3 vec_pair_801502C8_0[2];  /* +0x806A77D8 */
VEC3 vec_pair_801502C8_1[2];  /* +0x806A77F0 */
VEC3 vec_pair_801502C8_2[2];  /* +0x806A7808 */
